/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
******************************************************************************
Copyright (c), 2009  - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title: ADI System Services - ADC Control Module (ACM)

Description:
    This header file defines the MMRs for the ADC Control Module (ACM)
    It includes both the MMR addresses and the bitfield access macros.

*****************************************************************************/
#ifndef __ADI_ACM_REG_H__
#define __ADI_ACM_REG_H__

/*
** Shift count to reach ACM Control (ACM_CTL) Register fields
*/

/* Shift count to reach Trigger select 0 fields */
#define     ACM_CTRL_REG_TGR0_FLD_SHIFT         3u
/* Shift count to reach Trigger select 1 fields */
#define     ACM_CTRL_REG_TGR1_FLD_SHIFT         5u
/* Shift count to reach External Peripheral select fields */
#define     ACM_CTRL_REG_EPS_FLD_SHIFT          14u

/*
** ACM Control (ACM_CTL) Register Masks
*/

/* Mask to reserved fields */
#define     ACM_CTRL_REG_MASK                   0xC7FFu
/* Mask to ACM Enable field */
#define     ACM_CTRL_REG_ACM_EN_FLD_MASK        0xC7FEu
/* Mask to Trigger select 0 fields */
#define     ACM_CTRL_REG_TGR0_FLD_MASK          0xC7E7u
/* Mask to Trigger select 1 fields */
#define     ACM_CTRL_REG_TGR1_FLD_MASK          0xC79Fu
/* Mask to External Peripheral select fields */
#define     ACM_CTRL_REG_EPS_FLD_MASK           0x007FFu

/*
** ACM Control (ACM_CTL) Register/Bit-field manipulation macros
*/

/* Configure ACM Control Register */
#define     ACM_CTRL_REG_SET_VAL(v)             (*pACM_CTL = (v & ACM_CTRL_REG_MASK))
/* Enable ACM */
#define     ACM_CTRL_FLD_ACM_ENABLE             (*pACM_CTL |= ACM_EN)
/* Disable ACM */
#define     ACM_CTRL_FLD_ACM_DISABLE            (*pACM_CTL &= ~ACM_EN)
/* Query if ACM is enabled */
#define     ACM_CTRL_FLD_IS_ACM_ENABLED         (*pACM_CTL & ACM_EN)
/* Enable Timer 0 */
#define     ACM_CTRL_FLD_TMR0_ENABLE            (*pACM_CTL |= ACM_TMR0_EN)
/* Disable Timer 0 */
#define     ACM_CTRL_FLD_TMR0_DISABLE           (*pACM_CTL &= ~ACM_TMR0_EN)
/* Enable Timer 1 */
#define     ACM_CTRL_FLD_TMR1_ENABLE            (*pACM_CTL |= ACM_TMR1_EN)
/* Disable Timer 1 */
#define     ACM_CTRL_FLD_TMR1_DISABLE           (*pACM_CTL &= ~ACM_TMR1_EN)
/* Configure Trigger Select 0 */
#define     ACM_CTRL_FLD_TGR0_SET(v)            (*pACM_CTL = ((*pACM_CTL & ACM_CTRL_REG_TGR0_FLD_MASK) |\
                                                              (v << ACM_CTRL_REG_TGR0_FLD_SHIFT)))
/* Configure Trigger Select 1 */
#define     ACM_CTRL_FLD_TGR1_SET(v)            (*pACM_CTL = ((*pACM_CTL & ACM_CTRL_REG_TGR1_FLD_MASK) |\
                                                              (v << ACM_CTRL_REG_TGR1_FLD_SHIFT)))
