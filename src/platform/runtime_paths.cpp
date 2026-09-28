#include "kinoko/runtime_paths.h"
#include <SDL3/SDL_filesystem.h>
#include <filesystem>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <vector>
#endif
extern "C" int kinoko_use_executable_directory() {
    try {
        std::error_code error;
#if defined(__APPLE__)
        // SDL defaults to a bundle's Resources directory. Our DAT rule is
        // strictly beside the binary, including a future .app package.
        uint32_t size=0;
        _NSGetExecutablePath(nullptr,&size);
        if(!size) return 0;
        std::vector<char> buffer(size);
        if(_NSGetExecutablePath(buffer.data(),&size)!=0) return 0;
        const auto binary=std::filesystem::canonical(std::filesystem::u8path(buffer.data()),error);
        if(error) return 0;
        const auto directory=binary.parent_path();
#else
        const char* base=SDL_GetBasePath();
        if(!base || !*base) return 0;
        const auto directory=std::filesystem::u8path(base);
#endif
        std::filesystem::current_path(directory,error);
        return !error;
    } catch(...) { return 0; }
}
