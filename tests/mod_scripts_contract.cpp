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
    CHECK(evaluate(vm,R"SQ(
        ::stageStarted <- false;
        KinokoMods.For("probe").Register("stage","launch",{name="Launch test",create=function(){::stageStarted=true;}});
        ::CreateActor <- function(init,x,y,z,arg) { return {init=init,x=x,y=y,z=z,arg=arg}; };
        ::SetInitFunctionByID <- function(id,init,env) {
            if(id!=0x1234)throw "wrong map ID";
            env.Init1234 <- init;
        };
        ::CreateActorFromMap <- function(layer,env) {
            if(layer!="custom-enemies" || !("Init1234" in env))throw "wrong layer/environment";
            ::mapSpawned <- true;
        };
        local init=function(t){return t;};
        KinokoMods.For("probe").Register("enemy","mapped",{name="Mapped",create=function(){},init=init});
        local actor=KinokoMods.Spawn("enemy","probe:mapped",12,34,-1,"argument");
        if(actor.init!=init || actor.x!=12 || actor.y!=34 || actor.z!=-1 || actor.arg!="argument")throw "spawn forwarding";
        local env={};
        KinokoMods.BindMapActor("enemy","probe:mapped",0x1234,env);
        if(env.Init1234!=init)throw "init forwarding";
        local rejected=false;
        try { KinokoMods.BindMapActor("enemy","probe:mapped",0x1234,env); } catch(e){rejected=true;}
        if(!rejected)throw "map collision accepted";
        rejected=false;
        try { KinokoMods.BindMapActor("enemy","probe:mapped",65536,{}); } catch(e){rejected=true;}
        if(!rejected)throw "invalid map ID accepted";
        KinokoMods.SpawnMap("custom-enemies",[{kind="enemy",id="probe:mapped",mapId=0x1234}]);
        if(!mapSpawned)throw "map dispatch";
    )SQ"));
    std::vector<StageChoice> stages;CHECK(list_stages(vm,stages,error));
    CHECK(stages.size()==2 && stages[1].id=="probe:launch" && stages[1].name=="Launch test");
    CHECK(launch_stage(vm,"probe:launch",error));CHECK(evaluate(vm,"if(!stageStarted)throw \"stage did not run\";"));
    CHECK(!launch_stage(vm,"probe:missing",error));CHECK(sq_gettop(vm)==top);
    CHECK(select_startup_stage(vm,nullptr,nullptr,error));
    CHECK(!run_scripts(vm,error) && !error.empty());CHECK(sq_gettop(vm)==top);
    sq_close(vm);vm=sq_open(1024);
    CHECK(prepare("this is not valid squirrel !"));CHECK(!run_scripts(vm,error));CHECK(sq_gettop(vm)==0);
    sq_close(vm);vm=sq_open(1024);
    CHECK(prepare("throw \"deliberate failure\";"));CHECK(!run_scripts(vm,error));CHECK(error.find("deliberate failure")!=std::string::npos);
    sq_close(vm);vm=sq_open(1024);
    std::ifstream practice(KINOKO_PRACTICE_SCRIPT,std::ios::binary);
    const std::string practice_source{std::istreambuf_iterator<char>(practice),{}};
    CHECK(!practice_source.empty());CHECK(prepare(practice_source));CHECK(run_scripts(vm,error));
    CHECK(evaluate(vm,R"SQ(
        ::events <- [];
        ::savedata <- [{}];
        ::life <- 0; ::stageTimeStop <- false;
        ::TitleMenu <- {pl={EndStage=function(){events.append("title-end");}}};
        ::WorldMap <- {pl={EndStage=function(){events.append("world-end");}}};
        ::Logo <- {pl={EndStage=function(){events.append("logo-end");}}};
        ::PlayerStatus <- {life=0};
        ::Fader2 <- {FadeIn=function(a,b,c,d){events.append("fade");}};
        ::InitGlobal <- function(){events.append("global");};
        ::InitStage <- function(path){if(path!="mods/practice-stage.act")throw "map path";events.append("stage");};
        ::ChangeStageToWorld <- function(){events.append("world");};
        ::ChangeStageToTitle <- function(){events.append("title");};
        ::originalWorld <- ChangeStageToWorld; ::originalTitle <- ChangeStageToTitle;
    )SQ"));
    CHECK(launch_stage(vm,"probe:first-stage",error));
    CHECK(evaluate(vm,R"SQ(
        if(events.len()!=6 || events[0]!="global" || events[1]!="title-end" ||
           events[2]!="world-end" || events[3]!="logo-end" || events[4]!="stage" || events[5]!="fade")throw "startup order";
        if(life!=99 || PlayerStatus.life!=99 || !stageTimeStop)throw "practice flags";
        ChangeStageToWorld();
        if(events.top()!="title" || ChangeStageToWorld!=originalWorld || ChangeStageToTitle!=originalTitle)throw "return/restore";
        ::InitStage=function(path){throw "map load failure";};
    )SQ"));
    CHECK(!launch_stage(vm,"probe:first-stage",error));
    CHECK(evaluate(vm,"if(ChangeStageToWorld!=originalWorld || ChangeStageToTitle!=originalTitle)throw \"error restore\";"));
    sq_close(vm);clear();return 0;
}
