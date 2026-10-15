#include "fusion32/protocol772/editors.h"

#include <algorithm>

namespace fusion32::protocol772 {

namespace {

void AppendQuad(std::vector<std::uint8_t>* command, std::uint32_t value) {
    command->push_back(static_cast<std::uint8_t>(value & 0xFF));
    command->push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    command->push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    command->push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
}

// The plain `word length` + bytes form sending.cc::SendString writes. The
// one length never emitted is 0xFFFF, which TReadStream::readString takes as
// "a quad length follows"; writing it would desynchronise the server's
// reader. The server truncates at 4095 itself, so nothing is cut here.
void AppendString(std::vector<std::uint8_t>* command, const std::string& value) {
    const std::size_t length = std::min<std::size_t>(value.size(), 0xFFFEu);
    command->push_back(static_cast<std::uint8_t>(length & 0xFF));
    command->push_back(static_cast<std::uint8_t>((length >> 8) & 0xFF));
    for (std::size_t index = 0; index < length; ++index) {
        command->push_back(static_cast<std::uint8_t>(value[index]));
    }
}

}  // namespace

std::vector<std::uint8_t> BuildEditTextCommand(std::uint32_t object_id,
                                               const std::string& text) {
    std::vector<std::uint8_t> command;
    command.reserve(7 + text.size());
    command.push_back(kClientCommandEditText);
    AppendQuad(&command, object_id);
    AppendString(&command, text);
    return command;
}

std::vector<std::uint8_t> BuildEditListCommand(EditListType type,
                                               std::uint32_t id,
                                               const std::string& text) {
    std::vector<std::uint8_t> command;
    command.reserve(8 + text.size());
    command.push_back(kClientCommandEditList);
    command.push_back(static_cast<std::uint8_t>(type));
    AppendQuad(&command, id);
    AppendString(&command, text);
    return command;
}

bool DecodeEditor(MapScanner* scanner, std::uint8_t opcode, EditorUpdate* output) {
    switch (opcode) {
        case kServerCommandEditText: {
            output->kind = EditorUpdateKind::Text;
            output->list = EditListUpdate{};
            EditTextUpdate& text = output->text;
            text = EditTextUpdate{};
            if (!ReadScannerQuad(scanner, &text.object_id, "edit text object id")) return false;
            if (!ReadScannerWord(scanner, &text.type_id, "edit text type id")) return false;
            if (!ReadScannerWord(scanner, &text.max_length, "edit text max length")) return false;
            // Always two strings. Every branch of SendEditText writes a text
            // and an editor, and the fallback writes two empty ones.
            if (!ReadScannerString(scanner, &text.text, "edit text body")) return false;
            return ReadScannerString(scanner, &text.editor, "edit text editor");
        }

        case kServerCommandEditList: {
            output->kind = EditorUpdateKind::List;
            output->text = EditTextUpdate{};
            EditListUpdate& list = output->list;
            list = EditListUpdate{};
            if (!ReadScannerByte(scanner, &list.type, "edit list type")) return false;
            list.known_type = IsKnownEditListType(list.type);
            if (!ReadScannerQuad(scanner, &list.id, "edit list id")) return false;
            return ReadScannerString(scanner, &list.text, "edit list body");
        }

        default:
            return FailScanner(scanner, MapDecodeError::UnexpectedCommand,
                               "not an editor command");
    }
}

}  // namespace fusion32::protocol772
