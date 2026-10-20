#ifndef FUSION32_PROTOCOL772_OUTFIT_H
#define FUSION32_PROTOCOL772_OUTFIT_H

// Outfit: the two client commands that open and answer the 7.72 outfit
// chooser, and the accept/reject rules the server applies to the answer.
//
// Source for every layout, every bound and every constant here:
//   reference/game/src/receiving.cc::CGetOutfit (line 1390), CSetOutfit (1410)
//   reference/game/src/sending.cc::SendOutfit(TConnection*) (1661),
//     SendOutfit(TConnection*, TOutfit) (162), SendCreatureOutfit (1011)
//   reference/game/src/cr.hh::TOutfit (17)
//   reference/game/src/crplayer.cc::TPlayer::CheckOutfit (1175)
//   reference/game/src/operate.cc::AnnounceChangedCreature (81)
//   reference/game/src/connections.hh::ClientCommand, ServerCommand
//   reference/game/src/enums.hh::PREMIUM_ACCOUNT (456), GAMEMASTER_OUTFIT (527)
//
// The two server-to-client commands were already decoded before this
// milestone and stay where they are: SV_CMD_OUTFIT (200) in player_state.h as
// OutfitDialogUpdate, and SV_CMD_CREATURE_OUTFIT (142) as
// CreatureAttribute::Outfit, both reading the outfit through
// MapScanner::ReadOutfit. What this header adds is the client's half, plus the
// rules CSetOutfit applies to it, so a dialog can refuse a selection the
// server would otherwise drop in silence.
//
// Three things about 7.72 that this is not, and that nothing here invents:
// there is no addon byte, no mount, and no second look-type word. SendOutfit
// writes the id and then *either* an object type *or* four colour bytes, and
// that is the whole of an outfit on this wire.

// For OutfitDialogUpdate and the two server opcodes, which were decoded
// before this milestone and are not redeclared here.
#include "fusion32/protocol772/player_state.h"
#include "fusion32/protocol772/worldstate.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace fusion32::protocol772 {

// Source: reference/game/src/connections.hh::ClientCommand lines 67-68.
//
// These two numbers are also SV_CMD_BUDDY_DATA (210) and SV_CMD_BUDDY_ONLINE
// (211), already declared in player_state.h. That is not a mistake in either
// place: ClientCommand and ServerCommand are separate enums in
// connections.hh and 7.72 reuses the values freely in the two directions.
// Direction is what disambiguates an opcode on this protocol, never the number
// alone, so nothing may dispatch on a byte without knowing which way it was
// travelling.
constexpr std::uint8_t kClientCommandGetOutfit = 210;
constexpr std::uint8_t kClientCommandSetOutfit = 211;

// CGetOutfit reads nothing from the buffer, so the request is a bare opcode.
constexpr std::size_t kGetOutfitCommandSize = 1;

// CSetOutfit reads one word and then four bytes, unconditionally. This is the
// asymmetry that matters most in this file: the *server's* SendOutfit branches
// on `OutfitID == 0` and writes an object type instead of the colours, but the
// *client's* CSetOutfit has no such branch --
//
//     NewOutfit.OutfitID = Buffer->readWord();
//     Buffer->readBytes(NewOutfit.Colors, sizeof(NewOutfit.Colors));
//
// -- so a set command is always these seven bytes and can never express an
// object disguise. See OutfitSelectionError::DisguiseNotSelectable.
constexpr std::size_t kSetOutfitCommandSize = 7;

// The four colour bytes CSetOutfit reads and SendOutfit writes.
constexpr std::size_t kOutfitColorCount = 4;
using OutfitColors = std::array<std::uint8_t, kOutfitColorCount>;

// Highest colour byte CSetOutfit accepts. The server's test is
// `Colors[n] > 132` for each of the four, so 132 is valid and 133 is not.
constexpr std::uint8_t kMaxOutfitColor = 132;

// IMPORTANT: Fusion32 never names the four colour slots. `TOutfit` declares
// them as `uint8 Colors[4]` and every use -- CheckOutfit, CSetOutfit's bound
// test, SendOutfit's SendBytes -- addresses them by index only. Which slot
// tints which part of the sprite is decided entirely by the classic client's
// .spr/.dat template outside this repository, and the 7.72 protocol never
// states it. So this layer carries four indexed bytes in wire order and
// nothing here claims a body part for any of them; a renderer has to settle
// that mapping by observation, not by reading the protocol.

