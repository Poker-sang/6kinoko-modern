#pragma once
#include "kinoko/act_types.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
struct SQVM;
struct KinokoArchiveReader;

#include "kinoko/method_entry.hpp"
namespace kinoko::act {
// Read the table on every call: callbacks may replace it.
namespace method_boundary { using kinoko::method::entry; }
enum class TypeMethod { layout, resource };
enum class ResourceKind { texture, render_target, mesh, chip };
struct ResourceTypeDescriptor { const void* methods; const void* cache; char name[40]; };
inline const ResourceTypeDescriptor& resource_type_descriptor(ResourceKind kind) {
    static const ResourceTypeDescriptor types[] = {
        {nullptr, nullptr, ".?AVCActResource2D@@"}, {nullptr, nullptr, ".?AVCActRenderTarget@@"},
        {nullptr, nullptr, ".?AVCActResourceMesh@@"}, {nullptr, nullptr, ".?AVCActResourceChip@@"}
    };
    return types[static_cast<unsigned>(kind)];
}
class SerializableMethods {
protected:
    void* object_;
    template<class Fn> Fn entry(std::size_t slot) const noexcept { return method_boundary::entry<Fn>(object_,slot); }
public:
    explicit SerializableMethods(void* object) : object_(object) {}
    bool query_type(const void* descriptor, void** output) const {
        using Fn = std::uint8_t (KINOKO_METHOD_ENTRY *)(void*,void*,const void*,void*);
        return entry<Fn>(2)(object_,nullptr,descriptor,output) != 0;
    }
    void dispose() const {
        using Fn = void (KINOKO_METHOD_ENTRY *)(void*,void*);
        if (object_) entry<Fn>(3)(object_,nullptr);
    }
    void* delete_object(unsigned char flags) const {
        using Fn = void* (KINOKO_METHOD_ENTRY *)(void*,void*,unsigned char);
        return object_ ? entry<Fn>(4)(object_,nullptr,flags) : nullptr;
    }
    std::uint8_t read(KinokoArchiveReader** reader, std::int32_t version) const {
        using Fn = std::uint8_t (KINOKO_METHOD_ENTRY *)(void*,void*,KinokoArchiveReader**,std::int32_t);
        return entry<Fn>(1)(object_,nullptr,reader,version);
    }
    bool type_name(TypeMethod family, void* output) const {
        using GetType = void* (KINOKO_METHOD_ENTRY *)(void*,void*);
        using GetName = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*,void*);
        const auto get_type = entry<GetType>(family == TypeMethod::layout ? 4 : 5);
        auto* binder = get_type ? get_type(object_,nullptr) : nullptr;
        const auto get_name = method_boundary::entry<GetName>(binder,1);
        if (!get_name) return false;
        get_name(binder,nullptr,output); return true;
    }
    std::int32_t write(KinokoArchiveReader* writer) const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*,KinokoArchiveReader*);
        return entry<Fn>(0)(object_,nullptr,writer);
    }
};
class ResourceMethods : public SerializableMethods {
public:
    explicit ResourceMethods(KinokoActResource* resource) : SerializableMethods(resource) {}
    KinokoActResource* query(ResourceKind kind) const {
        void* result = nullptr;
        return query_type(&resource_type_descriptor(kind), &result) ? static_cast<KinokoActResource*>(result) : nullptr;
    }
    KinokoActResource* clone() const {
        using Fn = KinokoActResource* (KINOKO_METHOD_ENTRY *)(void*,void*);
        return entry<Fn>(9)(object_,nullptr);
    }
    std::uint8_t load(const char* prefix) const {
        using Fn = std::uint8_t (KINOKO_METHOD_ENTRY *)(void*,void*,const char*);
        return entry<Fn>(10)(object_,nullptr,prefix);
    }
    std::uint8_t unload() const {
        using Fn = std::uint8_t (KINOKO_METHOD_ENTRY *)(void*,void*);
        return entry<Fn>(11)(object_,nullptr);
    }
    std::uint8_t create_target(std::int32_t width, std::int32_t height) const {
        using Fn = std::uint8_t (KINOKO_METHOD_ENTRY *)(void*,void*,std::int32_t,std::int32_t);
        return entry<Fn>(12)(object_,nullptr,width,height);
    }
    std::int32_t register_class(SQVM* vm) const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*,SQVM*);
        return entry<Fn>(6)(object_,nullptr,vm);
    }
    std::int32_t bind_table(void* table, const char* name) const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*,void*,const char*);
        return entry<Fn>(7)(object_,nullptr,table,name);
    }
    std::int32_t bind_object(void* object, const char* name) const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*,void*,const char*);
        return entry<Fn>(8)(object_,nullptr,object,name);
    }
};
class DocumentMethods : public SerializableMethods {
public:
    explicit DocumentMethods(KinokoActDocument* value) : SerializableMethods(value) {}
    std::uint8_t load_resources(const char* prefix) const {
        using Fn = std::uint8_t (KINOKO_METHOD_ENTRY *)(void*,void*,const char*);
        return entry<Fn>(6)(object_,nullptr,prefix);
    }
    KinokoActDocument* clone() const {
        using Fn = KinokoActDocument* (KINOKO_METHOD_ENTRY *)(void*,void*);
        return entry<Fn>(5)(object_,nullptr);
    }
    std::int32_t suspend_resources() const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*);
        return entry<Fn>(7)(object_,nullptr);
    }
    std::int32_t resume_resources() const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*);
        return entry<Fn>(8)(object_,nullptr);
    }
};
class LayoutMethods : public SerializableMethods {
public:
    explicit LayoutMethods(KinokoActLayout* value) : SerializableMethods(value) {}
    KinokoActLayout* clone() const {
        using Fn = KinokoActLayout* (KINOKO_METHOD_ENTRY *)(void*,void*);
        return entry<Fn>(5)(object_,nullptr);
    }
    std::int32_t set_layer(KinokoActLayer* layer) const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*,KinokoActLayer*);
        return entry<Fn>(6)(object_,nullptr,layer);
    }
    std::int32_t update() const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*);
        const auto fn = entry<Fn>(7); return fn ? fn(object_,nullptr) : 0;
    }
    std::int32_t draw(float x, float y) const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*,float,float);
        const auto fn = entry<Fn>(8); return fn ? fn(object_,nullptr,x,y) : static_cast<std::int32_t>(0x80004005u);
    }
    std::int32_t register_class() const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*);
        return entry<Fn>(9)(object_,nullptr);
    }
};
// Keys and timelines share the clone slot but have different payloads.
class KeyMethods : public SerializableMethods {
public:
    explicit KeyMethods(void* value) : SerializableMethods(value) {}
    void* clone() const {
        using Fn = void* (KINOKO_METHOD_ENTRY *)(void*,void*);
        return entry<Fn>(5)(object_,nullptr);
    }
};
class LayerMethods : public SerializableMethods {
public:
    explicit LayerMethods(KinokoActLayer* value) : SerializableMethods(value) {}
    KinokoActLayer* clone() const {
        using Fn = KinokoActLayer* (KINOKO_METHOD_ENTRY *)(void*,void*);
        return entry<Fn>(5)(object_,nullptr);
    }
    std::int32_t set_resource(KinokoActResource* resource) const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*,KinokoActResource*);
        return entry<Fn>(6)(object_,nullptr,resource);
    }
    void position(float* x, float* y, float* z) const {
        using Fn = void (KINOKO_METHOD_ENTRY *)(void*,void*,float*,float*,float*);
        entry<Fn>(7)(object_,nullptr,x,y,z);
    }
    std::int32_t bind(void* act, void* global) const {
        using Fn = std::int32_t (KINOKO_METHOD_ENTRY *)(void*,void*,void*,void*);
        return entry<Fn>(8)(object_,nullptr,act,global);
    }
};
} // namespace kinoko::act
