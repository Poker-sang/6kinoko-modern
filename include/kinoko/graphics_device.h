#pragma once
#include <stdint.h>
#include <windows.h>
#include "kinoko/graphics_c_boundary.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Factory, device and swap_chain each own one backend reference. Renderer and
   draw consumers borrow device. This runtime object is not a serialized layout. */
typedef struct KinokoGraphics {
    KinokoGraphicsFactory *factory;
    KinokoGraphicsDevice *device;
    KinokoGraphicsSwapChain *swap_chain;
    KinokoGraphicsCapabilities capabilities;
    KinokoGraphicsPresentation present;
    KinokoGraphicsDisplayMode display;
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
