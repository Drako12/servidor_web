#include "thread.h"
#include "server.h"

pthread_mutex_t lock;


int create_pool(thread_pool *t_pool, int queue_size)
{
  int i;

  t_pool->queue_size = queue_size;
  t_pool->head = t_pool->tail = t_pool->queue_count = 0;
  t_pool->queue = (worker_task *)malloc(sizeof(worker_task) * queue_size);

  pthread_mutex_init(&(pool->lock), NULL);
  pthread_cond_init(&(pool->notify), NULL);
  for (i = 0, i < NTHREADS, i++)
  {
    t_pool->thread_id = i;
    pthread_create(&t_pool->pool[i], NULL, worker_start, &t_pool);
  }

}

int add_to_worker(thread_pool *t_pool, void *worker_task(void *))
{

  pthread_mutex_lock(&(t_pool->lock));

  t_pool->queue[pool->tail].function = worker_task;
  t_pool->tail += 1;
  t_pool->count += 1;

  pthread_cond_signal(&(pool->notify));
  pthread_mutex_unlock(&t_pool);

}

void worker_task(void *arg)
{



}

void *worker_start(thread_pool *t_pool)
{
  worker_task task;
  pthread_mutex_lock(&(t_pool->lock));

  task.function = pool->queue[pool->head].function;
  task.arg = pool->queue[pool->head].arg;


}
