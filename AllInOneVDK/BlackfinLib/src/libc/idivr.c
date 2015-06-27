/**************************************************************************
 *
 * idivr.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: idivr - divide a fract value by a 
                      fract value to give an integer result

    Synopsis:

        int idivr (fract parm1, fract parm2);

    Description:
 
        This function divides a fract argument by a
        fract argument to produce an integer result.
        The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.

    Algorithm:

        Interpret the fract bit-patterns as integers by using bitsr to
        convert to the container type. Then use standard integer divide.
        Be careful of FRACT_MIN / 0xffff, for which the integer divide may
        return an undefined result.

    Implementation:

    Example:
    
        #include <stdfix.h>
        fract parm1;
        fract parm2;
        int result;

        result = idivr (parm1,parm2);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=idivr")
#pragma file_attr("libFunc=_idivr")
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

#define __DISABLE_IDIVR 1

#include <stdint.h>
#include <stdfix.h>

extern int 
idivr(fract _dividend,
      fract _divisor) 
{
    const fract dividend = _dividend;
    const fract divisor = _divisor;
   
    int_r_t quo;
    int result;

    if ((dividend == FRACT_MIN) && (divisor == (-FRACT_EPSILON)))
    {
        /* result is FRACT_MIN bit-pattern but interpreted as unsigned
        ** int.
        */
        uint_ur_t uquo;
        quo = bitsr(FRACT_MIN);
        uquo = (uint_ur_t)quo;
        result =  (int)uquo;
    }
    else
    {
        /* do a standard integer divide.
        */
        int_r_t i_dividend = bitsr(dividend);
        int_r_t i_divisor = bitsr(divisor);
        quo = i_dividend / i_divisor;
        result = (int)quo;
    }
    return result;
}

/* End of file */
