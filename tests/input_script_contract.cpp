#include "kinoko/input_script_adapter.hpp"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
extern "C" void kinoko_trace(const char*) {}
extern "C" void kinoko_trace_i32(const char*, int32_t) {}
#ifdef KINOKO_STANDALONE_VM
SQInteger kinoko_squirrel_invoke_native(HSQUIRRELVM v,SQFUNCTION f){return f(v);}
#endif
namespace {
bool run(HSQUIRRELVM vm,const char* source,const char* file,bool adapt=false) {
    if(SQ_FAILED(sq_compilebuffer(vm,source,static_cast<SQInteger>(strlen(source)),file,SQTrue)))return false;
    std::string error;
    if(adapt && !kinoko_adapt_input_script(vm,error)){std::fprintf(stderr,"%s\n",error.c_str());return false;}
    sq_pushroottable(vm);
    return SQ_SUCCEEDED(sq_call(vm,1,SQFalse,SQTrue));
}
struct Reader {std::vector<unsigned char> bytes;size_t offset=0;};
SQInteger read(SQUserPointer user,SQUserPointer out,SQInteger count) {
    auto& r=*static_cast<Reader*>(user);
    if(count<0 || static_cast<size_t>(count)>r.bytes.size()-r.offset)return 0;
    std::memcpy(out,r.bytes.data()+r.offset,count);r.offset+=count;return count;
}
}
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"input script contract line %d\n",__LINE__);return 1;}}while(0)
int main(int argc,char** argv) {
    auto vm=sq_open(128);CHECK(vm);
    // Independently authored fixture with the original five public assignments.
    CHECK(run(vm,"function DisableInput() { input.x=0; input.y=0; input.b0=0; input.b2=0; input.b3=0; }",
        "data/script/global.nut",true));
    CHECK(run(vm,R"(
input <- {x=1,y=2,b0=3,b2=4,b3=5,moveX=6,menuX=7,moveY=8,menuY=9,
door=-1,pipeUp=-2,pipeDown=2,jump=1,confirm=2,useItem=3,attack=4,run=5,carry=6}
DisableInput();
foreach(k,v in input) { if(v != 0) throw k; }
)","fixture"));
    // Unrelated mod functions are not rewritten based only on field names.
    CHECK(run(vm,"input.b0=7; function Read() { return input.b0; } if(Read()!=7) throw 1;","mod.nut",true));
    CHECK(SQ_SUCCEEDED(sq_compilebuffer(vm,"function DisableInput() { input.b0=1; }",37,"data/script/global.nut",SQTrue)));
    std::string error;CHECK(!kinoko_adapt_input_script(vm,error) && !error.empty());
    sq_settop(vm,0);
    // Optional local original fixtures: validate every guarded variant without
    // executing game code or redistributing DAT. CI needs no proprietary assets.
    for(int i=1;i<argc;++i) {
        std::ifstream file(argv[i],std::ios::binary);CHECK(file.good());
        Reader r{{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()}};
        CHECK(!r.bytes.empty());
        auto key=r.bytes[0]^0xfa;for(auto& b:r.bytes)b^=key;
        CHECK(SQ_SUCCEEDED(sq_readclosure(vm,read,&r)));
        if(!kinoko_adapt_input_script(vm,error)){std::fprintf(stderr,"%s: %s\n",argv[i],error.c_str());return 1;}
        sq_settop(vm,0);
    }
    sq_close(vm);std::puts("input script adaptation passed");return 0;
}
