#pragma once
// Typed names used by the reconstructed C-linkage entry points. The object
// types are C++ runtime objects, not COM interfaces or a public C ABI.
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
#error "Game graphics consumers require C++"
#endif
