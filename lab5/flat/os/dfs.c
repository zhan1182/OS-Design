#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "queue.h"
#include "disk.h"
#include "dfs.h"
#include "synch.h"

#include "process.h"

static dfs_inode inodes[DFS_INODE_MAX_NUM]; // all inodes
static dfs_superblock sb; // superblock
static uint32 fbv[DFS_FBV_MAX_NUM_WORDS]; // Free block vector

// New added variable to check if filesystem has been opened
static int fs_open = 0;

static uint32 negativeone = 0xFFFFFFFF;
static inline uint32 invert(uint32 n) { return n ^ negativeone; }

// Add static?
static lock_t lock; // global lock for later use

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
  bcopy((char *) (&db_tmp), (char *) (&sb), sizeof(dfs_superblock));


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
  bcopy( (char *) (&sb), (char *) (&db_tmp), sizeof(struct dfs_superblock));
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
  int fbv_block_num = sb.fsb_num / sizeof(char) / DISK_BLOCKSIZE; // 16384 / 8 / 512 = 4
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

  for(ct = 0; ct < (sb.fsb_size / DISK_BLOCKSIZE); ct ++){
    if(DiskReadBlock(ct + disk_block_num, &(db_tmp_2[ct])) != DISK_BLOCKSIZE){
      return DFS_FAIL;
    }
  }
  
  bcopy((char *) db_tmp_2, b->data, sb.fsb_size);

  return sb.fsb_size;
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
  
  bcopy(b->data, (char *) db_tmp_2, sb.fsb_size);

  for(ct = 0; ct < (sb.fsb_size / DISK_BLOCKSIZE); ct ++){
    if(DiskWriteBlock(ct + disk_block_num, &(db_tmp_2[ct])) != DISK_BLOCKSIZE){
      return DFS_FAIL;
    }
  }

  return sb.fsb_size;
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
  
  for(ct = 0; ct < sb.inode_num_inArray; ct++){
    if((inodes[ct].inuse == 1)){
      if(dstrncmp(inodes[ct].filename, filename, dstrlen(filename)) == 0){
	return ct;
      }
    }
  }
  
  return DFS_FAIL;
}


//-----------------------------------------------------------------
// DfsInodeOpen: search the list of all inuse inodes for the 
// specified filename. If the filename exists, return the handle 
// of the inode. If it does not, allocate a new inode for this 
// filename and return its handle. Return DFS_FAIL on failure. 
// Remember to use locks whenever you allocate a new inode.
//-----------------------------------------------------------------

int DfsInodeOpen(char *filename) {

  int inode_ct;
  int ct;

  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }

  inode_ct = DfsInodeFilenameExists(filename);

  if(inode_ct != DFS_FAIL){
    return inode_ct;
  }

  
  if(LockHandleAcquire(lock) == SYNC_SUCCESS){
    dbprintf('s', "dfs (%d): got a lock before free fbv.\n", GetCurrentPid());
  }

  for(ct = 0; ct < sb.inode_num_inArray; ct++){
    if((inodes[ct].inuse == 0)){
	break;
    }
  }
  
  if(ct >= sb.inode_num_inArray){
    if(LockHandleRelease(lock) == SYNC_SUCCESS){
      dbprintf('s', "dfs (%d): released a lock after free fbv.\n", GetCurrentPid());
    }
    return DFS_FAIL;
  }

  // Set the inode as inuse and set the filename
  inodes[ct].inuse = 1;
  dstrncpy(inodes[ct].filename, filename, dstrlen(filename));
  
  if(LockHandleRelease(lock) == SYNC_SUCCESS){
    dbprintf('s', "dfs (%d): released a lock after free fbv.\n", GetCurrentPid());
  }

  return ct;
}


//-----------------------------------------------------------------
// DfsInodeDelete de-allocates any data blocks used by this inode, 
// including the indirect addressing block if necessary, then mark 
// the inode as no longer in use. Use locks when modifying the 
// "inuse" flag in an inode.Return DFS_FAIL on failure, and 
// DFS_SUCCESS on success.
//-----------------------------------------------------------------

