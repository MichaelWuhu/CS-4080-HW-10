#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

#define HEAP_SIZE (1024 * 1024)

typedef struct BlockHeader {
  size_t size;
  bool free;
} BlockHeader;

static unsigned char* heap;
static const size_t alignment = _Alignof(max_align_t);
static size_t headerSize;

static size_t alignSize(size_t size) {
  return (size + alignment - 1) / alignment * alignment;
}

void initMemory(void) {
  /* The only system allocation in hardcore mode. */
  heap = (unsigned char*)malloc(HEAP_SIZE);
  if (heap == NULL) {
    fprintf(stderr, "Not enough memory for the custom heap.\n");
    exit(1);
  }

  headerSize = alignSize(sizeof(BlockHeader));
  BlockHeader* first = (BlockHeader*)heap;
  first->size = HEAP_SIZE - headerSize;
  first->free = true;
}

static unsigned char* heapEnd(void) {
  return heap + HEAP_SIZE;
}

static BlockHeader* nextBlock(BlockHeader* block) {
  unsigned char* address = (unsigned char*)block + headerSize + block->size;
  return address < heapEnd() ? (BlockHeader*)address : NULL;
}

static BlockHeader* findFreeBlock(size_t size) {
  for (BlockHeader* block = (BlockHeader*)heap; block != NULL;
       block = nextBlock(block)) {
    if (block->free && block->size >= size) return block;
  }
  return NULL;
}

static void splitBlock(BlockHeader* block, size_t size) {
  size_t remainder = block->size - size;
  if (remainder < headerSize + alignment) return;

  BlockHeader* remainderBlock =
      (BlockHeader*)((unsigned char*)block + headerSize + size);
  remainderBlock->size = remainder - headerSize;
  remainderBlock->free = true;
  block->size = size;
}

static void mergeFreeBlocks(void) {
  for (BlockHeader* block = (BlockHeader*)heap; block != NULL;) {
    BlockHeader* next = nextBlock(block);
    if (next != NULL && block->free && next->free) {
      block->size += headerSize + next->size;
    } else {
      block = next;
    }
  }
}

static void releaseBlock(void* pointer) {
  BlockHeader* block =
      (BlockHeader*)((unsigned char*)pointer - headerSize);
  block->free = true;
  mergeFreeBlocks();
}

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    if (pointer != NULL) releaseBlock(pointer);
    return NULL;
  }

  size_t requested = alignSize(newSize);
  if (pointer == NULL) {
    BlockHeader* block = findFreeBlock(requested);
    if (block == NULL) {
      fprintf(stderr, "Custom heap is full.\n");
      exit(1);
    }
    splitBlock(block, requested);
    block->free = false;
    return (unsigned char*)block + headerSize;
  }

  BlockHeader* block =
      (BlockHeader*)((unsigned char*)pointer - headerSize);
  if (block->size >= requested) {
    splitBlock(block, requested);
    return pointer;
  }

  void* replacement = reallocate(NULL, oldSize, newSize);
  memcpy(replacement, pointer, oldSize);
  releaseBlock(pointer);
  return replacement;
}
