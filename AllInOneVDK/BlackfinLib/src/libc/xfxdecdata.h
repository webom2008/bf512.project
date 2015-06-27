/************************************************************************
 *
 * xfxdecdata.h 
 *
 * (c) Copyright 2008-2010 Analog Devices, Inc.  All rights reserved.
 * $Revision: 1.8.6.1 $
 ************************************************************************/

#ifndef XFXDECDATA_H
#define XFXDECDATA_H

#include <stdfix.h>

#include "rounding.h"

#if defined(_MISRA_RULES)
#pragma diag(push)
#pragma diag(suppress:misra_rule_6_3)
#pragma diag(suppress:misra_rule_19_4)
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

/* define the largest fixed-point types supported on the machine.
*/
#if defined(__FX_NO_ACCUM)
typedef long fract largest_signed_fixed_point_t;
typedef unsigned long fract largest_unsigned_fixed_point_t;
typedef uint_ulr_t largest_unsigned_fixed_point_container_t;
#define BITS_LARGEST_UNSIGNED_FX(x) bitsulr(x)
#else
typedef long accum largest_signed_fixed_point_t;
typedef unsigned long accum largest_unsigned_fixed_point_t;
typedef uint_ulk_t largest_unsigned_fixed_point_container_t;
#define BITS_LARGEST_UNSIGNED_FX(x) bitsulk(x)
#endif

/* an upper bound for the number of digits required to represent the
** integer part of the biggest accumulator.
** We define both the number of binary bits and the number of decimal digits.
*/
#if defined(__FX_NO_ACCUM)
#define MAX_BINARY_INTEGER_DIGITS 0
#else
#define MAX_BINARY_INTEGER_DIGITS ULACCUM_IBIT
#endif
#define MAX_DECIMAL_INTEGER_DIGITS (MAX_BINARY_INTEGER_DIGITS + 2) / 3

/* the type we use to represent our "parts"
*/
typedef unsigned int fx_decimal_part_t;

/* the number of decimal digits we represent in each "part" of fx_decimal_part
*/
#define DIGITS_PER_PART 8
/* ...and the maximum integer that a part can represent,
** i.e. 10^DIGITS_PER_PART
*/
#define MAX_PER_PART 100000000U

/* the maximum number of fractional bits in any of our fixed-point types.
** Should be the number of fractional bits in an unsigned long accum.
*/
#if defined(__FX_NO_ACCUM)
#define NUM_FRACT_BITS ULFRACT_FBIT
#else
#define NUM_FRACT_BITS ULACCUM_FBIT
#endif

/* the number of "parts" needed to represent our fractional bits.
** There's one decimal digit for every binary digit.
*/
#define NUM_PARTS (NUM_FRACT_BITS / DIGITS_PER_PART)

extern fx_decimal_part_t __fx_decimal_part[NUM_PARTS][NUM_FRACT_BITS+1];

#if defined(_MISRA_RULES)
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif

/* End of file */
