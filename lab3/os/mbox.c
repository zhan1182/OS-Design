#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "synch.h"
#include "queue.h"
#include "mbox.h"

//-------------------------------------------------------
//
// void MboxModuleInit();
//
// Initialize all mailboxes.  This process does not need
// to worry about synchronization as it is called at boot
// time.  Only initialize necessary items here: you can
// initialize others in MboxCreate.  In other words, 
// don't waste system resources like locks and semaphores
// on unused mailboxes.
//
//-------------------------------------------------------

static mbox_message system_mbox_message;
static mbox system_mbox[MBOX_NUM_MBOXES];

void MboxModuleInit() {
  int ct;

  /* system_mbox_message.system_buffer_head = 0; */
  /* system_mbox_message.system_buffer_tail = 0; */

  system_mbox_message.system_buffer_slot_used = 0;
  system_mbox_message.system_buffer_lock = LockCreate();
  system_mbox_message.system_buffer_fill = CondCreate(system_mbox_message.system_buffer_lock);
  system_mbox_message.system_buffer_empty = CondCreate(system_mbox_message.system_buffer_lock);

  // Initially, all system buffer slots are available for using
  for(ct = 0; ct < MBOX_NUM_BUFFERS; ct++){
    system_mbox_message.system_buffer_index_array[ct] = 0;
  }

  // inuse = -1: not activated, inuse = 0: activated, ready to be opened, no process is using it now
  // inuse > 0: some processes are using it
  for(ct = 0; ct < MBOX_NUM_MBOXES; ct++){
    system_mbox[ct].num_of_pid_inuse = -1;
  }

}

//-------------------------------------------------------
//
// mbox_t MboxCreate();
//
// Allocate an available mailbox structure for use. 
//
// Returns the mailbox handle on success
// Returns MBOX_FAIL on error.
//
//-------------------------------------------------------
mbox_t MboxCreate() {
  
  int ct;
  mbox_t mbox_ct;
  uint32 intrval;
  
  // grabbing a mbox should be an atomic operation
  intrval = DisableIntrs();
  for(mbox_ct = 0; mbox_ct < MBOX_NUM_MBOXES; mbox_ct++){
    // inuse = 0: the mbox is reserved, but not opened for any process yet
    if(system_mbox[mbox_ct].num_of_pid_inuse == -1 ){
      system_mbox[mbox_ct].num_of_pid_inuse = 0;
      break;
    }
  }
  RestoreIntrs(intrval);

  if(mbox_ct == MBOX_NUM_MBOXES){
    return MBOX_FAIL;
  }

  system_mbox[mbox_ct].mbox_buffer_head = 0;
  system_mbox[mbox_ct].mbox_buffer_tail = 0;
  system_mbox[mbox_ct].mbox_buffer_slot_used = 0;
  system_mbox[mbox_ct].mbox_buffer_lock = LockCreate();
  system_mbox[mbox_ct].mbox_buffer_fill = CondCreate(system_mbox[mbox_ct].mbox_buffer_lock);
  system_mbox[mbox_ct].mbox_buffer_empty = CondCreate(system_mbox[mbox_ct].mbox_buffer_lock);

  for(ct = 0; ct < PROCESS_MAX_PROCS; ct++){
    // No proc opens the mbox when it is created
    system_mbox[mbox_ct].mbox_pid_list[ct] = 0;
  }

  return mbox_ct;
}

