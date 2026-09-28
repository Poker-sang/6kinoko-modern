#include "kinoko/string_runtime.hpp"
#include "kinoko/resource_allocation.hpp"
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <type_traits>
using kinoko::text::StringLayout;
using kinoko::legacy::StringView;
namespace {
void require(bool condition) { if(!condition) std::abort(); }
int released=0;
int32_t release_texture(int32_t handle) { require(handle==17);++released;return 0; }
struct Throwing {
    const void* methods=nullptr;
    static int attempts,live;
    Throwing() { if(++attempts==3) throw std::bad_alloc();++live; }
    ~Throwing() { --live; }
};
int Throwing::attempts=0;
int Throwing::live=0;
}
int main() {
    static_assert(!std::is_copy_constructible_v<StringLayout>);
    static_assert(sizeof(decltype(StringLayout::layer))==sizeof(void*));
    require(!kinoko::act::allocate_resources<Throwing>(nullptr,4));
    require(Throwing::attempts==3 && Throwing::live==0);
    StringLayout destination;
    {
        StringLayout source;
        require(source.font_height==16 && source.line_space==2 && source.wrap_width==-1);
        auto* first=source.append_atlas(release_texture);
        first->texture=17;
        source.glyphs.emplace_back();
        source.glyphs.back().id=42;
        source.glyphs.back().quad.texture=17;
        source.glyphs.back().set_atlas(source.atlases.front());
        for(int i=0;i<100;++i) source.append_atlas(release_texture);
        require(source.glyphs.front().atlas.get()==first && first->references==1);
        source.atlases.erase(source.atlases.begin()+1);
        require(source.glyphs.front().atlas.get()==first);
        destination.replicate(source);
        require(first->references==2);
        destination.replicate(destination);
        require(first->references==2);
        StringView(&source.text).assign("abc",3);
        StringView(&source.pending).assign("d\0e",3);
        source.scale_x=2;source.alignment=2;source.cursor_x=41;source.next_glyph_id=43;
        StringLayout clone;
        clone.copy_state(source);
        require(clone.glyphs.empty() && clone.atlases.empty());
        require(clone.scale_x==2 && clone.alignment==2 && clone.cursor_x==41 && clone.next_glyph_id==43);
        require(StringView(&clone.pending).length()==3 && !std::memcmp(StringView(&clone.pending).data(),"d\0e",3));
        StringView(&source.text).assign("changed",7);
        require(!std::strcmp(StringView(&clone.text).data(),"abc"));
        source.glyphs.clear();source.atlases.clear();
        require(first->references==1 && released==0);
    }
    require(destination.glyphs.front().quad.texture==17 && destination.glyphs.front().id==42);
    destination.glyphs.clear();require(released==1);
    auto* array=kinoko::act::allocate_resources<StringLayout>(nullptr,3);
    require(array);
    array[0].append_atlas(release_texture)->texture=17;
    kinoko::act::destroy_resources(array,3,[](auto&){});
    require(released==2);
    return 0;
}
