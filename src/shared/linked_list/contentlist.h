#ifndef _P2P_CONTENT_LIST
#define _P2P_CONTENT_LIST

#include "../config/constants.h"

struct ContentList
{
  struct ContentListNode *start;
  struct ContentListNode *end;
};

struct ContentListNode
{
  char peer_name[PEER_NAME_SIZE];
  char content_name[CONTENT_NAME_SIZE];
  struct ContentListNode *prev;
  struct ContentListNode *next;
};

struct ContentList *content_list_create();

struct ContentListNode *content_list_find(struct ContentList *list, const char *peer_name, const char *content_name);

void content_list_push(struct ContentList *list, const char *peer_name, const char *content_name);

void content_list_remove(struct ContentList *list, const char *peer_name, const char *content_name);

#endif
