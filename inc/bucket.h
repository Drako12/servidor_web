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
  double to_be_consumed_tokens;
  double rate;
  long timestamp;

} t_bucket;

void bucket_init(t_bucket *bucket, double tokens, double capacity,
                 double rate);
bool bucket_consume(t_bucket *bucket, double tokens);
bool bucket_check(t_bucket *bucket);
struct timespec bucket_wait(t_bucket *bucket);
int timespec_compare(struct timespec *t1, struct timespec *t2);
int timespec_isset(struct timespec *t1);

#endif
