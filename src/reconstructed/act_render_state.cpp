#include "kinoko/act_layout_render.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/renderer.h"
#include "d3d9_blend_sink.hpp"
void kinoko::act::set_layout_blend(int32_t mode) {
    using namespace kinoko::render;
    if(mode<1 || mode>4) {
        D3D9BlendSink sink(*kinoko_graphics.device);
        sink.set_operation(BlendOperation::add);
        sink.set_source(mode==5?BlendFactor::destination_color:BlendFactor::one);
        sink.set_destination(mode==5?BlendFactor::one:BlendFactor::zero);
        return;
    }
    if(kinoko_renderer.state.blend==mode) return;
    D3D9BlendSink sink(*kinoko_renderer.device);
    // 42AC20 case 32 deliberately omits the central helper's operation write.
    set_blend(kinoko_renderer.state.blend,mode,&sink,BlendPath::act_layout);
}
