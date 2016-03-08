#ifndef K_RTX_H_
#define K_RTX_H_

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200 // user proc stack size 512B
#else
#define USR_SZ_STACK 0x100 // user proc stack size 218B
#endif // DEBUG_0

#endif // K_RTX_H_
