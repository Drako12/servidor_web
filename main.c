#include "server.h"

int main (int argc, char *argv[])
{
  int listenfd;
  struct server_info s_info;
  client_list cli_list;
  memset(&s_info, 0, sizeof(s_info));

  if (parse_param(argc, argv[1], argv[2], &s_info) == -1)
    return -1;
  
  listenfd = server_start(&s_info); 
  server_struct_init(&cli_list, listenfd); 
   
  while (1)
  { 
    int ret;
    reset_poll(&cli_list, listenfd);
       
    ret = poll(cli_list.client, cli_list.max_i + 1 , 0);
    if(ret < 0 || (cli_list.list_len > 0 && ret <= 0))
    continue;

    if (cli_list.client[0].revents & POLLIN)
    {
      if (check_connection(&cli_list, listenfd) == -1)
        return -1;      
      continue;
    }
    
    if (cli_list.head == NULL)
      continue;
      
    client_info *cli_info = cli_list.head;  
    
    while (cli_info)
    {     
     int cli_num;
     cli_num = cli_info->cli_num;

      if (cli_list.client[cli_num].revents & (POLLIN | POLLRDNORM))
      {
        if (get_http_request(cli_info, cli_list.client[cli_num].fd) == -1)
        {
          close_connection(cli_info, &cli_list);
          fprintf(stderr,"Erro no get request\n");
          break;
        }
        if (parse_http_request(cli_info, s_info.dir_path) == -1)
        {
          close_connection(cli_info, &cli_list);
          fprintf(stderr,"Erro no parse request\n");
          break;
        }
               
        if (open_file(cli_info) == -1 || cli_info->request_status != OK)
        {
          close_connection(cli_info, &cli_list);
          fprintf(stderr, "File error:%s\n", strerror(errno));
          break;
        }

        cli_list.client[cli_num].events = POLLOUT;                       
      }

      if (cli_list.client[cli_num].revents & POLLOUT)
      {
        if (cli_info->header_sent == false)
          send_http_response_header(cli_info, cli_list.client[cli_num].fd);
                
        if ((send_requested_data(cli_info, get_filedata(cli_info),
                                 cli_list.client[cli_num].fd)  == -1 ||
                                 feof(cli_info->fp)) &&
                                 cli_info->request_status == OK)
        {
        close_connection(cli_info, &cli_list);
        break;
        }
      }

      if (cli_list.client[cli_num].revents & (POLLERR | POLLHUP | POLLNVAL))
        close_connection(cli_info, &cli_list);
          
      cli_info = cli_info->next;       
    }  
  } 

return 0;
}

