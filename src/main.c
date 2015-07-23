#include "server.h"
#include "bucket.h"
#include "thread.h"

int finished = 0;

void signal_callback_handler(int signum)
{
  if (signum == SIGINT)
    finished = 1;
}

int main(int argc, char *argv[])
{
  int listenfd, s_socket;
  struct sigaction action;
  struct timespec poll_wait;
  server_info s_info;
  client_list cli_list;
  thread_pool pool;
  sigset_t sigmask;

  memset(&s_info, 0, sizeof(s_info));
  memset(&action, 0, sizeof(action));
  memset(&poll_wait, 0, sizeof(poll_wait));

  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  action.sa_handler = signal_callback_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, 0);
  sigprocmask(SIG_BLOCK, &sigmask, NULL);
  sigemptyset(&sigmask);

  if (parse_and_fill_server_info(argc, argv[1], argv[2], argv[3],
                                 &s_info) == -1)
    return -1;

  if ((listenfd = server_start_listen(&s_info)) == -1)
    return -1;

  s_socket = server_socket_init();

  client_list_init(&cli_list, listenfd, s_info.max_clients, s_socket);

  create_pool_and_queue_init(&pool);

  while (!finished)
  {
    int cli_num, ret;
    const struct timespec *time_p = NULL;

    if (cli_list.list_len > 0)
      time_p = &poll_wait;

    if ((ret = ppoll(cli_list.client, cli_list.list_len + 2, time_p,
                      &sigmask)) < 0)
      continue;

    if (cli_list.client[SERVER_INDEX].revents & POLLIN)
      if (check_connection(&cli_list, listenfd, &s_info) == -1 || --ret <= 0)
        continue;

    if(cli_list.client[LOCAL_SOCKET].revents & POLLIN)
      get_thread_msg(&cli_list);


    client_info *cli_info = cli_list.head;
    cli_num = LOCAL_SOCKET + 1;

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
        if (process_bucket_and_send_data(cli_info, &s_info, sockfd,
            &pool, &cli_list, cli_num) == -1)
        {
          close_connection(cli_info, &cli_list, cli_num);
          break;
        }

      if (cli_list.client[cli_num].revents & (POLLERR | POLLHUP | POLLNVAL))
      {
        close_connection(cli_info, &cli_list, cli_num);
        break;
      }

      //if (cli_info->bytes_read > 0)
       // set_clients(cli_info, &cli_list, cli_num);
      poll_wait = find_poll_wait_time(cli_info, poll_wait);
      cli_info = cli_info->next;
      cli_num++;
    }
  }

cleanup(&cli_list);
return 0;
}

