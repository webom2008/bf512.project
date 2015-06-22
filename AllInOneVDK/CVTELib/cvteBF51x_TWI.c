/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : cvteBF51x_TWI.c
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/3/2
  Last Modified :
  Description   : Two Wire Interface 
  Function List :
  History       :
  1.Date        : 2015/3/2
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/
#include "cvteBF51x_TWI.h"

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
typedef struct
{
    u8 prescale;
    u8 clkdiv_h;
    u8 clkdiv_l;
    void (*pDelay)(const unsigned long ms);
} TWI_ConfTypeDef;

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/
#define TWICount(x) (TWI_MASTER_CTL_DCNT & ((x) << 6))

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal variables                           *
 *----------------------------------------------*/
static int TWIsuccess;
static TWI_ConfTypeDef gTwiCfg;
static TWI_ConfTypeDef *pgTwiCfg = &gTwiCfg;

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

int TWI_Init(TWI_TypeDef *pDev, TWI_InitStruct *pInitStruct)
{
    u16 clkdiv;
    
    //PRESCALE = fSCLK/10MHz
    pgTwiCfg->prescale = (u8)((pInitStruct->sclk + 10000000/2) / 10000000);

    /* Note:
     * 1.CLKDIV = (1/SCL)/(1/10MHz)
     * 2.PRESCALE = SCLK / 10MHz
     * 注意:因为SCLK可能不是10MHz的倍数，例如132MHz，则PRESCALE存在误差。
     * SCLK=132MHz，计算如下:
     * 公式1.PRESCALE = (132M + 10M /2)/10M = 13;"10M/2"是除以10M之前的调整，解决四舍五入问题
     *      逆运算:SCLK / PRESCAL = 132M / 13 = 132/13 M，并不是10MHz哦
     * 公式2.变换为SCL = 10MHz / CLKDIV;
     *      此处注意2点:1)10MHz应该替换为132/13MHz;2)CLKDIV = CLKDIV_HI + CLKDIV_LO
     *      SCL = 132 / (13 * CLKDIV) MHz < 400KHz，得出CLKDIV = 26，SCL = 390.5KHz
     * HW Suggestion: SCLK < 330KHz ==> CLKDIV=31,SCLK=328KHz
     */
    if (TWI_ClockSpeed_100K == pInitStruct->TWI_ClockSpeed)
    {
        clkdiv = (u16)((pInitStruct->sclk / pgTwiCfg->prescale)/100000);
    }
    else //SPEED_400K
    {
        clkdiv = (u16)((pInitStruct->sclk / pgTwiCfg->prescale)/400000);
    }

    if (TWI_DutyCycle_2 == pInitStruct->TWI_DutyCycle)
    {
        pgTwiCfg->clkdiv_l = (u8)(clkdiv * 2 / 3);
    }
    else
    {
        pgTwiCfg->clkdiv_l = (u8)(clkdiv * 16 / 25);
    }
    pgTwiCfg->clkdiv_h = clkdiv - pgTwiCfg->clkdiv_l;
    pgTwiCfg->pDelay = pInitStruct->pTWI_Delay;
    
    pDev->CONTROL = TWI_CONTROL_ENA | pgTwiCfg->prescale;
    ssync();
    pDev->CLKDIV = ((pgTwiCfg->clkdiv_h) << 8) | (pgTwiCfg->clkdiv_l);
    ssync();
    pDev->INT_MASK = TWI_INT_MASK_RCVSERVM | TWI_INT_MASK_XMTSERVM | TWI_INT_MASK_MERRM | TWI_INT_MASK_MCOMPM;
    ssync();
	pDev->FIFO_CTL = TWI_FIFO_CTL_XMTFLUSH | TWI_FIFO_CTL_RCVFLUSH;
	ssync();
    
    return 0;
}

