#include "kinoko/file_service.h"
#include <new>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
struct KinokoFile { HANDLE handle; };
#else
#include <cstdio>
#include <limits>
#include <sys/types.h>
struct KinokoFile { std::FILE* handle; };
static_assert(sizeof(off_t)>=8, "File backend needs 64-bit offsets");
#endif
extern "C" KinokoFile* kinoko_file_open(const char* path, KinokoFileMode mode) {
    if (!path || mode<KINOKO_FILE_READ || mode>KINOKO_FILE_WRITE) return nullptr;
    auto* file=new(std::nothrow) KinokoFile{};
    if (!file) return nullptr;
#if defined(_WIN32)
    const bool write=mode==KINOKO_FILE_WRITE;
    const DWORD share=write?0:mode==KINOKO_FILE_READ_SHARED?FILE_SHARE_READ|FILE_SHARE_WRITE:FILE_SHARE_READ;
    file->handle=CreateFileA(path,write?GENERIC_WRITE:GENERIC_READ,share,nullptr,
        write?CREATE_ALWAYS:OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (file->handle==INVALID_HANDLE_VALUE) { delete file; return nullptr; }
#else
    file->handle=std::fopen(path,mode==KINOKO_FILE_WRITE?"wb":"rb");
    if (!file->handle) { delete file; return nullptr; }
#endif
    return file;
}
extern "C" void kinoko_file_close(KinokoFile* file) {
    if (!file) return;
#if defined(_WIN32)
    CloseHandle(file->handle);
#else
    std::fclose(file->handle);
#endif
    delete file;
}
extern "C" int kinoko_file_read(KinokoFile* file,void* data,uint32_t size,uint32_t* transferred) {
    if (transferred) *transferred=0;
    if (!file || !transferred || (!data && size)) return 0;
    if (!size) return 1;
#if defined(_WIN32)
    DWORD count=0;
    const bool ok=ReadFile(file->handle,data,size,&count,nullptr)!=0;
    *transferred=count; return ok;
#else
    *transferred=static_cast<uint32_t>(std::fread(data,1,size,file->handle));
    return std::ferror(file->handle)==0;
#endif
}
extern "C" int kinoko_file_write(KinokoFile* file,const void* data,uint32_t size,uint32_t* transferred) {
    if (transferred) *transferred=0;
    if (!file || !transferred || (!data && size)) return 0;
    if (!size) return 1;
#if defined(_WIN32)
    DWORD count=0;
    const bool ok=WriteFile(file->handle,data,size,&count,nullptr)!=0;
    *transferred=count; return ok;
#else
    *transferred=static_cast<uint32_t>(std::fwrite(data,1,size,file->handle));
    return *transferred==size && std::fflush(file->handle)==0;
#endif
}
extern "C" int64_t kinoko_file_seek(KinokoFile* file,int64_t distance,uint32_t origin) {
    if (!file || origin>KINOKO_FILE_END) return -1;
#if defined(_WIN32)
    LARGE_INTEGER move{}, position{}; move.QuadPart=distance;
    return SetFilePointerEx(file->handle,move,&position,origin)?position.QuadPart:-1;
#else
    const int origins[]={SEEK_SET,SEEK_CUR,SEEK_END};
    if (distance<std::numeric_limits<off_t>::min() || distance>std::numeric_limits<off_t>::max()) return -1;
    if (fseeko(file->handle,static_cast<off_t>(distance),origins[origin])!=0) return -1;
    return static_cast<int64_t>(ftello(file->handle));
#endif
}
extern "C" int64_t kinoko_file_size(KinokoFile* file) {
    if (!file) return -1;
#if defined(_WIN32)
    LARGE_INTEGER size{};
    return GetFileSizeEx(file->handle,&size)?size.QuadPart:-1;
#else
    const auto position=kinoko_file_seek(file,0,KINOKO_FILE_CURRENT);
    if (position<0) return -1;
    const auto size=kinoko_file_seek(file,0,KINOKO_FILE_END);
    return kinoko_file_seek(file,position,KINOKO_FILE_BEGIN)<0?-1:size;
#endif
}
