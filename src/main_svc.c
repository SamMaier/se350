#include <LPC17xx.h>
#include <system_LPC17xx.h>

#include "rtx.h"

#ifdef DEBUG_0
    #include "uart_polling.h"
    #include "printf.h"
#endif

int main() {
    // CMSIS system initialization
    SystemInit();

#ifdef DEBUG_0
    /* Since the standard C library functions are not allowed, a small printf function is provided that writes to
     * UART0 when DEBUG_0 is defined. To initialize it, we call init_printf */
    init_printf(NULL, putc);
#endif

    // start the RTX and built-in processes
    rtx_init();

    // We should never reach here!!!
    return RTX_ERR;
}
