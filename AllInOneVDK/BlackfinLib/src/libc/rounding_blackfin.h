/**************************************************************************
 *
 * rounding_blackfin.h : $Revision: 1.5.22.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#include <ccblkfn.h>
#include <sysreg.h>
#include <sys/platform.h>

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_19_15)
#endif /* _MISRA_RULES */

#define BIASED_ROUNDING ((sysreg_read(reg_ASTAT) & ASTAT_RND_MOD) != 0U)

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

/* End of file */
