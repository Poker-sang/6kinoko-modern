#pragma once
#include "kinoko/act_resource_records.hpp"
#include "kinoko/act_document_association.hpp"
#include "kinoko/memory_access.hpp"

struct KinokoActLayerHolder { KinokoActLayer *layer; };
struct KinokoActKeyHolder { KinokoActKey *key; };
static_assert(sizeof(KinokoActLayerHolder) == sizeof(void *));
static_assert(sizeof(KinokoActKeyHolder) == sizeof(void *));

namespace kinoko::act {
// Verified prefixes, not allocation sizes. Unknown bytes stay opaque.
// One-word wrappers own only their allocations, never the pointees.
struct LayerKeys {
    alignas(void*) std::array<uint8_t, sizeof(LayerAssociationRecord)> unknown0;
    legacy::StringRecord name;
    std::array<uint8_t, 8> unknown136;
    std::array<uint32_t, 3> position;
    std::array<uint8_t, 12> origin_bits;
    std::array<uint32_t, 3> previous_position;
    struct KeyNode *key_head;
    int32_t key_count;
    std::array<uint8_t, 4> unknown188;
    struct KeyNode *timeline_head;
    int32_t extra_count;
    uint32_t unknown200;
    const void* script_methods;
    ActCallbackRecord initialize;
    ActCallbackRecord update_callback;
};
struct KeyNode { KeyNode *next, *previous; KinokoActKey *key; };
struct LayoutKey { uint32_t unknown0; KinokoActLayout *layout; };
// Prefix within the document's embedded script, not a duplicate document schema.
struct ScriptUpdatePrefix {
    const void* methods;
    ActCallbackRecord initialize;
    ActCallbackRecord update_callback;
};
static_assert(offsetof(ScriptUpdatePrefix,update_callback)==offsetof(ScriptStorageRecord,update));
static_assert(sizeof(KeyNode)==3*sizeof(void*));
#if INTPTR_MAX == INT32_MAX
static_assert(offsetof(ScriptUpdatePrefix, update_callback) == 24);
static_assert(offsetof(LayerKeys, name) == 112);
static_assert(offsetof(LayerKeys, position) == 144 && offsetof(LayerKeys, previous_position) == 168);
static_assert(offsetof(LayerKeys, update_callback) == 228);
static_assert(offsetof(LayerKeys, key_head) == 180);
static_assert(offsetof(LayerKeys, key_count) == 184);
static_assert(offsetof(LayerKeys, timeline_head) == 192);
static_assert(offsetof(LayerKeys, extra_count) == 196);
static_assert(sizeof(KeyNode) == 12 && offsetof(KeyNode, key) == 8);
static_assert(offsetof(LayoutKey, layout) == 4);
#endif
// Keep the original x86 DWORD-distance interpretation, including malformed
// raw fixture spans, without C++ subtraction of unrelated/null pointers.
inline int32_t layer_distance(const DocumentPointerSpan<KinokoActLayer>& layers) noexcept {
    const uintptr_t bytes = static_cast<uintptr_t>(
        reinterpret_cast<uintptr_t>(layers.end) - reinterpret_cast<uintptr_t>(layers.begin));
    return static_cast<int32_t>(static_cast<intptr_t>(bytes) / static_cast<intptr_t>(sizeof(KinokoActLayer *)));
}
// The shared container stores void pointers. Read a borrowed typed pointer
// representation without aliasing that allocation as an array of another type.
inline KinokoActLayer *layer_at(const DocumentPointerSpan<KinokoActLayer>& layers, int32_t index) noexcept {
    const auto *slots = reinterpret_cast<const unsigned char *>(layers.begin);
    return kinoko::memory::load<KinokoActLayer *>(slots + static_cast<size_t>(index) * sizeof(KinokoActLayer *));
}
inline bool ordered_layers(const DocumentPointerSpan<KinokoActLayer>& layers) noexcept {
    return layers.begin && reinterpret_cast<intptr_t>(layers.end) >= reinterpret_cast<intptr_t>(layers.begin);
}
}