int TWI_Reset(TWI_TypeDef *pDev)
{
     /* reset TWI controller */
	pDev->CONTROL = TWI_CONTROL_RESET;
	ssync();

	/* clear errors before enabling TWI */
	pDev->MASTER_STAT = TWI_MASTER_STAT_BUFWRERR | TWI_MASTER_STAT_BUFRDERR \
	                    | TWI_MASTER_STAT_LOSTARB | TWI_MASTER_STAT_ANAK \
	                    | TWI_MASTER_STAT_DNAK;
	ssync();

	/* clear interrupts before enabling TWI */
	pDev->INT_STAT = TWI_INT_STAT_SINIT | TWI_INT_STAT_SCOMP | TWI_INT_STAT_SERR \
	                | TWI_INT_STAT_SOVF | TWI_INT_STAT_MCOMP | TWI_INT_STAT_MERR \
	                | TWI_INT_STAT_XMTSERV | TWI_INT_STAT_RCVSERV;
	ssync();

	/* flush rx and tx fifos */
	pDev->FIFO_CTL = TWI_FIFO_CTL_XMTFLUSH | TWI_FIFO_CTL_RCVFLUSH;
	ssync();
	
	/* set this to 1 so that we go into the write sequence the very first time */
	TWIsuccess = 1;
    
    return 0;
}

/*******************************************************************
*   Function:    ClrSclSda
*   Description: restores control of the TWI interface
*******************************************************************/
static bool ClrSclSda(TWI_TypeDef *pDev)
{
	int cnt = 0;
    do 
    {

    	pDev->MASTER_CTL = TWI_MASTER_CTL_SCLOVR;
        pgTwiCfg->pDelay(6);
        pDev->MASTER_CTL = 0;
        pgTwiCfg->pDelay(6);

	} while ((pDev->MASTER_STAT & TWI_MASTER_STAT_SDASEN) && (cnt++ < 8));

    pDev->MASTER_CTL = TWI_MASTER_CTL_SDAOVR | TWI_MASTER_CTL_SCLOVR;
    pgTwiCfg->pDelay(6);
    pDev->MASTER_CTL = TWI_MASTER_CTL_SDAOVR;
    pgTwiCfg->pDelay(6);
    pDev->MASTER_CTL = 0;
    return ((pDev->MASTER_STAT & TWI_MASTER_STAT_SDASEN) || (cnt == 8)) ? false : true;
}

int TWI_MasterRead( TWI_TypeDef *pDev,
                    const unsigned short DeviceAddr,
                    unsigned char *TWI_Data_Pointer,
                    const unsigned short TWI_Length)
{
    int i;
    unsigned long timeout  = 0;
    
    /***********************************************************
     An interrupt (RCVSERV) is set when RCVSTAT indicates one or
     two bytes in the FIFO are full (01 or 11).
    ***********************************************************/ 
    pDev->FIFO_CTL = 0;

	pDev->CONTROL = TWI_CONTROL_ENA | pgTwiCfg->prescale;
	pDev->CLKDIV = ((pgTwiCfg->clkdiv_h) << 8) | (pgTwiCfg->clkdiv_l);
    
    /***********************************************************
    The address needs to be shifted one place to the right
    e.g., 1010 001x becomes 0101 0001 (0x51) the TWI controller 
    will actually send out 1010 001x where x is either a 0 for 
    writes or 1 for reads
    ***********************************************************/ 
    pDev->MASTER_ADDR = DeviceAddr >> 1;
    
    pDev->MASTER_CTL = TWICount(TWI_Length) | TWI_MASTER_CTL_MEN \
                        | TWI_MASTER_CTL_MDIR | TWI_MASTER_CTL_FAST;  /* start transmission */
    ssync();
    
    for (i = 0; i < TWI_Length; i++)
    {
        timeout = 0x1000;
        /***********************************************************
        Poll the FIFO Status register to know when 
        1 bytes have been shifted into the RX FIFO 
        ***********************************************************/
        while (((pDev->FIFO_STAT & TWI_FIFO_STAT_RCVSTAT) == TWI_FIFO_STAT_RCV_EMPTY)&&(--timeout > 0))//rx fifo empty
        {
            ssync();
        }
        
        *TWI_Data_Pointer++ = pDev->RCV_DATA8;/* Read data from the RX fifo */
        ssync();
        if (!timeout)
        {
            udprintf("\r\n[TWI]TWI READ RCV FIFO EMPTY Timeout ERROR!");
            return -1;
        }
    }

    /***********************************************************
    check that master transfer has completed 
    MCOMP will be set when Count reaches zero 
    ***********************************************************/ 
    timeout = 0x1000;
    while (((pDev->INT_STAT & TWI_INT_STAT_MCOMP) == 0)&&(--timeout > 0))
    {
        ssync();
    }
    pDev->INT_STAT = TWI_INT_STAT_RCVSERV | TWI_INT_STAT_MCOMP;/* service TWI for next transmission */
    
	asm("nop;");
	asm("nop;");
	asm("nop;");
    
    if (!timeout)
    {
        udprintf("\r\n[TWI]TWI READ WAIT MCOMP Timeout ERROR!");
        return -2;
    }
    else
    {
        return TWI_Length;
    }
}

