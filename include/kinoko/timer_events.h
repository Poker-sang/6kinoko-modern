#pragma once

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoFrameEvent KinokoFrameEvent;
void kinoko_frame_timer_initialize(void)
#ifdef __cplusplus
    noexcept(false)
#endif
    ;
KinokoFrameEvent* kinoko_frame_timer_register(void);
int32_t kinoko_frame_timer_unregister(KinokoFrameEvent* event);
void kinoko_frame_timer_wait(KinokoFrameEvent* event);
#ifdef __cplusplus
}
#endif
