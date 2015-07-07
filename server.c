#include "server.h"

/*!
 * \brief Faz um parse os parametros do terminal para encontrar a porta e o
 * diretorio onde o servidor sera montado e escreve em uma estrutura
 *
 * \param[in] n_params Numero de parametros vindos do terminal
 * \param[in] dir_path Caminho do diretorio
 * \param[in] port Porta que o servidor ira escutar
 * \param[out] s_info Estrutura onde as informacoes serao salvas
 *
 * \return 0 se OK
 * \return -1 se der algum erro
 */

int parse_param(int n_params, char *dir_path, char *port,
                       struct server_info *s_info)
{  
  if(n_params != 3)
  {
    fprintf(stderr, "Bad parameters\nUsage: server [DIR_PATH] [PORT]\n");
    return -1;
  }

  strncpy(s_info->port, port, MAX_PORT_LEN);    

  if((atoi(s_info->port)) > MAX_PORT)
  {
    fprintf(stderr, "Bad port\nPort Number has to be lower than 65535\n"
                    "Usage: server [DIR_PATH] [PORT]\n");
    return -1;
  }

  strncpy(s_info->dir_path, dir_path, PATH_MAX);    
  
  return 0;
}

int server_start(const struct server_info *s_info)
{
  int enable = 1, listenfd = -1, ret = 0;
  struct addrinfo hints, *servinfo, *aux;
  
  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
    
  if ((ret = getaddrinfo(NULL, s_info->port, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "Getaddrinfo error: %s\n", gai_strerror(ret));
    return -1;
  }
  
 
  for (aux = servinfo; aux != NULL; aux = aux->ai_next)
  {
    if ((listenfd = socket(aux->ai_family, aux->ai_socktype,
                           aux->ai_protocol)) == -1)
    {
      fprintf(stderr, "Socket error: %s\n", strerror(errno));
      continue;
    }
    
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
   
    if (bind(listenfd, aux->ai_addr, aux->ai_addrlen) == -1)
    {
      close (listenfd);
      fprintf(stderr, "Bind error:%s\n",strerror(errno));
      continue;    
    }
  }

  if (listen(listenfd, MAX_LISTEN) == -1)
  {
    fprintf(stderr, "Listen error:%s\n",strerror(errno));
    return -1;    
  }
  
  
  return listenfd;
}

void server_init(client_list *cli_list, int listenfd)
{
  int i;
  memset(cli_list, 0, sizeof(*cli_list));   
  //memset(cli_list->client.fd, -1, sizeof(cli_list->client.fd));
  cli_list->client[SERVER_INDEX].fd = listenfd;
  cli_list->client[SERVER_INDEX].events = POLLIN;
  for(i = SERVER_INDEX + 1; i <= MAX_CLIENTS; i++)
    cli_list->client[i].fd = -1;
}

void reset_poll(client_list *cli_list, int listenfd)
{
  cli_list->client[0].fd = listenfd;
  cli_list->client[0].events = POLLIN;
  cli_list->client[0].revents = 0;
}

static client_info *list_add(client_list *cli_list)
{
  client_info *cli_info;
  
  cli_info = calloc(1, sizeof(*cli_info));
  if(cli_info == NULL)
    return NULL;
    
  if (cli_list->head == NULL)
  cli_list->head = cli_info;
  else
  {
    client_info *i = cli_list->head;
    while (i->next)
      i = i->next;
    i->next = cli_info;
  }
  cli_list->list_len++;
  return cli_info; 
}

int check_connection(client_list *cli_list, int listenfd)
{
  int i, connfd;
  socklen_t clilen;
  struct sockaddr_storage cli_addr;
    
    clilen = sizeof(cli_addr);
    if((connfd = accept(listenfd, (struct sockaddr *)&cli_addr,
      &clilen)) == -1)
    {
      return -1;
    }
    
    for (i = SERVER_INDEX + 1; i < MAX_CLIENTS; i++)
    {
      if (cli_list->client[i].fd < 0)
      {
        cli_list->client[i].fd = connfd;
        cli_list->client[i].events = POLLIN;
        if(!(list_add(cli_list)))
          fprintf(stderr,"Add to list error");
        break;
      }
    }
  return 0;
}

static void clean_struct(client_info *cli_info)
{ 
  if (cli_info->fp)
  fclose(cli_info->fp);
  free(cli_info->buffer);
}

static void list_del(client_info *cli_info, client_list *cli_list)
{
  if (cli_info == cli_list->head)
  {
    cli_list->head = cli_info->next;
    free(cli_info);   
  }
  else
  {
    client_info *current_cli = cli_info;
    client_info *i = cli_list->head;
    
    while (i->next)
    {
      if (i->next == current_cli)
      {
        i->next = current_cli->next;
        i = i->next;
        free(current_cli);
        break;
      }
      i = i->next;
    }  
  }
  cli_list->list_len--;
}
static void shift_client_list(client_list *cli_list, int cli_num)
{
  memmove(&cli_list->client[cli_num], &cli_list->client[cli_num + 1],
         (cli_list->list_len - cli_num) * sizeof(*cli_list->client));
}

void close_connection(client_info *cli_info, client_list *cli_list,
                      int cli_num)
{   
  clean_struct(cli_info);
  close(cli_list->client[cli_num].fd);
  shift_client_list(cli_list, cli_num);
  memset(&cli_list->client[cli_list->list_len], 0, sizeof(*cli_list->client));
  cli_list->client[cli_list->list_len].fd = -1;
  list_del(cli_info, cli_list);
}

int get_http_request(client_info *cli_info, int sockfd)
{     
  int num_bytes_read = 0;
  int num_bytes_aux = 0;
  
  cli_info->buffer = (char *) calloc(BUFSIZE, sizeof(char));
  if (cli_info->buffer == NULL)
    return -1;
    
  while (num_bytes_aux < BUFSIZE - 1)
  {   
    num_bytes_read = recv(sockfd, cli_info->buffer +
                          num_bytes_aux, BUFSIZE - num_bytes_aux - 1, 0);
                         
    if ((strstr(cli_info->buffer, "\r\n\r\n")) != NULL ||
       (strstr(cli_info->buffer, "\n\n")) != NULL)
    break;

    if (num_bytes_read < 0)
    {
      if (errno == ECONNRESET)
      {
        return -1;
      }
      else
      {
        fprintf(stderr, "Recv error:%s\n", strerror(errno));
        return -1;
      }
    }  
    else if (num_bytes_read == 0)
      return -1;
    
    
    if (num_bytes_read == 0)
    break;
    
    num_bytes_aux += num_bytes_read;     
  }
  return 0;
}

static int verify_request(const char *path, const char *realpath)
{
  if (strstr(path, "../") != NULL)
    return FORBIDDEN;    
  if (access(realpath, R_OK) == 0)
    return OK;
  else
    return NOT_FOUND;
    
  return -1;
}

int parse_http_request(client_info *cli_info, const char *dir_path)
{
  int ret;
  char path[PATH_MAX], method[MAX_METHOD_LEN];

  strcpy(path, dir_path);
  strcat(path, "/");

  if ((ret = sscanf(cli_info->buffer, "%"STR_METHOD"s %"STR_PATH"s",
                    method, path + strlen(path))) != 2)
    return -1;
  
  realpath(path, cli_info->file_path); 
  cli_info->request_status = verify_request(path, cli_info->file_path);
    
  return 0;
} 

int open_file(client_info *cli_info)
{  
  cli_info->fp = fopen(cli_info->file_path, "rb");
  if (cli_info->fp == NULL)
    return -1;
  
  return 0;
}

static char *status_msg(const http_code status)
{
  switch (status)
  {
    case OK:
      return "OK";
      break;

    case BAD_REQUEST:
      return "BAD REQUEST";
      break;

    case FORBIDDEN:
      return "FORBIDDEN";
      break;

    case NOT_FOUND:
      return "NOT FOUND";
      break;
    case INTERNAL_ERROR:
      return "INTERNAL ERROR";
      break;    
  }

  return NULL;
}

int send_http_response_header(client_info *cli_info, int sockfd)
{
  char response_msg[HEADERSIZE];
  int status = cli_info->request_status;
   
    snprintf(response_msg, sizeof(response_msg), "HTTP/1.0 %d %s\r\n\r\n",
             status, status_msg(status));

    if (send(sockfd, response_msg, strlen(response_msg), 0) < 0)
    {
      fprintf(stderr, "Send error:%s\n", strerror(errno));
      return -1;
    }    
  
  cli_info->header_sent = true;
  return 0;
}
//procurar erro aqui ou no send
int get_filedata(client_info *cli_info)
{
  int num_bytes_read = 0;
 
  num_bytes_read = fread(cli_info->buffer, 1, BUFSIZE, cli_info->fp);          
  if (num_bytes_read < 0)
  {
    fprintf(stderr,"erro no get_filedata %p\n%s,", cli_info->fp,
                                                   strerror(errno));
    return -1;
  }
  if (num_bytes_read == 0)
    fprintf(stderr,"num_bytes_read %d", num_bytes_read);
 
  return num_bytes_read;
}

int send_requested_data(client_info *cli_info, int num_bytes_read, 
                        int sockfd)
{
  int ret;
  //fazer if num_bytes_read = -1 para tentar ler novamente o arquivo ou
  // verificar o arquivo e descritor novamente 
  //for (i = 0; i < num_bytes_read; i++)
  //{
    ret = send(sockfd, cli_info->buffer, num_bytes_read, 0); 
    if (ret < 0)
    {
      if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
   //     continue;
      fprintf(stderr, "Send error:%s\n", strerror(errno));
      return -1;          
    }
  //  i += ret;
 // }
  
  return 0;
}
