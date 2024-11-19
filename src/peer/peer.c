#include <stdio.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pdu/pdu.h>
#include <linked_list/contentlist.h>
#include <config/constants.h>
#include <unistd.h> 
#include <errno.h>

// Enum representing what we are prompting. This is used to track follow up prompts
typedef enum PromptMode
{
  // Prompting for an action the user wants to perform
  PROMPT_ACTION,

  // Prompting for the content to be listed
  PROMPT_LIST,
  
  // Prompting for the content to register
  PROMPT_REGISTRATION,
  
  // Prompting for the content to deregister
  PROMPT_DEREGISTRATION,
  
  // Prompting for the content to download
  PROMPT_DOWNLOAD
} prompt_mode;

// List of actions the user can perform.
typedef enum UserAction
{
  LIST_CONTENT = 'L',
  REGISTER = 'R',
  DEREGISTER = 'T',
  DOWNLOAD = 'D',
  QUIT = 'Q'
} user_action;

// A function representing a process involving interactions with the index server.
typedef int (*udp_child_proc_func)(int udp_fd, struct sockaddr_in index_server_addr, char *arg);

// Peer name
char peer_name[PEER_NAME_MAX_LENGTH + 1];

// Content name
char content_name[CONTENT_NAME_SIZE + 1];

// Peer address
struct sockaddr_in peer_addr;

// Current prompt
prompt_mode input_mode = PROMPT_ACTION;

// Registered contents
struct ContentList *contents;


void create_child_process(int udp_fd, struct sockaddr_in index_server_addr, udp_child_proc_func proc_func, char *arg)
{
  switch (fork())
  {
  case 0:
    exit(proc_func(udp_fd, index_server_addr, arg));
    break;
  }

}

void wait_for_children()
{
  int terminated_proc;
  int status;

  fprintf(stdout, "Waiting for all child processes to finish...\n");
  while ((terminated_proc = waitpid(-1, &status, 0)) > 0)
  {
    fprintf(stdout, "Process %d terminated with code %d\n", terminated_proc, status);
  }
}


void peer_register_content(int udp_fd, struct sockaddr_in index_server_addr, char *arg){
  
  struct PDUContentRegistrationBody body;
  strcpy(body.info.content_name, content_name);
  strcpy(body.info.peer_name, peer_name);
  body.address = peer_addr;

  struct PDU pdu = {.type = PDU_CONTENT_REGISTRATION, .body.content_registration = body};
  write(udp_fd, &pdu, calc_pdu_size(pdu)); 
  struct PDU response_pdu;
  recvfrom(udp_fd, &response_pdu, sizeof(struct PDU), 0, NULL, NULL);
  switch(response_pdu.type)
  {
    case PDU_ACKNOWLEDGEMENT:
      fprintf(stdout, "Content registered.\n");
      break;
    case PDU_ERROR:
      fprintf(stderr, "ERROR: %s\n", response_pdu.body.error.message);
      break;
  }
}

void peer_deregister_content(int udp_fd, struct sockaddr_in index_server_addr){

  struct PDUContentDeregistrationBody body;
  strcpy(body.info.peer_name, peer_name);
  strcpy(body.info.content_name, content_name);

  struct PDU pdu = {.type = PDU_CONTENT_DEREGISTRATION, .body.content_deregistration = body};
  write(udp_fd, &pdu, calc_pdu_size(pdu)); 
  struct PDU response_pdu;
  recvfrom(udp_fd, &response_pdu, sizeof(struct PDU), 0, NULL, NULL);
  switch(response_pdu.type)
  {
    case PDU_ACKNOWLEDGEMENT:
      fprintf(stdout, "Content deregistered.\n");
      break;
    case PDU_ERROR:
      fprintf(stderr, "ERROR: %s\n", response_pdu.body.error.message);
      break;
  }
}

/**
 * Displays all available online content in the output.
 */
int list_online_content(int udp_fd, struct sockaddr_in index_server_addr, char *arg)
{
  // TODO: Send and receive packets for online content list

  struct PDU pdu = {.type = PDU_ONLINE_CONTENT_LIST, .body.content_data = NULL};
  write(udp_fd, &pdu, calc_pdu_size(pdu));
  struct PDU response_pdu;
  for(int i = 0; i < ARRAY_SIZE; i++){
    recvfrom(udp_fd, &response_pdu, sizeof(struct PDU), 0, NULL, NULL);
    fprintf(stdout, "%s\n", response_pdu.body.content_listing.content_name);
  }
  switch(response_pdu.type)
  {
    case PDU_ONLINE_CONTENT_LIST:
      break;
    case PDU_ERROR:
      fprintf(stderr, "ERROR: %s\n", response_pdu.body.error.message);
      break;
  }
  return 0;
}

