#include "server.h"

static int time_now()
{
  struct timeval time;
  gettimeofday(&time, NULL);

  return (int)(time.tv_sec * 1000 + time.tv_usec/1000);
}

int token_buffer_init(bucket *tbc, double tokens, double max_burst, double rate)
{
  tbc->capacity = max_burst;
  tbc->tokens = tokens;
  tbc->rate = rate;
  tbc->timestamp = time_now();
  
  return 0;
}

double token_buffer_consume(bucket *tbc, double tokens)
{
  int now = time_now();
  size_t delta = (size_t)(tbc->rate * (now - tbc->timestamp));
  
  if (tbc->tokens < tbc->capacity)
  {
    int new_tokens;
    new_tokens = delta * tbc->rate;
    if (tbc->tokens + new_tokens < tbc->capacity)
      tbc->tokens = tbc->tokens + new_tokens;
    else
      tbc->tokens = tbc->capacity;
  } 
  
  tbc->timestamp = now;
  
  if (tbc->tokens < tokens)
   return -1;

  tbc->tokens -= tokens; 
  
               
  return 0;
}
