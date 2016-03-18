#include "utils.h"

void log(char* format, ...) {
#ifndef DEBUG_0
    return;
#endif

    va_list args;
    va_start(args, format);

    printf(format, args);

    va_end(args);
}

void logln(char* format, ...) {
#ifndef DEBUG_0
    return;
#endif

    va_list args;
    va_start(args, format);

    log(format, args);
    printf("\n");

    va_end(args);
}
