/************************************************************************
 *
 * exit.c: $Revision: 1.18 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#pragma file_attr( "libGroup=stdlib.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=__exit")
#pragma file_attr(  "libFunc=_exit")
#pragma file_attr(  "libFunc=exit")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdlib.h>
#include "xatexit.h"

#if defined(__ADSPBLACKFIN__)
extern int _exit_value;
#endif

#if defined(__ADSP21000__)
#pragma diag(push)
#pragma diag(suppress:1635:"Don't complain about alleged return")
#pragma noreturn
static inline void
__lib_prog_term_jump(int status)
{
    /* Wrapping the asm statement with the jump to __lib_prog_term
     * in this noreturn inline function instead of using the asm directly
     * ensures that the compiler does not generate code after the jump
     * when used in exit().
     */
    asm volatile (
        ".extern ___lib_prog_term;"
        "jump (pc, ___lib_prog_term);"
        :: "r0" (status)
    );
}
#pragma diag(pop)
#endif


/* On SHARC without -threads, a linkage_name pragma in stdlib.h maps exit()
 * to _exit(). (Removing that pragma would mean that existing code that
 * overwrites _exit would no longer have an effect.)
 */

#pragma noreturn
void
exit(int status)
{
    int i;
    for (i = 0; i < _SYS_ATEXIT_SLOTS; i++) {
        if (__sys_atexit_funcs[i])
            __sys_atexit_funcs[i]();
    }

#if defined(__ADSPBLACKFIN__)
    // Store exit code for use by command line simulator.
    _exit_value = status;
#endif

#if defined(__ADSP21000__)
    __lib_prog_term_jump(status);
#else
    _Exit();    
#endif
}
