#include "kinoko/tas_bridge.hpp"
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
uint64_t sequence=0,target=1;
bool takeover=false,free_run=false;
uint64_t last_input=0;uint32_t mask=0;
std::string phase;
const auto& directory(){return runtime::options().tas_dir;}
void replace(const std::filesystem::path& temporary,const std::filesystem::path& destination) {
    // Same-directory publication. Readers open with delete sharing and retry a missing mailbox.
    std::error_code ec;std::filesystem::remove(destination,ec);ec.clear();
    std::filesystem::rename(temporary,destination,ec);
}
void publish(uint64_t frames,uint64_t total,const char* state) {
    const std::string key=std::to_string(sequence)+" "+std::to_string(frames)+" "+std::to_string(total)+" "+state;
    if(phase==key)return;
    auto path=directory()/"state.tmp";
    {std::ofstream out(path);out<<"KTAS1 "<<key<<'\n';if(!out)return;}
    replace(path,directory()/"state.txt");phase=key;
}
}
bool enabled(){return !directory().empty();}
void start(){if(!enabled())return;stopping=false;count=0;sequence=0;target=1;takeover=false;free_run=false;mask=0;last_input=SDL_GetTicks();phase.clear();}
void shutdown(){stopping=true;}
bool boundary(uint64_t frames,uint64_t total,bool live) {
    if(!enabled())return true;
    while(!stopping.load()) {
        std::ifstream in(directory()/"command.txt");uint64_t seq=0,arg=0;std::string verb;
        if(in>>seq>>verb>>arg && seq>sequence) {
            sequence=seq;
            if(verb=="stop"){stopping=true;return false;}
            if(verb=="pause"){free_run=false;target=frames;}
            if(verb=="target"){free_run=false;target=arg;}
            if(verb=="run")free_run=true;
            if(verb=="takeover"){takeover=true;free_run=false;target=frames;mask=0;}
        }
        if(!live && frames>=total && !takeover) {free_run=false;target=frames;}
        const bool advance=free_run || frames<target;
        publish(frames,total,advance?(live?"live":"playing"):(live?"live-paused":"paused"));
        if(takeover)return true; // Runtime changes mode, then re-enters this boundary.
        if(advance)return true;
        SDL_Delay(2);
    }
    return false;
}
bool take_control(){const bool value=takeover;takeover=false;return value;}
uint32_t input_mask() {
    std::ifstream in(directory()/"input.txt");uint64_t stamp;uint32_t value;
    if(in>>stamp>>value) {if(stamp!=last_input){last_input=stamp;mask=value;}}
    // The editor sends a zero mask on focus loss and refreshes its input lease every tick.
    std::error_code ec;const auto time=std::filesystem::last_write_time(directory()/"input.txt",ec);
    if(ec || std::filesystem::file_time_type::clock::now()-time>std::chrono::seconds(1))mask=0;
    return mask & ((1u<<19)-1);
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
