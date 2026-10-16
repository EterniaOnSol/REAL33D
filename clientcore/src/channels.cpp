#include "fusion32/protocol772/channels.h"

#include <algorithm>

namespace fusion32::protocol772 {

namespace {

void AppendWord(std::vector<std::uint8_t>* command, std::uint16_t value) {
    command->push_back(static_cast<std::uint8_t>(value & 0xFF));
    command->push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

}  // namespace

std::vector<std::uint8_t> BuildGetChannelsCommand() {
    return {kClientCommandGetChannels};
}

std::vector<std::uint8_t> BuildJoinChannelCommand(std::uint16_t channel) {
    std::vector<std::uint8_t> command;
    command.reserve(3);
    command.push_back(kClientCommandJoinChannel);
    AppendWord(&command, channel);
    return command;
}

std::vector<std::uint8_t> BuildLeaveChannelCommand(std::uint16_t channel) {
    std::vector<std::uint8_t> command;
    command.reserve(3);
    command.push_back(kClientCommandLeaveChannel);
    AppendWord(&command, channel);
    return command;
}

std::vector<std::uint8_t> BuildOpenPrivateChannelCommand(const std::string& name) {
    // 0xFFFF is the escape TReadStream::readString takes as "a quad length
    // follows". Writing it as a plain length would desynchronise the
    // server's reader, so the only length this builder can never emit is
    // that one. No real character name comes close.
    const std::size_t length = std::min<std::size_t>(name.size(), 0xFFFEu);

    std::vector<std::uint8_t> command;
    command.reserve(3 + length);
    command.push_back(kClientCommandOpenPrivateChannel);
    AppendWord(&command, static_cast<std::uint16_t>(length));
    for (std::size_t index = 0; index < length; ++index) {
        command.push_back(static_cast<std::uint8_t>(name[index]));
    }
    return command;
}

bool DecodeChannel(MapScanner* scanner, std::uint8_t opcode, ChannelUpdate* output) {
    output->channels.clear();
    output->channel = 0;
    output->name.clear();

    switch (opcode) {
        case kServerCommandCloseChannel:
            // SendCloseChannel writes the opcode and the number, nothing else.
            output->kind = ChannelUpdateKind::Closed;
            return ReadScannerWord(scanner, &output->channel, "closed channel");

        case kServerCommandOpenChannel:
        case kServerCommandOpenOwnChannel:
            // SendOpenChannel and SendOpenOwnChannel have the identical body;
            // only the opcode says whose channel it is.
            output->kind = opcode == kServerCommandOpenChannel
                               ? ChannelUpdateKind::Opened
                               : ChannelUpdateKind::OpenedOwn;
            if (!ReadScannerWord(scanner, &output->channel, "opened channel")) return false;
            return ReadScannerString(scanner, &output->name, "channel name");

        case kServerCommandPrivateChannel:
            // SendPrivateChannel writes a name and no number: a private
            // conversation is addressed by character, not by channel.
            output->kind = ChannelUpdateKind::PrivateOpened;
            return ReadScannerString(scanner, &output->name, "private channel name");

        case kServerCommandChannels:
            break;

        default:
            return FailScanner(scanner, MapDecodeError::UnexpectedCommand,
                               "not a channel command");
    }

    // SendChannels: a count, then that many word/string pairs. The count is
    // a byte, which is the ceiling SendChannels itself clamps to, so there
    // is nothing to reject here -- only every row to actually read.
    output->kind = ChannelUpdateKind::List;
    std::uint8_t count = 0;
    if (!ReadScannerByte(scanner, &count, "channel count")) return false;

    output->channels.reserve(count);
    for (std::uint8_t index = 0; index < count; ++index) {
        ChannelEntry entry;
        if (!ReadScannerWord(scanner, &entry.id, "channel id")) return false;
        if (!ReadScannerString(scanner, &entry.name, "channel name")) return false;
        // The one row that is an offer rather than a channel. SendChannels
        // emits it first when MayOpenChannel is true.
        entry.own_channel_offer = entry.id == kOwnChannelOfferId;
        output->channels.push_back(std::move(entry));
    }
    return true;
}

}  // namespace fusion32::protocol772
