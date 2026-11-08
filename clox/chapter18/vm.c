#include <stdio.h>
#include <stdarg.h>
#include "compiler.h"
#include "memory.h"
#include "vm.h"

typedef struct {
  Chunk* chunk;
  uint8_t* ip;
  Value* stack;
  Value* stackTop;
  int stackCapacity;
} VM;
static VM vm;

void initVM(void) {
  vm.chunk = NULL; vm.ip = NULL; vm.stack = NULL; vm.stackTop = NULL; vm.stackCapacity = 0;
}
void freeVM(void) {
  reallocate(vm.stack, sizeof(Value) * vm.stackCapacity, 0);
  initVM();
}
static void resetStack(void) { vm.stackTop = vm.stack; }
static void push(Value value) {
  int count = vm.stack == NULL ? 0 : (int)(vm.stackTop - vm.stack);
  if (count + 1 > vm.stackCapacity) {
    int oldCapacity = vm.stackCapacity;
    vm.stackCapacity = GROW_CAPACITY(oldCapacity);
    vm.stack = (Value*)reallocate(vm.stack, sizeof(Value) * oldCapacity,
                                  sizeof(Value) * vm.stackCapacity);
    vm.stackTop = vm.stack + count;
  }
  *vm.stackTop++ = value;
}
static Value pop(void) { return *--vm.stackTop; }
static Value peek(int distance) { return vm.stackTop[-1 - distance]; }
static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);
  resetStack();
}
static InterpretResult run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(valueType, op) do { \
  if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
    runtimeError("Operands must be numbers."); \
    return INTERPRET_RUNTIME_ERROR; \
  } \
  double b = AS_NUMBER(pop()); double a = AS_NUMBER(pop()); \
  push(valueType(a op b)); \
} while (false)
  for (;;) {
    uint8_t instruction = READ_BYTE();
    switch (instruction) {
      case OP_CONSTANT: push(READ_CONSTANT()); break;
      case OP_NIL: push(NIL_VAL); break;
      case OP_TRUE: push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
      case OP_EQUAL: { Value b = pop(); Value a = pop(); push(BOOL_VAL(valuesEqual(a, b))); break; }
      case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
      case OP_ADD: BINARY_OP(NUMBER_VAL, +); break;
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
      case OP_NOT: { Value value = pop(); push(BOOL_VAL(IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value)))); break; }
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) { runtimeError("Operand must be a number."); return INTERPRET_RUNTIME_ERROR; }
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
      case OP_RETURN: printValue(pop()); putchar('\n'); return INTERPRET_OK;
      default: return INTERPRET_RUNTIME_ERROR;
    }
  }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}
InterpretResult interpret(const char* source) {
  Chunk chunk;
  initChunk(&chunk);
  if (!compile(source, &chunk)) { freeChunk(&chunk); return INTERPRET_COMPILE_ERROR; }
  vm.chunk = &chunk; vm.ip = chunk.code; resetStack();
  InterpretResult result = run();
  freeChunk(&chunk);
  return result;
}
