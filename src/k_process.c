#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif

typedef struct {
    PCB* front[NUM_PRIORITIES];
    PCB* back[NUM_PRIORITIES];
} PQ;

/* global variables */
PCB **gp_pcbs; // array of pcbs
PCB *gp_current_process = NULL; // always point to the current STATE_RUN process

/* whether to continue to run the process before the UART receive interrupt
1 means to switch to another process, 0 means to continue the current process
this value will be set by UART handler */
U32 g_switch_flag = 0;

/* process initialization table */
PROC_INIT g_proc_table[NUM_PROCS];

extern void insert_message_delayed(PCB*, MSG_BUF*, int);

/* process priority queues */
PQ g_blocked_pq;
PQ g_ready_pq;

volatile int timer_i_proc_pending = 0;
volatile int uart_i_proc_pending = 0;

/* check if a given priority has no processes */
int pq_is_priority_empty(const PQ* pq, const int priority) {
    /* return true if priority is out of bounds */
    if (priority < HIGH || priority > HIDDEN) return 1;
    return pq->front[priority] == NULL;
}

/* push a given process onto the priority queue */
void pq_push(PQ* pq, PCB* proc) {
    int priority = proc->m_priority;
    if (pq_is_priority_empty(pq, priority)) {
        /* if queue is empty, set both the front and back to proc */
        pq->front[priority] = proc;
        pq->back[priority] = proc;
    } else {
        /* if queue is not empty, add proc to the back of the queue */
        pq->back[priority]->mp_next = proc;
        pq->back[priority] = proc;
    }
}

/* push a given process onto the front of the priority queue */
void pq_push_front(PQ* pq, PCB* proc) {
    int priority = proc->m_priority;
    if (pq_is_priority_empty(pq, priority)) {
        /* if queue is empty, set both the front and back to proc */
        pq->front[priority] = proc;
        pq->back[priority] = proc;
    } else {
        /* if queue is not empty, add proc to the front of the queue */
        proc->mp_next = pq->front[priority];
        pq->front[priority] = proc;
    }
}

/* get the next process of a given priority. Only used internally */
PCB* pq_pop_front(PQ* pq, const int priority) {
    PCB* front_proc;

    /* if our queue is empty, return a NULL pointer */
    if (pq_is_priority_empty(pq, priority)) return NULL;

    front_proc = pq->front[priority];
    pq->front[priority] = front_proc->mp_next;

    /* if queue only had 1 proc, it is now empty */
    if (pq->front[priority] == NULL) pq->back[priority] = NULL;

    front_proc->mp_next = NULL;
    return front_proc;
}

