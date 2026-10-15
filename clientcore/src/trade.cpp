#include "fusion32/protocol772/trade.h"

// BuildTradeObjectCommand is declared in movement.h, beside the other
// builders that take a MoveEndpoint, and implemented here with the rest of
// trade. trade.h cannot include movement.h: movement.h includes trade.h.
#include "fusion32/protocol772/movement.h"

namespace fusion32::protocol772 {

namespace {

// x/y/z, type id, stack index: the object reference CTradeObject reads with
// the same five readWord/readByte calls CMoveObject and CUseObject use.
void AppendObjectRef(std::vector<std::uint8_t>* command, const MoveEndpoint& where,
                     std::uint16_t type_id, std::uint8_t stack_index) {
    const auto word = [command](std::uint16_t value) {
        command->push_back(static_cast<std::uint8_t>(value & 0xFF));
        command->push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    };
    word(where.x);
    word(where.y);
    command->push_back(where.z);
    word(type_id);
    command->push_back(stack_index);
}

void AppendQuad(std::vector<std::uint8_t>* command, std::uint32_t value) {
    command->push_back(static_cast<std::uint8_t>(value & 0xFF));
    command->push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    command->push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    command->push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
}

}  // namespace

std::vector<std::uint8_t> BuildTradeObjectCommand(const MoveEndpoint& object,
                                                  std::uint16_t type_id,
                                                  std::uint8_t stack_index,
                                                  std::uint32_t trade_partner) {
    std::vector<std::uint8_t> command;
    command.reserve(13);
    command.push_back(kClientCommandTradeObject);
    AppendObjectRef(&command, object, type_id, stack_index);
    AppendQuad(&command, trade_partner);
    return command;
}

std::vector<std::uint8_t> BuildInspectTradeCommand(TradeOfferSide side,
                                                   std::uint8_t position) {
    std::vector<std::uint8_t> command;
    command.reserve(3);
    command.push_back(kClientCommandInspectTrade);
    // Zero means the own offer. See TradeOfferSideByte.
    command.push_back(TradeOfferSideByte(side));
    command.push_back(position);
    return command;
}

std::vector<std::uint8_t> BuildAcceptTradeCommand() {
    return {kClientCommandAcceptTrade};
}

std::vector<std::uint8_t> BuildRejectTradeCommand() {
    return {kClientCommandRejectTrade};
}

bool DecodeTrade(MapScanner* scanner, std::uint8_t opcode, TradeUpdate* output) {
    switch (opcode) {
        case kServerCommandCloseTrade:
            // SendCloseTrade writes the opcode and nothing else.
            output->kind = TradeUpdateKind::Closed;
            output->name.clear();
            output->items.clear();
            return true;

        case kServerCommandTradeOfferOwn:
            output->kind = TradeUpdateKind::OwnOffer;
            break;

        case kServerCommandTradeOfferPartner:
            output->kind = TradeUpdateKind::PartnerOffer;
            break;

        default:
            return FailScanner(scanner, MapDecodeError::UnexpectedCommand,
                               "not a trade command");
    }

    // SendTradeOffer's own order: the name, the object count, and then that
    // many SendItem objects.
    output->name.clear();
    output->items.clear();
    if (!ReadScannerString(scanner, &output->name, "trade partner name")) return false;

    std::uint8_t count = 0;
    if (!ReadScannerByte(scanner, &count, "trade object count")) return false;

    // A byte cannot exceed the UINT8_MAX ceiling SendTradeOffer enforces, so
    // there is no count to reject here; reserving is safe for the same
    // reason. Every object still has to actually be present, which is what
    // the loop below proves.
    output->items.reserve(count);
    for (std::uint8_t index = 0; index < count; ++index) {
        ItemThing item;
        if (!ReadScannerItem(scanner, &item)) return false;
        output->items.push_back(item);
    }
    return true;
}

}  // namespace fusion32::protocol772
