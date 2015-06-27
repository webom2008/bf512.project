/****************************************************************************
 *
 * raw64_to_float.h : $Revision: 1.3 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#ifndef __RAW_TO_FLOAT_H
#define __RAW_TO_FLOAT_H

#if defined(__DOCUMENTATION__)

    Function: raw64_to_float - Convert variable-precision long long to float

    Synopsis:

        #include "xutil_fr.h"
        float raw64_to_float (long long    raw64,
                              unsigned int binary_decimal_digits);

    Description:

        The raw64_to_float function is a support routine.

        It converts the parameter raw64, which represents a variable-precision,
        fixed-point, 64-bit integer to an single-precision IEEE floating-point
        value.

        The precision of the parameter raw64 is defined by the parameter
        binary_decimal_digits. The parameter specifies the number of binary
        digits to the right of the decimal point and should not greater
        than 64.

        As an example, if raw64 represents a fixed-point value in 1:63 format,
        then binary_decimal_digits should be set to 63; if raw64 is a 64-bit
        integer value then the parameter binary_decimal_digits should be set
        to 0.

    Error Conditions:

        The raw64_to_float function does not return any error conditions; it
        does not check the range of the parameter binary_decimal_digits.

    Algorithm:

        #include <math.h>

        long long raw64
        float     x = ldexpf (raw64,-binary_decimal_digits);

    Example:

        #include "xutil_fr.h"

        #define DIGITS_IN_DECIMAL_PART 63U

        long long raw_sum;
        float     sum;

        sum = raw64_to_float (raw_sum,DIGITS_IN_DECIMAL_PART);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libName  = libdsp")
#pragma file_attr("libGroup = integer_support")
#pragma file_attr("libGroup = floating_point_support")
#pragma file_attr("prefersMem = internal")
#pragma file_attr("prefersMemNum = 30")

#endif

#if !defined(TRACE)
#pragma optimize_for_speed
#endif

#ifdef _MISRA_RULES
#pragma diag(push)

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

#pragma diag(suppress:misra_rule_8_5)
/* Suppress Rule 8_5 (required) whereby there shall be no definitions of
** objects or functions in a header file. Header files should be used to
** declare objects, functions, typedefs, and macros. Header files shall
** not contain or produce definitions of objects or functions (or fragment
** of functions or objects) that occupy storage. This makes it clear that
** only C files contain executable source code and that header files only
** contain declarations.
**
** Rationale: The function defined in this header file is declared as an
** in-line function that should always be in-lined. It is a support routine
** that is used by more than one library function, and in-lining saves
** approximately 12 cycles (14% of the function's performance). Defining
** an inlined function is permitted in this case as it avoids multiple
** copies of the same source.
*/

#pragma diag(suppress:misra_rule_12_12)
/* Suppress Rule 12.12 (required) whereby the underlying bit representations
** of floating-point values shall not be used as the storage layout used for
** floating-point values may vary from one compiler to another, and therefore
** no floating-point manipulations shall be made which rely directly on the
** way the values are stored. The in-built operators and functions, which
** hide the storage details from the programmer, should be used.
**
** Rationale: The underlying storage layout used for floating-point values
** is well understood (it conforms to the IEEE standard), and use of this
** knowledge will be permitted in controlled circumstances where it is known
** the floating-point value is not an irregular floating-point value (i.e. it
** is not a NaN, Infinity, or denormalized number) and any manipulations of
** the value will guarantee that it will not become an irregular number.
*/

#pragma diag(suppress:misra_rule_11_4)
/* Suppress Rule 11.4 (advisory) whereby a cast should not be performed
** between a pointer type and an integral type as conversions of this
** type may be invalid if the new pointer type requires a stricter alignment.
**
** Rationale: The rule will be suppressed in this individual case as
** it enables a floating-point value to be manipulated as an int (see
** the Rationale for the suppression of rule 12.12 above).
*/
#endif

#include <float.h>

#pragma inline
#pragma always_inline
static float
raw64_to_float (long long    _raw64,
                unsigned int _binary_decimal_digits)
{

    /* local copies of the arguments */

    const long long    raw64 = _raw64;
    const unsigned int binary_decimal_digits = _binary_decimal_digits;

    float result;
    unsigned int *p_result = (unsigned int *)(&result);

    result = (float) raw64;
    if (*p_result != 0UL) {

        /* Decrement exponent by the number of places after the decimal point */

        *p_result -= (binary_decimal_digits << (FLT_MANT_DIG-1));
    }

    return result;

}

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif

#endif /* __RAW_TO_FLOAT_H */
