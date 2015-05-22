/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           system_gpio.c
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
#include "system_gpio.h"

#ifdef __SYSTEM_GPIO_C_

#define SYSTEM_GPIO_DEBUG_INFO

#ifdef SYSTEM_GPIO_DEBUG_INFO
#define SYSTEM_GPIO_DBG_INFO(fmt, arg...) AIO_printf("\r\n[SYSTEM_GPIO] "fmt, ##arg)
#else
#define SYSTEM_GPIO_DBG_INFO(fmt, arg...)
#endif



/*********************************************************************************************************
** Function name:           gpio_init
** Descriptions:            gpio_init
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void gpio_init(GPIO_TypeDef *GPIOx,u16 GPIO_Pin,GPIOFunc Function,GPIODir Direction)
{
    volatile u16 *port_fer = &GPIO_CON->PORTF_fer;
    volatile u16 *port_mux = &GPIO_CON->PORTF_mux;
    u16 offset = 0;
    
    if(GPIOx == GPIOF)
    {
        port_fer = &GPIO_CON->PORTF_fer;
        port_mux = &GPIO_CON->PORTF_mux;
        if(GPIO_Pin == 0)
        {
            offset = 0;
        }
        if((1 <= GPIO_Pin) && (GPIO_Pin <= 6))
        {
            offset = 2;
        }
        if(GPIO_Pin == 7)
        {
            offset = 4;
        }
        if((8 <= GPIO_Pin) && (GPIO_Pin <= 10))
        {
            offset = 6;
        }
        if((11 <= GPIO_Pin) && (GPIO_Pin <= 14))
        {
            offset = 8;
        }
        if(GPIO_Pin == 15)
        {
            offset = 10;
        }
    }
    else if(GPIOx == GPIOG)
    {
        port_fer = &GPIO_CON->PORTG_fer;
        port_mux = &GPIO_CON->PORTG_mux;
        if((GPIO_Pin <= 2))
        {
            offset = 0;
        }
        if(GPIO_Pin == 3)
        {
            offset = 2;
        }
        if(GPIO_Pin == 4)
        {
            offset = 4;
        }
        if((5 <= GPIO_Pin) && (GPIO_Pin <= 7))
        {
            offset = 6;
        }
        if(GPIO_Pin == 8)
        {
            offset = 8;
        }
        if((9 <= GPIO_Pin) && (GPIO_Pin <= 10))
        {
            offset = 10;
        }
        if(GPIO_Pin == 11)
        {
            offset = 12;
        }
        if((12 <= GPIO_Pin) && (GPIO_Pin <= 15))
        {
            offset = 14;
        }

    }
    else if(GPIOx == GPIOH)
    {
        port_fer = &GPIO_CON->PORTH_fer;
        port_mux = &GPIO_CON->PORTH_mux;
        if((GPIO_Pin <= 3))
        {
            offset = 0;
        }
        if((4 <= GPIO_Pin) && (GPIO_Pin <= 5))
        {
            offset = 2;
        }
        if(GPIO_Pin == 6)
        {
            offset = 4;
        }
        if(GPIO_Pin == 7)
        {
            offset = 6;
        }
    }
    
    if(Function == Function_GPIO)
    {
        *port_fer &= ~((u16)0x01 << GPIO_Pin);
        *port_mux &= ~((u16)0x03 << offset);
    }
    else if(Function == Function_First)
    {
        *port_fer |= ((u16)0x01 << GPIO_Pin);
        *port_mux |= ((u16)0x00 << offset);
    
    }
    else if(Function == Function_Second)
    {
        *port_fer |= ((u16)0x01 << GPIO_Pin);
        *port_mux |= ((u16)0x01 << offset);
    
    }
    else if(Function == Function_Third)
    {
        *port_fer |= ((u16)0x01 << GPIO_Pin);
        *port_mux |= ((u16)0x02 << offset);
    
    }
    else if(Function == Function_Fourth)
    {
        *port_fer |= ((u16)0x01 << GPIO_Pin);
        *port_mux |= ((u16)0x03 << offset);
    
    }
    
    if(Direction == GPIO_Mode_In)
    {
        GPIOx->IO_DIR &= ~((u16)0x01 << GPIO_Pin);
        GPIOx->IO_INEN |= ((u16)0x01 << GPIO_Pin);
    }
    else if(Direction == GPIO_Mode_Out)
    {
        GPIOx->IO_DIR |= ((u16)0x01 << GPIO_Pin);
        GPIOx->IO_INEN &= ~((u16)0x01 << GPIO_Pin);
    
    }
    
}

/*********************************************************************************************************
** Function name:           gpio_set
** Descriptions:            gpio_set
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void gpio_set(GPIO_TypeDef *GPIOx,u16 GPIO_Pin)
{
    GPIOx->IO_SET = ((u16)0x01 << GPIO_Pin);
}

/*********************************************************************************************************
** Function name:           gpio_clear
** Descriptions:            gpio_clear
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void gpio_clear(GPIO_TypeDef *GPIOx,u16 GPIO_Pin)
{
    GPIOx->IO_CLEAR = ((u16)0x01 << GPIO_Pin);
}

/*********************************************************************************************************
** Function name:           gpio_toggle
** Descriptions:            gpio_toggle
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void gpio_toggle(GPIO_TypeDef *GPIOx,u16 GPIO_Pin)
{
    GPIOx->IO_TOGGLE = ((u16)0x01 << GPIO_Pin);
}

/*********************************************************************************************************
** Function name:           system_gpio_init
** Descriptions:            system_gpio_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_gpio_init(void)
{
    //gpio_init(GPIOG,GPIO_Pin_15,Function_GPIO,GPIO_Mode_Out);
    return 0;
}


/*********************************************************************************************************
** Function name:           system_gpio_server
** Descriptions:            system_gpio_server
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_gpio_server(void)
{
    static u32 u32NextChangeTime = 0;
    static u8 Toogle = 0;
    
    if(IsOnTime(u32NextChangeTime))
    {
        u32NextChangeTime += 100;
        
        //AIO_printf("system_gpio_server.\n");
        
        if(Toogle)
        {
            //gpio_set(GPIOG,GPIO_Pin_15);
        }
        else
        {
            //gpio_clear(GPIOG,GPIO_Pin_15);
        
        }
        Toogle = !Toogle;
    }
    
    
    return 0;
}

#endif //__SYSTEM_GPIO_C_
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
