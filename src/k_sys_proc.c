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

/* initialization table */
PROC_INIT g_sys_procs[NUM_SYS_PROCS];

void set_sys_procs() {
    /* null process */
    g_sys_procs[0].m_pid = 0;
    g_sys_procs[0].m_priority = HIDDEN;
    g_sys_procs[0].m_stack_size = 0x100;
    g_sys_procs[0].m_type = 0;
    g_sys_procs[0].mpf_start_pc = &null_process;

    /* keyboard command decoder process */
    g_sys_procs[1].m_pid = 1;
    g_sys_procs[1].m_priority = LOW;
    g_sys_procs[1].m_stack_size = 0x100;
    g_sys_procs[1].m_type = 0;
    g_sys_procs[1].mpf_start_pc = &kcd_process;

    /* CRT display process */
    g_sys_procs[2].m_pid = 2;
    g_sys_procs[2].m_priority = LOW;
    g_sys_procs[2].m_stack_size = 0x100;
    g_sys_procs[2].m_type = 0;
    g_sys_procs[2].mpf_start_pc = &crt_process;
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
