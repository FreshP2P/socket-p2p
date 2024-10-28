#include "hashtable.h"

TABLE create_table()
{
  return (TABLE)calloc(SIZE, sizeof(struct DataItem *));
}

unsigned int get_hash(const char *key)
{
  unsigned int hash = 5381;
  int c;

  while (c = *key++)
  {
    hash = ((hash * 33) % SIZE + c) % SIZE;
  }

  return hash % SIZE;
}

void *get(TABLE table, const char *key)
{
  unsigned int index = get_hash(key);
  struct DataItem *curr = table[index];

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

void insert(TABLE table, char const *key, void const *value, size_t value_size)
{
  unsigned int index = get_hash(key);
  struct DataItem *prev = NULL;
  struct DataItem *curr = table[index];

  while (curr != NULL)
  {
    if (strcmp(curr->key, key) == 0)
    {
      curr->value = value;
      return;
    }

    prev = curr;
    curr = curr->next;
  }

  // Allocate space to store the new value
  void *new_value = malloc(value_size);
  memcpy(new_value, value, value_size);

  // Allocate the new data item in memory.
  struct DataItem *new_item = malloc(sizeof(struct DataItem));
  new_item->key = key;
  new_item->value = new_value;
  new_item->next = NULL;

  if (prev != NULL)
  {
    prev->next = new_item;
  }
  else
  {
    table[index] = new_item;
  }
}

struct DataItem *delete(TABLE table, char const *key)
{
  unsigned int index = get_hash(key);
  struct DataItem *prev = NULL;
  struct DataItem *curr = table[index];

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
      table[index] = NULL;
    }

    free(curr->value);
    free(curr);
    return;
  }
}