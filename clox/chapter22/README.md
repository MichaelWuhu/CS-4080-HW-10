# Chapter 22: Local Variables

Build from this directory:

```bash
cc -std=c11 -Wall -Wextra -pedantic -O2 \
  main.c compiler.c scanner.c chunk.c debug.c value.c memory.c object.c table.c vm.c \
  -o clox
```

Run a program containing local variables:

```bash
./clox 'var a = "global"; { var a = "local"; print a; } print a;'
```

This chapter adds lexical block scopes, compiler-resolved local variables,
stack slots for local values, local assignment, and initialization checks.
Local access uses `OP_GET_LOCAL` and `OP_SET_LOCAL` instead of a global hash
table lookup.

## Challenge 1

I would use a compiler-side hash table that maps each variable name to its
local slot. This would make resolving a local variable close to constant time
instead of scanning every local in the current scope. The tradeoff is extra
compiler memory and implementation complexity. For large scopes, the faster
lookup is probably worth it.

## Challenge 2

For `var a = a;`, I would report a compile error because the new local variable
is not initialized yet. This is safer and less confusing than accidentally
using a variable from an outer scope. Some languages allow the outer variable
to be used, but requiring an explicit name or initialization order makes the
behavior clearer.

## Challenge 3

See GitHub.

The implementation adds `const` variables. Assigning to a local constant is a
compile error, and assigning to a global constant is rejected at runtime.

## Challenge 4

See GitHub.

Local variables are stored in a dynamically growing compiler array, and local
variable bytecode operands use two bytes instead of one. This allows more than
256 locals to exist in one scope.

## Tests

```bash
./clox 'var a = 10; { var b = 20; print a + b; }'
./clox 'var a = 10; { a = a + 5; print a; }'
./clox 'const a = 1; a = 2;'
```

The last command intentionally reports an error.