/* pop a specific process */
PCB* pq_pop_PCB(PQ* pq, const PCB* proc) {
    PCB* found_proc;
    PCB* temp_proc = pq->front[proc->m_priority];

    /* proc is at the front of the queue */
    if (temp_proc == proc) return pq_pop_front(pq, proc->m_priority);

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

/* pop the first, highest-priority process */
PCB* pq_pop(PQ* pq) {
    int priority;
    PCB* proc = NULL;

    for (priority = HIGH; priority <= HIDDEN; priority++) {
        proc = pq->front[priority];
        if (proc != NULL) return pq_pop_front(pq, priority);
    }

    return NULL; // impossible - should return NULL process first
}

/* convenience functions, useful for external calls */
void pq_push_ready(PCB* proc) {
    pq_push(&g_ready_pq, proc);
}

void pq_push_ready_front(PCB* proc) {
    pq_push_front(&g_ready_pq, proc);
}

void pq_push_blocked(PCB* proc) {
    pq_push(&g_blocked_pq, proc);
}

PCB* pq_pop_ready() {
    return pq_pop(&g_ready_pq);
}

PCB* pq_pop_blocked() {
    return pq_pop(&g_blocked_pq);
}

PCB* pq_pop_PCB_ready(const PCB* proc) {
    return pq_pop_PCB(&g_ready_pq, proc);
}

PCB* pq_pop_PCB_blocked(const PCB* proc) {
    return pq_pop_PCB(&g_blocked_pq, proc);
}

/* initialize all processes in the system */
void process_init() {
    int i;
    U32 *sp;

    /* fill out the initialization table */

    for (i = 0; i < NUM_PROCS; i++) {
        g_proc_table[i].m_pid = -1; // uninitialized;
    }
    set_test_procs();
    set_sys_procs();

    /* initialize priority queues */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        g_blocked_pq.front[i] = NULL;
        g_blocked_pq.back[i]  = NULL;
        g_ready_pq.front[i]   = NULL;
        g_ready_pq.back[i]    = NULL;
    }

    /* initilize exception stack frame (i.e. initial context) for each process */
    for (i = 0; i < NUM_PROCS; i++) {
        int j;
        if (g_proc_table[i].m_pid == -1) continue;

        (gp_pcbs[i])->mp_next               = NULL;
        (gp_pcbs[i])->m_pid                 = g_proc_table[i].m_pid;
        (gp_pcbs[i])->m_priority            = g_proc_table[i].m_priority;
        (gp_pcbs[i])->m_state               = STATE_NEW;
        (gp_pcbs[i])->mp_msg_queue_front = NULL;
        (gp_pcbs[i])->mp_msg_queue_back  = NULL;

        sp = alloc_stack(g_proc_table[i].m_stack_size);
        *(--sp) = INITIAL_xPSR; // user process initial xPSR
        *(--sp) = (U32)(g_proc_table[i].mpf_start_pc); // PC contains the entry point of the process
        for (j = 0; j < 6; j++) { // R0-R3, R12 are cleared with 0
            *(--sp) = 0x0;
        }
        (gp_pcbs[i])->mp_sp = sp;

        if ((gp_pcbs[i])->m_priority != INTERRUPT) {
            pq_push_ready(gp_pcbs[i]);
        }
    }
}

/** Choose which queued process to run next by popping the next ready process
 * from the priority queue.
 *
 * @return global pointer to current process
 */
PCB *scheduler(void) {
    PCB *old_proc = gp_current_process;
    if (old_proc != NULL && old_proc->m_priority != INTERRUPT) {
        switch(old_proc->m_state) {
            case STATE_BLOCKED_MEMORY:
            case STATE_BLOCKED_MSG:
                break;
            case STATE_NEW:
            case STATE_READY:
            case STATE_RUN:
                if (timer_i_proc_pending || uart_i_proc_pending) {
                    pq_push_ready_front(old_proc);
                } else {
                    pq_push_ready(old_proc);
                }
                break;
            default:
                #ifdef DEBUG_0
                printf("scheduler: unknown state\n");
                #endif
                break;
        }
    }

    if (timer_i_proc_pending) {
        timer_i_proc_pending = 0;
        return gp_pcbs[PID_TIMER_IPROC];
    } else if (uart_i_proc_pending) {
        uart_i_proc_pending = 0;
        return gp_pcbs[PID_UART_IPROC];
    }

    return pq_pop_ready();
}

__asm __new_i_proc_rte() {
    POP {r0-r4, r12, pc}
}

