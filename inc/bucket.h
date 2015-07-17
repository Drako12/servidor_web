#ifndef BUCKET_H_INCLUDED
#define BUCKET_H_INCLUDED

#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define BUFSIZE BUFSIZ

typedef struct token_bucket
{
  double capacity;
  double tokens;
  double tokens_aux;
  long rate;
  long timestamp;

} bucket;
i

void bucket_init(bucket *tbc, double tokens, double capacity, long rate);
bool bucket_consume(bucket *tbc, double tokens);
bool bucket_check(bucket *tbc);
struct timespec bucket_wait(bucket *tbc);

#endif
