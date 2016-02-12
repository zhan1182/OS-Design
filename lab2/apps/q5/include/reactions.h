
#include "lab2-api.h"

#ifndef __USERPROG__
#define __USERPROG__


typedef struct molecules
{
  int init_h2o;
  int init_so4;
  sem_t h2o;
  sem_t h2;
  sem_t o2;
  sem_t so4;
  sem_t so2;
  sem_t h2so4;
} Molecules;


#define NUMBER_OF_PROCESS 5

#define REACTION_1_TO_RUN "reaction_1.dlx.obj"
#define REACTION_2_TO_RUN "reaction_2.dlx.obj"
#define REACTION_3_TO_RUN "reaction_3.dlx.obj"
#define INJECTION_H2O_TO_RUN "injection_h2o.dlx.obj"
#define INJECTION_SO4_TO_RUN "injection_so4.dlx.obj"

#endif
