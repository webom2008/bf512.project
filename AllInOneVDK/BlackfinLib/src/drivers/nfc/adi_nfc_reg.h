/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_nfc_reg.h,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
    This header file defines the MMRs for the NAND Flash Conroller (NFC) interface. 
    It includes both the MMR addresses and the bitfield access macros.

*********************************************************************************/

#ifndef __ADI_NFC_REG_H__
#define __ADI_NFC_REG_H__

/*********************************************************************************

Processor specific MMR defines for NFC

*********************************************************************************/

/* ADSP-BF52x (KOOKABURRA & MOKINGBIRD) Family */
/* NFC registers/register offsets for Kookaburra & Mokingbird class devices  */
#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

#define ADI_NFC_REG_BASE                        0xFFC03700

#define ADI_NFC_CONTROL_REG_OFFSET              0x0000  /* NFC Control register offset          */
#define ADI_NFC_STATUS_REG_OFFSET               0x0004  /* NFC Status register offset           */
#define ADI_NFC_IRQSTAT_REG_OFFSET              0x0008  /* NFC Interrupt Status register offset */
#define ADI_NFC_IRQMASK_REG_OFFSET              0x000C  /* NFC Interrupt Mask register offset   */
#define ADI_NFC_ECC0_REG_OFFSET                 0x0010  /* NFC ECC0 register offset             */
#define ADI_NFC_ECC1_REG_OFFSET                 0x0014  /* NFC ECC1 register offset             */
#define ADI_NFC_ECC2_REG_OFFSET                 0x0018  /* NFC ECC2 register offset             */
#define ADI_NFC_ECC3_REG_OFFSET                 0x001C  /* NFC ECC3 register offset             */
#define ADI_NFC_COUNT_REG_OFFSET                0x0020  /* NFC ECC Count register offset        */
#define ADI_NFC_RST_REG_OFFSET                  0x0024  /* NFC ECC Reset register offset        */
#define ADI_NFC_PGCTL_REG_OFFSET                0x0028  /* NFC Page Control register offset     */
#define ADI_NFC_READ_REG_OFFSET                 0x002C  /* NFC Read Data register offset        */
#define ADI_NFC_ADDR_REG_OFFSET                 0x0040  /* NFC Address register offset          */
#define ADI_NFC_CMD_REG_OFFSET                  0x0044  /* NFC Command register offset          */
#define ADI_NFC_DATA_WR_REG_OFFSET              0x0048  /* NFC Data Write register offset       */
#define ADI_NFC_DATA_RD_REG_OFFSET              0x004C  /* NFC Data Read register offset        */

#endif                              /* End of Kookaburra specific MMR defines                   */

/* ADSP-BF54x (MOAB) Family */
#if defined(__ADSP_MOAB__)          /* NFC registers/register offsets for Moab class devices    */

#define ADI_NFC_REG_BASE                        0xFFC03B00

#define ADI_NFC_CONTROL_REG_OFFSET              0x0000  /* NFC Control register offset          */
#define ADI_NFC_STATUS_REG_OFFSET               0x0004  /* NFC Status register offset           */
#define ADI_NFC_IRQSTAT_REG_OFFSET              0x0008  /* NFC Interrupt Status register offset */
#define ADI_NFC_IRQMASK_REG_OFFSET              0x000C  /* NFC Interrupt Mask register offset   */
#define ADI_NFC_ECC0_REG_OFFSET                 0x0010  /* NFC ECC0 register offset             */
#define ADI_NFC_ECC1_REG_OFFSET                 0x0014  /* NFC ECC1 register offset             */
#define ADI_NFC_ECC2_REG_OFFSET                 0x0018  /* NFC ECC2 register offset             */
#define ADI_NFC_ECC3_REG_OFFSET                 0x001C  /* NFC ECC3 register offset             */
#define ADI_NFC_COUNT_REG_OFFSET                0x0020  /* NFC ECC Count register offset        */
#define ADI_NFC_RST_REG_OFFSET                  0x0024  /* NFC ECC Reset register offset        */
#define ADI_NFC_PGCTL_REG_OFFSET                0x0028  /* NFC Page Control register offset     */
#define ADI_NFC_READ_REG_OFFSET                 0x002C  /* NFC Read Data register offset        */
#define ADI_NFC_ADDR_REG_OFFSET                 0x0040  /* NFC Address register offset          */
#define ADI_NFC_CMD_REG_OFFSET                  0x0044  /* NFC Command register offset          */
#define ADI_NFC_DATA_WR_REG_OFFSET              0x0048  /* NFC Data Write register offset       */
#define ADI_NFC_DATA_RD_REG_OFFSET              0x004C  /* NFC Data Read register offset        */