/* Trigger 0 Polarity - Falling Edge */
#define     ACM_CTRL_FLD_TGR0_FALL              (*pACM_CTL |= ACM_TRG_POL0)
/* Trigger 0 Polarity - Raising Edge */
#define     ACM_CTRL_FLD_TGR0_RAISE             (*pACM_CTL &= ~ACM_TRG_POL0)
/* Trigger 1 Polarity - Falling Edge */
#define     ACM_CTRL_FLD_TGR1_FALL              (*pACM_CTL |= ACM_TRG_POL1)
/* Trigger 1 Polarity - Raising Edge */
#define     ACM_CTRL_FLD_TGR1_RAISE             (*pACM_CTL &= ~ACM_TRG_POL1)
/* Chip Select / Start Conversion Polarity - Active High */
#define     ACM_CTRL_FLD_CS_HIGH                (*pACM_CTL |= ACM_CS_POL)
/* Chip Select / Start Conversion Polarity - Active Low */
#define     ACM_CTRL_FLD_CS_LOW                 (*pACM_CTL &= ~ACM_CS_POL)
/* ADC Clock Polarity - Raising Edge after CS active */
#define     ACM_CTRL_FLD_CLK_POL_RAISE          (*pACM_CTL |= ACM_CS_POL)
/* ADC Clock Polarity - Falling Edge after CS active */
#define     ACM_CTRL_FLD_CLK_POL_FALL           (*pACM_CTL &= ~ACM_CS_POL)
/* Configure External Peripheral select fields */
#define     ACM_CTRL_FLD_EPS_SET(v)             (*pACM_CTL = ((*pACM_CTL & ACM_CTRL_REG_EPS_FLD_MASK) |\
                                                              (v << ACM_CTRL_REG_EPS_FLD_SHIFT)))

/*
** Shift count to reach ACM Status (ACM_STAT) Register fields
*/

/* Shift count to reach Event Missed field */
#define     ACM_STAT_REG_EMISS_FLD_SHIFT        1u
/* Shift count to reach Event Completed field */
#define     ACM_CTRL_REG_ECOM_FLD_SHIFT         2u
/* Shift count to reach Current Event fields */
#define     ACM_CTRL_REG_CEVNT_FLD_SHIFT        3u

/*
** ACM Status (ACM_STAT) Register Masks
*/

/* Mask to reserved fields */
#define     ACM_STAT_REG_MASK                   0x007Fu
/* Mask to ACM Busy field */
#define     ACM_STAT_REG_BSY_FLD_MASK           0x0001u
/* Mask to Event Missed field */
#define     ACM_STAT_REG_EMISS_FLD_MASK         0x0002u
/* Mask to Events Completed field */
#define     ACM_STAT_REG_ECOM_FLD_MASK          0x0004u
/* Mask to Current Event field */
#define     ACM_STAT_REG_CEVNT_FLD_MASK         0x0078u

/*
** ACM Status (ACM_STAT) Register/Bit-field manipulation macros
*/

/* Get ACM Status register value */
#define     ACM_STAT_REG_GET_VAL                (*pACM_STAT & ACM_STAT_REG_MASK)
/* Get Busy Field value */
#define     ACM_STAT_REG_IS_ACM_BSY             (*pACM_STAT & ACM_STAT_REG_BSY_FLD_MASK)
/* Get Event Missed Field value */
#define     ACM_STAT_REG_IS_EVENT_MISSED        (*pACM_STAT & ACM_STAT_REG_EMISS_FLD_MASK)
/* Get Event Completed Field value */
#define     ACM_STAT_REG_IS_EVENT_COMPLETEED    (*pACM_STAT & ACM_STAT_REG_ECOM_FLD_MASK)
/* Get Current Event Field value */
#define     ACM_STAT_REG_GET_CURRENT_EVENT      (*pACM_STAT & ACM_STAT_REG_CEVNT_FLD_MASK)

/*
** ACM Event Status (ACM_ES) Register Masks
*/

/* Mask to reserved fields ACM Event Status Register */
#define     ACM_ES_REG_MASK                     0xFFFFu

/*
** ACM Event Status (ACM_ES) Register/Bit-field manipulation macros
*/

