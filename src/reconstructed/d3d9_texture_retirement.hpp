#pragma once
#include "kinoko/com_owner.hpp"
#include "kinoko/gpu_legacy_api.hpp"
namespace kinoko::render {
class D3D9TextureRetirement final {
    kinoko::ComOwner<IDirect3DBaseTexture9> owned_;
public:
    explicit D3D9TextureRetirement(IDirect3DBaseTexture9* owned):owned_(owned) {}
    void unbind(IDirect3DDevice9* device) {
        if(!device) return;
        for(DWORD stage=0;stage<8;++stage) {
            IDirect3DBaseTexture9* value=nullptr;
            if(SUCCEEDED(device->GetTexture(stage,&value)) && value) {
                kinoko::ComOwner<IDirect3DBaseTexture9> bound(value);
                if(bound.get()==owned_.get()) device->SetTexture(stage,nullptr);
            }
        }
    }
    void release() { owned_.reset(); }
};
}
