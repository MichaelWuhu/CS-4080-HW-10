#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "common.h"

/* Owned strings store their characters immediately after the object header.
 * Constant strings instead point into the source buffer. */
typedef struct ObjString ObjString;

struct ObjString {
  struct ObjString* next;
  int length;
  uint32_t hash;
  bool ownsChars;
  const char* externalChars;
  char chars[];
};

void initObjects(void);
void freeObjects(void);
ObjString* copyString(const char* chars, int length);
ObjString* constantString(const char* chars, int length);
ObjString* takeString(char* chars, int length);
const char* stringChars(const ObjString* string);

#endif
