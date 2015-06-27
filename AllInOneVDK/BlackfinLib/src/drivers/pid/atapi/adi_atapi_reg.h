/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_atapi_reg.h,v $
$Revision: 3459 $
$Date: 2010-09-29 10:37:28 -0400 (Wed, 29 Sep 2010) $

Description:
            This header file defines the MMRs for the ATAPI interface. It includes
            both the MMR addresses and the bitfield access macros.

*********************************************************************************/


#define ADI_ATA_REG_BASE                        0xFFC03800

#define ADI_ATA_CTRL_REG_OFFSET                 0x0000
#define ADI_ATA_STATUS_REG_OFFSET               0x0004
#define ADI_ATA_DEV_ADDR_REG_OFFSET             0x0008
#define ADI_ATA_DEV_TXBUF_REG_OFFSET            0x000C
#define ADI_ATA_DEV_RXBUF_REG_OFFSET            0x0010
#define ADI_ATA_INT_MASK_REG_OFFSET             0x0014
#define ADI_ATA_INT_STATUS_REG_OFFSET           0x0018
#define ADI_ATA_XFER_LEN_REG_OFFSET             0x001C
#define ADI_ATA_LINE_STATUS_REG_OFFSET          0x0020
#define ADI_ATA_SM_STATE_REG_OFFSET             0x0024
#define ADI_ATA_TERMINATE_REG_OFFSET            0x0028
#define ADI_ATA_PIO_TFRCNT_REG_OFFSET           0x002C
#define ADI_ATA_DMA_TFRCNT_REG_OFFSET           0x0030
#define ADI_ATA_UDMAIN_TFRCNT_REG_OFFSET        0x0034
#define ADI_ATA_UDMAOUT_TFRCNT_REG_OFFSET       0x0038

#define ADI_ATA_REG_TIM_0_REG_OFFSET            0x0040
#define ADI_ATA_PIO_TIM_0_REG_OFFSET            0x0044
#define ADI_ATA_PIO_TIM_1_REG_OFFSET            0x0048

#define ADI_ATA_MDMA_TIM_0_REG_OFFSET           0x0050
#define ADI_ATA_MDMA_TIM_1_REG_OFFSET           0x0054
#define ADI_ATA_MDMA_TIM_2_REG_OFFSET           0x0058

#define ADI_ATA_UDMA_TIM_0_REG_OFFSET           0x0060
#define ADI_ATA_UDMA_TIM_1_REG_OFFSET           0x0064
#define ADI_ATA_UDMA_TIM_2_REG_OFFSET           0x0068
#define ADI_ATA_UDMA_TIM_3_REG_OFFSET           0x006C

/************************************************************************************************************
 *  ATA Control Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_CTRL                            (ADI_ATA_REG_BASE + ADI_ATA_CTRL_REG_OFFSET)
#define pADI_ATA_CTRL                           ((volatile u16*)ADI_ATA_CTRL)

/* Start PIO/Register Operation*/
#define ADI_ATA_CTRL_PIO_START_MASK             0x0001
#define ADI_ATA_CTRL_PIO_START_SHIFT            0
#define ADI_ATA_CTRL_PIO_START_SET(R,V)         ( (R) = ( (V) << ADI_ATA_CTRL_PIO_START_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_PIO_START_MASK ) )
#define ADI_ATA_CTRL_PIO_START_GET(R)           ( ( (R) & ADI_ATA_CTRL_PIO_START_MASK ) >> ADI_ATA_CTRL_PIO_START_SHIFT )

/* Start Multi-word DMA Operation */
#define ADI_ATA_CTRL_MDMA_START_MASK            0x0002
#define ADI_ATA_CTRL_MDMA_START_SHIFT           1
#define ADI_ATA_CTRL_MDMA_START_SET(R,V)        ( (R) = ( (V) << ADI_ATA_CTRL_MDMA_START_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_MDMA_START_MASK ) )
#define ADI_ATA_CTRL_MDMA_START_GET(R)          ( ( (R) & ADI_ATA_CTRL_MDMA_START_MASK ) >> ADI_ATA_CTRL_MDMA_START_SHIFT )

/* Start Ultra DMA Operation */
#define ADI_ATA_CTRL_UDMA_START_MASK            0x0004
#define ADI_ATA_CTRL_UDMA_START_SHIFT           2
#define ADI_ATA_CTRL_UDMA_START_SET(R,V)        ( (R) = ( (V) << ADI_ATA_CTRL_UDMA_START_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_UDMA_START_MASK ) )
#define ADI_ATA_CTRL_UDMA_START_GET(R)          ( ( (R) & ADI_ATA_CTRL_UDMA_START_MASK ) >> ADI_ATA_CTRL_UDMA_START_SHIFT )

/* Transfer direction */
#define ADI_ATA_CTRL_XFER_DIR_MASK              0x0008
#define ADI_ATA_CTRL_XFER_DIR_SHIFT             3
#define ADI_ATA_CTRL_XFER_DIR_SET(R,V)          ( (R) = ( (V) << ADI_ATA_CTRL_XFER_DIR_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_XFER_DIR_MASK ) )
#define ADI_ATA_CTRL_XFER_DIR_GET(R)            ( ( (R) & ADI_ATA_CTRL_XFER_DIR_MASK ) >> ADI_ATA_CTRL_XFER_DIR_SHIFT )

/* IORDY enable */
#define ADI_ATA_CTRL_IORDY_EN_MASK              0x0010
#define ADI_ATA_CTRL_IORDY_EN_SHIFT             4
#define ADI_ATA_CTRL_IORDY_EN_SET(R,V)          ( (R) = ( (V) << ADI_ATA_CTRL_IORDY_EN_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_IORDY_EN_MASK ) )
#define ADI_ATA_CTRL_IORDY_EN_GET(R)            ( ( (R) & ADI_ATA_CTRL_IORDY_EN_MASK ) >> ADI_ATA_CTRL_IORDY_EN_SHIFT )

/* Flush FIFOs */
#define ADI_ATA_CTRL_FIFO_FLUSH_MASK            0x0020
#define ADI_ATA_CTRL_FIFO_FLUSH_SHIFT           5
#define ADI_ATA_CTRL_FIFO_FLUSH_SET(R,V)        ( (R) = ( (V) << ADI_ATA_CTRL_FIFO_FLUSH_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_FIFO_FLUSH_MASK ) )
#define ADI_ATA_CTRL_FIFO_FLUSH_GET(R)          ( ( (R) & ADI_ATA_CTRL_FIFO_FLUSH_MASK ) >> ADI_ATA_CTRL_FIFO_FLUSH_SHIFT )

/* Soft Reset */
#define ADI_ATA_CTRL_SOFT_RST_MASK              0x0040
#define ADI_ATA_CTRL_SOFT_RST_SHIFT             6
#define ADI_ATA_CTRL_SOFT_RST_SET(R,V)          ( (R) = ( (V) << ADI_ATA_CTRL_SOFT_RST_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_SOFT_RST_MASK ) )
#define ADI_ATA_CTRL_SOFT_RST_GET(R)            ( ( (R) & ADI_ATA_CTRL_SOFT_RST_MASK ) >> ADI_ATA_CTRL_SOFT_RST_SHIFT )

/* Device Reset */
#define ADI_ATA_CTRL_DEV_RST_MASK               0x0080
#define ADI_ATA_CTRL_DEV_RST_SHIFT              7
#define ADI_ATA_CTRL_DEV_RST_SET(R,V)           ( (R) = ( (V) << ADI_ATA_CTRL_DEV_RST_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_DEV_RST_MASK ) )
#define ADI_ATA_CTRL_DEV_RST_GET(R)             ( ( (R) & ADI_ATA_CTRL_DEV_RST_MASK ) >> ADI_ATA_CTRL_DEV_RST_SHIFT )

