#pragma once
#include <algorithm>
#include <cmath>
namespace Real33D {
// Local map camera only; never changes player/gameplay state.
struct MinimapView {
    double center_x = 0, center_y = 0;
    int floor = 7;
    double scale = 3;
    bool follow = true, player_known = false;
    int player_x = 0, player_y = 0, player_z = 7;
    void SetPlayer(int x, int y, int z, bool known) {
        if (known && (!player_known || z != player_z)) { floor = z; follow = true; }
        player_x = x; player_y = y; player_z = z; player_known = known;
        if (known && follow) { center_x = x; center_y = y; floor = z; }
    }
    void Recenter() { follow = true; if (player_known) { center_x = player_x; center_y = player_y; floor = player_z; } }
    void Pan(double pixels_x, double pixels_y) {
        follow = false;
        center_x = std::clamp(center_x - pixels_x / scale, 0.0, 65535.0);
        center_y = std::clamp(center_y - pixels_y / scale, 0.0, 65535.0);
    }
    void Zoom(bool in) { scale = std::clamp(scale * (in ? 2.0 : 0.5), 1.5, 12.0); }
    void BrowseFloor(int delta) { follow = false; floor = std::clamp(floor + delta, 0, 15); }
    double PixelX(int x, double width) const { return width / 2 + (x - center_x) * scale; }
    double PixelY(int y, double height) const { return height / 2 + (y - center_y) * scale; }
    int MapX(double px, double width) const { return static_cast<int>(std::floor(center_x + (px-width/2)/scale + 0.5)); }
    int MapY(double py, double height) const { return static_cast<int>(std::floor(center_y + (py-height/2)/scale + 0.5)); }
};
}
