// CLIENTCORE-EDITORS-772-001.
//
// Both directions of the 7.72 text editors: the two commands the client
// builds and the two the server sends. Behind every book, scroll, sign,
// tombstone, spellbook and house list.
//
// The server emitter below is a literal port of
// reference/game/src/sending.cc::SendEditText and SendEditList, asserted
// against hand-computed golden hex before it is used to build anything else.

#include "fusion32/protocol772/editors.h"
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

class EditorEncoder {
public:
    // SendEditText. Both strings are always written: every branch of the
    // sender produces a text and an editor, and the fallback produces two
    // empty ones.
    static std::vector<std::uint8_t> Text(std::uint32_t object_id,
                                          std::uint16_t type_id,
                                          std::uint16_t max_length,
                                          const std::string& text,
                                          const std::string& editor) {
        EditorEncoder encoder;
        encoder.Byte(kServerCommandEditText);
        encoder.Quad(object_id);
        encoder.Word(type_id);
        encoder.Word(max_length);
        encoder.Str(text);
        encoder.Str(editor);
        return encoder.out_;
    }

    // SendEditList.
    static std::vector<std::uint8_t> List(std::uint8_t type, std::uint32_t id,
                                          const std::string& text) {
        EditorEncoder encoder;
        encoder.Byte(kServerCommandEditList);
        encoder.Byte(type);
        encoder.Quad(id);
        encoder.Str(text);
        return encoder.out_;
    }

private:
    void Byte(std::uint8_t value) { out_.push_back(value); }

    void Word(std::uint16_t value) {
        out_.push_back(static_cast<std::uint8_t>(value));
        out_.push_back(static_cast<std::uint8_t>(value >> 8U));
    }

    void Quad(std::uint32_t value) {
        out_.push_back(static_cast<std::uint8_t>(value));
        out_.push_back(static_cast<std::uint8_t>(value >> 8U));
        out_.push_back(static_cast<std::uint8_t>(value >> 16U));
        out_.push_back(static_cast<std::uint8_t>(value >> 24U));
    }

    // Mirrors sending.cc::SendString: a plain word length, no 0xFFFF escape.
    void Str(const std::string& value) {
        Word(static_cast<std::uint16_t>(value.size()));
        for (const char character : value) {
            out_.push_back(static_cast<std::uint8_t>(character));
        }
    }

    std::vector<std::uint8_t> out_;
};

// ------------------------------------------------------------ golden: client

