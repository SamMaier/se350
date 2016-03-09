/** 
 * @file:   k_rtx_init.c
 * @brief:  Kernel initialization C file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_rtx_init.h"
#include "uart_polling.h"
#include "uart.h"
#include "k_memory.h"
#include "k_process.h"
#include "k_timer.h"

void k_rtx_init(void)
{
    __disable_irq();
    uart_irq_init(0);   // uart0, interrupt-driven
    uart1_init();       // uart1, polling
    memory_init();
    process_init();
    timer_init(0);
    __enable_irq();

    /* start the first process */
    k_release_processor();
}
