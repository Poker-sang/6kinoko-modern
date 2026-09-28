#pragma once
#include <cstdint>
namespace kinoko::format {
// Boost 1.44 hash_range<char> as used by the original Win32 serializer.
// Its seed arithmetic is ALWAYS uint32 and char ALWAYS sign-extends. Host
// size_t width and default char signedness must never change a DAT type ID.
constexpr std::uint32_t serialized_hash(const char* begin,const char* end) noexcept {
    std::uint32_t seed=0;
    for(auto* current=begin;current!=end;++current) {
        const auto byte=static_cast<unsigned char>(*current);
        const auto value=static_cast<std::uint32_t>(byte<128 ? int(byte) : int(byte)-256);
        seed ^= value+UINT32_C(0x9e3779b9)+(seed<<6)+(seed>>2);
    }
    return seed;
}
}
