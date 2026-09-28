#include "kinoko/directory_search.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
struct KinokoDirectorySearch {
    std::vector<std::string> names;
    size_t index=0;
};
namespace {
struct GlobDeleter { void operator()(char** names) const { SDL_free(names); } };
bool append_matches(KinokoDirectorySearch& search,const std::string& directory,const std::string& pattern) {
    int count=0;
    std::unique_ptr<char*,GlobDeleter> names(SDL_GlobDirectory(directory.c_str(),pattern.c_str(),SDL_GLOB_CASEINSENSITIVE,&count));
    if(!names) return false;
    for(int i=0;i<count;++i) {
        // SDL uses UTF-8; keep the existing native narrow-string boundary for
        // script/file APIs through the standard filesystem conversion.
        auto name=std::filesystem::u8path(names.get()[i]).string();
        if(std::find(search.names.begin(),search.names.end(),name)==search.names.end())
            search.names.push_back(std::move(name));
    }
    return true;
}
}
extern "C" KinokoDirectorySearch* kinoko_directory_first(const char* pattern) {
    if(!pattern || !*pattern) return nullptr;
    try {
        auto search=std::make_unique<KinokoDirectorySearch>();
        auto utf8=std::filesystem::path(pattern).generic_u8string();
        std::replace(utf8.begin(),utf8.end(),'\\','/');
        const auto path=std::filesystem::u8path(utf8);
        auto directory=path.parent_path().generic_u8string();
        if(directory.empty()) directory=".";
        auto filter=path.filename().u8string();
        if(filter.empty() || directory.find_first_of("*?")!=std::string::npos) return nullptr;
        // Compatibility is pattern normalization, not another OS backend.
        if(filter=="*.*") filter="*";
        if(!append_matches(*search,directory,filter)) return nullptr;
        if(filter.size()>=2 && filter.compare(filter.size()-2,2,".*")==0)
            if(!append_matches(*search,directory,filter.substr(0,filter.size()-2))) return nullptr;
        return search->names.empty()?nullptr:search.release();
    } catch(...) { return nullptr; }
}
extern "C" int kinoko_directory_next(KinokoDirectorySearch* search) {
    if(!search || search->index+1>=search->names.size()) return 0;
    ++search->index;
    return 1;
}
extern "C" const char* kinoko_directory_name(const KinokoDirectorySearch* search) {
    return search?search->names[search->index].c_str():nullptr;
}
extern "C" int kinoko_directory_close(KinokoDirectorySearch* search) {
    if(!search) return 0;
    delete search;
    return 1;
}
