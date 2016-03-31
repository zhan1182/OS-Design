#include "usertraps.h"
#include "misc.h"

#define ARRAY_SIZE 1001

void main (int argc, char *argv[])
{
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done
  int arr[ARRAY_SIZE];
  int ct;

  if (argc != 2) { 
    Printf("Usage: %s <handle_to_procs_completed_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  s_procs_completed = dstrtol(argv[1], NULL, 10);

  
  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("hello_world (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  for(ct = 0; ct < ARRAY_SIZE; ct++){
    arr[ct] = ct;
  }


  Printf("q2.3 cause the call stack to grow larger than 1 page (%d): Done!\n", getpid());
}
