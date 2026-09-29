#include "kinoko/platform.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <limits>
#include <mutex>
#include <vector>
#include <string>
#include <utility>
namespace kinoko::platform {
int legacy_scan(int scan) noexcept {
    switch (scan) {
    case SDL_SCANCODE_ESCAPE: return 1;
    case SDL_SCANCODE_MINUS: return 12;
    case SDL_SCANCODE_EQUALS: return 13;
    case SDL_SCANCODE_BACKSPACE: return 14;
    case SDL_SCANCODE_TAB: return 15;
    case SDL_SCANCODE_LEFTBRACKET: return 26;
    case SDL_SCANCODE_RIGHTBRACKET: return 27;
    case SDL_SCANCODE_RETURN: return 28;
    case SDL_SCANCODE_LCTRL: return 29;
    case SDL_SCANCODE_SEMICOLON: return 39;
    case SDL_SCANCODE_APOSTROPHE: return 40;
    case SDL_SCANCODE_GRAVE: return 41;
    case SDL_SCANCODE_LSHIFT: return 42;
    case SDL_SCANCODE_BACKSLASH: return 43;
    case SDL_SCANCODE_COMMA: return 51;
    case SDL_SCANCODE_PERIOD: return 52;
    case SDL_SCANCODE_SLASH: return 53;
    case SDL_SCANCODE_RSHIFT: return 54;
    case SDL_SCANCODE_KP_MULTIPLY: return 55;
    case SDL_SCANCODE_LALT: return 56;
    case SDL_SCANCODE_SPACE: return 57;
    case SDL_SCANCODE_CAPSLOCK: return 58;
    case SDL_SCANCODE_NUMLOCKCLEAR: return 69;
    case SDL_SCANCODE_SCROLLLOCK: return 70;
    case SDL_SCANCODE_KP_7: return 71;
    case SDL_SCANCODE_KP_8: return 72;
    case SDL_SCANCODE_KP_9: return 73;
    case SDL_SCANCODE_KP_MINUS: return 74;
    case SDL_SCANCODE_KP_4: return 75;
    case SDL_SCANCODE_KP_5: return 76;
    case SDL_SCANCODE_KP_6: return 77;
    case SDL_SCANCODE_KP_PLUS: return 78;
    case SDL_SCANCODE_KP_1: return 79;
    case SDL_SCANCODE_KP_2: return 80;
    case SDL_SCANCODE_KP_3: return 81;
    case SDL_SCANCODE_KP_0: return 82;
    case SDL_SCANCODE_KP_PERIOD: return 83;
    case SDL_SCANCODE_NONUSBACKSLASH: return 86;
    case SDL_SCANCODE_F11: return 87;
    case SDL_SCANCODE_F12: return 88;
    case SDL_SCANCODE_F13: return 100;
    case SDL_SCANCODE_F14: return 101;
    case SDL_SCANCODE_F15: return 102;
    case SDL_SCANCODE_INTERNATIONAL1: return 115;
    case SDL_SCANCODE_INTERNATIONAL3: return 125;
    case SDL_SCANCODE_LANG3: return 112;
    case SDL_SCANCODE_INTERNATIONAL2: return 112;
    case SDL_SCANCODE_INTERNATIONAL4: return 121;
    case SDL_SCANCODE_INTERNATIONAL5: return 123;
    case SDL_SCANCODE_KP_EQUALS: return 141;
    case SDL_SCANCODE_KP_ENTER: return 156;
    case SDL_SCANCODE_RCTRL: return 157;
    case SDL_SCANCODE_KP_DIVIDE: return 181;
    case SDL_SCANCODE_PRINTSCREEN: return 183;
    case SDL_SCANCODE_RALT: return 184;
    case SDL_SCANCODE_PAUSE: return 197;
    case SDL_SCANCODE_HOME: return 199;
    case SDL_SCANCODE_UP: return 200;
    case SDL_SCANCODE_PAGEUP: return 201;
    case SDL_SCANCODE_LEFT: return 203;
    case SDL_SCANCODE_RIGHT: return 205;
    case SDL_SCANCODE_END: return 207;
    case SDL_SCANCODE_DOWN: return 208;
    case SDL_SCANCODE_PAGEDOWN: return 209;
    case SDL_SCANCODE_INSERT: return 210;
    case SDL_SCANCODE_DELETE: return 211;
    case SDL_SCANCODE_LGUI: return 219;
    case SDL_SCANCODE_RGUI: return 220;
    case SDL_SCANCODE_APPLICATION: return 221;
    case SDL_SCANCODE_POWER: return 222;
    case SDL_SCANCODE_1: return 2;
    case SDL_SCANCODE_2: return 3;
    case SDL_SCANCODE_3: return 4;
    case SDL_SCANCODE_4: return 5;
    case SDL_SCANCODE_5: return 6;
    case SDL_SCANCODE_6: return 7;
    case SDL_SCANCODE_7: return 8;
    case SDL_SCANCODE_8: return 9;
    case SDL_SCANCODE_9: return 10;
    case SDL_SCANCODE_0: return 11;
    case SDL_SCANCODE_Q: return 16;
    case SDL_SCANCODE_W: return 17;
    case SDL_SCANCODE_E: return 18;
    case SDL_SCANCODE_R: return 19;
    case SDL_SCANCODE_T: return 20;
    case SDL_SCANCODE_Y: return 21;
    case SDL_SCANCODE_U: return 22;
    case SDL_SCANCODE_I: return 23;
    case SDL_SCANCODE_O: return 24;
    case SDL_SCANCODE_P: return 25;
    case SDL_SCANCODE_A: return 30;
    case SDL_SCANCODE_S: return 31;
    case SDL_SCANCODE_D: return 32;
    case SDL_SCANCODE_F: return 33;
    case SDL_SCANCODE_G: return 34;
    case SDL_SCANCODE_H: return 35;
    case SDL_SCANCODE_J: return 36;
    case SDL_SCANCODE_K: return 37;
    case SDL_SCANCODE_L: return 38;
    case SDL_SCANCODE_Z: return 44;
    case SDL_SCANCODE_X: return 45;
    case SDL_SCANCODE_C: return 46;
    case SDL_SCANCODE_V: return 47;
    case SDL_SCANCODE_B: return 48;
    case SDL_SCANCODE_N: return 49;
    case SDL_SCANCODE_M: return 50;
    case SDL_SCANCODE_F1: return 59;
    case SDL_SCANCODE_F2: return 60;
    case SDL_SCANCODE_F3: return 61;
    case SDL_SCANCODE_F4: return 62;
    case SDL_SCANCODE_F5: return 63;
    case SDL_SCANCODE_F6: return 64;
    case SDL_SCANCODE_F7: return 65;
    case SDL_SCANCODE_F8: return 66;
    case SDL_SCANCODE_F9: return 67;
    case SDL_SCANCODE_F10: return 68;
    default: return -1;
    }
}
std::int32_t legacy_axis(std::int16_t value) noexcept {
    return value < 0 ? static_cast<std::int32_t>(value) * 1000 / 32768
                     : static_cast<std::int32_t>(value) * 1000 / 32767;
}
struct Platform::State {
    SDL_Window* window{};
    bool initialized{};
    bool fullscreen_request{};
    std::string pending_title;
    bool title_pending{};
    std::mutex mutex;
    Input input;
    struct Joystick { SDL_JoystickID id; SDL_Joystick* handle; };
    std::vector<Joystick> joysticks;
    void add(SDL_JoystickID id) {
        for (auto& j : joysticks) if (j.id == id) return;
        if (joysticks.size() == input.controllers.size()) return;
        if (auto* j = SDL_OpenJoystick(id)) joysticks.push_back({id,j});
    }
};
Platform::Platform() : state_(std::make_unique<State>()) {}
Platform::~Platform() { close(); }
bool Platform::open(const char* title, int width, int height, bool hidden) {
    if (state_->initialized) return false;
    constexpr auto flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK;
    if (!SDL_InitSubSystem(flags)) return false;
    state_->initialized = true;
    state_->window = SDL_CreateWindow(title, width, height, hidden ? SDL_WINDOW_HIDDEN : 0);
    if (!state_->window) { close(); return false; }
    int count = 0;
    auto* ids = SDL_GetJoysticks(&count);
    for (int i = 0; i < count; ++i) state_->add(ids[i]);
    SDL_free(ids);
    pump();
    return true;
}
bool Platform::pump() {
    if (!state_->window) return false;
    bool running = true;
    SDL_Event event;
    float wheel = 0;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT ||
            (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(state_->window))) running = false;
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.windowID == SDL_GetWindowID(state_->window)
            && event.key.scancode == SDL_SCANCODE_RETURN && (event.key.mod & SDL_KMOD_ALT) && !event.key.repeat)
            state_->fullscreen_request = !state_->fullscreen_request;
        if (event.type == SDL_EVENT_JOYSTICK_ADDED) state_->add(event.jdevice.which);
        if (event.type == SDL_EVENT_JOYSTICK_REMOVED) {
            // Preserve assignment indices; a disconnected slot becomes neutral.
            for (auto& j : state_->joysticks) if (j.id == event.jdevice.which && j.handle) {
                SDL_CloseJoystick(j.handle); j.handle = nullptr;
            }
        }
        if (event.type == SDL_EVENT_MOUSE_WHEEL) wheel += event.wheel.y *
            (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f);
    }
    std::string title;
    bool update_title=false;
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        update_title=std::exchange(state_->title_pending,false);
        if(update_title) title=std::move(state_->pending_title);
    }
    if(update_title) SDL_SetWindowTitle(state_->window,title.c_str());
    Input next;
    const bool focused = (SDL_GetWindowFlags(state_->window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    int count = 0;
    const bool* keys = SDL_GetKeyboardState(&count);
    if (focused) for (int i = 0; i < count; ++i) {
        const int scan = legacy_scan(i);
        if (scan >= 0 && keys[i]) next.keys[scan] = 0x80;
    }
    next.controller_count = static_cast<int>(state_->joysticks.size());
    for (int i = 0; i < next.controller_count; ++i) {
        auto* j = state_->joysticks[i].handle;
        if (!focused || !j || !SDL_JoystickConnected(j)) continue;
        auto& output = next.controllers[i];
        const int axes = std::min(SDL_GetNumJoystickAxes(j),8);
        for (int a = 0; a < axes; ++a) {
            const auto value = legacy_axis(SDL_GetJoystickAxis(j,a));
            if (a < 6) output.axes[a] = value; else output.sliders[a-6] = value;
        }
        const int buttons = std::min(SDL_GetNumJoystickButtons(j),32);
        for (int b = 0; b < buttons; ++b) output.buttons[b] = SDL_GetJoystickButton(j,b) ? 0x80 : 0;
        for (int h = 0; h < std::min(SDL_GetNumJoystickHats(j),4); ++h) {
            switch (SDL_GetJoystickHat(j,h)) {
            case SDL_HAT_UP: output.pov[h]=0; break;
            case SDL_HAT_RIGHTUP: output.pov[h]=4500; break;
            case SDL_HAT_RIGHT: output.pov[h]=9000; break;
            case SDL_HAT_RIGHTDOWN: output.pov[h]=13500; break;
            case SDL_HAT_DOWN: output.pov[h]=18000; break;
            case SDL_HAT_LEFTDOWN: output.pov[h]=22500; break;
            case SDL_HAT_LEFT: output.pov[h]=27000; break;
            case SDL_HAT_LEFTUP: output.pov[h]=31500; break;
            default: break;
            }
        }
    }
    float dx=0,dy=0;
    const auto buttons=SDL_GetRelativeMouseState(&dx,&dy);
    if (focused) {
        next.mouse_x=static_cast<std::int32_t>(dx); next.mouse_y=static_cast<std::int32_t>(dy);
        next.mouse_wheel=static_cast<std::int32_t>(wheel*120);
        // DirectInput ordering: left, right, middle, X1, X2.
        const int order[]={SDL_BUTTON_LEFT,SDL_BUTTON_RIGHT,SDL_BUTTON_MIDDLE,SDL_BUTTON_X1,SDL_BUTTON_X2};
        for(int i=0;i<5;++i) next.mouse_buttons[i]=(buttons & SDL_BUTTON_MASK(order[i])) ? 0x80 : 0;
    }
    std::lock_guard<std::mutex> lock(state_->mutex);
    if (focused) {
        auto sum=[](std::int32_t a,std::int32_t b) {
            return static_cast<std::int32_t>(std::clamp<std::int64_t>(std::int64_t(a)+b,INT32_MIN,INT32_MAX));
        };
        next.mouse_x=sum(next.mouse_x,state_->input.mouse_x);
        next.mouse_y=sum(next.mouse_y,state_->input.mouse_y);
        next.mouse_wheel=sum(next.mouse_wheel,state_->input.mouse_wheel);
    }
    state_->input=next;
    return running;
}
bool Platform::take_fullscreen_request() noexcept { return std::exchange(state_->fullscreen_request,false); }
void Platform::request_title(const char* title) {
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->pending_title=title?title:"";
    state_->title_pending=true;
}
Input Platform::consume_input() {
    std::lock_guard<std::mutex> lock(state_->mutex);
    auto result=state_->input;
    state_->input.mouse_x=state_->input.mouse_y=state_->input.mouse_wheel=0;
    return result;
}
SDL_Window* Platform::window() const noexcept { return state_->window; }
void Platform::close() {
    for(auto& j:state_->joysticks) if(j.handle) SDL_CloseJoystick(j.handle);
    state_->joysticks.clear();
    if(state_->window) SDL_DestroyWindow(state_->window);
    state_->window=nullptr;
    if(state_->initialized) SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    state_->initialized=false;
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->input={};
    state_->fullscreen_request=false;
    state_->pending_title.clear(); state_->title_pending=false;
}
Platform& host() { static Platform instance; return instance; }
}
