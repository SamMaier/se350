#ifndef K_PROCESS_H_
#define K_PROCESS_H_

#include "k_rtx.h"

/* Definitions */
#define INITIAL_xPSR 0x01000000 // user process initial xPSR value

/* Functions */
void process_init(void);                          // initialize all procs in the system
PCB *scheduler(void);                             // pick the pid of the next to run process
int k_release_process(void);                      // kernel release_process function
int k_set_process_priority(const int, const int); // set process priority
int k_get_process_priority(const int);            // get process priority
int k_send_message(int process_id, void* message_envelope);
int k_send_message_delayed(int process_id, void* message_envelope, int delay);
void *k_receive_message(int *sender_id);

extern U32 *alloc_stack(U32 size_b); // allocate stack for a process
extern void __rte(void);             // pop exception stack frame
extern void set_test_procs(void);    // test process initial set up
extern void set_sys_procs(void);     // system process initial set up

#endif
