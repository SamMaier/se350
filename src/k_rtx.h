#ifndef K_RTX_H_
#define K_RTX_H_

/* RTX status codes */
#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0
#define NUM_PROCS 16

/* Process Priority.
 * The bigger the number is, the lower the priority is
 * HIDDEN is reserved for the null process
 * INTERRUPT is reserved for interrupt processes
 */
#define HIGH      0
#define MEDIUM    1
#define LOW       2
#define LOWEST    3
#define HIDDEN    4
#define INTERRUPT 5

/* Types of message envelopes */
#define DEFAULT 0
#define KCD_REG 1
#define CRT_DISPLAY 2

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B */
#endif /* DEBUG_0 */

/* Types */
typedef unsigned int U32;
typedef unsigned char U8;

/* process states */
typedef enum { NEW = 0, READY, RUN, BLOCKED_ON_MEMORY, BLOCKED_ON_MSG_RECEIVE } PROC_STATE_E;

/* Message envelope structure defintion. */
#define K_MSG_ENV
typedef struct message {
#ifdef K_MSG_ENV
    void *mp_next;    // pointer for queue towards front of queue
    int m_send_id;    // int process ID of sending process
    int m_receive_id; // int process ID of receiving process
    int m_data[5];    // other spot for data. Unused right now - not sure why it is suggested.
#endif
    int m_type;       // DEFAULT (normal) or KCD_REG (register key command)
    char m_text[4];   // Array of characters for message. I'm not sure why they say it is size one.
    U32 m_expiry;     // expiry time, in milliseconds
} MSG;

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project
*/
typedef struct pcb {
    struct pcb *mp_next;
    U32 *mp_sp;                 // stack pointer of the process
    U32 m_pid;                  // process id
    PROC_STATE_E m_state;       // state of the process
    U8 m_priority;              // process priority
    MSG *m_message_queue_front; // the first element of the message queue
    MSG *m_message_queue_back;  // the last element of the message queue
} PCB;


/* initialization table item */
typedef struct proc_init {
    int m_pid;               // process id
    int m_priority;          // initial priority, not used in this example
    int m_stack_size;        // size of stack in words
    void (*mpf_start_pc) (); // entry point of the process
} PROC_INIT;

#endif // ! K_RTX_H_
