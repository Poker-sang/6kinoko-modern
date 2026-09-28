#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void kinoko_diagnostics_initialize(void);
void kinoko_diagnostics_shutdown(void);
#ifdef _WIN32
int kinoko_report_exception(EXCEPTION_POINTERS *exception);
#endif
// Query at the formatting boundary; VM trace call sites remain intact.
int kinoko_diagnostics_accepts(const char *label);
void kinoko_trace(const char *message);
void kinoko_trace_hresult(const char *label, long value);

#ifdef __cplusplus
}
#endif
