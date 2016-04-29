#ifndef __FDISK_H__
#define __FDISK_H__

typedef unsigned int uint32;

#include "dfs_shared.h" // This gets us structures and #define's from main filesystem driver

#define FDISK_INODE_BLOCK_START 1 // Starts after super block (which is in file system block 0, physical block 1)
#define FDISK_INODE_NUM_BLOCKS 18 // Number of file system blocks to use for inodes
// ~~~~~ previously 16 for 128 bytes 128 inodes
// ~~~~~ 18 for 192 96bytes inodes
#define FDISK_NUM_INODES 192 //STUDENT: define this
#define FDISK_FBV_BLOCK_START 19//STUDENT: define this
#define FDISK_BOOT_FILESYSTEM_BLOCKNUM 0 // Where the boot record and superblock reside in the filesystem

#ifndef NULL
#define NULL (void *)0x0
#endif

//STUDENT: define additional parameters here, if any
#define DFS_INODE_MAX_NUM 192
#define DFS_FBV_MAX_NUM_WORDS 512 // 2 blocks, 2048 bytes, 2048 / 4 = 512
#define P_BLOCKSIZE 512 // physical block size
typedef struct p_block {
  char data[P_BLOCKSIZE];
} p_block;


#endif
