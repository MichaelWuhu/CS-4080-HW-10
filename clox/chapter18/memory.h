#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate((pointer), sizeof(type) * (oldCount), \
                      sizeof(type) * (newCount))
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate((pointer), sizeof(type) * (oldCount), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);
void initMemory(void);

#endif
