#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#define NTHREADS 4

typedef struct jobs_
{
  void (*function)(void *);
  void *arg;
  bool error;
  struct jobs_ *next;
} jobs;

typedef struct job_queue_
{
  jobs *head;
  int size;
} job_queue;

typedef struct thread_pool_
{
  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_t threads[NTHREADS];
  job_queue queue;
  bool finished;
} thread_pool;



void *handle_thread(void *pool);
int add_job(thread_pool *t_pool, void (*function)(void *), void *arg);
jobs *get_job(thread_pool *t_pool);
int do_job(jobs *job, thread_pool *t_pool);
int create_pool_and_queue_init(thread_pool *t_pool);

#endif
