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

int server_config_init(struct pollfd *client, int listenfd,
                       struct client_info *cli_info, const char *path)
{
  int i;

  client[0].fd = listenfd;
  client[0].events = POLLIN;
  
  for (i = 1; i < MAX_CLIENTS ; i++)
  {
    client[i].fd = -1;
    cli_info[i].status = NEW;
    strcpy(cli_info[i].file_path, path);
  }
  
  return 0;
}

int check_connection(struct pollfd *client, struct server_info *s_info, 
                     int listenfd)
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

int parse_http_request(struct client_info *cli_info, int cli_num)
{
  int ret;
  char path[PATH_MAX];

  if((ret = sscanf(cli_info[cli_num].buffer, "%*[^ ] %s", path)) != 1)
    return -1;
  
  strcat(cli_info[cli_num].file_path, path);  
  
  return 0;
}
  
int verify_path(const http_code http_response_code, const char *path)
{
  if(strstr(path, "../") != NULL)
    return FORBIDDEN;  
  if(access(path, R_OK) == 0)
    return OK;

  return -1;
}

int send_http_response_header(const int status, int cli_num,
                              struct pollfd *client)
{
  char response_msg[HEADERSIZE];
  if(status ==  200)
  {
    snprintf(response_msg, sizeof(response_msg),
             "HTTP/1.0 200 OK\r\n\r\n");
    if (send(client[cli_num].fd, response_msg, strlen(response_msg), 0) < 0)
    {
      fprintf(stderr, "Send error:%s\n", strerror(errno));
      return -1;
    }    
  }
  return 0;
}
/*
FILE *open_file(const char path, struct client_info *cli_info, 
                int client_num, FILE *fp)
{
  fp = fopen(path, "r");
  fseek(fp, 0, SEEK_END);
  file_len = ftell(fp);
  rewind(fp);
 
  strncpy(cli_info.path, path, PATH_MAX);
  cli_info.file_len = file_len;

  return fp;
}

int send_requested_data(FILE *fp, char *buffer, char *path, int sockfd)
{
  int num_bytes_read = 0, file_len = 0;

  num_bytes_read = fread(buffer, 1, file_len, fp);
  
  if (send(sockfd, buffer, num_bytes_read, 0) < 0)
  {
    fprintf(stderr, "Send error:%s\n", strerror(errno));
    return -1;
  }
  fclose(fp);
  return 0;
}
*/ 
int main (int argc, char *argv[])
{
  int listenfd;
  FILE *fp;
  char header[BUFSIZE];
  char content[BUFSIZE];
  struct pollfd client[MAX_CLIENTS];
  struct server_info s_info;
  struct client_info cli_info[MAX_CLIENTS];
  http_code http_response_code;

  memset(header, 0, sizeof(header));
  memset(content, 0, sizeof(content));
  memset(&s_info, 0, sizeof(s_info));

  if (parse_param(argc, argv[1], argv[2], &s_info) == -1)
    return -1;
  
  listenfd = server_start(&s_info);

  if (server_config_init(client, listenfd, cli_info, s_info.dir_path) == -1)
    return -1;
 
  while (1)
  { 
    int i, nready;

    nready = poll(client, s_info.max_i_cli + 1, 0);
    
    if(client[0].revents & POLLIN)
    {
      if(check_connection(client, &s_info, listenfd) == -1)
        return -1;
      if(--nready <= 0)
      continue;
    }

    for(i = 1; i <= s_info.max_i_cli; i++)
    {
     if(client[i].fd < 0)
       continue;

      if (client[i].revents == POLLIN)
      {
        if (get_http_request(client, cli_info, i) == -1)
          return -1;
        if (parse_http_request(cli_info, i) == -1)
          return -1;
        if ((cli_info[i].request_status = verify_path(http_response_code, 
                                  cli_info[i].file_path)) == -1)
          return -1;
          
        client[i].events = POLLOUT;
        if(--nready <=0)
          continue;
      }

      if (client[i].revents == POLLOUT)
      {
        send_http_response_header(cli_info[i].request_status, i, client);
        
        if(!(fp = open_file(path)))
        {
          fprintf(stderr, "File error:%s\n", strerror(errno));
          return -1;
        }
        
        send_requested_data(fp, &buffer, path, sockfd);

        if(--nready <= 0)
          break;
      }
      
    }
  
 } 
  /*
      
    
    //close_connection();
    close(sockfd);
    client[client_num].fd = -1;
           
    if (--clifd_num <= 0)
    break;
  } */
return 0;
}

