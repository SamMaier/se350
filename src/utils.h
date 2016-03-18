#ifndef UTILS_H
#define UTILS_H

#ifdef DEBUG_0
    #include "printf.h"
    #include "stdarg.h"
#endif

void log(char* format, ...);
void logln(char* format, ...);

#endif
