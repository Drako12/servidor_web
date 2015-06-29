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

static int parse_param(int n_params, char dir_path, char port,
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

static server_start(const struct server_info *s_info)
{
  int sockfd = -1, ret = 0; 
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

int server_config_init(struct pollfd *client, int *maxi)
{
  int i;

  client[0]->fd = listenfd;
  client[0]->events = POLLRDNORM;
  
  for (i = 1; i < 100; i++)
    client[i]->fd = -1;
  maxi = 0;

  return 0;
}

int set_poll(struct pollfd *client, struct sockaddr_storage *cli_addr
             int sockfd, int *maxi, socklen_t *clilen, int *client_num) 
{
  int connfd, nready = 0;
   
  nready =  poll(client, maxi + 1, -1);
  
  if (client[0]->revents & POLLRDNORM)
  {
    clilen = sizeof(cli_addr);
    connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

   for (client_num = 1; client_num < 100; client_num++)
   {
     if (client[client_num]->fd < 0)
     {
       client[client_num]->fd = connfd;
       break;
     }
   }
   
   if(client_numm == 100)
     return -1;
     
   client[client_num]->events = POLLRDNORM;
   
   if (i > maxi)
     maxi = i;

   if (--nready <= 0)
     continue;
  }

  return nready;
}
int check_connection(struct pollfd *client, int sockfd, int *maxi,
                     int *client_num)
{
  for (client_num= 1; client_num <= maxi; i++)
  {
    if ((sockfd = client[client_num]->fd) < 0)
      continue;
      
    if (client[client_num]->revents & (POLLRDNORM | POLLERR))
      return sockfd;
  }
}

int get_http_msg(struct pollfd *client, char *buffer, int sockfd, 
                 int client_num)
{     
  int num_bytes_read = 0;
  int num_bytes_aux = 0;

  while (num_bytes_aux < BUFSIZE - 1)
  {   
    num_bytes_header = recv(sockfd, header + num_bytes_aux, BUFSIZE -
                            num_bytes_aux - 1, 0);
                          
    if (num_bytes_read < 0)
    {
      if(errno == ECONNRESET)
      {
        close(sockfd);
        client[client_num]->fd = -1;
      }
      else
      {
        fprintf(stderr, "Recv error:%s\n", strerror(errno));
        return -1;
      }
      else if(num_bytes_read == 0)
      {
        close(sockfd);
        client[client_num]->fd = -1;
      }
    }

    if (num_bytes_header == 0)
      break;
    
    num_bytes_aux += num_bytes_read;     
  } 
}

int parse_http_msg(char *buffer, char *filename, char *path)
{
  int ret;

  if((ret = sscanf(buffer, "%*[^ ] %s", path)) != 1)
    
  
  strncpy(filename, strrchr(path), NAME_MAX);
  return 0;
}
        
int main (int argc, char *argv[])
{
  int nready, maxi, client_num, sockfd, len;
  socklen_t clilen;
  ssize_t n;
  FILE *fp;
  char buffer[BUFSIZE];
  char *buffer2;
  char *start_line, *end_line, filename[NAME_MAX], path[PATH_MAX];
  struct sockaddr_storage cli_addr;
  struct pollfd client[MAX_CLIENTS];
  struct server_info s_info; 
 
  memset(buffer, 0, sizeof buffer);
  memset(&s_info, 0, sizeof(s_info));

  if (parse_param(argc, argv[1], argv[2], &s_info) == -1)
    return -1;
  
  sockfd = server_start(&s_info);

  if (server_config_init(&client, &maxi) == -1)
    return -1;
 
  while (1)
  {
    
    nready = set_poll(&client, &cli_addr, sockfd, &maxi, &clilen, &client_num);
    
    if ((sockfd = check_connection(&client, sockfd, &maxi, &client_num)) == -1)
       return -1;
    if (get_http_msg(&client, &buffer, sockfd, client_num) == -1)
       return -1;
    parse_http_msg(buffer);   
        
                  
          start_line = buffer;
          if (memcmp ("GET", start_line, 3) == 0 )
          {
            start_line += 4;
            end_line = strchr(start_line, ' ');
            len = end_line - start_line;
            filename = (char*) calloc(len + 1, sizeof(char));
            memmove(filename, start_line, len);
            
            char filename_path[1000];
            char header[1000];
            int status = 0;
            if(filename[0] == '/')
            {
              memmove(filename, filename + 1, len); 
            }

            if(memcmp(filename, "../", 3) == 0)
            {
              snprintf(header, 1000 * sizeof(char), "HTTP/1.0 403 Forbidden\r\n\r\n");
              write(sockfd, header, strlen(header));
              status = 403;
            }

            if(path[strlen(path) - 1] != '/')
            {
              snprintf(filename_path, sizeof(filename_path),"%s/%s",path,filename);
            }
            else
            {
              snprintf(filename_path, sizeof(filename_path),"%s%s",path,filename);
            }
            
            fp = fopen(filename_path, "r");
            if(fp == NULL && status != 403)
            {               
             if (errno == EACCES)
               snprintf(header, 1000 * sizeof(char), "HTTP/1.0 403 Forbidden\r\n\r\n");
             if (errno == ENOENT) 
               snprintf(header, 1000 * sizeof(char), "HTTP/1.0 404 Not Found\r\n\r\n");
             
             write(sockfd, header , strlen(header) );
            }
            else if(status != 403)
            {
            snprintf(header, 1000 * sizeof(char), "HTTP/1.0 200 OK\r\n\r\n");
            write(sockfd, header , strlen(header) );
            
            fseek(fp, 0, SEEK_END);
            len = ftell(fp);
            rewind(fp);
            buffer2 = (char*) malloc(sizeof(char)*len);
            n = fread(buffer2, 1 , len, fp);
            write(sockfd, buffer2, n);
            fclose(fp);
            close(sockfd);
            client[i].fd = -1;
            }
          }
         
         memset (buffer, 0, sizeof buffer);
          
        }
        if (--nready <= 0)
          break;
      }
    }
  }
}

