#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "reactions.h"

void main(int argc, char ** argv)
{
  uint32 h_mem;
  sem_t s_procs_completed;

  Molecules * mols;
  int ct;

  if (argc != 3) { 
    Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  }

  // Convert the command-line strings into integers for use as handles
  h_mem = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  s_procs_completed = dstrtol(argv[2], NULL, 10);

  // Map shared memory page into this process's memory space
  if ((mols = (Molecules *) shmat(h_mem)) == NULL) {
    Printf("Could not map the shared page to virtual address in "); Printf(argv[0]); Printf(", exiting..\n");
    Exit();
  }

  for(ct = 0; ct < mols->init_so4; ct++){
    // Consume SO4
    sem_wait(mols->so4);
    
    // Produce so2
    sem_signal(mols->so2);
    Printf("A molecule SO2 is created\n");
    
    // Produce O2
    sem_signal(mols->o2);
    Printf("A molecule O2 is created\n");
  }
  

  // Signal the semaphore to tell the original process that we're done
  Printf("Reaction 2: PID %d is complete.\n", getpid());
  

  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }

  return;
}
