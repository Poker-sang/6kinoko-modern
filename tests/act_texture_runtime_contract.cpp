#include "kinoko/act_texture_bridge.hpp"
#include <cstring>
#include <memory>
#include <type_traits>

using namespace kinoko::act;
static_assert(std::is_standard_layout_v<TextureResource>);
static_assert(!std::is_copy_constructible_v<TextureResource>);
static_assert(!std::is_copy_assignable_v<TextureResource>);
static_assert(std::is_same_v<decltype(TextureResource::methods), const void*>);
static_assert(alignof(TextureResource) >= alignof(void*));

int main() {
    const auto cleanup = [](TextureResource* value) {
        destroy_resources(value, 1, [](auto& resource) { resource.clear_names(); });
    };
    int methods = 0;
    std::unique_ptr<TextureResource, decltype(cleanup)> source(create_texture_resource(&methods), cleanup);
    std::unique_ptr<TextureResource, decltype(cleanup)> clone(create_texture_resource(&methods), cleanup);
    if (!source || !clone) return 1;
    if (source->id != -1 || source->width != 256 || source->height != 256 ||
        !source->auto_size || source->texture || source->retained_texture) return 2;
    const char* name = "resource name longer than the inline string storage";
    kinoko::legacy::StringView(&source->name).assign(name, static_cast<std::uint32_t>(std::strlen(name)));
    kinoko::legacy::StringView(&source->texture_name).assign("image", 5);
    if (!clone->copy_names(*source)) return 3;
    source->clear_names();
    if (std::strcmp(kinoko::legacy::StringView(&clone->name).data(), name) ||
        std::strcmp(kinoko::legacy::StringView(&clone->texture_name).data(), "image")) return 4;
    source->width = 320; source->height = 240; source->source_width = 128;
    source->source_x = 7; source->auto_size = 0;
    clone->copy_image_properties(*source);
    clone->texture = 19; clone->retained_texture = 19; clone->borrows_texture = 1;
    const auto snapshot = texture_runtime_state(clone.get());
    if (snapshot.methods != &methods || snapshot.width != 320 || snapshot.height != 240 ||
        snapshot.source_x != 7 || snapshot.source_width != 128 || snapshot.auto_size || snapshot.handle != 19) return 5;
    clone->texture = 20; // Retained reference is independent of the mutable visible handle.
    if (clone->take_texture_reference() != 19 || clone->texture || clone->take_texture_reference()) return 6;
    clone->texture = 21; // Borrowed without an extra reference: never release somebody else's handle.
    if (clone->take_texture_reference() || clone->texture) return 7;
    clone->borrows_texture = 0; clone->texture = 22;
    if (clone->take_texture_reference() != 22 || clone->take_texture_reference()) return 8;
    clone->clear_names(); clone->clear_names();
    return 0;
}