/* Transfer count reset */
#define ADI_ATA_CTRL_TFRCNT_RST_MASK            0x0100
#define ADI_ATA_CTRL_TFRCNT_RST_SHIFT           8
#define ADI_ATA_CTRL_TFRCNT_RST_SET(R,V)        ( (R) = ( (V) << ADI_ATA_CTRL_TFRCNT_RST_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_TFRCNT_RST_MASK ) )
#define ADI_ATA_CTRL_TFRCNT_RST_GET(R)          ( ( (R) & ADI_ATA_CTRL_TFRCNT_RST_MASK ) >> ADI_ATA_CTRL_TFRCNT_RST_SHIFT )

/* end/terminate select */
#define ADI_ATA_CTRL_END_ON_TERM_MASK           0x0200
#define ADI_ATA_CTRL_END_ON_TERM_SHIFT          9
#define ADI_ATA_CTRL_END_ON_TERM_SET(R,V)       ( (R) = ( (V) << ADI_ATA_CTRL_END_ON_TERM_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_END_ON_TERM_MASK ) )
#define ADI_ATA_CTRL_END_ON_TERM_GET(R)         ( ( (R) & ADI_ATA_CTRL_END_ON_TERM_MASK ) >> ADI_ATA_CTRL_END_ON_TERM_SHIFT )

/* PIO-DMA enable */
#define ADI_ATA_CTRL_PIO_USE_DMA_MASK           0x0400
#define ADI_ATA_CTRL_PIO_USE_DMA_SHIFT          10
#define ADI_ATA_CTRL_PIO_USE_DMA_SET(R,V)       ( (R) = ( (V) << ADI_ATA_CTRL_PIO_USE_DMA_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_PIO_USE_DMA_MASK ) )
#define ADI_ATA_CTRL_PIO_USE_DMA_GET(R)         ( ( (R) & ADI_ATA_CTRL_PIO_USE_DMA_MASK ) >> ADI_ATA_CTRL_PIO_USE_DMA_SHIFT )

/* Ultra DMA-IN FIFO Threshold */
#define ADI_ATA_CTRL_UDMAIN_FIFO_THRS_MASK      0xF000
#define ADI_ATA_CTRL_UDMAIN_FIFO_THRS_SHIFT     12
#define ADI_ATA_CTRL_UDMAIN_FIFO_THRS_SET(R,V)  ( (R) = ( (V) << ADI_ATA_CTRL_UDMAIN_FIFO_THRS_SHIFT ) | ( (R) & ~ADI_ATA_CTRL_UDMAIN_FIFO_THRS_MASK ) )
#define ADI_ATA_CTRL_UDMAIN_FIFO_THRS_GET(R)    ( ( (R) & ADI_ATA_CTRL_UDMAIN_FIFO_THRS_MASK ) >> ADI_ATA_CTRL_UDMAIN_FIFO_THRS_SHIFT )


/************************************************************************************************************
 *  ATA Status Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_STATUS                              (ADI_ATA_REG_BASE + ADI_ATA_STATUS_REG_OFFSET)
#define pADI_ATA_STATUS                             ((volatile u16*)ADI_ATA_STATUS)

/* PIO Transfer in Progress (R0) */
#define ADI_ATA_STATUS_PIO_XFER_ON_MASK             0x0001
#define ADI_ATA_STATUS_PIO_XFER_ON_SHIFT            0
#define ADI_ATA_STATUS_PIO_XFER_ON_GET(R)           ( ( (R) & ADI_ATA_STATUS_PIO_XFER_ON_MASK ) >> ADI_ATA_STATUS_PIO_XFER_ON_SHIFT )

/* multi-word DMA Transfer in Progress (R0) */
#define ADI_ATA_STATUS_MDMA_XFER_ON_MASK            0x0002
#define ADI_ATA_STATUS_MDMA_XFER_ON_SHIFT           1
#define ADI_ATA_STATUS_MDMA_XFER_ON_GET(R)          ( ( (R) & ADI_ATA_STATUS_MDMA_XFER_ON_MASK ) >> ADI_ATA_STATUS_MDMA_XFER_ON_SHIFT )

/* Ultra DMA Transfer in Progress (R0) */
#define ADI_ATA_STATUS_UDMA_XFER_ON_MASK            0x0004
#define ADI_ATA_STATUS_UDMA_XFER_ON_SHIFT           2
#define ADI_ATA_STATUS_UDMA_XFER_ON_GET(R)          ( ( (R) & ADI_ATA_STATUS_UDMA_XFER_ON_MASK ) >> ADI_ATA_STATUS_UDMA_XFER_ON_SHIFT )


/* Ultra DMA input FIFO level (R0) */
#define ADI_ATA_STATUS_UDMA_IN_FL_MASK              0x00F0
#define ADI_ATA_STATUS_UDMA_IN_FL_SHIFT             4
#define ADI_ATA_STATUS_UDMA_IN_FL_GET(R)            ( ( (R) & ADI_ATA_STATUS_UDMA_IN_FL_MASK ) >> ADI_ATA_STATUS_UDMA_IN_FL_SHIFT )



