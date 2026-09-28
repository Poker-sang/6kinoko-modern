#include "kinoko/string_runtime.hpp"
#include "kinoko/act_method_dispatch.hpp"
#include "kinoko/act_texture_resource.hpp"
#include "sqpcheader.h"
#include "sqtable.h"
#include "sqclass.h"
#include "kinoko/act_resource_records_io.hpp"
#include "kinoko/act_key_records.hpp"
#include "kinoko/act_layer_lifecycle.h"
#include "kinoko/act_ownership.hpp"
#include "kinoko/act_document.h"
#include "kinoko/act_layer_access.h"
#include "kinoko/act_resource_records.hpp"
#include "kinoko/stage_records.hpp"
#include "kinoko/windows_owner.hpp"
#include "kinoko/map_chip_cache.hpp"
#include "kinoko/act_mesh.hpp"
#include "kinoko/act_array.hpp"
#include "kinoko/act_layer_storage.hpp"
#include "kinoko/act_layer_lifecycle.h"
#include "kinoko/string_layout.h"
#include "kinoko/squirrel_pair.hpp"
#include "kinoko/script_diagnostics.hpp"
// Recovered ACT behavior with typed C++ storage and explicit script boundaries.
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/memory_access.hpp"
#include "kinoko/legacy_string.hpp"
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
#include "kinoko/map_layout_records.hpp"
#include "kinoko/sprite.h"
#include "kinoko/game_math.h"
#include "kinoko/method_entry.hpp"
#include "kinoko/upstream_bindings.hpp"
#include "kinoko/squirrel_host_object.hpp"
#include <windows.h>

#include <mmsystem.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>



namespace {
const int32_t* pair_bytes(const int32_t* p) { return p; }
int32_t* pair_bytes(int32_t* p) { return p; }
template<size_t N> const int32_t* pair_bytes(const std::array<int32_t,N>& p) { return p.data(); }
template<size_t N> int32_t* pair_bytes(std::array<int32_t,N>& p) { return p.data(); }
template<class T> intptr_t pair_data(const T& p) { return kinoko::script::data_bits(kinoko::script::pair::read(pair_bytes(p))); }
template<class D,class S> void copy_pair(D& out,const S& in) { kinoko::script::pair::write(pair_bytes(out),kinoko::script::pair::read(pair_bytes(in))); }
kinoko::act::ScriptValueStorage pair_storage(const int32_t* p) { kinoko::act::ScriptValueStorage out{}; copy_pair(out,p); return out; }
SQRESULT get_game_integer(SQVM* vm,SQInteger index,int32_t* out) {
    SQInteger value{}; const auto result=sq_getinteger(vm,index,&value);
    if(SQ_SUCCEEDED(result)) *out=static_cast<int32_t>(value);
    return result;
}
}

namespace {
// Byte-backed views of the fields consumed by ACT publication. The source
// records remain owned by the ACT loader; these views never copy or destroy
// their embedded strings or pointer vectors. Offsets match 450950/450F30.
using ActPublicationRecord = kinoko::act::DocumentRecord;
struct ResourcePublicationRecord {
    const void* vtable;
    int32_t id;
    kinoko::legacy::StringRecord name;
};
static_assert(offsetof(ResourcePublicationRecord,name)==offsetof(kinoko::act::TextureResource,name));
struct LayerPublicationRecord {
    alignas(void*) unsigned char unknown0[sizeof(kinoko::act::LayerAssociationRecord)];
    kinoko::legacy::StringRecord name;
};
static_assert(offsetof(LayerPublicationRecord, name) == offsetof(kinoko::act::LayerStorageRecord,name));
struct ScriptPublicationRecord {
    alignas(void*) unsigned char unknown0[offsetof(kinoko::act::ScriptStorageRecord,file_name)];
    kinoko::legacy::StringRecord path;
};
static_assert(offsetof(ScriptPublicationRecord,path)==offsetof(kinoko::act::ScriptStorageRecord,file_name));
using ActPublicationView = kinoko::native::RecordView<ActPublicationRecord>;
using ResourcePublicationView = kinoko::native::RecordView<ResourcePublicationRecord>;
using LayerPublicationView = kinoko::native::RecordView<LayerPublicationRecord>;
using ScriptPublicationView = kinoko::native::RecordView<ScriptPublicationRecord>;

// The original class property tables live in pairs of global SQObject slots.
// Assignment adds a reference and releases the previous value. The VM owns
// temporary handles; these slots retain the class-level setter/getter tables.
struct PublishedPropertyTables {
    int32_t* setters;
    int32_t* getters;
    bool valid() const {
        return setters[0] == 0x0A000020 && pair_data(setters) != 0 &&
               getters[0] == 0x0A000020 && pair_data(getters) != 0;
    }
    bool ensure(SQVM* vm) const {
        if (valid()) return true;
        int32_t first[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
        int32_t second[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
        const bool created = kinoko_sqrat_new_table(vm, first) &&
                             kinoko_sqrat_new_table(vm, second);
        if (created) {
            kinoko_sqrat_assign_pair(vm, setters, first);
            kinoko_sqrat_assign_pair(vm, getters, second);
        }
        kinoko_sqrat_release_pair(vm, first);
        kinoko_sqrat_release_pair(vm, second);
        return created;
    }
    void clear(SQVM* vm) const {
        const int32_t empty[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
        kinoko_sqrat_assign_pair(vm, setters, empty);
        kinoko_sqrat_assign_pair(vm, getters, empty);
    }
    bool add(SQVM* vm, const char* name, int32_t offset, void* getter, void* setter) const {
        return kinoko_sqrat_set_offset_closure(vm, getters, name, offset, getter) &&
               kinoko_sqrat_set_offset_closure(vm, setters, name, offset, setter);
    }
};
PublishedPropertyTables layer_property_tables() { return {kinoko_layer_set_pair, kinoko_layer_get_pair}; }
PublishedPropertyTables layout_property_tables() { return {kinoko_layout_set_pair, kinoko_layout_get_pair}; }
int32_t* published_layout_class() { return kinoko_layout_class_pair; }

// Original 41E2C0/41E260 dispatch the native offset property accessors;
// 431650 is the original class type callback used by both registrations.
bool initialize_offset_property_class(SQVM* vm, const int32_t* klass,
                                      const int32_t* setters, const int32_t* getters,
                                      void* constructor, void* type_callback) {
    return kinoko_sqrat_initialize_class(vm, klass, setters, getters,
        constructor, reinterpret_cast<void*>(kinoko_native_property_get_callback),
        reinterpret_cast<void*>(kinoko_native_property_set_callback), type_callback) != 0;
}
bool initialize_native_property_class(SQVM* vm, const int32_t* klass,
                                      PublishedPropertyTables tables) {
    return initialize_offset_property_class(vm, klass, tables.setters, tables.getters,
        reinterpret_cast<void*>(kinoko_sqrat_no_constructor),
        reinterpret_cast<void*>(kinoko_native_class_weakref_callback));
}

// Typed wrapper around the existing source-backed Sqrat C boundary.
int32_t get_pair(void* object, const char* name, int32_t* output) {
    return kinoko_sqrat_get(object, name, (void *)(output));
}
}

int32_t kinoko_publish_cact_layer_property(
    SQVM* vm, const char *name, int32_t offset,
    void* getter, void* setter)
{
    const auto tables = layer_property_tables();
    return tables.valid() && tables.add(vm, name, offset,
                                        getter, setter);
}

int32_t kinoko_publish_cact_layer_members(
    SQVM* vm, const int32_t *class_pair)
{
    static const char *const direct_int_names[] = {
        "resourceID", "layerID", "parentID"
    };
    static const int32_t direct_int_offsets[] = {
        offsetof(kinoko::act::LayerAssociationRecord, resource_id),
        offsetof(kinoko::act::LayerAssociationRecord, layer_id),
        offsetof(kinoko::act::LayerAssociationRecord, parent_id)
    };
    static const char *const direct_float_names[] = {
        "dst_x", "dst_y", "dst_z", "x", "y", "z",
        "prev_x", "prev_y", "prev_z", "xPrev", "yPrev", "zPrev",
        "ox", "oy", "oz"
    };
    constexpr auto layer_position = offsetof(kinoko::act::LayerStorageRecord, position);
    constexpr auto layer_previous = offsetof(kinoko::act::LayerStorageRecord, previous_position);
    // 41E790's ox/oy/oz fields; lifecycle preserves their raw float bit patterns.
    constexpr auto layer_origin = offsetof(kinoko::act::LayerStorageRecord, origin_bits);
    static const int32_t direct_float_offsets[] = {
        layer_position, layer_position+4, layer_position+8,
        layer_position, layer_position+4, layer_position+8,
        layer_previous, layer_previous+4, layer_previous+8,
        layer_previous, layer_previous+4, layer_previous+8,
        layer_origin, layer_origin+4, layer_origin+8
    };
    static const char *const pointer_float_names[] = {
        "roll_x", "roll_y", "roll_z", "cor_x", "cor_y", "cor_z",
        "scale_x", "scale_y", "scale_z", "cos_x", "cos_y", "cos_z",
        "alpha"
    };
    static const int32_t pointer_float_offsets[] = {
        
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, rotation_x),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, rotation_y),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, rotation_z),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, rotation_pivot_x),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, rotation_pivot_y),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, rotation_pivot_z),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, scale_x),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, scale_y),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, scale_z),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, scale_pivot_x),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, scale_pivot_y),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, scale_pivot_z),
        offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, alpha)
    };
    static const char *const pointer_int_names[] = {
        "blend", "colorR", "colorG", "colorB"
    };
    static const int32_t pointer_int_offsets[] = { offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, blend), offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, red), offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, green), offsetof(kinoko::act::LayerAssociationRecord, property_aliases) + offsetof(kinoko::act::LayerPropertyAliases, blue) };
    static const char *const object_names[] = {
        "script", "layout", "resource"
    };
    const auto tables = layer_property_tables();
    int32_t null_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    size_t index;

    if (vm == 0 || class_pair == nullptr ||
        class_pair[0] != 0x08004000 || pair_data(class_pair) == 0)
        return 0;
    if (!tables.ensure(vm)) return 0;

    const bool published = [&]() {
        if (!initialize_native_property_class(vm, class_pair, tables))
            return false;

        if (!kinoko_publish_cact_layer_property(vm, "stName", offsetof(kinoko::act::LayerStorageRecord, name), reinterpret_cast<void*>(kinoko_cact_layer_get_string), reinterpret_cast<void*>(kinoko_cact_layer_set_string)))
            return false;
        for (index = 0; index < sizeof(direct_int_names) /
                             sizeof(direct_int_names[0]); ++index) {
            if (!kinoko_publish_cact_layer_property(vm, direct_int_names[index], direct_int_offsets[index], reinterpret_cast<void*>(kinoko_cact_layer_get_int), reinterpret_cast<void*>(kinoko_cact_layer_set_int)))
                return false;
        }
        if (!kinoko_publish_cact_layer_property(vm, "visible", offsetof(kinoko::act::LayerStorageRecord, visibility_flags), reinterpret_cast<void*>(kinoko_cact_layer_get_bool), reinterpret_cast<void*>(kinoko_cact_layer_set_bool)) ||
            !kinoko_publish_cact_layer_property(vm, "debugOnly", offsetof(kinoko::act::LayerStorageRecord, visibility_flags)+1, reinterpret_cast<void*>(kinoko_cact_layer_get_bool), reinterpret_cast<void*>(kinoko_cact_layer_set_bool)))
            return false;
        for (index = 0; index < sizeof(direct_float_names) /
                             sizeof(direct_float_names[0]); ++index) {
            if (!kinoko_publish_cact_layer_property(vm, direct_float_names[index], direct_float_offsets[index], reinterpret_cast<void*>(kinoko_cact_layer_get_float), reinterpret_cast<void*>(kinoko_cact_layer_set_float)))
                return false;
        }
        for (index = 0; index < sizeof(pointer_float_names) /
                             sizeof(pointer_float_names[0]); ++index) {
            if (!kinoko_publish_cact_layer_property(vm, pointer_float_names[index], pointer_float_offsets[index], reinterpret_cast<void*>(kinoko_cact_layer_get_pointer_float), reinterpret_cast<void*>(kinoko_cact_layer_set_pointer_float)))
                return false;
        }
        for (index = 0; index < sizeof(pointer_int_names) /
                             sizeof(pointer_int_names[0]); ++index) {
            if (!kinoko_publish_cact_layer_property(vm, pointer_int_names[index], pointer_int_offsets[index], reinterpret_cast<void*>(kinoko_cact_layer_get_pointer_int), reinterpret_cast<void*>(kinoko_cact_layer_set_pointer_int)))
                return false;
        }

        /* These are the three SQObject members installed by the original
           TypePropertyClass<string/object> helpers.  Their values are filled on
           each instance with the same raw/newslot distinction as the ACT path. */
        for (index = 0; index < sizeof(object_names) / sizeof(object_names[0]);
             ++index) {
            if (!kinoko_sqrat_set_pair(vm, class_pair, object_names[index], null_pair))
                return false;
        }
        return true;
    }();
    if (!published) tables.clear(vm);
    return published;
}

int32_t kinoko_publish_c2dlayout_properties(
    SQVM* vm, const int32_t class_pair[sizeof(HSQOBJECT)/sizeof(int32_t)])
{
    static const char *const float_names[] = {
        "roll_x", "roll_y", "roll_z", "cor_x", "cor_y", "cor_z",
        "scale_x", "scale_y", "scale_z", "cos_x", "cos_y", "coS_z",
        "alpha"
    };
    static const int32_t float_offsets[] = {
        offsetof(kinoko::act::Layout2DRecord,rotation)+0, offsetof(kinoko::act::Layout2DRecord,rotation)+4, offsetof(kinoko::act::Layout2DRecord,rotation)+8, offsetof(kinoko::act::Layout2DRecord,rotation)+12, offsetof(kinoko::act::Layout2DRecord,rotation)+16, offsetof(kinoko::act::Layout2DRecord,rotation)+20, offsetof(kinoko::act::Layout2DRecord,rotation)+24, offsetof(kinoko::act::Layout2DRecord,rotation)+28, offsetof(kinoko::act::Layout2DRecord,rotation)+32, offsetof(kinoko::act::Layout2DRecord,rotation)+36, offsetof(kinoko::act::Layout2DRecord,rotation)+40, offsetof(kinoko::act::Layout2DRecord,rotation)+44, offsetof(kinoko::act::Layout2DRecord,rotation)+48
    };
    static const char *const color_names[] = { "colorR", "colorG", "colorB" };
    static const int32_t color_offsets[] = { offsetof(kinoko::act::Layout2DRecord,red), offsetof(kinoko::act::Layout2DRecord,green), offsetof(kinoko::act::Layout2DRecord,blue) };
    const auto tables = layout_property_tables();
    size_t index;

    if (vm == 0 || class_pair == nullptr ||
        class_pair[0] != 0x08004000 || pair_data(class_pair) == 0)
        return 0;
    if (!tables.ensure(vm)) return 0;

    const bool published = [&]() {
        if (!initialize_native_property_class(vm, class_pair, tables))
            return false;

        for (index = 0; index < sizeof(float_names) / sizeof(float_names[0]);
             ++index) {
            if (!tables.add(vm, float_names[index], float_offsets[index], reinterpret_cast<void*>(kinoko_c2dlayout_get_float), reinterpret_cast<void*>(kinoko_c2dlayout_set_float)))
                return false;
        }
        if (!tables.add(vm, "blend", offsetof(kinoko::act::Layout2DRecord,blend), reinterpret_cast<void*>(kinoko_c2dlayout_get_int), reinterpret_cast<void*>(kinoko_c2dlayout_set_int)))
            return false;
        for (index = 0; index < sizeof(color_names) / sizeof(color_names[0]);
             ++index) {
            if (!tables.add(vm, color_names[index], color_offsets[index], reinterpret_cast<void*>(kinoko_c2dlayout_get_int), reinterpret_cast<void*>(kinoko_c2dlayout_set_color)))
                return false;
        }
        return true;
    }();
    if (!published) tables.clear(vm);
    return published;
}

