/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : includes.h
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/6/20
  Last Modified :
  Description   : includes.h header file
  Function List :
  History       :
  1.Date        : 2015/6/20
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/

#ifndef __INCLUDES_H__
#define __INCLUDES_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <stdio.h>

#include "Config/defines.h" // must include before blackfin lib.

//#include <cdefBF512.h>
//#include <ccblkfn.h>
//#include <sys/exception.h>
//#include <services/services.h>		// system service includes
//#include <services/stdio/adi_stdio.h>
//#include <drivers/adi_dev.h>        // device manager

#include <adi_types.h>
#include <drivers/adi_dev.h>
#include <drivers/uart/adi_uart.h>		// UART device driver includes
#include <services/services.h>
#include <services/dma/adi_dma.h>
#include <services/flag/adi_flag.h>
#include <services/int/adi_int.h>
#include <services/tmr/adi_tmr.h>

#include "Config/typedefs.h"
#include "CVTELib/cvteBF51x.h"
//#include "AIODrivers/SystemUart.h"
//#include "AIODrivers/AIODrivers.h"
//#include "AIODrivers/AioLed.h"
#include "Embedded/Platform/adi_ssl_Init.h" /* adapted from ./blackfin/Examples/Common Code */
#include "Embedded/Drivers/adi_stdio.h"




#ifdef _AIO_DEBUG_
#define PRINT_INFO(format,...) printf("\r\nFILE: "__FILE__", LINE: %d: "format, __LINE__, ##__VA_ARGS__)
//#define PRINT_INFO(format,...) printf("\r\nLINE: %d: "format, __LINE__, ##__VA_ARGS__)
#else
#define PRINT_INFO(format,...) do{}while(0)
#endif


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __INCLUDES_H__ */