/************************************************************************************************************
 *  ATA Device Address Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_DEV_ADDR                            (ADI_ATA_REG_BASE + ADI_ATA_DEV_ADDR_REG_OFFSET)
#define pADI_ATA_DEV_ADDR                           ((volatile u16*)ADI_ATA_DEV_ADDR)

#define ADI_ATA_DEV_ADDR_DEV_ADDR_MASK              0x001F
#define ADI_ATA_DEV_ADDR_DEV_ADDR_SHIFT             0
#define ADI_ATA_DEV_ADDR_DEV_ADDR_SET(R,V)          ( (R) = ( (V) << ADI_ATA_DEV_ADDR_DEV_ADDR_SHIFT ) | ( (R) & ~ADI_ATA_DEV_ADDR_DEV_ADDR_MASK ) )
#define ADI_ATA_DEV_ADDR_DEV_ADDR_GET(R)            ( ( (R) & ADI_ATA_DEV_ADDR_DEV_ADDR_MASK ) >> ADI_ATA_DEV_ADDR_DEV_ADDR_SHIFT )



/************************************************************************************************************
 *  ATA Device Transmit Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_DEV_TXBUF                               (ADI_ATA_REG_BASE + ADI_ATA_DEV_TXBUF_REG_OFFSET)
#define pADI_ATA_DEV_TXBUF                              ((volatile u16*)ADI_ATA_DEV_TXBUF)

#define ADI_ATA_DEV_TXBUF_SET(R,V)                      ( (R) = (u16)(V)  )
#define ADI_ATA_DEV_TXBUF_GET(R)                        ( (u16)(R) )



/************************************************************************************************************
 *  ATA Device Receive Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_DEV_RXBUF                               (ADI_ATA_REG_BASE + ADI_ATA_DEV_RXBUF_REG_OFFSET)
#define pADI_ATA_DEV_RXBUF                              ((volatile u16*)ADI_ATA_DEV_RXBUF)

#define ADI_ATA_DEV_RXBUF_SET(R,V)                      ( (R) = (u16)(V)  )
#define ADI_ATA_DEV_RXBUF_GET(R)                        ( (u16)(R) )



/************************************************************************************************************
 *  ATA Interrupt Mask Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_INT_MASK                                (ADI_ATA_REG_BASE + ADI_ATA_INT_MASK_REG_OFFSET)
#define pADI_ATA_INT_MASK                               ((volatile u16*)ADI_ATA_INT_MASK)

#define ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_MASK          0x0001
#define ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_SHIFT         0
#define ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_SET(R,V)      ( (R) = ( (V) << ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_SHIFT ) | ( (R) & ~ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_MASK ) )
#define ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_GET(R)        ( ( (R) & ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_MASK ) >> ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_SHIFT )

#define ADI_ATA_INT_MASK_PIO_DONE_MASK_MASK             0x0002
#define ADI_ATA_INT_MASK_PIO_DONE_MASK_SHIFT            1
#define ADI_ATA_INT_MASK_PIO_DONE_MASK_SET(R,V)         ( (R) = ( (V) << ADI_ATA_INT_MASK_PIO_DONE_MASK_SHIFT ) | ( (R) & ~ADI_ATA_INT_MASK_PIO_DONE_MASK_MASK ) )
#define ADI_ATA_INT_MASK_PIO_DONE_MASK_GET(R)           ( ( (R) & ADI_ATA_INT_MASK_PIO_DONE_MASK_MASK ) >> ADI_ATA_INT_MASK_PIO_DONE_MASK_SHIFT )

#define ADI_ATA_INT_MASK_MDMA_DONE_MASK_MASK            0x0004
#define ADI_ATA_INT_MASK_MDMA_DONE_MASK_SHIFT           2
#define ADI_ATA_INT_MASK_MDMA_DONE_MASK_SET(R,V)        ( (R) = ( (V) << ADI_ATA_INT_MASK_MDMA_DONE_MASK_SHIFT ) | ( (R) & ~ADI_ATA_INT_MASK_MDMA_DONE_MASK_MASK ) )
#define ADI_ATA_INT_MASK_MDMA_DONE_MASK_GET(R)          ( ( (R) & ADI_ATA_INT_MASK_MDMA_DONE_MASK_MASK ) >> ADI_ATA_INT_MASK_MDMA_DONE_MASK_SHIFT )

#define ADI_ATA_INT_MASK_UDMAIN_DONE_MASK_MASK          0x0008
#define ADI_ATA_INT_MASK_UDMAIN_DONE_MASK_SHIFT         3
#define ADI_ATA_INT_MASK_UDMAIN_DONE_MASK_SET(R,V)      ( (R) = ( (V) << ADI_ATA_INT_MASK_UDMAIN_DONE_MASK_SHIFT ) | ( (R) & ~ADI_ATA_INT_MASK_UDMAIN_DONE_MASK_MASK ) )
#define ADI_ATA_INT_MASK_UDMAIN_DONE_MASK_GET(R)        ( ( (R) & ADI_ATA_INT_MASK_UDMAIN_DONE_MASK_MASK ) >> ADI_ATA_INT_MASK_UDMAIN_DONE_MASK_SHIFT )

#define ADI_ATA_INT_MASK_UDMAOUT_DONE_MASK_MASK         0x0010
#define ADI_ATA_INT_MASK_UDMAOUT_DONE_MASK_SHIFT        4
#define ADI_ATA_INT_MASK_UDMAOUT_DONE_MASK_SET(R,V)     ( (R) = ( (V) << ADI_ATA_INT_MASK_UDMAOUT_DONE_MASK_SHIFT ) | ( (R) & ~ADI_ATA_INT_MASK_UDMAOUT_DONE_MASK_MASK ) )
#define ADI_ATA_INT_MASK_UDMAOUT_DONE_MASK_GET(R)       ( ( (R) & ADI_ATA_INT_MASK_UDMAOUT_DONE_MASK_MASK ) >> ADI_ATA_INT_MASK_UDMAOUT_DONE_MASK_SHIFT )

#define ADI_ATA_INT_MASK_HOST_TERM_XFER_MASK_MASK       0x0020
#define ADI_ATA_INT_MASK_HOST_TERM_XFER_MASK_SHIFT      5
#define ADI_ATA_INT_MASK_HOST_TERM_XFER_MASK_SET(R,V)   ( (R) = ( (V) << ADI_ATA_INT_MASK_HOST_TERM_XFER_MASK_SHIFT ) | ( (R) & ~ADI_ATA_INT_MASK_HOST_TERM_XFER_MASK_MASK ) )
#define ADI_ATA_INT_MASK_HOST_TERM_XFER_MASK_GET(R)     ( ( (R) & ADI_ATA_INT_MASK_HOST_TERM_XFER_MASK_MASK ) >> ADI_ATA_INT_MASK_HOST_TERM_XFER_MASK_SHIFT )

#define ADI_ATA_INT_MASK_MDMA_TERM_MASK_MASK            0x0040
#define ADI_ATA_INT_MASK_MDMA_TERM_MASK_SHIFT           6
#define ADI_ATA_INT_MASK_MDMA_TERM_MASK_SET(R,V)        ( (R) = ( (V) << ADI_ATA_INT_MASK_MDMA_TERM_MASK_SHIFT ) | ( (R) & ~ADI_ATA_INT_MASK_MDMA_TERM_MASK_MASK ) )
#define ADI_ATA_INT_MASK_MDMA_TERM_MASK_GET(R)          ( ( (R) & ADI_ATA_INT_MASK_MDMA_TERM_MASK_MASK ) >> ADI_ATA_INT_MASK_MDMA_TERM_MASK_SHIFT )

#define ADI_ATA_INT_MASK_UDMAIN_TERM_MASK_MASK          0x0080
#define ADI_ATA_INT_MASK_UDMAIN_TERM_MASK_SHIFT         7
#define ADI_ATA_INT_MASK_UDMAIN_TERM_MASK_SET(R,V)      ( (R) = ( (V) << ADI_ATA_INT_MASK_UDMAIN_TERM_MASK_SHIFT ) | ( (R) & ~ADI_ATA_INT_MASK_UDMAIN_TERM_MASK_MASK ) )
#define ADI_ATA_INT_MASK_UDMAIN_TERM_MASK_GET(R)        ( ( (R) & ADI_ATA_INT_MASK_UDMAIN_TERM_MASK_MASK ) >> ADI_ATA_INT_MASK_UDMAIN_TERM_MASK_SHIFT )

#define ADI_ATA_INT_MASK_UDMAOUT_TERM_MASK_MASK         0x0100
#define ADI_ATA_INT_MASK_UDMAOUT_TERM_MASK_SHIFT        8
#define ADI_ATA_INT_MASK_UDMAOUT_TERM_MASK_SET(R,V)     ( (R) = ( (V) << ADI_ATA_INT_MASK_UDMAOUT_TERM_MASK_SHIFT ) | ( (R) & ~ADI_ATA_INT_MASK_UDMAOUT_TERM_MASK_MASK ) )
#define ADI_ATA_INT_MASK_UDMAOUT_TERM_MASK_GET(R)       ( ( (R) & ADI_ATA_INT_MASK_UDMAOUT_TERM_MASK_MASK ) >> ADI_ATA_INT_MASK_UDMAOUT_TERM_MASK_SHIFT )


/************************************************************************************************************
 *  ATA Interrupt Status Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_INT_STATUS                          (ADI_ATA_REG_BASE + ADI_ATA_INT_STATUS_REG_OFFSET)
#define pADI_ATA_INT_STATUS                         ((volatile u16*)ADI_ATA_INT_STATUS)

/* ATA_DEV_INT (R0) */
#define ADI_ATA_INT_STATUS_ATA_DEV_INT_MASK         0x0001
#define ADI_ATA_INT_STATUS_ATA_DEV_INT_SHIFT        0
#define ADI_ATA_INT_STATUS_ATA_DEV_INT_SET(R,V)     ( (R) = ( (V) << ADI_ATA_INT_STATUS_ATA_DEV_INT_SHIFT ) | ( (R) & ~ADI_ATA_INT_STATUS_ATA_DEV_INT_MASK ) )
#define ADI_ATA_INT_STATUS_ATA_DEV_INT_CLR(R,V)     ( (R) &= ~( (1) << ADI_ATA_INT_STATUS_ATA_DEV_INT_SHIFT ))
#define ADI_ATA_INT_STATUS_ATA_DEV_INT_GET(R)       ( ( (R) & ADI_ATA_INT_STATUS_ATA_DEV_INT_MASK ) >> ADI_ATA_INT_STATUS_ATA_DEV_INT_SHIFT )

