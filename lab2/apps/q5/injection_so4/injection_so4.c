#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "reactions.h"

void main(int argc, char ** argv)
{
  uint32 h_mem;
  sem_t s_procs_completed;

  if (argc != 3) { 
    Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  }

  // Convert the command-line strings into integers for use as handles
  h_mem = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  s_procs_completed = dstrtol(argv[2], NULL, 10);

  // Map shared memory page into this process's memory space


  // Signal the semaphore to tell the original process that we're done
  Printf("Consumer: PID %d is complete.\n", getpid());


  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }

  return;
}
