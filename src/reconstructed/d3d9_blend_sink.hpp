#pragma once
#include "kinoko/render_blend.hpp"
#include "kinoko/gpu_legacy_api.hpp"
namespace kinoko::render {
// Non-owning adapter; scoped to a call so Reset cannot leave a cached device.
class D3D9BlendSink final : public BlendSink {
    IDirect3DDevice9 &device_;
    static DWORD factor(BlendFactor value) {
        switch(value) {
        case BlendFactor::zero: return D3DBLEND_ZERO;
        case BlendFactor::one: return D3DBLEND_ONE;
        case BlendFactor::destination_color: return D3DBLEND_DESTCOLOR;
        case BlendFactor::source_color: return D3DBLEND_SRCCOLOR;
        case BlendFactor::source_alpha: return D3DBLEND_SRCALPHA;
        case BlendFactor::inverse_source_alpha: return D3DBLEND_INVSRCALPHA;
        default: return 0;
        }
    }
public:
    explicit D3D9BlendSink(IDirect3DDevice9 &device) : device_(device) {}
    int32_t set_operation(BlendOperation value) override {
        if(value==BlendOperation::unchanged) return S_OK;
        return device_.SetRenderState(D3DRS_BLENDOP,
            value==BlendOperation::add ? D3DBLENDOP_ADD : D3DBLENDOP_REVSUBTRACT);
    }
    int32_t set_source(BlendFactor value) override {
        if(value==BlendFactor::unchanged) return S_OK;
        return device_.SetRenderState(D3DRS_SRCBLEND,factor(value));
    }
    int32_t set_destination(BlendFactor value) override {
        if(value==BlendFactor::unchanged) return S_OK;
        return device_.SetRenderState(D3DRS_DESTBLEND,factor(value));
    }
};
}
