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
    fprintf(stderr, "Bad parameters\n");
    return -1;
  }
  strncpy(s_info->dir_path, dir_path, PATH_MAX);    
  strncpy(s_info->port, port, MAX_PORT);

  return 0;
}

int server_start(const struct server_info *s_info)
{
  int yes = 1, listenfd = -1;
  struct sockaddr_in addr;
  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(s_info->port));
  addr.sin_addr.s_addr = INADDR_ANY;
  

  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
  {
    fprintf(stderr, "Socket error:%s\n",strerror(errno));
    return -1;
  }
  
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
  {
    close (listenfd);
    fprintf(stderr, "Bind error:%s\n",strerror(errno));
    return -1;    
  }

  if (listen(listenfd, MAX_LISTEN) == -1)
  {
    fprintf(stderr, "Listen error:%s\n",strerror(errno));
    return -1;    
  }
  
  return listenfd;
}

int server_config_init(struct pollfd *client, int listenfd)
{
  int i;

  client[0].fd = listenfd;
  client[0].events = POLLIN;
  
  for (i = 1; i < MAX_CLIENTS ; i++)
    client[i].fd = -1;

  return 0;
}

client_list *create_list()
{
  client_list *list = malloc(sizeof(*client_list));
  list->head == NULL;
  return list;
}

client_info *list_add(client_list *cli_list, int sockfd)
{
  client_info *cli_info;
  
  cli_info = malloc(sizeof(*cli_info));
  cli_info->sockfd = sockfd;
  cli_info->next = NULL;
  if(cli_list->head == NULL)
  cli_list->head = cli_info;
  else
  {
    client_info *i = cli_list->head;
    while(i->next)
     i = i->next;
    i->next = cli_info;
  }
  cli_list.list_len++;
  return cli_info; 
}

int check_connection(struct pollfd *client, struct server_info *s_info, 
                     client_list *cli_list, int listenfd)
{
  int i, connfd;
  socklen_t clilen;
  struct sockaddr_storage cli_addr;
    
    clilen = sizeof(cli_addr);
    connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);
    
    for (i = 1; i < MAX_CLIENTS; i++)
    {
      if (client[i].fd < 0)
      {
        client[i].fd = connfd;
        client[i].events = POLLIN;
        list_add(cli_list, connfd);
        break;
      }
    }
    
   if (i > s_info->max_i_cli) 
     s_info->max_i_cli = i; 
  
  return 0;
}

int get_http_request(struct pollfd *client, struct client_info *cli_info,
                     int cli_num)
{     
  int num_bytes_read = 0;
  int num_bytes_aux = 0;
  
  cli_info[cli_num].buffer = (char *) calloc(BUFSIZE, sizeof(char));
  if (cli_info[cli_num].buffer == NULL)
    return -1;
    
  while (num_bytes_aux < BUFSIZE - 1)
  {   
    num_bytes_read = recv(client[cli_num].fd, cli_info[cli_num].buffer +
                          num_bytes_aux, BUFSIZE - num_bytes_aux - 1, 0);
                         
    if((strstr(cli_info[cli_num].buffer, "\r\n\r\n")) != NULL ||
    (strstr(cli_info[cli_num].buffer, "\n\n")) != NULL)
    break;

    if (num_bytes_read < 0)
    {
      if(errno == ECONNRESET)
      {
        close(client[cli_num].fd);
        client[cli_num].fd = -1;
        return -1;
      }
      else
      {
        fprintf(stderr, "Recv error:%s\n", strerror(errno));
        return -1;
      }
    }  
    else if(num_bytes_read == 0)
    {
      close(client[cli_num].fd);
      client[cli_num].fd = -1;
      return -1;
    }
    
    if (num_bytes_read == 0)
    break;
    
    num_bytes_aux += num_bytes_read;     
  }
  return 0;
}

int parse_http_request(struct client_info *cli_info, int cli_num,
                       const char *dir_path)
{
  int ret;
  char path[PATH_MAX];

  strcpy(path, dir_path);
  strcat(path, "/");

  if((ret = sscanf(cli_info[cli_num].buffer, "%*[^ ] %s", path +
                   strlen(path))) != 1)
    return -1;
  
  realpath(path, cli_info[cli_num].file_path); 
  //strcat(cli_info[cli_num].file_path, path);
  
  return 0;
}
  
