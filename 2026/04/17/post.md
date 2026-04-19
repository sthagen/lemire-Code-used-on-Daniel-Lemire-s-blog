# The fastest way to match characters on ARM processors?

Consider the following problem. Given a string, you must match all of the ASCII white-space characters (`\t`, `\n`, `\r`, and the space) and some characters important in JSON (`:`, `,`, `[`, `]`, `{`, `}`). JSON is a text-based data format used for web services. A toy JSON document looks as follows.


```json
{
  "name": "Alice",
  "age": 30,
  "email": "alice@example.com",
  "tags": ["developer", "python", "open-source"],
  "active": true
}
```

We want to solve this problem using SIMD (single-instruction-multiple-data) instructions. With these instructions, you can compare a block of 16 bytes with another block of 16 bytes in one instruction.

It is a subproblem in the fast simdjson JSON library when we index a JSON document. We call this task *vectorized classification*. We also use the same technique when parsing DNS records, and so forth. In the actual simdjson library, we must also handle strings and quotes, and it gets more complicated. 

I need to define what I mean by 'matching' the characters. In my case, it is enough to get, for each block of 64 bytes, two 64-bit masks: one for spaces and one for important characters. To illustrate, let me consider a 16-byte variant:


```
{"name": "Ali" }
1000000100000001 // important characters
0000000010000010 // spaces
```

Thus, I want to get back the numbers `0b1000000100000001` and `0b0000000010000010` in binary format (they are 33025 and 130 in decimal).

I refer you to Langdale and Lemire (2019) for how to do it using the conventional SIMD instructions available on ARM processors (NEON). Their key idea is a table-driven, branch-free classifier: for each byte, split into low and high nibbles, use SIMD table lookups to map each nibble to a bitmask, and combine the two masks (with bitwise AND) to decide whether the byte belongs to a target set (whitespace or structural JSON characters). This avoids doing many separate equality comparisons per character.


There is now a better way on recent ARM processors.

The 128-bit version of NEON was introduced in 2011 with the ARMv8-A architecture (AArch64). Apple played an important role and it was first used by the Apple A7 chip in the iPhone 5S. You can count on all 64-bit ARM processors to support NEON, which is convenient. (There are 32-bit ARM processors but they are mostly used for embedded systems, not mainstream computing.)

