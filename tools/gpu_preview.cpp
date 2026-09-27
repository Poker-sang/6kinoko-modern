#include "kinoko/gpu_renderer.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
std::vector<char> shader(const char* name) {
    const char* base=SDL_GetBasePath();if(!base)throw std::runtime_error(SDL_GetError());
    std::ifstream file(std::string(base)+"shaders/"+name,std::ios::binary);
    if(!file)throw std::runtime_error(std::string("Cannot load staged shader: ")+name);
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
kinoko::gpu::Draw quad(kinoko::gpu::TextureId texture,float x,float y,float w,float h,uint32_t color=0xffffffffu) {
    kinoko::gpu::Draw d;d.texture=texture;
    d.vertices={{{x-.5f,y-.5f,.5f,1,color,0,0},{x+w-.5f,y-.5f,.5f,1,color,1,0},
                 {x-.5f,y+h-.5f,.5f,1,color,0,1},{x+w-.5f,y+h-.5f,.5f,1,color,1,1}}};
    return d;
}
}
int main(int,char**) {
    if(!SDL_Init(SDL_INIT_VIDEO)) {std::cerr<<SDL_GetError();return 1;}
    auto* window=SDL_CreateWindow("SDL GPU 2D preview (not the game) - resize / Esc",800,600,SDL_WINDOW_RESIZABLE);
    if(!window) {std::cerr<<SDL_GetError();SDL_Quit();return 1;}
    int result=0;
    try {
        const auto vs=shader("sprite.vert.dxbc"),fs=shader("sprite.frag.dxbc");
        kinoko::gpu::Renderer renderer(window,kinoko::gpu::ShaderEncoding::dxbc,{vs.data(),vs.size()},{fs.data(),fs.size()});
        std::cout<<"GPU driver: "<<renderer.driver()<<std::endl;
        std::vector<uint8_t> pixels(32*32*4);
        for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x) {
            const unsigned i=(y*32+x)*4;const bool light=((x/4+y/4)%2)==0;
            pixels[i]=light?255:40;pixels[i+1]=light?180:80;pixels[i+2]=light?40:220;pixels[i+3]=255;
        }
        const auto image=renderer.create_texture(32,32,pixels.data(),pixels.size(),32*4);
        const auto target=renderer.create_texture(256,256,nullptr,0,0,true);
        kinoko::gpu::Pass offscreen;offscreen.target=target;offscreen.clear_color={.04f,.08f,.12f,1};
        offscreen.draws.push_back(quad(image,0,0,256,256));
        offscreen.draws.push_back(quad(0,50,50,156,156,0x8040ffff));
        kinoko::gpu::Pass screen;screen.logical_width=800;screen.logical_height=600;screen.clear_color={.12f,.12f,.15f,1};
        screen.draws.push_back(quad(target,24,24,256,256));
        screen.draws.push_back(quad(image,304,24,200,200));screen.draws.back().linear=true;
        screen.draws.push_back(quad(image,528,24,248,248));screen.draws.back().wrap_u=screen.draws.back().wrap_v=true;
        screen.draws.back().vertices[1].u=screen.draws.back().vertices[3].u=3;
        screen.draws.back().vertices[2].v=screen.draws.back().vertices[3].v=3;
        using kinoko::render::BlendFactor;using kinoko::render::BlendOperation;
        for(int i=0;i<5;++i) {
            screen.draws.push_back(quad(image,24.f+i*154,330,140,200));
            auto overlay=quad(0,40.f+i*154,355,108,150,0x8080ff40);
            if(i==1)overlay.blend.destination=BlendFactor::one;
            if(i==2) {overlay.blend.destination=BlendFactor::one;overlay.blend.operation=BlendOperation::reverse_subtract;}
            if(i==3) {overlay.blend.source=BlendFactor::zero;overlay.blend.destination=BlendFactor::source_color;}
            if(i==4) {overlay.blend.source=BlendFactor::destination_color;overlay.blend.destination=BlendFactor::one;}
            screen.draws.push_back(overlay);
        }
        const std::vector<kinoko::gpu::Pass> frame{offscreen,screen};
        bool running=true;
        while(running) {
            SDL_Event event;while(SDL_PollEvent(&event))
                if(event.type==SDL_EVENT_QUIT || (event.type==SDL_EVENT_KEY_DOWN&&event.key.key==SDLK_ESCAPE))running=false;
            if(running&&!renderer.present(frame))SDL_Delay(16);
        }
        renderer.destroy_texture(target);renderer.destroy_texture(image);
    } catch(const std::exception& e) {
        std::cerr<<e.what()<<std::endl;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"SDL GPU preview",e.what(),window);result=1;
    }
    SDL_DestroyWindow(window);SDL_Quit();return result;
}
