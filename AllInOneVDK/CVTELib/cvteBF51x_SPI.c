/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : cvteBF51x_SPI.c
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/2/11
  Last Modified :
  Description   : spi libary
  Function List :
  History       :
  1.Date        : 2015/2/11
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/
#include "cvteBF51x_SPI.h"

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/
extern int udprintf(const char* fmt, ...);

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/


/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal variables                           *
 *----------------------------------------------*/


/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/
static void delay_us(u32 usec)
{
    u32 i = 0;
    u32 j = 0;
    for(i=0;i<usec;i++)
    {
        for(j=0;j<1;j++) asm("nop;");
    }
}

static void SPI_IO_Init(SPI_TypeDef* SPIx)
{
//    udprintf("\r\n>>SPI_IO_Init SPI");
    if (SPIx == SPI0)
    {
        gpio_init(GPIOG,GPIO_Pin_12,Function_First,GPIO_Mode_Out);
        gpio_init(GPIOG,GPIO_Pin_13,Function_First,GPIO_Mode_Out);
        gpio_init(GPIOG,GPIO_Pin_14,Function_First,GPIO_Mode_Out);
        gpio_init(GPIOG,GPIO_Pin_15,Function_First,GPIO_Mode_Out);
//        udprintf("0");
    }
    else if (SPIx == SPI1)
    {
        gpio_init(GPIOH,GPIO_Pin_0,Function_Second,GPIO_Mode_Out);
        gpio_init(GPIOH,GPIO_Pin_1,Function_Second,GPIO_Mode_Out);
        gpio_init(GPIOH,GPIO_Pin_2,Function_Second,GPIO_Mode_Out);
        gpio_init(GPIOH,GPIO_Pin_3,Function_Second,GPIO_Mode_Out);
//        udprintf("1");
    }
}

void SPI_SSEL_SW_Init(SPI_TypeDef *SPIx, u16 ssel)
{
    if (SPI0 == SPIx)
    {
        switch(ssel)
        {
        case SPI_SSEL1:
            gpio_init(GPIOF,GPIO_Pin_7,Function_GPIO,GPIO_Mode_Out);
            break;
        case SPI_SSEL2:
            gpio_init(GPIOG,GPIO_Pin_15,Function_GPIO,GPIO_Mode_Out);
            break;
        case SPI_SSEL3:
            gpio_init(GPIOH,GPIO_Pin_4,Function_GPIO,GPIO_Mode_Out);
            break;
        case SPI_SSEL4:
            break;
        case SPI_SSEL5:
            gpio_init(GPIOG,GPIO_Pin_3,Function_GPIO,GPIO_Mode_Out);
            break;
        default:
            break;
        }
        
    }
    else if (SPI1 == SPIx)
    {
        switch(ssel)
        {
        case SPI_SSEL1:
            gpio_init(GPIOH,GPIO_Pin_6,Function_GPIO,GPIO_Mode_Out);
            break;
        case SPI_SSEL2:
            gpio_init(GPIOF,GPIO_Pin_0,Function_GPIO,GPIO_Mode_Out);
            break;
        case SPI_SSEL3:
            gpio_init(GPIOG,GPIO_Pin_0,Function_GPIO,GPIO_Mode_Out);
            break;
        case SPI_SSEL4:
            gpio_init(GPIOF,GPIO_Pin_8,Function_GPIO,GPIO_Mode_Out);
            break;
        case SPI_SSEL5:
            gpio_init(GPIOG,GPIO_Pin_11,Function_GPIO,GPIO_Mode_Out);
            break;
        default:
            break;
        }
    }
}

