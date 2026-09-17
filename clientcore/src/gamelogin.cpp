#include "fusion32/protocol772/gamelogin.h"

#include <algorithm>

namespace fusion32::protocol772 {
namespace {

void Write16(std::uint8_t* out, std::uint16_t value) {
    out[0] = static_cast<std::uint8_t>(value);
    out[1] = static_cast<std::uint8_t>(value >> 8U);
}

void Write32(std::uint8_t* out, std::uint32_t value) {
    out[0] = static_cast<std::uint8_t>(value);
    out[1] = static_cast<std::uint8_t>(value >> 8U);
    out[2] = static_cast<std::uint8_t>(value >> 16U);
    out[3] = static_cast<std::uint8_t>(value >> 24U);
}

std::uint16_t Read16(const std::vector<std::uint8_t>& bytes, std::size_t at) {
    return static_cast<std::uint16_t>(bytes[at])
        | static_cast<std::uint16_t>(bytes[at + 1] << 8U);
}

std::uint32_t Read32(const std::vector<std::uint8_t>& bytes, std::size_t at) {
    return static_cast<std::uint32_t>(bytes[at])
        | (static_cast<std::uint32_t>(bytes[at + 1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[at + 2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[at + 3]) << 24U);
}

bool WriteString(std::array<std::uint8_t, kRsa1024BlockBytes>* block,
                 std::size_t* at, const std::string& value) {
    if (value.size() >= 0xFFFFU || *at + 2 + value.size() > block->size()) return false;
    Write16(block->data() + *at, static_cast<std::uint16_t>(value.size()));
    *at += 2;
    std::copy(value.begin(), value.end(), block->begin() + static_cast<std::ptrdiff_t>(*at));
    *at += value.size();
    return true;
}

bool ReadString(const std::vector<std::uint8_t>& bytes, std::size_t* at,
                std::string* output) {
    if (*at + 2 > bytes.size()) return false;
    std::uint64_t size = Read16(bytes, *at);
    *at += 2;
    if (size == 0xFFFFU) {
        if (*at + 4 > bytes.size()) return false;
        size = Read32(bytes, *at);
        *at += 4;
    }
    if (size > bytes.size() - *at) return false;
    output->assign(reinterpret_cast<const char*>(bytes.data() + *at),
                   static_cast<std::size_t>(size));
    *at += static_cast<std::size_t>(size);
    return true;
}

void Preserve(const std::vector<std::uint8_t>& bytes, std::size_t at,
              GameInitialMessage* result) {
    if (at <= bytes.size()) {
        result->unparsed_bytes.assign(bytes.begin() + static_cast<std::ptrdiff_t>(at),
                                      bytes.end());
    }
}

}  // namespace

GameLoginRequest BuildGameLoginRequest(const GameLoginRequestOptions& options,
                                       const Rsa1024PublicKey& rsa_key) {
    return BuildGameLoginRequest(options, rsa_key, SecureRandomBytes);
}

GameLoginRequest BuildGameLoginRequest(const GameLoginRequestOptions& options,
                                       const Rsa1024PublicKey& rsa_key,
                                       const RandomBytesFunction& random_bytes) {
    GameLoginRequest result;
    if (options.account_id == 0) {
        result.error = GameLoginBuildError::InvalidAccountId;
        return result;
    }
    if (options.terminal_type < 1 || options.terminal_type > 2
        || options.terminal_version != kLoginProtocolVersion) {
        result.error = GameLoginBuildError::InvalidTerminal;
        return result;
    }
    if (options.character_name.empty() || options.character_name.size() > 29) {
        result.error = GameLoginBuildError::CharacterNameTooLong;
        return result;
    }
    if (options.password.size() > 29) {
        result.error = GameLoginBuildError::PasswordTooLong;
        return result;
    }
    if (!rsa_key.initialized() || !random_bytes) {
        result.error = GameLoginBuildError::InvalidArgument;
        return result;
    }
    if (GenerateXteaKey(&result.xtea_key, random_bytes) != CryptoError::None) {
        result.error = GameLoginBuildError::RandomGenerationFailed;
        return result;
    }

    std::array<std::uint8_t, kRsa1024BlockBytes> plaintext{};
    std::array<std::uint8_t, kXteaKeyBytes> key_bytes{};
    if (result.xtea_key.SerializeLittleEndian(&key_bytes) != CryptoError::None) {
        result.error = GameLoginBuildError::RsaError;
        return result;
    }
    std::copy(key_bytes.begin(), key_bytes.end(), plaintext.begin() + 1);
    plaintext[17] = options.gamemaster_client ? 1 : 0;
    Write32(plaintext.data() + 18, options.account_id);
    std::size_t at = 22;
    if (!WriteString(&plaintext, &at, options.character_name)
        || !WriteString(&plaintext, &at, options.password)) {
        result.error = GameLoginBuildError::RsaError;
        return result;
    }
    bool padding_ok = true;
    if (at < plaintext.size()) {
        try {
            padding_ok = random_bytes(plaintext.data() + at, plaintext.size() - at);
        } catch (...) {
            padding_ok = false;
        }
    }
    if (!padding_ok) {
        result.error = GameLoginBuildError::RandomGenerationFailed;
        return result;
    }
    if (rsa_key.EncryptProtocolBlock(plaintext, &result.rsa_ciphertext) != CryptoError::None) {
        result.error = GameLoginBuildError::RsaError;
        return result;
    }

    result.payload.resize(1 + 2 + 2 + kRsa1024BlockBytes);
    result.payload[0] = kGameLoginRequestOpcode;
    Write16(result.payload.data() + 1, options.terminal_type);
    Write16(result.payload.data() + 3, options.terminal_version);
    std::copy(result.rsa_ciphertext.begin(), result.rsa_ciphertext.end(),
              result.payload.begin() + 5);
    const FrameEncodeResult frame = EncodeFrame(
        result.payload, kGameClientFrameLimits.max_send_payload);
    if (!frame.ok()) {
        result.error = GameLoginBuildError::FramingError;
        return result;
    }
    result.wire_bytes = frame.bytes;
    return result;
}

GameInitialMessage ParseGameInitialMessage(const XteaDecodeResult& decrypted) {
    GameInitialMessage result;
    if (!decrypted.ok()) {
        result.status = GameMessageStatus::CryptoError;
        result.crypto_error = decrypted.error;
        result.message_bytes = decrypted.input_payload;
        return result;
    }
    result.message_bytes = decrypted.message;
    const auto& bytes = decrypted.message;
    if (bytes.empty()) {
        result.status = GameMessageStatus::Incomplete;
        return result;
    }
    const std::uint8_t opcode = bytes[0];
    std::size_t at = 0;
    if (opcode == kGameInitOpcode) {
        if (bytes.size() < 8) {
            result.status = bytes.size() < 8 ? GameMessageStatus::Incomplete
                                             : GameMessageStatus::Malformed;
            Preserve(bytes, 0, &result);
            return result;
        }
        result.creature_id = Read32(bytes, 1);
        result.beat = Read16(bytes, 5);
        result.bug_reports = bytes[7] != 0;
        result.init_game_received = true;
        result.status = GameMessageStatus::InitGame;
        at = 8;
    }
    if (at < bytes.size() && bytes[at] == kGameRightsOpcode) {
        if (bytes.size() - at < 33) {
            result.status = GameMessageStatus::Incomplete;
            Preserve(bytes, at, &result);
            return result;
        }
        result.rights.assign(bytes.begin() + static_cast<std::ptrdiff_t>(at + 1),
                            bytes.begin() + static_cast<std::ptrdiff_t>(at + 33));
        result.rights_received = true;
        result.status = GameMessageStatus::Rights;
        at += 33;
    }
    if (at < bytes.size() && bytes[at] == kGameFullScreenOpcode) {
        result.status = GameMessageStatus::FullScreenUnparsed;
        result.unparsed_bytes.assign(bytes.begin() + static_cast<std::ptrdiff_t>(at),
                                     bytes.end());
        return result;
    }
    if (at < bytes.size()) {
        result.status = GameMessageStatus::Unsupported;
        result.unparsed_bytes.assign(bytes.begin() + static_cast<std::ptrdiff_t>(at),
                                     bytes.end());
        return result;
    }
    if (result.init_game_received) {
        return result;
    }
    if (opcode == kGameRightsOpcode) {
        if (bytes.size() != 33) {
            result.status = bytes.size() < 33 ? GameMessageStatus::Incomplete
                                              : GameMessageStatus::Malformed;
            Preserve(bytes, 0, &result);
            return result;
        }
        result.rights.assign(bytes.begin() + 1, bytes.end());
        result.rights_received = true;
        result.status = GameMessageStatus::Rights;
        return result;
    }
    if (opcode == kGameLoginErrorOpcode || opcode == kGameLoginPremiumOpcode
        || opcode == kGameLoginWaitingListOpcode) {
        std::size_t at = 1;
        if (!ReadString(bytes, &at, &result.text)) {
            result.status = GameMessageStatus::Incomplete;
            Preserve(bytes, 0, &result);
            return result;
        }
        if (opcode == kGameLoginWaitingListOpcode) {
            if (at >= bytes.size()) {
                result.status = GameMessageStatus::Incomplete;
                Preserve(bytes, 0, &result);
                return result;
            }
            result.wait_seconds = bytes[at++];
        }
        if (at != bytes.size()) {
            result.status = GameMessageStatus::Malformed;
            Preserve(bytes, at, &result);
            return result;
        }
        result.status = opcode == kGameLoginErrorOpcode
            ? GameMessageStatus::LoginError
            : (opcode == kGameLoginPremiumOpcode
                ? GameMessageStatus::PremiumNotice : GameMessageStatus::WaitingList);
        return result;
    }
    if (opcode == kGamePingOpcode) {
        result.status = bytes.size() == 1 ? GameMessageStatus::Ping
                                         : GameMessageStatus::Malformed;
        Preserve(bytes, 1, &result);
        return result;
    }
    if (opcode == kGameFullScreenOpcode) {
        result.status = GameMessageStatus::FullScreenUnparsed;
        result.unparsed_bytes = bytes;
        return result;
    }
    result.status = GameMessageStatus::Unsupported;
    result.unparsed_bytes = bytes;
    return result;
}

GameLoginSession::GameLoginSession(std::size_t read_chunk_size,
                                   std::chrono::milliseconds timeout)
    : connection_({kGameClientFrameLimits.max_receive_payload,
                    kGameClientFrameLimits.max_send_payload}, read_chunk_size),
      timeout_(timeout) {}

ConnectResult GameLoginSession::Connect(const std::string& host, std::uint16_t port) {
    authenticated_ = false;
    xtea_key_ = XteaKey{};
    return connection_.Connect(host, port, timeout_);
}

void GameLoginSession::Disconnect() noexcept {
    connection_.Disconnect();
    authenticated_ = false;
    xtea_key_ = XteaKey{};
    pending_frames_.clear();
}

FramedWriteResult GameLoginSession::SendLogin(const GameLoginRequest& request) {
    if (!request.ok()) {
        return {FrameError::InvalidConfiguration,
                {IoStatus::InvalidArgument, 0, 0, "invalid game login request"}};
    }
    xtea_key_ = XteaKey::FromLittleEndian(request.xtea_key.initialized()
        ? [&request]() {
            std::array<std::uint8_t, kXteaKeyBytes> bytes{};
            request.xtea_key.SerializeLittleEndian(&bytes);
            return bytes;
        }() : std::array<std::uint8_t, kXteaKeyBytes>{});
    return connection_.SendFrame(request.payload);
}

FramedWriteResult GameLoginSession::SendCommand(const std::vector<std::uint8_t>& payload) {
    if (payload.empty() || !xtea_key_.initialized()) {
        return {FrameError::InvalidConfiguration,
                {IoStatus::InvalidArgument, 0, 0, "no authenticated session"}};
    }
    const auto encoded = EncryptXteaPayload(
        xtea_key_, payload, kGameClientFrameLimits.max_send_payload);
    if (!encoded.ok()) {
        return {FrameError::InvalidConfiguration,
                {IoStatus::InvalidArgument, 0, 0, "xtea encode failed"}};
    }
    return connection_.SendFrame(encoded.packet.payload);
}

GameLoginSession::ReadResult GameLoginSession::ReadNextMessage() {
    for (int attempt = 0; attempt < 100; ++attempt) {
        FramedReadResult read;
        if (pending_frames_.empty()) {
            read = connection_.ReadOnce();
            if (!read.ok() && read.framing.frames.empty()) return {read, {}};
            for (auto& frame : read.framing.frames) {
                pending_frames_.push_back(std::move(frame));
            }
        } else {
            read.io_status = IoStatus::Ok;
        }
        if (!pending_frames_.empty()) {
            FramedPacket frame = std::move(pending_frames_.front());
            pending_frames_.pop_front();
            const auto decoded = DecryptXteaPayload(xtea_key_, frame);
            const auto message = ParseGameInitialMessage(decoded);
            if (message.authenticated()) authenticated_ = true;
            return {read, message};
        }
    }
    FramedReadResult timeout;
    timeout.io_status = IoStatus::TimedOut;
    timeout.message = "no complete game message";
    return {timeout, {}};
}

const char* GameLoginBuildErrorName(GameLoginBuildError error) noexcept {
    switch (error) {
        case GameLoginBuildError::None: return "None";
        case GameLoginBuildError::InvalidArgument: return "InvalidArgument";
        case GameLoginBuildError::InvalidAccountId: return "InvalidAccountId";
        case GameLoginBuildError::InvalidTerminal: return "InvalidTerminal";
        case GameLoginBuildError::CharacterNameTooLong: return "CharacterNameTooLong";
        case GameLoginBuildError::PasswordTooLong: return "PasswordTooLong";
        case GameLoginBuildError::RandomGenerationFailed: return "RandomGenerationFailed";
        case GameLoginBuildError::RsaError: return "RsaError";
        case GameLoginBuildError::FramingError: return "FramingError";
    }
    return "Unknown";
}

const char* GameMessageStatusName(GameMessageStatus status) noexcept {
    switch (status) {
        case GameMessageStatus::InitGame: return "InitGame";
        case GameMessageStatus::Rights: return "Rights";
        case GameMessageStatus::LoginError: return "LoginError";
        case GameMessageStatus::PremiumNotice: return "PremiumNotice";
        case GameMessageStatus::WaitingList: return "WaitingList";
        case GameMessageStatus::Ping: return "Ping";
        case GameMessageStatus::FullScreenUnparsed: return "FullScreenUnparsed";
        case GameMessageStatus::Unsupported: return "Unsupported";
        case GameMessageStatus::Incomplete: return "Incomplete";
        case GameMessageStatus::Malformed: return "Malformed";
        case GameMessageStatus::CryptoError: return "CryptoError";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
