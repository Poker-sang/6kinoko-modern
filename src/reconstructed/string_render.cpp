#include "kinoko/map_layout_records.hpp"
#include "kinoko/renderer.h"
#include "kinoko/act_layout_render.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/quad_render.h"
#include "kinoko/string_layout.h"
#include "kinoko/string_font.h"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/font_runtime.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/texture_store.h"
#include <windows.h>
#include "kinoko/graphics_api.hpp"
#include <algorithm>
#include <cstring>
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::StringView;
using LayoutRecord=kinoko::act::StringLayoutRecord;
using GlyphRecord=kinoko::act::StringGlyphRecord;
using AtlasRecord=kinoko::text::FontAtlas;
using kinoko::native::RecordView;
void* new_page(KinokoStringLayout* layout) {
    auto* page=kinoko_string_append_atlas(layout);
    auto* atlas=static_cast<AtlasRecord*>(page);
    atlas->cursor_x = 0;
    atlas->cursor_y = 0;
    atlas->row_height = 0;
    atlas->references = 0;
    atlas->width = 512;
    atlas->height = 512;
    kinoko_string_font_configure(&atlas->renderer, layout);
    atlas->texture = kinoko_string_font_texture(&atlas->renderer);
    return page;
}
// 404EE0's CSpriteEx geometry and texture coordinates. The generic RetDec
// wrapper lost the queried texture dimensions; use the actual texture store.
void rectangle(kinoko::render::QuadView quad, int32_t handle, int32_t x, int32_t y, int32_t w, int32_t h) {
    using Quad = kinoko::render::QuadRecord;
    quad.set(&Quad::texture, handle);
    if (handle) {
        const float tw = static_cast<float>(kinoko_texture_slots[handle].width);
        const float th = static_cast<float>(kinoko_texture_slots[handle].height);
        quad.set(&Quad::texture_width, tw); quad.set(&Quad::texture_height, th);
        const float du = w / tw, dv = h / th, u = x / tw, v = y / th;
        quad.set(&Quad::source_u_extent, du); quad.set(&Quad::source_v_extent, dv);
        auto vertices = quad.get(&Quad::vertices);
        vertices[0].u = vertices[2].u = u;
        vertices[0].v = vertices[1].v = v;
        vertices[1].u = vertices[3].u = du + u;
        vertices[2].v = vertices[3].v = dv + v;
        quad.set(&Quad::vertices, vertices);
    } else {
        quad.set(&Quad::texture_width, 0.0f); quad.set(&Quad::texture_height, 0.0f);
        quad.set(&Quad::source_u_extent, 0.0f); quad.set(&Quad::source_v_extent, 0.0f);
    }
    auto vertices = quad.get(&Quad::vertices);
    for (auto& vertex : vertices) vertex.color = 0xffffffffu;
    quad.set(&Quad::vertices, vertices);
    quad.set(&Quad::base_positions, std::array<kinoko::render::Position3, 4>{
        kinoko::render::Position3{-0.0f, -0.0f, 0.0f}, {static_cast<float>(w), -0.0f, 0.0f},
        {-0.0f, static_cast<float>(h), 0.0f}, {static_cast<float>(w), static_cast<float>(h), 0.0f}});

}

}
extern "C" int32_t kinoko_string_add_character(KinokoStringLayout* layout,const char* character) {
    const RecordView<LayoutRecord> text(layout);
    int32_t cursor=text.get(&LayoutRecord::cursor_x);
    const auto font_height=text.get(&LayoutRecord::font_height);
    if(*character=='\t') {
        const int32_t tab=4*font_height;
        text.set(&LayoutRecord::cursor_x,cursor+tab-cursor%tab);
        return 1;
    }
    if(*character=='\n') {
        text.set(&LayoutRecord::cursor_y,
            text.get(&LayoutRecord::cursor_y)+text.get(&LayoutRecord::line_height));
        text.set(&LayoutRecord::cursor_x,0);
        text.set(&LayoutRecord::line_height,font_height);
        return 1;
    }
    for(;;) {
        if(kinoko_string_atlas_size(layout)==0) new_page(layout);
        auto* page=kinoko_string_atlas_at(layout, kinoko_string_atlas_size(layout)-1);
        auto* atlas=static_cast<AtlasRecord*>(page);
        kinoko_string_font_configure(&atlas->renderer, layout);
        int32_t width=0,height=0;
        kinoko_string_font_upload(&atlas->renderer, atlas->texture, character, atlas->cursor_x, atlas->cursor_y, &width, &height);
        if(width>=atlas->width || height>=atlas->height) return 0;
        if(atlas->cursor_x+width>=atlas->width ||
            (!width && atlas->height-atlas->row_height-
                atlas->cursor_y>font_height)) {
            atlas->cursor_y =
                atlas->cursor_y+atlas->row_height;
            atlas->cursor_x = 0;
            atlas->row_height = 0;
            continue;
        }
        if(atlas->cursor_y+height>=atlas->height || !height) {
            new_page(layout);continue;
        }
        // 440A9B-440BF4 is absent from IDA's decompilation: width/height are
        // output parameters of 405F80, not constants. Follow the assembly.
        atlas->references = atlas->references+1;
        auto* glyph=kinoko_string_append_glyph(layout);
        const RecordView<GlyphRecord> sprite(glyph);
        auto quad=sprite.view(&GlyphRecord::quad);
        rectangle(quad,atlas->texture,atlas->cursor_x,
            atlas->cursor_y,width,height);
        quad.set(&kinoko::render::QuadRecord::positions, quad.get(&kinoko::render::QuadRecord::base_positions));
        sprite.set(&GlyphRecord::x,cursor);
        sprite.set(&GlyphRecord::y,text.get(&LayoutRecord::cursor_y));
        sprite.set(&GlyphRecord::width,width);
        sprite.set(&GlyphRecord::height,height);
        const auto id=text.get(&LayoutRecord::next_glyph_id);
        sprite.set(&GlyphRecord::id,id);
        atlas->last_glyph_id = id;
        sprite.set(&GlyphRecord::atlas,page);
        text.set(&LayoutRecord::next_glyph_id,id+1);
        atlas->row_height = (std::max)(atlas->row_height,height);
        atlas->cursor_x = atlas->cursor_x+width;
        cursor+=width;
        auto line_height=(std::max)(text.get(&LayoutRecord::line_height),height);
        if(text.get(&LayoutRecord::wrap_width)>=0 && cursor>=text.get(&LayoutRecord::wrap_width)) {
            text.set(&LayoutRecord::cursor_y,text.get(&LayoutRecord::cursor_y)+line_height);
            cursor=0;line_height=font_height;
        }
        text.set(&LayoutRecord::cursor_x,cursor);
        text.set(&LayoutRecord::line_height,line_height);
        text.set(&LayoutRecord::maximum_width,(std::max)(text.get(&LayoutRecord::maximum_width),cursor));
        return 1;
    }
}
