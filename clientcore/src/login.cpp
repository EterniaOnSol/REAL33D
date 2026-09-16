#include "fusion32/protocol772/login.h"

#include <algorithm>
#include <limits>

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

std::uint32_t Read32LE(const std::vector<std::uint8_t>& bytes, std::size_t at) {
    return static_cast<std::uint32_t>(bytes[at])
        | (static_cast<std::uint32_t>(bytes[at + 1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[at + 2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[at + 3]) << 24U);
}

std::uint32_t Read32BE(const std::vector<std::uint8_t>& bytes, std::size_t at) {
    return (static_cast<std::uint32_t>(bytes[at]) << 24U)
        | (static_cast<std::uint32_t>(bytes[at + 1]) << 16U)
        | (static_cast<std::uint32_t>(bytes[at + 2]) << 8U)
        | static_cast<std::uint32_t>(bytes[at + 3]);
}

bool TakeString(const std::vector<std::uint8_t>& bytes, std::size_t* at,
                std::string* out) {
    if (*at + 2 > bytes.size()) return false;
    std::uint64_t length = Read16(bytes, *at);
    *at += 2;
    if (length == 0xFFFFU) {
        if (*at + 4 > bytes.size()) return false;
        length = Read32LE(bytes, *at);
        *at += 4;
    }
    if (length > bytes.size() - *at) return false;
    out->assign(reinterpret_cast<const char*>(bytes.data() + *at),
                static_cast<std::size_t>(length));
    *at += static_cast<std::size_t>(length);
    return true;
}

void PreserveRemaining(const std::vector<std::uint8_t>& bytes, std::size_t at,
                       LoginResponse* result) {
    result->offset = at;
    if (at <= bytes.size()) {
        result->remaining_bytes.assign(bytes.begin() + static_cast<std::ptrdiff_t>(at),
                                       bytes.end());
    }
}

}  // namespace

LoginRequest BuildLoginRequest(const LoginRequestOptions& options,
                               const Rsa1024PublicKey& rsa_key) {
    return BuildLoginRequest(options, rsa_key, SecureRandomBytes);
}

LoginRequest BuildLoginRequest(const LoginRequestOptions& options,
                               const Rsa1024PublicKey& rsa_key,
                               const RandomBytesFunction& random_bytes) {
    LoginRequest result;
    if (options.account_id == 0) {
        result.error = LoginBuildError::InvalidAccountId;
        return result;
    }
    if (options.terminal_type > 2
        || options.terminal_version != kLoginProtocolVersion) {
        result.error = LoginBuildError::InvalidTerminal;
        return result;
    }
    if (options.password.size() > 29) {
        result.error = LoginBuildError::PasswordTooLong;
        return result;
    }
    if (!rsa_key.initialized() || !random_bytes) {
        result.error = LoginBuildError::InvalidArgument;
        return result;
    }

    result.xtea_key = XteaKey{};
    if (GenerateXteaKey(&result.xtea_key, random_bytes) != CryptoError::None) {
        result.error = LoginBuildError::RandomGenerationFailed;
        return result;
    }
    result.payload.fill(0);
    result.payload[0] = kLoginEnterAccountOpcode;
    Write16(result.payload.data() + 1, options.terminal_type);
    Write16(result.payload.data() + 3, options.terminal_version);
    Write32(result.payload.data() + 5, options.dat_signature);
    Write32(result.payload.data() + 9, options.spr_signature);
    Write32(result.payload.data() + 13, options.pic_signature);

    std::array<std::uint8_t, kRsa1024BlockBytes> rsa_plaintext{};
    std::array<std::uint8_t, kXteaKeyBytes> key_bytes{};
    if (result.xtea_key.SerializeLittleEndian(&key_bytes) != CryptoError::None) {
        result.error = LoginBuildError::RsaError;
        return result;
    }
    std::copy(key_bytes.begin(), key_bytes.end(), rsa_plaintext.begin() + 1);
    Write32(rsa_plaintext.data() + 17, options.account_id);
    Write16(rsa_plaintext.data() + 21,
            static_cast<std::uint16_t>(options.password.size()));
    std::copy(options.password.begin(), options.password.end(),
              rsa_plaintext.begin() + 23);

    const std::size_t used = 23 + options.password.size();
    bool padding_ok = true;
    if (used < rsa_plaintext.size()) {
        try {
            padding_ok = random_bytes(rsa_plaintext.data() + used,
                                      rsa_plaintext.size() - used);
        } catch (...) {
            padding_ok = false;
        }
    }
    if (!padding_ok) {
        result.error = LoginBuildError::RandomGenerationFailed;
        return result;
    }
    if (rsa_key.EncryptProtocolBlock(rsa_plaintext, &result.rsa_ciphertext) != CryptoError::None) {
        result.error = LoginBuildError::RsaError;
        return result;
    }
    std::copy(result.rsa_ciphertext.begin(), result.rsa_ciphertext.end(),
              result.payload.begin() + 17);
    const FrameEncodeResult framed = EncodeFrame(
        result.payload.data(), result.payload.size(), kLoginFrameLimits.max_send_payload);
    if (!framed.ok()) {
        result.error = LoginBuildError::FramingError;
        return result;
    }
    result.wire_bytes = framed.bytes;
    return result;
}

LoginResponse ParseLoginResponse(const XteaDecodeResult& decrypted) {
    LoginResponse result;
    if (!decrypted.ok()) {
        result.status = LoginParseStatus::CryptoError;
        result.crypto_error = decrypted.error;
        result.remaining_bytes = decrypted.input_payload;
        return result;
    }
    const auto& bytes = decrypted.message;
    std::size_t at = 0;
    bool got_characters = false;
    while (at < bytes.size()) {
        const std::size_t opcode_at = at;
        const std::uint8_t opcode = bytes[at++];
        if (opcode == kLoginErrorOpcode) {
            std::string message;
            if (!TakeString(bytes, &at, &message)) {
                result.status = LoginParseStatus::Incomplete;
                PreserveRemaining(bytes, opcode_at, &result);
                return result;
            }
            result.error_message = std::move(message);
            result.status = LoginParseStatus::ProtocolViolation;
            PreserveRemaining(bytes, at, &result);
            return result;
        }
        if (opcode == kLoginMotdOpcode) {
            std::string message;
            if (!TakeString(bytes, &at, &message)) {
                result.status = LoginParseStatus::Incomplete;
                PreserveRemaining(bytes, opcode_at, &result);
                return result;
            }
            result.motd = std::move(message);
            continue;
        }
        if (opcode == kLoginCharacterListOpcode) {
            if (got_characters || at >= bytes.size()) {
                result.status = LoginParseStatus::Malformed;
                PreserveRemaining(bytes, opcode_at, &result);
                return result;
            }
            const std::size_t count = bytes[at++];
            result.characters.reserve(count);
            for (std::size_t index = 0; index < count; ++index) {
                LoginCharacter character;
                if (!TakeString(bytes, &at, &character.name)
                    || !TakeString(bytes, &at, &character.world_name)
                    || at + 6 > bytes.size()) {
                    result.status = LoginParseStatus::Incomplete;
                    PreserveRemaining(bytes, opcode_at, &result);
                    return result;
                }
                character.world_address = Read32BE(bytes, at);
                at += 4;
                character.world_port = Read16(bytes, at);
                at += 2;
                result.characters.push_back(std::move(character));
            }
            if (at + 2 > bytes.size()) {
                result.status = LoginParseStatus::Incomplete;
                PreserveRemaining(bytes, opcode_at, &result);
                return result;
            }
            result.premium_days = Read16(bytes, at);
            at += 2;
            got_characters = true;
            continue;
        }
        result.status = LoginParseStatus::Unsupported;
        result.unsupported_opcode = opcode;
        PreserveRemaining(bytes, opcode_at, &result);
        return result;
    }
    result.status = got_characters ? LoginParseStatus::Decoded
                                   : LoginParseStatus::Incomplete;
    result.offset = at;
    return result;
}

const char* LoginBuildErrorName(LoginBuildError error) noexcept {
    switch (error) {
        case LoginBuildError::None: return "None";
        case LoginBuildError::InvalidArgument: return "InvalidArgument";
        case LoginBuildError::InvalidAccountId: return "InvalidAccountId";
        case LoginBuildError::InvalidTerminal: return "InvalidTerminal";
        case LoginBuildError::PasswordTooLong: return "PasswordTooLong";
        case LoginBuildError::RandomGenerationFailed: return "RandomGenerationFailed";
        case LoginBuildError::RsaError: return "RsaError";
        case LoginBuildError::FramingError: return "FramingError";
    }
    return "Unknown";
}

const char* LoginParseStatusName(LoginParseStatus status) noexcept {
    switch (status) {
        case LoginParseStatus::Decoded: return "Decoded";
        case LoginParseStatus::Incomplete: return "Incomplete";
        case LoginParseStatus::Unsupported: return "Unsupported";
        case LoginParseStatus::Malformed: return "Malformed";
        case LoginParseStatus::ProtocolViolation: return "ProtocolViolation";
        case LoginParseStatus::CryptoError: return "CryptoError";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
