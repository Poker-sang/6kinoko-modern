#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* 427950: owns the returned document; source remains borrowed. */
KinokoActDocument *__fastcall kinoko_act_clone(KinokoActDocument *source, void *unused);
#ifdef __cplusplus
}
#endif
