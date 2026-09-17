#include "fusion32/protocol772/talk_speaker.h"

#include <algorithm>
#include <vector>

namespace fusion32::protocol772 {

TalkSpeakerResolution ResolveTalkSpeaker(const WorldState& state,
                                         const TalkUpdate& talk) {
    TalkSpeakerResolution resolution;
    if (!talk.has_position) {
        resolution.outcome = TalkSpeakerOutcome::NotPositional;
        return resolution;
    }

    // Only creatures actually on the map are candidates. known_creatures keeps
    // entries for creatures that have scrolled out of view, mirroring
    // TConnection::KnownCreatureTable, and those have no actor to speak above.
    const std::vector<std::uint32_t> visible = state.visible_creature_ids();

    std::uint32_t match = 0;
    std::size_t matches = 0;
    for (const std::uint32_t id : visible) {
        const auto found = state.known_creatures.find(id);
        if (found == state.known_creatures.end()) continue;
        const CreatureRecord& creature = found->second;

        if (creature.position.x != talk.position.x
            || creature.position.y != talk.position.y
            || creature.position.z != talk.position.z) {
            continue;
        }
        // A named talk must agree with a named creature. If either side lacks a
        // name the position has to carry the decision on its own, which it can:
        // a field holds one creature, which is why one player can block
        // another's step.
        if (!talk.speaker.empty() && !creature.name.empty()
            && creature.name != talk.speaker) {
            continue;
        }

        ++matches;
        if (matches == 1) {
            match = id;
        }
    }

    resolution.candidates = matches;
    if (matches == 1) {
        resolution.outcome = TalkSpeakerOutcome::Resolved;
        resolution.creature_id = match;
    } else if (matches == 0) {
        resolution.outcome = TalkSpeakerOutcome::NoMatch;
    } else {
        resolution.outcome = TalkSpeakerOutcome::Ambiguous;
    }
    return resolution;
}

const char* TalkSpeakerOutcomeName(TalkSpeakerOutcome outcome) noexcept {
    switch (outcome) {
        case TalkSpeakerOutcome::Resolved: return "Resolved";
        case TalkSpeakerOutcome::NoMatch: return "NoMatch";
        case TalkSpeakerOutcome::Ambiguous: return "Ambiguous";
        case TalkSpeakerOutcome::NotPositional: return "NotPositional";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
