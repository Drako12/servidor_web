#include "server.h"

static int time_now()
{
  struct timeval time;
  gettimeofday(&time, NULL);

  return (int)(time.tv_sec * 1000 + time.tv_usec/1000);
}

int token_buffer_init(bucket *tbc, size_t max_burst, double rate)
{
  tbc->capacity = max_burst;
  tbc->tokens = max_burst;
  tbc->rate = rate;
  tbc->timestamp = time_now();
  
  return 0;
}

size_t token_buffer_consume(bucket *tbc, size_t bytes)
{
  int now = time_now();
  size_t delta = (size_t)(tbc->rate * (now - tbc->timestamp));
  
  if (tbc->capacity < (tbc->tokens + delta))
    tbc->tokens = tbc->capacity;
  else
    tbc->tokens += delta;
    
  tbc->timestamp = now;
      
  fprintf(stdout, "TOKENS %d  bytes: %d\n", tbc->tokens, bytes);
        
  if (tbc->tokens > 0)
    tbc->tokens -= bytes;
  else
    return -1;
              
  return 0;
}
