#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "value.h"

typedef struct {
  Value key;
  Value value;
  bool occupied;
  bool tombstone;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(const Table* table, Value key, Value* value);
bool tableSet(Table* table, Value key, Value value);
bool tableDelete(Table* table, Value key);
ObjString* tableFindString(const Table* table, const char* chars,
                           int length, uint32_t hash);
uint32_t hashValue(Value value);

#endif
