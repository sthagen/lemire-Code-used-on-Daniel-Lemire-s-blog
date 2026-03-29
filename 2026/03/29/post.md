# A Fast Immutable Map in Go

Consider the following problem. You have a large set of strings, maybe millions. You need to map these strings to 8-byte integers (`uint64`). These integers are given to you.

If you are working in Go, the standard solution is to create a map. The construction is trivial, something like the following loop.

```go
m := make(map[string]uint64, N)
for i, k := range keys {
    m[k] = values[i]
}
```

One downside is that the map may use over 50 bytes per entry.

In important scenarios, we might have the following conditions. The map is large (a million of entries or more), you do not need to modify it dynamically (it is immutable), and all queried keys are in the set. In such conditions, you can reduce the memory usage down to almost the size of the keys, so about 8 bytes per entry. One fast technique is the [binary fuse filters](https://arxiv.org/abs/2201.01174).


I implemented it as a Go library called [constmap](https://github.com/fastfilter/constmap) that provides an immutable map from strings to `uint64` values using binary fuse filters. This data structure is ideal when you have a fixed set of keys at construction time and need fast, memory-efficient lookups afterward. You can even construct the map once, save it to disk so you do not pay the cost of constructing the map each time you need it.

The usage is just as simple.

```go
package main

import (
	"fmt"
	"log"

	"github.com/fastfilter/constmap"
)

func main() {
	keys := []string{"apple", "banana", "cherry"}
	values := []uint64{100, 200, 300}

	cm, err := constmap.New(keys, values)
	if err != nil {
		log.Fatal(err)
	}

	fmt.Println(cm.Map("banana")) // 200
}
```

The construction time is higher (as expected for any compact data structure), but lookups are optimized for speed. I ran benchmarks on my Apple M4 Max processor to compare constmap lookups against Go's built-in `map[string]uint64`. The test uses 1 million keys.

| Data Structure | Lookup Time | Memory Usage |
|----------------|-------------|--------------|
| ConstMap       | 7.4 ns/op |  9 bytes/key   |
| Go Map         | 20 ns/op | 56 bytes/key    |

ConstMap is nearly 3 times faster than Go's standard map for lookups! And we reduced the memory usage by a factor of 6.


**Source Code** The implementation is available on GitHub: [github.com/fastfilter/constmap](https://github.com/fastfilter/constmap).