/* Get Event Status register value */
#define     ACM_ES_REG_GET_VAL                  (*pACM_ES)
/* Check if an Event conversion is done */
#define     ACM_ES_REG_IS_EVENT_DONE(nEvent)    (*pACM_ES & (1 << nEvent))
/* Clear a particular event conversion status bit */
#define     ACM_ES_REG_CLEAR_EVENT_DONE(nEvent) (*pACM_ES &= (1 << nEvent))
/* Clear event conversion status register */
#define     ACM_ES_REG_CLEAR_ALL_EVENT_DONE     (*pACM_ES &= ACM_ES_REG_MASK)

/*
** ACM Interrupt Mask (ACM_IMSK) Register Masks
*/

/* Mask to reserved fields ACM Interrupt Mask Register */
#define     ACM_IMASK_REG_MASK                  0xFFFFu

/*
** ACM Interrupt Mask (ACM_IMSK) Register/Bit-field manipulation macros
*/

/* Get Interrupt Mask register value */
#define     ACM_IMASK_REG_GET_VAL               (*pACM_IMSK)
/* Set Interrupt Mask register value */
#define     ACM_IMASK_REG_SET_VAL(v)            (*pACM_IMSK = (v & ACM_IMASK_REG_MASK))
/* Enable interrupt for a selected event completion */
#define     ACM_IMASK_REG_ENABLE_INT(nEvent)    (*pACM_IMSK |= (1 << nEvent))
/* Disable interrupt for a selected event completion */
#define     ACM_IMASK_REG_DISABLE_INT(nEvent)   (*pACM_IMSK &= ~(1 << nEvent))
/* Disable interrupt for all events completion */
#define     ACM_IMASK_REG_DISABLE_ALL_INT       (*pACM_IMSK = 0)

/*
** ACM Missed Event Status (ACM_MS) Register Masks
*/

/* Mask to reserved fields ACM Missed Event Status Register */
#define     ACM_MS_REG_MASK                     0xFFFFu

/*
** ACM Missed Event Status (ACM_MS) Register/Bit-field manipulation macros
*/

/* Get Missed Event Status register value */
#define     ACM_MS_REG_GET_VAL                      (*pACM_MS)
/* Check if an Event was missed */
#define     ACM_MS_REG_WAS_EVENT_MISSED(nEvent)     (*pACM_MS & (1 << nEvent))
/* Clear a particular event missed status bit */
#define     ACM_ES_REG_CLEAR_EVENT_MISSED(nEvent)   (*pACM_MS &= (1 << nEvent))
/* Clear missed event status register */
#define     ACM_ES_REG_CLEAR_ALL_EVENT_MISSED       (*pACM_MS &= ACM_MS_REG_MASK)

/*
** ACM Event Missed Interrupt Mask (ACM_EMSK) Register Masks
*/

/* Mask to reserved fields ACM Event Missed Interrupt Mask Register */
#define     ACM_EMASK_REG_MASK                  0xFFFFu

/*
** ACM Event Missed Interrupt Mask (ACM_EMSK) Register/Bit-field manipulation macros
*/

/* Get Event Missed Interrupt Mask register value */
#define     ACM_EMASK_REG_GET_VAL               (*pACM_EMSK)
/* Set Event Missed Interrupt Mask register value */
#define     ACM_EMASK_REG_SET_VAL(v)            (*pACM_EMSK = (v & ACM_EMASK_REG_MASK))
/* Enable interrupt for a selected event miss */
#define     ACM_EMASK_REG_ENABLE_INT(nEvent)    (*pACM_EMSK |= (1 << nEvent))
/* Disable interrupt for a selected event miss */
#define     ACM_EMASK_REG_DISABLE_INT(nEvent)   (*pACM_EMSK &= ~(1 << nEvent))
/* Disable interrupt for all events miss */
#define     ACM_EMASK_REG_DISABLE_ALL_INT       (*pACM_EMSK = 0)

/*
** Shift count to reach ACM Event Control (ACM_ERx) Register fields
*/

/* Shift count to reach Event parameter field */
#define     ACM_ERx_REG_EPF_FLD_SHIFT           1u
/* Shift count to reach ADC Channel Select field */
#define     ACM_ERx_REG_ADC_CH_SEL_FLD_SHIFT    1u

