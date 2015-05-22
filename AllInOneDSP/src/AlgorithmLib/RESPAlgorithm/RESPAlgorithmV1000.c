/*****************************************************************************
 * RESPAlgorithmV1000.c
 *****************************************************************************/
#if 1//_RESP_ALGORITHM_V1000_C_
#include "RESPAlgorithmV1000.h"

#include <string.h>

#define RESP_ALGORITHM_INPUT_FS         (100)
#define RESP_ALGORITHM_INPUT_MAX_TIME   (60) // 输入缓冲大小
#define RESP_ALGORITHM_INPUT_MAX_LEN    (RESP_ALGORITHM_INPUT_MAX_TIME * RESP_ALGORITHM_INPUT_FS)
#define RESP_ALGORITHM_OUTPUT_FS_DIVI   (1) //必须使用整数
#define RESP_ALGORITHM_OUTPUT_FS        (RESP_ALGORITHM_INPUT_FS / RESP_ALGORITHM_OUTPUT_FS_DIVI)
#define RESP_ALGORITHM_OUTPUT_MAX_TIME  (1)
#define RESP_ALGORITHM_OUTPUT_MAX_LEN   (RESP_ALGORITHM_OUTPUT_MAX_TIME * RESP_ALGORITHM_OUTPUT_FS)

#define RESP_SPECTRUM_WINDOW_TIME       (5)       
#define RESP_SPECTRUM_WINDOW_LEN        (RESP_SPECTRUM_WINDOW_TIME * RESP_ALGORITHM_OUTPUT_FS)

#define	RESP_UPDATE_TIME	            (0.5) // 每隔多长时间计算一次
#define	RESP_UPDATE_DATA_LEN	        (RESP_UPDATE_TIME * RESP_ALGORITHM_INPUT_FS)
#define	RESP_CAL_TIME_LEN	            (20) // 每次计算使用多长时间的数据
#define	RESP_CAL_DATA_LEN	            (RESP_CAL_TIME_LEN * RESP_ALGORITHM_INPUT_FS)
#define	RESP_FORM_DATA_TIME_LEN		    (60) // 维护识别出来的波形的时间长度
#define	RESP_FORM_DATA_MAX_NUM_PER_SEC  (20) // 最大心率为150时，每次心跳最多8个波形，每秒最多有20个波形
#define	RESP_FORM_DATA_MAX_LEN		    (RESP_FORM_DATA_TIME_LEN * RESP_FORM_DATA_MAX_NUM_PER_SEC)	// 可以保存的波形的最大个数

#define RESP_RATE_LIST_MAX_LEN          (60)
#define RESP_TF_MAX_LEN                 (100) // 三角波列表长度
#define RESP_bbCube_MAX_LEN             (50)

#define DEF_MY_EXP_FUNC
#ifdef DEF_MY_EXP_FUNC
#else
#include <math.h>
#endif

/*****************************************************************************
 * 输入数据
 *****************************************************************************/
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
int pRESP_Cal_Buf[RESP_CAL_DATA_LEN]; // 滤波后数据队列
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
u32 nRESP_Calc_Data_Num; // 数据长度
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
CRESPOut g_respAlgoOut;
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
CRESPIn g_respAlgoIn;
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
CRESPOut* p_respAlgoOut = &g_respAlgoOut;
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
CRESPIn* p_respAlgoIn = &g_respAlgoIn;
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
CRESP_Out_Item p_respOutData[RESP_ALGORITHM_OUTPUT_MAX_LEN];
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
CRESPAlertList resp_AlertList;

#ifndef __ADSPBF512__
CRESPLabelInfoList resp_LabelList; //标注信息列表
CRESPLabelInfo p_resp_Label[RESP_FORM_DATA_MAX_LEN];
#endif

/*****************************************************************************
 * 过程数据
 *****************************************************************************/
float TriAmp; //历史阈值
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
CTriangle_Form respRUN_TF[RESP_TF_MAX_LEN];	// 三角波列表
int RespRate; //呼吸率
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
float hisRespRate[RESP_RATE_LIST_MAX_LEN];  //呼吸率队列
int hisRespRateCount;

//=============================================================
//RESP Filter data
static const float HrB_Newborn[2] = {-1.86545, 0.86776};
static const float HrB1_Newborn[3] = {1, 0, -1};
static const float HrGainNewborn = 0.06612;

static const float HrB_Adult[2] = {-1.85861, 0.85953};
static const float HrB1_Adult[3] = {1, 0, -1};
static const float HrGainAdult = 0.07024;


#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
static FILTER_BUFFER_1 RespFilterBuffer;
//=============================================================

