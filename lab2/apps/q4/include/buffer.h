
#include "lab2-api.h"

#ifndef __USERPROG__
#define __USERPROG__

typedef struct circular_buffer {
  int head;
  int tail;
  int nitem;
  char space[BUFFERSIZE];
  lock_t buffer_lock;
  cond_t buffer_cond;
} Circular_Buffer;

#define PRODUCER_TO_RUN "producer.dlx.obj"
#define CONSUMER_TO_RUN "consumer.dlx.obj"
#define STRING_LENGTH 11

#endif
