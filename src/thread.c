#include "thread.h"

int create_pool_and_queue_init(thread_pool *t_pool)
{
  int i;

  t_pool->queue.head = t_pool->queue.tail = t_pool->queue.size = 0;

  pthread_mutex_init(&(t_pool->lock), NULL);
  pthread_cond_init(&(t_pool->notify), NULL);

  for (i = 0, i < NTHREADS, i++)
  {
    t_pool->thread_id = i;
    pthread_create(&t_pool->pool[i], NULL, handle_thread, &t_pool->pool[i]);
  }

}

void *handle_thread(thread_pool *t_pool)
{
  jobs *job;
  pthread_mutex_lock(&t_pool->lock);

  while(1)
  {
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

}