#endif                              /* End of Moab specific MMR defines                         */

/*********************************************************************************

NFC Control Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_CTRL                            (ADI_NFC_REG_BASE + ADI_NFC_CONTROL_REG_OFFSET)
#define pADI_NFC_CTRL                           ((volatile u16*)ADI_NFC_CTRL)

/* NFC Write strobe delay */
#define ADI_NFC_CTRL_WR_DLY_MASK                0x000F
#define ADI_NFC_CTRL_WR_DLY_SHIFT               0
#define ADI_NFC_CTRL_WR_DLY_SET(V)              (*pADI_NFC_CTRL = ((*pADI_NFC_CTRL & ~ADI_NFC_CTRL_WR_DLY_MASK) | ((V) << ADI_NFC_CTRL_WR_DLY_SHIFT)))

/* NFC Read strobe delay, value between 0x0 to 0xF */
#define ADI_NFC_CTRL_RD_DLY_MASK                0x00F0
#define ADI_NFC_CTRL_RD_DLY_SHIFT               4
#define ADI_NFC_CTRL_RD_DLY_SET(V)              (*pADI_NFC_CTRL = ((*pADI_NFC_CTRL & ~ADI_NFC_CTRL_RD_DLY_MASK) | ((V) << ADI_NFC_CTRL_RD_DLY_SHIFT)))

/* NAND Data width, 0 for x8, 1 for x16 */
#define ADI_NFC_CTRL_NWIDTH_MASK                0x0100
#define ADI_NFC_CTRL_NWIDTH_SET_8BIT            (*pADI_NFC_CTRL &= ~ADI_NFC_CTRL_NWIDTH_MASK)
#define ADI_NFC_CTRL_NWIDTH_SET_16BIT           (*pADI_NFC_CTRL |= ADI_NFC_CTRL_NWIDTH_MASK)

/* NFC Page Size, 0 for 256 bytes, 1 for 512 bytes */
#define ADI_NFC_CTRL_PG_SIZE_MASK               0x0200
#define ADI_NFC_CTRL_PG_SIZE_SHIFT              9
#define ADI_NFC_CTRL_PG_SIZE_SET_256BYTES       (*pADI_NFC_CTRL &= ~ADI_NFC_CTRL_PG_SIZE_MASK)
#define ADI_NFC_CTRL_PG_SIZE_SET_512BYTES       (*pADI_NFC_CTRL |= ADI_NFC_CTRL_PG_SIZE_MASK)

/* set NFC Control register */
#define ADI_NFC_CTRL_REG_SET_VAL(V)             (*pADI_NFC_CTRL = (V))

/*********************************************************************************

NFC Status Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_STATUS                          (ADI_NFC_REG_BASE + ADI_NFC_STATUS_REG_OFFSET)
#define pADI_NFC_STATUS                         ((volatile u16*)ADI_NFC_STATUS)

/* Not busy, 0 - Busy request, 1 - not Busy */
#define ADI_NFC_STATUS_NBUSY_MASK               0x0001
#define ADI_NFC_STATUS_NBUSY_STAT               (*pADI_NFC_STATUS & ADI_NFC_STATUS_NBUSY_MASK)

/* Write Buffer Full, 0 - write buffer not full, 1 - write buffer full */
#define ADI_NFC_STATUS_WB_FULL_MASK             0x0002
#define ADI_NFC_STATUS_WB_FULL_STAT             (*pADI_NFC_STATUS & ADI_NFC_STATUS_WB_FULL_MASK)

