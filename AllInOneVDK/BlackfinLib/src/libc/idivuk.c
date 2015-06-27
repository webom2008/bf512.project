/**************************************************************************
 *
 * idivuk.c : $Revision: 1.6.4.1 $
 * (c) Copyright 2008-2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: idivuk - divide an unsigned accum value by an 
                       unsigned accum value to give an unsigned integer result

    Synopsis:

        unsigned int idivuk (unsigned accum parm1, unsigned accum parm2);

    Description:
 
        This function divides an unsigned accum argument by an
        unsigned accum argument to produce an unsigned integer result.
        The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.

    Algorithm:

        Interpret the accum bit-patterns as integers by using bitsuk to
        convert to the container type. Then use standard integer divide.

    Implementation:

    Example:
    
        #include <stdfix.h>
        unsigned accum parm1;
        unsigned accum parm2;
        unsigned int result;

        result = idivuk (parm1,parm2);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=idivuk")
#pragma file_attr("libFunc=_idivuk")
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

#define __DISABLE_IDIVUK 1

#include <stdint.h>
#include <stdfix.h>

extern unsigned int 
idivuk(unsigned accum _dividend,
       unsigned accum _divisor) 
{
    const unsigned accum dividend = _dividend;
    const unsigned accum divisor = _divisor;
   
    uint_uk_t quo;
    unsigned int result;

    /* do a standard integer divide.
    */
    uint_uk_t i_dividend = bitsuk(dividend);
    uint_uk_t i_divisor = bitsuk(divisor);
    quo = i_dividend / i_divisor;
    result = (unsigned int)quo;

    return result;
}

/* End of file */
