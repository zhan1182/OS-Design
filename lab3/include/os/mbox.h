#ifndef __MBOX_OS__
#define __MBOX_OS__

#include "synch.h"

#define MBOX_NUM_MBOXES 16           // Maximum number of mailboxes allowed in the system
#define MBOX_NUM_BUFFERS 50          // Maximum number of message buffers allowed in the system
#define MBOX_MAX_BUFFERS_PER_MBOX 10 // Maximum number of buffer slots available to any given mailbox
#define MBOX_MAX_MESSAGE_LENGTH 50   // Buffer size of 50 for each message

#define MBOX_FAIL -1
#define MBOX_SUCCESS 1

//---------------------------------------------
// Define your mailbox structures here
//--------------------------------------------

#define MBOX_MAX_PROCS_NUM 32

typedef struct mbox_message {
  char system_message_buffers[MBOX_NUM_BUFFERS][MBOX_MAX_MESSAGE_LENGTH];
  int system_buffers_head;
  int system_buffers_tail;
  lock_t system_buffers_lock;
} mbox_message;

typedef struct mbox {
  int mbox_buffers_index_array[MBOX_MAX_BUFFERS_PER_MBOX];
  int mbox_buffers_head;
  int mbox_buffers_tail;  
  lock_t mbox_buffer_lock;
  int num_of_pid_inuse; // 0 means no process opens the mbox --> The mbox is available
  int mbox_pid_list[MBOX_MAX_PROCS_NUM];
} mbox;

typedef int mbox_t; // This is the "type" of mailbox handles

//-------------------------------------------
// Prototypes for Mbox functions you have to write
//-------------------------------------------

void MboxModuleInit();
mbox_t MboxCreate();
int MboxOpen(mbox_t m);
int MboxClose(mbox_t m);
int MboxSend(mbox_t m, int length, void *message);
int MboxRecv(mbox_t m, int maxlength, void *message);
int MboxCloseAllByPid(int pid);

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

#endif
