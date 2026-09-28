// Exercise the production callback adapter with deterministic VM identities.
// No fake pointer is dereferenced; VM operations below model its public stack ABI.
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_source_runtime.h"
#include <array>
#include <cstdio>

namespace {
SQInteger top = 7;
int32_t call_result = 0;
int destroyed = 0;
std::array<HSQOBJECT, 3> pushed{};
unsigned push_count = 0;
}
extern "C" {
void kinoko_trace_i32(const char*, int32_t) {}
SQInteger sq_gettop(HSQUIRRELVM) { return top; }
void sq_settop(HSQUIRRELVM, SQInteger value) { top = value; }
void sq_pushobject(HSQUIRRELVM, HSQOBJECT value) { pushed[push_count++] = value; ++top; }
void sq_pop(HSQUIRRELVM, SQInteger count) { top -= count; }
int32_t kinoko_sq_call(SQVM*, int32_t count, int32_t result, int32_t) {
    top -= count;
    if (call_result >= 0 && result) ++top;
    return call_result;
}
SQVM* kinoko_sq_pop(SQVM* vm, int32_t count) { top -= count; return vm; }
void* kinoko_sqplus_object_destroy(void* object) {
    ++destroyed;
    *static_cast<KinokoOwnedObjectWords*>(object) = {};
    return object;
}
}
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "callback contract line %d\n", __LINE__); return 1; } } while (0)
int main() {
    const uintptr_t high = sizeof(void*) > 4 ? uintptr_t(UINT64_C(0x123400000000)) : 0;
    for (const uintptr_t low : {uintptr_t(0x10001000), uintptr_t(0xf0001000)}) {
        KinokoScriptCallback callback{};
        callback.vm = reinterpret_cast<SQVM*>(high | low);
        callback.closure.type = OT_CLOSURE;
        callback.closure.value = static_cast<intptr_t>(high | 0xa0002000);
        callback.environment.type = OT_TABLE;
        callback.environment.value = static_cast<intptr_t>(high | 0xb0003000);
        for (int attempt = 0; attempt < 3; ++attempt) {
            top = 7; push_count = 0; call_result = 0;
            CHECK(kinoko_script_callback_invoke(&callback) == 0);
            CHECK(top == 7 && push_count == 2);
            CHECK(kinoko::script::data_bits(pushed[0]) == callback.closure.value);
            CHECK(kinoko::script::data_bits(pushed[1]) == callback.environment.value);
        }
        top = 7; push_count = 0; call_result = -1;
        CHECK(kinoko_script_callback_invoke(&callback) == -1 && top == 7);
        for (const int status : {0, -1}) {
            KinokoOwnedObjectWords argument{};
            argument.type = OT_USERPOINTER;
            argument.value = static_cast<intptr_t>(high | 0xc0004000);
            const auto payload = argument.value;
            const auto before = destroyed;
            top = 7; push_count = 0; call_result = status;
            CHECK(kinoko_script_callback_invoke_owned(&callback, &argument,
                argument.type, argument.value) == status);
            CHECK(push_count == 3 && kinoko::script::data_bits(pushed[2]) == payload);
            CHECK(destroyed == before + 1 && argument.value == 0);
            CHECK(top == (status < 0 ? 8 : 7));
        }
    }
    CHECK(kinoko_script_callback_invoke(nullptr) < 0);
    KinokoScriptCallback empty{};
    CHECK(kinoko_script_callback_invoke(&empty) < 0);
    return 0;
}
