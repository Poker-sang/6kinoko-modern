// Transitional Win32 ABI adapter; SDL itself and the frame cache are portable.
#include "kinoko/direct_input.h"
#include "kinoko/platform.hpp"
#include <algorithm>
#include <array>
extern "C" { extern unsigned char kinoko_keyboard_state[256]; KinokoInputSnapshot kinoko_input_snapshot{}; }
namespace { HWND window{}; std::array<KinokoControllerState,16> controllers{}; }
extern "C" HWND kinoko_input_window() { return window; }
extern "C" int32_t kinoko_input_initialize(HWND w,HINSTANCE) {
    window=w; return kinoko::platform::host().window()!=nullptr;
}
extern "C" int32_t kinoko_input_shutdown() {
    window=nullptr; controllers={}; kinoko_input_snapshot={};
    std::fill_n(kinoko_keyboard_state,256,0); return 1;
}
extern "C" int32_t kinoko_input_open_keyboard() { return window!=nullptr; }
extern "C" int32_t kinoko_input_open_controllers() { return window!=nullptr; }
extern "C" int32_t kinoko_input_open_mouse() { return window!=nullptr; }
extern "C" int32_t kinoko_input_poll() {
    const auto input=kinoko::platform::host().consume_input();
    std::copy(input.keys.begin(),input.keys.end(),kinoko_keyboard_state);
    for(int i=0;i<input.controller_count;++i) {
        auto& dst=controllers[i]; const auto& src=input.controllers[i];
        std::copy(src.axes.begin(),src.axes.end(),dst.axes);
        std::copy(src.sliders.begin(),src.sliders.end(),dst.sliders);
        std::copy(src.pov.begin(),src.pov.end(),dst.pov);
        std::copy(src.buttons.begin(),src.buttons.end(),dst.buttons);
    }
    kinoko_input_snapshot.controllers=controllers.data();
    kinoko_input_snapshot.controller_count=input.controller_count;
    auto& mouse=kinoko_input_snapshot.mouse;
    mouse.x=input.mouse_x;mouse.y=input.mouse_y;mouse.wheel=input.mouse_wheel;
    std::copy(input.mouse_buttons.begin(),input.mouse_buttons.end(),mouse.buttons);
    return 1;
}
extern "C" int32_t kinoko_input_key_down(int32_t scan) { return kinoko_keyboard_state[uint8_t(scan)]>>7; }
extern "C" const KinokoControllerState* kinoko_input_controller_state(int32_t i) {
    return i>=0 && i<kinoko_input_snapshot.controller_count ? &controllers[i] : nullptr;
}
