#include "kinoko/gpu_renderer.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
int main() {
    std::array<KinokoSpriteVertex,4> q{};
    for(unsigned i=0;i<4;++i)q[i]={float(i)-.5f,2.5f,.5f,1,0x80402010u,.25f,.75f};
    const auto v=kinoko::gpu::expand_quad(q);
    const unsigned order[]={0,1,2,2,1,3};
    for(unsigned i=0;i<6;++i) {
        if(v[i].position[0]!=q[order[i]].x||v[i].position[1]!=2.5f||v[i].position[3]!=1)return 1;
        if(v[i].color[0]!=64.f/255||v[i].color[1]!=32.f/255||v[i].color[2]!=16.f/255||v[i].color[3]!=128.f/255)return 2;
        if(v[i].uv[0]!=.25f||v[i].uv[1]!=.75f)return 3;
    }
    q[0].rhw=0;
    try{kinoko::gpu::expand_quad(q);return 4;}catch(const std::runtime_error&){}
    q[0].rhw=1;q[0].u=std::numeric_limits<float>::infinity();
    try{kinoko::gpu::expand_quad(q);return 5;}catch(const std::runtime_error&){}
    return 0;
}
