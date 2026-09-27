#pragma once
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>
#include <type_traits>

namespace kinoko::act {
// Native allocation metadata replaces the VC8 four-byte array cookie. All
// scalar and array texture/chip factories use this same aligned allocation.
template<class T> struct alignas(T) ResourceAllocation {
    std::size_t count;
};
template<class T> T* allocate_resources(const void* methods, std::size_t count = 1) noexcept {
    using Header = ResourceAllocation<T>;
    static_assert(std::is_nothrow_default_constructible_v<T>);
    static_assert(alignof(T) <= alignof(std::max_align_t));
    if (!count || count > (std::numeric_limits<std::size_t>::max() - sizeof(Header)) / sizeof(T)) return nullptr;
    auto* header = static_cast<Header*>(std::malloc(sizeof(Header) + sizeof(T) * count));
    if (!header) return nullptr;
    new (header) Header{count};
    auto* values = reinterpret_cast<T*>(header + 1);
    for (std::size_t i = 0; i < count; ++i) { new (values + i) T; values[i].methods = methods; }
    return values;
}
template<class T, class Clear> void* destroy_resources(T* values, unsigned char flags, Clear clear) {
    if (!values) return nullptr;
    auto* header = reinterpret_cast<ResourceAllocation<T>*>(values) - 1;
    // Native metadata is authoritative. Array teardown remains reverse order.
    for (auto i = header->count; i > 0; --i) { clear(values[i - 1]); values[i - 1].~T(); }
    header->count = 0;
    void* result = (flags & 2) ? static_cast<void*>(header) : static_cast<void*>(values);
    if (flags & 1) std::free(header);
    return result;
}
} // namespace kinoko::act
