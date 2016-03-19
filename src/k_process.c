#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "k_process.h"
#include "uart_polling.h"
#include "pq.h"
#include "utils.h"

#ifdef DEBUG_0
    #include "printf.h"
#endif

/* Global variables */
PCB** gp_pcbs; // array of pcbs
PCB* gp_current_process = NULL; // always point to the current STATE_RUN process

/* Process initialization table */
PROC_INIT g_proc_table[NUM_PROCS];

/* Process priority queues */
PQ g_blocked_pq;
PQ g_ready_pq;

volatile int timer_i_proc_pending = 0;
volatile int uart_i_proc_pending = 0;

extern void insert_message_delayed(PCB*, MSG_BUF*, int);

/* Priority queue convenience functions, useful for external calls */
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

/* Initialize all processes in the system */
void process_init() {
    int i;
    U32* sp;

    // fill out the initialization table
    for (i = 0; i < NUM_PROCS; i++) {
        g_proc_table[i].m_pid = -1; // uninitialized
    }

    set_test_procs();
    set_sys_procs();
    set_i_procs();

    // initialize priority queues
    for (i = 0; i < NUM_PRIORITIES; i++) {
        g_blocked_pq.front[i] = NULL;
        g_blocked_pq.back[i]  = NULL;
        g_ready_pq.front[i]   = NULL;
        g_ready_pq.back[i]    = NULL;
    }

    // initilize exception stack frame (i.e. initial context) for each process
    for (i = 0; i < NUM_PROCS; i++) {
        int j;

        // skip if this process is not being used
        if (g_proc_table[i].m_pid == -1) continue;

        (gp_pcbs[i])->mp_next            = NULL;
        (gp_pcbs[i])->m_pid              = g_proc_table[i].m_pid;
        (gp_pcbs[i])->m_priority         = g_proc_table[i].m_priority;
        (gp_pcbs[i])->m_state            = STATE_NEW;
        (gp_pcbs[i])->mp_msg_queue_front = NULL;
        (gp_pcbs[i])->mp_msg_queue_back  = NULL;

        sp = alloc_stack(g_proc_table[i].m_stack_size);
        *(--sp) = INITIAL_xPSR; // user process initial xPSR
        *(--sp) = (U32)(g_proc_table[i].mpf_start_pc); // PC contains the entry point of the process
        for (j = 0; j < 6; j++) { // R0-R3, R12 are cleared with 0
            *(--sp) = 0x0;
        }
        (gp_pcbs[i])->mp_sp = sp;

        // only push non-interrupt processes onto the ready queue
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
PCB* scheduler(void) {
    PCB* old_proc = gp_current_process;
    if (old_proc != NULL && old_proc->m_priority != INTERRUPT) {
        switch (old_proc->m_state) {
        case STATE_BLOCKED_MEMORY:
        case STATE_BLOCKED_MSG:
            break;
        case STATE_NEW:
        case STATE_READY:
        case STATE_RUN:
            // if this process got interrupted, we will resume it after the interrupt
            // handler has run
            if (timer_i_proc_pending || uart_i_proc_pending) {
                pq_push_ready_front(old_proc);
            } else {
                pq_push_ready(old_proc);
            }

            break;
        default:
            logln("scheduler: unknown state");
            break;
        }
    }

    if (timer_i_proc_pending) {
        timer_i_proc_pending = 0;
        return gp_pcbs[PID_TIMER_IPROC];
    } else if (uart_i_proc_pending) {
        uart_i_proc_pending = 0;
        return gp_pcbs[PID_UART_IPROC];
    } else {
        return pq_pop_ready();
    }
}

__asm __new_special_proc_rte() {
    POP {r0 - r4, r12, pc}
}

/*@brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 *@param: p_pcb_old, the old pcb that was in STATE_RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
int process_switch(PCB* p_pcb_old) {
    U32 state = gp_current_process->m_state;

    if (state == STATE_NEW) {
        if (gp_current_process != p_pcb_old && p_pcb_old->m_state != STATE_NEW) {
            switch (p_pcb_old->m_state) {
            case STATE_RUN:
            case STATE_READY:
                p_pcb_old->m_state = STATE_READY;
                break;
            case STATE_BLOCKED_MEMORY:
            case STATE_BLOCKED_MSG:
                // Don't set state to STATE_READY
                break;
            case STATE_NEW:
                logln("process_switch: process has state STATE_NEW but shouldn't");
                break;
            default:
                logln("process_switch: unknown state");
                break;
            };

            p_pcb_old->mp_sp = (U32*) __get_MSP();
        }

        gp_current_process->m_state = STATE_RUN;
        __set_MSP((U32) gp_current_process->mp_sp);

        // for some reason, we must call __new_special_proc_rte for the KCD and CRT procs too
        if (gp_current_process->m_pid == 0 || (gp_current_process->m_pid > 6 && gp_current_process->m_pid != PID_CLOCK)) {
            __new_special_proc_rte();
        } else {
            __rte();  // pop exception stack frame from the stack for a new processes
        }
    }

    /* The following will only execute if the if block above is FALSE */
    if (gp_current_process != p_pcb_old) {
        if (state == STATE_READY) {
            switch (p_pcb_old->m_state) {
            case STATE_RUN:
            case STATE_READY:
                p_pcb_old->m_state = STATE_READY;
                break;
            case STATE_BLOCKED_MEMORY:
            case STATE_BLOCKED_MSG:
                // Don't set state to STATE_READY
                break;
            case STATE_NEW:
                logln("process_switch: process has state STATE_NEW but shouldn't");
                break;
            default:
                logln("process_switch: unknown state");
                break;
            };

            p_pcb_old->mp_sp = (U32*) __get_MSP();  // save the old process's sp
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
    PCB* p_pcb_old = gp_current_process;
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

    process = pq_pop_PCB_ready(gp_pcbs[process_id]);
    if (process == NULL) process = pq_pop_PCB_blocked(gp_pcbs[process_id]);

    process->m_priority = priority;
    switch (process->m_state) {
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
        logln("k_set_process_priority: process has state STATE_RUN but is not current running process");
        break;
    default:
        logln("k_set_process_priority: unknown state");
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

int k_send_message(int process_id, MSG_BUF* message) {
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

int k_delayed_send(int process_id, void* message_envelope, int delay) {
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
void* k_receive_message(int* sender_id) {
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

void k_set_timer_interrupt_pending() {
    timer_i_proc_pending = 1;
}

void k_set_uart_interrupt_pending() {
    uart_i_proc_pending = 1;
}
