#pragma once
#include "kinoko/render_resources.hpp"
#include <algorithm>
#include <cstring>
#include <type_traits>
namespace kinoko::render {
// Borrowed decoded CV2 view. No legacy object layout or pointer-size assumption.
struct BitmapPixels {
    uint8_t bit_depth;
    uint32_t width,height,encoded_size;
    const uint16_t* palette;
    const uint8_t* pixels;
};
inline bool copy_indexed_raw(const BitmapPixels& bitmap, uint32_t source_pitch,
    const PixelMapping& locked) {
    if (!bitmap.palette || locked.pitch < 0 || size_t(locked.pitch) < size_t(bitmap.width) * 2u)
        return false;
    for (uint32_t row = 0; row < bitmap.height; ++row) {
        auto* destination = static_cast<uint16_t*>(static_cast<void*>(
            static_cast<uint8_t*>(locked.pixels) + size_t(row) * locked.pitch));
        const auto* source = bitmap.pixels + size_t(row) * source_pitch;
        for (uint32_t column = 0; column < bitmap.width; ++column)
            destination[column] = bitmap.palette[source[column]];
    }
    return true;
}

inline bool copy_raw_16(const BitmapPixels& bitmap, uint32_t source_pitch,
    const PixelMapping& locked) {
    if (locked.pitch < 0 || size_t(locked.pitch) < size_t(bitmap.width) * 2u)
        return false;
    for (uint32_t row = 0; row < bitmap.height; ++row) {
        auto* destination = static_cast<uint8_t*>(locked.pixels) + size_t(row) * locked.pitch;
        const auto* source = bitmap.pixels + size_t(row) * source_pitch;
        // The original raw branch copies complete 32-bit pairs only.
        std::memcpy(destination, source, size_t(bitmap.width / 2u) * 4u);
    }
    return true;
}

inline bool copy_raw_32(const BitmapPixels& bitmap, uint32_t source_pitch,
    const PixelMapping& locked) {
    if (locked.pitch < 0 || size_t(locked.pitch) < size_t(bitmap.width) * 4u)
        return false;
    for (uint32_t row = 0; row < bitmap.height; ++row) {
        auto* destination = static_cast<uint8_t*>(locked.pixels) + size_t(row) * locked.pitch;
        const auto* source = bitmap.pixels + size_t(row) * source_pitch;
        std::memcpy(destination, source, size_t(bitmap.width) * 4u);
    }
    return true;
}

template <typename T, typename Convert>
inline bool decode_rle(const BitmapPixels& bitmap, const PixelMapping& locked, Convert convert) {
    if (locked.pitch < 0 || size_t(locked.pitch) < size_t(bitmap.width) * sizeof(T))
        return false;
    const auto* cursor = bitmap.pixels;
    const auto* end = bitmap.pixels + bitmap.encoded_size;
    uint32_t run_left = 0;
    T run_value{};
    using Count = std::conditional_t<sizeof(T) == 4, uint32_t, uint16_t>;
    for (uint32_t row = 0; row < bitmap.height; ++row) {
        auto* destination = reinterpret_cast<T*>(static_cast<uint8_t*>(locked.pixels) +
            size_t(row) * locked.pitch);
        uint32_t column = 0;
        while (column < bitmap.width) {
            if (!run_left) {
                if (cursor + sizeof(Count) + sizeof(T) > end) return false;
                Count count;
                std::memcpy(&count, cursor, sizeof(count)); cursor += sizeof(count);
                std::memcpy(&run_value, cursor, sizeof(run_value)); cursor += sizeof(run_value);
                if (!count) return false;
                run_left = count;
            }
            const auto amount = (std::min)(run_left, bitmap.width - column);
            for (uint32_t i = 0; i < amount; ++i)
                destination[column + i] = convert(run_value);
            column += amount;
            run_left -= amount;
        }
    }
    return true;
}

inline bool copy_surface(const BitmapPixels& bitmap, uint32_t source_pitch, const PixelMapping& locked) {
    if (bitmap.encoded_size) {
        if (bitmap.bit_depth == 8) {
            if (!bitmap.palette) return false;
            return decode_rle<uint16_t>(bitmap, locked,
                [&bitmap](uint16_t value) { return bitmap.palette[value]; });
        }
        if (bitmap.bit_depth == 16)
            return decode_rle<uint16_t>(bitmap, locked, [](uint16_t value) { return value; });
        if (bitmap.bit_depth == 24 || bitmap.bit_depth == 32)
            return decode_rle<uint32_t>(bitmap, locked, [](uint32_t value) { return value; });
        return false;
    }
    if (bitmap.bit_depth == 8)
        return copy_indexed_raw(bitmap, source_pitch, locked);
    if (bitmap.bit_depth == 16)
        return copy_raw_16(bitmap, source_pitch, locked);
    if (bitmap.bit_depth == 24 || bitmap.bit_depth == 32)
        return copy_raw_32(bitmap, source_pitch, locked);
    return false;
}
}
