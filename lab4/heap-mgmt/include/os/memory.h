#ifndef	_memory_h_
#define	_memory_h_

// Put all your #define's in memory_constants.h
#include "memory_constants.h"
#include "queue.h"
extern int lastosaddress; // Defined in an assembly file

//--------------------------------------------------------
// Existing function prototypes:
//--------------------------------------------------------

/* typedef struct heapNode{ */
/*   struct heapNode *next; */
  
/* } heapNode; */

/* typedef struct heap { */
/*   struct Link *start; //starting address of the memory block */
/*   int size; //size of the memory block */
/*   int isOccupied; //if this block is occupied */
/* } heap; */

int MemoryGetSize();
void MemoryModuleInit();
uint32 MemoryTranslateUserToSystem (PCB *pcb, uint32 addr);
int MemoryMoveBetweenSpaces (PCB *pcb, unsigned char *system, unsigned char *user, int n, int dir);
int MemoryCopySystemToUser (PCB *pcb, unsigned char *from, unsigned char *to, int n);
int MemoryCopyUserToSystem (PCB *pcb, unsigned char *from, unsigned char *to, int n);
int MemoryPageFaultHandler(PCB *pcb);

//---------------------------------------------------------
// Put your function prototypes here
//---------------------------------------------------------
// All function prototypes including the malloc and mfree functions go here

int MemoryAllocPage(void);
uint32 MemorySetupPte (uint32 page);
void MemoryFreePage(uint32 page);

void *malloc(PCB * pcb, int memsize);
int mfree(PCB * pcb, void *ptr);

#endif	// _memory_h_
