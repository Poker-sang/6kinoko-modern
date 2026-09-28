#pragma once
#include <windows.h> // Native window/IME boundary, not thread ownership.
#include "kinoko/runtime_sync.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace kinoko::application {
struct Scene;
struct Manager;
struct Transition;
struct SceneMethods {
    void* (__fastcall *destroy)(Scene*, void*, unsigned);
    int32_t (__fastcall *update)(Scene*, void*);
    int32_t (__fastcall *draw)(Scene*, void*);
    void *reserved;
    int32_t (__fastcall *enter)(Scene*, void*, int32_t);
    int32_t (__fastcall *leave)(Scene*, void*, int32_t);
};
struct Scene { const SceneMethods *methods; };
struct ManagerMethods {
    int32_t (__fastcall *initialize)(Manager*, void*);
    int32_t (__fastcall *shutdown)(Manager*, void*);
    int32_t (__fastcall *update)(Manager*, void*);
    int32_t (__fastcall *draw)(Manager*, void*);
    Scene* (__fastcall *create_scene)(Manager*, void*, int32_t);
};
struct Manager { const ManagerMethods *methods; };
struct TransitionMethods {
    int32_t (__fastcall *initialize)(Transition*, void*);
    int32_t (__fastcall *shutdown)(Transition*, void*);
    int32_t (__fastcall *update)(Transition*, void*);
    uint8_t (__fastcall *ready)(Transition*, void*);
    int32_t (__fastcall *draw)(Transition*, void*);
};
struct Transition { const TransitionMethods *methods; };
// Original 40D790 copies exactly 40 bytes to application+8.
struct Configuration {
    HWND window = nullptr;
    HINSTANCE instance = nullptr;
    int32_t width = 0, height = 0, audio_options = 0;
    Manager *manager = nullptr;
    int32_t initial_scene = 0;
    Transition *transition = nullptr;
    uint8_t graphics = 1, input = 1, audio = 1, ime = 0;
    uint8_t show_cursor = 1, separate_draw = 0, show_fps = 0, archives = 1;
};
#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(Configuration) == 40);
static_assert(offsetof(Configuration, manager) == 20);
static_assert(offsetof(Configuration, graphics) == 32);
static_assert(offsetof(Configuration, archives) == 39);
#endif
static_assert(offsetof(Configuration, archives) == offsetof(Configuration, graphics) + 7);
static_assert(offsetof(SceneMethods, enter) == 4*sizeof(void*));
static_assert(offsetof(ManagerMethods, create_scene) == 4*sizeof(void*));

// Native owner, no longer overlaid on the split RetDec globals or byte block.
struct State {
    Configuration config;
    kinoko::runtime::Thread game_thread, display_thread, retire_thread, load_thread;
    kinoko::runtime::EventOwner display_event, retire_event;
    std::recursive_mutex scene_lock;
    std::atomic<bool> running{false};
    Scene *scene = nullptr, *pending_scene = nullptr;
    int32_t requested_scene = 0, current_scene = -1;
    uint32_t frame_count = 0, draw_count = 0, present_count = 0;
    uint32_t statistics_time = 0;
    bool com_initialized = false, timer_period = false, input_initialized = false;
    bool graphics_initialized = false, renderer_initialized = false, ime_initialized = false;
    bool constructed = false;
    State() = default;
    bool is_running() { return running.load(); }
    State(const State&) = delete;
    State& operator=(const State&) = delete;
};
extern State state;
void activate_pending_scene();
bool initialize_input_audio(const Configuration& configuration);
void update_frame();
bool draw_frame();
}
