#pragma once
// C-facing declarations shared with the original C contract fixtures. The
// modern game is C++; its backend types must never be interpreted as COM by C.
#ifdef __cplusplus
#include "kinoko/graphics_api.hpp"
typedef kinoko::graphics::Factory KinokoGraphicsFactory;
typedef kinoko::graphics::Device KinokoGraphicsDevice;
typedef kinoko::graphics::SwapChain KinokoGraphicsSwapChain;
typedef kinoko::graphics::BaseTexture KinokoGraphicsBaseTexture;
typedef kinoko::graphics::Texture KinokoGraphicsTexture;
typedef kinoko::graphics::Surface KinokoGraphicsSurface;
typedef kinoko::graphics::Capabilities KinokoGraphicsCapabilities;
typedef kinoko::graphics::Presentation KinokoGraphicsPresentation;
typedef kinoko::graphics::DisplayMode KinokoGraphicsDisplayMode;
#else
#ifdef KINOKO_GPU_GAME
#error "Modern graphics consumers require C++"
#endif
#include <d3d9.h>
typedef IDirect3D9 KinokoGraphicsFactory;
typedef IDirect3DDevice9 KinokoGraphicsDevice;
typedef IDirect3DSwapChain9 KinokoGraphicsSwapChain;
typedef IDirect3DBaseTexture9 KinokoGraphicsBaseTexture;
typedef IDirect3DTexture9 KinokoGraphicsTexture;
typedef IDirect3DSurface9 KinokoGraphicsSurface;
typedef D3DCAPS9 KinokoGraphicsCapabilities;
typedef D3DPRESENT_PARAMETERS KinokoGraphicsPresentation;
typedef D3DDISPLAYMODE KinokoGraphicsDisplayMode;
#endif