//-------------------------------------------------------
// 
// void MboxOpen(mbox_t);
//
// Open the mailbox for use by the current process.  Note
// that it is assumed that the internal lock/mutex handle 
// of the mailbox and the inuse flag will not be changed 
// during execution.  This allows us to get the a valid 
// lock handle without a need for synchronization.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxOpen(mbox_t handle) {

  unsigned int curr_pid = GetCurrentPid();

  // Check if handle is a valid number
  if(handle < 0 || handle >= MBOX_NUM_MBOXES){
    return MBOX_FAIL;
  }

  // Check if the mbox is reserved (activated)
  if(system_mbox[handle].num_of_pid_inuse < 0){
    return MBOX_FAIL;
  }

  // Acquire the lock
  if(LockHandleAcquire(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Acquire lock for the mbox %d!\n", handle);
    exitsim();
  }

  // Update the number of pid used
  if(system_mbox[handle].mbox_pid_list[curr_pid] == 1){
    printf("The mbox %d has already been opened\n", handle);
  }
  else if(system_mbox[handle].mbox_pid_list[curr_pid] == 0){
    system_mbox[handle].num_of_pid_inuse += 1;
    system_mbox[handle].mbox_pid_list[curr_pid] = 1;
  }
  else{
    printf("FATAL ERROR: Unkown Pid %d for mbox %d!\n", curr_pid, handle);
    // Release the lock
    if(LockHandleRelease(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
      printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
      exitsim();
    }
    return MBOX_FAIL;
  }
  
  // Release the lock
  if(LockHandleRelease(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
    exitsim();
  }


  return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxClose(mbox_t);
//
// Close the mailbox for use to the current process.
// If the number of processes using the given mailbox
// is zero, then disable the mailbox structure and
// return it to the set of available mboxes.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxClose(mbox_t handle) {

  unsigned int curr_pid = GetCurrentPid();
  int mbox_pid_status = system_mbox[handle].mbox_pid_list[curr_pid];

  // Check if handle is a valid number
  if(handle < 0 || handle >= MBOX_NUM_MBOXES){
    return MBOX_FAIL;
  }
  
  // Check if the mbox is reserved (activated)
  if(system_mbox[handle].num_of_pid_inuse < 0){
    return MBOX_FAIL;
  }

  // Acquire the lock
  if(LockHandleAcquire(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Acquire lock for the mbox %d!\n", handle);
    exitsim();
  }


  if(mbox_pid_status == 0){
    printf("The mbox %d has already been closed for process %d\n", handle, curr_pid);
  }
  else if(mbox_pid_status == 1){
    // Update the number of pid used
    system_mbox[handle].num_of_pid_inuse -= 1;
    system_mbox[handle].mbox_pid_list[curr_pid] = 0;
  }
  else{
    printf("FATAL ERROR: Unkown Pid %d for mbox %d\n", curr_pid, handle);
    // Release the lock
    if(LockHandleRelease(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
      printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
      exitsim();
    }
    return MBOX_FAIL;
  }
  
  // Return the mbox to the pool of available mboxes for the system
  // Reset this mbox, clear rest of messages in this mbox (clear the linkings in the system)
  if(system_mbox[handle].num_of_pid_inuse == 0){
    system_mbox[handle].num_of_pid_inuse = -1;
  }


  // Release the lock
  if(LockHandleRelease(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
    exitsim();
  }

  return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxSend(mbox_t handle,int length, void* message);
//
// Send a message (pointed to by "message") of length
// "length" bytes to the specified mailbox.  Messages of
// length 0 are allowed.  The call 
// blocks when there is not enough space in the mailbox.
// Messages cannot be longer than MBOX_MAX_MESSAGE_LENGTH.
// Note that the calling process must have opened the 
// mailbox via MboxOpen.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxSend(mbox_t handle, int length, void* message) {

  int ct;
  int system_buffer_index;
  unsigned int curr_pid = GetCurrentPid();
  char * message_str = (char *) message;

  // Check if the length of the message can fit into the buffer slot
  if(length < 0 || length >  MBOX_MAX_MESSAGE_LENGTH){
    return MBOX_FAIL;
  }

  // Check if handle is a valid number
  if(handle < 0 || handle >= MBOX_NUM_MBOXES){
    return MBOX_FAIL;
  }
  
  // Check if the mbox is reserved (activated)
  if(system_mbox[handle].num_of_pid_inuse < 0){
    return MBOX_FAIL;
  }

  // Check if the mbox is opened
  if(system_mbox[handle].mbox_pid_list[curr_pid] == 0){
    printf("Mbox Send Error: The mbox %d hasn't been opened yet\n", handle);
    return MBOX_FAIL;
  }

  // Acquire the lock of the mbox message buffer
  if(LockHandleAcquire(system_mbox_message.system_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Acquire lock for the mbox %d!\n", handle);
    exitsim();
  }

  // Check if the buffer is full. If so, wait
  while(system_mbox_message.system_buffer_slot_used == MBOX_NUM_BUFFERS){
    if(CondHandleWait(system_mbox_message.system_buffer_empty) != SYNC_SUCCESS){
      printf("FATAL ERROR: Wait on CV empty for system message!\n");
      exitsim();
    }
  }

  // Acquire the lock of the mbox
  if(LockHandleAcquire(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Acquire lock for the mbox %d!\n", handle);
    exitsim();
  }


  // Else, check if the mbox is full. If so, wait
  while(system_mbox[handle].mbox_buffer_slot_used == MBOX_MAX_BUFFERS_PER_MBOX){
    if(CondHandleWait(system_mbox[handle].mbox_buffer_empty) != SYNC_SUCCESS){
      printf("FATAL ERROR: Wait on CV empty for mbox %d!\n", handle);
      exitsim();
    }
  }

  // Find out an available slot in the system message buffer for the message
  for(system_buffer_index = 0; system_buffer_index < MBOX_NUM_BUFFERS; system_buffer_index++){
    if(system_mbox_message.system_buffer_index_array[system_buffer_index] == 0){
      system_mbox_message.system_buffer_index_array[system_buffer_index] = 1; // Reserved the slot
      break;
    }
  }
  
  // Double check if the index is valid
  if(system_buffer_index == MBOX_NUM_BUFFERS){
    printf("FATAL ERROR: System buffer sync error\n");
    exitsim();
  }


  // Put the message into the system message slot 
  for(ct = 0; ct < length; ct++){
    system_mbox_message.system_message_buffer[system_buffer_index][ct] = message_str[ct];
  }

  // Set the mbox linking
  system_mbox[handle].mbox_buffer_index_array[system_mbox[handle].mbox_buffer_head] = system_buffer_index;
  
  // Update head and used info
  system_mbox_message.system_buffer_slot_used += 1;
  /* system_mbox_message.system_buffer_head = (system_mbox_message.system_buffer_head + 1) % MBOX_NUM_BUFFERS; */

  system_mbox[handle].mbox_buffer_slot_used += 1;
  system_mbox[handle].mbox_buffer_head = (system_mbox[handle].mbox_buffer_head + 1) % MBOX_MAX_BUFFERS_PER_MBOX;

  
  // Signal mbox fill CV
  if(CondHandleSignal(system_mbox[handle].mbox_buffer_fill) != SYNC_SUCCESS){
    printf("FATAL ERROR: Signal on CV fill for mbox %d!\n", handle);
    exitsim();
  }
  
  // Release the lock of the mbox
  if(LockHandleRelease(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
    exitsim();
  }


  // Signal system buffer fill CV
  if(CondHandleSignal(system_mbox_message.system_buffer_fill) != SYNC_SUCCESS){
    printf("FATAL ERROR: Signal on CV empty for system message!\n");
    exitsim();
  }


  // Release the lock of the mbox message buffer
  if(LockHandleRelease(system_mbox_message.system_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
    exitsim();
  }


  return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxRecv(mbox_t handle, int maxlength, void* message);
//
// Receive a message from the specified mailbox.  The call 
// blocks when there is no message in the buffer.  Maxlength
// should indicate the maximum number of bytes that can be
// copied from the buffer into the address of "message".  
// An error occurs if the message is larger than maxlength.
// Note that the calling process must have opened the mailbox 
// via MboxOpen.
//   
// Returns MBOX_FAIL on failure.
// Returns number of bytes written into message on success.
//
//-------------------------------------------------------
int MboxRecv(mbox_t handle, int maxlength, void* message) {

  int ct;
  unsigned int curr_pid = GetCurrentPid();
  int system_buffer_index = system_mbox[handle].mbox_buffer_index_array[system_mbox[handle].mbox_buffer_tail];
  char * message_str = (char *) message;
  
  // Check if the length of the message can fit into the buffer slot
  if(maxlength < 0 || maxlength >  MBOX_MAX_MESSAGE_LENGTH){
    return MBOX_FAIL;
  }
  
  // Check if handle is a valid number
  if(handle < 0 || handle >= MBOX_NUM_MBOXES){
    return MBOX_FAIL;
  }
  
  // Check if the mbox is reserved (activated). If not, return FAIL
  if(system_mbox[handle].num_of_pid_inuse < 0){
    return MBOX_FAIL;
  }

  // Check if the mbox is opened. If the mbox is not opened, return FAIL
  if(system_mbox[handle].mbox_pid_list[curr_pid] == 0){
    printf("Mbox Send Error: The mbox %d hasn't been opened yet\n", handle);
    return MBOX_FAIL;
  }

  // Acquire the lock of the mbox message buffer
  if(LockHandleAcquire(system_mbox_message.system_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Acquire lock for the mbox %d!\n", handle);
    exitsim();
  }
  
  // Check if the buffer is empty. If so, wait
  while(system_mbox_message.system_buffer_slot_used == 0){
    if(CondHandleWait(system_mbox_message.system_buffer_fill) != SYNC_SUCCESS){
      printf("FATAL ERROR: Wait on CV empty for system message!\n");
      exitsim();
    }
  }

  // Acquire the lock of the mbox
  if(LockHandleAcquire(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Acquire lock for the mbox %d!\n", handle);
    exitsim();
  }
  
  
  // Else, check if the mbox is empty. If so, wait
  while(system_mbox[handle].mbox_buffer_slot_used == 0){
    if(CondHandleWait(system_mbox[handle].mbox_buffer_fill) != SYNC_SUCCESS){
      printf("FATAL ERROR: Wait on CV empty for mbox %d!\n", handle);
      exitsim();
    }
  }

  // Receive the message from the mbox
  for(ct = 0; ct < maxlength; ct++){
    message_str[ct] = system_mbox_message.system_message_buffer[system_buffer_index][ct];
  }

  // Make the system buffer slot available again
  system_mbox_message.system_buffer_index_array[system_mbox[handle].mbox_buffer_tail] = 0;
  system_mbox_message.system_buffer_slot_used -= 1;

  // Update mbox used and mbox buffer tail info
  system_mbox[handle].mbox_buffer_slot_used -= 1;
  system_mbox[handle].mbox_buffer_tail = (system_mbox[handle].mbox_buffer_tail + 1) % MBOX_MAX_BUFFERS_PER_MBOX;


  // Signal mbox empty CV
  if(CondHandleSignal(system_mbox[handle].mbox_buffer_empty) != SYNC_SUCCESS){
    printf("FATAL ERROR: Signal on CV fill for mbox %d!\n", handle);
    exitsim();
  }

  
  // Release the lock of the mbox
  if(LockHandleRelease(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
    exitsim();
  }


  // Signal system buffer empty CV
  if(CondHandleSignal(system_mbox_message.system_buffer_empty) != SYNC_SUCCESS){
    printf("FATAL ERROR: Signal on CV empty for system message!\n");
    exitsim();
  }


  // Release the lock of the mbox message buffer
  if(LockHandleRelease(system_mbox_message.system_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
    exitsim();
  }



  return MBOX_SUCCESS;
}

//--------------------------------------------------------------------------------
// 
// int MboxCloseAllByPid(int pid);
//
// Scans through all mailboxes and removes this pid from their "open procs" list.
// If this was the only open process, then it makes the mailbox available.  Call
// this function in ProcessFreeResources in process.c.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//--------------------------------------------------------------------------------
int MboxCloseAllByPid(int pid) {

  int ct;

  // Check if the pid is valid
  if(pid < 0 || pid >= PROCESS_MAX_PROCS){
    return MBOX_FAIL;
  }

  // Go througth all the mboxes
  for(ct = 0; ct < MBOX_NUM_MBOXES; ct++){
    // Acquire the lock of the mbox
    if(LockHandleAcquire(system_mbox[ct].mbox_buffer_lock) != SYNC_SUCCESS){
      printf("FATAL ERROR: Acquire lock for the mbox %d!\n", ct);
      exitsim();
    }
  
    // Check if the pid is in the mbox's "open procs" list. 
    // If so, remove the pid and make the mailbox available if no other proc opens the mbox
    // Else, do nothing
    if(system_mbox[ct].mbox_pid_list[pid] == 1){
      system_mbox[ct].mbox_pid_list[pid] = 0;
      system_mbox[ct].num_of_pid_inuse -= 1;
      if(system_mbox[ct].num_of_pid_inuse == 0){
	system_mbox[ct].num_of_pid_inuse = -1; // Put the mbox back to the pool
      }
    }

    // Release the lock of the mbox
    if(LockHandleRelease(system_mbox[ct].mbox_buffer_lock) != SYNC_SUCCESS){
      printf("FATAL ERROR: Release lock for the mbox %d!\n", ct);
      exitsim();
    }

  }

  return MBOX_SUCCESS;
}
