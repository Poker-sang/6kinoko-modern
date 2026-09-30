#pragma once
#include "kinoko/input_actions.hpp"
#include "kinoko/input_manager.h"
#include <istream>
#include <ostream>
#include <string>

namespace kinoko::replay {
inline constexpr uint32_t version=1;
inline constexpr uint64_t maximum_frames=1000000;
struct Frame {
    input::Frame actions;
    KinokoInputPublishedState legacy{};
    uint32_t clock{}, random_before{}, random_after{};
    uint64_t checkpoint{};
};
// Explicit little-endian wire format; no native pointers or struct padding.
class Writer {
public:
    Writer(std::ostream&,const std::string& identity);
    void append(const Frame&);
    void finish();
    // Export the flushed prefix with a footer without finalizing this writer.
    void snapshot(std::istream& prefix,std::ostream& destination);
    uint64_t count() const {return count_;}
private:
    std::ostream& out_; uint64_t count_=0,chain_=14695981039346656037ull;bool finished_=false;
};
class Reader {
public:
    // Validates the complete stream and footer before any game code executes.
    Reader(std::istream&,const std::string& identity);
    bool next(Frame&);
    uint64_t count() const {return total_;}
private:
    std::istream& in_;uint64_t total_=0,index_=0;
};
bool same_checkpoint(const Frame& expected,const Frame& actual);
}
