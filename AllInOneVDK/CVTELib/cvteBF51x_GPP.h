/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           system_gpio.h
** Last modified Date:  2013-12-02
** Last Version:        V1.0
** Descriptions:        system_gpio
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-12-02
** Version:             V1.00
** Descriptions:        system_gpio
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-12-02
** Version:             V1.00
** Descriptions:        system_gpio
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
#ifndef __SYSTEM_GPIO_H_
#define __SYSTEM_GPIO_H_

#ifdef __cplusplus
 extern "C" {
#endif

#ifdef SYS_GPIO_GLOBALS
#define SYS_GPIO_EXT
#else
#define SYS_GPIO_EXT    extern
#endif

#include "cvteBF51x.h"

typedef struct
{
  volatile u16 IO;                  /*!< GPIO Data register,                            Address offset: 0x00 */
  u16  RESERVED0;                   /*!< Reserved,                                     0x02                 */
  volatile u16 IO_CLEAR;            /*!< GPIO Clear register,                           Address offset: 0x04 */
  u16  RESERVED1;                   /*!< Reserved,                                     0x06                 */
  volatile u16 IO_SET;              /*!< GPIO Set register,                             Address offset: 0x08 */
  u16  RESERVED2;                   /*!< Reserved,                                     0x0A                 */
  volatile u16 IO_TOGGLE;           /*!< GPIO Toggle register,                          Address offset: 0x0C */
  u16  RESERVED3;                   /*!< Reserved,                                     0x0E                 */
  volatile u16 IO_MASKA;            /*!< GPIO Mask Interrupt A register,                Address offset: 0x10 */
  u16  RESERVED4;                   /*!< Reserved,                                     0x12                 */
  volatile u16 IO_MASKA_CLEAR;      /*!< GPIO Mask Interrupt A Clear register,          Address offset: 0x14 */
  u16  RESERVED5;                   /*!< Reserved,                                     0x16                 */
  volatile u16 IO_MASKA_SET;        /*!< GPIO Mask Interrupt A Set register,            Address offset: 0x18 */
  u16  RESERVED6;                   /*!< Reserved,                                     0x1A                 */
  volatile u16 IO_MASKA_TOGGLE;     /*!< GPIO Mask Interrupt A Toggle register,         Address offset: 0x1C */
  u16  RESERVED7;                   /*!< Reserved,                                     0x1A                 */
  volatile u16 IO_MASKB;            /*!< GPIO Mask Interrupt B register,                Address offset: 0x20 */
  u16  RESERVED8;                   /*!< Reserved,                                     0x1A                 */
  volatile u16 IO_MASKB_CLEAR;      /*!< GPIO Mask Interrupt B Clear register,          Address offset: 0x24 */
  u16  RESERVED9;                   /*!< Reserved,                                     0x1A                 */
  volatile u16 IO_MASKB_SET;        /*!< GPIO Mask Interrupt B SET register,            Address offset: 0x28 */
  u16  RESERVED10;                  /*!< Reserved,                                     0x1A                 */
  volatile u16 IO_MASKB_TOGGLE;     /*!< GPIO Mask Interrupt B Toggle register,         Address offset: 0x2C */
  u16  RESERVED11;                  /*!< Reserved,                                     0x1A                 */
  volatile u16 IO_DIR;              /*!< GPIO Direction register,                       Address offset: 0x30 */
  u16  RESERVED12;                   /*!< Reserved,                                     0x1A                 */
  volatile u16 IO_POLAR;            /*!< GPIO Polarity register,                        Address offset: 0x34 */
  u16  RESERVED13;                  /*!< Reserved,                                     0x1A                 */
  volatile u16 IO_EDGE;             /*!< GPIO Interrupt Sensitivity register,           Address offset: 0x38 */
  u16  RESERVED14;                  /*!< Reserved,                                     0x1A                 */
  volatile u16 IO_BOTH;             /*!< GPIO Both register,                            Address offset: 0x3C */
  u16  RESERVED15;                  /*!< Reserved,                                     0x1A                 */
  volatile u16 IO_INEN;             /*!< GPIO Input Enable register,                    Address offset: 0x40 */
  u16  RESERVED16;                   /*!< Reserved,                                     0x1A                 */
    
}GPIO_TypeDef;

typedef struct
{
  volatile u16 PORTF_fer;           /*!< Function Enable register,                      Address offset: 0x00 */
  u16  RESERVED0;                   /*!< Reserved,                                     0x02                 */
  volatile u16 PORTG_fer;           /*!< Function Enable register,                      Address offset: 0x04 */
  u16  RESERVED1;                   /*!< Reserved,                                     0x06                 */
  volatile u16 PORTH_fer;           /*!< Function Enable register,                      Address offset: 0x08 */
  u16  RESERVED2;                   /*!< Reserved,                                     0x0A                 */
  u32  RESERVED13;                   /*!< Reserved,                                     0x0A                 */
  volatile u16 PORTF_mux;           /*!< Port F Multiplexer Control register,           Address offset: 0x0C */
  u16  RESERVED3;                   /*!< Reserved,                                     0x0E                 */
  volatile u16 PORTG_mux;           /*!< Port G Multiplexer Control register,           Address offset: 0x10 */
  u16  RESERVED4;                   /*!< Reserved,                                     0x12                 */
  volatile u16 PORTH_mux;           /*!< Port H Multiplexer Control register,           Address offset: 0x14 */
  u16  RESERVED5;                   /*!< Reserved,                                     0x16                 */
  u16  RESERVED6[18];               /*!< Reserved,                                     0x16                 */
  volatile u16 PORTF_Hysteresis;    /*!< Port F Hysteresis Control register,            Address offset: 0x18 */
  u16  RESERVED7;                   /*!< Reserved,                                     0x1A                 */
  volatile u16 PORTG_Hysteresis;    /*!< Port G Hysteresis Control register,            Address offset: 0x1C */
  u16  RESERVED8;                   /*!< Reserved,                                     0x1A                 */
  volatile u16 PORTH_Hysteresis;    /*!< Port H Hysteresis Control register,            Address offset: 0x20 */
  u16  RESERVED9;                   /*!< Reserved,                                     0x1A                 */
  u16  RESERVED10[26];              /*!< Reserved,                                     0x16                 */
  volatile u16 NONGPIO_Drive;       /*!< Non GPIO Drive strength Control register,      Address offset: 0x24 */
  u16  RESERVED11;                  /*!< Reserved,                                     0x1A                 */
  volatile u16 NONGPIO_Hysteresis;  /*!< Non GPIO Hysteresis Shadow register,           Address offset: 0x28 */
  u16  RESERVED12;                  /*!< Reserved,                                     0x1A                 */
  
}GPIO_Con;

typedef enum
{ 
    Function_GPIO = 0, 
    Function_First,         
    Function_Second,        
    Function_Third,      
    Function_Fourth,
}GPIOFunc;

typedef enum
{ 
    GPIO_Mode_In = 0, 
    GPIO_Mode_Out,
}GPIODir;

#define GPIOF_BASE                  (SYSTEM_MMR_BASE + 0x0700)
#define GPIOG_BASE                  (SYSTEM_MMR_BASE + 0x1500)
#define GPIOH_BASE                  (SYSTEM_MMR_BASE + 0x1700)
#define GPIO_CON_BASE               (SYSTEM_MMR_BASE + 0x3200)

#define GPIOF                       ((GPIO_TypeDef *) GPIOF_BASE)
#define GPIOG                       ((GPIO_TypeDef *) GPIOG_BASE)
#define GPIOH                       ((GPIO_TypeDef *) GPIOH_BASE)
#define GPIO_CON                    ((GPIO_Con *) GPIO_CON_BASE)

#define GPIO_Pin_0                 (0)  /*!< Pin 0 selected */
#define GPIO_Pin_1                 (1)  /*!< Pin 1 selected */
#define GPIO_Pin_2                 (2)  /*!< Pin 2 selected */
#define GPIO_Pin_3                 (3)  /*!< Pin 3 selected */
#define GPIO_Pin_4                 (4)  /*!< Pin 4 selected */
#define GPIO_Pin_5                 (5)  /*!< Pin 5 selected */
#define GPIO_Pin_6                 (6)  /*!< Pin 6 selected */
#define GPIO_Pin_7                 (7)  /*!< Pin 7 selected */
#define GPIO_Pin_8                 (8)  /*!< Pin 8 selected */
#define GPIO_Pin_9                 (9)  /*!< Pin 9 selected */
#define GPIO_Pin_10                (10)  /*!< Pin 10 selected */
#define GPIO_Pin_11                (11)  /*!< Pin 11 selected */
#define GPIO_Pin_12                (12)  /*!< Pin 12 selected */
#define GPIO_Pin_13                (13)  /*!< Pin 13 selected */
#define GPIO_Pin_14                (14)  /*!< Pin 14 selected */
#define GPIO_Pin_15                (15)  /*!< Pin 15 selected */
#define GPIO_Pin_All               (16)  /*!< All pins selected */


/** 
  * @brief  GPIO Init structure definition  
  */

typedef struct
{
    u16 GPIO_Pin;                     /*!< Specifies the GPIO pins to be configured.
                                      This parameter can be any value of @ref GPIO_pins_define */
    GPIOFunc Function;                /*!< Specifies the operating mode for the selected pins.
                                      This parameter can be a value of @ref GPIOMode_TypeDef */
    GPIODir Direction;
    
}GPIO_InitTypeDef;

/** 
  * @brief  Bit_SET and Bit_RESET enumeration  
  */

typedef enum
{ Bit_RESET = 0,
  Bit_SET
}BitAction;

typedef enum
{
    PIN_HIGH_AS_1 = 0,
    PIN_RISING_EDGE_AS_1,
    PIN_LOW_AS_1,
    PIN_FALLING_EDGE_AS_1,
    PIN_ANY_EDGE_AS_1
} Pin_Interpret ;

SYS_GPIO_EXT int system_gpio_init(void);

SYS_GPIO_EXT int system_gpio_server(void);

SYS_GPIO_EXT void gpio_init(GPIO_TypeDef *GPIOx,u16 GPIO_Pin,GPIOFunc Function,GPIODir Direction);

SYS_GPIO_EXT void gpio_set(GPIO_TypeDef *GPIOx,u16 GPIO_Pin);

SYS_GPIO_EXT void gpio_clear(GPIO_TypeDef *GPIOx,u16 GPIO_Pin);

SYS_GPIO_EXT void gpio_toggle(GPIO_TypeDef *GPIOx,u16 GPIO_Pin);

SYS_GPIO_EXT void gpio_cfg_interrupt(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, Pin_Interpret type);

SYS_GPIO_EXT void gpio_set_interruptA(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, BitAction action);

SYS_GPIO_EXT void gpio_interruptA(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, Pin_Interpret type);

SYS_GPIO_EXT void gpio_set_interruptB(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, BitAction action);

SYS_GPIO_EXT void gpio_interruptB(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, Pin_Interpret type);

SYS_GPIO_EXT u8 gpio_ReadDataBit(GPIO_TypeDef* GPIOx, u16 GPIO_Pin);

SYS_GPIO_EXT u16 gpio_ReadData(GPIO_TypeDef* GPIOx);

SYS_GPIO_EXT void gpio_setInputBuf(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, BitAction action);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYSTEM_GPIO_H_           */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
