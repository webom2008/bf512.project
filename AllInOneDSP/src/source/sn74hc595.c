/*
 * sn74hc595.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include "sn74hc595.h"

//#define _SN74HC595_INFO_

#ifdef _SN74HC595_INFO_
#define DBG595_INFO(fmt, arg...) AIO_printf("\r\n[595] "fmt, ##arg)
#else
#define DBG595_INFO(fmt, arg...)
#endif

#define HCT595_SER_PIN      PG5
#define HCT595_SRCLK_PIN    PG11
#define HCT595_RCLK_PIN     PG7

#define HCT595_SER_LOW()    (*pPORTGIO_CLEAR = HCT595_SER_PIN)
#define HCT595_SER_HIGH()   (*pPORTGIO_SET = HCT595_SER_PIN)
#define HCT595_SRCLK_LOW()  (*pPORTGIO_CLEAR = HCT595_SRCLK_PIN)
#define HCT595_SRCLK_HIGH() (*pPORTGIO_SET = HCT595_SRCLK_PIN)
#define HCT595_RCLK_LOW()   (*pPORTGIO_CLEAR = HCT595_RCLK_PIN)
#define HCT595_RCLK_HIGH()  (*pPORTGIO_SET = HCT595_RCLK_PIN)

#define HCT595_IBP_ZERO_MASK        0x01
#define HCT595_TEMP_SELE_MASK       0x06
#define HCT595_ADC_CHANNEL_MASK     0x18

unsigned char g_HCT595Value;

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Statt -->
#pragma section("sdram0_bank3")
void AHCT595_Init(void);
#pragma section("sdram0_bank3")
void AHCT595_IO_Init(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

/**
  * @brief  This function initializes the 595 pin.(PG5/11/7)
  * @param  void
  * @retval void
  */
static void AHCT595_IO_Init(void)
{
    /*setup PF9 and PF10 as an output*/
	*pPORTGIO_INEN &= ~(HCT595_SER_PIN | HCT595_SRCLK_PIN | HCT595_RCLK_PIN);			/* input buffer disable */
	*pPORTGIO_DIR |= (HCT595_SER_PIN | HCT595_SRCLK_PIN | HCT595_RCLK_PIN);			/* output */

	/* clear interrupt settings */
	*pPORTGIO_EDGE &= ~(HCT595_SER_PIN | HCT595_SRCLK_PIN | HCT595_RCLK_PIN);         // Level Sensitivity
    *pPORTGIO_MASKA_CLEAR = (HCT595_SER_PIN | HCT595_SRCLK_PIN | HCT595_RCLK_PIN);

    /* now clear the flag */
	HCT595_SER_LOW();
    HCT595_SRCLK_LOW();
    HCT595_RCLK_LOW();
}

/**
  * @brief  Refresh 595 output data
  *         Note: all pin change when configure because of /OE = LOW
  * @param  void
  * @retval void
  */
static void AHCT595_Refresh(void)
{
    int i;
    
    HCT595_RCLK_LOW();
    //MSB First, LSB Last
    for (i=7; i >= 0; i--)
    {
        HCT595_SRCLK_LOW();
        if ((g_HCT595Value >> i) & 0x01)
        {
            HCT595_SER_HIGH();
        }
        else
        {
            HCT595_SER_LOW();
        }
        HCT595_SRCLK_HIGH();//上升沿，移位数据
        //delay();
    }
    HCT595_RCLK_HIGH();//上升沿，更新输出数据
}

void AHCT595_SetComADCChannel(COMADC_Channel_TypeDef channel)
{
    g_HCT595Value &= (unsigned char)~HCT595_ADC_CHANNEL_MASK;
    g_HCT595Value |= (unsigned char)(channel << 3) & (unsigned char)HCT595_ADC_CHANNEL_MASK;
    
    AHCT595_Refresh();
    //g_COMADC_channel = channel;
}

void AHCT595_SetTempChannel(TEMP_Channel_TypeDef channel)
{
    g_HCT595Value &= (unsigned char)~HCT595_TEMP_SELE_MASK;
    g_HCT595Value |= (unsigned char)(channel << 1) & (unsigned char)HCT595_TEMP_SELE_MASK;

    AHCT595_Refresh();
}

void AHCT595_SetIBPzero(EnableOrDisable status)
{
    if (ENABLE == status)
    {
        g_HCT595Value |= (unsigned char)HCT595_IBP_ZERO_MASK;

    }
    else
    {
        g_HCT595Value &= (unsigned char)~HCT595_IBP_ZERO_MASK;
    }
    AHCT595_Refresh();
}

void AHCT595_Init(void)
{
    g_HCT595Value = 0;
    
    AHCT595_IO_Init();
}



