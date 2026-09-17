#include "fusion32/protocol772/object_types.h"

#include <cstdlib>

namespace fusion32::protocol772 {
namespace {

// The server table itself is unbounded (`vector<TObjectType> ObjectTypes` in
// reference/game/src/objects.cc grows on demand) but the wire carries the type
// id as a little-endian word, and `kSkipMarkerBase` reserves the top page.
constexpr std::uint32_t kMaxDeclarableTypeId = kSkipMarkerBase - 1U;

std::string StripComment(const std::string& line) {
    bool in_string = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '"') in_string = !in_string;
        else if (line[i] == '#' && !in_string) return line.substr(0, i);
    }
    return line;
}

std::string Trim(const std::string& text) {
    std::size_t first = 0;
    while (first < text.size()
           && (text[first] == ' ' || text[first] == '\t' || text[first] == '\r')) {
        ++first;
    }
    std::size_t last = text.size();
    while (last > first
           && (text[last - 1] == ' ' || text[last - 1] == '\t' || text[last - 1] == '\r')) {
        --last;
    }
    return text.substr(first, last - first);
}

bool SplitRecord(const std::string& line, std::string* key, std::string* value) {
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) return false;
    *key = Trim(line.substr(0, equals));
    *value = Trim(line.substr(equals + 1));
    return true;
}

bool ParseUnsigned(const std::string& text, std::uint32_t* output) {
    if (text.empty()) return false;
    std::uint64_t value = 0;
    for (const char digit : text) {
        if (digit < '0' || digit > '9') return false;
        value = value * 10U + static_cast<std::uint64_t>(digit - '0');
        if (value > 0xFFFFFFFFULL) return false;
    }
    *output = static_cast<std::uint32_t>(value);
    return true;
}

// `Flags = {A,B,C}` on a single line. Verified against the shipped
// dat/objects.srv: all 4995 flag records are single-line and no `Name` string
// contains a `#`.
bool ParseFlags(const std::string& value, ObjectTypeEncoding* encoding) {
    if (value.size() < 2 || value.front() != '{' || value.back() != '}') return false;
    const std::string body = value.substr(1, value.size() - 2);
    bool bank = false;
    bool clip = false;
    bool bottom = false;
    bool top = false;
    std::size_t at = 0;
    while (at <= body.size()) {
        const std::size_t comma = body.find(',', at);
        const std::size_t end = comma == std::string::npos ? body.size() : comma;
        const std::string flag = Trim(body.substr(at, end - at));
        if (!flag.empty()) {
            if (flag == "LiquidContainer" || flag == "LiquidPool") {
                encoding->liquid_color = true;
            } else if (flag == "Cumulative") {
                encoding->cumulative = true;
            } else if (flag == "Bank") {
                bank = true;
            } else if (flag == "Clip") {
                clip = true;
            } else if (flag == "Bottom") {
                bottom = true;
            } else if (flag == "Top") {
                top = true;
            }
        }
        if (comma == std::string::npos) break;
        at = comma + 1;
    }

    // Same order as GetObjectPriority: the first matching flag wins.
    if (bank) {
        encoding->priority = ObjectPriority::Bank;
    } else if (clip) {
        encoding->priority = ObjectPriority::Clip;
    } else if (bottom) {
        encoding->priority = ObjectPriority::Bottom;
    } else if (top) {
        encoding->priority = ObjectPriority::Top;
    } else {
        encoding->priority = ObjectPriority::Low;
    }
    return true;
}

}  // namespace

bool ObjectTypeTable::Declare(std::uint16_t type_id, ObjectTypeEncoding encoding) {
    if (type_id >= kSkipMarkerBase) return false;
    if (entries_.size() <= type_id) {
        entries_.resize(static_cast<std::size_t>(type_id) + 1U);
    }
    if (entries_[type_id].known) return false;
    encoding.known = true;
    entries_[type_id] = encoding;
    declared_ += 1;
    if (type_id > max_type_id_) max_type_id_ = type_id;
    return true;
}

ObjectTypeEncoding ObjectTypeTable::Lookup(std::uint16_t type_id) const noexcept {
    if (type_id >= entries_.size()) return ObjectTypeEncoding{};
    return entries_[type_id];
}