void GoldenClientCommands() {
    // CEditText reads a quad object id and a string.
    //   89 | 2e160000 | 0200 "hi"
    {
        const auto command = BuildEditTextCommand(5678, "hi");
        CHECK(Hex(command) == "892e16000002006869");
        CHECK(command.size() == 9);
        CHECK(command[0] == kClientCommandEditText);
    }

    // Clearing a text is an empty string, not an absent one.
    {
        const auto command = BuildEditTextCommand(1, "");
        CHECK(Hex(command) == "89010000000000");
        CHECK(command.size() == 7);
    }

    // CEditList reads a type byte, a quad id and a string. For GuestList the
    // id is a house id the server narrows to uint16.
    //   8a | 01 | f4010000 | 0300 "Bob"
    {
        const auto command = BuildEditListCommand(EditListType::GuestList, 500, "Bob");
        CHECK(Hex(command) == "8a01f40100000300426f62");
        CHECK(command.size() == 11);
        CHECK(command[0] == kClientCommandEditList);
        CHECK(command[1] == 1);
    }

    // For DoorList the same field is an object id and keeps the full quad,
    // so the high bytes must survive.
    //   8a | 03 | efbeadde | 0100 "x"
    {
        const auto command = BuildEditListCommand(EditListType::DoorList, 0xDEADBEEF, "x");
        CHECK(Hex(command) == "8a03efbeadde010078");
        CHECK(command.size() == 9);
    }

    {
        const auto command = BuildEditListCommand(EditListType::SubownerList, 500, "Al");
        CHECK(command[1] == 2);
    }

    // The enum values are the server's own, from enums.hh:280-282. These are
    // compile-time facts, so they are asserted at compile time: MSVC at /W4
    // /WX rejects a constant condition inside the runtime CHECK macro
    // (C4127), and a static_assert states the intent better anyway.
    static_assert(static_cast<std::uint8_t>(EditListType::GuestList) == 1, "GUESTLIST");
    static_assert(static_cast<std::uint8_t>(EditListType::SubownerList) == 2, "SUBOWNERLIST");
    static_assert(static_cast<std::uint8_t>(EditListType::DoorList) == 3, "DOORLIST");
    CHECK(!IsKnownEditListType(0) && IsKnownEditListType(1) && IsKnownEditListType(3));
    CHECK(!IsKnownEditListType(4) && !IsKnownEditListType(255));

    // 0xFFFF is the escape TReadStream::readString takes as "a quad length
    // follows", so it is the one length these builders can never write. The
    // server truncates at 4095 on its own; nothing is cut locally.
    {
        const std::string absurd(0x10000, 'a');
        for (const auto& command : {BuildEditTextCommand(1, absurd),
                                    BuildEditListCommand(EditListType::DoorList, 1, absurd)}) {
            const std::size_t at = command[0] == kClientCommandEditText ? 5u : 6u;
            const std::uint16_t length =
                static_cast<std::uint16_t>(command[at] | (command[at + 1] << 8));
            CHECK(length == 0xFFFE);
        }
        // A 4096-character text is still sent whole: the server truncates.
        const std::string long_text(4096, 'b');
        const auto command = BuildEditTextCommand(1, long_text);
        CHECK(command.size() == 7 + 4096);
        CHECK(long_text.size() > kMaxEditorTextLength);
    }

    std::cout << "  client command golden bytes: PASS\n";
}

// ------------------------------------------------------------ golden: server

void GoldenServerCommands() {
    // SendEditText for a writable book: object 5678, type 1950, the
    // MAXLENGTH attribute minus one, a body and the last editor's name.
    //   96 | 2e160000 | 9e07 | ff03 | 0200 "hi" | 0300 "Bob"
    const auto text = EditorEncoder::Text(5678, 1950, 1023, "hi", "Bob");
    CHECK(Hex(text) == "962e1600009e07ff03020068690300426f62");

    // The fallback branch: two empty strings, still both present.
    //   96 | 01000000 | 0000 | 0000 | 0000 | 0000
    const auto blank = EditorEncoder::Text(1, 0, 0, "", "");
    CHECK(Hex(blank) == "96010000000000000000000000");

    //   97 | 02 | f4010000 | 0200 "Al"
    const auto list = EditorEncoder::List(
        static_cast<std::uint8_t>(EditListType::SubownerList), 500, "Al");
    CHECK(Hex(list) == "9702f40100000200416c");

    // Through the real dispatcher, so opcode routing is proven too.
    {
        const auto decoded = Decode(text);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Editor);
        CHECK(std::string(decoded.update.name) == "SV_CMD_EDIT_TEXT");
        CHECK(decoded.update.bytes_consumed == text.size());
        const EditorUpdate& update = decoded.update.editor;
        CHECK(update.kind == EditorUpdateKind::Text);
        CHECK(update.text.object_id == 5678);
        CHECK(update.text.type_id == 1950);
        CHECK(update.text.max_length == 1023);
        CHECK(update.text.text == "hi");
        CHECK(update.text.editor == "Bob");
    }

    {
        const auto decoded = Decode(blank);
        CHECK(decoded.ok());
        CHECK(decoded.update.editor.text.text.empty());
        CHECK(decoded.update.editor.text.editor.empty());
        CHECK(decoded.update.editor.text.max_length == 0);
    }

    {
        const auto decoded = Decode(list);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Editor);
        CHECK(std::string(decoded.update.name) == "SV_CMD_EDIT_LIST");
        const EditorUpdate& update = decoded.update.editor;
        CHECK(update.kind == EditorUpdateKind::List);
        CHECK(update.list.type == 2);
        CHECK(update.list.known_type);
        CHECK(update.list.id == 500);
        CHECK(update.list.text == "Al");
    }

    // A door list keeps the full quad: the server narrows only the house
    // lists, and narrowing here would corrupt an object id.
    {
        const auto door = EditorEncoder::List(
            static_cast<std::uint8_t>(EditListType::DoorList), 0xDEADBEEF, "x");
        const auto decoded = Decode(door);
        CHECK(decoded.ok());
        CHECK(decoded.update.editor.list.id == 0xDEADBEEF);
    }

    std::cout << "  server command golden bytes and decode: PASS\n";
}

