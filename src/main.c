#include "server.h"

int main (int argc, char *argv[])
{
  int listenfd;
  struct server_info s_info;
  //bucket tbc;
  client_list cli_list;
  memset(&s_info, 0, sizeof(s_info));

  if (parse_param(argc, argv[1], argv[2], &s_info) == -1)
    return -1;
  
  if ((listenfd = server_start(&s_info)) == -1)
    return -1;
 
  server_init(&cli_list, listenfd); 
    
  while (1)
  { 
    int ret, i = SERVER_INDEX + 1;
    
    reset_poll(&cli_list, listenfd);
       
    if((ret = poll(cli_list.client, cli_list.list_len + 1 , 0)) < 0)
      continue;

    if (cli_list.client[SERVER_INDEX].revents & POLLIN)
      if (check_connection(&cli_list, listenfd) == -1 || --ret <= 0)
        continue;
          
    client_info *cli_info = cli_list.head;  
        
    while(cli_info)
    {     
      int cli_num; 
      cli_num = i;

      if (cli_list.client[cli_num].revents & (POLLIN | POLLRDNORM))
      {
        if (get_http_request(cli_info, cli_list.client[cli_num].fd) == -1)
        {
          close_connection(cli_info, &cli_list, cli_num);
          break;
        }

        if (parse_http_request(cli_info, s_info.dir_path) == -1)
        {
          close_connection(cli_info, &cli_list, cli_num);
          break;
        }
               
        if (cli_info->request_status == OK)
          open_file(cli_info); 


        cli_list.client[cli_num].events = POLLOUT;                       
      }

      if (cli_list.client[cli_num].revents & POLLOUT)
      {
        if (cli_info->header_sent == false)
          send_http_response_header(cli_info, cli_list.client[cli_num].fd);
         // token_buffer_init(&tbc, BUFSIZE - 1, 10);
        else
          if (send_requested_data(cli_info, get_filedata(cli_info),
                              cli_list.client[cli_num].fd) == -1 )
            close_connection(cli_info, &cli_list, cli_num);
            
        //int tokens = 0;
        //tokens = get_filedata(cli_info);
        //while (token_buffer_consume(&tbc, tokens) != 0);
        //send_requested_data(cli_info, tokens, cli_list.client[cli_num].fd); 

        if (cli_info->request_status != OK || feof(cli_info->fp))
        {
          close_connection(cli_info, &cli_list, cli_num);
          break;
        }        
      }

      if (cli_list.client[cli_num].revents & (POLLERR | POLLHUP | POLLNVAL))
        close_connection(cli_info, &cli_list, cli_num);
          
      cli_info = cli_info->next;
      i++;
    }  
  } 

return 0;
}

