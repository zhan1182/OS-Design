//
//	memory.c
//
//	Routines for dealing with memory management.

//static char rcsid[] = "$Id: memory.c,v 1.1 2000/09/20 01:50:19 elm Exp elm $";

#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "memory.h"
#include "queue.h"

// num_pages = size_of_memory / size_of_one_page
// MEM_MAX_PAGES = (MEM_MAX_PHYS_MEM / MEM_PAGESIZE)
static uint32 freemap[MEM_MAX_PAGES >> 5];
static uint32 pagestart;
static int nfreepages;
static int freemapmax;


//----------------------------------------------------------------------
//
//	This silliness is required because the compiler believes that
//	it can invert a number by subtracting it from zero and subtracting
//	an additional 1.  This works unless you try to negate 0x80000000,
//	which causes an overflow when subtracted from 0.  Simply
//	trying to do an XOR with 0xffffffff results in the same code
//	being emitted.
//
//----------------------------------------------------------------------
static int negativeone = 0xFFFFFFFF;
static inline uint32 invert (uint32 n) {
  return (n ^ negativeone);
}

//----------------------------------------------------------------------
//
//	MemoryGetSize
//
//	Return the total size of memory in the simulator.  This is
//	available by reading a special location.
//
//----------------------------------------------------------------------
int MemoryGetSize() {
  return (*((int *)DLX_MEMSIZE_ADDRESS));
}


//----------------------------------------------------------------------
//
//	MemoryInitModule
//
//	Initialize the memory module of the operating system.
//      Basically just need to setup the freemap for pages, and mark
//      the ones in use by the operating system as "VALID", and mark
//      all the rest as not in use.
//
//----------------------------------------------------------------------
void MemoryModuleInit() {
  int ct;
  uint32 os_page_number = lastosaddress >> MEM_L1FIELD_FIRST_BITNUM; // Divide by 4KB
  uint32 os_page_free_map_index = os_page_number >> 5; // Divide by 32
  uint32 os_page_free_map_inuse_bit = os_page_number & 0x1f; // >> 0001 1111 == % 32

  // Init every bit of free map to 1
  for(ct = 0; ct < (MEM_MAX_PAGES >> 5); ct++){    
    freemap[ct] = 0xffffffff;
  }

  for(ct = 0; ct < os_page_free_map_index; ct++){    
    freemap[ct] = 0;
  }

  freemap[ct] = freemap[ct] >> os_page_free_map_inuse_bit;
 

  return;
}


//----------------------------------------------------------------------
//
// MemoryTranslateUserToSystem
//
//	Translate a user address (in the process referenced by pcb)
//	into an OS (physical) address.  Return the physical address.
//
//----------------------------------------------------------------------
uint32 MemoryTranslateUserToSystem (PCB *pcb, uint32 addr) {
  uint32 page_offset = addr & MEM_ADDRESS_OFFSET_MASK;
  uint32 page_number = addr >> MEM_L1FIELD_FIRST_BITNUM;

  uint32 pte_value = pcb->pagetable[page_number];

  if((pte_value & MEM_PTE_VALID) == 0){
    // Set the PROCESS_STACK_FAULT register to the addr
    pcb->currentSavedFrame[PROCESS_STACK_FAULT] = addr;

    if (MemoryPageFaultHandler(pcb) == MEM_FAIL ) {
      // Seg fault, the process has been killed
      return 0;
    }
  }

  // Return the physical addr
  return (pte_value & MEM_PTE_MASK) | page_offset;
}


