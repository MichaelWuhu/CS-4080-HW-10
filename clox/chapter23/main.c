#include <stdio.h>
#include "chunk.h"
#include "compiler.h"
#include "memory.h"
#include "vm.h"

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s \"expression\"\n", argv[0]);
    return 64;
  }
  initMemory();
  initVM();
  InterpretResult result = interpret(argv[1]);
  freeVM();
  return result == INTERPRET_OK ? 0 : 70;
}
