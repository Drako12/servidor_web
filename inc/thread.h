#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include "server.h"
#include <pthread.h>
#include <stdlib.h>

#define NTHREADS 4


typedef struct worker_task_
{

  void (*function)(void *);
  void *arg;
  client_info *head;
  client_info *tail;
  int len;

}worker_task;

typedef struct thread_pool_
{
  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_t pool[NTHREADS];
  worker_task *client_list;
  int thread_id;
  int queue_size;
}thread_pool;

