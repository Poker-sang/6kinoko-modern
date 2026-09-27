#pragma once
#include "kinoko/legacy_string.hpp"
#include "kinoko/mcd_data.h"
#include "kinoko/resource_allocation.hpp"
#include <memory>
#include <cstdint>
#include <cstddef>

namespace kinoko::act {
struct ChipResource final {
    const void* methods = nullptr;
    std::int32_t id = -1;
    legacy::StringRecord name{{}, 0, 15};
    legacy::StringRecord source_name{{}, 0, 15};
    legacy::StringRecord loaded_path{{}, 0, 15};
    std::shared_ptr<kinoko_mcd_data> data;

    ChipResource() noexcept = default;
    ChipResource(const ChipResource&) = delete;
    ChipResource& operator=(const ChipResource&) = delete;
    bool copy_names(const ChipResource& source) {
        for (auto member : {&ChipResource::name, &ChipResource::source_name, &ChipResource::loaded_path}) {
            const legacy::StringView input(const_cast<legacy::StringRecord*>(&(source.*member)));
            legacy::StringView(&(this->*member)).assign(input.data(), input.length());
            if ((this->*member).length != input.length()) return false;
        }
        return true;
    }
    void clear() noexcept {
        legacy::StringView(&loaded_path).destroy();
        data.reset();
        legacy::StringView(&source_name).destroy();
        legacy::StringView(&name).destroy();
    }
};
#if INTPTR_MAX == INT32_MAX
static_assert(offsetof(ChipResource, id) == 4);
static_assert(offsetof(ChipResource, name) == 8);
#endif
inline ChipResource* create_chip_resource(const void* methods, std::size_t count = 1) noexcept {
    return allocate_resources<ChipResource>(methods, count);
}
inline ChipResource& chip_resource(void* value) noexcept { return *static_cast<ChipResource*>(value); }
inline const ChipResource& chip_resource(const void* value) noexcept { return *static_cast<const ChipResource*>(value); }
} // namespace kinoko::act
