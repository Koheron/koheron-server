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
    // Set buffers
    bzero(read_str, WEBSOCK_READ_STR_LEN);
    bzero(payload, WEBSOCK_READ_STR_LEN);
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
        kserver->syslog.print(SysLog::CRITICAL, "WebSocket: No WebSocket Key");
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
        kserver->syslog.print(SysLog::CRITICAL, "WebSocket: Read error\n");
        return -1;
    }

    if (nb_bytes_rcvd == KSERVER_READ_STR_LEN) {
        kserver->syslog.print(SysLog::CRITICAL, 
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
        kserver->syslog.print(SysLog::CRITICAL, 
                              "WebSocket: No HTML header found\n");
        return -1;
    }

    kserver->syslog.print(SysLog::DEBUG, "[R] HTTP header\n");

    return nb_bytes_rcvd;
}

int WebSocket::set_send_header(unsigned char *bits, long long data_len,
                               WS_SendFormat_t format)
{
    memset(bits, 0, 10 + data_len);

    bits[0] = format;
    int mask_offset = 0;

    if (data_len <= WebSocketStreamSize::SmallStream) {
        bits[1] = data_len;
        mask_offset = WebSocketMaskOffset::SmallOffset;
    }
    else if (data_len > WebSocketStreamSize::SmallStream 
             && data_len <= 65535) {
        bits[1] = WebSocketStreamSize::MediumStream;
        bits[2] = (data_len >> 8) & 255;
        bits[3] = data_len & 255;
        mask_offset = WebSocketMaskOffset::MediumOffset;
    } else {
        bits[1] = WebSocketStreamSize::BigStream;
        bits[2] = (data_len >> 56) & 255;
        bits[3] = (data_len >> 48) & 255;
        bits[4] = (data_len >> 40) & 255;
        bits[5] = (data_len >> 32) & 255;
        bits[6] = (data_len >> 24) & 255;
        bits[7] = (data_len >> 16) & 255;
        bits[8] = (data_len >> 8) & 255;
        bits[9] = data_len & 255;
        mask_offset = WebSocketMaskOffset::BigOffset;
    }

    return mask_offset;
}

int WebSocket::send(const std::string& stream)
{
    long long data_len = stream.length();
    unsigned char *bits = new unsigned char[10 + data_len];

    int mask_offset = set_send_header(bits, data_len, TEXT);

    // XXX This copy is not very nice. 
    // It might produce some significant time for large buffers.
    // However not easy to concatenate bits with the buffer without copy:
    // http://stackoverflow.com/questions/541634/concatenating-two-memory-buffers-without-memcpy
    //
    // Maybe it would be better to copy the template header instead
    // of the buffer not easy nothing guarantees that we can allocate
    // the space before the buffer.
    std::copy(stream.begin(), stream.end(), &bits[mask_offset]);

    int err = send_request(bits, mask_offset + data_len);
    delete [] bits;
    return err;
}

int WebSocket::receive()
{
    if (connection_closed)
        return 0;

    int err = read_stream();

    if (err < 0)
        return -1;
    else if (err == 1) // Connection closed by client
        return 0;

    if (decode_raw_stream() < 0) {
        kserver->syslog.print(SysLog::CRITICAL, 
                              "WebSocket: Cannot decode stream\n");
        return -1;
    }

    kserver->syslog.print(SysLog::DEBUG, 
                          "[R] WebSocket: %u bytes\n", header.payload_size);

    return header.payload_size;
}

int WebSocket::decode_raw_stream()
{
    if (read_str_len < (int)header.header_size + 1)
        return -1;

    char masks[4];
    memcpy(masks, read_str+header.mask_offset, 4);
    memcpy(payload, read_str+header.mask_offset + 4, header.payload_size);

    for (unsigned long long i = 0; i < header.payload_size; ++i)
        payload[i] = (payload[i] ^ masks[i % 4]);

    payload[header.payload_size] = '\0';
    return 0;
}

int WebSocket::read_stream()
{
    reset_read_buff();

    if (read_header() < 0) {
        kserver->syslog.print(SysLog::CRITICAL,
                              "WebSocket: Cannot read header\n");
        return -1;
    }

    // Read payload
    int err = read_n_bytes(header.payload_size, header.payload_size);

    if (err < 0) {
        kserver->syslog.print(SysLog::CRITICAL,
                              "WebSocket: Cannot read payload\n");
        return -1;
    }

    return err;
}

