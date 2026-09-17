#ifndef FUSION32_PROTOCOL772_MAP_SCAN_H
#define FUSION32_PROTOCOL772_MAP_SCAN_H

#include "fusion32/protocol772/object_types.h"
#include "fusion32/protocol772/worldstate.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

// Highest floor index the server can address. Source:
// reference/game/src/sending.cc::SendFullScreen clamps EndZ with
// std::min<int>(PlayerZ + 2, 15).
constexpr std::int32_t kMaxFloor = 15;

// Source: reference/game/src/sending.cc::SendFullScreen. A player at or above
// the surface receives floors 7..0; otherwise PlayerZ-2 .. min(PlayerZ+2, 15).
constexpr std::int32_t kSurfaceFloor = 7;

enum class MapDecodeError {
    None,
    NotFullScreen,
    Truncated,
    InvalidPlayerFloor,
    ReservedObjectTypeId,
    UnknownObjectTypeId,
    TooManyObjectsInTile,
    CreatureNameTooLong,
    SkipRunExceedsWindow,
    EmptyObjectTypeTable,
    UnexpectedCommand,
    InvalidStackIndex,
    InvalidDirection,
    TrailingBytes,
};

const char* MapDecodeErrorName(MapDecodeError error) noexcept;

// The floor range SendFullScreen, SendRow and NotifyGo all derive from the
// viewport's current z.
struct FloorRange {
    std::int32_t first = 0;
    std::int32_t last = 0;
    std::int32_t step = 0;
    std::size_t count = 0;
};

// Source: reference/game/src/sending.cc::SendFullScreen lines 433-443, reused
// verbatim by SendRow.
FloorRange ViewportFloorRange(std::int32_t player_z) noexcept;

// Walks the tile / skip-marker stream shared by SV_CMD_FULLSCREEN,
// SV_CMD_ROW_*, SV_CMD_FLOOR_UP/DOWN and SV_CMD_FIELD_DATA. All four use
// SendMapPoint and SkipFlush unchanged; only the header and the scanned
// rectangle differ.
//
// `Skip` is a single file-scope counter in the server, reset once before the
// floor loop of a given command, so a run of empty tiles crosses floor
// boundaries within one command. The scanner carries that state across
// ScanRect calls and FinishRun checks it was fully consumed.
class MapScanner {
public:
    MapScanner(const std::vector<std::uint8_t>& bytes, std::size_t at,
               const ObjectTypeTable& types) noexcept;

    // Scans width*height positions in the server's x-outer / y-inner order,
    // appending every described tile to `floor`. `origin_x`/`origin_y` must
    // already include the floor offset.
    bool ScanRect(std::int32_t z, std::int32_t offset,
                  std::int32_t origin_x, std::int32_t origin_y,
                  std::int32_t width, std::int32_t height,
                  MapFloor* floor);

    // Verifies the trailing skip run did not outlive the scanned positions.
    bool FinishRun();

    std::size_t at() const noexcept { return at_; }
    MapDecodeError error() const noexcept { return error_; }
    std::size_t error_offset() const noexcept { return error_offset_; }
    const std::string& detail() const noexcept { return detail_; }
    bool ok() const noexcept { return error_ == MapDecodeError::None; }

    std::size_t described_tiles() const noexcept { return described_tiles_; }
    std::size_t skipped_tiles() const noexcept { return skipped_tiles_; }
    std::size_t thing_count() const noexcept { return thing_count_; }
    std::size_t creature_count() const noexcept { return creature_count_; }

private:
    bool Fail(MapDecodeError code, std::size_t offset, const char* text);
    bool ReadU8(std::uint8_t* output) noexcept;
    bool ReadU16(std::uint16_t* output) noexcept;
    bool ReadU32(std::uint32_t* output) noexcept;
    bool PeekU16(std::uint16_t* output) const noexcept;
    bool ReadRaw(std::size_t count, std::string* output);
    bool ReadRaw(std::size_t count, std::uint8_t* output) noexcept;
    bool ReadCreatureName(std::string* output);
    bool ReadOutfit(OutfitDescriptor* output);
    bool ReadCreatureDescriptor(CreatureThing* output);
    bool ReadDescribedTile(MapTile* tile, std::uint32_t* skip_after);

    // These helpers expose the scanner's bounds-checked reads to the command
    // decoders without widening the public surface.
    friend bool ReadStandaloneMapThing(MapScanner* scanner, MapThing* output);
    friend bool ReadViewportHeader(MapScanner* scanner, std::uint16_t* x,
                                   std::uint16_t* y, std::uint8_t* z);
    friend bool ReadFieldPosition(MapScanner* scanner, MapPosition* position);
    friend bool ReadScannerByte(MapScanner* scanner, std::uint8_t* value, const char* what);
    friend bool ReadScannerString(MapScanner* scanner, std::string* value, const char* what);
    bool ReadMapThing(MapThing* output);

    const std::vector<std::uint8_t>& bytes_;
    std::size_t at_;
    const ObjectTypeTable& types_;
    std::uint32_t skip_ = 0;
    MapDecodeError error_ = MapDecodeError::None;
    std::size_t error_offset_ = 0;
    std::string detail_;
    std::size_t described_tiles_ = 0;
    std::size_t skipped_tiles_ = 0;
    std::size_t thing_count_ = 0;
    std::size_t creature_count_ = 0;
};

// Reads exactly one object, as SV_CMD_ADD_FIELD and SV_CMD_CHANGE_FIELD carry
// it. No skip marker is involved.
bool ReadStandaloneMapThing(MapScanner* scanner, MapThing* output);

// Reads the LE word / LE word / byte position that SV_CMD_FULLSCREEN,
// SV_CMD_FIELD_DATA and the field commands all start with, rejecting a floor
// the server's loops cannot address.
bool ReadViewportHeader(MapScanner* scanner, std::uint16_t* x, std::uint16_t* y,
                        std::uint8_t* z);

// Reads a position without the floor bound check, for commands whose
// coordinates name an arbitrary field rather than a viewport centre.
bool ReadFieldPosition(MapScanner* scanner, MapPosition* position);

// Reads one unsigned byte through the scanner's bounds checking.
bool ReadScannerByte(MapScanner* scanner, std::uint8_t* value, const char* what);

// Reads a SendString-encoded text.
bool ReadScannerString(MapScanner* scanner, std::string* value, const char* what);

}  // namespace fusion32::protocol772

#endif
