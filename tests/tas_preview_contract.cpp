#include "kinoko/gpu_renderer.hpp"
#include "kinoko/runtime_options.hpp"
#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <vector>
int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("Expected shader directory");
        namespace fs=std::filesystem;
        auto root=fs::current_path()/"Testing"/"fixtures"/("tas-preview-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(root);
        std::vector<std::string> args={"contract","--save-dir",root.u8string(),"--record",(root/"unused.krec").u8string(),"--replay-status",(root/"unused.txt").u8string(),"--replay-identity",std::string(64,'a'),"--tas-dir",root.u8string(),"--tas-output",(root/"branch.krec").u8string()};
        std::vector<char*> av;for(auto& a:args)av.push_back(a.data());std::string error;
        if(!kinoko::runtime::parse_options(int(av.size()),av.data(),error))throw std::runtime_error(error);
#ifdef _WIN32
        const auto encoding=kinoko::gpu::ShaderEncoding::dxbc;const char* extension="dxbc";const char* entry="main";
#elif defined(__APPLE__)
        const auto encoding=kinoko::gpu::ShaderEncoding::msl;const char* extension="msl";const char* entry="main0";
#else
        const auto encoding=kinoko::gpu::ShaderEncoding::spirv;const char* extension="spv";const char* entry="main";
#endif
        auto read=[&](const char* stage){std::ifstream in(fs::u8path(argv[1])/(std::string("sprite.")+stage+"."+extension),std::ios::binary);return std::vector<char>(std::istreambuf_iterator<char>(in),{});};
        auto vertex=read("vert"),fragment=read("frag");
        if(vertex.empty()||fragment.empty())throw std::runtime_error("Missing shaders");
        if(!SDL_Init(SDL_INIT_VIDEO))throw std::runtime_error(SDL_GetError());
        auto* window=SDL_CreateWindow("TAS preview contract",640,480,SDL_WINDOW_HIDDEN);
        if(!window)throw std::runtime_error(SDL_GetError());
        {kinoko::gpu::Renderer renderer(window,encoding,{vertex.data(),vertex.size(),entry},{fragment.data(),fragment.size(),entry});
        kinoko::gpu::Pass pass;pass.clear_color={1,0,0,1};
        if(!renderer.present({pass},42))throw std::runtime_error("No offscreen frame");}
        SDL_DestroyWindow(window);SDL_Quit();
        std::ifstream in(root/"image.rgba",std::ios::binary);std::vector<unsigned char> b(std::istreambuf_iterator<char>(in),{});
        if(b.size()!=24+640*480*4 || b[8]!=42 || b[24]!=255 || b[25]!=0 || b[26]!=0 || b[27]!=255)throw std::runtime_error("Readback pixels/frame label mismatch");
        std::cout<<"PASS hidden offscreen RGBA readback and exact frame label; retained "<<root.u8string()<<'\n';return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
