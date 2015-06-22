/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : cvteBF51x_TWI.h
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/3/2
  Last Modified :
  Description   : cvteBF51x_TWI.c header file
  Function List :
  History       :
  1.Date        : 2015/3/2
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/
#ifndef __CVTEBF51X_TWI_H__
#define __CVTEBF51X_TWI_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "cvteBF51x.h"

typedef struct
{
  volatile u16 CLKDIV;          /*!< SCL Clock Divider Register,Address offset: 0x00 */
  u16  RESERVED0;               /*!< Reserved,                                  0x02 */
  volatile u16 CONTROL;         /*!< TWI CONTROL Register ,     Address offset: 0x04 */
  u16  RESERVED1;               /*!< Reserved,                                  0x06 */
  volatile u16 SLAVE_CTL;       /*!< TWI Slave Mode Control Register ,          0x08 */
  u16  RESERVED2;               /*!< Reserved,                                  0x0A */
  volatile u16 SLAVE_STAT;      /*!< TWI Slave Mode Status Register ,           0x0C */
  u16  RESERVED3;               /*!< Reserved,                                  0x0E */
  volatile u16 SLAVE_ADDR;      /*!< TWI Slave Mode Address Register,           0x10 */
  u16  RESERVED4;               /*!< Reserved,                                  0x12 */
  volatile u16 MASTER_CTL;      /*!< TWI Master Mode Control Register,          0x14 */
  u16  RESERVED5;               /*!< Reserved,                                  0x16 */
  volatile u16 MASTER_STAT;     /*!< TWI Master Mode Status Register,           0x18 */
  u16  RESERVED6;               /*!< Reserved,                                  0x1A */
  volatile u16 MASTER_ADDR;     /*!< TWI Master Mode Address Register ,         0x1C */
  u16  RESERVED7;               /*!< Reserved,                                  0x1E */
  
  volatile u16 INT_STAT;        /*!< TWI Interrupt Status Register ,            0x20 */
  u16  RESERVED8;               /*!< Reserved,                                  0x22 */
  volatile u16 INT_MASK;        /*!< TWI Interrupt Mask Register ,              0x24 */
  u16  RESERVED9;               /*!< Reserved,                                  0x26 */
  volatile u16 FIFO_CTL;        /*!< TWI FIFO Control Register ,                0x28 */
  u16  RESERVED10;              /*!< Reserved,                                  0x2A */
  volatile u16 FIFO_STAT;       /*!< TWI FIFO Status Register ,                 0x2C */
  u16  RESERVED11[41];          /*!< Reserved,                                  0x2E */
  
  volatile u16 XMT_DATA8;       /*!< TWI FIFO Transmit Data Single Byte Register ,  0x80 */
  u16  RESERVED12;              /*!< Reserved,                                      0x82 */
  volatile u16 XMT_DATA16;      /*!< TWI FIFO Transmit Data Double Byte Register,   0x84 */
  u16  RESERVED13;              /*!< Reserved,                                      0x86 */
  volatile u16 RCV_DATA8;       /*!< TWI FIFO Receive Data Single Byte Register,    0x88 */
  u16  RESERVED14;              /*!< Reserved,                                      0x8A */
  volatile u16 RCV_DATA16;      /*!< TWI FIFO Receive Data Double Byte Register,    0x8C */
  u16  RESERVED15;              /*!< Reserved,                                      0x8E */
    
} TWI_TypeDef;

typedef struct
{
    u32 sclk;
    u16 TWI_ClockSpeed;
    u16 TWI_DutyCycle;
//    u16 TWI_Mode;
//    u16 TWI_OwnAddress1;
//    u16 TWI_Ack;
//    u16 TWI_AcknowledgedAddress;
    
    void (*pTWI_Delay)(const unsigned long ms);
} TWI_InitStruct;

#define TWI0_BASE           (SYSTEM_MMR_BASE + 0x1400)
#define TWI0                ((TWI_TypeDef *) TWI0_BASE)

#define TWI_ClockSpeed_100K             ((u16)100)
#define TWI_ClockSpeed_400K             ((u16)400)

#define TWI_Mode_I2C                    ((u16)0x0000)

#define TWI_DutyCycle_16_9              ((u16)1609) /*!< I2C fast mode Tlow/Thigh = 16/9 */
#define TWI_DutyCycle_2                 ((u16)55) /*!< I2C fast mode Tlow/Thigh = 2 */

#define TWI_Ack_Enable                  ((u16)1)
#define TWI_Ack_Disable                 ((u16)0)

#define TWI_AcknowledgedAddress_7bit    ((u16)1)
#define TWI_AcknowledgedAddress_10bit   ((u16)0)


