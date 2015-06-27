/************************************************************************
 *
 * xstrtofx.c : $Revision: 1.13.4.1 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)

#pragma file_attr ("libGroup=stdfix.h")
#pragma file_attr ("libName=libc")

#pragma file_attr ("libFunc=__xstrtofx")
#pragma file_attr ("libFunc=strtofxhr")
#pragma file_attr ("libFunc=_strtofxhr")
#pragma file_attr ("libFunc=strtofxr")
#pragma file_attr ("libFunc=_strtofxr")
#pragma file_attr ("libFunc=strtofxlr")
#pragma file_attr ("libFunc=_strtofxlr")
#pragma file_attr ("libFunc=strtofxuhr")
#pragma file_attr ("libFunc=_strtofxuhr")
#pragma file_attr ("libFunc=strtofxur")
#pragma file_attr ("libFunc=_strtofxur")
#pragma file_attr ("libFunc=strtofxulr")
#pragma file_attr ("libFunc=_strtofxulr")
#pragma file_attr ("libFunc=strtofxhk")
#pragma file_attr ("libFunc=_strtofxhk")
#pragma file_attr ("libFunc=strtofxk")
#pragma file_attr ("libFunc=_strtofxk")
#pragma file_attr ("libFunc=strtofxlk")
#pragma file_attr ("libFunc=_strtofxlk")
#pragma file_attr ("libFunc=strtofxuhk")
#pragma file_attr ("libFunc=_strtofxuhk")
#pragma file_attr ("libFunc=strtofxuk")
#pragma file_attr ("libFunc=_strtofxuk")
#pragma file_attr ("libFunc=strtofxulk")
#pragma file_attr ("libFunc=_strtofxulk")
#pragma file_attr ("libFunc=scanf")
#pragma file_attr ("libFunc=_scanf")
#pragma file_attr ("libFunc=sscanf")
#pragma file_attr ("libFunc=_sscanf")
#pragma file_attr ("libFunc=fscanf")
#pragma file_attr ("libFunc=_fscanf")

#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")

#endif

#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

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

#pragma diag(suppress:misra_rule_19_7)
/* Suppress Rule 19.7 (advisory) whereby a function should be used in place
** of a function-like macro.
**
** The rationale is that we can generate more efficient code without the
** overhead of the function call.
*/

#pragma diag(suppress:misra_rule_19_4)
/* Suppress Rule 19.4 (required) whereby a macro may only expand to a
** constant, braced initialiser etc.
**
** Rationale: It's useful to use the isspace, isdigit and isxdigit macros
** but they disobey this rule.
*/

#pragma diag(suppress:misra_rule_17_4)
/* Suppress Rule 17.4 (required) whereby the only type of pointer arithmetic
** allowed is array indexing.
**
** Rationale: It's useful to use the isspace, isdigit and isxdigit macros
** but they disobey this rule.
*/

#pragma diag(suppress:misra_rule_12_1)
/* Suppress Rule 12.1 (advisory) Limited dependence should be placed on 
** C's operator precedence rules in expressions.
*/

#endif

#include "rounding.h"
#include "xstrtofx.h"
#include "xfxdecdata.h"

#define tonumber(x)  ((unsigned int)(x) - (unsigned int)'0')
#define toxnumber(x)  (((unsigned int)(x) >= (unsigned int)'a') \
                       ? ((unsigned int)(x)-(unsigned int)'a'+10U) \
                       : (((unsigned int)(x) >= (unsigned int)'A') \
                           ? ((unsigned int)(x)-(unsigned int)'A'+10U) \
                           : ((unsigned int)(x)-(unsigned int)'0')))

/* note that any digits past MAX_SIGNIFICANT_DIGITS are just ignored. */
#define MAX_SIGNIFICANT_DIGITS (MAX_DECIMAL_INTEGER_DIGITS+NUM_FRACT_BITS)

/* this is the minimum value of extra_container_bits such that the
** overflow may be tracked solely by considering the value held in
** our largest_unsigned_fixed_point_t result. If there are fewer
** extra_container_bits than this, then we need to track overflow
** explicitly in integer_part.
*/
#define REQUIRED_CONTAINER_HEADROOM 4

