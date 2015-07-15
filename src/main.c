#include "server.h"

int main(int argc, char *argv[])
{
  int listenfd;
  server_info s_info;
  client_list cli_list;
  memset(&s_info, 0, sizeof(s_info));

  if (parse_param(argc, argv[1], argv[2], argv[3], &s_info) == -1)
    return -1;
  
  if ((listenfd = server_start(&s_info)) == -1)
    return -1;
 
  server_init(&cli_list, listenfd); 
    
  while (1)
  { 
    int cli_num, ret;
    long wait = 0;
     
    reset_poll(&cli_list, listenfd);

    wait = find_poll_wait_time(&cli_list);
    
    if (cli_list.list_len == 0)
      wait = -1;

    if ((ret = poll(cli_list.client, cli_list.list_len + 1 , wait)) < 0)
      continue;

    if (cli_list.client[SERVER_INDEX].revents & POLLIN)
      if (check_connection(&cli_list, listenfd, s_info.client_rate)
                           == -1 || --ret <= 0)
        continue;
          
   
    client_info *cli_info = cli_list.head;  
    cli_num = SERVER_INDEX + 1;

    while (cli_info)
    {     
      int sockfd, ret;
           
      sockfd = cli_list.client[cli_num].fd;
      
      if (cli_list.client[cli_num].revents & (POLLIN | POLLRDNORM))
      {
        if ((ret = process_http_request(cli_info, s_info.dir_path, sockfd)
                                        < 0))
        {
          if (ret == -1)
            close_connection(cli_info, &cli_list, cli_num);
          break;
        }
        cli_list.client[cli_num].events = POLLOUT;                       
      }

      if (cli_list.client[cli_num].revents & POLLOUT)
        if (process_bucket_and_send_data(cli_info, &s_info, sockfd) == -1)
        {
          close_connection(cli_info, &cli_list, cli_num);
          break;
        }      

      if (cli_list.client[cli_num].revents & (POLLERR | POLLHUP | POLLNVAL))
      {
        close_connection(cli_info, &cli_list, cli_num);
        break;
      }

      cli_info = cli_info->next;
      cli_num++;
    }  
  } 

return 0;
}