/* Page write pending, 0 - No Page Write Pending, 1 - Page Write Pending */
#define ADI_NFC_STATUS_PG_WR_PEND_MASK          0x0004
#define ADI_NFC_STATUS_PG_WR_PEND_STAT          (*pADI_NFC_STATUS & ADI_NFC_STATUS_PG_WR_PEND_MASK)

/* Page Read pending, 0 - No Page Read Pending, 1 - Page Read Pending */
#define ADI_NFC_STATUS_PG_RD_PEND_MASK          0x0008
#define ADI_NFC_STATUS_PG_RD_PEND_STAT          (*pADI_NFC_STATUS & ADI_NFC_STATUS_PG_RD_PEND_MASK)

/* Write buffer empty, 0 - Write Buffer Not Empty, 1 - Write Buffer Empty */
#define ADI_NFC_STATUS_WB_EMPTY_MASK            0x0010
#define ADI_NFC_STATUS_WB_EMPTY_STAT            (*pADI_NFC_STATUS & ADI_NFC_STATUS_WB_EMPTY_MASK)

/*********************************************************************************

NFC Interrupt Status Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_IRQSTAT                         (ADI_NFC_REG_BASE + ADI_NFC_IRQSTAT_REG_OFFSET)
#define pADI_NFC_IRQSTAT                        ((volatile u16*)ADI_NFC_IRQSTAT)

/* Not busy IRQ, 0 - No raising edge on nBUSY detected, 1 - Raising edge on nBUSY detected */
#define ADI_NFC_IRQSTAT_NBUSYIRQ_MASK           0x0001
#define ADI_NFC_IRQSTAT_NBUSYIRQ_STAT           (*pADI_NFC_IRQSTAT & ADI_NFC_IRQSTAT_NBUSYIRQ_MASK)
#define ADI_NFC_IRQSTAT_NBUSYIRQ_CLEAR          (*pADI_NFC_IRQSTAT = ADI_NFC_IRQSTAT_NBUSYIRQ_MASK)

/* Write buffer Overflow, 0 - No Write buffer overflow detected, 1 - Write buffer overflow detected */
#define ADI_NFC_IRQSTAT_WB_OVF_MASK             0x0002
#define ADI_NFC_IRQSTAT_WB_OVF_STAT             (*pADI_NFC_IRQSTAT & ADI_NFC_IRQSTAT_WB_OVF_MASK)
#define ADI_NFC_IRQSTAT_WB_OVF_CLEAR            (*pADI_NFC_IRQSTAT = ADI_NFC_IRQSTAT_WB_OVF_MASK)

/* Write buffer edge detect, 0 - No raising edge on write buffer empty, 1 - Raising edge on write buffer empty */
#define ADI_NFC_IRQSTAT_WB_EMPTY_MASK            0x0004
#define ADI_NFC_IRQSTAT_WB_EMPTY_STAT            (*pADI_NFC_IRQSTAT & ADI_NFC_IRQSTAT_WB_EMPTY_MASK)
#define ADI_NFC_IRQSTAT_WB_EMPTY_CLEAR           (*pADI_NFC_IRQSTAT = ADI_NFC_IRQSTAT_WB_EMPTY_MASK)

/* Read Data ready, 0 - No Read data Read, 1 - Read data is in NFC_READ register */
#define ADI_NFC_IRQSTAT_RD_RDY_MASK             0x0008
#define ADI_NFC_IRQSTAT_RD_RDY_STAT             (*pADI_NFC_IRQSTAT & ADI_NFC_IRQSTAT_RD_RDY_MASK)
#define ADI_NFC_IRQSTAT_RD_RDY_CLEAR            (*pADI_NFC_IRQSTAT = ADI_NFC_IRQSTAT_RD_RDY_MASK)

/* Page write done, 0 - No page write completed, 1 - Page write completed */
#define ADI_NFC_IRQSTAT_PG_WR_DONE_MASK         0x0010
#define ADI_NFC_IRQSTAT_PG_WR_DONE_STAT         (*pADI_NFC_IRQSTAT & ADI_NFC_IRQSTAT_PG_WR_DONE_MASK)
#define ADI_NFC_IRQSTAT_PG_WR_DONE_CLEAR        (*pADI_NFC_IRQSTAT = ADI_NFC_IRQSTAT_PG_WR_DONE_MASK)