/*****************************************************************************
 * CalcRESPRate()临时变量
 *****************************************************************************/
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
int p_calcresprate_y[RESP_TF_MAX_LEN];
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
int p_calcRR_bbCubeSum[RESP_bbCube_MAX_LEN];
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
int p_calcRR_bbCubeCount[RESP_bbCube_MAX_LEN];

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
void RESPInitV1000(void);
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
void RESPRunV1000(void);
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
u32 FindTriangleForm(int* pData,// pData是波形数据，被找三角波的对象
                    u32 nDataLen,
                    float threshold,// threshold是找三角波的幅度阈值
                    CTriangle_Form* pTFList,// pTFList是找到的三角波存放的列表
                    u32 nMaxLen);// nMaxLen是三角波列表的最大长度
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
float CalcRESPRate(CTriangle_Form* pTF, int nTFCount, int Fs);
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
float CalcRESPRate1(float* pHisRespRate, int nHisRespRateCount, float r);
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
int AlertList_Append(CRESPAlertList* pList, CRESPAlertItem item);
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
int	resp_newborn_filter(FILTER_BUFFER_1* pFt, int wave);
#ifdef __ADSPBF512__
#pragma section("sdram0_bank1")
#endif
int	resp_adult_filter(FILTER_BUFFER_1* pFt, int wave);

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

/******************************************************************************/

#ifdef DEF_MY_EXP_FUNC
static double myexp1(double x)  //泰勒展开公式
{
    int i,k,m,t;
    int xm = (int)x;
    double sum;
    double e;
    double ef;
    double z;
    double sub = x-xm;
    
    m = 1;      //阶乘算法分母
    e = 1.0;    //e的xm
    ef = 1.0;  
    t = 10;     //算法精度
    z = 1;      //分子初始化
    sum=1;
    
    if (xm < 0) //判断xm是否大于0？
    {     
        xm = (-xm); 
        for(k=0; k < xm; k++)
        {
            ef *= 2.718281;
        }
        e /= ef;
    } 
    else
    {
        for(k=0; k<xm; k++)
        {
            e *= 2.718281;
        }
    }
    
    for(i=1; i<t; i++)
    {
        m *= i; 
        z *= sub;  
        sum += z/m;
    }
    
    return sum*e;
}

static double myexp2(double x)//计算e^x,实现系统的exp()功能 
{
    if(x == 0) return 1;
    if(x < 0) return 1/myexp2(-x);
    
    double y=x,ex_p1=0,ex_p2=0,ex_p3=0,ex_p=0,ex_px=0,ex_tmp=1,dex_px=1,tmp;
    int l;
    
    for(l=1, tmp=1; ((ex_px - ex_tmp) > 1e-10 || (ex_px-ex_tmp) < -1e-10) && dex_px > 1e-10; l++)
    {
        ex_tmp = ex_px;
        tmp *= y;
        tmp = tmp / l;
        ex_p1 += tmp;
        ex_p2 = ex_p1 + tmp * y / (l+1);
        ex_p3 = ex_p2 + tmp * y * y / (l+1) / (l+2);
        dex_px = ex_p3 - ex_p2;
        ex_px = ex_p3 - dex_px * dex_px / (ex_p3 - 2*ex_p2 + ex_p1);
    }
    return ex_px+1;
}
#endif

static float FcoeH(float x, float B, float C)
{
	// 算阈值遗传系数
	float a = 2 - 2 * B;
	float y;
#ifdef DEF_MY_EXP_FUNC
	float p1 = 1.0 / (1.0 + myexp1((0.0 - C) * x));
#else
	float p1 = 1.0 / (1.0 + exp((0.0 - C) * x));
#endif
	y = 1.0 - a + a * p1;
	return y;
}

// 新生儿模式
// IIR，ButterWorth, BandPass
// Fs:	     100Hz
// FC1:	     0.25Hz   
// FC2:       2.5Hz
// Mag:		3dB
// Order:		2
// Sections:	1
static int	resp_newborn_filter(FILTER_BUFFER_1* pFt, int wave)
{
	int j;
    float X,Y,T;
    int F;

	X = (float)wave;

	for(j=2; j>0; j--)
	{
        pFt->fHrX[j] = pFt->fHrX[j-1];
	}
    pFt->fHrX[0] = X;
    Y = HrB_Newborn[0] * pFt->fHrY[0] + HrB_Newborn[1] * pFt->fHrY[1];
    X = (HrB1_Newborn[0] * pFt->fHrX[0] + HrB1_Newborn[1] * pFt->fHrX[1] + \
        HrB1_Newborn[2] * pFt->fHrX[2]) * HrGainNewborn;
	T = X - Y;
		
	for(j=2; j>0; j--)
	{
        pFt->fHrY[j] = pFt->fHrY[j-1];
	}
    pFt->fHrY[0] = T;
	X = T;
	
    F = (int)T;
	
	return F;
}

