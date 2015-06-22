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
#define SYS_GPIO_GLOBALS
#include "Config/includes.h"


//#define SYSTEM_GPIO_DEBUG_INFO

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
    }
    else if(Function == Function_First)
    {
        *port_fer |= ((u16)0x01 << GPIO_Pin);
        *port_mux &= ~((u16)0x03 << offset);
    }
    else if(Function == Function_Second)
    {
        *port_fer |= ((u16)0x01 << GPIO_Pin);
        *port_mux &= ~((u16)0x03 << offset);
        *port_mux |= ((u16)0x01 << offset);
    }
    else if(Function == Function_Third)
    {
        *port_fer |= ((u16)0x01 << GPIO_Pin);
        *port_mux &= ~((u16)0x03 << offset);
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
        GPIOx->IO_EDGE &= ~((u16)0x01 << GPIO_Pin);// Level Sensitivity
        GPIOx->IO_POLAR &= ~((u16)0x01 << GPIO_Pin);
        GPIOx->IO_MASKA_CLEAR = ((u16)0x01 << GPIO_Pin);
        GPIOx->IO_MASKB_CLEAR = ((u16)0x01 << GPIO_Pin);
    }
    else if(Direction == GPIO_Mode_Out)
    {
        GPIOx->IO_DIR |= ((u16)0x01 << GPIO_Pin);
        GPIOx->IO_INEN &= ~((u16)0x01 << GPIO_Pin);
        GPIOx->IO_CLEAR = ((u16)0x01 << GPIO_Pin);
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

void gpio_setInputBuf(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, BitAction action)
{
    if (Bit_SET == action)
    {
        GPIOx->IO_INEN |= ((u16)0x01 << GPIO_Pin);
    }
    else
    {
        GPIOx->IO_INEN &= ~((u16)0x01 << GPIO_Pin);
    }
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

void gpio_cfg_interrupt(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, Pin_Interpret type)
{
    switch (type)
    {
        case PIN_HIGH_AS_1:
            GPIOx->IO_POLAR &= ~((u16)0x01 << GPIO_Pin);
            GPIOx->IO_EDGE &= ~((u16)0x01 << GPIO_Pin);
            break;
        case PIN_RISING_EDGE_AS_1:
            GPIOx->IO_POLAR &= ~((u16)0x01 << GPIO_Pin);
            GPIOx->IO_EDGE |= ((u16)0x01 << GPIO_Pin);
            GPIOx->IO_BOTH &= ~((u16)0x01 << GPIO_Pin);
            break;
        case PIN_LOW_AS_1:
            GPIOx->IO_POLAR |= ((u16)0x01 << GPIO_Pin);
            GPIOx->IO_EDGE &= ~((u16)0x01 << GPIO_Pin);
            break;
        case PIN_FALLING_EDGE_AS_1:
            GPIOx->IO_POLAR |= ((u16)0x01 << GPIO_Pin);
            GPIOx->IO_EDGE |= ((u16)0x01 << GPIO_Pin);
            GPIOx->IO_BOTH &= ~((u16)0x01 << GPIO_Pin);
            break;
        case PIN_ANY_EDGE_AS_1:
            GPIOx->IO_EDGE |= ((u16)0x01 << GPIO_Pin);
            GPIOx->IO_BOTH |= ((u16)0x01 << GPIO_Pin);
            break;
        default:
            break;
    }
}

/*****************************************************************************
 Prototype    : gpio_interruptA
 Description  : gpio intrrupt A init
 Input        : GPIO_TypeDef *GPIOx  
                u16 GPIO_Pin         
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/12/25
    Author       : qiuweibo
    Modification : Created function

*****************************************************************************/
void gpio_interruptA(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, Pin_Interpret type)
{
    gpio_cfg_interrupt(GPIOx, GPIO_Pin, type);
    GPIOx->IO_MASKA_SET = ((u16)0x01 << GPIO_Pin);//Enable Interrupt A  
    GPIOx->IO_MASKB_CLEAR= ((u16)0x01 << GPIO_Pin);//Disable Interrupt B
}

void gpio_set_interruptA(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, BitAction action)
{
    if (Bit_SET == action)
    {
        GPIOx->IO_MASKA_SET = ((u16)0x01 << GPIO_Pin);
    }
    else
    {
        GPIOx->IO_MASKA_CLEAR= ((u16)0x01 << GPIO_Pin);
    }
}

/*****************************************************************************
 Prototype    : gpio_interruptB
 Description  : gpio interrupt B init
 Input        : GPIO_TypeDef *GPIOx  
                u16 GPIO_Pin         
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/12/25
    Author       : qiuweibo
    Modification : Created function

*****************************************************************************/
void gpio_interruptB(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, Pin_Interpret type)
{
    gpio_cfg_interrupt(GPIOx, GPIO_Pin, type);
    GPIOx->IO_MASKB_SET = ((u16)0x01 << GPIO_Pin);//Enable Interrupt B  
    GPIOx->IO_MASKA_CLEAR= ((u16)0x01 << GPIO_Pin);//Disable Interrupt A
}

void gpio_set_interruptB(GPIO_TypeDef *GPIOx,u16 GPIO_Pin, BitAction action)
{
    if (Bit_SET == action)
    {
        GPIOx->IO_MASKB_SET = ((u16)0x01 << GPIO_Pin);
    }
    else
    {
        GPIOx->IO_MASKB_CLEAR= ((u16)0x01 << GPIO_Pin);
    }
}

/*****************************************************************************
 Prototype    : gpio_ReadDataBit
 Description  : Reads the specified port pin.
 Input        : GPIO_TypeDef* GPIOx  
                u16 GPIO_Pin         
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/12/25
    Author       : qiuweibo
    Modification : Created function

*****************************************************************************/
u8 gpio_ReadDataBit(GPIO_TypeDef* GPIOx, u16 GPIO_Pin)
{
  u8 bitstatus = 0x00;
    
  if ((GPIOx->IO & ((u16)0x01 << GPIO_Pin)) != (u16)Bit_RESET)
  {
    bitstatus = (u8)Bit_SET;
  }
  else
  {
    bitstatus = (u8)Bit_RESET;
  }
  return bitstatus;
}

/*****************************************************************************
 Prototype    : gpio_ReadData
 Description  : Reads the specified GPIO data port.
 Input        : GPIO_TypeDef* GPIOx  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/12/25
    Author       : qiuweibo
    Modification : Created function

*****************************************************************************/
u16 gpio_ReadData(GPIO_TypeDef* GPIOx)
{
  return ((u16)GPIOx->IO);
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
//    static u32 u32NextChangeTime = 0;
//    static u8 Toogle = 0;
//    
//    if(IsOnTime(u32NextChangeTime))
//    {
//        u32NextChangeTime += 100;
//        
//        //AIO_printf("system_gpio_server.\n");
//        
//        if(Toogle)
//        {
//            //gpio_set(GPIOG,GPIO_Pin_15);
//        }
//        else
//        {
//            //gpio_clear(GPIOG,GPIO_Pin_15);
//        
//        }
//        Toogle = !Toogle;
//    }
//    
//    
    return 0;
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
