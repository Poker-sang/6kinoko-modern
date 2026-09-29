#include "kinoko/input_actions.hpp"
#include <algorithm>
#include <limits>
#include <sstream>

namespace kinoko::input {
Bindings classic_bindings() {
    Bindings result;
    for (int i=0;i<Count;++i) result[i].push_back({Source::Legacy,i});
    return result;
}
int32_t legacy_count(Action a,const Sample& s) {
    switch(a) {
    case Left: case MenuLeft: return s.x<0 ? -s.x:0;
    case Right: case MenuRight: return s.x>0 ? s.x:0;
    case Up: case Door: case PipeUp: case MenuUp: return s.y<0 ? -s.y:0;
    case Down: case PipeDown: case MenuDown: return s.y>0 ? s.y:0;
    case Jump: case Confirm: return s.buttons[0];
    case Attack: case Run: case Carry: return s.buttons[3];
    case MenuAcceptAlt: case Pause: return s.buttons[1];
    case UseItem: return s.buttons[2];
    default:return 0;
    }
}
void advance(const Bindings& bindings,const Sample& sample,Frame& frame) {
    for(int a=0;a<Count;++a) {
        bool down=false;
        for(const auto& b:bindings[a]) {
            if(b.source==Source::Legacy) down|=legacy_count(static_cast<Action>(b.index),sample)>0;
            if(b.source==Source::Scan) down|=sample.keys[b.index];
            if(b.source==Source::Pad) down|=sample.pad[b.index];
        }
        const auto previous=frame.held[a];
        auto value=down ? (previous==std::numeric_limits<int32_t>::max()?previous:previous+1):0;
        if(bindings[a].size()==1 && bindings[a][0].source==Source::Legacy)
            value=legacy_count(static_cast<Action>(bindings[a][0].index),sample);
        frame.held[a]=value;
        frame.released[a]=previous>0 && value==0;
    }
}
namespace {
std::string trim(std::string s) {
    auto first=s.find_first_not_of(" \t\r\n");
    return first==s.npos?std::string():s.substr(first,s.find_last_not_of(" \t\r\n")-first+1);
}
int action(const std::string& s) {
    // Compatibility with the first split preset; canonical name is explicit.
    if(s=="menuBack")return MenuAcceptAlt;
    for(int i=0;i<Count;++i) if(s==names[i]) return i;
    return -1;
}
bool number(const std::string& s,int limit,int& n) {
    if(s.empty() || s.find_first_not_of("0123456789")!=s.npos || s.size()>3) return false;
    n=std::stoi(s);return n<limit;
}
}
bool parse_bindings(std::string_view text,Bindings& destination,std::string& error,
                    int (*key_scan)(const std::string&)) {
    Bindings result=classic_bindings();
    std::array<bool,Count> seen{};
    bool version=false; int line=0;
    std::istringstream input{std::string(text)};
    std::string row;
    auto fail=[&](const char* message) {error="line "+std::to_string(line)+": "+message;return false;};
    while(std::getline(input,row)) {
        ++line; row=trim(row.substr(0,row.find('#')));if(row.empty())continue;
        const auto equal=row.find('=');if(equal==row.npos)return fail("expected name = binding");
        const auto name=trim(row.substr(0,equal)), value=trim(row.substr(equal+1));
        if(name=="version") {if(version || value!="1")return fail("expected version = 1 once");version=true;continue;}
        const int a=action(name);if(a<0 || seen[a])return fail("unknown or duplicate action");
        seen[a]=true;result[a].clear();
        if(value=="none")continue;
        if(value.empty() || value.back()==',')return fail("empty binding");
        std::istringstream tokens(value);std::string token;
        while(std::getline(tokens,token,',')) {
            token=trim(token);int index=-1;
            if(token=="legacy")result[a].push_back({Source::Legacy,a});
            else if(token.rfind("key:",0)==0 && key_scan && (index=key_scan(trim(token.substr(4))))>=0 && index<256)
                result[a].push_back({Source::Scan,index});
            else if(token.rfind("scan:",0)==0 && number(token.substr(5),256,index))result[a].push_back({Source::Scan,index});
            else if(token.rfind("pad:",0)==0 && number(token.substr(4),32,index))result[a].push_back({Source::Pad,index});
            else return fail("invalid binding (legacy, key:name, scan:0..255, pad:0..31, or none)");
        }
    }
    if(!version)return fail("missing version = 1");
    destination=std::move(result);error.clear();return true;
}
}
