/**
 * @file:   k_sys_proc.c
 * @brief:  System processes: null process
 */

#include <LPC17xx.h>
#include "uart.h"
#include "uart_polling.h"
#include "k_rtx.h"
#include "k_sys_proc.h"

#define BIT(X) (1<<X)

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

extern int k_release_processor(void);
extern int k_send_message(int, MSG*);
extern PROC_INIT g_proc_table[NUM_PROCS];

/* timer */
extern U32 g_timer;

MSG* timeout_queue_front = NULL;

uint8_t g_buffer[]= "You Typed a Q\n\r";
uint8_t *gp_buffer = g_buffer;
uint8_t g_send_char = 0;
uint8_t g_char_in;
uint8_t g_char_out;

void set_sys_procs() {
    /* null process */
    g_proc_table[PROC_ID_NULL].m_pid = PROC_ID_NULL;
    g_proc_table[PROC_ID_NULL].m_priority = HIDDEN;
    g_proc_table[PROC_ID_NULL].m_stack_size = 0x100;
    g_proc_table[PROC_ID_NULL].mpf_start_pc = &null_process;

    /* keyboard command decoder process */
    g_proc_table[PROC_ID_KCD].m_pid = -1; // TODO
    g_proc_table[PROC_ID_KCD].m_priority = HIGH;
    g_proc_table[PROC_ID_KCD].m_stack_size = 0x100;
    g_proc_table[PROC_ID_KCD].mpf_start_pc = &kcd_process;

    /* CRT display process */
    g_proc_table[PROC_ID_CRT].m_pid = -1; // TODO
    g_proc_table[PROC_ID_CRT].m_priority = HIGH;
    g_proc_table[PROC_ID_CRT].m_stack_size = 0x100;
    g_proc_table[PROC_ID_CRT].mpf_start_pc = &crt_process;

    /* timer interrupt process */
    g_proc_table[PROC_ID_TIMER].m_pid = PROC_ID_TIMER;
    g_proc_table[PROC_ID_TIMER].m_priority = INTERRUPT;
    g_proc_table[PROC_ID_TIMER].m_stack_size = 0x0;
    g_proc_table[PROC_ID_TIMER].mpf_start_pc = &timer_i_process;

    /* UART interrupt process */
    g_proc_table[PROC_ID_UART].m_pid = PROC_ID_UART;
    g_proc_table[PROC_ID_UART].m_priority = INTERRUPT;
    g_proc_table[PROC_ID_UART].m_stack_size = 0x0;
    g_proc_table[PROC_ID_UART].mpf_start_pc = &uart_i_process;
}

void null_process() {
    while (1) {
        k_release_processor();
    }
}

void kcd_process() {
    while (1) {
        k_release_processor();
    }
}

void crt_process() {
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
    MSG* message;

    while (1) {
        LPC_TIM0->IR = BIT(0);

        g_timer++;

        message = timeout_queue_front;

        while (message != NULL) {
            if (message->m_expiry <= g_timer) {
                remove_message_delayed(message);
                k_send_message(message->m_receive_id, message);
                message = message->mp_next;
            } else {
                break;
            }
        }

        k_release_processor();
    }
}

void uart_i_process() {
    
}
