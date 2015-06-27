/**************************************************************************
 *
 * idivulr.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: idivulr - divide an unsigned long fract value by an 
                        unsigned long fract value to give an unsigned long
                        integer result

    Synopsis:

        unsigned long int idivulr (unsigned long fract parm1,
                                   unsigned long fract parm2);

    Description:
 
        This function divides an unsigned long fract argument by an
        unsigned long fract argument to produce an unsigned long integer
        result. The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.

    Algorithm:

        Interpret the long fract bit-patterns as integers by using bitsulr to
        convert to the container type. Then use standard integer divide.

    Implementation:

    Example:
    
        #include <stdfix.h>
        unsigned long fract parm1;
        unsigned long fract parm2;
        unsigned long int result;

        result = idivulr (parm1,parm2);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=idivulr")
#pragma file_attr("libFunc=_idivulr")
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")

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
#endif

#define __DISABLE_IDIVULR 1

#include <stdint.h>
#include <stdfix.h>

extern unsigned long int 
idivulr(unsigned long fract _dividend,
        unsigned long fract _divisor) 
{
    const unsigned long fract dividend = _dividend;
    const unsigned long fract divisor = _divisor;
   
    uint_ulr_t quo;
    unsigned long int result;

    /* do a standard integer divide.
    */
    uint_ulr_t i_dividend = bitsulr(dividend);
    uint_ulr_t i_divisor = bitsulr(divisor);
    quo = i_dividend / i_divisor;
    result = (unsigned long int)quo;

    return result;
}

/* End of file */
