#pragma once
#include <array>
#include <cstdint>
namespace kinoko::render {
class TextureBindingSink {
public:
    virtual ~TextureBindingSink() = default;
    // Nonzero handles must already be validated against the resource registry.
    virtual int32_t bind(uint32_t stage,int32_t handle)=0;
};
class TextureBindings {
    std::array<int32_t,8> handles_{};
public:
    void clear() { handles_.fill(0); }
    void forget(int32_t handle) { for(auto& v:handles_) if(v==handle) v=0; }
    // Caller validates stage and resource existence before entering this cache.
    int32_t bind(TextureBindingSink& sink,uint32_t stage,int32_t handle) {
        auto& cached=handles_[stage];
        if(!handle) { sink.bind(stage,0); cached=0; return 0; }
        if(handle==cached) return handle;
        const auto result=sink.bind(stage,handle);
        cached=handle; // Failed native calls are cached too, as in 405E94.
        return result;
    }
};
}
