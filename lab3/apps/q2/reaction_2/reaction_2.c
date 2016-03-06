#include "usertraps.h"
#include "misc.h"

#include "reactions.h"

#define SO4_SIZE 3
#define SO2_SIZE 3
#define O2_SIZE 2

void main (int argc, char *argv[])
{
  char so4_buffer[SO4_SIZE];

  char so2_molecule[SO2_SIZE] = {'S', 'O', '2'};
  char o2_molecule[O2_SIZE] = {'O', '2'};
  
  mbox_t h_mbox_so4;           // Handle to the mailbox for so4
  mbox_t h_mbox_so2;
  mbox_t h_mbox_o2;

  sem_t s_procs_completed; // Semaphore to signal the original process that we're done

  if (argc != 5) { 
    Printf("Usage: %s <mailbox so4> <mailbox so2> <mailbox o2> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  h_mbox_so4 = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  h_mbox_so2 = dstrtol(argv[2], NULL, 10); // The "10" means base 10
  h_mbox_o2 = dstrtol(argv[3], NULL, 10); // The "10" means base 10

  s_procs_completed = dstrtol(argv[4], NULL, 10);

  // Open the so4 mailbox
  if (mbox_open(h_mbox_so4) == MBOX_FAIL) {
    Printf("Reaction 2 (%d): Could not open the so4 mailbox!\n", getpid());
    Exit();
  }

  // Wait for the SO4 molecule from the mailbox
  if (mbox_recv(h_mbox_so4, SO4_SIZE, (void *) so4_buffer) == MBOX_FAIL) {
    Printf("Reaction 2 (%d): Could not receive message from the mbox %d!\n", getpid(), h_mbox_so4);
    Exit();
  }

  // Close the SO4 mailbox
  if (mbox_close(h_mbox_so4) == MBOX_FAIL) {
    Printf("Reaction 2 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }

  // Open the SO2 mailbox
  if (mbox_open(h_mbox_so2) == MBOX_FAIL) {
    Printf("Reaction 2 (%d): Could not open the so2 mailbox!\n", getpid());
    Exit();
  }


  // Send the SO2 to the SO2 mailbox
  if (mbox_send(h_mbox_so2, SO2_SIZE, (void *) so2_molecule) == MBOX_FAIL) {
    Printf("Reaction 2 (%d): Could not send message to the mbox %d!\n", getpid(), h_mbox_so2);
    Exit();
  }

  // Close the SO2 mailbox
  if (mbox_close(h_mbox_so2) == MBOX_FAIL) {
    Printf("Reaction 2 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }


  // Open the o2 mailbox
  if (mbox_open(h_mbox_o2) == MBOX_FAIL) {
    Printf("Reaction 2 (%d): Could not open the o2 mailbox!\n", getpid());
    Exit();
  }
  

  // Send the o2 to the o2 mailbox
  if (mbox_send(h_mbox_o2, O2_SIZE, (void *) o2_molecule) == MBOX_FAIL) {
    Printf("Reaction 2 (%d): Could not send message to the mbox %d!\n", getpid(), h_mbox_o2);
    Exit();
  }

  // Close the o2 mailbox
  if (mbox_close(h_mbox_o2) == MBOX_FAIL) {
    Printf("Reaction 2 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }

  // Now print a message to show that everything worked
  Printf("A molecule SO2 has been generated\n");
  Printf("A molecule O2 has been generated\n");

  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Reaction 2 (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("Reaction 2 (%d): Done!\n", getpid());
}
