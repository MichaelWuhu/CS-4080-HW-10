#include <stdio.h>

#include "debug.h"

static int constantInstruction(const char* name, const Chunk* chunk,
                               int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-18s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

static int longConstantInstruction(const char* name, const Chunk* chunk,
                                   int offset) {
  uint32_t constant = (uint32_t)chunk->code[offset + 1] |
                      ((uint32_t)chunk->code[offset + 2] << 8) |
                      ((uint32_t)chunk->code[offset + 3] << 16);
  printf("%-18s %4u '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 4;
}

void disassembleChunk(const Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);
  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

int disassembleInstruction(const Chunk* chunk, int offset) {
  printf("%04d %4d ", offset, getLine(chunk, offset));
  switch (chunk->code[offset]) {
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_LONG:
      return longConstantInstruction("OP_CONSTANT_LONG", chunk, offset);
    case OP_ADD:
      puts("OP_ADD");
      return offset + 1;
    case OP_SUBTRACT:
      puts("OP_SUBTRACT");
      return offset + 1;
    case OP_MULTIPLY:
      puts("OP_MULTIPLY");
      return offset + 1;
    case OP_DIVIDE:
      puts("OP_DIVIDE");
      return offset + 1;
    case OP_NEGATE:
      puts("OP_NEGATE");
      return offset + 1;
    case OP_RETURN:
      puts("OP_RETURN");
      return offset + 1;
    default:
      printf("Unknown opcode %d\n", chunk->code[offset]);
      return offset + 1;
  }
}
