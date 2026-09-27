#pragma once
#include "kinoko/render_resources.hpp"
#include "kinoko/gpu_legacy_api.hpp"
namespace kinoko::render {
class D3D9QuadSink final : public QuadSink {
    IDirect3DDevice9& device_;
public:
    explicit D3D9QuadSink(IDirect3DDevice9& device) : device_(device) {}
    int32_t set_layout(SpriteLayout layout) override {
        // 404BC0 uses the distinct original 0x4142 FVF and the same stride.
        return device_.SetFVF(layout==SpriteLayout::screen_rhw ?
            D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1 : 0x4142u);
    }
    int32_t draw_quad(const KinokoSpriteVertex* vertices) override {
        return device_.DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(*vertices));
    }
};
}
