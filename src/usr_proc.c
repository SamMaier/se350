#include <string.h>
#include <LPC17xx.H>
#include "utils.h"
#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "printf.h"

//#define SIMPLE_TESTS
//#define MEMORY_TESTS
#define MESSAGE_TESTS
//#define KCD_CRT_TESTS

extern PROC_INIT g_proc_table[];
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void (*g_test_proc_funcs[]) (void) = { &proc1, &proc2, &proc3, &proc4, &proc5, &proc6 };

int g_current_test;
int g_tests_passed;

void set_test_procs() {
    int i;
    for (i = 0; i < NUM_TEST_PROCS; i++) {
        g_test_procs[i].m_pid        = (U32) (i+1);
        g_test_procs[i].m_priority   = LOWEST;
        g_test_procs[i].m_stack_size = 0x100;
        g_test_procs[i].mpf_start_pc = g_test_proc_funcs[i];
    }

    /* wall clock process */
    g_proc_table[PID_CLOCK].m_pid        = PID_CLOCK;
    g_proc_table[PID_CLOCK].m_priority   = LOWEST;
    g_proc_table[PID_CLOCK].m_stack_size = 0x100;
    g_proc_table[PID_CLOCK].mpf_start_pc = &wall_clock_process;
}

int ctoi(char c) {
    int ret = c - '0';
    if (ret < 0 || ret > 9) return 0;
    return ret;
}

char itoc(int i) {
    return i + '0';
}

void wall_clock_print(int clock) {
    int hours = clock / 3600;
    int minutes = (clock % 3600) / 60;
    int seconds = clock % 60;

    MSG_BUF* msg = (MSG_BUF*) request_memory_block();
    msg->mtype = CRT_DISPLAY;
    msg->mtext[0] = itoc(hours / 10);
    msg->mtext[1] = itoc(hours % 10);
    msg->mtext[2] = ':';
    msg->mtext[3] = itoc(minutes / 10);
    msg->mtext[4] = itoc(minutes % 10);
    msg->mtext[5] = ':';
    msg->mtext[6] = itoc(seconds / 10);
    msg->mtext[7] = itoc(seconds % 10);
    msg->mtext[8] = '\r';
    msg->mtext[9] = '\n';
    msg->mtext[10] = '\0';

    send_message(PID_CRT, msg);
}

void wall_clock_process() {
    int clock;
    int running = 0;
    char version = 0;
    MSG_BUF* command = request_memory_block();

    command->mtype = KCD_REG;
    strcpy(command->mtext, "%W");
    send_message(PID_KCD, command);

    while (1) {
        int sender = 123;
        command = receive_message(&sender);

        switch (sender) {
        case PID_KCD:
            if (command->mtext[2] == 'R') {
                clock = 0;
                running = 1;
                version++;
                command->mtext[0] = version;
                delayed_send(PID_CLOCK, command, 1000);
                wall_clock_print(clock);
            } else if (command->mtext[2] == 'S'
                    && command->mtext[3] == ' '
                    && command->mtext[6] == ':'
                    && command->mtext[9] == ':') {
                int hours = ctoi(command->mtext[4]) * 10 + ctoi(command->mtext[5]);
                int minutes = ctoi(command->mtext[7]) * 10 + ctoi(command->mtext[8]);
                int seconds = ctoi(command->mtext[10]) * 10 + ctoi(command->mtext[11]);
                clock = 3600 * hours + 60 * minutes + seconds;
                clock %= 24 * 60 * 60;
                running = 1;
                version++;
                command->mtext[0] = version;
                delayed_send(PID_CLOCK, command, 1000);
                wall_clock_print(clock);
            } else if (command->mtext[2] == 'T') {
                running = 0;
                release_memory_block(command);
            } else {
                release_memory_block(command);
            }
            break;
        case PID_CLOCK:
            if (running) {
                clock++;
                clock %= 24 * 60 * 60;
                delayed_send(PID_CLOCK, command, 1000);
                wall_clock_print(clock);
            } else {
                release_memory_block(command);
            }
            break;
        }

        release_processor();
    }
}

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
 * @brief: A process that runs our 3 tests
 */
