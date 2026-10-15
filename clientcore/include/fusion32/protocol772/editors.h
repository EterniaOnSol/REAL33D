#ifndef FUSION32_PROTOCOL772_EDITORS_H
#define FUSION32_PROTOCOL772_EDITORS_H

// Text editors: the two client commands and the two server commands behind
// every book, scroll, sign, tombstone, spellbook and house list in 7.72.
//
// Source for every layout here:
//   reference/game/src/receiving.cc::CEditText, CEditList
//   reference/game/src/sending.cc::SendEditText, SendEditList
//   reference/game/src/enums.hh::GUESTLIST, SUBOWNERLIST, DOORLIST
//   reference/game/src/connections.hh::ClientCommand, ServerCommand
//
// Sits at the player_state.h layer and does not include movement.h, which
// includes it. Both builders take scalars and strings, so unlike trade
// neither has to be declared elsewhere.

#include "fusion32/protocol772/map_scan.h"
#include "fusion32/protocol772/worldstate.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

// Source: reference/game/src/connections.hh::ClientCommand.
constexpr std::uint8_t kClientCommandEditText = 137;
constexpr std::uint8_t kClientCommandEditList = 138;

// Source: reference/game/src/connections.hh::ServerCommand.
constexpr std::uint8_t kServerCommandEditText = 150;
constexpr std::uint8_t kServerCommandEditList = 151;

/**
 * Which list an SV_CMD_EDIT_LIST or CL_CMD_EDIT_LIST refers to.
 * Source: reference/game/src/enums.hh:280-282.
 *
 * The `id` that travels beside the type means two different things, which is
 * the one trap in this family. CEditList casts it to `uint16` and treats it
 * as a house id for GuestList and SubownerList, and keeps the full quad as
 * an object id for DoorList:
 *
 *     if(Type == GUESTLIST)         ChangeGuests((uint16)ID, Player, Text);
 *     else if(Type == SUBOWNERLIST) ChangeSubowners((uint16)ID, Player, Text);
 *     else if(Type == DOORLIST)     { Object Door = Object(ID); ... }
 *
 * This layer carries the quad through unchanged and does not narrow it. The
 * caller knows which list it asked for.
 */
enum class EditListType : std::uint8_t {
    GuestList = 1,
    SubownerList = 2,
    DoorList = 3,
};

/** True for the three values CEditList accepts. Anything else is logged by
 *  the server as "Unbekannter Typ" and silently dropped. */
constexpr bool IsKnownEditListType(std::uint8_t type) noexcept {
    return type >= 1 && type <= 3;
}

/**
 * CEditText and CEditList both read into `char Text[4096]`, and
 * TReadStream::readString keeps `MaxLength - 1` bytes and skips the rest, so
 * the server truncates anything longer to 4095 characters rather than
 * refusing it.
 */
constexpr std::size_t kMaxEditorTextLength = 4095;

/**
 * SendEditList refuses to send a list at or beyond this length
 * (`if(strlen(Text) >= 4000){ error(...); return; }`), so a well-formed
 * server list is always shorter. Informational: a client cannot be sent one
 * that is longer, and this layer enforces nothing on the way out.
 */
constexpr std::size_t kMaxServerEditListLength = 4000;

// ---------------------------------------------------------------- decoded

/**
 * Source: reference/game/src/sending.cc::SendEditText.
 *
 * Layout: `quad ObjectID`, `word TypeID`, `word MaxLength`, `string Text`,
 * `string Editor`. Both strings are always present -- every branch of the
 * sender writes two, and the fallback writes two empty ones.
 *
 * `max_length` is **not** a uniform capacity, and this is worth stating
 * because it looks like one:
 *
 *  - for a WRITE or WRITEONCE object it is the type's MAXLENGTH (or
 *    MAXLENGTHONCE) attribute minus one, so it is the longest text the
 *    server will accept back;
 *  - for a spellbook, and for anything else read-only, the sender sets it
 *    to `strlen(Text)` instead.
 *
 * So `max_length == text.size()` does not prove the object is read-only, and
 * nothing in this command definitively marks one. The client does not have
 * to guess: CEditText re-checks the WRITE / WRITEONCE flags and answers
 * NOTACCESSIBLE, so an edit of a read-only object is refused by Fusion32,
 * not by this layer.
 *
 * `editor` is the name of whoever last wrote the text, empty when nobody
 * has.
 */
struct EditTextUpdate {
    std::uint32_t object_id = 0;
    std::uint16_t type_id = 0;
    std::uint16_t max_length = 0;
    std::string text;
    std::string editor;
};

/**
 * Source: reference/game/src/sending.cc::SendEditList.
 * Layout: `byte Type`, `quad ID`, `string Text`.
 *
 * `known_type` is false when the byte is not one of the three in
 * EditListType. The server's own writer is the only producer, so that should
 * not happen; it is reported rather than rejected because the message is
 * still fully framed and skipping it would lose a list the client could
 * otherwise show.
 */
struct EditListUpdate {
    std::uint8_t type = 0;
    bool known_type = false;
    std::uint32_t id = 0;
    std::string text;
};

// Which of the two editor commands arrived.
enum class EditorUpdateKind {
    Text,
    List,
};

struct EditorUpdate {
    EditorUpdateKind kind = EditorUpdateKind::Text;
    EditTextUpdate text;
    EditListUpdate list;
};

// ------------------------------------------------------------------ builders

/**
 * Submits the edited text of an object. CL_CMD_EDIT_TEXT, opcode 137.
 * Layout: `quad ObjectID`, `string Text`.
 *
 * The object is named by its object id -- the one the matching
 * SV_CMD_EDIT_TEXT carried -- and not by a map position, so the text can be
 * submitted after the window has been open for a while.
 *
 * Nothing is enforced here. CEditText re-reads the object, re-checks WRITE
 * and WRITEONCE, and answers NOROOM when `strlen(Text) >= MaxLength` using
 * the type's own attribute. Since SendEditText reports that attribute minus
 * one, a text no longer than the `max_length` the client was given is
 * exactly the text the server will accept.
 *
 * As in channels, a length of 0xFFFF is the escape TReadStream::readString
 * takes as "a quad length follows", so the text is clamped to 0xFFFE bytes
 * and that length can never be emitted. The server truncates at 4095 on its
 * own.
 */
std::vector<std::uint8_t> BuildEditTextCommand(std::uint32_t object_id,
                                               const std::string& text);

/**
 * Submits an edited house or door list. CL_CMD_EDIT_LIST, opcode 138.
 * Layout: `byte Type`, `quad ID`, `string Text`.
 *
 * `id` is a house id for GuestList and SubownerList and an object id for
 * DoorList; see EditListType. The same 0xFFFF clamp applies.
 */
std::vector<std::uint8_t> BuildEditListCommand(EditListType type,
                                               std::uint32_t id,
                                               const std::string& text);

// ------------------------------------------------------------------- decoder

/**
 * Decodes SV_CMD_EDIT_TEXT or SV_CMD_EDIT_LIST, with the scanner positioned
 * just past the opcode. Reports failure through the scanner, so a truncated
 * message is an explicit MapDecodeError and never a half-filled editor.
 */
bool DecodeEditor(MapScanner* scanner, std::uint8_t opcode, EditorUpdate* output);

}  // namespace fusion32::protocol772

#endif
