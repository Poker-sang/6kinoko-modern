#include "kinoko/mod_resources.hpp"
#include <array>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <map>
#include <stdexcept>
namespace kinoko::mods {
namespace {
constexpr uint32_t constants[]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
uint32_t rotate(uint32_t x,int n){return (x>>n)|(x<<(32-n));}
std::vector<Info> modules;
std::vector<Entry> scripts;
struct Asset {std::filesystem::path path;std::string hash;};
std::map<std::string,Asset> assets;
std::string session_identity;
std::string script_error;
bool hex(const std::string& s){return s.size()==64 && s.find_first_not_of("0123456789abcdef")==s.npos;}
bool token(const std::string& s){return !s.empty() && s.size()<128 && s.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789._-")==s.npos;}
std::string normalize(std::string s) {
    for(auto& c:s){if(c=='\\')c='/';if(c>='A'&&c<='Z')c=char(c-'A'+'a');}
    if(s.rfind("./",0)==0)s.erase(0,2);
    if(s.rfind("data/",0)!=0 || s.size()>512)return {};
    std::istringstream parts(s);std::string part;
    while(std::getline(parts,part,'/'))if(part.empty() || part=="." || part==".." || part.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789._- ")!=part.npos || part.back()==' ' || part.back()=='.')return {};
    if(s.back()=='/')return {};
    return s;
}
bool contained(const std::filesystem::path& root,const std::filesystem::path& path) {
    auto base=std::filesystem::canonical(root),resolved=std::filesystem::canonical(path);
    auto r=resolved.lexically_relative(base);
    return !r.empty() && !r.is_absolute() && *r.begin()!="..";
}
KinokoFile* verified(const Asset& asset) {
    auto* file=kinoko_file_open_utf8(asset.path.u8string().c_str(),KINOKO_FILE_READ);
    if(!file)return nullptr;
    const auto size=kinoko_file_size(file);
    if(size<0 || size>64*1024*1024){kinoko_file_close(file);return nullptr;}
    try {
        std::string bytes(static_cast<size_t>(size),'\0');uint32_t count=0;
        if(!kinoko_file_read(file,bytes.data(),static_cast<uint32_t>(size),&count) || count!=size || sha256(bytes)!=asset.hash || kinoko_file_seek(file,0,KINOKO_FILE_BEGIN)<0) {
            kinoko_file_close(file);return nullptr;
        }
        return file;
    }catch(...){kinoko_file_close(file);return nullptr;}
}
}
std::string sha256(std::string_view bytes) {
    std::vector<uint8_t> data(bytes.begin(),bytes.end());const uint64_t bits=uint64_t(data.size())*8;
    data.push_back(0x80);while(data.size()%64!=56)data.push_back(0);
    for(int i=7;i>=0;--i)data.push_back(uint8_t(bits>>(i*8)));
    std::array<uint32_t,8> h={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    for(size_t at=0;at<data.size();at+=64) {
        uint32_t w[64]{};
        for(int i=0;i<16;++i)for(int j=0;j<4;++j)w[i]=(w[i]<<8)|data[at+i*4+j];
        for(int i=16;i<64;++i)w[i]=w[i-16]+(rotate(w[i-15],7)^rotate(w[i-15],18)^(w[i-15]>>3))+w[i-7]+(rotate(w[i-2],17)^rotate(w[i-2],19)^(w[i-2]>>10));
        auto a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],v=h[7];
        for(int i=0;i<64;++i){const auto t1=v+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^(~e&g))+constants[i]+w[i];const auto t2=(rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c));v=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=v;
    }
    const char* digits="0123456789abcdef";std::string out;
    for(auto n:h)for(int i=7;i>=0;--i)out+=digits[(n>>(i*4))&15];return out;
}
void clear(){assets.clear();modules.clear();scripts.clear();session_identity.clear();script_error.clear();}
std::string& startup_error(){return script_error;}
const std::vector<Entry>& entrypoints(){return scripts;}
const std::vector<Info>& active(){return modules;}
const std::string& identity(){return session_identity;}
bool load(const std::filesystem::path& catalog,std::string& error) {
    clear();error.clear();if(catalog.empty())return true;
    try {
        if(std::filesystem::file_size(catalog)>4*1024*1024)throw std::runtime_error("Mod catalog too large");
        std::ifstream in(catalog,std::ios::binary);std::string bytes{std::istreambuf_iterator<char>(in),{}};
        std::istringstream lines(bytes);std::string line;
        if(!std::getline(lines,line) || (line!="KINOKOMODS1" && line!="KINOKOMODS2"))throw std::runtime_error("Invalid Mod catalog header");
        const bool version2=line=="KINOKOMODS2";
        auto root=std::filesystem::absolute(catalog).parent_path()/"assets";
        bool file_section=false;
        while(std::getline(lines,line)) {
            std::vector<std::string> fields;std::istringstream row(line);std::string field;
            while(std::getline(row,field,'\t'))fields.push_back(field);
            if(fields.size()==4 && fields[0]=="M" && !file_section && scripts.empty() && token(fields[1]) && token(fields[2]) && hex(fields[3])) {
                if(std::any_of(modules.begin(),modules.end(),[&](const Info& m){return m.id==fields[1];}))throw std::runtime_error("Duplicate Mod id");
                modules.push_back({fields[1],fields[2],fields[3]});
            } else if(version2 && fields.size()==3 && fields[0]=="E" && !file_section && token(fields[1]) && normalize(fields[2])==fields[2] && fields[2].rfind("data/custom/"+fields[1]+"/",0)==0 && fields[2].size()>4 && fields[2].substr(fields[2].size()-4)==".nut") {
                if(std::any_of(scripts.begin(),scripts.end(),[&](const Entry& e){return e.id==fields[1];}))throw std::runtime_error("Duplicate entrypoint");
                scripts.push_back({fields[1],fields[2]});
            } else if(fields.size()==3 && fields[0]=="F" && normalize(fields[1])==fields[1] && !fields[1].empty() && hex(fields[2])) {
                file_section=true;const auto path=root/std::filesystem::u8path(fields[1]);
                if(!contained(root,path))throw std::runtime_error("Mod resource escapes snapshot");
                Asset asset{path,fields[2]};
                if(!assets.emplace(fields[1],asset).second)throw std::runtime_error("Duplicate Mod resource");
                auto* file=verified(asset);if(!file)throw std::runtime_error("Mod resource hash/read mismatch: "+fields[1]);kinoko_file_close(file);
            }else throw std::runtime_error("Invalid Mod catalog row");
        }
        if(modules.empty() || assets.empty())throw std::runtime_error("Empty Mod catalog");
        size_t next=0;
        for(const auto& entry:scripts) {
            while(next<modules.size() && modules[next].id!=entry.id)++next;
            if(next==modules.size() || !assets.count(entry.path))throw std::runtime_error("Invalid entrypoint order/resource");
            ++next;
        }
        session_identity=sha256(bytes);return true;
    }catch(const std::exception& e){error=e.what();clear();return false;}
}
bool contains(const char* path){return path && assets.count(normalize(path))!=0;}
Lookup open(const char* path) noexcept {
    if(!path || assets.empty())return {};
    try {const auto found=assets.find(normalize(path));if(found==assets.end())return {};return {true,verified(found->second)};}
    catch(...){return {true,nullptr};}
}
}