/* PIO_DONE_INT (W1C) */
#define ADI_ATA_INT_STATUS_PIO_DONE_MASK            0x0002
#define ADI_ATA_INT_STATUS_PIO_DONE_SHIFT           1
#define ADI_ATA_INT_STATUS_PIO_DONE_SET(R,V)        ( (R) = ( (V) << ADI_ATA_INT_STATUS_PIO_DONE_SHIFT ) | ( (R) & ~ADI_ATA_INT_STATUS_PIO_DONE_MASK ) )
#define ADI_ATA_INT_STATUS_PIO_DONE_CLR(R,V)        ( (R) &= ~( (1) << ADI_ATA_INT_STATUS_PIO_DONE_SHIFT ))
#define ADI_ATA_INT_STATUS_PIO_DONE_GET(R)          ( ( (R) & ADI_ATA_INT_STATUS_PIO_DONE_MASK ) >> ADI_ATA_INT_STATUS_PIO_DONE_SHIFT )

/* MDMA_DONE_INT (W1C) */
#define ADI_ATA_INT_STATUS_MDMA_DONE_MASK           0x0004
#define ADI_ATA_INT_STATUS_MDMA_DONE_SHIFT          2
#define ADI_ATA_INT_STATUS_MDMA_DONE_SET(R,V)       ( (R) = ( (V) << ADI_ATA_INT_STATUS_MDMA_DONE_SHIFT ) | ( (R) & ~ADI_ATA_INT_STATUS_MDMA_DONE_MASK ) )
#define ADI_ATA_INT_STATUS_MDMA_DONE_CLR(R,V)       ( (R) &= ~( (1) << ADI_ATA_INT_STATUS_MDMA_DONE_SHIFT ) )
#define ADI_ATA_INT_STATUS_MDMA_DONE_GET(R)         ( ( (R) & ADI_ATA_INT_STATUS_MDMA_DONE_MASK ) >> ADI_ATA_INT_STATUS_MDMA_DONE_SHIFT )

/* UDMAIN_DONE_INT (W1C) */
#define ADI_ATA_INT_STATUS_UDMAIN_DONE_MASK         0x0008
#define ADI_ATA_INT_STATUS_UDMAIN_DONE_SHIFT        3
#define ADI_ATA_INT_STATUS_UDMAIN_DONE_SET(R,V)     ( (R) = ( (V) << ADI_ATA_INT_STATUS_UDMAIN_DONE_SHIFT ) | ( (R) & ~ADI_ATA_INT_STATUS_UDMAIN_DONE_MASK ) )
#define ADI_ATA_INT_STATUS_UDMAIN_DONE_CLR(R,V)     ( (R) &= ~( (1) << ADI_ATA_INT_STATUS_UDMAIN_DONE_SHIFT ) )
#define ADI_ATA_INT_STATUS_UDMAIN_DONE_GET(R)       ( ( (R) & ADI_ATA_INT_STATUS_UDMAIN_DONE_MASK ) >> ADI_ATA_INT_STATUS_UDMAIN_DONE_SHIFT )

/* UDMAOUT_DONE_INT (W1C) */
#define ADI_ATA_INT_STATUS_UDMAOUT_DONE_MASK        0x0010
#define ADI_ATA_INT_STATUS_UDMAOUT_DONE_SHIFT       4
#define ADI_ATA_INT_STATUS_UDMAOUT_DONE_SET(R,V)    ( (R) = ( (V) << ADI_ATA_INT_STATUS_UDMAOUT_DONE_SHIFT ) | ( (R) & ~ADI_ATA_INT_STATUS_UDMAOUT_DONE_MASK ) )
#define ADI_ATA_INT_STATUS_UDMAOUT_DONE_CLR(R,V)    ( (R) &= ~( (1) << ADI_ATA_INT_STATUS_UDMAOUT_DONE_SHIFT ) )
#define ADI_ATA_INT_STATUS_UDMAOUT_DONE_GET(R)      ( ( (R) & ADI_ATA_INT_STATUS_UDMAOUT_DONE_MASK ) >> ADI_ATA_INT_STATUS_UDMAOUT_DONE_SHIFT )

/* HOST_TERM__XFER_INT (W1C) */
#define ADI_ATA_INT_STATUS_HOST_TERM_XFER_MASK      0x0020
#define ADI_ATA_INT_STATUS_HOST_TERM_XFER_SHIFT     5
#define ADI_ATA_INT_STATUS_HOST_TERM_XFER_SET(R,V)  ( (R) = ( (V) << ADI_ATA_INT_STATUS_HOST_TERM_XFER_SHIFT ) | ( (R) & ~ADI_ATA_INT_STATUS_HOST_TERM_XFER_MASK ) )
#define ADI_ATA_INT_STATUS_HOST_TERM_XFER_CLR(R,V)  ( (R) &= ~( (1) << ADI_ATA_INT_STATUS_HOST_TERM_XFER_SHIFT )  )
#define ADI_ATA_INT_STATUS_HOST_TERM_XFER_GET(R)    ( ( (R) & ADI_ATA_INT_STATUS_HOST_TERM_XFER_MASK ) >> ADI_ATA_INT_STATUS_HOST_TERM_XFER_SHIFT )

/* MDMA_TERM_INT (W1C) */
#define ADI_ATA_INT_STATUS_MDMA_TERM_MASK           0x0040
#define ADI_ATA_INT_STATUS_MDMA_TERM_SHIFT          6
#define ADI_ATA_INT_STATUS_MDMA_TERM_SET(R,V)       ( (R) = ( (V) << ADI_ATA_INT_STATUS_MDMA_TERM_SHIFT ) | ( (R) & ~ADI_ATA_INT_STATUS_MDMA_TERM_MASK ) )
#define ADI_ATA_INT_STATUS_MDMA_TERM_CLR(R,V)       ( (R) &= ~( (1) << ADI_ATA_INT_STATUS_MDMA_TERM_SHIFT )  )
#define ADI_ATA_INT_STATUS_MDMA_TERM_GET(R)         ( ( (R) & ADI_ATA_INT_STATUS_MDMA_TERM_MASK ) >> ADI_ATA_INT_STATUS_MDMA_TERM_SHIFT )

/* UDMAIN_TERM_INT (W1C) */
#define ADI_ATA_INT_STATUS_UDMAIN_TERM_MASK         0x0080
#define ADI_ATA_INT_STATUS_UDMAIN_TERM_SHIFT        7
#define ADI_ATA_INT_STATUS_UDMAIN_TERM_SET(R,V)     ( (R) = ( (V) << ADI_ATA_INT_STATUS_UDMAIN_TERM_SHIFT ) | ( (R) & ~ADI_ATA_INT_STATUS_UDMAIN_TERM_MASK ) )
#define ADI_ATA_INT_STATUS_UDMAIN_TERM_CLR(R,V)     ( (R) &= ~( (1) << ADI_ATA_INT_STATUS_UDMAIN_TERM_SHIFT ))
#define ADI_ATA_INT_STATUS_UDMAIN_TERM_GET(R)       ( ( (R) & ADI_ATA_INT_STATUS_UDMAIN_TERM_MASK ) >> ADI_ATA_INT_STATUS_UDMAIN_TERM_SHIFT )

