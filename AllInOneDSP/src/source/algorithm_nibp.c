/*
 * algorithm_nibp.c
 *
 *  Created on: 2013-12-27
 *      Author: QiuWeibo
 */
#include "algorithm_nibp.h"

//#define _ALG_NIBP_INFO_
#ifdef _ALG_NIBP_INFO_
#define ALG_NIBP_DBG(fmt, arg...) AIO_printf("\r\n[ALG_NIBP] "fmt, ##arg)
#else
#define ALG_NIBP_DBG(fmt, arg...)
#endif

bool g_nibp_algo_sw;
NIBP_ALGO_STATE_TypeDef g_nibp_algo_state;

#pragma section("sdram0_bank3")
CNIBPResult nibpResult;
#pragma section("sdram0_bank3")
CNIBPResult *p_nibpResult = &nibpResult;

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void nibp_algorithm_start(void);
#pragma section("sdram0_bank3")
void nibp_algorithm_init(void);
#pragma section("sdram0_bank3")
void nibp_algorithm_pushLastRet(int *pDP, int *pSP, int *pBPM);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->


static void nibp_algorithm_result(void)
{
    static bool printstate = false;
    
    l_nibp_PopAlgorithmData((void *)p_nibpResult);
    if (true == p_nibpResult->isRisingDone &&  false == printstate)
    {
        printstate = true;
        ALG_NIBP_DBG("isRisingDone!g_Rising_mmHgPre:%d",g_Rising_mmHgCur);
    }
    if (true == p_nibpResult->isAllDone &&  true == printstate)
    {
        printstate = false;
        ALG_NIBP_DBG("isAllDone!BPM:%d,DP:%d,MAP:%d,SP:%d",
                        p_nibpResult->BPM,
                        p_nibpResult->DP,
                        p_nibpResult->MAP,
                        p_nibpResult->SP);
    }
}

void nibp_algorithm_init(void)
{
#ifdef _NIBP_USED_WINDOWS_
    g_nibp_algo_sw = false;
#else
    g_nibp_algo_sw = true;
#endif
}

void nibp_algorithm_start(void)
{
    g_nibp_algo_state = NIBP_ALGO_RISING;
    memset(p_nibpResult, 0, sizeof(p_nibpResult));
    p_nibpResult->isAllDone = false;
    p_nibpResult->isRisingDone = false;
    l_nibp_AlgorithmInit();
}

void nibp_algorithm_handler(void)
{
    if (false == g_nibp_algo_sw) return;
    
    u32 u32Tick = 0, u32mmHg, u32adc;
    //u32 u32curtick;
    s32 s32Len = NIBP_PopAlgoResult(&u32Tick, &u32mmHg, &u32adc);
    if (s32Len)//有数据更新
    {
        //u32curtick = SysTick_Get();
        l_nibp_PushAlgorithmData(&u32Tick, &u32mmHg, &g_Rising_mmHgCur);
        nibp_algorithm_result();
        //AIO_printf("\r\nNIBP ALGO TICK = \t%d",SysTick_Get()-u32curtick);
    }
}

void nibp_algorithm_setPatient(char newType)
{
    l_nibp_SetAlgorithmPatient(newType);
}

void nibp_algorithm_pushLastRet(int *pDP, int *pSP, int *pBPM)
{
    l_nibp_pushLastRet(pDP, pSP, pBPM);
}