int TWI_MasterWrite(TWI_TypeDef *pDev,
                    const unsigned short DeviceAddr,
                    unsigned char *TWI_Data_Pointer,
                    const unsigned short Count,
                    const unsigned short TWI_Length)
{
    int i, j;
    unsigned long timeout  = 0;
    
	if (pDev->MASTER_STAT & TWI_MASTER_STAT_SDASEN)
	{
		if(!ClrSclSda(pDev))
		{
            udprintf("\r\n[TWI]TWI_MasterWrite ClrSclSda ERROR!");
			return -1;
        }
	}
    
    /* make sure no previous errors occured */
    if (0 == TWIsuccess)
    {
        udprintf("\r\n[TWI]TWIsuccess = 0");
		return -1;
    }

    /***********************************************************
     An interrupt (RCVSERV) is set when RCVSTAT indicates one or
     two bytes in the FIFO are full (01 or 11).
     An interrupt (XMTSERV) is set when XMTSTAT indicates one or
     two bytes in the FIFO are empty (01 or 00)
    ***********************************************************/ 
    pDev->FIFO_CTL = 0;

	pDev->CONTROL = TWI_CONTROL_ENA | pgTwiCfg->prescale;
	pDev->CLKDIV = ((pgTwiCfg->clkdiv_h) << 8) | (pgTwiCfg->clkdiv_l);

    /***********************************************************
    The address needs to be shifted one place to the right
    e.g., 1010 001x becomes 0101 0001 (0x51) the TWI controller 
    will actually send out 1010 001x where x is either a 0 for 
    writes or 1 for reads
    ***********************************************************/ 
    pDev->MASTER_ADDR = DeviceAddr >> 1;
    
    
    for (i = 0; i < Count; i++)
    {
        /***********************************************************
        Pre-load the tx FIFO with the first two bytes: this is 
        necessary to avoid the generation of the Buffer Read Error
        (BUFRDERR) which occurs whenever a transmit transfer is 
        initiated while the transmit buffer is empty
        ***********************************************************/
        pDev->XMT_DATA8 = *TWI_Data_Pointer++;  /* pointer to data */
        ssync();

        pDev->MASTER_CTL = TWICount(TWI_Length) | TWI_MASTER_CTL_FAST | TWI_MASTER_CTL_MEN;

        for (j = 0; j < (TWI_Length-1); j++)
        {
            timeout = 0x1000;
            
            /*******************************************************
            check that there's at least one byte location empty in 
            the tx fifo
            *******************************************************/
            while ((pDev->FIFO_STAT & TWI_FIFO_STAT_XMTSTAT == TWI_FIFO_STAT_XMT_FULL) && --timeout)
            {
                ssync();
            }
            
            if(!timeout)
            {
                udprintf("\r\n[TWI]TWI WRITE WAIT XMTSERV Timeout ERROR!");
                return -1;
            }
            pDev->XMT_DATA8 = *TWI_Data_Pointer++;      /* load the next sample into the TX FIFO */
            ssync();
        }
        
        /* check that master transfer has completed */
        timeout = 0x1000;
        
        while (!(pDev->INT_STAT & TWI_INT_STAT_MCOMP) && --timeout)          /* wait until transmission complete and MCOMP is set */
        {
            ssync();
        }
        if(!timeout)
        {
            udprintf("\r\n[TWI]TWI WRITE WAIT MCOMP Timeout ERROR!");
            return -2;
        }
        
        /* check if an error occured */
        if (pDev->INT_STAT & TWI_INT_STAT_MERR)
        {
            pDev->INT_STAT |= TWI_INT_STAT_MERR;
            TWIsuccess = 0;
            udprintf("\r\n[TWI]TWI WRITE MERR Timeout ERROR!");
        }
        
        pDev->INT_STAT = TWI_INT_STAT_XMTSERV | TWI_INT_STAT_MCOMP;/* service TWI for next transmission */
        ssync();
    }
	asm("nop;");
	asm("nop;");
	asm("nop;");
    
    if (0 == TWIsuccess) return -1;
    else return TWI_Length;
}