/* UDMAOUT_TERM_INT (W1C) */
#define ADI_ATA_INT_STATUS_UDMAOUT_TERM_MASK        0x0100
#define ADI_ATA_INT_STATUS_UDMAOUT_TERM_SHIFT       8
#define ADI_ATA_INT_STATUS_UDMAOUT_TERM_SET(R,V)    ( (R) = ( (V) << ADI_ATA_INT_STATUS_UDMAOUT_TERM_SHIFT ) | ( (R) & ~ADI_ATA_INT_STATUS_UDMAOUT_TERM_MASK ) )
#define ADI_ATA_INT_STATUS_UDMAOUT_TERM_CLR(R,V)    ( (R) &= ~( (1) << ADI_ATA_INT_STATUS_UDMAOUT_TERM_SHIFT ))
#define ADI_ATA_INT_STATUS_UDMAOUT_TERM_GET(R)      ( ( (R) & ADI_ATA_INT_STATUS_UDMAOUT_TERM_MASK ) >> ADI_ATA_INT_STATUS_UDMAOUT_TERM_SHIFT )

#define ADI_ATA_INT_STATUS_SET(R,M,S)              ( (R) = ( (1) << S ) | ( (R) & ~M ) )
#define ADI_ATA_INT_STATUS_CLR(R,S)                ( (R) &= ~( (1) << S ))

/************************************************************************************************************
 *  ATA Tranfer Length Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_XFER_LEN                            (ADI_ATA_REG_BASE + ADI_ATA_XFER_LEN_REG_OFFSET)
#define pADI_ATA_XFER_LEN                           ((volatile u16*)ADI_ATA_XFER_LEN)

#define ADI_ATA_XFER_LEN_SET(R,V)                   ( (R) = (u16)(V)  )
#define ADI_ATA_XFER_LEN_GET(R)                     ( (u16)(R) )


/************************************************************************************************************
 *  ATA Line Status Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_LINE_STATUS                         (ADI_ATA_REG_BASE + ADI_ATA_LINE_STATUS_REG_OFFSET)
#define pADI_ATA_LINE_STATUS                        ((volatile u16*)ADI_ATA_LINE_STATUS)

/* Device Interrupt to host line status (RO) */
#define ADI_ATA_LINE_STATUS_ATA_INTR_MASK           0x0001
#define ADI_ATA_LINE_STATUS_ATA_INTR_SHIFT          0
#define ADI_ATA_LINE_STATUS_ATA_INTR_GET(R)         ( ( (R) & ADI_ATA_LINE_STATUS_ATA_INTR_MASK ) >> ADI_ATA_LINE_STATUS_ATA_INTR_SHIFT )

/* Device dasp to host line status (RO)  */
#define ADI_ATA_LINE_STATUS_ATA_DASP_MASK           0x0002
#define ADI_ATA_LINE_STATUS_ATA_DASP_SHIFT          1
#define ADI_ATA_LINE_STATUS_ATA_DASP_GET(R)         ( ( (R) & ADI_ATA_LINE_STATUS_ATA_DASP_MASK ) >> ADI_ATA_LINE_STATUS_ATA_DASP_SHIFT )

/* ATA chip select-0 line status (RO) */
#define ADI_ATA_LINE_STATUS_ATA_CS0N_MASK           0x0004
#define ADI_ATA_LINE_STATUS_ATA_CS0N_SHIFT          2
#define ADI_ATA_LINE_STATUS_ATA_CS0N_GET(R)         ( ( (R) & ADI_ATA_LINE_STATUS_ATA_CS0N_MASK ) >> ADI_ATA_LINE_STATUS_ATA_CS0N_SHIFT )

/* ATA chip select-1 line status (RO) */
#define ADI_ATA_LINE_STATUS_ATA_CS1N_MASK           0x0008
#define ADI_ATA_LINE_STATUS_ATA_CS1N_SHIFT          3
#define ADI_ATA_LINE_STATUS_ATA_CS1N_GET(R)         ( ( (R) & ADI_ATA_LINE_STATUS_ATA_CS1N_MASK ) >> ADI_ATA_LINE_STATUS_ATA_CS1N_SHIFT )

/* ATA address line status (RO) */
#define ADI_ATA_LINE_STATUS_ATA_ADDR_MASK           0x00F0
#define ADI_ATA_LINE_STATUS_ATA_ADDR_SHIFT          4
#define ADI_ATA_LINE_STATUS_ATA_ADDR_GET(R)         ( ( (R) & ADI_ATA_LINE_STATUS_ATA_ADDR_MASK ) >> ADI_ATA_LINE_STATUS_ATA_ADDR_SHIFT )

/* ATA DMA request line status (RO) */
#define ADI_ATA_LINE_STATUS_ATA_DMAREQ_MASK         0x0100
#define ADI_ATA_LINE_STATUS_ATA_DMAREQ_SHIFT        7
#define ADI_ATA_LINE_STATUS_ATA_DMAREQ_GET(R)       ( ( (R) & ADI_ATA_LINE_STATUS_ATA_DMAREQ_MASK ) >> ADI_ATA_LINE_STATUS_ATA_DMAREQ_SHIFT )

/* ATA DMA acknowledge line status (RO) */
#define ADI_ATA_LINE_STATUS_ATA_DMAACKN_MASK        0x0200
#define ADI_ATA_LINE_STATUS_ATA_DMAACKN_SHIFT       8
#define ADI_ATA_LINE_STATUS_ATA_DMAACKN_GET(R)      ( ( (R) & ADI_ATA_LINE_STATUS_ATA_DMAACKN_MASK ) >> ADI_ATA_LINE_STATUS_ATA_DMAACKN_SHIFT )

/* ATA write line status (RO) */
#define ADI_ATA_LINE_STATUS_ATA_DIOWN_MASK          0x0400
#define ADI_ATA_LINE_STATUS_ATA_DIOWN_SHIFT         9
#define ADI_ATA_LINE_STATUS_ATA_DIOWN_GET(R)        ( ( (R) & ADI_ATA_LINE_STATUS_ATA_DIOWN_MASK ) >> ADI_ATA_LINE_STATUS_ATA_DIOWN_SHIFT )

/* ATA read line status (RO) */
#define ADI_ATA_LINE_STATUS_ATA_DIORN_MASK          0x0800
#define ADI_ATA_LINE_STATUS_ATA_DIORN_SHIFT         10
#define ADI_ATA_LINE_STATUS_ATA_DIORN_GET(R)        ( ( (R) & ADI_ATA_LINE_STATUS_ATA_DIORN_MASK ) >> ADI_ATA_LINE_STATUS_ATA_DIORN_SHIFT )

/* ATA IORDY line status (RO) */
#define ADI_ATA_LINE_STATUS_ATA_IORDY_MASK          0x1000
#define ADI_ATA_LINE_STATUS_ATA_IORDY_SHIFT         11
#define ADI_ATA_LINE_STATUS_ATA_IORDY_GET(R)        ( ( (R) & ADI_ATA_LINE_STATUS_ATA_IORDY_MASK ) >> ADI_ATA_LINE_STATUS_ATA_IORDY_SHIFT )



