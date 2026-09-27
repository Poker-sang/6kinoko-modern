#pragma once

#include <cstdint>

namespace kinoko::act {

// Render-time values. The legacy ACT object and the serialized property schema
// are adapted at the caller boundary; neither layout is this native object.
struct TextureRuntimeState {
    const void* methods = nullptr;
    std::int32_t handle = 0;
    std::int32_t width = 0, height = 0;
    float source_x = 0, source_y = 0;
    float source_width = 0, source_height = 0;
    std::uint8_t auto_size = 0;
};

}
