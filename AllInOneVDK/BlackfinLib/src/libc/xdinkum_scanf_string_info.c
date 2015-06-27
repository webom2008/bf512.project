/************************************************************************
 *
 * xdinkum_scanf_string_info.c 
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 * $Revision: 1.5.14.1 $
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr ("libName=libc")
#pragma file_attr ("libGroup=stdfix.h")

#pragma file_attr ("libFunc=__read_char_dinkum_scanf")
#pragma file_attr ("libFunc=__replace_char_dinkum_scanf")
#pragma file_attr ("libFunc=__set_end_position_dinkum_scanf")
#pragma file_attr ("libFunc=__set_overflow_dinkum_scanf")
#pragma file_attr ("libFunc=scanf")
#pragma file_attr ("libFunc=_scanf")
#pragma file_attr ("libFunc=fscanf")
#pragma file_attr ("libFunc=_fscanf")
#pragma file_attr ("libFunc=sscanf")
#pragma file_attr ("libFunc=_sscanf")

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

#pragma diag(suppress:misra_rule_19_7)
#pragma diag(suppress:misra_rule_12_1)
#pragma diag(suppress:misra_rule_12_10)
#pragma diag(suppress:misra_rule_12_13)
#pragma diag(suppress:misra_rule_10_3)
/* Suppress all rules that prevent us from using Dinkumware's macros.
** If and when we fix those macros we can reinstate the rules.
** Rationale: That is so that we can use the macros given to us by Dinkumware.
*/

#pragma diag(suppress:misra_rule_16_7)
/* Suppress Rule 16.7 (advisory) whereby a pointer parameter should be
** a pointer to const unless the memory is modifed.
**
** Rationale: some of these functions may modify the memory, other may
** not - but they must have constant signature.
*/
#endif

#include "xstdio.h"

#include "xdinkum_scanf_string_info.h"


/* this function returns the next character to be processed, advancing 
** by one in the input string.
*/
char
read_char_dinkum_scanf(void *cl, char curr_c, int *speculatively_read)
{
    dinkum_scanf_string_info_t *info = (dinkum_scanf_string_info_t *)cl;
    char c = (char)GETN(info->px);
    info->prev_char = curr_c;
    *speculatively_read += 1;
    return c;
}

/* this function backtracks by replacing the given character at the
** end and retreating by one character in the input. It returns the
** previous character.
*/
char
replace_char_dinkum_scanf(void *cl, char c, int *speculatively_read)
{
    dinkum_scanf_string_info_t *info = (dinkum_scanf_string_info_t *)cl;
    char prev;
    if (info->prev_char != (char)EOF)
    {
        UNGETN(info->px, (int)c);
        info->px->nget++; /* shouldn't UNGETN do this? */
        prev = info->prev_char;
        /* if we try to replace more, we will fail */
        info->prev_char = (char)EOF;
    }
    else
    {
        info->input_failure = true;
        prev = '0'; /* valid for any further processing */
    }
    *speculatively_read -= 1;
    return prev;
}

/* this function stores the current position (the end of the valid 
** portion of the input) for safe-keeping.
*/
void
set_end_position_dinkum_scanf(void *cl)
{
    /* no need to record end ptr */
}

/* this function records an overflow state.
*/
void
set_overflow_dinkum_scanf(void *cl)
{
    /* no need to record overflow */
}

/* End of file */