/************************************************************************************************************
 *  ATA State Machine Status Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_SM_STATE                            (ADI_ATA_REG_BASE + ADI_ATA_SM_STATE_REG_OFFSET)
#define pADI_ATA_SM_STATE                           ((volatile u16*)ADI_ATA_SM_STATE)

/* PIO mode State Machine current state (RO) */
#define ADI_ATA_SM_STATE_PIO_CSTATE_MASK            0x000F
#define ADI_ATA_SM_STATE_PIO_CSTATE_SHIFT           0
#define ADI_ATA_SM_STATE_PIO_CSTATE_GET(R)          ( ( (R) & ADI_ATA_SM_STATE_PIO_CSTATE_MASK ) >> ADI_ATA_SM_STATE_PIO_CSTATE_SHIFT )

/* DMA mode State Machine current state (RO) */
#define ADI_ATA_SM_STATE_DMA_CSTATE_MASK            0x00F0
#define ADI_ATA_SM_STATE_DMA_CSTATE_SHIFT           4
#define ADI_ATA_SM_STATE_DMA_CSTATE_GET(R)          ( ( (R) & ADI_ATA_SM_STATE_DMA_CSTATE_MASK ) >> ADI_ATA_SM_STATE_DMA_CSTATE_SHIFT )

/* UDMA in mode State Machine current state (RO) */
#define ADI_ATA_SM_STATE_UDMAIN_CSTATE_MASK         0x0F00
#define ADI_ATA_SM_STATE_UDMAIN_CSTATE_SHIFT        8
#define ADI_ATA_SM_STATE_UDMAIN_CSTATE_GET(R)       ( ( (R) & ADI_ATA_SM_STATE_UDMAIN_CSTATE_MASK ) >> ADI_ATA_SM_STATE_UDMAIN_CSTATE_SHIFT )

/* UDMA out mode State Machine current state (RO) */
#define ADI_ATA_SM_STATE_UDMAOUT_CSTATE_MASK        0xF000
#define ADI_ATA_SM_STATE_UDMAOUT_CSTATE_SHIFT       12
#define ADI_ATA_SM_STATE_UDMAOUT_CSTATE_GET(R)      ( ( (R) & ADI_ATA_SM_STATE_UDMAOUT_CSTATE_MASK ) >> ADI_ATA_SM_STATE_UDMAOUT_CSTATE_SHIFT )



