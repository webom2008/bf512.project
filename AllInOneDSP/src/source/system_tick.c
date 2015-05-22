/*
 * system_tick.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include <ccblkfn.h>
#include <stdio.h>

#include "system_tick.h"

typedef struct CountDownTimer_TAG
{	
    bool m_IsActive;    
    unsigned long m_ulTimeoutCounter;
}countdowntimer;

static volatile unsigned long g_ulTickCount;
static unsigned char g_sysTick;
static countdowntimer sCountDownTimer[MAX_NUM_COUNTDOWN_TIMERS] = { {0,0},{0,0},{0,0},{0,0},{0,0} };
unsigned char g_u8TickN10us;

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void SysTick_Init(void);
#pragma section("sdram0_bank3")
void Init_Timer_Interrupts(void);
#pragma section("sdram0_bank3")
void SysTick_TimeCfg(const unsigned char N10us);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

/*******************************************************************
*   Function:    SysTick_TimeCfg
*   Description: This function configure the core timer cycle (N * 10us)=10us~2550us
*   Note:   N10us must larger than 1
*******************************************************************/
static void SysTick_TimeCfg(const unsigned char N10us)
{
    unsigned long ulPeriod = 0;

    ulPeriod = getCodeCLK() / 100000;

	/* active state, auto reload, generate interrupt */
    *pTCNTL = TMPWR | TAUTORLD | TINT;
    *pTPERIOD = ulPeriod;
    if (N10us < 1)
    {
        *pTSCALE = 0;   //default, system tick = 10us
    }
    else
    {
        *pTSCALE = N10us-1;//TSCALE = 0~255
        g_u8TickN10us = N10us;
    }

    /* TCOUNT decrements once every TSCALE + 1 clock cycles. 
     * When the value of TCOUNT reaches 0, an interrupt is generated and the 
     * TINT bit of the TCNTL register is set.
     * Values written to the TPERIOD register are automatically copied to the TCOUNT register.
     */
    *pTCOUNT = ulPeriod;
}

void SysTick_Start(void)
{
    /* enable the timer */
    *pTCNTL |= TMREN;
}
void SysTick_Stop(void)
{
    /* enable the timer */
    *pTCNTL &= (unsigned long)~TMREN;
}


/*******************************************************************
*   Function:    Init_Timer_Interrupts
*   Description: This function initialises the interrupts for core timer
*******************************************************************/
static void Init_Timer_Interrupts(void)
{	
    register_handler(ik_timer, CoreTimer_ISR);
}

void SysTick_Init(void)
{
    g_ulTickCount = 0;
    SysTick_TimeCfg(10);//system tick configure as 10*10us = 100us
    Init_Timer_Interrupts();
}


/*******************************************************************
*   Function:    Delay1ms
*   Description: Delay for a fixed number of Ms, blocks
*******************************************************************/
void Delay1ms(const unsigned long ulMs)
{	
    unsigned long scale = *pTSCALE;
    unsigned long ulTime = (ulMs *1000) / (((scale & 0xFF)+1)*10);
    //-DEBUG140108-unsigned int uiTIMASK = cli();// saves the contents of the IMASK register 
    unsigned long ulEnd = (g_ulTickCount + ulTime);
    //-DEBUG140108-sti(uiTIMASK);//restores the contents of the IMASK register
    while( g_ulTickCount < ulEnd )
    {
        asm("nop;");
    }
}

//粗略的延时
void delay(const unsigned long count)
{
    unsigned long num = count;
    while(num--);
}

/*******************************************************************
*   Function:    SetTimeout
*   Description: Set a value for a global timeout, return the timer index
*******************************************************************/
unsigned int SetTimeout(const unsigned long ulTicks)
{   
    //-DEBUG140108-unsigned int uiTIMASK = cli();
    unsigned int n;

    /* we don't care which countdown timer is used, so search for a free timer structure */
    for( n = 0;  n < MAX_NUM_COUNTDOWN_TIMERS; n++ )
    {       
        if( false == sCountDownTimer[n].m_IsActive )        
        {           
            sCountDownTimer[n].m_IsActive = true; 
            sCountDownTimer[n].m_ulTimeoutCounter = ulTicks;
            //-DEBUG140108-sti(uiTIMASK);
            return n; 
        }  
    }
    
    //-DEBUG140108-sti(uiTIMASK);  
    return ((unsigned int)-1);
}