/*@brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 *@param: p_pcb_old, the old pcb that was in STATE_RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
int process_switch(PCB *p_pcb_old) {
    U32 state = gp_current_process->m_state;

    if (state == STATE_NEW) {
        if (gp_current_process != p_pcb_old && p_pcb_old->m_state != STATE_NEW) {
            switch(p_pcb_old->m_state) {
            case STATE_RUN:
            case STATE_READY:
                p_pcb_old->m_state = STATE_READY;
                break;
            case STATE_BLOCKED_MEMORY:
            case STATE_BLOCKED_MSG:
                // Don't set state to STATE_READY
                break;
            case STATE_NEW:
                #ifdef DEBUG_0
                printf("process_switch: process has state STATE_NEW but shouldn't\n");
                #endif
                break;
            default:
                #ifdef DEBUG_0
                printf("process_switch: unknown state\n");
                #endif
                break;
            };

            p_pcb_old->mp_sp = (U32 *) __get_MSP();
        }

        gp_current_process->m_state = STATE_RUN;
        __set_MSP((U32) gp_current_process->mp_sp);

        if (gp_current_process->m_pid == 0 || gp_current_process->m_pid > 6) {
            __new_i_proc_rte();
        } else {
            __rte();  // pop exception stack frame from the stack for a new processes
        }
    }

    /* The following will only execute if the if block above is FALSE */
    if (gp_current_process != p_pcb_old) {
        if (state == STATE_READY) {
            switch(p_pcb_old->m_state) {
            case STATE_RUN:
            case STATE_READY:
                p_pcb_old->m_state = STATE_READY;
                break;
            case STATE_BLOCKED_MEMORY:
            case STATE_BLOCKED_MSG:
                // Don't set state to STATE_READY
                break;
            case STATE_NEW:
                #ifdef DEBUG_0
                printf("process_switch: process has state STATE_NEW but shouldn't\n");
                #endif
                break;
            default:
                #ifdef DEBUG_0
                printf("process_switch: unknown state\n");
                #endif
                break;
            };

            p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
            gp_current_process->m_state = STATE_RUN;
            __set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack
        } else {
            gp_current_process = p_pcb_old; // revert back to the old proc on error
            return RTX_ERR;
        }
    }
    return RTX_OK;
}

/**
 * Remove the current process from the processor. A new process is determined
 * using the scheduler, and the old process is queued.

 * @return 0 on success, -1 on error
 * POST gp_current_process gets updated to next to run process
 */
int k_release_processor(void) {
    PCB *p_pcb_old = gp_current_process;
    gp_current_process = scheduler();

    if (gp_current_process == NULL) { // should never occur
        gp_current_process = p_pcb_old; // revert back to the old process
        return RTX_ERR;
    }

    if (p_pcb_old == NULL) { // this only happens once on initialization
        p_pcb_old = gp_current_process;
    }

    process_switch(p_pcb_old);
    return RTX_OK;
}
/**
 * Set the priority of a specified process. The process will be pushed back onto
 * the priority queue. If the process is unblocked and the new priority is
 * higher than the current process priority, the specified process will preempt
 * the current process.
 *
 * @param process_id process
 * @param priority
 * @return 0 if success, -1 if error
 */
int k_set_process_priority(const int process_id, const int priority) {
    PCB* process;

    if (process_id < 1 || process_id > 6) return RTX_ERR;
    if (priority < HIGH || priority > LOWEST) return RTX_ERR;

    if (process_id == gp_current_process->m_pid) {
        gp_current_process->m_priority = priority;
        return k_release_processor();
    }

    // TODO: make this generic?
    process = pq_pop_PCB_ready(gp_pcbs[process_id]);
    if (process == NULL) process = pq_pop_PCB_blocked(gp_pcbs[process_id]);

    process->m_priority = priority;
    switch(process->m_state) {
    case STATE_NEW:
    case STATE_READY:
        pq_push_ready(process);
        break;
    case STATE_BLOCKED_MSG:
        break;
    case STATE_BLOCKED_MEMORY:
        pq_push_blocked(process);
        break;
    case STATE_RUN:
        #ifdef DEBUG_0
        printf("k_set_process_priority: process has state STATE_RUN but is not current running process\n");
        #endif
        break;
    default:
        #ifdef DEBUG_0
        printf("k_set_process_priority: unknown state\n");
        #endif
        break;
    };

    return k_release_processor();
}

/**
 * @param process_id
 * @return priority of the process or -1 if error
 */
int k_get_process_priority(const int process_id) {
    PCB* process;

    if (process_id < 0 || process_id >= NUM_PROCS) return RTX_ERR;

    process = gp_pcbs[process_id];
    return process->m_priority;
}

/* Adds a given message to the target's message queue*/
void enqueue_message(PCB* target, MSG_BUF* message) {
    message->mp_next = NULL;
    if (target->mp_msg_queue_back == NULL) {
        target->mp_msg_queue_front = message;
    } else {
        target->mp_msg_queue_back->mp_next = message;
    }
    target->mp_msg_queue_back = message;
}

