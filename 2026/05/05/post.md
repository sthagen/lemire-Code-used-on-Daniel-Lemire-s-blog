# Mapping Strings to Float Arrays in Go: How Fast Can We Go?

A common pattern in modern software is to map a string key to a small array of floating-point numbers. Word embeddings, feature vectors, lookup tables for physical constants: all variations on the same theme. In Go, the obvious way to write this is a `map[string][]float32`. But how fast is it, really, and can we do better?

I have been working on [constmap](https://github.com/lemire/constmap), a Go library that builds an immutable map from strings to `uint64` values using the binary fuse filter construction. A lookup amounts to one hash, three array reads, and two XORs. There is no comparison, no chaining, no probing. The whole table fits in roughly 9 bytes per key, which often means it fits in cache where a Go map does not.

Go has fast maps, you cannot easily beat them in performance. But if you build a smaller data structure that causes fewer cache misses, you can definitively go faster.

By default, the constmap returns a `uint64`. But what if your value is an array of eight `float32` numbers? You have at least two options:

1. Keep the arrays in a separate slice `[][]float32`. The constmap returns the index.
2. Store a pointer to the float array directly inside the constmap's `uint64`.

The second option requires the `unsafe` package because we are smuggling a pointer through an integer field. It has some limitations. 

- You cannot and should not deserialize the data structure to disk.
- You must make sure that a reference remains to your float array, or else the garbage collector could collect it and you'd be left with a dangling pointer. It is trickier than it sounds because Go can collect your memory if it sees that it is no longer used. And it cannot see through your `unsafe` calls converting an integer to a pointer value. Thankfully, you can just put all your arrays of floats in an array and call `runtime.KeepAlive(mybigarray)` at a strategic location: this will prevent Go from collecting `mybigarray`. The call to `runtime.KeepAlive` is not free but also quite cheap so you can possibly use a lot of such calls. benchmark

I built three lookups over 100,000 keys, each mapping to an 8-element `[]float32`. We always access the first element of the array, to make sure that the bencmark is a bit fair. We have a large set of random queries (a query is a string).

We compare `map[string][]float32`, the standard `constmap` coupled with an array (so that the `constmap` constains indexes), and the `constmap`  that contains what is effectively a pointer to the location of the `[]float32`.

Run on an Apple M4 Max with Go's standard benchmark harness:

| Lookup                              | Time per op |
|-------------------------------------|-------------|
| `map[string][]float32`              | 21 ns     |
| ConstMap → index → `[][]float32`    |  11 ns     |
| ConstMap → pointer → `*[8]float32`  |  8.7 ns    |

The constmap with an index is already a twice as fast as the Go map. Replacing the index by a raw pointer shaves another 2 ns by skipping the indirection through the `[][]float32` slice header. It is a speedup of about 20% in my case.


The result is interesting on its own: a constmap lookup is fast enough that the *next* memory load, the slice header read, becomes a measurable fraction of the work. 

The benchmark and code are in [github.com/lemire/constmap](https://github.com/lemire/constmap). Run them with:

```
go test -bench 'FloatArray' -benchtime=1s
```

