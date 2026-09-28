#include "../unreal/REAL33D/Source/REAL33D/Public/Real33DMinimapView.h"
#include "../unreal/REAL33D/Source/REAL33D/Public/Real33DMinimapPalette.h"
#include <iostream>
#include <stdexcept>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
int main() {
    try {
        Real33D::MinimapPalette palette;
        const std::string source = "REAL33D_MINIMAP_PALETTE_1 3c5e857ff72fd1e52879eb8845adc72c0991786ad0eaf85f6847595c9677aedd\n";
        CHECK(palette.Load(source + "452 24\n200 129\n"));
        CHECK(palette.colors.at(452) == 24);
        const auto original = palette.colors;
        for (const auto& suffix : {"452 216\n", "99 24\n", "452 24\n452 30\n", "452\n", "garbage"}) {
            CHECK(!palette.Load(source + suffix)); CHECK(palette.colors == original);
        }
        CHECK(!palette.Load("REAL33D_MINIMAP_PALETTE_1 wrong_source\n452 24\n"));
        Real33D::MinimapView v;
        v.SetPlayer(32098,32205,7,true);
        CHECK(v.PixelX(32098,114)==57); CHECK(v.PixelY(32205,110)==55);
        CHECK(v.PixelX(32099,114)==60); CHECK(v.PixelY(32204,110)==52);
        CHECK(v.MapX(60,114)==32099); CHECK(v.MapY(52,110)==32204);
        v.Pan(6,0); CHECK(!v.follow); CHECK(v.center_x==32096);
        const auto before = v.PixelX(32098,114);
        v.SetPlayer(32099,32205,7,true);
        CHECK(v.PixelX(32099,114)-before==3);
        v.Zoom(true); CHECK(v.scale==6); CHECK(v.MapX(v.PixelX(32099,114),114)==32099);
        v.BrowseFloor(-1); CHECK(v.floor==6); CHECK(v.player_z==7);
        v.SetPlayer(32099,32206,8,true); CHECK(v.floor==8); CHECK(v.follow);
        v.BrowseFloor(-1); v.Recenter(); CHECK(v.floor==8); CHECK(v.center_y==32206);
        v.SetPlayer(32099,32206,7,true); CHECK(v.floor==7);
        for (int i=0;i<20;++i) v.Zoom(true);
        CHECK(v.scale==12);
        for (int i=0;i<20;++i) v.Zoom(false);
        CHECK(v.scale==1.5);
        v.BrowseFloor(100); CHECK(v.floor==15); v.BrowseFloor(-100); CHECK(v.floor==0);
        v.SetPlayer(0,0,0,false); CHECK(!v.player_known);
        std::cout << "real33d_minimap_view_tests: PASS (palette validation/isolation, north-up, inverse clicks, delta, pan, zoom bounds, floors/recenter/offline)\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