std::size_t MapStackInsertIndex(const std::vector<ObjectPriority>& existing,
                                ObjectPriority inserted) noexcept {
    // PlaceObject forces append for everything that is neither a creature nor
    // a plain low-priority object.
    const bool append = inserted != ObjectPriority::Creature
                     && inserted != ObjectPriority::Low;
    std::size_t index = 0;
    while (index < existing.size()) {
        const ObjectPriority current = existing[index];
        if (append) {
            if (current > inserted) break;
        } else {
            if (current >= inserted) break;
        }
        index += 1;
    }
    return index;
}

const char* ObjectPriorityName(ObjectPriority priority) noexcept {
    switch (priority) {
        case ObjectPriority::Bank: return "Bank";
        case ObjectPriority::Clip: return "Clip";
        case ObjectPriority::Bottom: return "Bottom";
        case ObjectPriority::Top: return "Top";
        case ObjectPriority::Creature: return "Creature";
        case ObjectPriority::Low: return "Low";
    }
    return "Unknown";
}

ObjectTypeTableLoadResult LoadObjectTypeTableFromObjectsSrv(const std::string& text) {
    ObjectTypeTableLoadResult result;
    bool have_type = false;
    std::uint16_t type_id = 0;
    ObjectTypeEncoding encoding;
    std::size_t line_number = 0;
    std::size_t at = 0;

    const auto flush = [&]() {
        if (!have_type) return true;
        // GetObjectPriority tests isCreatureContainer after the flags, so the
        // creature container always outranks whatever its record declares.
        if (type_id == kTypeIdCreatureContainer) {
            encoding.priority = ObjectPriority::Creature;
        }
        if (!result.table.Declare(type_id, encoding)) {
            result.error = ObjectTypeTableError::DuplicateTypeId;
            result.detail = "type id declared twice";
            return false;
        }
        have_type = false;
        return true;
    };

    while (at <= text.size()) {
        const std::size_t newline = text.find('\n', at);
        const std::size_t end = newline == std::string::npos ? text.size() : newline;
        const std::string line = Trim(StripComment(text.substr(at, end - at)));
        line_number += 1;
        result.line = line_number;

        std::string key;
        std::string value;
        if (!line.empty() && SplitRecord(line, &key, &value)) {
            if (key == "TypeID") {
                if (!flush()) return result;
                std::uint32_t parsed = 0;
                if (!ParseUnsigned(value, &parsed)) {
                    result.error = ObjectTypeTableError::InvalidTypeId;
                    result.detail = "TypeID is not a decimal number";
                    return result;
                }
                if (parsed > kMaxDeclarableTypeId) {
                    result.error = ObjectTypeTableError::TypeIdOutOfRange;
                    result.detail = "TypeID collides with the skip-marker page";
                    return result;
                }
                type_id = static_cast<std::uint16_t>(parsed);
                encoding = ObjectTypeEncoding{};
                have_type = true;
            } else if (key == "Flags") {
                if (!have_type) {
                    result.error = ObjectTypeTableError::MissingTypeId;
                    result.detail = "Flags record before any TypeID";
                    return result;
                }
                if (!ParseFlags(value, &encoding)) {
                    result.error = ObjectTypeTableError::MalformedFlags;
                    result.detail = "Flags record is not a single-line brace list";
                    return result;
                }
            }
        }

        if (newline == std::string::npos) break;
        at = newline + 1;
    }

    if (!flush()) return result;
    result.line = 0;
    return result;
}

const char* ObjectTypeTableErrorName(ObjectTypeTableError error) noexcept {
    switch (error) {
        case ObjectTypeTableError::None: return "None";
        case ObjectTypeTableError::MissingTypeId: return "MissingTypeId";
        case ObjectTypeTableError::InvalidTypeId: return "InvalidTypeId";
        case ObjectTypeTableError::TypeIdOutOfRange: return "TypeIdOutOfRange";
        case ObjectTypeTableError::MalformedFlags: return "MalformedFlags";
        case ObjectTypeTableError::DuplicateTypeId: return "DuplicateTypeId";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
