#include <LPC17xx.H>
#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "printf.h"

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void (*g_test_proc_funcs[]) (void) = { &proc1, &proc2, &proc3, &proc4, &proc5, &proc6 };

int g_current_test;
int g_tests_passed;

void set_test_procs() {
    int i;
    for (i = 0; i < NUM_TEST_PROCS; i++) {
        g_test_procs[i].m_pid = (U32) (i + NUM_SYS_PROCS);
        g_test_procs[i].m_priority = LOWEST;
        g_test_procs[i].m_stack_size = 0x100;
        g_test_procs[i].mpf_start_pc = g_test_proc_funcs[i];
    }
}

/**
 * @brief: A process that runs our 5 tests
 */
void proc1(void) {
    int i;
    int prev_tests_passed;
    set_process_priority(g_test_procs[0].m_pid, MEDIUM);

    printf("G021_test: START\n");
    printf("G02_test: total 5 tests\n");

    g_current_test = 1;
    g_tests_passed = 0;

    for (i = 1, prev_tests_passed = g_tests_passed; i < 6; i++, g_current_test++, prev_tests_passed = g_tests_passed) {
        set_process_priority(g_test_procs[i].m_pid, HIGH);

        if (g_tests_passed > prev_tests_passed) {
            /* test passed */
            printf("G021_test: test %d OK\n", g_current_test);
        } else {
            /* test failed */
            printf("G021_test: test %d FAIL\n", g_current_test);
        }
    }

    printf("G021_test: %d/5 OK\n", g_tests_passed);
    printf("G021_test: %d/5 FAIL\n", 5 - g_tests_passed);
    printf("G021_test: END\n");

    while (1) {
        release_processor();
    }
}

/**
 * @brief: tests set_process_priority and get_process_priority
 */
void proc2(void) {
    set_process_priority(g_test_procs[g_current_test].m_pid, HIGH);

    if (get_process_priority(g_test_procs[g_current_test].m_pid) == HIGH) g_tests_passed++;

    set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);

    while (1) {
        release_processor();
    }
}

/**
 * @brief: tests memory allocation
 */
void proc3(void) {
    void *mem_blk = request_memory_block();

    if (mem_blk != NULL) g_tests_passed++;

    release_memory_block(mem_blk);

    set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);

    while (1) {
        release_processor();
    }
}

/**
 * @brief: tests memory deallocation
 */
void proc4(void) {
    void *mem_blk = request_memory_block();

    if (release_memory_block(mem_blk) != -1) g_tests_passed++;

    set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);

    while (1) {
        release_processor();
    }
}

/**
 * @brief: tests 2 memory block allocations
 */
void proc5(void) {
    void *mem_blk_1 = request_memory_block();
    void *mem_blk_2 = request_memory_block();

    if (mem_blk_1 != NULL && mem_blk_2 != NULL && mem_blk_1 != mem_blk_2) g_tests_passed++;

    release_memory_block(mem_blk_1);
    release_memory_block(mem_blk_2);

    set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);

    while (1) {
        release_processor();
    }
}

/**
 * @brief tests repeated memory allocation and deallocation
 */
void proc6(void) {
    void *mem_blk;
    int i;
    int rel_val;
    int passing = 1;

    for (i = 0; i < 10; i++) {
        mem_blk = request_memory_block();

        if (mem_blk == NULL) {
            passing = 0;
            break;
        }

        rel_val = release_memory_block(mem_blk);

        if (rel_val == -1) {
            passing = 0;
            break;
        }
    }

    if (passing) g_tests_passed++;

    set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);

    while (1) {
        release_processor();
    }
}
