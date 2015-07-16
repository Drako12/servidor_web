#include "server.h"
#include "bucket.h"

/*int finished = 0;

void signal_callback_handler(int signum)
{
  if (signum == SIGINT)
    finished = 1;
}
*/
int main(int argc, char *argv[])
{
  int listenfd;
 // struct sigaction action;
  server_info s_info;
  client_list cli_list;
// sigset_t sigmask;

  memset(&s_info, 0, sizeof(s_info));
  
/*  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  action.sa_handler = signal_callback_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, 0);
  sigprocmask(SIG_BLOCK, &sigmask, NULL);
*/
  if (parse_param(argc, argv[1], argv[2], argv[3], &s_info) == -1)
    return -1;
  
  if ((listenfd = server_start_listen(&s_info)) == -1)
    return -1;
 
  server_init(&cli_list, listenfd); 
    
  while (1)// || !finished)
  { 
    int cli_num, ret;
    struct timespec poll_wait;
    const struct timespec *time_p = NULL;

    if (cli_list.list_len > 0)
    {
      poll_wait = find_poll_wait_time(&cli_list);
      time_p = &poll_wait;
    }
    reset_poll(&cli_list, listenfd);
   
    if ((ret = ppoll(cli_list.client, cli_list.list_len + 1, time_p,
                      NULL)) < 0)
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

      else if (cli_list.client[cli_num].revents & POLLOUT)
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

cleanup(&cli_list); 
return 0;
}

