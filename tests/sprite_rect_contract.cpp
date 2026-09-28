#include "kinoko/sprite.h"
#include "kinoko/texture_store.h"
#include <cstdio>
extern "C" { KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY]{}; }
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"sprite rect line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main() {
    void* methods[5]{};
    methods[4] = reinterpret_cast<void*>(&kinoko_sprite_set_rect_pivot);
    KinokoSprite sprite{};
    sprite.vtable = methods;
    kinoko_texture_slots[7].width = 256;
    kinoko_texture_slots[7].height = 128;
    kinoko_sprite_set_rect(&sprite, nullptr, 7, 24, 32, 12, 16);
    CHECK(sprite.texture == 7 && sprite.width == 12 && sprite.height == 16);
    CHECK(sprite.pivot_x == 0 && sprite.pivot_y == 0);
    CHECK(sprite.scale_x == 1 && sprite.scale_y == 1 && sprite.angle == 0);
    CHECK(sprite.vertices[0].u == 24.0f/256 && sprite.vertices[0].v == 32.0f/128);
    CHECK(sprite.vertices[3].u == 36.0f/256 && sprite.vertices[3].v == 48.0f/128);
    for (const auto& vertex : sprite.vertices) CHECK(vertex.rhw == 1 && vertex.color == 0xffffffffu);
    std::puts("PASS: virtual sprite rectangle dispatch and digit geometry");
}
