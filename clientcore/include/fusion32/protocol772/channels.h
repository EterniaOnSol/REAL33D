#ifndef FUSION32_PROTOCOL772_CHANNELS_H
#define FUSION32_PROTOCOL772_CHANNELS_H

// Channels: the four client commands and the five server commands that open,
// list, close and privately address a 7.72 chat channel.
//
// Source for every layout here:
//   reference/game/src/receiving.cc::CGetChannels, CJoinChannel,
//     CLeaveChannel, CPrivateChannel
//   reference/game/src/sending.cc::SendChannels, SendOpenChannel,
//     SendPrivateChannel, SendOpenOwnChannel, SendCloseChannel
//   reference/game/src/connections.hh::ClientCommand, ServerCommand
//
// This layer opens and closes channels. The speech that travels inside one
// is SV_CMD_TALK and CL_CMD_TALK, already covered by CHAT-772-001.
//
// Sits at the player_state.h layer and does not include movement.h, which
// includes it. Every builder here takes scalars, so unlike trade none of
// them has to be declared elsewhere.

#include "fusion32/protocol772/map_scan.h"
#include "fusion32/protocol772/worldstate.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

// Source: reference/game/src/connections.hh::ClientCommand.
constexpr std::uint8_t kClientCommandGetChannels = 151;
constexpr std::uint8_t kClientCommandJoinChannel = 152;
constexpr std::uint8_t kClientCommandLeaveChannel = 153;
constexpr std::uint8_t kClientCommandOpenPrivateChannel = 154;

// Source: reference/game/src/connections.hh::ServerCommand.
constexpr std::uint8_t kServerCommandChannels = 171;
constexpr std::uint8_t kServerCommandOpenChannel = 172;
constexpr std::uint8_t kServerCommandPrivateChannel = 173;
constexpr std::uint8_t kServerCommandOpenOwnChannel = 178;
constexpr std::uint8_t kServerCommandCloseChannel = 179;

/**
 * Not a channel number. SendChannels writes this id, followed by the name
 * "Private Chat Channel", as the first entry of the list when
 * `MayOpenChannel(Player->ID)` is true. It is an offer to create a channel,
 * not a channel that exists, and joining it would be meaningless: every real
 * channel satisfies `Channel < GetNumberOfChannels()`.
 *
 * The decoder flags such an entry rather than reporting channel 65535.
 */
constexpr std::uint16_t kOwnChannelOfferId = 0xFFFF;

/**
 * Source: reference/game/src/operate.hh::CHANNEL_RULEVIOLATIONS.
 *
 * Joining this one does not answer with SV_CMD_OPEN_CHANNEL. CJoinChannel
 * branches to SendOpenRequestQueue (174) and SendExistingRequests instead,
 * and neither is decoded by this project. A caller that joins channel 3 will
 * therefore see no channel open.
 */
constexpr std::uint16_t kChannelRuleViolations = 3;

/**
 * CPrivateChannel reads into `char Name[30]` with
 * `Buffer->readString(Name, sizeof(Name))`, and TReadStream::readString
 * keeps `MaxLength - 1` bytes and skips the rest. A longer name is therefore
 * silently truncated to 29 characters by the server, not refused.
 */
constexpr std::size_t kMaxPrivateChannelNameLength = 29;

// One row of SV_CMD_CHANNELS.
struct ChannelEntry {
    std::uint16_t id = 0;
    std::string name;
    // True for the kOwnChannelOfferId row. `id` is then meaningless.
    bool own_channel_offer = false;
};

// Which of the five channel commands arrived.
enum class ChannelUpdateKind {
    // SV_CMD_CHANNELS: the whole list, in answer to CL_CMD_GET_CHANNELS.
    List,
    // SV_CMD_OPEN_CHANNEL: a channel the player joined.
    Opened,
    // SV_CMD_OPEN_OWN_CHANNEL: same shape, but this one is the player's own.
    OpenedOwn,
    // SV_CMD_PRIVATE_CHANNEL: a private conversation with one named
    // character. Carries no channel number at all.
    PrivateOpened,
    // SV_CMD_CLOSE_CHANNEL: a channel number, nothing else.
    Closed,
};

/**
 * Source: SendChannels, SendOpenChannel, SendPrivateChannel,
 * SendOpenOwnChannel and SendCloseChannel.
 *
 * `channels` is filled for List only. `channel` is meaningful for Opened,
 * OpenedOwn and Closed. `name` is filled for Opened, OpenedOwn and
 * PrivateOpened -- and PrivateOpened is the one command that has a name
 * without a number, because a private conversation is addressed by character
 * rather than by channel.
 */
struct ChannelUpdate {
    ChannelUpdateKind kind = ChannelUpdateKind::Closed;
    std::vector<ChannelEntry> channels;
    std::uint16_t channel = 0;
    std::string name;
};

// ------------------------------------------------------------------ builders

/**
 * Asks for the channel list. CL_CMD_GET_CHANNELS, opcode 151.
 *
 * No payload: CGetChannels reads nothing from the buffer and answers with
 * SendChannels immediately.
 */
std::vector<std::uint8_t> BuildGetChannelsCommand();

/**
 * Joins a channel. CL_CMD_JOIN_CHANNEL, opcode 152, `word Channel`.
 *
 * CJoinChannel silently ignores a channel at or beyond
 * `GetNumberOfChannels()`, and the count is runtime server state this layer
 * cannot know, so no range is enforced here. The answer is
 * SV_CMD_OPEN_CHANNEL, or SV_CMD_OPEN_OWN_CHANNEL when `JoinChannel`
 * reports the channel belongs to this player, or -- for
 * kChannelRuleViolations -- the undecoded request queue.
 */
std::vector<std::uint8_t> BuildJoinChannelCommand(std::uint16_t channel);

/** Leaves a channel. CL_CMD_LEAVE_CHANNEL, opcode 153, `word Channel`.
 *  CLeaveChannel sends nothing back; the channel is simply left. */
std::vector<std::uint8_t> BuildLeaveChannelCommand(std::uint16_t channel);

/**
 * Opens a private conversation with a character by name.
 * CL_CMD_PRIVATE_CHANNEL, opcode 154, `string Name`.
 *
 * The string is the plain `word length` + bytes form. The generic
 * TReadStream::readString the server uses also understands a 0xFFFF escape
 * followed by a quad length, but no name needs it and emitting it would make
 * the frame ambiguous, so this builder never produces one: a name is clamped
 * to 0xFFFE bytes before the length is written. Nothing truncates at 29
 * here -- the server does that itself, and refusing locally would invent a
 * rule Fusion32 does not apply.
 *
 * An empty name is returned unsent by CPrivateChannel (`if(Name[0] == 0)`),
 * and an unknown one answers PLAYERNOTEXISTING through SendResult.
 */
std::vector<std::uint8_t> BuildOpenPrivateChannelCommand(const std::string& name);

// ------------------------------------------------------------------- decoder

/**
 * Decodes one of the five channel commands, with the scanner positioned just
 * past the opcode. Reports failure through the scanner, so a truncated
 * message is an explicit MapDecodeError and never a half-filled list.
 */
bool DecodeChannel(MapScanner* scanner, std::uint8_t opcode, ChannelUpdate* output);

}  // namespace fusion32::protocol772

#endif
