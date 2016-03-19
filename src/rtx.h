#ifndef RTX_H
#define RTX_H

#include "common.h"

/* ----- RTX User API ----- */
#define __SVC_0  __svc_indirect(0)

/* RTX initialization */
extern void k_rtx_init(void);
#define rtx_init() _rtx_init((U32)k_rtx_init)
extern void __SVC_0 _rtx_init(U32 p_func);

/* Processor Management */
extern int k_release_processor(void);
#define release_processor() _release_processor((U32)k_release_processor)
extern int __SVC_0 _release_processor(U32 p_func);

extern int k_get_process_priority(int process_id);
#define get_process_priority(process_id) _get_process_priority((U32)k_get_process_priority, process_id)
extern int _get_process_priority(U32 p_func, int process_id) __SVC_0;

extern int k_set_process_priority(int process_id, int priority);
#define set_process_priority(process_id, priority) _set_process_priority((U32)k_set_process_priority, process_id, priority)
extern int _set_process_priority(U32 p_func, int process_id, int priority) __SVC_0;

/* Memory Management */
extern void* k_request_memory_block(void);
#define request_memory_block() _request_memory_block((U32)k_request_memory_block)
extern void* _request_memory_block(U32 p_func) __SVC_0;

extern int k_release_memory_block(void* p_mem_blk);
#define release_memory_block(p_mem_blk) _release_memory_block((U32)k_release_memory_block, p_mem_blk)
extern int _release_memory_block(U32 p_func, void* p_mem_blk) __SVC_0;

/* Inter-process Communication Management */
extern int k_send_message(int, void*);
#define send_message(process_id, p_msg_envelope) _send_message((U32)k_send_message, process_id, p_msg_envelope)
extern int _send_message(U32 p_func, int process_id, void* p_msg_envelope) __SVC_0;

extern void* k_receive_message(int* sender_id);
#define receive_message(sender_id) _receive_message((U32)k_receive_message, sender_id)
extern void* _receive_message(U32 p_func, int* sender_id) __SVC_0;

/* Timing Service */
extern int k_delayed_send(int process_id, void* p_msg_envelope, int delay);
#define delayed_send(process_id, p_msg_envelope, delay) _delayed_send((U32)k_delayed_send, process_id, p_msg_envelope, delay)
extern int _delayed_send(U32 p_func, int process_id, void* p_msg_envelope, int delay) __SVC_0;

#endif // RTX_H
