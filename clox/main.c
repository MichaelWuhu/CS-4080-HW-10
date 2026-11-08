#include <stdio.h>

#include "chunk.h"
#include "debug.h"
#include "memory.h"
#include "vm.h"

int main(void) {
  initMemory();

  Chunk chunk;
  initChunk(&chunk);

  /* Same source line is compressed to one LineStart entry. */
  for (int i = 0; i < 300; i++) {
    writeConstant(&chunk, (double)i, i < 2 ? 10 : 20);
  }
  writeChunk(&chunk, OP_RETURN, 21);

  printf("constants: %d\n", chunk.constants.count);
  printf("line entries: %d\n", chunk.lineCount);
  printf("line[0] = %d, line[2] = %d, line[last] = %d\n",
         getLine(&chunk, 0), getLine(&chunk, 2),
         getLine(&chunk, chunk.count - 1));
  disassembleChunk(&chunk, "challenge test chunk");

  freeChunk(&chunk);

  Chunk calculation;
  initChunk(&calculation);
  writeConstant(&calculation, 1.2, 1);
  writeConstant(&calculation, 3.4, 1);
  writeChunk(&calculation, OP_ADD, 1);
  writeConstant(&calculation, 5.6, 1);
  writeConstant(&calculation, 2.0, 1);
  writeChunk(&calculation, OP_DIVIDE, 1);
  writeChunk(&calculation, OP_SUBTRACT, 1);
  writeChunk(&calculation, OP_RETURN, 1);
  interpret(&calculation);
  freeChunk(&calculation);

  /* More than eight live values forces the VM stack to grow dynamically. */
  Chunk deep;
  initChunk(&deep);
  for (int i = 1; i <= 32; i++) writeConstant(&deep, (double)i, 1);
  for (int i = 1; i < 32; i++) writeChunk(&deep, OP_ADD, 1);
  writeChunk(&deep, OP_RETURN, 1);
  interpret(&deep);
  freeChunk(&deep);

  freeVM();
  return 0;
}
