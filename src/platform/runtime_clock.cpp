#include "kinoko/runtime_clock.h"
#include <SDL3/SDL_timer.h>
#include <atomic>
namespace { std::atomic<bool> simulated{false}; std::atomic<uint32_t> simulation_time{1000}; }
extern "C" uint32_t kinoko_simulation_milliseconds() {
    return simulated.load() ? simulation_time.load():kinoko_clock_milliseconds();
}
extern "C" void kinoko_simulation_enable(int enabled) {
    simulation_time.store(1000);simulated.store(enabled!=0);
}
extern "C" void kinoko_simulation_frame(uint64_t frame) {
    simulation_time.store(static_cast<uint32_t>(1000+frame*1000/60));
}
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
