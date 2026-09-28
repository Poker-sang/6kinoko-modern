#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint32_t kinoko_clock_milliseconds(void);
void kinoko_clock_delay(uint32_t milliseconds);
int kinoko_clock_request_resolution(void);
void kinoko_clock_release_resolution(void);
#ifdef __cplusplus
}
#endif
