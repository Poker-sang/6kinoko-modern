#include "kinoko/replay_runtime.hpp"
#include "kinoko/replay.hpp"
#include "kinoko/tas_bridge.hpp"
#include "kinoko/runtime_options.hpp"
#include "kinoko/runtime_clock.h"
#include "kinoko/game_host.h"
#include "kinoko/game_runtime.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_priority.h"
#include "kinoko/map_manager_records.hpp"
#include <SDL3/SDL.h>
#include <squirrel.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>
#include <stdexcept>

extern "C" uint32_t kinoko_script_random_state();
extern "C" SQVM* kinoko_primary_vm;
namespace {
enum class Mode {Off,Record,Playback};
Mode mode=Mode::Off;
std::ifstream input;
std::ofstream output,status;
std::ofstream details;
uint64_t details_from=0;
bool details_frame=false;
std::unique_ptr<kinoko::replay::Reader> reader;
std::unique_ptr<kinoko::replay::Writer> writer;
kinoko::replay::Frame frame,expected;
uint64_t frame_index=0;
bool stopped=false,completed=false,failed=false;
kinoko::input::Frame previous_actions;
KinokoInputPublishedState previous_legacy{};
std::string pending_error;
void (*error_handler)(const char*)=nullptr;
void quit() {SDL_Event event{};event.type=SDL_EVENT_QUIT;SDL_PushEvent(&event);}
void note(const std::string& message) {status<<message<<'\n';status.flush();}
void fail(const std::string& message) {
    if(failed)return;
    failed=true;stopped=true;kinoko::tas::failure(message);
    note("FAILED at frame "+std::to_string(frame_index)+": "+message);
    if(error_handler)error_handler(message.c_str());
    else if(!kinoko::tas::enabled())pending_error=message; // Report on the main thread after the game worker joins.
    quit();
}
struct Hash {
    uint64_t value=14695981039346656037ull;
    std::vector<uint64_t> fields;
    void number(uint64_t n) {if(details_frame)fields.push_back(n);for(int i=0;i<8;++i){value=(value^static_cast<uint8_t>(n>>(8*i)))*1099511628211ull;}}
    void real(float f) {uint32_t bits;std::memcpy(&bits,&f,4);number(bits);}
    void text(const char* s) {while(*s)value=(value^static_cast<uint8_t>(*s++))*1099511628211ull;number(0);}
    void dump(const std::string& key) const {
        if(!details_frame)return;
        details<<std::dec<<frame_index<<' '<<key<<std::hex;
        for(auto n:fields)details<<' '<<n;
        details<<'\n';
    }
};
uint64_t checkpoint() {
    details_frame=details.is_open() && frame_index>=details_from;
    Hash h;
    h.number(kinoko_game_masks.update);h.number(kinoko_game_masks.render);
    const auto& objects=*kinoko_game_objects();
    using namespace kinoko;
    const camera::View c(objects.camera);
    h.real(c.get(&camera::Record::x));h.real(c.get(&camera::Record::y));
    const map::ManagerView m(objects.map);
    h.number(m.get(&map::ManagerRecord::width));h.number(m.get(&map::ManagerRecord::height));h.number(m.get(&map::ManagerRecord::last_id));
    const actor::ManagerView manager(objects.actors);
    std::vector<std::pair<uint32_t,uint64_t>> actors;
    void* tree=manager.bytes(&actor::ManagerPrefix::actors);
    auto* sentinel=manager.get(&actor::ManagerPrefix::actors).head;
    for(auto* entry=kinoko_actor_priority_first(tree);entry && entry!=sentinel;entry=kinoko_actor_priority_next(tree,entry)) {
        const actor::ActorView a(kinoko_actor_priority_value(entry));Hash item;
        item.number(a.get(&actor::ActorRecord::id));item.number(a.get(&actor::ActorRecord::active));
        item.number(a.get(&actor::ActorRecord::release_pending));item.number(a.get(&actor::ActorRecord::flags));
        item.real(a.get(&actor::ActorRecord::x));item.real(a.get(&actor::ActorRecord::y));
        item.real(a.get(&actor::ActorRecord::velocity_x));item.real(a.get(&actor::ActorRecord::velocity_y));
        item.real(a.get(&actor::ActorRecord::direction));
        item.number(a.get(&actor::ActorRecord::take));item.number(a.get(&actor::ActorRecord::frame_index));
        item.number(a.get(&actor::ActorRecord::frame_time));
        for(auto hit:a.get(&actor::ActorRecord::hits))item.number(hit);
        item.dump("actor:"+std::to_string(a.get(&actor::ActorRecord::pool_handle)));
        actors.emplace_back(a.get(&actor::ActorRecord::pool_handle),item.value);
    }
    std::sort(actors.begin(),actors.end());h.number(actors.size());
    for(auto item:actors){h.number(item.first);h.number(item.second);}
    // Raw root iteration never invokes script getters or serializes pointers.
    std::vector<std::pair<std::string,uint64_t>> globals;
    auto* vm=kinoko_primary_vm;
    if(vm) {
        const auto top=sq_gettop(vm);sq_pushroottable(vm);sq_pushnull(vm);
        while(SQ_SUCCEEDED(sq_next(vm,-2))) {
            const char* name=nullptr;
            if(sq_gettype(vm,-2)==OT_STRING && SQ_SUCCEEDED(sq_getstring(vm,-2,&name))) {
                const char* tracked[]={"score","life","star","playerType","playerItem","currentMap","currentTime","clearCount","stageBeginTime"};
                if(std::any_of(std::begin(tracked),std::end(tracked),[&](const char* n){return std::strcmp(n,name)==0;})) {
                    Hash item;const auto type=sq_gettype(vm,-1);item.number(type);
                    if(type==OT_INTEGER){SQInteger v;sq_getinteger(vm,-1,&v);item.number(v);}
                    if(type==OT_FLOAT){SQFloat v;sq_getfloat(vm,-1,&v);item.real(v);}
                    if(type==OT_BOOL){SQBool v;sq_getbool(vm,-1,&v);item.number(v);}
                    if(type==OT_STRING){const char* v;sq_getstring(vm,-1,&v);item.text(v);}
                    item.dump(std::string("global:")+name);globals.emplace_back(name,item.value);
                }
            }
            sq_pop(vm,2);
        }
        sq_settop(vm,top);
    }
    std::sort(globals.begin(),globals.end());
    for(const auto& item:globals){h.text(item.first.c_str());h.number(item.second);}
    h.dump("state");if(details_frame)details.flush();
    return h.value;
}
}
bool kinoko_replay_start() {
    const auto& settings=kinoko::runtime::options();
    const bool explicit_paths=!settings.recording.empty() || !settings.playback.empty();
    const char* value=SDL_getenv("KINOKO_REPLAY_MODE");
    if(explicit_paths)value=settings.recording.empty()?"play":"record";
    if(!value || !*value)return true;
    kinoko::tas::start();previous_actions={};previous_legacy={};
    frame_index=0;stopped=false;completed=false;failed=false;pending_error.clear();
    try {
        if(const char* path=SDL_getenv("KINOKO_REPLAY_DETAILS")) {
            details.open(std::filesystem::u8path(path),std::ios::out);
            if(!details)throw std::runtime_error("Cannot create replay details log");
            const char* first=SDL_getenv("KINOKO_REPLAY_DETAILS_FROM");
            details_from=first?std::stoull(first):0;
        }
        const std::string requested(value);
        if(requested!="record" && requested!="play")throw std::runtime_error("Unknown replay mode");
        std::string identity=settings.identity;
        if(!explicit_paths) {
            std::ifstream marker(".replay-session");std::string marker_mode;std::getline(marker,marker_mode);
            if(marker_mode!=requested)throw std::runtime_error("Use Record-Replay.cmd / Play-Replay.cmd to create an isolated session");
            std::ifstream identity_file("session.identity");std::getline(identity_file,identity);
        }
        const auto replay_path=explicit_paths?(requested=="record"?settings.recording:settings.playback):std::filesystem::path("session.krec");
        mode=requested=="record"?Mode::Record:Mode::Playback;
        status.open(explicit_paths?settings.status:std::filesystem::path("replay-status.txt"),std::ios::out|std::ios::trunc);
        if(!status)throw std::runtime_error("Cannot create replay status log");
        if(mode==Mode::Record) {
            if(std::filesystem::exists(replay_path))throw std::runtime_error("Recording already exists; create a fresh session");
            output.open(replay_path,std::ios::binary|std::ios::out);
            writer=std::make_unique<kinoko::replay::Writer>(output,identity);
        }else {
            input.open(replay_path,std::ios::binary);
            reader=std::make_unique<kinoko::replay::Reader>(input,identity);
            if(kinoko::tas::enabled()) {
                if(std::filesystem::exists(settings.tas_output))throw std::runtime_error("Branch already exists");
                output.open(settings.tas_output,std::ios::binary|std::ios::out);
                writer=std::make_unique<kinoko::replay::Writer>(output,identity);
            }
        }
        kinoko_simulation_enable(1);
        note("Started "+requested+"; format 1; fixed simulation clock 60 Hz; identity "+identity);
        return true;
    }catch(const std::exception& e){fail(e.what());return false;}
}
bool kinoko_replay_begin_frame() {
    if(mode==Mode::Off)return true;
    if(stopped)return false;
    try {
        if(kinoko::tas::enabled()) {
            for(;;) {
                if(!kinoko::tas::boundary(frame_index,reader?reader->count():0,mode==Mode::Record)){quit();return false;}
                if(!kinoko::tas::take_control())break;
                mode=Mode::Record;note("Takeover at completed frame count "+std::to_string(frame_index));
            }
        }
        kinoko_simulation_frame(frame_index);frame={};frame.clock=kinoko_simulation_milliseconds();
        frame.random_before=kinoko_script_random_state();
        if(mode==Mode::Playback) {
            if(!reader->next(expected)){completed=true;stopped=true;quit();return false;}
            if(expected.clock!=frame.clock || expected.random_before!=frame.random_before)
                throw std::runtime_error("Pre-frame clock/random state differs at frame "+std::to_string(frame_index));
        }
        return true;
    }catch(const std::exception& e){fail(e.what());return false;}
}
void kinoko_replay_input(KinokoInputManager* manager,kinoko::input::Frame& actions) {
    if(mode==Mode::Off || stopped)return;
    if(mode==Mode::Playback){manager->published=expected.legacy;actions=expected.actions;}
    if(kinoko::tas::enabled() && mode==Mode::Record) {
        const auto mask=kinoko::tas::input_mask();
        for(int i=0;i<kinoko::input::Count;++i) {
            actions.held[i]=(mask&(1u<<i))? (previous_actions.held[i]==INT32_MAX?INT32_MAX:previous_actions.held[i]+1):0;
            actions.released[i]=previous_actions.held[i]>0 && actions.held[i]==0;
        }
        auto& legacy=manager->published;legacy={};
        using namespace kinoko::input;
        auto combine=[&](int a,int b){return std::max(actions.held[a],actions.held[b]);};
        legacy.x=combine(Left,MenuLeft)?-combine(Left,MenuLeft):combine(Right,MenuRight);
        legacy.y=combine(Up,MenuUp)?-combine(Up,MenuUp):combine(Down,MenuDown);
        legacy.buttons[0]=combine(Jump,Confirm);legacy.buttons[1]=combine(Pause,MenuAcceptAlt);
        legacy.buttons[2]=actions.held[UseItem];legacy.buttons[3]=std::max(combine(Attack,Run),actions.held[Carry]);
        for(int i=0;i<4;++i)legacy.released[i]=previous_legacy.buttons[i]>0 && legacy.buttons[i]==0;
    }
    frame.legacy=manager->published;frame.actions=actions;
    previous_actions=actions;previous_legacy=manager->published;
}
void kinoko_replay_end_frame() {
    if(mode==Mode::Off || stopped)return;
    try {
        frame.random_after=kinoko_script_random_state();frame.checkpoint=checkpoint();
        if(mode==Mode::Record)writer->append(frame);
        else if(!kinoko::replay::same_checkpoint(expected,frame)) {
            std::ostringstream message;message<<"Desync at frame "<<frame_index<<"; checkpoint expected "<<std::hex<<expected.checkpoint
                <<", actual "<<frame.checkpoint<<"; RNG expected "<<expected.random_after<<", actual "<<frame.random_after;
            throw std::runtime_error(message.str());
        }
        if(mode==Mode::Playback && kinoko::tas::enabled())writer->append(frame);
        if(kinoko::tas::enabled()){output.flush();if(!output)throw std::runtime_error("Cannot publish live replay frame");}
        ++frame_index;kinoko::tas::completed(frame_index);
        if(!kinoko::tas::enabled() && mode==Mode::Playback && frame_index==reader->count()){completed=true;stopped=true;quit();}
    }catch(const std::exception& e){fail(e.what());}
}
void kinoko_replay_finish() {
    if(mode!=Mode::Off) {
    try {
        if((mode==Mode::Record || kinoko::tas::enabled()) && writer && !failed){writer->finish();completed=true;}
        note(std::string(completed?"COMPLETED ":failed?"FAILED ":"ABORTED ")+std::to_string(frame_index)+" frames");
    }catch(const std::exception& e){fail(e.what());}
    writer.reset();reader.reset();output.close();input.close();status.close();details.close();details_frame=false;
    kinoko_simulation_enable(0);mode=Mode::Off;kinoko::tas::finish();
    }
    // Startup failures can occur before a mode is selected. They still need reporting.
    if(!pending_error.empty()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Replay stopped",pending_error.c_str(),nullptr);
        pending_error.clear();
    }
}
bool kinoko_replay_allow_rebind() {
    if(mode==Mode::Off)return true;
    fail("Changing key assignments during a recording/replay is not supported. Configure keys before starting a session.");return false;
}
void kinoko_replay_set_error_handler(void (*handler)(const char*)) {error_handler=handler;}
