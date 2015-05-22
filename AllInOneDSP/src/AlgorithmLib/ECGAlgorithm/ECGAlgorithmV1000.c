/*****************************************************************************
 * ECGAlgorithmV1000.c
 *****************************************************************************/
#include <string.h>
#include <stdio.h>

#include "ECGAlgorithmV1000.h"

#define	ECG_CAL_TIME_LEN	    10		// 每次计算使用多长时间的数据
#define	ECG_CAL_DATA_LEN	    (ECG_CAL_TIME_LEN * ECG_ALGORITHM_INPUT_FS)
#define	ECG_UPDATE_DATA_LEN	    (8)

#define ST_VOLT_HIS_MAX_LEN		20	// ST历史队列长度
#define LEAD_MAX_LEN		    7	// 导联数
#define LEAD_NEED_CALC_LEN	    3	// 只有前三个导联需要去做滤波，后四个可以算出来

#define PVC_LIST_MAX_LEN	    350	// PVC计数队列长度

typedef struct _ARRHYTHMIA_ANALYSIS_RESULT1
{
    int PVCsCount;
    int RCount;
    int UnKCount;
    float PVCRate;
    bool Is_VENT_FIB;
    bool Is_ASYSTOLE;
    bool Is_VENT_TACH;
    bool Is_PAIR_PVC;
    bool Is_SIGLE_PVC;
    bool Is_TRIGENINY;
    bool Is_BIGENINY;
    bool Is_MISSED_BEAT;
    bool Is_TACHYCARDIA;
    bool Is_R_ON_T;
    bool Is_BRADYCARDIA;
} ARRHYTHMIA_ANALYSIS_RESULT1;

typedef struct RhythmForm 
{
    CTriangle_Form TF[8];
    int label[8];
    int tfCount;
    int mainFormIndex;
} CRhythmForm;

enum TFLabel
{
	TFL_UNKNOWN = 0,
	TFL_R = 1,
	TFL_PVC_LU = 2,
	TFL_PVC_RD = 3,
	TFL_PVC_RU = 4,
	TFL_PVC_LD = 5,
	TFL_Q = 6,
	TFL_S = 7,
	TFL_P = 8,
	TFL_T = 9,
};

typedef struct PVCPoint 
{
    int systick;
} CPVCPoint;

//// template for SinglePVC
static const int SinglePVCTemp1[5] = {1, 2, 1, 1, 1}; 
static const int SinglePVCTemp2[5] = {1, 4, 1, 1, 1}; 
//// template for PairPVC
static const int PairPVCTemp1[5] = {1, 2, 2, 1, 1}; 
static const int PairPVCTemp2[5] = {1, 4, 4, 1, 1}; 
static const int PairPVCTemp3[5] = {1, 2, 4, 1, 1}; 
static const int PairPVCTemp4[5] = {1, 4, 2, 1, 1}; 
//// template for Bigeniny
static const int BigeninyTemp1[5] = {1, 2, 1, 2, 1}; 
static const int BigeninyTemp2[5] = {1, 4, 1, 4, 1}; 
static const int BigeninyTemp3[5] = {1, 2, 1, 4, 1}; 
static const int BigeninyTemp4[5] = {1, 4, 1, 2, 1}; 
//// template for Trigeniny
static const int TrigeninyTemp1[6] = {1, 2, 1, 1, 2, 1}; 
static const int TrigeninyTemp2[6] = {1, 4, 1, 1, 4, 1}; 
static const int TrigeninyTemp3[6] = {1, 2, 1, 1, 4, 1}; 
static const int TrigeninyTemp4[6] = {1, 4, 1, 1, 2, 1}; 


// 可设置的参数 
static u8 WorkMode = 0;	        // 0 -- 诊断模式；1--监护模式；2--手术模式
static u8 PaceMode = 0;	        // 0 -- 给算法的数据不经过PACE抑制, 1 -- 经过PACE抑制
static u8 PaceShowMode = 0;	    // 0 -- 给显示的数据不经过PACE抑制, 1 -- 经过PACE抑制
static u8 NotchMode = 1;	    // 0 -- 关闭陷波；1 -- 50Hz陷波处理；2 -- 60Hz陷波处理；3 -- 50Hz和60Hz同时打开
static int Tachycardia_Limit;   // 心动过速上限
static int Bradycardia_Limit;   // 心动过缓下限
static float ISO_Interval;      // ST段分析的ISO位置
static float ST_Interval;       // ST段分析的ST位置
static u8 u8STAnalysis_sw;      // ST段分析的开关 0 -- 关闭 1 -- 开启(默认)
static u8 u8ArrhythmiaAnalysis_sw;  //心率失常分析的开关 0 -- 关闭 1 -- 开启(默认)

static bool isIntializedRRInterval = false;// 算法初始化标志位

static u8 proccessMode = 0;	        // 算法过程标志位
static bool isFindRwave = false;	// 找到QRS的标志位

static int calcInterval = 0;
static int calcCount = 0;
static int PrePosX = 0;	    // 前一个检查起始点的位置
static int PreQRS = 0;		// 前一个心律波的位置
static int InteDataRiseEdge = 0;    // 积分波上升沿位置
static int InteDataFallEdge = 0;    // 积分波下降沿位置
static int currentRwaveIndex = 0;	// 当前检测到QRS的位置
static int PVCCheckPoint = 0;	    // PVC检测起点
static u32 QuickQRSTick = 0;        // 快速QRS的systick

static int SampleRate = 0;	// 采样率
static int RRInterval1 = 0;	// RR间隔阈值
static float InteWaveThreshold = 0;	// 积分波幅度阈值
static float thresholdIy = 0;       // 边缘检测（斜率检测法）的积分波阈值
static float thresholdRawData = 0;  // 边缘检测（斜率检测法）的原波形阈值
static float RWaveThreshold1 = 0;
static float RWaveThreshold2 = 0;
static float preInteThres = 0;	// 边缘检测（幅度检测法），前一个上升沿检测的阈值，用于接下来的下降沿检测
static float RHighThreshold = 0;	// R波检测幅度阈值
static float fAmpThreshold;	// 原波形三角波检测阈值
static float Asystole_Time_Limit; // 停搏检测时间阈值

#pragma section("sdram0_bank1")
static float IntegrationBuffer[ECG_CAL_DATA_LEN];// 积分波缓存

// 输入数据
#pragma section("sdram0_bank1")
static int pECG_Cal_Buf[LEAD_MAX_LEN][ECG_CAL_DATA_LEN];	// 算法用的数据
#pragma section("sdram0_bank1")
static u32 pECG_SysTick[ECG_CAL_DATA_LEN]; // systick队列
u32 nECG_Calc_Data_Num;             // 数据长度

// 过程缓存
#pragma section("sdram0_bank1")
static FILTER_BUFFER_1 FilterNotch50[LEAD_NEED_CALC_LEN];	// 50Hz陷波缓存器，算法用
#pragma section("sdram0_bank1")
static FILTER_BUFFER_1 FilterNotch60[LEAD_NEED_CALC_LEN];	// 60Hz陷波缓存器，算法用
#pragma section("sdram0_bank1")
static FILTER_BUFFER_1 FilterNotch50Show[LEAD_NEED_CALC_LEN];	// 50Hz陷波缓存器，显示用
#pragma section("sdram0_bank1")
static FILTER_BUFFER_1 FilterNotch60Show[LEAD_NEED_CALC_LEN];	// 60Hz陷波缓存器，显示用
#pragma section("sdram0_bank1")
static FILTER_BUFFER_2 FilterBuffer[LEAD_NEED_CALC_LEN];	// 滤波缓存，算法用
#pragma section("sdram0_bank1")
static FILTER_BUFFER_2 FilterBufferShow[LEAD_NEED_CALC_LEN];	//滤波缓存，显示用
#pragma section("sdram0_bank1")
static int paceBuffer[LEAD_NEED_CALC_LEN][5];	// PACE抑制的数据缓存

