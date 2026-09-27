#include "kinoko/gpu_renderer.hpp"
#include <cmath>
#include <stdexcept>
namespace kinoko::gpu {
std::array<Vertex,6> expand_quad(const std::array<KinokoSpriteVertex,4>& input) {
    std::array<Vertex,6> output{};
    constexpr unsigned order[]={0,1,2,2,1,3};
    for(size_t i=0;i<6;++i) {
        const auto& v=input[order[i]];
        if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||
           !std::isfinite(v.rhw)||v.rhw<=0||!std::isfinite(v.u)||!std::isfinite(v.v))
            throw std::runtime_error("Invalid screen-space quad vertex");
        output[i]={{v.x,v.y,v.z,v.rhw},
            {float((v.color>>16)&255)/255,float((v.color>>8)&255)/255,
             float(v.color&255)/255,float(v.color>>24)/255},{v.u,v.v}};
    }
    return output;
}
}