#define TWI_CONTROL_RESET               ((u16)0x0000)
#define TWI_CONTROL_ENA                 ((u16)(1<<7))
#define TWI_CONTROL_SCCB                ((u16)(1<<8))

#define TWI_FIFO_CTL_XMTFLUSH           ((u16)(1<<0))
#define TWI_FIFO_CTL_RCVFLUSH           ((u16)(1<<1))
#define TWI_FIFO_CTL_XMTINTLEN          ((u16)(1<<2))
#define TWI_FIFO_CTL_RCVINTLEN          ((u16)(1<<3))

#define TWI_FIFO_STAT_XMTSTAT           ((u16)(0x03<<0))
#define TWI_FIFO_STAT_RCVSTAT           ((u16)(0x03<<2))
#define TWI_FIFO_STAT_RCV_EMPTY         ((u16)(0x0<<2))
#define TWI_FIFO_STAT_RCV_ONE           ((u16)(0x1<<2))
#define TWI_FIFO_STAT_RCV_RESERVED      ((u16)(0x2<<2))
#define TWI_FIFO_STAT_RCV_FULL          ((u16)(0x3<<2))
#define TWI_FIFO_STAT_XMT_EMPTY         ((u16)(0x0))
#define TWI_FIFO_STAT_XMT_ONE           ((u16)(0x1))
#define TWI_FIFO_STAT_XMT_RESERVED      ((u16)(0x2))
#define TWI_FIFO_STAT_XMT_FULL          ((u16)(0x3))

#define TWI_MASTER_STAT_MPROG           ((u16)(1<<0))
#define TWI_MASTER_STAT_LOSTARB         ((u16)(1<<1))
#define TWI_MASTER_STAT_ANAK            ((u16)(1<<2))
#define TWI_MASTER_STAT_DNAK            ((u16)(1<<3))
#define TWI_MASTER_STAT_BUFRDERR        ((u16)(1<<4))
#define TWI_MASTER_STAT_BUFWRERR        ((u16)(1<<5))
#define TWI_MASTER_STAT_SDASEN          ((u16)(1<<6))
#define TWI_MASTER_STAT_SCLSEN          ((u16)(1<<7))
#define TWI_MASTER_STAT_BUSBUSY         ((u16)(1<<8))

#define TWI_MASTER_CTL_MEN              ((u16)(1<<0))
#define TWI_MASTER_CTL_MDIR             ((u16)(1<<2))
#define TWI_MASTER_CTL_FAST             ((u16)(1<<3))
#define TWI_MASTER_CTL_STOP             ((u16)(1<<4))
#define TWI_MASTER_CTL_RSTART           ((u16)(1<<5))
#define TWI_MASTER_CTL_DCNT             ((u16)(0xFF<<6))
#define TWI_MASTER_CTL_SCLOVR           ((u16)(1<<14))
#define TWI_MASTER_CTL_SDAOVR           ((u16)(1<<15))

#define TWI_INT_MASK_SINITM             ((u16)(1<<0))
#define TWI_INT_MASK_SCOMPM             ((u16)(1<<1))
#define TWI_INT_MASK_SERRM              ((u16)(1<<2))
#define TWI_INT_MASK_SOVFM              ((u16)(1<<3))
#define TWI_INT_MASK_MCOMPM             ((u16)(1<<4))
#define TWI_INT_MASK_MERRM              ((u16)(1<<5))
#define TWI_INT_MASK_XMTSERVM           ((u16)(1<<6))
#define TWI_INT_MASK_RCVSERVM           ((u16)(1<<7))

#define TWI_INT_STAT_SINIT              ((u16)(1<<0))
#define TWI_INT_STAT_SCOMP              ((u16)(1<<1))
#define TWI_INT_STAT_SERR               ((u16)(1<<2))
#define TWI_INT_STAT_SOVF               ((u16)(1<<3))
#define TWI_INT_STAT_MCOMP              ((u16)(1<<4))
#define TWI_INT_STAT_MERR               ((u16)(1<<5))
#define TWI_INT_STAT_XMTSERV            ((u16)(1<<6))
#define TWI_INT_STAT_RCVSERV            ((u16)(1<<7))

int TWI_Init(TWI_TypeDef *pDev, TWI_InitStruct *pInitStruct);
int TWI_Reset(TWI_TypeDef *pDev);

int TWI_MasterRead( TWI_TypeDef *pDev,
                    const unsigned short DeviceAddr,
                    unsigned char *TWI_Data_Pointer,
                    const unsigned short TWI_Length);
int TWI_MasterWrite(TWI_TypeDef *pDev,
                    const unsigned short DeviceAddr,
                    unsigned char *TWI_Data_Pointer,
                    const unsigned short Count,
                    const unsigned short TWI_Length);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __CVTEBF51X_TWI_H__ */