int DfsInodeDelete(uint32 handle) {

  int ct;
  dfs_block tmp;
  int blk_number_array[DFS_BLOCKSIZE / 4];

  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }

  // Set filesize to 0
  inodes[handle].file_size = 0;

  // Free direct block 
  for(ct = 0; ct < 10; ct++){
    if(inodes[handle].direct_table[ct] != -1){
      DfsFreeBlock(inodes[handle].direct_table[ct]);
      inodes[handle].direct_table[ct] = -1;
    }
  }

  // Free indirect block
  if(inodes[handle].indirect_num != -1){
    if(DfsReadBlock(inodes[handle].indirect_num, &tmp) != sb.fsb_size){
      return DFS_FAIL;
    }
    bcopy((char *) (&tmp), (char *) blk_number_array, DFS_BLOCKSIZE);
    for(ct = 0; ct < DFS_BLOCKSIZE / 4; ct++){
      if(blk_number_array[ct] != -1){
	DfsFreeBlock(blk_number_array[ct]);
      }
    }
    // Set the indirect number to -1
    inodes[handle].indirect_num = -1;
  }

  // LOCK here
  
  if(LockHandleAcquire(lock) == SYNC_SUCCESS){
    dbprintf('s', "dfs (%d): got a lock before free fbv.\n", GetCurrentPid());
  }

  // Set the inode as unuse
  inodes[handle].inuse = 0;
  
  // Unlock
  if(LockHandleRelease(lock) == SYNC_SUCCESS){
    dbprintf('s', "dfs (%d): released a lock after free fbv.\n", GetCurrentPid());
  }

  return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsInodeReadBytes reads num_bytes from the file represented by 
// the inode handle, starting at virtual byte start_byte, copying 
// the data to the address pointed to by mem. Return DFS_FAIL on 
// failure, and the number of bytes read on success.
//-----------------------------------------------------------------

