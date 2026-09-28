#include "kinoko/platform.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <thread>
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
    std::thread title_worker([&] { platform.request_title(u8"日本語 title"); });
    title_worker.join();
    CHECK(std::strcmp(SDL_GetWindowTitle(platform.window()),"contract")==0);
    CHECK(platform.pump());
    CHECK(std::strcmp(SDL_GetWindowTitle(platform.window()),u8"日本語 title")==0);
    SDL_Event key{}; key.type=SDL_EVENT_KEY_DOWN;
    key.key.windowID=SDL_GetWindowID(platform.window());
    key.key.scancode=SDL_SCANCODE_RETURN; key.key.mod=SDL_KMOD_ALT;
    CHECK(SDL_PushEvent(&key)); CHECK(platform.pump());
    CHECK(platform.take_fullscreen_request()); CHECK(!platform.take_fullscreen_request());
    key.key.repeat=true;
    CHECK(SDL_PushEvent(&key)); CHECK(platform.pump()); CHECK(!platform.take_fullscreen_request());
    key.key.repeat=false; key.key.mod=SDL_KMOD_NONE;
    CHECK(SDL_PushEvent(&key)); CHECK(platform.pump()); CHECK(!platform.take_fullscreen_request());
    key.key.mod=SDL_KMOD_ALT; key.key.windowID+=1;
    CHECK(SDL_PushEvent(&key)); CHECK(platform.pump()); CHECK(!platform.take_fullscreen_request());
    SDL_Event unrelated_close{}; unrelated_close.type=SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    unrelated_close.window.windowID=key.key.windowID;
    CHECK(SDL_PushEvent(&unrelated_close)); CHECK(platform.pump());
    SDL_Event quit{};quit.type=SDL_EVENT_QUIT;
    CHECK(SDL_PushEvent(&quit));
    CHECK(!platform.pump());
    platform.close();platform.close();
    CHECK(platform.open("reopen",640,480,true));
    CHECK(!platform.take_fullscreen_request());
    CHECK(std::strcmp(SDL_GetWindowTitle(platform.window()),"reopen")==0);
    return 0;
}
