#ifndef __SYSTEM_CLOCK_H
#define __SYSTEM_CLOCK_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "aio_stm32_conf.h"

// HSE base clock 12M
#define SYSTEM_HSE_CLOCK    12000000
#define SYSTEMCLOCK_20M     20000000
//#define SYSTEMCLOCK_24M     24000000    //24MHz max

#ifdef SYSTEMCLOCK_20M
#define STM_PLL_CLOCK       RCC_PLLMul_5        //2~16,HSI /2 x MUL = 4MHz x 5 = 20MHz,
#define STM_HCLK_CLOCK      RCC_SYSCLK_Div1     //HCLK = SYSCLK / 1 = 20MHz
#define STM_PCLK1_CLOCK     RCC_HCLK_Div2       //PCLK1 = HCLK / 2 = 10MHz
#define STM_PCLK2_CLOCK     RCC_HCLK_Div1       //PCLK2 = HCLK / 1 = 20MHz
#endif

extern RCC_ClocksTypeDef g_RCC_Clocks;

extern void RCC_Configuration(void);

#ifdef __cplusplus
}
#endif

#endif /*__SYSTEM_CLOCK_H*/