int32_t kinoko_publish_c2dlayout_class(SQVM* vm, void* root_object)
{
    int32_t existing[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t class_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t empty_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t existing_result;
    int32_t base;

    if (vm == 0 || root_object == 0)
        return 0;
    existing_result = get_pair((void*)(uintptr_t)(root_object), "C2DLayout", existing);
    if (existing_result && existing[0] == 0x08004000 && pair_data(existing) != 0) {
        kinoko_sqrat_assign_pair(vm, published_layout_class(), existing);
        copy_pair(class_pair, existing);
        kinoko_sqrat_release_pair(vm, existing);
        if (layout_property_tables().valid())
            return 1;
        return kinoko_publish_c2dlayout_properties(vm, class_pair);
    }
    if (existing_result)
        kinoko_sqrat_release_pair(vm, existing);

    base = sq_gettop(vm);
    if (!kinoko_sqrat_new_class(vm, class_pair) || sq_gettop(vm) <= base) {
        kinoko_sqrat_trim_stack(vm, base);
        return 0;
    }
    if (class_pair[0] != 0x08004000 || pair_data(class_pair) == 0) {
        kinoko_sqrat_release_pair(vm, class_pair);
        kinoko_sqrat_trim_stack(vm, base);
        return 0;
    }

    kinoko_sqrat_assign_pair(vm, published_layout_class(), class_pair);
    bool published = kinoko_publish_c2dlayout_properties(vm, class_pair) != 0;
    if (published) {
        published = kinoko_sqrat_set_pair(vm,
            reinterpret_cast<const int32_t*>(kinoko::act::LayerObjectView(root_object).bytes(&kinoko::act::LayerObjectRecord::value)), "C2DLayout", class_pair) != 0;
        if (!published)
            kinoko_sqrat_assign_pair(vm, published_layout_class(), empty_pair);
    }
    if (published) kinoko_trace_i32("act:c2dlayout-class", pair_data(class_pair));
    kinoko_sqrat_release_pair(vm, class_pair);
    kinoko_sqrat_trim_stack(vm, base);
    return published;
}

namespace {
template<class Publish>
int32_t register_root_class(SQVM* machine, Publish publish) {
    if (!machine) return static_cast<int32_t>(E_INVALIDARG);
    kinoko::act::LayerObjectRecord root{};
    if (!kinoko_sqrat_root_construct(&root, machine)) return static_cast<int32_t>(E_FAIL);
    SQVM* const vm = machine;
    const bool published = publish(vm, &root);
    kinoko_sqrat_object_release(&root);
    return published ? 0 : static_cast<int32_t>(E_FAIL);
}
} // namespace

// Original 42B6D0 is a cdecl VM-only registration entry. The root-table
// registry used by the active binding owns the same class/property handles;
// do not create another decompiled map of Sqrat objects for this old caller.
extern "C" int32_t kinoko_register_c2dlayout_class(SQVM* machine) {
    return register_root_class(machine, [](SQVM* vm, void* root) {
        return kinoko_publish_c2dlayout_class(vm, (void*)(uintptr_t)(root));
    });
}


intptr_t kinoko_cact_associate_resource(SQVM* vm)
{
    KinokoActLayer* layer = nullptr; KinokoActResource* resource = nullptr;
    int32_t result = (int32_t)E_FAIL;
    if (sq_getinstanceup(vm, 1, (SQUserPointer*)(&layer), nullptr) >= 0 && layer != 0 &&
        sq_getinstanceup(vm, 2, (SQUserPointer*)(&resource), nullptr) >= 0 && resource != 0) {
        /* 424210 -> 4252E0 -> 41EF20: change only the resource and its ID. */
        const kinoko::native::RecordView<kinoko::act::LayerAssociationRecord> fields(layer);
        fields.set(&kinoko::act::LayerAssociationRecord::resource, resource);
        fields.set(&kinoko::act::LayerAssociationRecord::resource_id, ResourcePublicationView(resource).get(&ResourcePublicationRecord::id));
        kinoko::script::diagnostic_name("act:associate-layer", kinoko_string_data(LayerPublicationView(layer).bytes(&LayerPublicationRecord::name)));
        kinoko::script::diagnostic_name("act:associate-resource", kinoko_string_data(ResourcePublicationView(resource).bytes(&ResourcePublicationRecord::name)));
        result = 0;
    }
    sq_pushinteger(vm, result);
    return 1;
}

intptr_t kinoko_resource_load_texture(SQVM* vm) {
    KinokoActResource* resource = nullptr;
    const SQChar *prefix = nullptr;
    if (SQ_FAILED(sq_getinstanceup(vm, 1, reinterpret_cast<SQUserPointer *>(&resource), nullptr)) ||
        !resource || sq_gettop(vm) < 2) return 0;
    if (sq_gettype(vm, 2) != OT_NULL &&
        SQ_FAILED(sq_getstring(vm, 2, &prefix))) return 0;
    // 44FD20 forwards through virtual slot +40, retaining derived behavior.
    const int32_t result = kinoko::act::ResourceMethods(resource).load(prefix);
    sq_pushbool(vm, (result & 0xff) != 0);
    return 1;
}

int32_t kinoko_publish_texture_resource_class(SQVM* vm, void* root,
    const char *name, int32_t out[sizeof(HSQOBJECT)/sizeof(int32_t)]) {
    using R = kinoko::act::TextureResource;
    static const kinoko_native_view_property properties[] = {
        {"resourceID", offsetof(R, id), 0}, {"stName", offsetof(R, name), 4},
        {"image_width", offsetof(R, width), 0}, {"image_height", offsetof(R, height), 0},
        {"src_x", offsetof(R, source_x), 1}, {"src_y", offsetof(R, source_y), 1},
        {"src_width", offsetof(R, source_width), 1}, {"src_height", offsetof(R, source_height), 1}
    };
    if (get_pair((void*)(uintptr_t)(root), name, out) && out[0] == 0x08004000) return 1;
    kinoko_sqrat_release_pair(vm, out);
    return kinoko_publish_map_view_class(vm, (void*)(uintptr_t)(root), name, properties, sizeof(properties) / sizeof(properties[0]), 0, out) &&
        kinoko_sqrat_set_native_closure(vm, out, "LoadTexture", reinterpret_cast<void*>(kinoko_resource_load_texture), nullptr, 0);
}

int32_t kinoko_publish_cact_resource2d_class(SQVM* vm, void* root) {
    if (!vm || !root) return 0;
    int32_t klass[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    const auto ok = kinoko_publish_texture_resource_class(vm, (void*)(uintptr_t)(root), "CActResource2D", klass);
    if (ok) {
        kinoko_sqrat_assign_pair(vm, kinoko_resource2d_class_pair, klass);
        kinoko_resource2d_class_published = 1;
    }
    kinoko_sqrat_release_pair(vm, klass);
    return ok;
}

extern "C" int32_t kinoko_register_texture_resource_class(SQVM* machine) {
    return register_root_class(machine, [](SQVM* vm, void* root) {
        return kinoko_publish_cact_resource2d_class(vm, (void*)(uintptr_t)(root));
    });
}


extern "C" int32_t kinoko_register_render_target_class(SQVM* machine) {
    return register_root_class(machine, [](SQVM* vm, void* root) {
        int32_t klass[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
        const auto ok = kinoko_publish_texture_resource_class(vm, (void*)(uintptr_t)(root), "CActRenderTarget", klass);
        kinoko_sqrat_release_pair(vm, klass);
        return ok;
    });
}


extern "C" int32_t __fastcall kinoko_method_register_texture_resource(void*, void *, SQVM* vm) {
    return kinoko_register_texture_resource_class(vm);
}
extern "C" int32_t __fastcall kinoko_method_register_render_target(void*, void *, SQVM* vm) {
    return kinoko_register_render_target_class(vm);
}

int32_t kinoko_publish_cact_layer_class(SQVM* vm, void* root_object)
{
    int32_t existing[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t class_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    kinoko::act::LayerObjectRecord class_wrapper{kinoko_act_host_symbols()->sq_object_vtable, (SQVM*)(uintptr_t)(0), {static_cast<int32_t>(OT_NULL), 0}, 1, {}};
    void* method_source[2] = {reinterpret_cast<void*>(kinoko_act_layer_associate_method), nullptr};
    int32_t existing_result;
    int32_t base;

    if (vm == 0 || root_object == 0)
        return 0;
    existing_result = get_pair((void*)(uintptr_t)(root_object), "CActLayer", existing);
    if (existing_result) {
        int32_t already_published =
            existing[0] == 0x08004000 && pair_data(existing) != 0;
        kinoko_sqrat_release_pair(vm, existing);
        if (already_published)
            return 1;
    }

    base = sq_gettop(vm);
    if (!kinoko_sqrat_new_class(vm, class_pair) || sq_gettop(vm) <= base) {
        kinoko_sqrat_trim_stack(vm, base);
        return 0;
    }
    if (class_pair[0] != 0x08004000 || pair_data(class_pair) == 0) {
        kinoko_sqrat_release_pair(vm, class_pair);
        kinoko_sqrat_trim_stack(vm, base);
        return 0;
    }

    class_wrapper.vm = vm;
    copy_pair(class_wrapper.value, class_pair);
    if ((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(&class_wrapper), "AssociateResource", (const void *)(method_source), sizeof(method_source), (void *)(kinoko_cact_associate_resource), 0)) < 0) {
        kinoko_sqrat_release_pair(vm, class_pair);
        kinoko_sqrat_trim_stack(vm, base);
        return 0;
    }
    if (!kinoko_publish_cact_layer_members(vm, class_pair)) {
        kinoko_sqrat_release_pair(vm, class_pair);
        kinoko_sqrat_trim_stack(vm, base);
        return 0;
    }
    if (!kinoko_sqrat_set_pair(vm, reinterpret_cast<const int32_t*>(kinoko::act::LayerObjectView(root_object).bytes(&kinoko::act::LayerObjectRecord::value)), "CActLayer", class_pair)) {
        kinoko_sqrat_release_pair(vm, class_pair);
        kinoko_sqrat_trim_stack(vm, base);
        return 0;
    }
    kinoko_trace_i32("act:cact-layer-class", pair_data(class_pair));
    kinoko_sqrat_release_pair(vm, class_pair);
    kinoko_sqrat_trim_stack(vm, base);
    return 1;
}

// Original VM-only CActLayer class registration (41EFF0). Game instance
// publication still lives in its register method; only the class setup is shared.
extern "C" int32_t kinoko_register_cact_layer_class(SQVM* machine) {
    return register_root_class(machine, [](SQVM* vm, void* root) {
        return kinoko_publish_cact_layer_class(vm, (void*)(uintptr_t)(root));
    });
}


int32_t kinoko_publish_acting_player_properties(SQVM* vm,
                                                        const int32_t class_pair[sizeof(HSQOBJECT)/sizeof(int32_t)]) {
    static const char *names[] = { "staging", "marginLeft", "marginRight",
        "marginTop", "marginBottom", "offsetX", "offsetY", "visible",
        "resolutionMs", "screenWidth", "screenHeight", "stName" };
    static const int32_t offsets[] = { offsetof(kinoko::act::RuntimeRecord, stage_active),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, margin_left),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, margin_right),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, margin_top),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, margin_bottom),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, offset_x),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, offset_y),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, visible),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, resolution_ms),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, screen_width),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, screen_height),
        offsetof(kinoko::act::RuntimeRecord, stage_properties) + offsetof(kinoko::act::StagePropertyAliases, name) };
    int32_t get_table[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t set_table[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    const bool published = [&]() {
    if (!kinoko_sqrat_new_table(vm, get_table) ||
        !kinoko_sqrat_new_table(vm, set_table) ||
        !initialize_offset_property_class(vm, class_pair, set_table, get_table, nullptr, nullptr))
        return false;
    for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        if (!kinoko_sqrat_set_offset_closure(vm, get_table, names[i], offsets[i], reinterpret_cast<void*>(kinoko_acting_player_get_property)) ||
            !kinoko_sqrat_set_offset_closure(vm, set_table, names[i], offsets[i], reinterpret_cast<void*>(kinoko_acting_player_set_property)))
            return false;
    }
    return true;
    }();
    kinoko_sqrat_release_pair(vm, get_table);
    kinoko_sqrat_release_pair(vm, set_table);
    return published;
}

