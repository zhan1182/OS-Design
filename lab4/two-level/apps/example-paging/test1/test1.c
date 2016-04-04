#include "usertraps.h"
#include "misc.h"

void main (int argc, char *argv[]) {
  sem_t sem;
  int large_data[1000 * 256]; // size = 1M
  int i = 0;
  sem = dstrtol(argv[1], NULL, 10);

  Printf("--------------------Running test 1.0 (accessing large_data) ---------------------\n");
  large_data[1000 * 256 - 1] = 0x11;
  Printf("large_data[1000 * 256 - 1] = %d\n", large_data[1000 * 256 - 1]);
  Printf("--------------------End test 1.1 (successful output: output = 17 ) ---------------------\n\n");

  Printf("--------------------Running test 1.2 (accessing large_data beyond size) ---------------------\n");
  sem_signal(sem); // dont let makeproc die
  large_data[1001 * 256] = 0x12;
  // your code should throw illegal access error here as we are index that exceeds array size.

  Printf("large_data[1000 * 256] = %d\n", large_data[1000 * 256]);
  Printf("--------------------End test 1.2 (Your code is incorrect if you see this line :-)) ---------------------\n");
}
