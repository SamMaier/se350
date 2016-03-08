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
extern PROC_INIT g_proc_table[NUM_PROCS];

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

void uart_i_process() {
    while (1) {
        k_release_processor();
    }
}