// 结果
#pragma section("sdram0_bank1")
static CECGIn stDataIn;         //输入数据
CECGIn* pST_DataIn = &stDataIn;
#pragma section("sdram0_bank1")
static CECGOut stDataOut;       //输出数据
CECGOut* pST_DataOut = &stDataOut;
#pragma section("sdram0_bank1")
CAlert_List stAlertList;
#pragma section("sdram0_bank1")
CECG_Out_Item pDataOutData[ECG_ALGORITHM_INPUT_MAX_LEN];
#define ECG_LABEL_MAX_SIZE      (100)
#pragma section("sdram0_bank1")
static int LabelIndexList[ECG_LABEL_MAX_SIZE];// 标注的位置队列
#pragma section("sdram0_bank1")
static int LabelClassList[ECG_LABEL_MAX_SIZE];// 标注的类型队列
static int LabelCount = 0;// 标注的个数
static int HR;	// 心率
#pragma section("sdram0_bank1")
static ARRHYTHMIA_ANALYSIS_RESULT1 AAResult1;// 心律失常分析结果


// ST段分析数据
#pragma section("sdram0_bank1")
static float ST_Volt_His[LEAD_MAX_LEN][ST_VOLT_HIS_MAX_LEN];
#pragma section("sdram0_bank1")
static int ST_Volt_His_Count[LEAD_MAX_LEN];
#pragma section("sdram0_bank1")
static float ST_Volt_Val[LEAD_MAX_LEN];

// PVC计数
#pragma section("sdram0_bank1")
static CPVCPoint PVCList[PVC_LIST_MAX_LEN];	
static int PVCListCount = 0;
static int PVCCheckTimeCount = 0;

// 状态数据及算法参数
static float fAmpDif; // 数据最前一段的最大最小值差

//Add By QWB 中间过程数据
#define TRIANGLE_FORM_SIZE          (100)
#pragma section("sdram0_bank1")
static CTriangle_Form g_TriangleForm[TRIANGLE_FORM_SIZE]; // 三角波列表
#pragma section("sdram0_bank1")
static int g_malloc_int[TRIANGLE_FORM_SIZE];
#pragma section("sdram0_bank1")
static int g_malloc_int_array[TRIANGLE_FORM_SIZE][2];


#pragma section("sdram0_bank1")
void append_Label(int* indexList, int* classList, int* pCount, u32 index, u32 clas);
float Derivative_Squaring_Integration(float dataIn);
void CalcOtherFiltedChannels(CECGOut* pData, const u32 index);
void CalcOtherFiltedChannels1(int* pData);
int PaceProccess(int* dataBuf, int data, u8 flag);
void DataPreProccess(CECGOut *pSTDataOut, CECGIn *pSTDataIn, u32 stDataOutIndex, u32 currentSysTick);
void ECGAlgoRun(void);
void ECGAlgoInit(u32 FS);
int fillResult(void);
int CalcHeartRate(void);
u32 FindTriangleForm( int* pData,
                     u32 nDataLen,
                     int threshold,
                     CTriangle_Form* pTFList,
                     u32 nMaxLen);
void Calc_Feature2(int* pData1, CTriangle_Form TF, int* out1, int* out2);
int FindRForm(CTriangle_Form* pTFList, u32 nFormNum, int* pData1, int* pLabelList);
int InializingProgress(int headIndex);
void FindIntegralWave( float* dataBuf,
                      u32 bufferLen,
                      float threshold,
                      int* pPos,
                      int* pPpos,
                      float* pMaximum );
bool TemplateCompare(int* pTest, const int* pTemplate, int nLen);
void ArrhythmiaAnalysis(void);
void ReportArrhythmiaAnalysis(void);
void Init_ARRHYTHMIA_ANALYSIS_RESULT1(ARRHYTHMIA_ANALYSIS_RESULT1* pt);
int CheckIsDoubleRise(   float* pt1,
                        int len1,
                        float threshold1,
                        int* pt2,
                        int len2,
                        float threshold2,
                        int* pIndex);
int CheckRiseEdgeByLine(float* pt, int len, float threshold, int *pIndex);
int CheckFallEdgeByLine(float* pt, int len, float threshold, int *pIndex);
int CheckRWave(int* pt, int len, float threshold1, float threshold2, int* pIndex);
int CheckRiseEdge(void* inpt, u8 type, int len, float threshold, int* pIndex);
int CheckFallEdge(void* inpt, u8 type, int len, float threshold, int* pIndex);
void CalcThreshold1( int* pt1,
                    int len1,
                    float* pt2,
                    int len2,
                    int FS,
                    float *pThreshold1,
                    float *pThreshold2,
                    float *pThreshold3,
                    float *pThreshold4);
int CalcSingleSTVolt(int* pt, int len, int index, int FS, float ISO, float ST, int *pResult);
void InsertSTVolt(int QRSIndex);
void InsertPVC(int systick);
void CheckPVCList(int systick);
void AlertList_Clear(CAlert_List* pList);
int AlertList_Append(CAlert_List* pList, CAlert_Item item);


// 标注的增删改查
static void append_Label(int* indexList, int* classList, int* pCount, u32 index, u32 clas)
{
    int count = *pCount;
	if (count == ECG_LABEL_MAX_SIZE)
    {
		int i = 0;
		for (i = 0; i < ECG_LABEL_MAX_SIZE-1; i++)
        {
			indexList[i] = indexList[i + 1];
			classList[i] = classList[i + 1];
		}
		indexList[ECG_LABEL_MAX_SIZE-1] = index;
		classList[ECG_LABEL_MAX_SIZE-1] = clas;
	}
    else
    {
		indexList[count] = index;
		classList[count] = clas;
		count++;
        *pCount = count;
	}
}

#define INTEBUFFERLEN   80 // 积分窗口0.16s
#pragma section("sdram0_bank1")
static float g_inteBuffer[INTEBUFFERLEN];
static float Derivative_Squaring_Integration(float dataIn)//DEBUG_QWB:int -> float会数据截断风险
{
	static float derivBuffer[5];

	// 微分
	int i = 0;
	for (i = 0; i < 4; i++)
    {
		derivBuffer[i] = derivBuffer[i + 1];
	}
	derivBuffer[4] = dataIn;
	float deriv = 0.125 * (derivBuffer[4] + 2 * derivBuffer[3] - 2 * derivBuffer[1] - derivBuffer[0]);
	
	// 平方
	float square = deriv * deriv;

	// 积分
	float dataOut = 0;
	int max = INTEBUFFERLEN - 1;
	for (i = 0; i < max; i++)
    {
		g_inteBuffer[i] = g_inteBuffer[i + 1];
	}
	g_inteBuffer[max] = square;
	for (i = 0; i < INTEBUFFERLEN; i++)
    {
		dataOut += g_inteBuffer[i];
	}
	if (dataOut == 0)
    {
		dataOut = 0;
	}
    else
    {
		dataOut /= INTEBUFFERLEN;
	}

	return dataOut;
}

static void CalcOtherFiltedChannels(CECGOut* pData, const u32 index)
{
	// III = II - I  
	pData->pData[index].stDataFilter.s32ECG4 = \
	    pData->pData[index].stDataFilter.s32ECG2 \
	    - pData->pData[index].stDataFilter.s32ECG1;
	// aVR = -(I+II)/2
	pData->pData[index].stDataFilter.s32ECG5 = 0 \
	    - (pData->pData[index].stDataFilter.s32ECG1 \
	    + pData->pData[index].stDataFilter.s32ECG2) / 2;
	// aVL = (I-III)/2
	pData->pData[index].stDataFilter.s32ECG6 = \
	    (pData->pData[index].stDataFilter.s32ECG1 \
	    - pData->pData[index].stDataFilter.s32ECG4) / 2;
	// aVF = (II+III)/2
	pData->pData[index].stDataFilter.s32ECG7 = \
	    (pData->pData[index].stDataFilter.s32ECG2 \
	    + pData->pData[index].stDataFilter.s32ECG4) / 2;
}

static void CalcOtherFiltedChannels1(int* pData)
{
	// III = II - I  
	pData[3] = pData[1] - pData[0];
	// aVR = -(I+II)/2
	pData[4] = 0 - (pData[0] + pData[1]) / 2;
	// aVL = (I-III)/2
	pData[5] = (pData[0] - pData[3]) / 2;
	// aVF = (II+III)/2
	pData[6] = (pData[1] + pData[3]) / 2;
}