// 成人模式
// IIR，ButterWorth, BandPass
// Fs:	     100Hz
// FC1:	     0.1Hz   
// FC2:       2.5Hz
// Mag:		3dB
// Order:		2
// Sections:	1
static int	resp_adult_filter(FILTER_BUFFER_1* pFt, int wave)
{
	int j;
    float X,Y,T;
    int F;

	X = (float)wave;

	for(j=2; j>0; j--)
	{
        pFt->fHrX[j] = pFt->fHrX[j-1];
	}
    pFt->fHrX[0] = X;
    Y = HrB_Adult[0] * pFt->fHrY[0] + HrB_Adult[1] * pFt->fHrY[1];
    X = (HrB1_Adult[0] * pFt->fHrX[0] + HrB1_Adult[1] * pFt->fHrX[1] + \
        HrB1_Adult[2] * pFt->fHrX[2]) * HrGainAdult;
	T = X - Y;
		
	for(j=2; j>0; j--)
	{
        pFt->fHrY[j] = pFt->fHrY[j-1];
	}
    pFt->fHrY[0] = T;
	X = T;
	
    F = (int)T;
	
	return F;
}

void RESPInitV1000(void)
{
    p_respAlgoOut->u32RESPDataMax = RESP_ALGORITHM_OUTPUT_MAX_LEN;
    p_respAlgoOut->pData = p_respOutData;
    p_respAlgoOut->u32RESPDataNum = 0;

    memset(p_respOutData, 0, (sizeof(CRESP_Out_Item) * RESP_ALGORITHM_OUTPUT_MAX_LEN));
    
	resp_AlertList.u32AlertCount = 0;//告警信息列表
	memset(&resp_AlertList, 0, sizeof(CRESPAlertList));
    
#ifndef __ADSPBF512__
    memset(p_resp_Label, 0, (sizeof(CRESPLabelInfo) * RESP_FORM_DATA_MAX_LEN));
	resp_LabelList.nLabelNum = 0;
	resp_LabelList.pList = p_resp_Label;
#endif

    memset(pRESP_Cal_Buf, 0, (sizeof(int) * RESP_CAL_DATA_LEN));
	nRESP_Calc_Data_Num = RESP_CAL_DATA_LEN - RESP_UPDATE_DATA_LEN;

	TriAmp = 5000;
	hisRespRateCount = 0;
	RespRate = 0;
    memset(hisRespRate, 0, sizeof(float)*RESP_RATE_LIST_MAX_LEN);
}

