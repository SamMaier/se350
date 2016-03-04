/**
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */

#ifndef K_RTX_H_
#define K_RTX_H_

/*----- Definitations -----*/
#define DEFAULT 0
#define KCD_REG 1

#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0
#define NUM_TEST_PROCS 6
#define NUM_SYS_PROCS 1
#define NUM_PROCS NUM_TEST_PROCS + NUM_SYS_PROCS

/* Process Priority. The bigger the number is, the lower the priority is*/
#define HIGH    0
#define MEDIUM  1
#define LOW     2
#define LOWEST  3
#define HIDDEN  4

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#endif /* DEBUG_0 */

/*----- Types -----*/
typedef unsigned char U8;
typedef unsigned int U32;

/* process states */
typedef enum {NEW = 0, RDY, RUN, BLOCKED_ON_MEMORY, BLOCKED_ON_MSG_RECEIVE} PROC_STATE_E;


#define K_MSG_ENV
/*
  Message envelope structure defintion.
  Not sure why we were told to put some of the stuff in the #ifdef,
  going to pretend like it's always defined.
*/
typedef struct msgbuf {
#ifdef K_MSG_ENV
    void *mp_next; /* pointer for queue towards front of queue */
    void *mp_prev; /* pointer for queue towards back of queue */
    int m_send_id; /* int process ID of sending process */
    int m_recv_id; /* int process ID of receiving process */
    int m_kdata[5];/* other spot for data.
                      Unused right now - not sure why it is suggested. */
#endif
    int mtype;     /* DEFAULT (normal) or KCD_REG (register key command) */
    char mtext[1]; /* Array of characters for message.
                      I'm not sure why they say it is size one.*/
} MSGBUF;


/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project
*/
typedef struct pcb
{
	struct pcb *mp_next;
	U32 *mp_sp;		/* stack pointer of the process */
	U32 m_pid;		/* process id */
	PROC_STATE_E m_state;   /* state of the process */
	U8 m_priority; /* process priority */
    MSGBUF *m_message_queue_front; /* the first element of the message queue */
    MSGBUF *m_message_queue_back; /* the last element of the message queue */
} PCB;


/* initialization table item */
typedef struct proc_init
{
	int m_pid;	        /* process id */
	int m_priority;         /* initial priority, not used in this example. */
	int m_stack_size;       /* size of stack in words */
	void (*mpf_start_pc) ();/* entry point of the process */
} PROC_INIT;

#endif // ! K_RTX_H_
