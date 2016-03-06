/* @brief: rtx.h User API prototype, this is only an example
 * @author: Yiqing Huang
 * @date: 2014/01/17
 */
#ifndef RTX_H_
#define RTX_H_

/* ----- Definitations ----- */
#define RTX_ERR -1
#define NULL 0
#define NUM_TEST_PROCS 6
#define NUM_SYS_PROCS 1
/* Process Priority. The bigger the number is, the lower the priority is*/
#define HIGH    0
#define MEDIUM  1
#define LOW     2
#define LOWEST  3
#define HIDDEN  4
/* Types of message envelopes */
#define DEFAULT 0
#define KCD_REG 1

/* ----- Types ----- */
typedef unsigned int U32;
typedef unsigned char U8;

/* initialization table item */
typedef struct proc_init
{
	int m_pid;	        /* process id */
	int m_priority;         /* initial priority, not used in this example. */
	int m_stack_size;       /* size of stack in words */
	void (*mpf_start_pc) ();/* entry point of the process */
} PROC_INIT;

/* message envelope object */
typedef struct message {
    void *mp_next; // pointer for queue towards front of queue
    void *mp_prev; // pointer for queue towards back of queue
    int m_send_id; // int process ID of sending process
    int m_receive_id; // int process ID of receiving process
    int m_data[5]; // other spot for data. Unused right now - not sure why it is suggested.
    int m_type; // DEFAULT (normal) or KCD_REG (register key command)
    char m_text[4]; // Array of characters for message. I'm not sure why they say it is size one.
} MSG;

/* ----- RTX User API ----- */
#define __SVC_0  __svc_indirect(0)

extern int k_send_message(int, void*);
#define send_message(process_id, message_envelope) _send_message((U32)k_send_message, process_id, message_envelope)
extern int *_send_message(U32 p_func, int process_id, void* message_envelope) __SVC_0;

extern void *k_receive_message(int*);
#define receive_message(sender_id) _receive_message((U32)k_receive_message, sender_id)
extern void *_receive_message(U32 p_func, int *sender_id) __SVC_0;

extern void k_rtx_init(void);
#define rtx_init() _rtx_init((U32)k_rtx_init)
extern void __SVC_0 _rtx_init(U32 p_func);

extern int k_release_processor(void);
#define release_processor() _release_processor((U32)k_release_processor)
extern int __SVC_0 _release_processor(U32 p_func);

extern void *k_request_memory_block(void);
#define request_memory_block() _request_memory_block((U32)k_request_memory_block)
extern void *_request_memory_block(U32 p_func) __SVC_0;
/* __SVC_0 can also be put at the end of the function declaration */

extern int k_release_memory_block(void *);
#define release_memory_block(p_mem_blk) _release_memory_block((U32)k_release_memory_block, p_mem_blk)
extern int _release_memory_block(U32 p_func, void *p_mem_blk) __SVC_0;

extern int k_set_process_priority(int, int);
#define set_process_priority(process_id, priority) _set_process_priority((U32)k_set_process_priority, process_id, priority)
extern int _set_process_priority(U32 p_func, int process_id, int priority) __SVC_0;

extern int k_get_process_priority(int);
#define get_process_priority(process_id) _get_process_priority((U32)k_get_process_priority, process_id)
extern int _get_process_priority(U32 p_func, int process_id) __SVC_0;


#endif /* !RTX_H_ */
