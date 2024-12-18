#include "contentlist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ContentList *content_list_create()
{
  struct ContentList *new_list = calloc(1, sizeof(struct ContentList));
  new_list->start = NULL;
  new_list->end = NULL;

  return new_list;
}

struct ContentListNode *content_list_find(struct ContentList *list, const char *peer_name, const char *content_name)
{
  if (list == NULL)
  {
    return NULL;
  }
  
  struct ContentListNode *curr = list->start;
  while (curr != NULL)
  {
    fprintf(stdout, "%s\n", curr->content_name);
    if (strcmp(curr->peer_name, peer_name) == 0 && strcmp(curr->content_name, content_name) == 0)
    {
      return curr;
    }
    curr = curr->next;
  }
  
  return NULL;
}

void content_list_push_start(struct ContentList *list, const char *peer_name, const char *content_name)
{
  if (list == NULL)
  {
    return;
  }

  struct ContentListNode *new_node = malloc(sizeof(struct ContentListNode));
  strcpy(new_node->peer_name, peer_name);
  strcpy(new_node->content_name, content_name);

  if (list->start == NULL)
  {
    list->end = new_node;
  }
  else
  {
    list->start->prev = new_node;
  }

  new_node->next = list->start;
  list->start = new_node;

  list->count++;
}

void content_list_push_end(struct ContentList *list, const char *peer_name, const char *content_name)
{
  if (list == NULL)
  {
    return;
  }
  
  struct ContentListNode *new_node = malloc(sizeof(struct ContentListNode));
  strcpy(new_node->peer_name, peer_name);
  strcpy(new_node->content_name, content_name);
  
  if (list->end == NULL)
  {
    list->start = new_node;
  }
  else
  {
    list->end->next = new_node;
  }

  new_node->prev = list->end;
  list->end = new_node;

  list->count++;
}

void content_list_remove(struct ContentList *list, const char *peer_name, const char *content_name)
{
  if (list == NULL)
  {
    return;
  }
  
  struct ContentListNode *node_to_delete = content_list_find(list, peer_name, content_name);
  
  if (node_to_delete == NULL)
  {
    return;
  }

  if (node_to_delete->prev != NULL)
  {
    node_to_delete->prev->next = node_to_delete->next;
  }

  if (node_to_delete->next != NULL)
  {
    node_to_delete->next->prev = node_to_delete->prev;
  }
  
  if (node_to_delete == list->start)
  {
    list->start = node_to_delete->next;
  }
  
  if (node_to_delete == list->end)
  {
    list->end = node_to_delete->prev;
  }

  list->count--;

  free(node_to_delete);
}

void content_list_remove_start(struct ContentList *list, char *removed_peer_name, char *removed_content_name)
{
  struct ContentListNode *node_to_delete = list->start;
  list->start = node_to_delete->next;
  
  if (list->start != NULL)
  {
    list->start->prev = NULL;
  }
  else
  {
    list->end = NULL;
  }

  list->count--;

  strcpy(removed_peer_name, node_to_delete->peer_name);
  strcpy(removed_content_name, node_to_delete->content_name);

  free(node_to_delete);
}

void content_list_remove_end(struct ContentList *list, char *removed_peer_name, char *removed_content_name)
{
  struct ContentListNode *node_to_delete = list->end;
  list->end = node_to_delete->prev;

  if (list->end != NULL)
  {
    list->end->next = NULL;
  }
  else
  {
    list->start = NULL;
  }

  list->count--;

  free(node_to_delete);
}

void content_list_get_all(struct ContentList *list, struct ContentListNode **nodes)
{
  int i = 0;

  struct ContentListNode *curr = list->start;
  while (curr != NULL)
  {
    nodes[i++] = curr;
    curr = curr->next;
  }
}

void content_list_free(struct ContentList *list)
{
  struct ContentListNode *curr = list->start;
  while (curr != NULL)
  {
    struct ContentListNode *next = curr->next;
    free(curr);
    curr = next;
  }

  free(list);
}