void MaxLengthIsNotAlwaysACapacity() {
    // For a WRITE object SendEditText reports MAXLENGTH - 1, the longest
    // text CEditText will take back. For a spellbook or any read-only
    // object it reports strlen(Text) instead. So max_length == text.size()
    // does not prove read-only, and this layer must not pretend it does --
    // it reports both numbers and lets Fusion32 refuse the edit.
    const auto writable = EditorEncoder::Text(1, 1950, 1023, "hi", "Bob");
    const auto read_only = EditorEncoder::Text(2, 1950, 2, "hi", "");

    const auto a = Decode(writable);
    const auto b = Decode(read_only);
    CHECK(a.ok() && b.ok());
    CHECK(a.update.editor.text.max_length == 1023);
    CHECK(b.update.editor.text.max_length == b.update.editor.text.text.size());
    // Nothing in the decoded record claims writability either way.
    CHECK(a.update.editor.text.text == b.update.editor.text.text);

    std::cout << "  max_length reported, writability not inferred: PASS\n";
}

// ------------------------------------------------------------------ negative

void TruncationIsAlwaysAnError() {
    const std::vector<std::vector<std::uint8_t>> complete = {
        EditorEncoder::Text(5678, 1950, 1023, "hi", "Bob"),
        EditorEncoder::Text(1, 0, 0, "", ""),
        EditorEncoder::List(1, 500, "Bob"),
        EditorEncoder::List(3, 0xDEADBEEF, ""),
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

    // The editor string missing entirely. A decoder that stopped after the
    // body would accept this and silently lose the second string, so it is
    // asserted on its own rather than only as one prefix among many.
    {
        auto message = EditorEncoder::Text(5678, 1950, 1023, "hi", "Bob");
        message.resize(message.size() - 5);  // drop "Bob" and its length word
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    // A body length that outruns the payload.
    {
        auto message = EditorEncoder::Text(5678, 1950, 1023, "hi", "Bob");
        message[9] = 0x40;  // low byte of the body length word
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    // An editor length that outruns the payload.
    {
        auto message = EditorEncoder::Text(5678, 1950, 1023, "hi", "Bob");
        message[13] = 0x40;  // low byte of the editor length word
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    // A list text length that outruns the payload.
    {
        auto message = EditorEncoder::List(1, 500, "Bob");
        message[6] = 0x40;
        const auto decoded = Decode(message);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    std::cout << "  truncation, " << checked
              << " prefixes plus four shaped cases: PASS\n";
}

void MalformedAndUnknown() {
    // An unknown list type is framed correctly, so it is reported rather
    // than rejected: dropping it would lose a list the client could show.
    // The server's own writer is the only producer, so this should never
    // appear, and `known_type` is how a caller finds out that it did.
    for (const std::uint8_t type : {std::uint8_t{0}, std::uint8_t{4}, std::uint8_t{255}}) {
        const auto decoded = Decode(EditorEncoder::List(type, 500, "Bob"));
        CHECK(decoded.ok());
        CHECK(decoded.update.editor.list.type == type);
        CHECK(!decoded.update.editor.list.known_type);
        CHECK(decoded.update.editor.list.text == "Bob");
    }

    // DecodeEditor refuses anything that is not one of its two.
    {
        EditorUpdate update;
        const std::vector<std::uint8_t> bytes(16, 0);
        for (const std::uint8_t opcode : {std::uint8_t{110}, std::uint8_t{149},
                                          std::uint8_t{152}, std::uint8_t{170},
                                          std::uint8_t{137}}) {
            MapScanner scanner(bytes, 0, Types());
            CHECK(!DecodeEditor(&scanner, opcode, &update));
            CHECK(scanner.error() == MapDecodeError::UnexpectedCommand);
        }
    }

    // Opcodes no milestone decodes stay unsupported, so this one cannot be
    // read as covering more than it does. 125 and 171 were here until
    // CLIENTCORE-TRADE-772-001 and CLIENTCORE-CHANNELS-772-001 decoded them;
    // what is left is the gamemaster request queue, and this set must stay
    // identical to the one in player_state_tests.cpp.
    for (const std::uint8_t opcode : {std::uint8_t{174}, std::uint8_t{175},
                                      std::uint8_t{176}, std::uint8_t{177}}) {
        const auto decoded = Decode({opcode, 1, 2, 3, 4, 5, 6, 7});
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Unsupported);
        CHECK(decoded.update.bytes_consumed == 0);
    }

    // A reused output is fully overwritten in both directions: a list after
    // a text must leave no text behind, and a text after a list no list.
    {
        EditorUpdate update;
        const auto text = EditorEncoder::Text(5678, 1950, 1023, "hi", "Bob");
        const std::vector<std::uint8_t> text_body(text.begin() + 1, text.end());
        MapScanner first(text_body, 0, Types());
        CHECK(DecodeEditor(&first, kServerCommandEditText, &update));
        CHECK(update.text.text == "hi");

        const auto list = EditorEncoder::List(1, 500, "Bob");
        const std::vector<std::uint8_t> list_body(list.begin() + 1, list.end());
        MapScanner second(list_body, 0, Types());
        CHECK(DecodeEditor(&second, kServerCommandEditList, &update));
        CHECK(update.kind == EditorUpdateKind::List);
        CHECK(update.text.text.empty());
        CHECK(update.text.editor.empty());
        CHECK(update.text.object_id == 0);

        MapScanner third(text_body, 0, Types());
        CHECK(DecodeEditor(&third, kServerCommandEditText, &update));
        CHECK(update.kind == EditorUpdateKind::Text);
        CHECK(update.list.text.empty());
        CHECK(update.list.id == 0);
        CHECK(!update.list.known_type);
    }

    std::cout << "  unknown list types, wrong opcodes, output reuse: PASS\n";
}

void EditorsStoreNothing() {
    // Fusion32 resends the whole body every time the window opens, and
    // CEditText re-reads the object before accepting anything back.
    WorldState state;
    state.viewport_anchor = kAnchor;
    state.local_creature_id = 1000;

    const auto decoded = Decode(EditorEncoder::Text(5678, 1950, 1023, "hi", "Bob"));
    CHECK(decoded.ok());

    const WorldState before = state;
    ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(state.viewport_anchor == before.viewport_anchor);
    CHECK(state.local_creature_id == before.local_creature_id);
    CHECK(state.known_creatures.size() == before.known_creatures.size());
    CHECK(state.tile_count() == before.tile_count());
    CHECK(state.thing_count() == before.thing_count());

    std::cout << "  editors change no WorldState: PASS\n";
}

}  // namespace

int main() {
    try {
        GoldenClientCommands();
        GoldenServerCommands();
        MaxLengthIsNotAlwaysACapacity();
        TruncationIsAlwaysAnError();
        MalformedAndUnknown();
        EditorsStoreNothing();
    } catch (const std::exception& error) {
        std::cerr << "editors_tests FAILED: " << error.what() << '\n';
        return 1;
    }
    std::cout << "editors_tests: PASS\n";
    return 0;
}
