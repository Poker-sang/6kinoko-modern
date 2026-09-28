#include "kinoko/runtime_util.hpp"
#include "kinoko/file_io_layout.h"
#include "kinoko/compat/resource_rules.hpp"
#include "kinoko/legacy_string.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>
namespace {
static_assert(offsetof(KinokoPackageReader, entry_size) == sizeof(KinokoArchiveReader));
bool valid(const KinokoArchiveReader* reader) { return reader && reader->handle; }
bool package(const KinokoArchiveReader* reader) { return reader->methods == &kinoko_package_reader_methods; }
KinokoPackageReader& packaged(KinokoArchiveReader* reader) { return *reinterpret_cast<KinokoPackageReader*>(reader); }
KinokoArchiveReader* KINOKO_METHOD_ENTRY destroy(KinokoArchiveReader* reader, void*, uint8_t flags) {
    if (!reader) return nullptr;
    if (reader->methods != &kinoko_file_writer_methods) reader->methods = &kinoko_file_reader_methods;
    if (valid(reader)) kinoko_file_close(reader->handle);
    reader->handle = nullptr;
    if (flags & 1) std::free(reader);
    return reader;
}
int32_t open_file(KinokoArchiveReader* reader, const char* path, bool write) {
    reader->handle = kinoko_file_open(path, write ? KINOKO_FILE_WRITE : KINOKO_FILE_READ_SHARED);
    return reader->handle != nullptr;
}
int32_t KINOKO_METHOD_ENTRY open_read(KinokoArchiveReader* reader, void*, const char* path) { return open_file(reader,path,false); }
int32_t KINOKO_METHOD_ENTRY open_write(KinokoArchiveReader* reader, void*, const char* path) { return open_file(reader,path,true); }
int32_t KINOKO_METHOD_ENTRY open_string(KinokoArchiveReader* reader, void*, const void* string) {
    // 4072B0 is the std::string overload, not the byte-count accessor.
    return reader->methods->open_path(reader, nullptr, kinoko::legacy::StringView(const_cast<void*>(string)).data());
}
uint32_t KINOKO_METHOD_ENTRY transferred(KinokoArchiveReader* reader, void*) { return reader->transferred; }
int32_t KINOKO_METHOD_ENTRY read_file(KinokoArchiveReader* reader, void*, void* data, uint32_t size) {
    return kinoko_file_read(reader->handle, data, size, &reader->transferred);
}
int32_t KINOKO_METHOD_ENTRY write_file(KinokoArchiveReader* reader, void*, void* data, uint32_t size) {
    return kinoko_file_write(reader->handle, data, size, &reader->transferred);
}
uint32_t KINOKO_METHOD_ENTRY seek_file(KinokoArchiveReader* reader, void*, int32_t distance, uint32_t origin) {
    return static_cast<uint32_t>(kinoko_file_seek(reader->handle, distance, origin));
}
uint32_t KINOKO_METHOD_ENTRY size_file(KinokoArchiveReader* reader, void*) { return static_cast<uint32_t>(kinoko_file_size(reader->handle)); }
uint32_t KINOKO_METHOD_ENTRY size_package(KinokoArchiveReader* reader, void*) { return packaged(reader).entry_size; }
int32_t KINOKO_METHOD_ENTRY read_package(KinokoArchiveReader* reader, void*, void* data, uint32_t size) {
    auto& entry = packaged(reader);
    const uint32_t end = entry.entry_offset + entry.entry_size;
    // 410B90 uses uint32_t arithmetic, ignores ReadFile's int32_t, and decodes the
    // clamped request (not transferred count). Keep this original virtual ABI.
    if (end < size + entry.read_position) size = end - entry.read_position;
    kinoko_file_read(reader->handle, data, size, &reader->transferred);
    if (!reader->transferred) return 0;
    entry.read_position += reader->transferred;
    kinoko::compat::decode_archive_payload(data, size, entry.xor_key);
    return 1;
}
uint32_t KINOKO_METHOD_ENTRY seek_package(KinokoArchiveReader* reader, void*, int32_t distance, uint32_t origin) {
    auto& entry = packaged(reader);
    uint32_t position;
    if (origin == KINOKO_FILE_BEGIN) position = static_cast<uint32_t>(kinoko_file_seek(reader->handle, static_cast<int32_t>(entry.entry_offset + uint32_t(distance)), KINOKO_FILE_BEGIN));
    else if (origin == KINOKO_FILE_CURRENT) position = static_cast<uint32_t>(kinoko_file_seek(reader->handle, distance, KINOKO_FILE_CURRENT));
    else if (origin == KINOKO_FILE_END) position = static_cast<uint32_t>(kinoko_file_seek(reader->handle, static_cast<int32_t>(entry.entry_offset + entry.entry_size - uint32_t(distance)), KINOKO_FILE_BEGIN));
    else return 0;
    // ORIGINAL QUIRK (410C00): Seek stores entry-relative position although
    // Open/Read use absolute positions. Do not silently change virtual behavior.
    return entry.read_position = position - entry.entry_offset;
}
bool seek_absolute(KinokoFile* file, uint32_t position) {
    return kinoko_file_seek(file,position,KINOKO_FILE_BEGIN)>=0;
}
}
#define METHOD(name, fn) fn
extern "C" const KinokoReaderMethods kinoko_file_reader_methods{
    METHOD(destroy,destroy), METHOD(open_string,open_string), METHOD(open_path,open_read),
    METHOD(transfer,read_file), METHOD(transferred,transferred), METHOD(seek,seek_file), METHOD(size,size_file)};
