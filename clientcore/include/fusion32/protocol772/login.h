#ifndef FUSION32_PROTOCOL772_LOGIN_H
#define FUSION32_PROTOCOL772_LOGIN_H

#include "fusion32/protocol772/crypto.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

constexpr std::uint16_t kLoginProtocolVersion = 772;
constexpr std::uint8_t kLoginEnterAccountOpcode = 1;
constexpr std::uint8_t kLoginErrorOpcode = 10;
constexpr std::uint8_t kLoginMotdOpcode = 20;
constexpr std::uint8_t kLoginCharacterListOpcode = 100;

struct LoginFrameLimits {
    std::size_t max_receive_payload;
    std::size_t max_send_payload;
};

constexpr LoginFrameLimits kLoginFrameLimits{2046, 2048};

enum class LoginBuildError {
    None,
    InvalidArgument,
    InvalidAccountId,
    InvalidTerminal,
    PasswordTooLong,
    RandomGenerationFailed,
    RsaError,
    FramingError,
};

struct LoginRequestOptions {
    std::uint32_t account_id = 0;
    std::string password;
    std::uint16_t terminal_type = 1;
    std::uint16_t terminal_version = kLoginProtocolVersion;
    std::uint32_t dat_signature = 0;
    std::uint32_t spr_signature = 0;
    std::uint32_t pic_signature = 0;
};

struct LoginRequest {
    std::array<std::uint8_t, 145> payload{};
    std::array<std::uint8_t, kRsa1024BlockBytes> rsa_ciphertext{};
    std::vector<std::uint8_t> wire_bytes;
    XteaKey xtea_key;
    LoginBuildError error = LoginBuildError::None;

    bool ok() const noexcept { return error == LoginBuildError::None; }
};

LoginRequest BuildLoginRequest(
    const LoginRequestOptions& options,
    const Rsa1024PublicKey& rsa_key);

LoginRequest BuildLoginRequest(
    const LoginRequestOptions& options,
    const Rsa1024PublicKey& rsa_key,
    const RandomBytesFunction& random_bytes);

enum class LoginParseStatus {
    Decoded,
    Incomplete,
    Unsupported,
    Malformed,
    ProtocolViolation,
    CryptoError,
};

struct LoginCharacter {
    std::string name;
    std::string world_name;
    std::uint32_t world_address = 0;
    std::uint16_t world_port = 0;
};

struct LoginResponse {
    LoginParseStatus status = LoginParseStatus::Malformed;
    std::optional<std::string> error_message;
    std::optional<std::string> motd;
    std::vector<LoginCharacter> characters;
    std::uint16_t premium_days = 0;
    std::size_t offset = 0;
    std::uint8_t unsupported_opcode = 0;
    std::vector<std::uint8_t> remaining_bytes;
    CryptoError crypto_error = CryptoError::None;

    bool ok() const noexcept { return status == LoginParseStatus::Decoded; }
};

LoginResponse ParseLoginResponse(const XteaDecodeResult& decrypted);

const char* LoginBuildErrorName(LoginBuildError error) noexcept;
const char* LoginParseStatusName(LoginParseStatus status) noexcept;

}  // namespace fusion32::protocol772

#endif
