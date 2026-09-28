#include "kinoko/critical_section.h"
#include "kinoko/renderer.h"
#include "kinoko/graphics_device.h"
#include "kinoko/graphics_lock.hpp"
#include "kinoko/render_target.h"
#include "kinoko/diagnostics.h"
#include <atomic>

// 4013D0. A failed Reset skips after-reset notifications and retains the
// failure state; the message-loop cooperative-level poll decides the next try.
extern "C" int32_t kinoko_graphics_reset(void) {
    auto &state=kinoko_graphics;
    if (!state.factory || !state.device || state.cooperative_status==kinoko::graphics::error_devicelost) return 0;
    state.present.BackBufferFormat=state.present.Windowed ? state.display.Format : kinoko::graphics::format_x8r8g8b8;
    kinoko::graphics::Lock lock;
    kinoko_notify_device_listeners(KINOKO_DEVICE_BEFORE_RESET);
    if (state.swap_chain) { state.swap_chain->Release();state.swap_chain=nullptr; }
    if (FAILED(state.device->Reset(&state.present))) return 0;
    state.device->GetSwapChain(0,&state.swap_chain);
    kinoko_notify_device_listeners(KINOKO_DEVICE_AFTER_RESET);
    return 1;
}

extern "C" HRESULT kinoko_graphics_poll(void) {
    auto &state=kinoko_graphics;
    if (state.device) state.cooperative_status=state.device->TestCooperativeLevel();
    if (state.cooperative_status==kinoko::graphics::error_devicenotreset) kinoko_graphics_reset();
    return state.cooperative_status; // do not force kinoko::graphics::ok after Reset
}

extern "C" int32_t kinoko_graphics_toggle_window(void) {
    auto &parameters=kinoko_graphics.present;
    parameters.Windowed=!parameters.Windowed;
    const auto result=kinoko_graphics_reset();
    if (!result) { parameters.Windowed=!parameters.Windowed;return result; }
    // SDL owns desktop fullscreen and restoration; no native border metrics.
    return result;
}

extern "C" int32_t kinoko_graphics_begin_scene(void) {
    kinoko_graphics_lock.native->lock();
    auto *device=kinoko_graphics.device;
    if (!device) { kinoko_graphics_lock.native->unlock();return 0; }
    const auto status=device->BeginScene();
    static std::atomic<int32_t> traces{0};
    if (++traces<=5) kinoko_trace_hresult("401760:beginscene-hr",status);
    // 40177D compares exactly against zero, not merely SUCCEEDED(status).
    if (status!=kinoko::graphics::ok) { kinoko_graphics_lock.native->unlock();return 0; }
    return 1;
}
extern "C" int32_t kinoko_graphics_end_scene(void) {
    if (auto *device=kinoko_graphics.device)
        kinoko_trace_hresult("401790:endscene-hr",device->EndScene());
    kinoko_graphics_lock.native->unlock();
    return 0;
}
extern "C" int32_t kinoko_graphics_present(void) {
    if (!kinoko_renderer.present_pending || !kinoko_graphics_lock.native->try_lock()) return 0;
    auto *swap_chain=kinoko_graphics.swap_chain;
    // Retain the existing null-swap-chain startup boundary. The normal path
    // clears pending only on kinoko::graphics::ok, retaining it on WASSTILLDRAWING/failure.
    HRESULT status=kinoko::graphics::ok;
    if (swap_chain) status=swap_chain->Present(nullptr,nullptr,nullptr,nullptr,kinoko::graphics::presentation_donotwait);
    static std::atomic<int32_t> traces{0};
    if (++traces<=5) kinoko_trace_hresult("4017b0:present-hr",status);
    if (status==kinoko::graphics::ok) kinoko_renderer.present_pending=0;
    kinoko_graphics_lock.native->unlock();
    return status==kinoko::graphics::ok;
}
extern "C" int32_t kinoko_graphics_clear(void) {
    auto *device=kinoko_graphics.device;
    if (!device) return kinoko::graphics::error_invalidcall;
    const auto status=device->Clear(0,nullptr,kinoko::graphics::clear_target|kinoko::graphics::clear_zbuffer,kinoko_renderer.clear_color,1.0f,0);
    static std::atomic<int32_t> traces{0};
    if (++traces<=5) kinoko_trace_hresult("401820:clear-hr",status);
    return status;
}
