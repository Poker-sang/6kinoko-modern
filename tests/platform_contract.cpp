#include "kinoko/platform.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"line %d: %s (%s)\n",__LINE__,#x,SDL_GetError()); return 1; } } while(0)
int main() {
    using namespace kinoko::platform;
    CHECK(legacy_scan(SDL_SCANCODE_Z)==0x2c);
    CHECK(legacy_scan(SDL_SCANCODE_UP)==0xc8);
    CHECK(legacy_scan(SDL_SCANCODE_KP_8)==0x48);
    CHECK(legacy_scan(SDL_SCANCODE_RCTRL)==0x9d);
    CHECK(legacy_scan(SDL_SCANCODE_UNKNOWN)==-1);
    CHECK(legacy_axis(-32768)==-1000);
    CHECK(legacy_axis(32767)==1000);
    CHECK(legacy_axis(0)==0);
    Platform platform;
    CHECK(platform.open("contract",640,480,true));
    CHECK(!platform.open("second",640,480,true));
    CHECK(platform.pump());
    const auto input=platform.consume_input();
    for(auto key:input.keys) CHECK(key==0); // hidden/unfocused window
    CHECK(platform.consume_input().mouse_x==0);
    SDL_Event quit{};quit.type=SDL_EVENT_QUIT;
    CHECK(SDL_PushEvent(&quit));
    CHECK(!platform.pump());
    platform.close();platform.close();
    CHECK(platform.open("reopen",640,480,true));
    return 0;
}
