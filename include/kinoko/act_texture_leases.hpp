#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace kinoko::act {

// Native ownership of the extra texture-store reference held by each clone.
// The ACT record's borrowed bit and handle are original observable state.
class TextureCloneLeases {
public:
    bool insert(const void* resource, std::int32_t handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        return leases_.emplace(resource, handle).second;
    }
    std::optional<std::int32_t> take(const void* resource) {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found=leases_.find(resource);
        if (found==leases_.end()) return std::nullopt;
        const auto handle=found->second;
        leases_.erase(found);
        return handle;
    }
private:
    std::mutex mutex_;
    std::unordered_map<const void*, std::int32_t> leases_;
};

}
