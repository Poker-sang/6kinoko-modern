#include "kinoko/critical_section.h"
#include "kinoko/graphics_device.h"
#include "kinoko/string_font.h"
#include "kinoko/texture_store.h"
#include "kinoko/com_owner.hpp"
#include "kinoko/runtime_util.hpp"
#include <SDL3/SDL.h>
#include <cmath>
#include <stdexcept>
#include <string>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "kinoko/graphics_api.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
#include <list>

namespace {
using Renderer=kinoko::text::FontRenderer;
struct GraphicsLock {
    GraphicsLock() { kinoko_graphics_lock.native->lock(); }
    ~GraphicsLock() { kinoko_graphics_lock.native->unlock(); }
};
struct FontData {
    std::vector<unsigned char> bytes;
    stbtt_fontinfo font{};
    FontData() {
        const char* base=SDL_GetBasePath();
        if(!base)throw std::runtime_error(SDL_GetError());
        const auto path=std::string(base)+"fonts/NotoSansCJKjp-Regular.otf";
        size_t size=0;void* loaded=SDL_LoadFile(path.c_str(),&size);
        if(!loaded)throw std::runtime_error("Missing staged font: "+path);
        bytes.assign(static_cast<unsigned char*>(loaded),static_cast<unsigned char*>(loaded)+size);
        SDL_free(loaded);
        if(!stbtt_InitFont(&font,bytes.data(),0))throw std::runtime_error("Invalid staged font: "+path);
    }
};
const stbtt_fontinfo& font(){static const FontData data;return data.font;}
struct FontSession {
    Renderer* r;
    explicit FontSession(Renderer* value):r(value) {
        int ascent,descent,gap;stbtt_GetFontVMetrics(&font(),&ascent,&descent,&gap);
        r->ascent=static_cast<int32_t>(std::ceil(ascent*stbtt_ScaleForPixelHeight(&font(),float(std::max(1,r->font_height)))));
        r->cursor_x=int32_t(r->edge)+r->margin_left;r->cursor_y=int32_t(r->edge)+r->margin_top;
        r->gradient=r->bitmap;r->clear_pixels();
    }
    ~FontSession(){r->clear_pixels();}
};
void glyph(Renderer* r,uint32_t character,int32_t& width,int32_t& height) {
    const auto& f=font();const float scale=stbtt_ScaleForPixelHeight(&f,float(std::max(1,r->font_height)));
    const int index=stbtt_FindGlyphIndex(&f,static_cast<int>(character));
    int x0,y0,x1,y1,advance,bearing;
    stbtt_GetGlyphBitmapBox(&f,index,scale,scale,&x0,&y0,&x1,&y1);
    stbtt_GetGlyphHMetrics(&f,index,&advance,&bearing);
    const int bw=x1-x0,bh=y1-y0;
    const int bold=r->font_weight>=600?1:0;
    const int slant=r->italic?(bh+3)/4:0;
    int max_x=r->cursor_x+x1+bold+slant;
    if(max_x>r->bound_width && r->wrap){r->cursor_x=r->margin_left+r->edge;r->cursor_y+=r->font_height+r->line_space;max_x=r->cursor_x+x1+bold+slant;}
    const int max_y=r->cursor_y+r->ascent+y1;
    if(max_y>=r->bound_height || max_x>r->bound_width)return;
    if(bw>0 && bh>0){
        std::vector<unsigned char> bitmap(size_t(bw)*bh);
        stbtt_MakeGlyphBitmap(&f,bitmap.data(),bw,bh,bw,scale,scale,index);
        for(int y=0;y<bh;++y){
            const int dy=r->cursor_y+r->ascent+y0+y;
            if(dy<0||dy>=r->bound_height)continue;
            const uint32_t rgb=r->gradient?r->gradient[std::max(0,r->ascent+y0+y)]:r->color;
            for(int x=0;x<bw;++x){
                const int dx=r->cursor_x+x0+x+(r->italic?(bh-1-y)/4:0);
                for(int b=0;b<=bold;++b){
                    if(dx+b<0||dx+b>=r->bound_width)continue;
                    auto& pixel=r->output[size_t(dy)*r->stride+dx+b];
                    const auto alpha=std::max(pixel>>24,uint32_t(bitmap[size_t(y)*bw+x]));
                    pixel=(rgb&0xffffffu)|(alpha<<24);
                }
            }
        }
    }
    r->cursor_x+=static_cast<int32_t>(std::lround(advance*scale))+r->character_space;
    width=std::max(width,max_x);height=std::max(height,max_y);
}
void outline(Renderer* r,const uint32_t* source,uint32_t* destination) {
    const int32_t stride=r->stride;
    for(int32_t y=1;y<r->bound_height-1;++y)
        for(int32_t x=1;x<r->bound_width-1;++x) {
            const int32_t i=y*stride+x;
            const uint32_t pixel=source[i],alpha=pixel>>24;
            if(alpha) {
                const uint32_t factor=(std::min)(alpha+33,256u);
                destination[i]=0xff000000u|
                    (((factor*((pixel>>16)&255))>>8)<<16)|
                    (((factor*((pixel>>8)&255))>>8)<<8)|((factor*(pixel&255))>>8);
            } else {
                const uint32_t a=(std::max)({source[i-1]>>24,source[i+1]>>24,
                                            source[i-stride]>>24,source[i+stride]>>24});
                destination[i]=(destination[i]&0x00ffffffu)|(a<<24);
            }
        }
}
}
extern "C" void kinoko_string_font_rasterize(Renderer* r,const char* character,int32_t* width,int32_t* height) {
    const bool edge=r->edge!=0;
    std::vector<uint32_t> temporary;
    if(edge) {
        temporary.resize(size_t(r->stride)*r->bound_height);
        r->output = temporary.data();
    }
    int32_t w=0,h=0;
    {
        FontSession session(r);
        if(character && *character && *character!='<')
            glyph(r,kinoko::text::codepoint(character),w,h);
    }
    if(edge) outline(r,temporary.data(),
        static_cast<uint32_t*>(r->destination));
    if(width) *width=w+edge;
    if(height) *height=h+edge;
}
extern "C" int32_t kinoko_string_font_texture(Renderer* r) {
    kinoko::ComOwner<kinoko::graphics::Texture> texture;
    kinoko::graphics::Texture* value=nullptr;
    {
        GraphicsLock lock;
        if(kinoko::graphics::failed(kinoko::graphics::create_texture(kinoko_graphics.device,512,512,1,0,
            kinoko::graphics::format_a8r8g8b8,kinoko::graphics::pool_managed,&value))) return 0;
    }
    texture.reset(value);
    {
        GraphicsLock lock;kinoko::graphics::MappedPixels rect{};
        if(kinoko::graphics::failed(value->LockRect(0,&rect,nullptr,0))) return 0;
        std::memset(rect.pBits,0,4*512*512);
        r->output = static_cast<uint32_t*>(rect.pBits);
        r->destination = static_cast<uint32_t*>(rect.pBits);
        r->bound_height = 512;
        r->bound_width = 512;
        r->stride = static_cast<int32_t>(rect.Pitch/4);
        kinoko_string_font_rasterize(r, "", nullptr, nullptr);
        value->UnlockRect(0);
    }
    const int32_t handle=kinoko_texture_register(value,512,512);
    if(handle) texture.detach();
    return handle;
}
extern "C" void kinoko_string_font_upload(Renderer* r,int32_t handle,const char* character,
                                          int32_t x,int32_t y,int32_t* width,int32_t* height) {
    if(handle<=0 || handle>=KINOKO_TEXTURE_CAPACITY) return;
    auto* texture=static_cast<kinoko::graphics::Texture*>(kinoko_texture_slots[handle].texture);
    if(!texture) return;
    GraphicsLock lock;
    kinoko::ComOwner<kinoko::graphics::Surface> surface;
    kinoko::graphics::Surface* value=nullptr;
    if(kinoko::graphics::failed(texture->GetSurfaceLevel(0,&value))) return;
    surface.reset(value);kinoko::graphics::SurfaceDescription description{};value->GetDesc(&description);surface.reset();
    kinoko::graphics::PixelRect region{x,y,static_cast<int32_t>(description.Width),static_cast<int32_t>(description.Height)};
    kinoko::graphics::MappedPixels rect{};
    if(kinoko::graphics::failed(texture->LockRect(0,&rect,&region,0))) return;
    try {
        std::vector<unsigned char> pixels(size_t(description.Height-y)*rect.Pitch);
        r->output = reinterpret_cast<uint32_t*>(pixels.data());
        r->destination = reinterpret_cast<uint32_t*>(pixels.data());
        r->bound_height = static_cast<int32_t>(description.Height-y);
        r->bound_width = static_cast<int32_t>(description.Width-x);
        r->stride = static_cast<int32_t>(rect.Pitch/4);
        kinoko_string_font_rasterize(r, character, width, height);
        const uint32_t bytes_per_pixel=rect.Pitch/description.Width;
        for(int32_t row=0;row<*height;++row)
            kinoko::copy_bytes(static_cast<unsigned char*>(rect.pBits)+row*rect.Pitch,
                (description.Width-x)*bytes_per_pixel,pixels.data()+row*rect.Pitch,
                *width*bytes_per_pixel);
    } catch(...) { texture->UnlockRect(0);throw; }
    texture->UnlockRect(0);
}

