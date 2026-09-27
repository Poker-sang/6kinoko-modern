#pragma once
#include <cstdint>

namespace kinoko::graphics { struct Device; struct Surface; struct Texture; }

// Process-local runtime objects, never DAT records or original x86 byte views.
// A texture handle is a bounded registry index, not an encoded pointer.
using KinokoTextureHandle = std::int32_t;
struct KinokoTextureSlot {
    kinoko::graphics::Texture* texture;
    std::uint32_t width, height;
};

struct KinokoDeviceListener {
    void* context;
    void (*before_reset)(void* context);
    void (*after_reset)(void* context);
};
enum KinokoDeviceEvent { KINOKO_DEVICE_BEFORE_RESET, KINOKO_DEVICE_AFTER_RESET };

struct KinokoRenderState {
    std::int32_t blend, filter, cull, unknown24;
    std::uint32_t depth_flags;
    std::int32_t depth_function;
    std::uint32_t alpha_flags;
    std::int32_t alpha_function, alpha_reference, unknown48;
};
struct KinokoRenderer {
    kinoko::graphics::Device* device; // Borrowed from the graphics runtime.
    KinokoRenderState state;
    std::uint8_t present_pending;
    std::uint32_t clear_color;
    kinoko::graphics::Surface* backbuffer; // Each owns one acquired reference.
    kinoko::graphics::Surface* depth_stencil;
    KinokoDeviceListener listener; // Registration borrows this explicit member.
};
