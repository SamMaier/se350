#include <LPC17xx.H>
#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "printf.h"

//#define SIMPLE_TESTS
//#define MEMORY_TESTS
#define MESSAGE_TESTS

extern PROC_INIT g_proc_table[];

void (*g_test_proc_funcs[]) (void) = { &proc1, &proc2, &proc3, &proc4, &proc5, &proc6 };

int g_current_test;
int g_tests_passed;

void set_test_procs() {
    int i;
    for (i = 0; i < NUM_TEST_PROCS; i++) {
        g_proc_table[i+1].m_pid        = (U32) (i+1);
        g_proc_table[i+1].m_priority   = LOWEST;
        g_proc_table[i+1].m_stack_size = 0x100;
        g_proc_table[i+1].mpf_start_pc = g_test_proc_funcs[i];
    }

    /* wall clock process */
    g_proc_table[PID_CLOCK].m_pid        = -1; // TODO
    g_proc_table[PID_CLOCK].m_priority   = LOWEST;
    g_proc_table[PID_CLOCK].m_stack_size = 0x100;
    g_proc_table[PID_CLOCK].mpf_start_pc = &wall_clock_process;
}

void wall_clock_process() {}

#ifdef MEMORY_TESTS
/**
 * @brief: A process that runs our 5 tests
 */