// PACE抑制处理
int PaceProccess(int* dataBuf, int data, u8 flag)
{
    int i;
	// 数据长度是5哦
	// 数据缓存移动
	for (i = 0; i < 4; i++) {
		dataBuf[i] = dataBuf[i + 1];
	}

	int result = data;
	if (flag == 1) {
		// 数据修复
		result = 0.5 * ((dataBuf[3] + dataBuf[3] - dataBuf[2]) + (dataBuf[2] + dataBuf[2] - dataBuf[0])); 
	}

	// 数据进入缓存
	dataBuf[4] = result;

	return result;
}

void DataPreProccess(CECGOut *pSTDataOut, CECGIn *pSTDataIn, u32 stDataOutIndex, u32 currentSysTick) 
{
    int i = 0;
	int PacedData[7];

	int ForAlgFiltedData[LEAD_NEED_CALC_LEN];
	int FiltedData[LEAD_MAX_LEN];

	int ForShowFiltedData[LEAD_NEED_CALC_LEN];
	int ShowFiltedData[LEAD_MAX_LEN];

	// Pace抑制
	if (PaceMode == 1) {
		PacedData[0] = PaceProccess(paceBuffer[0], \
            pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG1, pSTDataIn->pace);
		PacedData[1] = PaceProccess(paceBuffer[1], \
            pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG2, pSTDataIn->pace);
		PacedData[2] = PaceProccess(paceBuffer[2], \
            pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG3, pSTDataIn->pace);
		for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
			ForAlgFiltedData[i] = PacedData[i];
		}
		if (PaceShowMode == 1) {
			for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
				ForShowFiltedData[i] = PacedData[i];
			}
		} else {
			ForShowFiltedData[0] = pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG1;
			ForShowFiltedData[1] = pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG2;
			ForShowFiltedData[2] = pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG3;
		}
	} else {
		ForAlgFiltedData[0] = pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG1;
		ForAlgFiltedData[1] = pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG2;
		ForAlgFiltedData[2] = pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG3;

		ForShowFiltedData[0] = pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG1;
		ForShowFiltedData[1] = pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG2;
		ForShowFiltedData[2] = pSTDataOut->pData[stDataOutIndex].stDataRaw.s32ECG3;
	}

	// 算法的数据
	//// 陷波
	switch (NotchMode) {
		case 1:
			for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
				ForAlgFiltedData[i] = Filter_Notch50Hz(&FilterNotch50[i], ForAlgFiltedData[i]);
			}
			break;
		case 2:
			for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
				ForAlgFiltedData[i] = Filter_Notch60Hz(&FilterNotch60[i], ForAlgFiltedData[i]);
			}
			break;
		case 3:
			for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
				ForAlgFiltedData[i] = Filter_Notch60Hz(&FilterNotch60[i], \
                    Filter_Notch50Hz(&FilterNotch50[i], ForAlgFiltedData[i]));
			}
			break;
	}
	//// 滤波
	for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
		FiltedData[i] = Filter_Jianhu(&FilterBuffer[i], ForAlgFiltedData[i]);
	}
	CalcOtherFiltedChannels1(FiltedData);

	//// 填入数据缓存
#if 0
	int IndexMax = ECG_CAL_DATA_LEN - 1;
	for (i = 0; i < IndexMax; i++) {
		pECG_SysTick[i] = pECG_SysTick[i + 1];
		pECG_Cal_Buf[0][i] = pECG_Cal_Buf[0][i + 1];
		pECG_Cal_Buf[1][i] = pECG_Cal_Buf[1][i + 1];
		pECG_Cal_Buf[2][i] = pECG_Cal_Buf[2][i + 1];
		pECG_Cal_Buf[3][i] = pECG_Cal_Buf[3][i + 1];
		pECG_Cal_Buf[4][i] = pECG_Cal_Buf[4][i + 1];
		pECG_Cal_Buf[5][i] = pECG_Cal_Buf[5][i + 1];
		pECG_Cal_Buf[6][i] = pECG_Cal_Buf[6][i + 1];
	}
	for (i = 0; i < LEAD_MAX_LEN; i++) {
		pECG_Cal_Buf[i][IndexMax] = FiltedData[i];
	}
	pECG_SysTick[IndexMax] = currentSysTick;
#else   //QWB20140527:先填进Buffer,最后统一移动数据
	for (i = 0; i < LEAD_MAX_LEN; i++) {
		pECG_Cal_Buf[i][nECG_Calc_Data_Num] = FiltedData[i];
	}
	pECG_SysTick[nECG_Calc_Data_Num] = currentSysTick;
#endif

	// 显示的数据
	// 滤波
	switch (WorkMode) {
		case 0:
			//// 陷波
			switch (NotchMode) {
				case 1:
					for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
						ForShowFiltedData[i] = Filter_Notch50Hz(&FilterNotch50Show[i], \
                                                                ForShowFiltedData[i]);
					}
					break;
				case 2:
					for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
						ForShowFiltedData[i] = Filter_Notch60Hz(&FilterNotch60Show[i], \
                                                                ForShowFiltedData[i]);
					}
					break;
				case 3:
					for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
						ForShowFiltedData[i] = Filter_Notch60Hz(&FilterNotch60Show[i], \
                            Filter_Notch50Hz(&FilterNotch50Show[i], ForShowFiltedData[i]));
					}
					break;
			}
			for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
				ShowFiltedData[i] = Filter_ZhenDuan(&FilterBufferShow[i], ForShowFiltedData[i]);
			}
			break;
		case 1:
			for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
				ShowFiltedData[i] = Filter_Jianhu(&FilterBufferShow[i], ForShowFiltedData[i]);
			}
			break;
		case 2:
			for (i = 0; i < LEAD_NEED_CALC_LEN; i++) {
				ShowFiltedData[i] = Filter_Shoushu(&FilterBufferShow[i], ForShowFiltedData[i]);
			}
			break;
	}
	CalcOtherFiltedChannels1(ShowFiltedData);
	// 显示数据
	pSTDataOut->pData[stDataOutIndex].stDataFilter.s32ECG1 = ShowFiltedData[0];
	pSTDataOut->pData[stDataOutIndex].stDataFilter.s32ECG2 = ShowFiltedData[1];
	pSTDataOut->pData[stDataOutIndex].stDataFilter.s32ECG3 = ShowFiltedData[2];
	pSTDataOut->pData[stDataOutIndex].stDataFilter.s32ECG4 = ShowFiltedData[3];
	pSTDataOut->pData[stDataOutIndex].stDataFilter.s32ECG5 = ShowFiltedData[4];
	pSTDataOut->pData[stDataOutIndex].stDataFilter.s32ECG6 = ShowFiltedData[5];
	pSTDataOut->pData[stDataOutIndex].stDataFilter.s32ECG7 = ShowFiltedData[6];
	pSTDataOut->pData[stDataOutIndex].stDataFilter.u32SysTick = currentSysTick;
}

