#pragma once
#include "kinoko/render_blend.hpp"
#include "kinoko/sprite_vertex.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
struct SDL_Window;
namespace kinoko::gpu {
using TextureId=uint32_t;
enum class ShaderEncoding { dxbc, spirv, msl };
struct ShaderCode { const void* bytes; size_t size; const char* entrypoint="main"; };
struct Blend {
    bool enabled=true;
    render::BlendOperation operation=render::BlendOperation::add;
    render::BlendFactor source=render::BlendFactor::source_alpha;
    render::BlendFactor destination=render::BlendFactor::inverse_source_alpha;
};
struct Draw {
    TextureId texture=0; // Built-in opaque white; never a native address.
    std::array<KinokoSpriteVertex,4> vertices{};
    Blend blend;
    bool linear=false,wrap_u=false,wrap_v=false;
};
struct Pass {
    TextureId target=0; // Swapchain. Other IDs must be render-target textures.
    bool clear=true;
    std::array<float,4> clear_color{0,0,0,1};
    // Logical coordinate extent. Zero selects target pixel extent.
    uint32_t logical_width=0,logical_height=0;
    std::vector<Draw> draws;
};
// Owner-thread only, including creation, uploads, present and destruction.
// Window must outlive Renderer. Errors throw std::runtime_error. This initial
// backend supports 2D XYZRHW quads only; no implicit mesh/depth emulation.
class Renderer {
public:
    Renderer(SDL_Window*,ShaderEncoding,ShaderCode vertex,ShaderCode fragment,bool debug=false);
    ~Renderer();
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    TextureId create_texture(uint32_t width,uint32_t height,const void* rgba,
                             size_t bytes,size_t pitch,bool render_target=false);
    void destroy_texture(TextureId);
    // Pass and draw order is exact. No sorting of translucent objects.
    // false means minimized/no available swapchain image, not a render failure.
    bool present(const std::vector<Pass>& passes);
    const char* driver() const;
private:
    struct State;
    std::unique_ptr<State> state_;
};
// CPU conversion is shared by the renderer and compile-only contract.
struct Vertex { float position[4],color[4],uv[2]; };
std::array<Vertex,6> expand_quad(const std::array<KinokoSpriteVertex,4>&);
}
