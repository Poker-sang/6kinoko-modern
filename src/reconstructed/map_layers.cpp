#include "kinoko/act_method_dispatch.hpp"
#include "kinoko/map_manager_records.hpp"
#include "kinoko/map_render.h"
#include "kinoko/act_runtime.h"
#include "kinoko/memory_access.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/act_source.h"
#include "kinoko/map_containers.h"
#include "kinoko/act_layer_access.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>

extern "C" {
extern unsigned char kinoko_map_layout_methods_storage, kinoko_map_render_layer_methods_storage;
}

using namespace kinoko::map;

extern "C" KinokoActLayout *kinoko_map_lookup_layout(KinokoMapManager *storage, const char *name) {
    if (!storage) return nullptr;
    const ManagerView manager(storage);
    if (!manager.get(&ManagerRecord::source_act) || !name)
        return 0;

    // 46F140 uses the source holder only for the layer count. 452020 resolves
    // the key from player+16 (the live ACT holder), after BeginStage's clone.
    const int32_t count = kinoko_act_source_layer_count(manager.get(&ManagerRecord::source_holder));
    for (int32_t index = 0; index < count; ++index) {
        auto *borrowed_layout = kinoko_act_layer_layout(manager.get(&ManagerRecord::player), index);
        if (!borrowed_layout) continue;
        const LayoutView layout(borrowed_layout);
        auto *layer = layout.get(&LayoutRecord::owning_layer);
        if (layout.get(&LayoutRecord::methods) != &kinoko_map_layout_methods_storage || !layer) continue;
        const kinoko::legacy::StringView layer_name(LayerView(layer).bytes(&LayerRecord::name));
        if (std::strcmp(layer_name.data(), name) == 0)
            return borrowed_layout;
    }
    return 0;
}


extern "C" KinokoRenderLayer *kinoko_map_make_render_layer(KinokoMapManager *manager,
                                                          const char *name) {
    auto *layout = kinoko_map_lookup_layout(manager, name);
    if (!layout)
        return 0;
    // std::list preserves the returned eight-byte object's address on append.
    return kinoko_map_append_render(manager, layout);
}

namespace {
using kinoko::memory::load;

}

extern "C" kinoko_mcd_data *kinoko_map_layer_chip_data(KinokoActLayout *layout) {
    if (!layout) return nullptr;
    auto *layer = LayoutView(layout).get(&LayoutRecord::owning_layer);
    if (!layer) return nullptr;
    auto *resource = LayerView(layer).get(&LayerRecord::resource);
    if (!resource) return nullptr;
    // Original virtual QueryType; this does not read or populate the cache.
    auto* chip = kinoko::act::ResourceMethods(resource).query(kinoko::act::ResourceKind::chip);
    return chip ? kinoko::act::chip_resource(chip).data.get() : nullptr;
}

extern "C" kinoko_mcd_data *kinoko_map_cached_chip_data(KinokoActLayout *layout) {
    if (!layout) return nullptr;
    auto *resource = LayoutView(layout).get(&LayoutRecord::cached_chip_resource);
    return resource ? kinoko::act::chip_resource(resource).data.get() : nullptr;
}

extern "C" kinoko_mcd_data *kinoko_map_query_chip_data(KinokoActLayout *layout) {
    if (!layout) return nullptr;
    const LayoutView map(layout);
    if (!map.get(&LayoutRecord::cached_chip_resource)) {
        kinoko::act::LayoutMethods(layout).set_layer(map.get(&LayoutRecord::owning_layer));
    }
    // Read again after virtual SetLayer. A non-null cache is not rebound.
    return kinoko_map_cached_chip_data(layout);
}
