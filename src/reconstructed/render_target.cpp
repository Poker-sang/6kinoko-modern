#include "graphics_texture.hpp"
#include "kinoko/critical_section.h"
#include "kinoko/renderer.h"
#include "kinoko/quad_render.h"
#include "kinoko/act_draw_records.hpp"
#include "kinoko/act_texture_resource.hpp"
#include "kinoko/graphics_lock.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/render_target.h"
#include "kinoko/texture_store.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/com_owner.hpp"
#include <windows.h>
#include "kinoko/graphics_api.hpp"
#include <algorithm>
#include <set>
#include "kinoko/device_listeners.hpp"
#include <stdexcept>
#include "kinoko/legacy_abi.h"
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
// 401DC0 -> 402670/402360 stores the unsigned handle as a 20-byte set-node
// value. It does not AddRef/retain the texture; the set owns only its nodes.
std::set<uint32_t> render_targets;
std::set<uint32_t> depth_targets;
kinoko::graphics::DeviceListeners device_listeners;
using GraphicsLock=kinoko::graphics::Lock;
int32_t create(uint32_t width,uint32_t height) {
    if(kinoko_graphics.capabilities.TextureCaps&0x20) width=height=(std::max)(width,height);
    kinoko::render::GraphicsTexture texture(kinoko_graphics.device);
    {
        GraphicsLock lock;
        if(FAILED(texture.create({width,height,kinoko::render::PixelFormat::argb8888,
            kinoko::render::TextureUsage::render_target}))) return 0;
    }
    const int32_t handle=kinoko_texture_register(texture.get(),width,height);
    if(handle) texture.detach();
    // Original inserts the returned handle even when registration returned 0.
    render_targets.insert(static_cast<uint32_t>(handle));
    return handle;
}
}
extern "C" void kinoko_initialize_renderer_sets(void) {
    render_targets.clear();depth_targets.clear();
}
extern "C" int32_t kinoko_set_render_target(int32_t handle) {
    // 401E60 selects level zero, releases the temporary surface reference,
    // and restores the renderer's cached backbuffer for handle zero.
    auto* device=kinoko_renderer.device;
    if(!device) return E_FAIL;
    if(!handle) return device->SetRenderTarget(0,kinoko_renderer.backbuffer);
    if(handle<0 || handle>=KINOKO_TEXTURE_CAPACITY) return E_INVALIDARG;
    auto* texture=static_cast<kinoko::graphics::Texture*>(kinoko_texture_slots[handle].texture);
    if(!texture) return E_INVALIDARG;
    kinoko::ComOwner<kinoko::graphics::Surface> surface;
    const auto status=texture->GetSurfaceLevel(0,surface.put());
    if(FAILED(status)) return status;
    device->SetRenderTarget(0,surface.get());
    return surface.detach()->Release();
}
extern "C" int32_t __fastcall kinoko_method_create_render_target(KinokoActResource *resource,void*,int32_t width,int32_t height) {
    // 449C10 returns true even if D3DX creation fails; never reinterpret that
    // return as handle != 0. Requested image dimensions remain unsquared.
    auto& view = kinoko::act::texture_resource(resource);
    view.source_x = 0.0f;view.source_y = 0.0f;
    view.width = width;view.height = height;
    view.source_width = static_cast<float>(width);
    view.source_height = static_cast<float>(height);
    view.auto_size = uint8_t{0};
    view.texture = create(static_cast<uint32_t>(width),static_cast<uint32_t>(height));
    return 1;
}

// 401660/4016E0: insertion order, duplicate suppression and borrowed objects.
extern "C" void kinoko_initialize_device_listeners(void) { device_listeners.clear(); }
extern "C" int32_t kinoko_add_device_listener(KinokoDeviceListener *object) {
    GraphicsLock lock;
    return device_listeners.add(object);
}
extern "C" void kinoko_remove_device_listener(KinokoDeviceListener *object) {
    GraphicsLock lock;
    device_listeners.remove(object);
}
extern "C" void kinoko_notify_device_listeners(KinokoDeviceEvent event) {
    // Caller holds the recursive graphics lock. No object-prefix/vtable cast.
    device_listeners.notify(event);
}

extern "C" int32_t kinoko_renderer_before_reset(KinokoRenderer *renderer) {
    // 401DA0 releases acquired references, leaving slot bits intact as original.
    renderer->backbuffer->Release();
    return renderer->depth_stencil->Release();
}
extern "C" int32_t kinoko_renderer_after_reset(KinokoRenderer *renderer) {
    auto *device=renderer->device;
    for(DWORD stage=0;stage<8;++stage) kinoko_graphics.device->SetTexture(stage,nullptr);
    kinoko_initialize_texture_cache();
    const KinokoRenderState saved=renderer->state;
    renderer->state={};renderer->present_pending=0;
    device->GetRenderTarget(0,&renderer->backbuffer);
    device->GetDepthStencilSurface(&renderer->depth_stencil);
    const auto state=[&](kinoko::graphics::RenderState type,DWORD value) { device->SetRenderState(type,value); };
    const DWORD alpha=saved.alpha_flags&255u;
    const DWORD test=(saved.alpha_flags>>8)&255u;
    if(alpha) state(kinoko::graphics::state_alphablendenable,alpha);
    if(test) state(kinoko::graphics::state_alphatestenable,test);
    renderer->state.alpha_flags=alpha|(test<<8);
    device->SetTextureStageState(0,kinoko::graphics::texture_stage_alphaop,kinoko::graphics::texture_operation_modulate);
    if(saved.alpha_function) { state(kinoko::graphics::state_alphafunc,saved.alpha_function);renderer->state.alpha_function=saved.alpha_function; }
    if(saved.alpha_reference) { state(kinoko::graphics::state_alpharef,saved.alpha_reference);renderer->state.alpha_reference=saved.alpha_reference; }
    state(kinoko::graphics::state_zenable,saved.depth_flags&255u);
    state(kinoko::graphics::state_zwriteenable,(saved.depth_flags>>8)&255u);
    renderer->state.depth_flags=saved.depth_flags&0xffff;
    state(kinoko::graphics::state_zfunc,saved.depth_function);renderer->state.depth_function=saved.depth_function;
    // The original process owns a single renderer; named state helpers use it.
    kinoko_render_set_filter(saved.filter);kinoko_render_set_blend(saved.blend);
    kinoko_render_set_cull(saved.cull);
    state(kinoko::graphics::state_stencilmask,255);
    return device->Clear(0,nullptr,kinoko::graphics::clear_stencil,0,1.0f,0);
}
