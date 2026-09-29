#include "kinoko/replay_runtime.hpp"
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
    sq_close(kinoko_primary_vm);kinoko_primary_vm=nullptr;
    kinoko_priority_destroy(&actors.actors);
    SDL_UnsetEnvironmentVariable(env,"KINOKO_REPLAY_MODE");SDL_Quit();fs::current_path(original);
    std::printf("Record/playback, physical-input isolation and first divergent frame verified; fixtures retained at %s\n",root.string().c_str());
    return 0;
}