void proc1(void) {
    int i;
    int prev_tests_passed;
    set_process_priority(g_proc_table[1].m_pid, MEDIUM);

    printf("G021_test: START\n");
    printf("G02_test: total 5 tests\n");

    g_current_test = 2;
    g_tests_passed = 0;

    for (i = 2, prev_tests_passed = g_tests_passed; i <= NUM_TEST_PROCS; i++, g_current_test++, prev_tests_passed = g_tests_passed) {
        set_process_priority(g_proc_table[i].m_pid, HIGH);

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
    set_process_priority(g_proc_table[g_current_test].m_pid, HIGH);

    if (get_process_priority(g_proc_table[g_current_test].m_pid) == HIGH) g_tests_passed++;

    set_process_priority(g_proc_table[g_current_test].m_pid, LOWEST);

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

    set_process_priority(g_proc_table[g_current_test].m_pid, LOWEST);

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

    set_process_priority(g_proc_table[g_current_test].m_pid, LOWEST);

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

    set_process_priority(g_proc_table[g_current_test].m_pid, LOWEST);

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

    set_process_priority(g_proc_table[g_current_test].m_pid, LOWEST);

    while (1) {
        release_processor();
    }
}
#endif

#ifdef MESSAGE_TESTS
/**
 * @brief: A process that runs our 5 tests
 */
void proc1(void) {
    g_current_test = 0;
    set_process_priority(g_proc_table[1].m_pid, LOW);

    printf("G021_test: START\n");
    printf("Tests will explicitly print out failure. No printout = no failure.\n");

    set_process_priority(g_proc_table[3].m_pid, HIGH);
    set_process_priority(g_proc_table[4].m_pid, MEDIUM);
    set_process_priority(g_proc_table[5].m_pid, HIGH);
    // Starting the sender
    g_current_test++; // Using g_current_test to see if our function is called before, during, or after the sender function
    set_process_priority(g_proc_table[2].m_pid, MEDIUM);
    g_current_test++;

    // Checking getting 3 from the mailbox
    set_process_priority(g_proc_table[6].m_pid, MEDIUM);

    printf("Tests finished!\n");
    while (1) {
        release_processor();
    }
}

/**
 * @brief: sends no message to 3, one to 4, two to 5, and three to 6
 */
void proc2(void) {
    MSG_BUF* ptr;
    ptr = (MSG_BUF*) request_memory_block();
    ptr->mtype = DEFAULT;
    ptr->mtext[0] = 'p';
    ptr->mtext[1] = 'l';
    ptr->mtext[2] = 'e';
    send_message(g_proc_table[4].m_pid, ptr);

    ptr = (MSG_BUF*) request_memory_block();
    ptr->mtype = DEFAULT;
    ptr->mtext[0] = 'e';
    ptr->mtext[1] = 's';
    ptr->mtext[2] = 'e';
    send_message(g_proc_table[5].m_pid, ptr);
    ptr = (MSG_BUF*) request_memory_block();
    ptr->mtype = DEFAULT;
    ptr->mtext[0] = 'd';
    ptr->mtext[1] = 'o';
    ptr->mtext[2] = 'n';
    send_message(g_proc_table[5].m_pid, ptr);

    ptr = (MSG_BUF*) request_memory_block();
    ptr->mtype = DEFAULT;
    ptr->mtext[0] = 't';
    ptr->mtext[1] = ' ';
    ptr->mtext[2] = 'f';
    send_message(g_proc_table[6].m_pid, ptr);
    ptr = (MSG_BUF*) request_memory_block();
    ptr->mtype = DEFAULT;
    ptr->mtext[0] = 'a';
    ptr->mtext[1] = 'i';
    ptr->mtext[2] = 'l';
    send_message(g_proc_table[6].m_pid, ptr);
    ptr = (MSG_BUF*) request_memory_block();
    ptr->mtype = DEFAULT;
    ptr->mtext[0] = ' ';
    ptr->mtext[1] = 'u';
    ptr->mtext[2] = 's';
    // send_message(g_proc_table[6].m_pid, ptr);
    send_message_delayed(g_proc_table[6].m_pid, ptr, 50);

    set_process_priority(g_proc_table[2].m_pid, LOWEST);
    while (1) {
        release_processor();
    }
}

/**
 * @brief: tests infinte block on receive
 */
void proc3(void) {
    void *msg = receive_message(NULL);

    release_memory_block(msg);

    printf("Process 3 FAILED, did not block infinitely.\n");

    while (1) {
        release_processor();
    }
}

/**
 * @brief: tests recieve with NULL int* argument
 */
void proc4(void) {
    MSG_BUF* msg = (MSG_BUF*) receive_message(NULL);

    if (msg->mtext[0] != 'p' || msg->mtext[1] != 'l' || msg->mtext[2] != 'e' || msg->mtype != DEFAULT) {
        printf("Process 4 FAILED, did not recieve message with null int* argument\n");
    }

    release_memory_block(msg);

    printf("Process 4 finished.\n");
    set_process_priority(g_proc_table[4].m_pid, LOWEST);
    while (1) {
        release_processor();
    }
}

/**
 * @brief: tests reception of 2 messages. Should interrupt sending process when it sends them.
 */
void proc5(void) {
    int sender_id = 354354;
    MSG_BUF* msg;
    if (g_current_test != 0) {
        printf("Process 5 FAILED, did not start at correct time before sender.\n");
    }
    msg = (MSG_BUF*) receive_message(&sender_id);
    if (g_current_test != 1) {
        printf("Process 5 FAILED, interrupt sender.\n");
    }

    if (msg->mtext[0] != 'e' || msg->mtext[1] != 's' || msg->mtext[2] != 'e' || sender_id != g_proc_table[2].m_pid) {
        printf("Process 5 FAILED - reception of first message incorrect.\n");
    }
    msg = (MSG_BUF*) receive_message(&sender_id);
    if (msg->mtext[0] != 'd' || msg->mtext[1] != 'o' || msg->mtext[2] != 'n' || sender_id != g_proc_table[2].m_pid) {
        printf("Process 5 FAILED - reception of second message incorrect.\n");
    }

    release_memory_block(msg);

    if (g_current_test != 1) {
        printf("Process 5 FAILED, did not finish before sender finished.\n");
    }

    printf("Process 5 finished.\n");
    set_process_priority(g_proc_table[5].m_pid, LOWEST);
    while (1) {
        release_processor();
    }
}

/**
 * @brief: tests reception of 3 messages. All 3 should be waiting.
 */
void proc6(void) {
    int sender_id = 1231231231;
    MSG_BUF* msg;
    if (g_current_test != 2) {
        printf("Process 6 FAILED, did not recieve messages after sending finished.\n");
    }
    msg = (MSG_BUF*) receive_message(&sender_id);
    if (msg->mtext[0] != 't' || msg->mtext[1] != ' ' || msg->mtext[2] != 'f' || sender_id != g_proc_table[2].m_pid) {
        printf("Process 6 FAILED - reception of first message incorrect.\n");
    }
    msg = (MSG_BUF*) receive_message(&sender_id);
    if (msg->mtext[0] != 'a' || msg->mtext[1] != 'i' || msg->mtext[2] != 'l' || sender_id != g_proc_table[2].m_pid) {
        printf("Process 6 FAILED - reception of second message incorrect.\n");
    }
    msg = (MSG_BUF*) receive_message(&sender_id);
    if (msg->mtext[0] != ' ' || msg->mtext[1] != 'u' || msg->mtext[2] != 's' || sender_id != g_proc_table[2].m_pid) {
        printf("Process 6 FAILED - reception of third message incorrect.\n");
    }

    release_memory_block(msg);

    printf("Process 6 finished.\n");
    set_process_priority(g_proc_table[6].m_pid, LOWEST);
    while (1) {
        release_processor();
    }
}

#endif

#ifdef SIMPLE_TESTS

void proc1(void) { while (1) { printf("Process 1\n"); release_processor(); } }
void proc2(void) { while (1) { printf("Process 2\n"); release_processor(); } }
void proc3(void) { while (1) { printf("Process 3\n"); release_processor(); } }
void proc4(void) { while (1) { printf("Process 4\n"); release_processor(); } }
void proc5(void) { while (1) { printf("Process 5\n"); release_processor(); } }
void proc6(void) { while (1) { printf("Process 6\n"); release_processor(); } }

#endif