//----------------------------------------------------------------------
//
//	MemoryMoveBetweenSpaces
//
//	Copy data between user and system spaces.  This is done page by
//	page by:
//	* Translating the user address into system space.
//	* Copying all of the data in that page
//	* Repeating until all of the data is copied.
//	A positive direction means the copy goes from system to user
//	space; negative direction means the copy goes from user to system
//	space.
//
//	This routine returns the number of bytes copied.  Note that this
//	may be less than the number requested if there were unmapped pages
//	in the user range.  If this happens, the copy stops at the
//	first unmapped address.
//
//----------------------------------------------------------------------
int MemoryMoveBetweenSpaces (PCB *pcb, unsigned char *system, unsigned char *user, int n, int dir) {
  unsigned char *curUser;         // Holds current physical address representing user-space virtual address
  int		bytesCopied = 0;  // Running counter
  int		bytesToCopy;      // Used to compute number of bytes left in page to be copied

  while (n > 0) {
    // Translate current user page to system address.  If this fails, return
    // the number of bytes copied so far.
    curUser = (unsigned char *)MemoryTranslateUserToSystem (pcb, (uint32)user);

    // If we could not translate address, exit now
    if (curUser == (unsigned char *)0) break;

    // Calculate the number of bytes to copy this time.  If we have more bytes
    // to copy than there are left in the current page, we'll have to just copy to the
    // end of the page and then go through the loop again with the next page.
    // In other words, "bytesToCopy" is the minimum of the bytes left on this page 
    // and the total number of bytes left to copy ("n").

    // First, compute number of bytes left in this page.  This is just
    // the total size of a page minus the current offset part of the physical
    // address.  MEM_PAGESIZE should be the size (in bytes) of 1 page of memory.
    // MEM_ADDRESS_OFFSET_MASK should be the bit mask required to get just the
    // "offset" portion of an address.
    bytesToCopy = MEM_PAGESIZE - ((uint32)curUser & MEM_ADDRESS_OFFSET_MASK);
    
    // Now find minimum of bytes in this page vs. total bytes left to copy
    if (bytesToCopy > n) {
      bytesToCopy = n;
    }

    // Perform the copy.
    if (dir >= 0) {
      bcopy (system, curUser, bytesToCopy);
    } else {
      bcopy (curUser, system, bytesToCopy);
    }

    // Keep track of bytes copied and adjust addresses appropriately.
    n -= bytesToCopy;           // Total number of bytes left to copy
    bytesCopied += bytesToCopy; // Total number of bytes copied thus far
    system += bytesToCopy;      // Current address in system space to copy next bytes from/into
    user += bytesToCopy;        // Current virtual address in user space to copy next bytes from/into
  }
  return (bytesCopied);
}

//----------------------------------------------------------------------
//
//	These two routines copy data between user and system spaces.
//	They call a common routine to do the copying; the only difference
//	between the calls is the actual call to do the copying.  Everything
//	else is identical.
//
//----------------------------------------------------------------------
int MemoryCopySystemToUser (PCB *pcb, unsigned char *from,unsigned char *to, int n) {
  return (MemoryMoveBetweenSpaces (pcb, from, to, n, 1));
}

int MemoryCopyUserToSystem (PCB *pcb, unsigned char *from,unsigned char *to, int n) {
  return (MemoryMoveBetweenSpaces (pcb, to, from, n, -1));
}

//---------------------------------------------------------------------
// MemoryPageFaultHandler is called in traps.c whenever a page fault 
// (better known as a "seg fault" occurs.  If the address that was
// being accessed is on the stack, we need to allocate a new page 
// for the stack.  If it is not on the stack, then this is a legitimate
// seg fault and we should kill the process.  Returns MEM_SUCCESS
// on success, and kills the current process on failure.  Note that
// fault_address is the beginning of the page of the virtual address that 
// caused the page fault, i.e. it is the vaddr with the offset zero-ed
// out.
//
// Note: The existing code is incomplete and only for reference. 
// Feel free to edit.
//---------------------------------------------------------------------
int MemoryPageFaultHandler(PCB *pcb) {

  uint32 ppagenum;

  uint32 addr = pcb->currentSavedFrame[PROCESS_STACK_FAULT];

  uint32 vpagenum = addr >> MEM_L1FIELD_FIRST_BITNUM;
  uint32 stackpagenum = pcb->currentSavedFrame[PROCESS_STACK_USER_STACKPOINTER] >> MEM_L1FIELD_FIRST_BITNUM;

  /* printf("addr = %x\nsp = %x\n", addr, pcb->currentSavedFrame[PROCESS_STACK_USER_STACKPOINTER]); */

  // segfault if the faulting address is not part of the stack
  if (vpagenum < stackpagenum) {
    dbprintf('m', "addr = %x\nsp = %x\n", addr, pcb->currentSavedFrame[PROCESS_STACK_USER_STACKPOINTER]);
    printf("FATAL ERROR (%d): segmentation fault at page address %x\n", findpid(pcb), addr);
    ProcessKill();
    return MEM_FAIL;
  }

  ppagenum = MemoryAllocPage();
  pcb->pagetable[vpagenum] = MemorySetupPte(ppagenum);
  dbprintf('m', "Returning from page fault handler\n");

  printf("allocate new page for stack grows\n");

  return MEM_SUCCESS;
}


