#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"

static ObjString* objects;
static Table strings;

void initObjects(void) {
  objects = NULL;
  initTable(&strings);
}

static uint32_t hashString(const char* chars, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)chars[i];
    hash *= 16777619;
  }
  return hash;
}

static ObjString* allocateString(int length, bool ownsChars) {
  size_t size = sizeof(ObjString);
  if (ownsChars) size += (size_t)length + 1;
  ObjString* string = (ObjString*)reallocate(NULL, 0, size);
  string->next = objects;
  string->length = length;
  string->hash = hashString(ownsChars ? string->chars : "", 0);
  string->ownsChars = ownsChars;
  string->externalChars = NULL;
  objects = string;
  return string;
}

ObjString* copyString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&strings, chars, length, hash);
  if (interned != NULL) return interned;
  ObjString* string = allocateString(length, true);
  memcpy(string->chars, chars, (size_t)length);
  string->chars[length] = '\0';
  string->hash = hash;
  tableSet(&strings, OBJ_VAL(string), NIL_VAL);
  return string;
}

ObjString* constantString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&strings, chars, length, hash);
  if (interned != NULL) return interned;
  ObjString* string = allocateString(length, false);
  string->externalChars = chars;
  string->hash = hash;
  tableSet(&strings, OBJ_VAL(string), NIL_VAL);
  return string;
}

ObjString* takeString(char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, (size_t)length + 1);
    return interned;
  }
  ObjString* string = allocateString(length, true);
  memcpy(string->chars, chars, (size_t)length + 1);
  FREE_ARRAY(char, chars, (size_t)length + 1);
  string->hash = hash;
  tableSet(&strings, OBJ_VAL(string), NIL_VAL);
  return string;
}

const char* stringChars(const ObjString* string) {
  return string->ownsChars ? string->chars : string->externalChars;
}

void freeObjects(void) {
  freeTable(&strings);
  ObjString* object = objects;
  while (object != NULL) {
    ObjString* next = object->next;
    size_t size = sizeof(ObjString) +
                  (object->ownsChars ? (size_t)object->length + 1 : 0);
    reallocate(object, size, 0);
    object = next;
  }
  objects = NULL;
}