void proc1(void) {
    MSG_BUF* result;
    int sender = 123;
    int numTests = 3;
    int testNumber = 0;
    g_current_test = 0;
    set_process_priority(g_proc_table[1].m_pid, LOW);

    printf("G021_test: START\n");
    printf("G021_test: total %d tests\n", numTests);

    set_process_priority(g_proc_table[3].m_pid, HIGH);
    set_process_priority(g_proc_table[4].m_pid, MEDIUM);
    set_process_priority(g_proc_table[5].m_pid, HIGH);
    // Starting the sender
    g_current_test++; // Using g_current_test to see if our function is called before, during, or after the sender function
    set_process_priority(g_proc_table[2].m_pid, MEDIUM);
    g_current_test++;

    // Checking getting 3 from the mailbox
    set_process_priority(g_proc_table[6].m_pid, MEDIUM);

    for (g_current_test = 0; g_current_test < numTests; g_current_test++) {
        sender = 123;
        result = receive_message(&sender);

        if (sender == 4) {
            testNumber = 1;
        } else if (sender == 5) {
            testNumber = 2;
        } else if (sender == 6) {
            testNumber = 3;
        }

        if (result->mtext[0] == 'y') {
            printf("G021_test: test %d OK\n", testNumber);
            g_tests_passed++;
        } else {
            printf("G021_test: test %d FAIL\n", testNumber);
        }

        release_memory_block(result);
    }

    printf("G021_test: %d/%d tests OK\r\n", g_tests_passed, numTests);
    printf("G021_test: %d/%d tests FAIL\r\n", (numTests - g_tests_passed), numTests);
    printf("G021_test: END\r\n");

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
    delayed_send(g_proc_table[6].m_pid, ptr, 1000 * 3); // 3 second delay

    set_process_priority(g_proc_table[1].m_pid, HIGH);
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

    //printf("Process 3 FAILED, did not block infinitely.\n");

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
        msg->mtext[0] = 'n';
        //printf("Process 4 FAILED, did not recieve message with null int* argument\n");
    } else {
        msg->mtext[0] = 'y';
    }

    send_message(g_proc_table[1].m_pid, msg);

    //printf("Process 4 finished.\n");
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
    int pass = 1;
    MSG_BUF* msg;
    if (g_current_test != 0) {
        pass = 0;
        //printf("Process 5 FAILED, did not start at correct time before sender.\n");
    }
    msg = (MSG_BUF*) receive_message(&sender_id);
    if (g_current_test != 1) {
        pass = 0;
        //printf("Process 5 FAILED, interrupt sender.\n");
    }

    if (msg->mtext[0] != 'e' || msg->mtext[1] != 's' || msg->mtext[2] != 'e' || sender_id != g_proc_table[2].m_pid) {
        pass = 0;
        //printf("Process 5 FAILED - reception of first message incorrect.\n");
    }
    msg = (MSG_BUF*) receive_message(&sender_id);
    if (msg->mtext[0] != 'd' || msg->mtext[1] != 'o' || msg->mtext[2] != 'n' || sender_id != g_proc_table[2].m_pid) {
        pass = 0;
        //printf("Process 5 FAILED - reception of second message incorrect.\n");
    }

    if (g_current_test != 1) {
        pass = 0;
        //printf("Process 5 FAILED, did not finish before sender finished.\n");
    }

    if (pass) {
        msg->mtext[0] = 'y';
    } else {
        msg->mtext[0] = 'n';
    }

    send_message(g_proc_table[1].m_pid, msg);

    //printf("Process 5 finished.\n");
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
    int pass = 1;
    MSG_BUF* msg;
    if (g_current_test != 2) {
        pass = 0;
        //printf("Process 6 FAILED, did not recieve messages after sending finished.\n");
    }
    msg = (MSG_BUF*) receive_message(&sender_id);
    if (msg->mtext[0] != 't' || msg->mtext[1] != ' ' || msg->mtext[2] != 'f' || sender_id != g_proc_table[2].m_pid) {
        pass = 0;
        //printf("Process 6 FAILED - reception of first message incorrect.\n");
    }
    msg = (MSG_BUF*) receive_message(&sender_id);
    if (msg->mtext[0] != 'a' || msg->mtext[1] != 'i' || msg->mtext[2] != 'l' || sender_id != g_proc_table[2].m_pid) {
        pass = 0;
        //printf("Process 6 FAILED - reception of second message incorrect.\n");
    }
    msg = (MSG_BUF*) receive_message(&sender_id);
    if (msg->mtext[0] != ' ' || msg->mtext[1] != 'u' || msg->mtext[2] != 's' || sender_id != g_proc_table[2].m_pid) {
        pass = 0;
        //printf("Process 6 FAILED - reception of third message incorrect.\n");
    }

    if (pass) {
        msg->mtext[0] = 'y';
    } else {
        msg->mtext[0] = 'n';
    }

    send_message(g_proc_table[1].m_pid, msg);

    //printf("Process 6 finished.\n");
    set_process_priority(g_proc_table[6].m_pid, LOWEST);
    while (1) {
        release_processor();
    }
}

#endif

#ifdef KCD_CRT_TESTS

void proc1(void) {
    MSG_BUF *message;

    set_process_priority(1, HIGH);
    printf("Starting process 1\n");

    message = request_memory_block();
    message->mtype = KCD_REG;
    strcpy(message->mtext, "%T");
    printf("Registering for command %T\n");
    send_message(PID_KCD, message);

    while (1) {
        printf("Process 1 waiting for command...\n");
        message = receive_message(NULL);
        printf("Process 1 received command!\n");

        message->mtype = CRT_DISPLAY;
        send_message(PID_CRT, message);
        release_processor();
    }
}

void proc2(void) { while (1) { release_processor(); } }
void proc3(void) {
    MSG_BUF* message;
    set_process_priority(3, MEDIUM);

    message = request_memory_block();
    message->mtype = KCD_REG;
    strcpy(message->mtext, "%X");
    send_message(PID_KCD, message);
    message = request_memory_block();
    message->mtype = KCD_REG;
    strcpy(message->mtext, "%G");
    send_message(PID_KCD, message);
    while (1) {
        message = receive_message(NULL);
        delayed_send(1, message, 5);

        release_processor();
    }
}
void proc4(void) { while (1) { release_processor(); } }
void proc5(void) { while (1) { release_processor(); } }
void proc6(void) { while (1) { release_processor(); } }

#endif

#ifdef SIMPLE_TESTS

void proc1(void) { while (1) { printf("Process 1\n"); release_processor(); } }
void proc2(void) { while (1) { printf("Process 2\n"); release_processor(); } }
void proc3(void) { while (1) { printf("Process 3\n"); release_processor(); } }
void proc4(void) { while (1) { printf("Process 4\n"); release_processor(); } }
void proc5(void) { while (1) { printf("Process 5\n"); release_processor(); } }
void proc6(void) { while (1) { printf("Process 6\n"); release_processor(); } }

#endif
