/**
 * @file: usr_proc.c
 * @brief: Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date: 2014/02/28
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include <LPC17xx.H>
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
		set_process_priority(g_test_procs[0].m_pid, LOWEST);
	
    printf("G021_test: START\n");
		printf("G02_test: total 5 tests\n");
	
		g_current_test = 1;
		g_tests_passed = 0;
		
		release_processor();
	
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
		set_process_priority(g_test_procs[g_current_test].m_pid, HIGH);
	
		if (get_process_priority(g_test_procs[g_current_test].m_pid) == HIGH) {
				printf("G021_test: test %d OK\n", g_current_test);
				g_tests_passed++;
		} else {
				printf("G021_test: test %d FAIL\n", g_current_test);
		}
	
		set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);
	
		g_current_test++;
		release_processor();
	
    while (1) {
				release_processor();
		}
}

/**
 * @brief:
 */
void proc3(void) {
		void *mem_blk = request_memory_block();
	
		if (mem_blk != NULL) {
				printf("G021_test: test %d OK\n", g_current_test);
				g_tests_passed++;
		} else {
				printf("G021_test: test %d FAIL\n", g_current_test);
		}
		
		release_memory_block(mem_blk);
	
		set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);
	
		g_current_test++;
		release_processor();
	
    while (1) {
        release_processor();
    }
}

/**
 * @brief:
 */
void proc4(void) {
		void *mem_blk = request_memory_block();
	
		if (release_memory_block(mem_blk) != -1) {
				printf("G021_test: test %d OK\n", g_current_test);
				g_tests_passed++;
		} else {
				printf("G021_test: test %d FAIL\n", g_current_test);
		}
			
		set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);
	
		g_current_test++;
		release_processor();
	
    while (1) {
        release_processor();
    }
}

/**
 * @brief:
 */
void proc5(void) {
    void *mem_blk_1 = request_memory_block();
		void *mem_blk_2 = request_memory_block();
	
		if (mem_blk_1 != NULL && mem_blk_2 != NULL && mem_blk_1 != mem_blk_2) {
				printf("G021_test: test %d OK\n", g_current_test);
				g_tests_passed++;
		} else {
				printf("G021_test: test %d FAIL\n", g_current_test);
		}
		
		release_memory_block(mem_blk_1);
		release_memory_block(mem_blk_2);
	
		set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);
	
		g_current_test++;
		release_processor();
	
    while (1) {
        release_processor();
    }
}

/**
 * @brief:
 */
void proc6(void) {
		void *mem_blk;
		int i;
		int rel_val;
		int passing = 1;
	
		for (i = 0; i < 25; i++) {
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
	
		if (passing) {
				printf("G021_test: test %d OK\n", g_current_test);
				g_tests_passed++;
		} else {
				printf("G021_test: test %d FAIL\n", g_current_test);
		}
	
		set_process_priority(g_test_procs[g_current_test].m_pid, LOWEST);
	
		g_current_test++;
		release_processor();
	
    while (1) {
        release_processor();
    }
}
