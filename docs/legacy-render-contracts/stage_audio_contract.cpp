// The stage host still compiles as C. This test-only translation unit builds
// the actual audio implementation once so fixtures can access its private
// owners without exporting globals or adding injection paths to the game.
#include "../src/reconstructed/audio_runtime.cpp"
#include "stage_audio_contract.h"
#include "kinoko/game_math.h"
#include "audio_output_fixture.hpp"
#include <array>
#include <cstdio>
#include <float.h>

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "stage audio line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

namespace {
class Device final : public kinoko::audio::OutputDevice {
public:
    AudioTestBuffer buffer;
    bool fail_create=false;
    kinoko::audio::OutputBufferPtr create(kinoko::audio::PcmFormat,std::size_t bytes) override {
        if(fail_create) return {};
        buffer.bytes.resize(bytes);return buffer.borrow(true);
    }
};
class DeviceFixture final {
    std::shared_ptr<kinoko::audio::OutputDevice> previous_;
public:
    explicit DeviceFixture(kinoko::audio::OutputDevice& device) : previous_(std::move(g_audio_device)) {
        g_audio_device={&device,[](kinoko::audio::OutputDevice*){}};
    }
    ~DeviceFixture() {
        kinoko_bgm_release_all_tracks_locked();g_audio_device=std::move(previous_);
    }
    DeviceFixture(const DeviceFixture&)=delete;
    DeviceFixture& operator=(const DeviceFixture&)=delete;
};
struct SoundCleanupGuard {
    ~SoundCleanupGuard() { kinoko_se_pool_release(); }
};

}

extern "C" int kinoko_test_bgm_pause(void) {
    AudioTestBuffer buffer;
    BgmTrack saved = std::move(g_kinoko_bgm_track);
    const int32_t old_handle = kinoko_active_bgm_slot;
    g_kinoko_bgm_track = BgmTrack{};
    auto& track = g_kinoko_bgm_track;
    track.buffer = buffer.borrow();
    track.handle = kinoko_active_bgm_slot = 123;
    track.buffer_bytes = 65536;
    track.started = track.playing = track.looping = 1;
    track.play_offset = buffer.cursor = 4096;
    track.buffered_bytes = 32768;
    track.write_window_start = 32768;
    buffer.status = 1u;
    kinoko_audio_pause_bgm();
    const bool paused = buffer.stops == 1 && !buffer.status && !track.playing;
    kinoko_bgm_service_track(&track);
    const bool serviced = buffer.plays == 0 && buffer.cursor == 4096 &&
        track.play_offset == 4096 && track.buffered_bytes == 32768 && buffer.locks == 0;
    kinoko_audio_pause_bgm();
    const bool resumed = buffer.plays == 1 && buffer.status == 1u &&
        buffer.cursor == 4096 && track.playing && track.started;
    kinoko_bgm_stop_for_handle(123);
    const bool stopped = buffer.cursor == 0 && !track.started && !track.playing;
    g_kinoko_bgm_track = std::move(saved);
    kinoko_active_bgm_slot = old_handle;
    CHECK(paused && serviced && resumed && stopped);
    CHECK(buffer.releases == 1);
    std::puts("PASS: PauseBgm toggles without rewind or service restart; StopBgm still rewinds");
    return 0;
}

extern "C" int kinoko_test_sound_cleanup(int32_t (*clear_all)(void)) {
    AudioTestBuffer buffers[3];
    SoundCleanupGuard cleanup;
    g_kinoko_se_entry_count = 2;
    g_kinoko_se_entries[0].buffer = buffers[0].borrow();
    g_kinoko_se_entries[1].buffer = buffers[1].borrow();
    g_kinoko_se_pool.stream_slots[0].buffer = buffers[2].borrow();
    g_kinoko_se_pool.initialized = 1;
    CHECK(clear_all() == 1);
    CHECK(!g_kinoko_se_entry_count && !g_kinoko_se_pool.initialized);
    CHECK(clear_all() == 1);
    for (const auto& buffer : buffers) CHECK(buffer.releases == 1 && buffer.refs == 0);
    CHECK(buffers[0].stops == 1 && buffers[1].stops == 1);
    return 0;
}

extern "C" int kinoko_test_bgm_preserves_game_math(void) {
    // Only the output device is replaced. DAT reads, the vendored Vorbis decoder,
    // SFL loop markers and the production initial ring fill remain real.
    Device device;
    DeviceFixture fixture(device);
    unsigned current = 0, x87 = 0, sse = 0;
    std::array<unsigned char, RETDEC_BGM_CHUNK_BYTES> reference{};
    _controlfp_s(&current, _RC_NEAR, _MCW_RC);
    CHECK(kinoko_bgm_prepare_track(1, "data/bgm/st1.ogg", 1, 1.0f));
    std::memcpy(reference.data(), device.buffer.bytes.data(), reference.size());
    kinoko_bgm_release_track_locked();
    CHECK(device.buffer.refs == 1);
    kinoko_enter_game_math();
    for (int failure = 0; failure <= 2; ++failure) {
        device.fail_create = failure == 1;
        device.buffer.fail_lock = failure == 2;
        CHECK(kinoko_bgm_prepare_track(1, "data/bgm/st1.ogg", 1, 1.0f) == (failure == 0));
        CHECK(__control87_2(0, 0, &x87, &sse));
        CHECK((x87 & _MCW_RC) == _RC_UP && (sse & _MCW_RC) == _RC_UP);
        if (!failure) CHECK(std::memcmp(reference.data(), device.buffer.bytes.data(), reference.size()) == 0);
        kinoko_bgm_release_track_locked();
        CHECK(device.buffer.refs == 1);
    }
    device.fail_create = device.buffer.fail_lock = false;
    CHECK(!kinoko_bgm_prepare_track(1, "data/script/constant.cv4", 1, 1.0f));
    CHECK(__control87_2(0, 0, &x87, &sse));
    CHECK((x87 & _MCW_RC) == _RC_UP && (sse & _MCW_RC) == _RC_UP);
    std::puts("PASS: real BGM decoding preserves PCM and game rounding on success/decoder/device/fill failure");
    return 0;
}

extern "C" int kinoko_test_sound_module(void) {
    auto device=kinoko::audio::open_sdl_output();
    CHECK(device);
    auto buffer=device->create({22050,1,16},4096);
    CHECK(buffer);
    std::puts("PASS: SDL output device and PCM buffer creation");
    return 0;
}
