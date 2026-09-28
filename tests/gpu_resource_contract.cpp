#include "kinoko/graphics_api.hpp"
#include "kinoko/com_owner.hpp"
#include <cstring>
int main(){
    kinoko::graphics::Texture* raw=nullptr;
    if(kinoko::graphics::failed(kinoko::graphics::create_texture(nullptr,4,2,1,0,kinoko::graphics::format_a8r8g8b8,kinoko::graphics::pool_managed,&raw)))return 1;
    kinoko::ComOwner<kinoko::graphics::Texture> texture(raw);
    kinoko::graphics::MappedPixels rect{};
    if(kinoko::graphics::failed(texture->LockRect(0,&rect,nullptr,0))||rect.Pitch!=16)return 2;
    const uint32_t pixel=0xff123456;std::memcpy(static_cast<unsigned char*>(rect.pBits)+20,&pixel,4);
    if(kinoko::graphics::failed(texture->UnlockRect(0)))return 3;
    kinoko::graphics::PixelRect region{1,1,4,2};if(kinoko::graphics::failed(texture->LockRect(0,&rect,&region,0)))return 4;
    uint32_t observed;std::memcpy(&observed,rect.pBits,4);if(observed!=pixel||rect.Pitch!=16)return 5;
    texture->UnlockRect(0);
    kinoko::ComOwner<kinoko::graphics::Surface> surface;if(kinoko::graphics::failed(texture->GetSurfaceLevel(0,surface.put())))return 6;
    texture.reset();kinoko::graphics::SurfaceDescription desc{};if(kinoko::graphics::failed(surface->GetDesc(&desc))||desc.Width!=4||desc.Height!=2)return 7;
    kinoko::graphics::Buffer buffer(16);void* bytes=nullptr;
    if(kinoko::graphics::failed(buffer.Lock(4,12,&bytes,0))||bytes!=buffer.bytes.data()+4)return 8;
    if(kinoko::graphics::succeeded(buffer.Lock(4,13,&bytes,0)))return 9;
    return 0;
}
