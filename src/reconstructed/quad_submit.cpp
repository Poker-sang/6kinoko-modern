#include "kinoko/texture_store.h"
#include "kinoko/graphics_device.h"
#include "kinoko/quad_render.h"
#include "kinoko/quad_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/diagnostics.h"
#include "graphics_quad_sink.hpp"
extern "C" {
void kinoko_trace_i32(const char *,int32_t);
}

namespace {
template<class Publish> int32_t submit(void* storage,int32_t texture,
    std::array<KinokoSpriteVertex,4>& vertices,
    const std::array<kinoko::render::Position3,4>& positions,float x,float y,Publish publish) {
    if (!storage || !kinoko_graphics.device) return E_FAIL;
    static volatile LONG traces;
    const bool trace=InterlockedIncrement(&traces)<=8;
    if (trace) {
        kinoko_trace_i32("draw:vertex-buffer",kinoko::legacy::address(storage));
        kinoko_trace_i32("draw:handle",texture);
    }
    for(size_t i=0;i<vertices.size();++i) {
        vertices[i].x=positions[i].x+x-0.5f;
        vertices[i].y=positions[i].y+y-0.5f;
        vertices[i].z=positions[i].z+0.5f;
        vertices[i].rhw=1.0f;
    }
    publish(vertices);
    const auto texture_result=kinoko_texture_bind_stage(0,texture);
    if (trace) kinoko_trace_hresult("draw:set-texture-hr",texture_result);
    auto *device=kinoko_graphics.device;
    if (!device || !kinoko::legacy::load<const void *>(device)) return E_FAIL;
    kinoko::render::GraphicsQuadSink sink(*device);
    const auto format_result=sink.set_layout(kinoko::render::SpriteLayout::screen_rhw);
    if (trace) kinoko_trace_hresult("draw:set-fvf-hr",format_result);
    // Original ignores setup HRESULTs; draw supplies the result.
    const auto result=sink.draw_quad(vertices.data());
    if (trace) kinoko_trace_hresult("draw:primitive-hr",result);
    return result;
}
}
// Legacy layouts still use their original fixed quad record.
extern "C" int32_t kinoko_quad_submit(KinokoQuad* storage,float x,float y) {
    if(!storage) return E_FAIL;
    using kinoko::render::QuadRecord;
    const kinoko::render::QuadView quad(storage);
    auto vertices=quad.get(&QuadRecord::vertices);
    return submit(storage,quad.get(&QuadRecord::texture),vertices,quad.get(&QuadRecord::positions),x,y,
        [&](const auto& value) { quad.set(&QuadRecord::vertices,value); });
}
int32_t kinoko::render::submit_quad(QuadState& quad,float x,float y) {
    return submit(&quad,quad.texture,quad.vertices,quad.positions,x,y,[](const auto&){});
}
