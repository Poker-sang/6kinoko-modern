#pragma once
#include "kinoko/file_service.h"
#include <utility>
namespace kinoko::io {
class File final {
    KinokoFile* file_ = nullptr;
public:
    File(const char* path, KinokoFileMode mode) : file_(kinoko_file_open(path, mode)) {}
    File(const char* path, KinokoFileMode mode, bool utf8) : file_(utf8?kinoko_file_open_utf8(path,mode):kinoko_file_open(path,mode)) {}
    ~File() { close(); }
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    explicit operator bool() const noexcept { return file_ != nullptr; }
    KinokoFile* get() const noexcept { return file_; }
    KinokoFile* detach() noexcept { return std::exchange(file_, nullptr); }
    void close() noexcept { kinoko_file_close(detach()); }
    bool read(void* data, uint32_t size) {
        uint32_t count=0;
        return kinoko_file_read(file_,data,size,&count) && count==size;
    }
    bool write(const void* data, uint32_t size) {
        uint32_t count=0;
        return kinoko_file_write(file_,data,size,&count) && count==size;
    }
};
}
