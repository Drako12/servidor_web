#include "network.h"

ssize_t readn (int fd, void *vptr, size_t n)
{
  size_t nleft;
  ssize_t nread;
  char *ptr;

  ptr = vptr;
  nleft = n;
  while (nleft > 0)
  {
    if ( (nread = read (fd, ptr, nleft) ) < 0)
    {
      if (errno == EINTR)
      {
        nread = 0;
      }
      else
        return (-1);
    }
    else if (nread == 0)
      break;

    nleft -= nread;
    ptr += nread;
  }
  return (n - nleft);
}

ssize_t writen (int fd, const void *vptr, size_t n)
{
  size_t nleft;
  ssize_t nwritten;
  const char *ptr;

  ptr = vptr;
  nleft = n;
  while (nleft > 0)
  {
    if ( (nwritten = write (fd, ptr, nleft) ) <= 0)
    {
      if (nwritten < 0 && errno == EINTR)
      {
        nwritten = 0;
      }
      else
      {
        return (-1);
      }
    }

    nleft -= nwritten;
    ptr += nwritten;
  }
  return (n);
}

int main (int argc, char *argv[])
{
  char *path;
  int connfd, nready, maxi, i, listenfd, sockfd, len;
  socklen_t clilen;
  ssize_t n;
  FILE *fp;
  char buffer[BUFSIZE];
  char *buffer2;
  char *start_line, *end_line, *filename;
  struct sockaddr_in addr;//, *servinfo, *p;
  struct sockaddr_storage cli_addr;
  struct pollfd client[100];
 
  memset (buffer, 0, sizeof buffer);
  path = argv[1];    
 
  if(argc > 3)
  {
    printf("argumentos demais");
    exit(1);
  }
 
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(argv[2]));
  addr.sin_addr.s_addr = INADDR_ANY;

  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
  {
    perror ("ERRO NO SOCKET");
    exit(1);
  }

  if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
  {
    close (listenfd);
    perror ("server: bind");
    exit(1);
  }  
  if (listen(listenfd, 10) == -1)
  {
    perror("listen");
    exit (1);
  }

  client[0].fd = listenfd;
  client[0].events = POLLRDNORM;
  
  for (i = 1; i < 100; i++)
    client[i].fd = -1;
  maxi = 0;

  while (1)
  {
    nready = poll (client, maxi + 1, -1);
    if (client[0].revents & POLLRDNORM)
    {
      clilen = sizeof (cli_addr);
      connfd = accept (listenfd, (struct sockaddr *) &cli_addr, &clilen);

      for (i = 1; i < 100; i++)
      {
        if (client[i].fd < 0)
        {
          client[i].fd = connfd;
          break;
        }
      }


      client[i].events = POLLRDNORM;
      if (i > maxi)
        maxi = i;

      if (--nready <= 0)
        continue;
    }
    for (i = 1; i <= maxi; i++)
    {
      if ( (sockfd = client[i].fd) < 0)
        continue;
      if (client[i].revents & (POLLRDNORM | POLLERR) )
      {
        if ( (n = read (sockfd, buffer, BUFSIZE) ) < 0)
        {
          if (errno == ECONNRESET)
          {
            close (sockfd);
            client[i].fd = -1;
          }
          else
          {
            printf ("read error");
          }
        }
        else if (n == 0)
        {
          close (sockfd);
          client[i].fd = -1;
        }
        else
        {
          if(memcmp("exit",buffer,4) == 0)
          {
            close(sockfd);
            client[i].fd = -1;
          }
                  
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
              writen(sockfd, header, strlen(header));
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
             
             writen(sockfd, header , strlen(header) );
            }
            else if(status != 403)
            {
            snprintf(header, 1000 * sizeof(char), "HTTP/1.0 200 OK\r\n\r\n");
            writen(sockfd, header , strlen(header) );
            
            fseek(fp, 0, SEEK_END);
            len = ftell(fp);
            rewind(fp);
            buffer2 = (char*) malloc(sizeof(char)*len);
            n = fread(buffer2, 1 , len, fp);
            writen(sockfd, buffer2, n);
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

