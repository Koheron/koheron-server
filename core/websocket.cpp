/// Implementation of websocket.hpp
///
/// Inspired by https://github.com/MarkusPfundstein/C---Websocket-Server
///
/// (c) Koheron

#include "websocket.hpp"

#include <cstring>
#include <sstream>
#include <iostream>
#include <cstdlib>

extern "C" {
    #include <sys/socket.h>	// socket definitions
    #include <sys/types.h>	// socket types      
    #include <arpa/inet.h>	// inet (3) funtions
    #include <unistd.h>
}

#include "crypto/base64.hpp"
#include "crypto/sha1.h"
#include "kserver.hpp"

namespace kserver {

WebSocket::WebSocket(std::shared_ptr<KServerConfig> config_, KServer *kserver_)
: config(config_),
  kserver(kserver_),
  comm_fd(-1),
  read_str_len(0),
  connection_closed(false)
{
    bzero(read_str, WEBSOCK_READ_STR_LEN);
    bzero(sha_str, 21);
}

void WebSocket::set_id(int comm_fd_)
{
    comm_fd = comm_fd_;
}

int WebSocket::authenticate()
{
    if (read_http_packet() < 0)
        return -1;

    static const std::string WSKeyIdentifier("Sec-WebSocket-Key: ");
    static const std::string WSProtocolIdentifier("Sec-WebSocket-Protocol: ");
    static const std::string WSMagic("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

    bool is_protocol 
        = (http_packet.find(WSProtocolIdentifier) != std::string::npos);

    std::size_t pos = http_packet.find(WSKeyIdentifier);

    if (pos == std::string::npos) {
        kserver->syslog.print<SysLog::CRITICAL>("WebSocket: No WebSocket Key");
        return -1;
    }

    std::string key;

    try {
        static const int WSKeyLen = 24;
        key = http_packet.substr(pos + WSKeyIdentifier.length(), WSKeyLen);
    } catch (std::out_of_range &err) {
        return -1;
    }

    key.append(WSMagic);

    SHA1(reinterpret_cast<const unsigned char*>(key.c_str()),
         key.length(), sha_str);
    std::string final_str = base64_encode(sha_str, 20);

    std::ostringstream oss;
    oss << "HTTP/1.1 101 Switching Protocols\r\n";
    oss << "Upgrade: websocket\r\n";
    oss << "Connection: Upgrade\r\n";
    oss << "Sec-WebSocket-Accept: " << final_str << "\r\n";

    // Chrome is not happy when the Protocol wasn't
    // specified in the HTTP request:
    // "Response must not include 'Sec-WebSocket-Protocol'
    // header if not present in request: chat"
    // so only answer protocol if requested
    if (is_protocol)
        oss << "Sec-WebSocket-Protocol: chat\r\n";

    oss << "\r\n";

    return send_request(oss.str());
}

int WebSocket::read_http_packet()
{
    reset_read_buff();

    int nb_bytes_rcvd = read(comm_fd, read_str, WEBSOCK_READ_STR_LEN);

    // Check reception ...
    if (nb_bytes_rcvd < 0) {
        kserver->syslog.print<SysLog::CRITICAL>("WebSocket: Read error\n");
        return -1;
    }

    if (nb_bytes_rcvd == KSERVER_READ_STR_LEN) {
        kserver->syslog.print<SysLog::CRITICAL>(
                              "WebSocket: Read buffer overflow\n");
        return -1;
    }

    if (nb_bytes_rcvd == 0) { // Connection closed by client
        connection_closed = true;
        return -1;
    }

    http_packet = std::string(read_str);
    std::size_t delim_pos = http_packet.find("\r\n\r\n");

    if (delim_pos == std::string::npos) {
        kserver->syslog.print<SysLog::CRITICAL>(
                              "WebSocket: No HTML header found\n");
        return -1;
    }

    if (config->verbose)
        kserver->syslog.print_dbg("[R] HTTP header\n");

    return nb_bytes_rcvd;
}

int WebSocket::set_send_header(unsigned char *bits, long long data_len,
                               unsigned int format)
{
    memset(bits, 0, 10 + data_len);

    bits[0] = format;
    int mask_offset = 0;

    if (data_len <= SMALL_STREAM) {
        bits[1] = data_len;
        mask_offset = SMALL_OFFSET;
    }
    else if (data_len > SMALL_STREAM
             && data_len <= 0xFFFF) {
        bits[1] = MEDIUM_STREAM;
        bits[2] = (data_len >> 8) & 0xFF;
        bits[3] = data_len & 0xFF;
        mask_offset = MEDIUM_OFFSET;
    } else {
        bits[1] = BIG_STREAM;
        bits[2] = (data_len >> 56) & 0xFF;
        bits[3] = (data_len >> 48) & 0xFF;
        bits[4] = (data_len >> 40) & 0xFF;
        bits[5] = (data_len >> 32) & 0xFF;
        bits[6] = (data_len >> 24) & 0xFF;
        bits[7] = (data_len >> 16) & 0xFF;
        bits[8] = (data_len >>  8) & 0xFF;
        bits[9] = data_len & 0xFF;
        mask_offset = BIG_OFFSET;
    }

    return mask_offset;
}

int WebSocket::send_cstr(const char *string)
{
    long unsigned int char_data_len = strlen(string);

    if (char_data_len + 10 > WEBSOCK_SEND_BUF_LEN) {
        kserver->syslog.print<SysLog::ERROR>(
                              "WebSocket: send_buf too small\n");
        return -1;
    }

    int mask_offset = set_send_header(send_buf, char_data_len, (1 << 7) + TEXT_FRAME);
    memcpy(&send_buf[mask_offset], string, char_data_len);
    return send_request(send_buf, mask_offset + char_data_len);
}

int WebSocket::exit()
{
    return send_request(send_buf, set_send_header(send_buf, 0, (1 << 7) + CONNECTION_CLOSE));
}

#define WEBSOCK_RCV(type, arg_type, arg_name)                                  \
int WebSocket::receive##type(arg_type arg_name)                                \
{                                                                              \
    if (connection_closed)                                                     \
        return 0;                                                              \
                                                                               \
    int err = read_stream();                                                   \
                                                                               \
    if (unlikely(err < 0))                                                     \
        return -1;                                                             \
    else if (err == 1) /* Connection closed by client*/                        \
        return 0;                                                              \
                                                                               \
    if (unlikely(decode_raw_stream##type(arg_name) < 0)) {                     \
        kserver->syslog.print<SysLog::CRITICAL>(                               \
                        "WebSocket: Cannot decode command stream\n");          \
        return -1;                                                             \
    }                                                                          \
                                                                               \
    if (config->verbose)                                                       \
        kserver->syslog.print_dbg(                                             \
                "[R] WebSocket: command of %u bytes\n", header.payload_size);  \
                                                                               \
    return header.payload_size;                                                \
}

WEBSOCK_RCV(_cmd, Command&, cmd) // receive_cmd
WEBSOCK_RCV(,,)                      // receive

int WebSocket::decode_raw_stream_cmd(Command& cmd)
{
    if (unlikely(read_str_len < (int)header.header_size + 1))
        return -1;

    char *mask = read_str + header.mask_offset;
    char *payload_ptr = read_str + header.mask_offset + 4;

    for (unsigned long long i = 0; i < HEADER_SIZE; ++i)
        cmd.header.data[i] = (payload_ptr[i] ^ mask[i % 4]);

    for (unsigned long long i = HEADER_SIZE; i < header.payload_size; ++i)
        cmd.buffer.data[i - HEADER_SIZE] = (payload_ptr[i] ^ mask[i % 4]);

    return 0;
}

int WebSocket::decode_raw_stream()
{
    if (unlikely(read_str_len < (int)header.header_size + 1))
        return -1;

    char *mask = read_str + header.mask_offset;
    payload = read_str + header.mask_offset + 4;

    for (unsigned long long i = 0; i < header.payload_size; ++i)
        payload[i] = (payload[i] ^ mask[i % 4]);

    return 0;
}

int WebSocket::read_stream()
{
    reset_read_buff();

    int read_head_err = read_header();

    if (unlikely(read_head_err < 0)) {
        kserver->syslog.print<SysLog::CRITICAL>(
                              "WebSocket: Cannot read header\n");
        return -1;
    }

    if (read_head_err == 1) // Connection closed
        return read_head_err;

    // Read payload
    int err = read_n_bytes(header.payload_size, header.payload_size);

    if (unlikely(err < 0)) {
        kserver->syslog.print<SysLog::CRITICAL>(
                              "WebSocket: Cannot read payload\n");
        return -1;
    }

    return err;
}

int WebSocket::check_opcode(unsigned int opcode)
{
    switch (opcode) {
      case CONTINUATION_FRAME:
        kserver->syslog.print<SysLog::CRITICAL>(
                    "WebSocket: Continuation frame is not suported\n");
        return -1;
      case TEXT_FRAME:
        break;
      case BINARY_FRAME:
        break;
      case CONNECTION_CLOSE:
        kserver->syslog.print<SysLog::INFO>(
                    "WebSocket: Connection close\n");
        return 1;
      case PING:
        kserver->syslog.print<SysLog::CRITICAL>(
                              "WebSocket: Ping is not suported\n");
        return -1;
      case PONG:

        // TODO If not sollicited by a Ping,
        // we should just ignore an unwanted Pong
        // instead of returning an error.

        kserver->syslog.print<SysLog::CRITICAL>(
                              "WebSocket: Pong is not suported\n");
        return -1;
      default:
        kserver->syslog.print<SysLog::CRITICAL>(
                              "WebSocket: Invalid opcode %u\n", opcode);
        return -1;
    }

    return 0;
}

int WebSocket::read_header()
{
    if (read_n_bytes(6,6) < 0)
        return -1;

    if (connection_closed)
        return 0;

//    printf("%s\n", read_str);

    header.fin = read_str[0] & 0x80;

    header.masked = read_str[1] & 0x80;
    unsigned char stream_size = read_str[1] & 0x7F;

    header.opcode = read_str[0] & 0x0F;

    int opcode_err = check_opcode(header.opcode);

    if (unlikely(opcode_err < 0)) {
        return -1;
    } else if (opcode_err == 1) {
        connection_closed = true;
        return opcode_err;
    }

    if (stream_size <= SMALL_STREAM) {
        header.header_size = SMALL_HEADER;
        header.payload_size = stream_size;
        header.mask_offset = SMALL_OFFSET;
    }
    else if (stream_size == MEDIUM_STREAM) {
        if (read_n_bytes(2, 2) < 0)
            return -1;

        if (connection_closed)
            return 0;

        header.header_size = MEDIUM_HEADER;
        unsigned short s = 0;
        memcpy(&s, (const char*)&read_str[2], 2);
        header.payload_size = ntohs(s);
        header.mask_offset = MEDIUM_OFFSET;
    }
    else if (stream_size == BIG_STREAM) {
        if (read_n_bytes(8, 8) < 0)
            return -1;

        if (connection_closed)
            return 0;

        header.header_size = BIG_HEADER;
        unsigned long long l = 0;
        memcpy(&l, (const char*)&read_str[2], 8);

        header.payload_size = be64toh(l);
        header.mask_offset = BIG_OFFSET;
    } else {
        kserver->syslog.print<SysLog::CRITICAL>(
                              "WebSocket: Couldn't decode stream size\n");
        return -1;
    }

    if (unlikely(header.payload_size > WEBSOCK_READ_STR_LEN - 56)) {
        kserver->syslog.print<SysLog::CRITICAL>(
                              "WebSocket: Message too large\n");
        return -1;
    }

    return 0;
}

int WebSocket::read_n_bytes(int bytes, int expected)
{
    int remaining = bytes;
    int bytes_read = -1;

    while (expected > 0) {
        while ((remaining > 0) && 
               ((bytes_read 
                   = read(comm_fd, &read_str[read_str_len], remaining)) > 0)) {

            if (bytes_read > 0) {
                read_str_len += bytes_read;
                remaining -= bytes_read;
                expected -= bytes_read;	
            }

            if (expected < 0)
                expected = 0;
        }

        if (bytes_read == 0) {
            kserver->syslog.print<SysLog::INFO>(
                              "WebSocket: Connection closed by client\n");
            connection_closed = true;
            return 1;
        }

        if (unlikely(read_str_len == KSERVER_READ_STR_LEN)) {
            kserver->syslog.print<SysLog::CRITICAL>(
                                  "WebSocket: Read buffer overflow\n");
            return -1;
        }
    }

    return 0;
}

int WebSocket::send_request(const std::string& request)
{
    return send_request(reinterpret_cast<const unsigned char*>(request.c_str()),
                        request.length());
}

int WebSocket::send_request(const unsigned char *bits, long long len)
{
    int bytes_send = 0;
    int remaining = len;
    int offset = 0;

    while ((remaining > 0) && 
           (bytes_send = write(comm_fd, &bits[offset], remaining)) > 0) {
        if (bytes_send > 0) {
            offset += bytes_send;
            remaining -= bytes_send;
        }
        else if (bytes_send == 0) {
            kserver->syslog.print<SysLog::ERROR>(
                                  "WebSocket: Cannot send request\n");
            return -1;
        }
    }

    if (unlikely(bytes_send < 0)) {
        kserver->syslog.print<SysLog::ERROR>(
                              "WebSocket: Cannot send request. Error #%u\n",
                              bytes_send);
        return -1;
    }

    if (config->verbose)
        kserver->syslog.print_dbg("[S] %i bytes\n", bytes_send);

    return bytes_send;
}

void WebSocket::reset_read_buff()
{
    bzero(read_str, read_str_len);
    read_str_len = 0;
}

} // namespace kserver
