#pragma once
#include <stdint.h>
#include "kinoko/font_runtime.hpp"
struct KinokoStringLayout;
#ifdef __cplusplus
extern "C" {
#endif
/* Internal CStringLayout atlas path. Input is one CharNextA character (or
   empty for atlas initialization), not the generic rich-text renderer. */
void kinoko_string_font_configure(kinoko::text::FontRenderer* renderer, KinokoStringLayout* layout);
void kinoko_string_font_rasterize(kinoko::text::FontRenderer* renderer, const char *character,
                                 int32_t *width, int32_t *height);
int32_t kinoko_string_font_texture(kinoko::text::FontRenderer* renderer);
void kinoko_string_font_upload(kinoko::text::FontRenderer* renderer, int32_t handle,
                              const char *character, int32_t x, int32_t y,
                              int32_t *width, int32_t *height);
#ifdef __cplusplus
}
#endif
