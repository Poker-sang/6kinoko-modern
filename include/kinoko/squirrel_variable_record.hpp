#pragma once
#include <cstddef>
#include <cstdint>

namespace kinoko::script::binding {
// Native source VarRef layout. Descriptor identities are borrowed pointers;
// offset is deliberately numeric: it also stores constants and byte offsets.
struct Variable {
    intptr_t offset;
    int32_t category;
    void* instance_type;
    void* value_type;
    uint16_t size;
    uint16_t flags;
};
#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(Variable) == 20 && offsetof(Variable, flags) == 18);
#endif
enum VariableFlags : uint16_t { ReadOnly = 1, Constant = 2, Static = 4 };
} // namespace kinoko::script::binding
