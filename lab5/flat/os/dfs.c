#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "queue.h"
#include "disk.h"
#include "dfs.h"
#include "synch.h"

static dfs_inode inodes[DFS_INODE_MAX_NUM]; // all inodes
static dfs_superblock sb; // superblock
static uint32 fbv[DFS_FBV_MAX_NUM_WORDS]; // Free block vector

// New added variable to check if filesystem has been opened
static int fs_open = 0;

static uint32 negativeone = 0xFFFFFFFF;
static inline uint32 invert(uint32 n) { return n ^ negativeone; }

lock_t lock; // global lock for later use

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.

// STUDENT: put your file system level functions below.
// Some skeletons are provided. You can implement additional functions.

///////////////////////////////////////////////////////////////////
// Non-inode functions first
///////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------
// DfsModuleInit is called at boot time to initialize things and
// open the file system for use.
//-----------------------------------------------------------------

void DfsModuleInit() {
  // You essentially set the file system as invalid and then open 
  // using DfsOpenFileSystem().
  lock = LockCreate(); // create lock for later use
  DfsInvalidate();
  DfsOpenFileSystem();
}

//-----------------------------------------------------------------
// DfsInavlidate marks the current version of the filesystem in
// memory as invalid.  This is really only useful when formatting
// the disk, to prevent the current memory version from overwriting
// what you already have on the disk when the OS exits.
//-----------------------------------------------------------------

void DfsInvalidate() {
// This is just a one-line function which sets the valid bit of the 
// superblock to 0.
  sb.valid = 0;
}

//-------------------------------------------------------------------
// DfsOpenFileSystem loads the file system metadata from the disk
// into memory.  Returns DFS_SUCCESS on success, and DFS_FAIL on 
// failure.
//-------------------------------------------------------------------

int DfsOpenFileSystem() {

  int ct;
  disk_block db_tmp;
  disk_block db_tmp_36[36];
  disk_block db_tmp_4[4];
  int inode_block_num = 0; // disk blocks for inodes
  int fbv_block_num = 0; // disk blocks for fbv

  //Basic steps:
  // Check that filesystem is not already open
  if(fs_open == 1){
    return DFS_FAIL;
  }

  fs_open = 1;

  // Read superblock from disk.  Note this is using the disk read rather 
  // than the DFS read function because the DFS read requires a valid 
  // filesystem in memory already, and the filesystem cannot be valid 
  // until we read the superblock. Also, we don't know the block size 
  // until we read the superblock, either.

  // Read the disk block No.1
  if(DiskReadBlock(1, &db_tmp) != DISK_BLOCKSIZE){
    return DFS_FAIL;
  }

  // Copy the data from the block we just read into the superblock in memory
  /* sb = (dfs_superblock) db_tmp; */
  // superblock has 24 bytes
  bcopy(&db_tmp, &sb, sizeof(dfs_superblock));


  // All other blocks are sized by virtual block size:
  // Read inodes
  inode_block_num = sb.inode_num_inArray * sizeof(dfs_inode) / DISK_BLOCKSIZE; // 192 * 96 / 512 = 36
  for(ct = 0; ct < inode_block_num; ct ++){
    if(DiskReadBlock(ct + (sb.inode_start_block * (DFS_BLOCKSIZE / DISK_BLOCKSIZE)), &(db_tmp_36[ct])) != DISK_BLOCKSIZE){
      return DFS_FAIL;
    }
  }
  /* inodes = (dfs_inode *) db_tmp_36; */
  bcopy((char *)db_tmp_36, (char *)inodes, sb.inode_num_inArray * sizeof(dfs_inode));

  // Read free block vector
  fbv_block_num = sb.fsb_num / 8 / DISK_BLOCKSIZE; // 16384 / 8 / 512 = 4
  for(ct = 0; ct < fbv_block_num; ct ++){
    if(DiskReadBlock(ct + (sb.fbv_start_block * (DFS_BLOCKSIZE / DISK_BLOCKSIZE)), &(db_tmp_4[ct])) != DISK_BLOCKSIZE){
      return DFS_FAIL;
    }
  }
  
  /* fbv = (uint32 *) db_tmp_4; */
  bcopy((char *) db_tmp_4, (char *) fbv, DISK_BLOCKSIZE * fbv_block_num);

  // Change superblock to be invalid, write back to disk, then change 
  sb.valid = 0;
  // ???????????
  bcopy(&sb, &db_tmp, sizeof(struct dfs_superblock));
  if(DiskWriteBlock(1, &db_tmp) != DISK_BLOCKSIZE){
    return DFS_FAIL;
  }

  // it back to be valid in memory
  sb.valid = 1;

  return 0;
}


//-------------------------------------------------------------------
// DfsCloseFileSystem writes the current memory version of the
// filesystem metadata to the disk, and invalidates the memory's 
// version.
//-------------------------------------------------------------------

