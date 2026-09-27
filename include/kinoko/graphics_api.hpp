#pragma once
#include <windows.h>
#include "kinoko/graphics_types.hpp"
#include <atomic>
#include <memory>
#include <vector>
namespace kinoko::graphics {
// Transitional source interface, not COM and not a D3D9 device. Only the
// audited game call surface exists; all rendering is recorded for SDL GPU.
struct Image;
struct Ref {
    std::atomic<ULONG> refs{1};
    virtual ~Ref()=default;
    virtual ULONG AddRef(){return ++refs;}
    virtual ULONG Release(){const auto n=--refs;if(!n)delete this;return n;}
};
struct Surface:Ref {
    std::shared_ptr<Image> image;
    explicit Surface(std::shared_ptr<Image> value):image(std::move(value)){}
    HRESULT GetDesc(kinoko::graphics::SurfaceDescription*);
};
struct Texture:Ref {
    std::shared_ptr<Image> image;
    bool locked=false;
    std::vector<uint8_t> staging;
    explicit Texture(std::shared_ptr<Image> value):image(std::move(value)){}
    HRESULT LockRect(UINT,kinoko::graphics::MappedPixels*,const RECT*,DWORD);
    HRESULT UnlockRect(UINT);
    HRESULT GetSurfaceLevel(UINT,Surface**);
};
struct Buffer:Ref {
    std::vector<uint8_t> bytes;
    explicit Buffer(UINT size):bytes(size){}
    HRESULT Lock(UINT offset,UINT size,void** out,DWORD);
    HRESULT Unlock(){return S_OK;}
};
struct Declaration:Ref {};
struct Device;
struct SwapChain:Ref {
    Device* device;
    explicit SwapChain(Device* value):device(value){}
    HRESULT Present(const RECT*,const RECT*,HWND,const RGNDATA*,DWORD);
};
struct Device:Ref {
    struct State;std::unique_ptr<State> state;
    Device(HWND,UINT,UINT);~Device() override;
    HRESULT GetDeviceCaps(kinoko::graphics::Capabilities*);
    HRESULT GetSwapChain(UINT,SwapChain**);
    HRESULT TestCooperativeLevel();HRESULT Reset(kinoko::graphics::Presentation*);
    HRESULT BeginScene();HRESULT EndScene();HRESULT present();
    HRESULT Clear(DWORD,const kinoko::graphics::ClearRect*,DWORD,kinoko::graphics::Color,float,DWORD);
    HRESULT GetRenderState(kinoko::graphics::RenderState,DWORD*);
    HRESULT SetRenderState(kinoko::graphics::RenderState,DWORD);
    HRESULT GetSamplerState(DWORD,kinoko::graphics::SamplerState,DWORD*);
    HRESULT SetSamplerState(DWORD,kinoko::graphics::SamplerState,DWORD);
    HRESULT SetTextureStageState(DWORD,kinoko::graphics::TextureStageState,DWORD);
    HRESULT GetTexture(DWORD,Texture**);HRESULT SetTexture(DWORD,Texture*);
    HRESULT GetRenderTarget(DWORD,Surface**);HRESULT SetRenderTarget(DWORD,Surface*);
    HRESULT GetDepthStencilSurface(Surface**);
    HRESULT GetTransform(kinoko::graphics::Transform,kinoko::graphics::Matrix*);
    HRESULT SetTransform(kinoko::graphics::Transform,const kinoko::graphics::Matrix*);
    HRESULT SetFVF(DWORD);
    HRESULT DrawPrimitiveUP(kinoko::graphics::Primitive,UINT,const void*,UINT);
    HRESULT CreateIndexBuffer(UINT,DWORD,kinoko::graphics::Format,kinoko::graphics::Pool,Buffer**,HANDLE*);
    HRESULT CreateVertexBuffer(UINT,DWORD,DWORD,kinoko::graphics::Pool,Buffer**,HANDLE*);
    HRESULT CreateVertexDeclaration(const kinoko::graphics::VertexElement*,Declaration**);
    HRESULT SetVertexDeclaration(Declaration*);
    HRESULT SetStreamSource(UINT,Buffer*,UINT,UINT);
    HRESULT SetIndices(Buffer*);
    HRESULT DrawIndexedPrimitive(kinoko::graphics::Primitive,INT,UINT,UINT,UINT,UINT);
};
struct Factory:Ref {
    HRESULT GetAdapterDisplayMode(UINT,kinoko::graphics::DisplayMode*);
    HRESULT CreateDevice(UINT,kinoko::graphics::DeviceKind,HWND,DWORD,kinoko::graphics::Presentation*,Device**);
};
Factory* create_factory(UINT);
void stop(); // Cancels producer backpressure before the application joins workers.
using BaseTexture=Texture;
using VertexBuffer=Buffer;
using IndexBuffer=Buffer;
HRESULT create_texture(Device*,UINT,UINT,UINT,DWORD,Format,Pool,Texture**);
}
