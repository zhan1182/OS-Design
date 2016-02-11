
#include "lab2-api.h"

#ifndef __USERPROG__
#define __USERPROG__

typedef struct circular_buffer {
  int head;
  int tail;
  char space[BUFFERSIZE];
  lock_t buffer_lock;
} Circular_Buffer;

#define PRODUCER_TO_RUN "producer.dlx.obj"
#define CONSUMER_TO_RUN "consumer.dlx.obj"
#define STRING_LENGTH 11

#endif
