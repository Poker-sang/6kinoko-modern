#pragma once
#include "kinoko/native_record_view.hpp"
#include "kinoko/native_control.h"
#include <cstddef>
#include <cstdint>

namespace kinoko::native {
// Layout schemas only. Count access is provided by the Win32 implementation;
// do not overlay std::shared_ptr or std::atomic on the existing C storage.
struct ControlRecord {
    const void* vtable;
    std::int32_t strong, weak;
    void* allocation;
};
struct ControlTable {
    const void *unknown_entry, *dispose, *destroy;
};
using ReferenceRecord = KinokoNativeReference;
// Native counted-base fields. Actor fields borrow controls; ownership operations
// still dispatch through Boost's real C++ interface, not this snapshot.
static_assert(offsetof(ControlRecord,strong)==sizeof(void*));
static_assert(offsetof(ControlRecord,weak)==sizeof(void*)+sizeof(int32_t));
static_assert(offsetof(ControlRecord,allocation)==sizeof(void*)+2*sizeof(int32_t));
static_assert(sizeof(ControlRecord)==2*sizeof(void*)+2*sizeof(int32_t));
static_assert(sizeof(ControlTable)==3*sizeof(void*) && sizeof(ReferenceRecord)==2*sizeof(void*));
}
