#ifndef COMMON_H
#define COMMON_H

#define BOOL unsigned char
#define TRUE 1
#define FALSE 0
#define NULL 0
#define RTX_ERR -1
#define RTX_OK 0

#define NUM_PROCS 16
#define NUM_TEST_PROCS 6
#define NUM_PRIORITIES 5

/* Process IDs */
#define PID_NULL 0
#define PID_P1   1
#define PID_P2   2
#define PID_P3   3
#define PID_P4   4
#define PID_P5   5
#define PID_P6   6
#define PID_A    7
#define PID_B    8
#define PID_C    9
#define PID_SET_PRIO     10
#define PID_CLOCK        11
#define PID_KCD          12
#define PID_CRT          13
#define PID_TIMER_IPROC  14
#define PID_UART_IPROC   15

/* Process Priority. The bigger the number is, the lower the priority is */
#define HIGH      0
#define MEDIUM    1
#define LOW       2
#define LOWEST    3
#define HIDDEN    4
#define INTERRUPT 5

/* Process States */
#define STATE_NEW             0
#define STATE_READY           1
#define STATE_RUN             2
#define STATE_BLOCKED_MEMORY  3
#define STATE_BLOCKED_MSG     4

/* Message Types */
#define DEFAULT 0
#define KCD_REG 1
#define CRT_DISPLAY 2

/* Types */
typedef unsigned char U8;
typedef unsigned int U32;

/* Initialization table item, exposed to user space */
typedef struct proc_init {
    int m_pid;                   // process id
    int m_priority;              // initial priority
    int m_stack_size;            // size of stack in words
    void (*mpf_start_pc)();      // entry point of the process
} PROC_INIT;

/* Message Buffer */
typedef struct msgbuf {
#ifdef K_MSG_ENV
    void* mp_next;               // pointer to next message in queue
    int m_send_pid;              // sender pid
    int m_recv_pid;              // receiver pid
    int m_kdata[4];              // extra 16B kernel data place holder
    int m_expiry;                // expiry time in milliseconds
#endif
    int mtype;                   // user defined message type
    char mtext[1];               // body of the message
} MSG_BUF;

/* Process Control Block */
typedef struct pcb {
    struct pcb* mp_next;
    U32* mp_sp;                  // stack pointer of the process
    U32 m_pid;                   // process id
    U32 m_state;                 // state of the process
    U8 m_priority;               // process priority
    MSG_BUF* mp_msg_queue_front; // the first element of the message queue
    MSG_BUF* mp_msg_queue_back;  // the last element of the message queue
} PCB;

#endif // COMMON_H
