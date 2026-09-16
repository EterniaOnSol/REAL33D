#ifndef FUSION32_PROTOCOL772_FRAMED_CONNECTION_H
#define FUSION32_PROTOCOL772_FRAMED_CONNECTION_H

#include "fusion32/protocol772/framing.h"
#include "fusion32/protocol772/tcp_transport.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

struct FramedReadResult {
    IoStatus io_status = IoStatus::Failed;
    FrameBatch framing;
    int platform_error = 0;
    std::string message;

    bool ok() const noexcept {
        return (io_status == IoStatus::Ok || io_status == IoStatus::RemoteClosed)
            && framing.ok();
    }
};

struct FramedWriteResult {
    FrameError framing_error = FrameError::None;
    WriteResult transport;

    bool ok() const noexcept {
        return framing_error == FrameError::None && transport.ok();
    }
};

class FramedConnection {
public:
    explicit FramedConnection(
        ClientFrameLimits limits,
        std::size_t read_chunk_size = 4096);

    ConnectResult Connect(
        const std::string& host,
        std::uint16_t port,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));

    void Disconnect() noexcept;
    FramedReadResult ReadOnce();
    FramedWriteResult SendFrame(const std::uint8_t* payload, std::size_t size);
    FramedWriteResult SendFrame(const std::vector<std::uint8_t>& payload);

    TcpTransport& transport() noexcept { return transport_; }
    const TcpTransport& transport() const noexcept { return transport_; }
    const FrameDecoder& decoder() const noexcept { return decoder_; }
    ClientFrameLimits limits() const noexcept { return limits_; }

private:
    ClientFrameLimits limits_;
    std::size_t read_chunk_size_;
    TcpTransport transport_;
    FrameDecoder decoder_;
};

}  // namespace fusion32::protocol772

#endif