void ECGAlgoRun(void)
{
    int i = 0;
	u32 stDataOutIndex = pST_DataOut->u32ECGDataNum - 1;//最新的一组输入数据
	u32 currentSysTick = stDataIn.u32SysTick;

	DataPreProccess(pST_DataOut, pST_DataIn, stDataOutIndex, currentSysTick);

	// 对原波形微分、平方、窗口积分处理
#if 0
	int IndexMax = ECG_CAL_DATA_LEN - 1;
	for (i = 0; i < IndexMax; i++) {//DEBUG_QWB:每个新数据均进行大数据移动耗时长
		IntegrationBuffer[i] = IntegrationBuffer[i + 1];
	}
	IntegrationBuffer[IndexMax] = Derivative_Squaring_Integration(pECG_Cal_Buf[0][IndexMax]);
#else //QWB20140527:先填进Buffer,最后统一移动数据
    IntegrationBuffer[nECG_Calc_Data_Num] = Derivative_Squaring_Integration(pECG_Cal_Buf[0][nECG_Calc_Data_Num]);
#endif
    // 停博检测
    int iniUseLen = ECG_ALGORITHM_INPUT_FS * 4;
    int maxV = Max_N(&pECG_Cal_Buf[0][ECG_CAL_DATA_LEN - iniUseLen], iniUseLen);
    int minV = Min_N(&pECG_Cal_Buf[0][ECG_CAL_DATA_LEN - iniUseLen], iniUseLen);
    fAmpDif = maxV - minV;

	// PVC计数
	PVCCheckTimeCount++;
	if (PVCCheckTimeCount > 2 * SampleRate) {
		PVCCheckTimeCount = 0;
		CheckPVCList(currentSysTick);
	}

    //Add by QWB 2014.5.27 Start
    nECG_Calc_Data_Num++;
	AlertList_Clear(&stAlertList); //清空报警列表
    if (ECG_CAL_DATA_LEN != nECG_Calc_Data_Num) return;
    //Add by QWB 2014.5.27 End

    if (isIntializedRRInterval == false)
    {
		// 未初始化
		InializingProgress(0);
	}
    else
    {
		// 已初始化
		// 相关下标后移一格
		for (i = 0; i < LabelCount; i++)
        {
			LabelIndexList[i] = LabelIndexList[i] > (ECG_UPDATE_DATA_LEN-1) ? \
                LabelIndexList[i] - ECG_UPDATE_DATA_LEN : (ECG_UPDATE_DATA_LEN-1);  // 标注的下标
		}
		PrePosX = PrePosX > (ECG_UPDATE_DATA_LEN-1) ? \
            PrePosX - ECG_UPDATE_DATA_LEN : (ECG_UPDATE_DATA_LEN-1);  // 搜索起点
		PreQRS = PreQRS > (ECG_UPDATE_DATA_LEN-1) ? \
            PreQRS - ECG_UPDATE_DATA_LEN : (ECG_UPDATE_DATA_LEN-1);  // 搜索起点
		InteDataRiseEdge = InteDataRiseEdge > (ECG_UPDATE_DATA_LEN-1) ? \
            InteDataRiseEdge - ECG_UPDATE_DATA_LEN : (ECG_UPDATE_DATA_LEN-1);
		InteDataFallEdge = InteDataFallEdge > (ECG_UPDATE_DATA_LEN-1) ? \
            InteDataFallEdge - ECG_UPDATE_DATA_LEN : (ECG_UPDATE_DATA_LEN-1);
		currentRwaveIndex = currentRwaveIndex > (ECG_UPDATE_DATA_LEN-1) ? \
            currentRwaveIndex - ECG_UPDATE_DATA_LEN : (ECG_UPDATE_DATA_LEN-1);
		PVCCheckPoint = PVCCheckPoint > (ECG_UPDATE_DATA_LEN-1) ? \
            PVCCheckPoint - ECG_UPDATE_DATA_LEN : (ECG_UPDATE_DATA_LEN-1);

		calcCount += ECG_UPDATE_DATA_LEN;

		// 检测是否重新初始化
		if ((ECG_CAL_DATA_LEN - PreQRS) / SampleRate > 5) { // 5秒以上检测不到波形
			int headIndex = PreQRS + 1 * SampleRate;
			int result = InializingProgress(headIndex);
			if (result == 1) {
				PrePosX = ECG_CAL_DATA_LEN - 1;
				PreQRS = PrePosX;
			}
		}


        if (fAmpDif > 690)
        {
		// 找当前要找的心律
		if ((ECG_CAL_DATA_LEN - PreQRS >= 0.3 * RRInterval1) // 距离上一个QRS波群有0.3个RR间隔的时间长度了
			&& (calcCount >= calcInterval) // 够计算一次的时间长度了
			)
        {
			// 计数清零
			calcCount = 0;
			if (proccessMode == 0)
            {
				int minIyLen = 0.8 * SampleRate;
				float minIy = Min_F(&IntegrationBuffer[ECG_CAL_DATA_LEN - minIyLen], minIyLen);
				int index = 0;
				float thre = (InteWaveThreshold - minIy) / 4 + minIy;
				int bIs = CheckRiseEdgeByLine(&IntegrationBuffer[PrePosX], \
                                ECG_CAL_DATA_LEN - PrePosX, thre, &index);
				if (bIs == 1)
                {
					// 找到上升沿
					proccessMode = 1;
					preInteThres = thre;
					InteDataRiseEdge = index + PrePosX;
					isFindRwave = false;
				}
                else
                {
					if (ECG_CAL_DATA_LEN - PrePosX > 0.4 * SampleRate) {
						// 蛮久都找不到上升沿
						float ma = Max_F(&IntegrationBuffer[PrePosX], ECG_CAL_DATA_LEN - PrePosX);
						float mi = Min_F(&IntegrationBuffer[PrePosX], ECG_CAL_DATA_LEN - PrePosX);
						float di = ma - mi;
						if (di > 0.15 * InteWaveThreshold) {
							int index = 0;
							thre = mi + 0.15 * di;
							int bIs = CheckRiseEdgeByLine(&IntegrationBuffer[PrePosX], \
                                        ECG_CAL_DATA_LEN - PrePosX, thre, &index);
							if (bIs == 1) {
								// 找到上升沿
								proccessMode = 1;

								preInteThres = thre;
								InteDataRiseEdge = index + PrePosX;
								isFindRwave = false;
							} else {
								PrePosX = ECG_CAL_DATA_LEN - 1;
							}
						} else {
							PrePosX = ECG_CAL_DATA_LEN - 0.2 * SampleRate - 1;
						}
					}
				}
			} /* if (proccessMode == 0) */
            else if (proccessMode == 1)// R波检测模式
            {
				if (isFindRwave == false)
                {
                    int inindex = ECG_CAL_DATA_LEN - SampleRate * 0.1;
					int index = 0;
					int bIs = CheckRiseEdge(&pECG_Cal_Buf[0][inindex], 0, \
                        ECG_CAL_DATA_LEN - inindex, thresholdRawData, &index);
					if (bIs == 1)// 原始波上有上升沿
                    {
						int count = SampleRate * 0.2;
						int index = 0;
						int high = 0;
						int result = QuickRClassifier(&pECG_Cal_Buf[0][ECG_CAL_DATA_LEN - count], \
                            count, SampleRate, RHighThreshold, &index, &high);
						if (result == 1) {
						    isFindRwave = true;
    						index = index + ECG_CAL_DATA_LEN - count;
    						currentRwaveIndex = index;

    						// 找到波，可以报出去了
    						QuickQRSTick = currentSysTick;
    						
    					}
					}
				}

				int index = 0;
				int bIs = CheckFallEdgeByLine(&IntegrationBuffer[InteDataRiseEdge], \
                    ECG_CAL_DATA_LEN - InteDataRiseEdge, preInteThres, &index);
				if (bIs == 1)// 出现积分波下降沿
                { 
                    index = index + InteDataRiseEdge;
                    InteDataFallEdge = index;
                    float wide = (float)(InteDataFallEdge - InteDataRiseEdge) / SampleRate * 1000;
                    //if (currentRwaveIndex > PreQRS && isFindRwave == true) {
                    if (currentRwaveIndex <= PreQRS) {
                        int llen = 0.015 * SampleRate;
                        int len = 0.030 * SampleRate;
                        Max_Index_N(&pECG_Cal_Buf[0][InteDataRiseEdge - llen], len, &index);
                        currentRwaveIndex = index + InteDataRiseEdge - llen;
                    }
                    if (wide > 50) {
                        int mid = (InteDataFallEdge + InteDataRiseEdge) / 2;
                        int leftWide = 0.25 * SampleRate;
                        int rightWide = 0.08 * SampleRate;
                        int index = 0;
                        int high = 0;
                        int result = FullRClassifier(&pECG_Cal_Buf[0][mid - leftWide], \
                            leftWide + rightWide, SampleRate, RHighThreshold, &index, &high);
                        if (result != 0) {
                            // 积分波有上升沿有下降沿，并找到了R波
                            PreQRS = currentRwaveIndex;
                            PrePosX = PreQRS + 0.06 * SampleRate;
                    
                            // 塞进标注队列
                            append_Label(LabelIndexList, LabelClassList, &LabelCount, PreQRS, TFL_R);
                    
                            // 参数自适应
                            InteWaveThreshold = Max_F(&IntegrationBuffer[InteDataRiseEdge], \
                            ECG_CAL_DATA_LEN - InteDataRiseEdge);   //// 积分波找波阈值
                            RHighThreshold = 0.2 * high;    //// R波幅度阈值
                    
                            //// 更新原始波上升沿阈值
                            float m1;
                            m1 = 0;
                            float tmp;
                            int len1 = 0.2 * SampleRate;
                            int headIndex = currentRwaveIndex - len1;
                            int* pt = &pECG_Cal_Buf[0][headIndex];
                            for (i = 4; i < len1; i++) {
                                tmp = pt[i] - pt[i - 4];
                                if (m1 < tmp) {
                                    m1 = tmp;
                                }
                            }
                            thresholdRawData = m1 / 2;  // 原始波阈值
                    
                            //// RR间隔阈值
                            if (LabelCount > 1) {
                                int RRData = 0;
                                int RRCount = LabelCount - 1;
                                for (i = 0; i < RRCount; i++) {
                                    RRData += LabelIndexList[i + 1] - LabelIndexList[i];
                                }
                                RRData /= RRCount;
                                if (my_abs(RRData - RRInterval1) < 0.5 * RRInterval1) {
                                    RRInterval1 = RRData;
                                }
                            }
                    
                            // ST段分析
                            InsertSTVolt(PreQRS);
                            
                            // 回到无波检测模式
                            proccessMode = 0;
                        } else {
                            PVCCheckPoint = InteDataRiseEdge - 0.09 * SampleRate;
                            proccessMode = 2;}
                    } else {
                        PrePosX = InteDataFallEdge;
                        proccessMode = 0;
					}
				}
			} /* if (proccessMode == 1) */
            else if (proccessMode == 2) {
				// 延长时间到了的话，做PVC检测
				int wide = 0.4 * SampleRate;
				if (PVCCheckPoint + wide < ECG_CAL_DATA_LEN) {
					// 检测PVC
					int index = 0;
					int result = 0;
					result = PVCClassifier(&pECG_Cal_Buf[0][PVCCheckPoint], wide, SampleRate, &index);
					if (result == 1 || result == 2) {
						index = index + PVCCheckPoint;
						PreQRS = index;
						PrePosX = InteDataFallEdge + 0.19 * SampleRate;

						// 找到PVC，可以报出去了，并且写入波队列中
						QuickQRSTick = currentSysTick;
						if (result == 1) {
							append_Label(LabelIndexList, LabelClassList, &LabelCount, PreQRS, TFL_PVC_LU);
						} else {
							append_Label(LabelIndexList, LabelClassList, &LabelCount, PreQRS, TFL_PVC_RD);
						}

						// PVC插入PVC计数队列
						InsertPVC(currentSysTick);
						
					} else {
						PrePosX = InteDataFallEdge;
					}

					// 回到无波检测状态
					proccessMode = 0;
				}
			}
			// 算心率
			HR = CalcHeartRate();

			// 心律失常分析
			ArrhythmiaAnalysis();

			// 填结果	
			fillResult();
		}
        else
        {
			// 不用计算
			// 填结果
			fillResult();
		}
	    } else {
            AAResult1.Is_ASYSTOLE = true;
            HR = 0;
            fillResult();// 填结果
        }
    	// 填报警信息
    	ReportArrhythmiaAnalysis();
    }
    
    // 删掉一段UPDATE_DATA_LEN长度的数据
	nECG_Calc_Data_Num = ECG_CAL_DATA_LEN - ECG_UPDATE_DATA_LEN;
	for (i = 0; i < LEAD_MAX_LEN; i++)
    {
        memcpy(&pECG_Cal_Buf[i][0], &pECG_Cal_Buf[i][ECG_UPDATE_DATA_LEN], sizeof(int)*nECG_Calc_Data_Num);
	}
    memcpy(&pECG_SysTick[0], &pECG_SysTick[ECG_UPDATE_DATA_LEN], sizeof(u32)*nECG_Calc_Data_Num);
    memcpy(&IntegrationBuffer[0], &IntegrationBuffer[ECG_UPDATE_DATA_LEN], sizeof(float)*nECG_Calc_Data_Num);
}

