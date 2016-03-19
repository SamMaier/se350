#ifndef K_PROCESS_H
#define K_PROCESS_H

#include "k_rtx.h"

/* Definitions */
#define INITIAL_xPSR 0x01000000 // user process initial xPSR (Program Status Register) value

/* Functions */
void process_init(void);
PCB* scheduler(void);
int k_release_process(void);
int k_set_process_priority(const int, const int);
int k_get_process_priority(const int);
int k_send_message(int process_id, void* p_msg_envelope);
void* k_receive_message(int* sender_id);
int k_delayed_send(int process_id, void* p_msg_envelope, int delay);

extern U32* alloc_stack(U32 size_b); // allocate stack for a process
extern void __rte(void);             // pop exception stack frame
extern void set_test_procs(void);
extern void set_sys_procs(void);
extern void set_i_procs(void);

#endif // K_PROCESS_H
