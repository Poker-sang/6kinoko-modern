#pragma once
#include "kinoko/file_io.h"
/* Native-width runtime readers; file records use explicit fixed-width fields. */
#ifdef __cplusplus
#include "kinoko/method_entry.hpp"
typedef struct KinokoReaderMethods {
    KinokoArchiveReader *(KINOKO_METHOD_ENTRY *destroy)(KinokoArchiveReader *, void*, uint8_t);
    int32_t (KINOKO_METHOD_ENTRY *open_string)(KinokoArchiveReader *, void*, const void *);
    int32_t (KINOKO_METHOD_ENTRY *open_path)(KinokoArchiveReader *, void*, const char *);
    int32_t (KINOKO_METHOD_ENTRY *transfer)(KinokoArchiveReader *, void*, void *, uint32_t);
    uint32_t (KINOKO_METHOD_ENTRY *transferred)(KinokoArchiveReader *, void*);
    uint32_t (KINOKO_METHOD_ENTRY *seek)(KinokoArchiveReader *, void*, int32_t, uint32_t);
    uint32_t (KINOKO_METHOD_ENTRY *size)(KinokoArchiveReader *, void*);
} KinokoReaderMethods;
#else
/* MSVC C cannot express thiscall. C consumers borrow table identities only;
   all typed virtual dispatch lives in file_io.cpp. */
typedef struct KinokoReaderMethods KinokoReaderMethods;
#endif
struct KinokoArchiveReader {
    const KinokoReaderMethods *methods;
    KinokoFile* handle;
    uint32_t transferred;
};
typedef struct KinokoPackageReader {
    KinokoArchiveReader base;
    uint32_t entry_size, entry_offset;
    /* Original starts absolute, but its virtual Seek stores a relative value.
       The guarded loader API keeps this absolute (existing compatibility). */
    uint32_t read_position;
    uint8_t xor_key, padding[3];
} KinokoPackageReader;
#ifdef __cplusplus
extern "C" {
#endif
extern const KinokoReaderMethods kinoko_file_reader_methods;
extern const KinokoReaderMethods kinoko_package_reader_methods;
extern const KinokoReaderMethods kinoko_file_writer_methods;
#ifdef __cplusplus
}
#endif
