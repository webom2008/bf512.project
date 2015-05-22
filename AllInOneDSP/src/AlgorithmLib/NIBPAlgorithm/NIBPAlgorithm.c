/*****************************************************************************
 * NIBPAlgorithm.c
 *****************************************************************************/
#include "NIBPAlgorithm_conf.h"

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->
#pragma section("sdram0_bank1")
void l_nibp_AlgorithmInit(void);
#pragma section("sdram0_bank1")
void l_nibp_pushLastRet(int *pDP, int *pSP, int *pBPM);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

void l_nibp_AlgorithmInit(void)
{
    NIBPInitV1000();
}

void l_nibp_pushLastRet(int *pDP, int *pSP, int *pBPM)
{
    p_nibpAlgoIn->lastDP = *pDP;
    p_nibpAlgoIn->lastSP= *pSP;
    p_nibpAlgoIn->lastBPM= *pBPM;
}

int l_nibp_PushAlgorithmData(u32* pu32Tick, u32* pu32mmHg, u16* pu32mmHgRising)
{
    p_nibpAlgoIn->u32mmHgPre = p_nibpAlgoIn->u32mmHgCur;
    p_nibpAlgoIn->u32mmHgCur = *pu32mmHg;
    p_nibpAlgoIn->u32mmHgRising = *pu32mmHgRising;

    if ((STEP_RELE_ADC_TAG == p_nibpAlgoIn->u32mmHgCur) \
        && (STEP_RELE_ADC_TAG != p_nibpAlgoIn->u32mmHgPre))
    {
        p_nibpAlgoIn->isOneStepDataReady = true;
    }

    if (STEP_RELE_ADC_TAG == p_nibpAlgoIn->u32mmHgCur)
    {
        NIBPRunV1000();
        return 0;
    }
    
    if (p_nibpAlgoOut->data.stDataRaw.u32DataNum < p_nibpAlgoOut->data.stDataRaw.u32DataMax)
	{
		// 未满，塞进队尾
        p_nibpAlgoOut->data.stDataRaw.pData[p_nibpAlgoOut->data.stDataRaw.u32DataNum] = p_nibpAlgoIn->u32mmHgCur;
        p_nibpAlgoOut->data.stDataRaw.u32DataNum++;
	}
	else
	{
		// 满了，要去掉一个再塞进队尾
        u32 i;
        u32 length = p_nibpAlgoOut->data.stDataRaw.u32DataMax - 1;
		for (i = 0; i < length; i++)
		{
			p_nibpAlgoOut->data.stDataRaw.pData[i] = p_nibpAlgoOut->data.stDataRaw.pData[i + 1];
		}
        p_nibpAlgoOut->data.stDataRaw.pData[length] = p_nibpAlgoIn->u32mmHgCur;
	}
	return 0;
}

// 获取计算结果
int l_nibp_PopAlgorithmData(void *pResult)
{
    *(CNIBPResult *)pResult = p_nibpAlgoOut->nibpResult;
    return 0;
}

int l_nibp_SetAlgorithmPatient(char newType)
{
    g_nibp_patient_cur = newType;
    return 0;
}

