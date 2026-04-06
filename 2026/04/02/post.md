# Can your AI rewrite your code in assembly?


Suppose you have several strings and you want to count the number of instances of the character `!` in your strings. In C++, you might solve the problem as follows if you are an old-school programmer.


```cpp
size_t c = 0;
for (const auto &str : strings) {
    c += std::count(str.begin(), str.end(), '!');
}
```

You can also get fancier with ranges.

```cpp
for (const auto &str : strings) {
    c += std::ranges::count(str, '!');
}
```

And so forth.

But what if you want to go faster? Maybe you'd want to rewrite this function in assembly. I decided to do so, and to have fun using both Grok and Claude as my AIs, setting up a friendly competition.

I started with my function and then I asked AIs to optimize it in assembly. Importantly, they knew which machine I was on, so they started to write ARM assembly.

By repeated prompting, I got the following functions.

- `count_classic`: Uses C++ standard library `std::count` for reference.
- `count_assembly`: A basic ARM64 assembly loop (byte-by-byte comparison). Written by Grok.
- `count_assembly_claude`: Claude's SIMD-optimized version using NEON instructions (16-byte chunks).
- `count_assembly_grok`: Grok's optimized version (32-byte chunks).
- `count_assembly_claude_2`: Claude's further optimized version (64-byte chunks with multiple accumulators).
- `count_assembly_grok_2`: Grok's latest version (64-byte chunks with improved accumulator handling).
- `count_assembly_claude_3`: Claude's most advanced version with additional optimizations.

You get the idea.

So, how is the performance? I use random strings of up to 1 kilobyte. In all cases, I test that the functions provide the correct count. I did not closely examine the code, so it is possible that mistakes could be hiding in the code.

I record the average number of instructions per string.

| name | instructions/string |
|------|---------------------|
|classic C++ | 1200 |
|claude assembly | 250 |
|grok assembly | 204 |
|claude assembly 2 | 183 |
|grok assembly 2 | 176 |
|claude assembly 3 | 154 |

By repeated optimization, I reduced the number of instructions by a factor of eight. The running time decreases similarly.

Can we get the AIs to rewrite the best option in C? Yes, although you need SIMD intrinsics. So there is no benefit to leaving the code in assembly in this instance.

An open question is whether the AIs could find optimizations that are not possible if we use a higher-level language like C or C++. It is an intriguing question that I will seek to answer later. For the time being, the AIs can beat my C++ compiler!