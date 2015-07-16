#ifndef BUCKET_H_INCLUDED
#define BUCKET_H_INCLUDED

#define _XOPEN_SOURCE 600
#define _GNU_SOURCE
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define BUFSIZE BUFSIZ

typedef struct token_bucket
{
  int capacity;
  int tokens;
  int tokens_aux;
  int rate;
  long timestamp;
} bucket;


void bucket_init(bucket *tbc, int tokens, int capacity, int rate);
bool bucket_consume(bucket *tbc, int tokens);
bool bucket_check(bucket *tbc);
struct timespec bucket_wait(bucket *tbc);

#endif
