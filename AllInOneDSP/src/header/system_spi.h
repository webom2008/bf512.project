/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           system_spi.h
** Last modified Date:  2013-12-02
** Last Version:        V1.0
** Descriptions:        system_spi
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-12-02
** Version:             V1.00
** Descriptions:        system_spi
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-12-02
** Version:             V1.00
** Descriptions:        system_spi
**
** Rechecked by:        
**--------------------------------------------------------------------------------------------------------
** Modified by:         
** Modified date:       
** Version:             
** Descriptions:        
**
** Rechecked by:        
*********************************************************************************************************/
#ifndef __SYSTEM_SPI_H_
#define __SYSTEM_SPI_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

#ifdef __SYSTEM_SPI_C_

/** @defgroup SPI_mode 
  * @{
  */

#define SPI_Mode_Master                 ((uint16_t)0x0000)
#define SPI_Mode_Slave                  ((uint16_t)0x0001)
#define IS_SPI_MODE(MODE) (((MODE) == SPI_Mode_Master) || \
                           ((MODE) == SPI_Mode_Slave))
/**
  * @}
  */

/** @defgroup SPI_data_size 
  * @{
  */

#define SPI_DataSize_8b                 ((uint16_t)0x0000)
#define SPI_DataSize_16b                ((uint16_t)0x0001)
#define IS_SPI_DATASIZE(DATASIZE) (((DATASIZE) == SPI_DataSize_16b) || \
                                   ((DATASIZE) == SPI_DataSize_8b))
/**
  * @}
  */ 

/** @defgroup SPI_Clock_Polarity 
  * @{
  */

#define SPI_CPOL_Low                    ((uint16_t)0x0000)
#define SPI_CPOL_High                   ((uint16_t)0x0001)
#define IS_SPI_CPOL(CPOL) (((CPOL) == SPI_CPOL_Low) || \
                           ((CPOL) == SPI_CPOL_High))
/**
  * @}
  */

/** @defgroup SPI_Clock_Phase 
  * @{
  */

#define SPI_CPHA_1Edge                  ((uint16_t)0x0000)
#define SPI_CPHA_2Edge                  ((uint16_t)0x0001)
#define IS_SPI_CPHA(CPHA) (((CPHA) == SPI_CPHA_1Edge) || \
                           ((CPHA) == SPI_CPHA_2Edge))
/**
  * @}
  */

/** @defgroup SPI_Slave_Select_management 
  * @{
  */

#define SPI_NSS_Soft                    ((uint16_t)0x0000)
#define SPI_NSS_Hard                    ((uint16_t)0x0001)
#define IS_SPI_NSS(NSS) (((NSS) == SPI_NSS_Soft) || \
                         ((NSS) == SPI_NSS_Hard))
/** @defgroup SPI_Slave_Select_management 
  * @{
  */

#define SPI_SSEL0                    ((uint16_t)0x0000)
#define SPI_SSEL1                    ((uint16_t)0x0001)
#define SPI_SSEL2                    ((uint16_t)0x0002)
#define SPI_SSEL3                    ((uint16_t)0x0003)
#define SPI_SSEL4                    ((uint16_t)0x0004)
#define SPI_SSEL5                    ((uint16_t)0x0005)
#define SPI_SSEL6                    ((uint16_t)0x0006)
#define SPI_SSEL7                    ((uint16_t)0x0007)


/** @defgroup SPI_MSB_LSB_transmission 
  * @{
  */

#define SPI_FirstBit_MSB                ((uint16_t)0x0000)
#define SPI_FirstBit_LSB                ((uint16_t)0x0001)
#define IS_SPI_FIRST_BIT(BIT) (((BIT) == SPI_FirstBit_MSB) || \
                               ((BIT) == SPI_FirstBit_LSB))


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

  uint16_t SPI_Mode;                /*!< Specifies the SPI operating mode.
                                         This parameter can be a value of @ref SPI_mode */

  uint16_t SPI_CPOL;                /*!< Specifies the serial clock steady state.
                                         This parameter can be a value of @ref SPI_Clock_Polarity */

  uint16_t SPI_CPHA;                /*!< Specifies the clock active edge for the bit capture.
                                         This parameter can be a value of @ref SPI_Clock_Phase */

  uint16_t SPI_FirstBit;            /*!< Specifies whether data transfers start from MSB or LSB bit.
                                         This parameter can be a value of @ref SPI_MSB_LSB_transmission */
                                         
  uint16_t SPI_DataSize;            /*!< Specifies the SPI data size.
                                         This parameter can be a value of @ref SPI_data_size */
                                             
  uint16_t SPI_NSS;                 /*!< Specifies whether the NSS signal is managed by
                                         hardware (NSS pin) or by software using the SSI bit.
                                         This parameter can be a value of @ref SPI_Slave_Select_management */
  uint16_t SPI_SSEL;
                        
            
  uint32_t SPI_BaudRate;   /*!< Specifies the Baud Rate prescaler value which will be
                                         used to configure the transmit and receive SCK clock.
                                         This parameter can be a value of @ref SPI_BaudRate_Prescaler
                                         @note The communication clock is derived from the master
                                               clock. The slave clock does not need to be set. */

}SPI_InitTypeDef;

#define SPI0_BASE           (SYSTEM_MMR_BASE + 0x0500)
#define SPI1_BASE           (SYSTEM_MMR_BASE + 0x3400)

#define SPI0                ((SPI_TypeDef *) SPI0_BASE)
#define SPI1                ((SPI_TypeDef *) SPI1_BASE)

#define Function_SPI0        (Function_First)
#define Function_SPI1        (Function_Second)

typedef struct SPI_DEVICE
{
    SPI_TypeDef *SPIx;
    u8 ch;
    u8 irq;
    u8 ssel;
    u8 *rx_buf;
    u8 *tx_buf;
    u16 rx_len;
    u16 tx_len;

}spi_device;

extern spi_device *pSpi_dev;

extern int system_spi_init(void);
extern int system_spi_server(void);
extern int system_spi_write_byte(u8 data);
extern int system_spi_write_buf(u8 *cmd,u8 cmd_len,u8 *buf,u16 len);
extern u8 system_spi_read_byte(void);
extern int system_spi_read_buf(u8 *cmd,u16 cmd_len,u8 *buf,u16 len);

#endif //__SYSTEM_SPI_C_

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus               */

#endif                                                                  /*  __SYSTEM_SPI_H_           */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
