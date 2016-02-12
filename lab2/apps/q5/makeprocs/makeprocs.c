#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "reactions.h"

void main (int argc, char *argv[])
{
  
  uint32 h_mem;                   // Used to hold handle to shared memory page
  sem_t s_procs_completed;        // Semaphore used to wait until all spawned processes have completed
  char h_mem_str[10];             // Used as command-line argument to pass mem_handle to new processes
  char s_procs_completed_str[10]; // Used as command-line argument to pass page_mapped handle to new processes

  Molecules * mols;

  if (argc != 3) {
    Printf("Usage: "); Printf(argv[0]); Printf(" <number of H2O molecules> <number of SO4 molecules>\n");
    Exit();
  }


  // Allocate space for a shared memory page, which is exactly 64KB
  // Note that it doesn't matter how much memory we actually need: we 
  // always get 64KB
  if ((h_mem = shmget()) == 0) {
    Printf("ERROR: could not allocate shared memory page in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }

  // Map shared memory page into this process's memory space
  if ((mols = (Molecules *) shmat(h_mem)) == NULL) {
    Printf("Could not map the shared page to virtual address in "); Printf(argv[0]); Printf(", exiting..\n");
    Exit();
  }

  // Convert string from ascii command line argument to integer number
  mols->init_h2o = dstrtol(argv[1], NULL, 10); // the "10" means base 10
  mols->init_so4 = dstrtol(argv[2], NULL, 10); // the "10" means base 10
  Printf("H2O: %d, SO4: %d\n", mols->init_h2o, mols->init_so4);

  // Init semaphores
  mols->h2o = sem_create(0);
  mols->h2 = sem_create(0);
  mols->o2 = sem_create(0);
  mols->so4 = sem_create(0);
  mols->so2 = sem_create(0);
  mols->h2so4 = sem_create(0);




  // Create semaphore to not exit this process until all other processes 
  // have signalled that they are complete.  To do this, we will initialize
  // the semaphore to (-1) * (number of signals), where "number of signals"
  // should be equal to the number of processes we're spawning - 1.  Once 
  // each of the processes has signaled, the semaphore should be back to
  // zero and the final sem_wait below will return.
  if ((s_procs_completed = sem_create(-(NUMBER_OF_PROCESS - 1))) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  // Setup the command-line arguments for the new process.  We're going to
  // pass the handles to the shared memory page and the semaphore as strings
  // on the command line, so we must first convert them from ints to strings.
  ditoa(h_mem, h_mem_str);
  ditoa(s_procs_completed, s_procs_completed_str);


  // Now we can create the processes.  Note that you MUST end your call to
  // process_create with a NULL argument so that the operating system
  // knows how many arguments you are sending.
  
  // Injection
  process_create(INJECTION_H2O_TO_RUN, h_mem_str, s_procs_completed_str, NULL);
  process_create(INJECTION_SO4_TO_RUN, h_mem_str, s_procs_completed_str, NULL);

  // Reaction
  process_create(REACTION_1_TO_RUN, h_mem_str, s_procs_completed_str, NULL);
  process_create(REACTION_2_TO_RUN, h_mem_str, s_procs_completed_str, NULL);
  process_create(REACTION_3_TO_RUN, h_mem_str, s_procs_completed_str, NULL);


  /* process_create(CONSUMER_TO_RUN, h_mem_str, s_procs_completed_str, NULL); */

  // And finally, wait until all spawned processes have finished.
  if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf("\n");
    Exit();
  }
  Printf("All other processes completed, exiting main process.\n");
}
