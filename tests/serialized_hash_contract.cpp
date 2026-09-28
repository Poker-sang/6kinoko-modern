#include "kinoko/serialized_hash.hpp"
#include "kinoko/boost_hash.h"
using kinoko::format::serialized_hash;
constexpr char layout[]=".?AVC2DLayout@@";
constexpr char timeline[]=".?AVCActTimeLine@@";
constexpr char text[]=".?AVCStringLayout@@";
constexpr char binary[]={static_cast<char>(0x80),0,static_cast<char>(0xff)};
// Existing original-format fixtures, not a host-width Boost recomputation.
static_assert(serialized_hash(layout,layout+sizeof(layout)-1)==0x655cd5b0u);
static_assert(serialized_hash(timeline,timeline+sizeof(timeline)-1)==0x9902f2c0u);
static_assert(serialized_hash(text,text+sizeof(text)-1)==0x9e695d47u);
static_assert(serialized_hash(binary,binary+sizeof(binary))==0xfb404e69u);
static_assert(serialized_hash(binary,binary)==0);
int main() {
    return static_cast<uint32_t>(kinoko_boost_hash_range(binary,binary+sizeof(binary)))==0xfb404e69u ? 0 : 1;
}
