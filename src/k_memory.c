#include "k_memory.h"
#include "utils.h"

/* Global variables */

/* The last allocated stack low address. 8 bytes aligned
 * The first stack starts at the RAM high address
 * stack grows down. Fully decremental stack */
U32* gp_stack;
U32* gp_heap_head; // points to the first free memory block in our heap linked list

extern PCB* gp_current_process;
extern int k_release_processor(void);
extern void pq_push_ready(PCB*);
extern PCB* pq_pop_blocked(void);
extern void pq_push_blocked(PCB*);

#ifdef DEBUG_0
    // keep track of how many memory blocks have been allocated for debugging
    int count = 1;
#endif

/*
 * @brief: Initialize RAM as follows:
 *
 * 0x10008000+---------------------------+ High Address
 *           |    Proc 1 STACK           |
 *           |---------------------------|
 *           |    Proc 2 STACK           |
 *           |---------------------------|<--- gp_stack
 *           |        HEAP               |
 *           |---------------------------|<--- p_end
 *           |        PCB 2              |
 *           |---------------------------|
 *           |        PCB 1              |
 *           |---------------------------|
 *           |        PCB pointers       |
 *           |---------------------------|<--- gp_pcbs
 *           |        Padding            |
 *           |---------------------------|
 *           |Image$$RW_IRAM1$$ZI$$Limit |
 *           |...........................|
 *           |       RTX  Image          |
 * 0x10000000+---------------------------+ Low Address
 */

void memory_init(void) {
    U8* p_end = (U8*)&Image$$RW_IRAM1$$ZI$$Limit;
    U32* previous = NULL;
    U32* current;
    int i;

    p_end += 4; // 4 bytes padding

    // allocate memory for pcb pointers
    gp_pcbs = (PCB**)p_end;
    p_end += (((U32)NUM_PROCS) * sizeof(PCB*));

    for (i = 0; i < NUM_PROCS; i++) {
        gp_pcbs[i] = (PCB*)p_end;
        p_end += sizeof(PCB);

        logln("gp_pcbs[%d] = 0x%x", i, gp_pcbs[i]);
    }

    // prepare for alloc_stack() to allocate memory for stacks
    gp_stack = (U32*)RAM_END_ADDR;
    if ((U32)gp_stack & 0x04) { // 8 bytes alignment
        --gp_stack;
    }

    /* allocate memory for heap
     * p_end points to the bottom of our heap
     *
     * Note: Stacks have not been allocated yet, we are assuming that
     * we won't run into the stack memory */

    current = (U32*)p_end + 4; // 4 byte padding

    for (i = 0; i < NUM_MEMORY_BLOCKS; i++, current = (U32*)((U8*)current + MEMORY_BLOCK_SIZE)) {
        // set current block to point to next block
        *((U32*)current) = (U32)previous;
        previous = current;
    }
    gp_heap_head = previous;
}

/**
* Allocate stack for a process, align to 8 bytes boundary
*
* @param size_b stack size in bytes
* @return the top of the stack (i.e. high address)
* POST: gp_stack is updated
*/
U32* alloc_stack(U32 size_b) {
    U8 numBytesForAllRegisters = 16 * 4; // 16 registers, 4 bytes a piece
    U32* sp;
    sp = gp_stack; // always 8 bytes aligned

    // update gp_stack
    gp_stack = (U32*)((U8*)sp - (size_b + numBytesForAllRegisters));

    // 8 bytes alignement adjustment to exception stack frame
    if ((U32)gp_stack & 0x04) {
        --gp_stack;
    }

    return sp;
}

/**
 * Gets a pointer to a memory block of size MEMORY_BLOCK_SIZE if there is block
 * available in the heap.
 *
 * @return pointer to this block. NULL if no blocks available
 * POST: gp_stack is updated
 */
void* k_request_memory_block(void) {
    void* returnVal;

#ifdef DEBUG_0
    log("k_request_memory_block #%d: entering ...", count);
#endif

    do {
        returnVal = (void*)gp_heap_head;
        if (gp_heap_head != NULL) {
            // making sure we do not deference the HEAD if it is null.
            gp_heap_head = (U32*)(*gp_heap_head);

#ifdef DEBUG_0
            logln(" allocated");
            count++;
#endif
        } else {
            logln("Out of memory, oops");

            if (gp_current_process->m_priority != INTERRUPT) {
                /* we have no free memory, set current process to STATE_BLOCKED_MEMORY */
                gp_current_process->m_state = STATE_BLOCKED_MEMORY;
            }

            k_release_processor();
        }
    } while (returnVal == NULL);

    return returnVal;
}

/**
 * Return a memory block to the heap. If there is a blocked process in the
 * priority queue with a higher priority than the current process, it is
 * preempted.
 *
 * @param p_mem_blk pointer to the reclaimed memory block
 * @return 0 on success
 */
int k_release_memory_block(void* p_mem_blk) {
    PCB* blocked_proc = pq_pop_blocked();
    U32* head_value = gp_heap_head;

#ifdef DEBUG_0
    logln("k_release_memory_block: releasing block #%d @ 0x%x", --count, p_mem_blk);
#endif

    gp_heap_head = (U32*)p_mem_blk;
    *gp_heap_head = (U32)head_value;

    /* preempt the current process if a blocked process has a higher priority */
    if (blocked_proc != NULL) {
        blocked_proc->m_state = STATE_READY;
        pq_push_ready(blocked_proc);

        if (gp_current_process->m_priority != INTERRUPT) {
            k_release_processor();
        }
    }

    return RTX_OK;
}
