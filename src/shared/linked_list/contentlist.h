#ifndef _P2P_CONTENT_LIST
#define _P2P_CONTENT_LIST

#include "../config/constants.h"

struct ContentList
{
  int count;
  
  // The most recently used node
  struct ContentListNode *start;
  
  // The least recently used node
  struct ContentListNode *end;
};

struct ContentListNode
{
  int served_count;
  char peer_name[PEER_NAME_SIZE + 1];
  char content_name[CONTENT_NAME_SIZE + 1];
  struct sockaddr_in *peer_addr;
  struct ContentListNode *prev;
  struct ContentListNode *next;
};

struct ContentList *content_list_create();

struct ContentListNode *content_list_find(struct ContentList *list, const char *peer_name, const char *content_name);

void content_list_push_start(struct ContentList *list, const char *peer_name, const char *content_name);

void content_list_push_end(struct ContentList *list, const char *peer_name, const char *content_name);

void content_list_remove(struct ContentList *list, const char *peer_name, const char *content_name);

void content_list_remove_start(struct ContentList *list, char *removed_peer_name, char *removed_content_name);

void content_list_remove_end(struct ContentList *list, char *removed_peer_name, char *removed_content_name);

void content_list_get_all(struct ContentList *list, struct ContentListNode **nodes);

void content_list_free(struct ContentList *list);

#endif
