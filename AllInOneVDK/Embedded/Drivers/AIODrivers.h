/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : AIODrivers.h
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/6/20
  Last Modified :
  Description   : AIODrivers.c header file
  Function List :
  History       :
  1.Date        : 2015/6/20
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/

#ifndef __AIODRIVERS_H__
#define __AIODRIVERS_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#ifdef _AIO_DRI_C_
#define AIO_DRI_EXT
#else
#define AIO_DRI_EXT    extern
#endif

AIO_DRI_EXT void InitDeviceDrivers(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __AIODRIVERS_H__ */
