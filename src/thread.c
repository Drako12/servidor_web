#include "thread.h"

/*!
 * \brief Cria a pool de threads e inicia as variaveis mutex e cond
 *
 * param[out] t_pool pool de threads
 *
 * return 0 para tudo ok
 * return -1 caso de algum erro
 *
 */

int create_pool_and_queue_init(thread_pool *t_pool)
{
  int i;

  t_pool->queue.size = 0;
  t_pool->finished = false;

  if (pthread_mutex_init(&(t_pool->lock), NULL) != 0)
    return -1;
  if (pthread_cond_init(&(t_pool->notify), NULL) != 0)
    return -1;

  for (i = 0; i < NTHREADS; i++)
    if (pthread_create(&(t_pool->threads[i]), NULL, handle_thread, t_pool)
        != 0)
      return -1;

return 0;
}

/*!
 * \brief Funcao executada pela thread na criacao, ela executa os jobs e
 * escreve no socket local ao finalizar
 *
 * param[in] pool Pool de threads
 *
 *
 */
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
  if (connect(sockfd, (struct sockaddr *)&main_addr, sizeof(main_addr)) == -1)
    t_pool->finished = true;


  while (!t_pool->finished)
  {
    job = get_job(t_pool);

    if (job)
    {
      job->function(job->arg);

      if (write(sockfd, &job->arg, sizeof(job->arg)) < 0)
        return NULL;

      free(job);
    }
  }
  return NULL;
}


/*!
 * \brief Adiciona um job ao queue de jobs
 *
 * param[out] function variavel que contem a funcao a ser adicionada ao job
 * param[in] t_pool Pool de threads
 *
 * return 0 para tudo ok
 * return -1 para erro
 *
 */

int add_job(thread_pool *t_pool, void (*function)(void *), void *arg)
{
  jobs *job;
  job = calloc(1, sizeof(*job));

  pthread_mutex_lock(&(t_pool->lock));

  if (t_pool->queue.head == NULL)
    t_pool->queue.head = job;
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

  pthread_cond_signal(&(t_pool->notify));
  pthread_mutex_unlock(&(t_pool->lock));

  return 0;
}

/*!
 * \brief Retorna um job da queue e diminui o tamanho dela em 1
 *
 * param[in] t_pool Pool de threads
 *
 * return job
 *
 */

jobs *get_job(thread_pool *t_pool)
{
  jobs *job;

  pthread_mutex_lock(&(t_pool->lock));

  if (t_pool->queue.size == 0)
    pthread_cond_wait(&(t_pool->notify), &(t_pool->lock));

 job = t_pool->queue.head;

  if (t_pool->queue.size > 0)
  {
    t_pool->queue.head = job->next;
    t_pool->queue.size--;
  }
  pthread_mutex_unlock(&(t_pool->lock));

  return job;
}
