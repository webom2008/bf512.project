/**************************************************************************
 *
 * idivlr.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: idivlr - divide a long fract value by a 
                       long fract value to give a long integer result

    Synopsis:

        long int idivlr (long fract parm1, long fract parm2);

    Description:
 
        This function divides a long fract argument by a
        long fract argument to produce a long integer result.
        The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.

    Algorithm:

        Interpret the long fract bit-patterns as integers by using bitslr to
        convert to the container type. Then use standard integer divide.
        Be careful of LFRACT_MIN / 0xffff, for which the integer divide may
        return an undefined result.

    Implementation:

    Example:
    
        #include <stdfix.h>
        long fract parm1;
        long fract parm2;
        long int result;

        result = idivlr (parm1,parm2);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=idivlr")
#pragma file_attr("libFunc=_idivlr")
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

#define __DISABLE_IDIVLR 1

#include <stdint.h>
#include <stdfix.h>

extern long int 
idivlr(long fract _dividend,
       long fract _divisor) 
{
    const long fract dividend = _dividend;
    const long fract divisor = _divisor;
   
    int_lr_t quo;
    long int result;

    if ((dividend == LFRACT_MIN) && (divisor == (-LFRACT_EPSILON)))
    {
        /* result is LFRACT_MIN bit-pattern but interpreted as unsigned
        ** long int.
        */
        uint_ulr_t uquo;
        quo = bitslr(LFRACT_MIN);
        uquo = (uint_ulr_t)quo;
        result =  (long int)uquo;
    }
    else
    {
        /* do a standard integer divide.
        */
        int_lr_t i_dividend = bitslr(dividend);
        int_lr_t i_divisor = bitslr(divisor);
        quo = i_dividend / i_divisor;
        result = (long int)quo;
    }
    return result;
}

/* End of file */
