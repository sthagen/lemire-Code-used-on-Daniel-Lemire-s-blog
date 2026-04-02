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

double pretty_print(const std::string &name, size_t num_values,
                    counters::event_aggregate agg) {
  std::print("{:<50} : ", name);
  std::print(" {:5.3f} ns ", agg.fastest_elapsed_ns() / double(num_values));
  std::print(" {:5.2f} Gv/s ", double(num_values) / agg.fastest_elapsed_ns());
  if (counters::has_performance_counters()) {
    std::print(" {:5.2f} GHz ", agg.cycles() / double(agg.elapsed_ns()));
    std::print(" {:5.2f} c ", agg.fastest_cycles() / double(num_values));
    std::print(" {:5.2f} i ", agg.fastest_instructions() / double(num_values));
    std::print(" {:5.2f} i/c ",
               agg.fastest_instructions() / double(agg.fastest_cycles()));
  }
  std::print("\n");
  return double(num_values) / agg.fastest_elapsed_ns();
}

void collect_benchmark_results(size_t input_size, size_t number_strings) {
  // Generate many strings of varying content, including leading/trailing spaces
  std::vector<std::string> strings;
  strings.reserve(number_strings);
  // Create strings with varying lengths up to input_size
  for (size_t i = 0; i < number_strings; ++i) {
    size_t len = (i % input_size) + 1;
    std::string s;
    s.reserve(len + 10);
    // Fill with random printable characters and occasional spaces
    for (size_t k = 0; k < len; ++k) {
      char c = char('!' + (k + i) % 94);
      s.push_back(c);
    }
    strings.push_back(std::move(s));
  }

  volatile uint64_t counter_classic = 0;

  auto count_classic = [&strings, &counter_classic]() {
    size_t c = 0;
    for (const auto &str : strings) {
      c += std::count(str.begin(), str.end(), '!');
    }
    counter_classic += c;
  };

  volatile uint64_t counter_assembly_claude = 0;
  auto count_assembly_claude = [&strings, &counter_assembly_claude]() {
    size_t c = 0;
    for (const auto &str : strings) {
      size_t local_c = 0;
      const char* data = str.data();
      size_t len = str.size();
      __asm__ volatile (
        // NEON: process 16 bytes at a time
        "dup v1.16b, %w[ch]\n"       // v1 = splat '!'
        "movi v2.16b, #0\n"          // v2 = byte accumulator
        "mov x0, %[data]\n"
        "mov x1, %[len]\n"
        "cmp x1, #16\n"
        "b.lt 2f\n"
        // Main NEON loop
        "1:\n"
        "ld1 {v0.16b}, [x0], #16\n"
        "cmeq v0.16b, v0.16b, v1.16b\n" // 0xFF for match, 0x00 otherwise
        "sub v2.16b, v2.16b, v0.16b\n"  // sub -1 = add 1 per match
        "sub x1, x1, #16\n"
        "cmp x1, #16\n"
        "b.ge 1b\n"
        // Horizontal sum of byte accumulator
        "2:\n"
        "uaddlv h3, v2.16b\n"        // sum all 16 bytes into h3
        "umov w2, v3.h[0]\n"         // extract integer to GPR
        // Scalar tail
        "3:\n"
        "cbz x1, 4f\n"
        "ldrb w3, [x0], #1\n"
        "cmp w3, %w[ch]\n"
        "cinc w2, w2, eq\n"          // branchless increment
        "sub x1, x1, #1\n"
        "b 3b\n"
        "4:\n"
        "mov %w[out], w2\n"
        : [out] "=r"(local_c)
        : [data] "r"(data), [len] "r"(len), [ch] "r"((int)'!')
        : "x0", "x1", "x2", "x3", "v0", "v1", "v2", "v3"
      );
      c += local_c;
    }
    counter_assembly_claude += c;
  };
  volatile uint64_t counter_assembly_grok = 0;
  auto count_assembly_grok = [&strings, &counter_assembly_grok]() {
    size_t c = 0;
    for (const auto &str : strings) {
      size_t local_c = 0;
      const char* data = str.data();
      size_t len = str.size();
      __asm__ volatile (
        "dup v1.16b, %w[ch]\n"       // v1 = splat '!'
        "movi v2.16b, #0\n"          // v2 = byte accumulator
        "mov x0, %[data]\n"
        "mov x1, %[len]\n"
        "cmp x1, #32\n"
        "b.lt 2f\n"
        // Main NEON loop: process 32 bytes per iteration
        "1:\n"
        "ld1 {v0.16b}, [x0], #16\n"
        "cmeq v0.16b, v0.16b, v1.16b\n"
        "sub v2.16b, v2.16b, v0.16b\n"
        "ld1 {v0.16b}, [x0], #16\n"
        "cmeq v0.16b, v0.16b, v1.16b\n"
        "sub v2.16b, v2.16b, v0.16b\n"
        "sub x1, x1, #32\n"
        "cmp x1, #32\n"
        "b.ge 1b\n"
        // Process remaining 16 bytes if any
        "2:\n"
        "cmp x1, #16\n"
        "b.lt 3f\n"
        "ld1 {v0.16b}, [x0], #16\n"
        "cmeq v0.16b, v0.16b, v1.16b\n"
        "sub v2.16b, v2.16b, v0.16b\n"
        "sub x1, x1, #16\n"
        // Horizontal sum
        "3:\n"
        "uaddlv h3, v2.16b\n"
        "umov w2, v3.h[0]\n"
        // Scalar tail
        "4:\n"
        "cbz x1, 5f\n"
        "ldrb w3, [x0], #1\n"
        "cmp w3, %w[ch]\n"
        "cinc w2, w2, eq\n"
        "sub x1, x1, #1\n"
        "b 4b\n"
        "5:\n"
        "mov %w[out], w2\n"
        : [out] "=r"(local_c)
        : [data] "r"(data), [len] "r"(len), [ch] "r"((int)'!')
        : "x0", "x1", "x2", "x3", "v0", "v1", "v2", "v3"
      );
      c += local_c;
    }
    counter_assembly_grok += c;
  };

  volatile uint64_t counter_claude2 = 0;
  auto count_assembly_claude_2 = [&strings, &counter_claude2]() {
    size_t c = 0;
    for (const auto &str : strings) {
      size_t local_c = 0;
      const char* data = str.data();
      size_t len = str.size();
      __asm__ volatile (
        "dup v1.16b, %w[ch]\n"
        "movi v2.16b, #0\n"          // accumulator 0
        "movi v4.16b, #0\n"          // accumulator 1
        "movi v7.16b, #0\n"          // accumulator 2
        "movi v16.16b, #0\n"         // accumulator 3
        "mov x0, %[data]\n"
        "mov x1, %[len]\n"
        // 64-byte loop
        "cmp x1, #64\n"
        "b.lt 20f\n"
        "10:\n"
        "ldp q0, q3, [x0]\n"        // load 32 bytes
        "ldp q5, q6, [x0, #32]\n"   // load next 32 bytes
        "add x0, x0, #64\n"
        "cmeq v0.16b, v0.16b, v1.16b\n"
        "cmeq v3.16b, v3.16b, v1.16b\n"
        "cmeq v5.16b, v5.16b, v1.16b\n"
        "cmeq v6.16b, v6.16b, v1.16b\n"
        "sub v2.16b, v2.16b, v0.16b\n"
        "sub v4.16b, v4.16b, v3.16b\n"
        "sub v7.16b, v7.16b, v5.16b\n"
        "sub v16.16b, v16.16b, v6.16b\n"
        "sub x1, x1, #64\n"
        "cmp x1, #64\n"
        "b.ge 10b\n"
        // 16-byte loop for remainder
        "20:\n"
        "cmp x1, #16\n"
        "b.lt 30f\n"
        "21:\n"
        "ld1 {v0.16b}, [x0], #16\n"
        "cmeq v0.16b, v0.16b, v1.16b\n"
        "sub v2.16b, v2.16b, v0.16b\n"
        "sub x1, x1, #16\n"
        "cmp x1, #16\n"
        "b.ge 21b\n"
        // Merge 4 accumulators and horizontal sum
        "30:\n"
        "add v2.16b, v2.16b, v4.16b\n"
        "add v7.16b, v7.16b, v16.16b\n"
        "add v2.16b, v2.16b, v7.16b\n"
        "uaddlv h3, v2.16b\n"
        "umov w2, v3.h[0]\n"
        // Scalar tail
        "31:\n"
        "cbz x1, 40f\n"
        "ldrb w3, [x0], #1\n"
        "cmp w3, %w[ch]\n"
        "cinc w2, w2, eq\n"
        "sub x1, x1, #1\n"
        "b 31b\n"
        "40:\n"
        "mov %w[out], w2\n"
        : [out] "=r"(local_c)
        : [data] "r"(data), [len] "r"(len), [ch] "r"((int)'!')
        : "x0", "x1", "x2", "x3",
          "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v16"
      );
      c += local_c;
    }
    counter_claude2 += c;
  };
  volatile uint64_t counter_assembly_grok2 = 0;
  auto count_assembly_grok2 = [&strings, &counter_assembly_grok2]() {
    size_t c = 0;
    for (const auto &str : strings) {
      size_t local_c = 0;
      const char* data = str.data();
      size_t len = str.size();
      __asm__ volatile (
        "dup v1.16b, %w[ch]\n"
        "movi v2.16b, #0\n"          // accumulator 0
        "movi v4.16b, #0\n"          // accumulator 1
        "movi v7.16b, #0\n"          // accumulator 2
        "movi v16.16b, #0\n"         // accumulator 3
        "mov x0, %[data]\n"
        "mov x1, %[len]\n"
        // 64-byte loop with consecutive loads
        "cmp x1, #64\n"
        "b.lt 20f\n"
        "10:\n"
        "ldp q0, q3, [x0], #32\n"    // load 32 bytes into q0, q3
        "ldp q5, q6, [x0], #32\n"    // load next 32 bytes into q5, q6
        "cmeq v0.16b, v0.16b, v1.16b\n"
        "cmeq v3.16b, v3.16b, v1.16b\n"
        "cmeq v5.16b, v5.16b, v1.16b\n"
        "cmeq v6.16b, v6.16b, v1.16b\n"
        "sub v2.16b, v2.16b, v0.16b\n"
        "sub v4.16b, v4.16b, v3.16b\n"
        "sub v7.16b, v7.16b, v5.16b\n"
        "sub v16.16b, v16.16b, v6.16b\n"
        "sub x1, x1, #64\n"
        "cmp x1, #64\n"
        "b.ge 10b\n"
        // 16-byte loop for remainder
        "20:\n"
        "cmp x1, #16\n"
        "b.lt 30f\n"
        "21:\n"
        "ld1 {v0.16b}, [x0], #16\n"
        "cmeq v0.16b, v0.16b, v1.16b\n"
        "sub v2.16b, v2.16b, v0.16b\n"
        "sub x1, x1, #16\n"
        "cmp x1, #16\n"
        "b.ge 21b\n"
        // Merge 4 accumulators and horizontal sum
        "30:\n"
        "add v2.16b, v2.16b, v4.16b\n"
        "add v7.16b, v7.16b, v16.16b\n"
        "add v2.16b, v2.16b, v7.16b\n"
        "uaddlv h3, v2.16b\n"
        "umov w2, v3.h[0]\n"
        // Scalar tail
        "31:\n"
        "cbz x1, 40f\n"
        "ldrb w3, [x0], #1\n"
        "cmp w3, %w[ch]\n"
        "cinc w2, w2, eq\n"
        "sub x1, x1, #1\n"
        "b 31b\n"
        "40:\n"
        "mov %w[out], w2\n"
        : [out] "=r"(local_c)
        : [data] "r"(data), [len] "r"(len), [ch] "r"((int)'!')
        : "x0", "x1", "x2", "x3",
          "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v16"
      );
      c += local_c;
    }
    counter_assembly_grok2 += c;
  };

  volatile uint64_t counter_claude3 = 0;
  auto count_assembly_claude_3 = [&strings, &counter_claude3]() {
    size_t c = 0;
    for (const auto &str : strings) {
      size_t local_c = 0;
      const char* data = str.data();
      size_t len = str.size();
      __asm__ volatile (
        "dup v1.16b, %w[ch]\n"
        // 8 independent byte-lane accumulators
        "movi v2.16b, #0\n"
        "movi v4.16b, #0\n"
        "movi v7.16b, #0\n"
        "movi v16.16b, #0\n"
        "movi v25.16b, #0\n"
        "movi v26.16b, #0\n"
        "movi v27.16b, #0\n"
        "movi v28.16b, #0\n"
        "mov x0, %[data]\n"
        "mov x1, %[len]\n"
        // 128-byte loop
        "cmp x1, #128\n"
        "b.lt 20f\n"
        "10:\n"
        // 4 independent ldp loads (no writeback, all read same x0)
        "ldp q17, q18, [x0]\n"
        "ldp q19, q20, [x0, #32]\n"
        "ldp q21, q22, [x0, #64]\n"
        "ldp q23, q24, [x0, #96]\n"
        "add x0, x0, #128\n"
        // 8 compares
        "cmeq v17.16b, v17.16b, v1.16b\n"
        "cmeq v18.16b, v18.16b, v1.16b\n"
        "cmeq v19.16b, v19.16b, v1.16b\n"
        "cmeq v20.16b, v20.16b, v1.16b\n"
        "cmeq v21.16b, v21.16b, v1.16b\n"
        "cmeq v22.16b, v22.16b, v1.16b\n"
        "cmeq v23.16b, v23.16b, v1.16b\n"
        "cmeq v24.16b, v24.16b, v1.16b\n"
        // 8 independent accumulates (each to its own accumulator)
        "sub v2.16b, v2.16b, v17.16b\n"
        "sub v4.16b, v4.16b, v18.16b\n"
        "sub v7.16b, v7.16b, v19.16b\n"
        "sub v16.16b, v16.16b, v20.16b\n"
        "sub v25.16b, v25.16b, v21.16b\n"
        "sub v26.16b, v26.16b, v22.16b\n"
        "sub v27.16b, v27.16b, v23.16b\n"
        "sub v28.16b, v28.16b, v24.16b\n"
        "sub x1, x1, #128\n"
        "cmp x1, #128\n"
        "b.ge 10b\n"
        // 16-byte loop for remainder
        "20:\n"
        "cmp x1, #16\n"
        "b.lt 30f\n"
        "21:\n"
        "ld1 {v0.16b}, [x0], #16\n"
        "cmeq v0.16b, v0.16b, v1.16b\n"
        "sub v2.16b, v2.16b, v0.16b\n"
        "sub x1, x1, #16\n"
        "cmp x1, #16\n"
        "b.ge 21b\n"
        // Tree-merge 8 accumulators into v2
        "30:\n"
        "add v2.16b, v2.16b, v25.16b\n"
        "add v4.16b, v4.16b, v26.16b\n"
        "add v7.16b, v7.16b, v27.16b\n"
        "add v16.16b, v16.16b, v28.16b\n"
        "add v2.16b, v2.16b, v4.16b\n"
        "add v7.16b, v7.16b, v16.16b\n"
        "add v2.16b, v2.16b, v7.16b\n"
        "uaddlv h3, v2.16b\n"
        "umov w2, v3.h[0]\n"
        // Scalar tail
        "31:\n"
        "cbz x1, 40f\n"
        "ldrb w3, [x0], #1\n"
        "cmp w3, %w[ch]\n"
        "cinc w2, w2, eq\n"
        "sub x1, x1, #1\n"
        "b 31b\n"
        "40:\n"
        "mov %w[out], w2\n"
        : [out] "=r"(local_c)
        : [data] "r"(data), [len] "r"(len), [ch] "r"((int)'!')
        : "x0", "x1", "x2", "x3",
          "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
          "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",
          "v25", "v26", "v27", "v28"
      );
      c += local_c;
    }
    counter_claude3 += c;
  };

  pretty_print("count_classic", number_strings, counters::bench(count_classic));
  pretty_print("count_assembly_claude", number_strings, counters::bench(count_assembly_claude));
  pretty_print("count_assembly_grok", number_strings, counters::bench(count_assembly_grok));
  pretty_print("count_assembly_claude_2", number_strings, counters::bench(count_assembly_claude_2));
  pretty_print("count_assembly_grok2", number_strings, counters::bench(count_assembly_grok2));
  pretty_print("count_assembly_claude_3", number_strings, counters::bench(count_assembly_claude_3));

  // Validate correctness with a single run of each
  counter_classic = 0;
  counter_assembly_claude = 0;
  counter_assembly_grok = 0;
  counter_claude2 = 0;
  counter_assembly_grok2 = 0;
  counter_claude3 = 0;
  count_classic();
  count_assembly_claude();
  count_assembly_grok();
  count_assembly_claude_2();
  count_assembly_grok2();
  count_assembly_claude_3();
  if (counter_classic != counter_assembly_claude) {
    std::cout << "Error: counts differ (classic vs claude): " << counter_classic << " vs " << counter_assembly_claude << std::endl;
  }
  if (counter_classic != counter_assembly_grok) {
    std::cout << "Error: counts differ (classic vs grok): " << counter_classic << " vs " << counter_assembly_grok << std::endl;
  }
  if (counter_classic != counter_claude2) {
    std::cout << "Error: counts differ (classic vs claude2): " << counter_classic << " vs " << counter_claude2 << std::endl;
  }
  if (counter_classic != counter_assembly_grok2) {
    std::cout << "Error: counts differ (classic vs grok2): " << counter_classic << " vs " << counter_assembly_grok2 << std::endl;
  }
  if (counter_classic != counter_claude3) {
    std::cout << "Error: counts differ (classic vs claude3): " << counter_classic << " vs " << counter_claude3 << std::endl;
  }
}

int main(int argc, char **argv) { collect_benchmark_results(1024, 100000); }
