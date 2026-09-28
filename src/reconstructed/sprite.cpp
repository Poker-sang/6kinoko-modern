#include "kinoko/runtime_util.hpp"
#include "kinoko/angle_math.h"
#include "kinoko/texture_store.h"
#include "kinoko/graphics_device.h"
#include "kinoko/sprite.h"

#include <cstddef>
#include "graphics_quad_sink.hpp"
using kinoko::render::SpriteLayout;

extern "C" {
}

static_assert(sizeof(KinokoSpriteVertex) == 28, "Original vertex stride");
#if INTPTR_MAX == INT32_MAX
static_assert(offsetof(KinokoSprite, vertices) == 8 &&
              offsetof(KinokoSprite, width) == 120 &&
              offsetof(KinokoSprite, angle) == 144 && sizeof(KinokoSprite) == 148,
              "Original CSprite field offsets");
#endif

extern "C" void kinoko_sprite_transform(KinokoSprite *sprite, float x, float y) {
    auto &vertices = sprite->vertices;
    vertices[0].x = x - sprite->pivot_x * sprite->scale_x - 0.5f;
    vertices[0].y = y - sprite->pivot_y * sprite->scale_y - 0.5f;
    vertices[1].x = sprite->width * sprite->scale_x + vertices[0].x;
    vertices[1].y = vertices[0].y;
    vertices[2].x = vertices[0].x;
    vertices[2].y = vertices[0].y + sprite->height * sprite->scale_y;
    vertices[3].y = vertices[2].y;
    vertices[3].x = vertices[1].x;

    if (sprite->angle != 0.0f) {
        const float cosine = kinoko_cos_degrees(sprite->angle);
        const float sine = kinoko_sin_degrees(sprite->angle);
        for (auto &vertex : vertices) {
            const float old_x = vertex.x;
            const float old_y = vertex.y;
            vertex.x = (old_x - x) * cosine + x - (old_y - y) * sine;
            vertex.y = (old_x - x) * sine + y + (old_y - y) * cosine;
        }
    }
}

namespace {
int32_t submit(KinokoSprite *sprite, SpriteLayout format) {
    auto *device = kinoko_graphics.device;
    if (!device || !*reinterpret_cast<void ***>(device))
        return 0;
    // Original ignores bind/FVF failure and returns DrawPrimitiveUP kinoko::graphics::Result.
    kinoko_texture_bind_stage(0, sprite->texture);
    kinoko::render::GraphicsQuadSink sink(*device);
    return kinoko::render::submit_quad(sink,format,sprite->vertices);
}
int32_t draw(KinokoSprite *sprite, float x, float y, SpriteLayout format) {
    if (!sprite) return 0;
    kinoko_sprite_transform(sprite, x, y);
    return submit(sprite, format);
}
}

// Original 404E10: explicit screen bounds, independent of pivot/scale/angle.
extern "C" int32_t __fastcall kinoko_sprite_draw_bounds(
    KinokoSprite *sprite, void *, float left, float top, float right, float bottom) {
    if (!sprite) return 0;
    auto &v = sprite->vertices;
    v[0].x = v[2].x = left - 0.5f;
    v[0].y = v[1].y = top - 0.5f;
    v[1].x = v[3].x = right - 0.5f;
    v[2].y = v[3].y = bottom - 0.5f;
    return submit(sprite, SpriteLayout::screen_rhw);
}

// Original CSprite vtable entries at 4ED2E8/4ED2EC/4ED2F0.
// EDX is unused; x and y remain callee-cleaned stack arguments.
extern "C" int32_t __fastcall kinoko_sprite_draw_404770(
    KinokoSprite *sprite, void *, float x, float y) {
    return draw(sprite, x, y, SpriteLayout::screen_rhw);
}

extern "C" int32_t __fastcall kinoko_sprite_draw_4049c0(
    KinokoSprite *sprite, void *, float x, float y) {
    return draw(sprite, x, y, SpriteLayout::screen_rhw);
}

extern "C" int32_t __fastcall kinoko_sprite_draw_404bc0(
    KinokoSprite *sprite, void *, float x, float y) {
    return draw(sprite, x, y, SpriteLayout::legacy_object_space);
}
