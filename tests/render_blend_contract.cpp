#include "kinoko/render_blend.hpp"
#include <vector>
#include <cstdint>
#include <limits>
using namespace kinoko::render;
struct Sink final : BlendSink {
    std::vector<int> calls;
    int32_t set_operation(BlendOperation v) override {
        calls.push_back(v==BlendOperation::add?10:11); return -1;
    }
    int32_t set_source(BlendFactor v) override {
        calls.push_back(v==BlendFactor::source_alpha?20:21); return -2;
    }
    int32_t set_destination(BlendFactor v) override {
        calls.push_back(v==BlendFactor::inverse_source_alpha?30:31); return 37;
    }
};
// Explicit checks remain active in Release. Compiled here; execution is separate.
int main() {
    Sink s; int32_t mode=0;
    if(set_blend(mode,1,&s)!=37 || mode!=1 || s.calls!=std::vector<int>{10,20,30}) return 1;
    s.calls.clear();
    if(set_blend(mode,1,&s)!=1 || !s.calls.empty()) return 2;
    if(set_blend(mode,3,&s)!=37 || s.calls!=std::vector<int>{11,31}) return 3;
    s.calls.clear();
    if(set_blend(mode,1,&s)!=37 || s.calls!=std::vector<int>{10,30}) return 4;
    s.calls.clear(); mode=4;
    set_blend(mode,1,&s,BlendPath::act_layout);
    if(s.calls!=std::vector<int>{20,30}) return 5;
    s.calls.clear(); mode=4;
    set_blend(mode,1,&s);
    if(s.calls!=std::vector<int>{10,20,30}) return 6;
    s.calls.clear(); mode=0;
    if(set_blend(mode,1,nullptr)!=0 || mode!=1) return 7;
    mode=std::numeric_limits<int32_t>::max();
    if(set_blend(mode,4,&s)!=0 || mode!=4 || !s.calls.empty()) return 8;
    mode=2;
    if(set_blend(mode,3,&s)!=-1 || mode!=3 || s.calls!=std::vector<int>{11}) return 9;
    return 0;
}
