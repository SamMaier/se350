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
PROC_INIT g_system_procs[NUM_SYSTEM_PROCS];

void set_system_procs() {
    /* null process */
    g_system_procs[0].m_pid = 0;
    g_system_procs[0].m_priority = HIDDEN;
    g_system_procs[0].m_stack_size = 0x100;
    g_system_procs[0].mpf_start_pc = &null_process;
}

void null_process() {
    while (1) {
        k_release_processor();
    }
}
