#pragma once
#include <stdint.h>

typedef struct KinokoSpriteVertex {
    float x, y, z, rhw;
    uint32_t color;
    float u, v;
} KinokoSpriteVertex;