int DfsInodeReadBytes(uint32 handle, void *mem, int start_byte, int num_bytes) {

  int start_virtual_block = start_byte / sb.fsb_size;
  int start_virtual_byte = start_byte % sb.fsb_size;
  int fs_blocknum;
  dfs_block tmp;

  int first_block_bytes;
  int rest_of_bytes;

  int num_of_block;
  int bytes_exceed;

  int ct = 0;
  int blk_number_array[DFS_BLOCKSIZE / 4];

  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }
  
  // Check if the inode is inuse
  if(inodes[handle].inuse == 0){
    return DFS_FAIL;
  }

  // Check if the start byte is out of the file
  if(start_byte < 0 || start_byte > inodes[handle].file_size){
    return DFS_FAIL;
  }

  // Check if reach the end of the file
  if(start_byte + num_bytes > inodes[handle].file_size){
    // Only read to the end of the file
    num_bytes = inodes[handle].file_size - start_byte;
  }


  // The start virtual block is within the direct table
  if(start_virtual_block <= 9){

    // Read the fs block as the starting point
    fs_blocknum = inodes[handle].direct_table[start_virtual_block];
    if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
      return DFS_FAIL;
    }
    
    // If num_bytes can be read from a single block
    if(start_virtual_byte + num_bytes <= 1024){
      bcopy((char *) (tmp.data + start_virtual_byte), (char *) mem, num_bytes);
    }
    else{
      // Read the rest of the first block 
      first_block_bytes = 1024 - start_virtual_byte;
      bcopy((char *) (tmp.data + start_virtual_byte), (char *) mem, first_block_bytes);
      mem += first_block_bytes;

      // Determine the rest of bytes and blocks
      rest_of_bytes = num_bytes - first_block_bytes;
      num_of_block = rest_of_bytes / sb.fsb_size;
      bytes_exceed = rest_of_bytes % sb.fsb_size;

      if(start_virtual_block + num_of_block <= 8){
	// if the rest block does not exceed 8
	for(ct = 1; ct <= num_of_block; ct++){
	  fs_blocknum = inodes[handle].direct_table[start_virtual_block + ct];
	  
	  if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
	    return DFS_FAIL;
	  }
	  bcopy((char *) (tmp.data), (char *) mem, sb.fsb_size);
	  mem += sb.fsb_size;
	  
	}
	// Read the last block to cover the rest of the bytes
	fs_blocknum = inodes[handle].direct_table[start_virtual_block + ct];	
	if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
	  return DFS_FAIL;
	}
	bcopy((char *) (tmp.data), (char *) mem, bytes_exceed);
      }
      else{
	// if the rest block exceeds 9
	
	// read all the remaining blocks from the 10 blocks
	for(ct = start_virtual_block + 1; ct < 10; ct++){
	    fs_blocknum = inodes[handle].direct_table[ct];  
	    if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
	      return DFS_FAIL;
	    }
	    bcopy((char *) (tmp.data), (char *) mem, sb.fsb_size);
	    mem += sb.fsb_size;	    
	  }
	
	// read the block from the indirect table
	if(inodes[handle].indirect_num != -1){
	  if(DfsReadBlock(inodes[handle].indirect_num, &tmp) != sb.fsb_size){
	    return DFS_FAIL;
	  }
	  bcopy((char *) (&tmp), (char *) blk_number_array, sb.fsb_size);
	}

	// Determine the rest of bytes and how many blocks read from the indirect block
	rest_of_bytes -= sb.fsb_size * (10 - start_virtual_block - 1);
	num_of_block = rest_of_bytes / sb.fsb_size;
	bytes_exceed = rest_of_bytes % sb.fsb_size;
	
	// Read the data from the indirect block
	for(ct = 0; ct < num_of_block; ct++){
	  fs_blocknum = blk_number_array[ct];  
	  if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
	    return DFS_FAIL;
	  }
	  bcopy((char *) (tmp.data), (char *) mem, sb.fsb_size);
	  mem += sb.fsb_size;	    
	}
	// Read one more block to cover the exceeded bytes
	fs_blocknum = blk_number_array[ct];  
	if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
	  return DFS_FAIL;
	}
	bcopy((char *) (tmp.data), (char *) mem, bytes_exceed);
      }
    }
  }
  else{
    // Start from the indirect block
    // read the block from the indirect table
    if(inodes[handle].indirect_num != -1){
      if(DfsReadBlock(inodes[handle].indirect_num, &tmp) != sb.fsb_size){
	return DFS_FAIL;
      }
      bcopy((char *) (&tmp), (char *) blk_number_array, sb.fsb_size);
    }
    
    // Indirect block start from block 11
    start_virtual_block -= 10;

    // Read the fs block as the starting point
    fs_blocknum = blk_number_array[start_virtual_block];
    if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
      return DFS_FAIL;
    }

    // If num_bytes can be read from a single block
    if(start_virtual_byte + num_bytes <= 1024){
      bcopy((char *) (tmp.data + start_virtual_byte), (char *) mem, num_bytes);
    }
    else{
      // Read the rest of the first block 
      first_block_bytes = 1024 - start_virtual_byte;
      bcopy((char *) (tmp.data + start_virtual_byte), (char *) mem, first_block_bytes);
      mem += first_block_bytes;
      
      // Determine the rest of bytes and blocks
      rest_of_bytes = num_bytes - first_block_bytes;
      num_of_block = rest_of_bytes / sb.fsb_size;
      bytes_exceed = rest_of_bytes % sb.fsb_size;

      // Read the data from the indirect block
      for(ct = 1; ct <= num_of_block; ct++){
	fs_blocknum = blk_number_array[start_virtual_block + ct];  
	if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
	  return DFS_FAIL;
	}
	bcopy((char *) (tmp.data), (char *) mem, sb.fsb_size);
	mem += sb.fsb_size;	    
      }
      // Read one more block to cover the exceeded bytes
      fs_blocknum = blk_number_array[start_virtual_block + ct];  
      if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
	return DFS_FAIL;
      }
      bcopy((char *) (tmp.data), (char *) mem, bytes_exceed);
    }
  }


  return num_bytes;
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
  
  int start_virtual_block = start_byte / sb.fsb_size;
  int start_virtual_byte = start_byte % sb.fsb_size;
  int fs_blocknum;
  dfs_block tmp;

  int first_block_bytes;
  int rest_of_bytes;

  int num_of_block;
  int bytes_exceed;

  int ct = 0;
  int blk_number_array[DFS_BLOCKSIZE / 4];

  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }
  
  // Check if the inode is inuse
  if(inodes[handle].inuse == 0){
    return DFS_FAIL;
  }

  // The start virtual block is within the direct table
  if(start_virtual_block <= 9){

    // Read the fs block as the starting point
    fs_blocknum = inodes[handle].direct_table[start_virtual_block];
    
    // Check if the block has been allocated
    if(fs_blocknum == -1){
      fs_blocknum = DfsAllocateBlock();
      inodes[handle].direct_table[start_virtual_block] = fs_blocknum;
    }

    if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
      return DFS_FAIL;
    }
    
    // If num_bytes can be written to the single block
    if(start_virtual_byte + num_bytes <= 1024){
      bcopy((char *) mem, (char *) (tmp.data + start_virtual_byte), num_bytes);
      if(DfsWriteBlock(fs_blocknum, &tmp) != sb.fsb_size){
	return DFS_FAIL;
      } 
    }
    else{
      // Read the rest of the first block 
      first_block_bytes = 1024 - start_virtual_byte;
      bcopy((char *) mem, (char *) (tmp.data + start_virtual_byte), first_block_bytes);
      if(DfsWriteBlock(fs_blocknum, &tmp) != first_block_bytes){
	return DFS_FAIL;
      }
      mem += first_block_bytes;

      // Determine the rest of bytes and blocks
      rest_of_bytes = num_bytes - first_block_bytes;
      num_of_block = rest_of_bytes / sb.fsb_size;
      bytes_exceed = rest_of_bytes % sb.fsb_size;

      if(start_virtual_block + num_of_block <= 8){
	// if the rest block does not exceed 8
	for(ct = 1; ct <= num_of_block; ct++){
	  fs_blocknum = inodes[handle].direct_table[start_virtual_block + ct];
	  if(fs_blocknum == -1){
	    fs_blocknum = DfsAllocateBlock();
	    inodes[handle].direct_table[start_virtual_block] = fs_blocknum;
	  }
	  bcopy((char *) mem, (char *) (tmp.data), sb.fsb_size);
	  mem += sb.fsb_size;
	  if(DfsWriteBlock(fs_blocknum, &tmp) != sb.fsb_size){
	    return DFS_FAIL;
	  }
	}

	// Read & write the last block to cover the rest of the bytes
	fs_blocknum = inodes[handle].direct_table[start_virtual_block + ct];	
	if(fs_blocknum == -1){
	  fs_blocknum = DfsAllocateBlock();
	  inodes[handle].direct_table[start_virtual_block] = fs_blocknum;
	}
	if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
	  return DFS_FAIL;
	}
	bcopy((char *) mem, (char *) (tmp.data), bytes_exceed);
	if(DfsWriteBlock(fs_blocknum, &tmp) != bytes_exceed){
	  return DFS_FAIL;
	}
      }
      else{
	// if the rest block exceeds 9
	
	// Overwrte all the remaining blocks from the 10 blocks
	for(ct = start_virtual_block + 1; ct < 10; ct++){
	  bcopy((char *) mem, (char *) (tmp.data), sb.fsb_size);
	  mem += sb.fsb_size;
	  
	  fs_blocknum = inodes[handle].direct_table[ct];  
	  if(fs_blocknum == -1){
	    fs_blocknum = DfsAllocateBlock();
	    inodes[handle].direct_table[start_virtual_block] = fs_blocknum;
	  }
	  if(DfsWriteBlock(fs_blocknum, &tmp) != sb.fsb_size){
	    return DFS_FAIL;
	  }
	}
	
	// read the block from the indirect table
	if(inodes[handle].indirect_num != -1){
	  if(DfsReadBlock(inodes[handle].indirect_num, &tmp) != sb.fsb_size){
	    return DFS_FAIL;
	  }
	  bcopy((char *) (&tmp), (char *) blk_number_array, sb.fsb_size);
	}
	else{
	  // Allocate an indirect block
	  inodes[handle].indirect_num = DfsAllocateBlock();
	  for(ct = 0; ct < DFS_BLOCKSIZE / 4; ct++){
	    blk_number_array[ct] = -1;
	  }
	}
	
	// Determine the rest of bytes and how many blocks read from the indirect block
	rest_of_bytes -= sb.fsb_size * (10 - start_virtual_block - 1);
	num_of_block = rest_of_bytes / sb.fsb_size;
	bytes_exceed = rest_of_bytes % sb.fsb_size;

	// Read the data from the indirect block
	for(ct = 0; ct < num_of_block; ct++){
	  fs_blocknum = blk_number_array[ct];
	  if(fs_blocknum == -1){
	    fs_blocknum = DfsAllocateBlock();
	    blk_number_array[ct] = fs_blocknum;
	  }
	  bcopy((char *) mem,(char *) (tmp.data), sb.fsb_size);
	  mem += sb.fsb_size;
	  if(DfsWriteBlock(fs_blocknum, &tmp) != sb.fsb_size){
	    return DFS_FAIL;
	  }
	}
	// Read one more block to cover the exceeded bytes
	fs_blocknum = blk_number_array[ct];  
	if(fs_blocknum == -1){
	  fs_blocknum = DfsAllocateBlock();
	  blk_number_array[ct] = fs_blocknum;
	}
	if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
	  return DFS_FAIL;
	}
	bcopy((char *) mem, (char *) (tmp.data), bytes_exceed);
	if(DfsWriteBlock(fs_blocknum, &tmp) != bytes_exceed){
	  return DFS_FAIL;
	}
      }
    }
  }
  else{
    // Start from the indirect block
    // read the block from the indirect table
    if(inodes[handle].indirect_num != -1){
      if(DfsReadBlock(inodes[handle].indirect_num, &tmp) != sb.fsb_size){
	return DFS_FAIL;
      }
      bcopy((char *) (&tmp), (char *) blk_number_array, sb.fsb_size);
    }
    else{
      inodes[handle].indirect_num = DfsAllocateBlock();
      for(ct = 0; ct < DFS_BLOCKSIZE / 4; ct++){
	blk_number_array[ct] = -1;
      }
    }
    
    // Indirect block start from block 11
    start_virtual_block -= 10;

    // Read the fs block as the starting point
    fs_blocknum = blk_number_array[start_virtual_block];
    if(fs_blocknum == -1){
      fs_blocknum = DfsAllocateBlock();
      blk_number_array[start_virtual_block] = fs_blocknum;
    }
    if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
      return DFS_FAIL;
    }

    // If num_bytes can be read from a single block
    if(start_virtual_byte + num_bytes <= 1024){
      bcopy((char *) mem, (char *) (tmp.data + start_virtual_byte),  num_bytes);
      if(DfsWriteBlock(fs_blocknum, &tmp) != sb.fsb_size){
	return DFS_FAIL;
      } 
    }
    else{
      // Read the rest of the first block 
      first_block_bytes = 1024 - start_virtual_byte;
      bcopy( (char *) mem, (char *) (tmp.data + start_virtual_byte), first_block_bytes);
      mem += first_block_bytes;
      if(DfsWriteBlock(fs_blocknum, &tmp) != sb.fsb_size){
	return DFS_FAIL;
      } 
      // Determine the rest of bytes and blocks
      rest_of_bytes = num_bytes - first_block_bytes;
      num_of_block = rest_of_bytes / sb.fsb_size;
      bytes_exceed = rest_of_bytes % sb.fsb_size;

      // Read the data from the indirect block
      for(ct = 1; ct <= num_of_block; ct++){
	fs_blocknum = blk_number_array[start_virtual_block + ct];  
	if(fs_blocknum == -1){
	  fs_blocknum = DfsAllocateBlock();
	  blk_number_array[start_virtual_block] = fs_blocknum;
	}
	bcopy( (char *) mem, (char *) (tmp.data), sb.fsb_size);
	mem += sb.fsb_size;
	if(DfsWriteBlock(fs_blocknum, &tmp) != sb.fsb_size){
	  return DFS_FAIL;
	}
      }
      // Read one more block to cover the exceeded bytes
      fs_blocknum = blk_number_array[start_virtual_block + ct];  
      if(fs_blocknum == -1){
	fs_blocknum = DfsAllocateBlock();
	blk_number_array[start_virtual_block] = fs_blocknum;
      }
      if(DfsReadBlock(fs_blocknum, &tmp) != sb.fsb_size){
	return DFS_FAIL;
      }
      bcopy( (char *) mem, (char *) (tmp.data), bytes_exceed);
      if(DfsWriteBlock(fs_blocknum, &tmp) != bytes_exceed){
	return DFS_FAIL;
      }
    }
  }


  // Update the file size
  if(start_byte + num_bytes - inodes[handle].file_size > 0){
    inodes[handle].file_size = start_byte + num_bytes;
  }


  return num_bytes;
}


