#pragma once
#include "kinoko/sprite_vertex.h"
#include <cstddef>
#include <cstdint>
namespace kinoko::render {
// Numeric results cross the existing C ABI unchanged. Zero is exact success
// for mapping; negative is failure. They are not serialized resource IDs.
struct PixelMapping { void* pixels=nullptr; std::ptrdiff_t pitch=0; };
enum class PixelFormat { argb1555, argb8888 };
enum class TextureUsage { sampled, render_target };
struct TextureDescription {
    uint32_t width, height;
    PixelFormat format;
    TextureUsage usage=TextureUsage::sampled;
};
class Texture {
public:
    virtual ~Texture() = default;
    virtual int32_t map(PixelMapping& output) = 0;
    virtual int32_t unmap() = 0;
};
// In-place creation: the implementing Texture owns the created resource.
// Re-creation releases the previous resource; there is no per-load wrapper allocation.
class TextureCreation {
public:
    virtual ~TextureCreation() = default;
    virtual int32_t create(const TextureDescription& description) = 0;
};
enum class SpriteLayout { screen_rhw, legacy_object_space };
class QuadSink {
public:
    virtual ~QuadSink() = default;
    virtual int32_t set_layout(SpriteLayout layout) = 0;
    // Exactly four vertices, consumed before return. Color is packed ARGB.
    // Coordinates already include the legacy screen-space half-pixel offset.
    virtual int32_t draw_quad(const KinokoSpriteVertex* vertices) = 0;
};
inline int32_t submit_quad(QuadSink& sink, SpriteLayout layout,
                           const KinokoSpriteVertex* vertices) {
    sink.set_layout(layout); // Setup failure does not suppress the draw.
    return sink.draw_quad(vertices);
}
enum class ScopeKind { blend, map_wrap, act_pass };
class SavedDrawState {
public:
    virtual ~SavedDrawState() = default;
    virtual void restore() = 0;
};
}
