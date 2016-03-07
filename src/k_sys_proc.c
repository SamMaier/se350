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
extern int k_send_message(int, void*);

/* initialization table */
PROC_INIT g_sys_procs[NUM_SYS_PROCS];

/* timer */
MSG* timeout_queue_front = NULL;

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

void enqueue_message_delayed(PCB* pcb, MSG* message, int delay) {
    MSG* current = timeout_queue_front;

    int expiry = get_sys_time() + delay;
    message->m_expiry = expiry;

    if (current == NULL || current->m_expiry >= expiry) {
        message->mp_next = current;
        timeout_queue_front = message;
    } else {
        while (current->mp_next != NULL && current->mp_next->m_expiry < expiry) {
            current = current->mp_next;
        }
        message->mp_next = current->mp_next;
        current->mp_next = message;
    }
}

void dequeue_message_delayed(MSG *message) {
    MSG* current = timeout_queue_front;
    MSG* previous;

    while (current != NULL) {
        if (current == message) {
            if (current == timeout_queue_front) {
                timeout_queue_front = current->mp_next;
                // deq curr
            } else {
                previous->mp_next = current->mp_next;
                // deq curr
            }
        } else {
            previous = current;
            current = current->mp_next;
        }
    }
}

/**
 * gets called once every millisecond
 */
void timer_i_process() {
    MSG* message = timeout_queue_front;
    U32 systemTime = get_sys_time();

    while (message != NULL) {
        if (message->m_expiry <= systemTime) {
            dequeue_message_delayed(message);
            k_send_message(message->m_receive_id, message);
            message = message->mp_next;
        } else {
            break;
        }
    }

    k_release_processor();
}
