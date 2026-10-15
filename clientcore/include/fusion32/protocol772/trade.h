#ifndef FUSION32_PROTOCOL772_TRADE_H
#define FUSION32_PROTOCOL772_TRADE_H

// Trade: the four client commands and the three server commands that carry a
// 7.72 trade, whether between two players or with an NPC shop.
//
// Source for every layout here:
//   reference/game/src/receiving.cc::CTradeObject, CInspectTrade,
//     CAcceptTrade, CRejectTrade
//   reference/game/src/sending.cc::SendTradeOffer, SendTradeObjects,
//     SendCloseTrade
//   reference/game/src/connections.hh::ClientCommand, ServerCommand
//
// Nothing here decides a trade. The client asks, and Fusion32 answers with
// the offers and the close, exactly as it does for a walk or a use.

// This header sits at the same layer as player_state.h and deliberately does
// not include movement.h, which includes it. BuildTradeObjectCommand needs a
// MoveEndpoint and is therefore declared in movement.h beside the other
// builders that name an object, exactly as BuildMoveObjectCommand is even
// though the container it addresses is described here. Its implementation
// lives in src/trade.cpp with the rest of trade.

#include "fusion32/protocol772/map_scan.h"
#include "fusion32/protocol772/worldstate.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

// Source: reference/game/src/connections.hh::ClientCommand.
constexpr std::uint8_t kClientCommandTradeObject = 125;
constexpr std::uint8_t kClientCommandInspectTrade = 126;
constexpr std::uint8_t kClientCommandAcceptTrade = 127;
constexpr std::uint8_t kClientCommandRejectTrade = 128;

// Source: reference/game/src/connections.hh::ServerCommand.
constexpr std::uint8_t kServerCommandTradeOfferOwn = 125;
constexpr std::uint8_t kServerCommandTradeOfferPartner = 126;
constexpr std::uint8_t kServerCommandCloseTrade = 127;

// SendTradeOffer refuses to send more objects than a byte can count:
// `if(ObjCount > UINT8_MAX){ error(...); return; }`. So a well-formed offer
// never exceeds this, and a count that implies more is a desynchronised
// stream rather than a very large bag.
constexpr std::size_t kMaxTradeOfferObjects = 255;

/**
 * Which of the two offers an inspect refers to.
 *
 * The wire form is inverted and it matters: CInspectTrade reads
 * `bool OwnOffer = (Buffer->readByte() == 0)`, so **zero means your own
 * offer** and any non-zero byte means the partner's. An enum exists here so
 * no caller has to remember that, and so the inversion is stated in one
 * place instead of at every call site.
 */
enum class TradeOfferSide {
    Own,
    Partner,
};

/** The byte CInspectTrade expects for a side. Own is 0; Partner is 1. */
constexpr std::uint8_t TradeOfferSideByte(TradeOfferSide side) noexcept {
    return side == TradeOfferSide::Own ? std::uint8_t{0} : std::uint8_t{1};
}

// Which of the three trade commands arrived.
enum class TradeUpdateKind {
    // SV_CMD_TRADE_OFFER_OWN: what this player is offering.
    OwnOffer,
    // SV_CMD_TRADE_OFFER_PARTNER: what the other side is offering. For an NPC
    // shop this is the shop's list.
    PartnerOffer,
    // SV_CMD_CLOSE_TRADE: the trade is over, however it ended. No payload.
    Closed,
};

/**
 * Source: reference/game/src/sending.cc::SendTradeOffer.
 *
 * The object list is **flat, and deliberately so**. SendTradeObjects walks
 * the offered object and then recurses through the contents of any container
 * it finds, emitting one SendItem per object in pre-order, while `ObjCount`
 * is the total `CountObjects` returns for the same walk. The wire therefore
 * carries no nesting and no per-object parent: a bag of three items arrives
 * as four consecutive objects with nothing marking where the bag ends.
 *
 * This layer reports exactly that and reconstructs nothing. Inferring a tree
 * would mean guessing which types are containers and how many children each
 * took, which the message does not say.
 */
struct TradeUpdate {
    TradeUpdateKind kind = TradeUpdateKind::Closed;

    // Offers only. The partner's character name, or the NPC's.
    std::string name;

    // Offers only, in the order SendTradeObjects emitted them.
    std::vector<ItemThing> items;
};

// ------------------------------------------------------------------ builders

// BuildTradeObjectCommand, CL_CMD_TRADE_OBJECT (125), is declared in
// movement.h because it takes a MoveEndpoint. It is implemented in
// src/trade.cpp with everything else here.

/**
 * Looks at one object inside an offer. CL_CMD_INSPECT_TRADE, opcode 126.
 *
 * `position` indexes the flat list the matching offer carried. The server
 * resolves it through `Player->InspectTrade` and, when it names a real
 * object, answers through the ordinary Look path -- a `TALK_INFO_MESSAGE`,
 * the same way BuildLookAtPointCommand is answered. An index naming nothing
 * is silently ignored by the server, which is why this command has no reply
 * of its own.
 */
std::vector<std::uint8_t> BuildInspectTradeCommand(TradeOfferSide side,
                                                   std::uint8_t position);

/**
 * Accepts the trade. CL_CMD_ACCEPT_TRADE, opcode 127. No payload:
 * CAcceptTrade reads nothing from the buffer and calls Player->AcceptTrade().
 *
 * Accepting is not completing. Fusion32 closes the trade only once both
 * sides have accepted, and says so with SV_CMD_CLOSE_TRADE.
 */
std::vector<std::uint8_t> BuildAcceptTradeCommand();

/**
 * Rejects the trade. CL_CMD_REJECT_TRADE, opcode 128. No payload:
 * CRejectTrade reads nothing and calls Player->RejectTrade().
 */
std::vector<std::uint8_t> BuildRejectTradeCommand();

// ------------------------------------------------------------------- decoder

/**
 * Decodes one of the three trade commands, with the scanner positioned just
 * past the opcode. Reports failure through the scanner, so a truncated
 * message is an explicit MapDecodeError and never a half-filled offer.
 */
bool DecodeTrade(MapScanner* scanner, std::uint8_t opcode, TradeUpdate* output);

}  // namespace fusion32::protocol772

#endif