extern "C" const KinokoReaderMethods kinoko_package_reader_methods{
    METHOD(destroy,destroy), METHOD(open_string,open_string), METHOD(open_path,open_read),
    METHOD(transfer,read_package), METHOD(transferred,transferred), METHOD(seek,seek_package), METHOD(size,size_package)};
extern "C" const KinokoReaderMethods kinoko_file_writer_methods{
    METHOD(destroy,destroy), METHOD(open_string,open_string), METHOD(open_path,open_write),
    METHOD(transfer,write_file), METHOD(transferred,transferred), METHOD(seek,seek_file), nullptr};
#undef METHOD
extern "C" void kinoko_reader_close(KinokoArchiveReader* reader) {
    if (reader) reader->methods->destroy(reader, nullptr,1);
}
extern "C" int32_t kinoko_reader_open(KinokoArchiveReader** slot, const char* path) {
    if (!slot || !path) return 0;
    kinoko_reader_close(*slot); *slot = nullptr;
    if (kinoko_archive_count) {
        auto* reader = static_cast<KinokoPackageReader*>(std::calloc(1,sizeof(KinokoPackageReader)));
        if (!reader) return 0;
        reader->base.methods = &kinoko_package_reader_methods;
        reader->base.handle = kinoko_archive_open_entry(path,&reader->entry_offset,&reader->entry_size);
        reader->read_position = reader->entry_offset;
        reader->xor_key = kinoko::compat::archive_payload_key(reader->entry_offset);
        *slot = &reader->base;
    } else {
        *slot = static_cast<KinokoArchiveReader*>(std::calloc(1,sizeof(KinokoArchiveReader)));
        if (!*slot) return 0;
        (*slot)->methods = &kinoko_file_reader_methods;
        open_file(*slot,path,false);
    }
    if (valid(*slot)) return 1;
    kinoko_reader_close(*slot); *slot = nullptr; return 0;
}
extern "C" int32_t kinoko_writer_open(KinokoArchiveReader** slot, const char* path) {
    if (!slot || !path) return 0;
    kinoko_reader_close(*slot);
    *slot = static_cast<KinokoArchiveReader*>(std::calloc(1,sizeof(KinokoArchiveReader)));
    if (!*slot) return 0;
    (*slot)->methods = &kinoko_file_writer_methods;
    if (open_file(*slot,path,true)) return 1;
    kinoko_reader_close(*slot); *slot=nullptr; return 0;
}
extern "C" uint32_t kinoko_reader_size(KinokoArchiveReader* reader) { return valid(reader) && reader->methods->size ? reader->methods->size(reader, nullptr) : 0; }
extern "C" int32_t kinoko_reader_read(KinokoArchiveReader* reader, void* data, uint32_t size) { return valid(reader) ? reader->methods->transfer(reader, nullptr,data,size) : 0; }
extern "C" int32_t kinoko_writer_write(KinokoArchiveReader* writer, const void* data, uint32_t size) { return valid(writer) ? writer->methods->transfer(writer, nullptr,const_cast<void*>(data),size) : 0; }
extern "C" uint32_t kinoko_reader_seek(KinokoArchiveReader* reader, int32_t distance, uint32_t origin) { return valid(reader) ? reader->methods->seek(reader, nullptr,distance,origin) : 0; }
extern "C" int32_t kinoko_reader_read_exact(KinokoArchiveReader* reader, void* data, uint32_t size) {
    if (!valid(reader) || !data || !size) return 0;
    uint32_t count=0;
    if (package(reader)) {
        auto& entry=packaged(reader);
        if (entry.entry_size > UINT32_MAX-entry.entry_offset) return 0;
        const auto end=entry.entry_offset+entry.entry_size;
        if (entry.read_position < entry.entry_offset || entry.read_position > end || size > end-entry.read_position) return 0;
        if (!seek_absolute(reader->handle,entry.read_position)) return 0;
        if (!kinoko_file_read(reader->handle,data,size,&count) || count != size) return 0;
        kinoko::compat::decode_archive_payload(data, size, entry.xor_key);
        entry.read_position+=count;
    } else if (!kinoko_file_read(reader->handle,data,size,&count) || count!=size) return 0;
    reader->transferred=count; return 1;
}
extern "C" int32_t kinoko_reader_seek_relative(KinokoArchiveReader* reader, uint32_t distance) {
    if (!valid(reader)) return 0;
    if (package(reader)) {
        auto& entry=packaged(reader);
        if (entry.entry_size > UINT32_MAX-entry.entry_offset) return 0;
        const auto end=entry.entry_offset+entry.entry_size;
        if (entry.read_position < entry.entry_offset || entry.read_position > end || distance > end-entry.read_position) return 0;
        const auto target=entry.read_position+distance;
        if (!seek_absolute(reader->handle,target)) return 0;
        entry.read_position=target;
    } else {
        if (kinoko_file_seek(reader->handle,distance,KINOKO_FILE_CURRENT)<0) return 0;
    }
    return 1;
}
