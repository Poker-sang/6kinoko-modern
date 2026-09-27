#include "kinoko/audio_output.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace kinoko::audio {
PcmRing::PcmRing(std::size_t bytes, std::size_t frame, std::uint8_t silence)
    : bytes_(bytes,silence), frame_bytes_(frame) {}
bool PcmRing::valid() const noexcept {
    return frame_bytes_ && !bytes_.empty() && bytes_.size()%frame_bytes_ == 0;
}
bool PcmRing::write(std::size_t offset,const void* data,std::size_t count) {
    if (!valid() || !data || offset>=bytes_.size() || count>bytes_.size() ||
        offset%frame_bytes_ || count%frame_bytes_) return false;
    const auto first=std::min(count,bytes_.size()-offset);
    std::memcpy(bytes_.data()+offset,data,first);
    std::memcpy(bytes_.data(),static_cast<const std::uint8_t*>(data)+first,count-first);
    return true;
}
std::size_t PcmRing::read(void* destination,std::size_t count) {
    if (!valid() || !destination || !running_) return 0;
    count-=count%frame_bytes_;
    std::size_t done=0;
    while(done<count) {
        if(cursor_==bytes_.size()) {
            if(!looping_) break;
            cursor_=0;
        }
        const auto take=std::min(count-done,bytes_.size()-cursor_);
        std::memcpy(static_cast<std::uint8_t*>(destination)+done,bytes_.data()+cursor_,take);
        cursor_+=take;done+=take;
    }
    if(looping_ && cursor_==bytes_.size()) cursor_=0;
    return done;
}
bool PcmRing::seek(std::size_t offset) {
    if(!valid() || offset>=bytes_.size() || offset%frame_bytes_) return false;
    cursor_=offset;return true;
}
float amplitude_from_db(std::int32_t db) noexcept {
    if(db<=-10000) return 0.0f;
    if(db>=0) return 1.0f;
    return static_cast<float>(std::pow(10.0,static_cast<double>(db)/2000.0));
}
}
