#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoFile KinokoFile;
enum KinokoFileMode { KINOKO_FILE_READ, KINOKO_FILE_READ_SHARED, KINOKO_FILE_WRITE };
enum KinokoFileOrigin { KINOKO_FILE_BEGIN, KINOKO_FILE_CURRENT, KINOKO_FILE_END };
/* Windows paths retain the original process ANSI encoding; POSIX paths are UTF-8.
   Read succeeds at EOF with a short/zero transferred count. Seek returns -1 on
   failure, independently of valid positions whose low word is 0xffffffff. */
KinokoFile* kinoko_file_open(const char* path, enum KinokoFileMode mode);
void kinoko_file_close(KinokoFile* file);
int kinoko_file_read(KinokoFile* file, void* data, uint32_t size, uint32_t* transferred);
int kinoko_file_write(KinokoFile* file, const void* data, uint32_t size, uint32_t* transferred);
int64_t kinoko_file_seek(KinokoFile* file, int64_t distance, uint32_t origin);
int64_t kinoko_file_size(KinokoFile* file);
#ifdef __cplusplus
}
#endif
