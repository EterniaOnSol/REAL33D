#ifndef FUSION32_PROTOCOL772_TCP_TRANSPORT_H
#define FUSION32_PROTOCOL772_TCP_TRANSPORT_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    RemoteClosed,
    Failed,
};

enum class IoStatus {
    Ok,
    TimedOut,
    RemoteClosed,
    NotConnected,
    InvalidArgument,
    Failed,
};

struct ConnectResult {
    IoStatus status = IoStatus::Failed;
    int platform_error = 0;
    std::string message;

    bool ok() const noexcept { return status == IoStatus::Ok; }
};

struct ReadResult {
    IoStatus status = IoStatus::Failed;
    std::vector<std::uint8_t> bytes;
    int platform_error = 0;
    std::string message;

    bool ok() const noexcept { return status == IoStatus::Ok; }
};

struct WriteResult {
    IoStatus status = IoStatus::Failed;
    std::size_t bytes_transferred = 0;
    int platform_error = 0;
    std::string message;

    bool ok() const noexcept { return status == IoStatus::Ok; }
};

class TcpTransport {
public:
    TcpTransport() = default;
    ~TcpTransport();

    TcpTransport(const TcpTransport&) = delete;
    TcpTransport& operator=(const TcpTransport&) = delete;
    TcpTransport(TcpTransport&& other) noexcept;
    TcpTransport& operator=(TcpTransport&& other) noexcept;

    ConnectResult Connect(
        const std::string& host,
        std::uint16_t port,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));

    void Disconnect() noexcept;

    ReadResult ReadSome(std::size_t max_bytes = 4096);
    WriteResult WriteAll(const std::uint8_t* data, std::size_t size);
    WriteResult WriteAll(const std::vector<std::uint8_t>& data);

    ConnectionState state() const noexcept { return state_; }
    bool connected() const noexcept { return state_ == ConnectionState::Connected; }
    int last_platform_error() const noexcept { return last_platform_error_; }
    const std::string& last_error_message() const noexcept { return last_error_message_; }

private:
    static constexpr std::uintptr_t kInvalidSocket = ~std::uintptr_t{0};

    void CloseSocket() noexcept;
    void SetFailure(int platform_error, std::string message);

    std::uintptr_t socket_ = kInvalidSocket;
    ConnectionState state_ = ConnectionState::Disconnected;
    int last_platform_error_ = 0;
    std::string last_error_message_;
};

const char* ConnectionStateName(ConnectionState state) noexcept;
const char* IoStatusName(IoStatus status) noexcept;

}  // namespace fusion32::protocol772

#endif