void RESPRunV1000(void)
{
    int index = p_respAlgoOut->u32RESPDataNum - 1; //最新的一组输入数据
    
	// 数据经过滤波
    //p_respAlgoOut->pData[index].stDataFilter.s32RESP = p_respAlgoOut->pData[index].stDataRaw.s32RESP;
    if (0 == p_respAlgoIn->filterMode) //newborn mode
    {
        p_respAlgoOut->pData[index].stDataFilter.s32RESP = \
            resp_newborn_filter(&RespFilterBuffer, p_respAlgoOut->pData[index].stDataRaw.s32RESP);
    }
    else //adult mode
    {
        p_respAlgoOut->pData[index].stDataFilter.s32RESP = \
            resp_adult_filter(&RespFilterBuffer, p_respAlgoOut->pData[index].stDataRaw.s32RESP);
    }
	p_respAlgoOut->pData[index].stDataFilter.u32SysTick = p_respAlgoOut->pData[index].stDataRaw.u32SysTick;

	// 滤波后，塞进buffer
    pRESP_Cal_Buf[nRESP_Calc_Data_Num++] = p_respAlgoOut->pData[index].stDataFilter.s32RESP;
    
	if (nRESP_Calc_Data_Num >= RESP_CAL_DATA_LEN)
    {
		// 算当前幅度阈值
		int cLen = RESP_ALGORITHM_INPUT_FS * 4;	
		int delta = Max_N(&pRESP_Cal_Buf[nRESP_Calc_Data_Num - cLen], cLen) \
            - Min_N(&pRESP_Cal_Buf[nRESP_Calc_Data_Num - cLen], cLen);
		float curAmp = 0.3 * delta;
		
		// 当前阈值和历史阈值的比率
		float di = my_abs(curAmp - TriAmp) / TriAmp;

		// 历史阈值的遗传系数
		float coeH = FcoeH(di, 0.98, 2);

		// 更新幅度阈值
		TriAmp = (1.0 - coeH) * curAmp + coeH * TriAmp;

		// 找三角波
		int tfCount = FindTriangleForm( pRESP_Cal_Buf,
		                                nRESP_Calc_Data_Num,
		                                TriAmp,
		                                respRUN_TF,
		                                RESP_TF_MAX_LEN);

		// 算当前呼吸率
		float curRespRate = CalcRESPRate(respRUN_TF, tfCount, RESP_ALGORITHM_INPUT_FS);

		// 塞进呼吸率队列
		if (RESP_RATE_LIST_MAX_LEN == hisRespRateCount) 
        {
			int i = 0;
			int maxCount = hisRespRateCount - 1;
			for (i = 0; i < maxCount; i++) 
            {
				hisRespRate[i] = hisRespRate[i + 1];
			}
			hisRespRate[hisRespRateCount - 1] = curRespRate;
		}
        else
        {
			hisRespRate[hisRespRateCount] = (curRespRate < 0 || curRespRate > 300) ? 0 : curRespRate;
			hisRespRateCount++;
		}

		// 更新呼吸率
		RespRate = CalcRESPRate1(hisRespRate, hisRespRateCount, 4000) + 0.5;
        //printf("%d\r\n", RespRate);

		p_respAlgoOut->pData[index].stResult.u32RESPRate = RespRate;

		// 去掉一段数据
		int moveLen = RESP_CAL_DATA_LEN - RESP_UPDATE_DATA_LEN;
		int i = 0;
		for (i = 0; i < moveLen; i++)
        {
			pRESP_Cal_Buf[i] = pRESP_Cal_Buf[i + (int)RESP_UPDATE_DATA_LEN];
		}
		nRESP_Calc_Data_Num = moveLen;
	}
    else
    {
		p_respAlgoOut->pData[index].stResult.u32RESPRate = RespRate;
	}
}

// 返回值是三角波的个数
static u32 FindTriangleForm(int* pData,// pData是波形数据，被找三角波的对象
                            u32 nDataLen,
                            float threshold,// threshold是找三角波的幅度阈值
                            CTriangle_Form* pTFList,// pTFList是找到的三角波存放的列表
                            u32 nMaxLen)// nMaxLen是三角波列表的最大长度
{
	if (NULL == pData)
	{
		return 0;
	}

	CCoordiate_Point stCPoint;
	CCoordiate_Point leftLow;
	CCoordiate_Point high;
	CCoordiate_Point rigthLow;
	CTriangle_Form TFTmp;
	int di = 0;
	u32 triCount = 0;
	int jstat = 0;
	CCoordiate_Point jPoint;

	int stat = 0;
	leftLow.u32X = 0;
	leftLow.u32Y = pData[leftLow.u32X];
	high = leftLow;
	rigthLow = leftLow;

    int i, j;
    
	for (i = 0; i < nDataLen; i++)
    {
		if (triCount >= nMaxLen)
        {
			break;
		}

		// 当前点坐标
		stCPoint.u32X = i;
		stCPoint.u32Y = pData[i];

		switch (stat) 
		{
		case 0: // 前一点的状态是三点在同一点上
			if (stCPoint.u32Y <= leftLow.u32Y)
            {
				// 下降
				leftLow = stCPoint;
				high = leftLow;
				rigthLow = leftLow;
			}
            else
            {
				// 上升
				stat = 1;
				high = stCPoint;
				rigthLow = stCPoint;
			}
			break;
		case 1: // 向上找的过程
			if (stCPoint.u32Y >= high.u32Y)
            {
				// 继续上升
				high = stCPoint;
				rigthLow = stCPoint;
			}
            else
            {
				// 向下的拐点
				di = high.u32Y - stCPoint.u32Y;
				if (stCPoint.u32Y > leftLow.u32Y)
                {
					// 下降但没低于左边的低点
					if (di >= threshold)
                    {
						// 找到一个符合三角波的头部，还要继续找到右半部的底部
						stat = 2;
					}
					rigthLow = stCPoint;
				}
                else
                {
					if (di >= threshold && high.u32Y - leftLow.u32Y >= threshold)
                    {
						// 找到一个符合三角波的头部，还要继续找到右半部的底部
						stat = 2;
					}
                    else
                    {
						// 回到三点同一个点的状态
						stat = 0;
						leftLow = stCPoint;
						high = stCPoint;
						rigthLow = stCPoint;
					}
				}
			}
			break;
		case 2: // 继续找右半部底部
			if (stCPoint.u32Y >= rigthLow.u32Y)
            {
				// 找到底部，完成一个三角波的查找
				jstat = 1;
				for (j = high.u32X; j >= 0; j--)
                {
					jPoint.u32X = j;
					jPoint.u32Y = pData[j];
					if (jstat == 1)
					{
						if (high.u32Y - jPoint.u32Y >= threshold)
                        {
							// 找到低于阈值的点，继续向下找到左半部的底部
							jstat = 2;
						}
					}
					else
					{
						if (jPoint.u32Y >= pData[j + 1])
                        {
							// 完成一个三角波的查找
							TFTmp.stLeft.u32X = j + 1;
							TFTmp.stLeft.u32Y = pData[j + 1];
							TFTmp.stPeak = high;
							TFTmp.stRight = rigthLow;
							pTFList[triCount] = TFTmp;
							triCount++;
							break;
						}
					}
				} //end of for (j = high.u32X; j >= 0; j--)
				// 回到三点同一个点的状态
				stat = 0;
				leftLow = stCPoint;
				high = stCPoint;
				rigthLow = stCPoint;
			}
            else
            {
				// 还在下降，继续查找
				rigthLow = stCPoint;
			}
			break;
        default:
            break;
		} //end of switch
	} //end of for (i = 0; i < nDataLen; i++)

	return triCount;
}

