#include "usertraps.h"
#include "misc.h"

#include "reactions.h"

void main (int argc, char *argv[])
{

  int init_h2o;
  int init_so4;
  int num_of_reaction_3;
  int totalProcs = 0;

  int i;                          // Loop index variable

  mbox_t h_mbox_h2o;                  // Used to hold handle to mailbox
  mbox_t h_mbox_h2;                  // Used to hold handle to mailbox
  mbox_t h_mbox_o2;                  // Used to hold handle to mailbox
  mbox_t h_mbox_so2;                  // Used to hold handle to mailbox
  mbox_t h_mbox_so4;                  // Used to hold handle to mailbox
  mbox_t h_mbox_h2so4;                  // Used to hold handle to mailbox

  sem_t s_procs_completed;        // Semaphore used to wait until all spawned processes have completed

  char h_mbox_h2o_str[10];            // Used as command-line argument to pass mem_handle to new processes
  char h_mbox_h2_str[10];
  char h_mbox_o2_str[10];
  char h_mbox_so2_str[10];
  char h_mbox_so4_str[10];
  char h_mbox_h2so4_str[10];

  char s_procs_completed_str[10]; // Used as command-line argument to pass page_mapped handle to new processes

  if (argc != 3) {
    Printf("Usage: %s <number of h2o> <number of so4>\n", argv[0]);
    Exit();
  }

  // Convert string from ascii command line argument to integer number
  init_h2o = dstrtol(argv[1], NULL, 10); // the "10" means base 10
  init_so4 = dstrtol(argv[2], NULL, 10); // the "10" means base 10

  Printf("makeprocs (%d): Init H2O %d, init SO4 %d\n", getpid(), init_h2o, init_so4);

  // Allocate space for a mailbox
  if ((h_mbox_h2o = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox h2o!", getpid());
    Exit();
  }

  if ((h_mbox_h2 = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox h2!", getpid());
    Exit();
  }

  if ((h_mbox_o2 = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox o2!", getpid());
    Exit();
  }

  if ((h_mbox_so2 = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox so2!", getpid());
    Exit();
  }

  if ((h_mbox_so4 = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox so4!", getpid());
    Exit();
  }

  if ((h_mbox_h2so4 = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox h2so4!", getpid());
    Exit();
  }


  // Open mailbox to prevent deallocation
  if (mbox_open(h_mbox_h2o) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), h_mbox_h2o);
    Exit();
  }

  if (mbox_open(h_mbox_h2) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), h_mbox_h2);
    Exit();
  }

  if (mbox_open(h_mbox_o2) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), h_mbox_o2);
    Exit();
  }

  if (mbox_open(h_mbox_so2) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), h_mbox_so2);
    Exit();
  }

  if (mbox_open(h_mbox_so4) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), h_mbox_so4);
    Exit();
  }

  if (mbox_open(h_mbox_h2so4) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), h_mbox_h2so4);
    Exit();
  }


  if(init_so4 <= init_h2o / 2 * 2){
    num_of_reaction_3 = init_so4;
  }
  else{
    num_of_reaction_3 = init_h2o / 2 * 2;
  }

  totalProcs = init_h2o + init_so4 + init_h2o / 2 + init_so4 + num_of_reaction_3;


  // Create semaphore to not exit this process until all other processes 
  // have signalled that they are complete.  To do this, we will initialize
  // the semaphore to (-1) * (number of signals), where "number of signals"
  // should be equal to the number of processes we're spawning - 1.  Once 
  // each of the processes has signaled, the semaphore should be back to
  // zero and the final sem_wait below will return.
  if ((s_procs_completed = sem_create(-(totalProcs - 1))) == SYNC_FAIL) {
    Printf("makeprocs (%d): Bad sem_create\n", getpid());
    Exit();
  }

  // Setup the command-line arguments for the new process.  We're going to
  // pass the handles to the shared memory page and the semaphore as strings
  // on the command line, so we must first convert them from ints to strings.
  ditoa(h_mbox_h2o, h_mbox_h2o_str);
  ditoa(h_mbox_h2, h_mbox_h2_str);
  ditoa(h_mbox_o2, h_mbox_o2_str);
  ditoa(h_mbox_so2, h_mbox_so2_str);
  ditoa(h_mbox_so4, h_mbox_so4_str);
  ditoa(h_mbox_h2so4, h_mbox_h2so4_str);

  ditoa(s_procs_completed, s_procs_completed_str);

  // Now we can create the processes.  Note that you MUST end your call to
  // process_create with a NULL argument so that the operating system
  // knows how many arguments you are sending.
  for(i = 0; i < init_h2o; i++) {
    process_create(GENERATOR_H2O_TO_RUN, 0, 1, h_mbox_h2o_str, s_procs_completed_str, NULL);
    Printf("makeprocs (%d): Process %d created\n", getpid(), i);
  }

  for(i = 0; i < init_so4; i++) {
    process_create(GENERATOR_SO4_TO_RUN, 0, 1, h_mbox_so4_str, s_procs_completed_str, NULL);
    Printf("makeprocs (%d): Process %d created\n", getpid(), i);
  }

  for(i = 0; i < init_h2o / 2; i++){
    process_create(REACTION_1_TO_RUN, 0, 1, h_mbox_h2o_str, h_mbox_h2_str, h_mbox_o2_str, s_procs_completed_str, NULL);
    Printf("makeprocs (%d): Process %d created\n", getpid(), i);
  }

  for(i = 0; i < init_so4; i++){
    process_create(REACTION_2_TO_RUN, 0, 1, h_mbox_so4_str, h_mbox_so2_str, h_mbox_o2_str, s_procs_completed_str, NULL);
    Printf("makeprocs (%d): Process %d created\n", getpid(), i);
  }


  for(i = 0; i < num_of_reaction_3; i++){
    /* Printf("h_mbox_h2_str = %s, h_mbox_o2_str = %s, h_mbox_so2_str = %s, h_mbox_h2so4_str = %s, s_procs_completed_str = %s\n", h_mbox_h2_str, h_mbox_o2_str, h_mbox_so2_str, h_mbox_h2so4_str, s_procs_completed_str); */
    process_create(REACTION_3_TO_RUN, 0 , 1, h_mbox_h2_str, h_mbox_o2_str, h_mbox_so2_str, h_mbox_h2so4_str, s_procs_completed_str, NULL);
    Printf("makeprocs (%d): Process %d created\n", getpid(), i);
  }
  

  // And finally, wait until all spawned processes have finished.
  if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf("\n");
    Exit();
  }
  Printf("makeprocs (%d): All other processes completed, exiting main process.\n", getpid());
}
