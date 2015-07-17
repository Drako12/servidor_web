#include "bucket.h"

/*!
 * \brief Funcao para pegar o tempo atual (timestamp)
 * \return tempo em milisegundos
 */

static long time_now()
{
  struct timespec time;
  //gettimeofday(&time, NULL);
  clock_gettime(CLOCK_REALTIME, &time);
  return (long)(time.tv_sec * 1000 + time.tv_nsec/1000000);
}

/*!
 * \brief Funcao para recarregar o bucket de acordo com o tempo passado
 * param[in] tbc Estrutura do token bucket dentro da estrutura do cliente
 * return tempo em milisegundos
 */

static void refill_tokens(bucket *tbc)
{
  long now, delta;
 
  now = time_now();
  delta = (now - tbc->timestamp);
  
  if (tbc->tokens < tbc->capacity)
  {
    double new_tokens;
    
    new_tokens = delta * tbc->rate * 0.001;
    
    if (tbc->tokens + new_tokens < tbc->capacity)
      tbc->tokens = tbc->tokens + new_tokens;
    else
      tbc->tokens = tbc->capacity;
  }  
  tbc->timestamp = now;
}

/*!
 * \brief Funcao para inicializar o bucket
 * \param[in] tokens Numero de tokens na inicializacao
 * \param[in] capacity Capacidade total do bucket
 * \param[in] rate Rate de transmissao
 * \param[out] tbc Estrutura do token bucket
 * 
 */

void bucket_init(bucket *tbc, double tokens, double capacity, long rate)
{
  tbc->capacity = capacity;
  tbc->tokens = tokens;
  tbc->rate = rate;
  tbc->timestamp = time_now();  
}

/*!
 * \brief Funcao para consumir os tokens do bucket
 * \param[in] tokens Numero de tokens para ser consumidos
 * \param[in] tbc Estrutura do token bucket
 * 
 * \return true se conseguir consumir 
 * \return false se nao tiver saldo suficiente para consumir
 */

bool bucket_consume(bucket *tbc, double tokens)
{
  refill_tokens(tbc);

  if (tbc->tokens < tokens)
    return false;

  tbc->tokens -= tokens; 
  return true;
}

/*!
 * \brief Funcao para checar se e' possivel ou nao consumir os tokens do bucket
 * \param[in] tbc Estrutura do token bucket
 * 
 * \return true se for possivel consumir
 * \return false se nao for possivel consumir
 */

bool bucket_check(bucket *tbc)
{
  refill_tokens(tbc);
   
  if (tbc->rate > BUFSIZE)
    tbc->tokens_aux = BUFSIZE - 1;
  else
    tbc->tokens_aux = tbc->rate;
    
  if (tbc->tokens < tbc->tokens_aux)
   return false;

  return true;
}

/*!
 * \brief Funcao para calcular o tempo de espera de acordo com o numero de
 * tokens
 *
 * \param[in] tbc Estrutura do token bucket
 * 
 * \return t_wait tempo a ser esperado
 */

struct timespec bucket_wait(bucket *tbc)
{
  double tokens_needed;
  struct timespec t_wait;
  memset(&t_wait, 0, sizeof(t_wait));
  refill_tokens(tbc);
  
  if (tbc->tokens >= tbc->tokens_aux)
    return t_wait;
    
  tokens_needed = tbc->tokens_aux - tbc->tokens;
  t_wait.tv_nsec = 1000 * tokens_needed / (tbc->rate/1000000);
  t_wait.tv_sec = (int)(tokens_needed / tbc->rate);
  return t_wait;
}


