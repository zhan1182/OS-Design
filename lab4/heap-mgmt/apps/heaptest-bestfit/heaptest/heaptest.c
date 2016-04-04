#include "usertraps.h"
#include "misc.h"

#define BLOCKSIZE 8 

void main (int argc, char *argv[]) {
  int *block1, *block2, *block3, *block4;
  int n = dstrtol(argv[1], NULL, 10);
  int ret_mfree = 0;

  // malloc 4 blocks of different sizes.
  block1 = (int*)malloc(5 * BLOCKSIZE);
  block2 = (int*)malloc(4 * BLOCKSIZE);
  block3 = (int*)malloc(5 * BLOCKSIZE);
  block4 = (int*)malloc(2 * BLOCKSIZE);

  if ( (block1 == NULL) ||
       (block2 == NULL) ||
       (block3 == NULL) ||
       (block4 == NULL)){
    Printf("test malloc: failed\n");
    return;
  }

  Printf("test: malloc: block1: %d\n", (int) block1);
  Printf("test: malloc: block2: %d\n", (int) block2);
  Printf("test: malloc: block3: %d\n", (int) block3);
  Printf("test: malloc: block4: %d\n", (int) block4);
  
  // free 2 of the blocks
  ret_mfree = mfree((void*)block2);
  ret_mfree += mfree((void*)block4);
  
  Printf("ret_mfree is %d.\n", ret_mfree);
  
  if (ret_mfree < (6 * BLOCKSIZE)){
    Printf("test mfree: failed\n");
    return;
  }

  /* ret_mfree = mfree((void *)block1); */
  /* ret_mfree += mfree((void*)block3); */

  // again malloc single block to test best-fit algo.
  block2 = (int*)malloc(2 * BLOCKSIZE);
    

  Printf("test: malloc: block2 after mfree: %d\n", (int) block2);

  /* expected output */
  /* test: malloc: block1: 16384 */
  /* test: malloc: block2: 16424 */
  /* test: malloc: block3: 16456 */
  /* test: malloc: block4: 16480 */
  /* test: malloc: block2 after mfree: 16480 */

  // Remember, we have given 5th page to the heap. 5th page starts at
  // 4 * 4096 = 16384. This must be the starting address of the first block.

  // Because of the best-fit algorithm, the starting address of block2
  // (after mfree = 16480) differs from what it was before mfree (16424)
}