namespace {
struct DynamicLayerDelete {
    void operator()(unsigned char* layer) const noexcept {
        kinoko_act_layer_clear((KinokoActLayer*)(layer));
        std::free(layer);
    }
};
struct DynamicLayerLock {
    CRITICAL_SECTION* section;
    explicit DynamicLayerLock(KinokoActRuntime* player) : section(reinterpret_cast<CRITICAL_SECTION*>(kinoko::native::RecordView<kinoko::act::RuntimeRecord>(player).bytes(&kinoko::act::RuntimeRecord::lock))) {
        EnterCriticalSection(section);
    }
    ~DynamicLayerLock() { LeaveCriticalSection(section); }
};
struct DynamicLayerParent {
    kinoko::act::LayerObjectRecord object;
    explicit DynamicLayerParent(SQVM* vm) : object{kinoko_act_host_symbols()->sq_object_vtable,vm,{static_cast<int32_t>(OT_NULL),0},0,{}} {}
    ~DynamicLayerParent() { kinoko_sqrat_release_pair(object.vm, object.value.data()); }
};

// 451F30: inactive players return zero, while an absent layer returns -1.
int32_t get_layer_order(KinokoActRuntime* player, KinokoActLayer* layer) {
    if (!player || !kinoko::native::RecordView<kinoko::act::RuntimeRecord>(player).get(&kinoko::act::RuntimeRecord::stage_active)) return 0;
    const auto holder = kinoko::native::RecordView<kinoko::act::RuntimeRecord>(player).get(&kinoko::act::RuntimeRecord::active_holder);
    auto* act = holder ? holder->document : nullptr;
    if (!act) return -1;
    const auto span = kinoko::act::DocumentView(act).get(&kinoko::act::DocumentRecord::layers);
    const auto begin = reinterpret_cast<uintptr_t>(span.begin), end = reinterpret_cast<uintptr_t>(span.end);
    for (auto slot=begin; slot<end; slot+=4)
        if (kinoko::memory::load<KinokoActLayer*>(reinterpret_cast<const void*>(slot))==layer) return static_cast<int32_t>((slot-begin)/4);
    return -1;
}

// 451F80/455990/455A20/455C40. A swap rebuilds child vectors, then flattens
// roots in preorder; exchanging only the two slots breaks hierarchy ordering.
bool swap_layers(KinokoActRuntime* player, int32_t first, int32_t second) {
    if (!player) return false;
    kinoko::windows::CriticalLock lock(reinterpret_cast<CRITICAL_SECTION*>(kinoko::native::RecordView<kinoko::act::RuntimeRecord>(player).bytes(&kinoko::act::RuntimeRecord::lock)));
    if (!kinoko::native::RecordView<kinoko::act::RuntimeRecord>(player).get(&kinoko::act::RuntimeRecord::stage_active)) return false;
    const auto holder = kinoko::native::RecordView<kinoko::act::RuntimeRecord>(player).get(&kinoko::act::RuntimeRecord::active_holder);
    auto* act = holder ? holder->document : nullptr;
    if (!act) return false;
    const auto span = kinoko::act::DocumentView(act).get(&kinoko::act::DocumentRecord::layers);
    const auto begin = reinterpret_cast<uintptr_t>(span.begin), end = reinterpret_cast<uintptr_t>(span.end);
    if (end<begin || (end-begin)%4 || (!begin && end)) return false;
    const auto count = (end-begin)/4;
    if (first<0 || second<0 || static_cast<uint32_t>(first)>=count ||
        static_cast<uint32_t>(second)>=count || first==second) return false;
    std::vector<KinokoActLayer*> layers;
    layers.reserve(count);
    for (uint32_t i=0;i<count;++i) layers.push_back(kinoko::act::layer_at(span, i));
    std::map<KinokoActLayer*,size_t> index;
    for (size_t i=0;i<layers.size();++i)
        if (!layers[i] || !index.emplace(layers[i],i).second) return false;
    // Bound malformed parent chains before traversing them. Valid ACT trees
    // preserve the original ancestor rejection in both directions.
    for (const auto layer: layers) {
        auto parent = kinoko::native::RecordView<kinoko::act::LayerAssociationRecord>(layer).get(&kinoko::act::LayerAssociationRecord::parent);
        size_t depth=0;
        while (parent) {
            if (!index.count(parent) || ++depth>layers.size()) return false;
            if ((layer==layers[first] && parent==layers[second]) ||
                (layer==layers[second] && parent==layers[first])) return false;
            parent=kinoko::native::RecordView<kinoko::act::LayerAssociationRecord>(parent).get(&kinoko::act::LayerAssociationRecord::parent);
        }
    }
    std::swap(layers[first],layers[second]);
    std::map<KinokoActLayer*,std::vector<KinokoActLayer*>> children;
    std::vector<KinokoActLayer*> roots, ordered, pending;
    for (const auto layer: layers) {
        const auto parent=kinoko::native::RecordView<kinoko::act::LayerAssociationRecord>(layer).get(&kinoko::act::LayerAssociationRecord::parent);
        if (parent) children[parent].push_back(layer);
        else roots.push_back(layer);
    }
    pending.assign(roots.rbegin(),roots.rend());
    ordered.reserve(layers.size());
    while (!pending.empty()) {
        const auto layer=pending.back(); pending.pop_back();
        ordered.push_back(layer);
        const auto found=children.find(layer);
        if (found!=children.end())
            pending.insert(pending.end(),found->second.rbegin(),found->second.rend());
    }
    // Prepare every replacement before publishing any hierarchy mutation.
    std::map<KinokoActLayer*,std::unique_ptr<kinoko::ActArray>> replacements;
    for (const auto layer: layers) {
        auto values=std::make_unique<kinoko::ActArray>();
        const auto found=children.find(layer);
        if(found!=children.end()) values->assign(found->second.begin(), found->second.end());
        replacements.emplace(layer,std::move(values));
    }
    for(auto& entry:replacements)
        kinoko::replace_act_array(kinoko::native::RecordView<kinoko::act::LayerAssociationRecord>(entry.first).bytes(&kinoko::act::LayerAssociationRecord::children), std::move(entry.second));
    std::memcpy(span.begin, ordered.data(), ordered.size()*sizeof(KinokoActLayer*));
    return true;
}

intptr_t get_layer_order_native(SQVM* vm) {
    KinokoActRuntime* player=nullptr; KinokoActLayer* layer=nullptr;
    if (SQ_FAILED(sq_getinstanceup(vm,1,reinterpret_cast<SQUserPointer*>(&player),nullptr)) ||
        SQ_FAILED(sq_getinstanceup(vm,2,reinterpret_cast<SQUserPointer*>(&layer),nullptr)))
        return sq_throwerror(vm,"invalid GetLayerOrder arguments");
    sq_pushinteger(vm,get_layer_order(player, layer));
    return 1;
}
intptr_t swap_layers_native(SQVM* vm) {
    KinokoActRuntime* player=nullptr;
    SQInteger first=0, second=0;
    if (SQ_FAILED(sq_getinstanceup(vm,1,reinterpret_cast<SQUserPointer*>(&player),nullptr)) ||
        SQ_FAILED(sq_getinteger(vm,2,&first)) ||
        SQ_FAILED(sq_getinteger(vm,3,&second)))
        return sq_throwerror(vm,"invalid SwapLayer arguments");
    try {
        sq_pushbool(vm,swap_layers(player, first, second));
        return 1;
    } catch (...) { return sq_throwerror(vm,"SwapLayer allocation failed"); }
}

intptr_t find_first_native(SQVM* vm) {
    KinokoActRuntime *player=nullptr;
    const SQChar* pattern=nullptr;
    if (SQ_FAILED(sq_getinstanceup(vm,1,reinterpret_cast<SQUserPointer*>(&player),nullptr)) ||
        SQ_FAILED(sq_getstring(vm,2,&pattern)))
        return sq_throwerror(vm,"invalid FindFirstFile arguments");
    sq_pushinteger(vm,kinoko_act_find_first(player,pattern));
    return 1;
}
enum class FindOperation { Next, Close, Name };
template<FindOperation operation> intptr_t find_by_id_native(SQVM* vm) {
    KinokoActRuntime *player=nullptr;
    SQInteger id=0;
    if (SQ_FAILED(sq_getinstanceup(vm,1,reinterpret_cast<SQUserPointer*>(&player),nullptr)) ||
        SQ_FAILED(sq_getinteger(vm,2,&id)))
        return sq_throwerror(vm,"invalid file enumeration arguments");
    if constexpr (operation==FindOperation::Name)
        sq_pushstring(vm,kinoko_act_find_name(player,id),-1);
    else if constexpr (operation==FindOperation::Close)
        sq_pushbool(vm,kinoko_act_find_close(player,id));
    else sq_pushbool(vm,kinoko_act_find_next(player,id));
    return 1;
}

// Original 4517C0. Native RAII replaces the old auto_ptr temporaries and the
// source-backed Sqrat bridge replaces Object/GetSlot/RootTable emulation.
template<bool string_layout> KinokoActLayer* create_layer(KinokoActRuntime* player, const char* name) {
    if (!player || !name) return 0;
    DynamicLayerLock lock(player);
    using namespace kinoko::act;
    const kinoko::native::RecordView<RuntimeRecord> runtime(player);
    if (!runtime.get(&RuntimeRecord::stage_active)) return nullptr;
    auto* holder = runtime.get(&RuntimeRecord::active_holder);
    auto* act = holder ? holder->document : nullptr;
    auto* vm = runtime.get(&RuntimeRecord::vm);
    if (!act || !vm) return 0;
    DynamicLayerParent parent(vm);
    LayerObjectRecord root{nullptr, vm, runtime.get(&RuntimeRecord::environment), 0, {}};
    if (!get_pair(&root, kinoko_string_data(runtime.bytes(&RuntimeRecord::name)), parent.object.value.data()) ||
        parent.object.value[0] != 0x0a000020) return 0;
    kinoko::memory::Allocation<unsigned char> storage(static_cast<unsigned char*>(std::calloc(1,sizeof(kinoko::act::LayerStorageRecord))));
    if (!storage || !kinoko_act_layer_initialize(reinterpret_cast<KinokoActLayer*>(storage.get()), vm)) return 0;
    std::unique_ptr<unsigned char,DynamicLayerDelete> owned(storage.release());
    auto* layer = reinterpret_cast<KinokoActLayer*>(owned.get());
    const LayerStorageView record(layer);
    kinoko::legacy::StringView(record.bytes(&LayerStorageRecord::name)).assign(name, static_cast<uint32_t>(std::strlen(name)));
    kinoko::memory::Allocation<KeyRecord> key(static_cast<KeyRecord*>(std::calloc(1,sizeof(KeyRecord))));
    const auto clear_layout=[](unsigned char* value) {
        if constexpr(string_layout) kinoko_method_destroy_string_layout(reinterpret_cast<KinokoStringLayout*>(value),nullptr);
        else std::free(value);
    };
    unsigned char* layout_storage=nullptr;
    if constexpr(string_layout) layout_storage=reinterpret_cast<unsigned char*>(kinoko_create_string_layout());
    else {
        layout_storage=static_cast<unsigned char*>(std::calloc(1,sizeof(Layout2DRecord)));
        if(layout_storage) kinoko_construct_c2dlayout(reinterpret_cast<KinokoActLayout*>(layout_storage));
    }
    if(!layout_storage) return 0;
    std::unique_ptr<unsigned char,decltype(clear_layout)> layout(layout_storage,clear_layout);
    if (!key) return 0;
    key->methods = kinoko_act_host_symbols()->key_vtable;
    key->script_name.capacity = 15;
    if (!kinoko_act_append_list(record.bytes(&LayerStorageRecord::keys), (void*)(key.get()))) return 0;
    key->layout = reinterpret_cast<KinokoActLayout*>(layout.release());
    auto* native_layout = key->layout;
    key.release();
    record.view(&LayerStorageRecord::keys).set(&LayerListRecord::count, int32_t{1});
    const DocumentView document(act);
    const auto layers = document.get(&DocumentRecord::layers);
    const auto begin = reinterpret_cast<uintptr_t>(layers.begin), end = reinterpret_cast<uintptr_t>(layers.end);
    if (end < begin || (end-begin)%4 || (!begin && end) || (end-begin)/4 >= 0x10000) return 0;
    const auto count = (end-begin)/4;
    int32_t maximum = -1;
    for (uint32_t i=0; i<count; ++i) {
        auto* old_layer = kinoko::memory::load<KinokoActLayer*>(layers.begin+i);
        if (old_layer) maximum = std::max(maximum, LayerStorageView(old_layer).view(&LayerStorageRecord::association).get(&LayerAssociationRecord::layer_id));
    }
    record.view(&LayerStorageRecord::association).set(&LayerAssociationRecord::layer_id, maximum < 0 ? 1 : static_cast<int32_t>(static_cast<uint32_t>(maximum)+1));
    kinoko_act_array_append(document.bytes(&DocumentRecord::layers), (void*)(uintptr_t)(layer));
    owned.release(); // ACT owns the layer before either publication callback.
    if constexpr(string_layout) kinoko_method_set_string_layer((KinokoStringLayout*)(uintptr_t)(native_layout), nullptr, layer);
    else kinoko_method_layout_set_layer((KinokoActLayout*)(uintptr_t)(native_layout), nullptr, (KinokoActLayer*)(uintptr_t)(layer));
    kinoko_method_register_act_layer((KinokoActLayer*)(uintptr_t)(layer), nullptr, reinterpret_cast<void*>(&parent.object), 0);
    if constexpr(string_layout) kinoko_method_register_string_layout(reinterpret_cast<KinokoStringLayout*>(native_layout), nullptr);
    else kinoko_method_register_layout((KinokoActLayout*)(uintptr_t)(native_layout), nullptr);
    return layer;
}
// Original 452010 stores the borrowed native instance in player+76. The
// decompiled 4556C0 thunk lost the member call and returned conversion status.
// Original instance conversion initializes a null argument to zero, allowing
// SetRenderTarget(null) to select the default target again.
intptr_t set_render_target_native(SQVM* vm) {
    SQUserPointer player = nullptr, target = nullptr;
    const auto machine = vm;
    if (SQ_FAILED(sq_getinstanceup(machine, 1, &player, nullptr)) || !player)
        return sq_throwerror(machine, "invalid SetRenderTarget receiver");
    sq_getinstanceup(machine, 2, &target, nullptr);
    kinoko::native::RecordView<kinoko::act::RuntimeRecord>(player).set(
        &kinoko::act::RuntimeRecord::render_target, static_cast<KinokoActResource*>(target));
    sq_pushbool(machine, SQTrue);
    return 1;
}
template<bool string_layout> intptr_t create_layer_native(SQVM* vm) {
    KinokoActRuntime* player = nullptr;
    const SQChar* name = nullptr;
    if (SQ_FAILED(sq_getinstanceup(vm,1,reinterpret_cast<SQUserPointer*>(&player),nullptr)) ||
        SQ_FAILED(sq_getstring(vm,2,&name))) return sq_throwerror(vm, "invalid CreateLayer arguments");
    try {
        DynamicLayerParent root(vm), klass(vm);
        const auto root_value = kinoko::script::upstream::sqrat_root(vm);
        std::memcpy(root.object.value.data(), &root_value, sizeof(root_value));
        if (!kinoko_publish_cact_layer_class(vm, reinterpret_cast<void*>(&root.object)) ||
            !get_pair(reinterpret_cast<void*>(&root.object), "CActLayer", klass.object.value.data()))
            return sq_throwerror(vm, "CActLayer class is unavailable");
        const auto layer = create_layer<string_layout>(player,name);
        const auto type = kinoko::memory::load<HSQOBJECT>(klass.object.value.data());
        if (!kinoko::script::upstream::sqrat_push_instance(vm,type,layer))
            return sq_throwerror(vm, "CActLayer class is unavailable");
        return 1;
    } catch (...) { return sq_throwerror(vm, "CreateLayer allocation failed"); }
}
}

int32_t kinoko_publish_acting_player_class(SQVM* vm,
                                                   void* root_object)
{
    int32_t existing[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t class_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    kinoko::script::ObjectStorage class_object{vm, {}};
    int32_t existing_result;
    int32_t base;

    if (vm == 0 || root_object == 0)
        return 0;
    existing_result = get_pair((void*)(uintptr_t)(root_object), "ActingPlayer", existing);
    if (existing_result) {
        int32_t already_published =
            existing[0] == 0x08004000 && pair_data(existing) != 0;
        if (already_published) {
            copy_pair(kinoko_acting_player_class_pair, existing);
        }
        kinoko_sqrat_release_pair(vm, existing);
        if (already_published)
            return 1;
    }

    base = sq_gettop(vm);
    if (!kinoko_sqrat_new_class(vm, class_pair) || sq_gettop(vm) <= base)
        return 0;
    if (class_pair[0] != 0x08004000 || pair_data(class_pair) == 0) {
        kinoko_sqrat_release_pair(vm, class_pair);
        return 0;
    }

    class_object.value = kinoko::script::pair::read(class_pair);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "SetCurrentTime", (void *)(kinoko_act_set_current_time), (void *)(kinoko_sqrat_call_integer1), 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "IncrementFrame", (void *)(kinoko_act_increment_frame), (void *)(kinoko_sqrat_call_integer0), 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "GetCurrentTime", (void *)(kinoko_act_get_current_time), (void *)(kinoko_sqrat_call_integer0), 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "GetCurrentFrame", (void *)(kinoko_act_get_current_frame), (void *)(kinoko_sqrat_call_integer0), 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "BeginStage", (void *)(kinoko_method_begin_stage), (void *)(kinoko_sqrat_call_integer1), 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "EndStage", (void *)(kinoko_act_end_stage), (void *)(kinoko_sqrat_call_integer0), 0);
    kinoko_sqrat_set_native_closure(vm, class_pair, "CreateLayer2D", reinterpret_cast<void*>(create_layer_native<false>), nullptr, 0);
    kinoko_sqrat_set_native_closure(vm, class_pair, "CreateLayerString", reinterpret_cast<void*>(create_layer_native<true>), nullptr, 0);
    kinoko_sqrat_set_native_closure(vm, class_pair, "GetLayerOrder", reinterpret_cast<void*>(get_layer_order_native), nullptr, 0);
    kinoko_sqrat_set_native_closure(vm, class_pair, "SwapLayer", reinterpret_cast<void*>(swap_layers_native), nullptr, 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "BitBlt", (void *)(kinoko_method_act_bitblt), (void *)(kinoko_native_draw_member_callback), 0);
    kinoko_sqrat_set_native_closure(vm, class_pair, "SetRenderTarget", reinterpret_cast<void*>(set_render_target_native), nullptr, 0);
    kinoko_sqrat_set_native_closure(vm, class_pair, "FindFirstFile", reinterpret_cast<void*>(find_first_native), nullptr, 0);
    kinoko_sqrat_set_native_closure(vm, class_pair, "FindNextFile", reinterpret_cast<void*>(find_by_id_native<FindOperation::Next>), nullptr, 0);
    kinoko_sqrat_set_native_closure(vm, class_pair, "FindClose", reinterpret_cast<void*>(find_by_id_native<FindOperation::Close>), nullptr, 0);
    kinoko_sqrat_set_native_closure(vm, class_pair, "GetFindFileName", reinterpret_cast<void*>(find_by_id_native<FindOperation::Name>), nullptr, 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "timeGetTime", (void *)(timeGetTime), (void *)(kinoko_sqrat_call_integer0), 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "Sleep", (void *)(kinoko_act_sleep), (void *)(kinoko_native_integer_member_callback), 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "SleepTo", (void *)(kinoko_act_sleep_to), (void *)(kinoko_native_integer_member_callback), 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "Suspend", (void *)(kinoko_act_suspend), (void *)(kinoko_native_nullary_member_callback), 0);
    kinoko_sqplus_register_actor_method(vm, reinterpret_cast<int32_t*>(&class_object), "Resume", (void *)(kinoko_act_resume), (void *)(kinoko_native_nullary_member_callback), 0);

    if (!kinoko_publish_acting_player_properties(vm, class_pair)) {
        kinoko_sqrat_release_pair(vm, class_pair);
        kinoko_sqrat_trim_stack(vm, base);
        return 0;
    }

    copy_pair(kinoko_acting_player_class_pair, class_pair);
    kinoko_sqrat_set_pair(vm, reinterpret_cast<const int32_t*>(kinoko::act::LayerObjectView(root_object).bytes(&kinoko::act::LayerObjectRecord::value)), "ActingPlayer", class_pair);
    kinoko_sqrat_release_pair(vm, class_pair);
    kinoko_sqrat_trim_stack(vm, base);
    return 1;
}

