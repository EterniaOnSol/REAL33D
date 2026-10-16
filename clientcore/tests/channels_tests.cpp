// CLIENTCORE-CHANNELS-772-001.
//
// Both directions of 7.72 channel membership: the four commands the client
// builds and the five the server sends.
//
// The server emitter below is a literal port of
// reference/game/src/sending.cc::SendChannels, SendOpenChannel,
// SendPrivateChannel, SendOpenOwnChannel and SendCloseChannel, asserted
// against hand-computed golden hex before it is used to build anything else.

#include "fusion32/protocol772/channels.h"
#include "fusion32/protocol772/map_scan.h"
#include "fusion32/protocol772/movement.h"

#include "fixtures/fullscreen_772_vectors.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace fusion32::protocol772;
namespace vectors = fusion32::protocol772::test_vectors;

class Failure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#define CHECK(condition) do { if (!(condition)) throw Failure(#condition); } while (false)

const ObjectTypeTable& Types() {
    static const ObjectTypeTable table = vectors::GoldenObjectTypeTable();
    return table;
}

const MapPosition kAnchor{32097, 32219, 7};

ServerUpdateDecodeResult Decode(const std::vector<std::uint8_t>& bytes) {
    return DecodeServerUpdate(bytes, 0, kAnchor, Types());
}

std::string Hex(const std::vector<std::uint8_t>& bytes) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (const std::uint8_t byte : bytes) {
        out.push_back(digits[byte >> 4]);
        out.push_back(digits[byte & 0x0F]);
    }
    return out;
}

// ------------------------------------------------------- server-side emitter

class ChannelEncoder {
public:
    // SendChannels. `rows` is already what the server decided to send,
    // including the leading kOwnChannelOfferId row when MayOpenChannel is
    // true, because that decision is server state this port does not model.
    static std::vector<std::uint8_t> List(const std::vector<ChannelEntry>& rows) {
        ChannelEncoder encoder;
        encoder.Byte(kServerCommandChannels);
        encoder.Byte(static_cast<std::uint8_t>(rows.size()));
        for (const ChannelEntry& row : rows) {
            encoder.Word(row.id);
            encoder.Str(row.name);
        }
        return encoder.out_;
    }

    // SendOpenChannel and SendOpenOwnChannel: identical bodies.
    static std::vector<std::uint8_t> Open(bool own, std::uint16_t channel,
                                          const std::string& name) {
        ChannelEncoder encoder;
        encoder.Byte(own ? kServerCommandOpenOwnChannel : kServerCommandOpenChannel);
        encoder.Word(channel);
        encoder.Str(name);
        return encoder.out_;
    }

    // SendPrivateChannel: a name and no channel number.
    static std::vector<std::uint8_t> Private(const std::string& name) {
        ChannelEncoder encoder;
        encoder.Byte(kServerCommandPrivateChannel);
        encoder.Str(name);
        return encoder.out_;
    }

    static std::vector<std::uint8_t> Close(std::uint16_t channel) {
        ChannelEncoder encoder;
        encoder.Byte(kServerCommandCloseChannel);
        encoder.Word(channel);
        return encoder.out_;
    }

private:
    void Byte(std::uint8_t value) { out_.push_back(value); }

    void Word(std::uint16_t value) {
        out_.push_back(static_cast<std::uint8_t>(value));
        out_.push_back(static_cast<std::uint8_t>(value >> 8U));
    }

    // Mirrors sending.cc::SendString, which writes a plain word length and
    // has no 0xFFFF escape. Only the generic TWriteStream::writeString has
    // one, and the game protocol never uses it on this path.
    void Str(const std::string& value) {
        Word(static_cast<std::uint16_t>(value.size()));
        for (const char character : value) {
            out_.push_back(static_cast<std::uint8_t>(character));
        }
    }

    std::vector<std::uint8_t> out_;
};

ChannelEntry Row(std::uint16_t id, const std::string& name) {
    ChannelEntry entry;
    entry.id = id;
    entry.name = name;
    return entry;
}

// ------------------------------------------------------------ golden: client

