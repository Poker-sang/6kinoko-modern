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

/* Borrow raw value bits without ownership transfer; payload follows pointer width. */
static inline HSQOBJECT kinoko_borrowed_object(int32_t type, intptr_t bits) {
    HSQOBJECT value;
    memset(&value, 0, sizeof(value));
    value._type = (SQObjectType)type;
    memcpy(&value._unVal, &bits, sizeof(bits));
    return value;
}
