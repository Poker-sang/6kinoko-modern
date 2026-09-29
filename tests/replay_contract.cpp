#include "kinoko/replay.hpp"
#include "kinoko/runtime_clock.h"
#include <sstream>
#include <cstdio>
#include <functional>
#include <limits>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"replay contract line %d\n",__LINE__);return 1;}}while(0)
int main() {
    using namespace kinoko::replay;
    const std::string identity(64,'a');
    std::stringstream stream(std::ios::in|std::ios::out|std::ios::binary);
    Writer writer(stream,identity);Frame a;
    a.actions.held[kinoko::input::Jump]=2;a.actions.held[kinoko::input::Door]=3;
    a.actions.released[kinoko::input::Attack]=1;
    a.legacy.x=-13;a.legacy.y=std::numeric_limits<int32_t>::min();a.legacy.buttons[0]=9;a.legacy.digits[8]=4;
    a.random_before=1;a.random_after=0xdeadbeef;a.checkpoint=0x1234567887654321ull;a.clock=1000;
    writer.append(a);Frame b=a;b.clock=1016;b.actions.held[kinoko::input::Jump]=0;b.checkpoint++;
    writer.append(b);writer.finish();
    const auto encoded=stream.str();CHECK(encoded.size()==80+2*208+17);
    CHECK(static_cast<unsigned char>(encoded[8])==1 && static_cast<unsigned char>(encoded[12])==19);
    Reader reader(stream,identity);CHECK(reader.count()==2);Frame read;
    CHECK(reader.next(read) && read.legacy.x==-13 && read.legacy.y==std::numeric_limits<int32_t>::min());
    CHECK(read.legacy.digits[8]==4 && read.actions.held[kinoko::input::Jump]==2 && same_checkpoint(read,a));
    KinokoActionState state{};kinoko::input::publish(read.actions,state);
    CHECK(state.published[kinoko::input::Door]==-3 && state.released[kinoko::input::Attack]==1);
    // Script suppression must not reset the persistent held counter.
    state.published[kinoko::input::Jump]=0;CHECK(state.held[kinoko::input::Jump]==2);
    CHECK(reader.next(read) && same_checkpoint(read,b));CHECK(!reader.next(read));
    read.random_after++;CHECK(!same_checkpoint(read,b));read=b;read.checkpoint++;CHECK(!same_checkpoint(read,b));
    auto rejects=[&](const std::string& data,const std::string& id) {
        try {std::istringstream input(data,std::ios::binary);Reader bad(input,id);return false;}catch(const std::exception&){return true;}
    };
    CHECK(rejects(encoded,std::string(64,'b')));
    for(size_t size:{size_t(0),size_t(7),size_t(79),size_t(81),encoded.size()-1})CHECK(rejects(encoded.substr(0,size),identity));
    for(size_t offset:{size_t(0),size_t(8),size_t(12),size_t(80),size_t(81),size_t(101),encoded.size()-2}) {
        auto damaged=encoded;damaged[offset]^=1;CHECK(rejects(damaged,identity));
    }
    CHECK(rejects(encoded+"trailing",identity));
    kinoko_simulation_enable(1);CHECK(kinoko_simulation_milliseconds()==1000);
    kinoko_simulation_frame(59);CHECK(kinoko_simulation_milliseconds()==1983);
    kinoko_simulation_frame(60);CHECK(kinoko_simulation_milliseconds()==2000);
    kinoko_simulation_frame(3600);CHECK(kinoko_simulation_milliseconds()==61000);
    kinoko_simulation_enable(0);
    std::puts("Replay wire, integrity, independent publication, desync and clock checks passed");return 0;
}
