#include "kinoko/renderer.h"
#include "kinoko/quad_render.h"
#include "graphics_blend_sink.hpp"
#include "kinoko/graphics_api.hpp"
extern "C" int32_t kinoko_render_set_blend(int32_t mode) {
    auto &cached=kinoko_renderer.state.blend;
    if(!kinoko_renderer.device) return kinoko::render::set_blend(cached,mode,nullptr);
    kinoko::render::GraphicsBlendSink sink(*kinoko_renderer.device);
    return kinoko::render::set_blend(cached,mode,&sink);
}
extern "C" int32_t kinoko_render_set_alpha(int32_t blend_enabled,int32_t test_enabled) {
    auto *device=kinoko_renderer.device;
    auto *flags=reinterpret_cast<unsigned char *>(&kinoko_renderer.state.alpha_flags);
    const auto blend=static_cast<uint8_t>(blend_enabled),test=static_cast<uint8_t>(test_enabled);
    HRESULT result=S_OK;
    if(flags[0]!=blend) {
        if(device) result=device->SetRenderState(kinoko::graphics::state_alphablendenable,blend);
        flags[0]=blend;
    }
    if(flags[1]!=test) {
        if(device) result=device->SetRenderState(kinoko::graphics::state_alphatestenable,test);
        flags[1]=test;
    }
    if(device) result=device->SetTextureStageState(0,kinoko::graphics::texture_stage_alphaop,kinoko::graphics::texture_operation_modulate);
    return result;
}

extern "C" int32_t kinoko_render_set_depth(int32_t test_enabled,int32_t write_enabled) {
    auto *device=kinoko_renderer.device;
    const auto test=static_cast<uint8_t>(test_enabled),write=static_cast<uint8_t>(write_enabled);
    HRESULT result=S_OK;
    if(device) {
        device->SetRenderState(kinoko::graphics::state_zenable,test);
        result=device->SetRenderState(kinoko::graphics::state_zwriteenable,write);
    }
    auto *flags=reinterpret_cast<unsigned char *>(&kinoko_renderer.state.depth_flags);
    flags[1]=write;flags[0]=test;
    return result;
}

extern "C" int32_t kinoko_render_set_filter(int32_t mode) {
    if(kinoko_renderer.state.filter==mode) return 0;
    auto *device=kinoko_renderer.device;
    HRESULT status=S_OK;
    if(device && (mode==1 || mode==2)) {
        device->SetSamplerState(0,kinoko::graphics::sampler_magfilter,mode);
        device->SetSamplerState(0,kinoko::graphics::sampler_minfilter,mode);
        status=device->SetSamplerState(0,kinoko::graphics::sampler_mipfilter,mode);
    }
    kinoko_renderer.state.filter=mode;
    return status;
}
extern "C" int32_t kinoko_render_set_cull(int32_t mode) {
    if(kinoko_renderer.state.cull==mode) return 0;
    HRESULT status=S_OK;
    if(kinoko_graphics.device && mode>=1 && mode<=3)
        status=kinoko_graphics.device->SetRenderState(kinoko::graphics::state_cullmode,mode);
    kinoko_renderer.state.cull=mode;
    return status;
}
