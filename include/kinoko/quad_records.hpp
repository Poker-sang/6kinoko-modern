#pragma once
#include "kinoko/sprite.h"
#include "kinoko/native_record_view.hpp"
#include "kinoko/quad_state.hpp"
#include <array>
#include <cstdint>
namespace kinoko::render {
// Borrowed texture handle. The enclosing layout/animation owns this storage.
struct QuadRecord {
    const void* vtable;
    int32_t texture;
    std::array<KinokoSpriteVertex, 4> vertices;
    float texture_width, texture_height;
    std::array<Position3, 4> base_positions, positions;
    float source_u_extent, source_v_extent;
};
using QuadView = native::RecordView<QuadRecord>;
static_assert(offsetof(QuadRecord,texture)==sizeof(void*));
static_assert(offsetof(QuadRecord,vertices)==sizeof(void*)+sizeof(int32_t));
#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(QuadRecord) == 232);
static_assert(offsetof(QuadRecord, positions) == 176);
#endif
}