/*******************************************************************
*   Function:    ResetTimeout
*   Description: 
*******************************************************************/
bool ResetTimeout(const unsigned int nIndex, const unsigned long ulTicks)
{   
    //-DEBUG140108-unsigned int uiTIMASK;
    if (nIndex > MAX_NUM_COUNTDOWN_TIMERS)
    {
        return false;
    }
    if (false == sCountDownTimer[nIndex].m_IsActive)
    {
        return false;
    }

    //-DEBUG140108-uiTIMASK = cli();
    sCountDownTimer[nIndex].m_ulTimeoutCounter = ulTicks;
    //-DEBUG140108-sti(uiTIMASK);
    return true; 
}

/*******************************************************************
*   Function:    ClearTimeout
*   Description: Set a value for a global timeout, return the timer
*******************************************************************/
unsigned long ClearTimeout(const unsigned int nIndex)
{   
    //-DEBUG140108-unsigned int uiTIMASK = cli();
    unsigned long ulTemp = (unsigned int)(-1); 
    if( nIndex < MAX_NUM_COUNTDOWN_TIMERS ) 
    {       
        /* turn off the timer */  
        ulTemp = sCountDownTimer[nIndex].m_ulTimeoutCounter;
        sCountDownTimer[nIndex].m_ulTimeoutCounter = 0;
        sCountDownTimer[nIndex].m_IsActive = false; 
    }   
    //-DEBUG140108-sti(uiTIMASK);  
    return (ulTemp);
}

/*******************************************************************
*   Function:    IsTimedout
*   Description: Checks to see if the timeout value has expired
*******************************************************************/
bool IsTimedout(const unsigned int nIndex)
{  
    //-DEBUG140108-unsigned int uiTIMASK = cli(); 
    if( nIndex < MAX_NUM_COUNTDOWN_TIMERS )
    {      
        //-DEBUG140108-sti(uiTIMASK);      
        return ( 0 == sCountDownTimer[nIndex].m_ulTimeoutCounter ); 
    }   
    //-DEBUG140108-sti(uiTIMASK);  
    return false;
}

unsigned long SysTick_Get(void)
{
    return g_ulTickCount;
}

/*
* @param:   ms, N * 1ms
* @return: unsigned long ,tick number
*/
unsigned long getTickNumberByMS(const unsigned long N1ms)
{
    return (unsigned long)((N1ms * 100) / g_u8TickN10us);//((N1ms * 1000) / (g_u8TickN10us * 10));
}

/*
* @param:   count, tick count
* @return: unsigned long ,ms
*/
unsigned long getMSByTickcount(const unsigned long count)
{
    return (unsigned long)((count * g_u8TickN10us) / 100);//(count* g_u8TickN10us * 10) / 1000;
}

static int SysTick_Compare(unsigned long ulTick1, unsigned long ulTick2) // Interval must less than 0x80000000
{
    if(ulTick1 == ulTick2)
    {
        return 0;       // ==
    }
    else if(ulTick1 < ulTick2)
    {
        if((ulTick2 - ulTick1) < 0x80000000)
        {
            return -1;  // <
        }
        else            // Tick Out
        {
            return 1;   // >
        }
    }
    else    // ulTick1 > ulTick2
    {
        if((ulTick1 - ulTick2) < 0x80000000)
        {
            return 1;   // >
        }
        else            // Tick Out
        {
            return -1;  // <
        }
    }
}

bool IsOnTime(const unsigned long ulTarget) // 0: less than target time     1: on time or over time with target
{
    if(SysTick_Compare(SysTick_Get(), ulTarget) == -1) // <
    {
        return false;
    }

    return true;
}

bool IsOverTime(const unsigned long ulBase, unsigned long ulDuration)    // 0: less than target time     1: over the target time
{
    unsigned long u32Target;

    u32Target = ulBase + ulDuration;

    if(SysTick_Compare(SysTick_Get(), u32Target) == 1)  // >
    {
        return true;
    }

    return false;
}

/*******************************************************************
*   Function:    EX_INTERRUPT_HANDLER(CoreTimer_ISR)
*   Description: Timer ISR
*******************************************************************/
EX_INTERRUPT_HANDLER(CoreTimer_ISR)
{	
    unsigned int n;
    
    //unsigned int uiTIMASK = cli();
    g_ulTickCount++;
    
    /* decrement each counter if it is non-zero */  
    for( n = 0; n < MAX_NUM_COUNTDOWN_TIMERS; n++ ) 
    {       
        if( 0 != sCountDownTimer[n].m_ulTimeoutCounter )       
        {          
            sCountDownTimer[n].m_ulTimeoutCounter--;
        }
    }
    //sti(uiTIMASK);
}

