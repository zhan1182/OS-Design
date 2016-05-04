#ifndef __DFS_H__
#define __DFS_H__

#include "dfs_shared.h"

#define DFS_INODE_MAX_NUM 192
#define DFS_FBV_MAX_NUM_WORDS 512 // 2 blocks, 2048 bytes, 2048 / 4 = 512

void DfsModuleInit();
void DfsInvalidate();
int DfsOpenFileSystem();
int DfsCloseFileSystem();
int DfsAllocateBlock();
int DfsFreeBlock(uint32 blocknum);
int DfsReadBlock(uint32 blocknum, dfs_block *b);
int DfsWriteBlock(uint32 blocknum, dfs_block *b);
int DfsInodeFilenameExists(char *filename);
int DfsInodeOpen(char *filename);
int DfsInodeDelete(uint32 handle);
int DfsInodeReadBytes(uint32 handle, void *mem, int start_byte, int num_bytes);
int DfsInodeWriteBytes(uint32 handle, void *mem, int start_byte, int num_bytes);
int DfsInodeFilesize(uint32 handle);
int DfsInodeAllocateVirtualBlock(uint32 handle, uint32 virtual_blocknum);
int DfsInodeTranslateVirtualToFilesys(uint32 handle, uint32 virtual_blocknum);

// New added function
int DfsInodeReset(uint32 handle);

#endif
