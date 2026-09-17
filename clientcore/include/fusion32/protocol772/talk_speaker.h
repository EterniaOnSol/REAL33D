#ifndef FUSION32_PROTOCOL772_TALK_SPEAKER_H
#define FUSION32_PROTOCOL772_TALK_SPEAKER_H

#include <cstdint>

#include "fusion32/protocol772/movement.h"
#include "fusion32/protocol772/worldstate.h"

namespace fusion32::protocol772 {

// Works out which visible creature a positional talk came from.
//
// SV_CMD_TALK carries a sender name and a position; it does not carry a
// creature id. reference/game/src/sending.cc's positional SendTalk writes
// `Sender`, `Mode`, `x`, `y`, `z`, `Text` and nothing else, so the id has to be
// recovered from world state the client already holds.
//
// The rule is a match on both fields, never a nearest-creature guess:
//
//   * the creature must be visible, meaning it stands on a stored tile, not
//     merely present in the known-creature mirror, which deliberately retains
//     creatures that have scrolled out of view;
//   * its position must equal the talk position exactly;
//   * when the talk carries a name, that name must match too.
//
// An empty name is not a failure. The positional overload's only caller in
// reference/game/src/moveuse.cc passes "" for the ANIMAL modes, so the name is
// genuinely absent there and position alone has to decide.
//
// Anything other than exactly one match is reported as such. Attaching speech
// to the wrong creature is worse than not attaching it at all, so the caller is
// told to fall back rather than given a best guess.

enum class TalkSpeakerOutcome : std::uint8_t {
    // Exactly one visible creature matched. `creature_id` is valid.
    Resolved,
    // No visible creature matched. The speaker may have scrolled out between
    // speaking and the command arriving.
    NoMatch,
    // More than one visible creature matched, so no choice can be justified.
    Ambiguous,
    // The talk form carries no position, so there is nothing to resolve
    // against. Channel and private forms take this path.
    NotPositional,
};

struct TalkSpeakerResolution {
    TalkSpeakerOutcome outcome = TalkSpeakerOutcome::NoMatch;
    // Meaningful only when outcome is Resolved.
    std::uint32_t creature_id = 0;
    // How many visible creatures matched. Reported for evidence.
    std::size_t candidates = 0;
};

TalkSpeakerResolution ResolveTalkSpeaker(const WorldState& state,
                                         const TalkUpdate& talk);

const char* TalkSpeakerOutcomeName(TalkSpeakerOutcome outcome) noexcept;

}  // namespace fusion32::protocol772

#endif
