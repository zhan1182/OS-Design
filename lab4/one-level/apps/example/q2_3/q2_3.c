#include "usertraps.h"
#include "misc.h"

int f(int n)
{

  int k = 10;
  k++;

  /* Printf("K is allocated at %d\n", &k); */

  if (n == 1){
    return 1;
  }

  return f(n - 1);
}

void main (int argc, char *argv[])
{
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done

  int N = 10000;
  int f_N;

  if (argc != 2) { 
    Printf("Usage: %s <handle_to_procs_completed_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  s_procs_completed = dstrtol(argv[1], NULL, 10);

  
  f_N = f(N);
  
  Printf("f %d = %d\n", N, f_N);
  
  

  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("hello_world (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("q2.3 cause the call stack to grow larger than 1 page (%d): Done!\n", getpid());
}
