#pragma once
#include "kinoko/graphics_device.h"
#include "kinoko/graphics_runtime.hpp"
#ifdef __cplusplus
extern "C" {
#endif
/* GetRenderTarget/GetDepthStencilSurface acquire references released before
   Reset/shutdown. Original leaves released slots intact until reacquisition. */
extern KinokoRenderer kinoko_renderer;
KinokoRenderer *kinoko_renderer_construct(void);
int32_t kinoko_renderer_initialize(void);
int32_t kinoko_render_set_filter(int32_t mode);
int32_t kinoko_render_set_cull(int32_t mode);
#ifdef __cplusplus
}
#endif