int WebSocket::check_opcode(unsigned int opcode)
{
    switch (opcode) {
      case WebSocketOpCode::ContinuationFrame:
        kserver->syslog.print(SysLog::CRITICAL,
                    "WebSocket: Continuation frame is not suported\n");
        return -1;
      case WebSocketOpCode::TextFrame:
        break;
      case WebSocketOpCode::BinaryFrame:
        break;
      case ConnectionClose:
        kserver->syslog.print(SysLog::INFO,
                    "WebSocket: Connection close\n");
        return 1;
      case WebSocketOpCode::Ping:
        kserver->syslog.print(SysLog::CRITICAL,
                              "WebSocket: Ping is not suported\n");
        return -1;
      case WebSocketOpCode::Pong:

        // TODO If not sollicited by a Ping,
        // we should just ignore an unwanted Pong
        // instead of returning an error.

        kserver->syslog.print(SysLog::CRITICAL,
                              "WebSocket: Pong is not suported\n");
        return -1;
      default:
        kserver->syslog.print(SysLog::CRITICAL, "WebSocket: Invalid opcode %u\n", opcode);
        return -1;
    }

    return 0;
}

int WebSocket::read_header()
{    
    if (read_n_bytes(6,6) < 0)
        return -1;

//    printf("%s\n", read_str);

    header.fin = read_str[0] & 0x80;

    header.masked = read_str[1] & 0x80;
    unsigned char stream_size = read_str[1] & 0x7F;

    header.opcode = read_str[0] & 0x0F;

    int opcode_err = check_opcode(header.opcode);

    if (opcode_err < 0) {
        return -1;
    } else if (opcode_err == 1) {
        connection_closed = true;
        return 0;
    }

    if (stream_size <= WebSocketStreamSize::SmallStream) {
        header.header_size = WebSocketHeaderSize::SmallHeader;
        header.payload_size = stream_size;
        header.mask_offset = WebSocketMaskOffset::SmallOffset;
    }
    else if (stream_size == WebSocketStreamSize::MediumStream) {
        if (read_n_bytes(2, 2) < 0)
            return -1;

        header.header_size = WebSocketHeaderSize::MediumHeader;
        unsigned short s = 0;
        memcpy(&s, (const char*)&read_str[2], 2);
        header.payload_size = ntohs(s);
        header.mask_offset = WebSocketMaskOffset::MediumOffset;
    }
    else if (stream_size == WebSocketStreamSize::BigStream) {
        if (read_n_bytes(8, 8) < 0)
            return -1;
        
        header.header_size = WebSocketHeaderSize::BigHeader;
        unsigned long long l = 0;
        memcpy(&l, (const char*)&read_str[2], 8);
			
        header.payload_size = be64toh(l);
        header.mask_offset = WebSocketMaskOffset::BigOffset;
    } else {
        kserver->syslog.print(SysLog::CRITICAL,
                              "WebSocket: Couldn't decode stream size\n");
        return -1;
    }

    if (header.payload_size > WEBSOCK_READ_STR_LEN - 56) {
        kserver->syslog.print(SysLog::CRITICAL,
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
            kserver->syslog.print(SysLog::INFO,
                              "WebSocket: Connection closed by client\n");
            connection_closed = true;
            return 1;
        }

        if (read_str_len == KSERVER_READ_STR_LEN) {
            kserver->syslog.print(SysLog::CRITICAL, 
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
            kserver->syslog.print(SysLog::ERROR,
                                  "WebSocket: Cannot send request\n");
            return -1;
        }
    }

    if (bytes_send < 0) {
        kserver->syslog.print(SysLog::ERROR,
                              "WebSocket: Cannot send request. Error #%u\n",
                              bytes_send);
        return -1;
    }

    kserver->syslog.print(SysLog::DEBUG, "[S] %i bytes\n", bytes_send);

    return bytes_send;
}

void WebSocket::reset_read_buff()
{
    bzero(read_str, WEBSOCK_READ_STR_LEN);
    read_str_len = 0;
}

int WebSocket::get_payload(char *payload_, unsigned int size)
{
    if (size <= header.payload_size) {
        kserver->syslog.print(SysLog::CRITICAL, "Buffer overflow\n");
        return -1;
    }

    memcpy(payload_, (const char*)&payload, header.payload_size);
    return header.payload_size;
}

} // namespace kserver
