#pragma once
#include <cstdint>
namespace kinoko::graphics {
using Result = std::int32_t;
inline constexpr Result error_out_of_memory = static_cast<Result>(0x8007000eu);
inline constexpr Result error_failure = static_cast<Result>(0x80004005u);
inline constexpr Result error_pointer = static_cast<Result>(0x80004003u);
inline constexpr Result error_argument = static_cast<Result>(0x80070057u);
constexpr bool failed(Result value) noexcept { return value < 0; }
constexpr bool succeeded(Result value) noexcept { return value >= 0; }
struct PixelRect { std::int32_t left, top, right, bottom; };

// State values preserve the original game command vocabulary. These are not
// SDK objects or serialized records. SDL GPU owns these runtime descriptions.
using Color=std::uint32_t;
using Format=std::uint32_t;
using Pool=std::uint32_t;
using DeviceKind=std::uint32_t;
using Primitive=std::uint32_t;
using RenderState=std::uint32_t;
using SamplerState=std::uint32_t;
using TextureStageState=std::uint32_t;
using Transform=std::uint32_t;
struct Capabilities {std::uint32_t TextureCaps=0,MaxTextureWidth=0,MaxTextureHeight=0;};
struct Presentation {
    std::uint32_t BackBufferWidth=0,BackBufferHeight=0; Format BackBufferFormat=0;
    std::uint32_t BackBufferCount=0; std::uint32_t MultiSampleType=0,MultiSampleQuality=0,SwapEffect=0;
    bool Windowed=false,EnableAutoDepthStencil=false;
    Format AutoDepthStencilFormat=0; std::uint32_t Flags=0;
    std::uint32_t FullScreen_RefreshRateInHz=0,PresentationInterval=0;
};
struct DisplayMode {std::uint32_t Width,Height,RefreshRate;graphics::Format Format;};
struct SurfaceDescription {
    graphics::Format Format;std::uint32_t Type;std::uint32_t Usage;graphics::Pool Pool;
    std::uint32_t MultiSampleType;std::uint32_t MultiSampleQuality;std::uint32_t Width,Height;
};
struct MappedPixels {std::int32_t Pitch;void* pBits;};
struct Matrix {float m[4][4];};
struct ClearRect {std::int32_t x1,y1,x2,y2;};
struct VertexElement {std::uint16_t Stream,Offset;std::uint8_t Type,Method,Usage,UsageIndex;};
static_assert(sizeof(Matrix)==64 && sizeof(VertexElement)==8);
inline constexpr std::uint32_t adapter_default=static_cast<std::uint32_t>(0x0u);
inline constexpr std::uint32_t blend_operation_add=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t blend_operation_revsubtract=static_cast<std::uint32_t>(0x3u);
inline constexpr std::uint32_t blend_destcolor=static_cast<std::uint32_t>(0x9u);
inline constexpr std::uint32_t blend_invsrcalpha=static_cast<std::uint32_t>(0x6u);
inline constexpr std::uint32_t blend_one=static_cast<std::uint32_t>(0x2u);
inline constexpr std::uint32_t blend_srcalpha=static_cast<std::uint32_t>(0x5u);
inline constexpr std::uint32_t blend_srccolor=static_cast<std::uint32_t>(0x3u);
inline constexpr std::uint32_t blend_zero=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t clear_stencil=static_cast<std::uint32_t>(0x4u);
inline constexpr std::uint32_t clear_target=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t clear_zbuffer=static_cast<std::uint32_t>(0x2u);
inline constexpr std::uint32_t compare_always=static_cast<std::uint32_t>(0x8u);
inline constexpr std::uint32_t compare_lessequal=static_cast<std::uint32_t>(0x4u);
inline constexpr std::uint32_t creation_hardware_vertexprocessing=static_cast<std::uint32_t>(0x40u);
inline constexpr std::uint32_t creation_multithreaded=static_cast<std::uint32_t>(0x4u);
inline constexpr std::uint32_t creation_software_vertexprocessing=static_cast<std::uint32_t>(0x20u);
inline constexpr std::uint32_t cull_ccw=static_cast<std::uint32_t>(0x3u);
inline constexpr std::uint32_t cull_cw=static_cast<std::uint32_t>(0x2u);
inline constexpr std::uint32_t cull_none=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t declaration_method_default=static_cast<std::uint32_t>(0x0u);
inline constexpr std::uint32_t element_float2=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t element_float3=static_cast<std::uint32_t>(0x2u);
inline constexpr std::uint32_t semantic_normal=static_cast<std::uint32_t>(0x3u);
inline constexpr std::uint32_t semantic_position=static_cast<std::uint32_t>(0x0u);
inline constexpr std::uint32_t semantic_texcoord=static_cast<std::uint32_t>(0x5u);
inline constexpr std::uint32_t device_hal=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t device_ref=static_cast<std::uint32_t>(0x2u);
inline constexpr kinoko::graphics::Result error_devicelost=static_cast<kinoko::graphics::Result>(0x88760868u);
inline constexpr kinoko::graphics::Result error_devicenotreset=static_cast<kinoko::graphics::Result>(0x88760869u);
inline constexpr kinoko::graphics::Result error_invalidcall=static_cast<kinoko::graphics::Result>(0x8876086cu);
inline constexpr kinoko::graphics::Result error_wasstilldrawing=static_cast<kinoko::graphics::Result>(0x8876021cu);
inline constexpr std::uint32_t fill_solid=static_cast<std::uint32_t>(0x3u);
inline constexpr std::uint32_t format_a1r5g5b5=static_cast<std::uint32_t>(0x19u);
inline constexpr std::uint32_t format_a8r8g8b8=static_cast<std::uint32_t>(0x15u);
inline constexpr std::uint32_t format_d24s8=static_cast<std::uint32_t>(0x4bu);
inline constexpr std::uint32_t format_index16=static_cast<std::uint32_t>(0x65u);
inline constexpr std::uint32_t format_x8r8g8b8=static_cast<std::uint32_t>(0x16u);
inline constexpr std::uint32_t vertex_diffuse=static_cast<std::uint32_t>(0x40u);
inline constexpr std::uint32_t vertex_normal=static_cast<std::uint32_t>(0x10u);
inline constexpr std::uint32_t vertex_tex1=static_cast<std::uint32_t>(0x100u);
inline constexpr std::uint32_t vertex_xyz=static_cast<std::uint32_t>(0x2u);
inline constexpr std::uint32_t vertex_xyzrhw=static_cast<std::uint32_t>(0x4u);
inline constexpr std::uint32_t multisample_none=static_cast<std::uint32_t>(0x0u);
inline constexpr std::uint32_t pool_default=static_cast<std::uint32_t>(0x0u);
inline constexpr std::uint32_t pool_managed=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t presentation_flag_discard_depthstencil=static_cast<std::uint32_t>(0x2u);
inline constexpr std::uint32_t presentation_donotwait=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t presentation_interval_one=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t texture_caps_squareonly=static_cast<std::uint32_t>(0x20u);
inline constexpr std::uint32_t primitive_trianglelist=static_cast<std::uint32_t>(0x4u);
inline constexpr std::uint32_t primitive_trianglestrip=static_cast<std::uint32_t>(0x5u);
inline constexpr std::uint32_t state_alphablendenable=static_cast<std::uint32_t>(0x1bu);
inline constexpr std::uint32_t state_alphafunc=static_cast<std::uint32_t>(0x19u);
inline constexpr std::uint32_t state_alpharef=static_cast<std::uint32_t>(0x18u);
inline constexpr std::uint32_t state_alphatestenable=static_cast<std::uint32_t>(0xfu);
inline constexpr std::uint32_t state_blendop=static_cast<std::uint32_t>(0xabu);
inline constexpr std::uint32_t state_cullmode=static_cast<std::uint32_t>(0x16u);
inline constexpr std::uint32_t state_destblend=static_cast<std::uint32_t>(0x14u);
inline constexpr std::uint32_t state_fillmode=static_cast<std::uint32_t>(0x8u);
inline constexpr std::uint32_t state_lighting=static_cast<std::uint32_t>(0x89u);
inline constexpr std::uint32_t state_srcblend=static_cast<std::uint32_t>(0x13u);
inline constexpr std::uint32_t state_stencilmask=static_cast<std::uint32_t>(0x3au);
inline constexpr std::uint32_t state_zenable=static_cast<std::uint32_t>(0x7u);
inline constexpr std::uint32_t state_zfunc=static_cast<std::uint32_t>(0x17u);
inline constexpr std::uint32_t state_zwriteenable=static_cast<std::uint32_t>(0xeu);
inline constexpr std::uint32_t resource_surface=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t sampler_addressu=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t sampler_addressv=static_cast<std::uint32_t>(0x2u);
inline constexpr std::uint32_t sampler_magfilter=static_cast<std::uint32_t>(0x5u);
inline constexpr std::uint32_t sampler_minfilter=static_cast<std::uint32_t>(0x6u);
inline constexpr std::uint32_t sampler_mipfilter=static_cast<std::uint32_t>(0x7u);
inline constexpr std::uint32_t swap_discard=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t address_clamp=static_cast<std::uint32_t>(0x3u);
inline constexpr std::uint32_t address_wrap=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t filter_linear=static_cast<std::uint32_t>(0x2u);
inline constexpr std::uint32_t filter_point=static_cast<std::uint32_t>(0x1u);
inline constexpr std::uint32_t texture_operation_modulate=static_cast<std::uint32_t>(0x4u);
inline constexpr std::uint32_t texture_stage_alphaop=static_cast<std::uint32_t>(0x4u);
inline constexpr std::uint32_t transform_projection=static_cast<std::uint32_t>(0x3u);
inline constexpr std::uint32_t transform_view=static_cast<std::uint32_t>(0x2u);
inline constexpr std::uint32_t transform_world=static_cast<std::uint32_t>(0x100u);
inline constexpr std::uint32_t usage_rendertarget=static_cast<std::uint32_t>(0x1u);
inline constexpr kinoko::graphics::Result ok=static_cast<kinoko::graphics::Result>(0x0u);
inline constexpr std::uint32_t sdk_version=static_cast<std::uint32_t>(0x20u);
constexpr VertexElement declaration_end(){return {0xff,0,17,0,0,0};}
}
