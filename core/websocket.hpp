/// Websocket protocol interface
///
/// (c) Koheron

#ifndef __WEBSOCKET_HPP__
#define __WEBSOCKET_HPP__

#include <string>
#include <memory>

#include "kserver_defs.hpp"
#include "config.hpp"

namespace kserver {

struct WebSocketStreamHeader {
    unsigned int header_size;
    int mask_offset;
    unsigned int payload_size;
    bool fin;
    bool masked;
    unsigned char opcode;
    unsigned char res[3];
};

typedef enum WebSocketSendFormat {
    TEXT = 129,
    BINARY = 130
} WS_SendFormat_t; 
	
enum WebSocketOpCode {
    ContinuationFrame = 0x0,
    TextFrame         = 0x1,
    BinaryFrame       = 0x2,
    ConnectionClose   = 0x8,
    Ping              = 0x9,
    Pong              = 0xA
};

enum WebSocketStreamSize {
    SmallStream = 125,
    MediumStream = 126,
    BigStream = 127
};

enum WebSocketHeaderSize {
    SmallHeader = 6,
    MediumHeader = 8,
    BigHeader = 14
};

enum WebSocketMaskOffset {
    SmallOffset = 2,
    MediumOffset = 4,
    BigOffset = 10
};

class KServer;

class WebSocket
{
  public:
    WebSocket(std::shared_ptr<KServerConfig> config_, KServer *kserver_);
    
    void set_id(int comm_fd_);
    
    int authenticate();
    
    int receive();
    
    int send(const std::string& stream);
    
    template<class T>
    int send(const T *data, unsigned int len);

    char *get_payload_no_copy() {return payload;}
    unsigned int payload_size() const {return header.payload_size;}
    
    /// Copy the received payload
    /// @payload_ Buffer where the payload must be copied
    /// @size Allocated size of @payload_ for sanity check
    int get_payload(char *payload_, unsigned int size);
    
    bool is_closed() const {return connection_closed;}
    
  private:
    std::shared_ptr<KServerConfig> config;
    KServer *kserver;
    
    int comm_fd;
    
    // Buffers
    int read_str_len;
    char read_str[WEBSOCK_READ_STR_LEN];
    char payload[WEBSOCK_READ_STR_LEN];
    unsigned char sha_str[21];
    
    void reset_read_buff();
    
    std::string http_packet;
    WebSocketStreamHeader header;
    bool connection_closed;
    
    // Internal functions
    int read_http_packet();
    int decode_raw_stream();
    int read_stream();
    int read_header();
    int check_opcode(unsigned int opcode);
    int read_n_bytes(int bytes, int expected);
    
    int set_send_header(unsigned char *bits, long long data_len,
                        WS_SendFormat_t format);
    int send_request(const std::string& request);
    int send_request(const unsigned char *bits, long long len);
};

template<class T>
int WebSocket::send(const T *data, unsigned int len)
{
    long unsigned int char_data_len = len*sizeof(T)/sizeof(char);

    unsigned char *bits = new unsigned char[10 + char_data_len];
    int mask_offset = set_send_header(bits, char_data_len, BINARY);
    
    memcpy(&bits[mask_offset], data, char_data_len);

    int err = send_request(bits, mask_offset + char_data_len);
    delete [] bits;

    return err;
}

} // namespace kserver

#endif // __WEBSOCKET_HPP__
