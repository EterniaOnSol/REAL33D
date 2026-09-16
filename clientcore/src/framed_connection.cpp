#include "fusion32/protocol772/framed_connection.h"

#include <stdexcept>
#include <utility>

namespace fusion32::protocol772 {

FramedConnection::FramedConnection(ClientFrameLimits limits, std::size_t read_chunk_size)
    : limits_(limits),
      read_chunk_size_(read_chunk_size),
      decoder_(limits.max_receive_payload) {
    if (limits_.max_send_payload == 0 || limits_.max_send_payload > kWirePayloadLimit) {
        throw std::invalid_argument("send payload limit must be between 1 and 65535");
    }
    if (read_chunk_size_ == 0) {
        throw std::invalid_argument("read chunk size must be non-zero");
    }
}

ConnectResult FramedConnection::Connect(
    const std::string& host,
    std::uint16_t port,
    std::chrono::milliseconds timeout) {
    decoder_.Reset();
    return transport_.Connect(host, port, timeout);
}

void FramedConnection::Disconnect() noexcept {
    transport_.Disconnect();
    decoder_.Reset();
}

FramedReadResult FramedConnection::ReadOnce() {
    FramedReadResult result;
    ReadResult read = transport_.ReadSome(read_chunk_size_);
    result.io_status = read.status;
    result.platform_error = read.platform_error;
    result.message = std::move(read.message);

    if (read.status == IoStatus::Ok) {
        result.framing = decoder_.Feed(read.bytes);
    } else if (read.status == IoStatus::RemoteClosed) {
        result.framing = decoder_.Finish();
    } else {
        result.framing.buffered_bytes = decoder_.buffered_bytes();
        result.framing.needs_more_data = decoder_.buffered_bytes() != 0;
        result.framing.error = decoder_.error();
    }
    return result;
}

FramedWriteResult FramedConnection::SendFrame(
    const std::uint8_t* payload,
    std::size_t size) {
    FramedWriteResult result;
    FrameEncodeResult encoded = EncodeFrame(payload, size, limits_.max_send_payload);
    result.framing_error = encoded.error;
    if (!encoded.ok()) {
        result.transport.status = IoStatus::InvalidArgument;
        result.transport.message = FrameErrorName(encoded.error);
        return result;
    }

    result.transport = transport_.WriteAll(encoded.bytes);
    return result;
}

FramedWriteResult FramedConnection::SendFrame(const std::vector<std::uint8_t>& payload) {
    return SendFrame(payload.data(), payload.size());
}

}  // namespace fusion32::protocol772
