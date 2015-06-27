/**************************************************************************
 *
 * rounding.h : $Revision: 1.2.6.1 $
 * (c) Copyright 2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__ADSPBLACKFIN__)
#include <rounding_blackfin.h>
#elif defined(__ADSP21000__)
#include <rounding_sharc.h>
#else
#error No rounding header for this platform
#endif

/* End of file */
