#ifndef FUSION32_PROTOCOL772_FRAMING_H
#define FUSION32_PROTOCOL772_FRAMING_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fusion32::protocol772 {

constexpr std::size_t kOuterHeaderBytes = 2;
constexpr std::size_t kWirePayloadLimit = 0xFFFF;

struct ClientFrameLimits {
    std::size_t max_receive_payload;
    std::size_t max_send_payload;
};

// Source-derived upper bounds from the selected Fusion32 buffers and encrypted
// packet layout. These are client-side directions.
constexpr ClientFrameLimits kLoginClientFrameLimits{2046, 2048};
constexpr ClientFrameLimits kGameClientFrameLimits{16392, 2048};

enum class FrameError {
    None,
    InvalidConfiguration,
    ZeroLength,
    ExceedsConfiguredMaximum,
    PayloadTooLargeForHeader,
    TruncatedFrame,
    StreamFinished,
};

struct FramedPacket {
    std::vector<std::uint8_t> payload;
};

struct FrameBatch {
    std::vector<FramedPacket> frames;
    std::vector<std::uint8_t> unconsumed_bytes;
    FrameError error = FrameError::None;
    std::size_t input_bytes_consumed = 0;
    std::size_t buffered_bytes = 0;
    bool needs_more_data = false;
    bool end_of_stream = false;

    bool ok() const noexcept { return error == FrameError::None; }
};

struct FrameEncodeResult {
    std::vector<std::uint8_t> bytes;
    FrameError error = FrameError::None;

    bool ok() const noexcept { return error == FrameError::None; }
};

class FrameDecoder {
public:
    explicit FrameDecoder(std::size_t max_payload_size);

    FrameBatch Feed(const std::uint8_t* data, std::size_t size);
    FrameBatch Feed(const std::vector<std::uint8_t>& data);
    FrameBatch Finish();

    void Reset() noexcept;

    std::size_t max_payload_size() const noexcept { return max_payload_size_; }
    std::size_t buffered_bytes() const noexcept { return buffer_.size(); }
    FrameError error() const noexcept { return error_; }
    const std::vector<std::uint8_t>& buffered_data() const noexcept { return buffer_; }

private:
    FrameBatch FailedBatch(
        const std::uint8_t* data,
        std::size_t size,
        FrameError error) const;

    std::size_t max_payload_size_;
    std::size_t expected_payload_size_ = 0;
    std::vector<std::uint8_t> buffer_;
    FrameError error_ = FrameError::None;
    bool finished_ = false;
};

FrameEncodeResult EncodeFrame(
    const std::uint8_t* payload,
    std::size_t payload_size,
    std::size_t max_payload_size);

FrameEncodeResult EncodeFrame(
    const std::vector<std::uint8_t>& payload,
    std::size_t max_payload_size);

const char* FrameErrorName(FrameError error) noexcept;

}  // namespace fusion32::protocol772

#endif