ARM NEON is good but getting old. It is no match for the AVX-512 instruction set available on x64 (AMD and Intel) processors. Not only do the AVX-512 instructions support wider registers (64 bytes as opposed to ARM NEON's 16 bytes), but they also have more powerful instructions.

But ARM has something else to offer: Scalable Vector Extension (SVE) and its successor, SVE2. Though SVE was first introduced in 2016, it took until 2022 before we had actual access. The Neoverse V1 architecture used by the Amazon Graviton 3 is the first one I had access to. Soon after, we got SVE2 with the Neoverse V2 and N2 architectures. Today it is readily available: the Graviton4 on AWS, the Microsoft Cobalt 100 on Azure, the Google Axion on Google Cloud (and newer Google Cloud ARM CPUs), the NVIDIA Grace CPU, as well as several chips from Qualcomm, MediaTek, and Samsung. Notice who I am not including? Apple. For unclear reasons, Apple has not yet adopted SVE2.

I have mixed feelings about SVE/SVE2. Like RISC-V, it breaks with the approach from ARM NEON and x64 SIMD that uses fixed-length register sizes (16 bytes, 32 bytes, 64 bytes). This means that you are expected to code without knowing how wide the registers are.

This is convenient for chip makers because it gives them the option of adjusting the register size to better suit their market. Yet it seems to have failed. While the Graviton 3 processor from Amazon had 256-bit registers... all commodity chips have had 128-bit registers after that.

On the plus side, SVE/SVE2 has masks a bit like AVX-512, so you can load and process data only in a subset of the registers. It solves a long-standing problem with earlier SIMD instruction sets where the input is not a multiple of the register size. Both SVE/SVE2 and AVX-512 might make tail handling nicer. Being able to operate on only part of the register allows clever optimizations. Sadly, SVE/SVE2 does not allow you to move masks to and from a general-purpose register efficiently, unlike AVX-512. And that's a direct consequence of their design with variable-length registers. Thus, even though your registers might always be 128-bit and contain 16 bytes, the instruction set is not allowed to assume that a mask fits in a 16-bit word.

I was pessimistic regarding SVE/SVE2 until I learned that [it is designed to be interoperable with ARM NEON](https://lemire.me/blog/2025/03/29/mixing-arm-neon-with-sve-code-for-fun-and-profit/). Thus you can use the SVE/SVE2 instructions with your ARM NEON code. This works especially well if you know that the SVE/SVE2 registers match the ARM NEON registers (16 bytes).

For the work I do, there are two SVE2 instructions that are important: `match` and `nmatch`. In their 8-bit versions, what they do is the following: given two vectors `a` and `b`, each containing up to 16 bytes, `match` sets a predicate bit to `true` for each position `i` where `a[i]` equals *any* of the bytes in `b`. In other words, `b` acts as a small lookup set, and `match` tests set membership for every byte of `a` simultaneously. The `nmatch` instruction is the logical complement: it sets a predicate bit to `true` wherever `a[i]` does *not* match any byte in `b`. A single instruction thus replaces a series of equality comparisons and OR-reductions that would otherwise be needed. In the code below, `op_chars` holds the 8 structural JSON characters and `ws_chars` holds the 4 whitespace characters; calling `svmatch_u8` once on a 16-byte chunk `d0` produces a predicate that has a `true` bit exactly where that input byte is a structural character. The code uses SVE2 intrinsics: compiler-provided C/C++ functions that map almost one-to-one to CPU SIMD instructions, so you get near-assembly control without writing assembly.


```cpp
// : , [ ] { }
uint8_t op_chars_data[16] = {
    0x3a, 0x2c, 0x5b, 0x5d, 0x7b, 0x7d, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0
};
// \t \n \r ' '
uint8_t ws_chars_data[16] = {
    0x09, 0x0a, 0x0d, 0x20, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0
};

// load the characters in SIMD registers
svuint8_t op_chars = svld1_u8(svptrue_b8(), op_chars_data);
svuint8_t ws_chars = svld1_u8(svptrue_b8(), ws_chars_data);

// load data
// const char * input = ...
svbool_t pg = svptrue_pat_b8(SV_VL16);
svuint8_t d = svld1_u8(pg, input);

// matching
svbool_t op = svmatch_u8(pg, d, op_chars);
svbool_t ws = svmatch_u8(pg, d, ws_chars);
```

In this code snippet, `svuint8_t` is an SVE vector type containing unsigned 8-bit lanes (bytes). `svbool_t` is an SVE predicate (mask) type. `svptrue_b8()` builds a predicate where all 8-bit lanes are active, and `svld1_u8(pg, ptr)` loads bytes from memory into an SVE vector, using predicate `pg` to decide which lanes are actually read.

If you paid attention thus far, you might have noticed that my code is slightly wrong since I am including 0 in the character sets. But it is fine as long as I assume that the zero byte is not present in the input. In practice, I could just repeat one of the characters, or use a bogus character that I do not expect to see in my inputs (such as the byte value `0xFF`, which cannot appear in a valid UTF-8 string).


In standard SVE/SVE2, `op` and `ws` are predicates, not integer masks. A practical trick is to materialize each predicate as bytes (`0xFF` for true, `0x00` for false), for example with `svdup_n_u8_z`.

```cpp
svuint8_t opm = svdup_n_u8_z(op, 0xFF);
svuint8_t wsm = svdup_n_u8_z(ws, 0xFF);
```

When SVE vectors are 128 bits, this byte vector maps naturally to a NEON `uint8x16_t` via `svget_neonq_u8`, and from there we can build scalar bitmasks efficiently with NEON operations (masking plus pairwise additions). Repeating this over four 16-byte chunks gives the two 64-bit masks needed for a 64-byte block.

How does it compare with pure NEON code? I compiled the routine processing blocks of 64 bytes using different compilers.

| method  |  GCC 16 | LLVM clang 20 |
|---------|---------|---------------|
| simdjson (NEON) | 69 | 66 |
| SVE/SVE2 (new!) |  42   | 52 |

Interestingly, GCC 16 even adopts SVE instructions in the pure NEON code, which suggests that recompiling old NEON code while targeting SVE/SVE2 could be beneficial.

I was hoping to test both compilers in a benchmark, but I wanted to quickly run my benchmarks on an AWS Graviton 4. I also did not want to compile GCC 16 from source. So I just kept LLVM clang 20 which was readily available in the images that AWS makes available (I picked RedHat 10).

The AWS Graviton 4 processor is a Neoverse V2 processor. Google has its own Neoverse V2 processors in its cloud. In my tests, it ran at 2.8 GHz.

My benchmark generates a random string of 1 MiB and computes the bitmaps indicating the positions of the characters. [It is available on GitHub](https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/tree/master/2026/04/17/benchmark). My results are as follow.


| method  |  GB/s | instructions/byte | instructions/cycle |
|---------|---------|---------------|-----------------------|
| simdjson (NEON) | 11.4| 0.94 | 3.5 |
| SVE/SVE2 (new!) | 14.4   | 0.67 | 3.8 |

So the SVE/SVE2 approach is about 25% faster than the NEON equivalent, and that's without any kind of fancy optimization. Importantly, the code is significantly simpler: the simdjson approach is non-trivial in comparison.

It might be that the SVE2 function `match` is the fastest way to match characters on ARM processors.


*Credit*: This post was motivated by a sketch by user `liuyang-664` on GitHub.

## References

Langdale, G., & Lemire, D. (2019). [Parsing gigabytes of JSON per second](https://doi.org/10.1002/spe.3396). The VLDB Journal, 28(6), 941-960.

Koekkoek, J., & Lemire, D. (2025). [Parsing millions of DNS records per second](https://doi.org/10.1002/spe.3396). Software: Practice and Experience, 55(4), 778-788.

Lemire, D. (2025). [Scanning HTML at Tens of Gigabytes Per Second on ARM Processors](https://doi.org/10.1002/spe.3420). Software: Practice and Experience, 55(7), 1256-1265.