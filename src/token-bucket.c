#include "server.h"

/*!
 * \brief Funcao para pegar o tempo atual (timestamp)
 * \return tempo em milisegundos
 */

static long time_now()
{
  struct timeval time;
  gettimeofday(&time, NULL);

  return (long)(time.tv_sec * 1000 + time.tv_usec/1000);
}

/*!
 * \brief Funcao para recarregar o bucket de acordo com o tempo passado
 * param[in] tbc Estrutura do token bucket dentro da estrutura do cliente
 * return tempo em milisegundos
 */

static void refill_tokens(bucket *tbc)
{
  long now = time_now();
  long delta = (now - tbc->timestamp);
  
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

void token_buffer_init(bucket *tbc, double tokens, double capacity, double rate)
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

bool token_buffer_consume(bucket *tbc, double tokens)
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

bool check_for_consume(bucket *tbc)
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

static long wait_time(bucket *tbc)
{
  double tokens_needed;
  long t_wait;

  
  refill_tokens(tbc);
  
  if (tbc->tokens >= tbc->tokens_aux)
    return 0;
    
  tokens_needed = tbc->tokens_aux - tbc->tokens;
  t_wait = 1000 * tokens_needed / tbc->rate;
  return t_wait;

}

/*!
 * \brief Funcao para calcular o tempo de espera do poll, o wait e' o menor 
 * tempo da lista de clientes
 *
 * \param[in] cli_list Lista de clientes
 * 
 * \return wait tempo a ser esperado pelo poll
 */

long find_poll_wait_time(client_list *cli_list)
{
  int cli_num;
  long wait = 0;
  client_info *cli_info; 
  cli_num = SERVER_INDEX + 1;
  cli_info = cli_list->head;  
    
  while (cli_info)
  {
    if (cli_info->can_send == false)
    {
      if (cli_list->client[cli_num].fd > 0)
        cli_list->client[cli_num].fd = cli_list->client[cli_num].fd * -1;
      
      if (wait == 0) 
        wait = wait_time(&cli_info->tbc);
      else if (wait_time(&cli_info->tbc) < wait)
        wait = wait_time(&cli_info->tbc);    
              
    }

    if ((cli_info->can_send = check_for_consume(&cli_info->tbc)) == true &&
                                                cli_list->client[cli_num].fd < 0)  

      cli_list->client[cli_num].fd = cli_list->client[cli_num].fd * -1;     
    
                
    cli_info = cli_info->next;
    cli_num++;
  }

  return wait;
}

