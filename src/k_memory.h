#ifndef K_MEM_H
#define K_MEM_H

#include "k_rtx.h"

/* ----- Definitions ----- */
#define RAM_END_ADDR 0x10008000
#define MEMORY_BLOCK_SIZE 128
#define NUM_MEMORY_BLOCKS 30

/* ----- Variables ----- */
/* This symbol is defined in the scatter file (see RVCT Linker User Guide) */
extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;
extern PCB** gp_pcbs;
extern PROC_INIT g_proc_table[NUM_PROCS];

/* ----- Functions ------ */
void memory_init(void);
U32* alloc_stack(U32 size_b);
void* k_request_memory_block(void);
int k_release_memory_block(void* p_mem_blk);

#endif // K_MEM_H
