/// (c) Koheron

#ifndef __KSERVER_SESSION_HPP__
#define __KSERVER_SESSION_HPP__

#include <string>
#include <ctime>
#include <vector>
#include <array>
#include <memory>
#include <unistd.h>
#include <type_traits>

#include "commands.hpp"
#include "serializer_deserializer.hpp"
#include "socket_interface_defs.hpp"
#include "kserver.hpp"
#include "websocket.hpp"

namespace kserver {

class SessionManager;

class SessionAbstract
{
  public:
    SessionAbstract(int sock_type_)
    : kind(sock_type_) {}

    template<typename... Tp> std::tuple<int, Tp...> deserialize(Command& cmd);
    template<typename Tp> int recv(Tp& container, Command& cmd);
    template<uint16_t class_id, uint16_t func_id, typename... Args> int send(Args&&... args);

    int kind;
};

/// Session
///
/// Receive and parse the client request for execution.
///
/// By calling the appropriate socket interface, it offers
/// an abstract communication layer to the devices. Thus
/// shielding them from the underlying communication protocol.
template<int sock_type>
class Session : public SessionAbstract
{
  public:
    Session(const std::shared_ptr<KServerConfig>& config_,
            int comm_fd, SessID id_,
            SessionManager& session_manager_);

    int run();

    unsigned int request_num() const {return requests_num;}
    unsigned int error_num() const {return errors_num;}
    SessID get_id() const {return id;}

    // Receive - Send

    // TODO Move in Session<TCP> specialization
    int rcv_n_bytes(char *buffer, uint64_t n_bytes);

    template<typename... Tp> std::tuple<int, Tp...> deserialize(Command& cmd, std::false_type);
    template<typename... Tp> std::tuple<int, Tp...> deserialize(Command& cmd, std::true_type);

    // XXX Error when removing this !
    template<typename T, size_t N>
    std::tuple<int, const std::array<T, N>&> extract_array(Command& cmd);

    // The command is passed in argument since for the WebSocket the vector data
    // are stored into it. This implies that the whole vector is already stored on
    // the stack which might not be a good thing.
    //
    // TCP sockets won't use it as it reads directly the TCP buffer.
    template<typename Tp>
    int recv(Tp& container, Command& cmd);

    template<typename T, size_t N>
    int recv(std::array<T, N>& arr, Command& cmd);

    template<typename T>
    int recv(std::vector<T>& vec, Command& cmd);

    template<uint16_t class_id, uint16_t func_id, typename... Args>
    int send(Args&&... args) {
        dyn_ser.build_command<class_id, func_id>(send_buffer, std::forward<Args>(args)...);
        const auto bytes_send = write(send_buffer.data(), send_buffer.size());

        if (bytes_send == 0)
            status = CLOSED;

        return bytes_send;
    }

  private:
    std::shared_ptr<KServerConfig> config;
    int comm_fd;  ///< Socket file descriptor
    SessID id;
    SessionManager& session_manager;

    struct EmptyBuffer {};
    std::conditional_t<sock_type == TCP || sock_type == UNIX,
            Buffer<KSERVER_RECV_DATA_BUFF_LEN>, EmptyBuffer> recv_data_buff;

    struct EmptyWebsock {
        EmptyWebsock(std::shared_ptr<KServerConfig> config_) {}
    };

    std::conditional_t<sock_type == WEBSOCK, WebSocket, EmptyWebsock> websock;

    // Monitoring
    unsigned int requests_num; ///< Number of requests received during the current session
    unsigned int errors_num;   ///< Number of requests errors during the current session

    std::vector<unsigned char> send_buffer;
    DynamicSerializer<1024> dyn_ser;

    enum {CLOSED, OPENED};
    int status;

  private:
    int init_socket();
    int exit_socket();

    int init_session() {
        errors_num = 0;
        requests_num = 0;
        return init_socket();
    }

    void exit_session() {
    }

    int read_command(Command& cmd);

