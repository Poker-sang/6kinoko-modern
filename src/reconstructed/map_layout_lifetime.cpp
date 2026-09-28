#include "kinoko/native_buffer.h"
#include "kinoko/map_render.h"
#include "kinoko/act_host.h"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/memory_access.hpp"
#include <array>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>

namespace {

struct VectorView { unsigned char *begin, *end; void* owner; };
struct Spec { size_t offset,width,copy_bytes; bool sprite=false; };
using Layout = kinoko::map::LayoutRecord;
using Quad=kinoko::render::QuadRecord;
using Chip=kinoko::map::ChipDefinition;
using Cached=kinoko::map::ChipSpriteCache;
constexpr std::array<Spec,10> vectors={{{offsetof(Layout, placements),32,32},
    {offsetof(Layout, chip_references),sizeof(Chip*),sizeof(Chip*)},
    {offsetof(Layout, texture_references),sizeof(void*),sizeof(void*)},
    {offsetof(Layout, render_quads),sizeof(Quad),sizeof(Quad),true},
    // Dormant opaque legacy render streams retain their 4/12-byte elements;
    // only the enclosing begin/end/owner records contain host pointers.
    {offsetof(Layout, render_reference_buffers),4,4},
    {offsetof(Layout, render_reference_buffers)+sizeof(Layout::ReservedRenderBuffer),12,12},
    {offsetof(Layout,chip_sprites),sizeof(Cached),offsetof(Cached,valid)+1,true},
    {offsetof(Layout,chip_definitions),sizeof(Chip),sizeof(Chip)},
    {offsetof(Layout,changed_chips),sizeof(Chip*),sizeof(Chip*)},
    {offsetof(Layout,chip_indices),sizeof(int32_t),sizeof(int32_t)}}};
struct Free { void operator()(void *p) const { std::free(p); } };
using Owned=std::unique_ptr<void,Free>;
Owned allocate(size_t size) {
    Owned value(std::malloc(size));if(!value) throw std::bad_alloc();return value;
}
}
extern "C" KinokoActLayout* __fastcall kinoko_clone_map_layout(KinokoActLayout* source,void*) {
    // This is 433AA0's full virtual clone, not ACT activation's cache reset.
    Owned output=allocate(sizeof(Layout));
    auto* result=static_cast<unsigned char*>(output.get());
    const auto* input=reinterpret_cast<const unsigned char*>(source);
    kinoko::memory::store(result, kinoko_act_host_symbols()->map_layout_vtable);
    kinoko::memory::store(result+sizeof(void*), kinoko_act_host_symbols()->map_view_vtable);
    constexpr size_t quad_begin=offsetof(Layout,sprite_and_base)+sizeof(void*);
    constexpr size_t quad_end=offsetof(Layout,sprite_and_base)+sizeof(Quad);
    std::memcpy(result+quad_begin,input+quad_begin,quad_end-quad_begin);
    const kinoko::map::LayoutView out(result), in(const_cast<unsigned char*>(input));
    auto copy=[&](auto member){ out.set(member,in.get(member)); };
    copy(&Layout::layer_type);copy(&Layout::max_chip_width);copy(&Layout::max_chip_height);
    copy(&Layout::chip_left);copy(&Layout::chip_top);copy(&Layout::chip_right);copy(&Layout::chip_bottom);
    copy(&Layout::owning_layer);copy(&Layout::cached_chip_resource);
    copy(&Layout::alpha);copy(&Layout::scale);copy(&Layout::blend);
    copy(&Layout::render_count);copy(&Layout::chip_sprite_count);
    copy(&Layout::maximum_chip_id);copy(&Layout::render_scan_cache);
    std::array<std::vector<uint32_t>,vectors.size()> buffers;
    for(size_t i=0;i<vectors.size();++i) {
        const auto spec=vectors[i];const auto in=kinoko::memory::load<VectorView>(input+spec.offset);
        const auto bytes=reinterpret_cast<uintptr_t>(in.end)-reinterpret_cast<uintptr_t>(in.begin);
        if(reinterpret_cast<intptr_t>(in.end)<reinterpret_cast<intptr_t>(in.begin) || bytes%spec.width || bytes>0x7fffffffu) throw std::bad_alloc();
        kinoko::memory::store(result+spec.offset, VectorView{});
        if(!bytes) continue;
        buffers[i].resize(bytes/4);
        auto* begin=reinterpret_cast<unsigned char*>(buffers[i].data());
        for(uint32_t pos=0;pos<bytes;pos+=spec.width) {
            std::memcpy(begin+pos,in.begin+pos,spec.copy_bytes);
            if(spec.sprite) kinoko::memory::store(begin+pos, kinoko_act_host_symbols()->chip_quad_vtable);
        }

    }
    // Fourth words between vector views are untouched, as in 433780/433AE0.
    kinoko::map::LayoutView(output.get()).set(&kinoko::map::LayoutRecord::suppress_next_binding, uint8_t{1});
    try {
        for(size_t i=0;i<vectors.size();++i)
            kinoko_native_buffer_replace(result+vectors[i].offset, buffers[i].data(), static_cast<uint32_t>(buffers[i].size()*4));
    } catch(...) {
        for(auto spec:vectors) kinoko_native_buffer_destroy(result+spec.offset);
        throw;
    }
    return static_cast<KinokoActLayout*>(output.release());
}
extern "C" void kinoko_clear_map_layout(KinokoActLayout* layout) {
    if (!layout) return;
    auto* bytes = reinterpret_cast<unsigned char*>(layout);
    kinoko::memory::store(bytes, kinoko_act_host_symbols()->map_layout_vtable);
    kinoko::memory::store(bytes + sizeof(void*), kinoko_act_host_symbols()->map_view_vtable);
    for (auto it = vectors.rbegin(); it != vectors.rend(); ++it) {
        const auto view = kinoko::memory::load<VectorView>(bytes + it->offset);
        // Sprite destructors reset identity; texture handles remain borrowed.
        if (it->sprite)
            for (auto* p = view.begin; p != view.end; p += it->width)
                kinoko::memory::store(p, kinoko_act_host_symbols()->color_vtable);
        kinoko_native_buffer_destroy(bytes + it->offset);
    }
    kinoko::memory::store(bytes + sizeof(void*), kinoko_act_host_symbols()->color_vtable);
}
extern "C" void* __fastcall kinoko_delete_map_sprite(void* sprite, void*, int32_t flags) {
    auto* layout = static_cast<unsigned char*>(sprite) - sizeof(void*);
    if (flags & 2) {
        auto* allocation = layout - sizeof(uint32_t);
        const auto count = kinoko::memory::load<uint32_t>(allocation);
        for (auto i = count; i > 0; --i)
            kinoko_clear_map_layout(reinterpret_cast<KinokoActLayout*>(layout + sizeof(Layout)*(i-1)));
        if (flags & 1) std::free(allocation);
        return allocation;
    }
    kinoko_clear_map_layout(reinterpret_cast<KinokoActLayout*>(layout));
    if (flags & 1) std::free(layout);
    return layout;
}
