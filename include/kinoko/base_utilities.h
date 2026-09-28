#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* One zlib 1.2.3 operation, returning bytes written or zero. */
int32_t kinoko_compress_buffer(const void* input, int32_t input_size,
    void* output, int32_t output_capacity);
int32_t kinoko_decompress_buffer(const void* input, int32_t input_size,
    void* output, int32_t output_capacity);

/* Both output buffers, when present, must hold 260 bytes. File is optional.
   Returns the CRT error code, zero on success; retains a trailing separator. */
int32_t kinoko_path_split(const char* path, char* directory, char* file);
#ifdef __cplusplus
}
#endif
