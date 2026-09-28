#pragma once
#include "kinoko/act_types.h"
#include "kinoko/font_runtime.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/quad_state.hpp"
#include <deque>
#include <memory>
#include <vector>
#include <utility>

namespace kinoko::text {
// Shared ownership keeps a replicated queue alive after its source is cleared.
// references retains the original prune semantics, independent of page owners.
struct Glyph {
    int32_t x=0,y=0,id=0,width=0,height=0;
    render::QuadState quad;
    std::shared_ptr<FontAtlas> atlas;
    Glyph()=default;
    Glyph(const Glyph& other) : x(other.x),y(other.y),id(other.id),width(other.width),
        height(other.height),quad(other.quad),atlas(other.atlas) {
        if(atlas) ++atlas->references;
    }
    Glyph& operator=(const Glyph& other) {
        if(this!=&other) { Glyph copy(other); swap(copy); }
        return *this;
    }
    void swap(Glyph& other) noexcept {
        using std::swap;
        swap(x,other.x);swap(y,other.y);swap(id,other.id);
        swap(width,other.width);swap(height,other.height);
        swap(quad,other.quad);swap(atlas,other.atlas);
    }
    void set_atlas(std::shared_ptr<FontAtlas> value) noexcept {
        if(atlas) --atlas->references;
        atlas=std::move(value);
        if(atlas) ++atlas->references;
    }
    ~Glyph() { if(atlas) --atlas->references; }
};
struct StringLayout {
    const void* methods=nullptr;
    legacy::StringRecord text{{},0,15}, pending{{},0,15}, face{{},0,15};
    int32_t font_height=16,font_weight=1;
    int32_t red=0,green=0,blue=0,base_red=255,base_green=255,base_blue=255;
    int32_t character_space=0,line_space=2;
    uint8_t edge=0;
    int32_t alignment=0;
    float scale_x=1,scale_y=1;
    int32_t wrap_width=-1;
    KinokoActLayer* layer=nullptr;
    float alpha=1;
    int32_t blend=1;
    std::vector<std::shared_ptr<FontAtlas>> atlases;
    std::deque<Glyph> glyphs;
    int32_t next_glyph_id=0,cursor_x=0,cursor_y=0,maximum_width=0,line_height=16,origin_x=0,origin_y=0;
    uint8_t rebuild=0;
    StringLayout()=default;
    StringLayout(const StringLayout&)=delete;
    StringLayout& operator=(const StringLayout&)=delete;
    ~StringLayout() {
        glyphs.clear();atlases.clear();
        legacy::StringView(&face).destroy();
        legacy::StringView(&pending).destroy();
        legacy::StringView(&text).destroy();
    }
    void copy_state(const StringLayout& source) {
        // 43EC30/43EB80 leave both caches empty in the clone. Do not copy
        // FontRenderer's owning raw buffers merely to discard them again.
        for(auto member:{&StringLayout::text,&StringLayout::pending,&StringLayout::face})
            legacy::StringView(&(this->*member)).assign(
                legacy::StringView(const_cast<legacy::StringRecord*>(&(source.*member))),0,UINT32_MAX);
        font_height=source.font_height;
        font_weight=source.font_weight;
        red=source.red;
        green=source.green;
        blue=source.blue;
        base_red=source.base_red;
        base_green=source.base_green;
        base_blue=source.base_blue;
        character_space=source.character_space;
        line_space=source.line_space;
        edge=source.edge;
        alignment=source.alignment;
        scale_x=source.scale_x;
        scale_y=source.scale_y;
        wrap_width=source.wrap_width;
        layer=source.layer;
        alpha=source.alpha;
        blend=source.blend;
        next_glyph_id=source.next_glyph_id;
        cursor_x=source.cursor_x;
        cursor_y=source.cursor_y;
        maximum_width=source.maximum_width;
        line_height=source.line_height;
        origin_x=source.origin_x;
        origin_y=source.origin_y;
        rebuild=source.rebuild;
        glyphs.clear();atlases.clear();
    }
    FontAtlas* append_atlas(int32_t (*release)(int32_t)) {
        auto page=std::shared_ptr<FontAtlas>(new FontAtlas,[release](FontAtlas* value) {
            if(value->texture) release(value->texture);
            delete value;
        });
        atlases.push_back(std::move(page));
        return atlases.back().get();
    }
    void replicate(const StringLayout& source) {
        if(this==&source) return;
        auto copy=source.glyphs; // Copy first: allocation failure preserves us.
        glyphs.swap(copy);
    }
};
}
