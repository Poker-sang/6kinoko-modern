#pragma once
#include "kinoko/act_types.h"
#include "kinoko/quad_records.hpp"
#include "kinoko/legacy_string.hpp"
namespace kinoko::act {
using render::Position3;
struct Layout2DRecord {
    const void *methods;
    render::QuadRecord quad;
    Position3 rotation,rotation_pivot,scale,scale_pivot;
    float alpha;
    int32_t blend,red,green,blue;
    KinokoActLayer *layer;
    int32_t texture;
    uint8_t pivots_initialized;
    uint8_t padding313[3];
};
struct Layout3DRecord {
    const void *methods;
    Position3 translation,rotation,scale;
    KinokoActLayer *layer;
    float world[16];
};
static_assert(sizeof(Layout2DRecord)==316 && offsetof(Layout2DRecord,layer)==304);
static_assert(offsetof(Layout2DRecord,rotation)==236 && offsetof(Layout2DRecord,texture)==308);
static_assert(sizeof(Layout3DRecord)==108 && offsetof(Layout3DRecord,world)==44);
}
