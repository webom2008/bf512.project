/**************************************************************************
 *
 * idivk.c : $Revision: 1.6.4.1 $
 * (c) Copyright 2008-2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: idivk - divide an accum value by an 
                      accum value to give an integer result

    Synopsis:

        int idivk (accum parm1, accum parm2);

    Description:
 
        This function divides an accum argument by an
        accum argument to produce an integer result.
        The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.

    Algorithm:

        Interpret the accum bit-patterns as integers by using bitsk to
        convert to the container type. Then use standard integer divide.
        Be careful of ACCUM_MIN / 0xffff, for which the integer divide may
        return an undefined result.

    Implementation:

    Example:
    
        #include <stdfix.h>
        accum parm1;
        accum parm2;
        int result;

        result = idivk (parm1,parm2);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=idivk")
#pragma file_attr("libFunc=_idivk")
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

#ifdef __FX_NO_ACCUM
#error This file requires _Accum types
#endif

#define __DISABLE_IDIVK 1

#include <stdint.h>
#include <stdfix.h>

extern int 
idivk(accum _dividend,
      accum _divisor) 
{
    const accum dividend = _dividend;
    const accum divisor = _divisor;
   
    int_k_t quo;
    int result;

    if ((dividend == ACCUM_MIN) && (divisor == (-ACCUM_EPSILON)))
    {
        /* result is ACCUM_MIN bit-pattern but interpreted as unsigned
        ** int.
        */
        uint_uk_t uquo;
        quo = bitsk(ACCUM_MIN);
        uquo = (uint_uk_t)quo;
        result =  (int)uquo;
    }
    else
    {
        /* do a standard integer divide.
        */
        int_k_t i_dividend = bitsk(dividend);
        int_k_t i_divisor = bitsk(divisor);
        quo = i_dividend / i_divisor;
        result = (int)quo;
    }
    return result;
}

/* End of file */