void GoldenClientCommands() {
    // CGetChannels reads nothing at all.
    {
        const auto command = BuildGetChannelsCommand();
        CHECK(Hex(command) == "97");
        CHECK(command.size() == 1);
        CHECK(command[0] == kClientCommandGetChannels);
    }

    // CJoinChannel and CLeaveChannel each read one word.
    {
        CHECK(Hex(BuildJoinChannelCommand(5)) == "980500");
        CHECK(Hex(BuildLeaveChannelCommand(3)) == "990300");
        CHECK(BuildJoinChannelCommand(5).size() == 3);
        CHECK(BuildLeaveChannelCommand(3).size() == 3);
        // Little-endian, and the full range is expressible.
        CHECK(Hex(BuildJoinChannelCommand(0)) == "980000");
        CHECK(Hex(BuildJoinChannelCommand(0xFFFF)) == "98ffff");
        CHECK(Hex(BuildJoinChannelCommand(0x1234)) == "983412");
    }

    // CPrivateChannel reads a string: word length, then the bytes.
    {
        CHECK(Hex(BuildOpenPrivateChannelCommand("Bob")) == "9a0300426f62");
        // An empty name is expressible. CPrivateChannel returns without
        // answering, which is the server's business, not the builder's.
        CHECK(Hex(BuildOpenPrivateChannelCommand("")) == "9a0000");
    }

    // The server truncates to 29 characters rather than refusing, so the
    // builder sends what it was given and does not invent a local rule.
    {
        const std::string long_name(40, 'a');
        const auto command = BuildOpenPrivateChannelCommand(long_name);
        CHECK(command.size() == 3 + 40);
        CHECK(command[1] == 40 && command[2] == 0);
        CHECK(long_name.size() > kMaxPrivateChannelNameLength);
    }

    // The one length this builder must never write is 0xFFFF, which
    // TReadStream::readString takes as "a quad length follows". A name that
    // long cannot exist, but emitting it would desynchronise the server's
    // reader, so the clamp is asserted rather than assumed.
    {
        const std::string absurd(0x10000, 'a');
        const auto command = BuildOpenPrivateChannelCommand(absurd);
        const std::uint16_t length =
            static_cast<std::uint16_t>(command[1] | (command[2] << 8));
        CHECK(length == 0xFFFE);
        CHECK(command.size() == 3u + 0xFFFEu);
    }

    std::cout << "  client command golden bytes: PASS\n";
}

// ------------------------------------------------------------ golden: server

void GoldenServerCommands() {
    // SendChannels with the own-channel offer first, exactly as it writes it
    // when MayOpenChannel(Player->ID) is true:
    //   ab | 02 | ffff 1400 "Private Chat Channel" | 0300 0400 "Help"
    const auto list = ChannelEncoder::List(
        {Row(kOwnChannelOfferId, "Private Chat Channel"), Row(3, "Help")});
    CHECK(Hex(list)
          == "ab02ffff1400507269766174652043686174204368616e6e656c0300040048656c70");

    //   ac | 0300 | 0400 "Help"
    const auto open = ChannelEncoder::Open(false, 3, "Help");
    CHECK(Hex(open) == "ac0300040048656c70");

    //   b2 | 0700 | 0300 "Bob"
    const auto open_own = ChannelEncoder::Open(true, 7, "Bob");
    CHECK(Hex(open_own) == "b207000300426f62");

    //   ad | 0300 "Bob"   -- a name and no channel number
    const auto private_channel = ChannelEncoder::Private("Bob");
    CHECK(Hex(private_channel) == "ad0300426f62");

    //   b3 | 0700
    const auto close = ChannelEncoder::Close(7);
    CHECK(Hex(close) == "b30700");

    // Through the real dispatcher, so opcode routing is proven too.
    {
        const auto decoded = Decode(list);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Channel);
        CHECK(std::string(decoded.update.name) == "SV_CMD_CHANNELS");
        CHECK(decoded.update.bytes_consumed == list.size());
        const ChannelUpdate& update = decoded.update.channel;
        CHECK(update.kind == ChannelUpdateKind::List);
        CHECK(update.channels.size() == 2);
        // The 0xFFFF row is an offer to create a channel, not channel 65535.
        CHECK(update.channels[0].own_channel_offer);
        CHECK(update.channels[0].name == "Private Chat Channel");
        CHECK(!update.channels[1].own_channel_offer);
        CHECK(update.channels[1].id == 3);
        CHECK(update.channels[1].name == "Help");
    }

    {
        const auto decoded = Decode(open);
        CHECK(decoded.ok());
        CHECK(std::string(decoded.update.name) == "SV_CMD_OPEN_CHANNEL");
        CHECK(decoded.update.channel.kind == ChannelUpdateKind::Opened);
        CHECK(decoded.update.channel.channel == 3);
        CHECK(decoded.update.channel.name == "Help");
        CHECK(decoded.update.channel.channels.empty());
    }

    {
        const auto decoded = Decode(open_own);
        CHECK(decoded.ok());
        CHECK(std::string(decoded.update.name) == "SV_CMD_OPEN_OWN_CHANNEL");
        // Same body as SV_CMD_OPEN_CHANNEL; only the opcode says whose it is.
        CHECK(decoded.update.channel.kind == ChannelUpdateKind::OpenedOwn);
        CHECK(decoded.update.channel.channel == 7);
        CHECK(decoded.update.channel.name == "Bob");
    }

    {
        const auto decoded = Decode(private_channel);
        CHECK(decoded.ok());
        CHECK(std::string(decoded.update.name) == "SV_CMD_PRIVATE_CHANNEL");
        CHECK(decoded.update.channel.kind == ChannelUpdateKind::PrivateOpened);
        CHECK(decoded.update.channel.name == "Bob");
        // Addressed by character, so there is no number to report.
        CHECK(decoded.update.channel.channel == 0);
    }

    {
        const auto decoded = Decode(close);
        CHECK(decoded.ok());
        CHECK(std::string(decoded.update.name) == "SV_CMD_CLOSE_CHANNEL");
        CHECK(decoded.update.channel.kind == ChannelUpdateKind::Closed);
        CHECK(decoded.update.channel.channel == 7);
        CHECK(decoded.update.channel.name.empty());
    }

    std::cout << "  server command golden bytes and decode: PASS\n";
}

