#include "fusion32/protocol772/tcp_transport.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <limits>
#include <sstream>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace fusion32::protocol772 {

namespace {

#ifdef _WIN32
using NativeSocket = SOCKET;
constexpr NativeSocket kNativeInvalidSocket = INVALID_SOCKET;

class SocketRuntime {
public:
    SocketRuntime() {
        WSADATA data{};
        error_ = WSAStartup(MAKEWORD(2, 2), &data);
    }

    ~SocketRuntime() {
        if (error_ == 0) {
            WSACleanup();
        }
    }

    int error() const noexcept { return error_; }

private:
    int error_ = 0;
};

SocketRuntime& Runtime() {
    static SocketRuntime runtime;
    return runtime;
}

int LastSocketError() { return WSAGetLastError(); }
bool IsInterrupted(int error) { return error == WSAEINTR; }
bool IsConnectPending(int error) {
    return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS || error == WSAEALREADY;
}
bool IsTimeout(int error) { return error == WSAETIMEDOUT || error == WSAEWOULDBLOCK; }
bool IsRemoteClose(int error) {
    return error == WSAECONNRESET || error == WSAECONNABORTED || error == WSAENOTCONN;
}
void CloseNativeSocket(NativeSocket socket) { closesocket(socket); }
int ShutdownHow() { return SD_BOTH; }

bool SetNonBlocking(NativeSocket socket, bool enabled, int* error) {
    u_long mode = enabled ? 1UL : 0UL;
    if (ioctlsocket(socket, FIONBIO, &mode) == SOCKET_ERROR) {
        *error = LastSocketError();
        return false;
    }
    return true;
}

std::string SocketErrorMessage(int error) {
    char* message = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = FormatMessageA(
        flags, nullptr, static_cast<DWORD>(error), 0,
        reinterpret_cast<char*>(&message), 0, nullptr);
    std::string result = length > 0 && message != nullptr
        ? std::string(message, length)
        : std::string("Winsock error ") + std::to_string(error);
    if (message != nullptr) {
        LocalFree(message);
    }
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n')) {
        result.pop_back();
    }
    return result;
}

#else
using NativeSocket = int;
constexpr NativeSocket kNativeInvalidSocket = -1;

int LastSocketError() { return errno; }
bool IsInterrupted(int error) { return error == EINTR; }
bool IsConnectPending(int error) {
    return error == EINPROGRESS || error == EWOULDBLOCK || error == EALREADY;
}
bool IsTimeout(int error) { return error == EAGAIN || error == EWOULDBLOCK || error == ETIMEDOUT; }
bool IsRemoteClose(int error) {
    return error == ECONNRESET || error == ECONNABORTED || error == ENOTCONN || error == EPIPE;
}
void CloseNativeSocket(NativeSocket socket) { close(socket); }
int ShutdownHow() { return SHUT_RDWR; }

bool SetNonBlocking(NativeSocket socket, bool enabled, int* error) {
    const int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        *error = LastSocketError();
        return false;
    }
    const int updated = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (fcntl(socket, F_SETFL, updated) == -1) {
        *error = LastSocketError();
        return false;
    }
    return true;
}

std::string SocketErrorMessage(int error) {
    return std::strerror(error);
}
#endif

NativeSocket ToNative(std::uintptr_t socket) {
    return static_cast<NativeSocket>(socket);
}

std::uintptr_t FromNative(NativeSocket socket) {
    return static_cast<std::uintptr_t>(socket);
}