/**
 * Terminates all content.
 */
int terminate_all_content(int udp_fd, struct sockaddr_in index_server_addr)
{

  struct PDUContentDeregistrationBody body;
  strcpy(body.info.content_name, "");
  strcpy(body.info.peer_name, "");
  body.info.peer_addr = &peer_addr;

  
  struct PDU pdu = {.type = PDU_CONTENT_DEREGISTRATION, .body.content_deregistration = body};
  write(udp_fd, &pdu, calc_pdu_size(pdu));

  return 0;

  
  //struct PDU response_pdu;
  //recvfrom(udp_fd, &response_pdu, sizeof(struct PDU), 0, NULL, NULL);

  
  //switch(response_pdu.type)
  //{
  //case PDU_ACKNOWLEDGEMENT:
  //  fprintf(stdout, "%s deregistered.\n", content_name);
  //  break;
  //case PDU_ERROR:
  //  fprintf(stderr, "ERROR: %s\n", response_pdu.body.error.message);
  //  break;
  //}
  
}


int peer_download_content(int udp_fd, struct sockaddr_in index_server_addr, char *arg){

  //fprintf(stdout, "Download: %s\n", arg);

  struct PDUContentDownloadRequestBody body;
  strcpy(body.info.content_name, arg);
  strcpy(body.info.peer_name, peer_name);
  body.info.peer_addr = &peer_addr;


  struct PDU pdu = {.type = PDU_CONTENT_AND_SERVER_SEARCH, .body.content_download_req = body};
  write(udp_fd, &pdu, calc_pdu_size(pdu)); 
  struct PDU response_pdu;
  recvfrom(udp_fd, &response_pdu, sizeof(struct PDU), 0, NULL, NULL);
  
  if(response_pdu.type == PDU_ERROR){
      fprintf(stdout, "ERROR: %s\n", response_pdu.body.error.message);
      return 0;
  }

  fprintf(stdout, "%d\n", response_pdu.body.content_download_req.info.peer_addr);
  

  


  return 0;

}


/**
 * Initiates the quit sequence, which deregisters all content and exits.
 */
void quit(int udp_fd, struct sockaddr_in index_server_addr)
{
  exit(terminate_all_content(udp_fd, index_server_addr));
}

/**
 * Process the user's selected action, based on what we are currently prompting.
 */
void process_action(int udp_fd, struct sockaddr_in index_server_addr, user_action action)
{
  switch (action)
  {
  case LIST_CONTENT:
    input_mode = PROMPT_LIST;
    fprintf(stdout, "Listing available content...\n");
    break;
  case REGISTER:
    input_mode = PROMPT_REGISTRATION;
    fprintf(stdout, "What file would you like to register?\n");
    break;
  case DEREGISTER:
    input_mode = PROMPT_DEREGISTRATION;
    fprintf(stdout, "What file would you like to deregister?\n");
    break;
  case DOWNLOAD:
    input_mode = PROMPT_DOWNLOAD;
    fprintf(stdout, "What file would you like to download?\n");
    break;
  case QUIT:
    fprintf(stdout, "Quitting...\n");
    quit(udp_fd, index_server_addr);
    break;
  default:
    fprintf(stdout, "%c is not a valid choice.\n", action);
    break;
  }
}

/**
 * Handles all actions as commanded by the user.
 * Only UDP connections with the index server should be utilized here.
 */
void process_user_input(int udp_fd, struct sockaddr_in index_server_addr, char *arg)
{
  switch (input_mode)
  {
  case PROMPT_ACTION:
    process_action(udp_fd, index_server_addr, (user_action)toupper(arg[0]));
    break;

  case PROMPT_LIST:
    fprintf(stdout, "Listing available content...\n");
    create_child_process(udp_fd, index_server_addr, list_online_content, NULL);
    input_mode = PROMPT_ACTION;
    break;
  
  case PROMPT_REGISTRATION:
    fprintf(stdout, "Registering %s...\n", arg);
     create_child_process(udp_fd, index_server_addr, peer_register_content, NULL);
    input_mode = PROMPT_ACTION;
    break;

  case PROMPT_DEREGISTRATION:
    fprintf(stdout, "Deregistering %s...\n", arg);
    create_child_process(udp_fd, index_server_addr, peer_deregister_content, NULL);
    input_mode = PROMPT_ACTION;
    break;
    
  case PROMPT_DOWNLOAD:
    // TODO: Download content with TCP connection
    fprintf(stdout, "Downloading %s...\n", arg);
    create_child_process(udp_fd, index_server_addr, peer_download_content, arg);
    input_mode = PROMPT_ACTION;
    break;
  }
}

