/************************************************************************
 *
 * xdays_to_mon.h
 *
 * (c) Copyright 2002-2004 Analog Devices, Inc.  All rights reserved.
 * $Revision: 4 $
 ************************************************************************/

/* Header file for internal run time library functions used by time.h. */

#ifndef __XDAYS_TO_MON_H

#define __XDAYS_TO_MON_H

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_6_3:"ADI header uses basic types")
#pragma diag(suppress:misra_rule_8_12:"ADI standard header does not specify bounds")
#pragma diag(suppress:misra_rule_19_7:"ADI header allows function macros")
#endif /* _MISRA_RULES */

extern const short _days_to_mon[];
extern const short _ldays_to_mon[];

#define DAYS_TO_MONTH(year) ((((year) & 0x3u) || (0u == (year))) ? _days_to_mon \
                                                           : _ldays_to_mon)

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif
