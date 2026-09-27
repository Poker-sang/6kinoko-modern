#include "kinoko/render_resources.hpp"
#include "kinoko/texture_pixels.hpp"
#include "kinoko/texture_bindings.hpp"
#include <array>
#include <vector>
using namespace kinoko::render;
struct Quad final : QuadSink {
    int step=0; const KinokoSpriteVertex* seen=nullptr;
    int32_t set_layout(SpriteLayout v) override { step=v==SpriteLayout::screen_rhw?1:10; return -1; }
    int32_t draw_quad(const KinokoSpriteVertex* v) override { ++step;seen=v;return 37; }
};
struct Bind final : TextureBindingSink {
    std::vector<int32_t> values;
    int32_t bind(uint32_t stage,int32_t handle) override { values.push_back(int32_t(stage));values.push_back(handle);return -7; }
};
int main() {
    static_assert(sizeof(KinokoSpriteVertex)==28);
    std::array<KinokoSpriteVertex,4> vertices{};
    Quad quad;
    if(submit_quad(quad,SpriteLayout::screen_rhw,vertices.data())!=37 || quad.step!=2 || quad.seen!=vertices.data()) return 1;
    Bind sink;TextureBindings cache;
    if(cache.bind(sink,2,9)!=-7 || cache.bind(sink,2,9)!=9 || sink.values!=std::vector<int32_t>{2,9}) return 2;
    if(cache.bind(sink,2,0)!=0 || cache.bind(sink,2,0)!=0 || sink.values.size()!=6) return 3;
    cache.bind(sink,2,9);cache.forget(9);
    if(cache.bind(sink,2,9)!=-7) return 4;
    alignas(uint32_t) std::array<uint8_t,24> output;output.fill(0xcd);
    const uint16_t source[]={0x8001,0x8002,0x8003,0x8004,0x8005,0x8006,0x8007,0x8008};
    BitmapPixels bitmap{16,3,2,0,nullptr,reinterpret_cast<const uint8_t*>(source)};
    if(!copy_surface(bitmap,8,{output.data(),12})) return 5;
    // Raw 16-bit odd-width trailing pixel is deliberately not copied.
    if(std::memcmp(output.data(),source,4) || output[4]!=0xcd || std::memcmp(output.data()+12,source+4,4) || output[16]!=0xcd) return 6;
    if(copy_surface(bitmap,8,{output.data(),5}) || copy_surface(bitmap,8,{output.data(),-1})) return 7;
    const uint16_t encoded[]={6,0x8123};
    bitmap.encoded_size=sizeof(encoded);bitmap.pixels=reinterpret_cast<const uint8_t*>(encoded);
    if(!copy_surface(bitmap,0,{output.data(),12})) return 8;
    uint16_t value=0;std::memcpy(&value,output.data()+16,2);if(value!=0x8123 || output[18]!=0xcd) return 9;
    bitmap.encoded_size=2;if(copy_surface(bitmap,0,{output.data(),12})) return 10;
    const uint8_t indexes[]={1,0};const uint16_t palette[]={0x8000,0xffff};
    bitmap={8,2,1,0,palette,indexes};
    if(!copy_surface(bitmap,2,{output.data(),12})) return 11;
    std::memcpy(&value,output.data(),2);if(value!=0xffff) return 12;
    return 0;
}
