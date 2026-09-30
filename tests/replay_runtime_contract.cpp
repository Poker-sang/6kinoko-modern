#include "kinoko/replay_runtime.hpp"
#include "kinoko/tas_bridge.hpp"
#include "kinoko/replay.hpp"
#include <future>
#include <thread>
#include "kinoko/runtime_options.hpp"
#include "kinoko/game_host.h"
#include "kinoko/game_runtime.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/map_manager_records.hpp"
#include "kinoko/input_manager.h"
#include "kinoko/actor_priority.h"
#include <SDL3/SDL.h>
#include <squirrel.h>
#include <sqstdmath.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cfenv>
namespace {
KinokoInputManager input_manager{};
kinoko::camera::Record camera{};
kinoko::map::ManagerRecord map{};
kinoko::actor::ManagerPrefix actors{};
kinoko::actor::ActorRecord actor{};
KinokoGameObjects objects{&input_manager,reinterpret_cast<KinokoActorManager*>(&actors),
    reinterpret_cast<KinokoCamera*>(&camera),reinterpret_cast<KinokoMapManager*>(&map)};
int errors=0;
void report(const char* message){++errors;std::fprintf(stderr,"Expected/observed replay error: %s\n",message);}
}
extern "C" {
SQVM* kinoko_primary_vm=nullptr;
KinokoGameMasks kinoko_game_masks{-1,-1};
const KinokoGameObjects* kinoko_game_objects(){return &objects;}
void kinoko_trace(const char*){}
void kinoko_trace_i32(const char*,int32_t){}
}
namespace {
bool evaluate(const char* script) {
    auto* vm=kinoko_primary_vm;const auto top=sq_gettop(vm);
    bool ok=SQ_SUCCEEDED(sq_compilebuffer(vm,script,strlen(script),"replay-fixture",SQTrue));
    if(ok){sq_pushroottable(vm);ok=SQ_SUCCEEDED(sq_call(vm,1,SQFalse,SQTrue));}
    sq_settop(vm,top);return ok;
}
bool reset() {
    camera={};input_manager={};actor={};actor.id=7;actor.pool_handle=42;
    if(actors.actors.head)kinoko_priority_clear(&actors.actors);else kinoko_priority_construct(&actors.actors);
    kinoko_actor_priority_insert(&actors.actors,reinterpret_cast<KinokoActor*>(&actor));
    if(kinoko_primary_vm)sq_close(kinoko_primary_vm);
    kinoko_primary_vm=sq_open(128);sq_pushroottable(kinoko_primary_vm);
    sqstd_register_mathlib(kinoko_primary_vm);sq_pop(kinoko_primary_vm,1);
    return evaluate("srand(1); score <- 0;");
}
std::string status() {const auto& p=kinoko::runtime::options().status;std::ifstream in(p.empty()?std::filesystem::path("replay-status.txt"):p);return {std::istreambuf_iterator<char>(in),{}};}
}
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"runtime replay contract line %d\n",__LINE__);return 1;}}while(0)
int main() {
    namespace fs=std::filesystem;
    const auto original=fs::current_path();
    const auto root=original/"Testing"/"fixtures"/("replay-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root/"record");fs::current_path(root/"record");
    std::ofstream("session.identity")<<std::string(64,'a');std::ofstream(".replay-session")<<"record";
    SDL_Init(SDL_INIT_EVENTS);kinoko_replay_set_error_handler(report);
    auto* env=SDL_GetEnvironment();SDL_SetEnvironmentVariable(env,"KINOKO_REPLAY_MODE","record",true);
    CHECK(reset());
    // Actual first divergent cos operand from the cross-platform game replay.
    // Exercise the registered VM function under the game's upward rounding.
    const auto rounding=std::fegetround();CHECK(std::fesetround(FE_UPWARD)==0);
    uint32_t operand_bits=0x3f5710c4;float operand;std::memcpy(&operand,&operand_bits,4);
    sq_pushroottable(kinoko_primary_vm);sq_pushstring(kinoko_primary_vm,"mathInput",-1);
    sq_pushfloat(kinoko_primary_vm,operand);sq_newslot(kinoko_primary_vm,-3,SQFalse);sq_pop(kinoko_primary_vm,1);
    CHECK(evaluate("mathResult <- cos(mathInput);"));
    sq_pushroottable(kinoko_primary_vm);sq_pushstring(kinoko_primary_vm,"mathResult",-1);
    CHECK(SQ_SUCCEEDED(sq_get(kinoko_primary_vm,-2)));SQFloat math_result;
    CHECK(SQ_SUCCEEDED(sq_getfloat(kinoko_primary_vm,-1,&math_result)));sq_pop(kinoko_primary_vm,2);
    uint32_t result_bits;std::memcpy(&result_bits,&math_result,4);
    CHECK(std::fesetround(rounding)==0);CHECK(result_bits==0x3f2ad9fc);
    CHECK(kinoko_replay_start());
    for(int i=0;i<4;++i) {
        CHECK(kinoko_replay_begin_frame());kinoko::input::Frame actions;
        actions.held[kinoko::input::Jump]=i+1;input_manager.published.buttons[0]=i+1;
        kinoko_replay_input(&input_manager,actions);kinoko::input::publish(actions,input_manager.actions);
        camera.x+=float(actions.held[kinoko::input::Jump]);CHECK(evaluate("score += rand();"));
        kinoko_replay_end_frame();
    }
    kinoko_replay_finish();CHECK(errors==0 && status().find("COMPLETED 4 frames")!=std::string::npos);
    for(int scenario=0;scenario<3;++scenario) {
        const auto run=root/("play-"+std::to_string(scenario));fs::create_directory(run);
        // Replay reads the original recording in place, independent of cwd.
        fs::current_path(original);
        std::vector<std::string> arguments={"contract","--save-dir",run.u8string(),"--replay",(root/"record/session.krec").u8string(),
            "--replay-status",(run/"replay-status.txt").u8string(),"--replay-identity",std::string(64,'a')};
        std::vector<char*> argv;for(auto& argument:arguments)argv.push_back(argument.data());std::string error;
        CHECK(kinoko::runtime::parse_options(static_cast<int>(argv.size()),argv.data(),error));
        SDL_UnsetEnvironmentVariable(env,"KINOKO_REPLAY_MODE");
        CHECK(reset() && kinoko_replay_start());const int prior_errors=errors;
        for(int i=0;i<4;++i) {
            CHECK(kinoko_replay_begin_frame());kinoko::input::Frame live;live.held[kinoko::input::Jump]=99;
            input_manager.published.buttons[0]=77;
            kinoko_replay_input(&input_manager,live);kinoko::input::publish(live,input_manager.actions);
            CHECK(live.held[kinoko::input::Jump]==i+1 && input_manager.published.buttons[0]==i+1);
            camera.x+=float(live.held[kinoko::input::Jump]);CHECK(evaluate("score += rand();"));
            if(i==2 && scenario==1)actor.x+=1; // Physical-state divergence.
            if(i==2 && scenario==2)CHECK(evaluate("rand();")); // RNG-only divergence.
            kinoko_replay_end_frame();
            if(i==2 && scenario){CHECK(!kinoko_replay_begin_frame());break;}
        }
        kinoko_replay_finish();
        CHECK(scenario ? (errors==prior_errors+1 && status().find("FAILED at frame 2")!=std::string::npos):
            (errors==prior_errors && status().find("COMPLETED 4 frames")!=std::string::npos));
    }
    // TAS branch: validate two source frames, pause, take over and release jump.
    const auto tas=root/"tas";fs::create_directory(tas);
    std::vector<std::string> args={"contract","--save-dir",tas.u8string(),"--replay",(root/"record/session.krec").u8string(),
        "--replay-status",(tas/"status.txt").u8string(),"--replay-identity",std::string(64,'a'),"--tas-dir",tas.u8string(),"--tas-output",(tas/"branch.krec").u8string()};
    std::vector<char*> av;for(auto& a:args)av.push_back(a.data());std::string error;
    CHECK(kinoko::runtime::parse_options(int(av.size()),av.data(),error));CHECK(reset() && kinoko_replay_start());
    std::ofstream(tas/"command.txt")<<"1 target 2\n";
    for(int i=0;i<2;++i){CHECK(kinoko_replay_begin_frame());kinoko::input::Frame action;
        kinoko_replay_input(&input_manager,action);camera.x+=float(action.held[kinoko::input::Jump]);CHECK(evaluate("score += rand();"));kinoko_replay_end_frame();}
    std::ofstream(tas/"command.txt")<<"2 takeover 0\n";
    auto controller=std::async(std::launch::async,[&] {
        for(int i=0;i<500;++i){std::ifstream in(tas/"state.txt");std::string line;std::getline(in,line);in.close();
            if(line.find("live-paused")!=line.npos){std::ofstream(tas/"input.txt")<<"1 0\n";std::ofstream(tas/"command.txt")<<"3 target 3\n";return true;}
            std::this_thread::sleep_for(std::chrono::milliseconds(2));}
        kinoko::tas::shutdown();return false;
    });
    CHECK(kinoko_replay_begin_frame());CHECK(controller.get());kinoko::input::Frame action;
    kinoko_replay_input(&input_manager,action);CHECK(action.held[kinoko::input::Jump]==0 && action.released[kinoko::input::Jump]==1);
    CHECK(evaluate("score += rand();"));kinoko_replay_end_frame();kinoko_replay_finish();
    {std::ifstream in(tas/"branch.krec",std::ios::binary);kinoko::replay::Reader branch(in,std::string(64,'a'));CHECK(branch.count()==3);}
    // Rebuild argument list explicitly (option,value pairs).
    args={"contract","--save-dir",tas.u8string(),"--replay",(tas/"branch.krec").u8string(),"--replay-status",(tas/"verify.txt").u8string(),"--replay-identity",std::string(64,'a')};
    av.clear();for(auto& a:args)av.push_back(a.data());CHECK(kinoko::runtime::parse_options(int(av.size()),av.data(),error));
    const int before=errors;CHECK(reset() && kinoko_replay_start());
    for(int i=0;i<3;++i){CHECK(kinoko_replay_begin_frame());kinoko::input::Frame a;kinoko_replay_input(&input_manager,a);
        camera.x+=float(a.held[kinoko::input::Jump]);CHECK(evaluate("score += rand();"));kinoko_replay_end_frame();}
    kinoko_replay_finish();CHECK(errors==before);
    // Execute actual edit plans through the real runtime, then verify the new
    // RNG/checkpoint stream from a fresh VM. Cover frame zero and a verified prefix.
    for(int first:{0,2}) {
        const auto edit=root/("edit-"+std::to_string(first));fs::create_directory(edit);
        {std::ofstream out(edit/"edit.bin",std::ios::binary);out.write("KTASED01",8);
        auto word=[&](uint32_t n){for(int b=0;b<4;++b)out.put(char(n>>(8*b)));};
        word(4);word(first);for(int f=0;f<4;++f)word(f==first?0:1u<<kinoko::input::Jump);}
        args={"contract","--save-dir",edit.u8string(),"--replay",(root/"record/session.krec").u8string(),
            "--replay-status",(edit/"generate.txt").u8string(),"--replay-identity",std::string(64,'a'),
            "--tas-dir",edit.u8string(),"--tas-output",(edit/"edited.krec").u8string()};
        av.clear();for(auto& a:args)av.push_back(a.data());CHECK(kinoko::runtime::parse_options(int(av.size()),av.data(),error));
        CHECK(reset() && kinoko_replay_start());std::ofstream(edit/"command.txt")<<"1 target 4\n";
        int held=0;const auto prior=errors;
        for(int f=0;f<4;++f){CHECK(kinoko_replay_begin_frame());kinoko::input::Frame a;
            kinoko_replay_input(&input_manager,a);held=f==first?0:held+1;
            CHECK(a.held[kinoko::input::Jump]==held);
            CHECK(a.released[kinoko::input::Jump]==(f==first&&first>0));
            camera.x+=float(held);CHECK(evaluate("score += rand();"));kinoko_replay_end_frame();}
        kinoko_replay_finish();CHECK(errors==prior);
        args={"contract","--save-dir",edit.u8string(),"--replay",(edit/"edited.krec").u8string(),
            "--replay-status",(edit/"verify.txt").u8string(),"--replay-identity",std::string(64,'a')};
        av.clear();for(auto& a:args)av.push_back(a.data());CHECK(kinoko::runtime::parse_options(int(av.size()),av.data(),error));
        CHECK(reset() && kinoko_replay_start());
        for(int f=0;f<4;++f){CHECK(kinoko_replay_begin_frame());kinoko::input::Frame a;kinoko_replay_input(&input_manager,a);
            camera.x+=float(a.held[kinoko::input::Jump]);CHECK(evaluate("score += rand();"));kinoko_replay_end_frame();}
        kinoko_replay_finish();CHECK(errors==prior && status().find("COMPLETED 4 frames")!=std::string::npos);
    }
    for(int length:{3,6}) {
        const int first=length==3?1:4;
        const auto edited=root/("length-"+std::to_string(length));fs::create_directory(edited);
        {std::ofstream output(edited/"edit.bin",std::ios::binary);output.write("KTASED02",8);
        auto word=[&](uint32_t number){for(int byte=0;byte<4;++byte)output.put(char(number>>(8*byte)));};
        word(length);word(first);word(4);for(int frame=0;frame<length;++frame)word(frame==first?0:1u<<kinoko::input::Jump);}
        args={"contract","--save-dir",edited.u8string(),"--replay",(root/"record/session.krec").u8string(),
            "--replay-status",(edited/"generate.txt").u8string(),"--replay-identity",std::string(64,'a'),
            "--tas-dir",edited.u8string(),"--tas-output",(edited/"edited.krec").u8string()};
        av.clear();for(auto& argument:args)av.push_back(argument.data());CHECK(kinoko::runtime::parse_options(int(av.size()),av.data(),error));
        CHECK(reset() && kinoko_replay_start());std::ofstream(edited/"command.txt")<<"1 speed 400\n";
        int duration=0;const int prior=errors;
        for(int frame=0;frame<length;++frame){
            if(frame==1)std::ofstream(edited/"command.txt")<<"2 target "<<length<<'\n';
            CHECK(kinoko_replay_begin_frame());kinoko::input::Frame actions;kinoko_replay_input(&input_manager,actions);
            duration=frame==first?0:duration+1;CHECK(actions.held[kinoko::input::Jump]==duration);
            camera.x+=float(duration);CHECK(evaluate("score += rand();"));kinoko_replay_end_frame();
        }
        kinoko_replay_finish();CHECK(errors==prior);
        args={"contract","--save-dir",edited.u8string(),"--replay",(edited/"edited.krec").u8string(),
            "--replay-status",(edited/"verify.txt").u8string(),"--replay-identity",std::string(64,'a')};
        av.clear();for(auto& argument:args)av.push_back(argument.data());CHECK(kinoko::runtime::parse_options(int(av.size()),av.data(),error));
        CHECK(reset() && kinoko_replay_start());
        for(int frame=0;frame<length;++frame){CHECK(kinoko_replay_begin_frame());kinoko::input::Frame actions;kinoko_replay_input(&input_manager,actions);
            camera.x+=float(actions.held[kinoko::input::Jump]);CHECK(evaluate("score += rand();"));kinoko_replay_end_frame();}
        kinoko_replay_finish();CHECK(errors==prior && status().find("COMPLETED "+std::to_string(length)+" frames")!=std::string::npos);
    }
    sq_close(kinoko_primary_vm);kinoko_primary_vm=nullptr;
    kinoko_priority_destroy(&actors.actors);
    SDL_UnsetEnvironmentVariable(env,"KINOKO_REPLAY_MODE");SDL_Quit();fs::current_path(original);
    std::printf("Record/playback, physical-input isolation and first divergent frame verified; fixtures retained at %s\n",root.string().c_str());
    return 0;
}