MSG_BUF* dequeue_message(PCB* target) {
    MSG_BUF* return_val = target->mp_msg_queue_front;

    if (return_val == NULL) {
        // Empty queue, don't do anything
    } else if (return_val == target->mp_msg_queue_back) {
        // Queue with exactly one element
        target->mp_msg_queue_front = NULL;
        target->mp_msg_queue_back = NULL;
    } else {
        // Queue with 2 or more elements
        target->mp_msg_queue_front = return_val->mp_next;
    }

    return return_val;
}

MSG_BUF* create_message_headers(void* message_envelope, int target_proc_id) {
    MSG_BUF* message = (MSG_BUF*) message_envelope;
    message->mp_next = NULL;
    message->m_send_pid = gp_current_process->m_pid;
    message->m_recv_pid = target_proc_id;
    message->m_expiry = 0;
    return message;
}

int k_send_message(int process_id, MSG_BUF *message) {
    PCB* target = gp_pcbs[process_id];
    enqueue_message(target, message);

    if (target->m_state == STATE_BLOCKED_MSG) {
        target->m_state = STATE_READY;
        pq_push_ready(target);
        // DO WE CALL RELEASE HERE??? WHAT IF CURR PROC HAS HIGHEST PRIORITY
        // slides don't have this call
        if (target->m_priority < gp_current_process->m_priority && gp_current_process->m_priority != INTERRUPT) {
            return k_release_processor();
        }
    }

    return RTX_OK;
}

/**
 * Adds the given message to the given PCB
 * Sends message to given process id
 * Preempts if higher priority proc waiting for message
 */
int k_send_message_envelope(int process_id, void* message_envelope) {
    MSG_BUF* message = create_message_headers(message_envelope, process_id);
    return k_send_message(process_id, message);
}

int k_send_message_delayed(int process_id, void* message_envelope, int delay) {
    MSG_BUF* message = create_message_headers(message_envelope, process_id);
    PCB* target;

    if (delay == 0) {
        return k_send_message(process_id, message);
    }

    target = gp_pcbs[process_id];
    insert_message_delayed(target, message, delay);

    return RTX_OK;
}

/**
 * Blocking recieve
 * sets sender_id's value to the id of the proc ID of the sender
 */
void *k_receive_message(int *sender_id) {
    MSG_BUF* message = dequeue_message(gp_current_process);
    while (message == NULL) {
        // No waiting messages, so preempt this process
        gp_current_process->m_state = STATE_BLOCKED_MSG;
        k_release_processor();
        message = dequeue_message(gp_current_process);
    }
    if (sender_id != NULL) {
        *sender_id = message->m_send_pid;
    }
    return (void*)message;
}

void k_timer_interrupt() {
    timer_i_proc_pending = 1;
    k_release_processor(); // TODO: maybe get rid of this call to make this consistent with k_uart_interrupt
}

void k_uart_interrupt() {
    uart_i_proc_pending = 1;
}
void print_queue(PQ *q) {
    int i;
    PCB *current;

    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (i == 0) printf("HIGH:\n");
        else if (i == 1) printf("MEDIUM:\n");
        else if (i == 2) printf("LOW:\n");
        else if (i == 3) printf("LOWEST:\n");
        else if (i == 4) printf("NULL:\n");
        else printf("INVALID PRIORITY:\n");

        current = q->front[i];
        while (current != NULL) {
            printf("\t%d\n", current->m_pid);
            current = current->mp_next;
        }
    }
}

void print_memory_blocked_procs() {
    printf("Processes blocked on memory\n");
    printf("---------------------------\n");

    print_queue(&g_blocked_pq);
}

void print_message_blocked_procs() {
    int i;
    PROC_INIT current;
    PCB* currentPCB;

    printf("Processes blocked on receive\n");
    printf("----------------------------\n");

    for (i = 0; i < NUM_PROCS; i++) {
        current = g_proc_table[i];
        if (current.m_pid == -1) continue;

        currentPCB = gp_pcbs[current.m_pid];

        if (currentPCB->m_state == STATE_BLOCKED_MSG) {
            printf("\t%d\n", current.m_pid);
        }
    }
}

void print_ready_procs() {
    printf("Processes in the Ready Queue\n");
    printf("----------------------------\n");

    print_queue(&g_ready_pq);
}
