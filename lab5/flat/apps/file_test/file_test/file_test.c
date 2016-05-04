#include "usertraps.h"
#include "misc.h"

#define STR_LEN 12
#define FILE_FAIL -1
#define FILE_SUCCESS 1

void main (int argc, char *argv[])
{
  int handle;
  int ct;
  char * filename = "abc";
  char * h = "hello world\n";
  char buffer[STR_LEN];

  Printf("Create a file: %s\n", filename);
  handle = file_open(filename, "w");
  if(handle == FILE_FAIL){
    Printf("Failed to create a file\n");
    Exit();
  }

  Printf("Write hello world to the file\n");
  ct = file_write(handle, h, STR_LEN);
  if(ct != STR_LEN){
    Printf("Write %d bytes to the file\n", STR_LEN);
    Exit();
  }

  Printf("Close the file\n");
  if(file_close(handle) != FILE_SUCCESS){
    Printf("Falied to close the file\n");
  }


  Printf("Open the same file for reading\n");
  handle = file_open(filename, "r");
  if(handle == FILE_FAIL){
    Printf("Failed to open file for reading\n");
    Exit();
  }

  Printf("Read data from the file\n");
  ct = file_read(handle, buffer, STR_LEN);
  if(ct != STR_LEN){
    Printf("Read %d bytes from the file\n", STR_LEN);
    Exit();
  }

  return;
}
