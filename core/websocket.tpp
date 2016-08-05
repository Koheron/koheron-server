/// Websocket template implementation
///
/// (c) Koheron

#include "websocket.hpp"
#include "kserver.hpp"

namespace kserver {

template<class T>
int WebSocket::send(const T *data, unsigned int len)
{
    long unsigned int char_data_len = len * sizeof(T) / sizeof(char);

    if (char_data_len + 10 > WEBSOCK_SEND_BUF_LEN) {
        kserver->syslog.print(SysLog::ERROR,
                              "WebSocket: send_buf too small\n");
        return -1;
    }

    int mask_offset = set_send_header(send_buf, char_data_len, BINARY);
    memcpy(&send_buf[mask_offset], data, char_data_len);
    return send_request(send_buf, mask_offset + char_data_len);
}

}