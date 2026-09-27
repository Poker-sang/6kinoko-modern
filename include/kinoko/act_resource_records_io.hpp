#pragma once
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_string.hpp"
#include "kinoko/native_record_view.hpp"
#include "kinoko/act_texture_resource.hpp"
#include <cstddef>
#include <cstdint>

#include "kinoko/boost_control.hpp"

namespace kinoko::act {
// Chip resources retain the original Win32 layout; textures are native objects.
// String fields own their native storage; MCD data is transferred on success.
struct ChipResourceRecord {
    const void *methods;
    int32_t id;
    legacy::StringRecord name;
    uint32_t unknown32;
    legacy::StringRecord source_name;
    uint32_t unknown60;
    kinoko_mcd_data *data;
    kinoko::native::upstream::CountedControl *shared_data; // one strong reference
    legacy::StringRecord loaded_path;
    uint32_t unknown96;
};
using ChipResourceFields = kinoko::native::RecordView<ChipResourceRecord>;
static_assert(sizeof(ChipResourceRecord) == 100);
static_assert(offsetof(ChipResourceRecord, source_name) == 36);
static_assert(offsetof(ChipResourceRecord, data) == 64);
static_assert(offsetof(ChipResourceRecord, shared_data) == 68);
static_assert(offsetof(ChipResourceRecord, loaded_path) == 72);
}
