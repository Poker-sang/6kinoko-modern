#include "kinoko/runtime_clock.h"
#include <SDL3/SDL_timer.h>
#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif
extern "C" uint32_t kinoko_clock_milliseconds() {
#if defined(_WIN32)
    return timeGetTime(); // Preserve the existing Windows uptime epoch and wrap.
#else
    return static_cast<uint32_t>(SDL_GetTicks());
#endif
}
extern "C" void kinoko_clock_delay(uint32_t milliseconds) { SDL_Delay(milliseconds); }
extern "C" int kinoko_clock_request_resolution() {
#if defined(_WIN32)
    return timeBeginPeriod(1)==TIMERR_NOERROR;
#else
    return 1;
#endif
}
extern "C" void kinoko_clock_release_resolution() {
#if defined(_WIN32)
    timeEndPeriod(1);
#endif
}
