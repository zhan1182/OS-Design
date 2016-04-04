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

// Define the arry of L2 page table statically here
/* static uint32 l2_page_table_array[MEM_L2_PAGE_TABLE_ARRAY_SIZE][MEM_L2_PAGE_TABLE_SIZE]; */
/* static uint32 l2_page_table_array_occupied[MEM_L2_PAGE_TABLE_ARRAY_SIZE]; */

static L2_PAGE_TABLE l2_page_table_pool[MEM_L2_PAGE_TABLE_ARRAY_SIZE];


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
  int ind;
  uint32 os_page_number = lastosaddress >> MEM_L2FIELD_FIRST_BITNUM; // Divide by 4KB
  uint32 os_page_free_map_index = os_page_number >> 5; // Divide by 32
  uint32 os_page_free_map_inuse_bit = os_page_number & 0x1f; // >> 0001 1111 == % 32

  /* printf("lastosaddress = %d\n", lastosaddress); */

  // Init every bit of free map to 1
  for(ct = 0; ct < (MEM_MAX_PAGES >> 5); ct++){    
    freemap[ct] = 0xffffffff;
  }

  // Occupy free map for the OS
  for(ct = 0; ct < os_page_free_map_index; ct++){    
    freemap[ct] = 0;
  }
  freemap[ct] = freemap[ct] >> os_page_free_map_inuse_bit;
 
  /* // Init the L2 page table array */
  /* for(ct = 0; ct < MEM_L2_PAGE_TABLE_ARRAY_SIZE; ct++){ */
  /*   l2_page_table_array_occupied[ct] = 0; */
  /*   for(ind = 0; ind < MEM_L2_PAGE_TABLE_SIZE; ind++){ */
  /*     l2_page_table_array[ct][ind] = 0; */
  /*   } */
  /* } */

  // Init the L2 page table pool
  for(ct = 0; ct < MEM_L2_PAGE_TABLE_ARRAY_SIZE; ct++){
    l2_page_table_pool[ct].inuse = 0;
    for(ind = 0; ind < MEM_L2_PAGE_TABLE_SIZE; ind++){
      l2_page_table_pool[ct].page_table[ind] = 0;
    }
  }


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
  uint32 page_offset;
  uint32 l1_page_number;
  uint32 l2_page_number;  

  L2_PAGE_TABLE * l2_page_table_pool_ptr;
  uint32 l2_pte_value;

  /* printf("Page translate\n"); */

  page_offset = addr & MEM_ADDRESS_OFFSET_MASK;

  l1_page_number = addr >> MEM_L1FIELD_FIRST_BITNUM;
  l2_page_number = (addr & 0xff000) >> MEM_L2FIELD_FIRST_BITNUM;

  /* uint32 l2_page_table_index = pcb->pagetable[l1_page_number]; */
  /* uint32 l2_pte_value = l2_page_table_array[l2_page_table_index][l2_page_number]; */

  l2_page_table_pool_ptr = (L2_PAGE_TABLE *) (pcb->pagetable[l1_page_number]);
  l2_pte_value = l2_page_table_pool_ptr->page_table[l2_page_number];

  /* printf("addr before translate = %x\n", addr); */

  /* printf("l2 pte value = %x\n", l2_pte_value); */

  if((l2_pte_value & MEM_PTE_VALID) == 0){
    // Set the PROCESS_STACK_FAULT register to the addr
    pcb->currentSavedFrame[PROCESS_STACK_FAULT] = addr;

    if (MemoryPageFaultHandler(pcb) == MEM_FAIL ) {
      // Seg fault, the process has been killed
      return 0;
    }
  }

  dbprintf('m', " l1page: %d, l2page:%d, table content: 0x%08x\n", l1_page_number, l2_page_number, l2_pte_value);
  dbprintf('m', "vaddr: 0x%08x,paddr: 0x%08x\n", addr, (l2_pte_value & MEM_PTE_MASK) | page_offset);


  // Return the physical addr
  return (l2_pte_value & MEM_PTE_MASK) | page_offset;
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
  uint32 l2_array_index;

  uint32 addr = pcb->currentSavedFrame[PROCESS_STACK_FAULT];

  /* uint32 l1_page_number = addr >> (MEM_L1_BITNUM + MEM_L2_BITNUM); */
  /* uint32 l2_page_number = addr >> MEM_L2_BITNUM; */

  /* uint32 vpagenum = l1_page_number * MEM_L2_PAGE_TABLE_SIZE + l2_page_number; */

  uint32 l1_page_number = addr >> MEM_L1FIELD_FIRST_BITNUM;
  uint32 vpagenum = addr >> MEM_L2FIELD_FIRST_BITNUM;

  uint32 stackpagenum = pcb->currentSavedFrame[PROCESS_STACK_USER_STACKPOINTER] >> MEM_L2FIELD_FIRST_BITNUM;

  uint32 * l2_array_ptr;

  /* printf("MemoryPageFaultHandler\n"); */
  /* printf("addr = %x\nsp = %x\n", addr, pcb->currentSavedFrame[PROCESS_STACK_USER_STACKPOINTER]); */

  // segfault if the faulting address is not part of the stack
  if (vpagenum < stackpagenum) {
    dbprintf('m', "addr = %x\nsp = %x\n", addr, pcb->currentSavedFrame[PROCESS_STACK_USER_STACKPOINTER]);
    printf("FATAL ERROR (%d): segmentation fault at page address %x\n", findpid(pcb), addr);
    ProcessKill();
    return MEM_FAIL;
  }

  ppagenum = MemoryAllocPage();

  /* if(pcb->pagetable[l1_page_number] == MEM_L2_PAGE_TABLE_SIZE){ */
  /*   // Get a new l2 page table */
  /*   pcb->pagetable[l1_page_number] = allocate_l2_page_table(); */
  /* } */

  /* l2_array_index = pcb->pagetable[l1_page_number]; */

  /* setup_l2_pte(MemorySetupPte(ppagenum), l2_array_index, vpagenum & 0xff); */


  if(pcb->pagetable[l1_page_number] == NULL){
    // Get a new l2 page table
    pcb->pagetable[l1_page_number] = (uint32 *) allocate_l2_page_table_ptr();
  }

  l2_array_ptr = pcb->pagetable[l1_page_number];

  setup_l2_pte_ptr(MemorySetupPte(ppagenum), (void *) (l2_array_ptr), (addr & 0xff000) >> MEM_L2FIELD_FIRST_BITNUM); // Change here


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
// Return pte for l2 page table
uint32 MemorySetupPte (uint32 page) {
  return (page << MEM_L2FIELD_FIRST_BITNUM) | 0x1;
}


