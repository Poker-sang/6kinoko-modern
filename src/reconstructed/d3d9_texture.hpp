#pragma once
#include "kinoko/render_resources.hpp"
#include "kinoko/com_owner.hpp"
#include "kinoko/gpu_legacy_api.hpp"
extern "C" HRESULT WINAPI D3DXCreateTexture(IDirect3DDevice9*,UINT,UINT,UINT,DWORD,
    D3DFORMAT,D3DPOOL,IDirect3DTexture9**);
namespace kinoko::render {
// Owns exactly one native reference; create replaces it, detach transfers it
// to the transitional legacy registry. The device remains borrowed.
class D3D9Texture final : public Texture, public TextureCreation {
    IDirect3DDevice9* device_;
    kinoko::ComOwner<IDirect3DTexture9> texture_;
public:
    explicit D3D9Texture(IDirect3DDevice9* device):device_(device) {}
    int32_t create(const TextureDescription& d) override {
        return D3DXCreateTexture(device_,d.width,d.height,1,
            d.usage==TextureUsage::render_target?D3DUSAGE_RENDERTARGET:0,
            d.format==PixelFormat::argb1555?D3DFMT_A1R5G5B5:D3DFMT_A8R8G8B8,
            d.usage==TextureUsage::render_target?D3DPOOL_DEFAULT:D3DPOOL_MANAGED,texture_.put());
    }
    int32_t map(PixelMapping& output) override {
        D3DLOCKED_RECT native{};
        const auto result=texture_->LockRect(0,&native,nullptr,0);
        output={native.pBits,native.Pitch};
        return result;
    }
    int32_t unmap() override { return texture_->UnlockRect(0); }
    IDirect3DTexture9* get() const { return texture_.get(); }
    IDirect3DTexture9* detach() { return texture_.detach(); }
    explicit operator bool() const { return bool(texture_); }
};
}
