#include "kinoko/tas_bridge.hpp"
#include "kinoko/tas_edit_plan.hpp"
#include "kinoko/runtime_options.hpp"
#include <SDL3/SDL.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <limits>
#include <vector>
namespace kinoko::tas {
namespace {
std::atomic<bool> stopping{false};
std::atomic<uint64_t> count{0};
std::atomic<uint32_t> window_mask{0};
std::atomic<int> window_command{0};
uint64_t sequence=0,target=1;
bool takeover=false,free_run=false;
uint64_t last_input=0;uint32_t mask=0;
std::string phase;
EditPlan edits;
const auto& directory(){return runtime::options().tas_dir;}
bool replace(const std::filesystem::path& temporary,const std::filesystem::path& destination) {
    // Same-directory publication. Readers open with delete sharing and retry a missing mailbox.
    std::error_code ec;std::filesystem::remove(destination,ec);ec.clear();
    std::filesystem::rename(temporary,destination,ec);return !ec;
}
void publish(uint64_t frames,uint64_t total,const char* state) {
    const std::string key=std::to_string(sequence)+" "+std::to_string(frames)+" "+std::to_string(total)+" "+state;
    if(phase==key)return;
    auto path=directory()/"state.tmp";
    {std::ofstream out(path);out<<"KTAS1 "<<key<<'\n';if(!out)return;}
    if(replace(path,directory()/"state.txt"))phase=key;
}
}
bool enabled(){return !directory().empty();}
bool embedded(){return enabled() && !runtime::options().tas_window;}
void pump_window(SDL_Window* window) {
    if(!enabled() || embedded())return;
    const bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0;
    const bool* keys=SDL_GetKeyboardState(nullptr);uint32_t value=0;
    auto set=[&](SDL_Scancode key,uint32_t bits){if(focused && keys[key])value|=bits;};
    set(SDL_SCANCODE_LEFT,(1u<<0)|(1u<<15));set(SDL_SCANCODE_RIGHT,(1u<<1)|(1u<<16));
    set(SDL_SCANCODE_UP,(1u<<2)|(1u<<8)|(1u<<9)|(1u<<17));set(SDL_SCANCODE_DOWN,(1u<<3)|(1u<<10)|(1u<<18));
    set(SDL_SCANCODE_Z,(1u<<4)|(1u<<11));set(SDL_SCANCODE_X,(1u<<5)|(1u<<6)|(1u<<7));
    set(SDL_SCANCODE_A,(1u<<12)|(1u<<13));set(SDL_SCANCODE_C,1u<<14);
    set(SDL_SCANCODE_SPACE,1u<<4);set(SDL_SCANCODE_RETURN,1u<<11);set(SDL_SCANCODE_ESCAPE,1u<<13);
    window_mask=value;
    static bool prev_step=false,prev_toggle=false;
    const bool step=focused&&keys[SDL_SCANCODE_F10],toggle=focused&&keys[SDL_SCANCODE_F9];
    if(step&&!prev_step)window_command=1;
    else if(toggle&&!prev_toggle)window_command=2;
    prev_step=step;prev_toggle=toggle;
}
void start(){
    if(!enabled())return;
    stopping=false;count=0;sequence=0;target=1;takeover=false;free_run=false;mask=0;last_input=0;phase.clear();edits={};
    const auto plan=directory()/"edit.bin";
    if(std::filesystem::exists(plan)){std::ifstream in(plan,std::ios::binary);edits.read(in);}
    std::ofstream capabilities(directory()/"capabilities.txt");capabilities<<"KTAS1 edits-v1\n";
}
void shutdown(){stopping=true;}
bool boundary(uint64_t frames,uint64_t total,bool live) {
    if(!enabled())return true;
    while(!stopping.load()) {
        const int local=window_command.exchange(0);
        if(edits.masks.empty() && local==1){free_run=false;target=frames+1;}
        if(edits.masks.empty() && local==2){free_run=!free_run;target=frames;}
        uint64_t seq=0,arg=0;std::string verb;bool read=false;
        {std::ifstream in(directory()/"command.txt");read=bool(in>>seq>>verb>>arg);}
        if(read && seq>sequence) {
            sequence=seq;
            if(verb=="stop"){stopping=true;return false;}
            if(verb=="pause"){free_run=false;target=frames;}
            if(verb=="target"){free_run=false;target=arg;}
            if(verb=="run")free_run=true;
            if(verb=="takeover"){takeover=true;free_run=false;target=frames;mask=0;}
        }
        if(!edits.masks.empty()){total=edits.masks.size();if(frames>=total){free_run=false;target=frames;}}
        if(!live && frames>=total && !takeover) {free_run=false;target=frames;}
        const bool advance=free_run || frames<target;
        publish(frames,total,advance?(live?"live":"playing"):(live?"live-paused":"paused"));
        if(advance && !live && !edits.masks.empty() && frames==edits.first)takeover=true;
        if(takeover)return true; // Runtime changes mode, then re-enters this boundary.
        if(advance)return true;
        SDL_Delay(2);
    }
    return false;
}
bool take_control(){const bool value=takeover;takeover=false;return value;}
uint32_t input_mask() {
    if(!edits.masks.empty()){const auto frame=count.load();if(frame>=edits.masks.size())throw std::runtime_error("TAS edit plan exhausted");return edits.masks[size_t(frame)];}
    std::ifstream in(directory()/"input.txt");uint64_t stamp;uint32_t value;
    if(in>>stamp>>value) {if(stamp!=last_input){last_input=stamp;mask=value;}}
    // The editor sends a zero mask on focus loss and refreshes its input lease every tick.
    std::error_code ec;const auto time=std::filesystem::last_write_time(directory()/"input.txt",ec);
    if(ec || std::filesystem::file_time_type::clock::now()-time>std::chrono::seconds(1))mask=0;
    return (mask | window_mask.load()) & ((1u<<19)-1);
}
void completed(uint64_t frames){count=frames;}
uint64_t frame_number(){return count.load();}
void failure(const std::string& message) {if(enabled()){std::ofstream out(directory()/"error.txt");out<<message;publish(count,0,"failed");}}
void finish(){if(enabled())publish(count,0,"finished");}
void image(uint64_t frames,uint32_t width,uint32_t height,const void* rgba) {
    if(!enabled() || !width || !height || width>4096 || height>4096)return;
    auto path=directory()/"image.tmp";
    {std::ofstream out(path,std::ios::binary);out.write("KTASIMG1",8);
    auto number=[&](uint64_t n,int size){for(int i=0;i<size;i++)out.put(char(n>>(8*i)));};
    number(frames,8);number(width,4);number(height,4);out.write(static_cast<const char*>(rgba),size_t(width)*height*4);if(!out)return;}
    replace(path,directory()/"image.rgba");
}
}
