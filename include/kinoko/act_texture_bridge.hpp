#pragma once

#include "kinoko/act_texture_resource.hpp"
#include "kinoko/act_texture_runtime.hpp"

namespace kinoko::act {

inline TextureRuntimeState texture_runtime_state(void* resource) {
    auto& fields = kinoko::act::texture_resource(resource);
    TextureRuntimeState state;
    state.methods = fields.methods;
    state.handle = fields.texture;
    state.width = fields.width;
    state.height = fields.height;
    state.source_x = fields.source_x;
    state.source_y = fields.source_y;
    state.source_width = fields.source_width;
    state.source_height = fields.source_height;
    state.auto_size = fields.auto_size;
    return state;
}

}
