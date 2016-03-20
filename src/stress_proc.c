#include <LPC17xx.H>
#include <string.h>
#include "rtx.h"
#include "uart_polling.h"
#include "stress_proc.h"
#include "utils.h"

extern PROC_INIT g_proc_table[];
PROC_INIT g_stress_procs[NUM_STRESS_TEST_PROCS];

void (*g_stress_procs[])(void) = { &procA, &procB, &procC };

void set_stress_test_procs() {
    g_stress_procs[PID_A].m_pid = PID_A;
    g_stress_procs[PID_A].m_priority = HIGH;
    g_stress_procs[PID_A].m_stack_size = 0x100;
    g_stress_procs[PID_A].mpf_start_pc = &procA;

    g_stress_procs[PID_B].m_pid = PID_B;
    g_stress_procs[PID_B].m_priority = HIGH;
    g_stress_procs[PID_B].m_stack_size = 0x100;
    g_stress_procs[PID_B].mpf_start_pc = &procB;

    g_stress_procs[PID_C].m_pid = PID_C;
    g_stress_procs[PID_C].m_priority = HIGH;
    g_stress_procs[PID_C].m_stack_size = 0x100;
    g_stress_procs[PID_C].mpf_start_pc = &procC;
}

void procA(void) {
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

void procB(void) {

}

void procC(void) {

}
