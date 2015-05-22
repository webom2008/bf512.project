/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           i2c_int.h
** Last modified Date:  2013-10-21
** Last Version:        V1.0
** Descriptions:        i2c程序
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-10-21
** Version:             V1.00
** Descriptions:        i2c函数
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-10-21
** Version:             V1.00
** Descriptions:        添加API函数
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
#ifndef __I2C_H_
#define __I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "aio_stm32_conf.h"

#ifdef __I2C_INT_C__

#define I2C_CH1      1
#define I2C_CH2      2

#define I2C_CH_SEL    I2C_CH1
     
#if     (I2C_CH_SEL == I2C_CH1)
#define I2C_CH                  I2C1
#define I2C_CH_CLK              RCC_APB1Periph_I2C1
#define I2C_CH_GPIO_CLK         RCC_AHBPeriph_GPIOB
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
#define I2C_CH_GPIO_CLK         RCC_AHBPeriph_GPIOB
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

typedef struct I2C_DEVICE
{
    I2C_TypeDef *I2Cx;
    u8 ch;
    u8 irq_ev;
    u8 irq_er;
    u8 done;
    u8 addr;
    u16 sub_addr;
    u16 sub_addr_len;
    u32 len;
    u8 *tx_buf;
    u8 *rx_buf;
    u32 tx_len;
    u32 rx_len;
    int (*write)(struct I2C_DEVICE *pI2cDev,u8 *buf,u32 len);
    int (*read)(struct I2C_DEVICE *pI2cDev,u8 *buf,u32 *len);
    
}i2c_device;
    
extern int i2c_init(void);
extern void i2c_server(void);
extern u8 i2c_read_byte(i2c_device *pDev,u8 dev_addr,u16 sub_addr);
extern int i2c_read_buf(i2c_device *pDev,u8 dev_addr,u16 sub_addr,u8 sub_addr_len,u8 *buf,u8 len);
extern int i2c_write_byte(i2c_device *pDev,u8 dev_addr,u16 sub_addr,u8 data);
extern int i2c_write_buf(i2c_device *pDev,u8 dev_addr,u16 sub_addr,u16 sub_addr_len,u8 *buf,u8 len);

#endif /* __I2C_INT_C__ */

#ifdef __cplusplus
}
#endif /*  __cplusplus                 */

#endif /*  __I2C_H_                    */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
