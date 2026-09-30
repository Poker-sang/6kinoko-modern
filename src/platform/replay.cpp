#include "kinoko/replay.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace kinoko::replay {
namespace {
constexpr char magic[8]={'K','I','N','O','R','P','L','1'};
constexpr size_t payload_size=191;
constexpr size_t header_size=8+4+4+64;
using Bytes=std::vector<uint8_t>;
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void put(Bytes& out,uint64_t n,int size) {for(int i=0;i<size;++i)out.push_back(static_cast<uint8_t>(n>>(8*i)));}
uint64_t get(const Bytes& in,size_t& at,int size) {
    require(at+size<=in.size(),"Truncated replay field");uint64_t n=0;
    for(int i=0;i<size;++i)n|=uint64_t(in[at++])<<(8*i);return n;
}
uint64_t hash(const Bytes& bytes,uint64_t h=14695981039346656037ull) {for(auto b:bytes)h=(h^b)*1099511628211ull;return h;}
void write(std::ostream& out,const Bytes& bytes) {
    out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());require(bool(out),"Cannot write replay");
}
Bytes read(std::istream& in,size_t size) {
    Bytes bytes(size);in.read(reinterpret_cast<char*>(bytes.data()),size);require(bool(in),"Truncated replay (recording was not finalized)");return bytes;
}
Bytes encode(const Frame& f) {
    Bytes b;
    for(auto n:f.actions.held)put(b,static_cast<uint32_t>(n),4);
    for(auto n:f.actions.released)put(b,n,1);
    put(b,static_cast<uint32_t>(f.legacy.x),4);put(b,static_cast<uint32_t>(f.legacy.y),4);
    for(auto n:f.legacy.buttons)put(b,static_cast<uint32_t>(n),4);
    for(auto n:f.legacy.released)put(b,n,1);
    for(auto n:f.legacy.digits)put(b,static_cast<uint32_t>(n),4);
    put(b,f.clock,4);put(b,f.random_before,4);put(b,f.random_after,4);put(b,f.checkpoint,8);
    require(b.size()==payload_size,"Replay action schema mismatch");return b;
}
Frame decode(const Bytes& b) {
    Frame f;size_t at=0;
    auto i32=[&] {uint32_t n=static_cast<uint32_t>(get(b,at,4));int32_t result;std::memcpy(&result,&n,4);return result;};
    for(auto& n:f.actions.held)n=i32();
    for(auto& n:f.actions.released){n=static_cast<uint8_t>(get(b,at,1));require(n<=1,"Invalid action release flag");}
    f.legacy.x=i32();f.legacy.y=i32();for(auto& n:f.legacy.buttons)n=i32();
    for(auto& n:f.legacy.released){n=static_cast<uint8_t>(get(b,at,1));require(n<=1,"Invalid legacy release flag");}
    for(auto& n:f.legacy.digits)n=i32();
    f.clock=static_cast<uint32_t>(get(b,at,4));f.random_before=static_cast<uint32_t>(get(b,at,4));
    f.random_after=static_cast<uint32_t>(get(b,at,4));f.checkpoint=get(b,at,8);return f;
}
void identity_ok(const std::string& id) {
    require(id.size()==64 && id.find_first_not_of("0123456789abcdefABCDEF")==id.npos,"Invalid replay session identity");
}
void header(std::istream& in,const std::string& identity) {
    auto b=read(in,header_size);size_t at=8;
    require(std::memcmp(b.data(),magic,8)==0,"Not a Kinoko replay");
    require(get(b,at,4)==version && get(b,at,4)==input::Count,"Unsupported replay version/action schema");
    require(std::string(b.begin()+at,b.end())==identity,"Replay build/data/save identity differs");
}
Bytes record(std::istream& in,uint64_t index) {
    auto b=read(in,8+payload_size);size_t at=0;
    require(get(b,at,8)==index,"Replay frame sequence differs");
    auto h=read(in,8);at=0;require(get(h,at,8)==hash(b),"Replay frame checksum differs");return b;
}
}
Writer::Writer(std::ostream& out,const std::string& identity):out_(out) {
    identity_ok(identity);Bytes b(magic,magic+8);put(b,version,4);put(b,input::Count,4);
    b.insert(b.end(),identity.begin(),identity.end());write(out_,b);
}
void Writer::append(const Frame& f) {
    require(!finished_ && count_<maximum_frames,"Replay recording limit reached");
    Bytes b;put(b,count_,8);auto payload=encode(f);b.insert(b.end(),payload.begin(),payload.end());
    write(out_,Bytes{1});write(out_,b);Bytes digest;put(digest,hash(b),8);write(out_,digest);
    chain_=hash(b,chain_);++count_;
}
void Writer::finish() {
    require(!finished_,"Replay already finalized");Bytes footer{0};put(footer,count_,8);put(footer,chain_,8);
    write(out_,footer);out_.flush();require(bool(out_),"Cannot finalize replay");finished_=true;
}
void Writer::snapshot(std::istream& prefix,std::ostream& destination) {
    require(!finished_,"Replay already finalized");out_.flush();require(bool(out_),"Cannot flush replay snapshot");
    uint64_t remaining=80+count_*208;
    while(remaining){auto block=read(prefix,size_t(std::min<uint64_t>(remaining,65536)));write(destination,block);remaining-=block.size();}
    Bytes footer{0};put(footer,count_,8);put(footer,chain_,8);write(destination,footer);
    destination.flush();require(bool(destination),"Cannot finalize replay snapshot");
}
Reader::Reader(std::istream& in,const std::string& identity):in_(in) {
    identity_ok(identity);header(in_,identity);uint64_t chain=14695981039346656037ull;
    for(;;) {
        const auto tag=read(in_,1)[0];
        if(tag==0) {
            auto footer=read(in_,16);size_t at=0;
            require(get(footer,at,8)==total_ && get(footer,at,8)==chain,"Replay footer differs");
            require(in_.peek()==std::char_traits<char>::eof(),"Trailing replay data");break;
        }
        require(tag==1 && total_<maximum_frames,"Invalid replay frame tag/count");
        auto b=record(in_,total_++);decode(Bytes(b.begin()+8,b.end()));chain=hash(b,chain);
    }
    in_.clear();in_.seekg(header_size);require(bool(in_),"Replay stream is not seekable");
}
bool Reader::next(Frame& f) {
    if(index_==total_)return false;
    require(read(in_,1)[0]==1,"Replay changed while playing");auto b=record(in_,index_++);
    f=decode(Bytes(b.begin()+8,b.end()));return true;
}
bool same_checkpoint(const Frame& a,const Frame& b) {
    return a.clock==b.clock && a.random_before==b.random_before && a.random_after==b.random_after && a.checkpoint==b.checkpoint;
}
}