bool WaitForConnect(
    NativeSocket socket,
    std::chrono::milliseconds timeout,
    int* error) {
    fd_set write_set;
    FD_ZERO(&write_set);
    FD_SET(socket, &write_set);

    const auto timeout_count = timeout.count();
    timeval wait{};
    wait.tv_sec = static_cast<long>(timeout_count / 1000);
    wait.tv_usec = static_cast<long>((timeout_count % 1000) * 1000);

#ifdef _WIN32
    const int selected = select(0, nullptr, &write_set, nullptr, &wait);
#else
    const int selected = select(socket + 1, nullptr, &write_set, nullptr, &wait);
#endif
    if (selected == 0) {
#ifdef _WIN32
        *error = WSAETIMEDOUT;
#else
        *error = ETIMEDOUT;
#endif
        return false;
    }
    if (selected < 0) {
        *error = LastSocketError();
        return false;
    }

    int socket_error = 0;
#ifdef _WIN32
    int option_size = static_cast<int>(sizeof(socket_error));
#else
    socklen_t option_size = static_cast<socklen_t>(sizeof(socket_error));
#endif
    if (getsockopt(socket, SOL_SOCKET, SO_ERROR,
                   reinterpret_cast<char*>(&socket_error), &option_size) != 0) {
        *error = LastSocketError();
        return false;
    }
    if (socket_error != 0) {
        *error = socket_error;
        return false;
    }
    return true;
}

bool SetSocketOptions(
    NativeSocket socket,
    std::chrono::milliseconds timeout,
    int* error) {
    int no_delay = 1;
    if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY,
                   reinterpret_cast<const char*>(&no_delay), sizeof(no_delay)) != 0) {
        *error = LastSocketError();
        return false;
    }

#ifdef _WIN32
    const DWORD timeout_ms = static_cast<DWORD>(timeout.count());
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms)) != 0
        || setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO,
                      reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms)) != 0) {
        *error = LastSocketError();
        return false;
    }
#else
    timeval socket_timeout{};
    socket_timeout.tv_sec = static_cast<long>(timeout.count() / 1000);
    socket_timeout.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
                   &socket_timeout, sizeof(socket_timeout)) != 0
        || setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO,
                      &socket_timeout, sizeof(socket_timeout)) != 0) {
        *error = LastSocketError();
        return false;
    }
#endif
    return true;
}

int ReceiveBytes(NativeSocket socket, std::uint8_t* data, std::size_t size) {
    const int count = static_cast<int>(std::min<std::size_t>(size, INT_MAX));
#ifdef _WIN32
    return recv(socket, reinterpret_cast<char*>(data), count, 0);
#else
    return static_cast<int>(recv(socket, data, static_cast<std::size_t>(count), 0));
#endif
}

int SendBytes(NativeSocket socket, const std::uint8_t* data, std::size_t size) {
    const int count = static_cast<int>(std::min<std::size_t>(size, INT_MAX));
#ifdef _WIN32
    return send(socket, reinterpret_cast<const char*>(data), count, 0);
#else
#ifdef MSG_NOSIGNAL
    return static_cast<int>(send(socket, data, static_cast<std::size_t>(count), MSG_NOSIGNAL));
#else
    return static_cast<int>(send(socket, data, static_cast<std::size_t>(count), 0));
#endif
#endif
}

}  // namespace

TcpTransport::~TcpTransport() {
    CloseSocket();
}

TcpTransport::TcpTransport(TcpTransport&& other) noexcept
    : socket_(other.socket_),
      state_(other.state_),
      last_platform_error_(other.last_platform_error_),
      last_error_message_(std::move(other.last_error_message_)) {
    other.socket_ = kInvalidSocket;
    other.state_ = ConnectionState::Disconnected;
    other.last_platform_error_ = 0;
}

TcpTransport& TcpTransport::operator=(TcpTransport&& other) noexcept {
    if (this != &other) {
        CloseSocket();
        socket_ = other.socket_;
        state_ = other.state_;
        last_platform_error_ = other.last_platform_error_;
        last_error_message_ = std::move(other.last_error_message_);
        other.socket_ = kInvalidSocket;
        other.state_ = ConnectionState::Disconnected;
        other.last_platform_error_ = 0;
    }
    return *this;
}

void TcpTransport::SetFailure(int platform_error, std::string message) {
    state_ = ConnectionState::Failed;
    last_platform_error_ = platform_error;
    last_error_message_ = std::move(message);
}

