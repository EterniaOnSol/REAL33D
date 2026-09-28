#pragma once
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
namespace Real33D {
// Presentation metadata only, loaded separately from observed map knowledge.
struct MinimapPalette {
    std::map<std::uint16_t, std::uint8_t> colors;
    bool Load(const std::string& text) {
        if (text.size() > 128U * 1024U) return false;
        std::istringstream in(text);
        std::string magic, source;
        if (!(in >> magic >> source) || magic != "REAL33D_MINIMAP_PALETTE_1"
            || source != "3c5e857ff72fd1e52879eb8845adc72c0991786ad0eaf85f6847595c9677aedd") return false;
        std::map<std::uint16_t, std::uint8_t> parsed;
        for (;;) {
            in >> std::ws;
            if (in.eof()) break;
            unsigned int type = 0, color = 0;
            if (!(in >> type >> color) || type < 100 || type >= 65280 || color == 0
                || color >= 216 || parsed.count(static_cast<std::uint16_t>(type))) return false;
            parsed.emplace(static_cast<std::uint16_t>(type), static_cast<std::uint8_t>(color));
        }
        colors.swap(parsed);
        return true;
    }
};
}
