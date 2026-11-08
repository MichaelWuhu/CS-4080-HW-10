# Chapter 17: Compiling Expressions

Build from this directory:

```bash
cc -std=c11 -Wall -Wextra -pedantic -O2 \
  main.c compiler.c scanner.c chunk.c debug.c value.c memory.c vm.c -o clox
./clox "1 + 2 * 3"
```

This implementation connects scanning, Pratt parsing, bytecode compilation,
and VM execution for number literals, grouping, unary negation, and the four
arithmetic operators.
