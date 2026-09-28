#include "kinoko/runtime_util.hpp"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/squirrel_host_object.hpp"
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include <cstdio>
namespace {
std::atomic<int32_t> watch_events;
void table_snapshot(const char* label, SQTable* table) {
    if (!label || !table) return;
    char message[640];
    std::snprintf(message, sizeof(message), "%s:table=%p used=%lld", label,
        static_cast<void*>(table), static_cast<long long>(table->CountUsed()));
    kinoko_trace(message);
    SQObjectPtr iterator, key, value;
    for (unsigned count = 0; count < 4096; ++count) {
        const auto next = table->Next(true, iterator, key, value);
        if (next == -1) break;
        iterator = next;
        std::snprintf(message, sizeof(message), "%s:key=(%08X,%llX) text=%.256s value=(%08X,%llX)",
            label, static_cast<unsigned>(type(key)), static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_rawval(key))),
            type(key) == OT_STRING ? _stringval(key) : "", static_cast<unsigned>(type(value)),
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_rawval(value))));
        kinoko_trace(message);
    }
}
}
extern "C" void kinoko_trace_squirrel_table_entries(const char* label, SQTable* table) {
#ifdef _MSC_VER
    __try { table_snapshot(label, table); }
    __except (EXCEPTION_EXECUTE_HANDLER) { kinoko_trace("sq-table:snapshot-fault"); }
#else
    if(kinoko_diagnostics_accepts(label)) table_snapshot(label, table);
#endif
}
extern "C" void kinoko_trace_ref_watch(const char* label, SQSharedState* shared_state,
    int32_t type, intptr_t data) {
    if (!kinoko_is_release_watch_data(data)) return;
    const auto sequence = ++watch_events;
    if (sequence > 4096) return;
#ifdef _MSC_VER
    __try {
#endif
        const auto value = kinoko::script::borrowed_value(type, data);
        const auto* object = reinterpret_cast<const SQRefCounted*>(data);
        char message[256];
        std::snprintf(message, sizeof(message), "sq-watch:%d %s type=%08X data=%p internal=%llu refs=%llu",
            sequence, label ? label : "event", static_cast<unsigned>(type), static_cast<const void*>(object),
            static_cast<unsigned long long>(object->_uiRef),
            static_cast<unsigned long long>(shared_state ? shared_state->_refs_table.DiagnosticRefs(value) : 0));
        kinoko_trace(message);
#ifdef _MSC_VER
    } __except (EXCEPTION_EXECUTE_HANDLER) { kinoko_trace("sq-watch:fault"); }
#endif
}