int DfsCloseFileSystem() {

  int ct;
  disk_block db_tmp;
  disk_block db_tmp_36[36];
  disk_block db_tmp_4[4];
  int inode_block_num = sb.inode_num_inArray * sizeof(dfs_inode) / DISK_BLOCKSIZE; // 192 * 96 / 512 = 36
  int fbv_block_num = sb.fsb_num / sizeof(uint8) / DISK_BLOCKSIZE; // 16384 / 8 / 512 = 4
  // Write the current memory verison of the filesystem metadata

  // Write sb
  /* db_tmp = (disk_block) sb; */
  /* dstrncpy(&db_tmp, &sb, 24); */

  bcopy((char *)&sb, (char *)&db_tmp, sizeof(dfs_superblock)); // no cast??
  
  if(DiskWriteBlock(1, &db_tmp) != DISK_BLOCKSIZE){
    return DFS_FAIL;
  }

  // Write inodes
  /* db_tmp_36 = (disk_block *) inodes; */
  /* dstrncpy(db_tmp_36, inodes, 192 * 96); */
  bcopy((char *) inodes, (char *) db_tmp_36, sb.inode_num_inArray * sizeof(dfs_inode));

  for(ct = 0; ct < inode_block_num; ct++){
    if(DiskWriteBlock(ct + (sb.inode_start_block * (DFS_BLOCKSIZE / DISK_BLOCKSIZE)), &db_tmp_36[ct]) != DISK_BLOCKSIZE){
      return DFS_FAIL;
    }
  }

  // Write fbv
  /* db_tmp_4 = (disk_block *) fbv; */
  /* dstrncpy(db_tmp_4, fbv, 512 * 4); */

  bcopy((char *) fbv, (char *) db_tmp_4, DISK_BLOCKSIZE * fbv_block_num);

  for(ct = 0; ct < fbv_block_num; ct++){
    if(DiskWriteBlock(ct + (sb.fbv_start_block * (DFS_BLOCKSIZE / DISK_BLOCKSIZE)), &db_tmp_4[ct]) != DISK_BLOCKSIZE){
      return DFS_FAIL;
    }
  }


  // Invalidates the memory's version of filesystem
  fs_open = 0;
  sb.valid = 0;

  return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsAllocateBlock allocates a DFS block for use. Remember to use 
// locks where necessary.
//-----------------------------------------------------------------

int DfsAllocateBlock() {
  // Check that file system has been validly loaded into memory
  // Find the first free block using the free block vector (FBV), mark it in use
  // Return handle to block
  int ct;
  int bit_index = 31;
  uint32 fbv_entry_mask = 0x80000000;
  int dfs_block_number;

  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }


  if(LockHandleAcquire(lock) == SYNC_SUCCESS)
    {
      dbprintf('s', "dfs (%d): got a lock before search fbv.\n", GetCurrentPid());
    }
  // Find a chunk of pages that has a least one free page
  for(ct = 0; ct < (sb.fsb_num / sizeof(fbv[0])); ct++){
    if(fbv[ct] != 1){
      break;
    }
  }
  
  // Find bit index of the free page 
  while( (fbv[ct] & fbv_entry_mask) == 1){
    fbv_entry_mask = fbv_entry_mask >> 1;
    bit_index -= 1;
  }
  
  // Mark the block number as inuse
  fbv[ct] = fbv[ct] | (0x1 << bit_index);

  if(LockHandleRelease(lock) == SYNC_SUCCESS)
    {
      dbprintf('s', "dfs (%d): released a lock after change fbv.\n", GetCurrentPid());
    }

  // Return the block number
  dfs_block_number = ct * 32 + (32 - bit_index);

  return dfs_block_number;
}


//-----------------------------------------------------------------
// DfsFreeBlock deallocates a DFS block.
//-----------------------------------------------------------------

int DfsFreeBlock(uint32 blocknum) {
  
  // Mart the block num in fbv as unused block
  uint32 fbv_index = blocknum >> 5;
  uint32 fbv_bit_index = blocknum & 0x1f;

  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }
  if(LockHandleAcquire(lock) == SYNC_SUCCESS)
    {
      dbprintf('s', "dfs (%d): got a lock before free fbv.\n", GetCurrentPid());
    }
  // Set the freemap entry to 1 --> freed!
  fbv[fbv_index] = fbv[fbv_index] & invert(0x1 << fbv_bit_index);
  if(LockHandleRelease(lock) == SYNC_SUCCESS)
    {
      dbprintf('s', "dfs (%d): released a lock after free fbv.\n", GetCurrentPid());
    }
  return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsReadBlock reads an allocated DFS block from the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to read from it.  Returns DFS_FAIL
// on failure, and the number of bytes read on success.  
//-----------------------------------------------------------------

int DfsReadBlock(uint32 blocknum, dfs_block *b) {
  
  int ct;
  uint32 disk_block_num = blocknum * 2;
  disk_block db_tmp_2[2];
  uint32 fbv_index = blocknum >> 5;
  uint32 fbv_bit_index = blocknum & 0x1f;

  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }

  // if the fs block has not been allocated, return fail
  if(fbv[fbv_index] && (0x1 << fbv_bit_index)){
    return DFS_FAIL;
  }

  for(ct = 0; ct < (DFS_BLOCKSIZE / DISK_BLOCKSIZE); ct ++){
    if(DiskReadBlock(ct + disk_block_num, &(db_tmp_2[ct])) != DISK_BLOCKSIZE){
      return DFS_FAIL;
    }
  }
  
  bcopy((char *) db_tmp_2, b->data, 2 * DISK_BLOCKSIZE);

  return 2 * DISK_BLOCKSIZE;
}