void SPI_SSEL_HW_Init(SPI_TypeDef *SPIx, u16 ssel)
{
    if (SPI0 == SPIx)
    {
        switch(ssel)
        {
        case SPI_SSEL1:
            gpio_init(GPIOF,GPIO_Pin_7,Function_First,GPIO_Mode_Out);
            break;
        case SPI_SSEL2:
            gpio_init(GPIOG,GPIO_Pin_15,Function_First,GPIO_Mode_Out);
            break;
        case SPI_SSEL3:
            gpio_init(GPIOH,GPIO_Pin_4,Function_Third,GPIO_Mode_Out);
            break;
        case SPI_SSEL4:
            break;
        case SPI_SSEL5:
            gpio_init(GPIOG,GPIO_Pin_3,Function_Third,GPIO_Mode_Out);
            break;
        default:
            break;
        }
        
    }
    else if (SPI1 == SPIx)
    {
        switch(ssel)
        {
        case SPI_SSEL1:
            gpio_init(GPIOH,GPIO_Pin_6,Function_Third,GPIO_Mode_Out);
            break;
        case SPI_SSEL2:
            gpio_init(GPIOF,GPIO_Pin_0,Function_Third,GPIO_Mode_Out);
            break;
        case SPI_SSEL3:
            gpio_init(GPIOG,GPIO_Pin_0,Function_Third,GPIO_Mode_Out);
            break;
        case SPI_SSEL4:
            gpio_init(GPIOF,GPIO_Pin_8,Function_Third,GPIO_Mode_Out);
            break;
        case SPI_SSEL5:
            gpio_init(GPIOG,GPIO_Pin_11,Function_Third,GPIO_Mode_Out);
            break;
        default:
            break;
        }
    }
}

static void SPI_SSEL_IO_Init(spi_device* pDev)
{
//    udprintf("\r\n>>SPI_SSEL_IO_Init");
    if (SPI_NSS_Soft == pDev->nss)
    {
        SPI_SSEL_SW_Init(pDev->SPIx, pDev->ssel);
    }
    else if (SPI_NSS_Hard == pDev->nss)
    {
        SPI_SSEL_HW_Init(pDev->SPIx, pDev->ssel);
    }
}

static void SPI_SSEL_HIGH(spi_device* pDev)
{
    if (SPI_NSS_Hard == pDev->nss) return;
    
    if (SPI0 == pDev->SPIx)
    {
        switch(pDev->ssel)
        {
        case SPI_SSEL1:
            gpio_set(GPIOF,GPIO_Pin_7);
            break;
        case SPI_SSEL2:
            gpio_set(GPIOG,GPIO_Pin_15);
            break;
        case SPI_SSEL3:
            gpio_set(GPIOH,GPIO_Pin_4);
            break;
        case SPI_SSEL4:
            break;
        case SPI_SSEL5:
            gpio_set(GPIOG,GPIO_Pin_3);
            break;
        default:
            break;
        }
    }
    else if (SPI1 == pDev->SPIx)
    {
        switch(pDev->ssel)
        {
        case SPI_SSEL1:
            gpio_set(GPIOH,GPIO_Pin_6);
            break;
        case SPI_SSEL2:
            gpio_set(GPIOF,GPIO_Pin_0);
            break;
        case SPI_SSEL3:
            gpio_set(GPIOG,GPIO_Pin_0);
            break;
        case SPI_SSEL4:
            gpio_set(GPIOF,GPIO_Pin_8);
            break;
        case SPI_SSEL5:
            gpio_set(GPIOG,GPIO_Pin_11);
            break;
        default:
            break;
        }
    }
}

static void SPI_SSEL_LOW(spi_device* pDev)
{
    if (SPI_NSS_Hard == pDev->nss) return;
    
    if (SPI0 == pDev->SPIx)
    {
        switch(pDev->ssel)
        {
        case SPI_SSEL1:
            gpio_clear(GPIOF,GPIO_Pin_7);
            break;
        case SPI_SSEL2:
            gpio_clear(GPIOG,GPIO_Pin_15);
            break;
        case SPI_SSEL3:
            gpio_clear(GPIOH,GPIO_Pin_4);
            break;
        case SPI_SSEL4:
            break;
        case SPI_SSEL5:
            gpio_clear(GPIOG,GPIO_Pin_3);
            break;
        default:
            break;
        }
    }
    else if (SPI1 == pDev->SPIx)
    {
        switch(pDev->ssel)
        {
        case SPI_SSEL1:
            gpio_clear(GPIOH,GPIO_Pin_6);
            break;
        case SPI_SSEL2:
            gpio_clear(GPIOF,GPIO_Pin_0);
            break;
        case SPI_SSEL3:
            gpio_clear(GPIOG,GPIO_Pin_0);
            break;
        case SPI_SSEL4:
            gpio_clear(GPIOF,GPIO_Pin_8);
            break;
        case SPI_SSEL5:
            gpio_clear(GPIOG,GPIO_Pin_11);
            break;
        default:
            break;
        }
    }
}