void ListEdgeCases() {
    // No channels available at all: count zero, no rows.
    const auto empty = ChannelEncoder::List({});
    CHECK(Hex(empty) == "ab00");
    const auto empty_decoded = Decode(empty);
    CHECK(empty_decoded.ok());
    CHECK(empty_decoded.update.channel.kind == ChannelUpdateKind::List);
    CHECK(empty_decoded.update.channel.channels.empty());

    // 0xFFFE is a number, not the offer sentinel. Only 0xFFFF is.
    const auto near_sentinel = ChannelEncoder::List({Row(0xFFFE, "x")});
    const auto near_decoded = Decode(near_sentinel);
    CHECK(near_decoded.ok());
    CHECK(!near_decoded.update.channel.channels[0].own_channel_offer);
    CHECK(near_decoded.update.channel.channels[0].id == 0xFFFE);

    // An empty channel name is what SendString writes for a zero-length
    // string, and is well-formed.
    const auto unnamed = ChannelEncoder::List({Row(1, "")});
    CHECK(Hex(unnamed) == "ab0101000000");
    const auto unnamed_decoded = Decode(unnamed);
    CHECK(unnamed_decoded.ok());
    CHECK(unnamed_decoded.update.channel.channels.size() == 1);
    CHECK(unnamed_decoded.update.channel.channels[0].name.empty());

    // The byte ceiling SendChannels clamps to, at its edge.
    std::vector<ChannelEntry> many;
    for (std::size_t index = 0; index < 255; ++index) {
        many.push_back(Row(static_cast<std::uint16_t>(index), "c"));
    }
    const auto full = Decode(ChannelEncoder::List(many));
    CHECK(full.ok());
    CHECK(full.update.channel.channels.size() == 255);

    std::cout << "  empty list, sentinel boundary, empty name, 255 rows: PASS\n";
}

// ------------------------------------------------------------------ negative

