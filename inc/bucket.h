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
  long long capacity;
  long long tokens;
  long long to_be_consumed_tokens;
  long long rate;
  long long timestamp;

} t_bucket;

void bucket_init(t_bucket *bucket, long long tokens,  long long capacity,
                  long long rate);
bool bucket_consume(t_bucket *bucket);
bool bucket_check(t_bucket *bucket);
struct timespec bucket_wait(t_bucket *bucket);
int timespec_compare(struct timespec *t1, struct timespec *t2);
int timespec_isset(struct timespec *t1);
long long time_now();

#endif
