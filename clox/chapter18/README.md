# Chapter 18: Types of Values

Build from this directory:

```bash
cc -std=c11 -Wall -Wextra -pedantic -O2 \
  main.c compiler.c scanner.c chunk.c debug.c value.c memory.c vm.c -o clox
./clox "!(5 - 4 > 3 * 2 == !nil)"
```

This implementation adds tagged values for numbers, Booleans, and `nil`,
runtime type checks, truthiness, equality, comparisons, logical not, and the
Chapter 17 expression compiler.
