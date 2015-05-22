/*
 * system_clock.c
 *
 *  Created on: 2013-9-11
 *      Author: QiuWeibo
 */

#include <cdefBF512.h>
#include <ccblkfn.h>
#include <bfrom.h>


#include "system_clock.h"

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void CLOCK_Init(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

/*******************************************************************
*   Function:    CLOCK_Init
*   Description: initializes the PLL registers
*******************************************************************/
void CLOCK_Init(void)
{
	u32 SIC_IWR1_reg;                /* backup SIC_IWR1 register */

	/* use Blackfin ROM SysControl() to change the PLL */
    ADI_SYSCTRL_VALUES sysctrl ={	VRCTL_VALUE,
									PLLCTL_VALUE,		/* (12MHz CLKIN x (MSEL=22))::CCLK = 264MHz */
									PLLDIV_VALUE,		/* (264MHz/(SSEL=2))::SCLK = 132MHz */
									PLLLOCKCNT_VALUE,
									PLLSTAT_VALUE };

	SIC_IWR1_reg = *pSIC_IWR1;				/* save SIC_IWR1 due to anomaly 05-00-0432 */
	*pSIC_IWR1 = 0;							/* disable wakeups from SIC_IWR1 */

	/* use the ROM function */
	bfrom_SysControl( SYSCTRL_WRITE | SYSCTRL_PLLCTL | SYSCTRL_PLLDIV, &sysctrl, NULL);

	*pSIC_IWR1 = SIC_IWR1_reg;				/* restore SIC_IWR1 due to anomaly 05-00-0432 */
}

unsigned long getVCOFreq(void)
{
    unsigned short int value = *pPLL_CTL;
    unsigned char mul = (unsigned char)(value >> 9) & 0x3F;//get bit[14:9]
    unsigned long vcoFreq = 0;

    if (value & 0x01)   //pass CLKIN/2 to PLL
    {
        if (0 == mul)
        {
            vcoFreq = (unsigned long)(SYSTEM_CLKIN * 32);//max 32x
        }
        else
        {
            vcoFreq = (unsigned long)(SYSTEM_CLKIN / 2) * mul;
        }
    }
    else                //pass CLKIN to PLL
    {
        if (0 == mul)
        {
            vcoFreq = (unsigned long)(SYSTEM_CLKIN * 64);//max 64x
        }
        else
        {
            vcoFreq = (unsigned long)SYSTEM_CLKIN * mul;
        }
    }
    
    return vcoFreq;

}

unsigned long getCodeCLK(void)
{
    unsigned short int value = *pPLL_DIV;
    unsigned char div = (unsigned char)(value >> 4) & 0x03;//get bit[5:4]
    unsigned long cclk = 0;
    cclk = getVCOFreq()/(1 << div);

    return cclk;
}

unsigned long getSystemCLK(void)
{
    unsigned short int value = *pPLL_DIV;
    unsigned char div = (unsigned char)(value & 0x0F);//get bit[5:4]
    unsigned long sclk = 0;
    sclk = getVCOFreq()/ div;
    
    return sclk;
}


