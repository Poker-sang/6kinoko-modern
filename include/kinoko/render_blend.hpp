#pragma once
#include <cstdint>
namespace kinoko::render {
// Values are semantic identifiers, not D3D or SDL enum values.
enum class BlendOperation { unchanged, add, reverse_subtract };
enum class BlendFactor { unchanged, zero, one, source_color, destination_color, source_alpha, inverse_source_alpha };
// Borrowed command sink. Results are passed through for the legacy caller;
// the transition algorithm deliberately does not stop on an earlier failure.
class BlendSink {
public:
    virtual ~BlendSink() = default;
    virtual int32_t set_operation(BlendOperation value) = 0;
    virtual int32_t set_source(BlendFactor value) = 0;
    virtual int32_t set_destination(BlendFactor value) = 0;
};
enum class BlendPath { central, act_layout };
struct BlendTransition { int32_t key; BlendOperation operation; BlendFactor source,destination; };
// 402770 updates only changed states. Unchanged is distinct from a zero factor.
inline constexpr BlendTransition transitions[]={
    {0,BlendOperation::add,BlendFactor::source_alpha,BlendFactor::inverse_source_alpha},
    {1,BlendOperation::add,BlendFactor::source_alpha,BlendFactor::one},
    {2,BlendOperation::reverse_subtract,BlendFactor::source_alpha,BlendFactor::one},
    {34,BlendOperation::reverse_subtract,BlendFactor::source_alpha,BlendFactor::one},
    {3,BlendOperation::add,BlendFactor::zero,BlendFactor::source_color},
    {27,BlendOperation::add,BlendFactor::zero,BlendFactor::source_color},
    {9,BlendOperation::unchanged,BlendFactor::unchanged,BlendFactor::one},{10,BlendOperation::reverse_subtract,BlendFactor::unchanged,BlendFactor::one},
    {11,BlendOperation::unchanged,BlendFactor::zero,BlendFactor::source_color},{19,BlendOperation::unchanged,BlendFactor::zero,BlendFactor::source_color},
    {16,BlendOperation::unchanged,BlendFactor::unchanged,BlendFactor::inverse_source_alpha},{18,BlendOperation::reverse_subtract,BlendFactor::unchanged,BlendFactor::unchanged},
    {24,BlendOperation::add,BlendFactor::unchanged,BlendFactor::inverse_source_alpha},{25,BlendOperation::add,BlendFactor::unchanged,BlendFactor::unchanged},
    {32,BlendOperation::add,BlendFactor::source_alpha,BlendFactor::inverse_source_alpha},
    {33,BlendOperation::unchanged,BlendFactor::source_alpha,BlendFactor::one}
};
inline int32_t set_blend(int32_t &cached_mode, int32_t mode, BlendSink *device,
                         BlendPath path=BlendPath::central) {
    if (cached_mode==mode) return mode;
    int32_t result=0;
    const auto key=int64_t{mode}-1+8*int64_t{cached_mode};
    if(device) for(const auto &entry:transitions) {
        if(entry.key!=key) continue;
        if(entry.operation!=BlendOperation::unchanged &&
           !(path==BlendPath::act_layout && key==32)) result=device->set_operation(entry.operation);
        if(entry.source!=BlendFactor::unchanged) result=device->set_source(entry.source);
        if(entry.destination!=BlendFactor::unchanged) result=device->set_destination(entry.destination);
        break;
    }
    cached_mode=mode;
    return result;
}
}
