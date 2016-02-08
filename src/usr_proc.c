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

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void set_test_procs() {
	int i;
	for (i = 0; i < NUM_TEST_PROCS; i++) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}

	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[3].mpf_start_pc = &proc4;
	g_test_procs[4].mpf_start_pc = &proc5;
	g_test_procs[5].mpf_start_pc = &proc6;
}

/**
 * @brief: a process that prints 5x6 uppercase letters
 *         and then yields the cpu. Requests for 6
 *         blocks of memory and releases them when the
 *         6th is printed.
 */
void proc1(void) {
	int i = 0;
	int ret_val = 10;
	int x = 0;
	void* memoryAllocs[6];

	while (1) {
		if (i != 0 && i%5 == 0) {
			uart1_put_string("\n\r");

			memoryAllocs[(i%30)/5] = request_memory_block();
			*(U32*)memoryAllocs[(i%30)/5] = (U32)i;
			*((U8*)memoryAllocs[(i%30)/5] + 127) = (U8)i;

			if (i%30 == 0) {
				// Releasing out of order for testing
				// release_memory_block(memoryAllocs[4]);
				// release_memory_block(memoryAllocs[2]);
				// release_memory_block(memoryAllocs[1]);
				// release_memory_block(memoryAllocs[0]);
				// release_memory_block(memoryAllocs[5]);
				// release_memory_block(memoryAllocs[3]);

				ret_val = release_processor();
#ifdef DEBUG_0
				printf("proc1: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
			}
			for ( x = 0; x < 5000; x++); // some artifical delay
		}
		uart1_put_char('A' + i%26);
		i++;
	}
}

/**
 * @brief: a process that prints 5x6 numbers
 *         and then yields the cpu.
 */
void proc2(void) {
	int i = 0;
	int ret_val = 20;
	int x = 0;
	while (1) {
		if (i != 0 && i%5 == 0) {
			uart1_put_string("\n\r");

			if (i%30 == 0) {
				ret_val = release_processor();
#ifdef DEBUG_0
				printf("proc2: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
			}
			for (x = 0; x < 5000; x++); // some artifical delay
		}
		uart1_put_char('0' + i%10);
		i++;
	}
}

/**
 * @brief:
 */
void proc3(void) {
	int ret_val = 30;
	while (1) {
		ret_val = release_processor();
	}
}

/**
 * @brief:
 */
void proc4(void) {
	int ret_val = 40;
	while (1) {
		ret_val = release_processor();
	}
}

/**
 * @brief:
 */
void proc5(void) {
	int ret_val = 50;
	while (1) {
		ret_val = release_processor();
	}
}

/**
 * @brief:
 */
void proc6(void) {
	int ret_val = 60;
	while (1) {
		ret_val = release_processor();
	}
}
