// -*- mode: C++ -*-
//
// ParametricRndNumGenerator.h — ddfuzz Csmith patch (design doc §5, component F).
//
// A new RNG backend for Csmith that sources its randomness from an external
// *parameter byte buffer* instead of a PRNG, so that mutating those bytes
// (Zest) deterministically mutates the generated program while every generated
// program stays valid + UB-free (because this hook sits strictly BELOW all of
// Csmith's decision/validity logic — see the design doc's "hook at the lowest
// primitive" note).
//
// It overrides ONLY the low-level primitive `genrand()`. All higher-level
// choices (`rnd_upto`, `rnd_flipcoin`, production selection, …) are inherited
// unchanged from Csmith, so the byte→structure mapping is exactly Csmith's own
// modulo arithmetic applied to our byte stream — that is what makes a bit-level
// param mutation propagate into a structural program mutation (Zest Obs. 2).
//
// On buffer exhaustion it switches to a deterministic fixed-seed xorshift
// fallback (Zest "extend-on-demand"), and it records the number of buffer bytes
// consumed to the `--rng-trace-out` file so the fuzzer can truncate admitted
// params to the bytes Csmith actually read.
//
// Integration: see patch/README.md. This subclasses DefaultRndNumGenerator so
// it reuses Default's rnd_upto/flipcoin/RandomHexDigits (which all funnel
// through the virtual genrand()).

#ifndef PARAMETRIC_RND_NUM_GENERATOR_H
#define PARAMETRIC_RND_NUM_GENERATOR_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include "DefaultRndNumGenerator.h"

class ParametricRndNumGenerator : public DefaultRndNumGenerator {
public:
    // Constructed via AbsRndNumGenerator::make_rndnum_generator once the
    // rParametricRndNumGenerator kind is selected (see README step 2).
    static ParametricRndNumGenerator *
    make_rndnum_generator(const unsigned long seed);

    virtual ~ParametricRndNumGenerator();

    // The single overridden primitive: assemble a 32-bit value from four bytes
    // pulled from the parameter buffer (or the fallback PRNG once exhausted).
    virtual unsigned long genrand(void) override;

    virtual RNDNUM_GENERATOR kind(void) override;

    // ---- ddfuzz control surface (wired to the new CLI flags) ----

    // Load the parameter buffer from a file (`--parametric-rng-file`).
    static void load_param_file(const std::string &path);

    // Where to write the consumed-byte count (`--rng-trace-out`).
    static void set_trace_out(const std::string &path);

    // Flush the consumed-byte count to the trace file. Call at end of run.
    static void write_trace(void);

    static bool requested(void) { return requested_; }

    static size_t consumed(void) { return consumed_; }

protected:
    ParametricRndNumGenerator(const unsigned long seed);

private:
    unsigned char next_byte(void);

    // Static so the singleton control surface + factory can reach them.
    static std::vector<unsigned char> buffer_;
    static size_t   cursor_;
    static size_t   consumed_;     // high-water mark of buffer bytes read
    static bool     exhausted_;
    static bool     requested_;
    static uint32_t fallback_state_;
    static std::string trace_path_;

    unsigned long seed_;
};

#endif // PARAMETRIC_RND_NUM_GENERATOR_H
