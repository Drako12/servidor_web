#include "thread.h"

int create_pool_and_queue_init(thread_pool *t_pool)
{
  int i;

  t_pool->queue.size = 0;

  pthread_mutex_init(&(t_pool->lock), NULL);
  pthread_cond_init(&(t_pool->notify), NULL);

  for (i = 0; i < NTHREADS; i++)
  {
    t_pool->thread_id = i;
    pthread_create(&t_pool->pool[i], NULL, handle_thread, &t_pool->pool[i]);
  }

return 0;
}

void *handle_thread(void *pool)
{
  thread_pool *t_pool = (thread_pool *)pool; 
  jobs *job;
  pthread_mutex_lock(&t_pool->lock);
  int sockfd;
  
  struct sockaddr_un client_addr;
//rever essa parte do codigo, acho que a comunicacao esta incorreta
  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sun_family = AF_UNIX;
  strcpy(client_addr.sun_path, "/tmp/SERVER_SOCKET");
  sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
  connect(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr));

  while(1)
  { 
    send(sockfd, "trabaia rapaiz", 10, 0); 
    if(t_pool->queue.size > 0)
    {
      job = get_job(t_pool);
      if(job)
        do_job(job, t_pool);
    }

    pthread_cond_wait(&t_pool->notify, &t_pool->lock);
  }
}


int add_job(thread_pool *t_pool, void (*function)(void *), void *arg)
{
  pthread_mutex_lock(&(t_pool->lock));

  jobs *job;
  job = calloc(1, sizeof(*job));

  if (t_pool->queue.head == NULL)
    t_pool->queue.head = job;
  else
  {
    jobs *i = t_pool->queue.head;

    while (i->next)
      i = i->next;
    i->next = job;
  }

  job->function = function;
  job->arg = arg;
  job->position += 1;
  t_pool->queue.tail = job;
  t_pool->queue.size += 1;

  pthread_cond_signal(&(t_pool->notify));
  pthread_mutex_unlock(&t_pool->lock);

  return 0;
}

jobs *get_job(thread_pool *t_pool)
{
  jobs *job;

  pthread_mutex_lock(&(t_pool->lock));

  job = t_pool->queue.head;

  if (t_pool->queue.size > 0)
  {
    t_pool->queue.head = job->next;
  }
  pthread_mutex_unlock(&(t_pool->lock));
  return job;
}

int do_job(jobs *job, thread_pool *t_pool)
{
  pthread_mutex_lock(&(t_pool->lock));
  job->function;
  pthread_mutex_unlock(&(t_pool->lock));

return 0;
}
