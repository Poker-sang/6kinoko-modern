#pragma once
#include <stdint.h>
/* A SqPlus external reference. The receiver consumes ownership of by-value
   arguments. Value bits can hold a pointer; type tags remain 32-bit. */
typedef struct KinokoOwnedObjectWords {
    const void* vtable;
    int32_t type;
    intptr_t value;
} KinokoOwnedObjectWords;