//---------------------------------------------------------------------
// You may need to implement the following functions and access them from process.c
// Feel free to edit/remove them
//---------------------------------------------------------------------

int MemoryAllocPage(void) {

  int ct;
  int bit_index = 31;
  uint32 freemap_entry_mask = 0x80000000;
  
  int physical_page_number;

  // Find a chunk of pages that has a least one free page
  for(ct = 0; ct < MEM_MAX_PAGES; ct++){
    if(freemap[ct] != 0){
      break;
    }
  }
  
  // Find bit index of the free page 
  while( (freemap[ct] & freemap_entry_mask) == 0){
    freemap_entry_mask = freemap_entry_mask >> 1;
    bit_index -= 1;
  }
  
  // Mark the page number in the freemap as inuse
  /* printf("bit_index = %d\n", bit_index); */
  /* printf("mask = %x\n", 0x1 << bit_index); */
  /* printf("inverted mask = %x\n", 0xffffffff - (0x1 << bit_index)); */
  /* freemap[ct] = freemap[ct] & ~(0x1 << bit_index); */


  /* printf("freemap[%d] = %d\n", ct, freemap[ct]); */


  freemap[ct] = freemap[ct] & invert(0x1 << bit_index);

  // Return the physical page number
  physical_page_number = ct * 32 + (32 - bit_index);

  /* printf("freemap[%d] = %d, bit_index = %d, physical page number = %d\n", ct, freemap[ct], bit_index, physical_page_number); */

  return physical_page_number;
}

// page here is the physical page number
uint32 MemorySetupPte (uint32 page) {
  return (page << MEM_L1FIELD_FIRST_BITNUM) | 0x1;
}


// page here is the physical page number
void MemoryFreePage(uint32 page) {
  
  uint32 freemap_index = page >> 5;
  uint32 freemap_bit_index = page & 0x1f;

  // Set the freemap entry to 1 --> freed!
  freemap[freemap_index] = freemap[freemap_index] | (0x1 << freemap_bit_index);

  return;
}

