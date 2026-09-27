#pragma once
#include "kinoko/render_resources.hpp"
#include "kinoko/com_owner.hpp"
#include "kinoko/graphics_api.hpp"
namespace kinoko::render {
// Owns exactly one native reference; create replaces it, detach transfers it
// to the transitional legacy registry. The device remains borrowed.
class GraphicsTexture final : public Texture, public TextureCreation {
    kinoko::graphics::Device* device_;
    kinoko::ComOwner<kinoko::graphics::Texture> texture_;
public:
    explicit GraphicsTexture(kinoko::graphics::Device* device):device_(device) {}
    int32_t create(const TextureDescription& d) override {
        return kinoko::graphics::create_texture(device_,d.width,d.height,1,
            d.usage==TextureUsage::render_target?kinoko::graphics::usage_rendertarget:0,
            d.format==PixelFormat::argb1555?kinoko::graphics::format_a1r5g5b5:kinoko::graphics::format_a8r8g8b8,
            d.usage==TextureUsage::render_target?kinoko::graphics::pool_default:kinoko::graphics::pool_managed,texture_.put());
    }
    int32_t map(PixelMapping& output) override {
        kinoko::graphics::MappedPixels native{};
        const auto result=texture_->LockRect(0,&native,nullptr,0);
        output={native.pBits,native.Pitch};
        return result;
    }
    int32_t unmap() override { return texture_->UnlockRect(0); }
    kinoko::graphics::Texture* get() const { return texture_.get(); }
    kinoko::graphics::Texture* detach() { return texture_.detach(); }
    explicit operator bool() const { return bool(texture_); }
};
}
