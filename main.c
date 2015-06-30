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
  if(n_params > 3 || n_params < 3)
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
  int sockfd = -1; 
  struct sockaddr_in addr;
  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(s_info->port));
  addr.sin_addr.s_addr = INADDR_ANY;
  

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
  {
    fprintf(stderr, "Socket error:%s\n",strerror(errno));
    return -1;
  }

  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
  {
    close (sockfd);
    fprintf(stderr, "Bind error:%s\n",strerror(errno));
    return -1;    
  }

  if (listen(sockfd, 10) == -1)
  {
    fprintf(stderr, "Listen error:%s\n",strerror(errno));
    return -1;    
  }
  
  return sockfd;
}

int server_config_init(struct pollfd *client, int sockfd,
                       struct client_info *cli_info)
{
  int i;

  client[0].fd = sockfd;
  client[0].events = POLLIN;
  
  for (i = 1; i < MAX_CLIENTS ; i++)
  {
    client[i].fd = -1;
    cli_info[i].status = NEW;
  }
  
  return 0;
}

int set_poll(struct pollfd *client, int *max_i_cli)              
{
  int connfd = 0, client_num, clifd_num;
    
  clifd_num =  poll(client, *max_i_cli + 1, -1);
}

int check_connection(struct pollfd *client, int sockfd, int max_i_cli, 
                     int client_num, struct sockaddr_storage *cli_addr,
                     socklen_t *clilen)
{
  if (client[0].revents & POLLIN)
  {
    *clilen = sizeof(cli_addr);
    connfd = accept(sockfd, (struct sockaddr *)&cli_addr, clilen);
    
    for (client_num = 1; client_num < MAX_CLIENTS; client_num++)
    {
      if (client[client_num].fd < 0)
      {
        client[client_num].fd = connfd;
        client[client_num].events = POLLIN;
        *max_i_cli = client_num; 
        return clifd_num;
      }
    }   
  }
  return -1;  

   if ((sockfd = client[client_num].fd) < 0)
      return -1;
      
   if (client[client_num].revents & (POLLIN | POLLERR))
      return sockfd;

  return -1;
}

int get_http_request(struct pollfd *client, char *header, int sockfd, 
                 int client_num)
{     
  int num_bytes_read = 0;
  int num_bytes_aux = 0;

  while (num_bytes_aux < BUFSIZE - 1)
  {   
    num_bytes_read = recv(sockfd, header + num_bytes_aux, BUFSIZE -
                            num_bytes_aux - 1, 0);
                          
    if (num_bytes_read < 0)
    {
      if(errno == ECONNRESET)
      {
        close(sockfd);
        client[client_num].fd = -1;
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
      close(sockfd);
      client[client_num].fd = -1;
      return -1;
    }
    
    if (num_bytes_read == 0)
    break;
    
    num_bytes_aux += num_bytes_read;     
  }
  return 0;
}

int parse_http_request(const char *header, int cli_num, 
                       struct client_info *cli_info)
{
  int ret;

  if((ret = sscanf(header, "%*[^ ] %s", 
                   cli_info[cli_num].file_path)) != 1)
    return -1;
  
  //strncpy(filename, strrchr(path), NAME_MAX);
  return 0;
}


int verify_path(const http_code http_response_code, const char *path)
{
  if(strstr(path, "../") == NULL)
    return FORBIDDEN;  
  if(access(path, R_OK) == 0)
    return OK;

  return -1;
}

int send_http_response_header(const http_code http_response_code,
                              int sockfd)
{
  char response_msg[HEADERSIZE];
  if(http_response_code ==  200)
  {
    snprintf(response_msg, sizeof(response_msg),
             "HTTP/1.0 200 OK\r\n\r\n");
    if (send(sockfd, response_msg, strlen(response_msg), 0) < 0)
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
  int i, max_i_cli = 0, sockfd, clifd_num;
  socklen_t clilen;
  FILE *fp;
  char header[BUFSIZE];
  char content[BUFSIZE];
  struct sockaddr_storage cli_addr;
  struct pollfd client[MAX_CLIENTS];
  struct server_info s_info;
  struct client_info cli_info[MAX_CLIENTS];
  http_code http_response_code;

  memset(header, 0, sizeof(header));
  memset(content, 0, sizeof(content));
  memset(&s_info, 0, sizeof(s_info));

  if (parse_param(argc, argv[1], argv[2], &s_info) == -1)
    return -1;
  
  sockfd = server_start(&s_info);

  if (server_config_init(client, sockfd, cli_info) == -1)
    return -1;
 
  while (1)
  {    
    clifd_num = set_poll(client, &max_i_cli, &cli_addr,
                         sockfd, &clilen);
    if(--clifd_num <= 0)
      continue;
    
    for(i = 1; i <= max_i_cli; i++)
    {
      if ((sockfd = check_connection(client, sockfd, i)) == -1)
        continue;

      if (client[i].revents == POLLIN && cli_info[i].status == NEW)
      {
        if (get_http_request(client, header, sockfd, i) == -1)
          return -1;
        if (parse_http_request(header, i, cli_info) == -1)
          return -1;
        if ((http_response_code = verify_path(http_response_code, 
                                  cli_info[i].file_path)) == -1)
          return -1;
        cli_info[i].status = STARTED;  
        client[i].events = POLLOUT;
      }
      if (client[i].revents == POLLOUT &&
          cli_info[i].status == STARTED)
      {
        send_http_response_header(http_response_code, sockfd);
       
      }
      
    }
  
 } 
  /*
      
    
    if(!(fp = open_file(path)))
    {
      fprintf(stderr, "File error:%s\n", strerror(errno));
      return -1;
    }
    send_requested_data(fp, &buffer, path, sockfd);
    //close_connection();
    close(sockfd);
    client[client_num].fd = -1;
           
    if (--clifd_num <= 0)
    break;
  } */
return 0;
}

