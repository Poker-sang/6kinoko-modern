#include "kinoko/directory_search.h"
#include <memory>
#include <string>
#include <algorithm>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
struct KinokoDirectorySearch {
    HANDLE handle=INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA current{};
    ~KinokoDirectorySearch() { if(handle!=INVALID_HANDLE_VALUE) FindClose(handle); }
};
#else
#include <dirent.h>
struct KinokoDirectorySearch {
    DIR* directory=nullptr;
    std::string pattern, current;
    ~KinokoDirectorySearch() { if(directory) closedir(directory); }
};
namespace {
const char* next_character(const char* text) {
    if(!*text) return text;
    ++text;
    while((static_cast<unsigned char>(*text)&0xc0)==0x80) ++text;
    return text;
}
bool wildcard(const char* pattern,const char* name) {
    const char* star=nullptr;
    const char* retry=nullptr;
    while(*name) {
        if(*pattern=='*') { star=pattern++; retry=name; }
        else if(*pattern=='?') { ++pattern; name=next_character(name); }
        else if(*pattern && *pattern==*name) { ++pattern; ++name; }
        else if(star) { pattern=star+1; name=retry=next_character(retry); }
        else return false;
    }
    while(*pattern=='*') ++pattern;
    return !*pattern;
}
bool matches(const std::string& pattern,const char* name) {
    // Only * and ? are special, as in the original API; [] stays literal.
    // *.* includes extensionless entries, and foo.* also includes foo.
    if(pattern=="*.*" || wildcard(pattern.c_str(),name)) return true;
    if(pattern.size()>=2 && pattern.compare(pattern.size()-2,2,".*")==0)
        return wildcard(pattern.substr(0,pattern.size()-2).c_str(),name);
    return false;
}
}
#endif
extern "C" KinokoDirectorySearch* kinoko_directory_first(const char* pattern) {
    if(!pattern || !*pattern) return nullptr;
    try {
        auto search=std::make_unique<KinokoDirectorySearch>();
#if defined(_WIN32)
        search->handle=FindFirstFileA(pattern,&search->current);
        if(search->handle==INVALID_HANDLE_VALUE) return nullptr;
#else
        std::string path(pattern);
        std::replace(path.begin(),path.end(),'\\','/');
        const auto separator=path.find_last_of('/');
        const std::string directory=separator==std::string::npos?".":separator==0?"/":path.substr(0,separator);
        search->pattern=path.substr(separator==std::string::npos?0:separator+1);
        if(search->pattern.empty()) return nullptr;
        search->directory=opendir(directory.c_str());
        if(!search->directory || !kinoko_directory_next(search.get())) return nullptr;
#endif
        return search.release();
    } catch(...) { return nullptr; }
}
extern "C" int kinoko_directory_next(KinokoDirectorySearch* search) {
    if(!search) return 0;
#if defined(_WIN32)
    WIN32_FIND_DATAA next{};
    if(!FindNextFileA(search->handle,&next)) return 0;
    search->current=next;
    return 1;
#else
    try {
        while(const auto* entry=readdir(search->directory)) {
            if(matches(search->pattern,entry->d_name)) { search->current=entry->d_name; return 1; }
        }
    } catch(...) {}
    return 0;
#endif
}
extern "C" const char* kinoko_directory_name(const KinokoDirectorySearch* search) {
    if(!search) return nullptr;
#if defined(_WIN32)
    return search->current.cFileName;
#else
    return search->current.c_str();
#endif
}
extern "C" int kinoko_directory_close(KinokoDirectorySearch* search) {
    if(!search) return 0;
#if defined(_WIN32)
    const bool ok=FindClose(search->handle)!=0;
    search->handle=INVALID_HANDLE_VALUE;
#else
    const bool ok=closedir(search->directory)==0;
    search->directory=nullptr;
#endif
    delete search;
    return ok;
}
