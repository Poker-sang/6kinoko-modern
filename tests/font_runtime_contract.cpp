#include "kinoko/font_runtime.hpp"
#include <type_traits>
#include <vector>
using kinoko::text::FontRenderer;
using kinoko::text::FontAtlas;
static_assert(std::is_same_v<decltype(FontRenderer::output), std::uint32_t*>);
static_assert(std::is_same_v<decltype(FontRenderer::device_context), void*>);
static_assert(std::is_same_v<decltype(FontAtlas::texture), std::int32_t>);
static_assert(!std::is_trivially_copyable_v<FontRenderer>);
static_assert(alignof(FontAtlas) >= alignof(void*));
int main() {
    std::vector<FontAtlas> pages(2);
    pages[1].texture = 17;
    pages[1].renderer.label = "independent label";
    std::uint32_t buffer = 0;
    pages[1].renderer.output = &buffer;
    auto copy = pages;
    copy.erase(copy.begin());
    copy[0].renderer.label[0] = 'I';
    if (copy[0].texture != 17 || copy[0].renderer.output != &buffer ||
        pages[1].renderer.label[0] != 'i') return 1;
    // Original list assignment duplicates nodes but retains pointer values.
    // Detach the borrowed sentinel before destructors exercise owned cleanup.
    pages[0].renderer.pixels.push_back(&buffer);
    FontRenderer renderer = pages[0].renderer;
    const bool shallow = renderer.pixels.front() == &buffer;
    renderer.pixels.clear();
    pages[0].renderer.pixels.clear();
    return shallow && renderer.font_weight == 400 && renderer.text_limit == 100000 ? 0 : 2;
}
