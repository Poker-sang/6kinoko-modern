#pragma once
#include <stdint.h>
#include <windows.h>
#include "kinoko/graphics_api.hpp"
#ifdef __cplusplus
extern "C" {
#endif
/* Factory, device and swap_chain each own one backend reference. Renderer and
   draw consumers borrow device. This runtime object is not a serialized layout. */
typedef struct KinokoGraphics {
    kinoko::graphics::Factory *factory;
    kinoko::graphics::Device *device;
    kinoko::graphics::SwapChain *swap_chain;
    kinoko::graphics::Capabilities capabilities;
    kinoko::graphics::Presentation present;
    kinoko::graphics::DisplayMode display;
    LONG original_window_style;
    HRESULT cooperative_status;
    int32_t unknown_state;
} KinokoGraphics;
extern KinokoGraphics kinoko_graphics;
void kinoko_graphics_initialize_runtime(void);
int32_t kinoko_graphics_create(HWND window,int32_t width,int32_t height);
int32_t kinoko_graphics_release(void);
int32_t kinoko_graphics_reset(void);
int32_t kinoko_graphics_toggle_window(void);
HRESULT kinoko_graphics_poll(void);
int32_t kinoko_graphics_begin_scene(void);
int32_t kinoko_graphics_end_scene(void);
int32_t kinoko_graphics_present(void);
int32_t kinoko_graphics_clear(void);
#ifdef __cplusplus
}
#endif