void *malloc(PCB * pcb, int memsize){
  
  uint32 restStart = 0; //rest starting virtual address
  uint32 start = 0; //starting virtual address
  Link *l = NULL;
  int length = 0;
  int ct = 0;
  Link *ptr = NULL; // iterat pointer
  Link *store = NULL; //store the node that needs to be removed because best fit found within it
  int restSize = 0; // the size used left from a larger block when applicable


  if(memsize <= 0) {return NULL;} // if memsize less than or equal to 0, fail
  
  if (memsize % 4 != 0)
    {
      memsize = 4 - memsize % 4 + memsize;
    }

  if(memsize > MEM_PAGESIZE) {return NULL;} // if memsize larger than total heap size, fail


  // set up the store link
  if ((store = AQueueAllocLink(pcb)) == NULL) 
    {
      printf("FATAL ERROR (MALLOC): could not get store link for heap in malloc!\n");
      exitsim();
    }
  store->size = MEM_PAGESIZE;

  // scan through the queue for best fit
  length = AQueueLength(&(pcb->heapQueue));
  ptr = (pcb->heapQueue).first;
  //l = ptr;
  
  for(ct = 0; ct < length; ct++)
    {
      if(ptr->isOccupied == 0 && ptr->size == memsize)
	{
	  // perfect best fit found
	  ptr->size = memsize;
	  ptr->isOccupied = 1; //this block will be occupied
	  l = ptr; // assign the pointer
	  printf("Found perfect!!!!\n");
	  return (void *)(l->start); // ?????
	}
      else if(ptr->isOccupied == 0 && ptr->size > (memsize))
	{
	  // candidate for best fit
	  if(ptr->size <= store->size)
	    {
	      //l = ; // store this pointer for future comparison or use
	      store = ptr; // remove this node later
	      store->size = ptr->size;
	      store->start = ptr->start;
	      //printf("store size is %d.\n", store->size);
	    }
	}
      
      ptr = AQueueNext(ptr);
   
    }
  //printf("store size is %d, start address is %d.\n", store->size, (int)store->start);
  //ptr = AQueuePrev(ptr);
  // check if best fit not found
  if((store->isOccupied == 1) || (store->size < memsize))
    {
      // no best fit found
      return NULL;
    }

  // best fit found
  start = store->start;
  restSize = store->size - memsize; 
  restStart = store->start + memsize; //????

  //printf("ct is %d, the start address is %08x, block size is %d, large size is %d, restStart is %08x.\n", ct, start, memsize, store->size, restStart);

  //after store necessary values, change the larger block from the queue
  /* if (AQueueRemove(&store) != QUEUE_SUCCESS) */
  /*   { */
  /*     printf("FATAL ERROR (MALLOC): could not remove the larger block from the heap queue!\n"); */
  /*     exitsim(); */
  /*   } */

    // set up the target link
  if ((l = AQueueAllocLink(pcb)) == NULL)
    {
      printf("FATAL ERROR (MALLOC): could not get rest link for heap in malloc!\n");
      exitsim();
    }

  l->size = memsize;
  l->isOccupied = 1;
  l->start = start;

  store->size = restSize;
  store->start = restStart;
  store->isOccupied = 0;
  
  // insert these two nodes back into the heap queue

  /* if (AQueueInsertLast(&(pcb->heapQueue), store) != QUEUE_SUCCESS) */
  /*   { */
  /*     printf("FATAL ERROR (MALLOC): could not insert the rest link into heap in malloc!\n"); */
  /*     exitsim(); */
  /*   } */

  if (AQueueInsertBefore(&(pcb->heapQueue), store, l) != QUEUE_SUCCESS)
    {
      printf("FATAL ERROR (MALLOC): could not insert link into heap in malloc!\n");
      exitsim();
    }

  //printf("start address is %d, size is %d, isOccupied = %d.\n", (int)l->start, l->size, l->isOccupied);
  //printf("l is %d.\n", l);

  // every thing is done!
  printf("MALLOC: Created a heap block of size %d bytes: virtual address %d, physical address %08x.\n", l->size, (int)l->start, (uint32) MemoryTranslateUserToSystem(pcb, l->start));

  return  (void *)(l->start);
}


