#include "kinoko/audio_output.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"audio output %d: %s (%s)\n",__LINE__,#x,SDL_GetError());return 1; } } while(0)
int main() {
    using namespace kinoko::audio;
    PcmRing ring(16,2,0);
    const std::array<unsigned char,8> samples{1,2,3,4,5,6,7,8};
    CHECK(ring.write(12,samples.data(),8));
    CHECK(!ring.write(1,samples.data(),8));
    CHECK(!ring.write(0,samples.data(),17));
    CHECK(ring.seek(12));ring.play(true);
    std::array<unsigned char,8> out{};
    CHECK(ring.read(out.data(),8)==8 && out==samples && ring.position()==4);
    ring.stop();CHECK(ring.read(out.data(),8)==0 && ring.position()==4);
    ring.play(true);CHECK(ring.read(out.data(),4)==4 && ring.position()==8);
    CHECK(ring.seek(12));ring.play(false);
    CHECK(ring.read(out.data(),8)==4 && ring.exhausted());
    CHECK(ring.read(out.data(),8)==0);
    PcmRing unsigned_pcm(8,1,128);unsigned_pcm.play(false);
    CHECK(unsigned_pcm.read(out.data(),8)==8);
    for(auto sample:out) CHECK(sample==128);
    PcmRing invalid(7,2,0);CHECK(!invalid.valid() && !invalid.seek(0));
    CHECK(amplitude_from_db(-10000)==0 && amplitude_from_db(0)==1);
    CHECK(std::abs(amplitude_from_db(-2000)-0.1f)<0.000001f);
    auto device=open_sdl_output();CHECK(device);
    CHECK(!device->create({44100,0,16},1024));
    CHECK(!device->create({44100,2,24},1024));
    CHECK(!device->create({44100,2,16},1023));
    auto first=device->create({44100,1,16},44100*8);
    auto second=device->create({22050,2,16},22050*16);
    CHECK(first && second && first->play(true) && second->play(true));
    auto wait_progress=[](const OutputBufferPtr& voice,std::size_t prior) {
        const auto start=SDL_GetTicks();
        while(SDL_GetTicks()-start<1500) { if(voice->position()!=prior) return true;SDL_Delay(5); }
        return false;
    };
    CHECK(wait_progress(first,0));CHECK(first->stop());
    const auto paused=first->position(),other=second->position();
    CHECK(wait_progress(second,other));CHECK(first->position()==paused && !first->playing());
    CHECK(first->play(true));CHECK(wait_progress(first,paused));
    CHECK(first->stop() && first->seek(0) && first->position()==0);
    auto single=device->create({44100,1,16},64);CHECK(single && single->play(false));
    const auto start=SDL_GetTicks();
    while(single->playing() && SDL_GetTicks()-start<1500) SDL_Delay(5);
    CHECK(!single->playing());
    CHECK(single->seek(0));SDL_Delay(50);
    CHECK(!single->playing() && single->position()==0);
    CHECK(single->play(false));
    device.reset(); // Outstanding buffers retain the device safely.
    CHECK(second->stop() && second->seek(0) && second->play(true));
    CHECK(wait_progress(second,0));
    return 0;
}
