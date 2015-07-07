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
#include <stdbool.h>

#define BUFSIZE BUFSIZ
#define HEADERSIZE 512
#define MAX_CLIENTS 512
#define MAX_PORT 65535
#define MAX_PORT_LEN 6
#define MAX_LISTEN 512
#define MAX_METHOD_LEN 4
#define SERVER_INDEX 0
#define FORMAT(S) 
#define RESOLVE(S) FORMAT(S) 
#define STR_STATUS FORMAT(MAX_HTTP_STATUS_LEN)
#define STR_METHOD FORMAT(MAX_METHOD_LEN) 
#define STR_PATH FORMAT(PATH_MAX) 

typedef enum http_code_
{
  OK = 200,
  BAD_REQUEST = 400,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  INTERNAL_ERROR = 500
}http_code;

typedef enum methods_
{
  GET,
  PUT
}methods;

struct server_info
{
  char dir_path[PATH_MAX];
  char port[MAX_PORT_LEN];
};

typedef struct client_info_
{
  char *buffer;
  char file_path[PATH_MAX];
  int request_status;
  bool  header_sent;
  FILE *fp;
  struct client_info_ *next;
  methods method;
} client_info;

typedef struct client_list_ 
{
  int list_len; 
  struct pollfd client[MAX_CLIENTS];  
  client_info *head;
} client_list;


int parse_param(int n_params, char *dir_path, char *port,
                struct server_info *s_info);
int server_start(const struct server_info *s_info);
void server_init(client_list *cli_list, int listenfd);
void reset_poll(client_list *cli_list, int listenfd);
void close_connection(client_info *cli_info, client_list *cli_list,
                      int cli_num);
int check_connection(client_list *cli_list, int listenfd);
int get_http_request(client_info *cli_info, int sockfd);
int parse_http_request(client_info *cli_info, const char *dir_path);
int open_file(client_info *cli_info);
int send_http_response_header(client_info *cli_info, int sockfd);
int get_filedata(client_info *cli_info);
int send_requested_data(client_info *cli_info, int num_bytes_read, 
                        int sockfd);

#endif
