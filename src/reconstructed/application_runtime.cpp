#include "kinoko/runtime_paths.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "kinoko/base_utilities.h"
#include "kinoko/game_runtime.h"
#include "kinoko/input_service.h"
#include "kinoko/timer_events.h"
#include "kinoko/application.h"
#include "kinoko/application_runtime.hpp"
#include "kinoko/audio_runtime.h"
#include "kinoko/graphics_device.h"
#include "kinoko/graphics_lock.hpp"
#include "kinoko/renderer.h"
#include "kinoko/render_target.h"
#include "kinoko/scene_queue.h"
#include "kinoko/archive_random.h"
#include "kinoko/game_math.h"
#include "../platform/resources/resource.h"
#include "kinoko/runtime_clock.h"
#include "kinoko/replay_runtime.hpp"
#include "kinoko/tas_bridge.hpp"
#include "kinoko/platform.hpp"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
void kinoko_trace(const char*);
}

namespace kinoko::application {
State state;
namespace {
using CriticalLock=kinoko::runtime::Lock;
using namespace kinoko::runtime;
void join(Thread& thread) {
    if (!thread) return;
    thread.reset();
}
unsigned long __stdcall load_scene_worker(void*) {
    if (state.config.manager) {
        auto* next = state.config.manager->methods->create_scene(
            state.config.manager, nullptr, state.requested_scene);
        CriticalLock lock(&state.scene_lock);
        state.pending_scene = next;
    }
    return 0;
}
void update_statistics() {
    const uint32_t now = kinoko_clock_milliseconds();
    if (!state.config.show_fps || now - state.statistics_time < 1000) return;
    state.statistics_time += 1000;
    char title[256];
    std::snprintf(title, sizeof(title), "%s    Game:%uFPS    Draw:%u+%uFPS",
        u8"魔理沙と６つのキノコ", state.frame_count, state.draw_count, state.present_count);
    kinoko::platform::host().request_title(title);
    state.frame_count = state.draw_count = state.present_count = 0;
}
unsigned long __stdcall game_loop(void*) {
    // This is a timer-registry borrow: unregister it, never close it separately.
    auto* const frame_event = kinoko_frame_timer_register();
    kinoko_trace("game:entry");
    while (state.is_running()) {
        update_statistics();
        update_frame();
        if (!state.config.separate_draw) draw_frame();
        kinoko_math_checkpoint("render-done", 0);
        if (frame_event) kinoko_frame_timer_wait(frame_event);
        else kinoko_clock_delay(16); // Retained reconstruction fallback for event allocation failure.
    }
    if (frame_event) kinoko_frame_timer_unregister(frame_event);
    kinoko_trace("game:exit");
    return 0;
}
unsigned long __stdcall game_worker(void*) {
    kinoko_run_game_math(game_loop, nullptr);
    return 0;
}
unsigned long __stdcall display_worker(void*) {
    while (state.is_running()) {
        if (state.config.separate_draw) {
            if (state.scene_lock.try_lock()) {
                if (state.scene && draw_frame() && kinoko_graphics_present()) ++state.draw_count;
                state.scene_lock.unlock();
            }
            wait(state.display_event.get(), 1);
        } else {
            if (wait(state.display_event.get()) != 0 ||
                !state.is_running()) break;
            // 40E090..40E0E2: retry nonblocking presentation at most eight times.
            for (unsigned attempt = 0; attempt < 8 && state.is_running(); ++attempt) {
                if (kinoko_graphics_present()) { ++state.present_count; break; }
                kinoko_clock_delay(1);
            }
        }
    }
    return 0;
}
unsigned long __stdcall retire_worker(void*) {
    while (state.is_running()) {
        if (wait(state.retire_event.get()) != 0) break;
        kinoko_destroy_retired_scenes();
    }
    return 0;
}
bool initialize(const Configuration& configuration) {
    state.config = configuration;
    kinoko_application_set_archive_mode(configuration.archives != 0);
    state.timer_period = kinoko_clock_request_resolution()!=0;
    kinoko_seed_random(kinoko_clock_milliseconds());
    if (!configuration.show_cursor) SDL_HideCursor();
    state.running.store(true);
    if (configuration.graphics) {
        if (!kinoko_graphics_create(configuration.width, configuration.height)) return false;
        state.graphics_initialized = true;
        kinoko_renderer_initialize();
        state.renderer_initialized = true;
    }
    if (!initialize_input_audio(configuration)) return false;
    state.statistics_time = kinoko_clock_milliseconds();
    if (configuration.manager) configuration.manager->methods->initialize(configuration.manager, nullptr);
    if (configuration.transition) configuration.transition->methods->initialize(configuration.transition, nullptr);
    state.requested_scene = configuration.initial_scene;
    state.current_scene = -1;
    state.pending_scene = configuration.manager
        ? configuration.manager->methods->create_scene(configuration.manager, nullptr, configuration.initial_scene) : nullptr;
    // Publish wake events before starting workers: shutdown must never signal a
    // null slot while its worker is just about to create and wait on that event.
    state.retire_event.reset(make_event());
    if (configuration.graphics) state.display_event.reset(make_event());
    if (!state.retire_event || (configuration.graphics && !state.display_event)) return false;
    state.retire_thread.start(retire_worker,nullptr,Priority::low);
    state.game_thread.start(game_worker);
    if (configuration.graphics) {
        state.display_thread.start(display_worker,nullptr,Priority::low);
    }
    return state.retire_thread && state.game_thread && (!configuration.graphics || state.display_thread);
}
void message_loop() {
    while (state.is_running() && kinoko::platform::host().pump()) {
        if (kinoko::platform::host().take_fullscreen_request()) {
            const bool was_windowed=kinoko_graphics.present.Windowed!=0;
            { CriticalLock lock(&state.scene_lock); kinoko_graphics_toggle_window(); }
            if (state.config.show_cursor && was_windowed != (kinoko_graphics.present.Windowed!=0)) {
                if(kinoko_graphics.present.Windowed) SDL_ShowCursor(); else SDL_HideCursor();
            }
        }
        kinoko_graphics_poll();
        SDL_Delay(1);
    }
}
#ifdef _WIN32
WNDPROC sdl_window_proc{};
constexpr UINT_PTR move_frame_timer_id=0x6b696e6f;
UINT_PTR move_frame_timer{};
uint64_t last_move_frame{};
bool moving_window{};
bool polling_move_frame{};

void stop_move_frames(HWND window) {
    moving_window=false;
    if (move_frame_timer) KillTimer(window,move_frame_timer);
    move_frame_timer=0;
}
void poll_move_frame() {
    if (!moving_window || polling_move_frame || !state.graphics_initialized || !state.is_running()) return;
    const auto now=SDL_GetTicks();
    if (now-last_move_frame<16) return;
    last_move_frame=now;
    polling_move_frame=true;
    kinoko_graphics_poll();
    polling_move_frame=false;
}
LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM key, LPARAM parameter) {
    // Narrow native boundary: keep GPU pumping during Windows move/size.
    // All ordinary input/window events are forwarded to SDL.
    if (message == WM_ENTERSIZEMOVE) {
        moving_window=true;
        last_move_frame=0;
        if (!move_frame_timer) move_frame_timer=SetTimer(window,move_frame_timer_id,16,nullptr);
    } else if (message == WM_EXITSIZEMOVE) {
    #ifdef _WIN32
    stop_move_frames(window);
#endif
    } else if (message == WM_TIMER && moving_window && move_frame_timer && key == move_frame_timer) {
        poll_move_frame();
        return 0;
    }
    const auto result=CallWindowProcW(sdl_window_proc,window,message,key,parameter);
    if (moving_window && (message == WM_MOVING || message == WM_SIZING)) poll_move_frame();
    return result;
}
#endif
}

