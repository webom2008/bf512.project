/*
 * algorithm_ecg.c
 *
 *  Created on: 2013-11-28
 *      Author: QiuWeibo
 */
#include <string.h>

#include "algorithm_ecg.h"

//#define _ALG_ECG_RUN_TIME_

//#define _ALG_ECG_INFO_
#ifdef _ALG_ECG_INFO_
#define ALG_ECG_DBG(fmt, arg...) AIO_printf("\r\n[ALG_ECG] "fmt, ##arg)
#else
#define ALG_ECG_DBG(fmt, arg...)
#endif

bool g_Is_ecg_algo_run;
bool g_ecg_algo_sw;

#pragma section("sdram0_bank3")
static long g_ecg_filResult[ECG_Sampling_Buffer_Len][3];//I,II,V;
#pragma section("sdram0_bank3")
static unsigned long g_ecg_filTick[ECG_Sampling_Buffer_Len];
#pragma section("sdram0_bank3")
static char g_ecg_filPace[ECG_Sampling_Buffer_Len];
static u16  g_ecg_fil_offset_in, g_ecg_fil_offset_out;

#pragma section("sdram0_bank3")
CECG_Out_Item ecg_algorithm_out;
CECG_Out_Item* p_ecg_algorithm_out = &ecg_algorithm_out;

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void ecg_algorithm_init(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

void ecg_algorithm_PushFilterResult(void);

void ecg_algorithm_init(void)
{
    g_Is_ecg_algo_run = false;
    g_ecg_algo_sw = true;
    g_ecg_fil_offset_in = g_ecg_fil_offset_out = 0;
    
    memset(p_ecg_algorithm_out, 0, sizeof(CECG_Out_Item));

    //-QWB-嵌入式算法不用初始化此两指针，在l_ecg_PopAlgorithmData()中会进行赋值
    p_ecg_algorithm_out->stResult.pAlertList = NULL;
    
    l_ecg_AlgorithmInit();
}

static void ecg_algo_refreshAlertType(void)
{
#ifdef _UPLOAD_1HZ_DATA_
    if (NULL == p_ecg_algorithm_out->stResult.pAlertList)//非法判断，防止越界
    {
        return;
    }
    int i = 0;
    u32 u32count = p_ecg_algorithm_out->stResult.pAlertList->u32AlertCount;
    if (u32count)//存在告警信息
    {
        for (i = 0; i < u32count; i++)
        {
            ECG_ArrhythmiaUpload((u8)p_ecg_algorithm_out->stResult.pAlertList->alertList[i].type);
        }
    }
#endif
}

static int ecg_algorithm_result(void)
{
    int i32CECGOutItemNum = 0;
    static u32 u32QuickQRSTick = 0;
    i32CECGOutItemNum = l_ecg_PopAlgorithmData((void *)p_ecg_algorithm_out, 1);
    if (i32CECGOutItemNum)//获取到有结果
    {
        ecg_algorithm_PushFilterResult();   //保存滤波后数据
        ecg_algo_refreshAlertType();
        if ((0 != p_ecg_algorithm_out->stResult.u32QuickQRSTick) \
            && (u32QuickQRSTick != p_ecg_algorithm_out->stResult.u32QuickQRSTick))
        {
            ALG_ECG_DBG("QRS:%d,CUR:%d",
                        p_ecg_algorithm_out->stResult.u32QuickQRSTick,
                        SysTick_Get());
            u32QuickQRSTick = p_ecg_algorithm_out->stResult.u32QuickQRSTick;
            ECG_SetDefibrillate();
        }
        ALG_ECG_DBG("BR(%d)ST(%d)C(%d)",
            p_ecg_algorithm_out->stResult.u32HeartRate,
            p_ecg_algorithm_out->stResult.nSTAmpDif,
            p_ecg_algorithm_out->stResult.nHRConfidence);
    }
    return i32CECGOutItemNum;
}

void ecg_algorithm_handler(void)
{
    if (false == g_ecg_algo_sw) return;
    
    u32 u32Tick = 0;
    char pace = 0;
    s32 s32I = 0, s32II = 0, s32V = 0;
#ifdef _ALG_ECG_RUN_TIME_
    u32 u32curtick;
#endif
    s32 s32Len = ECG_PopAlgoResult(&u32Tick, &s32I, &s32II, &s32V, &pace);
    if (s32Len)//有数据更新
    {
#ifdef _ALG_ECG_RUN_TIME_
        u32curtick = SysTick_Get();
#endif
        g_Is_ecg_algo_run = true;
        l_ecg_PushAlgorithmData(&u32Tick, &s32I, &s32II, &s32V, &pace);
        ecg_algorithm_result();
        g_Is_ecg_algo_run = false;
#ifdef _ALG_ECG_RUN_TIME_
        AIO_printf("\r\nECG ALGO RUN TICK =\t%d\tHR=%d",
                        SysTick_Get()-u32curtick,
                        (u16)p_ecg_algorithm_out->stResult.u32HeartRate);
#endif
    }
}

int ecg_algorithm_getHR(u16 *pHR)
{
    *pHR = (u16)p_ecg_algorithm_out->stResult.u32HeartRate;
    return 1;
}

void ecg_algorithm_setRun(u8 state)
{
    if (!state)
    {
        g_ecg_algo_sw = false;
    }
    else
    {
        g_ecg_algo_sw = true;
    }
}

//<!-- ECG Filter Data Begin -->
static void ecg_algorithm_PushFilterResult(void)
{
    g_ecg_filTick[g_ecg_fil_offset_in] = p_ecg_algorithm_out->stDataFilter.u32SysTick;
    g_ecg_filResult[g_ecg_fil_offset_in][0] = p_ecg_algorithm_out->stDataFilter.s32ECG1;
    g_ecg_filResult[g_ecg_fil_offset_in][1] = p_ecg_algorithm_out->stDataFilter.s32ECG2;
    g_ecg_filResult[g_ecg_fil_offset_in][2] = p_ecg_algorithm_out->stDataFilter.s32ECG3;
    g_ecg_filPace[g_ecg_fil_offset_in] = p_ecg_algorithm_out->stDataFilter.pace;

    if(++g_ecg_fil_offset_in >= ECG_Sampling_Buffer_Len)
        g_ecg_fil_offset_in -= ECG_Sampling_Buffer_Len;
    if(g_ecg_fil_offset_out == g_ecg_fil_offset_in)    // overflow
    {
        if(++g_ecg_fil_offset_out >= ECG_Sampling_Buffer_Len)
            g_ecg_fil_offset_out -= ECG_Sampling_Buffer_Len;
    }
}

static u16 ecg_algorithm_GetDataLen(void)
{
    s32 s32Len = g_ecg_fil_offset_in - g_ecg_fil_offset_out;
    if(s32Len < 0)  s32Len += ECG_Sampling_Buffer_Len;
    return (u16)s32Len;
}

//@return : 0 -- no data 1 -- have data
int ecg_algorithm_PopFilterResult(u32* pu32Tick, s32* ps32I, s32* ps32II, s32* ps32V, char* pPace)
{
    u16 s32Len = ecg_algorithm_GetDataLen();

    if(s32Len)
    {
        *pu32Tick = g_ecg_filTick[g_ecg_fil_offset_out];
        *ps32I = g_ecg_filResult[g_ecg_fil_offset_out][0];
        *ps32II = g_ecg_filResult[g_ecg_fil_offset_out][1];
        *ps32V = g_ecg_filResult[g_ecg_fil_offset_out][2];
        *pPace = g_ecg_filPace[g_ecg_fil_offset_out];

        if(++g_ecg_fil_offset_out >= ECG_Sampling_Buffer_Len)
            g_ecg_fil_offset_out -= ECG_Sampling_Buffer_Len;
    }
    return s32Len;
}

void ecg_algorithm_DataReset(void)
{
    g_ecg_fil_offset_out = g_ecg_fil_offset_in;
}

//<!-- ECG Filter Data End -->