// Source: reference/game/src/crnonpl.cc lines 1586-1589, which is where the
// numbers are assigned names, and crmain.cc lines 197/225, which comment the
// comparison as MALE.
constexpr std::uint8_t kPlayerSexMale = 1;
constexpr std::uint8_t kPlayerSexFemale = 2;

// Source: reference/game/src/crplayer.cc::CheckOutfit lines 1181-1205. These
// are the outfits the server forces a character into when the stored one is
// outside the allowed band, so they are what a freshly created character shows
// before its owner has ever opened the chooser.
constexpr std::uint16_t kMaleFirstOutfitId = 128;
constexpr std::uint16_t kFemaleFirstOutfitId = 136;
constexpr OutfitColors kDefaultOutfitColors{{78, 69, 58, 76}};

// Source: reference/game/src/crplayer.cc::CheckOutfit lines 1176-1185. A
// character holding GAMEMASTER_OUTFIT is forced to this id with all four
// colours zero, and -- see CGetOutfit -- is refused the chooser outright.
constexpr std::uint16_t kGamemasterOutfitId = 75;
constexpr OutfitColors kGamemasterOutfitColors{{0, 0, 0, 0}};

// Source: reference/game/src/sending.cc line 1674 and receiving.cc line 1428,
// both `LastOutfit += 3` under CheckRight(PREMIUM_ACCOUNT).
constexpr std::uint16_t kPremiumExtraOutfits = 3;

/**
 * The inclusive band of outfit ids a player may select.
 *
 * Source: reference/game/src/sending.cc::SendOutfit lines 1671-1675 and
 * reference/game/src/receiving.cc::CSetOutfit lines 1425-1429, which compute
 * it identically. SendOutfit puts it on the wire as the two trailing words of
 * SV_CMD_OUTFIT; CSetOutfit recomputes it to check the answer.
 *
 * A **band, not a list**. This is the 7.72 shape and it is worth saying out
 * loud because later protocol versions replaced it with a counted list of
 * individual outfits: here the server sends only the first and the last id,
 * and every id in between is implied.
 */
struct OutfitSelectionRange {
    std::uint16_t first = 0;
    std::uint16_t last = 0;

    /** How many ids the band names. Zero if it is inverted or empty. */
    constexpr std::size_t count() const noexcept {
        return last < first ? std::size_t{0}
                            : static_cast<std::size_t>(last - first) + 1U;
    }

    /** Exactly CSetOutfit's test: `id < First || id > Last` is a refusal. */
    constexpr bool contains(std::uint16_t outfit_id) const noexcept {
        return outfit_id >= first && outfit_id <= last;
    }
};

/**
 * The band the server will compute for a given character.
 *
 * Mirrors the server's own expression, including its shape: the test is
 * `Sex == 1`, so male is the 128 series and **anything else at all** is the
 * 136 series. Fusion32 does not test for 2.
 *
 * This is for checking and for tests, not for driving a dialog. The 7.72 game
 * protocol never tells the client its own sex -- that byte lives in the
 * querymanager character record (reference/querymanager/src/query.cc line 720)
 * and never reaches this connection. The authority on screen is therefore the
 * `first_outfit`/`last_outfit` pair SV_CMD_OUTFIT actually carried; see
 * DialogSelectionRange.
 */
OutfitSelectionRange SelectableOutfits(std::uint8_t sex, bool premium) noexcept;

/** The band SV_CMD_OUTFIT carried, which is the one a dialog must obey. */
OutfitSelectionRange DialogSelectionRange(const OutfitDialogUpdate& dialog) noexcept;

/**
 * Why CSetOutfit would drop a selection.
 *
 * The order of the checks is the server's order, so the reason reported here
 * is the first complaint the server would have made.
 */
