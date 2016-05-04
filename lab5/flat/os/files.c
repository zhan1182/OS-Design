#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "dfs.h"
#include "files.h"
#include "synch.h"

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.

// STUDENT: put your file-level functions here

static file_descriptor files[DFS_INODE_MAX_NUM];
static int file_ct = 0;
static lock_t lock;

void file_init(int handle){
  files[handle].inode_handle = -1;
  files[handle].pid = -1;
  files[handle].current_byte = 0;
  files[handle].mode_num = -1;
  files[handle].end_flag = 0;
  bzero(files[handle].filename, 44);
}

void files_initialize(){
  int ct;

  // Create a lock
  lock = LockCreate(); // create lock for later use

  // Initialize the array of file descriptors
  for(ct = 0; ct < DFS_INODE_MAX_NUM; ct++){
    file_init(ct);
  }
}

int get_file_mode(char * mode){
  if(dstrncmp(mode, "r", 1) == 0){
    return 0;
  }
  
  if(dstrncmp(mode, "w", 1) == 0){
    return 1;
  }

  if(dstrncmp(mode, "rw", 2) == 0){
    return 2;
  }

  return -1;
}

// Return the file handle if the file is not openned has another process
// If the inode handle is not used --> find an unused file descriptor --> use the inode
// Return FILE_FAIL otherwise
int file_check_process(int inode_handle, char * filename){
  int ct;

  for(ct = 0; ct < DFS_INODE_MAX_NUM; ct++){
    if(files[ct].inode_handle == inode_handle){
      if(files[ct].pid == -1 && dstrncmp(files[ct].filename, filename, dstrlen(filename)) == 0){
	return ct;
      }
      else{
	return FILE_FAIL;
      }
    }
  }

  for(ct = 0; ct < DFS_INODE_MAX_NUM; ct++){
    if(files[ct].inode_handle == -1){
      // Set the file descriptor
      files[ct].inode_handle = inode_handle;
      dstrncpy(files[ct].filename, filename, dstrlen(filename));
      return ct;
    }
  }

  return FILE_FAIL;
}

void lock_operation(int operation_mode){
  
  if(operation_mode == 1){
    if(LockHandleAcquire(lock) == SYNC_SUCCESS){
      dbprintf('s', "dfs (%d): got a lock before free fbv.\n", GetCurrentPid());
    }
  }
  else{
    if(LockHandleRelease(lock) == SYNC_SUCCESS){
      dbprintf('s', "dfs (%d): released a lock after free fbv.\n", GetCurrentPid());
    }
  }
}

int FileOpen(char *filename, char *mode)
{

  int mode_num = get_file_mode(mode);
  int inode_handle;
  int file_handle;

  // Reach the max file opened number in the system
  if(file_ct >= 15){
    return FILE_FAIL;
  }

  // Check if the mode makes sense
  if(mode_num < 0 || mode_num > 2){
    return FILE_FAIL;
  }

  // Lock here
  lock_operation(1);

  // Read mode
  if(mode_num == 0){
    inode_handle = DfsInodeFilenameExists(filename);
  }
  else{
    // Get the existing inode handle or open a new inode for this file
    inode_handle = DfsInodeOpen(filename);
  }

  if(inode_handle == DFS_FAIL){
    // File doesn't exist or cannot be created, unlock and return
    lock_operation(0);
    return FILE_FAIL;
  }

  // If the file has been opened by other processes --> return error
  // If r mode or w/rw mode (no create) --> return the file handle
  // If w/rw mode (create) --> find an unused file descriptor --> save the inode handle --> return the file handle
  file_handle = file_check_process(inode_handle, filename);
  if(file_handle == FILE_FAIL){
    // Unlock and return
    lock_operation(0);
    return FILE_FAIL;
  }
  
  // Open the file for the current process and save all the information
  files[file_handle].pid = GetCurrentPid();
  files[file_handle].mode_num = mode_num;

  // If w/rw mode --> reset the file size --> free all the used data blocks
  if(mode_num == 1 || mode_num == 2){
    files[file_handle].
  }


  if(dstrncmp(files[file_handle].filename, filename, dstrlen(filename)) != 0){
    dstrncpy(files[file_handle].filename, filename, dstrlen(filename));
  }
  
  // Unlock
  lock_operation(0);

  return file_handle;
}

// Handle is the index of the array of file descriptor
int FileClose(int handle)
{

  // If the file is not opened for the current process --> return error
  if(files[handle].pid != GetCurrentPid()){
    return FILE_FAIL;
  }

  // Lock here
  lock_operation(1);

  // Update file descriptor
  files[handle].pid = -1;
  files[handle].current_byte = 0;
  files[handle].mode_num = -1;
  files[handle].end_flag = 0;

  // Update the inode information??

  // Unlock
  lock_operation(0);

  return FILE_SUCCESS;
}

int FileRead(int handle, void *mem, int num_bytes)
{
  int number;

  // If the file is not opened for the current process --> return error
  if(files[handle].pid != GetCurrentPid()){
    return FILE_FAIL;
  }

  number = DfsInodeReadBytes(files[handle].inode_handle, mem, files[handle].current_byte, num_bytes);

  if(number == DFS_FAIL){
    return FILE_FAIL;
  }

  // Check if read to the end of the file
  if(number < num_bytes){
    // Set the EOF flag
    files[handle].end_flag = 1;
  }

  return number;
}


int FileWrite(int handle, void *mem, int num_bytes)
{
  int number;

  // If the file is not opened for the current process --> return error
  if(files[handle].pid != GetCurrentPid()){
    return FILE_FAIL;
  }

  number = DfsInodeWriteBytes(files[handle].inode_handle, mem, files[handle].current_byte, num_bytes);

  if(number == DFS_FAIL){
    return FILE_FAIL;
  }

  return number;
}

int FileSeek(int handle, int num_bytes, int from_where)
{

  // If the file is not opened for the current process --> return error
  if(files[handle].pid != GetCurrentPid()){
    return FILE_FAIL;
  }

  if(from_where == FILE_SEEK_SET){
    // Seek from the beginning
  }
  else if(from_where == FILE_SEEK_END){
  }
  else if(from_where == FILE_SEEK_CUR){
  }
  else{
    return FILE_FAIL;
  }

  // Clear the EOF flag
  files[handle].end_flag = 0;

  return 0;
}


int FileDelete(char *filename)
{
  int ct;

  // Lock here
  lock_operation(1);

  for(ct = 0; ct < DFS_INODE_MAX_NUM; ct++){
    // Find the file descriptor
    if(dstrncmp(files[ct].filename, filename, dstrlen(filename)) == 0){
      // If the file is opened by other process --> return error
      if(files[ct].pid != GetCurrentPid()){
	return FILE_FAIL;
      }
      // Delete the inode from the inode handle
      if(DfsInodeDelete(files[ct].inode_handle) != DFS_SUCCESS){
	// Unlock
	lock_operation(0);
	return FILE_FAIL;
      }
      // Re init the file descriptor
      file_init(ct);
    } 
  }
  // Unlock
  lock_operation(0);

  // The filename didn't find
  if(ct >= DFS_INODE_MAX_NUM){
    return FILE_FAIL;
  }

  return FILE_SUCCESS;
}
