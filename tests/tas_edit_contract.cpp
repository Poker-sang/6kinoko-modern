#include "kinoko/tas_edit_plan.hpp"
#include "kinoko/tas_bridge.hpp"
#include "kinoko/runtime_options.hpp"
#include "kinoko/runtime_clock.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <future>
#include <thread>
int main(){
 try {
    auto wire=[](uint32_t first,uint32_t mask){std::string s="KTASED01";auto word=[&](uint32_t n){for(int i=0;i<4;++i)s+=char(n>>(i*8));};word(3);word(first);word(0);word(mask);word(0);return s;};
    auto check=[](bool ok){if(!ok)throw std::runtime_error("TAS edit contract failed");};
    auto bytes=wire(1,16);std::istringstream good(bytes);kinoko::tas::EditPlan p;p.read(good);check(p.first==1&&p.masks[1]==16);
    for(auto bad:{bytes.substr(0,3),bytes.substr(0,bytes.size()-1),bytes+"x",wire(3,0),wire(0,1u<<19)}){
        bool rejected=false;try{std::istringstream in(bad);p.read(in);}catch(const std::exception&){rejected=true;}check(rejected);
    }
    namespace fs=std::filesystem;
    auto root=fs::current_path()/"Testing"/("tas-edits-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));fs::create_directories(root);
    std::vector<std::string> args={"contract","--save-dir",root.u8string(),"--replay",(root/"source.krec").u8string(),"--replay-status",(root/"status").u8string(),"--replay-identity",std::string(64,'a'),"--tas-dir",root.u8string(),"--tas-output",(root/"out.krec").u8string()};
    std::vector<char*> av;for(auto& a:args)av.push_back(a.data());std::string error;check(kinoko::runtime::parse_options(int(av.size()),av.data(),error));
    {std::ofstream out(root/"edit.bin",std::ios::binary);out<<bytes;}
    kinoko::tas::start();check(kinoko::tas::boundary(0,3,false));check(!kinoko::tas::take_control());
    {std::ofstream out(root/"command.txt");out<<"1 target 3";}
    kinoko::tas::completed(1);check(kinoko::tas::boundary(1,3,false));check(kinoko::tas::take_control());
    check(kinoko::tas::boundary(1,3,true));check(kinoko::tas::input_mask()==16);
    kinoko::tas::completed(2);check(kinoko::tas::boundary(2,3,true));check(kinoko::tas::input_mask()==0);
    {std::ofstream out(root/"command.txt");out<<"2 stop 0";}
    check(!kinoko::tas::boundary(3,3,true));
    {std::ofstream out(root/"edit.bin",std::ios::binary);out<<wire(0,16);}
    {std::ofstream out(root/"command.txt");out<<"1 target 1";}
    kinoko::tas::start();check(kinoko::tas::boundary(0,3,false));check(kinoko::tas::take_control());
    {std::ofstream out(root/"command.txt");out<<"2 speed 25";}
    check(kinoko::tas::boundary(0,3,true));check(kinoko::tas::frame_interval_ns()==100000000000ull/(60*25));
    {std::ofstream out(root/"command.txt");out<<"3 speed 400";}
    check(kinoko::tas::boundary(0,3,true));check(kinoko::tas::frame_interval_ns()==100000000000ull/(60*400));
    kinoko_simulation_enable(1);kinoko_simulation_frame(17);auto clock=kinoko_simulation_milliseconds();kinoko::tas::pace_frame();check(kinoko_simulation_milliseconds()==clock);kinoko_simulation_enable(0);
    auto variable=[](uint32_t size,uint32_t first,uint32_t source){std::string result="KTASED02";auto word=[&](uint32_t number){for(int byte=0;byte<4;++byte)result+=char(number>>(8*byte));};word(size);word(first);word(source);for(uint32_t frame=0;frame<size;++frame)word(0);return result;};
    for(auto plan:{variable(2,1,3),variable(5,3,3)}){std::istringstream stream(plan);kinoko::tas::EditPlan parsed;parsed.read(stream);check(parsed.source_count==3);}
    bool invalid=false;try{std::istringstream stream(variable(5,4,3));kinoko::tas::EditPlan parsed;parsed.read(stream);}catch(const std::exception&){invalid=true;}check(invalid);
    fs::remove(root/"edit.bin");
    auto command=[&](const char* text){std::ofstream out(root/"command.txt");out<<text;};
    auto wait=[&](auto ready){const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(3);while(!ready() && std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(std::chrono::milliseconds(1));return ready();};
    for(bool cancel:{false,true}) {
        command("1 seek 3");kinoko::tas::start();check(kinoko::tas::boundary(0,3,false));
        check(kinoko::tas::fast_seeking() && !kinoko::tas::publish_preview(2) && kinoko::tas::publish_preview(3));
        kinoko_simulation_enable(1);kinoko_simulation_frame(17);const auto simulation_clock=kinoko_simulation_milliseconds();
        const auto begin=std::chrono::steady_clock::now();for(int i=0;i<60;++i)kinoko::tas::pace_frame();
        check(std::chrono::steady_clock::now()-begin<std::chrono::milliseconds(200));
        check(kinoko_simulation_milliseconds()==simulation_clock);kinoko_simulation_enable(0);
        const uint64_t frame=cancel?2:3;kinoko::tas::completed(frame);
        const uint32_t pixel=0xff123456;
        if(cancel)command("2 pause 0");else kinoko::tas::image(frame,1,1,&pixel);
        auto paused=std::async(std::launch::async,[&]{return kinoko::tas::boundary(frame,3,false);});
        bool requested=true;
        if(cancel){requested=wait([&]{return kinoko::tas::requested_preview()==frame;});kinoko::tas::image(frame,1,1,&pixel);}
        const bool restored=wait([&]{return !kinoko::tas::fast_seeking();});
        command("3 stop 0");
        if(!requested || !restored)kinoko::tas::shutdown();
        check(!paused.get() && requested && restored);
        check(kinoko::tas::requested_preview()==0 && kinoko::tas::publish_preview(frame));
    }
    std::cout<<"PASS edit plan validation, verified-prefix takeover, frame-zero takeover and exact masks\n";
    return 0;
 }catch(const std::exception& e){std::cerr<<e.what();return 1;}
}
