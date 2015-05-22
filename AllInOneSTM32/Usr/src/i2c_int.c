/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           i2c.c
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
#include <string.h>
#include "i2c_int.h"

#ifdef __I2C_INT_C__

//#define I2C_DEBUG_INFO

#ifdef I2C_DEBUG_INFO
#define I2C_DBG_INFO(fmt, arg...) printf("\r\n[I2C] "fmt, ##arg)
#else
#define I2C_DBG_INFO(fmt, arg...)
#endif

//#define I2C_ERRO_INFO

#ifdef I2C_ERRO_INFO
#define I2C_ERR_INFO(fmt, arg...) printf("\r\n[I2C] "fmt, ##arg)
#else
#define I2C_ERR_INFO(fmt, arg...)
#endif

i2c_device g_i2c_dev;

i2c_device *g_pI2C_dev = &g_i2c_dev;

i2c_device g_i2c2_dev;

i2c_device *g_pI2C2_dev = &g_i2c2_dev;


#define I2C_CLK_SPEED           100000
#define I2C_OWN_ADDRESS         0x60
#define I2C_EEPROM_ADDRESS      0xA0



#define BIT_TRA             (1<<18)
#define BIT_MASTER          (1<<16)
#define BIT_TIMEOUT         (1<<14)
#define BIT_PERERR          (1<<12)
#define BIT_OVR             (1<<11)
#define BIT_AF              (1<<10)
#define BIT_ARLO            (1<<9)
#define BIT_BERR            (1<<8)
#define BIT_TXE             (1<<7)
#define BIT_RXNE            (1<<6)
#define BIT_STOP            (1<<4)
#define BIT_BTF             (1<<2)
#define BIT_ADDR            (1<<1)
#define BIT_START           (1<<0)

#define MASTER_START        (BIT_MASTER|BIT_START)
#define MASTER_ADDR         (BIT_MASTER|BIT_ADDR)
#define MASTER_RX_DATA      (BIT_MASTER|BIT_RXNE)
#define MASTER_TX_DATA      (BIT_MASTER|BIT_TRA|BIT_TXE)
#define MASTER_STOP         (BIT_STOP)
#define SLAVE_ADDR          (BIT_ADDR)
#define SLAVE_RX_DATA       (BIT_RXNE)
#define SLAVE_TX_DATA       (BIT_TRA|BIT_TXE)
#define SLAVE_STOP          (BIT_STOP)

#define BUF_OK              0
#define BUF_FULL            1
#define BUF_EMPTY           2
#define I2C_BUFFER_SIZE     128
#define BUF_NUM(x,y)        (x)>=(y)?(x-y):(I2C_BUFFER_SIZE-(y-x))

#define AIO_STM32_READ_MASK     (0x01)
#define AIO_STM32_WRITE_MASK    (0x00)

/*******************************************************************************/
typedef enum//range at 0x00 to 0x7F
{
    POWER_STATUS        = 0x00,
    SELF_CHECK          = 0x01,
    ECG_PROBE_RESULT    = 0x02,
    PROTECT_PRESSURE    = 0x03,
    CURRENT_PRESSURE    = 0x04,
    SOFTWARE_UPDATE,
    NIBP_VERIFY,
    NIBP_PROTECT_STATE,
} I2C_PacketID;

typedef struct
{
    u8 u8PowerStatus;
    u8 u8SelfCheck;
    
    /*
     * detect reslut for probe fall off and overload
     * |-bit7-|-bit6-|-bit5-|-bit4-|-bit3-|-bit2-|-bit1-|-bit0-|
     * |ECGV  |ECG2  |ECG1  |V1_OFF|RL_OFF|LL_OFF|RA_OFF|LA_OFF|
     */
    u8 u8ecgReslut;
} I2C_TxBufTypeDef;
I2C_TxBufTypeDef stm_i2c_tx_buf;
I2C_TxBufTypeDef *pstm_i2c_tx_buf = &stm_i2c_tx_buf;
#define STM32_I2C_TX_SIZE          (sizeof(stm_i2c_tx_buf) / sizeof(u8))
/*******************************************************************************/

