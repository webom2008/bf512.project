/*****************************************************************************
 * RESPAlgorithm.c
 *****************************************************************************/
#if 1//_RESP_ALGORITHM_C_
#include "RESPAlgorithm_conf.h"
#include <string.h>

//#define RESP_INPUT_DATA_REVERSE

#define RESP_ALGORITHM_INPUT_FS         (100)
#define RESP_ALGORITHM_INPUT_MAX_TIME   (60) // 输入缓冲大小
#define RESP_ALGORITHM_INPUT_MAX_LEN    (RESP_ALGORITHM_INPUT_MAX_TIME * RESP_ALGORITHM_INPUT_FS)
#define RESP_ALGORITHM_OUTPUT_FS_DIVI   (1) //必须使用整数
#define RESP_ALGORITHM_OUTPUT_FS        (RESP_ALGORITHM_INPUT_FS / RESP_ALGORITHM_OUTPUT_FS_DIVI)
#define RESP_ALGORITHM_OUTPUT_MAX_TIME  (1)
#define RESP_ALGORITHM_OUTPUT_MAX_LEN   (RESP_ALGORITHM_OUTPUT_MAX_TIME * RESP_ALGORITHM_OUTPUT_FS)

#define	RESP_UPDATE_TIME	            (0.5) // 每隔多长时间计算一次
#define	RESP_UPDATE_DATA_LEN	        (RESP_UPDATE_TIME * RESP_ALGORITHM_INPUT_FS)
#define	RESP_CAL_TIME_LEN	            (20) // 每次计算使用多长时间的数据
#define	RESP_CAL_DATA_LEN	            (RESP_CAL_TIME_LEN * RESP_ALGORITHM_INPUT_FS)


#pragma section("sdram0_bank2")
int pRESP_Cal_Buf[RESP_CAL_DATA_LEN]; // 滤波后数据队列
u32 nRESP_Calc_Data_Num; // 数据长度
#pragma section("sdram0_bank2")
CRESPOut g_respAlgoOut;
#pragma section("sdram0_bank2")
CRESPIn g_respAlgoIn;
CRESPOut* p_respAlgoOut = &g_respAlgoOut;
CRESPIn* p_respAlgoIn = &g_respAlgoIn;
#pragma section("sdram0_bank2")
CRESP_Out_Item p_respOutData[RESP_ALGORITHM_OUTPUT_MAX_LEN];
#pragma section("sdram0_bank2")
CRESPAlertList resp_AlertList;