void ECGAlgoInit(u32 FS)
{
    int i = 0;
    // 采样初始化
    SampleRate = FS;

    // 每隔0.016秒计算一次，提高效率
    //calcInterval = (float)SampleRate * 0.016;
    calcInterval = 3;

    // 
    Asystole_Time_Limit = 3;
    Tachycardia_Limit = 100;
    Bradycardia_Limit = 40;

    memset(LabelIndexList, 0, sizeof(int) * ECG_LABEL_MAX_SIZE);

	// ST段分析部分初始化
	memset(ST_Volt_His_Count, 0, sizeof(int) * LEAD_MAX_LEN);
	for (i = 0; i < LEAD_MAX_LEN; i++) {
		ST_Volt_Val[i] = 0;
	}
	ISO_Interval = 0.084;
	ST_Interval = 0.072;

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    memset(pECG_SysTick, 0, ECG_CAL_DATA_LEN * sizeof(u32));
    memset(IntegrationBuffer, 0, ECG_CAL_DATA_LEN * sizeof(float));
    
    pST_DataOut->u32ECGDataNum = 0;
    pST_DataOut->u32ECGDataMax = ECG_ALGORITHM_INPUT_MAX_LEN;
    pST_DataOut->pData = &pDataOutData[0];
    
	stAlertList.u32AlertCount = 0;
    isIntializedRRInterval = false;
    nECG_Calc_Data_Num = ECG_CAL_DATA_LEN - ECG_UPDATE_DATA_LEN;

    u8STAnalysis_sw = 1;
    u8ArrhythmiaAnalysis_sw = 1;
}


static int fillResult(void)
{
	// 填结果
	u32 index = stDataOut.u32ECGDataNum - 1;
	stDataOut.pData[index].stResult.u32HeartRate = HR;
	stDataOut.pData[index].stResult.nSTAmpDif[0] = ST_Volt_Val[0];
	stDataOut.pData[index].stResult.nPVCCount = PVCListCount;
	stDataOut.pData[index].stResult.pAlertList = &stAlertList;
	stDataOut.pData[index].stResult.u32QuickQRSTick = QuickQRSTick;
    QuickQRSTick = 0;

	return 0;
}


// 算心率
static int CalcHeartRate(void)
{
	float interval;
	int heartRate;
	int LCount;
	if (HR >= 0 && HR < 60)
    {
		LCount = 5;
	}
    else if (HR >= 80 && HR < 100)
    {
		LCount = 8;
	}
    else
    {
		LCount = 10;
	}
    
	if (LabelCount >= LCount)
    {
		interval = LabelIndexList[LabelCount - 1] - LabelIndexList[LabelCount - LCount];
		heartRate = (60.0 * SampleRate / (interval / (LCount - 1)))+0.5; 
	}
    else
    {
		interval = LabelIndexList[LabelCount - 1] - LabelIndexList[0];
		heartRate = (60.0 * SampleRate / (interval / (LabelCount - 1)))+0.5; 
	}
	return heartRate;
}


static u32 FindTriangleForm( int* pData,
                             u32 nDataLen,
                             int threshold,
                             CTriangle_Form* pTFList,
                             u32 nMaxLen)
{
	// pData是波形数据，被找三角波的对象
	// threshold是找三角波的幅度阈值
	// pTFList是找到的三角波存放的列表
	// nMaxLen是三角波列表的最大长度
	// 返回值是三角波的个数

	if (pData == NULL)
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
	int j;
    u32 i;
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
			//printf("Test 245 case 0\r\n");
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
					// 
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
					jPoint.u32Y =	 pData[j];
					//printf("jPoint (%d, %d) %d threshold %d\r\n", jPoint.u32X, jPoint.u32Y, high.u32Y - jPoint.u32Y, threshold);
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
				}
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
		}
	}

	return triCount;
}


static void Calc_Feature2(int* pData1, CTriangle_Form TF, int* out1, int* out2)
{
	int len = TF.stRight.u32X - TF.stLeft.u32X + 1;
	*out2 = (len - 1) * 1000 * 2;
}


