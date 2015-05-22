/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           system_spi.c
** Last modified Date:  2013-12-02
** Last Version:        V1.0
** Descriptions:        system_spi
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-12-02
** Version:             V1.00
** Descriptions:        system_spi
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-12-02
** Version:             V1.00
** Descriptions:        system_spi
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
#include "system_spi.h"

#ifdef __SYSTEM_SPI_C_

//#define SYSTEM_SPI_DEBUG_INFO

#ifdef SYSTEM_SPI_DEBUG_INFO
#define SYSTEM_SPI_DBG_INFO(fmt, arg...) AIO_printf("\r\n[SYSTEM_SPI] "fmt, ##arg)
#else
#define SYSTEM_SPI_DBG_INFO(fmt, arg...)
#endif


#define SPI_SCLK_PORT       GPIOG
#define SPI_MISO_PORT       GPIOG
#define SPI_MOSI_PORT       GPIOG
#define SPI_SEL2_PORT       GPIOG

#define SPI_SCLK_PIN        GPIO_Pin_12
#define SPI_MISO_PIN        GPIO_Pin_13
#define SPI_MOSI_PIN        GPIO_Pin_14
#define SPI_SEL2_PIN        GPIO_Pin_15

#define SPI_SPEED           (33000000)

#define SPI_CH0             (0)
#define SPI_CH1             (1)


#pragma section("sdram0_bank3")
spi_device g_spi_dev;
spi_device *pSpi_dev = &g_spi_dev;

#define SPI_BUFFER_SIZE     (128)

#pragma section("sdram0_bank3")
u8 spi_rx_buf[SPI_BUFFER_SIZE];
#pragma section("sdram0_bank3")
u8 spi_tx_buf[SPI_BUFFER_SIZE];

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
int system_spi_init(void);
#pragma section("sdram0_bank3")
void system_spi_hw_init(spi_device *pDev,u8 ch);
#pragma section("sdram0_bank3")
void system_spi_device_init(spi_device *pDev,u8 ch);
#pragma section("sdram0_bank3")
void system_spi_clk_init(spi_device *pDev);
#pragma section("sdram0_bank3")
void system_spi_pin_init(spi_device *pDev);
#pragma section("sdram0_bank3")
void system_spi_mode_init(spi_device *pDev);
#pragma section("sdram0_bank3")
void system_spi_interrupt_init(spi_device *pDev);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->


