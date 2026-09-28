#pragma once

#include <squirrel.h>
#include "kinoko/owned_script_object.h"
#include "kinoko/upstream_bindings.hpp"
#include "kinoko/native_record_view.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace kinoko::script {

// The game's SqPlus wrapper owns an EXTERNAL reference. SQObjectPtr instead
// owns an internal reference; it must never be overlaid on this 12-byte record.
struct ObjectStorage {
    const void* vtable;
    HSQOBJECT value;
};
static_assert(offsetof(ObjectStorage, value) == sizeof(void*));
static_assert(sizeof(KinokoOwnedObjectWords)==sizeof(ObjectStorage));
static_assert(offsetof(KinokoOwnedObjectWords,type)==offsetof(ObjectStorage,value)+offsetof(HSQOBJECT,_type));
static_assert(offsetof(KinokoOwnedObjectWords,value)==offsetof(ObjectStorage,value)+offsetof(HSQOBJECT,_unVal));
#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(HSQOBJECT) == 8 && sizeof(ObjectStorage) == 12);
#endif

#if INTPTR_MAX == INT32_MAX
inline int32_t address(const void* pointer) noexcept {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(pointer));
}
template<class T = void> T* pointer(int32_t value) noexcept {
    return reinterpret_cast<T*>(static_cast<uintptr_t>(static_cast<uint32_t>(value)));
}
inline int32_t data_bits(const HSQOBJECT& value) noexcept {
    int32_t bits;
    std::memcpy(&bits, &value._unVal, sizeof(bits));
    return bits;
}
inline HSQOBJECT borrowed_value(int32_t type, int32_t data) noexcept {
    HSQOBJECT result;
    result._type = static_cast<SQObjectType>(type);
    std::memcpy(&result._unVal, &data, sizeof(data));
    return result;
}
#endif

// Views also accept the legacy int32_t[3] temporaries, without type-punning
// them as live C++ objects or assuming stronger alignment than their storage.
class ObjectView final {
public:
    explicit ObjectView(void* storage) noexcept : record_(storage) {}
    explicit ObjectView(const void* storage) noexcept : ObjectView(const_cast<void*>(storage)) {}
#if INTPTR_MAX == INT32_MAX
    explicit ObjectView(int32_t storage) noexcept : ObjectView(pointer(storage)) {}
#endif
    HSQOBJECT value() const noexcept {
        return record_.get(&ObjectStorage::value);
    }
    void write(const HSQOBJECT& value) const noexcept {
        record_.set(&ObjectStorage::value, value);
    }
    void set_vtable(const void* vtable) const noexcept {
        record_.set(&ObjectStorage::vtable, vtable);
    }
    void initialize(const void* vtable) const noexcept {
        set_vtable(vtable);
        reset();
    }
    void reset() const noexcept {
        HSQOBJECT empty;
        sq_resetobject(&empty);
        write(empty);
    }
    void push(HSQUIRRELVM vm) const { sq_pushobject(vm, value()); }
    void retain(HSQUIRRELVM vm) const {
        upstream::sqplus_retain(vm, value());
    }
    void release(HSQUIRRELVM vm) const {
        upstream::sqplus_release(vm, value());
    }
    void assign(HSQUIRRELVM vm, HSQOBJECT incoming) const {
        // Acquire first: source and destination may be identical, or the old
        // destination may own the source. Keep the borrowed value in a local.
        write(upstream::sqplus_assign(vm, value(), incoming));
    }
    SQObjectType capture(HSQUIRRELVM vm, SQInteger index) const {
        const auto incoming = upstream::sqplus_capture(vm, value(), static_cast<int>(index));
        write(incoming);
        return incoming._type;
    }
    void* payload_data() const noexcept {
        return record_.bytes(&ObjectStorage::value);
    }
private:
    kinoko::native::RecordView<ObjectStorage> record_;
};

// Only use for balanced helper operations. BeginIteration intentionally leaves
// two values on the VM stack and therefore must NOT use this guard.
class StackTop final {
public:
    explicit StackTop(HSQUIRRELVM vm) : vm_(vm), top_(sq_gettop(vm)) {}
    ~StackTop() { sq_settop(vm_, top_); }
    StackTop(const StackTop&) = delete;
    StackTop& operator=(const StackTop&) = delete;
    SQInteger top() const noexcept { return top_; }
private:
    HSQUIRRELVM vm_;
    SQInteger top_;
};

} // namespace kinoko::script
