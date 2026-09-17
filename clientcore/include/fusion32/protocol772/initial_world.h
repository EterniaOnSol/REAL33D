#ifndef FUSION32_PROTOCOL772_INITIAL_WORLD_H
#define FUSION32_PROTOCOL772_INITIAL_WORLD_H

#include "fusion32/protocol772/map_scan.h"
#include "fusion32/protocol772/object_types.h"
#include "fusion32/protocol772/worldstate.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

// Source: reference/game/src/connections.hh, enum ServerCommand.
constexpr std::uint8_t kServerCommandFullScreen = 100;

struct FullScreenMessage {
    MapWindow window;
    std::int32_t first_floor = 0;
    std::int32_t last_floor = 0;
    std::int32_t floor_step = 0;
    std::vector<MapFloor> floors;
    std::size_t described_tiles = 0;
    std::size_t skipped_tiles = 0;
    std::size_t thing_count = 0;
    std::size_t creature_count = 0;
    std::size_t bytes_consumed = 0;

    std::size_t scanned_positions() const noexcept {
        return floors.size() * static_cast<std::size_t>(kTerminalWidth)
             * static_cast<std::size_t>(kTerminalHeight);
    }
};

struct FullScreenDecodeResult {
    FullScreenMessage message;
    MapDecodeError error = MapDecodeError::None;
    std::size_t error_offset = 0;
    std::string detail;
    // Everything the payload still holds after the FULLSCREEN command. The
    // 7.72 login path commits several commands into one encrypted frame, so
    // this is normally non-empty and must never be dropped.
    std::vector<std::uint8_t> remaining_bytes;

    bool ok() const noexcept { return error == MapDecodeError::None; }
};

// Decodes one SV_CMD_FULLSCREEN starting at `offset`. `types` supplies the
// LIQUIDCONTAINER/LIQUIDPOOL/CUMULATIVE flags that decide an item's on-wire
// length; without it the stream cannot be walked.
FullScreenDecodeResult DecodeFullScreen(const std::vector<std::uint8_t>& bytes,
                                        std::size_t offset,
                                        const ObjectTypeTable& types);

FullScreenDecodeResult DecodeFullScreen(const std::vector<std::uint8_t>& bytes,
                                        const ObjectTypeTable& types);

struct WorldStateApplyResult {
    std::vector<WorldStateAnomaly> anomalies;

    bool clean() const noexcept { return anomalies.empty(); }
};

// Replaces the map window/floors and folds every creature descriptor into the
// known-creature mirror, in emission order. Anomalies are reported, never
// swallowed.
WorldStateApplyResult ApplyFullScreen(WorldState* state, const FullScreenMessage& message);

// One server command the decoder recognizes by number but does not parse.
struct UnsupportedServerCommand {
    std::uint8_t opcode = 0;
    const char* name = "Unknown";
    std::size_t offset = 0;
};

enum class InitialWorldStatus {
    FullScreenDecoded,
    NoFullScreen,
    DecodeFailed,
};

struct InitialWorldResult {
    InitialWorldStatus status = InitialWorldStatus::NoFullScreen;
    FullScreenDecodeResult fullscreen;
    // First command left in the payload after FULLSCREEN, if any. The bytes
    // themselves stay in `fullscreen.remaining_bytes`.
    bool has_trailing_command = false;
    UnsupportedServerCommand trailing_command;

    bool ok() const noexcept { return status == InitialWorldStatus::FullScreenDecoded; }
};

// Consumes the tail that ParseGameInitialMessage preserved for a recognized but
// unparsed FULLSCREEN, and reports what follows it without parsing it.
InitialWorldResult DecodeInitialWorld(const std::vector<std::uint8_t>& preserved_bytes,
                                      const ObjectTypeTable& types);

const char* InitialWorldStatusName(InitialWorldStatus status) noexcept;

// Names every value of reference/game/src/connections.hh::ServerCommand so an
// unsupported opcode is reported by name instead of being discarded.
const char* ServerCommandName(std::uint8_t opcode) noexcept;

}  // namespace fusion32::protocol772

#endif
