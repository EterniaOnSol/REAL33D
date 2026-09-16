#ifndef FUSION32_PROTOCOL772_OBJECT_TYPES_H
#define FUSION32_PROTOCOL772_OBJECT_TYPES_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

// Source: reference/game/src/objects.hh, enum TYPEID_*.
// Ids 0..10 are server-internal container types and 99 is the creature
// container. reference/game/src/objects.cc::GetObjectTypeByName iterates real
// object types from TYPEID_CREATURE_CONTAINER + 1 upwards.
constexpr std::uint16_t kTypeIdMapContainer = 0;
constexpr std::uint16_t kTypeIdLastBodyContainer = 10;
constexpr std::uint16_t kTypeIdCreatureContainer = 99;
constexpr std::uint16_t kFirstMapObjectTypeId = 100;

// Source: reference/game/src/sending.cc::SendMapObject. A creature occupies the
// wire slot of an object and is introduced by one of these three words.
constexpr std::uint16_t kCreatureMarkerNew = 97;
constexpr std::uint16_t kCreatureMarkerOutdated = 98;
constexpr std::uint16_t kCreatureMarkerKnown = 99;

// Source: reference/game/src/sending.cc::SkipFlush. A skip marker is the byte
// pair (Count, 0xFF), which reads as a little-endian word >= 0xFF00.
constexpr std::uint16_t kSkipMarkerBase = 0xFF00;

// Source: reference/game/src/sending.cc, MAX_OBJECTS_PER_POINT.
constexpr std::size_t kMapObjectsPerPointLimit = 10;

// Per-type information the wire format depends on but does not carry.
// Source: reference/game/src/sending.cc::SendItem reads LIQUIDCONTAINER,
// LIQUIDPOOL and CUMULATIVE from the server object type table, so a decoder
// cannot know an item's on-wire length from the type id alone.
struct ObjectTypeEncoding {
    bool known = false;
    bool liquid_color = false;  // LIQUIDCONTAINER or LIQUIDPOOL
    bool cumulative = false;    // CUMULATIVE

    // Extra bytes that follow the type id word on the wire.
    std::size_t extra_bytes() const noexcept {
        return (liquid_color ? 1U : 0U) + (cumulative ? 1U : 0U);
    }
};

enum class ObjectTypeTableError {
    None,
    MissingTypeId,
    InvalidTypeId,
    TypeIdOutOfRange,
    MalformedFlags,
    DuplicateTypeId,
};

class ObjectTypeTable {
public:
    ObjectTypeTable() = default;

    // Declares one type. Returns false when the id repeats or exceeds the
    // configured ceiling.
    bool Declare(std::uint16_t type_id, ObjectTypeEncoding encoding);

    ObjectTypeEncoding Lookup(std::uint16_t type_id) const noexcept;
    bool empty() const noexcept { return declared_ == 0; }
    std::size_t declared() const noexcept { return declared_; }
    std::uint16_t max_type_id() const noexcept { return max_type_id_; }

private:
    std::vector<ObjectTypeEncoding> entries_;
    std::size_t declared_ = 0;
    std::uint16_t max_type_id_ = 0;
};

struct ObjectTypeTableLoadResult {
    ObjectTypeTable table;
    ObjectTypeTableError error = ObjectTypeTableError::None;
    std::size_t line = 0;
    std::string detail;

    bool ok() const noexcept { return error == ObjectTypeTableError::None; }
};

// Parses the `TypeID` and `Flags` records of the server's `dat/objects.srv`.
// Only the three wire-relevant flags are retained; every other record is
// ignored by design and never alters the produced encoding.
ObjectTypeTableLoadResult LoadObjectTypeTableFromObjectsSrv(const std::string& text);

const char* ObjectTypeTableErrorName(ObjectTypeTableError error) noexcept;

}  // namespace fusion32::protocol772

#endif
