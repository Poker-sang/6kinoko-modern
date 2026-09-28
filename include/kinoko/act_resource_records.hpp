#pragma once
#include "kinoko/native_record_view.hpp"
#include "kinoko/act_types.h"
#include "kinoko/act_frame.h"
#include "kinoko/act_script_storage.hpp"
#include "kinoko/legacy_string.hpp"

#include <array>
#include <cstdint>
#include <mutex>

struct SQVM;

namespace kinoko::act {
struct FindState; // owned native implementation, defined in act_runtime_lifecycle.cpp
// Representation only: the actual Squirrel API owns the external reference.
using ObjectStorage = ScriptValueStorage;
struct StagePropertyAliases {
    int32_t *margin_left, *margin_right, *margin_top, *margin_bottom;
    float *offset_x, *offset_y;
    uint8_t *visible;
    int32_t *resolution_ms, *screen_width, *screen_height;
    legacy::StringRecord *name;
};
static_assert(sizeof(StagePropertyAliases) == 11*sizeof(void*));
struct RuntimeRecord {
    KinokoActSourceHolder *source_holder; // borrowed; never owns the holder or source ACT
    int32_t current_time;
    uint8_t stage_active;
    std::array<uint8_t, 3> unknown9;
    KinokoActDocument *active_document; // owned clone; destructor never consults source_holder
    KinokoActSourceHolder *active_holder; // owned wrapper; borrows active_document
    std::recursive_mutex* lock; // owned; raw record storage does not construct mutexes
    KinokoActCommandStorage draw_commands;
    uint32_t unknown56;
    KinokoActSpriteStorage draw_sprites;
    uint32_t unknown72;
    KinokoActResource *render_target; // borrowed CActRenderTarget, texture handle at +68
    uint32_t unknown80;
    FindState *find_state; // owned C++ find map, never an emulated STL tree
    uint32_t find_count, unknown92, next_find_id, wake_time;
    uint8_t hidden;
    std::array<uint8_t, 3> unknown105;
    StagePropertyAliases stage_properties;
    SQVM *vm; // borrowed VM, owns environment through an external reference
    alignas(void*) ObjectStorage environment;
    kinoko::legacy::StringRecord name;
    uint32_t unknown188;
};
static_assert(std::is_standard_layout_v<RuntimeRecord>);
static_assert(sizeof(RuntimeRecord::wake_time) == 4);
}
