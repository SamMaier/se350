/**
 * @file:   k_process.h
 * @brief:  process management hearder file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
 * NOTE: Assuming there are only two user processes in the system
 */

#ifndef K_PROCESS_H_
#define K_PROCESS_H_

#include "k_rtx.h"

/* ----- Definitions ----- */

#define INITIAL_xPSR 0x01000000        /* user process initial xPSR value */

/* ----- Functions ----- */

/* initialize all procs in the system */
void process_init(void);
/* pick the pid of the next to run process */
PCB *scheduler(void);
/* kernel release_process function */
int k_release_process(void);
/* set process priority */
int k_set_process_priority(const int, const int);
/* get process priority */
int k_get_process_priority(const int);
/* Sends message to given process id and preempts if higher priority proc waiting for message*/
int send_message(int process_id, void* message_envelope);
/* Blocking recieve, sets sender_id's value to the id of the proc ID of the sender */
void *receive_message(int *sender_id);

extern U32 *alloc_stack(U32 size_b);   /* allocate stack for a process */
extern void __rte(void);               /* pop exception stack frame */
extern void set_test_procs(void);      /* test process initial set up */
extern void set_sys_procs(void);    /* system process initial set up */

#endif /* ! K_PROCESS_H_ */
