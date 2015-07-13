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
    
    new_tokens = delta * tbc->rate;
    
    if (tbc->tokens + new_tokens < tbc->capacity)
      tbc->tokens = tbc->tokens + new_tokens;
    else
      tbc->tokens = tbc->capacity;
  } 
  
  tbc->timestamp = now;

}

int token_buffer_init(client_info *cli_info, double tokens, double max_burst, double rate)
{
  cli_info->tbc.capacity = max_burst;
  cli_info->tbc.tokens = tokens;
  cli_info->tbc.rate = rate;
  cli_info->tbc.timestamp = time_now();
  
  return 0;
}

bool token_buffer_consume(bucket *tbc, double tokens)
{
  refill_tokens(tbc);
 
  if (tbc->tokens < tokens)
   return false;

  tbc->tokens -= tokens; 
                 
  return true;
}

long wait_time(bucket *tbc, double tokens)
{
  double tokens_needed, t_wait;
  refill_tokens(tbc);
  
  if (tbc->tokens >= tokens)
    return 0;
    
  tokens_needed = tokens - tbc->tokens;
  t_wait = (1000 * tokens_needed) / tbc->rate;
  //t_wait += ((1000 * tokens_needed) % tbc->rate) ? 1 : 0;
  return t_wait;

}
