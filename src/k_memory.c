/**
* @file:   k_memory.c
* @brief:  kernel memory managment routines
* @author: Yiqing Huang
* @date:   2014/01/17
*/

#include "k_memory.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

/* ----- Global Variables ----- */
U32 *gp_stack;  /* The last allocated stack low address. 8 bytes aligned */
                /* The first stack starts at the RAM high address */
                /* stack grows down. Fully decremental stack */

U32 *gp_heap_head; /* Points to the first free memory block in our heap linked list. */

extern PCB *gp_current_process;
extern int k_release_processor(void);

/**
 * @brief: Initialize RAM as follows:

0x10008000+---------------------------+ High Address
          |    Proc 1 STACK           |
          |---------------------------|
          |    Proc 2 STACK           |
          |---------------------------|<--- gp_stack
          |                           |
          |        HEAP               |
          |                           |
          |---------------------------|<--- p_end
          |        PCB 2              |
          |---------------------------|
          |        PCB 1              |
          |---------------------------|
          |        PCB pointers       |
          |---------------------------|<--- gp_pcbs
          |        Padding            |
          |---------------------------|
          |Image$$RW_IRAM1$$ZI$$Limit |
          |...........................|
          |       RTX  Image          |
          |                           |
0x10000000+---------------------------+ Low Address
*/

void memory_init(void) {
    U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
    U32 *previous = NULL;
    U32 *current;
    int i;

    /* 4 bytes padding */
    p_end += 4;

    /* allocate memory for pcb pointers   */
    gp_pcbs = (PCB **)p_end;
    p_end += (((U32)NUM_PROCS) * sizeof(PCB *)); // TODO make nicer

    for (i = 0; i < NUM_PROCS; i++) {
        gp_pcbs[i] = (PCB *)p_end;
        p_end += sizeof(PCB);
    }
#ifdef DEBUG_0
    for (i = 0; i < NUM_PROCS; i++) {
        printf("gp_pcbs[%d] = 0x%x \n", i, gp_pcbs[i]);
    }
#endif

    /* prepare for alloc_stack() to allocate memory for stacks */

    gp_stack = (U32 *)RAM_END_ADDR;
    if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
        --gp_stack;
    }

    /* allocate memory for heap */
    /* p_end points to the bottom of our heap
        Note: Stacks have not been allocated yet, we are assuming that
        we won't run into the stack memory.
    */

    current = (U32*)p_end + 4; // 4 byte padding

    for (i = 0; i < NUM_MEMORY_BLOCKS; i++, current = (U32*)((U8*)current + MEMORY_BLOCK_SIZE)) {
        *((U32*)current) = (U32)previous; // set current block to point to next block
        previous = current;
    }
    gp_heap_head = previous;
}

/**
* @brief: allocate stack for a process, align to 8 bytes boundary
* @param: size, stack size in bytes
* @return: The top of the stack (i.e. high address)
* POST:  gp_stack is updated.
*/

U32 *alloc_stack(U32 size_b) {
    U32 *sp;
    sp = gp_stack; /* gp_stack is always 8 bytes aligned */

    /* update gp_stack */
    gp_stack = (U32 *)((U8 *)sp - size_b);

    /* 8 bytes alignement adjustment to exception stack frame */
    if ((U32)gp_stack & 0x04) {
        --gp_stack;
    }
    return sp;
}

/**
 * @brief: gets memory block of size MEMORY_BLOCK_SIZE
 * @return: Pointer to this block. NULL if no blocks available.
 * POST:  gp_stack is updated.
 */
void *k_request_memory_block(void) {
    void* returnVal;
#ifdef DEBUG_0
    static int count = 0;
#endif

    returnVal = (void *)gp_heap_head;

#ifdef DEBUG_0
    printf("k_request_memory_block #%d: entering...\n", count);
#endif /* ! DEBUG_0 */

    if (gp_heap_head != NULL) {
        // Making sure we do not deference the HEAD if it is null.
        gp_heap_head = (U32*)(*gp_heap_head);
#ifdef DEBUG_0
        count++;
#endif
    } else {
#ifdef DEBUG_0
        printf("Out of memory, oops\n");
#endif
        /* we have no free memory, set current process to BLOCKED */
        gp_current_process->m_state = BLOCKED;
        k_release_processor();
    }

    return returnVal;
}

int k_release_memory_block(void *p_mem_blk) {
    U32* head_value = gp_heap_head;
#ifdef DEBUG_0
    printf("k_release_memory_block: releasing block @ 0x%x\n", p_mem_blk);
#endif /* ! DEBUG_0 */
    gp_heap_head = (U32*)p_mem_blk;
    *gp_heap_head = (U32)head_value;

    return RTX_OK;
}
