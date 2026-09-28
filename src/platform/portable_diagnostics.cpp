#include "kinoko/diagnostics.h"
#include "kinoko/diagnostics_filter.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
namespace { std::mutex mutex; FILE* file=nullptr; bool verbose=false; }
extern "C" void kinoko_diagnostics_initialize() {
    const char* enabled=SDL_getenv("KINOKO_TRACE");
    verbose=SDL_getenv("KINOKO_TRACE_VERBOSE") && std::strcmp(SDL_getenv("KINOKO_TRACE_VERBOSE"),"1")==0;
    if(enabled && std::strcmp(enabled,"1")==0) {
        const char* base=SDL_GetBasePath();
        if(base)file=std::fopen((std::string(base)+"kinoko-trace.log").c_str(),"ab");
    }
}
extern "C" void kinoko_diagnostics_shutdown(){std::lock_guard<std::mutex> lock(mutex);if(file)std::fclose(file);file=nullptr;}
extern "C" int kinoko_diagnostics_accepts(const char* label){return file && label && kinoko::diagnostics::accepts(label,verbose,kinoko::diagnostics::TraceFilter::all);}
extern "C" void kinoko_trace(const char* text){if(!kinoko_diagnostics_accepts(text))return;std::lock_guard<std::mutex> lock(mutex);std::fprintf(file,"%s\n",text);std::fflush(file);}
extern "C" void kinoko_trace_hresult(const char* label,long value){char text[256];std::snprintf(text,sizeof(text),"%s:0x%08x",label,static_cast<unsigned>(value));kinoko_trace(text);}