int mfree(PCB * pcb, void *ptr){
  Link *l = NULL;
  Link *next = NULL;
  Link *prev = NULL;
  Link *merge = NULL;
  int byte = 0; // used for store the total freed bytes
  uint32 start = 0;


  start = (uint32) ptr;

  if(ptr == NULL || (start < (uint32)(4 << MEM_L1FIELD_FIRST_BITNUM)) || (start >= ((uint32)(4 << MEM_L1FIELD_FIRST_BITNUM) + MEM_PAGESIZE)))
    {
      // when ptr is NULL, or ptr does not belong to the heap space, return -1
      return -1;
    }

  l = (pcb->heapQueue).first;
  while(l != NULL)
    {
      if(start == l->start)
	{
	  //found the belonged link node
	  break;
	}
      l = l->next;
    }

  //printf("l->size = %d.\n", l->size);
  l->isOccupied = 0; // free the memory block
  next = l->next;
  prev = l->prev;
  byte = l->size;

  if (prev != NULL && next != NULL && prev->isOccupied == 0 && next->isOccupied == 0)
    {
      //the adjecent block is also unused, merge here
      if ((merge = AQueueAllocLink(pcb)) == NULL) 
	{
	  printf("FATAL ERROR (MFREE): could not get merge for heap. Prev and next are unused!\n");
	  exitsim();
	}
      merge->size = l->size + next->size + prev->size;
      merge->start = prev->start;
      merge->isOccupied = 0;
      byte = merge->size;
      if (AQueueRemove(&next) != QUEUE_SUCCESS)
	{
	  printf("FATAL ERROR (MFREE): could not remove the next from the heap queue. Prev and next are unused!\n");
	  exitsim();
	}
      if (AQueueRemove(&prev) != QUEUE_SUCCESS)
	{
	  printf("FATAL ERROR (MFREE): could not remove the prev from the heap queue. Prev and next are unused!\n");
	  exitsim();
	}
      if (AQueueRemove(&l) != QUEUE_SUCCESS)
	{
	  printf("FATAL ERROR (MFREE): could not remove the target link from the heap queue. Prev and next are unused!\n");
	  exitsim();
	}
      if (AQueueInsertLast(&(pcb->heapQueue), merge) != QUEUE_SUCCESS)
	{
	  printf("FATAL ERROR (MFREE): could not insert the merge into heap in malloc. Prev and next are unused!\n");
	  exitsim();
	}
    }
  else if(prev != NULL && prev->isOccupied == 0)
    {
      if ((merge = AQueueAllocLink(pcb)) == NULL) 
	{
	  printf("FATAL ERROR (MFREE): could not get merge for heap in mfree. Prev is unused!\n");
	  exitsim();
	}
      merge->size = l->size + prev->size;
      merge->start = prev->start;
      merge->isOccupied = 0;
      byte = merge->size;
      if (AQueueRemove(&prev) != QUEUE_SUCCESS)
	{
	  printf("FATAL ERROR (MFREE): could not remove the prev from the heap queue. Prev is unused!\n");
	  exitsim();
	}
      if (AQueueRemove(&l) != QUEUE_SUCCESS)
	{
	  printf("FATAL ERROR (MFREE): could not remove the target link from the heap queue. Prev is unused!\n");
	  exitsim();
	}
      if (AQueueInsertLast(&(pcb->heapQueue), merge) != QUEUE_SUCCESS)
	{
	  printf("FATAL ERROR (MFREE): could not insert the merge into heap in malloc. Prev is unused!\n");
	  exitsim();
	}
    }
  else if(next != NULL && next->isOccupied == 0)
    {
      if ((merge = AQueueAllocLink(pcb)) == NULL) 
	{
	  printf("FATAL ERROR (MFREE): could not get merge for heap in mfree. Next is unused!\n");
	  exitsim();
	}
      merge->size = l->size + next->size;
      merge->start = l->start;
      merge->isOccupied = 0;
      byte = merge->size;
      if (AQueueRemove(&next) != QUEUE_SUCCESS)
	{
	  printf("FATAL ERROR (MFREE): could not remove the prev from the heap queue. Next is unused!\n");
	  exitsim();
	}
      if (AQueueRemove(&l) != QUEUE_SUCCESS)
	{
	  printf("FATAL ERROR (MFREE): could not remove the target link from the heap queue. Next is unused!\n");
	  exitsim();
	}
      if (AQueueInsertLast(&(pcb->heapQueue), merge) != QUEUE_SUCCESS)
	{
	  printf("FATAL ERROR (MFREE): could not insert the merge into heap in malloc. Next is unused!\n");
	  exitsim();
	}
    }
  else
    {
      // no adjacent block is unused, no remove and insert necessary
      printf("MFREE & Only l: Freeing heap block of size %d bytes: virtual address %06x, physical address %08x.\n", byte, l->start, (int)MemoryTranslateUserToSystem(pcb, l->start));
      return byte;
    }
  printf("MFREE & Merge: Freeing heap block of size %d bytes: virtual address %06x, physical address %08x.\n", byte, merge->start, (int)MemoryTranslateUserToSystem(pcb, merge->start));
  return byte;
}



