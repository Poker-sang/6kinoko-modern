#include "kinoko/input_actions.hpp"
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"action contract line %d\n",__LINE__);return 1;}}while(0)
int main() {
    using namespace kinoko::input;
    auto bindings=classic_bindings();Sample sample;Frame frame;
    sample.buttons[3]=37;sample.buttons[0]=2;sample.y=-14;
    advance(bindings,sample,frame);
    CHECK(frame.held[Attack]==37 && frame.held[Carry]==37 && frame.held[Run]==37);
    CHECK(frame.held[Jump]==2 && frame.held[Confirm]==2);
    CHECK(frame.held[Door]==14 && frame.held[Up]==14 && frame.held[PipeDown]==0);
    // Device handoff follows legacy counter, never synthesizes a fresh press.
    sample.buttons[3]=4;advance(bindings,sample,frame);CHECK(frame.held[Attack]==4);
    std::string error;
    CHECK(parse_bindings("version=1\nattack=scan:46,pad:7\nconfirm=none\n",bindings,error,nullptr));
    sample.keys[46]=true;frame={};advance(bindings,sample,frame);
    CHECK(frame.held[Attack]==1 && frame.held[Run]==4 && frame.held[Confirm]==0 && frame.held[Jump]==2);
    sample.keys[46]=false;sample.pad[7]=true;advance(bindings,sample,frame);
    CHECK(frame.held[Attack]==2 && !frame.released[Attack]);
    sample={};advance(bindings,sample,frame);
    CHECK(frame.held[Attack]==0 && frame.released[Attack]);
    advance(bindings,sample,frame);CHECK(!frame.released[Attack]);
    for(const char* invalid:{"attack=none", "version=2", "version=1\nattack=pad:32", "version=1\nattack=scan:999",
        "version=1\nattack=legacy,", "version=1\nwrong=none", "version=1\nattack=none\nattack=legacy"}) {
        CHECK(!parse_bindings(invalid,bindings,error,nullptr));
        CHECK(bindings[Attack].size()==2 && bindings[Confirm].empty());
    }
    // Existing keyconfig changes take effect live for inherited actions only.
    sample.buttons[3]=1;advance(bindings,sample,frame);
    CHECK(frame.held[Run]==1 && frame.held[Attack]==0);
    CHECK(parse_bindings("version=1\nmenuAcceptAlt=scan:1\npause=scan:30",bindings,error,nullptr));
    sample={};frame={};sample.keys[1]=true;advance(bindings,sample,frame);
    CHECK(frame.held[MenuAcceptAlt]==1 && frame.held[Pause]==0);
    sample.keys[1]=false;sample.keys[30]=true;advance(bindings,sample,frame);
    CHECK(frame.held[MenuAcceptAlt]==0 && frame.held[Pause]==1);
    CHECK(parse_bindings("version=1\nmenuBack=scan:1",bindings,error,nullptr));
    CHECK(!parse_bindings("version=1\nmenuBack=scan:1\nmenuAcceptAlt=scan:30",bindings,error,nullptr));
    std::puts("input actions passed");return 0;
}