//-----------------------------------------------------------------
// DfsInodeFilesize simply returns the size of an inode's file. 
// This is defined as the maximum virtual byte number that has 
// been written to the inode thus far. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

int DfsInodeFilesize(uint32 handle) {
  // Check if the filesystem opens
  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }
  
  // Check if the inode is inuse
  if(inodes[handle].inuse == 0){
    return DFS_FAIL;
  }
  return inodes[handle].file_size;
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

int DfsInodeAllocateVirtualBlock(uint32 handle, uint32 virtual_blocknum) {
  int ct;
  int fs_blocknum;
  dfs_block tmp;
  int blk_number_array[DFS_BLOCKSIZE / 4];

  // Check if the filesystem opens
  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }
  
  // Check if the inode is inuse
  if(inodes[handle].inuse == 0){
    return DFS_FAIL;
  }

  // Allocate a filesystem block
  fs_blocknum = DfsAllocateBlock();

  // Store the blocknum in the direct translation table
  if(virtual_blocknum <= 9){
    inodes[handle].direct_table[virtual_blocknum] = fs_blocknum;
  }
  else{
    // read the block from the indirect table
    if(inodes[handle].indirect_num != -1){
      if(DfsReadBlock(inodes[handle].indirect_num, &tmp) != sb.fsb_size){
	return DFS_FAIL;
      }
      bcopy((char *) (&tmp), (char *) blk_number_array, sb.fsb_size);
    }
    else{
      inodes[handle].indirect_num = DfsAllocateBlock();
      for(ct = 0; ct < DFS_BLOCKSIZE / 4; ct++){
	blk_number_array[ct] = -1;
      }
    }

    // Update the blk number array
    if(virtual_blocknum - 10 < DFS_BLOCKSIZE / 4){
      blk_number_array[virtual_blocknum - 10] = fs_blocknum;

      // Write the blk number array back
      bcopy( (char *) blk_number_array, (char *) (tmp.data), sb.fsb_size);
      if(DfsWriteBlock(fs_blocknum, &tmp) != sb.fsb_size){
	return DFS_FAIL;
      }
    }
    else{
      return DFS_FAIL;
    }
  }

  return sb.fsb_size;
}



