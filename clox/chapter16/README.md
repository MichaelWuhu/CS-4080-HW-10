# Chapter 16 scanner

Build from this directory with:

```bash
cc -std=c11 -Wall -Wextra -pedantic -O2 main.c scanner.c -o scanner
./scanner 'print 1 + 2; // comment'
```

This code implements the Chapter 16 scanner: whitespace and comments,
punctuation, strings, numbers, identifiers, keywords, line tracking, errors,
and EOF handling.
