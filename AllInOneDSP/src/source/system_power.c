/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           system_power.c
** Last modified Date:  2013-12-02
** Last Version:        V1.0
** Descriptions:        system_power
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-12-02
** Version:             V1.00
** Descriptions:        system_power
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-12-02
** Version:             V1.00
** Descriptions:        system_power
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
#include "system_power.h"

#ifdef __SYSTEM_POWER_C_

#define POWER_DEBUG_INFO

#ifdef POWER_DEBUG_INFO
#define POWER_DBG_INFO(fmt, arg...) printf("\r\n[POWER] "fmt, ##arg)
#else
#define POWER_DBG_INFO(fmt, arg...)
#endif



/*********************************************************************************************************
** Function name:           power_init
** Descriptions:            power_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_power_init(void)
{

    return 0;
}


/*********************************************************************************************************
** Function name:           PWR_EnterActiveMode
** Descriptions:            PWR_EnterActiveMode
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void PWR_EnterActiveMode(FunctionalState NewState)
{
    ADI_SYSCTRL_VALUES active;
        
    if(NewState == PWR_ENABLE)
    {
    
        bfrom_SysControl(SYSCTRL_READ | SYSCTRL_EXTVOLTAGE |
        SYSCTRL_PLLCTL, &active, NULL);
    
        active.uwPllCtl |= (BYPASS); /* PLL_OFF bit optional */
    
        bfrom_SysControl(SYSCTRL_WRITE | SYSCTRL_EXTVOLTAGE |
        SYSCTRL_PLLCTL, &active, NULL);
    }
    else if(NewState == PWR_DISABLE)
    {
    
        bfrom_SysControl(SYSCTRL_READ | SYSCTRL_EXTVOLTAGE |
        SYSCTRL_PLLCTL, &active, NULL);
    
        active.uwPllCtl &= ~(BYPASS); /* PLL_OFF bit optional */
    
        bfrom_SysControl(SYSCTRL_WRITE | SYSCTRL_EXTVOLTAGE |
        SYSCTRL_PLLCTL, &active, NULL);
    
    }
    return;

}

/*********************************************************************************************************
** Function name:           PWR_EnterSleepMode
** Descriptions:            PWR_EnterSleepMode
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void PWR_EnterSleepMode(FunctionalState NewState)
{
    ADI_SYSCTRL_VALUES sleep;
    
    /* use the ROM function */
    bfrom_SysControl(SYSCTRL_EXTVOLTAGE | SYSCTRL_PLLCTL |
    SYSCTRL_READ, &sleep, NULL);
    
    sleep.uwPllCtl |= STOPCK;                           /* either: Sleep Mode */
    
    /* use the ROM function */
    bfrom_SysControl(SYSCTRL_WRITE | SYSCTRL_EXTVOLTAGE |
    SYSCTRL_PLLCTL, &sleep, NULL);
    
    return; 
}

/*********************************************************************************************************
** Function name:           PWR_EnterDeepSleepMode
** Descriptions:            PWR_EnterDeepSleepMode
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void PWR_EnterDeepSleepMode(FunctionalState NewState)
{
    ADI_SYSCTRL_VALUES sleep;
    
    bfrom_SysControl(SYSCTRL_EXTVOLTAGE | SYSCTRL_PLLCTL |
    SYSCTRL_READ, &sleep, NULL);
    
    sleep.uwPllCtl |= PDWN;                             /* or: Deep Sleep Mode */
    
    bfrom_SysControl(SYSCTRL_WRITE | SYSCTRL_EXTVOLTAGE |
    SYSCTRL_PLLCTL, &sleep, NULL);
    
    return; 
    
}

/*********************************************************************************************************
** Function name:           PWR_EnterHibernateMode
** Descriptions:            PWR_EnterHibernateMode
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void PWR_EnterHibernateMode(FunctionalState NewState)
{
    ADI_SYSCTRL_VALUES hibernate;
    
    /* SCKELOW = 1: Enable Drive SCKE Low During Reset */
    /* Protect SDRAM contents during reset after wakeup */
    /* RTC/Reset Wake-Up Enable */
    /* Powerdown */
    hibernate.uwVrCtl=SCKELOW | WAKE |HIBERNATE;
   
    bfrom_SysControl(SYSCTRL_WRITE | SYSCTRL_VRCTL |
    SYSCTRL_EXTVOLTAGE, &hibernate, NULL);
    
    /* Hibernate State: no code executes until wakeup triggers reset */
  
}

/*********************************************************************************************************
** Function name:           enter_low_power_mode
** Descriptions:            enter_low_power_mode
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int enter_low_power_mode(POWER_MODE mode,FunctionalState NewState)
{
    switch(mode)
    {
        /* PLL Off */
        case PWR_ACTIVE:
            PWR_EnterActiveMode(NewState);
            break;
        
        /* System Clock Off */
        case PWR_SLEEP:
            PWR_EnterSleepMode(NewState);
            break;
        
        /* Core Clock and system clock off */
        case PWR_DEEP_SLEEP:
            PWR_EnterDeepSleepMode(NewState);
            break;
        
        /* Core Clock and system clock and regulator off */
        case PWR_HIBERNATE:
            PWR_EnterHibernateMode(NewState);
            break;
                
        default:
            break;
    }
    return 0;
}


/*********************************************************************************************************
** Function name:           system_power_server
** Descriptions:            system_power_server
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_power_server(void)
{
    static u32 PowerOffTime = 0;
    
    //enter_low_power_mode(PWR_ACTIVE,PWR_ENABLE);
    
    //while(1)
    {
    
    if(IsOnTime(PowerOffTime))
    {
        PowerOffTime += 1000;
        
        //AIO_printf("Power state:%d\n",PWR_SLEEP);
        
    }
    
    enter_low_power_mode(PWR_SLEEP,PWR_ENABLE);
    //enter_low_power_mode(PWR_DEEP_SLEEP,PWR_ENABLE);
    //enter_low_power_mode(PWR_HIBERNATE,PWR_ENABLE);
    }
    
    return 0;
}

#endif //__SYSTEM_POWER_C_
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
