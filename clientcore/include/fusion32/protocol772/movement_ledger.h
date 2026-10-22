#ifndef FUSION32_PROTOCOL772_MOVEMENT_LEDGER_H
#define FUSION32_PROTOCOL772_MOVEMENT_LEDGER_H

#include <cstddef>
#include <cstdint>
#include <deque>

#include "fusion32/protocol772/movement.h"
#include "fusion32/protocol772/worldstate.h"

namespace fusion32::protocol772 {

// Tells apart the two reasons the local player's position can change.
//
// Fusion32 moves a player for its own reasons as well as ours. In 7.72 one
// creature can displace another: reference/game/src/cract.cc TCreature::Move
// relocates whoever occupies the destination field, so another player walking
// into this one pushes them a field. From the wire that arrives as an ordinary
// SV_CMD_MOVE_CREATURE naming this client's creature, indistinguishable from
// the answer to a walk this client asked for.
//
// A live session made the cost of not distinguishing them concrete: the local
// player travelled eight fields while the client had sent zero walk commands,
// and a counter named "accepted steps" reported eight. Read as acceptance of
// requests that were never made, that is evidence of something that did not
// happen. The ledger exists so the client can only claim a step was its own
// when it actually asked for one.
//
// The rule is deliberately narrow: a move counts as this client's walk only if
// a walk is outstanding AND the field the player landed on is the field that
// walk asked for. Everything else is external, including a push, a teleport and
// a relocation the client has no explanation for.

enum class LocalMoveCause : std::uint8_t {
    // The oldest outstanding walk asked for exactly this step.
    SelfWalkAccepted,
    // Fusion32 moved this player for a reason of its own.
    ExternalRelocation,
    // Origin and destination are the same field, so nothing actually happened.
    NoMovement,
};

// Which of 7.72's two step shapes a request was.
//
// Both are walks and both are answered the same way -- an ordinary
// SV_CMD_MOVE_CREATURE naming this client's creature -- so the ledger has to
// know which it asked for to work out where it asked to land. A diagonal step
// changes both axes, and `StepPosition` cannot describe one.
//
// `cardinal` is meaningful only when `diagonal` is false, and `diagonal_direction`
// only when it is true. Readers that predate diagonals keep reading `cardinal`
// and keep getting what they always got, because a cardinal request still
// fills it exactly as before.
struct WalkStep {
    bool diagonal = false;
    CardinalDirection cardinal = CardinalDirection::North;
    DiagonalDirection diagonal_direction = DiagonalDirection::Northeast;

    // The field this step asked for, by whichever rule applies.
    MapPosition Apply(const MapPosition& from) const noexcept;

    // "north", "northeast", and so on. Never null.
    const char* Name() const noexcept;
};

struct LocalMoveOutcome {
    LocalMoveCause cause = LocalMoveCause::ExternalRelocation;
    // Non-zero only for SelfWalkAccepted.
    std::uint32_t request_id = 0;
    CardinalDirection direction = CardinalDirection::North;
    // The whole step, for a caller that has to tell a diagonal from a cardinal.
    // `direction` above stays the cardinal, unchanged, for every caller that
    // only ever asks about one.
    WalkStep step;
};

struct MovementLedgerCounts {
    // Walk commands this client actually put on the wire.
    std::uint32_t requested = 0;
    // Requests Fusion32 answered by moving us to the field we asked for.
    std::uint32_t accepted = 0;
    // Requests Fusion32 refused, counted from SV_CMD_SNAPBACK.
    std::uint32_t rejected = 0;
    // Position changes that were not ours: pushes, teleports, anything else.
    std::uint32_t external = 0;
    // Requests that were neither accepted nor refused before expiry.
    std::uint32_t unanswered = 0;
};

class MovementLedger {
public:
    // Records that a walk command was handed to the transport. Returns the id
    // that identifies this request in evidence.
    std::uint32_t NoteRequestSent(CardinalDirection direction, double sent_at) noexcept;

    // The same, for one of the four diagonal walks (CL_CMD_GO_* 106..109).
    //
    // Bookkeeping only: nothing about the wire changes here, and the command
    // itself is still built by BuildDiagonalWalkCommand. This exists because
    // without it an accepted diagonal would be counted as an external
    // relocation -- the destination never equals StepPosition(from, cardinal)
    // -- and the request would then expire unanswered. Both counters are read
    // as evidence of a defect, so a diagonal that worked perfectly would have
    // reported as two.
    std::uint32_t NoteRequestSent(DiagonalDirection direction, double sent_at) noexcept;

    // Records a server-reported move of the local player, and says whose it was.
    LocalMoveOutcome NoteLocalMove(const MapPosition& from,
                                   const MapPosition& to,
                                   double at) noexcept;

    // Records a refusal, and says which request it answered so the refusal can
    // be reported against the direction that was actually asked for.
    // `matched` is false when nothing was outstanding, which is itself a
    // surprise worth reporting rather than papering over.
    struct SnapbackMatch {
        bool matched = false;
        std::uint32_t request_id = 0;
        CardinalDirection direction = CardinalDirection::North;
        // As in LocalMoveOutcome: `direction` stays the cardinal and this
        // carries the whole step for a caller that needs to tell them apart.
        WalkStep step;
    };
    SnapbackMatch NoteSnapback(double at) noexcept;

    // Drops requests older than the deadline and counts them unanswered, so one
    // lost reply cannot mis-attribute every later move.
    std::size_t ExpireBefore(double deadline) noexcept;

    void Reset() noexcept;

    const MovementLedgerCounts& counts() const noexcept { return counts_; }
    std::size_t outstanding() const noexcept { return pending_.size(); }

private:
    struct Pending {
        std::uint32_t id = 0;
        WalkStep step;
        double sent_at = 0.0;
    };

    std::deque<Pending> pending_;
    MovementLedgerCounts counts_;
    std::uint32_t next_id_ = 0;
};

const char* LocalMoveCauseName(LocalMoveCause cause) noexcept;

}  // namespace fusion32::protocol772

#endif
