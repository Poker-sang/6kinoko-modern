#include "kinoko/mod_scripts.hpp"
#include "kinoko/mod_resources.hpp"
#include "kinoko/squirrel_source_runtime.h"
#include <squirrel.h>
#include <cstring>
#include <memory>
#include <stdexcept>

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