// page here is the physical page number
void MemoryFreePage(uint32 page) {
  
  uint32 freemap_index = page >> 5;
  uint32 freemap_bit_index = page & 0x1f;

  // Set the freemap entry to 1 --> freed!
  freemap[freemap_index] = freemap[freemap_index] | (0x1 << freemap_bit_index);

  return;
}

int malloc(PCB * pcb, int ihandle){
  
  return 0;
}


int mfree(PCB * pcb, int ihandle){
  
  return 0;
}


/* void memory_free_page_from_index(uint32 index){ */
/*   uint32 ct; */

/*   for(ct = 0; ct < MEM_L2_PAGE_TABLE_SIZE; ct++){ */
/*     if((l2_page_table_array[index][ct] & 0x1) == 1){ */
/*       // The l2 page table entry is valid, free this page */
/*       MemoryFreePage(l2_page_table_array[index][ct] >> MEM_L2FIELD_FIRST_BITNUM); */
/*       l2_page_table_array[index][ct] = 0; */
/*     } */
/*   } */
  
/*   l2_page_table_array_occupied[index] = 0; */

/*   return; */
/* } */


void memory_free_page_from_ptr(void * l2_page_table_ptr_void){
  uint32 ct;
  L2_PAGE_TABLE * l2_page_table_ptr = (L2_PAGE_TABLE *) (l2_page_table_ptr_void);

  for(ct = 0; ct < MEM_L2_PAGE_TABLE_SIZE; ct++){
    if((l2_page_table_ptr->page_table[ct] & 0x1) == 1){
      // The l2 page table entry is valid, free this page
      MemoryFreePage(l2_page_table_ptr->page_table[ct] >> MEM_L2FIELD_FIRST_BITNUM);
      l2_page_table_ptr->page_table[ct] = 0;
    }
  }
  
  l2_page_table_ptr->inuse = 0;
  /* l2_page_table_array_occupied[index] = 0; */

  return;
}



/* // Return the index of l2 page table array index */
/* uint32 allocate_l2_page_table() */
/* { */
/*   uint32 ct; */

/*   for(ct = 0; ct < MEM_L2_PAGE_TABLE_ARRAY_SIZE; ct++){ */
/*     if(l2_page_table_array_occupied[ct] == 0){ */
/*       // Occupy the l2 page table  */
/*       l2_page_table_array_occupied[ct] = 1; */
/*       break; */
/*     } */
/*   } */
  
/*   // return the index */
/*   return ct; */
/* } */

void * allocate_l2_page_table_ptr()
{
  uint32 ct;

  for(ct = 0; ct < MEM_L2_PAGE_TABLE_ARRAY_SIZE; ct++){
    if(l2_page_table_pool[ct].inuse == 0){
      l2_page_table_pool[ct].inuse = 1;
      break;
    }
  }
  
  // return the index
  return (void *) (&(l2_page_table_pool[ct]));
}


/* void setup_l2_pte(uint32 pte_value, int l2_array_index, int index) */
/* { */
/*   l2_page_table_array[l2_array_index][index] = pte_value; */
/*   return; */
/* } */


void setup_l2_pte_ptr(uint32 pte_value, void * l2_array_ptr, int index)
{
  L2_PAGE_TABLE * l2_page_table_ptr = (L2_PAGE_TABLE *) (l2_array_ptr);
  
  if(l2_page_table_ptr->inuse == 0){
    printf("ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  }

  l2_page_table_ptr->page_table[index] = pte_value;

  printf("l2_page_table_ptr->page_table[%d] = %x\n", index, l2_page_table_ptr->page_table[index]);

  return;
}

/* void setup_l2_ptr_no_index(uint32 pte_value, int l2_array_index) */
/* { */
/*   uint32 ct; */

/*   for(ct = 0; ct < MEM_L2_PAGE_TABLE_SIZE; ct++){ */
/*     if(l2_page_table_array[l2_array_index][ct] =) */
/*   } */

/* } */

void print_l2_pte(void * l2_array_ptr, int index)
{
  L2_PAGE_TABLE * l2_page_table_ptr = (L2_PAGE_TABLE *) (l2_array_ptr);
  
  if(l2_page_table_ptr->inuse == 0){
    printf("ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  }
  
  printf("l2_page_table[%d] = %x\n", index, l2_page_table_ptr->page_table[index]);
  return;
}
