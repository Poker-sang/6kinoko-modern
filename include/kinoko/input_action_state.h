#pragma once
#include <stdint.h>
/* POD suffix: original manager offsets and keyconfig.dat wire data stay intact. */
typedef struct KinokoActionState {
    int32_t held[19];
    uint8_t released[19];
    int32_t published[23]; /* 19 actions, moveX/moveY/menuX/menuY */
} KinokoActionState;
