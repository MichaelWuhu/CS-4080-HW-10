#include <string.h>

#include "memory.h"
#include "object.h"

static ObjString* objects;

void initObjects(void) { objects = NULL; }

static ObjString* allocateString(int length, bool ownsChars) {
  size_t size = sizeof(ObjString);
  if (ownsChars) size += (size_t)length + 1;
  ObjString* string = (ObjString*)reallocate(NULL, 0, size);
  string->next = objects;
  string->length = length;
  string->ownsChars = ownsChars;
  string->externalChars = NULL;
  objects = string;
  return string;
}

ObjString* copyString(const char* chars, int length) {
  ObjString* string = allocateString(length, true);
  memcpy(string->chars, chars, (size_t)length);
  string->chars[length] = '\0';
  return string;
}

ObjString* constantString(const char* chars, int length) {
  ObjString* string = allocateString(length, false);
  string->externalChars = chars;
  return string;
}

ObjString* takeString(char* chars, int length) {
  ObjString* string = allocateString(length, true);
  memcpy(string->chars, chars, (size_t)length + 1);
  FREE_ARRAY(char, chars, (size_t)length + 1);
  return string;
}

const char* stringChars(const ObjString* string) {
  return string->ownsChars ? string->chars : string->externalChars;
}

void freeObjects(void) {
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
