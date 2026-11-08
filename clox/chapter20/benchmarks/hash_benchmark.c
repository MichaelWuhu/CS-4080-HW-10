#include <stdio.h>
#include <string.h>
#include <time.h>

#include "memory.h"
#include "object.h"
#include "table.h"

static double seconds(clock_t start) {
  return (double)(clock() - start) / CLOCKS_PER_SEC;
}

static void uniqueKeys(Table* table, int count) {
  char chars[32];
  for (int i = 0; i < count; i++) {
    snprintf(chars, sizeof(chars), "key-%d", i);
    tableSet(table, OBJ_VAL(copyString(chars, (int)strlen(chars))),
             NUMBER_VAL(i));
  }
}

int main(void) {
  const int count = 3000;
  initMemory();
  initObjects();

  Table table;
  initTable(&table);

  clock_t start = clock();
  uniqueKeys(&table, count);
  printf("unique inserts: %.6f seconds\n", seconds(start));

  start = clock();
  Value value;
  for (int i = 0; i < count; i++) {
    char chars[32];
    snprintf(chars, sizeof(chars), "key-%d", i);
    tableGet(&table, OBJ_VAL(copyString(chars, (int)strlen(chars))), &value);
  }
  printf("unique lookups: %.6f seconds\n", seconds(start));

  start = clock();
  for (int i = 0; i < count; i += 2) {
    char chars[32];
    snprintf(chars, sizeof(chars), "key-%d", i);
    tableDelete(&table, OBJ_VAL(copyString(chars, (int)strlen(chars))));
  }
  printf("half deletes: %.6f seconds\n", seconds(start));

  freeTable(&table);
  freeObjects();
  return 0;
}
