/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : SystemUart.h
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/6/20
  Last Modified :
  Description   : SystemUart.c header file
  Function List :
  History       :
  1.Date        : 2015/6/20
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/

#ifndef __SYSTEMUART_H__
#define __SYSTEMUART_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#ifdef _DRI_SYS_UART_
#define SYS_UART_EXT
#else
#define SYS_UART_EXT    extern
#endif

SYS_UART_EXT void SysUartInit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SYSTEMUART_H__ */
