#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "disk.h"
#include "dfs.h"

void RunOSTests() {
  // STUDENT: run any os-level tests here
  
  int handle = 0;
  char *file1 = "test1";
  char mem[20000];
  char mem_read[20000];
  int start_byte = 0;
  int num_bytes = 0;
  int ct = 0;
  char stuff = 'a';
  int ct_err = 0;

  // first, initialize the file system
  if(DfsOpenFileSystem() == DFS_FAIL)
    {
      dbprintf('s', "ostests: failed to open file system.\n");
    }

  //make sure such file does not exist, if yes, error
  if(DfsInodeFilenameExists(file1) != DFS_FAIL)
    {
      dbprintf('s', "Error: found unexisted file.\n");
    }

  // now create an inode for such file
  handle = DfsInodeOpen(file1);
  // now check if such file exists
  if(DfsInodeFilenameExists(file1) != handle)
    {
      dbprintf('s', "Error: file inode did not create.\n");
    }

  // start to write bytes into memory
  // first write something into the mem
  
  for(ct = 0; ct < 512; ct++)
    {
      mem[ct] = stuff;
      stuff = 'z' ? 'a' : (stuff + 1);
    }

  // now that mem is stuffed, we write part of it into file
  num_bytes = 350; // write 350 bytes
  start_byte = 0; // start from 25th byte

  //////////////////////// ERROR HERE ///////////////////////
  if(DfsInodeWriteBytes(handle, mem, start_byte, num_bytes) != num_bytes)
    {
      dbprintf('s', "dfs write: write 350 bytes failed.\n");
    }

  // now check if previous 25 bytes are empty and all stuff are written, read from 0, to 512
  if(DfsInodeReadBytes(handle, mem_read, 0, 512) != num_bytes)
    {
      dbprintf('s', "dfs read: read 350 bytes failed.\n");
    }
  
  // now test if the read data is identicle to the written
  // first test if first 25 are 0
  for(ct = 0; ct < 25; ct++)
    {
      if(mem_read[ct] != 0)
	{
	  ct_err++;
	}
    }
  /* dbprintf('s', "Among the first 25 data, %d are not correct.\n", ct_err); */
  printf("Among the first 25 data, %d are not correct.\n", ct_err);
  // then test if next 350 bytes are what were written
  ct_err = 0;
  for(; ct < 375; ct++)
    {
      if(mem_read[ct] != mem[ct-25])
	{
	  ct_err++;
	}
    }
  /* dbprintf('s', "Among the next 350 bytes, %d are not correct.\n", ct_err); */
  printf("Among the next 350 bytes, %d are not correct.\n", ct_err);
  // last check if the remaining 138 bytes are null
  ct_err = 0;
  for(; ct < 512; ct++)
    {
      if(mem_read[ct] != 0)
	{
	  ct_err++;
	}
    }
  /* dbprintf('s', "Among the last 138 bytes, %d are not correct.\n", ct_err); */
  printf("Among the last 138 bytes, %d are not correct.\n", ct_err);

  // next test if we can over write into the file
  // change the mem
  stuff = '1';
  for(ct = 0; ct < 1024; ct++)
    {
      mem[ct] = stuff;
      stuff = '9' ? '1' : (stuff + 1);
    }
  // now write this into the previous
  if(DfsInodeWriteBytes(handle, (void *)mem, 0, 1024) != num_bytes)
    {
      dbprintf('s', "dfs write: write 1024 bytes failed.\n");
    }
  
  // now check if previous 25 bytes are empty and all stuff are written, read from 0, to 512
  if(DfsInodeReadBytes(handle, mem_read, 0, 1024) != num_bytes)
    {
      dbprintf('s', "dfs read: read 1024 bytes failed.\n");
    }

  ct_err = 0;
  for(ct = 0; ct < 1024; ct++)
    {
      if(mem_read[ct] != mem[ct])
	{
	  ct_err++;
	}
    }
  /* dbprintf('s', "Among the first dfs block, %d are not correct.\n", ct_err); */
  printf("Among the first dfs block, %d are not correct.\n", ct_err);

  // we need to check if those data still there after closing file system
  // now we test how it works when write more than one block
  stuff = 'a';
  for(ct = 0; ct < 1025; ct++)
    {
      mem[ct] = stuff;
      stuff = 'z' ? 'a' : (stuff + 1);
    }
  
// now write this into the previous
  if(DfsInodeWriteBytes(handle, (void *)mem, 1024, 1025) != num_bytes)
    {
      dbprintf('s', "dfs write: write 1024 bytes failed.\n");
    }
  
  // now check if previous 25 bytes are empty and all stuff are written, read from 0, to 512
  if(DfsInodeReadBytes(handle, mem_read, 1024, 1025) != num_bytes)
    {
      dbprintf('s', "dfs read: read 1024 bytes failed.\n");
    }

  ct_err = 0;
  for(ct = 0; ct < 1025; ct++)
    {
      if(mem_read[ct] != mem[ct])
	{
	  ct_err++;
	}
    }
  /* dbprintf('s', "Among the second dfs block, %d are not correct.\n", ct_err); */
  printf("Among the second dfs block, %d are not correct.\n", ct_err);

  // check if in the third block, all are null except the first byte
  if(DfsInodeReadBytes(handle, mem_read, 2048, 1024) != 1024)
    {
      dbprintf('s', "dfs read: third 1024 bytes failed.\n");
    }
  if(mem_read[0] == 0)
    {
      dbprintf('s', "Error: previous write was not right.\n");
    }

  for(ct = 1; ct < 1024; ct++)
    {
      if(mem_read[ct] != 0)
	{
	  dbprintf('s', "Error: a byte is not written while it is not supposed to.\n");
	  break;
	}
    }

  // now we write into enough bytes to create indirect table
  stuff = 'A';
  for(ct = 0; ct < 13313; ct++)
    {
      mem[ct] = stuff;
      stuff = 'Z' ? 'A' : (stuff + 1);
    }
  // write till 5th indirect block, and one more extra byte in 6th
  if(DfsInodeWriteBytes(handle, (void *)mem, 2048, 13313) != 13313)
    {
      dbprintf('s', "dfs write: write 13313 bytes failed.\n");
    }

  // we should check this by using block print

  // close inode
  if(DfsInodeDelete(handle) != DFS_SUCCESS)
    {
      dbprintf('s', "Error: inode delete fail.\n");
    }

  // close the file system
  if(DfsCloseFileSystem() != DFS_SUCCESS)
    {
      dbprintf('s', "Error: file system close fail.\n");
    }


  printf("======================== ostests completed ===============================.\n");
}