void TcpTransport::CloseSocket() noexcept {
    if (socket_ != kInvalidSocket) {
        CloseNativeSocket(ToNative(socket_));
        socket_ = kInvalidSocket;
    }
}

ConnectResult TcpTransport::Connect(
    const std::string& host,
    std::uint16_t port,
    std::chrono::milliseconds timeout) {
    CloseSocket();
    state_ = ConnectionState::Disconnected;
    last_platform_error_ = 0;
    last_error_message_.clear();

    if (host.empty() || port == 0 || timeout.count() <= 0) {
        SetFailure(0, "host, port and timeout must be valid");
        return {IoStatus::InvalidArgument, 0, last_error_message_};
    }

#ifdef _WIN32
    if (Runtime().error() != 0) {
        SetFailure(Runtime().error(), SocketErrorMessage(Runtime().error()));
        return {IoStatus::Failed, last_platform_error_, last_error_message_};
    }
#endif

    state_ = ConnectionState::Connecting;

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* addresses = nullptr;
    const std::string port_text = std::to_string(port);
    const int address_status = getaddrinfo(host.c_str(), port_text.c_str(), &hints, &addresses);
    if (address_status != 0) {
#ifdef _WIN32
        const std::string message = gai_strerrorA(address_status);
#else
        const std::string message = gai_strerror(address_status);
#endif
        SetFailure(address_status, message);
        return {IoStatus::Failed, address_status, message};
    }

    int final_error = 0;
    std::string final_message = "no address could be connected";
    for (addrinfo* address = addresses; address != nullptr; address = address->ai_next) {
        NativeSocket candidate = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (candidate == kNativeInvalidSocket) {
            final_error = LastSocketError();
            final_message = SocketErrorMessage(final_error);
            continue;
        }

        int option_error = 0;
        if (!SetNonBlocking(candidate, true, &option_error)) {
            final_error = option_error;
            final_message = SocketErrorMessage(final_error);
            CloseNativeSocket(candidate);
            continue;
        }

        bool connected = connect(candidate, address->ai_addr,
#ifdef _WIN32
                                 static_cast<int>(address->ai_addrlen)
#else
                                 address->ai_addrlen
#endif
                                 ) == 0;
        if (!connected) {
            const int connect_error = LastSocketError();
            if (IsConnectPending(connect_error)) {
                connected = WaitForConnect(candidate, timeout, &option_error);
            } else {
                option_error = connect_error;
            }
        }

        if (connected && !SetNonBlocking(candidate, false, &option_error)) {
            connected = false;
        }
        if (connected && !SetSocketOptions(candidate, timeout, &option_error)) {
            connected = false;
        }

        if (connected) {
            socket_ = FromNative(candidate);
            state_ = ConnectionState::Connected;
            last_platform_error_ = 0;
            last_error_message_.clear();
            freeaddrinfo(addresses);
            return {IoStatus::Ok, 0, {}};
        }

        final_error = option_error;
        final_message = SocketErrorMessage(final_error);
        CloseNativeSocket(candidate);
    }

    freeaddrinfo(addresses);
    SetFailure(final_error, final_message);
    const IoStatus status = IsTimeout(final_error) ? IoStatus::TimedOut : IoStatus::Failed;
    return {status, final_error, final_message};
}

void TcpTransport::Disconnect() noexcept {
    if (socket_ != kInvalidSocket) {
        shutdown(ToNative(socket_), ShutdownHow());
    }
    CloseSocket();
    state_ = ConnectionState::Disconnected;
    last_platform_error_ = 0;
    last_error_message_.clear();
}

