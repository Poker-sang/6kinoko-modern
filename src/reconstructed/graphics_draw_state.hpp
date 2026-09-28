#pragma once
#include "kinoko/render_resources.hpp"
#include "kinoko/graphics_api.hpp"
namespace kinoko::render {
// Snapshot owns values, borrows device. Raw native states remain in this adapter
// so even unfamiliar values and failed-Get defaults round-trip without coercion.
class GraphicsDrawState final : public SavedDrawState {
    kinoko::graphics::Device* device_;
    ScopeKind kind_;
    std::uint32_t u_=0,v_=0,blend_[4]{};
    inline static constexpr kinoko::graphics::RenderState types_[4]={
        kinoko::graphics::state_srcblend,kinoko::graphics::state_destblend,kinoko::graphics::state_blendop,kinoko::graphics::state_alphablendenable};
public:
    GraphicsDrawState(kinoko::graphics::Device* device,ScopeKind kind):device_(device),kind_(kind) {
        if(!device_) return;
        if(kind_!=ScopeKind::blend) {
            if(kind_==ScopeKind::act_pass) u_=v_=kinoko::graphics::address_wrap;
            device_->GetSamplerState(0,kinoko::graphics::sampler_addressu,&u_);
            device_->GetSamplerState(0,kinoko::graphics::sampler_addressv,&v_);
            const auto address=kind_==ScopeKind::act_pass?kinoko::graphics::address_clamp:kinoko::graphics::address_wrap;
            device_->SetSamplerState(0,kinoko::graphics::sampler_addressu,address);
            device_->SetSamplerState(0,kinoko::graphics::sampler_addressv,address);
        }
        if(kind_!=ScopeKind::map_wrap) {
            for(int i=0;i<4;++i) device_->GetRenderState(types_[i],&blend_[i]);
            device_->SetRenderState(kinoko::graphics::state_alphablendenable,true);
        }
    }
    void restore() override {
        if(!device_) return;
        if(kind_!=ScopeKind::map_wrap)
            for(int i=0;i<4;++i) device_->SetRenderState(types_[i],blend_[i]);
        if(kind_!=ScopeKind::blend) {
            device_->SetSamplerState(0,kinoko::graphics::sampler_addressu,u_);
            device_->SetSamplerState(0,kinoko::graphics::sampler_addressv,v_);
        }
        device_=nullptr;
    }
    ~GraphicsDrawState() override { restore(); }
    GraphicsDrawState(const GraphicsDrawState&)=delete;
    GraphicsDrawState& operator=(const GraphicsDrawState&)=delete;
};
}
