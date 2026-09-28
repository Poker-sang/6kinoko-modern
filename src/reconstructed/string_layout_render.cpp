#include "kinoko/act_method_dispatch.hpp"
#include "kinoko/string_runtime.hpp"
#include "graphics_draw_state.hpp"
#include "kinoko/act_layout_render.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/string_layout.h"
#include "kinoko/renderer.h"
#include "kinoko/quad_render.h"
#include "kinoko/legacy_memory.hpp"
#include <cstring>
using kinoko::legacy::pointer;
using kinoko::legacy::StringView;
namespace {
using TextRecord=kinoko::text::StringLayout;
using GlyphRecord=kinoko::text::Glyph;
}
extern "C" int32_t __fastcall kinoko_method_set_string_layer(KinokoStringLayout* object,void*,KinokoActLayer* layer) {
    if(!layer) return E_FAIL;
    reinterpret_cast<TextRecord*>(object)->layer=layer;return 0;
}
extern "C" int32_t __fastcall kinoko_method_update_string_layout(KinokoStringLayout* layout,void*) {
    auto& text=*reinterpret_cast<TextRecord*>(layout);
    auto *layer=text.layer;
    if(!layer) return E_FAIL;
    if(text.rebuild) { kinoko_string_rebuild_queue(layout);text.rebuild = uint8_t{0}; }
    StringView pending(&text.pending),displayed(&text.text);
    // Original consumes pending multibyte characters before visibility testing.
    while(pending.length()) {
        const auto bytes=static_cast<uint32_t>(CharNextA(pending.data())-pending.data());
        char character[8]{};memcpy_s(character,sizeof(character),pending.data(),bytes);
        kinoko_string_add_character(layout, character);
        displayed.append(character,static_cast<uint32_t>(std::strlen(character)));
        pending.assign(pending,bytes,UINT32_MAX);
    }
    const kinoko::map::LayerView owner(layer);
    if(!owner.get(&kinoko::map::LayerRecord::visible)) return 0;
    const auto alpha=static_cast<uint32_t>(static_cast<int64_t>(text.alpha*255.0));
    const uint32_t color=(alpha<<24)|(uint32_t(static_cast<uint8_t>(text.base_red))<<16)|
        (uint32_t(static_cast<uint8_t>(text.base_green))<<8)|static_cast<uint8_t>(text.base_blue);
    const uint32_t count=kinoko_string_queue_size(layout);
    for(uint32_t i=0;i<count;++i) {
        auto& glyph=*reinterpret_cast<GlyphRecord*>(kinoko_string_queue_at(layout, i));
        auto quad=glyph.quad;
        for(auto &vertex:quad.vertices) vertex.color=color;
        glyph.quad = quad;
    }
    float x=0,y=0,z=0;
    kinoko::act::LayerMethods(layer).position(&x,&y,&z);
    text.origin_x = static_cast<int32_t>(x);text.origin_y = static_cast<int32_t>(y);
    for(uint32_t i=0;i<count;++i) {
        auto& glyph=*reinterpret_cast<GlyphRecord*>(kinoko_string_queue_at(layout, i));
        float gx=static_cast<float>(glyph.x);
        const auto alignment=text.alignment;
        if(alignment==1) gx-=text.maximum_width/2;
        if(alignment==2) gx-=text.maximum_width;
        const auto sx=text.scale_x,sy=text.scale_y;
        const float dx=static_cast<float>(double(sx)*gx+text.origin_x);
        const auto height=glyph.height;
        const float dy=static_cast<float>((double(height)-double(height)*sy)*0.5+
            text.origin_y+double(sy)*static_cast<float>(glyph.y));
        auto quad=glyph.quad;quad.positions=quad.base_positions;
        for(auto &p:quad.positions) { p.x*=sx;p.y*=sy;p.z*=1.0f;p.x+=dx;p.y+=dy;p.z+=0.0f; }
        glyph.quad = quad;
    }
    return 0;
}
extern "C" int32_t __fastcall kinoko_method_draw_string_layout(KinokoStringLayout* layout,void*,float x,float y) {
    auto& text=*reinterpret_cast<TextRecord*>(layout);
    auto *layer=text.layer;
    if(!layer) return E_FAIL;
    if(!kinoko::map::LayerView(layer).get(&kinoko::map::LayerRecord::visible)) return 0;
    auto *device=kinoko_graphics.device;
    kinoko::render::GraphicsDrawState saved(device,kinoko::render::ScopeKind::blend);
    kinoko::act::set_layout_blend(text.blend);
    kinoko_render_set_filter(2);
    const uint32_t count=kinoko_string_queue_size(layout);
    for(uint32_t i=0;i<count;++i) {
        auto& glyph=*reinterpret_cast<GlyphRecord*>(kinoko_string_queue_at(layout, i));
        kinoko::render::submit_quad(glyph.quad,x,y);
    }
    saved.restore();
    return 0;
}