/* Clear selected flag(s) in NFC IRQ status register */
#define ADI_NFC_IRQSTAT_CLEAR_FLAGS(V)          (*pADI_NFC_IRQSTAT = (V))

/* Clear NFC IRQ status register */
#define ADI_NFC_IRQSTAT_CLEAR_ALL               (*pADI_NFC_IRQSTAT = 0x001F)

/*********************************************************************************

NFC Interrupt Mask Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_IRQMASK                         (ADI_NFC_REG_BASE + ADI_NFC_IRQMASK_REG_OFFSET)
#define pADI_NFC_IRQMASK                        ((volatile u16*)ADI_NFC_IRQMASK)

/* Mask nBUSY interrupt, 0 - Unmask, 1 - mask */
#define ADI_NFC_IRQMASK_NBUSYIRQ_MASK           0x0001
#define ADI_NFC_IRQMASK_NBUSYIRQ_ENABLE         (*pADI_NFC_IRQMASK &= ~ADI_NFC_IRQMASK_NBUSYIRQ_MASK)
#define ADI_NFC_IRQMASK_NBUSYIRQ_DISABLE        (*pADI_NFC_IRQMASK |= ADI_NFC_IRQMASK_NBUSYIRQ_MASK)

/* Mask Write buffer Overflow interrupt, 0 - Unmask, 1 - mask */
#define ADI_NFC_IRQMASK_WB_OVF_MASK             0x0002
#define ADI_NFC_IRQMASK_WB_OVF_ENABLE           (*pADI_NFC_IRQMASK &= ~ADI_NFC_IRQMASK_WB_OVF_MASK)
#define ADI_NFC_IRQMASK_WB_OVF_DISABLE          (*pADI_NFC_IRQMASK |= ADI_NFC_IRQMASK_WB_OVF_MASK)

/* Mask Write buffer edge detect interrupt, 0 - Unmask, 1 - mask */
#define ADI_NFC_IRQMASK_WB_EMPTY_MASK           0x0004
#define ADI_NFC_IRQMASK_WB_EMPTY_ENABLE         (*pADI_NFC_IRQMASK &= ~ADI_NFC_IRQMASK_WB_EMPTY_MASK)
#define ADI_NFC_IRQMASK_WB_EMPTY_DISABLE        (*pADI_NFC_IRQMASK |= ADI_NFC_IRQMASK_WB_EMPTY_MASK)

/* Mask Read Data ready interrupt, 0 - Unmask, 1 - mask */
#define ADI_NFC_IRQMASK_RD_RDY_MASK             0x0008
#define ADI_NFC_IRQMASK_RD_RDY_ENABLE           (*pADI_NFC_IRQMASK &= ~ADI_NFC_IRQMASK_RD_RDY_MASK)
#define ADI_NFC_IRQMASK_RD_RDY_DISABLE          (*pADI_NFC_IRQMASK |= ADI_NFC_IRQMASK_RD_RDY_MASK)

/* Mask Page write done interrupt, 0 - Unmask, 1 - mask */
#define ADI_NFC_IRQMASK_PG_WR_DONE_MASK         0x0010
#define ADI_NFC_IRQMASK_PG_WR_DONE_ENABLE       (*pADI_NFC_IRQMASK &= ~ADI_NFC_IRQMASK_PG_WR_DONE_MASK)
#define ADI_NFC_IRQMASK_PG_WR_DONE_DISABLE      (*pADI_NFC_IRQMASK |= ADI_NFC_IRQMASK_PG_WR_DONE_MASK)

/* Enable selected NFC IRQ status bits */
#define ADI_NFC_IRQMASK_ENABLE(V)               (*pADI_NFC_IRQMASK &= (0x1F & ~(V)))
/* Disable selected NFC IRQ status bits */
#define ADI_NFC_IRQMASK_DISABLE(V)              (*pADI_NFC_IRQMASK |= (V))

/* Enable all NFC IRQ status bits */
#define ADI_NFC_IRQMASK_ENABLE_ALL              (*pADI_NFC_IRQMASK = 0)
/* Disable all NFC IRQ status bits */
#define ADI_NFC_IRQMASK_DISABLE_ALL             (*pADI_NFC_IRQMASK = 0x1F)

