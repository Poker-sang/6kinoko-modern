#pragma once
#include <cstring>
#include <cstdlib>
#include <memory>
#include <type_traits>
namespace kinoko::memory {
template<class T> T load(const void* source) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    T value;std::memcpy(&value,source,sizeof(value));return value;
}
template<class T> void store(void* destination,const T& value) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    std::memcpy(destination,&value,sizeof(value));
}
struct Free { void operator()(void* value) const noexcept { std::free(value); } };
template<class T> using Allocation=std::unique_ptr<T,Free>;
}