// 40D836..40D872: input service, keyboard and enumeration are required when
// enabled. Mouse creation is attempted, but its return does not gate startup.
bool initialize_input_audio(const Configuration& configuration) {
    if (configuration.input) {
        state.input_initialized = static_cast<uint8_t>(kinoko_input_initialize()) != 0;
        if (!state.input_initialized) return false;
        if (!static_cast<uint8_t>(kinoko_input_open_keyboard())) return false;
        if (!static_cast<uint8_t>(kinoko_input_open_controllers())) return false;
        kinoko_input_open_mouse();
    }
    return !configuration.audio || static_cast<uint8_t>(kinoko_audio_initialize_device()) != 0;
}

void update_frame() {
    if (state.config.input) kinoko_input_poll();
    kinoko_math_checkpoint("input-done", 0);
    auto* manager = state.config.manager;
    auto* transition = state.config.transition;
    if (manager) manager->methods->update(manager, nullptr);
    if (state.current_scene != state.requested_scene) {
        if (!transition || transition->methods->ready(transition, nullptr)) activate_pending_scene();
    } else {
        if (transition) transition->methods->update(transition, nullptr);
        if (state.scene) {
            state.requested_scene = state.scene->methods->update(state.scene, nullptr);
            if (state.requested_scene == -1) state.running.store(false);
            else ++state.frame_count;
        }
        if (state.requested_scene != -1 && state.requested_scene != state.current_scene) {
            join(state.load_thread);
            state.load_thread.start(load_scene_worker);
        }
    }
    kinoko_math_checkpoint("scene-done", 0);
}
bool draw_frame() {
    { kinoko::graphics::Lock lock; kinoko_renderer.present_pending = 0; }
    int32_t ready = 1;
    if (state.scene) ready = state.scene->methods->draw(state.scene, nullptr) & 1;
    if (state.config.manager) ready &= state.config.manager->methods->draw(state.config.manager, nullptr);
    if (state.config.transition) ready &= state.config.transition->methods->draw(state.config.transition, nullptr);
    if (ready) {
        { kinoko::graphics::Lock lock; kinoko_renderer.present_pending = 1; }
        if (!state.config.separate_draw && state.display_event) kinoko::runtime::signal(state.display_event.get());
    }
    return ready != 0;
}

}

