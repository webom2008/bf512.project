/**
  *(C) COPYRIGHT 2014 CVTE.SEECARE
  ******************************************************************************
  * @file    IAP/inc/config.h
  * @author  CVTE.SEECARE.QiuWeibo
  * @version V1.0.0
  * @date    2014/01/21
  * @brief   Header file for config.c
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "includes.h"

/* Exported types ------------------------------------------------------------*/
typedef  void (*pFunction)(void);
/* Exported constants --------------------------------------------------------*/


/*User Start Adrress @ page 16*/
#define USER_START_ADDRESS  ((uint32_t)0x08004000)

//define for I2C Interrface.
#define I2C_CH1      1
#define I2C_CH2      2

#define I2C_CH_SEL    I2C_CH1
     
#if     (I2C_CH_SEL == I2C_CH1)
#define I2C_CH                  I2C1
#define I2C_CH_CLK              RCC_APB1Periph_I2C1
#define I2C_CH_GPIO_CLK         RCC_APB2Periph_GPIOB
#define I2C_SCL_PORT            GPIOB
#define I2C_SDA_PORT            GPIOB
#define I2C_SCL_PIN             GPIO_Pin_6
#define I2C_SDA_PIN             GPIO_Pin_7
#define I2C_SCL_PIN_SOURCE      GPIO_PinSource6
#define I2C_SDA_PIN_SOURCE      GPIO_PinSource7
#define I2C_CH_EV_IRQn          I2C1_EV_IRQn
#define I2C_CH_ER_IRQn          I2C1_ER_IRQn
#define I2C_CH_EV_IRQHandler    I2C1_EV_IRQHandler
#define I2C_CH_ER_IRQHandler    I2C1_ER_IRQHandler
#elif   (I2C_CH_SEL == I2C_CH2)
#define I2C_CH                  I2C2
#define I2C_CH_CLK              RCC_APB1Periph_I2C2
#define I2C_CH_GPIO_CLK         RCC_APB2Periph_GPIOB
#define I2C_SCL_PORT            GPIOB
#define I2C_SDA_PORT            GPIOB
#define I2C_SCL_PIN             GPIO_Pin_10
#define I2C_SDA_PIN             GPIO_Pin_11
#define I2C_SCL_PIN_SOURCE      GPIO_PinSource10
#define I2C_SDA_PIN_SOURCE      GPIO_PinSource11
#define I2C_CH_EV_IRQn          I2C2_EV_IRQn
#define I2C_CH_ER_IRQn          I2C2_ER_IRQn
#define I2C_CH_EV_IRQHandler    I2C2_EV_IRQHandler
#define I2C_CH_ER_IRQHandler    I2C2_ER_IRQHandler
#endif

#define PAGE_SIZE                         (0x400)       // 1 Kbyte
#define FLASH_SIZE                        (0x10000)     //64 K
#define TOTAL_PAGE_NUMBER    		    ((uint16_t)0x40)//64 PAGES
#define TOTAL_USER_PAGE_NUMBER    	    ((uint16_t)0x30)//48 PAGES

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void RCC_Configuration(void);
void GPIO_Configuration(void);
void NVIC_Configuration(void);

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
