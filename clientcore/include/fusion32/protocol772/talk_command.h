#ifndef FUSION32_PROTOCOL772_TALK_COMMAND_H
#define FUSION32_PROTOCOL772_TALK_COMMAND_H

#include <cstdint>
#include <string>
#include <vector>

#include "fusion32/protocol772/movement.h"

namespace fusion32::protocol772 {

// Building the client's own talk command.
//
// Source: reference/game/src/receiving.cc::CTalk, reached from
// receiving.cc::ReceiveData on CL_CMD_TALK. The layout is
//
//     byte    CL_CMD_TALK              150
//     byte    Mode
//     string  Addressee                only for the four addressed modes
//     word    Channel                  only for the three channel modes
//     string  Text
//
// `string` is the same encoding the server uses: a word length followed by that
// many bytes (reference/game/src/utils.cc::TReadStream::readString, which also
// accepts 0xFFFF as an escape introducing a quad length; nothing here needs it).
//
// THE ACCEPTED MODE SET IS NOT THE SERVER'S. CTalk's whitelist and the three
// SendTalk overloads do not agree, and the difference runs both ways:
//
//   * the client may send ANONYMOUS_BROADCAST (13) and ANONYMOUS_MESSAGE (15),
//     which no SendTalk overload accepts, so they can never come back;
//   * the client may not send HIGHLIGHT_CHANNELCALL (12), ANIMAL_LOW (16) or
//     ANIMAL_LOUD (17), which only the server originates.
//
// Reusing the incoming table here would therefore be wrong in both directions.

// The modes CTalk's whitelist accepts.
bool IsClientTalkMode(std::uint8_t mode) noexcept;

// Which extra field the mode requires, decided exactly as CTalk decides it.
bool TalkModeNeedsAddressee(std::uint8_t mode) noexcept;
bool TalkModeNeedsChannel(std::uint8_t mode) noexcept;

// Why a command was refused before it reached the wire.
//
// Each of these mirrors a check CTalk performs. Refusing here rather than
// sending means the player is told, instead of the server discarding the
// command with a line in a log the player cannot see.
enum class TalkBuildError {
    None,
    // CTalk's whitelist rejects the mode.
    UnsupportedMode,
    // CTalk: `if(Text[0] == 0)` ... "Kein Text."
    EmptyText,
    // char Text[256], and readString truncates rather than refusing, so an
    // over-long message would arrive silently cut. Refused here instead.
    TextTooLong,
    // CTalk: `if(findFirst(Text, '\n') != NULL)` is an error() for the server.
    TextContainsNewline,
    // CTalk: `if(Addressee[0] == 0)` ... "Adressat nicht angegeben."
    MissingAddressee,
    // char Addressee[30], same truncation reasoning as the text.
    AddresseeTooLong,
};

const char* TalkBuildErrorName(TalkBuildError error) noexcept;

// reference/game/src/receiving.cc::CTalk, `char Text[256]` and
// `char Addressee[30]`, both minus the terminator readString writes.
constexpr std::size_t kMaxTalkTextLength = 255;
constexpr std::size_t kMaxTalkAddresseeLength = 29;

struct TalkCommandResult {
    std::vector<std::uint8_t> payload;
    TalkBuildError error = TalkBuildError::None;

    bool ok() const noexcept { return error == TalkBuildError::None; }
};

// Builds any client talk command. `addressee` and `channel` are ignored for
// modes that do not carry them, so a caller cannot smuggle a field the server
// will not read.
//
// The channel number is deliberately not validated: CTalk checks it against
// GetNumberOfChannels(), which is server state this client has no copy of.
// Sending an out-of-range channel is refused by the server, not here.
TalkCommandResult BuildTalkCommand(std::uint8_t mode,
                                   const std::string& text,
                                   const std::string& addressee = std::string(),
                                   std::uint16_t channel = 0);

// The ordinary case: speak out loud where you stand.
TalkCommandResult BuildSayCommand(const std::string& text);

}  // namespace fusion32::protocol772

#endif
