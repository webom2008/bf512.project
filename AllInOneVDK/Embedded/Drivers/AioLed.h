/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : AioLed.h
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/6/20
  Last Modified :
  Description   : AioLed.c header file
  Function List :
  History       :
  1.Date        : 2015/6/20
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/

#ifndef __AIOLED_H__
#define __AIOLED_H__

/* disable misra diagnostics as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#include <services/services_misra_suppress.h>
#endif /* _MISRA_RULES */

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */



enum {                                                  // Command IDs
    AIO_LED_CMD_ON,
    AIO_LED_CMD_OFF,
};

/*********************************************************************

Data Structures

*********************************************************************/

extern ADI_DEV_PDD_ENTRY_POINT  AIOLedEntryPoint;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif /* __AIOLED_H__ */
