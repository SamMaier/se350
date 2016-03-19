#include "k_i_proc.h"
#include <LPC17xx.h>
#include <string.h>
#include "uart.h"
#include "uart_polling.h"
#include "k_memory.h"
#include "debug_printer.h"
#include "utils.h"

extern int k_release_processor(void);
extern int k_send_message(int, MSG_BUF*);
extern void* k_request_memory_block(void);
extern int k_release_memory_block(void*);
extern MSG_BUF* dequeue_message(PCB*);
extern void remove_message_delayed(MSG_BUF* message);

extern PROC_INIT g_proc_table[NUM_PROCS];
extern PCB** gp_pcbs;

extern U32 g_timer;
extern MSG_BUF* timeout_queue_front;

// UART interrupt globals
uint8_t g_char_in;

void set_i_procs() {
    /* timer interrupt process */
    g_proc_table[PID_TIMER_IPROC].m_pid = PID_TIMER_IPROC;
    g_proc_table[PID_TIMER_IPROC].m_priority = INTERRUPT;
    g_proc_table[PID_TIMER_IPROC].m_stack_size = 0x100;
    g_proc_table[PID_TIMER_IPROC].mpf_start_pc = &timer_i_process;

    /* UART interrupt process */
    g_proc_table[PID_UART_IPROC].m_pid = PID_UART_IPROC;
    g_proc_table[PID_UART_IPROC].m_priority = INTERRUPT;
    g_proc_table[PID_UART_IPROC].m_stack_size = 0x100;
    g_proc_table[PID_UART_IPROC].mpf_start_pc = &uart_i_process;
}

__asm push_registers() {
    PUSH{r4 - r11}
    BX LR
}

__asm pop_registers() {
    POP{r4 - r11}
    BX LR
}

// gets called once every millisecond
void timer_i_process() {
    MSG_BUF* message;

    while (1) {
        LPC_TIM0->IR = BIT(0);

        g_timer++;

        message = timeout_queue_front;

        while (message != NULL) {
            if (message->m_expiry <= g_timer) {
                remove_message_delayed(message);
                k_send_message(message->m_recv_pid, message);
                message = message->mp_next;
            } else {
                break;
            }
        }

        push_registers();
        k_release_processor();
        pop_registers();
    }
}

// gets called on input and output
void uart_i_process() {
    PCB* uart_pcb = gp_pcbs[PID_UART_IPROC];
    char buffer[MEMORY_BLOCK_SIZE - 2 - sizeof(int)];
    int isBufferFull = 0;
    int i = 0;

    while (1) {
        uint8_t IIR_IntId; // Interrupt ID from IIR
        LPC_UART_TypeDef* pUart = (LPC_UART_TypeDef*) LPC_UART0;

        if (!isBufferFull && uart_pcb->mp_msg_queue_front != NULL) {
            // Can get new message
            MSG_BUF* msg = dequeue_message(uart_pcb);
            strcpy(buffer, msg->mtext);
            isBufferFull = 1;
            i = 0;
            k_release_memory_block(msg);
            pUart->IER |= IER_THRE; // enable whatever THRE is
        }

        /* Reading IIR automatically acknowledges the interrupt */
        IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR
        if (IIR_IntId & IIR_RDA) { // Receive Data Avaialbe
            /* read UART. Read RBR will clear the interrupt */
            MSG_BUF* ptr;
            g_char_in = pUart->RBR;

#ifdef _DEBUG_HOTKEYS
            if (g_char_in == 'r') {
                print_ready_procs();
            } else if (g_char_in == 'm') {
                print_memory_blocked_procs();
            } else if (g_char_in == 's') {
                print_message_blocked_procs();
            }
#endif
            ptr = (MSG_BUF*) k_request_memory_block();

            if (ptr != NULL) {
                ptr->mtype = DEFAULT;
                ptr->mtext[0] = g_char_in;
                ptr->mtext[1] = '\0';
                k_send_message(PID_KCD, ptr);
            } else {
                logln("Out of memory in uart_i_process");
            }
        } else if (IIR_IntId & IIR_THRE) {
            /* THRE Interrupt, transmit holding register becomes empty */
            if (isBufferFull) {
                if (buffer[i] == '\0') {
                    if (uart_pcb->mp_msg_queue_front != NULL) {
                        // Can get new message
                        MSG_BUF* msg = dequeue_message(uart_pcb);
                        strcpy(buffer, msg->mtext);
                        isBufferFull = 1;
                        k_release_memory_block(msg);
                    } else {
                        isBufferFull = 0;
                        pUart->IER &= ~IER_THRE; // toggle the IER_THRE bit
                    }

                    i = 0;
                    pUart->THR = '\0';
                } else {
                    pUart->THR = buffer[i];
                    i++;
                }
            }
        } else {
            /* not implemented yet */
            logln("Should not get here!");
            return;
        }

        push_registers();
        k_release_processor();
        pop_registers();
    }
}
