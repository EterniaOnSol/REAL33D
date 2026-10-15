// CLIENTCORE-TRADE-772-001.
//
// Both directions of the 7.72 trade: the four commands the client builds and
// the three the server sends.
//
// The server emitter below is a literal port of
// reference/game/src/sending.cc::SendTradeOffer, SendTradeObjects and
// SendCloseTrade. It is asserted against hand-computed golden hex before it
// is used to build anything else, because a generator that agrees with a
// decoder written from the same misreading proves nothing.

#include "fusion32/protocol772/map_scan.h"
#include "fusion32/protocol772/movement.h"
#include "fusion32/protocol772/trade.h"

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
//
// Port of SendTradeOffer / SendTradeObjects / SendCloseTrade. `items` is
// already the flattened pre-order walk SendTradeObjects produces, because
// that is exactly what reaches the wire: the recursion into container
// contents leaves no marker behind it.
class TradeEncoder {
public:
    static std::vector<std::uint8_t> Offer(bool own_offer, const std::string& name,
                                           const std::vector<ItemThing>& items) {
        TradeEncoder encoder;
        encoder.Byte(own_offer ? kServerCommandTradeOfferOwn
                               : kServerCommandTradeOfferPartner);
        encoder.Str(name);
        encoder.Byte(static_cast<std::uint8_t>(items.size()));
        for (const ItemThing& item : items) encoder.Item(item);
        return encoder.out_;
    }

    static std::vector<std::uint8_t> Close() {
        TradeEncoder encoder;
        encoder.Byte(kServerCommandCloseTrade);
        return encoder.out_;
    }

private:
    void Byte(std::uint8_t value) { out_.push_back(value); }

    void Word(std::uint16_t value) {
        out_.push_back(static_cast<std::uint8_t>(value));
        out_.push_back(static_cast<std::uint8_t>(value >> 8U));
    }

    void Str(const std::string& value) {
        Word(static_cast<std::uint16_t>(value.size()));
        for (const char character : value) {
            out_.push_back(static_cast<std::uint8_t>(character));
        }
    }

    // Mirrors SendItem: the disguised type id, then one byte per flag that
    // asks for one.
    void Item(const ItemThing& item) {
        Word(item.type_id);
        if (item.has_liquid_color) Byte(item.liquid_color);
        if (item.has_amount) Byte(item.amount);
    }

    std::vector<std::uint8_t> out_;
};

ItemThing Plain(std::uint16_t type_id) {
    ItemThing item;
    item.type_id = type_id;
    return item;
}

ItemThing Cumulative(std::uint16_t type_id, std::uint8_t amount) {
    ItemThing item = Plain(type_id);
    item.has_amount = true;
    item.amount = amount;
    return item;
}

ItemThing Liquid(std::uint16_t type_id, std::uint8_t color) {
    ItemThing item = Plain(type_id);
    item.has_liquid_color = true;
    item.liquid_color = color;
    return item;
}

// ------------------------------------------------------------ golden: client

void GoldenClientCommands() {
    // CTradeObject, map object. 0x7D, then the x/y/z/type/stack reference
    // CMoveObject and CUseObject share, then the partner quad.
    //   7d | 617d | db7d | 07 | 260b | 02 | e9030000
    {
        const auto command = BuildTradeObjectCommand(
            MoveEndpoint::OnMap(MapPosition{32097, 32219, 7}), 2854, 2, 1001);
        CHECK(Hex(command) == "7d617ddb7d07260b02e9030000");
        CHECK(command.size() == 13);
        CHECK(command[0] == kClientCommandTradeObject);
    }

    // The same command naming an object inside open container 1, slot 3.
    // CheckSpecialCoordinates sees x = 0xFFFF and y = CONTAINER_FIRST + 1.
    //   7d | ffff | 4100 | 03 | 4808 | 03 | ea030000
    {
        const auto command = BuildTradeObjectCommand(
            MoveEndpoint::InContainer(1, 3), 2120, 3, 1002);
        CHECK(Hex(command) == "7dffff410003480803ea030000");
        CHECK(command.size() == 13);
    }

    // A body slot: x = 0xFFFF, y = the slot, z = 0.
    {
        const auto command = BuildTradeObjectCommand(
            MoveEndpoint::InInventory(5), 2854, 0, 1001);
        CHECK(Hex(command) == "7dffff050000260b00e9030000");
    }

    // CInspectTrade reads `OwnOffer = (readByte() == 0)`. Zero is the own
    // offer; this is the inversion TradeOfferSide exists to hide, and the
    // one place it is checked against the server's own comparison.
    {
        const auto own = BuildInspectTradeCommand(TradeOfferSide::Own, 0);
        CHECK(Hex(own) == "7e0000");
        CHECK(own.size() == 3);
        CHECK(own[1] == 0);

        const auto partner = BuildInspectTradeCommand(TradeOfferSide::Partner, 5);
        CHECK(Hex(partner) == "7e0105");
        CHECK(partner[1] != 0);

        CHECK(TradeOfferSideByte(TradeOfferSide::Own) == 0);
        CHECK(TradeOfferSideByte(TradeOfferSide::Partner) != 0);
    }

    // CAcceptTrade and CRejectTrade read nothing at all.
    {
        const auto accept = BuildAcceptTradeCommand();
        CHECK(Hex(accept) == "7f");
        CHECK(accept.size() == 1);

        const auto reject = BuildRejectTradeCommand();
        CHECK(Hex(reject) == "80");
        CHECK(reject.size() == 1);
    }

    std::cout << "  client command golden bytes: PASS\n";
}