static int FindRForm(CTriangle_Form* pTFList, u32 nFormNum, int* pData1, int* pLabelList)
{
	if (nFormNum == 0)
	{
		return 0;
	}

	CAlert_Item item;
    int i;
    nFormNum = nFormNum > TRIANGLE_FORM_SIZE ? TRIANGLE_FORM_SIZE : nFormNum;
	// 特征计算
	for (i = 0; i < nFormNum; i++)
    {
		Calc_Feature2(pData1, pTFList[i], &(g_malloc_int_array[i][0]), &(g_malloc_int_array[i][1]));
	}

	// 特征过滤
	//// R波宽度
	int R2L = 40000;	// 40ms
	int R2H = 145000;   // 145ms
	//// 过滤
	int count = 0;
	for (i = 0; i < nFormNum; i++)
    {
		if (g_malloc_int_array[i][1] >= R2L && g_malloc_int_array[i][1] <= R2H)
        {
			//// R波，标号TFL_R
			pLabelList[i] = TFL_R;
			count++;
		}
	}
	return count;
}

static int InializingProgress(int headIndex) 
{
    int i;
	int result = 0;
	fAmpThreshold = 0.30 * fAmpDif;
	
	// 找出三角波
	int paraList[5];
	paraList[0] = fAmpThreshold;
	int tf1Count = FindTriangleForm(&pECG_Cal_Buf[0][headIndex], \
                        ECG_CAL_DATA_LEN - headIndex, paraList[0], g_TriangleForm, \
                        TRIANGLE_FORM_SIZE); // 只用headIndex之后数据去找三角波
	//// 下标全部加上起始位置
	for (i = 0; i < tf1Count; i++) {
		g_TriangleForm[i].stPeak.u32X += headIndex;
		g_TriangleForm[i].stLeft.u32X += headIndex;
		g_TriangleForm[i].stRight.u32X += headIndex;
	}
	// 找R波个数
	memset(g_malloc_int, 0, sizeof(int) * TRIANGLE_FORM_SIZE);
	int RFormCount = FindRForm( g_TriangleForm,
                                tf1Count,
                                pECG_Cal_Buf[0],
                                g_malloc_int);
	if (RFormCount >= 4)
    {
		isIntializedRRInterval = true;

		int sum = 0;
		for (i = 1; i < tf1Count; i++)
        {
			sum += g_TriangleForm[i].stPeak.u32X - g_TriangleForm[i - 1].stPeak.u32X;
		}
		RRInterval1 = sum / (tf1Count - 1);

		InteWaveThreshold = \
            Max_F(  &IntegrationBuffer[g_TriangleForm[tf1Count - 3].stLeft.u32X],
                    ECG_CAL_DATA_LEN - g_TriangleForm[tf1Count - 3].stLeft.u32X);

		PrePosX = g_TriangleForm[tf1Count - 1].stRight.u32X;
		PreQRS = PrePosX;

		// 取第二个R波作为初始化样本
		int indexCount = 0;
		int index = 0;
		for (i = 0; i < tf1Count; i++)
        {
			if (g_malloc_int[i] == TFL_R)
            {
				indexCount++;
				if (indexCount == 2)
                {
					index = g_TriangleForm[i].stPeak.u32X;
					break;
				}
			}
		}
		int indexA = index - 0.22 * SampleRate;
		int indexB = index + 0.22 * SampleRate;
		CalcThreshold1( &pECG_Cal_Buf[0][indexA],
                        indexB - indexA,
                        &IntegrationBuffer[indexA],
                        indexB - indexA,
                        SampleRate,
                        &thresholdRawData,
                        &thresholdIy,
                        &RWaveThreshold1,
                        &RWaveThreshold2 );
		RHighThreshold = 0.2 * fAmpDif;
        result = 1;
    } else {
        //printf("RFormCount %d, tf1Count %d\r\n", RFormCount, tf1Count);
        result = 0;
    }

    return result;
}


static void FindIntegralWave( float* dataBuf,
                              u32 bufferLen,
                              float threshold,
                              int* pPos,
                              int* pPpos,
                              float* pMaximum )
{
	int flag = 0;
	int posA = 0;
	int posB = 0;
	*pPos = 0;
	*pPpos = 0;
	*pMaximum = 0;
    int i = 0;

	int maxIndex = bufferLen;
	for (i = 1; i < maxIndex; i++)
    {
		if (dataBuf[i - 1] <= threshold && dataBuf[i] >= threshold)
        {
			// 找到了上升沿
			posA = i;
			flag = 1;
		}
        else if (dataBuf[i - 1] >= threshold && dataBuf[i] <= threshold)
        {
			// 找到下降沿
			if (flag == 1)
            {
				posB = i - 1;
				flag = 2;
				break;
			}
		}
	}

	if (flag == 2)
    {
		*pPpos = posA;
		*pPos = posB;
		*pMaximum = Max_F(&dataBuf[*pPpos], *pPos - *pPpos);
	}
}

static bool TemplateCompare(int* pTest, const int* pTemplate, int nLen)
{
	bool flag = true;
    int i = 0;
	for (i = 0; i < nLen; i++)
	{
		if (pTest[i] != pTemplate[i])
		{
			flag = false;
		}
	}
	return flag;
}

// 心律失常分析
static void ArrhythmiaAnalysis(void)
{
    int i = 0;
	int j;
	// 初始化心律失常分析结果
	Init_ARRHYTHMIA_ANALYSIS_RESULT1(&AAResult1);
    
	// 统计
	if (LabelCount > 0)
    {
		//// 找到第一个大于要求时间点的标号
		int firstLabel = 0;
		int leastTimePoint = (int)ECG_CAL_DATA_LEN - SampleRate * 30;	 
		for (i = 0; i < LabelCount; i++)
        {
			if (LabelIndexList[i] > leastTimePoint)
            {
				firstLabel = i;
				break;
			}
		}

		for (i = firstLabel; i < LabelCount; i++)
        {
			switch (LabelClassList[i])
            {
				case TFL_R:
					AAResult1.RCount++;
					break;
				case TFL_PVC_LU:
				case TFL_PVC_RU:
					AAResult1.PVCsCount++;
					break;
				default:
					AAResult1.UnKCount++;
					break;
			}
		}
	}

	// 停博检测
	if (LabelCount == 0)
    {
		AAResult1.Is_ASYSTOLE = true;
        HR = 0;
	}
    else
    {
		float noBeatTimeLen = ((float)ECG_CAL_DATA_LEN - LabelIndexList[LabelCount - 1]) / SampleRate;
		if (noBeatTimeLen >= Asystole_Time_Limit)
        {
			// 停搏
			AAResult1.Is_ASYSTOLE = true;
            HR = 0;
		}
	}


	// 心动速度检测
	if (AAResult1.Is_ASYSTOLE == false)
    {
		if (HR >= Tachycardia_Limit)
        {
			if (AAResult1.PVCsCount > 1)
            {
				// 室速
				AAResult1.Is_VENT_TACH = true;
			}
            else
            {
				// 心动过速
				AAResult1.Is_TACHYCARDIA = true;
			}
		}
        else if (HR <= Bradycardia_Limit && HR != 0)
        {
			// 心动过缓
			AAResult1.Is_BRADYCARDIA = true;
		}
        
		// 如果心率大于350就设为350， 小于0就设为0
		if (HR > 350) 
        {
			HR = 350;
		}
        else if (HR < 0)
        {
			HR = 0;
		}

		// 检测漏博
		if (LabelCount > 5)
        {
			float interval[5];
			int initI = LabelCount - 6;
			for (i = 0; i < 5; i++)
            {
				interval[i] = LabelIndexList[initI + i + 1] - LabelIndexList[initI + i];
			}
			float intervalRate = (4 * interval[2]) / (interval[0] + interval[1] + interval[3] + interval[4]);
			if (intervalRate >= 1.96)
            {
				AAResult1.Is_MISSED_BEAT = true;
			}
		}

		// 单室早、双室早、二连律、三连律
		bool flag = false;
	
		int TestBuffer5[5];
		int TestBuffer6[6];
		int maxIndex1 = LabelCount - 5 + 1;
		int index;
		float TimeLen;
		for (i = 0; i < maxIndex1; i++)
        {
			TimeLen = (float)(ECG_CAL_DATA_LEN - LabelIndexList[LabelCount - i - 3]) / SampleRate;
			if (TimeLen >= 4.5)
            {
				//// 当前测试区的中心波离最新时间点的时间间隔超过阈值，不再检测
				break;
			}

			//// 移抄对比数据
			for (j = 0; j < 5; j++)
            {
				index = LabelCount - i - 5 + j;
				TestBuffer5[j] = LabelClassList[index];
			}
			if (TemplateCompare(TestBuffer5, SinglePVCTemp1, 5) || TemplateCompare(TestBuffer5, SinglePVCTemp2, 5))
            {
				//// 单个室早
				AAResult1.Is_SIGLE_PVC = true;
				flag = true;
				break;
			}
            else if (TemplateCompare(TestBuffer5, PairPVCTemp1, 5) || TemplateCompare(TestBuffer5, PairPVCTemp2, 5) || 
					TemplateCompare(TestBuffer5, PairPVCTemp3, 5) || TemplateCompare(TestBuffer5, PairPVCTemp4, 5)) 
            {
				//// 双室早
				AAResult1.Is_PAIR_PVC = true;
				flag = true;
				break;
			}
            else if (TemplateCompare(TestBuffer5, BigeninyTemp1, 5) || TemplateCompare(TestBuffer5, BigeninyTemp2, 5) ||
					TemplateCompare(TestBuffer5, BigeninyTemp3, 5) || TemplateCompare(TestBuffer5, BigeninyTemp4, 5)) 
			{
				//// 二连律
				AAResult1.Is_BIGENINY = true;
				flag = true;
				break;
			}
		}


		if (!flag)
        {
			int maxIndex2 = LabelCount - 6 + 1;
			for (i = 0; i < maxIndex2; i++)
            {
				TimeLen = (float)(ECG_CAL_DATA_LEN - LabelIndexList[LabelCount - i - 3]) / SampleRate;
				if (TimeLen >= 4.5)
                {
					//// 当前测试区的中心波离最新时间点的时间间隔超过阈值，不再检测
					break;
				}

				//// 移抄对比数据
				for (j = 0; j < 6; j++)
                {
					index = LabelCount - i - 6 + j;
					TestBuffer6[j] = LabelClassList[index];
				}
				if (TemplateCompare(TestBuffer6, TrigeninyTemp1, 6) || TemplateCompare(TestBuffer6, TrigeninyTemp2, 6) || 
						TemplateCompare(TestBuffer6, TrigeninyTemp3, 6) || TemplateCompare(TestBuffer6, TrigeninyTemp4, 6))
				{
					//// 三连律
					AAResult1.Is_TRIGENINY = true;
					flag = true;
					break;
				}
			}
		}
	}
}

