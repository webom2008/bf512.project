/************************************************************************
 *
 * strtofxlr.c : $Revision: 1.9.4.1 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr ("libName=libc")
#pragma file_attr ("libGroup=stdfix.h")

#pragma file_attr ("libFunc=strtofxlr")
#pragma file_attr ("libFunc=_strtofxlr")

#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")
#endif

#if defined(__DOCUMENTATION__)

    Function: strtofxlr - parse a string representing a long fract

    Synopsis:

        long fract strtofxlr (const char * __restrict _nptr,
                              char ** __restrict _endptr);

    Description:

        This function takes a string argument. It first of all discards
        any leading whitespace. It then attempts to interpret the following
        characters as a long fract value. If successful, this value is
        returned and (if endptr is non-null) a pointer to the remaining
        portion of the string is stored in endptr. If the string was not
        a valid representation of a long fract, endptr is set to point to the
        beginning of the original string.

    Error Conditions:

        If the string represents a value outside of the range of a long
        fract, a saturated result is returned and errno is set to ERANGE.

    Algorithm:

        We call xstrtofx to do the work, passing in the maximum value for
        a long fract.

    Implementation:

    Example:

        #include <stdfix.h>
        char *my_string = " 0.5 is a long fract value";
        char **tail;
        long fract result;

        result = strtofxlr (my_string,tail);

***************************************************************************
#endif

#define __DISABLE_STRTOFXLR 1
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

long fract 
strtofxlr(const char * __restrict _nptr,
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
                        LFRACT_FBIT,
                        true,
                        LFRACT_MAX);

    return lrbits((int_lr_t)result_i);
}

/* End of file */
