/**
 * @file:   k_sys_proc.c
 * @brief:  System processes: null process
 */
#include <string.h>
#include <LPC17xx.h>
#include "utils.h"
#include "uart.h"
#include "uart_polling.h"
#include "k_rtx.h"
#include "k_sys_proc.h"
#include "k_memory.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

extern int k_release_processor(void);
extern int k_uart_interrupt(void);
extern int k_send_message(int, MSG_BUF*);
extern int k_send_message_envelope(int, MSG_BUF*);
extern int k_set_process_priority(const int, const int);
extern void *k_receive_message(int*);
extern void *k_request_memory_block(void);
extern int k_release_memory_block(void*);
extern MSG_BUF* dequeue_message(PCB*);
extern PROC_INIT g_proc_table[NUM_PROCS];
extern PCB** gp_pcbs;
extern void print_memory_blocked_procs(void);
extern void print_message_blocked_procs(void);
extern void print_ready_procs(void);

/* timer */
extern U32 g_timer;

/* UART interrupt globals */
uint8_t g_char_in;

MSG_BUF* timeout_queue_front = NULL;

int g_KCD_REG[256];
char g_command_buffer[(MEMORY_BLOCK_SIZE - 2 - sizeof(int))];

void set_sys_procs() {
    /* null process */
    g_proc_table[PID_NULL].m_pid = PID_NULL;
    g_proc_table[PID_NULL].m_priority = HIDDEN;
    g_proc_table[PID_NULL].m_stack_size = 0x100;
    g_proc_table[PID_NULL].mpf_start_pc = &null_process;

    /* set priority process */
    g_proc_table[PID_SET_PRIO].m_pid = PID_SET_PRIO;
    g_proc_table[PID_SET_PRIO].m_priority = HIGH;
    g_proc_table[PID_SET_PRIO].m_stack_size = 0x100;
    g_proc_table[PID_SET_PRIO].mpf_start_pc = &set_priority_process;
    
    /* keyboard command decoder process */
    g_proc_table[PID_KCD].m_pid = PID_KCD;
    g_proc_table[PID_KCD].m_priority = HIGH;
    g_proc_table[PID_KCD].m_stack_size = 0x100;
    g_proc_table[PID_KCD].mpf_start_pc = &kcd_process;

    /* CRT display process */
    g_proc_table[PID_CRT].m_pid = PID_CRT;
    g_proc_table[PID_CRT].m_priority = HIGH;
    g_proc_table[PID_CRT].m_stack_size = 0x100;
    g_proc_table[PID_CRT].mpf_start_pc = &crt_process;

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

void null_process() {
    while (1) {
        k_release_processor();
    }
}

void set_priority_process(void) {
    MSG_BUF* command = k_request_memory_block();
    command->mtype = KCD_REG;
    strcpy(command->mtext, "%C");
    // TODO: this should be changed to just k_send_message once that issue's fix is merged with this patch
    k_send_message_envelope(PID_KCD, command);
    
    while (1) {
        int sender = 123;
        int proc_id = -1;
        int priority = -1;
        command = (MSG_BUF *) k_receive_message(&sender);
        
        proc_id = command->mtext[3] - '0';
        priority = command->mtext[5] - '0';
        
        // Only allowing setting of usr procs and stress test procs
        if (command->mtext[4] == ' ' && proc_id <= 9 && proc_id >= 1 && priority >= HIGH && priority <= HIDDEN) {
            k_set_process_priority(proc_id, priority);
        } else {
            printf("Error: invalid priority process arguments\r\n");
        }
        
    }
}

void kcd_process(void) {
    int buf_length = 0;
    int i;
    for (i = 0; i < 256; i++) {
        g_KCD_REG[i] = -1;
    }
    while (1) {
        int sender;
        MSG_BUF *msg = (MSG_BUF *) k_receive_message(&sender);
        if (msg->mtype == DEFAULT) {
            char char_in;
            char_in = msg->mtext[0];

            if (buf_length >= (MEMORY_BLOCK_SIZE - 2 - sizeof(int))) {
                char_in = '\r';
            } else {
                g_command_buffer[buf_length] = char_in;
                buf_length++;
            }
            if (char_in == '\r') {
                g_command_buffer[buf_length] = '\n';
                buf_length++;
                g_command_buffer[buf_length] = '\0';
                buf_length = 0;
                // Echoing back to the display
                strcpy(msg->mtext, "\r\n");
                msg->mtype = CRT_DISPLAY;
                k_send_message(PID_CRT, msg);
                if (g_command_buffer[0] == '%' && g_KCD_REG[g_command_buffer[1]] > -1) {
                    MSG_BUF* command_block = (MSG_BUF *) k_request_memory_block();
                    if (command_block != NULL) {
                        command_block->mtype = DEFAULT;
                        command_block->m_send_pid = PID_KCD;
                        command_block->m_recv_pid = g_KCD_REG[g_command_buffer[1]];
                        strcpy(command_block->mtext, g_command_buffer);
                        k_send_message(g_KCD_REG[g_command_buffer[1]], command_block);
                    } else {
                        printf("Out of memory \r\n");
                        // Ran out of memory
                    }
                }
            } else {
                msg->mtext[1] = '\0';
                msg->mtype = CRT_DISPLAY;
                k_send_message(PID_CRT, msg);
            }
        } else if (msg->mtype == KCD_REG) {
            if (msg->mtext[0] == '%') {
                g_KCD_REG[msg->mtext[1]] = sender;
            } else {
                // Register command didn't start with a %
            }
            k_release_memory_block(msg);
        } else {
            // discarding this message
        }

        k_release_processor();
    }
}

void crt_process() {
    while (1) {
        LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef*) LPC_UART0;
        MSG_BUF *msg = (MSG_BUF *) k_receive_message(NULL);

        if (msg->mtype == CRT_DISPLAY) {
            pUart->IER = IER_THRE | IER_RLS | IER_RBR;
            k_send_message(PID_UART_IPROC, msg);
        } else {
            // Doesn't make sense to get here - it should only get CRT_DISPLAY calls
            k_release_memory_block(msg);
        }

        k_release_processor();
    }
}

