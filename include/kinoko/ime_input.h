#pragma once
#include <stdint.h>
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_ime_initialize(void);
void kinoko_ime_release(HWND window);
int32_t kinoko_ime_dispatch(HWND window, UINT message, WPARAM key, LPARAM parameter);
#ifdef __cplusplus
}
#endif
