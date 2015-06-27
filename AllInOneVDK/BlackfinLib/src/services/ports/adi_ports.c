/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_ports.c,v $
$Revision: 2386 $
$Date: 2010-03-25 17:26:08 -0400 (Thu, 25 Mar 2010) $

Description:
            Port Configuration module for the System Services Library

*********************************************************************************/

#include <services/services.h>

#if defined(__ADSP_BRAEMAR__)
#include "adi_ports_bf534.c"        // Braemar class port control
#endif

#if defined(__ADSP_STIRLING__)
#include "adi_ports_bf538.c"        // Stirling class port control
#endif

#if defined(__ADSP_MOAB__)
#include "adi_ports_bf54x.c"        // Moab class port control
#endif

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__)
#include "adi_ports_bf52x.c"        // Kookaburra/Mockingbird/Brodie class port control
#endif

#if defined(__ADSP_MOY__)
#include "adi_ports_bf50x.c"        // Moy class port control
#endif

#if defined(__ADSP_DELTA__)
#include "adi_ports_bf59x.c"        // Moy class port control
#endif
