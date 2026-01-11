#ifndef __M_SWAP__
#define __M_SWAP__

#ifdef __cplusplus
extern "C" {
#endif

unsigned short SwapSHORT(unsigned short);
unsigned long  SwapLONG(unsigned long);

/* Pe x86 (Little Endian), nu avem nevoie de swap pentru WAD-uri (care sunt tot LE) */
#define SHORT(x)	((unsigned short)(x))
#define LONG(x)		((unsigned long)(x))

#ifdef __cplusplus
}
#endif

#endif