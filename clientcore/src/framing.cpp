#include "fusion32/protocol772/framing.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace fusion32::protocol772 {

namespace {

std::size_t ReadPayloadSize(const std::vector<std::uint8_t>& buffer) {
    return static_cast<std::size_t>(buffer[0])
        | (static_cast<std::size_t>(buffer[1]) << 8U);
}

}  // namespace

FrameDecoder::FrameDecoder(std::size_t max_payload_size)
    : max_payload_size_(max_payload_size) {
    if (max_payload_size_ == 0 || max_payload_size_ > kWirePayloadLimit) {
        throw std::invalid_argument("frame payload limit must be between 1 and 65535");
    }
    buffer_.reserve(max_payload_size_ + kOuterHeaderBytes);
}

FrameBatch FrameDecoder::FailedBatch(
    const std::uint8_t* data,
    std::size_t size,
    FrameError error) const {
    FrameBatch result;
    result.error = error;
    result.buffered_bytes = buffer_.size();
    if (data != nullptr && size > 0) {
        result.unconsumed_bytes.assign(data, data + size);
    }
    return result;
}

FrameBatch FrameDecoder::Feed(const std::uint8_t* data, std::size_t size) {
    if (error_ != FrameError::None) {
        return FailedBatch(data, size, error_);
    }
    if (finished_) {
        return FailedBatch(data, size, FrameError::StreamFinished);
    }

    FrameBatch result;
    if (size > 0 && data == nullptr) {
        error_ = FrameError::InvalidConfiguration;
        result.error = error_;
        return result;
    }

    std::size_t position = 0;
    while (position < size) {
        if (buffer_.size() < kOuterHeaderBytes) {
            const std::size_t needed = kOuterHeaderBytes - buffer_.size();
            const std::size_t copied = std::min(needed, size - position);
            buffer_.insert(buffer_.end(), data + position, data + position + copied);
            position += copied;
            if (buffer_.size() < kOuterHeaderBytes) {
                break;
            }
        }

        if (expected_payload_size_ == 0) {
            expected_payload_size_ = ReadPayloadSize(buffer_);
            if (expected_payload_size_ == 0) {
                error_ = FrameError::ZeroLength;
            } else if (expected_payload_size_ > max_payload_size_) {
                error_ = FrameError::ExceedsConfiguredMaximum;
            }

            if (error_ != FrameError::None) {
                result.error = error_;
                result.input_bytes_consumed = position;
                result.buffered_bytes = buffer_.size();
                result.unconsumed_bytes.assign(data + position, data + size);
                return result;
            }
        }

        const std::size_t frame_size = kOuterHeaderBytes + expected_payload_size_;
        const std::size_t needed = frame_size - buffer_.size();
        const std::size_t copied = std::min(needed, size - position);
        buffer_.insert(buffer_.end(), data + position, data + position + copied);
        position += copied;

        if (buffer_.size() < frame_size) {
            break;
        }

        FramedPacket packet;
        packet.payload.assign(buffer_.begin() + static_cast<std::ptrdiff_t>(kOuterHeaderBytes),
                              buffer_.end());
        result.frames.push_back(std::move(packet));
        buffer_.clear();
        expected_payload_size_ = 0;
    }

    result.input_bytes_consumed = position;
    result.buffered_bytes = buffer_.size();
    result.needs_more_data = !buffer_.empty();
    return result;
}

FrameBatch FrameDecoder::Feed(const std::vector<std::uint8_t>& data) {
    return Feed(data.data(), data.size());
}

FrameBatch FrameDecoder::Finish() {
    if (error_ != FrameError::None) {
        return FailedBatch(nullptr, 0, error_);
    }

    FrameBatch result;
    finished_ = true;
    result.buffered_bytes = buffer_.size();
    if (!buffer_.empty()) {
        error_ = FrameError::TruncatedFrame;
        result.error = error_;
    } else {
        result.end_of_stream = true;
    }
    return result;
}

void FrameDecoder::Reset() noexcept {
    expected_payload_size_ = 0;
    buffer_.clear();
    error_ = FrameError::None;
    finished_ = false;
}

FrameEncodeResult EncodeFrame(
    const std::uint8_t* payload,
    std::size_t payload_size,
    std::size_t max_payload_size) {
    FrameEncodeResult result;
    if (max_payload_size == 0 || max_payload_size > kWirePayloadLimit) {
        result.error = FrameError::InvalidConfiguration;
        return result;
    }
    if (payload_size == 0) {
        result.error = FrameError::ZeroLength;
        return result;
    }
    if (payload_size > kWirePayloadLimit) {
        result.error = FrameError::PayloadTooLargeForHeader;
        return result;
    }
    if (payload_size > max_payload_size) {
        result.error = FrameError::ExceedsConfiguredMaximum;
        return result;
    }
    if (payload == nullptr) {
        result.error = FrameError::InvalidConfiguration;
        return result;
    }

    result.bytes.resize(kOuterHeaderBytes + payload_size);
    result.bytes[0] = static_cast<std::uint8_t>(payload_size & 0xFFU);
    result.bytes[1] = static_cast<std::uint8_t>((payload_size >> 8U) & 0xFFU);
    std::copy(payload, payload + payload_size, result.bytes.begin() + 2);
    return result;
}

FrameEncodeResult EncodeFrame(
    const std::vector<std::uint8_t>& payload,
    std::size_t max_payload_size) {
    return EncodeFrame(payload.data(), payload.size(), max_payload_size);
}

const char* FrameErrorName(FrameError error) noexcept {
    switch (error) {
        case FrameError::None: return "None";
        case FrameError::InvalidConfiguration: return "InvalidConfiguration";
        case FrameError::ZeroLength: return "ZeroLength";
        case FrameError::ExceedsConfiguredMaximum: return "ExceedsConfiguredMaximum";
        case FrameError::PayloadTooLargeForHeader: return "PayloadTooLargeForHeader";
        case FrameError::TruncatedFrame: return "TruncatedFrame";
        case FrameError::StreamFinished: return "StreamFinished";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
