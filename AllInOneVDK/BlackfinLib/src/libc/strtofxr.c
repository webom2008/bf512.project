/************************************************************************
 *
 * strtofxr.c : $Revision: 1.8.4.1 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr ("libName=libc")
#pragma file_attr ("libGroup=stdfix.h")

#pragma file_attr ("libFunc=strtofxr")
#pragma file_attr ("libFunc=_strtofxr")

#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")
#endif

#if defined(__DOCUMENTATION__)

    Function: strtofxr - parse a string representing a fract

    Synopsis:

        fract strtofxr (const char * __restrict _nptr,
                        char ** __restrict _endptr);

    Description:

        This function takes a string argument. It first of all discards
        any leading whitespace. It then attempts to interpret the following
        characters as a fract value. If successful, this value is
        returned and (if endptr is non-null) a pointer to the remaining
        portion of the string is stored in endptr. If the string was not
        a valid representation of a fract, endptr is set to point to the
        beginning of the original string.

    Error Conditions:

        If the string represents a value outside of the range of a 
        fract, a saturated result is returned and errno is set to ERANGE.

    Algorithm:

        We call xstrtofx to do the work, passing in the maximum value for
        a fract.

    Implementation:

    Example:

        #include <stdfix.h>
        char *my_string = " 0.5 is a fract value";
        char **tail;
        fract result;

        result = strtofxr (my_string,tail);

***************************************************************************
#endif

#define __DISABLE_STRTOFXR 1
#include <stdfix.h>
#include <stdbool.h>

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

#pragma diag(suppress:misra_rule_5_6)
#pragma diag(suppress:misra_rule_5_7)
/* Suppress Rule 5.6 and 5.7 (advisory), whereby a single identifier name 
** may not used in multiple namespaces. 
**
** Rationale: we used nptr and endptr as the identifier name for the 
** structure members and the local variables for clarity.
*/
#endif

#include "xfxdecdata.h"
#include "xstrtofx.h"
#include "xstrtofx_string_info.h"

fract 
strtofxr(const char * __restrict _nptr,
         char ** __restrict _endptr)
{
    const char * const __restrict nptr = _nptr;
    char ** const __restrict endptr = _endptr;
    strtofx_string_data_t string_info;

    /* this is the result of strtofx, but held in a generic integer container
    ** that's big enough to hold the result of conversion to any fixed-point
    ** type. We then must convert to our fixed-point type.
    */
    largest_unsigned_fixed_point_container_t result_i;

    string_info.nptr = nptr;
    string_info.endptr = endptr;
    string_info.index = 0;

    result_i = xstrtofx(&string_info,
                        read_char_strtofx,
                        replace_char_strtofx,
                        set_end_position_strtofx,
                        set_overflow_strtofx,
                        FRACT_FBIT,
                        true,
                        FRACT_MAX);

    return rbits((int_r_t)result_i);
}

/* End of file */
