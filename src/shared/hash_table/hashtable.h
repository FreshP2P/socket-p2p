/**
 * hash_table.h: Hash table header
 * Adapted from Tutorials Point's "Hash Table Program in C"
 * Source: https://www.tutorialspoint.com/data_structures_algorithms/hash_table_program_in_c.htm
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 32

struct DataItem {
  char *key;
  void *value;
  struct DataItem *next;
};

typedef struct DataItem **TABLE;

extern TABLE create_table();

extern unsigned int get_hash(const char *key);

extern void *get(TABLE table, const char *key);

extern void insert(TABLE table, char const *key, void const *value, size_t value_size);

extern struct DataItem *delete(TABLE table, char const *key);
