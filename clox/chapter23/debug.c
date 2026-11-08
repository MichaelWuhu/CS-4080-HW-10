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

static int jumpInstruction(const char* name, int sign, const Chunk* chunk,
                            int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-18s %4d -> %d\n", name, offset,
         offset + 3 + sign * jump);
  return offset + 3;
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
    case OP_POP: puts("OP_POP"); return offset + 1;
    case OP_DUP: puts("OP_DUP"); return offset + 1;
    case OP_NIL: puts("OP_NIL"); return offset + 1;
    case OP_TRUE: puts("OP_TRUE"); return offset + 1;
    case OP_FALSE: puts("OP_FALSE"); return offset + 1;
    case OP_EQUAL: puts("OP_EQUAL"); return offset + 1;
    case OP_GREATER: puts("OP_GREATER"); return offset + 1;
    case OP_LESS: puts("OP_LESS"); return offset + 1;
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
    case OP_NOT:
      puts("OP_NOT");
      return offset + 1;
    case OP_PRINT:
      puts("OP_PRINT");
      return offset + 1;
    case OP_DEFINE_GLOBAL:
      return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_DEFINE_CONST_GLOBAL:
      return constantInstruction("OP_DEFINE_CONST_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL:
      return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
      return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_LOCAL: {
      uint16_t slot = (uint16_t)chunk->code[offset + 1] |
                      ((uint16_t)chunk->code[offset + 2] << 8);
      printf("%-18s %4d\n", "OP_GET_LOCAL", slot);
      return offset + 3;
    }
    case OP_SET_LOCAL: {
      uint16_t slot = (uint16_t)chunk->code[offset + 1] |
                      ((uint16_t)chunk->code[offset + 2] << 8);
      printf("%-18s %4d\n", "OP_SET_LOCAL", slot);
      return offset + 3;
    }
    case OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP:
      return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_RETURN:
      puts("OP_RETURN");
      return offset + 1;
    default:
      printf("Unknown opcode %d\n", chunk->code[offset]);
      return offset + 1;
  }
}
