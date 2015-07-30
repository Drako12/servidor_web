#include "thread.h"

int create_pool_and_queue_init(thread_pool *t_pool)
{
  int i;

  t_pool->queue.size = 0;

  pthread_mutex_init(&(t_pool->lock), NULL);
  pthread_cond_init(&(t_pool->notify), NULL);

  for (i = 0; i < NTHREADS; i++)
    pthread_create(&(t_pool->threads[i]), NULL, handle_thread, t_pool);


return 0;
}

void *handle_thread(void *pool)
{
  int sockfd;
  struct sockaddr_un main_addr;
  thread_pool *t_pool = (thread_pool *)pool;
  jobs *job;

  memset(&main_addr, 0, sizeof(main_addr));
  main_addr.sun_family = AF_UNIX;
  strcpy(main_addr.sun_path, "/tmp/SERVER_SOCKET");
  sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
  connect(sockfd, (struct sockaddr *)&main_addr, sizeof(main_addr));

  while (1)
  {
    pthread_mutex_lock(&(t_pool->lock));

    if (t_pool->queue.size == 0)
      pthread_cond_wait(&(t_pool->notify), &(t_pool->lock));

    job = get_job(t_pool);
    pthread_mutex_unlock(&(t_pool->lock));

    if (job)
    {
      void (*func)(void *arg);
      void *args;

      func = job->function;
      args = job->arg;
      func(args);
      write(sockfd, &job->arg, sizeof(job->arg));
      free(job);
    }
  }
}


int add_job(thread_pool *t_pool, void (*function)(void *), void *arg)
{
  jobs *job;
  job = calloc(1, sizeof(*job));

  pthread_mutex_lock(&(t_pool->lock));
  if (t_pool->queue.head == NULL)
  {
    t_pool->queue.head = job;
  }
  else
  {
    jobs *i = t_pool->queue.head;

    while (i->next)
      i = i->next;
    i->next = job;
  }

  t_pool->queue.size += 1;
  job->function = function;
  job->arg = arg;

  if (t_pool->queue.size >= 1)
    pthread_cond_signal(&(t_pool->notify));

  pthread_mutex_unlock(&(t_pool->lock));

  return 0;
}

jobs *get_job(thread_pool *t_pool)
{
  jobs *job;

  job = t_pool->queue.head;

  if (t_pool->queue.size > 0)
  {
    t_pool->queue.head = job->next;
    t_pool->queue.size--;
  }
  return job;
}
