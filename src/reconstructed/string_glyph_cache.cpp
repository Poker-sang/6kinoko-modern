#include "kinoko/runtime_util.hpp"
#include "kinoko/string_runtime.hpp"
#include "kinoko/resource_allocation.hpp"
#include "kinoko/texture_store.h"
#include "kinoko/string_layout.h"
#include "kinoko/memory_access.hpp"
#include "kinoko/script_diagnostics.hpp"
#include <algorithm>
#include <climits>
using kinoko::legacy::StringView;
using Layout=kinoko::text::StringLayout;

extern "C" int32_t kinoko_string_prune_atlases(KinokoStringLayout* receiver) {
    auto& layout=*reinterpret_cast<Layout*>(receiver);
    int32_t minimum=INT_MAX;
    for(const auto& glyph:layout.glyphs) minimum=(std::min)(minimum,glyph.id);
    auto& pages=layout.atlases;
    pages.erase(std::remove_if(pages.begin(),pages.end(),[minimum](const auto& page) {
        return page->last_glyph_id<minimum && page->references<=0;
    }),pages.end());
    return 1;
}

// 4410C0 rebuilds the pending byte string and discards glyph sprites. It does
// not rasterize text here: later update consumes stBackQueue using kinoko::text::next.
extern "C" int32_t kinoko_string_rebuild_queue(KinokoStringLayout* receiver) {
    auto* layout=receiver;
    auto& record=*reinterpret_cast<kinoko::text::StringLayout*>(receiver);
    StringView text(&record.text);
    StringView queue(&record.pending);
    std::string pending(text.data(),text.length());
    pending.append(queue.data(),queue.length());
    text.assign("",0);
    queue.assign("",0);
    using Layout=kinoko::text::StringLayout;
    record.rebuild = uint8_t{1};
    record.cursor_x = 0;
    record.cursor_y = 0;
    record.line_height = record.font_height;
    record.maximum_width = 0;
    // Original passes the concatenated buffer through strlen (441160).
    queue.append(pending.c_str(),static_cast<uint32_t>(std::strlen(pending.c_str())));
    record.glyphs.clear();
    return kinoko_string_prune_atlases(receiver);
}


extern "C" KinokoStringLayout* kinoko_create_string_layout() {
    auto* layout=kinoko::act::allocate_resources<Layout>(kinoko_string_layout_methods());
    if(!layout) return nullptr;
    try {
        static const char face[]="\x82\x6c\x82\x72\x20\x83\x53\x83\x56\x83\x62\x83\x4e";
        StringView(&layout->face).assign(face,13);
    } catch(...) {
        kinoko::act::destroy_resources(layout,1,[](auto&){});
        return nullptr;
    }
    return reinterpret_cast<KinokoStringLayout*>(layout);
}
extern "C" KinokoStringLayout* __fastcall kinoko_method_delete_string_layout(KinokoStringLayout* object,void*,unsigned char flags) {
    return reinterpret_cast<KinokoStringLayout*>(kinoko::act::destroy_resources(
        reinterpret_cast<Layout*>(object),flags,[](auto&){}));
}
extern "C" void* kinoko_string_append_atlas(KinokoStringLayout* layout) {
    return reinterpret_cast<Layout*>(layout)->append_atlas(kinoko_texture_release);
}
extern "C" uint32_t kinoko_string_atlas_size(KinokoStringLayout* layout) {
    return static_cast<uint32_t>(reinterpret_cast<Layout*>(layout)->atlases.size());
}
extern "C" void* kinoko_string_atlas_at(KinokoStringLayout* layout,uint32_t index) {
    return reinterpret_cast<Layout*>(layout)->atlases.at(index).get();
}
extern "C" KinokoStringLayout* __fastcall kinoko_method_clone_string_layout(KinokoStringLayout* source,void*) {
    auto* out=kinoko_create_string_layout();
    if(!out) return nullptr;
    try {
        reinterpret_cast<Layout*>(out)->copy_state(*reinterpret_cast<Layout*>(source));
    } catch(...) {
        kinoko_method_delete_string_layout(out,nullptr,1);
        return nullptr;
    }
    return out;
}
extern "C" KinokoStringLayout* __fastcall kinoko_method_destroy_string_layout(KinokoStringLayout* object,void*) {
    return kinoko_method_delete_string_layout(object,nullptr,1);
}
extern "C" int32_t kinoko_string_layout_type_identity;
extern "C" const void* __fastcall kinoko_method_string_layout_type(KinokoStringLayout*,void*) {
    return &kinoko_string_layout_type_identity;
}