/*********************************************************************************************************
** Function name:           SPI_Init
** Descriptions:            SPI_Init
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void SPI_Init(SPI_TypeDef* SPIx, SPI_InitTypeDef* SPI_InitStruct)
{
    SPIx->CTL = SPI_CTL_EMISO | SPI_CTL_SZ | SPI_CTL_TIMOD_TD;
    
    if(SPI_InitStruct->SPI_Mode == SPI_Mode_Master)
    {
        SPIx->CTL |= SPI_CTL_MSTR;
    
    }
    else if(SPI_InitStruct->SPI_Mode == SPI_Mode_Slave)
    {
        SPIx->CTL &= ~SPI_CTL_MSTR;
    
    }
    if(SPI_InitStruct->SPI_CPOL == SPI_CPOL_Low)
    {
        SPIx->CTL |= SPI_CTL_CPOL;
    
    }
    else if(SPI_InitStruct->SPI_CPOL == SPI_CPOL_High)
    {
        SPIx->CTL &= ~SPI_CTL_CPOL;
    
    }
    if(SPI_InitStruct->SPI_CPHA == SPI_CPHA_1Edge)
    {
        SPIx->CTL |= SPI_CTL_CPHA;
    
    }
    else if(SPI_InitStruct->SPI_CPHA == SPI_CPHA_2Edge)
    {
        SPIx->CTL &= ~SPI_CTL_CPHA;
    
    }
    
    if(SPI_InitStruct->SPI_FirstBit == SPI_FirstBit_LSB)
    {
        SPIx->CTL |= SPI_CTL_LSBF;
    
    }
    else if(SPI_InitStruct->SPI_FirstBit == SPI_FirstBit_MSB)
    {
        SPIx->CTL &= ~SPI_CTL_LSBF;
    
    }
    
    if(SPI_InitStruct->SPI_DataSize == SPI_DataSize_16b)
    {
        SPIx->CTL |= SPI_CTL_SIZE;
    
    }
    else if(SPI_InitStruct->SPI_DataSize == SPI_DataSize_8b)
    {
        SPIx->CTL &= ~SPI_CTL_SIZE;
    
    }
    
    if(SPI_InitStruct->SPI_NSS == SPI_NSS_Hard)
    {
        SPIx->CTL |= SPI_CTL_PSSE;
    
    }
    else if(SPI_InitStruct->SPI_NSS == SPI_NSS_Soft)
    {
        SPIx->CTL &= ~SPI_CTL_PSSE;
    
    }
    
    SPIx->BAUD = (getSystemCLK()/2/SPI_InitStruct->SPI_BaudRate);
    SPIx->FLG |= (0x0001 << SPI_InitStruct->SPI_SSEL);
    SPIx->CTL |= SPI_CTL_SPE;

}

/**
  * @brief  Enables or disables the specified SPI peripheral.
  * @param  SPIx: where x can be 1, 2 or 3 to select the SPI peripheral.
  * @param  NewState: new state of the SPIx peripheral. 
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SPI_Cmd(SPI_TypeDef* SPIx, u8 NewState)
{
    if(NewState == ENABLE)
    {
        SPIx->CTL |= SPI_CTL_SPE;
    }
    else if(NewState == DISABLE)
    {
        SPIx->CTL &= ~SPI_CTL_SPE;
    
    }
}

/*********************************************************************************************************
** Function name:           SPI_Timod
** Descriptions:            SPI_Timod
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void SPI_Timod(SPI_TypeDef* SPIx, u8 mode)
{
    
    SPIx->CTL &= ~SPI_CTL_TIMOD;
    SPIx->CTL |= (mode & SPI_CTL_TIMOD);
}

/*********************************************************************************************************
** Function name:           SPI_SSel
** Descriptions:            SPI_SSel
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void SPI_SSel(SPI_TypeDef* SPIx, u8 Ssel,u8 NewState)
{
    if(NewState)
    {
        SPIx->FLG |= (1<<(Ssel+8));
    }
    else
    {
        SPIx->FLG &= ~(1<<(Ssel+8));
    }
}

/*********************************************************************************************************
** Function name:           system_spi_device_init
** Descriptions:            system_spi_device_init
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void system_spi_device_init(spi_device *pDev,u8 ch)
{
    if(ch == SPI_CH0)
    {
        SYSTEM_SPI_DBG_INFO("system_spi_device_init SPI_CH0");
        pDev->SPIx = SPI0;
        pDev->ch = SPI_CH0;
        pDev->ssel = SPI_SSEL2;
    }
    else if(ch == SPI_CH1)
    {
        pDev->SPIx = SPI1;
        pDev->ch = SPI_CH1;
        pDev->ssel = SPI_SSEL1;
    }
    
    pDev->rx_buf = spi_rx_buf;
    pDev->tx_buf = spi_tx_buf;
    pDev->rx_len = 0;
    pDev->tx_len = 0;
    
    
}

/*********************************************************************************************************
** Function name:           system_spi_clk_init
** Descriptions:            system_spi_clk_init
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void system_spi_clk_init(spi_device *pDev)
{
    return;    
}

/*********************************************************************************************************
** Function name:           system_spi_pin_init
** Descriptions:            system_spi_pin_init
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void system_spi_pin_init(spi_device *pDev)
{
    if(pDev->ch == SPI_CH0)
    {
        SYSTEM_SPI_DBG_INFO("system_spi_pin_init SPI_CH0");
        gpio_init(GPIOG,SPI_SCLK_PIN,Function_SPI0,GPIO_Mode_Out);
        gpio_init(GPIOG,SPI_MISO_PIN,Function_SPI0,GPIO_Mode_Out);
        gpio_init(GPIOG,SPI_MOSI_PIN,Function_SPI0,GPIO_Mode_Out);
        gpio_init(GPIOG,SPI_SEL2_PIN,Function_SPI0,GPIO_Mode_Out);
    }
    else if(pDev->ch == SPI_CH1)
    {
        gpio_init(GPIOH,GPIO_Pin_0,Function_SPI1,GPIO_Mode_Out);
        gpio_init(GPIOH,GPIO_Pin_1,Function_SPI1,GPIO_Mode_Out);
        gpio_init(GPIOH,GPIO_Pin_2,Function_SPI1,GPIO_Mode_Out);
        gpio_init(GPIOH,GPIO_Pin_3,Function_SPI1,GPIO_Mode_Out);
    }

}

/*********************************************************************************************************
** Function name:           system_spi_mode_init
** Descriptions:            system_spi_mode_init
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void system_spi_mode_init(spi_device *pDev)
{
    SPI_TypeDef *SPIx = pDev->SPIx;
    SPI_InitTypeDef SPI_InitStructure;
    
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;//SPI_CPOL_Low;//SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;//SPI_CPHA_2Edge;//SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_SSEL = SPI_SSEL2;
    
    SPI_InitStructure.SPI_BaudRate = SPI_SPEED;
    
    SPI_Init(SPIx, &SPI_InitStructure);

}

/*********************************************************************************************************
** Function name:           system_spi_interrupt_init
** Descriptions:            system_spi_interrupt_init
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void system_spi_interrupt_init(spi_device *pDev)
{
    SPI_TypeDef *SPIx = pDev->SPIx;
    SPI_Cmd(SPIx, ENABLE);
}

/*********************************************************************************************************
** Function name:           system_spi_hw_init
** Descriptions:            system_spi_hw_init
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void system_spi_hw_init(spi_device *pDev,u8 ch)
{
    system_spi_device_init(pDev,ch);
    system_spi_clk_init(pDev);
    system_spi_pin_init(pDev);
    system_spi_mode_init(pDev);
    system_spi_interrupt_init(pDev);

}

/*********************************************************************************************************
** Function name:           system_spi_init
** Descriptions:            system_spi_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_spi_init(void)
{
    system_spi_hw_init(pSpi_dev,SPI_CH0);
    
    return 0;
}

/*********************************************************************************************************
** Function name:           system_spi_reg_check
** Descriptions:            system_spi_reg_check
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_spi_reg_check(spi_device *pDev)
{
    SPI_TypeDef *SPIx = pDev->SPIx;
    
    SYSTEM_SPI_DBG_INFO("SPIx->CTL = %.4X", SPIx->CTL);
    SYSTEM_SPI_DBG_INFO("SPIx->FLG = %.4X", SPIx->FLG);
    SYSTEM_SPI_DBG_INFO("SPIx->STAT = %.4X", SPIx->STAT);
    //SYSTEM_SPI_DBG_INFO("SPIx->TDBR = %.4X", SPIx->TDBR);
    //SYSTEM_SPI_DBG_INFO("SPIx->RDBR = %.4X", SPIx->RDBR);
    SYSTEM_SPI_DBG_INFO("SPIx->BAUD = %.4X", SPIx->BAUD);
    SYSTEM_SPI_DBG_INFO("SPIx->SHADOW = %.4X", SPIx->SHADOW);
    
    return 0;    
}

/*********************************************************************************************************
** Function name:           system_spi_write_byte
** Descriptions:            system_spi_write_byte
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_spi_write_byte(u8 data)
{
    SPI_TypeDef *SPIx = pSpi_dev->SPIx;
    u8 ret = 0;
    
    SPI_SSel(SPIx, pSpi_dev->ssel,0);

        while(SPIx->STAT & SPI_STAT_TXS);
    SPIx->TDBR = data;
    
    while(!(SPIx->STAT & SPI_STAT_RXS));
    ret = SPIx->RDBR;
    
    SPI_SSel(SPIx, pSpi_dev->ssel,1);
    
    return ret;
}

/*********************************************************************************************************
** Function name:           system_spi_write_buf
** Descriptions:            system_spi_write_buf
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_spi_write_buf(u8 *cmd,u8 cmd_len,u8 *buf,u16 len)
{
    SPI_TypeDef *SPIx = pSpi_dev->SPIx;
    u8 temp = 0;
    int i;
        
    SPI_SSel(SPIx, pSpi_dev->ssel,0);
    for(i=0;i<cmd_len;i++)
    {
        while(SPIx->STAT & SPI_STAT_TXS);
        SPIx->TDBR = cmd[i];
    
        while(!(SPIx->STAT & SPI_STAT_RXS));
        temp = SPIx->RDBR;
        
    }
    
    for(i=0;i<len;i++)
    {
        while(SPIx->STAT & SPI_STAT_TXS);
        SPIx->TDBR = buf[i];
    
        while(!(SPIx->STAT & SPI_STAT_RXS));
        temp = SPIx->RDBR;
        
    }
    SPI_SSel(SPIx, pSpi_dev->ssel,1);
    
    return 0;
}

/*********************************************************************************************************
** Function name:           system_spi_read_byte
** Descriptions:            system_spi_read_byte
** input parameters:        none
** output parameters:       none
** Returned value:          0:receive data
*********************************************************************************************************/
u8 system_spi_read_byte(void)
{
    SPI_TypeDef *SPIx = pSpi_dev->SPIx;
    u8 ret = 0;
    
    SPI_SSel(SPIx, pSpi_dev->ssel,0);
    
    while(SPIx->STAT & SPI_STAT_TXS);
    SPIx->TDBR = 0xFF;
    
    while(!(SPIx->STAT & SPI_STAT_RXS));
    ret = SPIx->RDBR;
    
    SPI_SSel(SPIx, pSpi_dev->ssel,1);
    
    return ret;
}

