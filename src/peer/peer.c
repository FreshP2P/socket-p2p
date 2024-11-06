#include <stdio.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pdu/pdu.h>
#include <config/constants.h>
#include <sys/select.h>

void print_prompt()
{
  fprintf(stdout, "Commands:\n");
  fprintf(stdout, "register <content_name>: Register content\n");
  fprintf(stdout, "download <content_name>: Download content\n");
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

  print_prompt();

  while (1)
  {
    select(FD_SETSIZE, &rfds, NULL, NULL, NULL);

    if (FD_ISSET(STDIN_FILENO, &rfds))
    {

    }

    if (FD_ISSET(tcp_socket_fd, &rfds))
    {

    }
  }

  return 0;
}
