#pragma once
#include <cstdint>
#include <cstdlib>
#include <list>
#include <string>

namespace kinoko::text {
// Native runtime storage, never a byte-backed ACT/DAT record. The current
// single-character path leaves bitmap null and pixels empty. Copying preserves
// the original pointer-list assignment (not a deep buffer clone); generic tagged
// rendering must not be added without revisiting that ownership contract.
struct FontRenderer {
    void *device_context=nullptr, *font_handle=nullptr, *previous_font=nullptr;
    char face[256]{};
    std::uint8_t colors[6]{};
    std::int32_t font_height=0, font_weight=400;
    std::uint8_t italic=0, edge=0, wrap=0;
    std::int32_t text_limit=100000, margin_left=0, margin_top=0, character_space=0, line_space=0;
    std::int32_t cursor_x=0, cursor_y=0, ascent=0;
    std::uint32_t *output=nullptr, *destination=nullptr; // Borrowed for one map/raster call.
    std::int32_t bound_height=0, bound_width=0, stride=0;
    std::uint32_t *gradient=nullptr, *bitmap=nullptr;
    std::list<void*> pixels;
    std::uint32_t color=0;
    std::string label;

    FontRenderer() = default;
    FontRenderer(const FontRenderer&) = default;
    FontRenderer& operator=(const FontRenderer&) = default;
    void clear_pixels() { for (void* value : pixels) std::free(value); pixels.clear(); }
    ~FontRenderer() { clear_pixels(); std::free(bitmap); }
};
struct FontAtlas {
    std::int32_t cursor_x=0, cursor_y=0, row_height=0, width=0, height=0;
    std::int32_t last_glyph_id=0;
    FontRenderer renderer;
    std::int32_t texture=0, references=0;
    // Texture release belongs to the shared page owner, not this destructor.
};
}
