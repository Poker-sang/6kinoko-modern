#include "kinoko/string_runtime.hpp"
#include "kinoko/string_layout.h"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/font_runtime.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include <windows.h>
#include <algorithm>
#include <cstddef>
#include <deque>
#include <cstdlib>
#include <new>
#include <stdexcept>

#include "kinoko/act_host.h"
namespace {


using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::address;
using kinoko::legacy::StringView;
using Layout=kinoko::text::StringLayout;
using Deque=std::deque<kinoko::text::Glyph>;
Deque& queue(KinokoStringLayout* layout) { return reinterpret_cast<Layout*>(layout)->glyphs; }
void pop(Deque& q,bool front) { if(front) q.pop_front();else q.pop_back(); }

}

extern "C" int32_t kinoko_string_push_back(KinokoStringLayout* object,const char* text) {
    if(!text) return 0;
    auto& layout=*reinterpret_cast<Layout*>(object);
    StringView(&layout.pending).append(text,static_cast<uint32_t>(std::strlen(text)));
    return 1;
}
extern "C" int32_t kinoko_string_mark_rebuild(KinokoStringLayout* object) {
    reinterpret_cast<Layout*>(object)->rebuild=1;
    return 1;
}
extern "C" int32_t kinoko_string_clear(KinokoStringLayout* object) {
    auto& layout=*reinterpret_cast<Layout*>(object);
    StringView(&layout.text).assign("",0);
    StringView(&layout.pending).assign("",0);
    layout.cursor_x = 0;
    layout.cursor_y = 0;
    layout.maximum_width = 0;
    layout.line_height = layout.font_height;
    return kinoko_string_mark_rebuild(object);
}
extern "C" int32_t kinoko_string_character_bytes(const char* text) {
    return text && *text?static_cast<int32_t>(CharNextA(text)-text):0;
}
extern "C" int32_t kinoko_string_pop(KinokoStringLayout* object,int32_t count,int32_t front) {
    if(count<0) return 0;
    if(!count) return 1;
    auto& q=queue(object);
    auto& layout=*reinterpret_cast<Layout*>(object);
    StringView pending(&layout.pending);
    if(front) while(count && !q.empty()) { pop(q,true);--count; }
    while(count && pending.length()) {
        const uint32_t size=pending.length();
        const char* data=pending.data();
        if(front) {
            // 4406AA subtracts one before the erase end iterator. Preserve the
            // original quirk: an ASCII pending character erases zero bytes.
            const uint32_t bytes=static_cast<uint32_t>(CharNextA(data)-data)-1;
            pending.assign(pending,bytes,UINT32_MAX);
        } else {
            const uint32_t bytes=static_cast<uint32_t>(data+size-CharPrevA(data,data+size));
            pending.assign(pending,0,size-bytes);
        }
        --count;
    }
    if(!front) while(count && !q.empty()) { pop(q,false);--count; }
    return kinoko_string_prune_atlases(object);
}
extern "C" int32_t kinoko_string_replicate(KinokoStringLayout* object,KinokoStringLayout* source) {
    if(!source) return 0;
    reinterpret_cast<Layout*>(object)->replicate(*reinterpret_cast<Layout*>(source));
    return 1;
}

// 442300 appends a default CSpriteEx value into the one-element block deque.
// The original leaves non-vtable/default-texture fields for 404EE0 to fill.
extern "C" void* kinoko_string_append_glyph(KinokoStringLayout* layout) {
    auto& q=queue(layout);
    q.emplace_back();
    return &q.back();
}

extern "C" uint32_t kinoko_string_queue_size(KinokoStringLayout* object) { return static_cast<uint32_t>(queue(object).size()); }
extern "C" void* kinoko_string_queue_at(KinokoStringLayout* object,uint32_t index) { return &queue(object).at(index); }
