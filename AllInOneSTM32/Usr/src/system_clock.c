#include "system_clock.h"

RCC_ClocksTypeDef g_RCC_Clocks;

/*******************************************************************
* @brief    SYSCLK, HCLK, PCLK2 and PCLK1 configuration use HSI
* @param    None
* @retval   None
*******************************************************************/
void RCC_Configuration(void)
{
    //Resets the RCC clock configuration to the default reset state.
    RCC_DeInit();

    RCC_HSICmd(ENABLE);

    //Configures the AHB clock (HCLK).
    RCC_HCLKConfig(STM_HCLK_CLOCK);

    //Configures the High Speed APB clock (PCLK2).
    RCC_PCLK2Config(STM_PCLK2_CLOCK);

    //Configures the Low Speed APB clock (PCLK1).
    RCC_PCLK1Config(STM_PCLK1_CLOCK);
    
    //Note: The PLL output frequencey must be in the range of 16-24MHz
    //      detail for datasheet page 73.
    RCC_PLLConfig(RCC_PLLSource_HSI_Div2, STM_PLL_CLOCK);
    
    RCC_PLLCmd(ENABLE);//Enable PLL
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);//Wait till PLL is ready
    
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);//Select PLL as system clock source

    while(RCC_GetSYSCLKSource() != 0X08);//Wait till PLL is used as system clock source

    //get system clock infomations.
    RCC_GetClocksFreq(&g_RCC_Clocks);
}

