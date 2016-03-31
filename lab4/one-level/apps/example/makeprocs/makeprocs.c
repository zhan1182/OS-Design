#include "usertraps.h"
#include "misc.h"

#define HELLO_WORLD "hello_world.dlx.obj"
#define Q2_2 "q2_2.dlx.obj"
#define Q2_3 "q2_3.dlx.obj"

void main (int argc, char *argv[])
{
  int num_hello_world = 0;             // Used to store number of processes to create
  int i;                               // Loop index variable
  sem_t s_procs_completed;             // Semaphore used to wait until all spawned processes have completed
  char s_procs_completed_str[10];      // Used as command-line argument to pass page_mapped handle to new processes
  
  sem_t s2; 
  char s2_str[10];

  sem_t s3; 
  char s3_str[10];

  sem_t s4; 
  char s4_str[10];



  if (argc != 2) {
    Printf("Usage: %s <number of hello world processes to create>\n", argv[0]);
    Exit();
  }

  // Convert string from ascii command line argument to integer number
  num_hello_world = dstrtol(argv[1], NULL, 10); // the "10" means base 10
  Printf("makeprocs (%d): Creating %d hello_world processes\n", getpid(), num_hello_world);

  // Create semaphore to not exit this process until all other processes 
  // have signalled that they are complete.
  if ((s_procs_completed = sem_create(0)) == SYNC_FAIL) {
    Printf("makeprocs (%d): Bad sem_create\n", getpid());
    Exit();
  }

  // Setup the command-line arguments for the new processes.  We're going to
  // pass the handles to the semaphore as strings
  // on the command line, so we must first convert them from ints to strings.
  ditoa(s_procs_completed, s_procs_completed_str);

  // Create Hello World processes
  Printf("-------------------------------------------------------------------------------------\n");
  Printf("makeprocs (%d): Creating %d hello world's in a row, but only one runs at a time\n", getpid(), num_hello_world);
  for(i=0; i<num_hello_world; i++) {
    Printf("makeprocs (%d): Creating hello world #%d\n", getpid(), i);

    // Create Hello World processes
    process_create(HELLO_WORLD, s_procs_completed_str, NULL);

    if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
      Printf("Bad semaphore s_procs_completed (%d) in %s\n", s_procs_completed, argv[0]);
      Exit();
    }
  }


  // Create q2.2 access memory outside of page
  if ((s2 = sem_create(0)) == SYNC_FAIL) {
    Printf("makeprocs (%d): Bad sem_create\n", getpid());
    Exit();
  }

  ditoa(s2, s2_str);

  process_create(Q2_2, s2_str, NULL);
  

  if (sem_wait(s2) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in %s\n", s2, argv[0]);
    Exit();
  }


  // Create q2.3 Cause the user function call stack to grow larger than 1 page
  if ((s3 = sem_create(0)) == SYNC_FAIL) {
    Printf("makeprocs (%d): Bad sem_create\n", getpid());
    Exit();
  }

  ditoa(s3, s3_str);

  process_create(Q2_3, s3_str, NULL);
  

  if (sem_wait(s3) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in %s\n", s3, argv[0]);
    Exit();
  }

  
  /* // Call Hello World 100 times!! */
  /* if ((s4 = sem_create(0)) == SYNC_FAIL) { */
  /*   Printf("makeprocs (%d): Bad sem_create\n", getpid()); */
  /*   Exit(); */
  /* } */

  /* ditoa(s4, s4_str); */

  /* for(i = 0; i < 100; i++) { */
  /*   Printf("makeprocs (%d): Creating hello world #%d\n", getpid(), i); */

  /*   // Create Hello World processes */
  /*   process_create(HELLO_WORLD, s4_str, NULL); */

  /*   if (sem_wait(s4) != SYNC_SUCCESS) { */
  /*     Printf("Bad semaphore s_procs_completed (%d) in %s\n", s4, argv[0]); */
  /*     Exit(); */
  /*   } */
  /* } */





  Printf("-------------------------------------------------------------------------------------\n");
  Printf("makeprocs (%d): All other processes completed, exiting main process.\n", getpid());

}