void TruncationIsAlwaysAnError() {
    const std::vector<std::vector<std::uint8_t>> complete = {
        ChannelEncoder::List({Row(kOwnChannelOfferId, "Private Chat Channel"), Row(3, "Help")}),
        ChannelEncoder::Open(false, 3, "Help"),
        ChannelEncoder::Open(true, 7, "Bob"),
        ChannelEncoder::Private("Bob"),
        ChannelEncoder::Close(7),
    };

    std::size_t checked = 0;
    for (const auto& message : complete) {
        for (std::size_t length = 1; length < message.size(); ++length) {
            const std::vector<std::uint8_t> prefix(
                message.begin(), message.begin() + static_cast<long>(length));
            const auto decoded = Decode(prefix);
            CHECK(!decoded.ok());
            CHECK(decoded.error == MapDecodeError::Truncated);
            CHECK(decoded.update.bytes_consumed == 0);
            ++checked;
        }
    }
    CHECK(checked > 0);

    // A count promising three rows when one is present.
    {
        auto message = ChannelEncoder::List({Row(3, "Help")});
        message[1] = 3;
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    // A channel name whose length outruns the payload.
    {
        auto message = ChannelEncoder::Open(false, 3, "Help");
        message[3] = 0x40;  // low byte of the name length word
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    // A private-channel name whose length outruns the payload.
    {
        auto message = ChannelEncoder::Private("Bob");
        message[1] = 0x40;
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    // A close with half of its word.
    {
        const std::vector<std::uint8_t> half{kServerCommandCloseChannel, 0x07};
        const auto decoded = Decode(half);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    std::cout << "  truncation, " << checked
              << " prefixes plus four shaped cases: PASS\n";
}

void MalformedAndUnknown() {
    // DecodeChannel refuses anything that is not one of its five, rather
    // than decoding a channel out of someone else's command.
    {
        ChannelUpdate update;
        const std::vector<std::uint8_t> bytes = {0x00, 0x00, 0x00};
        for (const std::uint8_t opcode : {std::uint8_t{110}, std::uint8_t{170},
                                          std::uint8_t{174}, std::uint8_t{180},
                                          std::uint8_t{177}}) {
            MapScanner scanner(bytes, 0, Types());
            CHECK(!DecodeChannel(&scanner, opcode, &update));
            CHECK(scanner.error() == MapDecodeError::UnexpectedCommand);
        }
    }

    // The request queue is NOT decoded by this milestone. Joining
    // kChannelRuleViolations answers with SV_CMD_OPEN_REQUEST_QUEUE (174)
    // and SendExistingRequests, and claiming otherwise would be the easiest
    // way to overstate what channels now covers.
    for (const std::uint8_t opcode : {std::uint8_t{174}, std::uint8_t{175},
                                      std::uint8_t{176}, std::uint8_t{177}}) {
        const auto decoded = Decode({opcode, 1, 2, 3, 4});
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Unsupported);
        CHECK(decoded.update.bytes_consumed == 0);
    }

    // A reused output is fully overwritten: a close after a list must leave
    // no rows behind, and a private open must leave no channel number.
    {
        ChannelUpdate update;
        const auto list = ChannelEncoder::List({Row(3, "Help")});
        const std::vector<std::uint8_t> list_body(list.begin() + 1, list.end());
        MapScanner first(list_body, 0, Types());
        CHECK(DecodeChannel(&first, kServerCommandChannels, &update));
        CHECK(!update.channels.empty());

        const std::vector<std::uint8_t> close_body{0x07, 0x00};
        MapScanner second(close_body, 0, Types());
        CHECK(DecodeChannel(&second, kServerCommandCloseChannel, &update));
        CHECK(update.channels.empty());
        CHECK(update.name.empty());
        CHECK(update.channel == 7);

        const std::vector<std::uint8_t> private_body{0x03, 0x00, 'B', 'o', 'b'};
        MapScanner third(private_body, 0, Types());
        CHECK(DecodeChannel(&third, kServerCommandPrivateChannel, &update));
        CHECK(update.channel == 0);
        CHECK(update.name == "Bob");
    }

    std::cout << "  wrong opcodes, undecoded request queue, output reuse: PASS\n";
}

void ChannelsStoreNothing() {
    // Fusion32 keeps the channel lists itself and resends them on request.
    WorldState state;
    state.viewport_anchor = kAnchor;
    state.local_creature_id = 1000;

    const auto list = ChannelEncoder::List({Row(3, "Help")});
    const auto decoded = Decode(list);
    CHECK(decoded.ok());

    const WorldState before = state;
    ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(state.viewport_anchor == before.viewport_anchor);
    CHECK(state.local_creature_id == before.local_creature_id);
    CHECK(state.known_creatures.size() == before.known_creatures.size());
    CHECK(state.tile_count() == before.tile_count());
    CHECK(state.thing_count() == before.thing_count());

    std::cout << "  channels change no WorldState: PASS\n";
}

}  // namespace

int main() {
    try {
        GoldenClientCommands();
        GoldenServerCommands();
        ListEdgeCases();
        TruncationIsAlwaysAnError();
        MalformedAndUnknown();
        ChannelsStoreNothing();
    } catch (const std::exception& error) {
        std::cerr << "channels_tests FAILED: " << error.what() << '\n';
        return 1;
    }
    std::cout << "channels_tests: PASS\n";
    return 0;
}