static float CalcRESPRate(CTriangle_Form* pTF, int nTFCount, int Fs) 
{
    int i;
	int yCount = nTFCount - 1;
	int blockLen = 100;
	int stepLen = blockLen / 2;
	int stepCount = blockLen * 3 / stepLen;
	int head = 0;
	int tail = 0;
	int summ = 0;
	int count = 0;
	int moveCount = 1;
	int bbCubeC = 0;
	int index = 0;

	// nTFCount 不能超过100
	if (nTFCount == 1 || nTFCount > RESP_TF_MAX_LEN)
    {
		return 0.0;
	}
    else if (nTFCount == 2)
    {
		return 60.0 * Fs / (pTF[1].stPeak.u32X - pTF[0].stPeak.u32X);
	}

	for (i = 0; i < yCount; i++)
    {
		p_calcresprate_y[i] = pTF[i + 1].stPeak.u32X - pTF[i].stPeak.u32X;
	}


	// 统计
	head = Max_N(p_calcresprate_y, yCount);
	tail = head - blockLen;
	while ((moveCount < stepCount) && (bbCubeC < RESP_bbCube_MAX_LEN))
    {
		count = 0;
		summ = 0;
		for (i = 0; i < yCount; i++)
        {
			if ((p_calcresprate_y[i] <= head) && (p_calcresprate_y[i] > tail))
            {
				summ += p_calcresprate_y[i];
				count++;
			}
		}
		p_calcRR_bbCubeSum[bbCubeC] = summ;
		p_calcRR_bbCubeCount[bbCubeC] = count;
		bbCubeC++;

		head -= stepLen;
		tail = head - blockLen;
		moveCount++;
	}

	// 查找最大值
	Max_Index_N(p_calcRR_bbCubeCount, bbCubeC, &index);

	// 算呼吸率
	float rate = 60.0 * Fs / ((float)p_calcRR_bbCubeSum[index] / (float)p_calcRR_bbCubeCount[index]);

	return rate;
}

static float CalcRESPRate1(float* pHisRespRate, int nHisRespRateCount, float r)
{
    float n = 0;
	float a = 0;
	float R = 0;
    int i = 0;
    float rate = 0;

    if(nHisRespRateCount == 1)
    {
        rate = pHisRespRate[0];
    }
    else
    {
	    n = nHisRespRateCount;
	    a = 2.0 / (n + n * r);
	    R = a * (1 - r) / (1.0 - n);
        while (i < nHisRespRateCount)
        {
            rate += (a + R * i) * pHisRespRate[i];
            i = i + 1;
	    }
    }
	return rate;
}


static int AlertList_Append(CRESPAlertList* pList, CRESPAlertItem item)
{
    int i;
	if (pList->u32AlertCount >= RESP_ALERT_MAX_NUM)
	{
		int moveCount = pList->u32AlertCount - 1;
		for (i = 0; i < moveCount; i++)
		{
			pList->alertList[i] = pList->alertList[i + 1];
		}
		pList->u32AlertCount--;
	}

	pList->alertList[pList->u32AlertCount] = item;
	pList->u32AlertCount++;
	return pList->u32AlertCount;
}


#endif //_RESP_ALGORITHM_V1000_C_

