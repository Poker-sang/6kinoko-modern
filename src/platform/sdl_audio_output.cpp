#include "kinoko/audio_output.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <climits>
#include <mutex>
#include <new>
namespace kinoko::audio {
namespace {
struct DeviceState {
    SDL_AudioDeviceID id{};
    bool initialized{};
    ~DeviceState() {
        if(id) SDL_CloseAudioDevice(id);
        if(initialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
};
std::mutex devices_mutex;
std::vector<std::weak_ptr<DeviceState>> devices;
bool output_muted=false;
struct StreamLock {
    SDL_AudioStream* stream;
    explicit StreamLock(SDL_AudioStream* s) : stream(SDL_LockAudioStream(s) ? s : nullptr) {}
    ~StreamLock() { if(stream) SDL_UnlockAudioStream(stream); }
    explicit operator bool() const noexcept { return stream!=nullptr; }
};
class SdlBuffer final : public OutputBuffer {
    std::shared_ptr<DeviceState> device_;
    SDL_AudioStream* stream_{};
    PcmRing ring_;
    bool flushed_{}, failed_{};
    // Serializes public control calls, never acquired by the audio callback.
    // Bind/unbind take SDL's device lock: never call them under StreamLock.
    mutable std::mutex control_;
    static void SDLCALL feed(void* userdata,SDL_AudioStream* stream,int additional,int) {
        auto& self=*static_cast<SdlBuffer*>(userdata);
        if(additional<=0 || self.failed_ || !self.ring_.running()) return;
        std::array<std::uint8_t,8192> scratch;
        const auto frame=self.ring_.frame_bytes();
        const auto capacity=scratch.size()-scratch.size()%frame;
        std::size_t remaining=static_cast<std::size_t>(additional);
        remaining+=(frame-remaining%frame)%frame;
        while(remaining) {
            const auto got=self.ring_.read(scratch.data(),std::min(remaining,capacity));
            if(!got) break;
            if(!SDL_PutAudioStreamData(stream,scratch.data(),static_cast<int>(got))) {
                self.failed_=true;self.ring_.stop();return;
            }
            remaining-=got;
        }
        if(self.ring_.exhausted() && !self.flushed_) {
            self.flushed_=true;
            if(!SDL_FlushAudioStream(stream)) { self.failed_=true;self.ring_.stop(); }
        }
    }
public:
    SdlBuffer(std::shared_ptr<DeviceState> device,PcmFormat format,std::size_t bytes)
        : device_(std::move(device)),ring_(bytes,format.frame_bytes(),format.bits==8 ? 128 : 0) {
        if(!ring_.valid()) return;
        SDL_AudioSpec spec{};
        spec.freq=static_cast<int>(format.rate);spec.channels=format.channels;
        spec.format=format.bits==8 ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE;
        stream_=SDL_CreateAudioStream(&spec,&spec);
        if(stream_ && !SDL_SetAudioStreamGetCallback(stream_,feed,this)) {
            SDL_DestroyAudioStream(stream_);stream_=nullptr;
        }
    }
    ~SdlBuffer() override {
        // SDL destroys/unbinds under its locks and waits out an in-flight feed.
        // The shared device outlives this stream even if its public owner closed.
        if(stream_) SDL_DestroyAudioStream(stream_);
    }
    bool valid() const noexcept { return stream_!=nullptr; }
    bool write(std::size_t offset,const void* data,std::size_t bytes) override {
        StreamLock lock(stream_);return lock && ring_.write(offset,data,bytes);
    }
    bool play(bool looping) override {
        std::lock_guard<std::mutex> control(control_);
        {
            StreamLock lock(stream_);
            if(!lock || failed_) return false;
            ring_.play(looping);
        }
        if(SDL_GetAudioStreamDevice(stream_)) return true;
        if(SDL_BindAudioStream(device_->id,stream_)) return true;
        StreamLock lock(stream_);if(lock) ring_.stop();return false;
    }
    bool stop() override {
        std::lock_guard<std::mutex> control(control_);
        SDL_UnbindAudioStream(stream_);
        StreamLock lock(stream_);if(!lock) return false;
        ring_.stop();return true;
    }
    bool seek(std::size_t offset) override {
        std::lock_guard<std::mutex> control(control_);
        StreamLock lock(stream_);if(!lock) return false;
        // A one-shot can remain bound after draining. Repositioning it must
        // not restart playback before the caller explicitly calls play().
        const bool ended=ring_.exhausted() && SDL_GetAudioStreamAvailable(stream_)==0;
        if(!ring_.seek(offset)) return false;
        if(ended) ring_.stop();
        flushed_=failed_=false;
        return SDL_ClearAudioStream(stream_);
    }
    bool playing() const override {
        StreamLock lock(stream_);
        return lock && !failed_ && ring_.running() &&
            (!ring_.exhausted() || SDL_GetAudioStreamAvailable(stream_)>0);
    }
    std::size_t position() const override {
        StreamLock lock(stream_);return lock ? ring_.position() : 0;
    }
    void volume_db(std::int32_t db) override { SDL_SetAudioStreamGain(stream_,amplitude_from_db(db)); }
};
class SdlDevice final : public OutputDevice {
    std::shared_ptr<DeviceState> state_;
public:
    explicit SdlDevice(std::shared_ptr<DeviceState> state) : state_(std::move(state)) {}
    OutputBufferPtr create(PcmFormat format,std::size_t bytes) override {
        if(!format.rate || format.rate>INT_MAX || !format.channels || format.channels>8 ||
            (format.bits!=8 && format.bits!=16) || !bytes || bytes>INT_MAX || bytes%format.frame_bytes()) return {};
        try {
            auto buffer=std::make_shared<SdlBuffer>(state_,format,bytes);
            return buffer->valid() ? buffer : OutputBufferPtr{};
        } catch(const std::bad_alloc&) { SDL_SetError("Audio buffer allocation failed");return {}; }
    }
};
}
std::shared_ptr<OutputDevice> open_sdl_output() {
    try {
        auto state=std::make_shared<DeviceState>();
        if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) return {};
        state->initialized=true;
        SDL_AudioSpec spec{SDL_AUDIO_F32,2,44100};
        state->id=SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec);
        if(!state->id || !SDL_ResumeAudioDevice(state->id)) return {};
        {std::lock_guard<std::mutex> lock(devices_mutex);
        SDL_SetAudioDeviceGain(state->id,output_muted?0.f:1.f);devices.push_back(state);}
        return std::make_shared<SdlDevice>(std::move(state));
    } catch(const std::bad_alloc&) { SDL_SetError("Audio device allocation failed");return {}; }
}
void mute_output(bool muted) {
    std::lock_guard<std::mutex> lock(devices_mutex);output_muted=muted;
    for(auto it=devices.begin();it!=devices.end();) {
        if(auto device=it->lock()){SDL_SetAudioDeviceGain(device->id,muted?0.f:1.f);++it;}
        else it=devices.erase(it);
    }
}
}
