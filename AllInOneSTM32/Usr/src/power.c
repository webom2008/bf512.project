/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           power.c
** Last modified Date:  2013-10-14
** Last Version:        V1.00
** Descriptions:        the specific codes for NIBP target boards
**                      User may modify it as needed
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-10-14
** Version:             V1.00
** Descriptions:        The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-10-14
** Version:             V1.00
** Descriptions:        Add some comment
**
*********************************************************************************************************/
#include "main.h"
#include "system_adc.h"

#ifdef __POWER_C__

#define POWER_DEBUG_INFO

#ifdef POWER_DEBUG_INFO
#define POWER_DBG_INFO(fmt, arg...) printf("\r\n[POWER] "fmt, ##arg)
#else
#define POWER_DBG_INFO(fmt, arg...)
#endif

#define PWR_SLEEPEntry_WFI          ((uint8_t)0x01)
#define PWR_SLEEPEntry_WFE          ((uint8_t)0x02)
#define IS_PWR_SLEEP_ENTRY(ENTRY)   (((ENTRY) == PWR_SLEEPEntry_WFI) || ((ENTRY) == PWR_SLEEPEntry_WFE))
#define CR_DS_MASK                  ((uint32_t)0xFFFFFFFC)


#define PWR_BIT_P5VAE               (7)
#define PWR_BIT_N5VAE               (6)
#define PWR_BIT_D3V3E               (5)
#define PWR_BIT_VDDINT              (4)
#define PWR_BIT_1V25E               (3)
#define PWR_BIT_NIBP                (2)
#define PWR_BIT_OVLD_OFF            (1)

#define PWR_NIBP_LOW                (1000*4095/3000)
#define PWR_NIBP_HIGH               (3000*4095/3000)
#define PWR_D3V3E_LOW               (1000*4095/3000)
#define PWR_D3V3E_HIGH              (3000*4095/3000)
#define PWR_VDDINT_LOW              (1000*4095/3000)
#define PWR_VDDINT_HIGH             (3000*4095/3000)
#define PWR_1V25E_LOW               (1000*4095/3000)
#define PWR_1V25E_HIGH              (3000*4095/3000)
#define PWR_BIT_5VAE_LOW            (1000*4095/3000)
#define PWR_BIT_5VAE_HIGH           (3000*4095/3000)
#define PWR_OVLD_OFF_LOW            (1000*4095/3000)
#define PWR_OVLD_OFF_HIGH           (3000*4095/3000)

u8 g_power_status = 0;

/*********************************************************************************************************
** Function name:           power_init
** Descriptions:            power_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int power_init(void)
{
    return 0;

}

/*********************************************************************************************************
** Function name:           PWR_EnterSLEEPMode
** Descriptions:            PWR_EnterSLEEPMode
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void PWR_EnterSLEEPMode(uint32_t PWR_Regulator, uint8_t PWR_SLEEPEntry)
{
    uint32_t tmpreg = 0;

    /* Check the parameters */
    assert_param(IS_PWR_REGULATOR(PWR_Regulator));
    assert_param(IS_PWR_SLEEP_ENTRY(PWR_SLEEPEntry));

    /* Select the regulator state in Sleep mode ---------------------------------*/
    tmpreg = PWR->CR;

    /* Clear PDDS and LPDSR bits */
    tmpreg &= CR_DS_MASK;

    /* Set LPDSR bit according to PWR_Regulator value */
    tmpreg |= PWR_Regulator;

    /* Store the new value */
    PWR->CR = tmpreg;

    /* Clear SLEEPDEEP bit of Cortex System Control Register */
    SCB->SCR &= (uint32_t)~((uint32_t)SCB_SCR_SLEEPDEEP);

    /* Select SLEEP mode entry -------------------------------------------------*/
    if(PWR_SLEEPEntry == PWR_SLEEPEntry_WFI)
    {   
        /* Request Wait For Interrupt */
        __WFI();
    }
    else
    {
        /* Request Wait For Event */
        __WFE();
    }
}

/*********************************************************************************************************
** Function name:           enter_low_power_mode
** Descriptions:            enter_low_power_mode
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void enter_low_power_mode(POWER_MODE mode)
{
    switch(mode)
    {
        case SLEEP:
            PWR_EnterSLEEPMode(PWR_Regulator_ON,PWR_SLEEPEntry_WFI);
            break;
        
        case STOP:
            PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFI);
            break;
        
        case LPSTOP:
            PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
            break;
        
        case STANDBY:
            PWR_EnterSTANDBYMode();
            PWR_WakeUpPinCmd(ENABLE);
            break;
        
        default:
            break;
    }
}

/*********************************************************************************************************
** Function name:           power_server
** Descriptions:            power_server
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void power_server(void)
{
    static u32 u32NextChangeTime = 0;
    
    if(IsOnTime(u32NextChangeTime))
    {
        g_power_status = 0;
        if(g_SysADCResult.DET_5VAE)
        {
            SETBIT(g_power_status,PWR_BIT_P5VAE);
        }
        if(g_SysADCResult.DET_D3V3E)
        {
            SETBIT(g_power_status,PWR_BIT_D3V3E);
        }
        if(g_SysADCResult.DET_VDDINT)
        {
            SETBIT(g_power_status,PWR_BIT_VDDINT);
        }
        if(g_SysADCResult.DET_NIBP)
        {
            SETBIT(g_power_status,PWR_BIT_NIBP);
        }
        if(g_SysADCResult.DET_OVLD_OFF)
        {
            SETBIT(g_power_status,PWR_BIT_OVLD_OFF);
        }
        
        u32NextChangeTime += getNextTimeByMS(10);
    }    
}

void enter_softwareUpdate(u8 *pBuf)
{
    if (0xAA != *pBuf) return;
    BKP_WriteBackupRegister(BKP_DR3, 0xAA);
    NVIC_SystemReset(); //reboot to bootloader
}

#endif /* __POWER_C__ */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/