/************************************************************************************************************
 *  ATA Terminate Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_TERMINATE                           (ADI_ATA_REG_BASE + ADI_ATA_TERMINATE_REG_OFFSET)
#define pADI_ATA_TERMINATE                          ((volatile u16*)ADI_ATA_TERMINATE)

#define ADI_ATA_TERMINATE_ATA_HOST_TERM_MASK        0x0001
#define ADI_ATA_TERMINATE_ATA_HOST_TERM_SHIFT       0
#define ADI_ATA_TERMINATE_ATA_HOST_TERM_SET(R,V)    ( (R) = ( (V) << ADI_ATA_TERMINATE_ATA_HOST_TERM_SHIFT ) | ( (R) & ~ADI_ATA_TERMINATE_ATA_HOST_TERM_MASK ) )
#define ADI_ATA_TERMINATE_ATA_HOST_TERM_GET(R)      ( ( (R) & ADI_ATA_TERMINATE_ATA_HOST_TERM_MASK ) >> ADI_ATA_TERMINATE_ATA_HOST_TERM_SHIFT )

/************************************************************************************************************
 *  ATA PIO Transfer Count Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_PIO_TFRCNT                          (ADI_ATA_REG_BASE + ADI_ATA_PIO_TFRCNT_REG_OFFSET)
#define pADI_ATA_PIO_TFRCNT                         ((volatile u16*)ADI_ATA_PIO_TFRCNT)

#define ADI_ATA_PIO_TFRCNT_GET(R)                   ( (u16)(R) )

/************************************************************************************************************
 *  ATA DMA Transfer Count Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_DMA_TFRCNT                          (ADI_ATA_REG_BASE + ADI_ATA_DMA_TFRCNT_REG_OFFSET)
#define pADI_ATA_DMA_TFRCNT                         ((volatile u16*)ADI_ATA_DMA_TFRCNT)

#define ADI_ATA_DMA_TFRCNT_GET(R)                   ( (u16)(R) )

/************************************************************************************************************
 *  ATA UDMA In Transfer Count Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_UDMAIN_TFRCNT                          (ADI_ATA_REG_BASE + ADI_ATA_UDMAIN_TFRCNT_REG_OFFSET)
#define pADI_ATA_UDMAIN_TFRCNT                         ((volatile u16*)ADI_ATA_UDMAIN_TFRCNT)

#define ADI_ATA_UDMAIN_TFRCNT_GET(R)                   ( (u16)(R) )

/************************************************************************************************************
 *  ATA UDMA Out Transfer Count Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_UDMAOUT_TFRCNT                          (ADI_ATA_REG_BASE + ADI_ATA_UDMAOUT_TFRCNT_REG_OFFSET)
#define pADI_ATA_UDMAOUT_TFRCNT                         ((volatile u16*)ADI_ATA_UDMAOUT_TFRCNT)

#define ADI_ATA_UDMAOUT_TFRCNT_GET(R)                   ( (u16)(R) )

/************************************************************************************************************
 *  ATA Register Transfer Timing 0 Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_REG_TIM_0                           (ADI_ATA_REG_BASE + ADI_ATA_REG_TIM_0_REG_OFFSET)
#define pADI_ATA_REG_TIM_0                          ((volatile u16*)ADI_ATA_REG_TIM_0)

#define ADI_ATA_REG_TIM_0_T2_REG_MASK               0x00FF
#define ADI_ATA_REG_TIM_0_T2_REG_SHIFT              0
#define ADI_ATA_REG_TIM_0_T2_REG_SET(R,V)           ( (R) = ( (V) << ADI_ATA_REG_TIM_0_T2_REG_SHIFT ) | ( (R) & ~ADI_ATA_REG_TIM_0_T2_REG_MASK ) )
#define ADI_ATA_REG_TIM_0_T2_REG_GET(R)             ( ( (R) & ADI_ATA_REG_TIM_0_T2_REG_MASK ) >> ADI_ATA_REG_TIM_0_T2_REG_SHIFT )

#define ADI_ATA_REG_TIM_0_TEOC_REG_MASK             0xFF00
#define ADI_ATA_REG_TIM_0_TEOC_REG_SHIFT            8
#define ADI_ATA_REG_TIM_0_TEOC_REG_SET(R,V)         ( (R) = ( (V) << ADI_ATA_REG_TIM_0_TEOC_REG_SHIFT ) | ( (R) & ~ADI_ATA_REG_TIM_0_TEOC_REG_MASK ) )
#define ADI_ATA_REG_TIM_0_TEOC_REG_GET(R)           ( ( (R) & ADI_ATA_REG_TIM_0_TEOC_REG_MASK ) >> ADI_ATA_REG_TIM_0_TEOC_REG_SHIFT )

/************************************************************************************************************
 *  ATA PIO Timing 0 Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_PIO_TIM_0                           (ADI_ATA_REG_BASE + ADI_ATA_PIO_TIM_0_REG_OFFSET)
#define pADI_ATA_PIO_TIM_0                          ((volatile u16*)ADI_ATA_PIO_TIM_0)

#define ADI_ATA_PIO_TIM_0_T1_REG_MASK               0x000F
#define ADI_ATA_PIO_TIM_0_T1_REG_SHIFT              0
#define ADI_ATA_PIO_TIM_0_T1_REG_SET(R,V)           ( (R) = ( (V) << ADI_ATA_PIO_TIM_0_T1_REG_SHIFT ) | ( (R) & ~ADI_ATA_PIO_TIM_0_T1_REG_MASK ) )
#define ADI_ATA_PIO_TIM_0_T1_REG_GET(R)             ( ( (R) & ADI_ATA_PIO_TIM_0_T1_REG_MASK ) >> ADI_ATA_PIO_TIM_0_T1_REG_SHIFT )

#define ADI_ATA_PIO_TIM_0_T2_REG_MASK               0x0FF0
#define ADI_ATA_PIO_TIM_0_T2_REG_SHIFT              4
#define ADI_ATA_PIO_TIM_0_T2_REG_SET(R,V)           ( (R) = ( (V) << ADI_ATA_PIO_TIM_0_T2_REG_SHIFT ) | ( (R) & ~ADI_ATA_PIO_TIM_0_T2_REG_MASK ) )
#define ADI_ATA_PIO_TIM_0_T2_REG_GET(R)             ( ( (R) & ADI_ATA_PIO_TIM_0_T2_REG_MASK ) >> ADI_ATA_PIO_TIM_0_T2_REG_SHIFT )

#define ADI_ATA_PIO_TIM_0_T4_REG_MASK               0xF000
#define ADI_ATA_PIO_TIM_0_T4_REG_SHIFT              12
#define ADI_ATA_PIO_TIM_0_T4_REG_SET(R,V)           ( (R) = ( (V) << ADI_ATA_PIO_TIM_0_T4_REG_SHIFT ) | ( (R) & ~ADI_ATA_PIO_TIM_0_T4_REG_MASK ) )
#define ADI_ATA_PIO_TIM_0_T4_REG_GET(R)             ( ( (R) & ADI_ATA_PIO_TIM_0_T4_REG_MASK ) >> ADI_ATA_PIO_TIM_0_T4_REG_SHIFT )

/************************************************************************************************************
 *  ATA PIO Timing 1 Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_PIO_TIM_1                           (ADI_ATA_REG_BASE + ADI_ATA_PIO_TIM_1_REG_OFFSET)
#define pADI_ATA_PIO_TIM_1                          ((volatile u16*)ADI_ATA_PIO_TIM_1)

#define ADI_ATA_PIO_TIM_1_TEOC_REG_MASK             0x00FF
#define ADI_ATA_PIO_TIM_1_TEOC_REG_SHIFT            0
#define ADI_ATA_PIO_TIM_1_TEOC_REG_SET(R,V)         ( (R) = ( (V) << ADI_ATA_PIO_TIM_1_TEOC_REG_SHIFT ) | ( (R) & ~ADI_ATA_PIO_TIM_1_TEOC_REG_MASK ) )
#define ADI_ATA_PIO_TIM_1_TEOC_REG_GET(R)           ( ( (R) & ADI_ATA_PIO_TIM_1_TEOC_REG_MASK ) >> ADI_ATA_PIO_TIM_1_TEOC_REG_SHIFT )

/************************************************************************************************************
 *  ATA MDMA Timing 0 Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_MDMA_TIM_0                          (ADI_ATA_REG_BASE + ADI_ATA_MDMA_TIM_0_REG_OFFSET)
#define pADI_ATA_MDMA_TIM_0                         ((volatile u16*)ADI_ATA_MDMA_TIM_0)

#define ADI_ATA_MDMA_TIM_0_TD_MASK                  0x00FF
#define ADI_ATA_MDMA_TIM_0_TD_SHIFT                 0
#define ADI_ATA_MDMA_TIM_0_TD_SET(R,V)              ( (R) = ( (V) << ADI_ATA_MDMA_TIM_0_TD_SHIFT ) | ( (R) & ~ADI_ATA_MDMA_TIM_0_TD_MASK ) )
#define ADI_ATA_MDMA_TIM_0_TD_GET(R)                ( ( (R) & ADI_ATA_MDMA_TIM_0_TD_MASK ) >> ADI_ATA_MDMA_TIM_0_TD_SHIFT )

#define ADI_ATA_MDMA_TIM_0_TM_MASK                  0xFF00
#define ADI_ATA_MDMA_TIM_0_TM_SHIFT                 8
#define ADI_ATA_MDMA_TIM_0_TM_SET(R,V)              ( (R) = ( (V) << ADI_ATA_MDMA_TIM_0_TM_SHIFT ) | ( (R) & ~ADI_ATA_MDMA_TIM_0_TM_MASK ) )
#define ADI_ATA_MDMA_TIM_0_TM_GET(R)                ( ( (R) & ADI_ATA_MDMA_TIM_0_TM_MASK ) >> ADI_ATA_MDMA_TIM_0_TM_SHIFT )


/************************************************************************************************************
 *  ATA MDMA Timing 1 Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_MDMA_TIM_1                          (ADI_ATA_REG_BASE + ADI_ATA_MDMA_TIM_1_REG_OFFSET)
#define pADI_ATA_MDMA_TIM_1                         ((volatile u16*)ADI_ATA_MDMA_TIM_1)

#define ADI_ATA_MDMA_TIM_1_TKW_MASK                 0x00FF
#define ADI_ATA_MDMA_TIM_1_TKW_SHIFT                0
#define ADI_ATA_MDMA_TIM_1_TKW_SET(R,V)             ( (R) = ( (V) << ADI_ATA_MDMA_TIM_1_TKW_SHIFT ) | ( (R) & ~ADI_ATA_MDMA_TIM_1_TKW_MASK ) )
#define ADI_ATA_MDMA_TIM_1_TKW_GET(R)               ( ( (R) & ADI_ATA_MDMA_TIM_1_TKW_MASK ) >> ADI_ATA_MDMA_TIM_1_TKW_SHIFT )

#define ADI_ATA_MDMA_TIM_1_TKR_MASK                 0xFF00
#define ADI_ATA_MDMA_TIM_1_TKR_SHIFT                8
#define ADI_ATA_MDMA_TIM_1_TKR_SET(R,V)             ( (R) = ( (V) << ADI_ATA_MDMA_TIM_1_TKR_SHIFT ) | ( (R) & ~ADI_ATA_MDMA_TIM_1_TKR_MASK ) )
#define ADI_ATA_MDMA_TIM_1_TKR_GET(R)               ( ( (R) & ADI_ATA_MDMA_TIM_1_TKR_MASK ) >> ADI_ATA_MDMA_TIM_1_TKR_SHIFT )


/************************************************************************************************************
 *  ATA MDMA Timing 2 Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_MDMA_TIM_2                          (ADI_ATA_REG_BASE + ADI_ATA_MDMA_TIM_2_REG_OFFSET)
#define pADI_ATA_MDMA_TIM_2                         ((volatile u16*)ADI_ATA_MDMA_TIM_2)

#define ADI_ATA_MDMA_TIM_2_TH_MASK                  0x00FF
#define ADI_ATA_MDMA_TIM_2_TH_SHIFT                 0
#define ADI_ATA_MDMA_TIM_2_TH_SET(R,V)              ( (R) = ( (V) << ADI_ATA_MDMA_TIM_2_TH_SHIFT ) | ( (R) & ~ADI_ATA_MDMA_TIM_2_TH_MASK ) )
#define ADI_ATA_MDMA_TIM_2_TH_GET(R)                ( ( (R) & ADI_ATA_MDMA_TIM_2_TH_MASK ) >> ADI_ATA_MDMA_TIM_2_TH_SHIFT )

#define ADI_ATA_MDMA_TIM_2_TEOC_MASK                0xFF00
#define ADI_ATA_MDMA_TIM_2_TEOC_SHIFT               8
#define ADI_ATA_MDMA_TIM_2_TEOC_SET(R,V)            ( (R) = ( (V) << ADI_ATA_MDMA_TIM_2_TEOC_SHIFT ) | ( (R) & ~ADI_ATA_MDMA_TIM_2_TEOC_MASK ) )
#define ADI_ATA_MDMA_TIM_2_TEOC_GET(R)              ( ( (R) & ADI_ATA_MDMA_TIM_2_TEOC_MASK ) >> ADI_ATA_MDMA_TIM_2_TEOC_SHIFT )



/************************************************************************************************************
 *  ATA UDMA Timing 0 Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_UDMA_TIM_0                          (ADI_ATA_REG_BASE + ADI_ATA_UDMA_TIM_0_REG_OFFSET)
#define pADI_ATA_UDMA_TIM_0                         ((volatile u16*)ADI_ATA_UDMA_TIM_0)

#define ADI_ATA_UDMA_TIM_0_TACK_MASK                0x00FF
#define ADI_ATA_UDMA_TIM_0_TACK_SHIFT               0
#define ADI_ATA_UDMA_TIM_0_TACK_SET(R,V)            ( (R) = ( (V) << ADI_ATA_UDMA_TIM_0_TACK_SHIFT ) | ( (R) & ~ADI_ATA_UDMA_TIM_0_TACK_MASK ) )
#define ADI_ATA_UDMA_TIM_0_TACK_GET(R)              ( ( (R) & ADI_ATA_UDMA_TIM_0_TACK_MASK ) >> ADI_ATA_UDMA_TIM_0_TACK_SHIFT )

#define ADI_ATA_UDMA_TIM_0_TENV_MASK                0xFF00
#define ADI_ATA_UDMA_TIM_0_TENV_SHIFT               8
#define ADI_ATA_UDMA_TIM_0_TENV_SET(R,V)            ( (R) = ( (V) << ADI_ATA_UDMA_TIM_0_TENV_SHIFT ) | ( (R) & ~ADI_ATA_UDMA_TIM_0_TENV_MASK ) )
#define ADI_ATA_UDMA_TIM_0_TENV_GET(R)              ( ( (R) & ADI_ATA_UDMA_TIM_0_TENV_MASK ) >> ADI_ATA_UDMA_TIM_0_TENV_SHIFT )


/************************************************************************************************************
 *  ATA UDMA Timing 1 Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_UDMA_TIM_1                          (ADI_ATA_REG_BASE + ADI_ATA_UDMA_TIM_1_REG_OFFSET)
#define pADI_ATA_UDMA_TIM_1                         ((volatile u16*)ADI_ATA_UDMA_TIM_1)

#define ADI_ATA_UDMA_TIM_1_TDVS_MASK                0x00FF
#define ADI_ATA_UDMA_TIM_1_TDVS_SHIFT               0
#define ADI_ATA_UDMA_TIM_1_TDVS_SET(R,V)            ( (R) = ( (V) << ADI_ATA_UDMA_TIM_1_TDVS_SHIFT ) | ( (R) & ~ADI_ATA_UDMA_TIM_1_TDVS_MASK ) )
#define ADI_ATA_UDMA_TIM_1_TDVS_GET(R)              ( ( (R) & ADI_ATA_UDMA_TIM_1_TDVS_MASK ) >> ADI_ATA_UDMA_TIM_1_TDVS_SHIFT )

#define ADI_ATA_UDMA_TIM_1_TCYC_TDVS_MASK           0xFF00
#define ADI_ATA_UDMA_TIM_1_TCYC_TDVS_SHIFT          8
#define ADI_ATA_UDMA_TIM_1_TCYC_TDVS_SET(R,V)       ( (R) = ( (V) << ADI_ATA_UDMA_TIM_1_TCYC_TDVS_SHIFT ) | ( (R) & ~ADI_ATA_UDMA_TIM_1_TCYC_TDVS_MASK ) )
#define ADI_ATA_UDMA_TIM_1_TCYC_TDVS_GET(R)         ( ( (R) & ADI_ATA_UDMA_TIM_1_TCYC_TDVS_MASK ) >> ADI_ATA_UDMA_TIM_1_TCYC_TDVS_SHIFT )


/************************************************************************************************************
 *  ATA UDMA Timing 2 Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_UDMA_TIM_2                          (ADI_ATA_REG_BASE + ADI_ATA_UDMA_TIM_2_REG_OFFSET)
#define pADI_ATA_UDMA_TIM_2                         ((volatile u16*)ADI_ATA_UDMA_TIM_2)

#define ADI_ATA_UDMA_TIM_2_TSS_MASK                 0x00FF
#define ADI_ATA_UDMA_TIM_2_TSS_SHIFT                0
#define ADI_ATA_UDMA_TIM_2_TSS_SET(R,V)             ( (R) = ( (V) << ADI_ATA_UDMA_TIM_2_TSS_SHIFT ) | ( (R) & ~ADI_ATA_UDMA_TIM_2_TSS_MASK ) )
#define ADI_ATA_UDMA_TIM_2_TSS_GET(R)               ( ( (R) & ADI_ATA_UDMA_TIM_2_TSS_MASK ) >> ADI_ATA_UDMA_TIM_2_TSS_SHIFT )

#define ADI_ATA_UDMA_TIM_2_TMLI_MASK                0xFF00
#define ADI_ATA_UDMA_TIM_2_TMLI_SHIFT               8
#define ADI_ATA_UDMA_TIM_2_TMLI_SET(R,V)            ( (R) = ( (V) << ADI_ATA_UDMA_TIM_2_TMLI_SHIFT ) | ( (R) & ~ADI_ATA_UDMA_TIM_2_TMLI_MASK ) )
#define ADI_ATA_UDMA_TIM_2_TMLI_GET(R)              ( ( (R) & ADI_ATA_UDMA_TIM_2_TMLI_MASK ) >> ADI_ATA_UDMA_TIM_2_TMLI_SHIFT )


/************************************************************************************************************
 *  ATA UDMA Timing 3 Register MMR and bitfield manipulation
 ************************************************************************************************************
 */

