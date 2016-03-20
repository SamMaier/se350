#include <LPC17xx.H>
#include "stress_proc.h"
#include <string.h>
#include "rtx.h"
#include "uart_polling.h"
#include "utils.h"

extern PROC_INIT g_proc_table[];

void set_stress_procs() {
    g_proc_table[PID_A].m_pid = PID_A;
    g_proc_table[PID_A].m_priority = HIGH;
    g_proc_table[PID_A].m_stack_size = 0x100;
    g_proc_table[PID_A].mpf_start_pc = &procA;

    g_proc_table[PID_B].m_pid = PID_B;
    g_proc_table[PID_B].m_priority = HIGH;
    g_proc_table[PID_B].m_stack_size = 0x100;
    g_proc_table[PID_B].mpf_start_pc = &procB;

    g_proc_table[PID_C].m_pid = PID_C;
    g_proc_table[PID_C].m_priority = HIGH;
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
        msg->mtext[0] = num;
        msg->m_recv_pid = PID_B;
        send_message(PID_KCD, msg);
        num++;

        release_processor();
    }
}

void procB() {

}

void procC() {

}
