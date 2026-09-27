#pragma once
#include "kinoko/render_resources.hpp"
#include <d3d9.h>
namespace kinoko::render {
// Snapshot owns values, borrows device. Raw native states remain in this adapter
// so even unfamiliar values and failed-Get defaults round-trip without coercion.
class D3D9DrawState final : public SavedDrawState {
    IDirect3DDevice9* device_;
    ScopeKind kind_;
    DWORD u_=0,v_=0,blend_[4]{};
    inline static constexpr D3DRENDERSTATETYPE types_[4]={
        D3DRS_SRCBLEND,D3DRS_DESTBLEND,D3DRS_BLENDOP,D3DRS_ALPHABLENDENABLE};
public:
    D3D9DrawState(IDirect3DDevice9* device,ScopeKind kind):device_(device),kind_(kind) {
        if(!device_) return;
        if(kind_!=ScopeKind::blend) {
            if(kind_==ScopeKind::act_pass) u_=v_=D3DTADDRESS_WRAP;
            device_->GetSamplerState(0,D3DSAMP_ADDRESSU,&u_);
            device_->GetSamplerState(0,D3DSAMP_ADDRESSV,&v_);
            const auto address=kind_==ScopeKind::act_pass?D3DTADDRESS_CLAMP:D3DTADDRESS_WRAP;
            device_->SetSamplerState(0,D3DSAMP_ADDRESSU,address);
            device_->SetSamplerState(0,D3DSAMP_ADDRESSV,address);
        }
        if(kind_!=ScopeKind::map_wrap) {
            for(int i=0;i<4;++i) device_->GetRenderState(types_[i],&blend_[i]);
            device_->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
        }
    }
    void restore() override {
        if(!device_) return;
        if(kind_!=ScopeKind::map_wrap)
            for(int i=0;i<4;++i) device_->SetRenderState(types_[i],blend_[i]);
        if(kind_!=ScopeKind::blend) {
            device_->SetSamplerState(0,D3DSAMP_ADDRESSU,u_);
            device_->SetSamplerState(0,D3DSAMP_ADDRESSV,v_);
        }
        device_=nullptr;
    }
    ~D3D9DrawState() override { restore(); }
    D3D9DrawState(const D3D9DrawState&)=delete;
    D3D9DrawState& operator=(const D3D9DrawState&)=delete;
};
}
