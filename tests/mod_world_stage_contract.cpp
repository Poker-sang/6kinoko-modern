// Optional original-script contract: supply locally extracted worldmap/savedata/move CV4.
// Executes original bytecode in Squirrel with narrow non-rendering API fixtures.
#include <squirrel.h>
#include <sqstdaux.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <iterator>
void diagnostic(HSQUIRRELVM,const SQChar* format,...) {
    va_list arguments;va_start(arguments,format);std::vfprintf(stderr,format,arguments);va_end(arguments);
}
struct Input { std::string bytes; size_t pos=0; };
SQInteger read_bytes(SQUserPointer opaque,SQUserPointer target,SQInteger count) {
    auto& input=*static_cast<Input*>(opaque);
    if(count<0 || static_cast<size_t>(count)>input.bytes.size()-input.pos)return -1;
    std::memcpy(target,input.bytes.data()+input.pos,static_cast<size_t>(count));
    input.pos+=static_cast<size_t>(count);return count;
}
bool error(SQVM* vm) {
    sq_getlasterror(vm);const SQChar* message=nullptr;
    if(SQ_SUCCEEDED(sq_getstring(vm,-1,&message)))std::fprintf(stderr,"%s\n",message);
    return false;
}
bool source(SQVM* vm,const char* text) {
    const auto top=sq_gettop(vm);
    if(SQ_FAILED(sq_compilebuffer(vm,text,std::strlen(text),"world contract",SQFalse)))return error(vm);
    sq_pushroottable(vm);
    if(SQ_FAILED(sq_call(vm,1,SQFalse,SQTrue)))return error(vm);
    sq_settop(vm,top);return true;
}
bool load(SQVM* vm,const char* path,const char* scope=nullptr) {
    std::ifstream file(path,std::ios::binary);
    Input input{std::string{std::istreambuf_iterator<char>(file),{}}};
    if(input.bytes.empty())return false;
    const auto top=sq_gettop(vm);
    if(SQ_FAILED(sq_readclosure(vm,read_bytes,&input)))return error(vm);
    sq_pushroottable(vm);
    if(scope){sq_pushstring(vm,scope,-1);if(SQ_FAILED(sq_get(vm,-2)))return error(vm);sq_remove(vm,-2);}
    if(SQ_FAILED(sq_call(vm,1,SQFalse,SQTrue)))return error(vm);
    sq_settop(vm,top);return true;
}
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"world-stage contract line %d\n",__LINE__);sq_close(vm);return 1;}}while(0)
int main(int argc,char** argv) {
    if(argc!=4 && argc!=6){std::fprintf(stderr,"Supply worldmap.cv4 savedata.cv4 move.cv4 [playerstatus.cv4 presentation.nut]\n");return 2;}
    auto* vm=sq_open(2048);
    sq_setprintfunc(vm,diagnostic);sqstd_seterrorhandlers(vm);
    CHECK(source(vm,R"SQ(
        ::CompileFile <- function(path,env) {};
        ::world <- {};
        ::TYPE_2HEAD <- 0;
    )SQ"));
    CHECK(load(vm,argv[1],"world"));
    CHECK(load(vm,argv[2]));
    CHECK(load(vm,argv[3],"world"));
    CHECK(source(vm,R"SQ(
        InitSaveDataTable(savedata[0]);
        ::currentSavedata = savedata[0].weakref();
        local first=InitStageSaveData("w1-c01a.act").ref();
        first.clear=7; first.score=1234;
        local added=InitStageSaveData("w1-c16a.act").ref();
        if(added==first || added.stageName!="w1-c16a" || added.clear!=0 || added.isBoss)
            throw "new-stage save identity";
        added.clear=1;
        if(InitStageSaveData("w1-c16a.act").ref().clear!=1 || first.clear!=7 || first.score!=1234)
            throw "save re-entry or original progress";
        ::selectedChip <- 973;
        world.event <- {layout={
            GetChipByPosition=function(x,y){return 0;},
            GetChipLayout=function(index){return {chipID=selectedChip};}
        }};
        world.marisaX <- 80; world.marisaY <- 816;
        world.world <- 1; world.w8_stage <- 0;
        if(world.GetStageNo()!="c16a")throw "original node lookup";
        selectedChip=958;
        if(world.GetStageNo()!="c01a")throw "original first node changed";
        ::queued <- null;
        ::entered <- null;
        ::SetGlobalUpdateFunction <- function(callback){queued=callback;};
        ::InitStage <- function(path){entered=path;};
        ::PlaySE <- function(id){};
        ::Fader1 <- {FadeOut=function(a,b,c,d){}};
        ::clearCount <- 0;
        // Fields normally published by WorldMap.Init before entering a stage.
        world.isShop <- false; world.suspend <- false; world.lastEnterStage <- "";
        world.useOldMenu <- false;
        world.player <- {visible=true,Suspend=function(){}};
        ::PlayerStatus <- {useMap=true,useStage=false};
        ::WorldMap <- {global=world,Suspend=function(){world.Suspend();}};
        world.InitStage("c16a");
        if(world.lastEnterStage!="w1-c16a" || !world.suspend || queued==null)
            throw "original transition setup";
        for(local i=0;i<35 && entered==null;++i)queued.call(world);
        if(entered!="w1-c16a.act")throw "normal delayed transition";
        if(currentSavedata.status.worldPosX!=80 || currentSavedata.status.worldPosY!=816)
            throw "return position persistence";
        // Actual original movement code: test new road in both directions.
        world.CheckBlock <- function(x,y){return true;};
        ::roadFlag <- 176;
        world.rail <- {layout={GetChipByPosition=function(x,y){return 0;},GetChipLayout=function(i){return {chipID=202};}}};
        world.mapChip <- {GetChipInfo=function(id){return {flag0=roadFlag};}};
        world.funcUpdate <- null;world.moveCount <- 0;world.walkDirection <- 0;
        if(!world.SetNextMove(0,-1,false) || world.funcUpdate!=world.Update_MoveUp)throw "house upward exit";
        roadFlag=32;
        if(!world.SetNextMove(0,1,false) || world.funcUpdate!=world.Update_MoveDown)throw "node downward exit";
    )SQ"));
    if(argc==6) {
        CHECK(source(vm,"::PlayerStatus = {};"));
        CHECK(load(vm,argv[4],"PlayerStatus"));
        CHECK(source(vm,R"SQ(
            ::selectedLabel <- null; ::selectedWorldLabel <- null;
            PlayerStatus.resource <- {};
            foreach(name in ["blank","wmap_stage01","wmap_stage02","wmap_name01","wmap_stage0b","mod_stage16"])
                PlayerStatus.resource[name] <- name;
            PlayerStatus.wmap_stage <- {AssociateResource=function(r){selectedLabel=r;}};
            PlayerStatus.wmap_name <- {AssociateResource=function(r){selectedWorldLabel=r;}};
        )SQ"));
        std::ifstream entry(argv[5],std::ios::binary);
        const std::string text{std::istreambuf_iterator<char>(entry),{}};
        CHECK(!text.empty() && source(vm,text.c_str()));
        CHECK(source(vm,R"SQ(
            PlayerStatus.SetWorld(1,0,"c16a");
            if(selectedLabel!="mod_stage16" || selectedWorldLabel!="wmap_name01" || PlayerStatus.stage!="c16a")
                throw "new stage presentation/identity";
            PlayerStatus.SetWorld(1,0,"c02a");
            if(selectedLabel!="wmap_stage02" || PlayerStatus.stage!="c02a")throw "original label changed";
            PlayerStatus.SetWorld(1,0,"marisahouse");
            if(selectedLabel!="wmap_stage0b")throw "house label changed";
            PlayerStatus.SetWorld(1,0,"");
            if(selectedLabel!="blank" || PlayerStatus.stage!="")throw "blank label not restored";
        )SQ"));
        std::puts("PASS: presentation entrypoint with actual original PlayerStatus.SetWorld and original-node restoration");
    }
    sq_close(vm);
    std::puts("PASS: original CV4 node mapping, independent save/re-entry, delayed transition, return position and road directions");
    return 0;
}
