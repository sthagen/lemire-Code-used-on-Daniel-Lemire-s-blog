#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <print>
#include <random>
#include <string>
#include <vector>

#include "counters/bench.h"


// randomizer
static inline uint64_t rng(uint64_t h) {
  h ^= h >> 33;
  h *= UINT64_C(0xff51afd7ed558ccd);
  h ^= h >> 33;
  h *= UINT64_C(0xc4ceb9fe1a85ec53);
  h ^= h >> 33;
  return h;
}


__attribute__((noinline)) void cond_sum_random(uint64_t howmany,
                                               uint64_t *out) {
  while (howmany != 0) {
    uint64_t randomval = rng(howmany);
    if ((randomval & 1) == 1)
      *out++ = randomval;
    howmany--;
  }
}


double pretty_print(const std::string &name, size_t num_values,
                    counters::event_aggregate agg) {
  std::print("{:<50} : ", name);
  std::print(" {:5.3f} ns ", agg.fastest_elapsed_ns() / double(num_values));
  std::print(" {:5.2f} Gv/s ", double(num_values) / agg.fastest_elapsed_ns());
  if (counters::has_performance_counters()) {
    std::print(" {:5.2f} bm ", agg.branch_misses() / double(num_values));
  }
  std::print("\n");
  return double(num_values) / agg.fastest_elapsed_ns();
}

void collect_benchmark_results(size_t max_input_size) {
  std::vector<uint64_t> buffer(max_input_size);
  for(double num_values_d = 1'000; num_values_d <= max_input_size; num_values_d *= 1.7782794100389228 ) {
    size_t num_values = static_cast<size_t>(round(num_values_d));
    pretty_print(std::to_string(num_values), num_values, counters::bench(
    [&]() { cond_sum_random(num_values, buffer.data()); }
    ));
  }
}

int main(int argc, char **argv) { 
  if (!counters::has_performance_counters()) {
    std::cerr << "Performance counters not available. Run as root or check permissions.\n";
    return 1;
  }
  collect_benchmark_results(1000000);
}
