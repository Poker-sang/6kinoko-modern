// Portable game input service over the SDL frame cache.
#include "kinoko/input_service.h"
#include "kinoko/platform.hpp"
#include <algorithm>
#include <array>
extern "C" { extern unsigned char kinoko_keyboard_state[256]; KinokoInputSnapshot kinoko_input_snapshot{}; }
namespace { bool initialized{}; std::array<KinokoControllerState,16> controllers{}; }
extern "C" int32_t kinoko_input_initialize() {
    initialized=kinoko::platform::host().window()!=nullptr;
    return initialized;
}
extern "C" int32_t kinoko_input_shutdown() {
    initialized=false; controllers={}; kinoko_input_snapshot={};
    std::fill_n(kinoko_keyboard_state,256,0); return 1;
}
extern "C" int32_t kinoko_input_open_keyboard() { return initialized; }
extern "C" int32_t kinoko_input_open_controllers() {
    // Manager/script initialization enumerates controllers before the first
    // update frame. Publish the main-thread startup snapshot now, like 408C50.
    return initialized ? kinoko_input_poll() : 0;
}
extern "C" int32_t kinoko_input_open_mouse() { return initialized; }
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
