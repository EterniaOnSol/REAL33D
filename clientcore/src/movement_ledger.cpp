#include "fusion32/protocol772/movement_ledger.h"

namespace fusion32::protocol772 {

std::uint32_t MovementLedger::NoteRequestSent(CardinalDirection direction,
                                             double sent_at) noexcept {
    ++next_id_;
    pending_.push_back(Pending{next_id_, direction, sent_at});
    ++counts_.requested;
    return next_id_;
}

LocalMoveOutcome MovementLedger::NoteLocalMove(const MapPosition& from,
                                              const MapPosition& to,
                                              double at) noexcept {
    (void)at;
    LocalMoveOutcome outcome;

    // A move that does not move is not a relocation. The server re-announces a
    // position after refusing a step, which arrives as a move whose origin and
    // destination are the same field. Counting it would inflate the external
    // total, and that total is read as evidence of how often something other
    // than this client moved the player.
    if (from.x == to.x && from.y == to.y && from.z == to.z) {
        outcome.cause = LocalMoveCause::NoMovement;
        return outcome;
    }

    if (pending_.empty()) {
        ++counts_.external;
        return outcome;
    }

    // Only the oldest outstanding request can be the one being answered: the
    // server handles a connection's commands in order.
    const Pending& oldest = pending_.front();
    const MapPosition expected = StepPosition(from, oldest.direction);
    if (!(expected.x == to.x && expected.y == to.y && expected.z == to.z)) {
        // We asked to go somewhere and ended up somewhere else. That is not our
        // step, and the request stays outstanding: a push does not consume it.
        ++counts_.external;
        return outcome;
    }

    outcome.cause = LocalMoveCause::SelfWalkAccepted;
    outcome.request_id = oldest.id;
    outcome.direction = oldest.direction;
    pending_.pop_front();
    ++counts_.accepted;
    return outcome;
}

MovementLedger::SnapbackMatch MovementLedger::NoteSnapback(double at) noexcept {
    (void)at;
    ++counts_.rejected;
    SnapbackMatch match;
    if (pending_.empty()) return match;
    match.matched = true;
    match.request_id = pending_.front().id;
    match.direction = pending_.front().direction;
    pending_.pop_front();
    return match;
}

std::size_t MovementLedger::ExpireBefore(double deadline) noexcept {
    std::size_t expired = 0;
    while (!pending_.empty() && pending_.front().sent_at < deadline) {
        pending_.pop_front();
        ++counts_.unanswered;
        ++expired;
    }
    return expired;
}

void MovementLedger::Reset() noexcept {
    pending_.clear();
    counts_ = MovementLedgerCounts{};
    next_id_ = 0;
}

const char* LocalMoveCauseName(LocalMoveCause cause) noexcept {
    switch (cause) {
        case LocalMoveCause::SelfWalkAccepted: return "SelfWalkAccepted";
        case LocalMoveCause::ExternalRelocation: return "ExternalRelocation";
        case LocalMoveCause::NoMovement: return "NoMovement";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