void insert_message_delayed(PCB* pcb, MSG_BUF* message, int delay) {
    MSG_BUF* current = timeout_queue_front;
    MSG_BUF* next;

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

void remove_message_delayed(MSG_BUF *message) {
    MSG_BUF* current = timeout_queue_front;
    MSG_BUF* previous;

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

__asm push_registers() {
    PUSH{r4-r11}
    BX LR
}

__asm pop_registers() {
    POP{r4-r11}
    BX LR
}

/**
 * gets called once every millisecond
 */
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

void uart_i_process() {
    PCB* uart_pcb = gp_pcbs[PID_UART_IPROC];
    char buffer[MEMORY_BLOCK_SIZE - 2 - sizeof(int)];
    int isBufferFull = 0;
    int i = 0;

    while (1) {
        uint8_t IIR_IntId; // Interrupt ID from IIR
        LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef*) LPC_UART0;

        if (!isBufferFull && uart_pcb->mp_msg_queue_front != NULL){
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
            MSG_BUF * ptr;
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
            ptr = (MSG_BUF *) k_request_memory_block();

            if (ptr != NULL) {
                ptr->mtype = DEFAULT;
                ptr->mtext[0] = g_char_in;
                ptr->mtext[1] = '\0';
                k_send_message(PID_KCD, ptr);
            } else {
                #ifdef DEBUG_0
                printf("Out of memory in uart_i_process\n");
                #endif
            }
        } else if (IIR_IntId & IIR_THRE) {
        /* THRE Interrupt, transmit holding register becomes empty */
            if (isBufferFull) {
                if (buffer[i] == '\0' ) {
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
        } else {  /* not implemented yet */
#ifdef DEBUG_0
            uart1_put_string("Should not get here!\n\r");
#endif // DEBUG_0
            return;
        }
        push_registers();
        k_release_processor();
        pop_registers();
    }
}
