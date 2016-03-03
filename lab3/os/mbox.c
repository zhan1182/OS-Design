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

  system_mbox_message.system_buffers_head = 0;
  system_mbox_message.system_buffers_tail = 0;
  system_mbox_message.system_buffers_slot_left = 50;
  system_mbox_message.system_buffers_lock = LockCreate();

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
  system_mbox[mbox_ct].mbox_buffers_head = 0;
  system_mbox[mbox_ct].mbox_buffers_tail = 0;
  system_mbox[mbox_ct].mbox_buffer_lock = LockCreate();
  for(ct = 0; ct < MBOX_MAX_PROCS_NUM; ct++){
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
  // Check if handle is a valid number
  if(handle < 0 || handle >= MBOX_NUM_MBOXES){
    return MBOX_FAIL;
  }

  // Check if the mbox is reserved (activated)
  if(system_mbox[handle].num_of_pid_inuse < 0){
    return MBOX_FAIL;
  }

  unsigned int curr_pid = GetCurrentPid();

  // Acquire the lock
  if(LockHandleAcquire(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Acquire lock for the mbox %d!\n", handle);
    Exit();
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
    printf("FATAL ERROR: Unkown Pid or mbox %d not reserved!\n", handle);
    // Release the lock
    if(LockHandleRelease(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
      printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
      Exit();
    }
    return MBOX_FAIL;
  }
  
  // Release the lock
  if(LockHandleRelease(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
    Exit();
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
  // Check if handle is a valid number
  if(handle < 0 || handle >= MBOX_NUM_MBOXES){
    return MBOX_FAIL;
  }
  
  // Check if the mbox is reserved (activated)
  if(system_mbox[handle].num_of_pid_inuse < 0){
    return MBOX_FAIL;
  }

  unsigned int curr_pid = GetCurrentPid();

  // Acquire the lock
  if(LockHandleAcquire(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Acquire lock for the mbox %d!\n", handle);
    Exit();
  }

  int mbox_pid_status = system_mbox[handle].mbox_pid_list[curr_pid];
  if(mbox_pid_status == -1 || mbox_pid_status == 0){
    printf("The mbox %d has already been closed or not reserved\n", handle);
  }
  else if(mbox_pid_status == 1){
    // Update the number of pid used
    system_mbox[handle].num_of_pid_inuse -= 1;
    system_mbox[handle].mbox_pid_list[GetCurrentPid()] = 0;

    // Return the mbox to the pool of available mboxes for the system
    if(system_mbox[handle].num_of_pid_inuse == 0){
      system_mbox[handle].num_of_pid_inuse = -1;
    }
  }
  else{
    printf("FATAL ERROR: Unkown Pid or mbox %d not reserved!\n", handle);
    // Release the lock
    if(LockHandleRelease(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
      printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
      Exit();
    }
    return MBOX_FAIL;
  }
  

  // Release the lock
  if(LockHandleRelease(system_mbox[handle].mbox_buffer_lock) != SYNC_SUCCESS){
    printf("FATAL ERROR: Release lock for the mbox %d!\n", handle);
    Exit();
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
  return MBOX_FAIL;
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
  return MBOX_FAIL;
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
  return MBOX_FAIL;
}
