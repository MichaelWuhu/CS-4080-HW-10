#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"

#define TABLE_MAX_LOAD 0.75

static bool keysEqual(Value a, Value b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL: return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ: {
      ObjString* left = AS_STRING(a);
      ObjString* right = AS_STRING(b);
      return left->length == right->length &&
             memcmp(stringChars(left), stringChars(right),
                    (size_t)left->length) == 0;
    }
  }
  return false;
}

uint32_t hashValue(Value value) {
  switch (value.type) {
    case VAL_BOOL: return value.as.boolean ? 1u : 0u;
    case VAL_NIL: return 0x9e3779b9u;
    case VAL_NUMBER: {
      uint64_t bits;
      memcpy(&bits, &value.as.number, sizeof(bits));
      return (uint32_t)(bits ^ (bits >> 32));
    }
    case VAL_OBJ: return AS_STRING(value)->hash;
  }
  return 0;
}

void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity, Value key) {
  uint32_t index = hashValue(key) & (uint32_t)(capacity - 1);
  Entry* tombstone = NULL;

  for (;;) {
    Entry* entry = &entries[index];
    if (!entry->occupied) {
      if (!entry->tombstone) return tombstone != NULL ? tombstone : entry;
      if (tombstone == NULL) tombstone = entry;
    } else if (keysEqual(entry->key, key)) {
      return entry;
    }
    index = (index + 1) & (uint32_t)(capacity - 1);
  }
}

static void adjustCapacity(Table* table, int capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].occupied = false;
    entries[i].tombstone = false;
    entries[i].key = NIL_VAL;
    entries[i].value = NIL_VAL;
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (!entry->occupied) continue;
    Entry* destination = findEntry(entries, capacity, entry->key);
    destination->occupied = true;
    destination->key = entry->key;
    destination->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool tableSet(Table* table, Value key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }

  Entry* entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = !entry->occupied;
  if (isNewKey) table->count++;
  entry->occupied = true;
  entry->tombstone = false;
  entry->key = key;
  entry->value = value;
  return isNewKey;
}

bool tableGet(const Table* table, Value key, Value* value) {
  if (table->count == 0) return false;
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (!entry->occupied) return false;
  *value = entry->value;
  return true;
}

bool tableDelete(Table* table, Value key) {
  if (table->count == 0) return false;
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (!entry->occupied) return false;
  entry->occupied = false;
  entry->tombstone = true;
  entry->key = NIL_VAL;
  entry->value = BOOL_VAL(true);
  return true;
}

ObjString* tableFindString(const Table* table, const char* chars,
                           int length, uint32_t hash) {
  if (table->count == 0) return NULL;
  uint32_t index = hash & (uint32_t)(table->capacity - 1);
  for (;;) {
    Entry* entry = &table->entries[index];
    if (!entry->occupied) {
      if (!entry->tombstone) return NULL;
    } else if (IS_STRING(entry->key)) {
      ObjString* string = AS_STRING(entry->key);
      if (string->length == length && string->hash == hash &&
          memcmp(stringChars(string), chars, (size_t)length) == 0) {
        return string;
      }
    }
    index = (index + 1) & (uint32_t)(table->capacity - 1);
  }
}
