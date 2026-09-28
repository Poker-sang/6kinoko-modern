#include "kinoko/act_ownership.hpp"
#include "kinoko/act_layer_lifecycle.h"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/act_resource_records_io.hpp"
#include "kinoko/render_target.h"
#include "kinoko/legacy_string.h"
#include "kinoko/native_buffer.h"
#include "kinoko/act_array.h"
#include "kinoko/act_list.h"
#include "kinoko/map_render.h"
#include "kinoko/string_layout.h"
#include "kinoko/squirrel_api_types.h"
// Native C++ continuation of the recovered ACT path. Original function names
// remain C ABI ports until the surrounding decompiled host is migrated.
#include "kinoko/act_runtime.h"
#include "kinoko/act_script_payload.hpp"
#include "kinoko/act_document_records.hpp"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/act_layer_storage.hpp"
#include "kinoko/act_key_records.hpp"
#include "kinoko/act_mesh.hpp"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/memory_access.hpp"
#include "kinoko/sqrat_object_bridge.h"
#include "kinoko/native_property_bridge.h"
#include "kinoko/squirrel_binding.h"
#include "kinoko/squirrel_native_calls.h"
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/squirrel_native_arguments.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_legacy_api.h"
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/squirrel_compile_bridge.h"
#include "kinoko/squirrel_value_bridge.h"
#include "kinoko/actor_methods.h"
#include "kinoko/actor_animation.h"
#include "kinoko/actor_cleanup.h"
#include "kinoko/act_clone.h"
#include "kinoko/act_resource.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_object.h"
#include "kinoko/texture_store.h"
#include "kinoko/map_render.h"
#include "kinoko/sprite.h"
#include "kinoko/game_math.h"
#include <windows.h>
#include "kinoko/graphics_api.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>



extern "C" int32_t __fastcall kinoko_method_delete_act_script(void* script, void *) {
    if (script) {
        kinoko_destroy_cact_script((void*)(uintptr_t)(script));
        std::free(script);
    }
    return 0;
}

namespace {
void clear_layout(KinokoActLayout* layout) {
    if (!layout) return;
    const auto methods = kinoko::memory::load<const void*>(layout);
    if (methods == kinoko_act_host_symbols()->map_layout_vtable) kinoko_clear_map_layout(layout);
}
void clear_key(KinokoActKey* value) {
    if (!value) return;
    using namespace kinoko::act;
    const KeyView key(value);
    key.set(&KeyRecord::methods, kinoko_act_host_symbols()->key_vtable);
    dispose_owned(key.get(&KeyRecord::layout));
    key.set(&KeyRecord::layout, static_cast<KinokoActLayout*>(nullptr));
    auto name = key.view(&KeyRecord::script_name);
    kinoko_string_destroy(name.data());
    *name.bytes(&kinoko::legacy::StringRecord::characters) = 0;
    name.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
    name.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
}
}
void kinoko_destroy_cact_key(void* value) {
    if (!value) return;
    // The layer's second list owns CActTimeLine, not a key with a layout.
    if (kinoko::memory::load<const void*>(value)==kinoko_act_timeline_vtable())
        kinoko_native_buffer_destroy(kinoko::native::RecordView<kinoko::act::TimelineRecord>(value).bytes(&kinoko::act::TimelineRecord::pairs));
    else clear_key(static_cast<KinokoActKey*>(value));
    std::free(value);
}
extern "C" void* __fastcall kinoko_method_destroy_layout(KinokoActLayout* layout,void*) {
    if(layout && kinoko::memory::load<const void*>(layout)==kinoko_string_layout_methods())
        return kinoko_method_destroy_string_layout(reinterpret_cast<KinokoStringLayout*>(layout),nullptr);
    clear_layout(layout);
    std::free(layout);
    return layout;
}

void kinoko_destroy_cact_list(void* list_slot) {
    if (!list_slot) return;
    auto* head = kinoko::memory::load<void*>(list_slot);
    if (!head) return;
    kinoko_act_list_dispose_payloads(head);
    kinoko_act_list_drop_storage(head);
    kinoko::memory::store(list_slot, static_cast<void*>(nullptr));
}

static void clear_resource(KinokoActResource* resource)
{
    if (resource == 0)
        return;
    if(kinoko::memory::load<const void*>(resource)==kinoko::mesh::resource_methods()) {
        kinoko::mesh::clear_resource(reinterpret_cast<kinoko::mesh::Resource*>(resource));return;
    }
    using namespace kinoko::act;
    if (kinoko::memory::load<const void*>(resource) == kinoko_act_host_symbols()->chip_resource_vtable) {
        kinoko::act::chip_resource(resource).clear();
    } else {
        auto& texture = kinoko::act::texture_resource(resource);
        const auto handle = texture.texture;
        const bool borrowed = texture.borrows_texture != 0;
        // 449360 resets the device target before releasing an owned target.
        if (!borrowed && handle && kinoko::memory::load<const void*>(resource) == kinoko_act_host_symbols()->render_target_vtable)
            kinoko_set_render_target(0);
        // Native clones retain a store reference separately from the original
        // borrowed bit. A borrowed handle without that reference is not ours.
        if (const auto owned = texture.take_texture_reference()) kinoko_texture_release(owned);
        texture.clear_names();
    }
}

