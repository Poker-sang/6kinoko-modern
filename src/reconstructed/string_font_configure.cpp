#include "kinoko/string_runtime.hpp"
#include "kinoko/string_font.h"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/legacy_string.hpp"
#include <cstring>
using kinoko::legacy::StringView;
extern "C" void kinoko_string_font_configure(kinoko::text::FontRenderer* r,KinokoStringLayout* layout) {
    // 440910/440CA0 preserve the other config bytes and set equal RGB endpoints.
    using Layout=kinoko::text::StringLayout;
    auto& text=*reinterpret_cast<Layout*>(layout);
    strcpy_s(reinterpret_cast<char*>(r->face),256,
        StringView(&text.face).data());
    const uint8_t colors[]={static_cast<uint8_t>(text.red),
        static_cast<uint8_t>(text.green),
        static_cast<uint8_t>(text.blue)};
    auto* endpoints=r->colors;
    for(int channel=0;channel<3;++channel)
        endpoints[channel*2]=endpoints[channel*2+1]=colors[channel];
    r->font_height = text.font_height;
    r->font_weight = text.font_weight;
    r->edge = text.edge;
    r->character_space = text.character_space;
    r->line_space = text.line_space;
    r->color = (uint32_t(endpoints[0])<<16)|
        (uint32_t(endpoints[2])<<8)|endpoints[4];
    // 40EE30's equal-color branch leaves an existing gradient allocation alone.
    r->clear_pixels();
}
