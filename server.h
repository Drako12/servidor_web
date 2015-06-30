#ifndef SERVER_H_INCLUDED
#define SERVEFR_H_INCLUDED

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

#define BUFSIZE BUFSIZ
#define MAX_CLIENTS 100
#define MAX_PORT 65536
#define MAX_PATH 1024 

struct server_info
{
  char dir_path[MAXPATH];
  char port[MAXPORT];
};

typedef enum
{
  OK = 200,
  FORBIDDEN = 403,
  NOT FOUND = 404,
  INTERNAL ERROR 500,
   
}http_code;
#endif 