    int64_t get_pack_length() {
        Buffer<sizeof(uint64_t)> buff;
        const auto err = rcv_n_bytes(buff.data(), sizeof(uint32_t));

        if (err < 0) {
            return -1;
        }

        return std::get<0>(buff.deserialize<uint32_t>());
    }

    template<class T> int write(const T *data, unsigned int len);

friend class SessionManager;
};

template<int sock_type>
Session<sock_type>::Session(const std::shared_ptr<KServerConfig>& config_,
                            int comm_fd_, SessID id_,
                            SessionManager& session_manager_)
: SessionAbstract(sock_type)
, config(config_)
, comm_fd(comm_fd_)
, id(id_)
, session_manager(session_manager_)
, websock(config_)
, requests_num(0)
, errors_num(0)
, send_buffer(0)
, status(OPENED)
{}

template<int sock_type>
int Session<sock_type>::run()
{
    if (init_session() < 0)
        return -1;

    while (!session_manager.kserver.exit_comm.load()) {
        Command cmd;
        const int nb_bytes_rcvd = read_command(cmd);

        if (session_manager.kserver.exit_comm.load())
            break;

        if (nb_bytes_rcvd <= 0) {

            // We don't call exit_session() here because the
            // socket is already closed.

            return nb_bytes_rcvd;
        }

        requests_num++;

        if (unlikely(session_manager.dev_manager.execute(cmd) < 0)) {
            errors_num++;
        }

        if (status == CLOSED)
            break;
    }

    exit_session();
    return 0;
}

// -----------------------------------------------
// TCP
// -----------------------------------------------

template<>
int Session<TCP>::rcv_n_bytes(char *buffer, uint64_t n_bytes);

template<>
template<typename T, size_t N>
inline int Session<TCP>::recv(std::array<T, N>& arr, Command& cmd)
{
    return rcv_n_bytes(reinterpret_cast<char*>(arr.data()), size_of<T, N>);
}

template<>
template<typename T>
inline int Session<TCP>::recv(std::vector<T>& vec, Command& cmd)
{
    const auto length = get_pack_length() / sizeof(T);

    if (length < 0)
        return -1;

    vec.resize(length);
    const auto err = rcv_n_bytes(reinterpret_cast<char *>(vec.data()), length * sizeof(T));

    return err;
}

template<>
template<>
inline int Session<TCP>::recv(std::string& str, Command& cmd)
{
    const auto length = get_pack_length();

    if (length < 0)
        return -1;

    str.resize(length);
    const auto err = rcv_n_bytes(const_cast<char*>(str.data()), length);

    return err;
}

template<>
template<typename... Tp>
inline std::tuple<int, Tp...> Session<TCP>::deserialize(Command& cmd, std::false_type)
{
    return std::make_tuple(0);
}

template<>
template<typename... Tp>
inline std::tuple<int, Tp...> Session<TCP>::deserialize(Command& cmd, std::true_type)
{
    constexpr auto pack_len = required_buffer_size<Tp...>();
    Buffer<pack_len> buff;
    const int err = rcv_n_bytes(buff.data(), pack_len);
    return std::tuple_cat(std::make_tuple(err), buff.deserialize<Tp...>());
}

template<>
template<class T>
inline int Session<TCP>::write(const T *data, unsigned int len)
{
    const int bytes_send = sizeof(T) * len;
    const int n_bytes_send = ::write(comm_fd, (void*)data, bytes_send);

    if (n_bytes_send == 0) {
       return 0;
    }

    if (unlikely(n_bytes_send < 0)) {
       return -1;
    }

    if (unlikely(n_bytes_send != bytes_send)) {
        return -1;
    }

    return bytes_send;
}


// -----------------------------------------------
// Unix socket
// -----------------------------------------------

// Unix socket has the same interface than TCP socket
template<>
class Session<UNIX> : public Session<TCP>
{
  public:
    Session<UNIX>(const std::shared_ptr<KServerConfig>& config_,
                  int comm_fd_, SessID id_,
                  SessionManager& session_manager_)
    : Session<TCP>(config_, comm_fd_, id_, session_manager_) {}
};

// -----------------------------------------------
// WebSocket
// -----------------------------------------------

template<>
template<typename T, size_t N>
inline int Session<WEBSOCK>::recv(std::array<T, N>& arr, Command& cmd)
{
    arr = cmd.payload.extract_array<T, N>();
    return 0;
}

template<>
template<typename T>
inline int Session<WEBSOCK>::recv(std::vector<T>& vec, Command& cmd)
{
    const auto length = std::get<0>(cmd.payload.deserialize<uint32_t>());

    if (length > CMD_PAYLOAD_BUFFER_LEN) {
        return -1;
    }

    cmd.payload.to_vector(vec, length / sizeof(T));
    return 0;
}

template<>
template<>
inline int Session<WEBSOCK>::recv(std::string& str, Command& cmd)
{
    const auto length = std::get<0>(cmd.payload.deserialize<uint32_t>());

    if (length > CMD_PAYLOAD_BUFFER_LEN) {
        return -1;
    }

    cmd.payload.to_string(str, length);
    return 0;
}

template<>
template<typename... Tp>
inline std::tuple<int, Tp...> Session<WEBSOCK>::deserialize(Command& cmd, std::false_type)
{
    return std::make_tuple(0);
}

template<>
template<typename... Tp>
inline std::tuple<int, Tp...> Session<WEBSOCK>::deserialize(Command& cmd, std::true_type)
{
    return std::tuple_cat(std::make_tuple(0), cmd.payload.deserialize<Tp...>());
}

template<>
template<class T>
inline int Session<WEBSOCK>::write(const T *data, unsigned int len)
{
    return websock.send(data, len);
}

// -----------------------------------------------
// Select session kind
// -----------------------------------------------

#define CASE_TCP(...)                                             \
case TCP:                                                       \
    return static_cast<Session<TCP>*>(this)-> __VA_ARGS__;

#define CASE_UNIX(...)                                            \
case UNIX:                                                      \
    return static_cast<Session<UNIX>*>(this)-> __VA_ARGS__;

#define CASE_WEBSOCK(...)                                         \
case WEBSOCK:                                                   \
    return static_cast<Session<WEBSOCK>*>(this)-> __VA_ARGS__;

#define SWITCH_SOCK_TYPE(...)     \
    switch (this->kind) {         \
      CASE_TCP(__VA_ARGS__)       \
      CASE_UNIX(__VA_ARGS__)      \
      CASE_WEBSOCK(__VA_ARGS__)   \
      default: assert(false);     \
    }

// For the template in the middle, see:
// http://stackoverflow.com/questions/1682844/templates-template-function-not-playing-well-with-classs-template-member-funct/1682885#1682885

template<typename... Tp>
inline std::tuple<int, Tp...> SessionAbstract::deserialize(Command& cmd) {
    SWITCH_SOCK_TYPE(template deserialize<Tp...>(cmd, std::integral_constant<bool, 0 < sizeof...(Tp)>()))
    return std::tuple_cat(std::make_tuple(-1), std::tuple<Tp...>());
}

template<typename Tp>
inline int SessionAbstract::recv(Tp& container, Command& cmd) {
    SWITCH_SOCK_TYPE(recv(container, cmd))
    return -1;
}

template<uint16_t class_id, uint16_t func_id, typename... Args>
inline int SessionAbstract::send(Args&&... args) {
    SWITCH_SOCK_TYPE(send<class_id, func_id>(std::forward<Args>(args)...))
    return -1;
}

// Cast abstract session unique_ptr
template<int sock_type>
Session<sock_type>*
cast_to_session(const std::unique_ptr<SessionAbstract>& sess_abstract)
{
    return static_cast<Session<sock_type>*>(sess_abstract.get());
}

} // namespace kserver

#endif // __KSERVER_SESSION_HPP__
