#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
namespace kinoko::audio {
struct PcmFormat {
    std::uint32_t rate{};
    std::uint16_t channels{}, bits{};
    std::size_t frame_bytes() const noexcept { return std::size_t(channels) * (bits / 8); }
};
// Portable byte-ring mechanics; serialized by the owning output stream.
// No pointer-sized field is serialized into CV3/DAT or the legacy host records.
class PcmRing {
public:
    PcmRing(std::size_t bytes, std::size_t frame_bytes, std::uint8_t silence);
    bool valid() const noexcept;
    bool write(std::size_t offset, const void* data, std::size_t bytes);
    std::size_t read(void* destination, std::size_t bytes);
    bool seek(std::size_t offset);
    void play(bool looping) noexcept { looping_ = looping; running_ = true; }
    void stop() noexcept { running_ = false; }
    bool running() const noexcept { return running_; }
    bool exhausted() const noexcept { return cursor_ == bytes_.size(); }
    std::size_t position() const noexcept { return cursor_; }
    std::size_t frame_bytes() const noexcept { return frame_bytes_; }
private:
    std::vector<std::uint8_t> bytes_;
    std::size_t frame_bytes_{}, cursor_{};
    bool running_{}, looping_{};
};
float amplitude_from_db(std::int32_t hundredth_db) noexcept;
class OutputBuffer {
public:
    virtual ~OutputBuffer() = default;
    virtual bool write(std::size_t offset, const void* data, std::size_t bytes) = 0;
    virtual bool play(bool looping) = 0;
    // Stop freezes this voice and its queued samples; seek explicitly discards them.
    virtual bool stop() = 0;
    virtual bool seek(std::size_t offset) = 0;
    virtual bool playing() const = 0;
    // Source bytes already handed to the converter, not a hardware DAC cursor.
    virtual std::size_t position() const = 0;
    virtual void volume_db(std::int32_t hundredth_db) = 0;
};
using OutputBufferPtr = std::shared_ptr<OutputBuffer>;
class OutputDevice {
public:
    virtual ~OutputDevice() = default;
    virtual OutputBufferPtr create(PcmFormat format, std::size_t bytes) = 0;
};
std::shared_ptr<OutputDevice> open_sdl_output();
}
