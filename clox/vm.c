#include <stdio.h>

#include "memory.h"
#include "vm.h"

static VM vm;

void initVM(void) {
  vm.chunk = NULL;
  vm.ip = NULL;
  vm.stack = NULL;
  vm.stackTop = NULL;
  vm.stackCapacity = 0;
}

void freeVM(void) {
  reallocate(vm.stack, sizeof(Value) * vm.stackCapacity, 0);
  initVM();
}

static void resetStack(void) {
  vm.stackTop = vm.stack;
}

static void push(Value value) {
  int count = vm.stack == NULL ? 0 : (int)(vm.stackTop - vm.stack);
  if (count + 1 > vm.stackCapacity) {
    int oldCapacity = vm.stackCapacity;
    vm.stackCapacity = GROW_CAPACITY(oldCapacity);
    vm.stack = (Value*)reallocate(vm.stack,
        sizeof(Value) * oldCapacity,
        sizeof(Value) * vm.stackCapacity);
    vm.stackTop = vm.stack + count;
  }
  *vm.stackTop++ = value;
}

static Value pop(void) {
  return *--vm.stackTop;
}

static InterpretResult run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() \
  (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) do { \
    Value right = pop(); \
    vm.stackTop[-1] = vm.stackTop[-1] op right; \
  } while (false)

  for (;;) {
    uint8_t instruction = READ_BYTE();
    switch (instruction) {
      case OP_CONSTANT:
        push(READ_CONSTANT());
        break;
      case OP_CONSTANT_LONG:
        {
          uint32_t index = READ_BYTE();
          index |= (uint32_t)READ_BYTE() << 8;
          index |= (uint32_t)READ_BYTE() << 16;
          push(vm.chunk->constants.values[index]);
        }
        break;
      case OP_ADD:
        BINARY_OP(+);
        break;
      case OP_SUBTRACT:
        BINARY_OP(-);
        break;
      case OP_MULTIPLY:
        BINARY_OP(*);
        break;
      case OP_DIVIDE:
        BINARY_OP(/);
        break;
      case OP_NEGATE:
        vm.stackTop[-1] = -vm.stackTop[-1];
        break;
      case OP_RETURN:
        printf("result: ");
        printf("%g\n", pop());
        return INTERPRET_OK;
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = chunk->code;
  resetStack();
  return run();
}
