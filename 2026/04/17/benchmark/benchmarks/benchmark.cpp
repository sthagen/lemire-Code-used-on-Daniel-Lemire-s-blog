#include <cassert>
#include <cstdint>
#include <iostream>
#include <print>
#include <random>
#include <string>
#include <vector>

#include "counters/bench.h"

#include "neon.h"
#include "sve.h"  // requires -msve-vector-bits=128

// Build a JSON-like string whose length is a multiple of 64.
// Characters are drawn from a weighted alphabet that includes
// structural chars (:,[]{}) and whitespace (\t \n \r space).
static std::string make_json_like(size_t target_bytes, uint64_t seed = 42) {
  static constexpr char alphabet[] =
      // structural (op) chars — repeated to raise frequency
      ": , [ ] { } : , [ ] { }"
      // whitespace
      "    \t\n\r"
      // generic printable
      "abcdefghijklmnopqrstuvwxyz0123456789\"_-.";
  constexpr size_t alpha_len = sizeof(alphabet) - 1;

  // Round up to next multiple of 64.
  const size_t len = (target_bytes + 63) & ~size_t(63);
  std::string s(len, '\0');
  std::mt19937_64 rng(seed);
  for (size_t i = 0; i < len; ++i)
    s[i] = alphabet[rng() % alpha_len];
  return s;
}

double pretty_print(const std::string &name, size_t num_bytes,
                    counters::event_aggregate agg) {
  std::print("{:<50} : ", name);
  std::print(" {:5.3f} ns/byte ", agg.fastest_elapsed_ns() / double(num_bytes));
  std::print(" {:5.2f} GB/s ",
             double(num_bytes) / agg.fastest_elapsed_ns());
  if (counters::has_performance_counters()) {
    std::print(" {:5.2f} GHz ", agg.cycles() / double(agg.elapsed_ns()));
    std::print(" {:5.2f} c/byte ", agg.fastest_cycles() / double(num_bytes));
    std::print(" {:5.2f} i ", agg.fastest_instructions());
    std::print(" {:5.2f} i/c ",
               agg.fastest_instructions() / double(agg.fastest_cycles()));
  }
  std::print("\n");
  return double(num_bytes) / agg.fastest_elapsed_ns();
}

int main() {
  constexpr size_t INPUT_BYTES = 1 << 20; // 1 MiB
  const std::string input = make_json_like(INPUT_BYTES);
  const size_t num_blocks = input.size() / 64;

  // ── Correctness pass ────────────────────────────────────────────────────
  size_t mismatches = 0;
  for (size_t b = 0; b < num_blocks; ++b) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(input.data()) + b * 64;

    // SVE returns {op, whitespace}
    auto [sve_op, sve_ws] = sve::classify(p);
    // NEON returns {whitespace, op}
    auto [neon_ws, neon_op] = neon::classify(p);

    if (sve_op != neon_op || sve_ws != neon_ws) {
      ++mismatches;
      std::print("mismatch at block {}: sve_op={:#018x} neon_op={:#018x} "
                 "sve_ws={:#018x} neon_ws={:#018x}\n",
                 b, sve_op, neon_op, sve_ws, neon_ws);
      if (mismatches > 10) { std::print("(stopping after 10 mismatches)\n"); break; }
    }
  }
  if (mismatches == 0)
    std::print("Correctness check passed ({} blocks).\n\n", num_blocks);
  else
    std::print("FAILED: {} mismatches.\n\n", mismatches);

  // ── Benchmarks ──────────────────────────────────────────────────────────
  volatile uint64_t sink = 0; // prevent dead-code elimination

  auto bench_sve = [&]() {
    uint64_t acc = 0;
    for (size_t b = 0; b < num_blocks; ++b) {
      const uint8_t* p = reinterpret_cast<const uint8_t*>(input.data()) + b * 64;
      auto [op, ws] = sve::classify(p);
      acc += op + ws;
    }
    sink = acc;
  };

  auto bench_neon = [&]() {
    uint64_t acc = 0;
    for (size_t b = 0; b < num_blocks; ++b) {
      const uint8_t* p = reinterpret_cast<const uint8_t*>(input.data()) + b * 64;
      auto [ws, op] = neon::classify(p);
      acc += op + ws;
    }
    sink = acc;
  };

  pretty_print("sve::classify",  input.size(), counters::bench(bench_sve));
  pretty_print("neon::classify", input.size(), counters::bench(bench_neon));
}
