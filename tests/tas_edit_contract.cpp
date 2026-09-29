#include "kinoko/tas_edit_plan.hpp"
#include "kinoko/tas_bridge.hpp"
#include "kinoko/runtime_options.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
int main(){
 try {
    auto wire=[](uint32_t first,uint32_t mask){std::string s="KTASED01";auto word=[&](uint32_t n){for(int i=0;i<4;++i)s+=char(n>>(i*8));};word(3);word(first);word(0);word(mask);word(0);return s;};
    auto check=[](bool ok){if(!ok)throw std::runtime_error("TAS edit contract failed");};
    auto bytes=wire(1,16);std::istringstream good(bytes);kinoko::tas::EditPlan p;p.read(good);check(p.first==1&&p.masks[1]==16);
    for(auto bad:{bytes.substr(0,bytes.size()-1),bytes+"x",wire(3,0),wire(0,1u<<19)}){
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
    kinoko::tas::start();check(kinoko::tas::boundary(0,3,false));check(kinoko::tas::take_control());
    std::cout<<"PASS edit plan validation, verified-prefix takeover, frame-zero takeover and exact masks\n";
    return 0;
 }catch(const std::exception& e){std::cerr<<e.what();return 1;}
}
