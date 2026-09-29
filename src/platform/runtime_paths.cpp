#include "kinoko/runtime_paths.h"
#include "kinoko/runtime_options.hpp"
#include <SDL3/SDL_filesystem.h>
#include <filesystem>
#include <stdexcept>
namespace kinoko::runtime {
namespace { Options settings; }
const Options& options() { return settings; }
bool parse_options(int argc,char** argv,std::string& error) {
    settings={};
    try {
        Options parsed;
        for(int i=1;i<argc;++i) {
            const std::string key=argv[i];
            if(key!="--save-dir" && key!="--record" && key!="--replay" && key!="--replay-status" && key!="--replay-identity")
                throw std::runtime_error("Unknown argument: "+key);
            if(++i==argc || !*argv[i])throw std::runtime_error("Missing value for "+key);
            if(key=="--replay-identity")parsed.identity=argv[i];
            else {
                const auto path=std::filesystem::absolute(std::filesystem::u8path(argv[i])).lexically_normal();
                if(key=="--save-dir")parsed.save_dir=path;
                if(key=="--record")parsed.recording=path;
                if(key=="--replay")parsed.playback=path;
                if(key=="--replay-status")parsed.status=path;
            }
        }
        if(!parsed.recording.empty() && !parsed.playback.empty())throw std::runtime_error("Choose --record or --replay");
        if(!parsed.recording.empty() || !parsed.playback.empty()) {
            if(parsed.save_dir.empty() || parsed.status.empty() || parsed.identity.size()!=64 || parsed.identity.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)
                throw std::runtime_error("Replay requires --save-dir, --replay-status and a 64-digit --replay-identity; use the session launcher");
        }
        if(!parsed.save_dir.empty())std::filesystem::create_directories(parsed.save_dir);
        settings=std::move(parsed);return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
std::string save_path(const char* name) {
    if(!name)return {};
    if(settings.save_dir.empty())return name;
    const auto relative=std::filesystem::u8path(name).lexically_normal();
    if(relative.empty() || relative.has_root_path())return {};
    for(const auto& part:relative)if(part=="..")return {};
    return (settings.save_dir/relative).u8string();
}
}
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
