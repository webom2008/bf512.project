/*
 * system_sdram.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include "system_sdram.h"

#define _SDRAM_DEBUG_INFO_

#ifdef _SDRAM_DEBUG_INFO_
#define SDRAM_DBG_INFO(fmt, arg...) AIO_printf("\r\n[SDRAM] "fmt, ##arg)
#else
#define SDRAM_DBG_INFO(fmt, arg...)
#endif


#define SDRAM_SIZE_IN_BYTES 0x01000000	/* SDRAM is 16 MByte */
#define SDRAM_END_ADDR      0x00FFFFFF

void speed_w_test_2(void)
{
    int i,cont;
    volatile unsigned short* DDRdata;
    DDRdata = 0;
    
    for(i = 0; i < (SDRAM_END_ADDR/2); i++)        
    {
        if(i % 2)
        {
            *DDRdata    =    0xaaaa;
            DDRdata++;
        }
        if(!(i % 2 ))
        {
            *DDRdata    =    0x5555;
            DDRdata++;
        }    
    }        
}

void speed_r_test_2(void)
{
    int i,cont,temp1,temp2;
    volatile unsigned short* DDRdata;
    
    DDRdata = 0;
    for(i = 0;i < (SDRAM_END_ADDR/2); i++)    
    {
        if(i % 2)
        {
            temp1 = *DDRdata;           
            
            if(temp1 != 0xaaaa)
            {
                temp2 = *DDRdata;
                while(1)
                {
                    AIO_printf("\r\n [SDRAM]error! Addr is 0x%08x, Data is %x",i,temp1);
                }
            }
            
            DDRdata++;
        }
        if(!(i % 2 ))
        {
            temp1 = *DDRdata;           
            
            if(temp1 != 0x5555)
            {
                temp2 = *DDRdata;
                while(1)
                {
                    AIO_printf("\r\n [SDRAM]error! Addr is 0x%08x, Data is %x",i,temp1);
                }
            }
            
            DDRdata++;
        }
    }
    SDRAM_DBG_INFO("speed_r_test_2 OK!");
}

void SDRAM_Init(void)
{
    u16 *pTmp;
    while((*pEBIU_SDSTAT & SDCI) == 0){}
    
    /* clear SDRAM EAB sticky error status (W1C) */
    *pEBIU_SDSTAT |= SDEASE;
    
    /* SDRAM Refresh Rate Control Register */
    *pEBIU_SDRRC = 0x0805;
    
    /* SDRAM Memory Bank Control Register */
    *pEBIU_SDBCTL = EBCAW_9 | //Page size 512(16bit)
                    EBSZ_16 | //16 MB of SDRAM
                    EBE;      //SDRAM enable
    
    /* SDRAM Memory Global Control Register */
    //*pEBIU_SDGCTL = (EMREN|SCTLE|PSS|TWR_2|TRCD_3|TRP_3|TRAS_6|PASR_ALL|CL_3);
    *pEBIU_SDGCTL = //~CDDBG  & // Control disable during bus grant off 
                    ~FBBRW  & // Fast back to back read to write off   
                    ~EBUFE  & // External buffering enabled off 
                    ~SRFS   & // Self-refresh setting off 
                    ~PSM    & // Powerup sequence mode (PSM) first 
                    ~PUPSD  & // Powerup start delay (PUPSD) off 
                    TCSR    | // Temperature compensated self-refresh at 85   
                    EMREN   | // Extended mode register enabled on  
                    PSS     | // Powerup sequence start enable (PSSE) on 
                    TWR_2   | // Write to precharge delay TWR = 2 (14-15 ns) 
                    TRCD_3  | // RAS to CAS delay TRCD =3 (15-20ns) 
                    TRP_3   | // Bank precharge delay TRP = 2 (15-20ns) 
                    TRAS_6  | // Bank activate command delay TRAS = 4 
                    //PASR_B0 | // Partial array self refresh  Only SDRAM Bank0
                    PASR_ALL | //All 4 SDRAM Banks Refreshed In Self-Refresh   
                    CL_3    | // CAS latency 
                    SCTLE  ;  // SDRAM clock enable
                    
    
    /* Finalize SDC initialization */
    pTmp = (u16*) 0x0;
    *pTmp = 0xBEEF;
    while((*pEBIU_SDSTAT & SDRS) == 1){}
}

void SDRAM_SelfCheck(void)
{
    SDRAM_DBG_INFO("SDRAM_SelfCheck start tick = %d",SysTick_Get());
    speed_w_test_2();
    speed_r_test_2();
    SDRAM_DBG_INFO("SDRAM_SelfCheck stop tick = %d",SysTick_Get());
}