//-----------------------------------------------------------------
// DfsWriteBlock writes to an allocated DFS block on the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to write to it.  Returns DFS_FAIL
// on failure, and the number of bytes written on success.  
//-----------------------------------------------------------------

int DfsWriteBlock(uint32 blocknum, dfs_block *b){

  int ct;
  uint32 disk_block_num = blocknum * 2;
  disk_block db_tmp_2[2];

  uint32 fbv_index = blocknum >> 5;
  uint32 fbv_bit_index = blocknum & 0x1f;

  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }

  // if the fs block has not been allocated, return fail
  if(fbv[fbv_index] && (0x1 << fbv_bit_index)){
    return DFS_FAIL;
  }
  
  bcopy(b->data, (char *) db_tmp_2, 2 * DISK_BLOCKSIZE);

  for(ct = 0; ct < (DFS_BLOCKSIZE / DISK_BLOCKSIZE); ct ++){
    if(DiskWriteBlock(ct + disk_block_num, &(db_tmp_2[ct])) != DISK_BLOCKSIZE){
      return DFS_FAIL;
    }
  }

  return 2 * DISK_BLOCKSIZE;
}


////////////////////////////////////////////////////////////////////////////////
// Inode-based functions
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------
// DfsInodeFilenameExists looks through all the inuse inodes for 
// the given filename. If the filename is found, return the handle 
// of the inode. If it is not found, return DFS_FAIL.
//-----------------------------------------------------------------

int DfsInodeFilenameExists(char *filename) {

  int ct;

  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }
  
  for(ct = 0; ct < DFS_INODE_MAX_NUM; ct++){
    if((inodes[ct].inuse == 1) && inodes[])
  }


  return 0;
}


//-----------------------------------------------------------------
// DfsInodeOpen: search the list of all inuse inodes for the 
// specified filename. If the filename exists, return the handle 
// of the inode. If it does not, allocate a new inode for this 
// filename and return its handle. Return DFS_FAIL on failure. 
// Remember to use locks whenever you allocate a new inode.
//-----------------------------------------------------------------

uint32 DfsInodeOpen(char *filename) {
  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }

  return 0;
}


//-----------------------------------------------------------------
// DfsInodeDelete de-allocates any data blocks used by this inode, 
// including the indirect addressing block if necessary, then mark 
// the inode as no longer in use. Use locks when modifying the 
// "inuse" flag in an inode.Return DFS_FAIL on failure, and 
// DFS_SUCCESS on success.
//-----------------------------------------------------------------

int DfsInodeDelete(uint32 handle) {
  return 0;
}


//-----------------------------------------------------------------
// DfsInodeReadBytes reads num_bytes from the file represented by 
// the inode handle, starting at virtual byte start_byte, copying 
// the data to the address pointed to by mem. Return DFS_FAIL on 
// failure, and the number of bytes read on success.
//-----------------------------------------------------------------

int DfsInodeReadBytes(uint32 handle, void *mem, int start_byte, int num_bytes) {
  return 0;
}


//-----------------------------------------------------------------
// DfsInodeWriteBytes writes num_bytes from the memory pointed to 
// by mem to the file represented by the inode handle, starting at 
// virtual byte start_byte. Note that if you are only writing part 
// of a given file system block, you'll need to read that block 
// from the disk first. Return DFS_FAIL on failure and the number 
// of bytes written on success.
//-----------------------------------------------------------------

int DfsInodeWriteBytes(uint32 handle, void *mem, int start_byte, int num_bytes) {
  
  return 0;
}


//-----------------------------------------------------------------
// DfsInodeFilesize simply returns the size of an inode's file. 
// This is defined as the maximum virtual byte number that has 
// been written to the inode thus far. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeFilesize(uint32 handle) {
  return 0;
}


//-----------------------------------------------------------------
// DfsInodeAllocateVirtualBlock allocates a new filesystem block 
// for the given inode, storing its blocknumber at index 
// virtual_blocknumber in the translation table. If the 
// virtual_blocknumber resides in the indirect address space, and 
// there is not an allocated indirect addressing table, allocate it. 
// Return DFS_FAIL on failure, and the newly allocated file system 
// block number on success.
//-----------------------------------------------------------------

uint32 DfsInodeAllocateVirtualBlock(uint32 handle, uint32 virtual_blocknum) {

  return 0;
}



//-----------------------------------------------------------------
// DfsInodeTranslateVirtualToFilesys translates the 
// virtual_blocknum to the corresponding file system block using 
// the inode identified by handle. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeTranslateVirtualToFilesys(uint32 handle, uint32 virtual_blocknum) {
  return 0;
}
