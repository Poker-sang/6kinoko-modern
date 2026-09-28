#pragma once
#include "kinoko/file_io.h"
/* Recovered x86 layouts shared only by legacy virtual users and fixtures. */
#ifdef __cplusplus
typedef struct KinokoReaderMethods {
    KinokoArchiveReader *(__fastcall *destroy)(KinokoArchiveReader *, void*, uint8_t);
    int32_t (__fastcall *open_string)(KinokoArchiveReader *, void*, const void *);
    int32_t (__fastcall *open_path)(KinokoArchiveReader *, void*, const char *);
    int32_t (__fastcall *transfer)(KinokoArchiveReader *, void*, void *, uint32_t);
    uint32_t (__fastcall *transferred)(KinokoArchiveReader *, void*);
    uint32_t (__fastcall *seek)(KinokoArchiveReader *, void*, int32_t, uint32_t);
    uint32_t (__fastcall *size)(KinokoArchiveReader *, void*);
} KinokoReaderMethods;
#else
/* MSVC C cannot express thiscall. C consumers borrow table identities only;
   all typed virtual dispatch lives in file_io.cpp. */
typedef struct KinokoReaderMethods KinokoReaderMethods;
#endif
struct KinokoArchiveReader {
    const KinokoReaderMethods *methods;
    HANDLE handle;
    DWORD transferred;
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
