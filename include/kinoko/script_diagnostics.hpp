#pragma once
#include <cstdint>
namespace kinoko::script {
// Low-word trace compatibility only. Never use this to recover an address.
inline int32_t diagnostic_address(const void* value) noexcept {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
}
