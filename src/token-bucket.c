#include "server.h"

static long time_now()
{
  struct timeval time;
  gettimeofday(&time, NULL);

  return (long)(time.tv_sec * 1000 + time.tv_usec/1000);
}

static void refill_tokens(bucket *tbc)
{
  long now = time_now();
  size_t delta = (size_t)(tbc->rate * (now - tbc->timestamp));
  
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

int token_buffer_init(bucket *tbc, double tokens, double max_burst, double rate)
{
  tbc->capacity = max_burst;
  tbc->tokens = tokens;
  tbc->rate = rate;
  tbc->timestamp = time_now();  

  return 0;
}

void token_buffer_consume(bucket *tbc)
{
  refill_tokens(tbc);
    
 // if (tbc->tokens < tbc->tokens_aux)
 //  return false;

  tbc->tokens -= tbc->tokens_aux; 
 // return true;
}

bool check_for_consume(bucket *tbc)
{
  refill_tokens(tbc);
 
  if (tbc->tokens < tbc->tokens_aux)
   return false;

  //tbc->tokens -= tokens_aux; 
  return true;
}


long wait_time(bucket *tbc)
{
  double tokens_needed;
  long t_wait;
  refill_tokens(tbc);
  
  if (tbc->tokens >= tbc->tokens_aux)
    return 0;
    
  tokens_needed = tbc->tokens_aux - tbc->tokens;
  t_wait = (1000 * tokens_needed) / tbc->rate;
  return t_wait;

}

long find_poll_wait_time(client_list *cli_list)
{
  int i;
  long wait = 0;
  client_info *cli_info; 

  i = SERVER_INDEX + 1;
 
  cli_info = cli_list->head;  
    
  while (cli_info)
  {
    if (cli_info->can_send == false && cli_list->client[i].fd > 0)
    {
      cli_list->client[i].fd = cli_list->client[i].fd * -1;
      if (wait == 0) 
        wait = wait_time(&cli_info->tbc) * 1000;
      if (wait_time(&cli_info->tbc) * 1000 < wait)
        wait = wait_time(&cli_info->tbc) * 1000;        
      break;    
    }

    if (check_for_consume(&cli_info->tbc) == true &&
                          cli_list->client[i].fd < 0)  
    {
      cli_list->client[i].fd = cli_list->client[i].fd * -1;
      break;
    }
                
    cli_info = cli_info->next;
    i++;
  }

  return wait;
}

