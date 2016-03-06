#include "usertraps.h"
#include "misc.h"

#include "reactions.h"

#define H2_SIZE 3
#define O2_SIZE 2
#define SO2_SIZE 3
#define H2SO4_SIZE 5


void main (int argc, char *argv[])
{
  char h2_buffer[H2_SIZE];
  char o2_buffer[O2_SIZE];
  char so2_buffer[SO2_SIZE];

  char h2so4_molecule[H2SO4_SIZE] = {'H', '2', 'S', 'O', '4'};

  mbox_t h_mbox_h2;
  mbox_t h_mbox_o2;
  mbox_t h_mbox_so2;
  mbox_t h_mbox_h2so4;

  sem_t s_procs_completed; // Semaphore to signal the original process that we're done

  /* int ct; */
  /* Printf("argc = %d\n", argc); */
  /* for(ct = 0 ; ct < argc; ct++){ */
  /*   Printf("argv[%d] = %s\n", ct, argv[ct]); */
  /* } */

  if (argc != 6) { 
    Printf("Usage: %s <mailbox h2> <mailbox o2> <mailbox so2> <mailbox h2so4> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  h_mbox_h2 = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  h_mbox_o2 = dstrtol(argv[2], NULL, 10); // The "10" means base 10
  h_mbox_so2 = dstrtol(argv[3], NULL, 10); // The "10" means base 10
  h_mbox_h2so4 = dstrtol(argv[4], NULL, 10); // The "10" means base 10


  s_procs_completed = dstrtol(argv[5], NULL, 10);

  // Open the h2 mailbox
  if (mbox_open(h_mbox_h2) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not open the so4 mailbox!\n", getpid());
    Exit();
  }

  // Wait for the h2 molecule from the mailbox
  if (mbox_recv(h_mbox_h2, H2_SIZE, (void *) h2_buffer) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not receive message from the mbox %d!\n", getpid(), h_mbox_h2);
    Exit();
  }

  // Close the h2 mailbox
  if (mbox_close(h_mbox_h2) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }


  // Open the o2 mailbox
  if (mbox_open(h_mbox_o2) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not open the o2 mailbox!\n", getpid());
    Exit();
  }
  

  // Send the o2 to the o2 mailbox
  if (mbox_recv(h_mbox_o2, O2_SIZE, (void *) o2_buffer) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not send message to the mbox %d!\n", getpid(), h_mbox_o2);
    Exit();
  }

  // Close the o2 mailbox
  if (mbox_close(h_mbox_o2) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }


  // Open the SO2 mailbox
  if (mbox_open(h_mbox_so2) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not open the so2 mailbox!\n", getpid());
    Exit();
  }

  // Send the SO2 to the SO2 mailbox
  if (mbox_recv(h_mbox_so2, SO2_SIZE, (void *) so2_buffer) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not send message to the mbox %d!\n", getpid(), h_mbox_so2);
    Exit();
  }

  // Close the SO2 mailbox
  if (mbox_close(h_mbox_so2) == MBOX_FAIL) {
    Printf("Reaction 2 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }

    // Open the h2so4 mailbox
  if (mbox_open(h_mbox_h2so4) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not open the h2so4 mailbox!\n", getpid());
    Exit();
  }
  

  // Send the h2so4 to the o2 mailbox
  if (mbox_send(h_mbox_h2so4, H2SO4_SIZE, (void *) h2so4_molecule) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not send message to the mbox %d!\n", getpid(), h_mbox_h2so4);
    Exit();
  }

  // Close the h2so4 mailbox
  if (mbox_close(h_mbox_h2so4) == MBOX_FAIL) {
    Printf("Reaction 3 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }



  // Now print a message to show that everything worked
  Printf("A molecule H2SO4 has been generated\n");

  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Reaction 3 (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("Reaction 3 (%d): Done!\n", getpid());
}