/* Address and pointer variable */
#define ADI_ATA_UDMA_TIM_3                          (ADI_ATA_REG_BASE + ADI_ATA_UDMA_TIM_3_REG_OFFSET)
#define pADI_ATA_UDMA_TIM_3                         ((volatile u16*)ADI_ATA_UDMA_TIM_3)

#define ADI_ATA_UDMA_TIM_3_TZAH_MASK                0x00FF
#define ADI_ATA_UDMA_TIM_3_TZAH_SHIFT               0
#define ADI_ATA_UDMA_TIM_3_TZAH_SET(R,V)            ( (R) = ( (V) << ADI_ATA_UDMA_TIM_3_TZAH_SHIFT ) | ( (R) & ~ADI_ATA_UDMA_TIM_3_TZAH_MASK ) )
#define ADI_ATA_UDMA_TIM_3_TZAH_GET(R)              ( ( (R) & ADI_ATA_UDMA_TIM_3_TZAH_MASK ) >> ADI_ATA_UDMA_TIM_3_TZAH_SHIFT )

#define ADI_ATA_UDMA_TIM_3_TRP_MASK                 0xFF00
#define ADI_ATA_UDMA_TIM_3_TRP_SHIFT                8
#define ADI_ATA_UDMA_TIM_3_TRP_SET(R,V)             ( (R) = ( (V) << ADI_ATA_UDMA_TIM_3_TRP_SHIFT ) | ( (R) & ~ADI_ATA_UDMA_TIM_3_TRP_MASK ) )
#define ADI_ATA_UDMA_TIM_3_TRP_GET(R)               ( ( (R) & ADI_ATA_UDMA_TIM_3_TRP_MASK ) >> ADI_ATA_UDMA_TIM_3_TRP_SHIFT )