u8 i2c_rx_buf[I2C_BUFFER_SIZE];
u8 i2c_tx_buf[I2C_BUFFER_SIZE];
u8 i2c_rx_point_cur, i2c_rx_point_handle;

u8 i2c2_rx_buf[I2C_BUFFER_SIZE];
u8 i2c2_tx_buf[I2C_BUFFER_SIZE];


/*********************************************************************************************************
** Function name:           i2c_device_init
** Descriptions:            i2c_device_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
static int i2c_device_init(i2c_device *pDev,u8 ch)
{
    if(ch == I2C_CH1)
    {
        pDev->I2Cx = I2C1;
        pDev->irq_ev = I2C1_EV_IRQn;
        pDev->irq_er = I2C1_ER_IRQn;
        //pDev->tx_buf = i2c_tx_buf;//del by QWB
        pDev->tx_buf = (u8 *)pstm_i2c_tx_buf;//add by QWB
        pDev->rx_buf = i2c_rx_buf;

    }
    else if(ch == I2C_CH2)
    {
        pDev->I2Cx = I2C2;
        pDev->irq_ev = I2C2_EV_IRQn;
        pDev->irq_er = I2C2_ER_IRQn;
        pDev->tx_buf = i2c2_tx_buf;
        pDev->rx_buf = i2c2_rx_buf;
    }
    pDev->ch = ch;
    pDev->tx_len = 0;
    pDev->rx_len = 0;

    return 0;
}

/*********************************************************************************************************
** Function name:           i2c_clk_init
** Descriptions:            i2c_clk_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
static void i2c_clk_init(i2c_device *pDev)
{
    if(pDev->ch == I2C_CH1)
    {
        printf("\ni2c_clk_init:ch1\n");
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    }
    else if(pDev->ch == I2C_CH2)
    {
        printf("i2c_clk_init:ch2\n");
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    }
}

/*********************************************************************************************************
** Function name:           i2c_pin_init
** Descriptions:            i2c_pin_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
static void i2c_pin_init(i2c_device *pDev)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    if(pDev->ch == I2C_CH1)
    {        
        //printf("i2c_pin_init:ch1\n");
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOB, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOB, &GPIO_InitStructure);
    }
    else if(pDev->ch == I2C_CH2)
    {
        
        //printf("i2c_pin_init:ch2\n");
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOB, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOB, &GPIO_InitStructure);
    }

}

/*********************************************************************************************************
** Function name:           i2c_interrupt_init
** Descriptions:            i2c_interrupt_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
static int i2c_interrupt_init(i2c_device *pDev)
{

    NVIC_InitTypeDef NVIC_InitStructure;    
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    NVIC_InitStructure.NVIC_IRQChannel = pDev->irq_er;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;   //抢占式中断优先级0~3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;          //响应式中断优先级0~3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    NVIC_InitStructure.NVIC_IRQChannel = pDev->irq_ev;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;   //抢占式中断优先级0~3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;          //响应式中断优先级0~3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    I2C_ITConfig(pDev->I2Cx,I2C_IT_BUF|I2C_IT_EVT,ENABLE);
    I2C_ITConfig(pDev->I2Cx,I2C_IT_ERR,ENABLE);
    return 0;
}

/*********************************************************************************************************
** Function name:           i2c_mode_init
** Descriptions:            i2c_mode_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
static void i2c_mode_init(i2c_device *pDev)
{
    I2C_InitTypeDef I2C_InitStructure;

    I2C_DeInit(pDev->I2Cx);
    I2C_InitStructure.I2C_ClockSpeed = I2C_CLK_SPEED;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;//I2C_DutyCycle_2;//I2C_DutyCycle_16_9;
    I2C_InitStructure.I2C_OwnAddress1 = I2C_OWN_ADDRESS;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;

    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; 
    
    I2C_Init(pDev->I2Cx,&I2C_InitStructure);
    I2C_Cmd(pDev->I2Cx,ENABLE);


}

static void i2c_onReadHandle(u8 *pData)
{
    u8 id = pData[0] >> 1;
    switch(id)
    {
    case (u8)POWER_STATUS:
        g_pI2C_dev->tx_buf[0] = pstm_i2c_tx_buf->u8PowerStatus;
        break;
    case (u8)SELF_CHECK:
        g_pI2C_dev->tx_buf[0] = pstm_i2c_tx_buf->u8SelfCheck;
        break;
    case (u8)ECG_PROBE_RESULT:
        g_pI2C_dev->tx_buf[0] = pstm_i2c_tx_buf->u8ecgReslut;
        break;
    case (u8)CURRENT_PRESSURE:
        g_pI2C_dev->tx_buf[0] = (g_SysADCResult.DET_NIBP >> 8) & 0xFF;
        g_pI2C_dev->tx_buf[1] = g_SysADCResult.DET_NIBP & 0xFF;
        break;
    case (u8)PROTECT_PRESSURE:
        g_pI2C_dev->tx_buf[0] = p_nibp_press->Index;
        g_pI2C_dev->tx_buf[1] = (p_nibp_press->u16PressADC_Max >> 8) & 0xFF;//MSB
        g_pI2C_dev->tx_buf[2] = p_nibp_press->u16PressADC_Max & 0xFF;       //LSB    
        break;
    case (u8)NIBP_PROTECT_STATE:
        g_pI2C_dev->tx_buf[0] = (u8)g_nibp_fast_rele_state;
        break;
    default:
        break;
    }
}

static void i2c_OnWriteHandle(u8 *pData)
{
    u8 id = pData[0] >> 1;
    if (0 == pData[1]) return;  //no data lenght
    
    switch(id)
    {
    case (u8)PROTECT_PRESSURE:
        NIBP_SetSafePress(&pData[2],3);
        break;
    case (u8)POWER_STATUS:
        break;
    case (u8)SELF_CHECK:
        break;
    case (u8)ECG_PROBE_RESULT:
        break;
    case (u8)SOFTWARE_UPDATE:
        enter_softwareUpdate(&pData[2]);
        break;
    case (u8)NIBP_VERIFY:
        NIBP_SetNIBPVerify(pData[2]);
        break;
    case (u8)NIBP_PROTECT_STATE:
        NIBP_SetNIBPFastReleState(pData[2]);
        break;
    default:
        break;
    }
}

/*********************************************************************************************************
** Function name:           I2C1_EV_IRQHandler
** Descriptions:            I2C1_EV_IRQHandler
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
void I2C1_EV_IRQHandler(void)
{
    i2c_device *pDev = g_pI2C_dev;
    I2C_TypeDef *I2Cx = pDev->I2Cx;
    u32 event = I2C_GetLastEvent(I2Cx);
    
    /* master */
    if((event & BIT_MASTER))
    {
        
        if((event & MASTER_START) == MASTER_START )
        {
            I2C_DBG_INFO("\r\nI2C1_EV_IRQHandler MASTER_START");
            I2C_AcknowledgeConfig(I2Cx, ENABLE);	
            I2C_SendData(I2Cx,pDev->addr);
        }
        if( (event & MASTER_ADDR) == MASTER_ADDR )
        {
            I2C_DBG_INFO("\r\nI2C1_EV_IRQHandler MASTER_RX_ADDR");
        }
        if( ((event & MASTER_RX_DATA) == MASTER_RX_DATA) )
        {
            if(pDev->sub_addr_len == 1)
            {
                pDev->sub_addr_len = 0;
                I2C_SendData(I2Cx,pDev->sub_addr);
            }
            else if(pDev->len)
            {
                pDev->rx_buf[pDev->rx_len] = I2C_ReceiveData(I2Cx);
                pDev->rx_len = (pDev->rx_len+1)%I2C_BUFFER_SIZE;   
                pDev->len--;
                
                
                
            }                
            else if(pDev->len == 0)
            {
                I2C_AcknowledgeConfig(I2Cx, DISABLE);	
                I2C_GenerateSTOP(I2Cx, ENABLE);
            }                    
        }
        if((event & MASTER_TX_DATA) == MASTER_TX_DATA)
        {
            if(pDev->sub_addr_len == 1)
            {
                pDev->sub_addr_len = 0;
                I2C_SendData(I2Cx,pDev->sub_addr);
            }
            else if(pDev->len)
            {
                I2C_SendData(I2Cx,pDev->tx_buf[pDev->tx_len]);
                //pDev->tx_len = (pDev->tx_len+1)%I2C_BUFFER_SIZE;//del by QWB
                pDev->tx_len = (pDev->tx_len+1)%STM32_I2C_TX_SIZE;//add by QWB
                pDev->len--;

            }                
            else if(pDev->len == 0)
            {
                if((I2Cx->SR1 & BIT_BTF))
                I2C_GenerateSTOP(I2Cx, ENABLE);
            }                    
        }
    }
    /* slave */
    if(!(event & BIT_MASTER))
    {
#if 0
        if(event & SLAVE_ADDR)
        {
            //I2C_DBG_INFO("I2C1_EV_IRQHandler Receive addr=0x%02X",g_pI2C2_dev->rx_buf[g_pI2C2_dev->rx_len]);
            pDev->rx_len = 0;
            pDev->tx_len = 0;
        }
        if(event & SLAVE_RX_DATA)
        {
            pDev->rx_buf[pDev->rx_len] = I2C_ReceiveData(I2Cx);
            pDev->rx_len = (pDev->rx_len+1)%I2C_BUFFER_SIZE;        
        }
        if((event & SLAVE_TX_DATA) == SLAVE_TX_DATA)
        {
            I2C_SendData(I2Cx,pDev->tx_buf[pDev->tx_len]);
            //pDev->tx_len = (pDev->tx_len+1)%I2C_BUFFER_SIZE;//del by QWB
            pDev->tx_len = (pDev->tx_len+1)%STM32_I2C_TX_SIZE;//add by QWB
        }
        if(event & SLAVE_STOP)
        {
            I2C_DBG_INFO("I2C1_EV_IRQHandler stop detected.");
            I2C_Cmd(I2Cx,ENABLE);
            pDev->done = 1;
        }
#else
        if(event & SLAVE_ADDR)
        {
            //I2C_DBG_INFO("I2C1_EV_IRQHandler Receive addr=0x%02X",g_pI2C2_dev->rx_buf[g_pI2C2_dev->rx_len]);
            pDev->rx_len = 0;
            pDev->tx_len = 0;
        }
        if(event & SLAVE_RX_DATA)
        {
            pDev->rx_buf[pDev->rx_len] = I2C_ReceiveData(I2Cx);
            pDev->rx_len = (pDev->rx_len+1)%I2C_BUFFER_SIZE;         
        }
        if((event & SLAVE_TX_DATA) == SLAVE_TX_DATA)
        {
            I2C_SendData(I2Cx,pDev->tx_buf[pDev->tx_len]);
            pDev->tx_len = (pDev->tx_len+1) % I2C_BUFFER_SIZE;
        }
        if(event & SLAVE_STOP)
        {
            I2C_DBG_INFO("I2C1_EV_IRQHandler stop detected.");
            I2C_Cmd(I2Cx,ENABLE);
            if (pDev->rx_len)
            {
                if (pDev->rx_buf[0] & AIO_STM32_READ_MASK) //read operation
                {
                    i2c_onReadHandle(pDev->rx_buf);
                }
                else //write operation
                {
                    i2c_OnWriteHandle(pDev->rx_buf);
                }
            }
            pDev->done = 1;
        }
#endif
    }
    
}

