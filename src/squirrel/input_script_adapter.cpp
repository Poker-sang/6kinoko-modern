#include "sqpcheader.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"
#include "kinoko/input_script_adapter.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {
struct Patch {int instruction;const char* name;bool write;};
struct Rule {const char* source;const char* name;uint32_t hash;std::vector<Patch> patches;};
#include "input_adapter_rules.inc"
std::string string(const SQObjectPtr& o) {return sq_type(o)==OT_STRING?_stringval(o):"";}
uint32_t fingerprint(const SQFunctionProto* f) {
    uint32_t hash=2166136261u;
    auto byte=[&](uint8_t value){hash=(hash^value)*16777619u;};
    for(int i=0;i<f->_ninstructions;++i) {
        auto& op=f->_instructions[i];
        uint32_t words[]={static_cast<uint32_t>(op._arg1),op.op,op._arg0,op._arg2,op._arg3};
        for(auto word:words)for(int b=0;b<4;++b)byte(static_cast<uint8_t>(word>>(8*b)));
    }
    for(int i=0;i<f->_nliterals;++i)if(sq_type(f->_literals[i])==OT_STRING) {
        const auto value=string(f->_literals[i]);
        if(std::any_of(value.begin(),value.end(),[](unsigned char c){return c>=128;}))continue;
        for(unsigned char c:value)byte(c);
        byte(0);
    }
    return hash;
}
bool jump(unsigned char op) {
    return op==_OP_JMP || op==_OP_JNZ || op==_OP_JZ || op==_OP_AND || op==_OP_OR ||
        op==_OP_FOREACH || op==_OP_POSTFOREACH || op==_OP_PUSHTRAP;
}
SQObjectPtr adapt(HSQUIRRELVM vm,SQFunctionProto* old) {
    auto source=string(old->_sourcename);
    for(auto& c:source) {if(c=='\\')c='/';else if(c>='A' && c<='Z')c+='a'-'A';}
    const auto name=string(old->_name);
    const Rule* rule=nullptr;bool recognized=false;
    const auto hash=fingerprint(old);
    for(const auto& r:rules)if(source==r.source && name==r.name) {
        recognized=true;if(hash==r.hash){rule=&r;break;}
    }
    // Only closures actually using input need a known version. Arbitrary mod
    // scripts outside the original source paths use named actions directly.
    bool uses_input=false;
    for(int i=0;i<old->_nliterals;++i)uses_input|=string(old->_literals[i])=="input";
    if(recognized && uses_input && !rule)
        throw std::runtime_error("Unsupported original input script: "+source+" / "+name);
    std::vector<SQObjectPtr> children;
    bool changed=rule && !rule->patches.empty();
    for(int i=0;i<old->_nfunctions;++i) {
        auto child=adapt(vm,_funcproto(old->_functions[i]));
        changed|=_funcproto(child)!=_funcproto(old->_functions[i]);children.push_back(child);
    }
    if(!changed)return SQObjectPtr(old);
    std::vector<SQObjectPtr> literals(old->_literals,old->_literals+old->_nliterals);
    auto literal=[&](const char* value) {
        for(size_t i=0;i<literals.size();++i)if(string(literals[i])==value)return static_cast<int>(i);
        literals.emplace_back(SQString::Create(_ss(vm),value));return static_cast<int>(literals.size()-1);
    };
    std::vector<SQInstruction> code;
    std::vector<int> starts(old->_ninstructions+1),originals(old->_ninstructions);
    for(int i=0;i<old->_ninstructions;++i) {
        starts[i]=static_cast<int>(code.size());
        auto op=old->_instructions[i];bool wrote=false;
        if(rule)for(const auto& p:rule->patches)if(p.instruction==i) {
            if(!p.write)op._arg1=literal(p.name);
            else {
                // Mirror a script assignment without destroying receiver/value.
                // Original SET still runs last with its original result semantics.
                code.emplace_back(_OP_LOAD,op._arg2,literal(p.name));
                code.emplace_back(_OP_SET,op._arg3,op._arg1,op._arg2,op._arg3);
                wrote=true;
            }
        }
        if(wrote) {
            int load=i-2;if(old->_instructions[i-1].op==_OP_NEG)--load;
            code.push_back(old->_instructions[load]);
        }
        originals[i]=static_cast<int>(code.size());code.push_back(op);
    }
    starts[old->_ninstructions]=static_cast<int>(code.size());
    for(int i=0;i<old->_ninstructions;++i)if(jump(old->_instructions[i].op)) {
        const int adjust=old->_instructions[i].op==_OP_POSTFOREACH?0:1;
        const int target=i+adjust+old->_instructions[i]._arg1;
        if(target<0 || target>old->_ninstructions)throw std::runtime_error("Invalid input adapter jump");
        code[originals[i]]._arg1=starts[target]-originals[i]-adjust;
    }
    auto* f=SQFunctionProto::Create(static_cast<SQInteger>(code.size()),static_cast<SQInteger>(literals.size()),
        old->_nparameters,old->_nfunctions,old->_noutervalues,old->_nlineinfos,old->_nlocalvarinfos,old->_ndefaultparams);
    SQObjectPtr owner(f);
    f->_sourcename=old->_sourcename;f->_name=old->_name;f->_stacksize=old->_stacksize;
    f->_bgenerator=old->_bgenerator;f->_varparams=old->_varparams;
    std::copy(code.begin(),code.end(),f->_instructions);
    std::copy(literals.begin(),literals.end(),f->_literals);
    std::copy(children.begin(),children.end(),f->_functions);
    std::copy_n(old->_parameters,old->_nparameters,f->_parameters);
    std::copy_n(old->_outervalues,old->_noutervalues,f->_outervalues);
    std::copy_n(old->_defaultparams,old->_ndefaultparams,f->_defaultparams);
    for(int i=0;i<old->_nlineinfos;++i) {
        f->_lineinfos[i]=old->_lineinfos[i];f->_lineinfos[i]._op=starts.at(old->_lineinfos[i]._op);
    }
    for(int i=0;i<old->_nlocalvarinfos;++i) {
        f->_localvarinfos[i]=old->_localvarinfos[i];
        f->_localvarinfos[i]._start_op=starts.at(old->_localvarinfos[i]._start_op);
        f->_localvarinfos[i]._end_op=starts.at(old->_localvarinfos[i]._end_op);
    }
    return owner;
}
}
bool kinoko_adapt_input_script(HSQUIRRELVM vm,std::string& error) {
    HSQOBJECT value;sq_resetobject(&value);
    if(SQ_FAILED(sq_getstackobj(vm,-1,&value)) || value._type!=OT_CLOSURE) {error="Expected script closure";return false;}
    try {
        auto* closure=_closure(value);
        closure->_function=adapt(vm,_funcproto(closure->_function));
        error.clear();return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