//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->
#pragma section("sdram0_bank2")
void l_resp_AlgorithmInit(void);
#pragma section("sdram0_bank2")
int l_resp_PushAlgorithmData(u32* pu32Tick, s32* ps32resp);
#pragma section("sdram0_bank2")
int l_resp_PopAlgorithmData(void* pBuf, int i32CRESPOutItemNum);
#pragma section("sdram0_bank2")
int l_resp_DebugInterface(RESP_ALG_DEBUG_TypeDef type, char* pBuf, unsigned char nLen);
#pragma section("sdram0_bank2")
void MainInit(void);
#pragma section("sdram0_bank2")
void MainRun(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

static s32 ChangeToVol(s32 data)
{
	return (s32)((float)0.298023223876953125 * (float)data);// 单位是微伏
}

//static void AlertList_Clear(CRESPAlertList* pList)
//{
//	pList->u32AlertCount = 0;
//}

void l_resp_AlgorithmInit(void)
{
    //RESPInitV1000();
    MainInit();
}

int l_resp_PushAlgorithmData(u32* pu32Tick, s32* ps32resp)
{
    p_respAlgoIn->s32RESP = *ps32resp;
    p_respAlgoIn->u32SysTick = *pu32Tick;

    if (p_respAlgoOut->u32RESPDataNum < p_respAlgoOut->u32RESPDataMax)
	{
		// 未满，塞进队尾
#ifdef RESP_INPUT_DATA_REVERSE
        p_respAlgoOut->pData[p_respAlgoOut->u32RESPDataNum].stDataRaw.s32RESP = \
        ChangeToVol(0 - p_respAlgoIn->s32RESP);
#else
        p_respAlgoOut->pData[p_respAlgoOut->u32RESPDataNum].stDataRaw.s32RESP = \
        ChangeToVol(p_respAlgoIn->s32RESP);
#endif
        p_respAlgoOut->pData[p_respAlgoOut->u32RESPDataNum].stDataRaw.u32SysTick = \
        p_respAlgoIn->u32SysTick;
        p_respAlgoOut->u32RESPDataNum++;
	}
	else
	{
		// 满了，要去掉一个再塞进队尾
        u32 i;
        u32 length = p_respAlgoOut->u32RESPDataMax - 1;
		for (i = 0; i < length; i++)
		{
			p_respAlgoOut->pData[i] = p_respAlgoOut->pData[i + 1];
		}
        memset(p_respAlgoOut->pData+length, 0, sizeof(CRESP_Out_Item));
#ifdef RESP_INPUT_DATA_REVERSE
        p_respAlgoOut->pData[length].stDataRaw.s32RESP = ChangeToVol(0 - p_respAlgoIn->s32RESP);
#else
        p_respAlgoOut->pData[length].stDataRaw.s32RESP = ChangeToVol(p_respAlgoIn->s32RESP);
#endif
		p_respAlgoOut->pData[length].stDataRaw.u32SysTick = p_respAlgoIn->u32SysTick;
	}
    
//    AlertList_Clear(&resp_AlertList);
    
	//RESPRunV1000();
	MainRun();
    
	return 0;
}

// 获取计算结果，返回计算结果个数，-1: 错误
int l_resp_PopAlgorithmData(void* pBuf, int i32CRESPOutItemNum)
{
    int i32CpyLen = my_min(i32CRESPOutItemNum, p_respAlgoOut->u32RESPDataNum);
    int i;
    
	// 抄送
	for (i = 0; i < i32CpyLen; i++)
	{
        ((CRESP_Out_Item*)pBuf)[i] = p_respAlgoOut->pData[i];
	}
    
	// 删掉队列中被拿掉的数据
    p_respAlgoOut->u32RESPDataNum -= i32CpyLen;
    for (i = 0; i < p_respAlgoOut->u32RESPDataNum; i++)
	{
		p_respAlgoOut->pData[i] = p_respAlgoOut->pData[i + i32CpyLen];
	}
    
	return i32CpyLen;
}

int l_resp_DebugInterface(RESP_ALG_DEBUG_TypeDef type, char* pBuf, unsigned char nLen)
{
    return RespAlgoDebugInterface(type, pBuf, nLen);
}

static void MainInit(void)
{
    p_respAlgoOut->u32RESPDataMax = RESP_ALGORITHM_OUTPUT_MAX_LEN;
    p_respAlgoOut->pData = p_respOutData;
    p_respAlgoOut->u32RESPDataNum = 0;

    memset(p_respOutData, 0, (sizeof(CRESP_Out_Item) * RESP_ALGORITHM_OUTPUT_MAX_LEN));
    memset(&resp_AlertList, 0, sizeof(CRESPAlertList));

	nRESP_Calc_Data_Num = RESP_CAL_DATA_LEN - RESP_UPDATE_DATA_LEN;
    memset(pRESP_Cal_Buf, 0, (sizeof(int) * RESP_CAL_DATA_LEN));

    RespAlgoInit();

}

static void MainRun(void)
{
	RespAlgoRun((void*)p_respAlgoIn,
              pRESP_Cal_Buf,
              RESP_CAL_DATA_LEN,
              &nRESP_Calc_Data_Num,
              RESP_UPDATE_DATA_LEN,
              RESP_ALGORITHM_INPUT_FS,
              (void*)p_respAlgoOut,
              &resp_AlertList);
}

#endif //_RESP_ALGORITHM_C_