/*********************************************************************************************************
** Function name:           system_spi_read_buf
** Descriptions:            system_spi_read_buf
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_spi_read_buf(u8 *cmd,u16 cmd_len,u8 *buf,u16 len)
{
    SPI_TypeDef *SPIx = pSpi_dev->SPIx;
    u8 ret = 0;
    int i;
    
    SPI_SSel(SPIx, pSpi_dev->ssel,0);
    
    for(i=0; i<cmd_len; i++)
    {
        while(SPIx->STAT & SPI_STAT_TXS);
        SPIx->TDBR = cmd[i];

        while(!(SPIx->STAT & SPI_STAT_RXS));
        ret = SPIx->RDBR;
        
    }
    
    for(i=0; i<len; i++)
    {
        while(SPIx->STAT & SPI_STAT_TXS);
        SPIx->TDBR = 0xFF;
        
        while(!(SPIx->STAT & SPI_STAT_RXS));
        buf[i] = SPIx->RDBR;
    }
        
    SPI_SSel(SPIx, pSpi_dev->ssel,1);

    return 0;
}

/*********************************************************************************************************
** Function name:           system_spi_server
** Descriptions:            system_spi_server
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_spi_server(void)
{
    static u32 u32NextChangeTime = 0;
        
    u8 rx_buf[16];
    u8 tx_buf[16] = {0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    u8 temp;
    
    if(IsOnTime(u32NextChangeTime))
    {
        u32NextChangeTime = SysTick_Get()+1000;
        
        //AIO_printf("system_spi_server.\n");
        
        //system_spi_write_buf(tx_buf,6);
        //system_spi_read_buf(tx_buf,3,rx_buf,6);
        //AIO_UART_SendBuf((char *)rx_buf, 6);
        
        //system_spi_reg_check(pSpi_dev);
    }
    
    
    return 0;
}

#endif //__SYSTEM_SPI_C_
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
