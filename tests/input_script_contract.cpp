#include "kinoko/input_script_adapter.hpp"
#include "kinoko/act_script_storage.hpp"
#include "kinoko/squirrel_game_objects.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
extern "C" {
char kinoko_sqrat_trace_enabled=0;
const void* kinoko_sqrat_object_vtable() {static const int tag=1;return &tag;}
const void* kinoko_sqrat_root_vtable() {static const int tag=2;return &tag;}
}
extern "C" void kinoko_trace(const char*) {}
extern "C" void kinoko_trace_i32(const char*, int32_t) {}
#ifdef KINOKO_STANDALONE_VM
SQInteger kinoko_squirrel_invoke_native(HSQUIRRELVM v,SQFUNCTION f){return f(v);}
#endif
namespace {
SQInteger write(SQUserPointer user,SQUserPointer bytes,SQInteger count) {
    auto& out=*static_cast<std::vector<unsigned char>*>(user);
    auto* first=static_cast<unsigned char*>(bytes);out.insert(out.end(),first,first+count);return count;
}
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
    // Exercise both real ACT execution entry points, not only the adapter.
    const char* fixture="function DisableInput() { input.x=0; input.y=0; input.b0=0; input.b2=0; input.b3=0; }";
    for(int twice=0;twice<2;++twice) {
        CHECK(SQ_SUCCEEDED(sq_compilebuffer(vm,fixture,strlen(fixture),"data/script/global.nut",SQTrue)));
        std::vector<unsigned char> data;
        CHECK(SQ_SUCCEEDED(sq_writeclosure(vm,write,&data)));sq_pop(vm,1);
        kinoko::act::ScriptStorageRecord script{};script.bytes=data.data();script.size=static_cast<uint32_t>(data.size());
        sq_pushroottable(vm);HSQOBJECT root;sq_getstackobj(vm,-1,&root);sq_pop(vm,1);
        kinoko::act::ScriptValueStorage environment{};std::memcpy(environment.data(),&root,sizeof(root));
        const auto top=sq_gettop(vm);
        CHECK((twice?kinoko_execute_act_file_bytecode(vm,&script,environment.data()):
            kinoko_execute_embedded_act_script(vm,&script,environment.data()))==1);
        CHECK(sq_gettop(vm)==top);
        CHECK(run(vm,"input.jump=7; input.attack=9; input.run=10; input.carry=11; DisableInput(); if(input.jump!=0 || input.attack!=0 || input.run!=0 || input.carry!=0) throw 1;","fixture"));
    }
    // Unrelated mod functions are not rewritten based only on field names.
    CHECK(run(vm,"input.b0=7; function Read() { return input.b0; } if(Read()!=7) throw 1;","mod.nut",true));
    const char* unsupported="function DisableInput() { input.b0=1; }";
    CHECK(SQ_SUCCEEDED(sq_compilebuffer(vm,unsupported,strlen(unsupported),"data/script/global.nut",SQTrue)));
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
