#pragma once
#include <windows.h>
#include <d3d9.h>
#ifdef KINOKO_GPU_GAME
#include <atomic>
#include <memory>
#include <vector>
namespace kinoko::gpu_legacy {
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
    HRESULT GetDesc(D3DSURFACE_DESC*);
};
struct Texture:Ref {
    std::shared_ptr<Image> image;
    bool locked=false;
    std::vector<uint8_t> staging;
    explicit Texture(std::shared_ptr<Image> value):image(std::move(value)){}
    HRESULT LockRect(UINT,D3DLOCKED_RECT*,const RECT*,DWORD);
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
    HRESULT GetDeviceCaps(D3DCAPS9*);
    HRESULT GetSwapChain(UINT,SwapChain**);
    HRESULT TestCooperativeLevel();HRESULT Reset(D3DPRESENT_PARAMETERS*);
    HRESULT BeginScene();HRESULT EndScene();HRESULT present();
    HRESULT Clear(DWORD,const D3DRECT*,DWORD,D3DCOLOR,float,DWORD);
    HRESULT GetRenderState(D3DRENDERSTATETYPE,DWORD*);
    HRESULT SetRenderState(D3DRENDERSTATETYPE,DWORD);
    HRESULT GetSamplerState(DWORD,D3DSAMPLERSTATETYPE,DWORD*);
    HRESULT SetSamplerState(DWORD,D3DSAMPLERSTATETYPE,DWORD);
    HRESULT SetTextureStageState(DWORD,D3DTEXTURESTAGESTATETYPE,DWORD);
    HRESULT GetTexture(DWORD,Texture**);HRESULT SetTexture(DWORD,Texture*);
    HRESULT GetRenderTarget(DWORD,Surface**);HRESULT SetRenderTarget(DWORD,Surface*);
    HRESULT GetDepthStencilSurface(Surface**);
    HRESULT GetTransform(D3DTRANSFORMSTATETYPE,D3DMATRIX*);
    HRESULT SetTransform(D3DTRANSFORMSTATETYPE,const D3DMATRIX*);
    HRESULT SetFVF(DWORD);
    HRESULT DrawPrimitiveUP(D3DPRIMITIVETYPE,UINT,const void*,UINT);
    HRESULT CreateIndexBuffer(UINT,DWORD,D3DFORMAT,D3DPOOL,Buffer**,HANDLE*);
    HRESULT CreateVertexBuffer(UINT,DWORD,DWORD,D3DPOOL,Buffer**,HANDLE*);
    HRESULT CreateVertexDeclaration(const D3DVERTEXELEMENT9*,Declaration**);
    HRESULT SetVertexDeclaration(Declaration*);
    HRESULT SetStreamSource(UINT,Buffer*,UINT,UINT);
    HRESULT SetIndices(Buffer*);
    HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE,INT,UINT,UINT,UINT,UINT);
};
struct Factory:Ref {
    HRESULT GetAdapterDisplayMode(UINT,D3DDISPLAYMODE*);
    HRESULT CreateDevice(UINT,D3DDEVTYPE,HWND,DWORD,D3DPRESENT_PARAMETERS*,Device**);
};
Factory* create_factory(UINT);
void stop(); // Cancels producer backpressure before the application joins workers.
}
#define IDirect3D9 kinoko::gpu_legacy::Factory
#define IDirect3DDevice9 kinoko::gpu_legacy::Device
#define IDirect3DSwapChain9 kinoko::gpu_legacy::SwapChain
#define IDirect3DBaseTexture9 kinoko::gpu_legacy::Texture
#define IDirect3DTexture9 kinoko::gpu_legacy::Texture
#define IDirect3DSurface9 kinoko::gpu_legacy::Surface
#define IDirect3DVertexBuffer9 kinoko::gpu_legacy::Buffer
#define IDirect3DIndexBuffer9 kinoko::gpu_legacy::Buffer
#define IDirect3DVertexDeclaration9 kinoko::gpu_legacy::Declaration
#define Direct3DCreate9 kinoko::gpu_legacy::create_factory
#define D3DXCreateTexture kinoko_gpu_create_texture
extern "C" HRESULT WINAPI kinoko_gpu_create_texture(IDirect3DDevice9*,UINT,UINT,UINT,DWORD,D3DFORMAT,D3DPOOL,IDirect3DTexture9**);
#endif
