#include "kinoko/critical_section.h"
#include "kinoko/graphics_device.h"
#include "kinoko/string_font.h"
#include "kinoko/texture_store.h"
#include "kinoko/com_owner.hpp"
#include <windows.h>
#include "kinoko/graphics_api.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
#include <list>

extern "C" {
extern char *kinoko_game_window_slot;

}
namespace {
inline char*& game_window_slot = kinoko_game_window_slot;
using Renderer=kinoko::text::FontRenderer;
struct GraphicsLock {
    GraphicsLock() { EnterCriticalSection(&kinoko_graphics_lock.native); }
    ~GraphicsLock() { LeaveCriticalSection(&kinoko_graphics_lock.native); }
};
// 40F1C0/40F2E0. Each character creates/selects a font, then restores the DC.
struct FontSession {
    Renderer* r;
    explicit FontSession(Renderer* renderer):r(renderer) {
        auto font=CreateFontA(r->font_height,0,0,0,
            r->font_weight,r->italic,
            0,0,128,4,0,2,49,reinterpret_cast<char*>(r->face));
        r->font_handle = static_cast<void*>(font);
        auto dc=GetDC(reinterpret_cast<HWND>(game_window_slot));
        r->device_context = static_cast<void*>(dc);
        r->previous_font = static_cast<void*>(SelectObject(dc,font));
        TEXTMETRICA metrics{};GetTextMetricsA(dc,&metrics);
        r->ascent = static_cast<int32_t>(metrics.tmAscent);
        r->cursor_x = int32_t(r->edge)+r->margin_left;
        r->cursor_y = int32_t(r->edge)+r->margin_top;
        r->gradient = r->bitmap;
        r->clear_pixels();
    }
    ~FontSession() {
        r->clear_pixels();
        auto dc=static_cast<HDC>(r->device_context);
        DeleteObject(SelectObject(dc,static_cast<HGDIOBJ>(r->previous_font)));
        ReleaseDC(reinterpret_cast<HWND>(game_window_slot),dc);
        r->device_context = static_cast<void*>(nullptr);
        r->font_handle = static_cast<void*>(nullptr);
        r->previous_font = static_cast<void*>(nullptr);
    }
};
void glyph(Renderer* r,UINT character,int32_t& width,int32_t& height) {
    MAT2 transform{};transform.eM11.value=transform.eM22.value=1;
    GLYPHMETRICS metrics{};
    auto dc=static_cast<HDC>(r->device_context);
    const DWORD size=GetGlyphOutlineA(dc,character,GGO_GRAY4_BITMAP,&metrics,0,nullptr,&transform);
    if(size==GDI_ERROR) return;
    // 40F3C8 has no zero-height guard. Preserve the recovered operation rather
    // than inventing space metrics; this path still awaits user validation.
    const uint32_t pitch=(size/metrics.gmBlackBoxY)&~3u;
    const int32_t max_x=static_cast<int32_t>(metrics.gmBlackBoxX)+metrics.gmptGlyphOrigin.x+r->cursor_x;
    const int32_t max_y=static_cast<int32_t>(metrics.gmBlackBoxY)+r->cursor_y+r->ascent-metrics.gmptGlyphOrigin.y;
    if(max_y>=r->bound_height) return;
    if(max_x>r->bound_width) {
        if(!r->wrap) return;
        r->cursor_x = r->margin_left+r->edge;
        r->cursor_y = r->cursor_y+
            r->font_height+r->line_space;
    }
    if(size) {
        std::vector<unsigned char> bitmap(size);
        GetGlyphOutlineA(dc,character,GGO_GRAY4_BITMAP,&metrics,size,bitmap.data(),&transform);
        const int32_t stride=r->stride;
        auto* output=static_cast<uint32_t*>(r->output)+metrics.gmptGlyphOrigin.x+r->cursor_x
            +stride*(r->cursor_y+r->ascent-metrics.gmptGlyphOrigin.y);
        const auto* gradient=static_cast<uint32_t*>(r->gradient);
        if(gradient) gradient+=r->ascent-metrics.gmptGlyphOrigin.y;
        for(uint32_t y=0;y<metrics.gmBlackBoxY;++y) {
            const uint32_t rgb=gradient?gradient[y]:r->color;
            for(uint32_t x=0;x<metrics.gmBlackBoxX;++x)
                output[y*stride+x]=rgb|((0x0ff00000u*bitmap[y*pitch+x])&0xff000000u);
        }
    }
    // FontSession resets accent/ruby before each single-character call, so
    // neither branch of the generic tagged renderer is reachable here.
    r->cursor_x = r->cursor_x+
        metrics.gmCellIncX+r->character_space;
    width=(std::max)(width,max_x);height=(std::max)(height,max_y);
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
        const auto lead=static_cast<unsigned char>(character[0]);
        const bool double_byte=(lead>=0x81 && lead<0xa0)||(lead>=0xe0 && lead<0xff);
        // The '<' tag prefix consumes the rest when no '>' exists (40FDC4).
        // CStringLayout supplies exactly one character, so it never has a tag.
        if(lead && lead!='<' && (!double_byte || character[1])) {
            const UINT code=double_byte?(UINT(lead)<<8)|static_cast<unsigned char>(character[1])
                :static_cast<UINT>(static_cast<signed char>(lead));
            glyph(r,code,w,h);
        }
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
        if(FAILED(kinoko::graphics::create_texture(kinoko_graphics.device,512,512,1,0,
            kinoko::graphics::format_a8r8g8b8,kinoko::graphics::pool_managed,&value))) return 0;
    }
    texture.reset(value);
    {
        GraphicsLock lock;kinoko::graphics::MappedPixels rect{};
        if(FAILED(value->LockRect(0,&rect,nullptr,0))) return 0;
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
    if(FAILED(texture->GetSurfaceLevel(0,&value))) return;
    surface.reset(value);kinoko::graphics::SurfaceDescription description{};value->GetDesc(&description);surface.reset();
    RECT region{x,y,static_cast<LONG>(description.Width),static_cast<LONG>(description.Height)};
    kinoko::graphics::MappedPixels rect{};
    if(FAILED(texture->LockRect(0,&rect,&region,0))) return;
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
            memcpy_s(static_cast<unsigned char*>(rect.pBits)+row*rect.Pitch,
                (description.Width-x)*bytes_per_pixel,pixels.data()+row*rect.Pitch,
                *width*bytes_per_pixel);
    } catch(...) { texture->UnlockRect(0);throw; }
    texture->UnlockRect(0);
}

