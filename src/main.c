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
        if ((ret = get_http_request(cli_info, sockfd)) < 0 )
        {
          if (ret == -1)
            close_connection(cli_info, &cli_list, cli_num);
          break;
        }

        if (parse_http_request(cli_info, s_info.dir_path) == -1)
        {
          close_connection(cli_info, &cli_list, cli_num);
          break;
        }              

        cli_list.client[cli_num].events = POLLOUT;                       
      }

      if (cli_list.client[cli_num].revents & POLLOUT)
      {
        if (cli_info->header_sent == false)
        {

          if (send_http_response(cli_info, sockfd, 
                                 check_request(cli_info, &s_info)) == -1  ||
                                 cli_info->request_status != OK)
          {
            close_connection(cli_info, &cli_list, cli_num);
            break;
          }
          
          open_file(cli_info); 
        }    
        else
        {         
          cli_info->can_send = check_for_consume(&cli_info->tbc);
               
          if (cli_info->can_send == true)                                   
          {                        
            cli_info->tbc.tokens_aux = get_filedata(cli_info);
            token_buffer_consume(&cli_info->tbc, cli_info->tbc.tokens_aux);
            
            if ((send_requested_data(cli_info, sockfd)) == -1)
            {
              close_connection(cli_info, &cli_list, cli_num);
              break;
            }
          
          }

          if (feof(cli_info->fp))
          {
            close_connection(cli_info, &cli_list, cli_num);
            break;
          }          
        }
      }

      if (cli_list.client[cli_num].revents & (POLLERR | POLLHUP | POLLNVAL))
        close_connection(cli_info, &cli_list, cli_num);
          
      cli_info = cli_info->next;
      cli_num++;
    }  
  } 

return 0;
}

