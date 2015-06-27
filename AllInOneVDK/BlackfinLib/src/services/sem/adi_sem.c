/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_sem.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This file contains the code for the semaphore service

*********************************************************************************/

#include <services/services.h>



/*********************************************************************

Include operating environment specific functionality

*********************************************************************/

#if defined(ADI_SSL_STANDALONE)
#include "adi_sem_standalone.c"
#endif

#if defined(ADI_SSL_VDK)
#include "adi_sem_vdk.c"
#endif

#if defined(ADI_SSL_THREADX)
#include "adi_sem_threadx.c"
#endif


#if defined(ADI_SSL_INTEGRITY)
#include "adi_sem_integrity.c"
#endif

#if defined(ADI_SSL_UCOS)
#include "adi_sem_ucos.c"
#endif

