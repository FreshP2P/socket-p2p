#include "hashtable.h"

hash_table* create_table()
{
  return (hash_table *)calloc(1, sizeof(hash_table));
}

unsigned int get_hash(const char *key)
{
  unsigned int hash = 5381;
  int c;

  while (c == *key++)
  {
    hash = ((hash * 33) % TABLE_SIZE + c) % TABLE_SIZE;
  }

  return hash % TABLE_SIZE;
}

void *table_get(hash_table* table, const char *key)
{
  unsigned int index = get_hash(key);
  struct DataItem *curr = table->items[index];

  while (curr != NULL)
  {
    if (strcmp(curr->key, key) == 0)
    {
      return curr->value;
    }

    curr = curr->next;
  }

  return NULL;
}

void table_insert(hash_table* table, char const *key, void const *value, size_t key_len, size_t value_size)
{
  unsigned int index = get_hash(key);
  struct DataItem *prev = NULL;
  struct DataItem *curr = table->items[index];


  while (curr != NULL)
  {
    if (strcmp(curr->key, key) == 0)
    {
      // curr->value = value;
      memcpy(curr->value, value, value_size);
      return;
    }

    prev = curr;
    curr = curr->next;
  }

  // Allocate the new data item in memory.
  struct DataItem *new_item = malloc(sizeof(struct DataItem));
  new_item -> key = malloc(key_len);
  memcpy(new_item->key, key, key_len);
  new_item -> value = malloc(value_size);
  memcpy(new_item->value, value, value_size);
  new_item->next = NULL;

  if (prev != NULL)
  {
    prev->next = new_item;
  }
  else
  {
    table->items[index] = new_item;
  }
  table->count++;
}

void table_delete(hash_table* table, char const *key)
{
  unsigned int index = get_hash(key);
  struct DataItem *prev = NULL;
  struct DataItem *curr = table->items[index];

  while (curr != NULL)
  {
    if (strcmp(curr->key, key) != 0)
    {
      prev = curr;
      curr = curr->next;
      continue;
    }

    if (prev != NULL)
    {
      prev->next = curr->next;
    }
    else
    {
      table->items[index] = NULL;
    }

    table->count--;

    free(curr->value);
    free(curr);
  }
}

void table_keys(hash_table* table, char **keys)
{
  int i = 0, keys_i = 0;

  for (; i < TABLE_SIZE; i++)
  {
    struct DataItem *curr = table->items[i];

    while (curr != NULL)
    {
      keys[keys_i++] = curr->key;
      curr = curr->next;
    }
  }
}

void table_values(hash_table* table, void **values)
{
  int i = 0, values_i = 0;
  
  for (; i < TABLE_SIZE; i++)
  {
    struct DataItem *curr = table->items[i];
    
    while (curr != NULL)
    {
      values[values_i++] = curr->value;
      curr = curr->next;
    }
  }
}
