#ifndef	_memory_h_
#define	_memory_h_

// Put all your #define's in memory_constants.h
#include "memory_constants.h"

extern int lastosaddress; // Defined in an assembly file

//--------------------------------------------------------
// Existing function prototypes:
//--------------------------------------------------------

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

int malloc(PCB * pcb, int ihandle);
int mfree(PCB * pcb, int ihandle);

void memory_free_page_from_index(uint32 index);
uint32 allocate_l2_page_table();
void setup_l2_pte(uint32 pte_value, int l2_array_index, int index);

void memory_free_page_from_ptr(int index);
uint32 * allocate_l2_page_table_ptr(int * pcb_page_table_array);
void setup_l2_pte_ptr(uint32 pte_value, void * l2_array_ptr, int index);

void print_l2_pte(void * l2_array_ptr, int index);

#endif	// _memory_h_
