#include "kinoko/mesh_model.hpp"
#include <cstring>
#include <stdexcept>
using namespace kinoko::mesh;
struct Bytes {
    std::vector<std::uint8_t> data;
    std::size_t cursor=0;
    template<class T> void put(const T& value) {
        const auto* bytes=reinterpret_cast<const std::uint8_t*>(&value);
        data.insert(data.end(),bytes,bytes+sizeof(value));
    }
    void count(std::uint32_t value){put(value);}
    void string(const char* value){count(static_cast<std::uint32_t>(std::strlen(value)));data.insert(data.end(),value,value+std::strlen(value));}
    static bool read(void* context,void* output,std::uint32_t size) {
        auto& self=*static_cast<Bytes*>(context);
        if(size>self.data.size()-self.cursor) return false;
        std::memcpy(output,self.data.data()+self.cursor,size);self.cursor+=size;return true;
    }
    Input input(){cursor=0;return {this,read};}
    void node_tail(const char* name,std::uint32_t version,std::uint32_t children) {
        string(name);Matrix matrix{};matrix[0]=matrix[5]=matrix[10]=matrix[15]=1;put(matrix);
        if(version>=12)count(7);if(version>=14)count(9);count(children);
    }
};
int main() {
    if(decode_model({}) || decode_material({})) return 1;
    for(auto version:{10u,12u,14u,20u}) {
        Bytes bytes;bytes.count(version);bytes.node_tail("root",version,1);
        bytes.count(static_cast<std::uint32_t>(NodeType::node));bytes.node_tail("child",version,0);
        auto model=decode_model(bytes.input());
        if(!model || model->name!="root" || model->children.size()!=1 || model->children[0]->name!="child" ||
            model->reference_group!=(version>=12?7:0) || model->flags!=(version>=14?9:0) || bytes.cursor!=bytes.data.size()) return 2;
        bytes.data.pop_back();bool failed=false;
        try{decode_model(bytes.input());}catch(const std::runtime_error&){failed=true;}
        if(!failed)return 3;
    }
    for(auto count:{3u,65536u}) {
        Bytes bytes;bytes.count(14);bytes.node_tail("root",14,1);bytes.count(static_cast<std::uint32_t>(NodeType::mesh));
        bytes.put(std::uint8_t{1});Vector3 zero{};bytes.put(zero);bytes.put(zero);bytes.put(zero);bytes.put(0.0f);
        bytes.count(count);bytes.data.resize(bytes.data.size()+count*(count>65535?4:2));
        for(int i=0;i<7;++i)bytes.count(0);
        bytes.node_tail("mesh",14,0);
        auto model=decode_model(bytes.input());
        if(!model || model->children.size()!=1 || !model->children[0]->geometry ||
            model->children[0]->geometry->indices.size()!=count*(count>65535?4:2)) return 4;
    }
    Bytes old;old.count(9);if(decode_model(old.input()))return 5;
    Bytes material;material.count(13);
    for(auto name:{"a","b","c","d"})material.string(name);
    std::array<std::uint8_t,16> colors{};colors[0]=255;colors[1]=128;material.put(colors);
    auto result=decode_material(material.input());
    if(!result || result->names[3]!="d" || result->colors[0]!=1.0f || result->colors[1]!=static_cast<float>(128.0/255.0)) return 6;
    Bytes latest;latest.count(15);for(auto name:{"a","b","c","d","e"})latest.string(name);
    std::array<float,16> floats{};floats[15]=0.75f;latest.put(floats);
    result=decode_material(latest.input());if(!result || result->names[4]!="e" || result->colors[15]!=0.75f)return 7;
    Bytes invalid;invalid.count(16);if(decode_material(invalid.input()))return 8;
    return 0;
}
