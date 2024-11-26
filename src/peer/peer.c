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
  body.info.peer_addr = peer_addr;

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

  fprintf(stdout, "%s\n", peer_name);

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
    fprintf(stdout, "Peer: %s Content: %s\n", response_pdu.body.content_listing.peer_name, response_pdu.body.content_listing.content_name);
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
  strcpy(body.info.peer_name, peer_name);
  
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
  body.info.peer_addr = peer_addr;


  struct PDU pdu = {.type = PDU_CONTENT_AND_SERVER_SEARCH, .body.content_download_req = body};
  write(udp_fd, &pdu, calc_pdu_size(pdu)); 
  struct PDU search_response_pdu;
  recvfrom(udp_fd, &search_response_pdu, sizeof(struct PDU), 0, NULL, NULL);
  
  if(search_response_pdu.type == PDU_ERROR){
      fprintf(stdout, "ERROR: %s\n", search_response_pdu.body.error.message);
      return 0;
  }

  int sd;
  struct sockaddr_in server = search_response_pdu.body.content_download_req.info.peer_addr;

  server.sin_family = AF_INET;

  /* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't create a socket\n");
		exit(1);
	}

  fprintf(stdout, "Connecting to content server %d:%d\n", server.sin_addr.s_addr, server.sin_port);

  /* Connecting to the server */
  if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
	  fprintf(stderr, "Can't connect: %s\n", strerror(errno));
	  exit(1);
	}

  struct PDU req_pdu;
  req_pdu.type = PDU_CONTENT_DOWNLOAD_REQUEST;
  req_pdu.body.content_download_req = search_response_pdu.body.content_download_req;
  write(sd, &req_pdu, calc_pdu_size(req_pdu));

  struct PDU data_pdu;
  FILE *fptr;
  char full_path[257];
  sprintf(full_path, "/Users/sameerqureshi/Desktop/COE768/P2P/socket-p2p/%s_files/%s", peer_name, body.info.content_name);
	fptr = fopen(full_path, "a");
	int total_file_bytes = 0;
  int i;
	while ((i = read(sd, &data_pdu, sizeof(struct PDU))) > 0){
		fprintf(stdout, "Read %d bytes\n", i);
		if(data_pdu.type == PDU_ERROR){
			fprintf(stderr, data_pdu.body.error.message);
			break;
		}
		total_file_bytes += i - 1;
		fprintf(stdout, "Total file bytes: %d\n", total_file_bytes);
		fwrite(data_pdu.body.content_data.data, sizeof(char), data_pdu.body.content_data.data_len, fptr);
	}


  peer_register_content(udp_fd, index_server_addr, body.info.content_name);
	
	fprintf(stdout, "Done\n");

	fclose(fptr);	
	close(sd);

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

int file_download(int sd, int udp_fd, struct sockaddr_in index_server_addr)
{
	char file_name[CONTENT_NAME_SIZE];
	char full_path[257];
	char buf[DATA_BODY_SIZE]; // buffer with the 0th char as the error flag
	char sync_buf[2];
	int n, file_name_bytes_read, bytes_read, total_read, read_failed;
	FILE *file;

	// get the file client wants
	struct PDU req_pdu;
  read(sd, &req_pdu, sizeof(struct PDU));

	sprintf(full_path, "../../../%s_files/%s", req_pdu.body.content_download_req.info.peer_name, req_pdu.body.content_download_req.info.content_name);

	if ((file = fopen(full_path, "r")) == NULL)
	{
		// Can't find file, send an error
    struct PDU err_pdu;
    err_pdu.type = PDU_ERROR;

		sprintf(err_pdu.body.error.message, "Can\'t open \'%s\'! Does the file exist? No one knows!\n", file_name);
		fprintf(stderr, err_pdu.body.error.message);
		write(sd, &err_pdu, calc_pdu_size(err_pdu));
	}
	else
	{
		// Send file in chunks of BUFLEN bytes
    struct PDU data_pdu;
    data_pdu.type = PDU_CONTENT_DATA;

		read_failed = 0;
		total_read = 0;

		do
		{
			// read the chunk
			bytes_read = fread(data_pdu.body.content_data.data, sizeof(char), DATA_BODY_SIZE, file);
      data_pdu.body.content_data.data_len = bytes_read;
			total_read += bytes_read;
			fprintf(stdout, "Read: %d bytes; Total: %d bytes\n", bytes_read, total_read);

			// send the chunk, write an error message in case this encounters a problem
			if (bytes_read == 0) {
				break;
			}
			fprintf(stdout, "Sending %d bytes\n", bytes_read + 1);
			if (write(sd, &data_pdu, calc_pdu_size(data_pdu)) == -1)
			{
				read_failed = 1;
				fprintf(stderr, "File send encountered a problem. Is the connection still here? Error: %s\n", strerror(errno));
				break;
			}
		} while (bytes_read > 0); // keep going while there is more to read

		if (read_failed)
			fprintf(stdout, "File transfer failed\n");
		else
			fprintf(stdout, "File transfer complete\n");
	}

	if (file != NULL)
	{
		fclose(file);
	}

	close(sd);

	return (0);
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

  peer_addr = reg_addr;

  FD_ZERO(&afds);

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

  while (1)
  {
    FD_SET(tcp_socket_fd, &afds); /* Listening on a TCP socket */
    FD_SET(STDIN_FILENO, &afds);  /* Listening on stdin */
    memcpy(&rfds, &afds, sizeof(rfds));

    select(FD_SETSIZE, &rfds, NULL, NULL, NULL);

    if (FD_ISSET(tcp_socket_fd, &rfds))
    {
      struct sockaddr_in client;
      int client_len;
      int new_fd = accept(tcp_socket_fd, (struct sockaddr *)&client, &client_len);
      switch (fork())
      {
      case 0: /* child */
        exit(file_download(new_fd, index_udp_socket_fd, index_addr));
      default: /* parent */
        (void)close(new_fd);
        break;
      case -1:
        fprintf(stderr, "fork: error\n");
      }
    }

    if (FD_ISSET(STDIN_FILENO, &rfds))
    {
      char read_in[INPUT_MAX_LENGTH + 1];
      int read_len = read(STDIN_FILENO, read_in, INPUT_MAX_LENGTH);
      read_in[read_len - 1] = 0;

      strcpy(content_name, read_in);

      process_user_input(index_udp_socket_fd, index_addr, read_in);
    }
  }

  return 0;
}
