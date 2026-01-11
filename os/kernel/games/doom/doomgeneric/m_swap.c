/* m_swap.c */

#include "m_swap.h"

/*
 * Swap 16bit, that is, bytes 0 and 1
 */
unsigned short SwapSHORT(unsigned short x)
{
    return (x>>8) | (x<<8);
}

/*
 * Swap 32bit, that is, bytes 0 and 3, 1 and 2
 */
unsigned long SwapLONG(unsigned long x)
{
    return (x>>24)
        | ((x>>8) & 0xff00)
        | ((x<<8) & 0xff0000)
        | (x<<24);
}