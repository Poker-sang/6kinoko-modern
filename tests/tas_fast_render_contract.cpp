#include "kinoko/graphics_api.hpp"
#include "kinoko/com_owner.hpp"
#include "kinoko/sprite_vertex.h"
#include "kinoko/platform.hpp"
#include "kinoko/runtime_options.hpp"
#include "kinoko/tas_bridge.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    try {
        namespace fs=std::filesystem;
        auto check=[](bool ok){if(!ok)throw std::runtime_error("Fast render contract failed");};
        const auto root=fs::current_path()/"Testing"/("fast-render-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(root);
        std::vector<std::string> args={"contract","--save-dir",root.u8string(),"--record",(root/"unused.krec").u8string(),"--replay-status",(root/"status").u8string(),"--replay-identity",std::string(64,'a'),"--tas-dir",root.u8string(),"--tas-output",(root/"branch.krec").u8string()};
        std::vector<char*> av;for(auto& arg:args)av.push_back(arg.data());std::string error;
        check(kinoko::runtime::parse_options(int(av.size()),av.data(),error));
        auto command=[&](const char* text){std::ofstream out(root/"command.txt");out<<text;};
        auto pixels=[&](uint64_t frame,uint32_t color){
            std::ifstream in(root/"image.rgba",std::ios::binary);std::vector<unsigned char> data(std::istreambuf_iterator<char>(in),{});
            check(data.size()==24+640*480*4 && data[8]==frame);
            check(data[24]==((color>>16)&255) && data[25]==((color>>8)&255) && data[26]==(color&255));
        };
        check(kinoko::platform::host().open("Fast render contract",640,480,true));
        {
            kinoko::graphics::Device device(640,480);
            auto frame=[&](uint64_t count,uint32_t color){
                kinoko::tas::completed(count);
                check(kinoko::graphics::succeeded(device.BeginScene()));
                check(kinoko::graphics::succeeded(device.Clear(0,nullptr,kinoko::graphics::clear_target|kinoko::graphics::clear_zbuffer,color,1,0)));
                check(kinoko::graphics::succeeded(device.EndScene()));
                check(kinoko::graphics::succeeded(device.TestCooperativeLevel()));
            };
            for(bool cancel:{false,true}) {
                fs::remove(root/"image.rgba");command("1 seek 6");kinoko::tas::start();
                check(kinoko::tas::boundary(0,6,true));
                for(uint64_t count=1;count<6;++count){frame(count,0xff00ff00);check(!fs::exists(root/"image.rgba"));}
                const uint64_t target=cancel?5:6;
                if(cancel)command("2 pause 0");else frame(6,0xffff0000);
                auto paused=std::async(std::launch::async,[&]{return kinoko::tas::boundary(target,6,true);});
                const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(3);
                while(kinoko::tas::fast_seeking() && std::chrono::steady_clock::now()<until){device.TestCooperativeLevel();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
                const bool restored=!kinoko::tas::fast_seeking();
                command("3 stop 0");if(!restored)kinoko::tas::shutdown();
                check(!paused.get() && restored);pixels(target,cancel?0xff00ff00:0xffff0000);
            }
            // An offscreen target generated in an intermediate mixed frame must
            // remain sampleable after a deferred screen-only frame.
            using namespace kinoko::graphics;
            command("1 seek 3");kinoko::tas::start();check(kinoko::tas::boundary(0,3,true));
            kinoko::ComOwner<Texture> texture;kinoko::ComOwner<Surface> target,back;
            check(succeeded(create_texture(&device,4,4,1,usage_rendertarget,format_a8r8g8b8,pool_default,texture.put())));
            check(succeeded(texture->GetSurfaceLevel(0,target.put())) && succeeded(device.GetRenderTarget(0,back.put())));
            kinoko::tas::completed(1);check(succeeded(device.BeginScene()));device.SetRenderTarget(0,target.get());
            device.Clear(0,nullptr,clear_target|clear_zbuffer,0xff0000ff,1,0);device.SetRenderTarget(0,back.get());
            device.Clear(0,nullptr,clear_target|clear_zbuffer,0xffff0000,1,0);device.EndScene();device.TestCooperativeLevel();
            frame(2,0xff00ff00);
            kinoko::tas::completed(3);check(succeeded(device.BeginScene()));
            device.Clear(0,nullptr,clear_target|clear_zbuffer,0xffff0000,1,0);device.SetTexture(0,texture.get());device.SetRenderState(state_cullmode,cull_none);
            const KinokoSpriteVertex vertices[]={{0,0,0,1,0xffffffff,0,0},{640,0,0,1,0xffffffff,1,0},{0,480,0,1,0xffffffff,0,1},{640,480,0,1,0xffffffff,1,1}};
            check(succeeded(device.DrawPrimitiveUP(primitive_trianglestrip,2,vertices,sizeof(KinokoSpriteVertex))));
            device.EndScene();device.TestCooperativeLevel();pixels(3,0xff0000ff);device.SetTexture(0,nullptr);
            // Report a synthetic comparison; this is not a gameplay benchmark.
            command("1 target 1");kinoko::tas::start();
            auto start=std::chrono::steady_clock::now();for(uint64_t count=1;count<=120;++count)frame(count,0xff123456);
            const auto normal=std::chrono::steady_clock::now()-start;
            command("1 seek 121");kinoko::tas::start();check(kinoko::tas::boundary(0,121,true));
            start=std::chrono::steady_clock::now();for(uint64_t count=1;count<=120;++count)frame(count,0xff123456);frame(121,0xff123456);
            const auto fast=std::chrono::steady_clock::now()-start;pixels(121,0xff123456);
            std::cout<<"120 synthetic screen frames: normal_ms="<<std::chrono::duration<double,std::milli>(normal).count()<<", fast_ms="<<std::chrono::duration<double,std::milli>(fast).count()<<'\n';
        }
        kinoko::tas::shutdown();kinoko::platform::host().close();
        std::cout<<"PASS deferred screen rendering and exact target/cancel pixels; retained "<<root.u8string()<<'\n';return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
