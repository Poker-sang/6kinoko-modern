#pragma once
#include <cstdint>
namespace kinoko::script {
// Low-word trace compatibility only. Never use this to recover an address.
inline int32_t diagnostic_address(const void* value) noexcept {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
}

extern "C" void kinoko_trace(const char*);
extern "C" void kinoko_trace_squirrel_name(const char*,int32_t);
namespace kinoko::script {
inline void diagnostic_name(const char* label,const char* name) {
#if INTPTR_MAX == INT32_MAX
    kinoko_trace_squirrel_name(label,diagnostic_address(name));
#else
    kinoko_trace(label); if(name) kinoko_trace(name);
#endif
}
}