enum class OutfitSelectionError {
    None,
    // An object disguise. CSetOutfit has no branch for one: it would read the
    // low two bytes of the object type as the first two colours and the two
    // bytes after the command as the other two. Unrepresentable, so refused
    // here rather than mangled on the wire.
    DisguiseNotSelectable,
    // Outside the band. receiving.cc lines 1431-1438.
    OutfitIdOutOfRange,
    // Some colour above kMaxOutfitColor. receiving.cc lines 1440-1446.
    ColorOutOfRange,
};

const char* OutfitSelectionErrorName(OutfitSelectionError error) noexcept;

/** CSetOutfit's two tests, in its order, against an explicit band. */
OutfitSelectionError ValidateOutfitSelection(const OutfitSelectionRange& range,
                                             std::uint16_t outfit_id,
                                             const OutfitColors& colors) noexcept;

/**
 * The same, for a descriptor that came off the wire. Reports
 * DisguiseNotSelectable for an object disguise before anything else, because
 * that is the case the command cannot encode at all.
 */
OutfitSelectionError ValidateOutfitSelection(const OutfitSelectionRange& range,
                                             const OutfitDescriptor& outfit) noexcept;

// ------------------------------------------------------------------ builders

/**
 * Asks for the outfit chooser. CL_CMD_GET_OUTFIT, opcode 210. No payload:
 * CGetOutfit reads nothing from the buffer.
 *
 * **Two different answers are both correct**, and a dialog has to expect
 * either. Normally the server replies with SV_CMD_OUTFIT (200), decoded as
 * ServerUpdateKind::OutfitDialog. But a character holding GAMEMASTER_OUTFIT is
 * refused: CGetOutfit sends a TALK_FAILURE_MESSAGE (mode 23) reading "You may
 * not change your outfit." and returns without sending 200 at all. That
 * arrives as ServerUpdateKind::Message, so nothing is lost -- but a caller
 * that waits for 200 before showing anything waits forever.
 */
std::vector<std::uint8_t> BuildGetOutfitCommand();

/**
 * Answers the chooser. CL_CMD_SET_OUTFIT, opcode 211, always
 * kSetOutfitCommandSize bytes: the id as a little-endian word, then the four
 * colours in wire order.
 *
 * An ordinary builder, and deliberately not a gate: it encodes what it is
 * given without applying the server's band or colour tests, exactly as
 * BuildUseObjectCommand does not test MULTIUSE and BuildTradeObjectCommand
 * does not pre-apply isMapContainer. Call ValidateOutfitSelection when the
 * point is to refuse a selection before sending it.
 *
 * **The server acknowledges nothing.** CSetOutfit returns void and sends no
 * reply of any kind -- no confirmation, no second SV_CMD_OUTFIT, and no error
 * when it drops the selection. The one observable effect is indirect:
 * `AnnounceChangedCreature(CREATURE_OUTFIT_CHANGED)`, which reaches every
 * connection that knows the creature as SV_CMD_CREATURE_OUTFIT (142) through
 * reference/game/src/operate.cc line 81.
 *
 * And that announcement is **conditional**, which is the trap here. CSetOutfit
 * broadcasts only `if(Player->Outfit == Player->OrgOutfit)`, i.e. only when
 * the player is wearing their own outfit at that moment. A player who is
 * invisible or under an illusion keeps the appearance they have; the selection
 * is stored in OrgOutfit and surfaces later, when the effect ends and
 * reference/game/src/crmain.cc line 606 restores it. So a dialog must close on
 * its own and must not wait for a 142 that may never arrive.
 */
std::vector<std::uint8_t> BuildSetOutfitCommand(std::uint16_t outfit_id,
                                                const OutfitColors& colors);

/**
 * The same from a descriptor, for the ordinary flow of echoing back
 * OutfitDialogUpdate::current with edited colours.
 *
 * `object_type` is never sent, because the command has nowhere to put it: see
 * kSetOutfitCommandSize. A disguised descriptor therefore encodes as its id
 * plus its `colors` member, which for a descriptor that came off the wire is
 * all zeroes -- the server will drop it on the band test. ValidateOutfitSelection
 * reports that as DisguiseNotSelectable first, and is the right call to make
 * before this one.
 */
std::vector<std::uint8_t> BuildSetOutfitCommand(const OutfitDescriptor& outfit);

}  // namespace fusion32::protocol772

#endif