//-----------------------------------------------------------------
// DfsInodeTranslateVirtualToFilesys translates the 
// virtual_blocknum to the corresponding file system block using 
// the inode identified by handle. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

int DfsInodeTranslateVirtualToFilesys(uint32 handle, uint32 virtual_blocknum) {
  
  dfs_block tmp;
  int blk_number_array[DFS_BLOCKSIZE / 4];

  // Check if the filesystem opens
  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }
  
  // Check if the inode is inuse
  if(inodes[handle].inuse == 0){
    return DFS_FAIL;
  }

  // Return the direct table if virtual blocknum less than 10
  if(virtual_blocknum <= 9){
    return inodes[handle].direct_table[virtual_blocknum];
  }

  // read the block from the indirect table
  if(inodes[handle].indirect_num != -1){
    if(DfsReadBlock(inodes[handle].indirect_num, &tmp) != sb.fsb_size){
      return DFS_FAIL;
    }
    bcopy((char *) (&tmp), (char *) blk_number_array, sb.fsb_size);
    return blk_number_array[virtual_blocknum - 10];
  }

  return DFS_FAIL;
}


int DfsInodeReset(uint32 handle){
  
  int ct;
  dfs_block tmp;
  int blk_number_array[DFS_BLOCKSIZE / 4];

  // Check if the filesystem opens
  if(fs_open == 0 || sb.valid == 0){
    return DFS_FAIL;
  }
  
  // Check if the inode is inuse
  if(inodes[handle].inuse == 0){
    return DFS_FAIL;
  }

  // Set filesize to 0
  inodes[handle].file_size = 0;

  // Free direct block 
  for(ct = 0; ct < 10; ct++){
    if(inodes[handle].direct_table[ct] != -1){
      DfsFreeBlock(inodes[handle].direct_table[ct]);
      inodes[handle].direct_table[ct] = -1;
    }
  }

  // Free indirect block
  if(inodes[handle].indirect_num != -1){
    if(DfsReadBlock(inodes[handle].indirect_num, &tmp) != sb.fsb_size){
      return DFS_FAIL;
    }
    bcopy((char *) (&tmp), (char *) blk_number_array, DFS_BLOCKSIZE);
    for(ct = 0; ct < DFS_BLOCKSIZE / 4; ct++){
      if(blk_number_array[ct] != -1){
	DfsFreeBlock(blk_number_array[ct]);
      }
    }
    // Set the indirect number to -1
    inodes[handle].indirect_num = -1;
  }
  

}