/*
** ACM Event Control (ACM_ERx) Register Masks
*/

/* Mask to reserved fields */
#define     ACM_ERx_REG_MASK                    0x006Fu
/* Mask to Enable Event field */
#define     ACM_ERx_REG_ENAEV_FLD_MASK          0x0001u
/* Mask to Event parameter field*/
#define     ACM_ERx_REG_EPF_FLD_MASK            0x006Eu
/* Mask to ADC Channel Select field */
#define     ACM_ERx_REG_ADC_CH_SEL_FLD_MASK     0x0061u
/* Mask to ADC Range Select field */
#define     ACM_ERx_REG_ADC_RANGE_FLD_MASK      0x0010u
/* Mask to ADC Logic (Single-ended/Differential) field */
#define     ACM_ERx_REG_ADC_LOGIC_FLD_MASK      0x0020u

/*
** ACM Event Control (ACM_ERx) Register/Bit-field manipulation macros
*/

/* Configure ADC Event Control Register */
#define     ACM_ERx_REG_SET_VAL(e,v)            (*((volatile u16 *)(pACM_ER0 + (e<<1))) = (v & ACM_ERx_REG_MASK))
/* Enable an Event */
#define     ACM_ERx_REG_ENABLE_EVENT(e)         (*((volatile u16 *)(pACM_ER0 + (e<<1))) |= ACM_ERx_REG_ENAEV_FLD_MASK)
/* Disable an Event */
#define     ACM_ERx_REG_DISABLE_EVENT(e)        (*((volatile u16 *)(pACM_ER0 + (e<<1))) &= ~ACM_ERx_REG_ENAEV_FLD_MASK)
/* Query if the enable is enabled */
#define     ACM_ERx_REG_IS_EVENT_ENABLED(e)     (*((volatile u16 *)(pACM_ER0 + (e<<1))) & ACM_ERx_REG_ENAEV_FLD_MASK)
/* Configure ADC Event Parameter fields */
#define     ACM_ERx_REG_SET_EPF(e,v)            (*((volatile u16 *)(pACM_ER0 + (e<<1))) = ((*((volatile u16 *)(pACM_ER0 + (e<<1))) & ACM_ERx_REG_EPF_FLD_MASK) |\
                                                                                             (v << ACM_ERx_REG_EPF_FLD_SHIFT)))
/* Configure ADC Channel Select field */
#define     ACM_ERx_REG_SET_ADC_CH_SEL(e,v)     (*((volatile u16 *)(pACM_ER0 + (e<<1))) = ((*((volatile u16 *)(pACM_ER0 + (e<<1))) & ACM_ERx_REG_ADC_CH_SEL_FLD_MASK) |\
                                                                                             (v << ACM_ERx_REG_ADC_CH_SEL_FLD_SHIFT)))
/* Set ADC Range Select field */
#define     ACM_ERx_REG_SET_ADC_RANGE(e)        (*((volatile u16 *)(pACM_ER0 + (e<<1))) |= ACM_ERx_REG_ADC_RANGE_FLD_MASK)
/* Clear ADC Range Select field */
#define     ACM_ERx_REG_CLEAR_ADC_RANGE(e)      (*((volatile u16 *)(pACM_ER0 + (e<<1))) &= ~ACM_ERx_REG_ADC_RANGE_FLD_MASK)
/* Set ADC Logic Select field */
#define     ACM_ERx_REG_SET_ADC_LOGIC(e)        (*((volatile u16 *)(pACM_ER0 + (e<<1))) |= ACM_ERx_REG_ADC_LOGIC_FLD_MASK)
/* Clear ADC Logic Select field */
#define     ACM_ERx_REG_CLEAR_ADC_LOGIC(e)      (*((volatile u16 *)(pACM_ER0 + (e<<1))) &= ~ACM_ERx_REG_ADC_LOGIC_FLD_MASK)

/*
** ACM Event Time (ACM_ETx) Register/Bit-field manipulation macros
*/

