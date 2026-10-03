#include "kinoko/runtime_paths.h"
#include "kinoko/runtime_options.hpp"
#include <SDL3/SDL_filesystem.h>
#include <filesystem>
#include <stdexcept>
namespace kinoko::runtime {
namespace {
Options settings;
bool same_file(const std::filesystem::path& first,const std::filesystem::path& second) {
    if(first.empty() || second.empty())return false;
    if(first==second)return true;
    std::error_code error;
    if(std::filesystem::equivalent(first,second,error))return true;
    // Resolve existing parent links even when an output file does not exist yet.
    return std::filesystem::weakly_canonical(first)==std::filesystem::weakly_canonical(second);
}
}
const Options& options() { return settings; }
bool parse_options(int argc,char** argv,std::string& error) {
    settings={};
    try {
        Options parsed;
        for(int i=1;i<argc;++i) {
            const std::string key=argv[i];
            if(key=="--tas-window"){parsed.tas_window=true;continue;}
            if(key!="--save-dir" && key!="--record" && key!="--replay" && key!="--replay-status" && key!="--replay-identity" && key!="--tas-dir" && key!="--tas-output")
                throw std::runtime_error("Unknown argument: "+key);
            if(++i==argc || !*argv[i])throw std::runtime_error("Missing value for "+key);
            if(key=="--replay-identity")parsed.identity=argv[i];
            else {
                const auto path=std::filesystem::absolute(std::filesystem::u8path(argv[i])).lexically_normal();
                if(key=="--tas-dir")parsed.tas_dir=path;
                if(key=="--tas-output")parsed.tas_output=path;
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
            // The status stream is truncated before the replay is opened.
            if(same_file(parsed.status,parsed.recording) || same_file(parsed.status,parsed.playback))
                throw std::runtime_error("--replay-status must differ from the recording/replay file");
        }
        if(parsed.tas_window && parsed.tas_dir.empty())throw std::runtime_error("--tas-window requires --tas-dir");
        if(!parsed.tas_dir.empty()) {
            if(parsed.save_dir.empty() || (parsed.recording.empty() && parsed.playback.empty()) || parsed.tas_output.empty())throw std::runtime_error("TAS requires isolated save and replay paths");
            if(!std::filesystem::is_directory(parsed.tas_dir))throw std::runtime_error("TAS directory does not exist");
            // --record writes its recording path; tas_output is only used for playback branches.
            if(!parsed.playback.empty() && (same_file(parsed.tas_output,parsed.playback) || same_file(parsed.tas_output,parsed.status)))
                throw std::runtime_error("--tas-output must differ from the replay input and status files");
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
