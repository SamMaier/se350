/**
 * @file: usr_proc.c
 * @brief: Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date: 2014/02/28
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "printf.h"

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void (*g_test_proc_funcs[])(void) = {&proc1, &proc2, &proc3, &proc4, &proc5, &proc6};

int g_current_test;
int g_tests_passed;

void set_test_procs() {
    int i;
    for (i = 0; i < NUM_TEST_PROCS; i++) {
        g_test_procs[i].m_pid=(U32)(i+1);
        g_test_procs[i].m_priority=LOWEST;
        g_test_procs[i].m_stack_size=0x100;
				g_test_procs[i].mpf_start_pc = g_test_proc_funcs[i];
    }
}

/**
 * @brief: a process that prints 5x6 uppercase letters
 *         and then yields the cpu. Requests for 6
 *         blocks of memory and releases them when the
 *         6th is printed.
 */
void proc1(void) {
		int i;
	
		set_process_priority(g_test_procs[0].m_pid, LOWEST);
	
    printf("G021_test: START\n");
		printf("G02_test: total 5 tests\n");
	
		g_current_test = 1;
		g_tests_passed = 0;
	
		set_process_priority(2, MEDIUM);
	
		//for (i = 1; i < NUM_TEST_PROCS; i++, g_current_test++) {
		//		set_process_priority(g_test_procs[i].m_pid, HIGH);
		//}
	
		printf("G021_test: %d/5 OK\n", g_tests_passed);
		printf("G021_test: %d/5 FAIL\n", 5 - g_tests_passed);
		printf("G021_test: END\n");
	
		while (1) {
				release_processor();
		}
}

/**
 * @brief: a process that prints 5x6 numbers
 *         and then yields the cpu.
 */
void proc2(void) {
		printf("test %d OK\n", g_current_test);
		g_tests_passed++;
	
		set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);
	
		release_processor();
	
    while (1) {
				release_processor();
		}
}

/**
 * @brief:
 */
void proc3(void) {
    printf("test %d OK\n", g_current_test);
		g_tests_passed++;
	
		set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);
	
		release_processor();
	
    while (1) {
        release_processor();
    }
}

/**
 * @brief:
 */
void proc4(void) {
    printf("test %d OK\n", g_current_test);
		g_tests_passed++;
	
		set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);
	
		release_processor();
	
    while (1) {
        release_processor();
    }
}

/**
 * @brief:
 */
void proc5(void) {
    printf("test %d OK\n", g_current_test);
		g_tests_passed++;
	
		set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);
	
		release_processor();
	
    while (1) {
        release_processor();
    }
}

/**
 * @brief:
 */
void proc6(void) {
    printf("test %d OK\n", g_current_test);
		g_tests_passed++;
	
		set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);
	
		release_processor();
	
    while (1) {
        release_processor();
    }
}
