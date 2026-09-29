#include "kinoko/input_actions.hpp"
#include "kinoko/input_manager.h"
#include "kinoko/input_service.h"
#include "kinoko/platform.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <string>

namespace {
int key_scan(const std::string& name) {
    auto code=SDL_GetScancodeFromName(name.c_str());
    return code==SDL_SCANCODE_UNKNOWN ? -1:kinoko::platform::legacy_scan(code);
}
const kinoko::input::Bindings& bindings() {
    static const auto value=[] {
        auto result=kinoko::input::classic_bindings();
        const char* base=SDL_GetBasePath();
        if(!base)return result;
        const std::string path=std::string(base)+"input-actions.cfg";
        SDL_PathInfo info{};
        if(!SDL_GetPathInfo(path.c_str(),&info))return result;
        size_t size=0;void* bytes=SDL_LoadFile(path.c_str(),&size);
        std::string error;
        if(!bytes)error="Cannot read input-actions.cfg";
        else {
            kinoko::input::parse_bindings(std::string_view(static_cast<char*>(bytes),size),result,error,key_scan);
            SDL_free(bytes);
        }
        if(!error.empty())SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Input bindings",
            (error+"\nUsing original keyconfig.dat bindings for all actions.").c_str(),nullptr);
        return result;
    }();
    return value;
}
}
extern "C" void kinoko_input_actions_update(KinokoInputManager* manager) {
    if(!manager)return;
    using namespace kinoko::input;
    static_assert(Count==19);
    Sample sample;
    sample.x=manager->published.x;sample.y=manager->published.y;
    std::copy_n(manager->published.buttons,6,sample.buttons);
    for(int i=0;i<256;++i)sample.keys[i]=kinoko_input_key_down(i)!=0;
    for(int i=0;i<kinoko_input_snapshot.controller_count;++i)
        for(int b=0;b<32;++b)sample.pad[b]|=kinoko_input_snapshot.controllers[i].buttons[b]!=0;
    Frame frame;
    std::copy_n(manager->actions.held,Count,frame.held);
    advance(bindings(),sample,frame);
    std::copy_n(frame.held,Count,manager->actions.held);
    std::copy_n(frame.released,Count,manager->actions.released);
    auto* out=manager->actions.published;
    std::copy_n(frame.held,Count,out);
    // Negative direction wins, matching original keyboard opposite-key priority.
    out[Count]=out[Left] ? -out[Left]:out[Right];
    out[Count+1]=out[Up] ? -out[Up]:out[Down];
    out[Count+2]=out[MenuLeft] ? -out[MenuLeft]:out[MenuRight];
    out[Count+3]=out[MenuUp] ? -out[MenuUp]:out[MenuDown];
    out[Door]=-out[Door];out[PipeUp]=-out[PipeUp];
}
