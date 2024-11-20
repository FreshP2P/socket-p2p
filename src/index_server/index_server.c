#include <stdio.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pdu/pdu.h>
#include <config/constants.h>
#include <hash_table/hashtable.h>
#include <linked_list/contentlist.h>
#include <unistd.h> 
#include <pthread.h>

hash_table* addr_table;
sem_t addr_sem;
hash_table* content_table;
sem_t content_sem;

struct ContentListNode *content_array[ARRAY_SIZE];
struct ContentListNode *(*content_ptr)[] = content_array;

int register_content(int s, struct sockaddr_in client_addr, struct PDUContentRegistrationBody body)
{
  struct ContentListNode *node = malloc(sizeof(struct ContentListNode));

  node->served_count = 0;
  strcpy(node->peer_name, body.info.peer_name);
  strcpy(node->content_name, body.info.content_name);
  node->peer_addr = body.info.peer_addr; 

  for(int i = 0; i < ARRAY_SIZE; i++){
    if(strcmp((*content_ptr)[i] -> content_name, body.info.content_name) == 0 && 
    strcmp((*content_ptr)[i] -> peer_name, body.info.peer_name) == 0){
      struct PDU err_pdu = {.type = PDU_ERROR, .body.error = {.message = "Name and content already taken."}};
      sendto(s, &err_pdu, calc_pdu_size(err_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
      return 0;
    }
  }

  for(int i = 0; i < ARRAY_SIZE; i++){
    if(strcmp((*content_ptr)[i] -> content_name, "") == 0){
      (*content_ptr)[i] = node;
      break;
    }
  }

  struct PDUAcknowledgement ack_body;
  strcpy(ack_body.peer_name, body.info.peer_name);

  struct PDU ack_pdu = {.type = PDU_ACKNOWLEDGEMENT, .body.ack = ack_body};
  sendto(s, &ack_pdu, calc_pdu_size(ack_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
  return 0;
  
}

int deregister_content(int s, struct sockaddr_in client_addr, struct PDUContentDeregistrationBody body)
{

  struct PDUAcknowledgement ack_body;
  strcpy(ack_body.peer_name, body.info.peer_name);


  if(strcmp(body.info.content_name, "") == 0){
    for(int i = 0; i < ARRAY_SIZE; i++){
      if(strcmp((*content_ptr)[i] -> peer_name, body.info.peer_name) == 0){
        fprintf(stdout, "Insde loop\n");
        struct ContentListNode *node = malloc(sizeof(struct ContentListNode));
        node->served_count = 0;
        strcpy(node->peer_name, "");
        strcpy(node->content_name, "");
        (*content_ptr)[i] = node;
      }
    }
  }
  else{
    for(int i = 0; i < ARRAY_SIZE; i++){
      if(strcmp((*content_ptr)[i] -> peer_name, body.info.peer_name) == 0 && 
        strcmp((*content_ptr)[i] -> content_name, body.info.content_name) == 0){
        
        struct ContentListNode *node = malloc(sizeof(struct ContentListNode));
        node->served_count = 0;
        strcpy(node->peer_name, "");
        strcpy(node->content_name, "");
        (*content_ptr)[i] = node;

        struct PDU ack_pdu = {.type = PDU_ACKNOWLEDGEMENT, .body.ack = ack_body};
        sendto(s, &ack_pdu, calc_pdu_size(ack_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
      }
    }
  }
  return 0;
}

int search_content(int s, struct sockaddr_in client_addr, struct PDUContentDownloadRequestBody body)
{

  struct PDU search_pdu;
  struct PDUContentDownloadRequestBody search_body;
  int count = 0;

  search_pdu.type = PDU_CONTENT_AND_SERVER_SEARCH;
  strcpy(search_body.info.content_name, "");


  // Find highest count for content

  for(int i = 0; i < ARRAY_SIZE; i++){
    if(strcmp((*content_ptr)[i] -> content_name, body.info.content_name) == 0 &&
      strcmp((*content_ptr)[i] -> peer_name, body.info.peer_name) != 0){
        if((*content_ptr)[i] -> served_count >= count){
          count = (*content_ptr)[i] -> served_count;
        }
    }
  }

  int found;
  found = 0;


  for(int i = 0; i < ARRAY_SIZE; i++){
    if(strcmp((*content_ptr)[i] -> content_name, body.info.content_name) == 0 &&
      strcmp((*content_ptr)[i] -> peer_name, body.info.peer_name) != 0){
        if((*content_ptr)[i] -> served_count <= count){
          strcpy(search_body.info.content_name, (*content_ptr)[i]->content_name);
          strcpy(search_body.info.peer_name, (*content_ptr)[i] -> peer_name);
          memcpy(&search_body.info.peer_addr, &(*content_ptr)[i] -> peer_addr, sizeof(struct sockaddr_in));
          search_pdu.body.content_download_req = search_body;
          count = (*content_ptr)[i] -> served_count;
          found = i;
        }
    }
  }

  // Update the count for selected node

  (*content_ptr)[found] -> served_count = (*content_ptr)[found] -> served_count + 1;


  if(strcmp(search_body.info.content_name, "") == 0){
    struct PDU err_pdu = {.type = PDU_ERROR, .body.error = {.message = "Content not found."}};
    sendto(s, &err_pdu, calc_pdu_size(err_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    return 0;
  }

  fprintf(stdout, "Serving Peer: %s\n", search_body.info.peer_name);

  sendto(s, &search_pdu, calc_pdu_size(search_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
  return 0;
}

int list_content(int s, struct sockaddr_in client_addr)
{
  for(int i = 0; i < ARRAY_SIZE; i++){
    if(strcmp((*content_ptr)[i] -> content_name, "") != 0 ){
      struct PDU listing_pdu;
      struct PDUContentListingBody listing_body;

      listing_pdu.type = PDU_ONLINE_CONTENT_LIST;
      strcpy(listing_body.content_name, (*content_ptr)[i]->content_name);
      strcpy(listing_body.peer_name, (*content_ptr)[i]->peer_name);
      listing_pdu.body.content_listing = listing_body;
    
      sendto(s, &listing_pdu, calc_pdu_size(listing_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    }
  }
}

int process_peer_req(int s, struct sockaddr_in client_addr, struct PDU pdu)
{
  switch (pdu.type)
  {
  case PDU_CONTENT_REGISTRATION:
    return register_content(s, client_addr, pdu.body.content_registration);
  case PDU_CONTENT_DEREGISTRATION:
    return deregister_content(s, client_addr, pdu.body.content_deregistration);
  case PDU_ONLINE_CONTENT_LIST:
    return list_content(s, client_addr);
  case PDU_CONTENT_AND_SERVER_SEARCH:
    return search_content(s, client_addr, pdu.body.content_download_req);
  default:
    break;
  }
  return 0;
}

int main(int argc, char const *argv[])
{
  struct sockaddr_in client_sin; /* the from address of a client	*/
  int server_sock;               /* server socket		*/
  int alen;                      /* from-address length		*/
  struct sockaddr_in sin;        /* an Internet endpoint address         */
  int s, type;                   /* socket descriptor and socket type    */
  int port = 8080;

  switch (argc)
  {
  case 1:
    break;
  case 2:
    port = atoi(argv[1]);
    break;
  default:
    fprintf(stderr, "Usage: %s [port]\n", argv[0]);
    return 1;
  }

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);

  /* Allocate a socket */
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0)
  {
    fprintf(stderr, "can't create socket\n");
    return 1;
  }

  /* Bind the socket */
  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    fprintf(stderr, "Can't bind to %d port!\n", port);
    return 1;
  }
  
  // listen to up to 5 connections
  listen(s, 5);
  alen = sizeof(client_sin);

  addr_table = create_table();
  content_table = create_table();

  sem_init(&addr_sem, 1, 1);
  sem_init(&content_sem, 1, 1);

  fprintf(stdout, "Binded index server to port %d.\n", port);

  int i = 0;
  struct ContentListNode *node = malloc(sizeof(struct ContentListNode));
  node->served_count = 0;
  strcpy(node->content_name, "");
  strcpy(node->peer_name, "");

  for(; i < ARRAY_SIZE; i++){
    content_array[i] = malloc(sizeof(struct ContentListNode));
    content_array[i] = node;
  }

  while (1)
  {
    struct PDU received_pdu;

    if (recvfrom(s, &received_pdu, sizeof(received_pdu), 0,
                 (struct sockaddr *)&client_sin, &alen) < 0)
    {
      fprintf(stdout, "Error encountered while receiving!\n");
      continue;
    }
    process_peer_req(s, client_sin, received_pdu);
  }

  return 0;
}
