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
extern void *k_request_memory_block(void);
extern MSG* dequeue_message(PCB*);
extern PROC_INIT g_proc_table[NUM_PROCS];
extern PCB** gp_pcbs;

/* timer */
extern U32 g_timer;

/* UART interrupt globals */
uint8_t g_send_char = 0;
uint8_t g_char_in;

MSG* timeout_queue_front = NULL;

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


void print_memory_blocked_procs() {
    printf("mem procs");
}

void print_message_blocked_procs() {
    printf("msg procs");
}

void print_ready_procs() {
    printf("Ready procs");
}

void uart_i_process() {
    PCB* uart_pcb = gp_pcbs[PROC_ID_UART];
	uint8_t IIR_IntId; // Interrupt ID from IIR
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef*) LPC_UART0;
    char* buffer;

    while (1) {
        if (buffer == NULL && uart_pcb->m_message_queue_front != NULL){
            // Can get new message
            MSG* msg = dequeue_message(uart_pcb);
            buffer = msg->m_text;
            pUart->IER |= IER_THRE; // enable whatever THRE is
        }
        
        /* Reading IIR automatically acknowledges the interrupt */
        IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR
        if (IIR_IntId & IIR_RDA) { // Receive Data Avaialbe
            /* read UART. Read RBR will clear the interrupt */
            struct message * ptr;
            g_char_in = pUart->RBR;
            
#define _DEBUG_HOTKEYS
#ifdef _DEBUG_HOTKEYS
            if (g_char_in == 'r') {
                print_ready_procs();
            } else if (g_char_in == 'm') {
                print_memory_blocked_procs();
            } else if (g_char_in == 's') {
                print_message_blocked_procs();
            }
#endif
            ptr = (struct message *) k_request_memory_block();
            ptr->m_type = DEFAULT;
            ptr->m_text[0] = g_char_in;
            ptr->m_text[1] = '\0';
            k_send_message(PROC_ID_KCD, ptr);

            
        } else if (IIR_IntId & IIR_THRE) {
        /* THRE Interrupt, transmit holding register becomes empty */
            if (buffer != NULL) {
                if (*buffer == '\0' ) {
                    buffer = NULL;
                    pUart->IER ^= IER_THRE; // toggle the IER_THRE bit
                    pUart->THR = '\0';
                    g_send_char = 0;
                } else {
                    pUart->THR = *buffer;
                    buffer++;
                }
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
