#pragma once
#include "kinoko/legacy_string.hpp"
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstdint>
struct SQVM;
namespace kinoko::act {
// Byte words hold a native HSQOBJECT; APIs copy it with memcpy, never int indexing.
using ScriptValueStorage = std::array<int32_t,2*sizeof(void*)/sizeof(int32_t)>;
struct ActCallbackRecord {
    SQVM* vm;
    alignas(void*) ScriptValueStorage environment, closure;
};
struct ScriptStorageRecord {
    const void* methods;
    ActCallbackRecord initialize, update, release;
    legacy::StringRecord file_name;
    uint32_t unknown88;
    void* bytes;
    uint32_t size;
    uint8_t loaded, compiled;
    std::array<uint8_t,2> padding102;
};
using ScriptStorageView = native::RecordView<ScriptStorageRecord>;
static_assert(sizeof(ActCallbackRecord)==5*sizeof(void*));
static_assert(offsetof(ActCallbackRecord,closure)==3*sizeof(void*));
#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(ScriptStorageRecord)==104);
#endif
}
