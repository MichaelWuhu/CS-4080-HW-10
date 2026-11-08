# Chapter 19: Strings

Build from this directory:

```bash
cc -std=c11 -Wall -Wextra -pedantic -O2 \
  main.c compiler.c scanner.c chunk.c debug.c value.c memory.c object.c vm.c \
  -o clox
./clox '"hello, " + "world!"'
```

This implementation adds string literals, string equality, string
concatenation, and runtime type checking for `+`. It also implements the first
two chapter challenges:

1. `ObjString` uses a flexible array member. Owned string characters are stored
   immediately after the object header in one allocation, so the object and
   its character data do not require separate allocations.
2. String literals are non-owning constant strings. Their characters point
   directly into the source buffer, while concatenated strings own their
   character data. The `ownsChars` flag tells the runtime which representation
   is being used, so source characters are never freed.

## Challenge 3

I would allow a string to be combined with any value using `+`, converting the
other value to its printable string form. For example, `"score: " + 10`
would produce `"score: 10"`. This is convenient for output and is similar to
JavaScript's implicit string conversion, while Python normally requires an
explicit `str()` call for `+`. The tradeoff is that
implicit conversion can hide mistakes, such as accidentally adding a number to
text. If strict type safety were the priority, I would keep the current runtime
error and require an explicit conversion function.

## Tests

```bash
./clox '"hello"'
./clox '"hello, " + "world"'
./clox '"same" == "same"'
./clox '"number: " + 123'
./clox '1 + 2'
```

The fourth command intentionally reports a runtime error because this version
keeps the strict behavior described in the chapter.
