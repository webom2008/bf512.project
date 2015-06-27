/************************************************************************
 *
 * xstrtofx_string_info.c 
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 * $Revision: 1.5.14.1 $
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr ("libName=libc")
#pragma file_attr ("libGroup=stdfix.h")

#pragma file_attr ("libFunc=__read_char_strtofx")
#pragma file_attr ("libFunc=__replace_char_strtofx")
#pragma file_attr ("libFunc=__set_end_position_strtofx")
#pragma file_attr ("libFunc=__set_overflow_strtofx")
#pragma file_attr ("libFunc=strtofxhr")
#pragma file_attr ("libFunc=_strtofxhr")
#pragma file_attr ("libFunc=strtofxr")
#pragma file_attr ("libFunc=_strtofxr")
#pragma file_attr ("libFunc=strtofxlr")
#pragma file_attr ("libFunc=_strtofxlr")
#pragma file_attr ("libFunc=strtofxhk")
#pragma file_attr ("libFunc=_strtofxhk")
#pragma file_attr ("libFunc=strtofxk")
#pragma file_attr ("libFunc=_strtofxk")
#pragma file_attr ("libFunc=strtofxlk")
#pragma file_attr ("libFunc=_strtofxlk")
#pragma file_attr ("libFunc=strtofxuhr")
#pragma file_attr ("libFunc=_strtofxuhr")
#pragma file_attr ("libFunc=strtofxur")
#pragma file_attr ("libFunc=_strtofxur")
#pragma file_attr ("libFunc=strtofxulr")
#pragma file_attr ("libFunc=_strtofxulr")
#pragma file_attr ("libFunc=strtofxuhk")
#pragma file_attr ("libFunc=_strtofxuhk")
#pragma file_attr ("libFunc=strtofxuk")
#pragma file_attr ("libFunc=_strtofxuk")
#pragma file_attr ("libFunc=strtofxulk")
#pragma file_attr ("libFunc=_strtofxulk")

#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")
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

#pragma diag(suppress:misra_rule_17_4)
/* Suppress Rule 17.4 (required) whereby only arrays shall be used for
** pointer arithmetic.
**
** Rationale: the input to strtofxfx is a pointer to an array, not an array.
*/

#pragma diag(suppress:misra_rule_20_5)
/* Suppress Rule 20.5 (required) whereby errno may not be used.
**
** Rationale: this library function is defined as setting errno when the
** input string represents a constant that is out of range.
*/

#pragma diag(suppress:misra_rule_11_5)
/* Suppress Rule 11.5 (required) whereby you may not cast away a const
** qualifier.
**
** Rationale: we need to store a pointer related to the input paramater,
** which is a const char *, into the parameter that holds the return pointer,
** which is a char **.
*/

#pragma diag(suppress:misra_rule_16_7)
/* Suppress Rule 16.7 (advisory) whereby a pointer parameter should be
** a pointer to const unless the memory is modifed.
**
** Rationale: some of these functions may modify the memory, other may
** not - but they must have constant signature.
*/
#endif

#include <errno.h>
#include <stdlib.h>

#include "xstrtofx_string_info.h"


/* this function returns the next character to be processed, advancing 
** by one in the input string.
*/
char
read_char_strtofx(void *cl, char curr_c, int *speculatively_read)
{
    strtofx_string_data_t *string_info = (strtofx_string_data_t *)cl;
    string_info->index++;
    *speculatively_read += 1;
    return string_info->nptr[string_info->index - 1];
}

/* this function backtracks by replacing the given character at the
** end and retreating by one character in the input. It returns the
** previous character.
*/
char
replace_char_strtofx(void *cl, char c, int *speculatively_read)
{
    char prev_c = '\0';
    strtofx_string_data_t *string_info = (strtofx_string_data_t *)cl;
    string_info->index--;
    *speculatively_read -= 1;
    if (string_info->index > 0)
    {
        prev_c = string_info->nptr[string_info->index - 1];
    }
    return prev_c;
}

/* this function stores the current position (the end of the valid 
** portion of the input) for safe-keeping.
*/
void
set_end_position_strtofx(void *cl)
{
    strtofx_string_data_t *string_info = (strtofx_string_data_t *)cl;
    if (string_info->endptr != NULL)
    {
        *string_info->endptr = (char *)&string_info->nptr[string_info->index];
    }
}

/* this function records an overflow state.
*/
void
set_overflow_strtofx(void *cl)
{
    errno = ERANGE;
}

/* End of file */
