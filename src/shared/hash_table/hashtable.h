/**
 * hash_table.h: Hash table header
 * Adapted from Tutorials Point's "Hash Table Program in C"
 * Source: https://www.tutorialspoint.com/data_structures_algorithms/hash_table_program_in_c.htm
 */

#ifndef _P2P_HASH_TABLE
#define _P2P_HASH_TABLE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define TABLE_SIZE 32

struct DataItem {
  char *key;
  void *value;
  struct DataItem *next;
};

typedef struct HashTable
{
  int count;
  struct DataItem *items[TABLE_SIZE];
} hash_table;

extern hash_table create_table();

extern unsigned int get_hash(const char *key);

extern void *table_get(hash_table table, const char *key);

extern void table_insert(hash_table table, char const *key, void const *value, size_t key_len, size_t value_size);

extern void table_delete(hash_table table, char const *key);

extern void table_values(hash_table table, void **values);

#endif
