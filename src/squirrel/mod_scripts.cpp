#include "kinoko/mod_scripts.hpp"
#include "kinoko/mod_resources.hpp"
#include "kinoko/squirrel_source_runtime.h"
#include <squirrel.h>
#include <cstring>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <SDL3/SDL.h>

namespace kinoko::mods {
namespace {
constexpr char registry[] = R"SQ(
local definitions = { stage = {}, enemy = {}, boss = {}, transformation = {} };
local ordered = { stage = [], enemy = [], boss = [], transformation = [] };
local category = function(kind) : (definitions) {
    if (!(kind in definitions)) throw "Unknown Mod content kind";
    return definitions[kind];
};
local register = function(owner, kind, name, definition) : (category, ordered) {
    local target = category(kind);
    if (typeof name != "string" || name.len() == 0 || name.len() > 127)
        throw "Invalid content name";
    for (local i = 0; i < name.len(); ++i)
        if ("abcdefghijklmnopqrstuvwxyz0123456789._-".find(name.slice(i,i+1)) == null)
            throw "Invalid content name";
    if (typeof definition != "table" || !("name" in definition) ||
        typeof definition.name != "string" || !("create" in definition) ||
        typeof definition.create != "function") throw "Expected name and create function";
    local id = owner + ":" + name;
    if (id in target) throw "Duplicate Mod content id";
    target[id] <- clone definition;
    ordered[kind].append(id);
    return id;
};
if ("KinokoMods" in getroottable()) throw "KinokoMods root name already exists";
::KinokoMods <- {
    apiVersion = 1,
    For = function(owner) : (register) {
        return { Register = function(kind, name, definition) : (register, owner) {
            return register(owner, kind, name, definition);
        }};
    },
    List = function(kind) : (category, ordered) { category(kind); return clone ordered[kind]; },
    Get = function(kind, id) : (category) {
        local target = category(kind);
        if (!(id in target)) throw "Unknown Mod content id";
        return clone target[id];
    },
    Create = function(kind, id, arguments) : (category) {
        local target = category(kind);
        if (!(id in target)) throw "Unknown Mod content id";
        if (typeof arguments != "array") throw "Expected argument array";
        local callArgs = clone arguments;
        callArgs.insert(0, getroottable());
        return target[id].create.acall(callArgs);
    },
    Spawn = function(kind, id, x, y, direction, argument) : (category) {
        if (kind != "enemy" && kind != "boss") throw "Spawn expects enemy or boss";
        local target = category(kind);
        if (!(id in target) || !("init" in target[id]) || typeof target[id].init != "function")
            throw "Content requires an actor init function";
        return CreateActor(target[id].init, x, y, direction, argument);
    },
    BindMapActor = function(kind, id, mapId, environment) : (category) {
        if (kind != "enemy" && kind != "boss") throw "Map binding expects enemy or boss";
        if (typeof mapId != "integer" || mapId < 0 || mapId > 65535 || typeof environment != "table")
            throw "Expected a 16-bit map ID and environment table";
        local target = category(kind);
        if (!(id in target) || !("init" in target[id]) || typeof target[id].init != "function")
            throw "Content requires an actor init function";
        local key = "Init";
        for (local shift = 12; shift >= 0; shift -= 4) {
            local digit = (mapId >> shift) & 15;
            key += "0123456789abcdef".slice(digit, digit+1);
        }
        if (key in environment) throw "Map ID already bound in this environment";
        SetInitFunctionByID(mapId, target[id].init, environment);
        return environment;
    },
    SpawnMap = function(layer, bindings) {
        if (typeof layer != "string" || typeof bindings != "array") throw "Expected layer and binding array";
        local environment = {};
        foreach (binding in bindings)
            KinokoMods.BindMapActor(binding.kind, binding.id, binding.mapId, environment);
        CreateActorFromMap(layer, environment);
    }
};
)SQ";

void execute(SQVM* vm, const std::string& source, const char* name) {
    const auto top=sq_gettop(vm);
    struct Stack { SQVM* vm; SQInteger top; ~Stack(){sq_settop(vm,top);} } stack{vm,top};
    if(SQ_SUCCEEDED(sq_compilebuffer(vm,source.data(),static_cast<SQInteger>(source.size()),name,SQFalse))) {
        sq_pushroottable(vm);
        if(SQ_SUCCEEDED(kinoko_sq_call(vm,1,SQFalse,SQFalse)))return;
    }
    sq_getlasterror(vm);const SQChar* detail=nullptr;
    sq_getstring(vm,-1,&detail);
    throw std::runtime_error(std::string(name)+": "+(detail?detail:"Squirrel execution failed"));
}

}
namespace {
struct StackGuard {SQVM* vm;SQInteger top;~StackGuard(){sq_settop(vm,top);}};
void check(SQVM* vm,SQRESULT result) {
    if(SQ_SUCCEEDED(result))return;
    sq_getlasterror(vm);const SQChar* detail=nullptr;sq_getstring(vm,-1,&detail);
    throw std::runtime_error(detail?detail:"Mod stage API failed");
}
void method(SQVM* vm,const char* name) {
    sq_pushroottable(vm);sq_pushstring(vm,"KinokoMods",-1);check(vm,sq_get(vm,-2));
    sq_pushstring(vm,name,-1);check(vm,sq_get(vm,-2));sq_push(vm,-2);
}
}
bool list_stages(SQVM* vm,std::vector<StageChoice>& stages,std::string& error) {
    stages.clear();error.clear();
    if(!vm){error="Missing script VM";return false;}
    StackGuard stack{vm,sq_gettop(vm)};
    try {
        method(vm,"List");sq_pushstring(vm,"stage",-1);check(vm,kinoko_sq_call(vm,2,SQTrue,SQFalse));
        const auto array=sq_gettop(vm),count=sq_getsize(vm,array);
        if(sq_gettype(vm,array)!=OT_ARRAY)throw std::runtime_error("Invalid stage list");
        for(SQInteger i=0;i<count;++i) {
            sq_pushinteger(vm,i);check(vm,sq_get(vm,array));const SQChar* id=nullptr;
            check(vm,sq_getstring(vm,-1,&id));StageChoice choice{id,{}};sq_pop(vm,1);
            const auto before=sq_gettop(vm);
            method(vm,"Get");sq_pushstring(vm,"stage",-1);sq_pushstring(vm,choice.id.c_str(),-1);
            check(vm,kinoko_sq_call(vm,3,SQTrue,SQFalse));sq_pushstring(vm,"name",-1);check(vm,sq_get(vm,-2));
            const SQChar* name=nullptr;check(vm,sq_getstring(vm,-1,&name));choice.name=name;
            stages.push_back(std::move(choice));sq_settop(vm,before);
        }
        return true;
    }catch(const std::exception& e){error=e.what();stages.clear();return false;}
}
bool launch_stage(SQVM* vm,const std::string& id,std::string& error) {
    error.clear();if(!vm){error="Missing script VM";return false;}
    StackGuard stack{vm,sq_gettop(vm)};
    try {
        method(vm,"Create");sq_pushstring(vm,"stage",-1);sq_pushstring(vm,id.c_str(),-1);sq_newarray(vm,0);
        check(vm,kinoko_sq_call(vm,4,SQFalse,SQFalse));return true;
    }catch(const std::exception& e){error="Stage "+id+": "+e.what();return false;}
}
bool select_startup_stage(SQVM* vm,SDL_Window* window,const char* selection,std::string& error) {
    error.clear();if(!selection || !*selection)return true;
    if(std::strcmp(selection,"@choose")!=0)return launch_stage(vm,selection,error);
    std::vector<StageChoice> stages;if(!list_stages(vm,stages,error))return false;
    if(stages.empty()){error="No stages registered by enabled Mods";return false;}
    size_t page=0;constexpr size_t per_page=6;
    for(;;) {
        std::vector<std::string> labels;
        const auto end=std::min(stages.size(),page+per_page);
        for(size_t i=page;i<end;++i)labels.push_back(stages[i].name+" ["+stages[i].id+"]");
        std::vector<SDL_MessageBoxButtonData> buttons;
        for(size_t i=0;i<labels.size();++i)buttons.push_back({0,static_cast<int>(i+1),labels[i].c_str()});
        buttons.push_back({SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,0,"Return to title"});
        if(page)buttons.push_back({0,-2,"Previous"});
        if(end<stages.size())buttons.push_back({0,-3,"Next"});
        SDL_MessageBoxData data{};data.flags=SDL_MESSAGEBOX_INFORMATION;data.window=window;
        data.title="Mod stages";data.message="Choose a registered stage";
        data.numbuttons=static_cast<int>(buttons.size());data.buttons=buttons.data();
        int selected=0;
        if(!SDL_ShowMessageBox(&data,&selected)){error=SDL_GetError();return false;}
        if(selected==0 || selected==-1)return true;
        if(selected==-2 && page){page-=per_page;continue;}
        if(selected==-3 && end<stages.size()){page+=per_page;continue;}
        if(selected>0 && static_cast<size_t>(selected)<=labels.size())
            return launch_stage(vm,stages[page+selected-1].id,error);
        error="Invalid stage selection";return false;
    }
}
bool run_scripts(SQVM* vm,std::string& error) {
    error.clear();if(entrypoints().empty())return true;
    try {
        if(!vm)throw std::runtime_error("Mod scripts require an initialized VM");
        execute(vm,registry,"Mod content API v1");
        for(const auto& entry:entrypoints()) {
            auto opened=open(entry.path.c_str());
            if(!opened.matched || !opened.file)throw std::runtime_error("Cannot read entrypoint: "+entry.path);
            std::unique_ptr<KinokoFile,decltype(&kinoko_file_close)> file(opened.file,kinoko_file_close);
            const auto size=kinoko_file_size(file.get());
            if(size<0 || size>64*1024*1024)throw std::runtime_error("Invalid entrypoint size");
            std::string source(static_cast<size_t>(size),'\0');uint32_t read=0;
            if(!kinoko_file_read(file.get(),source.data(),static_cast<uint32_t>(size),&read) || read!=size || source.find('\0')!=std::string::npos)
                throw std::runtime_error("Entrypoint must be plain UTF-8 Squirrel source: "+entry.path);
            if(source.compare(0,3,"\xef\xbb\xbf")==0)source.erase(0,3);
            execute(vm,"local mod = KinokoMods.For(\""+entry.id+"\");\n"+source,entry.path.c_str());
        }
        return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
}
