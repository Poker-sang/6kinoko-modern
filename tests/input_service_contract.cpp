#include "kinoko/input_service.h"
#include "kinoko/platform.hpp"
#include <cstdio>
#include <algorithm>
extern "C" { unsigned char kinoko_keyboard_state[256]{}; }
static_assert(sizeof(KinokoControllerState)==80 && sizeof(KinokoMouseState)==20);
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"failed: %s\n",#x); return 1; } } while(0)
int main() {
    CHECK(!kinoko_input_initialize());
    CHECK(!kinoko_input_open_keyboard() && !kinoko_input_open_mouse() && !kinoko_input_open_controllers());
    auto& platform=kinoko::platform::host();
    CHECK(platform.open("input service contract",640,480,true));
    CHECK(kinoko_input_initialize());
    CHECK(kinoko_input_open_keyboard() && kinoko_input_open_mouse() && kinoko_input_open_controllers());
    CHECK(kinoko_input_snapshot.controllers);
    CHECK(kinoko_input_snapshot.controller_count>=0 && kinoko_input_snapshot.controller_count<=16);
    CHECK(!kinoko_input_controller_state(-1));
    CHECK(!kinoko_input_controller_state(kinoko_input_snapshot.controller_count));
    CHECK(kinoko_input_poll());
    std::fill_n(kinoko_keyboard_state,256,0x80);
    CHECK(kinoko_input_shutdown());
    for(auto value:kinoko_keyboard_state) CHECK(value==0);
    CHECK(!kinoko_input_snapshot.controllers && !kinoko_input_snapshot.controller_count);
    CHECK(!kinoko_input_open_keyboard() && !kinoko_input_open_mouse());
    CHECK(kinoko_input_initialize()); CHECK(kinoko_input_open_controllers());
    CHECK(kinoko_input_shutdown());
    platform.close();
    return 0;
}
