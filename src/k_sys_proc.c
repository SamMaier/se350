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
    g_proc_table[0].m_pid = 0;
    g_proc_table[0].m_priority = HIDDEN;
    g_proc_table[0].m_stack_size = 0x100;
    g_proc_table[0].mpf_start_pc = &null_process;

    /* keyboard decoder process */
    g_proc_table[12].m_pid = -1; // TODO
    g_proc_table[12].m_priority = HIGH;
    g_proc_table[12].m_stack_size = 0x100;
    g_proc_table[12].mpf_start_pc = &kcd_process;

    /* CRT display process */
    g_proc_table[13].m_pid = -1; // TODO
    g_proc_table[13].m_priority = HIGH;
    g_proc_table[13].m_stack_size = 0x100;
    g_proc_table[13].mpf_start_pc = &crt_process;

    /* timer interrupt process */
    g_proc_table[14].m_pid = 14;
    g_proc_table[14].m_priority = INTERRUPT;
    g_proc_table[14].m_stack_size = 0x0;
    g_proc_table[14].mpf_start_pc = &timer_i_process;

    /* UART interrupt process */
    g_proc_table[15].m_pid = 15;
    g_proc_table[15].m_priority = INTERRUPT;
    g_proc_table[15].m_stack_size = 0x0;
    g_proc_table[15].mpf_start_pc = &uart_i_process;
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
    uint8_t IIR_IntId; // Interrupt ID from IIR
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef*) LPC_UART0;

    while (1) {

        /* Reading IIR automatically acknowledges the interrupt */
        IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR
        if (IIR_IntId & IIR_RDA) { // Receive Data Avaialbe
            /* read UART. Read RBR will clear the interrupt */
            g_char_in = pUart->RBR;
            g_buffer[12] = g_char_in; // nasty hack
            g_send_char = 1;

            
        } else if (IIR_IntId & IIR_THRE) {
        /* THRE Interrupt, transmit holding register becomes empty */

            if (*gp_buffer != '\0' ) {
                g_char_out = *gp_buffer;
#ifdef DEBUG_0
                // uart1_put_string("Writing a char = ");
                // uart1_put_char(g_char_out);
                // uart1_put_string("\n\r");

                // you could use the printf instead
                printf("Writing a char = %c \n\r", g_char_out);
#endif // DEBUG_0
                pUart->THR = g_char_out;
                gp_buffer++;
            } else {
#ifdef DEBUG_0
                uart1_put_string("Finish writing. Turning off IER_THRE\n\r");
#endif // DEBUG_0
                pUart->IER ^= IER_THRE; // toggle the IER_THRE bit
                pUart->THR = '\0';
                g_send_char = 0;
                gp_buffer = g_buffer;
            }
        } else {  /* not implemented yet */
#ifdef DEBUG_0
			uart1_put_string("Should not get here!\n\r");
#endif // DEBUG_0
            return;
        }
        k_release_processor();
    }
}
