
Under Linux and macOS, you may run:

```
cmake -B build
cmake --build build
./build/benchmark
```

## Assembly Implementations

All assembly implementations count occurrences of `'!'` in a string using AArch64 (ARM64) NEON SIMD instructions. They differ in how many bytes they process per loop iteration, how many accumulators they use, and how they issue loads.

### 1. `count_classic` (C++ standard library)

```cpp
c += std::count(str.begin(), str.end(), '!');
```

Calls the standard library `std::count`. The compiler is free to auto-vectorize this. On Apple Clang/libc++, this typically produces NEON-vectorized code internally.

### 2. `count_assembly_claude` (NEON, 16 bytes/iteration, 1 accumulator)

**Setup:**
| Instruction | What it does |
|------------|--------------|
| `dup v1.16b, %w[ch]` | Broadcasts the character `'!'` (0x21) into all 16 byte lanes of NEON register v1. v1 = `[!,!,!,!,!,!,!,!,!,!,!,!,!,!,!,!]` |
| `movi v2.16b, #0` | Zeros out v2, which will accumulate per-lane match counts (16 independent byte counters). |
| `mov x0, %[data]` | Copies the string's data pointer into x0 (our read cursor). |
| `mov x1, %[len]` | Copies the string length into x1 (bytes remaining). |
| `cmp x1, #16` | Check if we have at least 16 bytes to process. |
| `b.lt 2f` | If fewer than 16 bytes, skip the NEON loop entirely. |

**Main NEON loop (16 bytes per iteration):**
| Instruction | What it does |
|------------|--------------|
| `ld1 {v0.16b}, [x0], #16` | Loads 16 bytes from memory at x0 into v0, then advances x0 by 16. |
| `cmeq v0.16b, v0.16b, v1.16b` | Compares each of the 16 bytes in v0 against `'!'` in v1. Each lane becomes `0xFF` if equal, `0x00` if not. |
| `sub v2.16b, v2.16b, v0.16b` | Subtracts v0 from the accumulator v2. Since matching lanes hold `0xFF` = -1 in signed byte arithmetic, subtracting -1 adds 1. Non-matching lanes (0x00) leave the accumulator unchanged. |
| `sub x1, x1, #16` | Decrease remaining byte count by 16. |
| `cmp x1, #16` | Check if 16 or more bytes remain. |
| `b.ge 1b` | If yes, loop back. |

**Horizontal sum (reduce 16 byte-counters to one scalar):**
| Instruction | What it does |
|------------|--------------|
| `uaddlv h3, v2.16b` | **Unsigned Add Long across Vector.** Sums all 16 bytes in v2 into a single 16-bit halfword in h3 (the low 16 bits of v3). |
| `umov w2, v3.h[0]` | Extracts that 16-bit integer from NEON register v3 lane 0 into the general-purpose register w2. |

**Scalar tail (processes remaining 0-15 bytes one at a time):**
| Instruction | What it does |
|------------|--------------|
| `cbz x1, 4f` | If zero bytes remain, jump to the end. |
| `ldrb w3, [x0], #1` | Load one byte from x0 into w3, advance x0 by 1. |
| `cmp w3, %w[ch]` | Compare the byte against `'!'`. |
| `cinc w2, w2, eq` | **Conditional Increment.** If the comparison was equal, w2 = w2 + 1. Otherwise w2 is unchanged. Branchless -- avoids a branch misprediction penalty. |
| `sub x1, x1, #1` | Decrease remaining count. |
| `b 3b` | Loop back. |
| `mov %w[out], w2` | Write the final count to the C++ output variable. |

**Bottleneck:** The single accumulator v2 creates a dependency chain -- each `sub v2, v2, v0` must wait for the previous one to complete before the next iteration's `sub` can execute.

### 3. `count_assembly_grok` (NEON, 32 bytes/iteration, 1 accumulator)

**Setup:** Identical to `count_assembly_claude`, except it checks for `x1 >= 32` before entering the main loop.

**Main NEON loop (32 bytes per iteration):**
| Instruction | What it does |
|------------|--------------|
| `ld1 {v0.16b}, [x0], #16` | Load first 16 bytes, advance pointer. |
| `cmeq v0.16b, v0.16b, v1.16b` | Compare against `'!'`. |
| `sub v2.16b, v2.16b, v0.16b` | Accumulate matches into v2. |
| `ld1 {v0.16b}, [x0], #16` | Load *next* 16 bytes, advance pointer. |
| `cmeq v0.16b, v0.16b, v1.16b` | Compare against `'!'`. |
| `sub v2.16b, v2.16b, v0.16b` | Accumulate into the *same* v2. |
| `sub x1, x1, #32` | Decrease remaining by 32. |
| `cmp`/`b.ge` | Loop if 32+ bytes remain. |

