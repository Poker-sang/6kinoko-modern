#include "kinoko/mod_scripts.hpp"
#include "kinoko/mod_resources.hpp"
#include "retained_fixture.hpp"
#include <squirrel.h>
#include <fstream>
#include <cstring>
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"Mod scripts contract line %d: %s\n",__LINE__,error.c_str());return 1;}}while(0)
bool evaluate(SQVM* vm,const char* source) {
    const auto top=sq_gettop(vm);
    if(SQ_FAILED(sq_compilebuffer(vm,source,std::strlen(source),"assertions",SQTrue))){sq_settop(vm,top);return false;}
    sq_pushroottable(vm);const auto result=sq_call(vm,1,SQFalse,SQTrue);sq_settop(vm,top);return SQ_SUCCEEDED(result);
}
int main(){
    using namespace kinoko::mods;
    std::string error;auto* vm=sq_open(1024);
    CHECK(run_scripts(nullptr,error));
    CHECK(evaluate(vm,"if (\"KinokoMods\" in getroottable()) throw \"unexpected API\";"));
    auto root=retained_fixture("mod-scripts");
    std::filesystem::create_directories(root/"assets/data/custom/probe");
    auto prepare=[&](const std::string& source){
        std::ofstream(root/"assets/data/custom/probe/main.nut",std::ios::binary)<<source;
        std::ofstream(root/"catalog.tsv",std::ios::binary)<<"KINOKOMODS2\nM\tprobe\t1\t"<<sha256("probe")<<"\nE\tprobe\tdata/custom/probe/main.nut\nF\tdata/custom/probe/main.nut\t"<<sha256(source)<<"\n";
        return load(root/"catalog.tsv",error);
    };
    CHECK(prepare("foreach(kind in [\"stage\",\"enemy\",\"boss\",\"transformation\"]) mod.Register(kind,\"test\",{name=\"Test\",create=function(value){return value+1;}});"));
    CHECK(entrypoints().size()==1 && contains("data/custom/probe/main.nut"));
    const auto top=sq_gettop(vm);CHECK(run_scripts(vm,error));CHECK(sq_gettop(vm)==top);
    CHECK(evaluate(vm,R"SQ(
        foreach(kind in ["stage","enemy","boss","transformation"]) {
            if(KinokoMods.List(kind)[0]!="probe:test") throw "id";
            if(KinokoMods.Create(kind,"probe:test",[41])!=42) throw "factory";
            local d=KinokoMods.Get(kind,"probe:test"); d.name="changed";
            if(KinokoMods.Get(kind,"probe:test").name!="Test") throw "copy";
        }
        local rejected=false;
        try { KinokoMods.For("probe").Register("enemy","test",{name="Duplicate",create=function(){}}); }
        catch(e){rejected=true;}
        if(!rejected)throw "duplicate accepted";
        rejected=false;
        try { KinokoMods.Create("enemy","missing",[]); }catch(e){rejected=true;}
        if(!rejected)throw "missing accepted";
    )SQ"));
    CHECK(!run_scripts(vm,error) && !error.empty());CHECK(sq_gettop(vm)==top);
    sq_close(vm);vm=sq_open(1024);
    CHECK(prepare("this is not valid squirrel !"));CHECK(!run_scripts(vm,error));CHECK(sq_gettop(vm)==0);
    sq_close(vm);vm=sq_open(1024);
    CHECK(prepare("throw \"deliberate failure\";"));CHECK(!run_scripts(vm,error));CHECK(error.find("deliberate failure")!=std::string::npos);
    sq_close(vm);clear();return 0;
}
