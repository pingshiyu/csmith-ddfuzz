// ParametricRndNumGenerator.cpp — ddfuzz Csmith patch (design doc §5).
//
// See ParametricRndNumGenerator.h for the rationale. Only genrand() is
// overridden; everything else is inherited from DefaultRndNumGenerator so the
// generated programs are byte-for-byte what Csmith would produce for the
// equivalent random stream — hence still valid and UB-free.

#include "ParametricRndNumGenerator.h"

#include <fstream>
#include <sstream>
#include <iterator>

#include "SequenceFactory.h"

// ---- static storage ----
std::vector<unsigned char> ParametricRndNumGenerator::buffer_;
size_t   ParametricRndNumGenerator::cursor_        = 0;
size_t   ParametricRndNumGenerator::consumed_      = 0;
bool     ParametricRndNumGenerator::exhausted_     = false;
bool     ParametricRndNumGenerator::requested_     = false;
// Fixed fallback seed — deterministic extend-on-demand (matches the host-side
// toy generator's constant so behaviour is documented + reproducible).
uint32_t ParametricRndNumGenerator::fallback_state_ = 0xD1CE4B9Fu;
std::string ParametricRndNumGenerator::trace_path_;

ParametricRndNumGenerator::ParametricRndNumGenerator(const unsigned long seed)
    : DefaultRndNumGenerator(seed, SequenceFactory::make_sequence()), seed_(seed)
{
}

ParametricRndNumGenerator::~ParametricRndNumGenerator()
{
}

ParametricRndNumGenerator *
ParametricRndNumGenerator::make_rndnum_generator(const unsigned long seed)
{
    return new ParametricRndNumGenerator(seed);
}

void
ParametricRndNumGenerator::load_param_file(const std::string &path)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    buffer_.clear();
    cursor_ = 0;
    consumed_ = 0;
    exhausted_ = false;
    requested_ = true;
    if (in) {
        buffer_.assign((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    }
}

void
ParametricRndNumGenerator::set_trace_out(const std::string &path)
{
    trace_path_ = path;
}

void
ParametricRndNumGenerator::write_trace(void)
{
    if (trace_path_.empty()) return;
    std::ofstream out(trace_path_.c_str(), std::ios::trunc);
    if (out) {
        out << consumed_ << "\n";
    }
}

// Deterministic xorshift32 fallback used once the parameter buffer is exhausted.
static inline uint32_t xorshift32(uint32_t &s)
{
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

unsigned char
ParametricRndNumGenerator::next_byte(void)
{
    if (cursor_ < buffer_.size()) {
        unsigned char b = buffer_[cursor_++];
        if (cursor_ > consumed_) consumed_ = cursor_;
        return b;
    }
    exhausted_ = true;
    return static_cast<unsigned char>(xorshift32(fallback_state_) & 0xFF);
}

unsigned long
ParametricRndNumGenerator::genrand(void)
{
    // Big-endian assembly of four bytes → a 32-bit value. The exact byte order
    // is arbitrary but must be STABLE (it is part of the params→program map).
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        v = (v << 8) | static_cast<uint32_t>(next_byte());
    }
    return static_cast<unsigned long>(v);
}

RNDNUM_GENERATOR
ParametricRndNumGenerator::kind(void)
{
    // Requires the new enumerator added in AbsRndNumGenerator.h (README step 1).
    return RNDNUM_GENERATOR::rParametricRndNumGenerator;
}