void releaseAlgorithm()
{
}

static void ReportArrhythmiaAnalysis(void)
{
	CAlert_Item item;
	
    if (0 == u8ArrhythmiaAnalysis_sw) return;   //关闭心率失常分析
    
	if (AAResult1.Is_ASYSTOLE)//停搏
	{
        item.type = ASYSTOLE;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}

	if (AAResult1.Is_BIGENINY)//室早二联律（BGM）
	{
        item.type = VBIG;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}

	if (AAResult1.Is_BRADYCARDIA)//心动过缓（BRD）
	{
        item.type = BRD;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}

	if (AAResult1.Is_MISSED_BEAT)//漏搏（MIS）
	{
        item.type = MISSING_QRS;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}

	if (AAResult1.Is_PAIR_PVC)//二连发室早（CPT）
	{
        item.type = VCOUPLET;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}

	if (AAResult1.Is_R_ON_T)//R on T (ROT)
	{
        item.type = VRONT;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}

	if (AAResult1.Is_SIGLE_PVC)//-TODO-单连发室早
	{
        item.type = SIGLE_PVC;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}

	if (AAResult1.Is_TACHYCARDIA)//室上性心动过速（TAC）
	{
        item.type = TAC;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}

	if (AAResult1.Is_TRIGENINY)//室早三联律（TGM）
	{
        item.type = VTRIG;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}

	if (AAResult1.Is_VENT_FIB)//室颤
	{
        item.type = VFIB;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}

	if (AAResult1.Is_VENT_TACH)//室速
	{
        item.type = VTAC;
		item.u32Time = stDataIn.u32SysTick; // 当前systick
		item.u32AlertIndex = 0;
		AlertList_Append(&stAlertList, item);
	}
}

static void Init_ARRHYTHMIA_ANALYSIS_RESULT1(ARRHYTHMIA_ANALYSIS_RESULT1* pt)
{
	pt->PVCsCount = 0;
	pt->RCount = 0;
	pt->UnKCount = 0;
	pt->PVCRate = 0;

	pt->Is_ASYSTOLE = false;
	pt->Is_BIGENINY = false;
	pt->Is_BRADYCARDIA = false;
	pt->Is_MISSED_BEAT = false;
	pt->Is_PAIR_PVC = false;
	pt->Is_R_ON_T = false;
	pt->Is_SIGLE_PVC = false;
	pt->Is_TACHYCARDIA = false;
	pt->Is_TRIGENINY = false;
	pt->Is_VENT_FIB = false;
	pt->Is_VENT_TACH = false;
}

static int CheckIsDoubleRise(   float* pt1,
                                int len1,
                                float threshold1,
                                int* pt2,
                                int len2,
                                float threshold2,
                                int* pIndex) 
{
	int bIs = CheckRiseEdge(pt1, 1, len1, threshold1, pIndex);
	if (bIs == 1)
    {
		bIs = CheckRiseEdge(pt2, 0, len2, threshold2, pIndex);
	}

	return bIs;
}

int CheckRiseEdgeByLine(float* pt, int len, float threshold, int *pIndex)
{
	int bIs = 0;
    int i;
	*pIndex = 0;
	for (i = 1; i < len; i++) {
		if (pt[i - 1] <= threshold && pt[i] >= threshold) {
			// 找到了上升沿
			*pIndex = i;
			bIs = 1;
			return bIs;
		}
	}
	return bIs;
}

int CheckFallEdgeByLine(float* pt, int len, float threshold, int *pIndex)
{
	int bIs = 0;
    int i;
	*pIndex = 0;
	for (i = 1; i < len; i++) {
		if (pt[i - 1] >= threshold && pt[i] <= threshold) {
			// 找到了下降沿
			*pIndex = i - 1;
			bIs = 1;
			return bIs;
		}
	}
	return bIs;
}

static int CheckRWave(int* pt, int len, float threshold1, float threshold2, int* pIndex)
{
	int mx = 0;
	int m = Max_Index_N(pt, len, &mx);
	int bIs = 0;
	if (len - mx > 5)
    {
		int XA = mx - 5;
		int XB = mx + 5;
		if (XA > 0)
        {
			float ra = (float)(m - pt[XA]) / (mx - XA);
			float rb = (float)(m - pt[XB]) / (XB - mx);
			if (ra > threshold1 && rb > threshold2)
            {
				bIs = 1;
				*pIndex = mx;
			}
		}
	}
	return bIs;
}

// 检测上升沿
static int CheckRiseEdge(void* inpt, u8 type, int len, float threshold, int* pIndex)
{
	if (type == 0)
    {
		int* pt = (int*)inpt;
		int bIs = 0;
		if (len < 4)
        {
			*pIndex = 0;
			return bIs;
		}

		*pIndex = 3;
		while (*pIndex < len)
        {
			if ((pt[*pIndex] - pt[*pIndex - 3]) > threshold)
            {
				bIs = 1;
				break;
			}
            *pIndex = *pIndex +1;
		}
		return bIs;
	}
    else
    {
		float* pt = (float*)inpt;
		int bIs = 0;
		if (len < 4)
        {
			*pIndex = 0;
			return bIs;
		}

		*pIndex = 3;
		while (*pIndex < len)
        {
			if ((pt[*pIndex] - pt[*pIndex - 3]) > threshold)
            {
				bIs = 1;
				break;
			}
            *pIndex = *pIndex +1;
		}
		return bIs;
	}
}

