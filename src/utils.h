#ifndef UTILS_H
#define UTILS_H

#ifdef DEBUG_0
    #include "printf.h"

    #define log(format, args...) printf(format, ## args);
    #define logln(format, args...) log(format, ## args); printf("\n");
#else
    #define log(format, args...)
    #define logln(format, args...)
#endif // DEBUG_0

int ctoi(char);
char itoc(int);

#endif // UTILS_H
