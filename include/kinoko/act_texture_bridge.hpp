#pragma once

#include "kinoko/act_resource_records_io.hpp"
#include "kinoko/act_texture_runtime.hpp"

namespace kinoko::act {

inline TextureRuntimeState texture_runtime_state(void* resource) {
    const TextureResourceFields fields(resource);
    TextureRuntimeState state;
    state.methods = fields.get(&TextureResourceRecord::methods);
    state.handle = fields.get(&TextureResourceRecord::texture);
    state.width = fields.get(&TextureResourceRecord::width);
    state.height = fields.get(&TextureResourceRecord::height);
    state.source_x = fields.get(&TextureResourceRecord::source_x);
    state.source_y = fields.get(&TextureResourceRecord::source_y);
    state.source_width = fields.get(&TextureResourceRecord::source_width);
    state.source_height = fields.get(&TextureResourceRecord::source_height);
    state.auto_size = fields.get(&TextureResourceRecord::auto_size);
    return state;
}

}