void print_prompt()
{
  fprintf(stdout, "Commands:\n");
  fprintf(stdout, "L: List available content\n");
  fprintf(stdout, "R: Register content\n");
  fprintf(stdout, "T: Deregister content\n");
  fprintf(stdout, "D: Download content\n");
  fprintf(stdout, "Q: Quit\n");
}

void prompt_peer_name()
{
  memset(peer_name, 0, PEER_NAME_MAX_LENGTH + 1);
  puts("What is the peer name? (10 characters only)");
  
  int name_len = read(STDIN_FILENO, peer_name, PEER_NAME_MAX_LENGTH + 1);
  peer_name[name_len - 1] = 0;
  fprintf(stdout, "Peer name: %s\n", peer_name);
}


int main(int argc, char const *argv[])
{
  /* code */
  struct sockaddr_in reg_addr; /* address of this peer */
  int reg_alen;                /* peer address length */
  int tcp_socket_fd, type;     /* socket descriptor and socket type */
  fd_set rfds, afds;           /* descriptor set to contain stdin and TCP socket */

  char *index_host = "localhost";
  int index_port = 8080;
  struct hostent *index_host_ent; /* info of index server */
  struct sockaddr_in index_addr;  /* index server address */
  int index_udp_socket_fd;        /* index server UDP socket */
  peer_addr = reg_addr;

  switch (argc)
  {
  case 1:
    break;
  case 2:
    index_host = (char *)argv[1];
  case 3:
    index_host = (char *)argv[1];
    index_port = atoi(argv[2]);
    break;
  default:
    fprintf(stderr, "usage: socket-p2p-peer [index_host [index_port]]\n");
    exit(1);
  }

  memset(&index_addr, 0, sizeof(index_addr));
  index_addr.sin_family = AF_INET;
  index_addr.sin_port = htons(index_port);

  /* Map host name to IP address, allowing for dotted decimal */
  if (index_host_ent = gethostbyname(index_host))
  {
    memcpy(&index_addr.sin_addr, index_host_ent->h_addr_list[0], index_host_ent->h_length);
  }
  else if ((index_addr.sin_addr.s_addr = inet_addr(index_host)) == INADDR_NONE)
  {
    fprintf(stderr, "Can't get host entry %s:%d!\n", index_host, index_port);
  }

  tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  reg_addr.sin_family = AF_INET;
  reg_addr.sin_port = htons(0);
  reg_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(tcp_socket_fd, (struct sockaddr *)&reg_addr, sizeof(reg_addr));

  reg_alen = sizeof(struct sockaddr_in);
  getsockname(tcp_socket_fd, (struct sockaddr *)&reg_addr, &reg_alen);
  listen(tcp_socket_fd, 5);

  FD_ZERO(&afds);
  FD_SET(tcp_socket_fd, &afds); /* Listening on a TCP socket */
  FD_SET(STDIN_FILENO, &afds);  /* Listening on stdin */
  memcpy(&rfds, &afds, sizeof(rfds));

  /* Create and connect UDP socket */
  index_udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (index_udp_socket_fd < 0)
  {
    fprintf(stderr, "Can't create socket \n");
  }
  if (connect(index_udp_socket_fd, (struct sockaddr *)&index_addr, sizeof(index_addr)) < 0)
  {
    fprintf(stderr, "Can't connect to %s:%d!\n", index_host, index_port);
  }

  contents = content_list_create();
  prompt_peer_name();

  print_prompt();

  fprintf(stdout, "Peer address: %d\n", reg_addr.sin_addr.s_addr);

  while (1)
  {
    select(FD_SETSIZE, &rfds, NULL, NULL, NULL);

    if (FD_ISSET(STDIN_FILENO, &rfds))
    {
      char read_in[INPUT_MAX_LENGTH + 1];
      int read_len = read(STDIN_FILENO, read_in, INPUT_MAX_LENGTH);
      read_in[read_len - 1] = 0;

      strcpy(content_name, read_in);

      process_user_input(index_udp_socket_fd, index_addr, read_in);
    }

    if (FD_ISSET(tcp_socket_fd, &rfds))
    {

      fprintf(stdout, "In TCP\n");

    }
  }

  return 0;
}