static void SPI_Mode_Init(SPI_TypeDef* SPIx, SPI_InitTypeDef* SPI_InitStruct)
{
//    udprintf("\r\n>>SPI_Mode_Init");
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
    
    SPIx->BAUD = SPI_InitStruct->SCLK/(2 * SPI_InitStruct->SPI_BaudRate);
    SPIx->FLG |= (0x0001 << SPI_InitStruct->SPI_SSEL);
    SPIx->CTL |= SPI_CTL_SPE;
}

static void SPI_Cmd(SPI_TypeDef* SPIx, u8 NewState)
{
//    udprintf("\r\n>>SPI_Cmd");
    if(NewState)
    {
        SPIx->CTL |= SPI_CTL_SPE;
    }
    else
    {
        SPIx->CTL &= ~SPI_CTL_SPE;
    }
}

int SPI_Init(spi_device* pDev, SPI_InitTypeDef* pSPI_InitStruct)
{
    SPI_IO_Init(pDev->SPIx);
    SPI_SSEL_IO_Init(pDev);
    SPI_Mode_Init(pDev->SPIx, pSPI_InitStruct);
    SPI_Cmd(pDev->SPIx, 1);
    return 0;
}

int SPI_write_byte(spi_device* pDev, u8 data)
{
    SPI_TypeDef *SPIx = pDev->SPIx;
    u8 ret = 0;
    
    SPI_SSEL_LOW(pDev);

    while(SPIx->STAT & SPI_STAT_TXS);
    SPIx->TDBR = data;
    
    while(!(SPIx->STAT & SPI_STAT_RXS));
    ret = SPIx->RDBR;
    
    delay_us(pDev->delay);
    SPI_SSEL_HIGH(pDev);
    
    return ret;
}

int SPI_write_buf(spi_device* pDev, u8 *cmd,u8 cmd_len,u8 *buf,u16 len)
{
    SPI_TypeDef *SPIx = pDev->SPIx;
    u8 temp = 0;
    int i;
    
    SPI_SSEL_LOW(pDev);
    
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
    delay_us(pDev->delay);
    SPI_SSEL_HIGH(pDev);
    
    return 0;
}

u8 SPI_read_byte(spi_device* pDev)
{
    SPI_TypeDef *SPIx = pDev->SPIx;
    u8 ret = 0;
    
    SPI_SSEL_LOW(pDev);
    
    while(SPIx->STAT & SPI_STAT_TXS);
    SPIx->TDBR = 0x00;//0xFF;
    
    while(!(SPIx->STAT & SPI_STAT_RXS));
    ret = SPIx->RDBR;
    
    delay_us(pDev->delay);
    SPI_SSEL_HIGH(pDev);
    
    return ret;
}

int SPI_read_buf(spi_device* pDev, u8 *cmd,u16 cmd_len,u8 *buf,u16 len)
{
    SPI_TypeDef *SPIx = pDev->SPIx;
    u8 ret = 0;
    int i;
    
//    udprintf("\r\n>>SPI_read_buf");
    SPI_SSEL_LOW(pDev);
//    udprintf("1");
    
    for(i=0; i<cmd_len; i++)
    {
        while(SPIx->STAT & SPI_STAT_TXS);
        SPIx->TDBR = cmd[i];

        while(!(SPIx->STAT & SPI_STAT_RXS));
        ret = SPIx->RDBR;
    }
//    udprintf("2");
    
    for(i=0; i<len; i++)
    {
        while(SPIx->STAT & SPI_STAT_TXS);
        SPIx->TDBR = 0x00;//0xFF;
        
        while(!(SPIx->STAT & SPI_STAT_RXS));
        buf[i] = SPIx->RDBR;
    }
//    udprintf("3");
        
    delay_us(pDev->delay);
    SPI_SSEL_HIGH(pDev);

//    udprintf("4");
    return 0;
}

void SPI_Cfg_prinf(spi_device* pDev)
{
    if (SPI0 == pDev->SPIx)
    {
        udprintf("\r\n>SPI0");
    }
    else if (SPI1 == pDev->SPIx)
    {
        udprintf("\r\n>SPI1");
    }
    udprintf("\r\n>CTL: 0x%04x",pDev->SPIx->CTL);
    udprintf("\r\n>FLG: 0x%04x",pDev->SPIx->FLG);
    udprintf("\r\n>BAUD:0x%04x",pDev->SPIx->BAUD);
}
