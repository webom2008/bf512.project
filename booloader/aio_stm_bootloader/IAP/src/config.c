/**
  *(C) COPYRIGHT 2014 CVTE.SEECARE
  ******************************************************************************
  * @file    IAP/src/main.c 
  * @author  CVTE.SEECARE.QiuWeibo
  * @version V1.0.0
  * @date    2014/01/21
  * @brief   configuration file
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
NVIC_InitTypeDef NVIC_InitStructure;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/



/**
  * @brief  Configures the different system clocks.
  * @param  None
  * @retval : None
  */
void RCC_Configuration(void)
{
    /* Enable peripheral clocks --------------------------------------------------*/
    /* Enable I2C_CH clock */
    RCC_APB1PeriphClockCmd(I2C_CH_CLK, ENABLE);
    /* Enable I2C_CH GPIO clock */
    RCC_APB2PeriphClockCmd(I2C_CH_GPIO_CLK, ENABLE);
}



/**
  * @brief  Configures the GPIO ports related to SPI1 and
  *   User button B2.
  * @param  None
  * @retval : None
  */
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Configure I2C pins: SCL and SDA ----------------------------------------*/
    GPIO_InitStructure.GPIO_Pin =  I2C_SCL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(I2C_SCL_PORT, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin =  I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(I2C_SDA_PORT, &GPIO_InitStructure);

#if 0
    /*========================== GPIO related to  User button B2===================*/
    /* Configure KEY_BUTTON pin as input */
    GPIO_InitStructure.GPIO_Pin = GPIO_PIN_KEY_BUTTON;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIO_KEY_BUTTON, &GPIO_InitStructure);
#endif
}





/**
  * @brief  Configures NVIC and Vector Table base location.
  * @param  None
  * @retval : None
  */
void NVIC_Configuration(void)
{
#ifdef  VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
#endif

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* Configure and enable I2Cx event interrupt -------------------------------*/
    NVIC_InitStructure.NVIC_IRQChannel = I2C_CH_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);


    /* Configure and enable I2Cx error interrupt -------------------------------*/
    NVIC_InitStructure.NVIC_IRQChannel = I2C_CH_ER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

}

/************************END OF FILE****/
