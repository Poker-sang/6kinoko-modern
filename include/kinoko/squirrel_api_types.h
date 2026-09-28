#pragma once

#include <stdint.h>
#include <string.h>
#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdio.h>
#include <sqstdblob.h>
#include <sqstdmath.h>
#include <sqstdstring.h>

static inline SQFloat kinoko_float_bits(int32_t bits) {
    SQFloat value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