namespace {
void* delete_resource(KinokoActResource* resource, unsigned char flags) {
    if (!resource) return nullptr;
    using namespace kinoko::act;
    const auto* methods = kinoko::memory::load<const void*>(resource);
    if (methods == kinoko::mesh::resource_methods()) {
        return kinoko::mesh::release_resource(reinterpret_cast<kinoko::mesh::Resource*>(resource), flags);
    }
    const auto clear = [](auto& value) { clear_resource(reinterpret_cast<KinokoActResource*>(&value)); };
    if (methods == kinoko_act_host_symbols()->chip_resource_vtable)
        return destroy_resources(&chip_resource(resource), flags, clear);
    return destroy_resources(&texture_resource(resource), flags, clear);
}
}
void kinoko_destroy_cact_resource(KinokoActResource* resource) {
    delete_resource(resource, 1);
}

void kinoko_destroy_cact_object(KinokoActDocument* object_ptr)
{
    if (!object_ptr) return;
    const kinoko::act::DocumentView document(object_ptr);
    document.set(&kinoko::act::DocumentRecord::vtable,
        kinoko_act_host_symbols()->act_vtable);

    using namespace kinoko::act;
    // 427610 re-reads each end after virtual callbacks. Both payload passes
    // precede vector destruction; resources' vector is destroyed first.
    auto *layer = document.get(&DocumentRecord::layers).begin;
    while (layer != document.get(&DocumentRecord::layers).end) {
        dispose_owned(kinoko::memory::load<KinokoActLayer *>(layer));
        ++layer;
    }
    auto *resource = document.get(&DocumentRecord::resources).begin;
    while (resource != document.get(&DocumentRecord::resources).end) {
        dispose_owned(kinoko::memory::load<KinokoActResource *>(resource));
        ++resource;
    }
    kinoko_act_array_destroy((void*)(document.bytes(&DocumentRecord::resources)));
    kinoko_act_array_destroy((void*)(document.bytes(&DocumentRecord::layers)));

    kinoko_destroy_cact_script((void*)(document.bytes(&kinoko::act::DocumentRecord::script)));
    auto clear_string = [](unsigned char* storage) {
        kinoko_string_destroy(storage);
        const kinoko::native::RecordView<kinoko::legacy::StringRecord> record(storage);
        std::memset(record.bytes(&kinoko::legacy::StringRecord::characters), 0, sizeof(int32_t));
        record.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
        record.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
    };
    clear_string(document.bytes(&kinoko::act::DocumentRecord::resource_path));
    clear_string(document.bytes(&kinoko::act::DocumentRecord::name));
}

namespace {
template<class T, void (*Clear)(T*)>
void* delete_with_flags(T* object, size_t size, unsigned char flags) {
    if (!object) return nullptr;
    auto* bytes = reinterpret_cast<unsigned char*>(object);
    if (flags & 2) {
        auto* allocation = bytes - sizeof(uint32_t);
        const auto count = kinoko::memory::load<uint32_t>(allocation);
        for (auto i = count; i > 0; --i) Clear(reinterpret_cast<T*>(bytes + (i - 1) * size));
        if (flags & 1) std::free(allocation);
        return allocation;
    }
    Clear(object);
    if (flags & 1) std::free(object);
    return object;
}
}
void* kinoko_destroy_cact_with_flags(KinokoActDocument* object, unsigned char flags) {
    return delete_with_flags<KinokoActDocument, kinoko_destroy_cact_object>(object, sizeof(kinoko::act::DocumentRecord), flags);
}
// Original deleting-destructor sizes: 420830=36, 4209B0=348,
// 429720/429780/429840=100. Bit two destroys arrays in reverse order;
// bit one releases the allocation, including its four-byte array cookie.
extern "C" void* __fastcall kinoko_method_delete_act_key(KinokoActKey* object,void*,unsigned char flags) {
    return delete_with_flags<KinokoActKey, clear_key>(object,36,flags);
}
extern "C" void* __fastcall kinoko_method_delete_act_layer(KinokoActLayer* object,void*,unsigned char flags) {
    return delete_with_flags<KinokoActLayer, kinoko_act_layer_clear>(object,sizeof(kinoko::act::LayerStorageRecord),flags);
}
extern "C" void* __fastcall kinoko_method_delete_act_resource(KinokoActResource* object,void*,unsigned char flags) {
    return delete_resource(object, flags);
}


namespace {
void clear_layout_identity(KinokoActLayout* layout) {
    using namespace kinoko::act;
    const kinoko::native::RecordView<Layout2DRecord> record(layout);
    record.set(&Layout2DRecord::methods, kinoko_act_host_symbols()->layout_vtable);
    record.view(&Layout2DRecord::quad).set(&kinoko::render::QuadRecord::vtable, kinoko_act_host_symbols()->color_vtable);
}
}
extern "C" void* __fastcall kinoko_delete_layout_sprite(void* sprite, void*, int32_t flags) {
    // Original secondary this adjustment; array cookie precedes the full layout.
    auto* layout = reinterpret_cast<KinokoActLayout*>(static_cast<unsigned char*>(sprite) - offsetof(kinoko::act::Layout2DRecord, quad));
    return delete_with_flags<KinokoActLayout, clear_layout_identity>(layout, sizeof(kinoko::act::Layout2DRecord), static_cast<unsigned char>(flags));
}
