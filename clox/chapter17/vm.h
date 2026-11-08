#ifndef CLOX_CHAPTER17_VM_H
#define CLOX_CHAPTER17_VM_H

typedef enum { INTERPRET_OK, INTERPRET_COMPILE_ERROR, INTERPRET_RUNTIME_ERROR } InterpretResult;

void initVM(void);
void freeVM(void);
InterpretResult interpret(const char* source);

#endif
