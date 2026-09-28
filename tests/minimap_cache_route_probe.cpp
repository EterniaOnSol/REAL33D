// Replays only a legitimate client-owned terrain cache. No network or gameplay
// commands; output is explicitly an estimate, not proof of successful movement.
#include "fusion32/protocol772/minimap.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
using namespace fusion32::protocol772;
int main(int argc, char** argv) {
    try {
        if (argc != 8) throw std::runtime_error("usage: probe cache startX startY startZ goalX goalY goalZ");
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) throw std::runtime_error("cache not readable");
        file.seekg(0,std::ios::end);
        if (file.tellg() < 0 || file.tellg() > 32*1024*1024) throw std::runtime_error("cache size rejected");
        file.seekg(0);
        const std::string text((std::istreambuf_iterator<char>(file)),{});
        std::istringstream header(text);
        std::string magic, scope;
        header >> magic >> scope;
        KnownMinimap map;
        if (!map.Load(text,scope)) throw std::runtime_error("cache rejected");
        const MapPosition start{std::stoi(argv[2]),std::stoi(argv[3]),std::stoi(argv[4])};
        const MapPosition goal{std::stoi(argv[5]),std::stoi(argv[6]),std::stoi(argv[7])};
        std::vector<MapPosition> path;
        const bool found = map.FindNavigationPath(start,goal,{},path);
        std::cout << "ESTIMATE_ONLY=YES\nNETWORK_COMMANDS=0\nLIVE_OBSERVATIONS=" << map.live_count()
            << "\nKNOWN_CELLS=" << map.terrain().size() << "\nSTART=" << start.x << ',' << start.y << ',' << start.z
            << "\nGOAL=" << goal.x << ',' << goal.y << ',' << goal.z
            << "\nGOAL_KNOWN=" << (map.terrain().count(goal) ? "YES" : "NO")
            << "\nOBSERVED_SEGMENT=" << (found ? "FOUND" : "NONE")
            << "\nREACHES_GOAL=" << (found && (path.empty() || path.back() == goal) ? "YES" : "NO")
            << "\nESTIMATED_STEPS=" << path.size() << '\n';
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