// ------------------------------------------------------------ golden: server

void GoldenServerCommands() {
    // Emitter against hand-computed bytes, before it is trusted anywhere.
    //   7d | 0300 "Bob" | 02 | 6500 | c800 07
    // 101 is plain, 200 is cumulative in the golden table, so only the
    // second object carries an extra byte.
    const auto own_offer = TradeEncoder::Offer(true, "Bob", {Plain(101), Cumulative(200, 7)});
    CHECK(Hex(own_offer) == "7d0300426f62026500c80007");

    //   7e | 0200 "Al" | 01 | 2c01 02      (300 is a liquid in the table)
    const auto partner_offer = TradeEncoder::Offer(false, "Al", {Liquid(300, 2)});
    CHECK(Hex(partner_offer) == "7e0200416c012c0102");

    // SendCloseTrade writes the opcode and calls FinishSendData.
    const auto close = TradeEncoder::Close();
    CHECK(Hex(close) == "7f");

    // Now the decoder, through the real dispatcher, so the opcode routing is
    // proven and not just the body reader.
    {
        const auto decoded = Decode(own_offer);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Trade);
        CHECK(std::string(decoded.update.name) == "SV_CMD_TRADE_OFFER_OWN");
        // bytes_consumed spans the whole message, opcode included.
        CHECK(decoded.update.bytes_consumed == own_offer.size());
        const TradeUpdate& trade = decoded.update.trade;
        CHECK(trade.kind == TradeUpdateKind::OwnOffer);
        CHECK(trade.name == "Bob");
        CHECK(trade.items.size() == 2);
        CHECK(trade.items[0].type_id == 101);
        CHECK(!trade.items[0].has_amount && !trade.items[0].has_liquid_color);
        CHECK(trade.items[1].type_id == 200);
        CHECK(trade.items[1].has_amount && trade.items[1].amount == 7);
    }

    {
        const auto decoded = Decode(partner_offer);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Trade);
        CHECK(std::string(decoded.update.name) == "SV_CMD_TRADE_OFFER_PARTNER");
        const TradeUpdate& trade = decoded.update.trade;
        CHECK(trade.kind == TradeUpdateKind::PartnerOffer);
        CHECK(trade.name == "Al");
        CHECK(trade.items.size() == 1);
        CHECK(trade.items[0].type_id == 300);
        CHECK(trade.items[0].has_liquid_color && trade.items[0].liquid_color == 2);
        CHECK(!trade.items[0].has_amount);
    }

    {
        const auto decoded = Decode(close);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Trade);
        CHECK(std::string(decoded.update.name) == "SV_CMD_CLOSE_TRADE");
        // Opcode only, so the whole message is one byte.
        CHECK(decoded.update.bytes_consumed == 1);
        CHECK(decoded.update.trade.kind == TradeUpdateKind::Closed);
        CHECK(decoded.update.trade.items.empty());
        CHECK(decoded.update.trade.name.empty());
    }

    std::cout << "  server command golden bytes and decode: PASS\n";
}

// ------------------------------------------------- structure the wire cannot say

