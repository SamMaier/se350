/**
 * @file:   k_sys_proc.c
 * @brief:  System processes: null process
 */

#include "k_rtx.h"
#include "k_sys_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

extern int k_release_processor(void);
extern int get_sys_time(void);

/* initialization table */
PROC_INIT g_sys_procs[NUM_SYS_PROCS];

/* timer */
MSG* timeout_queue_front = NULL;
MSG* timeout_queue_back = NULL;

void set_sys_procs() {
    /* null process */
    g_sys_procs[0].m_pid = 0;
    g_sys_procs[0].m_priority = HIDDEN;
    g_sys_procs[0].m_stack_size = 0x100;
    g_sys_procs[0].mpf_start_pc = &null_process;

    /* timer interrupt process */
    g_sys_procs[1].m_pid = 1;
    g_sys_procs[1].m_priority = HIGH;
    g_sys_procs[1].m_stack_size = 0x100;
    g_sys_procs[1].mpf_start_pc = &timer_i_process;
}

void null_process() {
    while (1) {
        k_release_processor();
    }
}

void enqueue_message_delayed(PCB* pcb, MSG* message, int delay) {
    MSG* currentMessage;
    MSG* nextMessage;

    int expiry = get_sys_time() + delay;
    message->m_expiry = expiry;

    currentMessage = timeout_queue_front;

    if (currentMessage == NULL) {
        // no messages in the queue
        message->mp_next = NULL;
        message->mp_prev = NULL;

        timeout_queue_front = message;
        timeout_queue_back = message;
    } else if (timeout_queue_front == timeout_queue_back) {
        // one message in the queue
        if (currentMessage->m_expiry <= message->m_expiry) {
            // insert message after currentMessage
            currentMessage->mp_next = message;
            message->mp_prev = currentMessage;

            timeout_queue_back = message;
        } else {
            // insert message before currentMessage
            message->mp_next = currentMessage;
            currentMessage->mp_prev = message;

            timeout_queue_front = message;
        }
    } else {
        // queue has at least 2 messages

        if (message->m_expiry <= currentMessage->m_expiry) {
            // insert message before currentMessage
            message->mp_next = currentMessage;
            currentMessage->mp_prev = message;

            timeout_queue_front = message;
        } else {
            // insert message either in the middle or at the end of the queue
            nextMessage = currentMessage->mp_next;

            while (nextMessage != NULL && nextMessage->m_expiry < message->m_expiry) {
                currentMessage = nextMessage;
                nextMessage = currentMessage->mp_next;
            }

            // insert message after currentMessage
            currentMessage->mp_next = message;
            message->mp_prev = currentMessage;

            if (nextMessage != NULL) {
                // inserting message in the middle of the queue
                message->mp_next = nextMessage;
                nextMessage->mp_prev = message;
            } else {
                // inserting message at the end of the queue
                message->mp_next = NULL;

                timeout_queue_back = message;
            }
        }
    }
}

void remove_message_delayed(MSG *message) {
    MSG* previousMessage;

    if (timeout_queue_front == NULL) {
        // queue is empty, don't do anything
    } else if (timeout_queue_front == timeout_queue_back && timeout_queue_front == message) {
        // message is the only element in the queue
        timeout_queue_front = NULL;
        timeout_queue_back = NULL;

        message->mp_next = NULL;
        message->mp_prev = NULL;
    } else {
        // queue has at least 2 elements
        if (timeout_queue_front == message) {
            // message is the first element
            timeout_queue_front = message->mp_next;

            message->mp_next = NULL;
        } else if (timeout_queue_back == message) {
            // message is the last element
            timeout_queue_back = message->mp_prev;

            message->mp_prev = NULL;
        } else {
            // message is somewhere in the middle
            previousMessage = message->mp_prev;
            previousMessage->mp_next = message->mp_next;

            message->mp_prev = NULL;
            message->mp_next = NULL;
        }
    }
}

/**
 * gets called once every millisecond
 */
void timer_i_process() {
    MSG* message = timeout_queue_front;
    U32 systemTime = get_sys_time();

    while (message != NULL) {
        if (message->m_expiry <= systemTime) {
            remove_message_delayed(message);
            k_send_message(message->m_receive_id, message);
            message = message->mp_next;
        } else {
            break;
        }
    }
}
