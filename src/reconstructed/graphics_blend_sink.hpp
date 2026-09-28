#pragma once
#include "kinoko/render_blend.hpp"
#include "kinoko/graphics_api.hpp"
namespace kinoko::render {
// Non-owning adapter; scoped to a call so Reset cannot leave a cached device.
class GraphicsBlendSink final : public BlendSink {
    kinoko::graphics::Device &device_;
    static std::uint32_t factor(BlendFactor value) {
        switch(value) {
        case BlendFactor::zero: return kinoko::graphics::blend_zero;
        case BlendFactor::one: return kinoko::graphics::blend_one;
        case BlendFactor::destination_color: return kinoko::graphics::blend_destcolor;
        case BlendFactor::source_color: return kinoko::graphics::blend_srccolor;
        case BlendFactor::source_alpha: return kinoko::graphics::blend_srcalpha;
        case BlendFactor::inverse_source_alpha: return kinoko::graphics::blend_invsrcalpha;
        default: return 0;
        }
    }
public:
    explicit GraphicsBlendSink(kinoko::graphics::Device &device) : device_(device) {}
    int32_t set_operation(BlendOperation value) override {
        if(value==BlendOperation::unchanged) return kinoko::graphics::ok;
        return device_.SetRenderState(kinoko::graphics::state_blendop,
            value==BlendOperation::add ? kinoko::graphics::blend_operation_add : kinoko::graphics::blend_operation_revsubtract);
    }
    int32_t set_source(BlendFactor value) override {
        if(value==BlendFactor::unchanged) return kinoko::graphics::ok;
        return device_.SetRenderState(kinoko::graphics::state_srcblend,factor(value));
    }
    int32_t set_destination(BlendFactor value) override {
        if(value==BlendFactor::unchanged) return kinoko::graphics::ok;
        return device_.SetRenderState(kinoko::graphics::state_destblend,factor(value));
    }
};
}
