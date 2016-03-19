/**
 * @file:   k_sys_proc.h
 * @brief:  System processes header file
 */

#ifndef SYS_PROC_H_
#define SYS_PROC_H_

void set_sys_procs(void);
void set_priority_process(void);
void null_process(void);
void timer_i_process(void);
void kcd_process(void);
void crt_process(void);
void uart_i_process(void);

#endif /* SYS_PROC_H_ */
