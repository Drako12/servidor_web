#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#define _GNU_SOURCE
#include "bucket.h"
#include "thread.h"
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
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/un.h>

#define MAX_INI_LINE 80
#define BUFSIZE BUFSIZ
#define HEADERSIZE 512
#define NCLIENTS 128
#define MAX_PORT 65535
#define MAX_PORT_LEN 6
#define MAX_LISTEN 512
#define MAX_METHOD_LEN 4
#define SERVER_INDEX 0
#define LOCAL_SOCKET 1
#define MAX_LONG 10
#define INI_OFFSET 7
#define FORMAT(S)
#define RESOLVE(S) FORMAT(S)
#define STR_STATUS FORMAT(MAX_HTTP_STATUS_LEN)
#define STR_METHOD FORMAT(MAX_METHOD_LEN)
#define STR_LONG FORMAT(MAX_LONG)

#define STR_PATH FORMAT(PATH_MAX)
#define ARRAY_LEN(a) sizeof(a)/sizeof(*a)

typedef enum http_code_
{
  OK = 200,
  ACCEPTED = 202,
  BAD_REQUEST = 400,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  INTERNAL_ERROR = 500
}http_code;

typedef enum methods_
{
  GET = 1,
  PUT
} methods;

typedef enum job_status_
{
  RUNNING = 0,
  FINISHED,
  ERROR

} job_stat;
typedef struct server_info_
{
  char dir_path[PATH_MAX];
  char port[MAX_PORT_LEN];
  long long client_rate;
  int max_clients;
} server_info;

typedef struct client_info_
{
  char *buffer;
  char header[HEADERSIZE];
  char file_path[PATH_MAX];
  int request_status;
  int partial_send;
  int bytes_read;
  int bytes_write;
  int sockfd;
  int header_size;
  long long timeout;
  long long timestamp;
  long content_length;
  bool header_sent;
  bool is_ready;
  bool io_error;
  job_stat job_status;
  FILE *fp;
  struct client_info_ *next;
  methods method;
  t_bucket bucket;
} client_info;

typedef struct client_list_
{
  int list_len;
  struct pollfd *client;
  client_info *head;
} client_list;


int save_pid_file();
int change_conf(client_list *cli_list, server_info *s_info);
int parse_and_fill_server_info(int n_params, char *dir_path, char *port,
                               char *rate, server_info *s_info);
int server_start_listen(const server_info *s_info);
void client_list_init(client_list *cli_list, int listenfd, int max_clients,
                      int sockfd);
int get_thread_msg(client_list *cli_list);
void close_connection(client_info *cli_info, client_list *cli_list,
                      int cli_num);
int check_connection(client_list *cli_list, server_info *s_info,
                     thread_pool *t_pool);
int process_http_request(client_info *cli_info, const char *dir_path,
                         thread_pool *t_pool, client_list *cli_list,
                         int cli_num);
int build_and_send_header(client_info *cli_info, client_list *cli_list,
                          const char *dir_path);
int open_file(client_info *cli_info);
int process_bucket_and_data(client_info *cli_info, thread_pool *t_pool,
                            client_list *cli_list, int cli_num);
int set_nonblock(int sockfd);
void set_clients(client_info *cli_info, client_list *cli_list, int cli_num);
struct timespec find_poll_wait_time(client_info *cli_info,
                                    struct timespec t_wait);
void cleanup(client_list *cli_list, thread_pool *t_pool);
int server_socket_init();
void read_filedata(void *cli_info);
void write_client_data (void *cli_info);
long long find_timeout(long long now, client_info *cli_info);

#endif
