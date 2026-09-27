#pragma once
#include <stdint.h>

// Decoded MCD storage; disk IDs and handles remain 32-bit, pointers are native.
struct kinoko_mcd_chip {
    uint32_t chip_id;
    unsigned char bytes[48];
};

struct kinoko_mcd_texture {
    uint32_t texture_id;
    int32_t handle;
};

struct kinoko_mcd_data {
    uint32_t chip_count;
    struct kinoko_mcd_chip *chips;
    uint32_t texture_count;
    struct kinoko_mcd_texture *textures;
};

