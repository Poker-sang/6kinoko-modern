#pragma once
#include "kinoko/sprite_vertex.h"
#include <array>
#include <cstdint>
namespace kinoko::render {
struct Position3 { float x, y, z; };
// Native glyph geometry; no legacy vtable or fixed-size ABI.
struct QuadState {
    int32_t texture=0;
    std::array<KinokoSpriteVertex,4> vertices{};
    float texture_width=0, texture_height=0;
    std::array<Position3,4> base_positions{}, positions{};
    float source_u_extent=0, source_v_extent=0;
};
int32_t submit_quad(QuadState& quad,float x,float y);
}
