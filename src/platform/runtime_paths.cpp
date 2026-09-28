#include "kinoko/runtime_paths.h"
#include <SDL3/SDL_filesystem.h>
#include <filesystem>
extern "C" int kinoko_use_executable_directory() {
    try {
        const char* base=SDL_GetBasePath();
        if(!base || !*base) return 0;
        auto directory=std::filesystem::u8path(base).lexically_normal();
        if(directory.filename().empty()) directory=directory.parent_path();
        // SDL's default for a standard .app bundle is Contents/Resources.
        // Our packaging rule keeps DAT beside the binary in Contents/MacOS.
        // This is a path convention, with no native OS API or backend.
        if(directory.filename()=="Resources" && directory.parent_path().filename()=="Contents"
            && directory.parent_path().parent_path().extension()==".app")
            directory=directory.parent_path()/"MacOS";
        std::error_code error;
        std::filesystem::current_path(directory,error);
        return !error;
    } catch(...) { return 0; }
}