/* Configure ADC Event Time Register */
#define     ACM_ETx_REG_SET_VAL(e,v)            (*((volatile u32 *)(pACM_ET0 + e)) = v)

/*
** Shift count to reach ACM Timing Configuration 0 (ACM_TC0) Register fields
*/

/* Shift count to reach Setup Cycle field */
#define     ACM_TC0_REG_SC_FLD_SHIFT            8u

/*
** ACM Timing Configuration 0 (ACM_TC0) Register Masks
*/

/* Mask to reserved fields */
#define     ACM_TC0_REG_MASK                    0xFFFFu
/* Mask to ACM Clock Divisor */
#define     ACM_TC0_REG_CLKDIV_FLD_MASK         0xFF00u
/* Mask to ACM Setup Cycle */
#define     ACM_TC0_REG_SC_FLD_MASK             0x00FFu

/*
** ACM Timing Configuration 0 (ACM_TC0) Register/Bit-field manipulation macros
*/

/* Set ACM Timing Configuration 0 register value */
#define     ACM_TC0_REG_SET_VAL(v)              (*pACM_TC0 = (v & ACM_TC0_REG_MASK))
/* Set ACM Clock Divisor value */
#define     ACM_TC0_REG_SET_CKDIV_FLD_VAL(v)    (*pACM_TC0 = ((*pACM_TC0 & ACM_TC0_REG_CLKDIV_FLD_MASK) | v))
/* Set ACM Setup Cycle value */
#define     ACM_TC0_REG_SET_SC_FLD_VAL(v)       (*pACM_TC0 = ((*pACM_TC0 & ACM_TC0_REG_SC_FLD_MASK) |\
                                                               v << ACM_TC0_REG_SC_FLD_SHIFT))

/*
** Shift count to reach ACM Timing Configuration 1 (ACM_TC1) Register fields
*/

/* Shift count to reach Hold Cycle field */
#define     ACM_TC1_REG_HC_FLD_SHIFT             8u
/* Shift count to reach Zero Cycle field */
#define     ACM_TC1_REG_ZC_FLD_SHIFT            12u

/*
** ACM Timing Configuration 1 (ACM_TC1) Register Masks
*/

/* Mask to reserved fields */
#define     ACM_TC1_REG_MASK                    0xFFFFu
/* Mask to ACM Start Conversion/ Chip-select (CS) width */
#define     ACM_TC1_REG_CSW_FLD_MASK            0xFF00u
/* Mask to ACM Hold Cycle */
#define     ACM_TC1_REG_HC_FLD_MASK             0xF0FFu
/* Mask to ACM Zero Cycle */
#define     ACM_TC1_REG_ZC_FLD_MASK             0x0FFFu

/*
** ACM Timing Configuration 1 (ACM_TC0) Register/Bit-field manipulation macros
*/

/* Set ACM Timing Configuration 1 register value */
#define     ACM_TC1_REG_SET_VAL(v)              (*pACM_TC1 = (v & ACM_TC1_REG_MASK))
/* Set ACM Clock Divisor value */
#define     ACM_TC1_REG_SET_CSW_FLD_VAL(v)      (*pACM_TC1 = ((*pACM_TC1 & ACM_TC1_REG_CSW_FLD_MASK) | v))
/* Set ACM Hold Cycle value */
#define     ACM_TC1_REG_SET_HC_FLD_VAL(v)       (*pACM_TC1 = ((*pACM_TC1 & ACM_TC1_REG_HC_FLD_MASK) |\
                                                               (v << ACM_TC1_REG_HC_FLD_SHIFT)))
/* Set ACM Zero Cycle value */
#define     ACM_TC1_REG_SET_ZC_FLD_VAL(v)       (*pACM_TC1 = ((*pACM_TC1 & ACM_TC1_REG_ZC_FLD_MASK) |\
                                                               (v << ACM_TC1_REG_ZC_FLD_SHIFT)))

#endif /* __ADI_ACM_REG_H__ */

/*****/

/*
**
** EOF:
**
*/