/* We have to convert a string to a fixed-point value.
** First we find any exponent part. We also count the number of
** digits before any (hexa)decimal point, and record this in the
** exponent too. This allows us to interpret the numerical digits 
** without any further need to remember where the decimal point was.
** This also means we can skip any leading zeros with impunity, so
** we only start the interpretation at the first non-zero character.
**
** We keep the integer part in a separate variable from the fractional
** part. This makes it easier to saturate to the upper limit.
**
** The string may be:
** - some number of whitespace characters
** - an optional '+' or '-' character
** - an optional "0x"/"0X" in which case the following characters are
**   interpreted as hex
** - a non-zero number of (hexa)decimal characters, optionally containing
**   a (hexa)decimal point
** - an optional exponent, which in the decimal case begins with the
**   'e' or 'E' character and in the hex case begins with 'p' or 'P'.
**   In the hex case, the following exponent is a binary exponent.
**   The exponent itself may be preceded by '+' or '-'
** 
** The value is returned in an integer container that is big enough to
** hold any of the fixed-point types. The function returns this bit-pattern
** so that the caller need only truncate the value to the correct size,
** no other conversion is necessary.
*/

largest_unsigned_fixed_point_container_t
xstrtofx(void *cl,
         char (*read_char)(void *, char, int *),
         char (*replace_char)(void *, char, int *),
         void (*set_end_position)(void *),
         void (*set_overflow)(void *),
         int targ_fract_bits,
         bool is_signed,
         largest_unsigned_fixed_point_t upper_saturation_limit)
{
    /* here's where we keep our final result.
    */
    largest_unsigned_fixed_point_container_t result = 0U;

    /* we keep the integer part alone here, as we may need it to check
    ** for overflow.
    */
    unsigned int integer_part = 0U;

    /* this is the maximum positive integer that can be represented exactly by
    ** the target fixed-point value.
    */
    const unsigned int max_integer_part
        = (unsigned int)upper_saturation_limit;

    /* this is the maximum positive value that can be represented by the 
    ** target fixed-point value, but the bit-pattern is held in an
    ** integer container.
    */
    const largest_unsigned_fixed_point_container_t upper_saturation_limit_i
        = BITS_LARGEST_UNSIGNED_FX(upper_saturation_limit);

    /* this is the maximum possible exponent applied to the first non-zero
    ** digit before we are certain to overflow.
    */
    int max_exponent = MAX_DECIMAL_INTEGER_DIGITS;

    /* this is the number of extra, unused bytes/bits that are at the top
    ** of the largest_unsigned_fixed_point_container_t when we hold an
    ** largest_unsigned_fixed_point_t. These bytes/bits may be used for
    ** overflow.
    */
    const size_t extra_container_bytes
        = (sizeof(result) - sizeof(upper_saturation_limit));
    const int extra_container_bits
        = (int)extra_container_bytes * (int)CHAR_BIT;

    /* is the value represented by the string negative? */
    bool is_negative = false;

    /* does the value saturate at the most negative value? */
    bool saturate_at_most_negative = false;

    /* does the string represent a value is hexadecimal format? */
    bool is_hex = false;

    /* the upper-case letter that introduces the exponent. */
    int exponent_letter = 'E';

    /* the exponent factor tells us how many units in the exponent are
    ** represented by a single digit. So for decimal, the exponent 
    ** represents a power of 10, and the digits are also base 10, so the
    ** factor is 1. For hexadecimal, the exponent represents powers of 2
    ** but the digits are hexadecimal, so the factor is 4.
    */
    int exponent_factor = 1; /* decimal digits per decimal character */

    /* the power to which the first non-zero digit must be raised
    ** to give the result.
    */
    int exponent = 0;

    /* this buffer contains the digits of the string, excluding leading
    ** zeros.
    */
    unsigned int digits_buf[MAX_SIGNIFICANT_DIGITS];

    /* the following are used when parsing the string, to keep track of
    ** what characters we've seen so far. A "digit" is either 0-9 or
    ** 0-9,a-f,A-F when in hexadecimal format.
    */
    bool seen_decimal_point = false;
    bool seen_digit = false;
    bool seen_non_zero = false;

    /* the number of numerical digits, excluding leading zeros,
    ** that are in the non-exponent part of the string.
    */
    int num_digits;

    /* the number of the first bit that we are throwing away, when we convert
    ** from the largest_unsigned_fixed_point_t result to the target fractional
    ** bits.
    */
    const int msb_of_rounded = NUM_FRACT_BITS - targ_fract_bits;

    /* the number of the last bit that we are keeping, when we convert
    ** from the largest_unsigned_fixed_point_t result to the target fractional
    ** bits.
    */
    const int lsb_of_result = msb_of_rounded + 1;

    /* the current char in the string that we're processing */
    char curr_char;

    /* the number of characters we've read speculatively, hoping to prove
    ** some valid input. If we fail, we must backtrack these.
    ** We also replace the first invalid character back where it came from,
    ** too.
    */
    int speculatively_read = 0;

    /* a shift amount */
    int shift;

    curr_char = read_char(cl, '\0', &speculatively_read);

    /* discard initial whitespace. */
    while (isspace(curr_char) != 0U)
    {
        curr_char = read_char(cl, curr_char, &speculatively_read);
    }

    if ((curr_char == '+') || (curr_char == '-'))
    {
        /* remember that we're negative, but unset it if it's -0. */
        if (curr_char == '-')
        {
            is_negative = true;
        }
        curr_char = read_char(cl, curr_char, &speculatively_read);
    }

    /* are we dealing with hex or not?
    */
    if (curr_char == '0')
    {
        speculatively_read = 0;
        curr_char = read_char(cl, curr_char, &speculatively_read);
        if ((curr_char == 'x') || (curr_char == 'X'))
        {
            is_hex = true;
            exponent_factor = 4; /* binary digits per hex character */
            max_exponent = MAX_BINARY_INTEGER_DIGITS;
            exponent_letter = 'P';

            /* we've definitely got some valid input. But speculatively
            ** read past the "x"
            */
            curr_char = read_char(cl, curr_char, &speculatively_read);
        }
        else
        {
            /* replace the digit to be processed in the normal way for a
            ** decimal digit.
            */
            curr_char = replace_char(cl, curr_char, &speculatively_read);
        }
    }

    num_digits = 0;

    while (curr_char != '\0')
    {
        if ((curr_char == '.') && (!seen_decimal_point))
        {
            if (seen_digit)
            {
                speculatively_read = 0;
            }
            seen_decimal_point = true;
        }
        else if ((is_hex ? isxdigit(curr_char) : isdigit(curr_char)) != 0U)
        {
            speculatively_read = 0;
            if ((!seen_non_zero) && seen_decimal_point)
            {
                exponent -= exponent_factor;
            }
            else if (seen_non_zero && (!seen_decimal_point))
            {
                exponent += exponent_factor;
            }
            else
            {
                /* do nothing - keep misra happy */
            }
            seen_digit = true;
            if ((curr_char != '0') || seen_non_zero)
            {
                /* ignore leading zeros */
                seen_non_zero = true;
                if (num_digits < MAX_SIGNIFICANT_DIGITS)
                {
                    digits_buf[num_digits] = toxnumber(curr_char);
                    num_digits++;
                }
            }
        }
        else
        {
            int c_upper = toupper((int)curr_char);
            
            if (seen_digit && (c_upper == exponent_letter))
            {
                int this_exponent = 0;
                int new_exponent;
                bool exponent_negative = false;
                curr_char = read_char(cl, curr_char, &speculatively_read);
                
                if ((curr_char == '+') || (curr_char == '-'))
                {
                    if (curr_char == '-')
                    {
                        exponent_negative = true;
                    }
                    curr_char = read_char(cl, curr_char, &speculatively_read);
                }

                while (isdigit(curr_char) != 0U)
                {
                    unsigned int exponent_digit = tonumber(curr_char);
                    speculatively_read = 0;
                    if (this_exponent > (INT_MAX/ 10))
                    {
                        this_exponent = INT_MAX;
                    }
                    else
                    {
                        this_exponent *= 10;
                        this_exponent += (int)exponent_digit;
                    }
                    curr_char = read_char(cl, curr_char, &speculatively_read);
                }

                if (exponent_negative)
                {
                    this_exponent = -this_exponent;
                }

                new_exponent = exponent + this_exponent;
                if (   (new_exponent < 0)
                    && (exponent > 0) && (this_exponent > 0))
                {
                    /* we overflowed past max possible exponent.
                    ** Saturate to max value.
                    */
                    exponent = INT_MAX;
                }
                else if (   (new_exponent > 0)
                         && (exponent < 0) && (this_exponent < 0))
                {
                    /* we overflowed past min possible exponent.
                    ** Saturate to min value.
                    */
                    exponent = INT_MIN;
                }
                else
                {
                    exponent = new_exponent;
                }
            }
            /* We either had an exponent or it's an illegal character,
            ** so we've come to the end of the valid portion of the string.
            */
            break;
        }
        curr_char = read_char(cl, curr_char, &speculatively_read);
    }

    /* backtrack out anything we read speculatively that wasn't part of the
    ** valid input.
    */
    while (speculatively_read != 0)
    {
        curr_char = replace_char(cl, curr_char, &speculatively_read);
    }

    set_end_position(cl);

    if (!seen_non_zero)
    {

        /* there were no non-zeros in the string. Return 0. */
        result = 0U;

    }
    else if (is_negative && (!is_signed))
    {

        /* saturate to zero, and set errno. */
        set_overflow(cl);
        result = 0U;
 
    }
    else if (exponent > max_exponent)
    {

        /* we overflowed. Set error and return the max value. */
        set_overflow(cl);
        result = upper_saturation_limit_i;
        if (is_negative)
        {
            saturate_at_most_negative = true;
        }

    }
    else
    {
        int i; /* an iterator */

        /* this is a bit-pattern containing a 1 in the last
        ** position of the result.
        */
        largest_unsigned_fixed_point_container_t last_bit = 1ULL;
        last_bit <<= (lsb_of_result-1);

        if (is_hex)
        {
            largest_unsigned_fixed_point_container_t curr_digit;
            i = 0;

            while (i < num_digits)
            {

                /* this digit represents the exponents up to and include
                ** the following, because each digit represents 4 binary
                ** exponents.
                */
                int high_exponent = exponent + exponent_factor - 1;

                if (   (extra_container_bits < REQUIRED_CONTAINER_HEADROOM)
                    && (high_exponent >= 0))
                {
                    /* a contributor to the integer part. We keep a separate
                    ** record of the integer part in case it is bigger
                    ** than the value that can be held in the LUAContainer,
                    ** in which case we must detect the overflow.
                    ** If the container is bigger than
                    ** largest_unsigned_fixed_point_container_t 
                    ** then this code will removed at build time.
                    */
                    curr_digit = digits_buf[i];
                    if (   (MAX_BINARY_INTEGER_DIGITS == 0)
                        || (exponent < 0))
                    {
                        shift = -exponent;
                        curr_digit >>=
                            (largest_unsigned_fixed_point_container_t)shift;
                    }
                    else
                    {
                        shift = exponent;
                        curr_digit <<=
                            (largest_unsigned_fixed_point_container_t)shift;
                    }
                    integer_part |= (unsigned int)curr_digit;
                }

                /* this part assembles the value into the result */
                if (high_exponent >= (-targ_fract_bits))
                {
                    /* a contributor to the result */
                    curr_digit = digits_buf[i];
                    if (exponent < (-targ_fract_bits))
                    {
                        /* get rid of any bits below size of desired result */
                        shift = (-targ_fract_bits-exponent);
                        curr_digit >>=
                            (largest_unsigned_fixed_point_container_t)shift;
                        curr_digit <<=
                            (largest_unsigned_fixed_point_container_t)shift;
                    }
                    shift = NUM_FRACT_BITS + exponent;
                    curr_digit <<=
                        (largest_unsigned_fixed_point_container_t)shift;
                    result |=
                        (largest_unsigned_fixed_point_container_t)curr_digit;
                }

                /* here we deal with rounding. */
                if (exponent < (-targ_fract_bits))
                {
                    /* This digit contributes to rounding.
                    ** A value exactly half-way between will round up if we're
                    ** in biased rounding mode, or to nearest even if we're
                    ** unbiased. The first time we enter here we set the
                    ** limit to be half-way between the LSB of the result.
                    ** If we're not negative or we're doing biased rounding,
                    ** then we can always resolve the rounding straightaway,
                    ** but if we've got a negative value we only want to round
                    ** up if we're more than half-way between, which means
                    ** we keep going till we prove it.
                    */
                    unsigned int limit = 0x8U;

                    /* are we doing biased or unbiased rounding? */
                    bool biased = (bool)BIASED_ROUNDING;

                    curr_digit = digits_buf[i];
                    if (high_exponent >= (-targ_fract_bits))
                    {
                        /* get rid of any bits that aren't being rounded */
                        shift = high_exponent + targ_fract_bits;
                        curr_digit <<=
                            (largest_unsigned_fixed_point_container_t)shift;
                        curr_digit &= 0xfU;
                    }

                    if ((curr_digit == limit) && ((!biased) || is_negative))
                    {
                        /* we can't tell which way to round until we've
                        ** worked out if we're bigger or equal to the
                        ** mid-point.
                        */
                        limit = 0U;
                        while ((i + 1) < num_digits)
                        {
                            i++;
                            curr_digit = digits_buf[i];        
                            if (curr_digit != limit)
                            {
                                break;
                            }
                        }
                    }

                    if (   (curr_digit > limit)
                        || (   (curr_digit == limit)
                            && (   (biased && (!is_negative))
                                || (   (!biased)
                                    && ((result & last_bit) != 0U)))))
                    {
                        result += last_bit;
                        if (   (extra_container_bits
                                < REQUIRED_CONTAINER_HEADROOM)
                            && (result == 0U))
                        {
                            integer_part++; /* we overflowed */
                        }
                        /* fract part may now overflow into int part, but that
                        ** doesn't matter.
                        */
                    }
                    break;
                }

                exponent -= exponent_factor;
                i++;
            }

#if MAX_BINARY_INTEGER_DIGITS > 0
            if (extra_container_bits >= REQUIRED_CONTAINER_HEADROOM) 
            {
                largest_unsigned_fixed_point_container_t tmp
                    = result >> NUM_FRACT_BITS;
                integer_part = (unsigned int)tmp;
            }
#else
            /* avoid shift by number of bits in type, which causes a 
            ** compilation warning.
            */
            integer_part = 0U;
#endif

        } else {

            int j; /* an iterator */

            /* the decimal values of the fractional bits */
            fx_decimal_part_t part[NUM_PARTS];

            /* the power of 10 represented by the current digit */
            unsigned int column = 1U;

            unsigned int digit;

#if MAX_BINARY_INTEGER_DIGITS > 0
            /* first find the integer part of the result. This cannot
            ** overflow since we only have a small number of digits.
            */
            i = exponent;
            while (i >= 0)
            {
                digit = (i < num_digits) ? digits_buf[i] : 0U;
                integer_part += (column * digit);
                column *= 10U;
                i--;
            }
            result = ((largest_unsigned_fixed_point_container_t)integer_part
                      << NUM_FRACT_BITS);
#else
            /* avoid shift by number of bits in type, which causes a 
            ** compilation warning.
            ** If MAX_BINARY_INTEGER_DIGITS == 0, then we know exponent <= 0.
            */
            integer_part += (((exponent == 0) && (0 < num_digits))
                             ? digits_buf[0] : 0U);
            result = 0U;
#endif

            /* find all the fract parts of the result. */
            for (j = 0; j < NUM_PARTS; j++)
            {
                int stop_i = exponent + (DIGITS_PER_PART * j) + 1;
                column = 1U;
                part[NUM_PARTS-1-j] = 0U;
                if (stop_i < num_digits)
                {
                    i = stop_i + DIGITS_PER_PART - 1;
                    if (stop_i < 0)
                    {
                        stop_i = 0;
                    }
                    while (i >= stop_i)
                    {
                        digit = (i < num_digits) ? digits_buf[i] : 0U;
                        part[NUM_PARTS-1-j] += (column * digit);
                        column *= 10U;
                        i--;
                    }
                }
            }


            /* now successively try to subtract the int value associated with 
            ** each fract bit from our fract representation. If it goes, set
            ** the bit in the result.
            */
            for (i = NUM_FRACT_BITS; i >= lsb_of_result; i--)
            {
                j = NUM_PARTS - 1;
                while ((j > 0) && (part[j] == __fx_decimal_part[j][i])) 
                {
                    j--;
                }
                if (part[j] >= __fx_decimal_part[j][i])
                {
                    unsigned int borrow = 0U;
                    largest_unsigned_fixed_point_container_t tmp = 1ULL;
                    int k;
                    for (k = 0; k < NUM_PARTS; k++)
                    {
                        unsigned int part_to_subtract = __fx_decimal_part[k][i]
                                                      + borrow;
                        if (part[k] < part_to_subtract)
                        {
                            borrow = 1U;
                            part[k] = MAX_PER_PART
                                    - (part_to_subtract - part[k]);
                        }
                        else
                        {
                            borrow = 0U;
                            part[k] -= part_to_subtract;
                        }
                    }
                    result |= (tmp << (i-1));
                }
            }

            /* what's left? If it's closer to 
            ** __fx_decimal_part[][lsb_of_result] than to zero, then round up
            ** by one. If we're negative, then when we're exactly half-way
            ** between then we shouldn't round up, so that 3.5 rounds to 4
            ** but -3.5 rounds to -3. That's consistent with biased rounding.
            **
            ** We compare against __fx_decimal_part[][msb_of_rounded],
            ** which equals __fx_decimal_part[][lsb_of_result]/2.
            */
            i = NUM_PARTS-1;
            while (   (i > 0)
                   && (part[i] == __fx_decimal_part[i][msb_of_rounded]))
            {
                i--;
            }
            if (   (part[i] > __fx_decimal_part[i][msb_of_rounded])
                || (   (!is_negative)
                    && (part[i] >= __fx_decimal_part[i][msb_of_rounded])))
            {
                result += last_bit;
                if (result == 0U)
                {
                    integer_part++;
                }
            }

        }

        /* now saturate down to the max value passed into this function, and
        ** find out if the string actually represented the most negative
        ** value.
        */
        if (   (integer_part > max_integer_part)
            || (result > upper_saturation_limit_i))
        {
            /* we overflowed. Must saturate the result. 
            ** Detect if we're actually the most negative value.
            ** In that single case we should not set errno.
            */
     
            /* how many bits are in our result container? */
            int result_bits = (int)sizeof(result) * (int)CHAR_BIT;

            if (   (!is_negative)
                || ((result << (result_bits - NUM_FRACT_BITS)) != 0U)
                || (integer_part != (max_integer_part + 1U)))
            {
                /* we are not the most negative value, so set errno. */
                set_overflow(cl);
            }

            /* now saturate */
            result = upper_saturation_limit_i;
            if (is_negative)
            {
                saturate_at_most_negative = true;
            }
        } 
    }

    /* get a largest_unsigned_fixed_point_t from our value held in the
    ** container. Note that this saturates to the fixed-point size.
    */
    shift = NUM_FRACT_BITS - targ_fract_bits;
    result >>= (largest_unsigned_fixed_point_container_t)shift;

    /* and negate the result if it should be negative */
    if (is_negative)
    {
        result = 0U - result;
    }
    if (saturate_at_most_negative)
    {
        /* add 1 to the upper_saturation limit. This takes us from something
        ** like 0x8001 to 0x8000.
        */
        result -= 1U;
    }

    return result;
}

/* End of file */