void FlatObjectListIsNotATree() {
    // SendTradeObjects emits a bag and then recurses into its contents, so a
    // backpack holding three items arrives as four consecutive objects with
    // nothing marking where the bag ends. The decoder must report four
    // objects and must not invent nesting.
    const auto offer = TradeEncoder::Offer(
        true, "Bob", {Plain(102), Plain(101), Cumulative(200, 3), Liquid(300, 1)});
    const auto decoded = Decode(offer);
    CHECK(decoded.ok());
    CHECK(decoded.update.trade.items.size() == 4);
    CHECK(decoded.update.trade.items[0].type_id == 102);
    CHECK(decoded.update.trade.items[3].type_id == 300);

    // An empty offer is well-formed: count zero, no objects.
    const auto empty = TradeEncoder::Offer(false, "Shopkeeper", {});
    const auto empty_decoded = Decode(empty);
    CHECK(empty_decoded.ok());
    CHECK(empty_decoded.update.trade.items.empty());
    CHECK(empty_decoded.update.trade.name == "Shopkeeper");

    // The byte ceiling SendTradeOffer enforces, exercised at its edge.
    std::vector<ItemThing> many;
    for (std::size_t index = 0; index < kMaxTradeOfferObjects; ++index) {
        many.push_back(Plain(101));
    }
    const auto full = TradeEncoder::Offer(true, "Bob", many);
    const auto full_decoded = Decode(full);
    CHECK(full_decoded.ok());
    CHECK(full_decoded.update.trade.items.size() == kMaxTradeOfferObjects);

    // An empty name is what SendString writes for a zero-length string. The
    // server refuses a NULL name, which is not the same thing and cannot be
    // expressed on the wire at all.
    const auto unnamed = TradeEncoder::Offer(true, "", {Plain(101)});
    CHECK(Hex(unnamed) == "7d0000016500");
    const auto unnamed_decoded = Decode(unnamed);
    CHECK(unnamed_decoded.ok());
    CHECK(unnamed_decoded.update.trade.name.empty());
    CHECK(unnamed_decoded.update.trade.items.size() == 1);

    std::cout << "  flat object list, empty offer, 255 objects: PASS\n";
}

// ------------------------------------------------------------------ negative

