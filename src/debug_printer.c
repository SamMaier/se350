#include "debug_printer.h"
#include "utils.h"
#include "pq.h"

extern PROC_INIT g_proc_table[NUM_PROCS];
extern PCB** gp_pcbs;
extern PQ g_ready_pq;
extern PQ g_blocked_pq;

const char* const PRIORITY_NAMES[] = { "HIGH", "MEDIUM", "LOW", "LOWEST", "NULL" };

// Prints a priority queue
void print_queue(PQ* q) {
    int i;
    PCB* current;

    for (i = 0; i < NUM_PRIORITIES; i++) {
        logln("%s:", PRIORITY_NAMES[i]);
        current = q->front[i];
        while (current != NULL) {
            logln("\t%d", current->m_pid);
            current = current->mp_next;
        }
    }
}

void print_ready_procs() {
    logln("Processes in the Ready Queue");
    logln("----------------------------");

    print_queue(&g_ready_pq);
}

void print_memory_blocked_procs() {
    logln("Processes blocked on memory");
    logln("---------------------------");

    print_queue(&g_blocked_pq);
}

void print_message_blocked_procs() {
    int i;
    PROC_INIT current;
    PCB* currentPCB;

    logln("Processes blocked on receive");
    logln("----------------------------");

    for (i = 0; i < NUM_PROCS; i++) {
        current = g_proc_table[i];
        if (current.m_pid == -1) continue;

        currentPCB = gp_pcbs[current.m_pid];

        if (currentPCB->m_state == STATE_BLOCKED_MSG) {
            logln("\t%d (%s)", current.m_pid, PRIORITY_NAMES[current.m_priority]);
        }
    }
}
