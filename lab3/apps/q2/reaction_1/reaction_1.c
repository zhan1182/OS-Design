#include "usertraps.h"
#include "misc.h"

#include "reactions.h"

#define H2O_SIZE 3
#define H2_SIZE 2
#define O2_SIZE 2

void main (int argc, char *argv[])
{
  int ct;

  char h2o_buffer_1[H2O_SIZE];
  char h2o_buffer_2[H2O_SIZE];

  char h2_molecule[H2_SIZE] = {'H', '2'};
  char o2_molecule[O2_SIZE] = {'O', '2'};
  
  mbox_t h_mbox_h2o;           // Handle to the mailbox for h2o
  mbox_t h_mbox_h2;
  mbox_t h_mbox_o2;

  sem_t s_procs_completed; // Semaphore to signal the original process that we're done

  if (argc != 5) { 
    Printf("Usage: %s <mailbox h2o> <mailbox h2> <mailbox o2> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  h_mbox_h2o = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  h_mbox_h2 = dstrtol(argv[2], NULL, 10); // The "10" means base 10
  h_mbox_o2 = dstrtol(argv[3], NULL, 10); // The "10" means base 10

  s_procs_completed = dstrtol(argv[4], NULL, 10);

  // Open the h2o mailbox
  if (mbox_open(h_mbox_h2o) == MBOX_FAIL) {
    Printf("Reaction 1 (%d): Could not open the h2o mailbox!\n", getpid());
    Exit();
  }

  // Wait for the first h2o molecule from the mailbox
  if (mbox_recv(h_mbox_h2o, H2O_SIZE, (void *) h2o_buffer_1) == MBOX_FAIL) {
    Printf("Reaction 1 (%d): Could not receive message from the mbox %d!\n", getpid(), h_mbox_h2o);
    Exit();
  }

  // Wait for the second h2o molecule from the mailbox  
  if (mbox_recv(h_mbox_h2o, H2O_SIZE, (void *) h2o_buffer_2) == MBOX_FAIL) {
    Printf("Reaction 1 (%d): Could not receive message from the mbox %d!\n", getpid(), h_mbox_h2o);
    Exit();
  }

  // Close the h2o mailbox
  if (mbox_close(h_mbox_h2o) == MBOX_FAIL) {
    Printf("Reaction 1 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }

  // Open the h2 mailbox
  if (mbox_open(h_mbox_h2) == MBOX_FAIL) {
    Printf("Reaction 1 (%d): Could not open the h2 mailbox!\n", getpid());
    Exit();
  }


  // Send the first h2 to the h2 mailbox
  if (mbox_send(h_mbox_h2, H2_SIZE, (void *) h2_molecule) == MBOX_FAIL) {
    Printf("Generator h2 (%d): Could not send message to the mbox %d!\n", getpid(), h_mbox_h2);
    Exit();
  }

  // Send the second h2 to the h2 mailbox
  if (mbox_send(h_mbox_h2, H2_SIZE, (void *) h2_molecule) == MBOX_FAIL) {
    Printf("Generator h2 (%d): Could not send message to the mbox %d!\n", getpid(), h_mbox_h2);
    Exit();
  }


  // Close the h2 mailbox
  if (mbox_close(h_mbox_h2) == MBOX_FAIL) {
    Printf("Generator h2 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }


  // Open the o2 mailbox
  if (mbox_open(h_mbox_o2) == MBOX_FAIL) {
    Printf("Reaction 1 (%d): Could not open the o2 mailbox!\n", getpid());
    Exit();
  }
  

  // Send the o2 to the o2 mailbox
  if (mbox_send(h_mbox_o2, O2_SIZE, (void *) o2_molecule) == MBOX_FAIL) {
    Printf("Reaction o2 (%d): Could not send message to the mbox %d!\n", getpid(), h_mbox_o2);
    Exit();
  }

  // Close the o2 mailbox
  if (mbox_close(h_mbox_o2) == MBOX_FAIL) {
    Printf("Reaction o2 (%d): Could not close the mailbox!\n", getpid());
    Exit();
  }

  // Now print a message to show that everything worked
  Printf("Two molecules H2 has been generated\n");
  Printf("A molecule O2 has been generated\n");

  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Reaction 1 (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("Reaction 1 (%d): Done!\n", getpid());
}
