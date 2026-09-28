#pragma once
#include "kinoko/act_resource_records.hpp"
#include "kinoko/sprite.h"

struct KinokoBlitCommand {
    int32_t blend;
    float alpha, x, y;
    int32_t source_x, source_y, width, height, texture;
};
struct KinokoBlitSprite { KinokoBlitCommand command; KinokoSprite sprite; };
namespace kinoko::act {
using BlitCommand = KinokoBlitCommand;
using BlitSprite = KinokoBlitSprite;
static_assert(sizeof(BlitCommand) == 36);
static_assert(offsetof(BlitSprite,sprite) % alignof(KinokoSprite) == 0);
#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(BlitSprite) == 184);
static_assert(offsetof(BlitCommand, texture) == 32 && offsetof(BlitSprite, sprite) == 36);
#endif
}
