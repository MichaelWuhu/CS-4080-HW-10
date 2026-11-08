# Chapter 21: Global Variables

Build from this directory:

```bash
cc -std=c11 -Wall -Wextra -pedantic -O2 \
  main.c compiler.c scanner.c chunk.c debug.c value.c memory.c object.c table.c vm.c \
  -o clox
```

Run a program containing global variables:

```bash
./clox 'var count = 1; print count; count = count + 2; print count;'
```

This chapter adds variable declarations, global reads, global assignments,
`print` statements, and bytecode instructions for defining, getting, and
setting globals. Global values are stored in the hash table from Chapter 20.

## Challenge 1

I would keep a compiler-side hash table that maps each interned variable name
to the constant-table index already assigned to it. When the same identifier is
used again, the compiler reuses that index instead of adding another string
constant. This makes compilation slightly more complex and uses extra memory
while compiling, but it reduces constant-table usage and does not add runtime
work. Since the constant limit is per compiled chunk, this is a worthwhile
trade-off.

## Challenge 2

I would assign each global variable a slot in a global-value array and compile
known global names to that slot. Access would then be an array lookup instead
of a hash-table lookup. Because Lox supports late binding and the REPL can
define globals later, the compiler would still need a name-to-slot table for
unknown names. Once a name receives a slot, later accesses could use the slot
directly. If a global is redefined, its existing slot would simply be updated.

## Challenge 3

I would keep unknown global variables as runtime errors instead of reporting
them as compile errors. Lox supports late binding, which allows a function to
refer to a global that is declared later. Reporting the error during
compilation would break that behavior and make REPL use more difficult. The
runtime error still catches the problem if the code actually tries to use the
undefined variable. Many scripting languages, including JavaScript and
Python, also allow names to be resolved at runtime in at least some situations.

## Tests

```bash
./clox 'var a = 10; print a;'
./clox 'var a = 10; a = a + 5; print a;'
./clox 'var message = "hello"; print message + " world";'
./clox 'print missing;'
```

The last command intentionally reports an undefined-variable runtime error.
