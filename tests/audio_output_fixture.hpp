#pragma once
#include "kinoko/audio_output.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Borrowed stack fixture: production uses ordinary shared owners from SDL.
class AudioTestBuffer final : public kinoko::audio::OutputBuffer {
public:
    unsigned refs=1;
    int releases=0,stops=0,plays=0,locks=0,unlocks=0;
    std::uint32_t status=0,cursor=0;
    std::int32_t volume=0;
    bool fail_lock=false;
    std::vector<unsigned char> bytes=std::vector<unsigned char>(16);
    kinoko::audio::OutputBufferPtr borrow(bool add_ref=false) {
        if(add_ref) ++refs;
        return {this,[](kinoko::audio::OutputBuffer* base) {
            auto* self=static_cast<AudioTestBuffer*>(base);++self->releases;--self->refs;
        }};
    }
    bool write(std::size_t offset,const void* data,std::size_t size) override {
        ++locks;
        if(fail_lock || offset>=bytes.size() || size>bytes.size()) return false;
        const auto first=std::min(size,bytes.size()-offset);
        std::memcpy(bytes.data()+offset,data,first);
        std::memcpy(bytes.data(),static_cast<const unsigned char*>(data)+first,size-first);
        ++unlocks;return true;
    }
    bool play(bool) override { ++plays;status=1;return true; }
    bool stop() override { ++stops;status=0;return true; }
    bool seek(std::size_t value) override { cursor=static_cast<std::uint32_t>(value);return true; }
    bool playing() const override { return status!=0; }
    std::size_t position() const override { return cursor; }
    void volume_db(std::int32_t value) override { volume=value; }
};