// 检测下降沿
static int CheckFallEdge(void* inpt, u8 type, int len, float threshold, int* pIndex) 
{
	if (type == 0)
    {
		int* pt = (int*)inpt;
		int bIs = 0;
		if (len < 4)
        {
			*pIndex = 0;
			return bIs;
		}

		*pIndex = 3;
		while (*pIndex < len)
        {
			if ((pt[*pIndex - 3] - pt[*pIndex]) > threshold)
            {
				bIs = 1;
				break;
			}
            *pIndex = *pIndex +1;
		}
		return bIs;
	}
    else
    {
		float* pt = (float*)inpt;
		int bIs = 0;
		if (len < 4)
        {
			*pIndex = 0;
			return bIs;
		}

		*pIndex = 3;
		while (*pIndex < len)
        {
			if ((pt[*pIndex - 3] - pt[*pIndex]) > threshold)
            {
				bIs = 1;
				break;
			}
            *pIndex = *pIndex +1;
		}
		return bIs;
	}
}

static void CalcThreshold1( int* pt1,
                            int len1,
                            float* pt2,
                            int len2,
                            int FS,
                            float *pThreshold1,
                            float *pThreshold2,
                            float *pThreshold3,
                            float *pThreshold4)
{
	// 找微分最大值
	float m1, m2;
	m1 = 0;
	m2 = 0;
	float tmp;
    int i;
	for (i = 4; i < len1; i++)
    {
		tmp = pt1[i] - pt1[i - 4];
		if (m1 < tmp)
        {
			m1 = tmp;
		}
	}
	for (i = 4; i < len2; i++)
    {
		tmp = pt2[i] - pt2[i - 4];
		if (m2 < tmp)
        {
			m2 = tmp;
		}
	}
	
	*pThreshold1 = m1 / 2;	    // 原始波阈值
	*pThreshold2 = m2 / 2.5;	// 积分波阈值

	// 找原始波的最大值及位置
	int index;
	int max;
	max = Max_Index_N(pt1, len1, &index);
	int XA = index - 5;
	int XB = index + 5;
	float ra = (max - pt1[XA]) / (index - XA);
	float rb = (max - pt1[XB]) / (XB - index);
	*pThreshold3 = 0.6 * ra;
	*pThreshold4 = 0.6 * rb;

}

int CalcSingleSTVolt(int* pt, int len, int index, int FS, float ISO, float ST, int *pResult) 
{
    int i;
	int Aindex = index - (ISO + 0.02) * FS;
	int Bindex = index - (ISO - 0.02) * FS;
	int Cindex = index + (ST - 0.02) * FS;
	int Dindex = index + (ST + 0.02) * FS;

	if (Aindex < 0 || Aindex > len 
		|| Bindex < 0 || Bindex > len 
		|| Cindex < 0 || Cindex > len 
		|| Dindex < 0 || Dindex > len) {
			return 0;
	}

	int Vpq = 0;
	for (i = Aindex; i < Bindex; i++) {
		Vpq += pt[i];
	}
	Vpq /= Bindex - Aindex;

	int Vst = 0;
	for (i = Cindex; i < Dindex; i++) {
		Vst += pt[i];
	}
	Vst /= Dindex - Cindex;

	*pResult = Vst - Vpq;

	return 1;
}

void InsertSTVolt(int QRSIndex) 
{
    int i;
	int result[7];
	int bIs = CalcSingleSTVolt(pECG_Cal_Buf[0], ECG_CAL_DATA_LEN, QRSIndex, \
        SampleRate, 0.084, 0.072, &result[0]);
	if (bIs == 1) {
		if (ST_Volt_His_Count[0] >= ST_VOLT_HIS_MAX_LEN) {
			int moveLen = ST_VOLT_HIS_MAX_LEN - 1;
			for (i = 0; i < moveLen; i++) {
				ST_Volt_His[0][i] = ST_Volt_His[0][i + 1];
			}
			ST_Volt_His[0][moveLen] = result[0];
		} else {
			ST_Volt_His[0][ST_Volt_His_Count[0]] = result[0];
			ST_Volt_His_Count[0]++;
		}

		ST_Volt_Val[0] = result[0];
		//printf("STVolt %f\r\n", ST_Volt_Val[0]);
	}
}


void InsertPVC(int systick)
{
    int i;
	CPVCPoint point;
	point.systick = systick;
	if (PVCListCount >= PVC_LIST_MAX_LEN) {
		int moveLen = PVC_LIST_MAX_LEN - 1;
		for (i = 0; i < moveLen; i++) {
			PVCList[i] = PVCList[i + 1];
		}
		PVCList[moveLen] = point;
		PVCListCount = PVC_LIST_MAX_LEN;
	} else {
		PVCList[PVCListCount] = point;
		PVCListCount++;
	}
}

void CheckPVCList(int systick) 
{
    int i;
	u32 delayTime;
	int firstIndex = 0;
	for (i = 0; i < PVCListCount; i++) {
		delayTime = (systick - PVCList[i].systick) / 10000;
		if (delayTime < 60) { // 一分钟以内的就保留
			firstIndex = i;
		}
	}

	int cutLen = firstIndex;
	int moveLen = PVCListCount - cutLen;
	for (i = 0; i < moveLen; i++) {
		PVCList[i] = PVCList[i + cutLen];
	}
	PVCListCount = moveLen;
}

static void AlertList_Clear(CAlert_List* pList)
{
	pList->u32AlertCount = 0;
}

static int AlertList_Append(CAlert_List* pList, CAlert_Item item)
{
    int i;
	if (pList->u32AlertCount >= ECG_ALERT_MAX_NUM)
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

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//Debug Interface for ECG V1000 Start
static void ECGV1000_setFilter(u8 newSel)
{
    //WorkMode 0 -- 诊断模式；1--监护模式；2--手术模式
    switch(newSel)
    {
        case 1: //SURGICAL
            WorkMode = 2;
            break;
        case 2: //MONITOR
            WorkMode = 1;
            NotchMode = 3;//默认陷波器全开
            break;
        case 3: //DIAGNOSE
            WorkMode = 0;
            NotchMode = 0; //默认陷波器全关
            break;
        default:
            break;
    }
}

int ECGV1000DebugInterface( ECG_ALG_DEBUG_TypeDef type,
                            void* pBuf,
                            unsigned char nLen)
{    
    u8 *pVal = (u8 *)pBuf;
    switch(type)
    {
    case ECG_FILTER_SEL:
        ECGV1000_setFilter(pVal[0]);
        break;
    case ECG_NOTCH_SELECT:
        NotchMode = pVal[0];
        break;
    case ECG_PACE_SW:
        if (0 == pVal[0])   //PACE关,默认
            PaceMode = 0;
        else
            PaceMode = 1;
        break;
    case ECG_PACE_OVERSHOOT_SW:
        if (0 == pVal[0])   //PACE过冲抑制关,默认
            PaceShowMode = 0;
        else
            PaceShowMode = 1;
        break;
    case ECG_ST_SW:
        if (!pVal[0])
            u8STAnalysis_sw = 0;
        else
            u8STAnalysis_sw = 1;
        break;
    case ECG_ST_MEASURE_SETTING:
        if (nLen != 4) return -1;
        //ISO
        u16 ms;
        ms = (pVal[0] << 8) | pVal[1];
        ISO_Interval = (float)(ms / 1000);//s
        //ST
        ms = (pVal[2] << 8) | pVal[3];
        ST_Interval = (float)(ms / 1000);
        break;
    case ECG_ARRHYTHMIA_SW:
        if (!pVal[0])
            u8ArrhythmiaAnalysis_sw = 0;
        else
            u8ArrhythmiaAnalysis_sw = 1;
        break;
    case ECG_Tachycardia_Limit_SET:
        Tachycardia_Limit = (pVal[0] << 8) | pVal[1];
        break;
    case ECG_Bradycardia_Limit_SET:
        Bradycardia_Limit = (pVal[0] << 8) | pVal[1];
        break;
    default:
        return -1;
    }
    return 0;
}
//Debug Interface for ECG V1000 End


