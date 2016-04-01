#include "k_rtx_init.h"
#include "uart.h"
#include "k_memory.h"
#include "k_process.h"
#include "k_timer.h"

#ifdef DEBUG_0
    #include "uart_polling.h"
#endif

void k_rtx_init(void) {
    __disable_irq();

    /* UART */
    uart_irq_init(0);   // uart0, interrupt-driven

#ifdef DEBUG_0
    uart1_init();       // uart1, polling
#endif

    /* Kernel */
    memory_init();
    process_init();
    timer_init(0);
	  timer_init(1);

    __enable_irq();

    // start the first process
    k_release_processor();
}
