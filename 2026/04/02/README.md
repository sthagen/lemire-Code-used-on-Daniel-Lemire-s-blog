# Can AI Write Assembly? Benchmarking Character Counting in Strings

This project explores whether AI models (specifically Grok and Claude) can generate efficient ARM64 assembly code for a simple task: counting the number of exclamation marks ('!') in a collection of strings.

## Overview

The benchmark compares several implementations of a character counting function:

- **count_classic**: Uses C++ standard library `std::count` for reference.
- **count_assembly**: A basic ARM64 assembly loop (byte-by-byte comparison).
- **count_assembly_claude**: Claude's SIMD-optimized version using NEON instructions (16-byte chunks).
- **count_assembly_grok**: Grok's optimized version (32-byte chunks).
- **count_assembly_claude_2**: Claude's further optimized version (64-byte chunks with multiple accumulators).
- **count_assembly_grok2**: Grok's latest version (64-byte chunks with improved accumulator handling).

Each function processes 100,000 strings of varying lengths (up to 1024 characters), filled with random printable characters including occasional '!' symbols.

## Building

This project uses CMake. From the `benchmark/` subdirectory:

```bash
cd benchmark
cmake -B build
cmake --build build -j 10
```

## Running

Execute the benchmark:

```bash
./build/benchmark
```

The output shows timing results for each implementation, measured in nanoseconds per operation.

## Results

Typical performance (on ARM64 hardware):

- count_classic: ~58 ns
- count_assembly: ~270 ns
- count_assembly_claude: ~12 ns
- count_assembly_grok: ~8 ns
- count_assembly_claude_2: ~6.5 ns
- count_assembly_grok2: ~6.5 ns

The SIMD versions achieve significant speedups through parallel processing of multiple bytes.

## Observations

Both AI models successfully generated working ARM64 assembly, with Grok and Claude iteratively improving their implementations. The competition drove optimizations from basic loops to advanced SIMD techniques, demonstrating AI's capability to produce efficient low-level code.

## Requirements

- ARM64-compatible system
- CMake 3.10+
- C++23 compatible compiler (e.g., Clang)
- Performance counters library (included via CPM)