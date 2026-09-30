#pragma once
#include <cstdint>
#include <istream>
#include <stdexcept>
#include <vector>
namespace kinoko::tas {
// Little-endian input intentions only; checkpoints are produced by the game.
struct EditPlan {
    uint32_t first=0,source_count=0;
    std::vector<uint32_t> masks;
    void read(std::istream& in) {
        char magic[8];in.read(magic,8);
        const std::string version(magic,8);
        if(!in || (version!="KTASED01" && version!="KTASED02"))throw std::runtime_error("Invalid TAS edit plan header");
        auto word=[&](){uint32_t n=0;for(int i=0;i<4;++i){int b=in.get();if(b<0)throw std::runtime_error("Truncated TAS edit plan");n|=uint32_t(b)<<(8*i);}return n;};
        const auto size=word();first=word();source_count=version=="KTASED02"?word():size;
        if(!size || size>1000000 || !source_count || source_count>1000000 || first>=size || first>source_count)throw std::runtime_error("Invalid TAS edit plan range");
        masks.clear();masks.reserve(size);
        for(uint32_t i=0;i<size;++i){auto m=word();if(m>>19)throw std::runtime_error("Invalid TAS action mask");masks.push_back(m);}
        if(in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Trailing TAS edit data");
    }
};
}