ReadResult TcpTransport::ReadSome(std::size_t max_bytes) {
    if (state_ != ConnectionState::Connected || socket_ == kInvalidSocket) {
        return {IoStatus::NotConnected, {}, 0, "transport is not connected"};
    }
    if (max_bytes == 0 || max_bytes > static_cast<std::size_t>(INT_MAX)) {
        return {IoStatus::InvalidArgument, {}, 0, "read size must be between 1 and INT_MAX"};
    }

    ReadResult result;
    result.bytes.resize(max_bytes);
    int received = -1;
    do {
        received = ReceiveBytes(ToNative(socket_), result.bytes.data(), result.bytes.size());
    } while (received < 0 && IsInterrupted(LastSocketError()));

    if (received > 0) {
        result.bytes.resize(static_cast<std::size_t>(received));
        result.status = IoStatus::Ok;
        return result;
    }
    result.bytes.clear();
    if (received == 0) {
        CloseSocket();
        state_ = ConnectionState::RemoteClosed;
        result.status = IoStatus::RemoteClosed;
        result.message = "peer closed the connection";
        return result;
    }

    const int error = LastSocketError();
    result.platform_error = error;
    result.message = SocketErrorMessage(error);
    if (IsTimeout(error)) {
        result.status = IoStatus::TimedOut;
        return result;
    }
    if (IsRemoteClose(error)) {
        CloseSocket();
        state_ = ConnectionState::RemoteClosed;
        result.status = IoStatus::RemoteClosed;
        return result;
    }

    CloseSocket();
    SetFailure(error, result.message);
    result.status = IoStatus::Failed;
    return result;
}

WriteResult TcpTransport::WriteAll(const std::uint8_t* data, std::size_t size) {
    if (state_ != ConnectionState::Connected || socket_ == kInvalidSocket) {
        return {IoStatus::NotConnected, 0, 0, "transport is not connected"};
    }
    if (size > 0 && data == nullptr) {
        return {IoStatus::InvalidArgument, 0, 0, "write buffer is null"};
    }

    WriteResult result;
    result.status = IoStatus::Ok;
    while (result.bytes_transferred < size) {
        const std::size_t remaining = size - result.bytes_transferred;
        int sent = -1;
        do {
            sent = SendBytes(ToNative(socket_), data + result.bytes_transferred, remaining);
        } while (sent < 0 && IsInterrupted(LastSocketError()));

        if (sent > 0) {
            result.bytes_transferred += static_cast<std::size_t>(sent);
            continue;
        }

        const int error = sent == 0 ? 0 : LastSocketError();
        result.platform_error = error;
        result.message = sent == 0 ? "socket wrote zero bytes" : SocketErrorMessage(error);
        result.status = IsTimeout(error) ? IoStatus::TimedOut
            : (IsRemoteClose(error) ? IoStatus::RemoteClosed : IoStatus::Failed);

        // A partial frame cannot be safely resumed by a higher layer after an
        // error, so fail the connection instead of exposing ambiguous stream state.
        CloseSocket();
        state_ = result.status == IoStatus::RemoteClosed
            ? ConnectionState::RemoteClosed
            : ConnectionState::Failed;
        last_platform_error_ = error;
        last_error_message_ = result.message;
        return result;
    }
    return result;
}

WriteResult TcpTransport::WriteAll(const std::vector<std::uint8_t>& data) {
    return WriteAll(data.data(), data.size());
}

const char* ConnectionStateName(ConnectionState state) noexcept {
    switch (state) {
        case ConnectionState::Disconnected: return "Disconnected";
        case ConnectionState::Connecting: return "Connecting";
        case ConnectionState::Connected: return "Connected";
        case ConnectionState::RemoteClosed: return "RemoteClosed";
        case ConnectionState::Failed: return "Failed";
    }
    return "Unknown";
}

const char* IoStatusName(IoStatus status) noexcept {
    switch (status) {
        case IoStatus::Ok: return "Ok";
        case IoStatus::TimedOut: return "TimedOut";
        case IoStatus::RemoteClosed: return "RemoteClosed";
        case IoStatus::NotConnected: return "NotConnected";
        case IoStatus::InvalidArgument: return "InvalidArgument";
        case IoStatus::Failed: return "Failed";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
