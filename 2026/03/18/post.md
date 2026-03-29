# How many branches can your CPU predict

Modern processors have the ability to execute many instructions per cycle, on a single core. To be able to execute many instructions per cycle in practice, processors predict branches. I have made the point over the years that modern CPUs have an incredible ability to [predict branches](https://lemire.me/blog/2019/10/16/benchmarking-is-hard-processors-learn-to-predict-branches/).

It makes benchmarking difficult because if you test on small datasets, you can get surprising results that might not work on real data.

My go-to benchmark is a function like so:

```
while (howmany != 0) {
    val = generate_random_value()
    if(val is odd) write to buffer
    decrement howmany
}
```

The processor tries to predict the branch (`if` clause). Because we use random values, the processor should mispredict one time out of two.

However, if we repeat multiple times the benchmark, always using the same random values, the processor learns the branches. How many can processors learn? I test using three recent processors.

- The AMD Zen 5 processor can predict perfectly 30,000 branches.
- The Apple M4 processor can predict perfectly 10,000 branches.
- Intel Emerald Rapids can predict perfectly 5,000 branches.


![](branch_mispredictions.png)

[My source code is available](https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/tree/master/2026/03/18/benchmark).