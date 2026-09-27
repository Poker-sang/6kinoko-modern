#include "kinoko/act_method_dispatch.hpp"
#include <array>
#include <cstring>
using namespace kinoko::act;
#if defined(_MSC_VER) && defined(_M_IX86)
#define CALLBACK __fastcall
#define RECEIVER void* self, void*
#else
#define CALLBACK
#define RECEIVER void* self
#endif
struct Fixture { void** methods; int events=0; int width=0,height=0; };
static std::array<void*,13> first{}, second{};
static std::uint8_t CALLBACK query(RECEIVER, const void* descriptor, void* output) {
    auto& value = *static_cast<Fixture*>(self); ++value.events;
    const auto* type = static_cast<const ResourceTypeDescriptor*>(descriptor);
    *static_cast<void**>(output) = std::strcmp(type->name,".?AVCActResourceChip@@") == 0 ? self : nullptr;
    return *static_cast<void**>(output) != nullptr;
}
static int CALLBACK bind(RECEIVER, void*, const char*) {
    auto& value = *static_cast<Fixture*>(self); value.methods=second.data(); value.events+=10; return 0;
}
static int CALLBACK bound(RECEIVER, void*, const char*) { static_cast<Fixture*>(self)->events+=100; return 0; }
static std::uint8_t CALLBACK create(RECEIVER, int width, int height) {
    auto& value = *static_cast<Fixture*>(self);value.width=width;value.height=height;return 1;
}
static int CALLBACK draw(RECEIVER, float x, float y) {
    return x==1.25f && y==-2.5f ? 17 : -1;
}
int main() {
    first[2]=reinterpret_cast<void*>(query);first[7]=reinterpret_cast<void*>(bind);
    second[8]=reinterpret_cast<void*>(bound);second[12]=reinterpret_cast<void*>(create);
    Fixture value{first.data()};auto* resource=reinterpret_cast<KinokoActResource*>(&value);
    ResourceMethods methods(resource);
    if (methods.query(ResourceKind::texture) || methods.query(ResourceKind::chip)!=resource) return 1;
    methods.bind_table(nullptr,"resource");methods.bind_object(nullptr,"resource");
    if (value.events!=112) return 2; // Second call must observe the replaced method table.
    if (!methods.create_target(320,200) || value.width!=320 || value.height!=200) return 3;
    second[8]=reinterpret_cast<void*>(draw);
    if (LayoutMethods(reinterpret_cast<KinokoActLayout*>(&value)).draw(1.25f,-2.5f)!=17) return 4;
    return 0;
}
