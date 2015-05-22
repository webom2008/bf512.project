/*
 * system_twi.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 *  Describe:
 */
#include <ccblkfn.h>

#include "system_twi.h"

#define _CLR_SCL_SDA_

#define TWI_SPEED_330KHZ
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
#if defined(TWI_SPEED_400KHZ)
#define CLKDIV_HI 9    /* SCL high period */
#define CLKDIV_LO 17    /* SCL low period */

#elif defined(TWI_SPEED_330KHZ)
#define CLKDIV_HI 10    /* SCL high period */
#define CLKDIV_LO 21    /* SCL low period */

#elif defined(TWI_SPEED_100KHZ)
#define CLKDIV_HI 34    /* SCL high period */
#define CLKDIV_LO 68    /* SCL low period */

#endif

#define TWICount(x) (DCNT & ((x) << 6))

u8 g_u8PRESCALE;


//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void SystemTWI_Init(void);
#pragma section("sdram0_bank3")
void TWI_Init(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

/***********************************************************
TWI Master Initialization subroutine
**********************************************************/ 
static void TWI_Init(void)
{
    /***********************************************************
    Enable the TWI controller and set the Prescale value
    Prescale = 10 (0xA) for an SCLK = 100 MHz (CLKIN = 50MHz)
    Prescale = SCLK / 10 MHz

    P1 points to the base of the system MMRs
    ***********************************************************/ 
    *pTWI_CONTROL = TWI_ENA | g_u8PRESCALE;

    /***********************************************************
    Set CLKDIV:
    For example, for an SCL of 400 KHz (period = 1/400 KHz = 2500 ns) 
    and an internal time reference of 10 MHz (period = 100 ns):
    CLKDIV = 2500 ns / 100 ns = 25
    For an SCL with a 30% duty cycle, then CLKLOW = 17 (0x11) and CLKHI = 8.
    ***********************************************************/ 
    *pTWI_CLKDIV = ((CLKDIV_HI) << 8) | (CLKDIV_LO);

    /***********************************************************
    enable these signals to generate a TWI interrupt: optional
    ***********************************************************/ 
    *pTWI_INT_MASK = RCVSERV | XMTSERV | MERR | MCOMP;

    /* flush rx and tx fifos */
	*pTWI_FIFO_CTL = XMTFLUSH | RCVFLUSH;
	ssync();
}

#ifdef _CLR_SCL_SDA_
/*******************************************************************
*   Function:    ClrSclSda
*   Description: restores control of the TWI interface
*******************************************************************/
static bool ClrSclSda(void)
{

	int cnt = 0;

    do {
    	*pTWI_MASTER_CTL = SCLOVR;
        Delay1ms(6);
        *pTWI_MASTER_CTL = 0;
        Delay1ms(6);
	} while ((*pTWI_MASTER_STAT & SDASEN) && (cnt++ < 8));

    *pTWI_MASTER_CTL = SDAOVR | SCLOVR;
    Delay1ms(6);
    *pTWI_MASTER_CTL = SDAOVR;
    Delay1ms(6);
    *pTWI_MASTER_CTL = 0;
    
    return ((*pTWI_MASTER_STAT & SDASEN) || (cnt == 8)) ? false : true;
}
#endif

/***********************************************************
Starting the Read transfer
Program the Master Control register with: 
1. the number of bytes to transfer: TWICount(x) 
2. Repeated Start (RESTART): optional
3. speed mode: FAST or SLOW 
4. direction of transfer: 
      MDIR = 1 for reads, MDIR = 0 for writes 
5. Master Enable MEN. This will kick off the master transfer 
***********************************************************/ 
int TWI_MasterMode_Read(const unsigned short DeviceAddr,
                         unsigned char *TWI_Data_Pointer,
                         const unsigned short TWI_Length)
{
    int i;
    unsigned long timeout  = 0;
    /***********************************************************
    The address needs to be shifted one place to the right
    e.g., 1010 001x becomes 0101 0001 (0x51) the TWI controller 
    will actually send out 1010 001x where x is either a 0 for 
    writes or 1 for reads
    ***********************************************************/ 
    *pTWI_MASTER_ADDR = DeviceAddr >> 1;
    
    /***********************************************************
     An interrupt (RCVSERV) is set when RCVSTAT indicates one or
     two bytes in the FIFO are full (01 or 11).
    ***********************************************************/ 
    *pTWI_FIFO_CTL = 0;

    *pTWI_MASTER_CTL = TWICount(TWI_Length) | MEN | MDIR | FAST;  /* start transmission */
    ssync();
    for (i = 0; i < TWI_Length; i++)
    {
        timeout = 0x1000;
        /***********************************************************
        Poll the FIFO Status register to know when 
        1 bytes have been shifted into the RX FIFO 
        ***********************************************************/
        while (((*pTWI_FIFO_STAT & 0xC) == RCV_EMPTY)&&(--timeout > 0))//rx fifo empty
        {
            ssync();
        }
        
        *TWI_Data_Pointer++ = *pTWI_RCV_DATA8;/* Read data from the RX fifo */
        ssync();
        if (!timeout)
        {
            AIO_printf("\r\n[TWI]TWI READ RCV FIFO EMPTY Timeout ERROR!");
            return -1;
        }
    }

    /***********************************************************
    check that master transfer has completed 
    MCOMP will be set when Count reaches zero 
    ***********************************************************/ 
    timeout = 0x1000;
    while (((*pTWI_INT_STAT & MCOMP) == 0)&&(--timeout > 0))
    {
        ssync();
    }
    if (!timeout)
    {
        AIO_printf("\r\n[TWI]TWI READ WAIT MCOMP Timeout ERROR!");
        return -2;
    }
    *pTWI_INT_STAT = RCVSERV | MCOMP;			/* service TWI for next transmission */
    
	asm("nop;");
	asm("nop;");
	asm("nop;");
    return TWI_Length;
}

/***********************************************************
Initiating the Write operation
Program the Master Control register with:
1. the number of bytes to transfer: TWICount(x) 
2. Repeated Start (RESTART): optional 
3. speed mode: FAST or Standard 
4. direction of transfer: 
      MDIR = 1 for reads, MDIR = 0 for writes
5. Master Enable MEN. Setting this bit will kick off the transfer
***********************************************************/ 
int TWI_MasterMode_Write(const unsigned short DeviceAddr,
                         unsigned char *TWI_Data_Pointer,
                         const unsigned short Count,
                         const unsigned short TWI_Length)

{
    int i, j;
    unsigned long timeout  = 0;
    
#ifdef _CLR_SCL_SDA_
	if (*pTWI_MASTER_STAT & SDASEN)
	{
		if(!ClrSclSda())
		{
            AIO_printf("\r\n[TWI]TWI_MasterMode_Write ClrSclSda ERROR!");
			return -1;
        }
	}
#endif
    
    /***********************************************************
    The address needs to be shifted one place to the right
    e.g., 1010 001x becomes 0101 0001 (0x51) the TWI controller 
    will actually send out 1010 001x where x is either a 0 for 
    writes or 1 for reads
    ***********************************************************/ 
    *pTWI_MASTER_ADDR = DeviceAddr >> 1;
    
    /***********************************************************
     An interrupt (RCVSERV) is set when RCVSTAT indicates one or
     two bytes in the FIFO are full (01 or 11).
     An interrupt (XMTSERV) is set when XMTSTAT indicates one or
     two bytes in the FIFO are empty (01 or 00)
    ***********************************************************/ 
    *pTWI_FIFO_CTL = 0;
    
    for (i = 0; i < Count; i++)
    {
        /***********************************************************
        Pre-load the tx FIFO with the first two bytes: this is 
        necessary to avoid the generation of the Buffer Read Error
        (BUFRDERR) which occurs whenever a transmit transfer is 
        initiated while the transmit buffer is empty
        ***********************************************************/
        *pTWI_XMT_DATA8 = *TWI_Data_Pointer++;  /* pointer to data */
        ssync();

        *pTWI_MASTER_CTL = TWICount(TWI_Length) | FAST | MEN;
        
        //AIO_printf("\r\n[TWI]TWI WRITE Data len = %d",TWI_Length);//add by QWB 20140226
        
        timeout = 0x1000;

        for (j = 0; j < (TWI_Length-1); j++)
        {
            timeout = 0x1000;//add by QWB 20140226
            /*******************************************************
            check that there's at least one byte location empty in 
            the tx fifo
            *******************************************************/
            while ((*pTWI_FIFO_STAT == XMTSTAT) && --timeout)
            {
                ssync();
            }
            if(!timeout)
            {
                AIO_printf("\r\n[TWI]TWI WRITE WAIT XMTSERV Timeout ERROR!");
                return -1;
            }
            *pTWI_XMT_DATA8 = *TWI_Data_Pointer++;      /* load the next sample into the TX FIFO */
            ssync();
        }
        
        /* check that master transfer has completed */
        timeout = 0x1000;
        
        while (!(*pTWI_INT_STAT & MCOMP) && --timeout)          /* wait until transmission complete and MCOMP is set */
        {
            ssync();
        }
        if(!timeout)
        {
            AIO_printf("\r\n[TWI]TWI WRITE WAIT MCOMP Timeout ERROR!");
            return -2;
        }
        
        /* check if an error occured */
        if ((*pTWI_INT_STAT & MERR) == MERR)
        {
            *pTWI_INT_STAT |= MERR;
            AIO_printf("\r\n[TWI]TWI WRITE MERR Timeout ERROR!");
        }
        
        *pTWI_INT_STAT = XMTSERV | MCOMP;               /* service TWI for next transmission */
        ssync();
    }
	asm("nop;");
	asm("nop;");
	asm("nop;");
    return TWI_Length;
}

void SystemTWI_Init(void)
{
    g_u8PRESCALE = (u8)((getSystemCLK() + 10000000/2) / 10000000);
    TWI_Init();
}