/*********************************************************************************************************
** Function name:           I2C1_ER_IRQHandler
** Descriptions:            I2C1_ER_IRQHandler
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
void I2C1_ER_IRQHandler(void)
{
    I2C_TypeDef *I2Cx = g_pI2C_dev->I2Cx;
    u32 event = I2C_GetLastEvent(I2Cx);

    I2C_ERR_INFO("I2C1_ER_IRQHandler event = 0x%.8X.",event);
    
    if(event & BIT_TIMEOUT)
    {
        I2C_ERR_INFO("I2C1_ER_IRQHandler BIT_TIMEOUT.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_TIMEOUT;
    }
    if(event & BIT_PERERR)
    {
        I2C_ERR_INFO("I2C1_ER_IRQHandler BIT_PERERR.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_PERERR;
    }
    if(event & BIT_OVR)
    {
        I2C_ERR_INFO("I2C1_ER_IRQHandler BIT_OVR.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_OVR;
    }
    if(event & BIT_AF)
    {
        I2C_ERR_INFO("I2C1_ER_IRQHandler BIT_AF.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_AF;
    }
    if(event & BIT_ARLO)
    {
        I2C_ERR_INFO("I2C1_ER_IRQHandler BIT_ARLO.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_ARLO;
    }
    if(event & BIT_BERR)
    {
        I2C_ERR_INFO("I2C1_ER_IRQHandler BIT_BERR.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_BERR;
    }
}

/*********************************************************************************************************
** Function name:           I2C2_EV_IRQHandler
** Descriptions:            I2C2_EV_IRQHandler
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
void I2C2_EV_IRQHandler(void)
{    
    i2c_device *pDev = g_pI2C2_dev;
    I2C_TypeDef *I2Cx = pDev->I2Cx;
    u32 event = I2C_GetLastEvent(I2Cx);
    //I2C_DBG_INFO("\r\nI2C2_EV_IRQHandler event=0x%.8X",event);
    
    /* master */
    if((event & BIT_MASTER))
    {
        
        if((event & MASTER_START) == MASTER_START )
        {
            //I2C_DBG_INFO("\r\nI2C2_EV_IRQHandler MASTER_START");
            I2C_AcknowledgeConfig(I2Cx, ENABLE);	
            I2C_SendData(I2Cx,pDev->addr);
        }
        if( (event & MASTER_ADDR) == MASTER_ADDR )
        {
            //I2C_DBG_INFO("\r\nI2C2_EV_IRQHandler MASTER_RX_ADDR");
        }
        if( ((event & MASTER_RX_DATA) == MASTER_RX_DATA) )
        {
            //I2C_DBG_INFO("\r\nI2C2_EV_IRQHandler MASTER_RX_DATA");
            if(pDev->sub_addr_len == 1)
            {
                pDev->sub_addr_len = 0;
                I2C_SendData(I2Cx,pDev->sub_addr);
            }
            else if(pDev->len)
            {
                pDev->rx_buf[pDev->rx_len] = I2C_ReceiveData(I2Cx);
                pDev->rx_len = (pDev->rx_len+1)%I2C_BUFFER_SIZE;   
                pDev->len--;
                
                
                
            }                
            else if(pDev->len == 0)
            {
                I2C_AcknowledgeConfig(I2Cx, DISABLE);	
                I2C_GenerateSTOP(I2Cx, ENABLE);
            }                    
        }
        if((event & MASTER_TX_DATA) == MASTER_TX_DATA)
        {
            if(pDev->sub_addr_len == 1)
            {
                pDev->sub_addr_len = 0;
                I2C_SendData(I2Cx,pDev->sub_addr);
            }
            else if(pDev->len)
            {
                I2C_SendData(I2Cx,pDev->tx_buf[pDev->tx_len]);
                pDev->tx_len = (pDev->tx_len+1)%I2C_BUFFER_SIZE; 
                pDev->len--;

            }                
            else if(pDev->len == 0)
            {
                if((I2Cx->SR1 & BIT_BTF))
                I2C_GenerateSTOP(I2Cx, ENABLE);
            }                    
        }
    }
    /* slave */
    if(!(event & BIT_MASTER))
    {
        if(event & SLAVE_ADDR)
        {
            //I2C_DBG_INFO("I2C2_EV_IRQHandler Receive addr=0x%02X",g_pI2C2_dev->rx_buf[g_pI2C2_dev->rx_len]);
            pDev->rx_len = 0;
            pDev->tx_len = 0;
        }
        if(event & SLAVE_RX_DATA)
        {
            pDev->rx_buf[pDev->rx_len] = I2C_ReceiveData(I2Cx);
            pDev->rx_len = (pDev->rx_len+1)%I2C_BUFFER_SIZE;        
        }
        if((event & SLAVE_TX_DATA) == SLAVE_TX_DATA)
        {
            I2C_SendData(I2Cx,pDev->tx_buf[pDev->tx_len]);
            pDev->tx_len = (pDev->tx_len+1)%I2C_BUFFER_SIZE;  
        }
        if(event & SLAVE_STOP)
        {
            I2C_DBG_INFO("I2C2_EV_IRQHandler stop detected.");
            I2C_Cmd(I2Cx,ENABLE);
            pDev->done = 1;
        }
    }
    
}