int verify_path(const char *path)
{
  if(strstr(path, "../") != NULL)
    return FORBIDDEN;  
  if(access(path, R_OK) == 0)
    return OK;
  else
    return NOT_FOUND;
    
  return -1;
}

int send_http_response_header(struct client_info *cli_info, int cli_num,
                              struct pollfd *client)
{
  char response_msg[HEADERSIZE];
  if(cli_info[cli_num].request_status ==  200)
  {
    snprintf(response_msg, sizeof(response_msg),
             "HTTP/1.0 200 OK\r\n\r\n");
    if (send(client[cli_num].fd, response_msg, strlen(response_msg), 0) < 0)
    {
      fprintf(stderr, "Send error:%s\n", strerror(errno));
      return -1;
    }    
  }
  cli_info[cli_num].header_sent = true;
  return 0;
}

int open_file(struct client_info *cli_info, int cli_num)
{  
  cli_info[cli_num].fp = fopen(cli_info[cli_num].file_path, "rb");
  if (cli_info[cli_num].fp == NULL)
    return -1;
  
  return 0;
}

int send_requested_data(struct pollfd *client, struct client_info *cli_info,
                        int cli_num)
{
  int num_bytes_read = 0;
 
  num_bytes_read = fread(cli_info[cli_num].buffer, sizeof(char), BUFSIZE,
                         cli_info[cli_num].fp);

  if (num_bytes_read < BUFSIZE)
    cli_info[cli_num].file_finished = true;

  if (send(client[cli_num].fd, cli_info[cli_num].buffer, 
           num_bytes_read, 0) < 0)
  {
    fprintf(stderr, "Send error:%s\n", strerror(errno));
    return -1;
  }
  
  return 0;
}
 
int main (int argc, char *argv[])
{
  int listenfd;
  //struct pollfd client[MAX_CLIENTS];
  struct server_info s_info;
  struct client_list *cli_list = NULL;
 
  memset(cli_list, 0, sizeof(cli_list));
  memset(&s_info, 0, sizeof(s_info));

  if (parse_param(argc, argv[1], argv[2], &s_info) == -1)
    return -1;
  
  listenfd = server_start(&s_info);

  if (server_config_init(client, listenfd) == -1)
    return -1;
  
  if(cli_list = create_list()) == NULL)
  {
    fprintf(stderr, "Error creating list\n");
    return -1;
  }
   
  while (1)
  { 
    int i, nready;
    const client_info *cli_info = client_list->head;  
   
    nready = poll(client, cli_list.list_len  + 1, 0);
    
    if(client[0].revents & POLLIN)
    {
      if(check_connection(client, &s_info, cli_list, listenfd) == -1)
        return -1;
      if(--nready <= 0)
      continue;
    }

    while(cli_list->head)
    {
         
      if(cli_info->sockfd < 0)
       continue;

      if (cli_info->client.revents == POLLIN)
      {
        if (get_http_request(client, cli_info, i) == -1)
        {
          fprintf(stderr,"Erro no get request\n");
          return -1;
        }
        if (parse_http_request(cli_info, i, s_info.dir_path) == -1)
        {
          fprintf(stderr,"Erro no parse request\n");
          return -1;
        }
        
        if ((cli_info[i].request_status = verify_path(cli_info[i].file_path))
                                                      == -1)
        {
          fprintf(stderr,"Erro no path\n");
          return -1;
        }
        
           
        if(open_file(cli_info, i) == -1)
        {
          fprintf(stderr, "File error:%s\n", strerror(errno));
          return -1;
        }

        client[i].events = POLLOUT;
        
        if(--nready <=0)
          continue;
      }

      if (client[i].revents == POLLOUT)
      {
        if(cli_info[i].header_sent == false)
          send_http_response_header(cli_info, i, client);
          
        send_requested_data(client, cli_info, i);
        if(cli_info[i].file_finished == true)
        {
          close(client[i].fd);
          client[i].fd = -1;
          cli_info[i].file_finished = false;
          cli_info[i].header_sent = false;
          fclose(cli_info[i].fp); 
          continue;
        }
      }     
    }  
  } 

return 0;
}

