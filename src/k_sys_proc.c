/**
 * @file:   k_sys_proc.c
 * @brief:  System processes: null process
 */

#include "k_rtx.h"
#include "k_sys_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

extern int k_release_processor(void);
extern int get_sys_time(void);

/* initialization table */
PROC_INIT g_sys_procs[NUM_SYS_PROCS];

/* timer */
MSG* timeout_queue_front = NULL;
MSG* timeout_queue_back = NULL;

void set_sys_procs() {
    /* null process */
    g_sys_procs[0].m_pid = 0;
    g_sys_procs[0].m_priority = HIDDEN;
    g_sys_procs[0].m_stack_size = 0x100;
    g_sys_procs[0].mpf_start_pc = &null_process;

    /* timer interrupt process */
    g_sys_procs[1].m_pid = 1;
    g_sys_procs[1].m_priority = HIGH;
    g_sys_procs[1].m_stack_size = 0x100;
    g_sys_procs[1].mpf_start_pc = &timer_i_process;
}

void null_process() {
    while (1) {
        k_release_processor();
    }
}

/**
 * gets called once every millisecond
 */
void timer_i_process() {
    MSG* message = timeout_queue_front;

    while (message != NULL) {
        if (message->m_expiry <= get_sys_time()) {
            k_send_message(message->m_receive_id, message);
            message = message->mp_next;
        } else {
            break;
        }
    }
}

void enqueue_message_delayed(PCB* pcb, MSG* message, int delay) {
    int expiry = get_sys_time() + delay;
    MSG* temp = timeout_queue_front;
    MSG* next;

    message->m_expiry = expiry;

    // TODO

    // if (temp == NULL) {
    //     timeout_queue_front = message;
    //     timeout_queue_back = message;
    // }
    //
    // while (temp != NULL) {
    //     next = temp->mp_next;
    //     if (next == NULL) {
    //
    //     } else {
    //
    //     }
    // }
}
