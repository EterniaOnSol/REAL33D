#ifndef FUSION32_PROTOCOL772_GAMELOGIN_H
#define FUSION32_PROTOCOL772_GAMELOGIN_H

#include "fusion32/protocol772/framed_connection.h"
#include "fusion32/protocol772/login.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

constexpr std::uint8_t kGameLoginRequestOpcode = 10;
constexpr std::uint8_t kGameInitOpcode = 10;
constexpr std::uint8_t kGameRightsOpcode = 11;
constexpr std::uint8_t kGameLoginErrorOpcode = 20;
constexpr std::uint8_t kGameLoginPremiumOpcode = 21;
constexpr std::uint8_t kGameLoginWaitingListOpcode = 22;
constexpr std::uint8_t kGamePingOpcode = 30;
constexpr std::uint8_t kGameFullScreenOpcode = 100;

struct GameLoginRequestOptions {
    std::uint32_t account_id = 0;
    std::string character_name;
    std::string password;
    bool gamemaster_client = false;
    std::uint16_t terminal_type = 1;
    std::uint16_t terminal_version = kLoginProtocolVersion;
};

enum class GameLoginBuildError {
    None,
    InvalidArgument,
    InvalidAccountId,
    InvalidTerminal,
    CharacterNameTooLong,
    PasswordTooLong,
    RandomGenerationFailed,
    RsaError,
    FramingError,
};

struct GameLoginRequest {
    std::vector<std::uint8_t> payload;
    std::vector<std::uint8_t> wire_bytes;
    std::array<std::uint8_t, kRsa1024BlockBytes> rsa_ciphertext{};
    XteaKey xtea_key;
    GameLoginBuildError error = GameLoginBuildError::None;

    bool ok() const noexcept { return error == GameLoginBuildError::None; }
};

GameLoginRequest BuildGameLoginRequest(
    const GameLoginRequestOptions& options,
    const Rsa1024PublicKey& rsa_key);

GameLoginRequest BuildGameLoginRequest(
    const GameLoginRequestOptions& options,
    const Rsa1024PublicKey& rsa_key,
    const RandomBytesFunction& random_bytes);

enum class GameMessageStatus {
    InitGame,
    Rights,
    LoginError,
    PremiumNotice,
    WaitingList,
    Ping,
    FullScreenUnparsed,
    Unsupported,
    Incomplete,
    Malformed,
    CryptoError,
};

struct GameInitialMessage {
    GameMessageStatus status = GameMessageStatus::Malformed;
    std::uint32_t creature_id = 0;
    std::uint16_t beat = 0;
    bool bug_reports = false;
    bool init_game_received = false;
    bool rights_received = false;
    std::vector<std::uint8_t> rights;
    std::string text;
    std::uint8_t wait_seconds = 0;
    std::vector<std::uint8_t> message_bytes;
    std::vector<std::uint8_t> unparsed_bytes;
    CryptoError crypto_error = CryptoError::None;

    bool authenticated() const noexcept { return init_game_received; }
};

GameInitialMessage ParseGameInitialMessage(const XteaDecodeResult& decrypted);

class GameLoginSession {
public:
    explicit GameLoginSession(
        std::size_t read_chunk_size = 4096,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));

    ConnectResult Connect(const std::string& host, std::uint16_t port = 7172);
    // See TcpTransport::SetReadTimeout. An interactive caller wants this much
    // shorter than the connect timeout, because it bounds how long the session
    // sits inside a read before the caller gets control back.
    bool SetReadTimeout(std::chrono::milliseconds timeout) {
        return connection_.transport().SetReadTimeout(timeout);
    }

    void Disconnect() noexcept;
    FramedWriteResult SendLogin(const GameLoginRequest& request);

    // Sends an authenticated client command. Source:
    // reference/game/src/communication.cc::ReceiveCommand, which for a
    // connection past CONNECTION_CONNECTED requires the outer size to be a
    // multiple of eight and decrypts it as XTEA blocks carrying a leading
    // little-endian payload length.
    FramedWriteResult SendCommand(const std::vector<std::uint8_t>& payload);
    struct ReadResult {
        FramedReadResult transport;
        GameInitialMessage message;

        bool ok() const noexcept {
            return transport.ok() && message.status != GameMessageStatus::CryptoError;
        }
    };

    ReadResult ReadNextMessage();

    bool connected() const noexcept { return connection_.transport().connected(); }
    bool authenticated() const noexcept { return authenticated_; }
    const XteaKey& xtea_key() const noexcept { return xtea_key_; }

private:
    FramedConnection connection_;
    std::chrono::milliseconds timeout_;
    XteaKey xtea_key_;
    bool authenticated_ = false;
    std::deque<FramedPacket> pending_frames_;
};

const char* GameLoginBuildErrorName(GameLoginBuildError error) noexcept;
const char* GameMessageStatusName(GameMessageStatus status) noexcept;

}  // namespace fusion32::protocol772

#endif
