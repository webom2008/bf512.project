/****************************************************************************
 *
 * conj_fr32.c : $Revision: 1.3.10.1 $
 *
 * (c) Copyright 2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: conj_fr32 - complex conjugate

    Synopsis:

        #include <complex.h>
        complex_fract32 conj_fr32 (complex_fract32 a);

    Description:

        The conj_fr32 function conjugates the complex input a and returns
        the result.

    Error Conditions:

        The conj_fr32 function does not return an error condition.

    Algorithm:

        res.re = a.re
        res.im = -(a.im)

    Example:

        #include <complex.h>

        complex_fract32 a = {0x40000000, 0x60000000};
        /* a.re = 0.5r, a.im = 0.75r */

        complex_fract32 res = conj_fr32(a);
        /* res.re = 0.5r (0x40000000), res.im = -0.75r (0xA0000000) */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =conj_fr32")
#pragma file_attr("libFunc  =_conj_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#endif

#if !defined(TRACE)
#pragma optimize_for_speed
#endif

#ifdef _MISRA_RULES
#pragma diag(suppress:misra_rule_6_3)
/* Suppress Rule 6.3 (advisory) whereby typedefs that indicate
** size and signedness should be used in place of the basic types.
**
** Rationale: The C/C++ manual documents the prototypes of the library
** functions using the intrinsic C data types and these prototypes must
** correspond to the actual prototypes that are defined in the header
** files. In addition, Rule 8.3 requires that the type of the result
** and the type of the parameters that are specified in a function
** definition and a function declaration are identical.
*/

#pragma diag(suppress:misra_rule_18_4)
/* Suppress Rule 18.4 (required) that states that unions shall not be used.
**
** Rationale: The use of unions is permitted so long as use of the feature
** is required in order to use the compiler built-in function to obtain
** the best possible run-time performance.
*/
#endif

#if !defined(__NO_BUILTIN)
#define __NO_BUILTIN
#endif

#include <complex.h>

extern complex_fract32
conj_fr32 (complex_fract32 _a)
{
    union _complex_fract32_union
    {
        struct complex_fract32 a;
        long long              raw;
    } c;

    c.a = _a;
    c.raw = __builtin_conj_fr32 (c.raw);
    return c.a;
}

/* End of File */