**Remainder 16-byte block:** If 16-31 bytes were left after the main loop, process one more 16-byte chunk with the same load/cmeq/sub pattern.

**Horizontal sum + scalar tail:** Identical to `count_assembly_claude`.

**Improvement over claude:** Processes twice as many bytes per loop iteration, halving the loop overhead (the `sub x1`/`cmp`/`b.ge` instructions execute half as often). **Limitation:** Still uses a single accumulator v2, so the two `sub v2, v2, v0` instructions are strictly sequential -- the second depends on the result of the first.

### 4. `count_assembly_claude_2` (NEON, 64 bytes/iteration, 4 accumulators, `ldp`)

**Setup:**
| Instruction | What it does |
|------------|--------------|
| `dup v1.16b, %w[ch]` | Broadcast `'!'` into v1. |
| `movi v2.16b, #0` | Zero accumulator 0. |
| `movi v4.16b, #0` | Zero accumulator 1. |
| `movi v7.16b, #0` | Zero accumulator 2. |
| `movi v16.16b, #0` | Zero accumulator 3. |
| `mov x0`/`mov x1` | Load data pointer and length. |

**Main loop (64 bytes per iteration):**
| Instruction | What it does |
|------------|--------------|
| `ldp q0, q3, [x0]` | **Load Pair of Quadwords.** Loads 32 bytes (2 x 128-bit registers) from `x0` in a single instruction. q0 gets bytes 0-15, q3 gets bytes 16-31. The pointer is NOT advanced (no writeback). |
| `ldp q5, q6, [x0, #32]` | Load the next 32 bytes from offset +32. q5 gets bytes 32-47, q6 gets bytes 48-63. |
| `add x0, x0, #64` | Advance the read pointer by 64. (Separated from the loads to keep the `ldp` instructions simple and pipelineable.) |
| `cmeq v0.16b, v0.16b, v1.16b` | Compare chunk 0 against `'!'`. |
| `cmeq v3.16b, v3.16b, v1.16b` | Compare chunk 1. |
| `cmeq v5.16b, v5.16b, v1.16b` | Compare chunk 2. |
| `cmeq v6.16b, v6.16b, v1.16b` | Compare chunk 3. |
| `sub v2.16b, v2.16b, v0.16b` | Accumulate chunk 0 matches into accumulator 0 (v2). |
| `sub v4.16b, v4.16b, v3.16b` | Accumulate chunk 1 matches into accumulator 1 (v4). |
| `sub v7.16b, v7.16b, v5.16b` | Accumulate chunk 2 matches into accumulator 2 (v7). |
| `sub v16.16b, v16.16b, v6.16b` | Accumulate chunk 3 matches into accumulator 3 (v16). |
| `sub`/`cmp`/`b.ge` | Decrease remaining by 64, loop if 64+ bytes remain. |

**Key design choices:**
- All 4 loads are issued before any compare, giving memory latency time to resolve before the data is consumed.
- Each of the 4 `sub` instructions writes to a *different* accumulator (v2, v4, v7, v16). No data dependency between them -- the CPU can execute all 4 in parallel or in any order.

**16-byte remainder loop:** Handles 16-63 leftover bytes with single-register load/cmeq/sub into v2.

**Merge accumulators + horizontal sum:**
| Instruction | What it does |
|------------|--------------|
| `add v2.16b, v2.16b, v4.16b` | Merge accumulator 1 into accumulator 0. |
| `add v7.16b, v7.16b, v16.16b` | Merge accumulator 3 into accumulator 2. |
| `add v2.16b, v2.16b, v7.16b` | Merge into one final accumulator. (Done as a tree -- 2 adds in parallel, then 1 -- rather than a chain of 3.) |
| `uaddlv h3, v2.16b` | Sum all 16 bytes to one 16-bit value. |
| `umov w2, v3.h[0]` | Extract to scalar register. |

**Scalar tail:** Identical to the others.

### 5. `count_assembly_grok_2` (NEON, 64 bytes/iteration, 4 accumulators, `ldp` with writeback)

**Setup:** Identical to `count_assembly_claude_2`.

