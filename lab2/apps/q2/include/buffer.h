
#include "lab2-api.h"

#ifndef __USERPROG__
#define __USERPROG__

typedef struct circular_buffer {
  int head;
  int tail;
  char space[BUFFERSIZE];
} Circular_Buffer;

#define PRODUCER_TO_RUN "producer.dlx.obj"
#define CONSUMER_TO_RUN "consumer.dlx.obj"


#endif
