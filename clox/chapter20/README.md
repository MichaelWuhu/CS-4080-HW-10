# Chapter 20: Hash Tables

Build from this directory:

```bash
cc -std=c11 -Wall -Wextra -pedantic -O2 \
  main.c compiler.c scanner.c chunk.c debug.c value.c memory.c object.c table.c vm.c \
  -o clox
./clox '\"hello, \" + \"world!\"'
```

This implementation adds an open-addressed hash table with linear probing,
tombstones, automatic growth, FNV-1a string hashing, and string interning. It
also supports primitive values as keys for Challenge 1.

## Challenge 1

See GitHub.

## Challenge 2

I chose open addressing with linear probing because it uses one contiguous
array, has good cache locality, and is simple to implement. The table grows at
a 75% load factor, which limits long probe sequences. Tombstones preserve
lookup chains after deletion. Separate chaining would make deletion simpler,
but it needs linked-list allocations and usually has worse cache locality.
FNV-1a is fast and distributes ordinary strings reasonably well, while a
cryptographic hash would be unnecessary overhead for this interpreter.

## Challenge 3

See GitHub.

Build the benchmark from this directory with:

```bash
cc -std=c11 -Wall -Wextra -pedantic -O2 \
  benchmarks/hash_benchmark.c table.c object.c value.c memory.c -o hash_benchmark
./hash_benchmark
```

The table API can also be used directly from C with `tableSet`, `tableGet`,
and `tableDelete`.
