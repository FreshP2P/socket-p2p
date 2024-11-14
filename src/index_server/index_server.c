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

hash_table* addr_table;
sem_t addr_sem;
hash_table* content_table;
sem_t content_sem;

int register_content(int s, struct sockaddr_in client_addr, struct PDUContentRegistrationBody body)
{

  struct ContentList *list;
  char peer_name[PEER_NAME_SIZE + 1], content_name[CONTENT_NAME_SIZE + 1];

  strcpy(peer_name, body.info.peer_name);
  strcpy(content_name, body.info.content_name);

  sem_wait(&content_sem);
  list = (struct ContentList *)table_get(content_table, content_name);
  sem_post(&content_sem);

  if (content_list_find(list, peer_name, content_name) != NULL)
  {
    struct PDU err_pdu = {.type = PDU_ERROR, .body.error = {.message = "Name and content already taken."}};
    sendto(s, &err_pdu, calc_pdu_size(err_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    return 0;
  }
  
  sem_wait(&content_sem);
  if (list == NULL)
  {
    list = content_list_create();
    table_insert(content_table, content_name, &list, sizeof(peer_name), sizeof(list));
  }

  content_list_push_end(list, peer_name, content_name);
  sem_post(&content_sem);
  sem_wait(&addr_sem);

  if (!table_get(addr_table, peer_name))
  {
    table_insert(addr_table, peer_name, &client_addr, sizeof(peer_name), sizeof(struct sockaddr_in));
  }
  sem_post(&addr_sem);

  struct PDUAcknowledgement ack_body;
  strcpy(ack_body.peer_name, peer_name);


  struct PDU ack_pdu = {.type = PDU_ACKNOWLEDGEMENT, .body.ack = ack_body};
  sendto(s, &ack_pdu, calc_pdu_size(ack_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
  return 0;
}

int deregister_content(int s, struct sockaddr_in client_addr, struct PDUContentDeregistrationBody body)
{
  struct ContentList *list;
  char peer_name[PEER_NAME_SIZE + 1], content_name[CONTENT_NAME_SIZE + 1];
  int new_list_count = 0;

  strcpy(peer_name, body.info.peer_name);
  strcpy(content_name, body.info.content_name);
  
  sem_wait(&content_sem);
  list = (struct ContentList *)table_get(content_table, content_name);
  content_list_remove(list, peer_name, content_name);
  new_list_count = list->count;
  sem_post(&content_sem);
  
  if (new_list_count == 0)
  {
    sem_wait(&content_sem);
    content_list_free(list);
    sem_post(&content_sem);

    sem_wait(&addr_sem);
    table_delete(addr_table, peer_name);
    sem_post(&addr_sem);
  }

  struct PDUAcknowledgement ack_body;
  strcpy(ack_body.peer_name, peer_name);

  struct PDU ack_pdu = {.type = PDU_ACKNOWLEDGEMENT, .body.ack = ack_body};
  sendto(s, &ack_pdu, calc_pdu_size(ack_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
  return 0;
}

int search_content(int s, struct sockaddr_in client_addr, struct PDUContentDownloadRequestBody body)
{
  char target_peer_name[PEER_NAME_SIZE + 1], target_content_name[CONTENT_NAME_SIZE + 1];
  struct ContentList *list = (struct ContentList *)table_get(content_table, body.info.content_name);
  if (list == NULL)
  {
    struct PDU err_pdu = {.type = PDU_ERROR, .body.error = {.message = "Content not found."}};
    sendto(s, &err_pdu, calc_pdu_size(err_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    return 0;
  }

  // extract the least used peer
  content_list_remove_end(list, target_peer_name, target_content_name);
  
  // mark it as recently used
  content_list_push_start(list, target_peer_name, target_content_name);

  struct sockaddr_in *address = (struct sockaddr_in *)table_get(addr_table, target_peer_name);
  if (address == NULL)
  {
    struct PDU err_pdu = {.type = PDU_ERROR, .body.error = {.message = "Address not found."}};
    sendto(s, &err_pdu, calc_pdu_size(err_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    return 0;
  }

  body.address = *address;
  struct PDU response_pdu = {.type = PDU_CONTENT_DOWNLOAD_REQUEST, .body.content_download_req = body};
  
  sendto(s, &response_pdu, calc_pdu_size(response_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
  return 0;
}

int list_content(int s, struct sockaddr_in client_addr)
{
  int i = 0;
  int num_contents = content_table->count;
  
  sem_wait(&content_sem);
  char *content_names[num_contents];
  table_keys(content_table, content_names);

  fprintf(stdout, "List: %d\n", content_table->count);  
  for (; i < content_table->count; i++)
  {
    struct PDUContentListingBody listing_body = {
      .end_of_list = (i == (num_contents - 1))
    };

    strcpy(listing_body.content_name, content_names[i]);

    struct PDU listing_pdu = {.type = PDU_ONLINE_CONTENT_LIST, .body.content_listing = listing_body};
    fprintf(stdout, "Sending list\n"); 
    sendto(s, &listing_pdu, calc_pdu_size(listing_pdu), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
  }
  sem_post(&content_sem);
  return 0;
}

int process_peer_req(int s, struct sockaddr_in client_addr, struct PDU pdu)
{
  //fprintf(stdout, "Process %c PDU from %d...\n", pdu.type, client_addr.sin_addr.s_addr);
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
    //fprintf(stdout, "No actions needed for PDU type %c.\n", pdu.type);
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

  while (1)
  {
    struct PDU received_pdu;

    if (recvfrom(s, &received_pdu, sizeof(received_pdu), 0,
                 (struct sockaddr *)&client_sin, &alen) < 0)
    {
      fprintf(stdout, "Error encountered while receiving!\n");
      continue;
    }
    // process the request in a separate process
    switch (fork())
    {
      case 0:
        process_peer_req(s, client_sin, received_pdu);
        break;
      default:
        continue;
    }

  }

  return 0;
}
