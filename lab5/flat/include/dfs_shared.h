#ifndef __DFS_SHARED__
#define __DFS_SHARED__

typedef struct dfs_superblock {
  // STUDENT: put superblock internals here
  int valid; //valid indicator for the file system
  int fsb_size; //file system block size
  int fsb_num; //total number of file system blocks
  int inode_start_block; //the starting file system block number for the array of inodes
  int inode_num_inArray; //number of inodes in the inodes array
  int fbv_start_block; //the starting file system block number for the free block vector
} dfs_superblock;

#define DFS_BLOCKSIZE 1024  // Must be an integer multiple of the disk blocksize

typedef struct dfs_block {
  char data[DFS_BLOCKSIZE];
} dfs_block;

typedef struct dfs_inode {
  // STUDENT: put inode structure internals here
  // IMPORTANT: sizeof(dfs_inode) MUST return 96 in order to fit in enough
  // inodes in the filesystem (and to make your life easier).  To do this, 
  // adjust the maximumm length of the filename until the size of the overall inode 
  // is 96 bytes.
  int inuse; //in use indicator to tell if an inode is free or in use
  int file_size; // the maximum byte that has been written to this file
  int indirect_num; //a block number of a file system block on the disk which holds a table of indirect address translations for the virtual blocks beyond the first 10
  int direct_table[10]; // table of direct address translations for the first 10 virtual blocks
  char filename[44]; //filename string, max of 44 = 96-52
  
} dfs_inode;

#define DFS_MAX_FILESYSTEM_SIZE 0x1000000  // 16MB

#define DFS_FAIL -1
#define DFS_SUCCESS 1



#endif
