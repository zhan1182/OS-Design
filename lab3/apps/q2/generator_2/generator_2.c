#include "usertraps.h"
#include "misc.h"

#include "reactions.h"

#define SO4_SIZE 3

void main (int argc, char *argv[])
{
  char so4[SO4_SIZE] = {'S', 'O', '4'};

  mbox_t h_mbox;           // Handle to the mailbox
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done

  if (argc != 3) { 
    Printf("Usage: %s <handle_to_mailbox> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  h_mbox = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  s_procs_completed = dstrtol(argv[2], NULL, 10);

  // Open the mailbox
  if (mbox_open(h_mbox) == MBOX_FAIL) {
    Printf("Generator SO4 (%d): Could not open the mailbox!\n", getpid());
    Exit();
  }

  // Send a message to the mailbox
  if (mbox_send(h_mbox, SO4_SIZE, (void *) so4) == MBOX_FAIL) {
    Printf("Generator SO4 (%d): Could not send message to the mailbox %d!\n", getpid(), h_mbox);
    Exit();
  }
 
  // Now print a message to show that everything worked
  Printf("Generate SO4 Molecule\n");

  // Close the mbox
  if (mbox_close(h_mbox) == MBOX_FAIL) {
    Printf("Generator SO4 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }


  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Generator SO4 (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("Generator SO4 (%d): Done!\n", getpid());

}
