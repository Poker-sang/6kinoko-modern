#pragma once
#include "kinoko/render_resources.hpp"
#include "kinoko/graphics_api.hpp"
namespace kinoko::render {
class GraphicsQuadSink final : public QuadSink {
    kinoko::graphics::Device& device_;
public:
    explicit GraphicsQuadSink(kinoko::graphics::Device& device) : device_(device) {}
    int32_t set_layout(SpriteLayout layout) override {
        // 404BC0 uses the distinct original 0x4142 FVF and the same stride.
        return device_.SetFVF(layout==SpriteLayout::screen_rhw ?
            kinoko::graphics::vertex_xyzrhw|kinoko::graphics::vertex_diffuse|kinoko::graphics::vertex_tex1 : 0x4142u);
    }
    int32_t draw_quad(const KinokoSpriteVertex* vertices) override {
        return device_.DrawPrimitiveUP(kinoko::graphics::primitive_trianglestrip,2,vertices,sizeof(*vertices));
    }
};
}