/*********************************************************************************

NFC ECC0 register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_ECC0                            (ADI_NFC_REG_BASE + ADI_NFC_ECC0_REG_OFFSET)
#define pADI_NFC_ECC0                           ((volatile u16*)ADI_NFC_ECC0)

/*********************************************************************************

NFC ECC1 register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_ECC1                            (ADI_NFC_REG_BASE + ADI_NFC_ECC1_REG_OFFSET)
#define pADI_NFC_ECC1                           ((volatile u16*)ADI_NFC_ECC1)

/*********************************************************************************

NFC ECC2 register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_ECC2                            (ADI_NFC_REG_BASE + ADI_NFC_ECC2_REG_OFFSET)
#define pADI_NFC_ECC2                           ((volatile u16*)ADI_NFC_ECC2)

/*********************************************************************************

NFC ECC3 register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_ECC3                            (ADI_NFC_REG_BASE + ADI_NFC_ECC3_REG_OFFSET)
#define pADI_NFC_ECC3                           ((volatile u16*)ADI_NFC_ECC3)

/*********************************************************************************

NFC Count register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_COUNT                           (ADI_NFC_REG_BASE + ADI_NFC_COUNT_REG_OFFSET)
#define pADI_NFC_COUNT                          ((volatile u16*)ADI_NFC_COUNT)

/*********************************************************************************

NFC ECC Reset register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_RST                             (ADI_NFC_REG_BASE + ADI_NFC_RST_REG_OFFSET)
#define pADI_NFC_RST                            ((volatile u16*)ADI_NFC_RST)

/* Reset registers/counters */
#define ADI_NFC_RESET_REGS                      (*pADI_NFC_RST = 1)

/*********************************************************************************

NFC Page Control register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_PGCTL                           (ADI_NFC_REG_BASE + ADI_NFC_PGCTL_REG_OFFSET)
#define pADI_NFC_PGCTL                          ((volatile u16*)ADI_NFC_PGCTL)

/* Page Read Start, 0 - No effect, 1 - Start Page Read */
#define ADI_NFC_PGCTL_PAGE_READ_START           (*pADI_NFC_PGCTL = 1)

/* Page Write Start, 0 - No effect, 1 - Start Page Write */
#define ADI_NFC_PGCTL_PAGE_WRITE_START          (*pADI_NFC_PGCTL = 2)

/*********************************************************************************

NFC Read Data register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_READ_DATA                       (ADI_NFC_REG_BASE + ADI_NFC_READ_REG_OFFSET)
#define pADI_NFC_READ_DATA                      ((volatile u16*)ADI_NFC_READ_DATA)

/*********************************************************************************

NFC Address register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_ADDR                            (ADI_NFC_REG_BASE + ADI_NFC_ADDR_REG_OFFSET)
#define pADI_NFC_ADDR                           ((volatile u16*)ADI_NFC_ADDR)

/*********************************************************************************

NFC Command register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_CMD                             (ADI_NFC_REG_BASE + ADI_NFC_CMD_REG_OFFSET)
#define pADI_NFC_CMD                            ((volatile u16*)ADI_NFC_CMD)

/* Insert command */
#define ADI_NFC_INSERT_CMD(V)                   (*pADI_NFC_CMD = (V))

/*********************************************************************************

NFC Data Write register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_WRITE_DATA                      (ADI_NFC_REG_BASE + ADI_NFC_DATA_WR_REG_OFFSET)
#define pADI_NFC_WRITE_DATA                     ((volatile u16*)ADI_NFC_WRITE_DATA)

/*********************************************************************************

NFC Data Read register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_NFC_DATA_READ                       (ADI_NFC_REG_BASE + ADI_NFC_DATA_RD_REG_OFFSET)
#define pADI_NFC_DATA_READ                      ((volatile u16*)ADI_NFC_DATA_READ)

/* Trigger a read request */
#define ADI_NFC_TRIGGER_DATA_READ               (*pADI_NFC_DATA_READ = 0)

#endif  /* __ADI_NFC_REG_H__ */

/*****/
