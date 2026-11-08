#ifndef CLOX_CHAPTER17_COMPILER_H
#define CLOX_CHAPTER17_COMPILER_H

#include <stdbool.h>
#include "chunk.h"

bool compile(const char* source, Chunk* chunk);

#endif
