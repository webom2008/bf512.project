/**************************************************************************
 *
 * idivur.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: idivur - divide an unsigned fract value by an 
                       unsigned fract value to give an unsigned integer result

    Synopsis:

        unsigned int idivur (unsigned fract parm1, unsigned fract parm2);

    Description:
 
        This function divides an unsigned fract argument by an
        unsigned fract argument to produce an unsigned integer result.
        The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.

    Algorithm:

        Interpret the fract bit-patterns as integers by using bitsur to
        convert to the container type. Then use standard integer divide.

    Implementation:

    Example:
    
        #include <stdfix.h>
        unsigned fract parm1;
        unsigned fract parm2;
        unsigned int result;

        result = idivur (parm1,parm2);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=idivur")
#pragma file_attr("libFunc=_idivur")
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

#define __DISABLE_IDIVUR 1

#include <stdint.h>
#include <stdfix.h>

extern unsigned int 
idivur(unsigned fract _dividend,
       unsigned fract _divisor) 
{
    const unsigned fract dividend = _dividend;
    const unsigned fract divisor = _divisor;
   
    uint_ur_t quo;
    unsigned int result;

    /* do a standard integer divide.
    */
    uint_ur_t i_dividend = bitsur(dividend);
    uint_ur_t i_divisor = bitsur(divisor);
    quo = i_dividend / i_divisor;
    result = (unsigned int)quo;

    return result;
}

/* End of file */