extern "C" void kinoko_application_construct() {
    if (kinoko::application::state.constructed) return;
    kinoko_initialize_scene_queue();
    kinoko::application::state.constructed = true;
}
extern "C" int32_t kinoko_application_frame_count() {
    return static_cast<int32_t>(kinoko::application::state.frame_count);
}
extern "C" void kinoko_application_shutdown() {
    using namespace kinoko::application;
    state.running.store(false);
    kinoko::graphics::stop();
    join(state.game_thread);
    join(state.load_thread);
    if (state.retire_event) kinoko::runtime::signal(state.retire_event.get());
    join(state.retire_thread);
    if (state.display_event) kinoko::runtime::signal(state.display_event.get());
    join(state.display_thread);
    kinoko_destroy_retired_scenes();
    state.retire_event.reset(); state.display_event.reset();
    {
        kinoko::runtime::Lock lock(&state.scene_lock);
        if (state.scene) state.scene->methods->destroy(state.scene, nullptr, 1);
        state.scene = nullptr;
        if (state.pending_scene) state.pending_scene->methods->destroy(state.pending_scene, nullptr, 1);
        state.pending_scene = nullptr;
    }
    if (auto* manager = state.config.manager) {
        manager->methods->shutdown(manager, nullptr); std::free(manager); state.config.manager = nullptr;
    }
    if (auto* transition = state.config.transition) {
        transition->methods->shutdown(transition, nullptr); std::free(transition); state.config.transition = nullptr;
    }
    kinoko_audio_shutdown_device();
    if (state.input_initialized) { kinoko_input_shutdown(); state.input_initialized = false; }
    if (state.renderer_initialized) {
        kinoko_renderer_before_reset(&kinoko_renderer);
        kinoko_remove_device_listener(&kinoko_renderer.listener);
        state.renderer_initialized = false;
    }
    if (state.graphics_initialized) { kinoko_graphics_release(); state.graphics_initialized = false; }
    if (state.timer_period) { kinoko_clock_release_resolution(); state.timer_period = false; }
}
extern "C" int kinoko_application_run(int show_command) {
    using namespace kinoko::application;
    kinoko_application_initialize_host();
    if (!kinoko_use_executable_directory()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Startup error",
            "Cannot access the executable directory.", nullptr);
        return 1;
    }
    for(const char* archive : {"6kinoko_a.dat","6kinoko_b.dat","6kinoko_c.dat"}) {
        SDL_PathInfo info{};
        if(!SDL_GetPathInfo(archive,&info) || info.type!=SDL_PATHTYPE_FILE || info.size==0) {
            char message[256];std::snprintf(message,sizeof(message),"Place the original %s beside the game executable.",archive);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Missing game data",message,nullptr);return 1;
        }
    }
    // SDL expects UTF-8; the inherited title accessor contains CP932 bytes.
    auto& platform = kinoko::platform::host();
    if (!platform.open(u8"魔理沙と６つのキノコ",640,480)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"SDL initialization failed",SDL_GetError(),nullptr); return 1;
    }
#ifdef _WIN32
    HWND window=static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(platform.window()),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr));
    if (!window) { platform.close(); return 1; }
    sdl_window_proc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window,GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(window_proc)));
    if (!sdl_window_proc) { platform.close(); return 1; }
    // The entrypoint still receives Windows launch hints; window operations
    // themselves are SDL calls on the main thread.
    if(show_command==SW_HIDE || kinoko::tas::enabled()) SDL_HideWindow(platform.window());
    else {
        SDL_ShowWindow(platform.window());
        if(show_command==SW_SHOWMINIMIZED || show_command==SW_MINIMIZE || show_command==SW_SHOWMINNOACTIVE)
            SDL_MinimizeWindow(platform.window());
        else if(show_command==SW_SHOWMAXIMIZED) SDL_MaximizeWindow(platform.window());
    }
#else
    (void)show_command;
    if(!kinoko::tas::enabled())SDL_ShowWindow(platform.window());
#endif
    if(!kinoko_replay_start()) {kinoko_replay_finish();platform.close();return 1;}
    Configuration config;
    config.manager = kinoko::game::create_manager();
    kinoko_application_open_archives();
    if (!config.manager || !initialize(config)) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error",u8"初期化失敗",platform.window());
    else message_loop();
#ifdef _WIN32
    stop_move_frames(window);
#endif
    kinoko::tas::shutdown();
    kinoko_application_shutdown();
    kinoko_replay_finish();
#ifdef _WIN32
    SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(sdl_window_proc));
#endif
    platform.close();
    return 0;
}