/*********************************************************************************************************
** Function name:           I2C2_ER_IRQHandler
** Descriptions:            I2C2_ER_IRQHandler
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
void I2C2_ER_IRQHandler(void)
{
    I2C_TypeDef *I2Cx = g_pI2C2_dev->I2Cx;
    u32 event = I2C_GetLastEvent(I2Cx);

    I2C_ERR_INFO("I2C2_ER_IRQHandler event = 0x%.8X.",event);
    
    if(event & BIT_TIMEOUT)
    {
        I2C_ERR_INFO("I2C2_ER_IRQHandler BIT_TIMEOUT.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_TIMEOUT;
    }
    if(event & BIT_PERERR)
    {
        I2C_ERR_INFO("I2C2_ER_IRQHandler BIT_PERERR.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_PERERR;
    }
    if(event & BIT_OVR)
    {
        I2C_ERR_INFO("I2C2_ER_IRQHandler BIT_OVR.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_OVR;
    }
    if(event & BIT_AF)
    {
        I2C_ERR_INFO("I2C2_ER_IRQHandler BIT_AF.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_AF;
    }
    if(event & BIT_ARLO)
    {
        I2C_ERR_INFO("I2C2_ER_IRQHandler BIT_ARLO.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_ARLO;
    }
    if(event & BIT_BERR)
    {
        I2C_ERR_INFO("I2C2_ER_IRQHandler BIT_BERR.");
        I2Cx->SR1 = I2Cx->SR1 & ~BIT_BERR;
    }
}

/*********************************************************************************************************
** Function name:           i2c_hw_init
** Descriptions:            i2c_hw_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int i2c_hw_init(i2c_device *pDev,u8 ch)
{
    i2c_device_init(pDev,ch);
    i2c_clk_init(pDev);
    i2c_pin_init(pDev);
    i2c_mode_init(pDev);
    i2c_interrupt_init(pDev);
    return 0;
}

/*********************************************************************************************************
** Function name:           i2c_init
** Descriptions:            i2c_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int i2c_init(void)
{
    i2c_rx_point_cur = i2c_rx_point_handle = 0;
    memset(pstm_i2c_tx_buf, 0, sizeof(I2C_TxBufTypeDef));
    
    i2c_hw_init(g_pI2C_dev,I2C_CH1);
    //i2c_hw_init(g_pI2C2_dev,I2C_CH2);
    
    return 0;
}

/*********************************************************************************************************
** Function name:           i2c_check_event
** Descriptions:            i2c_check_event
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int i2c_check_event(I2C_TypeDef* I2Cx,uint32_t I2C_EVENT)
{
    int timeout = 10000;
    
    while( (!I2C_CheckEvent(I2Cx, I2C_EVENT)) && (timeout--))
    {
        delay_us(1);
    }

    if(timeout>1)
    {
        return 0;
    }
    
    return 1;
}
  
/*********************************************************************************************************
** Function name:           i2c_wait
** Descriptions:            i2c_wait
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int i2c_wait(i2c_device *pDev)
{
    int timeout = 10000;
    while((pDev->len) && (timeout--))
    {
        delay_us(10);
    }

    if(timeout>1)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*********************************************************************************************************
** Function name:           i2c_read_byte
** Descriptions:            i2c_read_byte
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
u8 i2c_read_byte(i2c_device *pDev,u8 dev_addr,u16 sub_addr)
{
    I2C_TypeDef *I2Cx = g_pI2C_dev->I2Cx;
    
    int ret = 0;
    
    I2C_GenerateSTART(I2Cx, ENABLE);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT) )
    {
        I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
        ret = -1;
    }
    
    I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Transmitter);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) )
    {
        I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
        ret = -2;
    }
    
    I2C_SendData(I2Cx, sub_addr);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED) )
    {
        I2C_DBG_INFO("%s\n", "i2c sub_addr transmit error!");
        ret = -3;
    }
    

    I2C_GenerateSTART(I2Cx, ENABLE);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT) )
    {
        I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
        ret = -4;
    }

    I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Receiver);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) )
    {
        I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
        ret = -5;
    }

    I2C_AcknowledgeConfig(I2Cx, DISABLE);	
    I2C_GenerateSTOP(I2Cx, ENABLE);	
    
    
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) )
    {
        I2C_DBG_INFO("%s\n", "i2c data master receive error!");
        ret = -6;
    }
    ret = I2C_ReceiveData(I2Cx);

    I2C_AcknowledgeConfig(I2Cx, ENABLE);
    
    return ret;

}

/*********************************************************************************************************
** Function name:           i2c_read_buf
** Descriptions:            i2c_read_buf
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int i2c_read_buf(i2c_device *pDev,u8 dev_addr,u16 sub_addr,u8 sub_addr_len,u8 *buf,u8 len)
{
    I2C_TypeDef *I2Cx = pDev->I2Cx;
    
    int ret = 0;
    
    pDev->addr = dev_addr|1;
    pDev->sub_addr = sub_addr;
    pDev->sub_addr_len = sub_addr_len;
    pDev->len = len;
    pDev->rx_buf = buf;
    pDev->rx_len = 0;
    
    I2C_GenerateSTART(I2Cx, ENABLE);
    i2c_wait(pDev);

    #if 0
    I2C_ITConfig(I2Cx,I2C_IT_BUF|I2C_IT_EVT,DISABLE);
    if(sub_addr_len == 1)
    {
        I2C_GenerateSTART(I2Cx, ENABLE);
        if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT) )
        {
            I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
            ret = -1;
        }
        
        I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Transmitter);
        if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) )
        {
            I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
            ret = -2;
        }
        
        I2C_SendData(I2Cx, sub_addr);
        if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED) )
        {
            I2C_DBG_INFO("%s\n", "i2c sub_addr transmit error!");
            ret = -3;
        }
    }

    I2C_GenerateSTART(I2Cx, ENABLE);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT) )
    {
        I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
        ret = -4;
    }

    I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Receiver);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) )
    {
        I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
        ret = -5;
    }

    while(len)
    {
        if(len == 1)
        {
            I2C_AcknowledgeConfig(I2Cx, DISABLE);	
            I2C_GenerateSTOP(I2Cx, ENABLE);	
            
        }
        if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) )
        {
            I2C_DBG_INFO("%s\n", "i2c data master receive error!");
            ret = -3;
        }
        *buf++ = I2C_ReceiveData(I2Cx);
        len--;
    }
        
    I2C_AcknowledgeConfig(I2Cx, ENABLE);
    
    I2C_ITConfig(I2Cx,I2C_IT_BUF|I2C_IT_EVT,ENABLE);

    #endif
    return ret;

}

/*********************************************************************************************************
** Function name:           i2c_write_byte
** Descriptions:            i2c_write_byte
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int i2c_write_byte(i2c_device *pDev,u8 dev_addr,u16 sub_addr,u8 data)
{
    I2C_TypeDef *I2Cx = pDev->I2Cx;

    int ret = 0;
    
    I2C_GenerateSTART(I2Cx, ENABLE);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT) )
    {
        I2C_DBG_INFO("%s\n", "i2c start transmit error!");
        ret = -1;
    }
    
    I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Transmitter);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) )
    {
        I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
        ret = -1;
    }
    
    I2C_SendData(I2Cx, sub_addr);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED) )
    {
        I2C_DBG_INFO("%s\n", "i2c sub_addr transmit error!");
        ret = -2;
    }
    
    I2C_SendData(I2Cx, data);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED) )
    {
        I2C_DBG_INFO("%s\n", "i2c data transmit error!");
        ret = -3;
    }
    
    I2C_GenerateSTOP(I2Cx, ENABLE);
    
    return ret;
}

/*********************************************************************************************************
** Function name:           i2c_write_buf
** Descriptions:            i2c_write_buf
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int i2c_write_buf(i2c_device *pDev,u8 dev_addr,u16 sub_addr,u16 sub_addr_len,u8 *buf,u8 len)
{    
    I2C_TypeDef *I2Cx = pDev->I2Cx;
    
    int ret = 0;

    pDev->addr = dev_addr;
    pDev->sub_addr = sub_addr;
    pDev->sub_addr_len = sub_addr_len;
    pDev->len = len;
    pDev->tx_buf = buf;
    pDev->tx_len = 0;
    
    I2C_GenerateSTART(I2Cx, ENABLE);
    i2c_wait(pDev);

#if 0    
    int i = 0;
    
    I2C_ITConfig(I2Cx,I2C_IT_BUF|I2C_IT_EVT,DISABLE);
    I2C_GenerateSTART(I2Cx, ENABLE);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT) )
    {
        I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
        ret = -1;
    }
    
    I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Transmitter);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) )
    {
        I2C_DBG_INFO("%s\n", "i2c dev_addr transmit error!");
        ret = -1;
    }
    
    I2C_SendData(I2Cx, sub_addr);
    if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED) )
    {
        I2C_DBG_INFO("%s\n", "i2c sub_addr transmit error!");
        ret = -2;
    }
    
    for(i=0; i<len; i++)
    {
        I2C_SendData(I2Cx, buf[i]);
        if(i2c_check_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED) )
        {
            I2C_DBG_INFO("%s\n", "i2c buf transmit error!");
            ret = -3;
       }
    }
    
    I2C_GenerateSTOP(I2Cx, ENABLE);
    
    I2C_ITConfig(I2Cx,I2C_IT_BUF|I2C_IT_EVT,ENABLE);

#endif
    return ret;
    
}

/*********************************************************************************************************
** Function name:           i2c_server
** Descriptions:            i2c_server
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void i2c_server(void)
{
    static u32 u32UpdateTime = 0;    
    
    #if 0
    static u32 u32NextChangeTime = 0;    
    u8 tx_buf[16] = {0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    u8 rx_buf[16];
    
    if(IsOnTime(u32NextChangeTime))                                     /* is on time or over time */
    {
        u32NextChangeTime += 1000;
        
        //memcpy(g_pI2C_dev->tx_buf,tx_buf,6);        
        //i2c_write_buf(g_pI2C_dev,I2C_EEPROM_ADDRESS,0,1,tx_buf,6);
        i2c_read_buf(g_pI2C_dev,I2C_EEPROM_ADDRESS,0,1,rx_buf,6);
        UART_SendBuf(rx_buf,6);

    }
    #endif
    
    if(IsOnTime(u32UpdateTime))                                         /* is on time or over time */
    {
        u32UpdateTime += 10;
#if 0 //del by QWB
        g_pI2C_dev->tx_buf[0] = g_power_status;//0xaa;//g_power_status;
        g_pI2C_dev->tx_buf[1] = Sys_Self_Check_Status;//0xbb;//Sys_Self_Check_Status;
#endif
        pstm_i2c_tx_buf->u8PowerStatus = g_power_status;//0xaa;
        pstm_i2c_tx_buf->u8SelfCheck = Sys_Self_Check_Status;//0xbb;
        pstm_i2c_tx_buf->u8ecgReslut = g_u8ecg_status;//0xcc;
    }
#if 0 //del by QWB
    if(g_pI2C_dev->done)
    {
        g_pI2C_dev->done  = 0;
        
        UART_SendBuf(g_pI2C_dev->rx_buf,g_pI2C_dev->rx_len);
        g_pI2C_dev->rx_len = 0;
        
    }
#endif
}


#endif /* __I2C_INT_C__ */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
