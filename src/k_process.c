/**
 * @file:   k_process.c
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/02/28
 * NOTE: The example code shows one way of implementing context switching.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes only two simple user processes and NO HARDWARE INTERRUPTS.
 *       The purpose is to show how context switch could be done under stated assumptions.
 *       These assumptions are not true in the required RTX Project!!!
 *       If you decide to use this piece of code, you need to understand the assumptions and
 *       the limitations.
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs;                  /* array of pcbs */
PCB *gp_current_process = NULL; /* always point to the current RUN process */

U32 g_switch_flag = 0;          /* whether to continue to run the process before the UART receive interrupt */
                                /* 1 means to switch to another process, 0 means to continue the current process */
                                /* this value will be set by UART handler */

/* process initialization table */
PROC_INIT g_proc_table[NUM_TEST_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];

/* process priority queues */
PCB *g_proc_priority_front[5] = {NULL, NULL, NULL, NULL, NULL};
PCB *g_proc_priority_back[5] = {NULL, NULL, NULL, NULL, NULL};

int is_proc_priority_empty(int priority) {
    /* return true if priority is out of bounds */
    if (priority < 0 || priority > 4) return 1;
    return (g_proc_priority_front[priority] == NULL);
}

void proc_priority_push(PCB *proc) {
    if (is_proc_priority_empty(proc->m_priority)) {
        /* if queue is empty, set both the front and back to proc */
        g_proc_priority_front[proc->m_priority] = proc;
        g_proc_priority_back[proc->m_priority] = proc;
    } else {
        /* if queue is not empty, add proc to the back of the queue */
        g_proc_priority_back[proc->m_priority]->mp_next = proc;
        g_proc_priority_back[proc->m_priority] = proc;
    }
}

PCB *proc_priority_pop_front(int priority) {
    PCB *front_proc;

    /* if our queue is empty, return a NULL pointer */
    if (is_proc_priority_empty(priority)) return NULL;

    front_proc = g_proc_priority_front[priority];
    g_proc_priority_front[priority] = front_proc->mp_next;

    /* if queue only had 1 proc, it is now empty */
    if (g_proc_priority_front[priority] == NULL) g_proc_priority_back[priority] = NULL;

    front_proc->mp_next = NULL;
    return front_proc;
}

PCB *proc_priority_pop_proc(PCB *proc) {
    PCB *found_proc;
    PCB *temp_proc = g_proc_priority_front[proc->m_priority];

    /* proc is at the front of the queue */
    if (temp_proc == proc) return proc_priority_pop_front(proc->m_priority);

    /* traverse our queue till we find proc */
    while (temp_proc != NULL && temp_proc->mp_next != NULL && temp_proc->mp_next != proc) {
        temp_proc = temp_proc->mp_next;
    }

    /* proc does not exist */
    if (temp_proc->mp_next == NULL) return NULL;

    /* remove and return temp_proc->mp_next */
    found_proc = temp_proc->mp_next;
    temp_proc->mp_next = found_proc->mp_next;
	found_proc->mp_next = NULL;
    return found_proc;
}

PCB *proc_priority_pop_ready() {
    int priority;
    PCB *proc = NULL;

    for (priority = 0; priority < 5; priority++) {
        proc = g_proc_priority_front[priority];

        while (proc != NULL) {
            if (proc->m_state == BLOCKED) proc = proc->mp_next;
            else return proc_priority_pop_proc(proc);
        }
    }

    /* should never reach here, NULL process should always be returned */
    return proc;
}

PCB *proc_priority_pop_blocked() {
    int priority;
    int max_priority = gp_current_process->m_priority;
    PCB *proc = NULL;

    for (priority = 0; priority < max_priority; priority++) {
        proc = g_proc_priority_front[priority];

        while (proc != NULL) {
            if (proc->m_state == RDY) proc = proc->mp_next;
            else return proc_priority_pop_proc(proc);
        }
    }

    /* if no blocked process of higher priority is found, return NULL */
    return proc;
}

/**
 * @biref: initialize all processes in the system
 * NOTE: We assume there are only two user processes in the system in this example.
 */
void process_init()
{
    int i;
    U32 *sp;

        /* fill out the initialization table */
    set_test_procs();
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        g_proc_table[i].m_pid = g_test_procs[i].m_pid;
        g_proc_table[i].m_priority = g_test_procs[i].m_priority;
        g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
        g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
    }

    /* initilize exception stack frame (i.e. initial context) for each process */
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        int j;
        (gp_pcbs[i])->mp_next = NULL;
        (gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
        (gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
        (gp_pcbs[i])->m_state = NEW;

        sp = alloc_stack((g_proc_table[i]).m_stack_size);
        *(--sp)  = INITIAL_xPSR; // user process initial xPSR
        *(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
        for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
            *(--sp) = 0x0;
        }
        (gp_pcbs[i])->mp_sp = sp;

        proc_priority_push(gp_pcbs[i]);
    }
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */

PCB *scheduler(void)
{
    PCB *old_proc = gp_current_process;
    if (old_proc != NULL) proc_priority_push(old_proc);

    gp_current_process = proc_priority_pop_ready();
    return gp_current_process;
}

/*@brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 *@param: p_pcb_old, the old pcb that was in RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
int process_switch(PCB *p_pcb_old)
{
    PROC_STATE_E state;

    state = gp_current_process->m_state;

    if (state == NEW) {
        if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
            if (p_pcb_old->m_state != BLOCKED) p_pcb_old->m_state = RDY;
            p_pcb_old->mp_sp = (U32 *) __get_MSP();
        }
        gp_current_process->m_state = RUN;
        __set_MSP((U32) gp_current_process->mp_sp);
        __rte();  // pop exception stack frame from the stack for a new processes
    }

    /* The following will only execute if the if block above is FALSE */

    if (gp_current_process != p_pcb_old) {
        if (state == RDY){
            if (p_pcb_old->m_state != BLOCKED) p_pcb_old->m_state = RDY;
            p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
            gp_current_process->m_state = RUN;
            __set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack
        } else {
            gp_current_process = p_pcb_old; // revert back to the old proc on error
            return RTX_ERR;
        }
    }
    return RTX_OK;
}
/**
 * @brief release_processor().
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void)
{
    PCB *p_pcb_old = NULL;

    p_pcb_old = gp_current_process;
    gp_current_process = scheduler();

    if ( gp_current_process == NULL  ) {
        gp_current_process = p_pcb_old; // revert back to the old process
        return RTX_ERR;
    }
    if ( p_pcb_old == NULL ) {
        p_pcb_old = gp_current_process;
    }
    process_switch(p_pcb_old);
    return RTX_OK;
}

int set_process_priority(int process_id, int priority) {
    PCB* process;

    if (process_id <= 0 || process_id >= NUM_TEST_PROCS) return RTX_ERR;
    if (priority < 0 || priority >= HIDDEN) return RTX_ERR;

    process = proc_priority_pop_proc(gp_pcbs[process_id]);
    process->m_priority = priority;
    proc_priority_push(process);

    /* preempt if the new priority is ready and has a higher priority */
    if (priority < gp_current_process->m_priority && process->m_state == RDY) {
        k_release_processor();
    }

    return RTX_OK;
}

int get_process_priority(int process_id) {
    PCB* process;

    if (process_id < 0 || process_id >= NUM_TEST_PROCS) return RTX_ERR;

    process = gp_pcbs[process_id];
    return process->m_priority;
}
