/*
 * algorithm_resp.c
 *
 *  Created on: 2013-11-28
 *      Author: QiuWeibo
 */
#include <string.h>

#include "algorithm_resp.h"

//#define _ALG_RESP_INFO_
#ifdef _ALG_RESP_INFO_
#define ALG_RESP_DBG(fmt, arg...) AIO_printf("\r\n[ALG_RESP] "fmt, ##arg)
#else
#define ALG_RESP_DBG(fmt, arg...)
#endif

bool g_Is_resp_algo_run;

#pragma section("sdram0_bank3")
CRESP_Out_Item resp_algorithm_out;
CRESP_Out_Item* p_resp_algorithm_out = &resp_algorithm_out;

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void resp_algorithm_init(void);
#pragma section("sdram0_bank3")
void resp_algorithm_setFilter(char mode);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

void resp_algorithm_init(void)
{
    g_Is_resp_algo_run = false;
    memset(p_resp_algorithm_out, 0, sizeof(CRESP_Out_Item));

    //-QWB-嵌入式算法不用初始化此两指针，在l_resp_PopAlgorithmData()中会进行赋值
    p_resp_algorithm_out->stResult.pAlertList = NULL;
    //p_resp_algorithm_out->stResult.pLabelList1 = NULL;
    
    l_resp_AlgorithmInit();
}

static void resp_algo_refreshAlertType(void)
{
#if 1//def _UPLOAD_1HZ_DATA_
    if (NULL == p_resp_algorithm_out->stResult.pAlertList)//非法判断，防止越界
    {
        return;
    }
    int i = 0;
    u32 u32count = p_resp_algorithm_out->stResult.pAlertList->u32AlertCount;
    if (u32count)//存在告警信息
    {
        for (i = 0; i < u32count; i++)
        {
            RESP_AlarmUpload((u8)p_resp_algorithm_out->stResult.pAlertList->alertList[i].alertType);
        }
    }
#else
    RESP_AsphyxiaUpload((u8)p_resp_algorithm_out->stResult.u32RESPRate);
#endif
}

static int resp_algorithm_result(void)
{
    int i32RESPOutItemNum = 0;
    i32RESPOutItemNum = l_resp_PopAlgorithmData((void *)p_resp_algorithm_out, 1);
    if (i32RESPOutItemNum)//获取到有结果
    {
        resp_algo_refreshAlertType();
        ALG_RESP_DBG("RR(%d)", p_resp_algorithm_out->stResult.u32RESPRate);
    }
    return i32RESPOutItemNum;
}

int resp_algorithm_PopFilterResult(u32* pu32Tick, s32* ps32resp)
{
    *pu32Tick = p_resp_algorithm_out->stDataFilter.u32SysTick;
    *ps32resp = p_resp_algorithm_out->stDataFilter.s32RESP;
    return 1;
}

void resp_algorithm_handler(void)
{
    u32 u32Tick = 0;
    s32 s32resp = 0;
    //u32 u32curtick;
    s32 s32Len = RESP_PopAlgoResult(&u32Tick, &s32resp);
    if (s32Len)//有数据更新
    {
        //u32curtick = SysTick_Get();
        g_Is_resp_algo_run = true;
        l_resp_PushAlgorithmData(&u32Tick, &s32resp);
        resp_algorithm_result();
        g_Is_resp_algo_run = false;
        //AIO_printf("\r\nRESP ALGO TICK = \t%d",SysTick_Get()-u32curtick);
    }
}

int resp_algorithm_getRR(u8 *pRR)
{
    *pRR = (u8)p_resp_algorithm_out->stResult.u32RESPRate;
    return 1;
}

void resp_algorithm_setFilter(char mode)
{
    l_resp_DebugInterface(RESP_FILTER_SEL, &mode, 1);
}

