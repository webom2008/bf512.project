/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : cvteBF51x_SPI.h
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/2/11
  Last Modified :
  Description   : cvteBF51x_SPI.c header file
  Function List :
  History       :
  1.Date        : 2015/2/11
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/
#ifndef __CVTEBF51X_SPI_H__
#define __CVTEBF51X_SPI_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "cvteBF51x.h"

#define SPI_Mode_Master                 ((u16)0x0000)
#define SPI_Mode_Slave                  ((u16)0x0001)

#define SPI_DataSize_8b                 ((u16)0x0000)
#define SPI_DataSize_16b                ((u16)0x0001)

#define SPI_CPOL_Low                    ((u16)0x0000)
#define SPI_CPOL_High                   ((u16)0x0001)

#define SPI_CPHA_1Edge                  ((u16)0x0000)
#define SPI_CPHA_2Edge                  ((u16)0x0001)

#define SPI_NSS_Soft                    ((u16)0x0000)
#define SPI_NSS_Hard                    ((u16)0x0001)

#define SPI_SSEL1                    ((u16)0x0001)
#define SPI_SSEL2                    ((u16)0x0002)
#define SPI_SSEL3                    ((u16)0x0003)
#define SPI_SSEL4                    ((u16)0x0004)
#define SPI_SSEL5                    ((u16)0x0005)

#define SPI_FirstBit_MSB                ((u16)0x0000)
#define SPI_FirstBit_LSB                ((u16)0x0001)

#define SPI_CTL_SPE             (1<<14)
#define SPI_CTL_WOM             (1<<13)
#define SPI_CTL_MSTR            (1<<12)
#define SPI_CTL_CPOL            (1<<11)
#define SPI_CTL_CPHA            (1<<10)
#define SPI_CTL_LSBF            (1<<9)
#define SPI_CTL_SIZE            (1<<8)
#define SPI_CTL_EMISO           (1<<5)
#define SPI_CTL_PSSE            (1<<4)
#define SPI_CTL_GM              (1<<3)
#define SPI_CTL_SZ              (1<<2)
#define SPI_CTL_TIMOD_RD        (0<<0)
#define SPI_CTL_TIMOD_TD        (1<<0)
#define SPI_CTL_TIMOD_RD_DMA    (2<<0)
#define SPI_CTL_TIMOD_TD_DMA    (3<<0)
#define SPI_CTL_TIMOD           (3<<0)

#define SPI_FLG_FLG7            (1<<15)
#define SPI_FLG_FLG6            (1<<14)
#define SPI_FLG_FLG5            (1<<13)
#define SPI_FLG_FLG4            (1<<12)
#define SPI_FLG_FLG3            (1<<11)
#define SPI_FLG_FLG2            (1<<10)
#define SPI_FLG_FLG1            (1<<9)
#define SPI_FLG_FLS7            (1<<7)
#define SPI_FLG_FLS6            (1<<6)
#define SPI_FLG_FLS5            (1<<5)
#define SPI_FLG_FLS4            (1<<4)
#define SPI_FLG_FLS3            (1<<3)
#define SPI_FLG_FLS2            (1<<2)
#define SPI_FLG_FLS1            (1<<1)

#define SPI_STAT_TXCOL          (1<<6)
#define SPI_STAT_RXS            (1<<5)
#define SPI_STAT_RBSY           (1<<4)
#define SPI_STAT_TXS            (1<<3)
#define SPI_STAT_TXE            (1<<2)
#define SPI_STAT_MODF           (1<<1)
#define SPI_STAT_SPIF           (1<<0)

typedef struct
{
  volatile u16 CTL;         /*!< SPI Control register,                        Address offset: 0x00 */
  u16  RESERVED0;           /*!< Reserved,                                    0x02                 */
  volatile u16 FLG;         /*!< SPI Flag register,                           Address offset: 0x04 */
  u16  RESERVED1;           /*!< Reserved,                                    0x06                 */
  volatile u16 STAT;        /*!< SPI Status register,                         Address offset: 0x08 */
  u16  RESERVED2;           /*!< Reserved,                                    0x0A                 */
  volatile u16 TDBR;        /*!< SPI Transmit Data Buffer register,           Address offset: 0x0C */
  u16  RESERVED3;           /*!< Reserved,                                    0x0E                 */
  volatile u16 RDBR;        /*!< SPI Receive Data Buffer register,            Address offset: 0x10 */
  u16  RESERVED4;           /*!< Reserved,                                    0x12                 */
  volatile u16 BAUD;        /*!< SPI Baudrate register,                       Address offset: 0x14 */
  u16  RESERVED5;           /*!< Reserved,                                    0x16                 */
  volatile u16 SHADOW;      /*!< SPI Shadow register,                         Address offset: 0x18 */
  u16  RESERVED6;           /*!< Reserved,                                    0x1A                 */
    
}SPI_TypeDef;

typedef struct
{

  u16 SPI_Mode;                /*!< Specifies the SPI operating mode.
                                         This parameter can be a value of @ref SPI_mode */

  u16 SPI_CPOL;                /*!< Specifies the serial clock steady state.
                                         This parameter can be a value of @ref SPI_Clock_Polarity */

  u16 SPI_CPHA;                /*!< Specifies the clock active edge for the bit capture.
                                         This parameter can be a value of @ref SPI_Clock_Phase */

  u16 SPI_FirstBit;            /*!< Specifies whether data transfers start from MSB or LSB bit.
                                         This parameter can be a value of @ref SPI_MSB_LSB_transmission */
                                         
  u16 SPI_DataSize;            /*!< Specifies the SPI data size.
                                         This parameter can be a value of @ref SPI_data_size */
                                             
  u16 SPI_NSS;                 /*!< Specifies whether the NSS signal is managed by
                                         hardware (NSS pin) or by software using the SSI bit.
                                         This parameter can be a value of @ref SPI_Slave_Select_management */
  u16 SPI_SSEL;
                        
            
  u32 SPI_BaudRate;   /*!< Specifies the Baud Rate prescaler value which will be
                                         used to configure the transmit and receive SCK clock.
                                         This parameter can be a value of @ref SPI_BaudRate_Prescaler
                                         @note The communication clock is derived from the master
                                               clock. The slave clock does not need to be set. */
  u32 SCLK;                                             

}SPI_InitTypeDef;

#define SPI0_BASE           (SYSTEM_MMR_BASE + 0x0500)
#define SPI1_BASE           (SYSTEM_MMR_BASE + 0x3400)

#define SPI0                ((SPI_TypeDef *) SPI0_BASE)
#define SPI1                ((SPI_TypeDef *) SPI1_BASE)

typedef struct SPI_DEVICE
{
    SPI_TypeDef *SPIx;
    u16  ssel;
    u16  nss;
    u16  delay;
} spi_device;

int SPI_Init(spi_device* pDev, SPI_InitTypeDef* pSPI_InitStruct);
int SPI_write_byte(spi_device* pDev, u8 data);
int SPI_write_buf(spi_device* pDev, u8 *cmd,u8 cmd_len,u8 *buf,u16 len);
u8  SPI_read_byte(spi_device* pDev);
int SPI_read_buf(spi_device* pDev, u8 *cmd,u16 cmd_len,u8 *buf,u16 len);
void SPI_Cfg_prinf(spi_device* pDev);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __CVTEBF51X_SPI_H__ */
