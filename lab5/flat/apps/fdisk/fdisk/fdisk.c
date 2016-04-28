#include "usertraps.h"
#include "misc.h"

#include "fdisk.h"

dfs_superblock sb;
dfs_inode inodes[DFS_INODE_MAX_NUM];
uint32 fbv[DFS_FBV_MAX_NUM_WORDS];

int diskblocksize = 0; // These are global in order to speed things up
int disksize = 0;      // (i.e. fewer traps to OS to get the same number)

int FdiskWriteBlock(uint32 blocknum, dfs_block *b); //You can use your own function. This function 
//calls disk_write_block() to write physical blocks to disk

void main (int argc, char *argv[])
{
  int ct = 0;
  char block[512];
  int num_filesystem_blocks;
	// STUDENT: put your code here. Follow the guidelines below. They are just the main steps. 
	// You need to think of the finer details. You can use bzero() to zero out bytes in memory

  //bzero(0, DFS_MAX_FILESYSTEM_SIZE); // ????

  //Initializations and argc check
  for(ct = 0; ct < DFS_BLOCKSIZE; ct++)
    {
      block[ct] = 0;
    }

  // Need to invalidate filesystem before writing to it to make sure that the OS
  // doesn't wipe out what we do here with the old version in memory
  // You can use dfs_invalidate(); but it will be implemented in Problem 2. You can just do 
  // sb.valid = 0
  sb.valid = 0;
  disksize = DFS_MAX_FILESYSTEM_SIZE; // 16MB
  diskblocksize = 512;
  num_filesystem_blocks = disksize / DFS_BLOCKSIZE;

  sb.fsb_size = DFS_BLOCKSIZE; // 1024
  sb.fsb_num = num_filesystem_blocks; // 16384
  sb.inode_start_block = FDISK_INODE_BLOCK_START; // 1
  sb.inode_num_inArray = DFS_INODE_MAX_NUM; // 192
  sb.fbv_start_block = FDISK_FBV_BLOCK_START; // 19
  // Make sure the disk exists before doing anything else
  if(disk_create() == DFS_FAIL)
    {
      Printf("fdisk (%d): Fail to create physical disk.\n", getpid());
    }

  // Write all inodes as not in use and empty (all zeros)
  for(ct = 0; ct < DFS_INODE_MAX_NUM; ct++)
    {
      inodes[ct].inuse = 0;
      inodes[ct].file_size = 0;
      inodes[ct].indirect_num = 0; //not allocated yet
    }
  // Next, setup free block vector (fbv) and write free block vector to the disk
  // set all the currently used to 1, and not used to 0, first 19 blocks are occupied
  fbv[0] = fbv[0] | 0xffffe000;
  for(ct = 1; ct < DFS_FBV_MAX_NUM_WORDS; ct++)
    {
      fbv[ct] = 0;
    }
  
  // Finally, setup superblock as valid filesystem and write superblock and boot record to disk: 
  // boot record is all zeros in the first physical block, and superblock structure goes into the second physical block
  sb.valid = 1;
  // write boot record into p block 0
  if(disk_write_block(0, block) == DFS_FAIL)
    {
      Printf("fdisk (%d): Fail to write boot record into physical disk.\n", getpid());
    }
  // write super block into p block 1
  /* block = (char *) (&sb); // ??? */
  if(disk_write_block(1, (char *) (&sb)) == DFS_FAIL)
    {
      Printf("fdisk (%d): Fail to write superblock into physical disk.\n", getpid());
    }
  if(disk_write_block(2, (char *) (inodes)) == DFS_FAIL)
    {
      Printf("fdisk (%d): Fail to write inodes into physical disk.\n", getpid());
    }
  if(disk_write_block(38, (char *) (fbv)) == DFS_FAIL)
    {
      Printf("fdisk (%d): Fail to write fbv into physical disk.\n", getpid());
    }



  Printf("fdisk (%d): Formatted DFS disk for %d bytes.\n", getpid(), disksize);
  
}

int FdiskWriteBlock(uint32 blocknum, dfs_block *b) {
  // STUDENT: put your code here
  return 0;
}
