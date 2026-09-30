#pragma once
#include "kinoko/graphics_types.hpp"
#include <atomic>
#include <memory>
#include <vector>
namespace kinoko::graphics {
// Transitional source interface, not COM and not a D3D9 device. Only the
// audited game call surface exists; all rendering is recorded for SDL GPU.
struct Image;
struct Ref {
    std::atomic<std::uint32_t> refs{1};
    virtual ~Ref()=default;
    virtual std::uint32_t AddRef(){return ++refs;}
    virtual std::uint32_t Release(){const auto n=--refs;if(!n)delete this;return n;}
};
struct Surface:Ref {
    std::shared_ptr<Image> image;
    explicit Surface(std::shared_ptr<Image> value):image(std::move(value)){}
    kinoko::graphics::Result GetDesc(kinoko::graphics::SurfaceDescription*);
};
struct Texture:Ref {
    std::shared_ptr<Image> image;
    bool locked=false;
    std::vector<uint8_t> staging;
    explicit Texture(std::shared_ptr<Image> value):image(std::move(value)){}
    kinoko::graphics::Result LockRect(std::uint32_t,kinoko::graphics::MappedPixels*,const kinoko::graphics::PixelRect*,std::uint32_t);
    kinoko::graphics::Result UnlockRect(std::uint32_t);
    kinoko::graphics::Result GetSurfaceLevel(std::uint32_t,Surface**);
};
struct Buffer:Ref {
    std::vector<uint8_t> bytes;
    explicit Buffer(std::uint32_t size):bytes(size){}
    kinoko::graphics::Result Lock(std::uint32_t offset,std::uint32_t size,void** out,std::uint32_t);
    kinoko::graphics::Result Unlock(){return kinoko::graphics::ok;}
};
struct Declaration:Ref {};
struct Device;
struct SwapChain:Ref {
    Device* device;
    explicit SwapChain(Device* value):device(value){}
    kinoko::graphics::Result Present();
};
struct Device:Ref {
    struct State;std::unique_ptr<State> state;
    Device(std::uint32_t,std::uint32_t);~Device() override;
    kinoko::graphics::Result GetDeviceCaps(kinoko::graphics::Capabilities*);
    kinoko::graphics::Result GetSwapChain(std::uint32_t,SwapChain**);
    kinoko::graphics::Result TestCooperativeLevel();kinoko::graphics::Result Reset(kinoko::graphics::Presentation*);
    kinoko::graphics::Result BeginScene();kinoko::graphics::Result EndScene();kinoko::graphics::Result present();
    kinoko::graphics::Result Clear(std::uint32_t,const kinoko::graphics::ClearRect*,std::uint32_t,kinoko::graphics::Color,float,std::uint32_t);
    kinoko::graphics::Result GetRenderState(kinoko::graphics::RenderState,std::uint32_t*);
    kinoko::graphics::Result SetRenderState(kinoko::graphics::RenderState,std::uint32_t);
    kinoko::graphics::Result GetSamplerState(std::uint32_t,kinoko::graphics::SamplerState,std::uint32_t*);
    kinoko::graphics::Result SetSamplerState(std::uint32_t,kinoko::graphics::SamplerState,std::uint32_t);
    kinoko::graphics::Result SetTextureStageState(std::uint32_t,kinoko::graphics::TextureStageState,std::uint32_t);
    kinoko::graphics::Result GetTexture(std::uint32_t,Texture**);kinoko::graphics::Result SetTexture(std::uint32_t,Texture*);
    kinoko::graphics::Result GetRenderTarget(std::uint32_t,Surface**);kinoko::graphics::Result SetRenderTarget(std::uint32_t,Surface*);
    kinoko::graphics::Result GetDepthStencilSurface(Surface**);
    kinoko::graphics::Result GetTransform(kinoko::graphics::Transform,kinoko::graphics::Matrix*);
    kinoko::graphics::Result SetTransform(kinoko::graphics::Transform,const kinoko::graphics::Matrix*);
    kinoko::graphics::Result SetFVF(std::uint32_t);
    kinoko::graphics::Result DrawPrimitiveUP(kinoko::graphics::Primitive,std::uint32_t,const void*,std::uint32_t);
    kinoko::graphics::Result CreateIndexBuffer(std::uint32_t,std::uint32_t,kinoko::graphics::Format,kinoko::graphics::Pool,Buffer**);
    kinoko::graphics::Result CreateVertexBuffer(std::uint32_t,std::uint32_t,std::uint32_t,kinoko::graphics::Pool,Buffer**);
    kinoko::graphics::Result CreateVertexDeclaration(const kinoko::graphics::VertexElement*,Declaration**);
    kinoko::graphics::Result SetVertexDeclaration(Declaration*);
    kinoko::graphics::Result SetStreamSource(std::uint32_t,Buffer*,std::uint32_t,std::uint32_t);
    kinoko::graphics::Result SetIndices(Buffer*);
    kinoko::graphics::Result DrawIndexedPrimitive(kinoko::graphics::Primitive,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
};
struct Factory:Ref {
    kinoko::graphics::Result GetAdapterDisplayMode(std::uint32_t,kinoko::graphics::DisplayMode*);
    kinoko::graphics::Result CreateDevice(std::uint32_t,kinoko::graphics::DeviceKind,std::uint32_t,kinoko::graphics::Presentation*,Device**);
};
Factory* create_factory(std::uint32_t);
void stop(); // Cancels producer backpressure before the application joins workers.
// Worker-side, outside graphics/scene locks; preserves TAS render packets.
void wait_for_tas_render();
using BaseTexture=Texture;
using VertexBuffer=Buffer;
using IndexBuffer=Buffer;
kinoko::graphics::Result create_texture(Device*,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,Format,Pool,Texture**);
}
