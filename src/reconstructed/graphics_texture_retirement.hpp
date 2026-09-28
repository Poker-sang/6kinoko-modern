#pragma once
#include "kinoko/com_owner.hpp"
#include "kinoko/graphics_api.hpp"
namespace kinoko::render {
class GraphicsTextureRetirement final {
    kinoko::ComOwner<kinoko::graphics::BaseTexture> owned_;
public:
    explicit GraphicsTextureRetirement(kinoko::graphics::BaseTexture* owned):owned_(owned) {}
    void unbind(kinoko::graphics::Device* device) {
        if(!device) return;
        for(std::uint32_t stage=0;stage<8;++stage) {
            kinoko::graphics::BaseTexture* value=nullptr;
            if(kinoko::graphics::succeeded(device->GetTexture(stage,&value)) && value) {
                kinoko::ComOwner<kinoko::graphics::BaseTexture> bound(value);
                if(bound.get()==owned_.get()) device->SetTexture(stage,nullptr);
            }
        }
    }
    void release() { owned_.reset(); }
};
}
