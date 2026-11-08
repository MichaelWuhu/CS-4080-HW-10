#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"

typedef enum {
  INTERPRET_OK,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

typedef struct {
  Chunk* chunk;
  uint8_t* ip;
  Value* stack;
  Value* stackTop;
  int stackCapacity;
} VM;

void initVM(void);
void freeVM(void);
InterpretResult interpret(Chunk* chunk);

#endif
