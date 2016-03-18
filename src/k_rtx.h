#ifndef K_RTX_H
#define K_RTX_H

#include "common.h"

#ifdef DEBUG_0
    #define USR_SZ_STACK 0x200 // user proc stack size 512B
#else
    #define USR_SZ_STACK 0x100 // user proc stack size 128B
#endif // DEBUG_0

#endif // K_RTX_H
