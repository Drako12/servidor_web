#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <poll.h>
#include <ctype.h>
#include <limits.h>

#define BUFSIZE BUFSIZ
#define HEADERSIZE 512
#define MAX_CLIENTS 512
#define MAX_PORT 65536
#define MAX_LISTEN 512
#define NEW 0
#define STARTED 1
#define OLD 2

struct server_info
{
  char dir_path[PATH_MAX];
  char port[MAX_PORT];
  int max_i_cli;
};

struct client_info
{
  char *buffer;
  char file_path[PATH_MAX];
  char *file_position;
  int file_len;
  int request_status;
  int status;
  FILE *fp;
};

typedef enum 
{
  OK = 200,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  INTERNAL_ERROR = 500
}http_code;

#endif 
