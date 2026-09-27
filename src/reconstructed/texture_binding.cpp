#include "kinoko/texture_store.h"
#include "kinoko/graphics_device.h"
#include "kinoko/texture_bindings.hpp"

extern "C" void kinoko_trace_i32(const char*,int32_t);
namespace {
// 4059C0 initializes eight consecutive borrowed handles at original 51AEE8.
// They are cache keys only; ownership stays with the store and D3D device.
kinoko::render::TextureBindings stage_handles;
class BindingSink final : public kinoko::render::TextureBindingSink {
    IDirect3DDevice9& device_;
public:
    explicit BindingSink(IDirect3DDevice9& device):device_(device) {}
    int32_t bind(uint32_t stage,int32_t handle) override {
        return device_.SetTexture(stage,handle?kinoko_texture_slots[handle].texture:nullptr);
    }
};
}
extern "C" void kinoko_initialize_texture_cache(void) { stage_handles.clear(); }
extern "C" void kinoko_texture_forget_bindings(int32_t handle) {
    stage_handles.forget(handle);
}
extern "C" int32_t kinoko_texture_bind_stage(int32_t stage,int32_t handle) {
    auto* device=kinoko_graphics.device;
    // Existing invalid-device/handle guards remain reconstruction boundaries.
    // Eight stages correspond to the recovered cache, not an unbounded offset.
    if (stage<0 || stage>=KINOKO_TEXTURE_STAGE_COUNT || !device ||
        !*reinterpret_cast<void***>(device)) return E_FAIL;
    if (handle && (handle<0 || handle>=KINOKO_TEXTURE_CAPACITY || !kinoko_texture_slots[handle].texture)) {
        kinoko_trace_i32("texture:unresolved-handle",handle);
        return E_FAIL;
    }
    BindingSink sink(*device);
    return stage_handles.bind(sink,static_cast<uint32_t>(stage),handle);
}
