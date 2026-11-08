# Chapter 14: Chunks of Bytecode

This folder contains a small, buildable `clox` chunk implementation for all
three end-of-chapter challenges. It is separate from the Java `jlox` work in the
repository because Chapter 14 begins the book's C bytecode interpreter.

## Build and run

From the repository root:

```bash
cc -std=c11 -Wall -Wextra -pedantic -O2 \
  clox/main.c clox/chunk.c clox/debug.c clox/value.c clox/memory.c \
  -o clox/clox
./clox/clox
```

The test driver creates 300 constants, so constants with indexes 256 and above
must use `OP_CONSTANT_LONG`. It also writes several instructions on repeated
source lines and checks the compressed line table through `getLine()`.

## Challenge 1

`Chunk.lines` stores `{offset, line}` pairs only when the source line changes.
For an instruction offset, `getLine()` scans backward to the most recent pair.
This is run-length encoding: repeated line numbers occupy one table entry
instead of one integer per bytecode byte.

## Challenge 2

`OP_CONSTANT` remains a two-byte instruction for indexes below 256. For larger
indexes, `writeConstant()` emits `OP_CONSTANT_LONG` followed by the index as
three little-endian bytes. The disassembler reconstructs the same 24-bit value.

This allows up to `2^24` constant indexes without making common constants
larger, at the cost of one additional opcode and a four-byte instruction in the
rare large-constant case.

## Challenge 3

`memory.c` implements the hardcore version of `reallocate()`. `initMemory()`
calls `malloc()` once to create a 1 MiB heap. Every block has a small header
recording its size and whether it is free. Allocation scans the heap for a
suitable free block and splits it when possible. Freeing marks the block free
and merges adjacent free blocks. Resizing allocates a replacement and copies
the old bytes when the existing block cannot grow in place.