void TruncationIsAlwaysAnError() {
    // Every proper prefix of a well-formed offer must fail, and must fail as
    // a decode error rather than as a short or half-filled offer. One byte
    // of the name length, one byte of an item's extra byte, the count
    // promising an object that never arrives: all of them live here.
    const std::vector<std::vector<std::uint8_t>> complete = {
        TradeEncoder::Offer(true, "Bob", {Plain(101), Cumulative(200, 7)}),
        TradeEncoder::Offer(false, "Al", {Liquid(300, 2)}),
        TradeEncoder::Offer(true, "Shopkeeper", {}),
    };

    std::size_t checked = 0;
    for (const auto& message : complete) {
        for (std::size_t length = 1; length < message.size(); ++length) {
            const std::vector<std::uint8_t> prefix(message.begin(),
                                                   message.begin() + static_cast<long>(length));
            const auto decoded = Decode(prefix);
            CHECK(!decoded.ok());
            CHECK(decoded.error == MapDecodeError::Truncated);
            // Nothing partially applied: the failure is reported, not a
            // shorter offer that looks legitimate.
            CHECK(decoded.update.bytes_consumed == 0);
            ++checked;
        }
    }
    CHECK(checked > 0);

    // A count that promises more objects than the payload holds. Two
    // objects announced, one delivered.
    {
        auto message = TradeEncoder::Offer(true, "Bob", {Plain(101)});
        message[1 + 2 + 3] = 2;  // opcode, length word, "Bob", then the count
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    // A name length longer than the bytes that follow it.
    {
        auto message = TradeEncoder::Offer(true, "Bob", {Plain(101)});
        message[1] = 0x40;  // low byte of the length word: 64 characters
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    // A cumulative object whose amount byte never arrives. 200 is cumulative
    // in the golden table, so SendItem would have written one more byte.
    {
        auto message = TradeEncoder::Offer(true, "Bob", {Cumulative(200, 7)});
        message.pop_back();
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    // A liquid object missing its colour byte.
    {
        auto message = TradeEncoder::Offer(false, "Al", {Liquid(300, 2)});
        message.pop_back();
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    std::cout << "  truncation and short payloads, " << checked
              << " prefixes plus five shaped cases: PASS\n";
}

void MalformedBodies() {
    // An object type the table does not declare. The decoder cannot know how
    // many extra bytes it carries, so guessing would desynchronise the
    // stream; it has to refuse.
    {
        TradeUpdate update;
        const std::vector<std::uint8_t> bytes = {
            0x03, 0x00, 'B', 'o', 'b',  // name
            0x01,                        // one object
            0x39, 0x30,                  // type 12345, undeclared
        };
        MapScanner scanner(bytes, 0, Types());
        CHECK(!DecodeTrade(&scanner, kServerCommandTradeOfferOwn, &update));
        CHECK(scanner.error() == MapDecodeError::UnknownObjectTypeId);
    }

    // DecodeTrade refuses an opcode that is not one of its three, rather
    // than decoding a trade out of someone else's command.
    {
        TradeUpdate update;
        const std::vector<std::uint8_t> bytes = {0x00};
        for (const std::uint8_t opcode : {std::uint8_t{110}, std::uint8_t{124},
                                          std::uint8_t{128}, std::uint8_t{170}}) {
            MapScanner scanner(bytes, 0, Types());
            CHECK(!DecodeTrade(&scanner, opcode, &update));
            CHECK(scanner.error() == MapDecodeError::UnexpectedCommand);
        }
    }

    // A close carries no payload, so an empty buffer past the opcode is
    // correct rather than truncated. This is the one trade command that
    // cannot be short.
    {
        TradeUpdate update;
        const std::vector<std::uint8_t> bytes;
        MapScanner scanner(bytes, 0, Types());
        CHECK(DecodeTrade(&scanner, kServerCommandCloseTrade, &update));
        CHECK(update.kind == TradeUpdateKind::Closed);
    }

    // A reused output is fully overwritten: a close after an offer must not
    // leave the previous offer's name or objects behind.
    {
        TradeUpdate update;
        const auto offer = TradeEncoder::Offer(true, "Bob", {Plain(101)});
        const std::vector<std::uint8_t> body(offer.begin() + 1, offer.end());
        MapScanner first(body, 0, Types());
        CHECK(DecodeTrade(&first, kServerCommandTradeOfferOwn, &update));
        CHECK(!update.items.empty());

        const std::vector<std::uint8_t> nothing;
        MapScanner second(nothing, 0, Types());
        CHECK(DecodeTrade(&second, kServerCommandCloseTrade, &update));
        CHECK(update.items.empty());
        CHECK(update.name.empty());
    }

    std::cout << "  malformed bodies, wrong opcodes, output reuse: PASS\n";
}

// -------------------------------------------------------------- no world state

void TradeStoresNothing() {
    // A trade is a dialogue Fusion32 owns outright and resends in full. It is
    // surfaced as an event and must leave WorldState untouched, exactly like
    // the effects.
    WorldState state;
    state.viewport_anchor = kAnchor;
    state.local_creature_id = 1000;
    state.combat.target_creature_id = 1001;

    const auto offer = TradeEncoder::Offer(true, "Bob", {Plain(101), Cumulative(200, 7)});
    const auto decoded = Decode(offer);
    CHECK(decoded.ok());

    const WorldState before = state;
    ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(state.viewport_anchor == before.viewport_anchor);
    CHECK(state.local_creature_id == before.local_creature_id);
    CHECK(state.known_creatures.size() == before.known_creatures.size());
    CHECK(state.tile_count() == before.tile_count());
    CHECK(state.thing_count() == before.thing_count());
    // Still attacking whoever it was attacking: a trade revokes no target.
    CHECK(state.combat.target_creature_id == before.combat.target_creature_id);
    for (std::size_t index = 0; index < state.containers.size(); ++index) {
        CHECK(state.containers[index].open == before.containers[index].open);
        CHECK(state.containers[index].objects.size()
              == before.containers[index].objects.size());
    }

    std::cout << "  trade changes no WorldState: PASS\n";
}

}  // namespace

int main() {
    try {
        GoldenClientCommands();
        GoldenServerCommands();
        FlatObjectListIsNotATree();
        TruncationIsAlwaysAnError();
        MalformedBodies();
        TradeStoresNothing();
    } catch (const std::exception& error) {
        std::cerr << "trade_tests FAILED: " << error.what() << '\n';
        return 1;
    }
    std::cout << "trade_tests: PASS\n";
    return 0;
}
