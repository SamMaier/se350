#include <LPC17xx.H>
#include "stress_proc.h"
#include <string.h>
#include "rtx.h"
#include "uart_polling.h"
#include "utils.h"

extern PROC_INIT g_proc_table[];

void set_stress_procs() {
    g_proc_table[PID_A].m_pid        = PID_A;
    g_proc_table[PID_A].m_priority   = MEDIUM;
    g_proc_table[PID_A].m_stack_size = 0x100;
    g_proc_table[PID_A].mpf_start_pc = &procA;

    g_proc_table[PID_B].m_pid        = PID_B;
    g_proc_table[PID_B].m_priority   = MEDIUM;
    g_proc_table[PID_B].m_stack_size = 0x100;
    g_proc_table[PID_B].mpf_start_pc = &procB;

    g_proc_table[PID_C].m_pid        = PID_C;
    g_proc_table[PID_C].m_priority   = MEDIUM;
    g_proc_table[PID_C].m_stack_size = 0x100;
    g_proc_table[PID_C].mpf_start_pc = &procC;
}

void procA() {
    MSG_BUF* cmd = request_memory_block();
    int num = 0;

    cmd->mtype = KCD_REG;
    strcpy(cmd->mtext, "%Z");
    logln("Registering for command %Z");
    send_message(PID_KCD, cmd);

    while (1) {
        int sender = 123321;
        cmd = receive_message(&sender);

        if (sender == PID_KCD && cmd->mtext[1] == 'Z') {
            release_memory_block(cmd);
            break;
        } else {
            logln("Command %Z did not get called");
            release_memory_block(cmd);
        }
    }

    while (1) {
        MSG_BUF* msg = (MSG_BUF*) request_memory_block();
        msg->mtype = COUNT_REPORT;
        msg->m_kdata[0] = num;
        send_message(PID_B, msg);
        num++;

        release_processor();
    }
}

void procB() {
    MSG_BUF* msg;

    while (1) {
        msg = (MSG_BUF*) receive_message(NULL);
        send_message(PID_C, msg);
    }
}

void procC() {
    MSG_BUF* front = NULL;
    MSG_BUF* back = NULL;
    MSG_BUF* p;
    MSG_BUF* q;

    while (1) {
        if (front == NULL) {
            p = receive_message(NULL);
        } else {
            // dequeue p from the local queue
            p = front;
            front = front->mp_next;
            if (front == NULL) {
                back = NULL;
            }
            p->mp_next = NULL;
        }

        if (p->mtype == COUNT_REPORT) {
            if (p->m_kdata[0] % 20 == 0) {
                p->mtext[0] = 'P';
                p->mtext[1] = 'r';
                p->mtext[2] = 'o';
                p->mtext[3] = 'c';
                p->mtext[4] = 'e';
                p->mtext[5] = 's';
                p->mtext[6] = 's';
                p->mtext[7] = ' ';
                p->mtext[8] = 'C';
                p->mtext[9] = '\r';
                p->mtext[10] = '\n';
                p->mtext[11] = '\0';
                p->mtype = CRT_DISPLAY;
                send_message(PID_CRT, p);

                // hibernate
                q = (MSG_BUF*) request_memory_block();
                q->mtype = WAKEUP_10;
                delayed_send(PID_C, q, ONE_SECOND * 10);

                while (1) {
                    p = receive_message(NULL);
                    if (p->mtype == WAKEUP_10) {
                        break;
                    } else {
                        // push p to the local queue
                        if (front == NULL) {
                            front = p;
                            back = p;
                        } else {
                            back->mp_next = p;
                            back = p;
                        }
                    }
                }
            }
        }

        release_memory_block(p);
        release_processor();
    }
}
