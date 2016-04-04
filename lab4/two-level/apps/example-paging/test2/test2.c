#include "usertraps.h"
#include "misc.h"

void main (int argc, char *argv[]) {
  sem_t sem;
  sem = dstrtol(argv[1], NULL, 10);

  Printf("--------------------Running test 2.0 (accessing illegal address) ---------------------\n");
  Printf("Test (%d): accessing 0xFFFF0000\n", getpid());
  printf("%d\n", *(int*)0xFFFF0000);

  /* your code should throw illegal access error here as we are exceeding virtual memory size */
  /* When run with -D m flag (see the lab description), you should see the following output. */
  /* Translating 0xffff0000 */
  /* Out of range (L1 = 12b, L2 = 12b size=1024 entry=1048560) */
  /* The code that throws above error is in simulation_source/dlxsim.cc */

  sem_signal(sem);
  Printf("--------------------End test 2.0 (Your code is incorrect if you see this line printed. :-)) ---------------------\n");

}
