/************************************************************************
 *
 * xget_tm.h 
 *
 * (c) Copyright 2002-2004 Analog Devices, Inc.  All rights reserved.
 * $Revision: 3542 $
 ************************************************************************/

/* Header file for internal users of the function __broken_down_time */

#ifndef __XGET_TM_H

#define __XGET_TM_H

#include <time.h>

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_6_3:"ADI header uses basic types")
#pragma diag(suppress:misra_rule_20_2:"ADI standard header")
#endif /* _MISRA_RULES */

struct tm *
__broken_down_time(time_t _secs, char _isgmtime, struct tm *_tm_ptr);

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif
