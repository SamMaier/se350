/**
 * @file:   k_sys_proc.c
 * @brief:  System processes: null process
 */

#include <LPC17xx.h>
#include "k_rtx.h"
#include "k_sys_proc.h"

#define BIT(X) (1<<X)

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

extern int k_release_processor(void);
extern int k_send_message(int, void*);

/* initialization table */
PROC_INIT g_sys_procs[NUM_SYS_PROCS];

/* timer */
extern U32 g_timer;

MSG* timeout_queue_front = NULL;

void set_sys_procs() {
    /* null process */
    g_sys_procs[0].m_pid = 0;
    g_sys_procs[0].m_priority = HIDDEN;
    g_sys_procs[0].m_stack_size = 0x100;
    g_sys_procs[0].mpf_start_pc = &null_process;

    /* timer interrupt process */
    g_sys_procs[1].m_pid = 1;
    g_sys_procs[1].m_priority = INTERRUPT;
    g_sys_procs[1].m_stack_size = 0x0;
    g_sys_procs[1].mpf_start_pc = &timer_i_process;
}

void null_process() {
    while (1) {
        k_release_processor();
    }
}

void insert_message_delayed(PCB* pcb, MSG* message, int delay) {
    MSG* current = timeout_queue_front;
    MSG* next;

    int expiry = g_timer + delay;
    message->m_expiry = expiry;

    if (current == NULL || current->m_expiry >= expiry) {
        message->mp_next = current;
        timeout_queue_front = message;
    } else {
        next = current->mp_next;
        while (next != NULL && next->m_expiry < expiry) {
            current = next;
            next = current->mp_next;
        }
        message->mp_next = current->mp_next;
        current->mp_next = message;
    }
}

void remove_message_delayed(MSG *message) {
    MSG* current = timeout_queue_front;
    MSG* previous;

    while (current != NULL) {
        if (current == message) {
            if (current == timeout_queue_front) {
                // found element at front
                timeout_queue_front = NULL;
                return;
            } else {
                // found element, not at front
                previous->mp_next = current->mp_next;
                return;
            }
        } else {
            // didn't find element
            previous = current;
            current = current->mp_next;
        }
    }
}

/**
 * gets called once every millisecond
 */
void timer_i_process() {
    while (1) {
        LPC_TIM0->IR = BIT(0);

        g_timer++;

        k_release_processor();

        // MSG* message = timeout_queue_front;
        //
        // while (message != NULL) {
        //     if (message->m_expiry <= g_timer) {
        //         remove_message_delayed(message);
        //         k_send_message(message->m_receive_id, message);
        //         message = message->mp_next;
        //     } else {
        //         break;
        //     }
        // }
    }
}

void kcd_process() {}

void crt_process() {}
