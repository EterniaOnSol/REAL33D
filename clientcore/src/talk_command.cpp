#include "fusion32/protocol772/talk_command.h"

#include <algorithm>

namespace fusion32::protocol772 {
namespace {

void AppendWord(std::vector<std::uint8_t>* out, std::uint16_t value) {
    out->push_back(static_cast<std::uint8_t>(value & 0xFF));
    out->push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

// The client half of SendString: word length, then the bytes.
void AppendString(std::vector<std::uint8_t>* out, const std::string& value) {
    AppendWord(out, static_cast<std::uint16_t>(value.size()));
    out->insert(out->end(), value.begin(), value.end());
}

}  // namespace

bool IsClientTalkMode(std::uint8_t mode) noexcept {
    // reference/game/src/receiving.cc::CTalk, in the order it tests them.
    switch (static_cast<TalkMode>(mode)) {
        case TalkMode::Say:
        case TalkMode::Whisper:
        case TalkMode::Yell:
        case TalkMode::PrivateMessage:
        case TalkMode::ChannelCall:
        case TalkMode::GamemasterRequest:
        case TalkMode::GamemasterAnswer:
        case TalkMode::PlayerAnswer:
        case TalkMode::GamemasterBroadcast:
        case TalkMode::GamemasterChannelCall:
        case TalkMode::GamemasterMessage:
        case TalkMode::AnonymousChannelCall:
            return true;
        // Declared in enums.hh and accepted by CTalk, but by no SendTalk
        // overload, so they are send-only. TalkMode does not name them because
        // the incoming decoder can never see them.
        case TalkMode::HighlightChannelCall:
        case TalkMode::AnimalLow:
        case TalkMode::AnimalLoud:
            return false;
    }
    // TALK_ANONYMOUS_BROADCAST 13 and TALK_ANONYMOUS_MESSAGE 15 are in CTalk's
    // whitelist but absent from TalkMode, which only names what can arrive.
    return mode == 13 || mode == 15;
}

bool TalkModeNeedsAddressee(std::uint8_t mode) noexcept {
    // CTalk: PRIVATE_MESSAGE, GAMEMASTER_ANSWER, GAMEMASTER_MESSAGE,
    // ANONYMOUS_MESSAGE.
    return mode == static_cast<std::uint8_t>(TalkMode::PrivateMessage)
        || mode == static_cast<std::uint8_t>(TalkMode::GamemasterAnswer)
        || mode == static_cast<std::uint8_t>(TalkMode::GamemasterMessage)
        || mode == 15;
}

bool TalkModeNeedsChannel(std::uint8_t mode) noexcept {
    // CTalk: CHANNEL_CALL, GAMEMASTER_CHANNELCALL, ANONYMOUS_CHANNELCALL.
    return mode == static_cast<std::uint8_t>(TalkMode::ChannelCall)
        || mode == static_cast<std::uint8_t>(TalkMode::GamemasterChannelCall)
        || mode == static_cast<std::uint8_t>(TalkMode::AnonymousChannelCall);
}

TalkCommandResult BuildTalkCommand(std::uint8_t mode,
                                   const std::string& text,
                                   const std::string& addressee,
                                   std::uint16_t channel) {
    TalkCommandResult result;

    if (!IsClientTalkMode(mode)) {
        result.error = TalkBuildError::UnsupportedMode;
        return result;
    }
    if (text.empty()) {
        result.error = TalkBuildError::EmptyText;
        return result;
    }
    if (text.size() > kMaxTalkTextLength) {
        result.error = TalkBuildError::TextTooLong;
        return result;
    }
    if (text.find('\n') != std::string::npos) {
        result.error = TalkBuildError::TextContainsNewline;
        return result;
    }

    const bool needs_addressee = TalkModeNeedsAddressee(mode);
    if (needs_addressee) {
        if (addressee.empty()) {
            result.error = TalkBuildError::MissingAddressee;
            return result;
        }
        if (addressee.size() > kMaxTalkAddresseeLength) {
            result.error = TalkBuildError::AddresseeTooLong;
            return result;
        }
    }

    result.payload.push_back(kClientCommandTalk);
    result.payload.push_back(mode);
    if (needs_addressee) {
        AppendString(&result.payload, addressee);
    }
    if (TalkModeNeedsChannel(mode)) {
        AppendWord(&result.payload, channel);
    }
    AppendString(&result.payload, text);
    return result;
}

TalkCommandResult BuildSayCommand(const std::string& text) {
    return BuildTalkCommand(static_cast<std::uint8_t>(TalkMode::Say), text);
}

const char* TalkBuildErrorName(TalkBuildError error) noexcept {
    switch (error) {
        case TalkBuildError::None: return "None";
        case TalkBuildError::UnsupportedMode: return "UnsupportedMode";
        case TalkBuildError::EmptyText: return "EmptyText";
        case TalkBuildError::TextTooLong: return "TextTooLong";
        case TalkBuildError::TextContainsNewline: return "TextContainsNewline";
        case TalkBuildError::MissingAddressee: return "MissingAddressee";
        case TalkBuildError::AddresseeTooLong: return "AddresseeTooLong";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
