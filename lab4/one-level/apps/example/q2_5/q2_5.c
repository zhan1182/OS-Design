#include "usertraps.h"
#include "misc.h"


void main (int argc, char *argv[])
{
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done

  int ct;
  int N = 1000000;

  if (argc != 2) { 
    Printf("Usage: %s <handle_to_procs_completed_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  s_procs_completed = dstrtol(argv[1], NULL, 10);

  Printf("q2.5 Process start counting number (%d):\n", getpid());
  
  for(ct = 0; ct < N; ct++){
  }
  

  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("hello_world (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("q2.5 Process counting number (%d): Done!\n", getpid());
}