int32_t kinoko_publish_acting_player(SQVM* vm,
                                             const int32_t *act_pair,
                                             const char *name,
                                             KinokoActRuntime* player_ptr,
                                             int32_t out_pair[sizeof(HSQOBJECT)/sizeof(int32_t)])
{
    int32_t base;
    int32_t result;
    SQObjectPtr* instance_slot;
    int32_t actual[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    kinoko::act::LayerObjectRecord object_wrapper{kinoko_act_host_symbols()->sq_root_vtable, (SQVM*)(uintptr_t)(0), {static_cast<int32_t>(OT_NULL), 0}, 1, {}};

    if (vm == 0 || act_pair == nullptr || name == nullptr || out_pair == nullptr ||
        kinoko_acting_player_class_pair[0] != 0x08004000 || pair_data(kinoko_acting_player_class_pair) == 0)
        return 0;
    kinoko::script::diagnostic_name("act:pair-name", name);
    kinoko_trace_i32("act:pair-act", pair_data(act_pair));
    kinoko::script::diagnostic_name("act:acting-name", name);
    kinoko_trace_i32("act:acting-resource", kinoko::script::diagnostic_address(player_ptr));
    kinoko_trace_i32("act:acting-class-type", kinoko_acting_player_class_pair[0]);
    kinoko_trace_i32("act:acting-class-data", pair_data(kinoko_acting_player_class_pair));
    kinoko::script::pair::reset(out_pair);
    base = sq_gettop(vm);
    sq_pushobject(vm, kinoko::script::pair::read(act_pair));
    sq_pushstring(vm, name, -1);
    sq_pushobject(vm, kinoko::script::pair::read(kinoko_acting_player_class_pair));
    if (sq_createinstance(vm, -1) < 0) {
        kinoko_sqrat_trim_stack(vm, base);
        return 0;
    }
    sq_remove(vm, -2);
    instance_slot = kinoko_sq_get_up(vm, -1);
    kinoko_trace_i32("act:acting-instance-slot", kinoko::script::diagnostic_address(instance_slot));
    kinoko_trace_i32("act:acting-instance-type",
                     instance_slot != 0 ? static_cast<int32_t>(type(*instance_slot)) : 0);
    kinoko_trace_i32("act:acting-instance-data",
                     instance_slot != 0 ? kinoko::script::data_bits(*instance_slot) : 0);
    if (instance_slot != 0 &&
        type(*instance_slot) == OT_INSTANCE &&
        _instance(*instance_slot) != nullptr) {
        auto* instance = _instance(*instance_slot);
        kinoko_trace_i32("act:acting-instance-class",
                         kinoko::script::diagnostic_address(instance->_class));
        kinoko_trace_i32("act:acting-instance-user-before",
                         kinoko::script::diagnostic_address(instance->_userpointer));
    }
    if (sq_setinstanceup(vm, -1, player_ptr) < 0) {
        kinoko_sqrat_trim_stack(vm, base);
        return 0;
    }
    if (instance_slot != 0 &&
        type(*instance_slot) == OT_INSTANCE &&
        _instance(*instance_slot) != nullptr) {
        auto* instance = _instance(*instance_slot);
        kinoko_trace_i32("act:acting-instance-class-after",
                         kinoko::script::diagnostic_address(instance->_class));
        kinoko_trace_i32("act:acting-instance-user-after",
                         kinoko::script::diagnostic_address(instance->_userpointer));
    }
    kinoko::script::pair::capture(vm, -1, out_pair);
    kinoko_sqrat_retain_pair(vm, out_pair);
    result = sq_newslot(vm, -3, ((0) != 0));
    if (std::strcmp(name, "pl") == 0 || std::strcmp(name, "player") == 0) {
        kinoko::script::diagnostic_name("act:acting-read-name", name);
        object_wrapper.vm = vm;
        copy_pair(object_wrapper.value, act_pair);
        if (get_pair(reinterpret_cast<void*>(&object_wrapper), name, actual)) {
            kinoko_trace_i32("act:acting-read-type", actual[0]);
            kinoko_trace_i32("act:acting-read-data", pair_data(actual));
            if (actual[0] == 0x0A008000 && pair_data(actual) != 0) {
                kinoko_trace_i32("act:acting-read-class",
                                 kinoko::script::diagnostic_address(_instance(kinoko::script::pair::read(actual))->_class));
                kinoko_trace_i32("act:acting-read-user",
                                 kinoko::script::diagnostic_address(_instance(kinoko::script::pair::read(actual))->_userpointer));
            }
            kinoko_sqrat_release_pair(vm, actual);
        } else {
            kinoko_trace("act:acting-read-failed");
        }
    }
    kinoko_trace_i32("act:pair-act-after", pair_data(act_pair));
    kinoko_sqrat_trim_stack(vm, base);
    return result >= 0 && out_pair[0] == 0x0A008000;
}

int32_t kinoko_execute_act_source_script(SQVM* vm, void* script_ptr, const int32_t* environment_pair) {
    if (!vm || !script_ptr || !environment_pair) return 0;
    using namespace kinoko::act;
    const ScriptStorageView script(script_ptr);
    const auto* source = static_cast<const char*>(script.get(&ScriptStorageRecord::bytes));
    const auto size = static_cast<int32_t>(script.get(&ScriptStorageRecord::size));
    if (!source || size <= 0 || size > 0x1000000) return 0;
    return kinoko_sq_compile_act_source(vm, source, static_cast<int32_t>(strnlen(source, size)), environment_pair);
}

int32_t kinoko_execute_act_callback(void* script_ptr, int32_t offset, const char* trace_label) {
    if (!script_ptr) return 0;
    // The callback offset is an existing dispatch parameter (Init/Update/OnCreate).
    auto* slot = static_cast<unsigned char*>(script_ptr) + offset;
    const auto callback = kinoko::memory::load<kinoko::act::ActCallbackRecord>(slot);
    kinoko_trace_i32("act:callback-offset", offset);
    kinoko_trace_i32("act:callback-vm", kinoko::script::diagnostic_address(callback.vm));
    kinoko_trace_i32("act:callback-type", callback.closure[0]);
    kinoko_trace_i32("act:callback-data", pair_data(callback.closure));
    if (!callback.vm || callback.closure[0] == static_cast<int32_t>(OT_NULL) || !pair_data(callback.closure)) return 0;
    if (trace_label) kinoko_trace(trace_label);
    const auto result = kinoko_sqrat_invoke_callback(slot);
    kinoko_trace_i32("act:callback-result", result);
    return result;
}

namespace {
// 51B924 maps the script environment's object pointer to CActScript. Original
// insert is unique; re-registering an environment does not replace its owner.
std::map<intptr_t, void*> act_script_owners;
void refresh_act_script_callbacks(SQVM* vm, void* script, const int32_t *environment) {
    kinoko::act::LayerObjectRecord wrapper{kinoko_sqrat_object_vtable(), vm, pair_storage(environment), 0, {}};
    kinoko_copy_act_callback(vm, (void*)(uintptr_t)(script), offsetof(kinoko::act::ScriptStorageRecord,initialize), (void*)(&wrapper), "Init");
    kinoko_copy_act_callback(vm, (void*)(uintptr_t)(script), offsetof(kinoko::act::ScriptStorageRecord,update), (void*)(&wrapper), "Update");
    kinoko_copy_act_callback(vm, (void*)(uintptr_t)(script), offsetof(kinoko::act::ScriptStorageRecord,release), (void*)(&wrapper), "OnCreate");
}
}

extern "C" void kinoko_forget_act_script(void* script) {
    for (auto entry = act_script_owners.begin(); entry != act_script_owners.end();) {
        if (entry->second == script) entry = act_script_owners.erase(entry);
        else ++entry;
    }
}

int32_t kinoko_compile_act_file(SQVM* vm, const char *path, const int32_t *environment) {
    if (!vm || !path || !environment || environment[0] == 0x01000001) return 0;
    KinokoArchiveReader *reader = nullptr;
    try {
        std::string resolved(path);
        const char *extension = kinoko_string_data(&kinoko_act_script_extension);
        if (extension && *extension) {
            const auto dot = resolved.rfind('.');
            if (dot != std::string::npos) resolved = resolved.substr(0, dot) + extension;
        }
        if (!kinoko_reader_open(&reader, resolved.c_str())) {
            if (reader) kinoko_reader_close(reader);
            return 0;
        }
        const uint32_t size = kinoko_reader_size(reader);
        if (size > 0x1000000) {
            kinoko_reader_close(reader);
            return 0;
        }
        std::vector<unsigned char> buffer(static_cast<size_t>(size) + 1, 0);
        const bool read = !size || kinoko_reader_read_exact(reader, buffer.data(), size);
        kinoko_reader_close(reader);
        reader = 0;
        if (!read) return 0;
        kinoko::act::ScriptStorageRecord script{};
        script.bytes = buffer.data(); script.size = size;
        const bool compiled = size >= 2 && buffer[0] == 0xfa && buffer[1] == 0xfa;
        const bool ok = compiled ? kinoko_execute_act_file_bytecode(vm, &script, environment)
            : kinoko_sq_compile_act_source(vm, reinterpret_cast<const char*>(buffer.data()), static_cast<int32_t>(strnlen(reinterpret_cast<const char*>(buffer.data()), size)), environment);
        if (!ok) return 0;
        const auto owner = act_script_owners.find(pair_data(environment));
        if (owner != act_script_owners.end()) refresh_act_script_callbacks(vm, (void*)(uintptr_t)(owner->second), environment);
        return 1;
    } catch (...) {
        if (reader) kinoko_reader_close(reader);
        return 0;
    }
}

intptr_t kinoko_local_compile_file_native(SQVM* vm) {
    const SQChar *path = nullptr;
    int32_t environment[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
    if (sq_gettop(vm) < 2 || SQ_FAILED(sq_getstring(vm, 2, &path))) return 0;
    if (sq_gettop(vm) >= 3)
        kinoko::script::pair::capture(vm, 3, environment);
    sq_pushbool(vm, kinoko_compile_act_file(vm, path, environment));
    return 1;
}

int32_t kinoko_publish_act_script_constants(SQVM* vm, const int32_t *environment)
{
    static const char *const names[] = {
        "BLEND_NORMAL", "BLEND_ALPHA", "BLEND_ADD", "BLEND_SUB",
        "BLEND_MULTI", "BLEND_INVERT"
    };
    kinoko::act::LayerObjectRecord object{kinoko_act_host_symbols()->sq_object_vtable, vm, pair_storage(environment), 0, {}};
    int32_t user[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
    int32_t have_user = get_pair(reinterpret_cast<void*>(&object), "u", user);
    int32_t needs_user = !have_user || user[0] == static_cast<int32_t>(OT_NULL);
    kinoko_sqrat_release_pair(vm, user);
    /* 416056..41605F passes the same Sqrat &object as source and destination. */
    if (needs_user && !kinoko_sqrat_set_pair(vm, environment, "u", environment))
        return 0;
    /* 415FD0:4160DC installs these in the script environment before loading. */
    for (int32_t value = 0; value < 6; ++value) {
        if (!kinoko_sqrat_bind_int(vm, environment, names[value], value))
            return 0;
    }
    return kinoko_sqrat_set_native_closure(vm, environment, "CompileFile", reinterpret_cast<void*>(kinoko_local_compile_file_native), nullptr, 0);
}

extern "C" int32_t kinoko_register_act_script(void* script, void* object) {
    using namespace kinoko::act;
    if (!script || !object) return static_cast<int32_t>(E_FAIL);
    const LayerObjectView wrapper(object);
    const auto* environment = reinterpret_cast<const int32_t*>(wrapper.bytes(&LayerObjectRecord::value));
    if (environment[0] == static_cast<int32_t>(OT_NULL)) return static_cast<int32_t>(E_FAIL);
    auto* vm = wrapper.get(&LayerObjectRecord::vm);
    if (!vm || !kinoko_publish_act_script_constants(vm, environment)) return static_cast<int32_t>(E_FAIL);
    const ScriptStorageView record(script);
    if (!record.get(&ScriptStorageRecord::loaded)) return 0;
    bool ok = false;
    if (record.get(&ScriptStorageRecord::compiled)) {
        ok = kinoko_execute_embedded_act_script(vm, script, environment) != 0;
    } else {
        const char* path = kinoko_string_data(record.bytes(&ScriptStorageRecord::file_name));
        ok = path && *path ? kinoko_compile_act_file(vm, path, environment) != 0
                          : kinoko_execute_act_source_script(vm, script, environment) != 0;
    }
    if (!ok) return static_cast<int32_t>(E_FAIL);
    refresh_act_script_callbacks(vm, script, environment);
    act_script_owners.emplace(pair_data(environment), script);
    return 0;
}

int32_t kinoko_prepare_cact_layer_objects(SQVM* vm, KinokoActLayer* layer,
                                         int32_t script_pair[sizeof(HSQOBJECT)/sizeof(int32_t)]) {
    using namespace kinoko::act;
    if (!vm || !layer || !script_pair) return 0;
    auto* machine = vm;
    const LayerStorageView storage(layer);
    const auto script = storage.view(&LayerStorageRecord::script_object);
    const auto layout = storage.view(&LayerStorageRecord::layout_object);
    auto reset = [machine](LayerObjectView object, const void* methods) {
        auto* old_vm = object.get(&LayerObjectRecord::vm);
        auto* pair = reinterpret_cast<int32_t*>(object.bytes(&LayerObjectRecord::value));
        if (object.get(&LayerObjectRecord::owns_reference) && old_vm)
            kinoko_sqrat_release_pair(old_vm, pair);
        object.set(&LayerObjectRecord::methods, methods);
        object.set(&LayerObjectRecord::vm, machine);
        object.set(&LayerObjectRecord::owns_reference, uint8_t{1});
        HSQOBJECT empty;
        sq_resetobject(&empty);
        std::memcpy(pair, &empty, sizeof empty);
    };
    reset(script, kinoko_act_host_symbols()->layer_ref_vtable);
    kinoko::act::ScriptValueStorage table{static_cast<int32_t>(OT_NULL), 0};
    if (!kinoko_sqrat_new_table(machine, table.data())) return 0;
    script.set(&LayerObjectRecord::value, table);
    kinoko_sqrat_retain_pair(machine, reinterpret_cast<const int32_t*>(script.bytes(&LayerObjectRecord::value)));
    kinoko_sqrat_release_pair(machine, table.data());
    const auto value = script.get(&LayerObjectRecord::value);
    std::memcpy(script_pair, value.data(), sizeof value);
    if (!kinoko_publish_act_script_constants(vm, script_pair)) return 0;
    reset(layout, kinoko_act_host_symbols()->layer_layout_vtable);
    return 1;
}

// 41F580: publish in the ACT table, create a fresh script table, execute its
// script, then expose the inner native layer and associated resource wrappers.
// The second stack argument is unused by the original implementation.
extern "C" int32_t __fastcall kinoko_method_register_act_layer(KinokoActLayer* layer, void*, void* parent, int32_t) {
    using namespace kinoko::act;
    if (!layer || !parent) return static_cast<int32_t>(E_FAIL);
    const LayerObjectView parent_object(parent);
    auto* parent_pair = reinterpret_cast<const int32_t*>(parent_object.bytes(&LayerObjectRecord::value));
    if (parent_pair[0] == static_cast<int32_t>(OT_NULL)) return static_cast<int32_t>(E_FAIL);
    auto* vm = parent_object.get(&LayerObjectRecord::vm);
    if (!vm) return static_cast<int32_t>(E_FAIL);
    const LayerStorageView record(layer);
    std::memcpy(record.bytes(&LayerStorageRecord::origin_bits), record.bytes(&LayerStorageRecord::position), 12);
    LayerObjectRecord root{}; int32_t klass[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
    int32_t outer[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0}, inner[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0}, script[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
    if (!kinoko_sqrat_root_construct(&root, vm)) return static_cast<int32_t>(E_FAIL);
    bool ok = kinoko_publish_cact_layer_class(vm, reinterpret_cast<void*>(&root)) &&
        get_pair(reinterpret_cast<void*>(&root), "CActLayer", klass) &&
        kinoko_create_bound_instance(vm, parent_pair, kinoko_string_data(record.bytes(&LayerStorageRecord::name)), klass, layer, outer) &&
        kinoko_prepare_cact_layer_objects(vm, layer, script);
    if (ok) {
        kinoko_sqrat_assign_pair(vm, reinterpret_cast<int32_t*>(record.view(&LayerStorageRecord::layout_object).bytes(&LayerObjectRecord::value)), outer);
        ok = kinoko_sqrat_raw_set_pair(vm, outer, "script", script) &&
            kinoko_sqrat_set_pair(vm, script, "thisAct", parent_pair) &&
            kinoko_register_act_script(record.bytes(&LayerStorageRecord::script), record.bytes(&LayerStorageRecord::script_object)) >= 0;
    }
    if (ok) {
        ok = kinoko_create_bound_instance(vm, script, "layer", klass, layer, inner) != 0;
        auto* resource = record.view(&LayerStorageRecord::association).get(&LayerAssociationRecord::resource);
        if (resource) {
            const kinoko::act::ResourceMethods methods(resource);
            methods.bind_table(record.bytes(&LayerStorageRecord::layout_object), "resource");
            methods.bind_object(record.bytes(&LayerStorageRecord::script_object), "resource");
        }
    }
    kinoko_sqrat_release_pair(vm, inner);
    kinoko_sqrat_release_pair(vm, outer);
    kinoko_sqrat_release_pair(vm, klass);
    kinoko_sqrat_object_release(&root);
    return ok ? 0 : static_cast<int32_t>(E_FAIL);
}

int32_t kinoko_bind_original_layout(KinokoActLayout* layout, bool map) {
    using namespace kinoko::act;
    auto* layer = !layout ? nullptr : map ? kinoko::map::LayoutView(layout).get(&kinoko::map::LayoutRecord::owning_layer)
        : kinoko::native::RecordView<Layout2DRecord>(layout).get(&Layout2DRecord::layer);
    if (!layer) return static_cast<int32_t>(E_FAIL);
    const LayerStorageView record(layer);
    const auto layout_object = record.view(&LayerStorageRecord::layout_object);
    auto* layout_pair = reinterpret_cast<const int32_t*>(layout_object.bytes(&LayerObjectRecord::value));
    if (layout_pair[0] == static_cast<int32_t>(OT_NULL)) return static_cast<int32_t>(E_FAIL);
    auto* vm = layout_object.get(&LayerObjectRecord::vm);
    if (!vm) return static_cast<int32_t>(E_INVALIDARG);
    kinoko::act::LayerObjectRecord root{}; int32_t  klass[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
    int32_t outer[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0}, script[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
    if (!kinoko_sqrat_root_construct(&root, vm)) return static_cast<int32_t>(E_FAIL);
    const bool registered = map
        ? kinoko_publish_c2dmaplayout_class(vm, reinterpret_cast<void*>(&root), klass) != 0
        : kinoko_publish_c2dlayout_class(vm, reinterpret_cast<void*>(&root)) && get_pair(reinterpret_cast<void*>(&root), "C2DLayout", klass);
    const bool ok = registered &&
        kinoko_create_unbound_instance(vm, klass, layout, outer) &&
        kinoko_sqrat_raw_set_pair(vm, layout_pair, "layout", outer) &&
        kinoko_create_bound_instance(vm, reinterpret_cast<const int32_t*>(record.view(&LayerStorageRecord::script_object).bytes(&LayerObjectRecord::value)), "layout", klass, layout, script);
    kinoko_sqrat_release_pair(vm, script);
    kinoko_sqrat_release_pair(vm, outer);
    kinoko_sqrat_release_pair(vm, klass);
    kinoko_sqrat_object_release((void *)(&root));
    if (!ok) return static_cast<int32_t>(E_FAIL);
    using namespace kinoko::act;
    const auto aliases = LayerStorageView(layer).view(&LayerStorageRecord::association)
        .view(&LayerAssociationRecord::property_aliases);
    if (map) {
        const kinoko::map::LayoutView view(layout);
        aliases.set(&LayerPropertyAliases::alpha, reinterpret_cast<float*>(view.bytes(&kinoko::map::LayoutRecord::alpha)));
        aliases.set(&LayerPropertyAliases::blend, reinterpret_cast<int32_t*>(view.bytes(&kinoko::map::LayoutRecord::blend)));
    } else {
        const kinoko::native::RecordView<Layout2DRecord> view(layout);
        const auto rotation = view.view(&Layout2DRecord::rotation);
        const auto pivot = view.view(&Layout2DRecord::rotation_pivot);
        const auto scale = view.view(&Layout2DRecord::scale);
        const auto scale_pivot = view.view(&Layout2DRecord::scale_pivot);
        const LayerPropertyAliases values{
            reinterpret_cast<float*>(rotation.bytes(&Position3::x)), reinterpret_cast<float*>(rotation.bytes(&Position3::y)), reinterpret_cast<float*>(rotation.bytes(&Position3::z)),
            reinterpret_cast<float*>(pivot.bytes(&Position3::x)), reinterpret_cast<float*>(pivot.bytes(&Position3::y)), reinterpret_cast<float*>(pivot.bytes(&Position3::z)),
            reinterpret_cast<float*>(scale.bytes(&Position3::x)), reinterpret_cast<float*>(scale.bytes(&Position3::y)), reinterpret_cast<float*>(scale.bytes(&Position3::z)),
            reinterpret_cast<float*>(scale_pivot.bytes(&Position3::x)), reinterpret_cast<float*>(scale_pivot.bytes(&Position3::y)), reinterpret_cast<float*>(scale_pivot.bytes(&Position3::z)),
            reinterpret_cast<float*>(view.bytes(&Layout2DRecord::alpha)), reinterpret_cast<int32_t*>(view.bytes(&Layout2DRecord::blend)),
            reinterpret_cast<int32_t*>(view.bytes(&Layout2DRecord::red)), reinterpret_cast<int32_t*>(view.bytes(&Layout2DRecord::green)), reinterpret_cast<int32_t*>(view.bytes(&Layout2DRecord::blue))};
        kinoko::memory::store(aliases.data(), values);
    }
    return 0;
}

extern "C" int32_t __fastcall kinoko_method_register_layout(KinokoActLayout* layout, void *) {
    return kinoko_bind_original_layout(layout, false);
}
extern "C" int32_t __fastcall kinoko_method_register_map_layout(KinokoActLayout* layout, void *) {
    return kinoko_bind_original_layout(layout, true);
}

intptr_t kinoko_map_chip_count(SQVM* vm) {
    KinokoActLayout* layout = nullptr;
    if (sq_getinstanceup(vm, 1, (SQUserPointer*)(&layout), nullptr) < 0 || layout == 0)
        return 0;
    sq_pushinteger(vm, kinoko::map::placement_count(layout));
    return 1;
}

intptr_t kinoko_map_get_chip_layout(SQVM* vm) {
    KinokoActLayout* layout = nullptr;
    int32_t index = -1;
    kinoko::map::Placement* begin;
    int32_t count;
    kinoko::act::LayerObjectRecord root{};
    int32_t chip_class[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t instance[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    if (sq_getinstanceup(vm, 1, (SQUserPointer*)(&layout), nullptr) < 0 || layout == 0 ||
        get_game_integer(vm, 2, &index) < 0)
        return 0;
    begin = kinoko::map::LayoutView(layout).get(&kinoko::map::LayoutRecord::placements).begin;
    count = kinoko::map::placement_count(layout);
    if (index < 0 || index >= count) {
        sq_pushnull(vm);
        return 1;
    }
    if (!kinoko_sqrat_root_construct(&root, vm))
        return 0;
    if (get_pair(reinterpret_cast<void*>(&root), "ChipLayout", chip_class) &&
        kinoko_create_unbound_instance(vm, chip_class, begin + index, instance))
        sq_pushobject(vm, kinoko::script::pair::read(instance));
    else
        sq_pushnull(vm);
    kinoko_sqrat_release_pair(vm, instance);
    kinoko_sqrat_release_pair(vm, chip_class);
    kinoko_sqrat_object_release((void *)(&root));
    return 1;
}

namespace {
template<class T> T* native_instance_argument(SQVM* vm, int32_t* index = nullptr) {
    void* value = nullptr;
    if (SQ_FAILED(sq_getinstanceup(vm, 1, &value, nullptr)) || !value ||
        (index && SQ_FAILED(get_game_integer(vm, 2, index)))) return nullptr;
    return static_cast<T*>(value);
}
}

intptr_t kinoko_map_get_chip_by_position(SQVM* vm) {
    int32_t x = 0, y = 0;
    auto* layout = native_instance_argument<KinokoActLayout>(vm, &x);
    // GetChipByPosition calls 435220, whose 435243..435265 prologue binds
    // an empty cache even for invisible event layers. Other chip-data users
    // (e.g. PreArrangement) do not have this lazy-binding contract.
    const bool valid_position = layout &&
        get_game_integer(vm, 3, &y) >= 0;
    struct kinoko_mcd_data *data = valid_position
        ? kinoko_map_query_chip_data(layout) : nullptr;
    int32_t found = -1;
    if (valid_position && data != nullptr) {
        auto* layer = kinoko::map::LayoutView(layout).get(&kinoko::map::LayoutRecord::owning_layer);
        for (int32_t index = 0;; ++index) {
            auto* record = kinoko::map::placement_at(layout, index);
            if (!record) break;
            struct kinoko_mcd_chip *chip = kinoko_mcd_find_chip(data, record->chip_id);
            int32_t left = record->left;
            int32_t top = record->top;
            record->fractional_left = (float)left +
                (layer ? kinoko::map::LayerView(layer).get(&kinoko::map::LayerRecord::position_x) : 0.0f);
            record->fractional_top = (float)top +
                (layer ? kinoko::map::LayerView(layer).get(&kinoko::map::LayerRecord::position_y) : 0.0f);
            if (chip != nullptr && left <= x && top <= y &&
                (int64_t)left + kinoko_mcd_i16(chip->bytes + 12) > x &&
                (int64_t)top + kinoko_mcd_i16(chip->bytes + 14) > y) {
                found = index;
                break;
            }
        }
    }
    sq_pushinteger(vm, found);
    return 1;
}

intptr_t kinoko_map_set_chip_rect(SQVM* vm) {
    int32_t id = 0, rectangle[4];
    auto* layout = native_instance_argument<KinokoActLayout>(vm, &id);
    for (int32_t i=0;i<4;++i) {
        if (get_game_integer(vm, i+3, rectangle+i)<0) {
            sq_pushbool(vm,SQFalse);return 1;
        }
    }
    const bool ok=kinoko::map::set_chip_rectangle(layout,id,
        static_cast<int16_t>(rectangle[0]),static_cast<int16_t>(rectangle[1]),
        static_cast<int16_t>(rectangle[2]),static_cast<int16_t>(rectangle[3]));
    sq_pushbool(vm,ok ? SQTrue : SQFalse);
    return 1;
}

intptr_t kinoko_map_set_chip_layout(SQVM* vm) {
    int32_t index = -1, left = 0, top = 0;
    auto* layout = native_instance_argument<KinokoActLayout>(vm, &index);
    auto* record = kinoko::map::placement_at(layout, index);
    int32_t ok = record != 0 && get_game_integer(vm, 3, &left) >= 0 &&
                 get_game_integer(vm, 4, &top) >= 0;
    if (ok) {
        record->left = left;
        record->top = top;
    }
    sq_pushbool(vm, ((ok) != 0));
    return 1;
}

intptr_t kinoko_map_set_chip_id(SQVM* vm) {
    int32_t index = -1, id = 0;
    auto* layout = native_instance_argument<KinokoActLayout>(vm, &index);
    auto* record = kinoko::map::placement_at(layout, index);
    int32_t ok = record != 0 && get_game_integer(vm, 3, &id) >= 0;
    if (ok)
        record->chip_id = static_cast<uint32_t>(id);
    sq_pushbool(vm, ((ok) != 0));
    return 1;
}

intptr_t kinoko_map_get_chip_id(SQVM* vm) {
    int32_t index = -1;
    auto* layout = native_instance_argument<KinokoActLayout>(vm, &index);
    auto* record = kinoko::map::placement_at(layout, index);
    sq_pushinteger(vm, record ? static_cast<int32_t>(record->chip_id) : -1);
    return 1;
}

intptr_t kinoko_map_prearrangement(SQVM* vm) {
    auto* layout = native_instance_argument<KinokoActLayout>(vm, nullptr);
    struct kinoko_mcd_data *data = kinoko_map_cached_chip_data(layout);
    if (layout == 0 || data == nullptr) {
        sq_pushinteger(vm, (int32_t)E_FAIL);
        return 1;
    }
    kinoko::map::prepare_placements(layout);
    sq_pushinteger(vm, 0);
    return 1;
}

// Original 433740 stores the fractional position and truncates it into left.
// 433770 intentionally updates only f_top; preserve that asymmetry.
intptr_t kinoko_chip_set_fractional_left(SQVM* vm) {
    auto* chip = native_instance_argument<kinoko::map::Placement>(vm);
    SQFloat value = 0;
    if (!chip || !kinoko::script::upstream::sqrat_float_argument(vm, 2, value)) return 0;
    chip->fractional_left = value;
    // __ftol2_sse produces a signed 64-bit integer; the caller keeps EAX.
    const int64_t truncated = std::isfinite(value) &&
        static_cast<double>(value) >= -9223372036854775808.0 &&
        static_cast<double>(value) < 9223372036854775808.0
        ? static_cast<int64_t>(value) : INT64_MIN;
    chip->left = static_cast<int32_t>(truncated);
    return 0;
}

intptr_t kinoko_map_get_left(SQVM* vm) {
    auto* layout = native_instance_argument<KinokoActLayout>(vm, nullptr);
    auto* first = kinoko::map::placement_at(layout, 0);
    sq_pushinteger(vm, first ? first->left : 0);
    return 1;
}

// Original 435F00 scans backwards only within maxChipWidth of the last left.
intptr_t kinoko_map_get_right(SQVM* vm) {
    using namespace kinoko::map;
    auto* layout = native_instance_argument<KinokoActLayout>(vm);
    const auto range = layout ? LayoutView(layout).get(&LayoutRecord::placements) : PlacementBuffer{};
    int32_t right = range.end != range.begin ? range.end[-1].left : 0;
    auto* data = kinoko_map_cached_chip_data(layout);
    if (range.end != range.begin && data) {
        const auto minimum = static_cast<int32_t>(static_cast<uint32_t>(right) -
            static_cast<uint32_t>(LayoutView(layout).get(&LayoutRecord::max_chip_width)));
        for (auto* record = range.end; record != range.begin;) {
            --record;
            const auto left = record->left;
            if (left < minimum) break;
            auto* chip = kinoko_mcd_find_chip(data, record->chip_id);
            if (chip) {
                const auto edge = static_cast<int32_t>(static_cast<uint32_t>(left) + kinoko_mcd_i16(chip->bytes + 12));
                if (edge >= right) right = edge;
            }
        }
    }
    sq_pushinteger(vm, right);
    return 1;
}

int32_t kinoko_publish_map_view_class(SQVM* vm, void* root,
    const char *name, const struct kinoko_native_view_property *properties,
    int32_t property_count, int32_t is_map, int32_t out[sizeof(HSQOBJECT)/sizeof(int32_t)]) {
    int32_t get_table[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t set_table[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t base = sq_gettop(vm);
    if (get_pair((void*)(uintptr_t)(root), name, out) && out[0] == 0x08004000)
        return 1;
    kinoko_sqrat_release_pair(vm, out);
    const bool published = [&]() {
    if (!kinoko_sqrat_new_class(vm, out))
        return false;
    if (!kinoko_sqrat_new_table(vm, get_table) ||
        !kinoko_sqrat_new_table(vm, set_table) ||
        !initialize_native_property_class(vm, out, {set_table, get_table}))
        return false;
    for (int32_t i = 0; i < property_count; ++i) {
        const struct kinoko_native_view_property *p = properties + i;
        void* getter = p->kind == 1 ? reinterpret_cast<void*>(kinoko_c2dlayout_get_float) :
            p->kind == 2 ? reinterpret_cast<void*>(kinoko_cact_layer_get_bool) :
            p->kind == 3 ? reinterpret_cast<void*>(kinoko_native_view_get_short) :
            p->kind == 4 ? reinterpret_cast<void*>(kinoko_cact_layer_get_string) :
                          reinterpret_cast<void*>(kinoko_c2dlayout_get_int);
        void* setter = p->kind == 1 ? reinterpret_cast<void*>(kinoko_c2dlayout_set_float) :
            p->kind == 2 ? reinterpret_cast<void*>(kinoko_cact_layer_set_bool) :
            p->kind == 3 ? reinterpret_cast<void*>(kinoko_native_view_set_short) :
            p->kind == 4 ? reinterpret_cast<void*>(kinoko_cact_layer_set_string) :
                          reinterpret_cast<void*>(kinoko_c2dlayout_set_int);
        if (!kinoko_sqrat_set_offset_closure(vm, get_table, p->name, p->offset, (void *)(intptr_t)(getter)) ||
            !kinoko_sqrat_set_offset_closure(vm, set_table, p->name, p->offset, (void *)(intptr_t)(setter)))
            return false;
    }
    if (!is_map && std::strcmp(name, "ChipLayout") == 0 &&
        !kinoko_sqrat_set_native_closure(vm, set_table, "f_left", reinterpret_cast<void*>(kinoko_chip_set_fractional_left), nullptr, 0))
        return false;
    if (is_map &&
        (!kinoko_sqrat_set_native_closure(vm, get_table, "left", reinterpret_cast<void*>(kinoko_map_get_left), nullptr, 0) ||
         !kinoko_sqrat_set_native_closure(vm, get_table, "right", reinterpret_cast<void*>(kinoko_map_get_right), nullptr, 0) ||
         !kinoko_sqrat_set_native_closure(vm, out, "PreArrangement", reinterpret_cast<void*>(kinoko_map_prearrangement), nullptr, 0) ||
         !kinoko_sqrat_set_native_closure(vm, get_table, "chipCount", reinterpret_cast<void*>(kinoko_map_chip_count), nullptr, 0) ||
         !kinoko_sqrat_set_native_closure(vm, out, "GetChipLayout", reinterpret_cast<void*>(kinoko_map_get_chip_layout), nullptr, 0) ||
         !kinoko_sqrat_set_native_closure(vm, out, "GetChipByPosition", reinterpret_cast<void*>(kinoko_map_get_chip_by_position), nullptr, 0) ||
         !kinoko_sqrat_set_native_closure(vm, out, "SetChipRect", reinterpret_cast<void*>(kinoko_map_set_chip_rect), nullptr, 0) ||
         !kinoko_sqrat_set_native_closure(vm, out, "SetChipLayout", reinterpret_cast<void*>(kinoko_map_set_chip_layout), nullptr, 0) ||
         !kinoko_sqrat_set_native_closure(vm, out, "SetChipID", reinterpret_cast<void*>(kinoko_map_set_chip_id), nullptr, 0) ||
         !kinoko_sqrat_set_native_closure(vm, out, "GetChipID", reinterpret_cast<void*>(kinoko_map_get_chip_id), nullptr, 0)))
        return false;
    return kinoko_sqrat_set_pair(vm,
        reinterpret_cast<const int32_t*>(kinoko::act::LayerObjectView(root).bytes(&kinoko::act::LayerObjectRecord::value)), name, out) != 0;
    }();
    kinoko_sqrat_release_pair(vm, get_table);
    kinoko_sqrat_release_pair(vm, set_table);
    kinoko_sqrat_trim_stack(vm, base);
    return published;
}

int32_t kinoko_publish_c2dmaplayout_class(SQVM* vm, void* root,
                                                int32_t out[sizeof(HSQOBJECT)/sizeof(int32_t)]) {
    static const struct kinoko_native_view_property map_properties[] = {
        { "layerType", offsetof(kinoko::map::LayoutRecord,layer_type), 0 }, { "maxChipWidth", offsetof(kinoko::map::LayoutRecord,max_chip_width), 0 },
        { "maxChipHeight", offsetof(kinoko::map::LayoutRecord,max_chip_height), 0 }, { "mapChipLeft", offsetof(kinoko::map::LayoutRecord,chip_left), 0 },
        { "mapChipRight", offsetof(kinoko::map::LayoutRecord,chip_right), 0 }, { "mapChipTop", offsetof(kinoko::map::LayoutRecord,chip_top), 0 },
        { "mapChipBottom", offsetof(kinoko::map::LayoutRecord,chip_bottom), 0 }, { "alpha", offsetof(kinoko::map::LayoutRecord,alpha), 1 },
        { "scale", offsetof(kinoko::map::LayoutRecord,scale), 1 }, { "blend", offsetof(kinoko::map::LayoutRecord,blend), 0 }
    };
    static const struct kinoko_native_view_property chip_properties[] = {
        { "chipID", 0, 0 }, { "left", 4, 0 }, { "top", 8, 0 },
        { "f_left", 12, 1 }, { "f_top", 16, 1 }, { "layoutID", 20, 0 },
        { "visible", 24, 2 }, { "alpha", 28, 1 }
    };
    int32_t chip_class[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t ok = kinoko_publish_map_view_class(vm, (void*)(uintptr_t)(root), "ChipLayout", chip_properties, sizeof(chip_properties) / sizeof(chip_properties[0]), 0, chip_class);
    kinoko_sqrat_release_pair(vm, chip_class);
    return ok && kinoko_publish_map_view_class(vm, (void*)(uintptr_t)(root), "C2DMapLayout", map_properties, sizeof(map_properties) / sizeof(map_properties[0]), 1, out);
}

// 433C90: cdecl, one VM argument, HRESULT result (IDA 4341ED: retn).
extern "C" int32_t kinoko_register_map_layout_class(SQVM* machine) {
    return register_root_class(machine, [](SQVM* vm, void* root) {
        int32_t klass[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
        const auto ok = kinoko_publish_c2dmaplayout_class(vm, (void*)(uintptr_t)(root), klass);
        kinoko_sqrat_release_pair(vm, klass);
        return ok;
    });
}


intptr_t kinoko_resource_get_chip_info(SQVM* vm) {
    KinokoActResource* resource = nullptr; int32_t id = 0;
    kinoko::act::LayerObjectRecord root{};
    int32_t klass[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 }, instance[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    struct kinoko_mcd_chip *chip;
    if (sq_getinstanceup(vm, 1, (SQUserPointer*)(&resource), nullptr) < 0 || resource == 0 ||
        get_game_integer(vm, 2, &id) < 0)
        return 0;
    chip = kinoko_mcd_find_chip(kinoko::act::chip_resource(resource).data.get(), (uint32_t)id);
    if (chip == nullptr || !kinoko_sqrat_root_construct(&root, vm)) {
        sq_pushnull(vm);
        return 1;
    }
    if (get_pair(reinterpret_cast<void*>(&root), "ChipInfo", klass) &&
        kinoko_create_unbound_instance(vm, klass, (void*)(chip->bytes), instance))
        sq_pushobject(vm, kinoko::script::pair::read(instance));
    else
        sq_pushnull(vm);
    kinoko_sqrat_release_pair(vm, klass);
    kinoko_sqrat_release_pair(vm, instance);
    kinoko_sqrat_object_release((void *)(&root));
    return 1;
}

intptr_t kinoko_resource_set_chip_flag(SQVM* vm) {
    auto* resource = native_instance_argument<KinokoActResource>(vm);
    SQInteger id = 0, flag = -1;
    // Original 42FEF8/42FEFD accepts only bit zero, despite the 64-bit storage.
    auto *data = resource ? kinoko::act::chip_resource(resource).data.get() : nullptr;
    auto *chip = data && kinoko::script::upstream::sqrat_integer_argument(vm, 2, id) &&
        kinoko::script::upstream::sqrat_integer_argument(vm, 3, flag) && flag == 0
        ? kinoko_mcd_find_chip(data, static_cast<uint32_t>(id)) : nullptr;
    if (chip) {
        uint32_t bits;
        std::memcpy(&bits, chip->bytes + 16, sizeof(bits));
        bits = kinoko::script::upstream::sqrat_bool_argument(vm, 4) ? bits | 1u : bits & ~1u;
        std::memcpy(chip->bytes + 16, &bits, sizeof(bits));
    }
    sq_pushbool(vm, chip != nullptr);
    return 1;
}

int32_t kinoko_publish_chip_resource_class(SQVM* vm, void* root, int32_t out[sizeof(HSQOBJECT)/sizeof(int32_t)]) {

    static const struct kinoko_native_view_property info_properties[] = {
        { "chipID", 0, 0 }, { "textureID", 4, 0 },
        { "left", 8, 3 }, { "top", 10, 3 }, { "width", 12, 3 }, { "height", 14, 3 },
        { "flag0", 16, 0 }, { "flag1", 20, 0 }, { "visible", 32, 2 },
        { "boundType", 34, 3 }, { "boundLeft", 36, 3 }, { "boundTop", 38, 3 },
        { "boundWidth", 40, 3 }, { "boundHeight", 42, 3 }
    };
    static const struct kinoko_native_view_property resource_properties[] = {
        { "resourceID", offsetof(kinoko::act::ChipResource, id), 0 },
        { "stName", offsetof(kinoko::act::ChipResource, name), 4 }
    };
    if (get_pair((void*)(uintptr_t)(root), "CActResourceChip", out) && out[0] == 0x08004000) return 1;
    kinoko_sqrat_release_pair(vm, out);
    int32_t info[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t ok;
    ok = kinoko_publish_map_view_class(vm, (void*)(uintptr_t)(root), "ChipInfo", info_properties, sizeof(info_properties) / sizeof(info_properties[0]), 0, info) &&
        kinoko_publish_map_view_class(vm, (void*)(uintptr_t)(root), "CActResourceChip", resource_properties, sizeof(resource_properties) / sizeof(resource_properties[0]), 0, out) &&
        kinoko_sqrat_set_native_closure(vm, out, "GetChipInfo", reinterpret_cast<void*>(kinoko_resource_get_chip_info), nullptr, 0) &&
        kinoko_sqrat_set_native_closure(vm, out, "SetChipFlag", reinterpret_cast<void*>(kinoko_resource_set_chip_flag), nullptr, 0);
    kinoko_sqrat_release_pair(vm, info);
    return ok;
}

// Reconstructed C callers use this cdecl port; the virtual slot has its own
// ECX/stack-cleanup adapter matching original 42F350 (retn 4).
extern "C" int32_t kinoko_register_chip_resource_class(SQVM* machine) {
    return register_root_class(machine, [](SQVM* vm, void* root) {
        int32_t klass[sizeof(HSQOBJECT)/sizeof(int32_t)] = {static_cast<int32_t>(OT_NULL), 0};
        const auto ok = kinoko_publish_chip_resource_class(vm, (void*)(uintptr_t)(root), klass);
        kinoko_sqrat_release_pair(vm, klass);
        return ok;
    });
}


extern "C" int32_t __fastcall kinoko_method_register_chip_resource(
    void* receiver, void *, SQVM* vm) {
    return kinoko_register_chip_resource_class(vm);
}

int32_t kinoko_get_act_resource_class(SQVM* vm, KinokoActResource* resource, int32_t out[sizeof(HSQOBJECT)/sizeof(int32_t)]) {
    if(kinoko::memory::load<const void*>(resource)==kinoko::mesh::resource_methods()) {
        kinoko::act::LayerObjectRecord root{};
        if(!kinoko_sqrat_root_construct(&root, vm)) return 0;
        const auto ok=kinoko_publish_mesh_resource_class(vm, reinterpret_cast<void*>(&root), out);
        kinoko_sqrat_object_release((void *)(&root));return ok;
    }
    if (kinoko::memory::load<const void*>(resource) != kinoko_act_host_symbols()->chip_resource_vtable) {
        copy_pair(out, kinoko_resource2d_class_pair);
        kinoko_sqrat_retain_pair(vm, out);
        return out[0] == 0x08004000;
    }
    kinoko::act::LayerObjectRecord root{};
    if (!kinoko_sqrat_root_construct(&root, vm)) return 0;
    const auto ok = kinoko_publish_chip_resource_class(vm, reinterpret_cast<void*>(&root), out);
    kinoko_sqrat_object_release((void *)(&root));
    return ok;
}

// Resource Object/Table publication recovered from 4467E0/446920 and the
// corresponding Chip/RenderTarget entries. The Sqrat wrapper remains an ABI
// record; source ClassType::PushInstance owns the actual instance operation.
int32_t kinoko_bind_original_resource(KinokoActResource* resource, void* object,
    const char *name, const char *class_name, bool raw) {
    using namespace kinoko::act;
    if (!resource || !object) return static_cast<int32_t>(E_FAIL);
    const LayerObjectView wrapper(object);
    const auto* pair = reinterpret_cast<const int32_t*>(wrapper.bytes(&LayerObjectRecord::value));
    if (pair[0] == static_cast<int32_t>(OT_NULL))
        return static_cast<int32_t>(E_FAIL);
    SQVM* const vm = wrapper.get(&LayerObjectRecord::vm);
    if (!vm || (raw && (!name || !*name))) return static_cast<int32_t>(E_FAIL);
    if (!name || !*name) name = kinoko_string_data(ResourcePublicationView(resource).bytes(&ResourcePublicationRecord::name));
    kinoko::act::LayerObjectRecord root{}; int32_t  klass[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 }, instance[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    if (!kinoko_sqrat_root_construct(&root, vm)) return static_cast<int32_t>(E_FAIL);
    bool registered = get_pair(reinterpret_cast<void*>(&root), class_name, klass) && klass[0] == 0x08004000;
    if (!registered) {
        kinoko_sqrat_release_pair(vm, klass);
        registered = kinoko::act::ResourceMethods(resource).register_class(vm) >= 0 &&
            get_pair(reinterpret_cast<void*>(&root), class_name, klass) && klass[0] == 0x08004000;
    }
    bool ok = false;
    if (registered) {
        if (raw) {
            ok = kinoko_create_unbound_instance(vm, klass, resource, instance) &&
                kinoko_sqrat_raw_set_pair(vm, pair, name, instance);
        } else {
            ok = kinoko_create_bound_instance(vm, pair, name, klass, resource, instance);
        }
    }
    kinoko_sqrat_release_pair(vm, instance);
    kinoko_sqrat_release_pair(vm, klass);
    kinoko_sqrat_object_release((void *)(&root));
    return ok ? 0 : static_cast<int32_t>(E_FAIL);
}

int32_t kinoko_publish_act_resource_pairs(
    SQVM* vm, const int32_t layer_pair[sizeof(HSQOBJECT)/sizeof(int32_t)],
    const int32_t script_pair[sizeof(HSQOBJECT)/sizeof(int32_t)], KinokoActResource* resource)
{
    int32_t outer_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t script_resource_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t resource_class_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t null_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { 0x01000001, 0 };

    if (vm == 0 || layer_pair == nullptr || script_pair == nullptr)
        return 0;
    if (resource == 0) {
        return kinoko_sqrat_raw_set_pair(vm, layer_pair, "resource", null_pair) &&
               kinoko_sqrat_raw_set_pair(vm, script_pair, "resource", null_pair);
    }
    if (!kinoko_get_act_resource_class(vm, resource, resource_class_pair))
        return 0;

    /* 4467E0 -> 448FB0 uses sq_newslot on the outer layer object. */
    if (!kinoko_create_bound_instance(vm, layer_pair, "resource", resource_class_pair, (void*)(uintptr_t)(resource), outer_pair)) {
        kinoko_sqrat_release_pair(vm, outer_pair);
        kinoko_sqrat_release_pair(vm, resource_class_pair);
        return 0;
    }
    kinoko_sqrat_release_pair(vm, outer_pair);

    /* 446920 -> 448910 uses sq_rawset on the script table. */
    if (!kinoko_create_unbound_instance(vm, resource_class_pair, (void*)(uintptr_t)(resource), script_resource_pair) ||
        !kinoko_sqrat_raw_set_pair(vm, script_pair, "resource", script_resource_pair)) {
        kinoko_sqrat_release_pair(vm, script_resource_pair);
        kinoko_sqrat_release_pair(vm, resource_class_pair);
        return 0;
    }
    kinoko_sqrat_release_pair(vm, script_resource_pair);
    kinoko_sqrat_release_pair(vm, resource_class_pair);
    return 1;
}

namespace {
// Own the three temporary Sqrat references across cloning and registration,
// as 450A56..450D61 does. Native ACTs/layers/resources remain borrowed here.
struct StageTables {
    SQVM *vm;
    kinoko::act::LayerObjectRecord act{}, global{}, resources{};
    explicit StageTables(SQVM* machine) : vm(machine) {
        for (auto *object : {&act, &global, &resources}) {
            object->methods = kinoko_act_host_symbols()->sq_object_vtable;
            object->vm = machine; object->value[0] = static_cast<int32_t>(OT_NULL);
        }
    }
    ~StageTables() {
        for (auto *object : {&resources, &global, &act})
            kinoko_sqrat_release_pair(vm, object->value.data());
    }
    bool resolve(KinokoActRuntime* resource, const char *document_name = nullptr) {
        const kinoko::native::RecordView<kinoko::act::RuntimeRecord> runtime(resource);
        kinoko::act::LayerObjectRecord root{act.methods,vm,runtime.get(&kinoko::act::RuntimeRecord::environment),0,{}};
        const auto *name = document_name ? document_name : kinoko_string_data(runtime.bytes(&kinoko::act::RuntimeRecord::name));
        return name && *name && get_pair(reinterpret_cast<void*>(&root), name, act.value.data()) &&
            act.value[0] == static_cast<int32_t>(OT_TABLE) &&
            get_pair(reinterpret_cast<void*>(&act), "global", global.value.data()) && global.value[0] == static_cast<int32_t>(OT_TABLE) &&
            get_pair(reinterpret_cast<void*>(&act), "resource", resources.value.data()) && resources.value[0] == static_cast<int32_t>(OT_TABLE);
    }
};
int publish_stage_objects(KinokoActDocument* act, KinokoActRuntime* runtime, StageTables &tables, int32_t *active_count) {
    using namespace kinoko::act;
    const DocumentView document(act);
    if (active_count) *active_count = 0;
    // Each virtual owns its own registration work. Ignore HRESULTs and re-read
    // the ranges after callbacks, rather than imposing a factory/rollback path.
    for (int32_t index = 0;; ++index) {
        const auto range = document.get(&DocumentRecord::resources);
        if (index >= static_cast<intptr_t>(reinterpret_cast<uintptr_t>(range.end)-reinterpret_cast<uintptr_t>(range.begin))/static_cast<intptr_t>(sizeof(*range.begin))) break;
        auto *resource = kinoko::memory::load<KinokoActResource *>(range.begin + index);
        ResourceMethods(resource).bind_object(&tables.resources, nullptr);
    }
    for (int32_t index = 0;; ++index) {
        const auto range = document.get(&DocumentRecord::layers);
        if (index >= static_cast<intptr_t>(reinterpret_cast<uintptr_t>(range.end)-reinterpret_cast<uintptr_t>(range.begin))/static_cast<intptr_t>(sizeof(*range.begin))) break;
        auto *layer = kinoko::memory::load<KinokoActLayer *>(range.begin + index);
        const auto status = LayerMethods(layer).bind(&tables.act, &tables.global);
        kinoko_trace_i32("act:layer-script-result", status >= 0);
        const kinoko::native::RecordView<LayerKeys> keys(layer);
        auto *head = keys.get(&LayerKeys::key_head);
        auto *key = head && head->next != head && keys.get(&LayerKeys::key_count) &&
            !keys.get(&LayerKeys::extra_count) ? head->next->key : nullptr;
        auto *layout = key ? kinoko::memory::load<LayoutKey>(key).layout : nullptr;
        if (layout) {
            const auto result = LayoutMethods(layout).register_class();
            kinoko_trace_i32("act:layout-register-result", result);
        }
        kinoko_trace_i32("act:layer-published", index);
        if (active_count) ++*active_count;
    }
    return 1;
}
}
int32_t kinoko_publish_act_layers(SQVM* vm, KinokoActDocument* act, KinokoActRuntime* runtime, int32_t *active_count) {
    if (active_count) *active_count = 0;
    if (!vm || !act || !runtime) return 0;
    StageTables tables((SQVM*)(uintptr_t)vm);
    if (!tables.resolve(runtime, kinoko_act_document_name(act))) return 0;
    return publish_stage_objects(act, runtime, tables, active_count);
}

int32_t kinoko_bind_act_resource_object(KinokoActRuntime* resource_ptr)
{
    using namespace kinoko::act;
    kinoko_trace_i32("450950:bind-resource", kinoko::script::diagnostic_address(resource_ptr));
    if (!resource_ptr) return 0;
    const kinoko::native::RecordView<RuntimeRecord> runtime(resource_ptr);
    auto *holder = runtime.get(&RuntimeRecord::source_holder);
    auto *source = holder ? holder->document : nullptr;
    if (!source) return 0;
    auto* copy = kinoko::act::DocumentMethods(source).clone();
    if (!copy) return 0; // retained null-allocation boundary
    auto *previous = runtime.get(&RuntimeRecord::active_document);
    if (previous != copy) delete_document(previous);
    runtime.set(&RuntimeRecord::active_document, copy); // 450B4B, before holder allocation
    auto *new_holder = static_cast<KinokoActSourceHolder*>(std::malloc(sizeof(KinokoActSourceHolder)));
    if (new_holder) new_holder->document = runtime.get(&RuntimeRecord::active_document);
    auto *old_holder = runtime.get(&RuntimeRecord::active_holder);
    if (old_holder != new_holder) std::free(old_holder);
    runtime.set(&RuntimeRecord::active_holder, new_holder);
    kinoko_trace_i32("450950:bind-new-link", kinoko::script::diagnostic_address(new_holder));
    kinoko_trace("450950:bind-link-stored");
    if (!new_holder) return 0; // new document remains runtime-owned on allocation failure
    const DocumentView document(runtime.get(&RuntimeRecord::active_document));
    // Original 44FF90: these are borrowed aliases into the owned clone.
    const StagePropertyAliases aliases{
        reinterpret_cast<int32_t*>(document.bytes(&DocumentRecord::margin_left)),
        reinterpret_cast<int32_t*>(document.bytes(&DocumentRecord::margin_right)),
        reinterpret_cast<int32_t*>(document.bytes(&DocumentRecord::margin_top)),
        reinterpret_cast<int32_t*>(document.bytes(&DocumentRecord::margin_bottom)),
        reinterpret_cast<float*>(document.bytes(&DocumentRecord::offset_x)),
        reinterpret_cast<float*>(document.bytes(&DocumentRecord::offset_y)),
        document.bytes(&DocumentRecord::visible),
        reinterpret_cast<int32_t*>(document.bytes(&DocumentRecord::resolution_ms)),
        reinterpret_cast<int32_t*>(document.bytes(&DocumentRecord::screen_width)),
        reinterpret_cast<int32_t*>(document.bytes(&DocumentRecord::screen_height)),
        reinterpret_cast<kinoko::legacy::StringRecord*>(document.bytes(&DocumentRecord::name))};
    runtime.set(&RuntimeRecord::stage_properties, aliases);
    kinoko_trace("450950:bind-fields-stored");
    return 1;
}

int32_t kinoko_register_runtime_act_script(SQVM* vm, KinokoActRuntime* resource_ptr,
                                                 KinokoActDocument* act)
{
    kinoko::act::LayerObjectRecord root{(const void*)(uintptr_t)(0), vm, {0, 0}, 0, {}};
    int32_t parent[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 }, global[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    kinoko::act::LayerObjectRecord object{(const void*)(uintptr_t)(0), vm, {0, 0}, 0, {}};
    auto* script = ActPublicationView(act).bytes(&ActPublicationRecord::script);
    int32_t result = 0;
    root.value = kinoko::native::RecordView<kinoko::act::RuntimeRecord>(resource_ptr).get(&kinoko::act::RuntimeRecord::environment);
    do {
    if (!get_pair(reinterpret_cast<void*>(&root), kinoko_string_data(ActPublicationView(act).bytes(&ActPublicationRecord::name)), parent))
        break;
    copy_pair(object.value, parent);
    if (!get_pair(reinterpret_cast<void*>(&object), "global", global))
        break;
    object.methods = kinoko_act_host_symbols()->sq_object_vtable;
    copy_pair(object.value, global);
    result = kinoko_register_act_script((void*)(uintptr_t)(script), reinterpret_cast<void*>(&object)) >= 0;
    } while (false);
    kinoko_sqrat_release_pair(vm, global);
    kinoko_sqrat_release_pair(vm, parent);
    return result;
}

int32_t kinoko_begin_stage_this(KinokoActRuntime* resource_ptr, int32_t stage) {
    using namespace kinoko::act;
    SQVM* vm = nullptr;
    int32_t result = static_cast<int32_t>(E_FAIL);
    kinoko_trace_i32("450950:enter-resource", kinoko::script::diagnostic_address(resource_ptr));
    kinoko_trace_i32("450950:enter-stage", stage);
    if (!resource_ptr) return result;
    const kinoko::native::RecordView<RuntimeRecord> runtime(resource_ptr);
    {
        kinoko::windows::CriticalLock lock(reinterpret_cast<CRITICAL_SECTION*>(runtime.bytes(&RuntimeRecord::lock)));
        auto* holder = runtime.get(&RuntimeRecord::source_holder);
        if (holder && holder->document) {
            auto* source = holder->document;
            kinoko::act::DocumentMethods(source).resume_resources();
        }
        vm = runtime.get(&RuntimeRecord::vm);
        StageTables tables(vm);
        if (runtime.get(&RuntimeRecord::stage_active) != 1 && vm &&
            runtime.get(&RuntimeRecord::environment)[0] != static_cast<int32_t>(OT_NULL) && tables.resolve(resource_ptr)) {
            if (stage >= 0) runtime.set(&RuntimeRecord::current_time, stage);
            if (kinoko_bind_act_resource_object(resource_ptr)) {
                runtime.set(&RuntimeRecord::stage_active, uint8_t{1});
                result = 0;
                auto* act = runtime.get(&RuntimeRecord::active_document);
                int32_t layer_count = 0;
                publish_stage_objects(act, resource_ptr, tables, &layer_count);
                kinoko_trace_i32("450950:layer-count", layer_count);
                kinoko_register_act_script(DocumentView(act).bytes(&DocumentRecord::script), &tables.global);
                auto* source = runtime.get(&RuntimeRecord::source_holder)->document;
                kinoko_execute_act_callback(DocumentView(source).bytes(&DocumentRecord::script), offsetof(ScriptStorageRecord,initialize), "act:callback-init");
                for (int32_t index = 0;; ++index) {
                    const DocumentView current(runtime.get(&RuntimeRecord::active_document));
                    const auto layers = current.get(&DocumentRecord::layers);
                    if (!ordered_layers(layers) || index >= layer_distance(layers)) break;
                    auto* layer = layer_at(layers, index);
                    auto* script = layer ? LayerStorageView(layer).bytes(&LayerStorageRecord::script) : nullptr;
                    kinoko_execute_act_callback(script, offsetof(ScriptStorageRecord,initialize), "act:layer-callback-init");
                }
            }
        }
    }
    auto* act = runtime.get(&RuntimeRecord::active_document);
    const char* act_name = act ? kinoko_string_data(DocumentView(act).bytes(&DocumentRecord::name)) : nullptr;
    kinoko::script::diagnostic_name("450950:act-name", act_name);
    kinoko_trace_i32("450950:user", kinoko::script::diagnostic_address(resource_ptr));
    kinoko_trace_i32("450950:resource", kinoko::script::diagnostic_address(resource_ptr));
    kinoko_trace_i32("450950:vm", kinoko::script::diagnostic_address(vm));
    kinoko_trace_i32("450950:result", result);
    if (result == 0) {
        kinoko_trace_i32("450950:act", kinoko::script::diagnostic_address(runtime.get(&RuntimeRecord::source_holder)));
        // Trace originally reads the active byte together with the following padding.
        kinoko_trace_i32("450950:active", kinoko::memory::load<int32_t>(runtime.bytes(&RuntimeRecord::stage_active)));
    }
    return result;
}

int32_t kinoko_root_table_register_resource(void* root_object,
                                                    KinokoActRuntime* resource_ptr)
{
    int32_t act_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t global_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t resource_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    int32_t player_pair[sizeof(HSQOBJECT)/sizeof(int32_t)] = { static_cast<int32_t>(OT_NULL), 0 };
    KinokoActSourceHolder* holder;
    KinokoActDocument* act;
    ActPublicationView act_view(nullptr);
    const char *act_name;
    const char *script_path;
    kinoko::act::LayerObjectRecord global_object{(const void*)(uintptr_t)(0), (SQVM*)(uintptr_t)(0), {static_cast<int32_t>(OT_NULL), 0}, 1, {}};
    void* script_ptr;
    int32_t result = 0;
    SQVM* vm;

    if (root_object == 0 || resource_ptr == 0)
        return 0;
    vm = kinoko::act::LayerObjectView(root_object).get(&kinoko::act::LayerObjectRecord::vm);
    if (vm == 0)
        return 0;
    kinoko_trace_i32("450f30:root", kinoko::script::diagnostic_address(root_object));
    kinoko_trace_i32("450f30:resource", kinoko::script::diagnostic_address(resource_ptr));
    kinoko_trace_i32("450f30:vm", kinoko::script::diagnostic_address(vm));

    do {
    if (!kinoko_bind_act_resource_root((void*)(uintptr_t)(resource_ptr), vm, reinterpret_cast<const int32_t*>(kinoko::act::LayerObjectView(root_object).bytes(&kinoko::act::LayerObjectRecord::value)))) {
        kinoko_trace("450f30:root-bind-failed");
        break;
    }
    if (!kinoko_publish_cact_resource2d_class(vm, reinterpret_cast<void*>(root_object))) {
        kinoko_trace("450f30:resource2d-class-failed");
        break;
    }
    holder = kinoko::native::RecordView<kinoko::act::RuntimeRecord>(resource_ptr).get(&kinoko::act::RuntimeRecord::source_holder);
    act = holder ? holder->document : nullptr;
    if (act == 0)
        break;
    act_view = ActPublicationView(act);
    act_name = kinoko_string_data(act_view.bytes(&ActPublicationRecord::name));
    if (act_name == nullptr || *act_name == 0)
        break;
    /* 451022..45104A retains the registration key for 4513F0 teardown. */
    kinoko_string_assign_cstr(reinterpret_cast<int32_t*>(kinoko::native::RecordView<kinoko::act::RuntimeRecord>(resource_ptr).bytes(&kinoko::act::RuntimeRecord::name)), act_name);
    kinoko::script::diagnostic_name("450f30:act-name", act_name);

    if (!kinoko_publish_cact_layer_class(vm, reinterpret_cast<void*>(root_object)) ||
        !kinoko_publish_acting_player_class(vm, reinterpret_cast<void*>(root_object)) ||
        !kinoko_sqrat_new_table(vm, act_pair) ||
        !kinoko_sqrat_set_pair(vm, reinterpret_cast<const int32_t*>(kinoko::act::LayerObjectView(root_object).bytes(&kinoko::act::LayerObjectRecord::value)), act_name, act_pair) ||
        !kinoko_sqrat_new_table(vm, global_pair) ||
        !kinoko_sqrat_new_table(vm, resource_pair))
        break;

    if (std::strcmp(act_name, "Fader2") == 0 &&
        global_pair[0] == 0x0A000020 && pair_data(global_pair) != 0 &&
        !kinoko_is_release_watch_data(pair_data(global_pair)) &&
        kinoko_release_watch_count < 8) {
        kinoko_release_watch_data[kinoko_release_watch_count++] =
            pair_data(global_pair);
        kinoko::script::diagnostic_name("sq-watch:fader-global-name", act_name);
        kinoko_trace_i32("sq-watch:fader-global-data", pair_data(global_pair));
        kinoko_trace_i32("sq-watch:fader-global-internal",
                         static_cast<int32_t>(_table(kinoko::script::pair::read(global_pair))->_uiRef));
        kinoko_trace_ref_watch("fader-global-created",
                               kinoko_primary_shared_state,
                               global_pair[0], pair_data(global_pair));

    }

    kinoko_sqrat_bind_string(vm, act_pair, "stName", act_name);
    kinoko_sqrat_bind_int(vm, act_pair, "resolutionMs", act_view.get(&ActPublicationRecord::resolution_ms));
    kinoko_sqrat_bind_int(vm, act_pair, "screenWidth", act_view.get(&ActPublicationRecord::screen_width));
    kinoko_sqrat_bind_int(vm, act_pair, "screenHeight", act_view.get(&ActPublicationRecord::screen_height));
    if (!kinoko_sqrat_set_pair(vm, act_pair, "global", global_pair) ||
        !kinoko_sqrat_set_pair(vm, act_pair, "resource", resource_pair) ||
        !kinoko_sqrat_set_pair(vm, global_pair, "thisAct", act_pair) ||
        !kinoko_sqrat_set_delegate(vm, global_pair, act_pair) ||
        !kinoko_publish_act_script_constants(vm, global_pair))
        break;

    if (!kinoko_publish_acting_player(vm, act_pair, "pl", resource_ptr, player_pair))
        break;
    kinoko_sqrat_release_pair(vm, player_pair);
    if (!kinoko_publish_acting_player(vm, act_pair, "player", resource_ptr, player_pair))
        break;
    kinoko_sqrat_release_pair(vm, player_pair);

    script_ptr = act_view.bytes(&ActPublicationRecord::script);
    script_path = kinoko_string_data(ScriptPublicationView(script_ptr).bytes(&ScriptPublicationRecord::path));
    if (script_path && *script_path)
        kinoko::script::diagnostic_name("450f30:script-path", script_path);
    global_object.methods = kinoko_act_host_symbols()->sq_object_vtable;
    global_object.vm = vm;
    copy_pair(global_object.value, global_pair);
    {
        // 451214 ignores Register's HRESULT, then invokes any captured OnCreate.
        const auto registered = kinoko_register_act_script((void*)(uintptr_t)(script_ptr), reinterpret_cast<void*>(&global_object));
        kinoko_trace_i32("450f30:embedded-script-result", registered >= 0);
        kinoko_execute_act_callback((void*)(uintptr_t)(script_ptr), offsetof(kinoko::act::ScriptStorageRecord,release), "act:callback-oncreate");
    }

    /* 466100 only publishes the ACT.  BeginStage is a script-facing
       operation; forcing it here activates PlayerStatus/StageClear before
       the scene selects them and makes them cover TitleMenu. */
    kinoko_trace("act:published-without-begin-stage");
    result = 1;
    } while (false);


    if (player_pair[0] != static_cast<int32_t>(OT_NULL))
        kinoko_sqrat_release_pair(vm, player_pair);
    if (resource_pair[0] != static_cast<int32_t>(OT_NULL))
        kinoko_sqrat_release_pair(vm, resource_pair);
    if (global_pair[0] != static_cast<int32_t>(OT_NULL))
        kinoko_sqrat_release_pair(vm, global_pair);
    if (act_pair[0] != static_cast<int32_t>(OT_NULL))
        kinoko_sqrat_release_pair(vm, act_pair);
    return result;
}

int32_t kinoko_root_table_construct_this(KinokoActRuntime* resource_ptr,
                                                 SQVM* vm,
                                                 void* output_ptr)
{
    kinoko::act::LayerObjectRecord root_object{(const void*)(uintptr_t)(0), (SQVM*)(uintptr_t)(0), {static_cast<int32_t>(OT_NULL), 0}, 0, {}};
    int32_t result;

    if (resource_ptr == 0 || vm == 0)
        return (int32_t)0x80070057u;
    if (!kinoko_sqrat_root_construct(&root_object, vm))
        return (int32_t)0x80004005u;

    /* 450E30 accepts an optional pre-existing Sqrat object only to verify
       that it belongs to the same VM.  The normal loader passes nullptr. */
    if (output_ptr != 0 &&
        kinoko::act::LayerObjectView(output_ptr).get(&kinoko::act::LayerObjectRecord::vm) != vm) {
        kinoko_sqrat_object_release((void *)(&root_object));
        return (int32_t)0x80070057u;
    }

    result = kinoko_root_table_register_resource(reinterpret_cast<void*>(&root_object), (KinokoActRuntime*)(uintptr_t)(resource_ptr));
    kinoko_sqrat_object_release((void *)(&root_object));
    return result;
}

extern "C" int32_t __fastcall kinoko_method_bind_chip_object(KinokoActResource* resource, void *, void* object, const char *name) {
    return kinoko_bind_original_resource(resource, object, name, "CActResourceChip", false);
}

extern "C" int32_t __fastcall kinoko_method_bind_chip_table(KinokoActResource* resource, void *, void* object, const char *name) {
    return kinoko_bind_original_resource(resource, object, name, "CActResourceChip", true);
}

extern "C" int32_t __fastcall kinoko_method_bind_texture_object(KinokoActResource* resource, void *, void* object, const char *name) {
    return kinoko_bind_original_resource(resource, object, name, "CActResource2D", false);
}

extern "C" int32_t __fastcall kinoko_method_bind_texture_table(KinokoActResource* resource, void *, void* object, const char *name) {
    return kinoko_bind_original_resource(resource, object, name, "CActResource2D", true);
}

extern "C" int32_t __fastcall kinoko_method_bind_render_target_object(KinokoActResource* resource, void *, void* object, const char *name) {
    return kinoko_bind_original_resource(resource, object, name, "CActRenderTarget", false);
}

extern "C" int32_t __fastcall kinoko_method_bind_render_target_table(KinokoActResource* resource, void *, void* object, const char *name) {
    return kinoko_bind_original_resource(resource, object, name, "CActRenderTarget", true);
}

// Recovered 445530/455330 method bridges. Source Squirrel owns captures and
// values; preserve the existing trace calls and explicit x86 member dispatch.
extern "C" intptr_t kinoko_sqrat_call_integer0(struct SQVM* a1) {
    void* method_holder = nullptr;
    void* instance = nullptr;
    void* method;
    int32_t result;

    if (sq_getuserdata(a1, -1, (SQUserPointer*)(&method_holder), (SQUserPointer*)nullptr) < 0 ||
        method_holder == 0 || kinoko::memory::load<void*>(method_holder) == 0 ||
        sq_getinstanceup(a1, 1, (SQUserPointer*)(&instance), nullptr) < 0)
        return 0;
    method = kinoko::memory::load<void*>(method_holder);
    result = kinoko::method::invoke<int32_t>(
        (void *)(intptr_t)instance, (void *)(intptr_t)method);
    sq_pushinteger(a1, result);
    return 1;
}

extern "C" intptr_t kinoko_sqrat_call_integer1(struct SQVM* a1) {
    static volatile LONG trace_count;
    void* outer_payload = nullptr;
    void* instance_ptr = nullptr;
    int32_t argument = 0;
    int32_t outer_status;
    int32_t instance_status;
    int32_t argument_status;
    int32_t result;
    void* method;
    LONG trace_index;

    /* Original 455330 is a Sqrat native wrapper.  The first outer value is
       the userdata payload containing the target method; the first script
       argument is the class instance and the second is the integer argument.
       RetDec dropped the indirect __thiscall and returned the conversion
       status from 48A7D0 instead. */
    trace_index = InterlockedIncrement(&trace_count);
    if (trace_index <= 128)
        kinoko_trace_i32("450950:wrapper-entry", kinoko::script::diagnostic_address(a1));
    outer_status = sq_getuserdata(a1, -1, (SQUserPointer*)(&outer_payload), (SQUserPointer*)nullptr);
    if (trace_index <= 128) {
        kinoko_trace_i32("450950:wrapper-top", sq_gettop(a1));
        kinoko_trace_i32("450950:wrapper-outer-status", outer_status);
        kinoko_trace_i32("450950:wrapper-outer", kinoko::script::diagnostic_address(outer_payload));
    }
    if (outer_payload == 0 || kinoko::memory::load<void*>(outer_payload) == 0)
        return 0;
    method = kinoko::memory::load<void*>(outer_payload);
    instance_status = sq_getinstanceup(a1, 1, (SQUserPointer*)(&instance_ptr), nullptr);
    argument_status = get_game_integer(a1, 2, &argument);
    if (argument_status < 0)
        return 0;
    if (trace_index <= 128) {
        kinoko_trace_i32("450950:wrapper-method", kinoko::script::diagnostic_address(method));
        kinoko_trace_i32("450950:wrapper-instance-status", instance_status);
        kinoko_trace_i32("450950:wrapper-instance", kinoko::script::diagnostic_address(instance_ptr));
        kinoko_trace_i32("450950:wrapper-argument-status", argument_status);
        kinoko_trace_i32("450950:wrapper-argument", argument);
        if (method == reinterpret_cast<void*>(kinoko_method_begin_stage))
            kinoko_trace("450950:wrapper-begin-stage");
    }
    result = kinoko::method::invoke<int32_t>(
        (void *)(intptr_t)instance_ptr,
        (void *)(intptr_t)method,
        argument);
    sq_pushinteger(a1, result);
    return 1;
}


namespace {
intptr_t string_property_set(SQVM* vm) {
    int32_t offset=0;
    auto* object=static_cast<unsigned char*>(kinoko_c2dlayout_property_offset(vm, &offset));
    if(!object) return 0;
    SQInteger value=0;
    if(!kinoko::script::upstream::sqrat_integer_argument(vm,2,value)) return 0;
    using Text=kinoko::text::StringLayout;
    if(offset==offsetof(Text,font_height)) value=std::clamp<SQInteger>(value,1,127);
    else if(offset==offsetof(Text,font_weight)) value=(std::max<SQInteger>)(value,1);
    else if(offset==offsetof(Text,red) || offset==offsetof(Text,green) || offset==offsetof(Text,blue) ||
        offset==offsetof(Text,base_red) || offset==offsetof(Text,base_green) || offset==offsetof(Text,base_blue)) value=std::clamp<SQInteger>(value,0,255);
    else if(offset==offsetof(Text,character_space) || offset==offsetof(Text,line_space)) value=(std::max<SQInteger>)(value,0);
    kinoko::memory::store(object+offset, static_cast<int32_t>(value));
    return 0;
}
intptr_t string_value_get(SQVM* vm) {
    int32_t offset=0;
    auto* object=static_cast<unsigned char*>(kinoko_c2dlayout_property_offset(vm, &offset));
    if(!object) return 0;
    sq_pushstring(vm,kinoko::legacy::StringView(object+offset).data(),-1);
    return 1;
}
intptr_t string_face_set(SQVM* vm) {
    int32_t offset=0;
    auto* object=static_cast<unsigned char*>(kinoko_c2dlayout_property_offset(vm, &offset));
    const SQChar* value=nullptr;
    if(!object || SQ_FAILED(sq_getstring(vm,2,&value))) return 0;
    static const char face[]="\x82\x6c\x82\x72\x20\x83\x53\x83\x56\x83\x62\x83\x4e";
    if(!*value) value=face;
    kinoko::legacy::StringView(object+offsetof(kinoko::text::StringLayout, face)).assign(value,static_cast<uint32_t>(std::strlen(value)));
    return 0;
}
template<int Method> intptr_t string_method(SQVM* vm) {
    const auto machine=vm;
    KinokoStringLayout* object=nullptr;
    if(SQ_FAILED(sq_getinstanceup(machine,1,reinterpret_cast<SQUserPointer*>(&object),nullptr)) || !object)
        return sq_throwerror(machine,"invalid CStringLayout receiver");
    if constexpr(Method==0 || Method==2 || Method==3 || Method==4 || Method==5)
        if(sq_gettop(machine)<2) return sq_throwerror(machine,"missing CStringLayout argument");
    try {
        int32_t result=0;
        if constexpr(Method==0 || Method==4) {
            const SQChar* text=nullptr;
            if(sq_gettype(machine,2)!=OT_NULL && SQ_FAILED(sq_getstring(machine,2,&text)))
                return sq_throwerror(machine,"expected text");
            if constexpr(Method==0) result=kinoko_string_push_back((KinokoStringLayout*)(uintptr_t)(object), text);
            else { sq_pushinteger(machine,kinoko_string_character_bytes(text));return 1; }
        } else if constexpr(Method==1) result=kinoko_string_clear((KinokoStringLayout*)(uintptr_t)(object));
        else if constexpr(Method==2 || Method==3) {
            SQInteger count=0;
            if(!kinoko::script::upstream::sqrat_integer_argument(machine,2,count)) return sq_throwerror(machine,"expected count");
            result=kinoko_string_pop((KinokoStringLayout*)(uintptr_t)(object), count, Method==2);
        } else if constexpr(Method==5) {
            KinokoStringLayout* source=nullptr;
            if(sq_gettype(machine,2)!=OT_NULL && SQ_FAILED(sq_getinstanceup(machine,2,reinterpret_cast<SQUserPointer*>(&source),nullptr)))
                return sq_throwerror(machine,"expected CStringLayout");
            result=kinoko_string_replicate((KinokoStringLayout*)(uintptr_t)(object), (KinokoStringLayout*)(uintptr_t)(source));
        } else result=kinoko_string_mark_rebuild((KinokoStringLayout*)(uintptr_t)(object));
        sq_pushbool(machine,result!=0);return 1;
    } catch(...) { return sq_throwerror(machine,"CStringLayout allocation failed"); }
}
}

// 43EDA0 publishes a non-owning Sqrat class. Actual method/descriptor creation
// uses the vendored Sqrat bridge; no original code addresses or class registry.
extern "C" int32_t kinoko_publish_string_layout_class(SQVM* vm,void* root,int32_t* out) {
    if(!vm || !root || !out) return 0;
    if(get_pair((void*)(uintptr_t)(root), "CStringLayout", out) && out[0]==0x08004000) return 1;
    kinoko_sqrat_release_pair(vm, out);
    const int32_t top=sq_gettop(vm);
    int32_t setters[sizeof(HSQOBJECT)/sizeof(int32_t)]={static_cast<int32_t>(OT_NULL),0},getters[sizeof(HSQOBJECT)/sizeof(int32_t)]={static_cast<int32_t>(OT_NULL),0};
    bool ok=kinoko_sqrat_new_class(vm, out) && kinoko_sqrat_new_table(vm, setters) && kinoko_sqrat_new_table(vm, getters) &&
        initialize_native_property_class(vm, out, {setters, getters});
    struct Property { const char* name; int32_t offset,kind; bool readonly; };
    using Text=kinoko::text::StringLayout;
    static const Property properties[]={
        {"alpha",offsetof(Text,alpha),1,false},{"blend",offsetof(Text,blend),0,false},{"alignment",offsetof(Text,alignment),0,false},
        {"scaleX",offsetof(Text,scale_x),1,false},{"scaleY",offsetof(Text,scale_y),1,false},{"wordBreakWidth",offsetof(Text,wrap_width),0,false},
        {"stText",offsetof(Text,text),3,false},{"stFontFaceName",offsetof(Text,face),3,false},{"fontHeight",offsetof(Text,font_height),0,false},
        {"fontWeight",offsetof(Text,font_weight),0,false},{"colorR",offsetof(Text,red),0,false},{"colorG",offsetof(Text,green),0,false},
        {"colorB",offsetof(Text,blue),0,false},{"baseR",offsetof(Text,base_red),0,false},{"baseG",offsetof(Text,base_green),0,false},{"baseB",offsetof(Text,base_blue),0,false},
        {"charactorSpace",offsetof(Text,character_space),0,false},{"lineSpace",offsetof(Text,line_space),0,false},{"addEdge",offsetof(Text,edge),2,false},
        {"cursorX",offsetof(Text,cursor_x),0,true},{"cursorY",offsetof(Text,cursor_y),0,true},{"queueCount",offsetof(Text,pending)+offsetof(kinoko::legacy::StringRecord,length),0,true}
    };
    for(const auto& p:properties) {
        const auto getter=p.kind==1?reinterpret_cast<void*>(kinoko_c2dlayout_get_float):p.kind==2?reinterpret_cast<void*>(kinoko_cact_layer_get_bool):
            p.kind==3?reinterpret_cast<void*>(string_value_get):reinterpret_cast<void*>(kinoko_c2dlayout_get_int);
        const auto setter=p.kind==1?reinterpret_cast<void*>(kinoko_c2dlayout_set_float):p.kind==2?reinterpret_cast<void*>(kinoko_cact_layer_set_bool):
            p.kind==3?(p.offset==offsetof(Text,face)?reinterpret_cast<void*>(string_face_set):reinterpret_cast<void*>(kinoko_cact_layer_set_string)):reinterpret_cast<void*>(string_property_set);
        if(ok) ok=kinoko_sqrat_set_offset_closure(vm, getters, p.name, p.offset, (void *)(intptr_t)(getter)) &&
            (p.readonly || kinoko_sqrat_set_offset_closure(vm, setters, p.name, p.offset, (void *)(intptr_t)(setter)));
    }
    struct Method { const char* name; intptr_t (*call)(SQVM*); };
    static const Method methods[]={
        {"PushBack",string_method<0>},{"Clear",string_method<1>},{"PopFront",string_method<2>},
        {"PopBack",string_method<3>},{"GetCharacterBytes",string_method<4>},
        {"ReplicateText",string_method<5>},{"Rebuild",string_method<6>}
    };
    for(const auto& m:methods) if(ok) ok=kinoko_sqrat_set_native_closure(vm, out, m.name, reinterpret_cast<void*>(m.call), nullptr, 0);
    if(ok) ok=kinoko_sqrat_set_pair(vm, reinterpret_cast<const int32_t*>(kinoko::act::LayerObjectView(root).bytes(&kinoko::act::LayerObjectRecord::value)), "CStringLayout", out);
    kinoko_sqrat_release_pair(vm, getters);kinoko_sqrat_release_pair(vm, setters);
    kinoko_sqrat_trim_stack(vm, top);
    if(!ok) kinoko_sqrat_release_pair(vm, out);
    return ok;
}

extern "C" int32_t __fastcall kinoko_method_register_string_layout(KinokoStringLayout* layout,void*) {
    auto* layer=layout ? reinterpret_cast<kinoko::text::StringLayout*>(layout)->layer : nullptr;
    if(!layer) return E_FAIL;
    const kinoko::act::LayerStorageView storage(layer);
    const auto wrapper=storage.view(&kinoko::act::LayerStorageRecord::layout_object);
    const auto* pair=reinterpret_cast<const int32_t*>(wrapper.bytes(&kinoko::act::LayerObjectRecord::value));
    if(pair[0]==0x01000001) return E_FAIL;
    SQVM* const vm = wrapper.get(&kinoko::act::LayerObjectRecord::vm);
    if(!vm) return E_INVALIDARG;
    kinoko::act::LayerObjectRecord root{}; int32_t klass[sizeof(HSQOBJECT)/sizeof(int32_t)]={static_cast<int32_t>(OT_NULL),0},outer[sizeof(HSQOBJECT)/sizeof(int32_t)]={static_cast<int32_t>(OT_NULL),0},script[sizeof(HSQOBJECT)/sizeof(int32_t)]={static_cast<int32_t>(OT_NULL),0};
    if(!kinoko_sqrat_root_construct(&root, vm)) return E_FAIL;
    const bool ok=kinoko_publish_string_layout_class(vm, reinterpret_cast<void*>(&root), klass) &&
        kinoko_create_unbound_instance(vm, klass, layout, outer) &&
        kinoko_sqrat_raw_set_pair(vm, pair, "layout", outer) &&
        kinoko_create_bound_instance(vm, reinterpret_cast<const int32_t*>(storage.view(&kinoko::act::LayerStorageRecord::script_object).bytes(&kinoko::act::LayerObjectRecord::value)), "layout", klass, layout, script);
    kinoko_sqrat_release_pair(vm, script);kinoko_sqrat_release_pair(vm, outer);
    kinoko_sqrat_release_pair(vm, klass);kinoko_sqrat_object_release((void *)(&root));
    if(!ok) return E_FAIL;
    using namespace kinoko::act;
    const auto aliases = LayerStorageView(layer).view(&LayerStorageRecord::association)
        .view(&LayerAssociationRecord::property_aliases);
    auto& text=*reinterpret_cast<kinoko::text::StringLayout*>(layout);
    aliases.set(&LayerPropertyAliases::alpha, reinterpret_cast<float*>(&text.alpha));
    aliases.set(&LayerPropertyAliases::blend, reinterpret_cast<int32_t*>(&text.blend));
    aliases.set(&LayerPropertyAliases::red, reinterpret_cast<int32_t*>(&text.red));
    aliases.set(&LayerPropertyAliases::green, reinterpret_cast<int32_t*>(&text.green));
    aliases.set(&LayerPropertyAliases::blue, reinterpret_cast<int32_t*>(&text.blue));
    return 0;
}

namespace {
intptr_t mesh_replace_texture(SQVM* vm) {
    kinoko::mesh::Resource *resource=nullptr;
    KinokoActResource *texture=nullptr;
    const SQChar *name=nullptr;
    auto *machine=vm;
    if(SQ_FAILED(sq_getinstanceup(machine,1,reinterpret_cast<SQUserPointer*>(&resource),nullptr)) ||
        SQ_FAILED(sq_getstring(machine,2,&name)) ||
        SQ_FAILED(sq_getinstanceup(machine,3,reinterpret_cast<SQUserPointer*>(&texture),nullptr)) || !resource) return 0;
    sq_pushinteger(machine,kinoko::mesh::replace_texture(resource,name,texture));return 1;
}
}
extern "C" int32_t kinoko_publish_mesh_resource_class(SQVM* vm,void* root,int32_t *out) {
    if(get_pair((void*)(uintptr_t)(root), "CActResourceMesh", out) && out[0]==0x08004000) return 1;
    kinoko_sqrat_release_pair(vm, out);
    static const kinoko_native_view_property properties[]={
        {"resourceID",offsetof(kinoko::mesh::Resource,id),0},{"stName",offsetof(kinoko::mesh::Resource,name),4},{"stMeshName",offsetof(kinoko::mesh::Resource,mesh_name),4}
    };
    return kinoko_publish_map_view_class(vm, (void*)(uintptr_t)(root), "CActResourceMesh", properties, 3, 0, out) &&
        kinoko_sqrat_set_native_closure(vm, out, "LoadMesh", reinterpret_cast<void*>(kinoko_resource_load_texture), nullptr, 0) &&
        kinoko_sqrat_set_native_closure(vm, out, "SetReplaceTexture", reinterpret_cast<void*>(mesh_replace_texture), nullptr, 0);
}
extern "C" int32_t __fastcall kinoko_method_register_mesh_resource(void*,void *,SQVM* vm) {
    if(!vm) return E_INVALIDARG;
    kinoko::act::LayerObjectRecord root{}; int32_t klass[sizeof(HSQOBJECT)/sizeof(int32_t)]={static_cast<int32_t>(OT_NULL),0};
    if(!kinoko_sqrat_root_construct(&root, vm)) return E_FAIL;
    const auto ok=kinoko_publish_mesh_resource_class(vm, reinterpret_cast<void*>(&root), klass);
    kinoko_sqrat_release_pair(vm, klass);kinoko_sqrat_object_release((void *)(&root));
    return ok?S_OK:E_FAIL;
}
extern "C" int32_t __fastcall kinoko_method_bind_mesh_object(KinokoActResource* resource,void *,void* object,const char *name) {
    return kinoko_bind_original_resource(resource,object,name,"CActResourceMesh",false);
}
extern "C" int32_t __fastcall kinoko_method_bind_mesh_table(KinokoActResource* resource,void *,void* object,const char *name) {
    return kinoko_bind_original_resource(resource,object,name,"CActResourceMesh",true);
}
extern "C" int32_t __fastcall kinoko_method_register_layout_3d(KinokoActLayout* layout,void *) {
    auto* layer=layout ? kinoko::native::RecordView<kinoko::act::Layout3DRecord>(layout).get(&kinoko::act::Layout3DRecord::layer) : nullptr;
    if(!layer) return E_FAIL;
    const kinoko::act::LayerStorageView storage(layer);
    const auto wrapper=storage.view(&kinoko::act::LayerStorageRecord::layout_object);
    const auto* pair=reinterpret_cast<const int32_t*>(wrapper.bytes(&kinoko::act::LayerObjectRecord::value));
    if(pair[0]==0x01000001) return E_FAIL;
    const auto vm = wrapper.get(&kinoko::act::LayerObjectRecord::vm);
    if(!vm) return E_INVALIDARG;
    kinoko::act::LayerObjectRecord root{}; int32_t klass[sizeof(HSQOBJECT)/sizeof(int32_t)]={static_cast<int32_t>(OT_NULL),0},outer[sizeof(HSQOBJECT)/sizeof(int32_t)]={static_cast<int32_t>(OT_NULL),0},script[sizeof(HSQOBJECT)/sizeof(int32_t)]={static_cast<int32_t>(OT_NULL),0};
    if(!kinoko_sqrat_root_construct(&root, vm)) return E_FAIL;
    // 43C2B0 spells the translation-Z script property "coS_z". Do not invent
    // a trans_z alias or copy the serialized trans/roll offset alias into SQ.
    static const kinoko_native_view_property properties[]={
        {"trans_x",offsetof(kinoko::act::Layout3DRecord,translation)+0,1},{"trans_y",offsetof(kinoko::act::Layout3DRecord,translation)+4,1},{"coS_z",offsetof(kinoko::act::Layout3DRecord,translation)+8,1},
        {"roll_x",offsetof(kinoko::act::Layout3DRecord,rotation)+0,1},{"roll_y",offsetof(kinoko::act::Layout3DRecord,rotation)+4,1},{"roll_z",offsetof(kinoko::act::Layout3DRecord,rotation)+8,1},
        {"scale_x",offsetof(kinoko::act::Layout3DRecord,scale)+0,1},{"scale_y",offsetof(kinoko::act::Layout3DRecord,scale)+4,1},{"scale_z",offsetof(kinoko::act::Layout3DRecord,scale)+8,1}
    };
    const bool ok=kinoko_publish_map_view_class(vm, reinterpret_cast<void*>(&root), "C3DLayout", properties, 9, 0, klass) &&
        kinoko_create_unbound_instance(vm, klass, layout, outer) &&
        kinoko_sqrat_raw_set_pair(vm, pair, "layout", outer) &&
        kinoko_create_bound_instance(vm, reinterpret_cast<const int32_t*>(storage.view(&kinoko::act::LayerStorageRecord::script_object).bytes(&kinoko::act::LayerObjectRecord::value)), "layout", klass, layout, script);
    kinoko_sqrat_release_pair(vm, script);kinoko_sqrat_release_pair(vm, outer);
    kinoko_sqrat_release_pair(vm, klass);kinoko_sqrat_object_release((void *)(&root));
    return ok?S_OK:E_FAIL;
}
