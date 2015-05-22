/*****************************************************************************
 * ECGAlgorithm.c
 *****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "ECGAlgorithm_conf.h"
#include "ECGAlgorithm.h"

//#define ECG_INPUT_DATA_REVERSE

static void CalcOtherChannels(CECGOut* pData, u32 index)
{
	// III = II - I  
	pData->pData[index].stDataRaw.s32ECG4 = pData->pData[index].stDataRaw.s32ECG2 - pData->pData[index].stDataRaw.s32ECG1;
	// aVR = -(I+II)/2
	pData->pData[index].stDataRaw.s32ECG5 = 0 - (pData->pData[index].stDataRaw.s32ECG1 + pData->pData[index].stDataRaw.s32ECG2) / 2;
	// aVL = (I-III)/2
	pData->pData[index].stDataRaw.s32ECG6 = (pData->pData[index].stDataRaw.s32ECG1 - pData->pData[index].stDataRaw.s32ECG4) / 2;
	// aVF = (II+III)/2
	pData->pData[index].stDataRaw.s32ECG7 = (pData->pData[index].stDataRaw.s32ECG2 + pData->pData[index].stDataRaw.s32ECG4) / 2;
}

static s32 ChangeToVol(s32 data)
{
	return (s32)((float)0.298023223876953125 * (float)data);// 单位是微伏
}

void l_ecg_AlgorithmInit(void)
{
    ECGAlgoInit(ECG_ALGORITHM_INPUT_FS);
}

int l_ecg_PushAlgorithmData(u32* pu32Tick, s32* ps32I, s32* ps32II, s32* ps32V, char* pPace)
{
    pST_DataIn->s32ECG1 = *ps32I;
    pST_DataIn->s32ECG2 = *ps32II;
    pST_DataIn->s32ECG3 = *ps32V;
    pST_DataIn->u32SysTick = *pu32Tick;
    pST_DataIn->pace = *pPace;

	if (pST_DataOut->u32ECGDataNum < pST_DataOut->u32ECGDataMax)
	{
		// 未满，塞进队尾
#ifdef ECG_INPUT_DATA_REVERSE
		pST_DataOut->pData[pST_DataOut->u32ECGDataNum].stDataRaw.s32ECG1 = ChangeToVol(0 - pST_DataIn->s32ECG1);//取数据的翻转
		pST_DataOut->pData[pST_DataOut->u32ECGDataNum].stDataRaw.s32ECG2 = ChangeToVol(0 - pST_DataIn->s32ECG2);
		pST_DataOut->pData[pST_DataOut->u32ECGDataNum].stDataRaw.s32ECG3 = ChangeToVol(0 - pST_DataIn->s32ECG3);
#else
        pST_DataOut->pData[pST_DataOut->u32ECGDataNum].stDataRaw.s32ECG1 = ChangeToVol(pST_DataIn->s32ECG1);
        pST_DataOut->pData[pST_DataOut->u32ECGDataNum].stDataRaw.s32ECG2 = ChangeToVol(pST_DataIn->s32ECG2);
        pST_DataOut->pData[pST_DataOut->u32ECGDataNum].stDataRaw.s32ECG3 = ChangeToVol(pST_DataIn->s32ECG3);
#endif
		pST_DataOut->pData[pST_DataOut->u32ECGDataNum].stDataRaw.u32SysTick = pST_DataIn->u32SysTick;
        pST_DataOut->pData[pST_DataOut->u32ECGDataNum].stDataRaw.pace = pST_DataIn->pace;

		CalcOtherChannels(pST_DataOut, pST_DataOut->u32ECGDataNum);

		pST_DataOut->u32ECGDataNum++;
	}
	else
	{
		// 满了，要去掉一个再塞进队尾
		int length = pST_DataOut->u32ECGDataMax - 1;
        u32 i = 0;
		for (i = 0; i < length; i++)
		{
			pST_DataOut->pData[i] = pST_DataOut->pData[i + 1];
		}
		memset(pST_DataOut->pData+length, 0, sizeof(CECG_Out_Item));
#ifdef ECG_INPUT_DATA_REVERSE
		pST_DataOut->pData[length].stDataRaw.s32ECG1 = ChangeToVol(0 - pST_DataIn->s32ECG1);
		pST_DataOut->pData[length].stDataRaw.s32ECG2 = ChangeToVol(0 - pST_DataIn->s32ECG2);
		pST_DataOut->pData[length].stDataRaw.s32ECG3 = ChangeToVol(0 - pST_DataIn->s32ECG3);
#else
		pST_DataOut->pData[length].stDataRaw.s32ECG1 = ChangeToVol(pST_DataIn->s32ECG1);
		pST_DataOut->pData[length].stDataRaw.s32ECG2 = ChangeToVol(pST_DataIn->s32ECG2);
		pST_DataOut->pData[length].stDataRaw.s32ECG3 = ChangeToVol(pST_DataIn->s32ECG3);
#endif
		pST_DataOut->pData[length].stDataRaw.u32SysTick = pST_DataIn->u32SysTick;
		pST_DataOut->pData[length].stDataRaw.pace = pST_DataIn->pace;

		CalcOtherChannels(pST_DataOut, length);
	}
    
	ECGAlgoRun();
	return 0;
}

// 获取计算结果，返回计算结果个数，-1: 错误
int l_ecg_PopAlgorithmData(void* pBuf, int i32CECGOutItemNum)
{
    int i = 0;
	int i32CpyLen = my_min(i32CECGOutItemNum, pST_DataOut->u32ECGDataNum);
	// 抄送
	for (i = 0; i < i32CpyLen; i++)
	{
		((CECG_Out_Item*)pBuf)[i] = pST_DataOut->pData[i];
	}
	// 删掉队列中被拿掉的数据
	pST_DataOut->u32ECGDataNum -= i32CpyLen;
	for (i = 0; i < pST_DataOut->u32ECGDataNum; i++)
	{
		pST_DataOut->pData[i] = pST_DataOut->pData[i + i32CpyLen];
	}
	return i32CpyLen;
}

int l_ecg_DebugInterface(ECG_ALG_DEBUG_TypeDef type, void* pBuf, unsigned char nLen)
{
    return ECGV1000DebugInterface(type, pBuf, nLen);
}