**Main loop (64 bytes per iteration):**
| Instruction | What it does |
|------------|--------------|
| `ldp q0, q3, [x0], #32` | Load 32 bytes into q0 and q3. **Post-index writeback**: x0 is automatically advanced by 32 after the load. |
| `ldp q5, q6, [x0], #32` | Load next 32 bytes (from the already-advanced x0) into q5 and q6. x0 advances by another 32. Total: x0 moved by 64. |

The rest of the loop body (cmeq x 4, sub x 4, loop control) is identical to `count_assembly_claude_2`.

**Difference from claude_2:** Uses post-index writeback (`[x0], #32`) instead of offset loads + explicit `add`. This saves one instruction per iteration (the `add x0, x0, #64`) but creates a dependency between the two `ldp` instructions -- the second `ldp` needs the updated x0 from the first. In `claude_2`, both loads can be issued simultaneously since they read x0 at different offsets without modifying it.

**Everything else** (16-byte remainder, accumulator merge, horizontal sum, scalar tail) is identical to `count_assembly_claude_2`.

### 6. `count_assembly_claude_3` (NEON, 64 bytes/iteration, 4 accumulators, `ld1` multi-register + `subs`)

**Setup:** Identical to `count_assembly_claude_2`.

**Main loop (64 bytes per iteration, 11 instructions):**
| Instruction | What it does |
|------------|--------------|
| `ld1 {v17.16b, v18.16b, v19.16b, v20.16b}, [x0], #64` | **Multi-register Load.** Loads 64 bytes (4 x 128-bit registers) in a single instruction and auto-increments x0 by 64. Replaces the 2 `ldp` + 1 `add` (3 instructions) from claude_2. |
| `cmeq v17.16b, v17.16b, v1.16b` | Compare chunk 0 against `'!'`. |
| `cmeq v18.16b, v18.16b, v1.16b` | Compare chunk 1. |
| `cmeq v19.16b, v19.16b, v1.16b` | Compare chunk 2. |
| `cmeq v20.16b, v20.16b, v1.16b` | Compare chunk 3. |
| `sub v2.16b, v2.16b, v17.16b` | Accumulate chunk 0 matches into accumulator 0. |
| `sub v4.16b, v4.16b, v18.16b` | Accumulate chunk 1 matches into accumulator 1. |
| `sub v7.16b, v7.16b, v19.16b` | Accumulate chunk 2 matches into accumulator 2. |
| `sub v16.16b, v16.16b, v20.16b` | Accumulate chunk 3 matches into accumulator 3. |
| `subs x1, x1, #64` | **Subtract and Set flags.** Decreases remaining count by 64 AND sets the condition flags in one instruction. Replaces the separate `sub` + `cmp` (2 instructions) from claude_2. |
| `b.ge 10b` | Loop while remaining >= 0 (meaning there were 64+ bytes before the subtract). |

**Loop entry uses pre-subtraction:**
| Instruction | What it does |
|------------|--------------|
| `subs x1, x1, #64` | Pre-subtract 64 from the length before entering the loop. If the result is negative, skip the main loop entirely. |
| `b.lt 20f` | Branch to remainder handling if fewer than 64 bytes. |

After the main loop, `adds x1, x1, #64` restores the true remainder (0-63 bytes).

**Key improvements over claude_2 and grok2:**
- The `ld1` multi-register instruction replaces 3 instructions (`ldp` + `ldp` + `add`) with 1, reducing the loop body from 14 to 11 instructions per 64 bytes.
- Unlike grok2's post-index `ldp` pair, there is no serialization between loads -- the single `ld1` instruction handles all 64 bytes and the pointer update atomically.
- `subs` combines the subtract and compare into one instruction, eliminating another instruction per iteration.

**16-byte remainder + merge + scalar tail:** Same as claude_2, also using `subs` for tighter loop control.

### Summary of differences

| Version | Bytes/iter | Accumulators | Load strategy | Loop body insns | Key limitation |
|---------|-----------|-------------|---------------|-----------------|----------------|
| claude | 16 | 1 (v2) | `ld1` x 1 | 6 | Accumulator dependency chain; high loop overhead |
| grok | 32 | 1 (v2) | `ld1` x 2 | 9 | Accumulator dependency chain (serial `sub`s) |
| claude_2 | 64 | 4 (v2,v4,v7,v16) | `ldp` + offset, both loads independent | 14 | Extra `add` for pointer advance |
| grok2 | 64 | 4 (v2,v4,v7,v16) | `ldp` + post-index writeback | 13 | Second `ldp` depends on first's writeback |
| claude_3 | 64 | 4 (v2,v4,v7,v16) | `ld1` multi-register (single insn for 64B) + `subs` | 11 | -- |

