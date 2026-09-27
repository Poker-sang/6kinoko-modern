#include "kinoko/act_chip_resource.hpp"
#include "kinoko/act_texture_resource.hpp"
#include <cstring>
#include <limits>
#include <memory>
#include <vector>
#include <type_traits>
using namespace kinoko::act;
static_assert(!std::is_copy_constructible_v<ChipResource>);
static_assert(!std::is_copy_assignable_v<ChipResource>);
static_assert(sizeof(decltype(kinoko_mcd_data::chips)) == sizeof(void*));
int main() {
    int frees = 0, identity = 1;
    const auto clear = [](ChipResource& value) { value.clear(); };
    const auto destroy = [clear](ChipResource* value) { destroy_resources(value,1,clear); };
    std::unique_ptr<ChipResource,decltype(destroy)> source(create_chip_resource(&identity),destroy);
    std::unique_ptr<ChipResource,decltype(destroy)> clone(create_chip_resource(&identity),destroy);
    if (!source || !clone || source->id != -1 || source->data || source->methods != &identity) return 1;
    source->data = std::shared_ptr<kinoko_mcd_data>(new kinoko_mcd_data{},[&](auto* value) { ++frees; delete value; });
    const char* name = "MCD resource with an independently owned long filename";
    kinoko::legacy::StringView(&source->source_name).assign(name,static_cast<uint32_t>(std::strlen(name)));
    if (!clone->copy_names(*source)) return 2;
    clone->data = source->data;
    auto* original = source->data.get();
    source->data = std::make_shared<kinoko_mcd_data>(); // Reload must not invalidate the clone.
    if (frees || clone->data.get() != original || clone->data.use_count() != 1) return 3;
    source.reset();
    if (frees || std::strcmp(kinoko::legacy::StringView(&clone->source_name).data(),name)) return 4;
    clone.reset();
    if (frees != 1) return 5;
    auto* array = create_chip_resource(&identity,3);
    if (!array || reinterpret_cast<std::uintptr_t>(array) % alignof(ChipResource)) return 6;
    for (int i=0; i<3; ++i) array[i].id=i;
    std::vector<int> order;
    void* allocation = destroy_resources(array,2,[&](auto& value) { order.push_back(value.id); value.clear(); });
    if (order != std::vector<int>({2,1,0})) return 7;
    std::free(allocation); // flags 2 destroys but does not free.
    auto* targets = create_texture_resource(&identity,2);
    if (!targets) return 8;
    targets[0].texture=11; targets[1].texture=12;
    order.clear();
    destroy_resources(targets,3,[&](auto& value) { order.push_back(value.take_texture_reference()); value.clear_names(); });
    if (order != std::vector<int>({12,11})) return 9;
    if (create_chip_resource(&identity,0) || create_chip_resource(&identity,std::numeric_limits<std::size_t>::max())) return 10;
    return 0;
}
