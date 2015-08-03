#include "bucket.h"

/*!
 * \brief Funcao para pegar o tempo atual (timestamp)
 *
 * \return tempo em milisegundos
 */

long long time_now()
{
  struct timespec time;
  clock_gettime(CLOCK_REALTIME, &time);
  return (long long)(time.tv_sec * 1000 + time.tv_nsec/1000000);
}

/*!
 * \brief Funcao para recarregar o bucket de acordo com o tempo passado
 *
 * \param[in] bucket Estrutura do token bucket dentro da estrutura do cliente
 *
 */

static void refill_tokens(t_bucket *bucket)
{
  long long now, delta;

  now = time_now();
  delta = (now - bucket->timestamp);

  if (bucket->tokens < bucket->capacity)
  {
    long long new_tokens;

    new_tokens = delta * bucket->rate * 0.001;

    if (bucket->tokens + new_tokens < bucket->capacity)
      bucket->tokens = bucket->tokens + new_tokens;
    else
      bucket->tokens = bucket->capacity;
  }
  bucket->timestamp = now;
}

/*!
 * \brief Funcao para inicializar o bucket
 *
 * \param[in] tokens Numero de tokens na inicializacao
 * \param[in] capacity Capacidade total do bucket
 * \param[in] rate Rate de transmissao
 * \param[out] bucket Estrutura do token bucket
 *
 */

void bucket_init(t_bucket *bucket, long long tokens, long long capacity, long long rate)
{
  bucket->capacity = capacity;
  bucket->tokens = tokens;
  bucket->rate = rate;
  bucket->timestamp = time_now();
}

/*!
 * \brief Funcao para consumir os tokens do bucket
 *
 * \param[in] tokens Numero de tokens para ser consumidos
 * \param[in] bucket Estrutura do token bucket
 *
 * \return true se conseguir consumir
 * \return false se nao tiver saldo suficiente para consumir
 */

bool bucket_consume(t_bucket *bucket)
{
  refill_tokens(bucket);

  if (bucket->tokens < bucket->to_be_consumed_tokens)
    return false;

  bucket->tokens -= bucket->to_be_consumed_tokens;
  return true;
}

/*!
 * \brief Funcao para checar se e' possivel ou nao consumir os tokens do bucket
 *
 * \param[in] bucket Estrutura do token bucket
 *
 * \return true se for possivel consumir
 * \return false se nao for possivel consumir
 */

bool bucket_check(t_bucket *bucket)
{
  refill_tokens(bucket);

  if (bucket->tokens < bucket->to_be_consumed_tokens)
   return false;

  return true;
}

/*!
 * \brief Funcao para calcular o tempo de espera de acordo com o numero de
 * tokens para encher o bucket para ter o minimo para o envio
 *
 * \param[in] bucket Estrutura do token bucket
 *
 * \return t_wait tempo a ser esperado
 */

struct timespec bucket_wait(t_bucket *bucket)
{
  long long tokens_needed;
  struct timespec t_wait;
  memset(&t_wait, 0, sizeof(t_wait));
  refill_tokens(bucket);

  if (bucket->tokens >= bucket->to_be_consumed_tokens)
    return t_wait;

  tokens_needed = bucket->to_be_consumed_tokens - bucket->tokens;
  t_wait.tv_nsec = (long)(100000000 * tokens_needed / (bucket->rate));
  t_wait.tv_sec = (long)(tokens_needed / bucket->rate);
  return t_wait;
}

/*!
 * \brief Funcao que compara 2 timespecs
 *
 * \param[in] t1 Timespec a ser comparado
 * \param[in] t2 Timespec a ser comparado
 *
 * \return 1 caso t1 seja maior que t2
 * \return -1 caso t1 seja menor que t2
 * \return 0 caso sejam iguais
 */

int timespec_compare(struct timespec *t1, struct timespec *t2)
{
  if (t1->tv_sec < t2->tv_sec)
    return -1;
  else if (t1->tv_sec > t2->tv_sec)
    return 1;
  else if (t1->tv_nsec < t2->tv_nsec)
    return -1;
  else if (t1->tv_nsec > t2->tv_nsec)
    return 1;
  else
    return 0;
}

/*!
 * \brief Funcao para checar se um timespec tem algum valor
 *
 * \param[in] t1 Timespec a ser checado
 *
 * \return 1 caso t1 esteja setado
 * \return 0 caso nao esteja
 */

int timespec_isset(struct timespec *t1)
{
  if (t1->tv_sec != 0 || t1->tv_nsec != 0)
    return 1;
  else
    return 0;
}


