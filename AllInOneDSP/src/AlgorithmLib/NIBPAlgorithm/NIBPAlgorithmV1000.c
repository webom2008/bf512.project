/*****************************************************************************
 * NIBPAlgorithmV1000.c
 *****************************************************************************/
#include "NIBPAlgorithm_conf.h"
//#include "system_uart.h"

#include <stdlib.h> //malloc,free,qsort

//#define _NIBP_ALG_INFO_
#ifdef _NIBP_ALG_INFO_
#include "system_uart.h"
#define NIBPAlgo_DBG(fmt, arg...) AIO_printf("\r\n[NIBPAlgo] "fmt, ##arg)
#else
#define NIBPAlgo_DBG(fmt, arg...)
#endif


#define NIBP_GET_BPM_QUEUES_COUNT       (8)

/*****************************************************************************
 * 输入输出数据
 *****************************************************************************/


#pragma section("sdram0_bank1")
u32 p_NibpDataIn[NIBP_FILTER_WINDOW_LEN];
#pragma section("sdram0_bank1")
u32 p_NibpDataFil[NIBP_STEEP_WINDOW_LEN];
#pragma section("sdram0_bank1")
CNIBPOut g_nibpAlgoOut;
CNIBPOut *p_nibpAlgoOut = &g_nibpAlgoOut;

#pragma section("sdram0_bank1")
CNIBPIn g_nibpAlgoIn;
CNIBPIn *p_nibpAlgoIn = &g_nibpAlgoIn;

char g_nibp_patient_cur;
static char g_isRisingFirst;
static int g_pulse_average_ms;
static float Reliability;

/*****************************************************************************
 * 过程数据
 *****************************************************************************/
#pragma section("sdram0_bank1")
FEATURE_LIST Feature_list;
#pragma section("sdram0_bank1")
FEATURE_LIST *p_Feature_list = &Feature_list;
#pragma section("sdram0_bank1")
QUEUE_S Queue_s;
#pragma section("sdram0_bank1")
QUEUE_S *p_Queue_s = &Queue_s;
#pragma section("sdram0_bank1")
int pSleepWinData[NIBP_STEEP_WINDOW_LEN];
#pragma section("sdram0_bank1")
int pSleepWindownData2[NIBP_STEEP_WINDOW_LEN];
#pragma section("sdram0_bank1")
QUEUE_S g_nibpQueueSnew;    //计算Q时，升压降压分时复用
#pragma section("sdram0_bank1")
QUEUE_S g_NIBP_QueuesNew2;  //计算Q时，升压降压分时复用
#pragma section("sdram0_bank1")
int pNIBPgetBPMBPMList[NIBP_GET_BPM_QUEUES_COUNT];
#pragma section("sdram0_bank1")
int p_temp_queue_int[QUEUE_S_LEN]; //分时复用
#pragma section("sdram0_bank1")
int pQueuesDataDeltaUse[QUEUE_S_LEN];
#pragma section("sdram0_bank1")
int g_DiffList[100];
#pragma section("sdram0_bank1")
int g_CountList[100];
#pragma section("sdram0_bank1")
static int g_MeanList[100];
#pragma section("sdram0_bank1")
int g_fallingINT[50];

//static int g_intListIndex;
//static int g_intMaxEnergyIndex;
//static int g_intMaxEnergy;

#define KDP             0.31
#define KDPbase         47.5
#define KSP             -0.03
#define KSPbase         51
#define KOTDP           -2.3
#define KOTDPbase       12
#define KOTSP           0.48
#define KOTSPbase       30.5
#define KDPmin          60.5
#define KDPmax          64.5
#define KSPmin          47
#define KSPmax          60.5


#define FOR_BASELINE    0.8
#define FOR_BASELINE2   0.4

#define WAVELET_CUT     400
#define MAX_STEP_LEN    200
#define MODE_START      (0)
#define MODE_UP         (1)
#define MODE_DOWN       (2)
#define MAX_STEP        (160)

#define NIBP_RISING_CUT_LEN           (100 - NIBP_DATA_FILTER_SIZE)



//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void NIBPfromFeature(FEATURE_LIST* pFeature_list,
                    int* pDP,
                    int *pDE,
                    int* pSP,
                    int *pSE,
                    int* pMAP,
                    int *pMAE,
                    int *pBPM);
#pragma section("sdram0_bank3")
void QerrCutRising(QUEUE_S* QueueS);
#pragma section("sdram0_bank3")
void QerrCutFalling(QUEUE_S* QueueS);
#pragma section("sdram0_bank3")
void SteepMove(QUEUE_S* QueueS, int len);
#pragma section("sdram0_bank3")
int Filter(const float* pfFilter,int FilterLen,int* indata,int inlen,
                    int outlen,int* outdata,int defau);
#pragma section("sdram0_bank3")
int MedianButQ_Abs(int* DataIn,int DataInLen,QUEUE_S* QueueSin,int Window);
#pragma section("sdram0_bank3")
void GetSteepRising(int* DataIn,int DataLen,QUEUE_S* pQueueSin,QUEUE_S* pQueueSnew,
    int MinWindow,int MaxWindow,int MaxCut,int MinCut,int MinIn);
#pragma section("sdram0_bank3")
void GetSteepFalling(int *DataIn,
                            int DataLen,
                            int cut1,
                            float cut2,
                            float cut3,
                            QUEUE_S *pQueue);
#pragma section("sdram0_bank3")
void GetUnitList(QUEUE_S* QueueSnow, QUEUE_S* QueueSnew);
#pragma section("sdram0_bank3")
void NIBPgetRisingQ(int* pDataIn,int* pDataFil,int* pDataFilLen,QUEUE_S* pQueueS);
#pragma section("sdram0_bank3")
int NIBPgetBPM(QUEUE_S* pQueueS, int nLen);
#pragma section("sdram0_bank3")
bool NIBPgetPressureMax(QUEUE_S* pQueueS,int *pBPM,int *pPressureMax, int LatestP);
#pragma section("sdram0_bank3")
int NIBP_Rising_All( int* pDataIn,
                    int DataLen,
                    int* pDataFil,
                    int* pDataFilLen,
                    QUEUE_S* pQueueS,
                    int Pressure,
                    int *pPulse_ms,
                    char First,
                    char *pEnd);
#pragma section("sdram0_bank3")
int NIBP_Step_Rising(int* pDataIn,
                    int DataLen,
                    int* pDataFil,
                    int* pDataFilLen,
                    QUEUE_S* pQueueS,
                    int* power,
                    int* pPulse_ms);
#pragma section("sdram0_bank3")
void GetFallingUnitList(    int* pDataIn,
                            int* pDataLen,
                            QUEUE_S* pQueueS,
                            QUEUE_S* QueueSnew,
                            int *pDiffList,
                            int *pCountList,
                            int *pMeanList,
                            int *pPulse_ms);
#pragma section("sdram0_bank3")
void NIBPgetFallingQ(int* pDataIn,   //I/O，存放滤波前、后的数据
                    int* pDataFil,
                    int* pDataLen,  //I/O，滤波前、后的数据长度
                    QUEUE_S* pQueueS);
#pragma section("sdram0_bank3")
void NIBPgetFallingPulseInterval(QUEUE_S* pQueueS,
                                int *pDiffList,
                                int *pCountList,
                                int *MeanList,
                                int ListIndex);
#pragma section("sdram0_bank3")
bool NIBPProcessQ(QUEUE_S* pQueueS);
#pragma section("sdram0_bank3")
bool NIBPProcessQWithEnd(QUEUE_S* pQueueS, int End);
#pragma section("sdram0_bank3")
void GetFeatureList(QUEUE_S* pQueueS, FEATURE_LIST* pFeatureList);
#pragma section("sdram0_bank3")
void FilFeatureList(FEATURE_LIST* pFeatureList);
#pragma section("sdram0_bank3")
void NIBP_RisingAddUnitList(    int* pDataIn,       //input,输入原始数据
                                int  DataInLen,     //input,输入数据长度
                                int* pDataFil,
                                int  DataFilLen,
                                QUEUE_S* pQueueS,
                                int *pPulse_ms);
#pragma section("sdram0_bank3")
bool NIBP_FallingWithEnd(   int* pDataIn,  //input,输入原始数据
                            int* pDataFil,
                            int* pDataLen,  //input,输入数据长度
                            QUEUE_S* pQueueS,   //output,算法分析处理的重要数据
                            FEATURE_LIST* pFeatureList, //output,压力与能量列表
                            int* pDP,       //output,舒张压
                            int *pDE,
                            int* pSP,       //output,收缩压
                            int *pSE,
                            int* pMAP,      //output,平均压
                            int *pMAE,
                            int PMax,     //input,预充压最大值
                            int* pBPM,
                            int* pPulse_ms,
                            int End);//添加End作为嵌入式最低泻压结束
#pragma section("sdram0_bank3")
void NIBPInit(QUEUE_S* QueueS, FEATURE_LIST* pFeatureList);
#pragma section("sdram0_bank3")
void NIBPInitV1000(void);
#pragma section("sdram0_bank3")
void NIBPRunV1000(void);
#pragma section("sdram0_bank3")
void ForResult(int DP_Last, int SP_Last, int BPM_Last, int *DP_Now, int *SP_Now, int *BPM_Now);
#pragma section("sdram0_bank3")
float ForReliability(int *MeanH_List,
                    int *MeanD_List,
                    int *Count_List,
                    int ListLen,
                    int StartIndex,
                    int EndIndex);

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

const float fFir_6_100hz_35s[NIBP_DATA_FILTER_SIZE] =
{
    0.000447800588667803,  
    -0.000105422692224901,              
    -0.000952029326004866,              
    -0.002312592830894632,               
    -0.004226409943361889,               
    -0.006420885716992339,               
    -0.008253050105353834,               
    -0.008755715501060560,               
    -0.006794675428708898,               
    -0.001312828213578494,               
    0.008389763091506590,               
    0.022406554056683962,                
    0.040072172663723322,                
    0.059955735708328288,                
    0.080017402851408084,                
    0.097908232374486245,                
    0.111358622547970950,                 
    0.118577325875405180,                 
    0.118577325875405180,                 
    0.111358622547970950,                 
    0.097908232374486245,                
    0.080017402851408084,                
    0.059955735708328288,                
    0.040072172663723322,               
    0.022406554056683962,                
    0.008389763091506590,               
    -0.001312828213578494,               
    -0.006794675428708898,               
    -0.008755715501060560,               
    -0.008253050105353834,               
    -0.006420885716992339,               
    -0.004226409943361889,               
    -0.002312592830894632,               
    -0.000952029326004866,              
    -0.000105422692224901,              
    0.000447800588667803
};    

const float fFir_5_1000hz_85s[NIBP_FEATURE_FILTER_SIZE] =
{
    0.008976999273044338, 
    0.009145974502260437,                
    0.009312974713106381,                
    0.009477870179969791,                
    0.009640532232781553,                 
    0.009800833391838003,                
    0.009958647501765170,                
    0.010113849864461242,                 
    0.010266317370854422,                 
    0.010415928631314506,                 
    0.010562564104557835,                 
    0.010706106224886958,                 
    0.010846439527608007,                 
    0.010983450772470848,                 
    0.011117029064979495,                 
    0.011247065975422217,                 
    0.011373455655474013,                 
    0.011496094952226402,                 
    0.011614883519502848,                 
    0.011729723926321311,                 
    0.011840521762368548,                 
    0.011947185740354865,                 
    0.012049627795121254,                 
    0.012147763179375212,                 
    0.012241510555935378,                 
    0.012330792086369584,                 
    0.012415533515915126,                 
    0.012495664254574894,                 
    0.012571117454287526,                 
    0.012641830082074736,                 
    0.012707742989073789,                 
    0.012768800975368541,                 
    0.012824952850537301,                 
    0.012876151489841397,                 
    0.012922353885983747,                 
    0.012963521196372160,                 
    0.012999618785827836,                 
    0.013030616264685276,                 
    0.013056487522235529,                 
    0.013077210755470553,                 
    0.013092768493092512,                 
    0.013103147614757692,                 
    0.013108339365530675,                 
    0.013108339365530675,                 
    0.013103147614757692,                
    0.013092768493092512,                
    0.013077210755470553,                
    0.013056487522235529,                
    0.013030616264685276,                 
    0.012999618785827836,                 
    0.012963521196372160,                  
    0.012922353885983747,                 
    0.012876151489841397,                 
    0.012824952850537301,                 
    0.012768800975368541,                 
    0.012707742989073789,                
    0.012641830082074736,                
    0.012571117454287526,                 
    0.012495664254574894,                 
    0.012415533515915126,                 
    0.012330792086369584,                 
    0.012241510555935378,                 
    0.012147763179375212,                 
    0.012049627795121254,                 
    0.011947185740354865,                 
    0.011840521762368548,                 
    0.011729723926321311,                 
    0.011614883519502848,                 
    0.011496094952226402,                 
    0.011373455655474013,                 
    0.011247065975422217,                 
    0.011117029064979495,                 
    0.010983450772470848,                 
    0.010846439527608007,                 
    0.010706106224886958,                 
    0.010562564104557835,                 
    0.010415928631314506,                 
    0.010266317370854422,                 
    0.010113849864461242,                 
    0.009958647501765170,                
    0.009800833391838003,                
    0.009640532232781553,                 
    0.009477870179969791,                
    0.009312974713106381,                
    0.009145974502260437,                
    0.008976999273044338 
};

static int Filter(const float* pfFilter,    //滤波器参数表指针
                    int FilterLen,          //滤波器参数表长度
                    int* indata,            //滤波前的原始数据指针
                    int inlen,              //滤波前的原始数据的长度
                    int outlen,             //滤波后的数据的长度
                    int* outdata,           //滤波后的数据指针
                    int defau)              //上次滤波后最后一个值
{
    double fTmp;
    int i,j;
    
    //滤波前后数据长度非法，直接退出
    //必须满足(inLen - outLen > FilterLen)条件
    if(outlen > inlen-FilterLen+1) return -1;
    
    for (j = 0; j < outlen; j++)
    {
        fTmp = 0;
        for (i = 0; i < FilterLen; i++)
        {
            if(STEP_RELE_ADC_TAG == indata[i+j]) //若为标签值，则保持前一个数据
            {
                fTmp = (j > 0)?(outdata[j-1]):(defau);
                break;
            }
            fTmp += pfFilter[i] * indata[i+j]; //非标签值，则正常滤波算法(后向算法)
        }
        outdata[j]=fTmp; //储存滤波算法计算后的值
    }
    return 1;
}

//返回计算结果(中值)
static int MedianButQ_Abs(int* DataIn,      //input,待处理的数据指针
                        int DataInLen,      //input,待处理数据长度
                        QUEUE_S* QueueSin,  //input,队列指针
                        int Window)         //input,x轴相关的差值
{
    int ii;
    int jj;
    int MedianDataLen = 0;
    int Delta;
    
    memset(pSleepWinData, 0, sizeof(pSleepWinData));

    if(QueueSin->nLen > 1)
    {
        for(ii=0; ii<QueueSin->nLen-1; ii++)
        {
            if(QueueSin->pSteep[ii+1].Mode == 2 && QueueSin->pSteep[ii].Mode < 2)
            {
                QueueSin->pSteep[ii].Mode = 3;
            }
        }            
        for(ii=QueueSin->nLen-1; ii>0; ii--)
        {
            if(QueueSin->pSteep[ii-1].Mode == 2 && QueueSin->pSteep[ii].Mode < 2)
            {
                QueueSin->pSteep[ii].Mode = 3;
            }
        }              
    }
    
    ii = QueueSin->nLen-1;
    jj = DataInLen-1;
    while(1)
    {
        if(ii < QueueSin->nLen-1)
        {
            Delta = (QueueSin->pSteep[ii+1].Xstart - QueueSin->pSteep[ii].Xstart)/4;
        }
        else
        {
            Delta = Window;
        }
        
        if(jj <= 0)
        {
            break;
        }
        else if(ii <= 0)
        {
            pSleepWinData[MedianDataLen] = my_abs(DataIn[jj]);
            MedianDataLen++;
        }
        else if (((QueueSin->pSteep[ii].Mode >= 2 && jj <= QueueSin->pSteep[ii].Xend*2 \
            - QueueSin->pSteep[ii].Xstart) \
            || jj <= QueueSin->pSteep[ii].Xend+my_min((QueueSin->pSteep[ii].Xend \
            -QueueSin->pSteep[ii].Xstart)/2, Delta)) \
            && jj >= QueueSin->pSteep[ii].Xstart-Window)
        {
            jj = QueueSin->pSteep[ii].Xstart - Window - 1;
            ii--;
        }
        else
        {
            pSleepWinData[MedianDataLen] = my_abs(DataIn[jj]);
            MedianDataLen++;
        }
        jj--;
    }

    //从小到大排序
    qsort(pSleepWinData, MedianDataLen, sizeof(int), IntCmp);
    
    return pSleepWinData[MedianDataLen/2];
}

static int MedianABS(int* DataIn, int DataInLen)
{
    int DataOut, i;
    memset(pSleepWinData, 0, sizeof(pSleepWinData));
    DataInLen = my_min(DataInLen, NIBP_STEEP_WINDOW_LEN); //add by QWB
    for(i=0; i < DataInLen; i++)
    {
        pSleepWinData[i] = my_abs(DataIn[i]);
    }
    qsort(pSleepWinData, DataInLen, sizeof(int), IntCmp);
    DataOut = pSleepWinData[(int)(DataInLen*0.4)];
    return DataOut;
}

static int GetStepFeature(  int *DataIn,
                            int DataLen,
                            int Start,
                            int End,
                            int *StepStart,
                            int *StepEnd)
{
    int DataLast, MaxNow = 0, MaxIndex = 0;
	int Enable = 0;
    int ii;

    for (ii = Start; ii < End; ii++)
    {
        if (DataIn[ii] > MaxNow)
        {
            MaxNow = DataIn[ii];
            MaxIndex = ii;
        }
    }

    DataLast = MaxNow;
	*StepEnd = DataLen-1;
    for (ii = MaxIndex+1; ii < DataLen; ii++)
    {
        if (DataIn[ii] > 0 && (DataIn[ii] <= DataLast || DataIn[ii] > MaxNow*0.2))
        {
            DataLast = DataIn[ii];
        }
        else
        {
			*StepEnd = ii-1;
            break;
		}
    }
    DataLast = MaxNow;
    *StepStart = 0;
    for (ii = MaxIndex-1; ii > 0; ii--)
    {
        if (DataIn[ii] > 0 && (DataIn[ii] <= DataLast || DataIn[ii] > MaxNow*0.2))
        {
            DataLast = DataIn[ii];
        }
        else
        {
            *StepStart = ii+1;
            break;
        }
    }
	if (*StepEnd - *StepStart < 35)
		Enable = 1;

	return Enable;
}

static void GetSteepFalling(int *DataIn,
                            int DataLen,
                            int cut1,
                            float cut2,
                            float cut3,
                            QUEUE_S *pQueue)
{
	int DataUseLen;

    int ii;
    int DeltaMax; 
    int StepStart, StepEnd;

    int Mode = MODE_START;
    int MaxNow = 0;
    int MinNow = 0;
    int Start = 0;
    int End = 0;   
    int DeltaUpLast = 0;
    int DeltaDownLast = 0;
    int DeltaUpNow = 0;
    int DeltaDownNow = 0;

    DataLen = my_min(NIBP_STEEP_WINDOW_LEN, DataLen);
    memset(pSleepWindownData2, 0, sizeof(pSleepWindownData2));

	pQueue->nLen = 0;
    if (DataLen > 0)
    {
		Diff_N(DataIn, DataLen, pSleepWindownData2);
		DataUseLen = DataLen-1;
		MaxNow = pSleepWindownData2[0];
        MinNow = pSleepWindownData2[0];

        for (ii = 0; ii < DataUseLen; ii++)
        {
            if (pSleepWindownData2[ii] > MaxNow)
            {
                MaxNow = pSleepWindownData2[ii];
                DeltaUpNow = MaxNow - MinNow;
            }
            if (pSleepWindownData2[ii] < MinNow)
            {
                MinNow = pSleepWindownData2[ii];
                DeltaDownNow = MaxNow - MinNow;
            }            
            DeltaMax = my_max(DeltaUpLast, DeltaDownLast);

            if ((Mode == MODE_START || Mode == MODE_DOWN) \
                && pSleepWindownData2[ii] < MaxNow - my_max(cut1+DeltaMax*cut3, DeltaMax*cut2) \
                && pSleepWindownData2[ii] <= 0)
            {
                MinNow = pSleepWindownData2[ii];
                End = ii;
                if (Mode == MODE_DOWN \
                    && End - Start < MAX_STEP \
                    && (pQueue->nLen) < MAX_STEP_LEN-1)
                {                 
                    if(GetStepFeature(pSleepWindownData2, DataUseLen, Start, End, &StepStart, &StepEnd))
                    {  
						if(pQueue->nLen == 0 || StepStart - pQueue->pSteep[pQueue->nLen-1].Xstart > 30)
                        {
							pQueue->pSteep[pQueue->nLen].Xstart = StepStart;
							pQueue->pSteep[pQueue->nLen].Xend = StepEnd;
							pQueue->pSteep[pQueue->nLen].Ystart = DataIn[StepStart];
							pQueue->pSteep[pQueue->nLen].Yend = DataIn[StepEnd];
							pQueue->nLen ++; 
						}
					}
                }
				if (Mode == MODE_DOWN)
				{
                    DeltaUpLast = DeltaUpNow;
                }
                Mode = MODE_UP;
            }
            else if ((Mode == MODE_START || Mode == MODE_UP) \
                && pSleepWindownData2[ii] > MinNow + my_max(cut1+DeltaMax*cut3, DeltaMax*cut2) \
                && pSleepWindownData2[ii] > my_max(cut1+DeltaMax*cut3, DeltaMax*cut2)*0.75)
            {
                MaxNow = pSleepWindownData2[ii];
                Start = ii;
                if (Mode == MODE_UP)
                {
                    DeltaDownLast = DeltaDownNow;
                }
                Mode = MODE_DOWN;
            }
        }
    }
}


static void GetSteepRising(int* DataIn,       //input,滤波后的数据指针
                    int DataLen,        //input,滤波后的数据长度
                    QUEUE_S* pQueueSin, //input,原始数据的队列
                    QUEUE_S* pQueueSnew,//input,临时数据的队列
                    int MinWindow,      //input,
                    int MaxWindow,      //input,
                    int MaxCut,         //input,
                    int MinCut,         //input,
                    int MinIn)          //input,
{
    
    int Median, Mode2en = 0, Mode2start, Mode2end, AllCut;
    int DataUseLen, WindowEnd, MaxData, MaxIndex, MinData, MinIndex;
    int i, j, k;
    
    memset(pSleepWindownData2, 0, sizeof(pSleepWindownData2));

    Diff_N(DataIn, DataLen, pSleepWindownData2);//微分运算
    Diff_N(pSleepWindownData2, DataLen-1, pSleepWindownData2);//微分运算
    
    Zoom(pSleepWindownData2, DataLen-2, 100, pSleepWindownData2);//数据的缩放
    
    for(i = 0; i < DataLen-2; i++)  //平方运算
    {
        pSleepWindownData2[i] = pSleepWindownData2[i]*pSleepWindownData2[i]*GetDirection(pSleepWindownData2[i]);
    }

    //去掉Q的数据，中值计算
    Median = MedianButQ_Abs(pSleepWindownData2, DataLen-2, pQueueSin, 1);
    Median = my_max(1, Median);
    
    for(i = 0; i < DataLen-2; i++)
    {
        pSleepWindownData2[i] = pSleepWindownData2[i]*1000/Median;
    }
    DataUseLen = DataLen-2;
    
    MaxCut = my_max(Max_N(pSleepWindownData2, DataLen-2)/10, MaxCut*1000);
    MinCut = my_min(Min_N(pSleepWindownData2, DataLen-2)/10, MinCut*1000);
    AllCut = (MaxCut - MinCut)*1.2;

    i = 0;
    while(1)
    {
        if(i >= DataUseLen-1) break;
        if(Mode2en > 0)
        {
            Mode2en = Mode2en-1;
        }
        WindowEnd = my_min(DataUseLen-1, i+MaxWindow-1);
        MaxData = Max_Index_N(&pSleepWindownData2[i], WindowEnd-i+1, &MaxIndex);
        MinData = Min_Index_N(&pSleepWindownData2[i], WindowEnd-i+1, &MinIndex);
        MaxIndex = i+MaxIndex-1;
        MinIndex = i+MinIndex-1;
        
        if((MaxData > MaxCut) && (MinData < MinCut) && ((MaxData-MinData)>AllCut) \
            && (MinIndex-MaxIndex >= MinWindow) && (MinIndex < WindowEnd))
        {
            k = MinIndex;
            for(j=MaxIndex-1; j>=0; j--)
            {
                if(pSleepWindownData2[j] <= pSleepWindownData2[MaxIndex]*0.9)
                {
                    if(DataIn[j] > MinIn)
                    {
                        for(k = MinIndex; k<DataUseLen-1; k++)
                        {
                            if((pSleepWindownData2[k] >= pSleepWindownData2[MinIndex]*0.9) \
                                || (DataIn[k+1]<DataIn[k]))
                            {
                                pQueueSnew->pSteep[pQueueSnew->nLen].Xstart = j;
                                pQueueSnew->pSteep[pQueueSnew->nLen].Ystart = DataIn[j];
                                pQueueSnew->pSteep[pQueueSnew->nLen].Xend = k;
                                pQueueSnew->pSteep[pQueueSnew->nLen].Yend = DataIn[k];
                                pQueueSnew->pSteep[pQueueSnew->nLen].Mode = 1;
                                pQueueSnew->nLen++;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            i = my_max(i+MaxWindow, k+MinWindow);
        }
        else if((MaxData > MaxCut/2) && (MinData < MinCut/2) \
            && (MaxData-MinData > AllCut/2) && (MinIndex-MaxIndex >= MinWindow) \
            && (MinIndex < WindowEnd))
        {
            k = MinIndex;
            for(j=MaxIndex-1; j>=0; j--)
            {
                if(pSleepWindownData2[j] <= pSleepWindownData2[MaxIndex]*0.9)
                {
                    if(DataIn[j] > MinIn)
                    {
                        for(k = MinIndex; k<DataUseLen-1; k++)
                        {
                            if((pSleepWindownData2[k] >= pSleepWindownData2[MinIndex]*0.9) \
                                || (DataIn[k+1]<DataIn[k]))
                            {
                                Mode2start = j;
                                Mode2end = k;
                                Mode2en = 2;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            i = k+1;
        }
        else if((MaxData > MaxCut/2) && (MinData < MinCut/2) \
            && (MaxData - MinData > AllCut/2) && (MaxIndex - MinIndex >= MinWindow) \
            && (MinIndex < WindowEnd) && (Mode2en >= 1))
        {
            pQueueSnew->pSteep[pQueueSnew->nLen].Xstart = Mode2start;
            pQueueSnew->pSteep[pQueueSnew->nLen].Ystart = DataIn[Mode2start];
            pQueueSnew->pSteep[pQueueSnew->nLen].Xend = Mode2end;
            pQueueSnew->pSteep[pQueueSnew->nLen].Yend = DataIn[Mode2end];
            pQueueSnew->pSteep[pQueueSnew->nLen].Mode = 2;
            pQueueSnew->nLen++;
            Mode2en = 0;
            i = my_max(i+MaxWindow, Mode2end+MinWindow);
        }
        else if((MaxData > MaxCut) && (MaxIndex > i))
        {
            i = MaxIndex;
        }
        else
        {
            i = i+MaxWindow;
        }
    } //end of while(1)
}

//把新队列合并到当前队列中
static void GetUnitList(QUEUE_S* QueueSnow, QUEUE_S* QueueSnew)
{
    int mean;
    int i, j;
    
    if (QueueSnew->nLen == 0)
    {
        return;
    }
    else if (QueueSnow->nLen < 2)
    {
        *QueueSnow = *QueueSnew;
    }
    else
    {
        mean = 0;
        for (i = 0; i < QueueSnow->nLen-1; i++)
        {
            mean += QueueSnow->pSteep[i + 1].Xstart - QueueSnow->pSteep[i].Xstart;
            mean += QueueSnow->pSteep[i + 1].Xend - QueueSnow->pSteep[i].Xend;
        }
        mean /= (QueueSnow->nLen-1)*2;

        for (i = 0; i < QueueSnew->nLen; i++)
        {
            for (j = QueueSnow->nLen-1; j >= 0; j--)
            {
                if ((my_abs(QueueSnew->pSteep[i].Xstart - QueueSnow->pSteep[j].Xstart) <= mean/5) \
                    || (my_abs(QueueSnew->pSteep[i].Xend - QueueSnow->pSteep[j].Xend) <= mean/5))
                {
                    break;
                }
                else if (QueueSnew->pSteep[i].Xstart > QueueSnow->pSteep[j].Xstart)
                {
                    Queue_S_Insert(QueueSnow, j+1, QueueSnew->pSteep[i]);
                    break;
                }
            }
        }
    }
}

//int p_temp_queue_int[QUEUE_S_LEN];
static void GetFallingUnitList(int* pDataIn,    //滤波后的数据指针
                               int* pDataLen,   //滤波后的数据长度
                               QUEUE_S* pQueueS,    //当前的队列
                               QUEUE_S* QueueSnew,  //待合并的新的临时队列
                               int *pDiffList,
                               int *pCountList,
                               int *pMeanList,
                               int *pPulse_ms)  
{
	STEEP_UNIT NewSteep;
    int index = 0, jj;
	int sum = 0;
	int	MeanH = 0;
	int CountDiff = 0;
	int	MeanDiff = 0;
    int pulse_ms_new = 0;

    //NIBPAlgo_DBG("22222222222===============QueueSnew->nLen=%d",QueueSnew->nLen);

    if(QueueSnew->nLen > 0)
    {
        //memset(p_temp_queue_int, 0, sizeof(p_temp_queue_int));//-DEBUG-
        for(index = 0; index < QueueSnew->nLen; index++)
        {
            p_temp_queue_int[index] = QueueSnew->pSteep[index].Xstart;
        }
        qsort(p_temp_queue_int, QueueSnew->nLen, sizeof(int), IntCmp);
        NewSteep.Xstart = p_temp_queue_int[QueueSnew->nLen/2]+(float)0.5;

        //memset(p_temp_queue_int, 0, sizeof(p_temp_queue_int));//-DEBUG-
        for(index = 0; index < QueueSnew->nLen; index++)
        {
            p_temp_queue_int[index] = QueueSnew->pSteep[index].Ystart;
        }
        qsort(p_temp_queue_int, QueueSnew->nLen, sizeof(int), IntCmp);
        NewSteep.Ystart = p_temp_queue_int[QueueSnew->nLen/2]+(float)0.5;

        //memset(p_temp_queue_int, 0, sizeof(p_temp_queue_int));//-DEBUG-
        for(index = 0; index < QueueSnew->nLen; index++)
        {
            p_temp_queue_int[index] = QueueSnew->pSteep[index].Xend;
        }
        qsort(p_temp_queue_int, QueueSnew->nLen, sizeof(int), IntCmp);
        NewSteep.Xend = p_temp_queue_int[QueueSnew->nLen/2]+(float)0.5;
        
        //memset(p_temp_queue_int, 0, sizeof(p_temp_queue_int));//-DEBUG-
        for(index = 0; index < QueueSnew->nLen; index++)
        {
            p_temp_queue_int[index] = QueueSnew->pSteep[index].Yend;
        }
        qsort(p_temp_queue_int, QueueSnew->nLen, sizeof(int), IntCmp);
        NewSteep.Yend = p_temp_queue_int[QueueSnew->nLen/2]+(float)0.5;
        
        //memset(p_temp_queue_int, 0, sizeof(p_temp_queue_int));//-DEBUG-
        for(index = 0; index < QueueSnew->nLen; index++)
        {
            p_temp_queue_int[index] = QueueSnew->pSteep[index].Mode;
        }
        qsort(p_temp_queue_int, QueueSnew->nLen, sizeof(int), IntCmp);
        NewSteep.Mode = p_temp_queue_int[QueueSnew->nLen/2]+(float)0.5;
    }
    else
    {
        NewSteep.Xstart = 0;
        NewSteep.Ystart = Mean_N(pDataIn, *pDataLen);
        NewSteep.Xend = 0;
        NewSteep.Yend = NewSteep.Ystart;
        NewSteep.Mode = 0;
    }

	if(NULL == QueueSnew || QueueSnew->nLen < 2)
	{
        if(QueueSnew->nLen < 1){
		    MeanH = 0;
        }
        else{
            MeanH = QueueSnew->pSteep[0].Yend - QueueSnew->pSteep[0].Ystart;
        }
	    CountDiff = 0;
		MeanDiff = 0;
    }
	else
    {
		sum = 0;
		for(index=0; index<QueueSnew->nLen; index++)
		{
			sum += (QueueSnew->pSteep[index].Yend - QueueSnew->pSteep[index].Ystart);
        }
		MeanH = sum/QueueSnew->nLen;
		sum = 0;
		for(index=0; index<QueueSnew->nLen-1; index++)
		{
			sum += (QueueSnew->pSteep[index+1].Xstart - QueueSnew->pSteep[index].Xstart);
        }
		CountDiff = QueueSnew->nLen-1;
		MeanDiff = sum/CountDiff;
        pulse_ms_new = MeanDiff * 1000 / NIBP_SAMPLE_FS;
        if (!g_pulse_average_ms)
        {
            g_pulse_average_ms = pulse_ms_new;
        }
        else
        {
            g_pulse_average_ms = (pulse_ms_new + g_pulse_average_ms)/2;
        }
        NIBPAlgo_DBG("==========MeanDiff=%d",MeanDiff);
	}
    
    *pPulse_ms = g_pulse_average_ms;//每个阶梯的脉搏周期平均
    NIBPAlgo_DBG("==========g_pulse_average_ms=%d",g_pulse_average_ms);

	if(pQueueS->nLen == 0 || pQueueS->pSteep[pQueueS->nLen-1].Ystart > NewSteep.Ystart)
	{
		pQueueS->pSteep[pQueueS->nLen] = NewSteep;
		pMeanList[pQueueS->nLen] = MeanH;//
		pCountList[pQueueS->nLen] = CountDiff;//
		NIBPAlgo_DBG("1111====pCountList[%d]=%d",pQueueS->nLen,pCountList[pQueueS->nLen]);
		pDiffList[pQueueS->nLen] = MeanDiff;//
    }
	else
    {
		for(index=0; index<pQueueS->nLen; index++)
        {
			if(pQueueS->pSteep[index].Ystart <= NewSteep.Ystart)
            {
				for(jj=pQueueS->nLen; jj>index; jj--)
				{
					pQueueS->pSteep[jj] = pQueueS->pSteep[jj-1];
					pMeanList[jj] = pMeanList[jj-1];//
					pCountList[jj] = pCountList[jj-1];//
					pDiffList[jj] = pDiffList[jj-1];//
                }
				pQueueS->pSteep[index] = NewSteep;
                
				pMeanList[index] = MeanH;//
				pCountList[index] = CountDiff;//
		        NIBPAlgo_DBG("2222====pCountList[%d]=%d",index,pCountList[index]);
		        pDiffList[index] = MeanDiff;//                
				break;
			}
		}
	}
    pQueueS->nLen++;
    /*
    NIBPAlgo_DBG("GetFallingUnitList NewSteep.Ystart=%d",NewSteep.Ystart);
    for(index=0; index<pQueueS->nLen; index++)
    {
        NIBPAlgo_DBG("pQueueS->pSteep[%d].Ystart=%d",index,pQueueS->pSteep[index].Ystart);
	}
	*/
}

//移动队列(X轴)
static void SteepMove(QUEUE_S* QueueS, int len)
{
    int i;
    for (i = 0; i < QueueS->nLen; i++)
    {
        QueueS->pSteep[i].Xstart += len;
        QueueS->pSteep[i].Xend += len;
    }
}

static int NIBPgetBPM(QUEUE_S* pQueueS, int nLen)
{
    int BPMout = 0, index = 0, BpmUse = 0, BpmCount = 0;
    int BPMIndex = 0;
    int DeltaMaxIndex = 0;
    int startIndex = 0, endIndex = 0;
    
    if((NULL == pQueueS) || (pQueueS->nLen < 2))
    {
        return BPMout;
    }

    if(pQueueS->nLen >= nLen)
    {
        for(index = 0; index < pQueueS->nLen; index++)
        {
            p_temp_queue_int[index] = \
                (pQueueS->pSteep[index].Yend - pQueueS->pSteep[index].Ystart) \
                /(pQueueS->pSteep[index].Xend-pQueueS->pSteep[index].Xstart);
        }
        Max_Index_N(p_temp_queue_int, pQueueS->nLen, &DeltaMaxIndex);

        startIndex = DeltaMaxIndex;
        endIndex = DeltaMaxIndex;
        for(;endIndex-startIndex < nLen-1;)
        {
            if(startIndex>0 && endIndex<pQueueS->nLen-1)
            {
                if(p_temp_queue_int[startIndex-1] > p_temp_queue_int[endIndex+1])
                {
                    startIndex--;
                }
                else
                {
                    endIndex++;
                }
            }
            else if(startIndex <= 0)
            {
                endIndex++;
            }
            else
            {
                startIndex--;
            }
        }
    }
    else
    {
        startIndex = 0;
        endIndex = pQueueS->nLen-1;
    }
    
    for(index=startIndex; index<endIndex; index++)
    {
        pNIBPgetBPMBPMList[index-startIndex] = \
            pQueueS->pSteep[index+1].Xstart - pQueueS->pSteep[index].Xstart;
    }
    
    //排序
    qsort(pNIBPgetBPMBPMList, endIndex-startIndex, sizeof(int), IntCmp);
    BPMout = pNIBPgetBPMBPMList[(endIndex-startIndex)/2];//计算均值

    //过滤不合理的数据
    for(index = startIndex; index < endIndex; index++)
    {
        if(my_abs(pQueueS->pSteep[index+1].Xstart-pQueueS->pSteep[index].Xstart-BPMout) \
            < BPMout*0.4)
        {
            BpmUse = BpmUse+pQueueS->pSteep[index+1].Xstart-pQueueS->pSteep[index].Xstart;
            BpmCount++;
        }
    }
    
    if(0 != BpmUse) //计算脉率
    {
        BPMout = (float)6000*BpmCount/BpmUse+(float)0.5;
    }
    return BPMout;
}

static void NIBPgetFallingPulseInterval(QUEUE_S* pQueueS,
                                        int *pDiffList,
                                        int *pCountList,
                                        int *MeanList,
                                        int ListIndex)
{
    int index = 0;
    int sum = 0;
    pCountList[ListIndex] = 0;
    *(pDiffList + ListIndex*10 + pCountList[ListIndex]) = 0;
    
    NIBPAlgo_DBG("111111111===============pQueueS->nLen=%d",pQueueS->nLen);

    
    if(NULL == pQueueS || pQueueS->nLen < 2)
    {
        return;
    }
    
    for(index=0; index < pQueueS->nLen-1; index++)
    {
        *(pDiffList + ListIndex*10 + pCountList[ListIndex]) = \
            (pQueueS->pSteep[index+1].Xstart - pQueueS->pSteep[index].Xstart);
        pCountList[ListIndex]++;
    }
    for(index=0; index<pQueueS->nLen; index++)
    {
        sum += (pQueueS->pSteep[index].Yend - pQueueS->pSteep[index].Ystart);
    }
    MeanList[ListIndex] = sum/pQueueS->nLen;
}

static void QerrCutFalling(QUEUE_S* QueueS)
{
    int i,j;
	int MaxEnergy = 0, MaxIndex = 0;
	int NewLen = 0;
    int ii;
    
    if(QueueS->nLen <= 0)
    {
        return;
    }
    for(i = 1,j = 1; i < QueueS->nLen; i++)
    {
        if(QueueS->pSteep[i].Xstart - QueueS->pSteep[i-1].Xend \
            > (QueueS->pSteep[i-1].Xend - QueueS->pSteep[i-1].Xstart)*0.8)
        {
            QueueS->pSteep[j] = QueueS->pSteep[i];
            j++;
        }
    }
    QueueS->nLen = j;
    
	for(ii = 0; ii < QueueS->nLen; ii++)
    {
		if(QueueS->pSteep[ii].Yend-QueueS->pSteep[ii].Ystart > MaxEnergy)
        {
            MaxEnergy = QueueS->pSteep[ii].Yend-QueueS->pSteep[ii].Ystart;
			MaxIndex = ii;
		}
	}
    NewLen = 0;
    for(ii = 0; ii < QueueS->nLen; ii++)
    {
		if(QueueS->pSteep[ii].Yend-QueueS->pSteep[ii].Ystart > MaxEnergy*0.5)
		{
            QueueS->pSteep[NewLen].Xstart = QueueS->pSteep[ii].Xstart;
    		QueueS->pSteep[NewLen].Xend = QueueS->pSteep[ii].Xend;
            QueueS->pSteep[NewLen].Ystart = QueueS->pSteep[ii].Ystart;
    		QueueS->pSteep[NewLen].Yend = QueueS->pSteep[ii].Yend;
    		NewLen++;
        }
	}
	QueueS->nLen = NewLen;
}

static void QerrCutRising(QUEUE_S* QueueS)
{
    int i,j;
	int index = 0;

	if(QueueS->nLen <= 0)
    {
        return;
    }
    for(index = 0; index < QueueS->nLen; index++)
    {
        if(QueueS->pSteep[index].Xend - QueueS->pSteep[index].Xstart == 0)
        {
            p_temp_queue_int[index] = 0;
        }
        else
        {
            p_temp_queue_int[index] = (QueueS->pSteep[index].Yend - QueueS->pSteep[index].Ystart)/(QueueS->pSteep[index].Xend - QueueS->pSteep[index].Xstart) - (QueueS->pSteep[QueueS->nLen-1].Yend - QueueS->pSteep[0].Ystart)/(QueueS->pSteep[QueueS->nLen-1].Xend - QueueS->pSteep[0].Xstart)*FOR_BASELINE;
        }
    }
    for(i = 1,j = 1; i < QueueS->nLen; i++)
    {
        if(p_temp_queue_int[i] > p_temp_queue_int[i-1]*0.4)
        {
            QueueS->pSteep[j] = QueueS->pSteep[i];
            j++;
        }
		else
		{
			p_temp_queue_int[i] = p_temp_queue_int[i-1];
		}
    }
    
    QueueS->nLen = j;
}

static void NIBPgetRisingQ(int* pDataIn,
                            int* pDataFil,
                            int* pDataFilLen,
                            QUEUE_S* pQueueS)
{
    Queue_S_Init(&g_nibpQueueSnew);

    //删除最旧一段数据
    memcpy(pDataFil, &pDataFil[NIBP_NEW_WINDOW_LEN],
        (NIBP_STEEP_WINDOW_LEN-NIBP_NEW_WINDOW_LEN)*sizeof(int));
    
    //FIR滤波算法
    Filter(fFir_6_100hz_35s,
        NIBP_DATA_FILTER_SIZE,
        &pDataIn[NIBP_FILTER_WINDOW_LEN-NIBP_NEW_WINDOW_LEN-NIBP_DATA_FILTER_SIZE+1],
        NIBP_NEW_WINDOW_LEN+NIBP_DATA_FILTER_SIZE-1,
        NIBP_NEW_WINDOW_LEN,
        &pDataFil[NIBP_STEEP_WINDOW_LEN-NIBP_NEW_WINDOW_LEN],
        pDataFil[NIBP_STEEP_WINDOW_LEN-NIBP_NEW_WINDOW_LEN-1]);

    //更新滤波数据的总长度
    *pDataFilLen += NIBP_NEW_WINDOW_LEN;// = my_min(STEEP_WINDOW_LEN, *pDataFilLen+NEW_WINDOW_LEN);
    
    if(*pDataFilLen >= NIBP_STEEP_WINDOW_LEN)
    {
        //X轴方向移动队列
        SteepMove(pQueueS, (-1)*NIBP_NEW_WINDOW_LEN);

        if(pQueueS->nLen <= 1)
        {
            GetSteepRising( pDataFil,
                            NIBP_STEEP_WINDOW_LEN,
                            pQueueS,
                            &g_nibpQueueSnew,
                            6,
                            24,
                            30,
                            -30,
                            20*10000);
        }
        else 
        {
            GetSteepRising( pDataFil,
                            NIBP_STEEP_WINDOW_LEN,
                            pQueueS,
                            &g_nibpQueueSnew,
                            6,
                            24,
                            15,
                            -15,
                            20*10000);
        }
        
        GetUnitList(pQueueS, &g_nibpQueueSnew);//把新队列合并至当前队列
        QerrCutRising(pQueueS);
    }
}

void NIBPProcessRQ(QUEUE_S* pQueueS, QUEUE_S* pNewQueueS)
{
    int index = 0;
    int DeltaMax = 0;
    int DeltaMaxIndex = 0;

    if(pQueueS->nLen <= 0)
    {
        return;
    }
    for(index = 0; index < pQueueS->nLen; index++)
    {
        if(pQueueS->pSteep[index].Xend - pQueueS->pSteep[index].Xstart == 0)
        {
            p_temp_queue_int[index] = 0;
        }
        else
        {
            p_temp_queue_int[index] = (pQueueS->pSteep[index].Yend - pQueueS->pSteep[index].Ystart) \
                /(pQueueS->pSteep[index].Xend - pQueueS->pSteep[index].Xstart) \
                - (pQueueS->pSteep[pQueueS->nLen-1].Yend - pQueueS->pSteep[0].Ystart) \
                /(pQueueS->pSteep[pQueueS->nLen-1].Xend - pQueueS->pSteep[0].Xstart)*FOR_BASELINE;
        }
    }
    DeltaMax = Max_Index_N(p_temp_queue_int, pQueueS->nLen, &DeltaMaxIndex);

    for(index = 0; index < pQueueS->nLen; index++)
    {
        if(pQueueS->pSteep[index].Yend-pQueueS->pSteep[index].Ystart >= DeltaMax*(float)0.4)
        {
            pNewQueueS->pSteep[pNewQueueS->nLen].Xstart = pQueueS->pSteep[index].Xstart;
            pNewQueueS->pSteep[pNewQueueS->nLen].Xend = pQueueS->pSteep[index].Xend;
            pNewQueueS->pSteep[pNewQueueS->nLen].Ystart = pQueueS->pSteep[index].Ystart;
            pNewQueueS->pSteep[pNewQueueS->nLen].Yend = pQueueS->pSteep[index].Yend;
            pNewQueueS->pSteep[pNewQueueS->nLen].Mode = pQueueS->pSteep[index].Mode;
            pNewQueueS->pSteep[pNewQueueS->nLen].eva = pQueueS->pSteep[index].eva;
            pNewQueueS->nLen++;
        }
    }
}

static bool NIBPgetPressureMax(QUEUE_S* pQueueS,        //input,待分析的数据源
                                    int *pBPM,          //output,脉搏
                                    int *pPressureMax,  //output,血压最大值结果
                                    int LatestP)
{
    int index = 0;
    int DeltaMax = 0;
    int DeltaMaxIndex = 0;

    for(index = 0; index < pQueueS->nLen; index++)
    {
        if(pQueueS->pSteep[index].Xend - pQueueS->pSteep[index].Xstart == 0)
        {
            p_temp_queue_int[index] = 0;
        }
        else
        {
            p_temp_queue_int[index] = \
                (pQueueS->pSteep[index].Yend - pQueueS->pSteep[index].Ystart) \
                /(pQueueS->pSteep[index].Xend - pQueueS->pSteep[index].Xstart) \
                - (pQueueS->pSteep[pQueueS->nLen-1].Yend - pQueueS->pSteep[0].Ystart) \
                /(pQueueS->pSteep[pQueueS->nLen-1].Xend - pQueueS->pSteep[0].Xstart) \
                *FOR_BASELINE;
        }
    }
    DeltaMax = Max_Index_N(p_temp_queue_int, pQueueS->nLen, &DeltaMaxIndex);

    for(index = DeltaMaxIndex+1; index < pQueueS->nLen && DeltaMaxIndex != pQueueS->nLen-1; index++)
    {
        if((DeltaMax >= 1000) \
            && ((NIBP_STEEP_WINDOW_LEN - pQueueS->pSteep[pQueueS->nLen-1].Xend \
            > my_max(250, 6000*2.4/(*pBPM)) \
            && (p_temp_queue_int[index]+p_temp_queue_int[index-1])/2 \
            < DeltaMax*(float)MAX_PRESSURE_COEFFICIENT_NOBPM) \
            || ((p_temp_queue_int[index]+p_temp_queue_int[index-1])/2 \
            < DeltaMax*(float)MAX_PRESSURE_COEFFICIENT)) \
            && (LatestP > 100*10000))
        {
            if((NIBP_STEEP_WINDOW_LEN - pQueueS->pSteep[pQueueS->nLen-1].Xend \
                > my_max(250, 6000*2.4/(*pBPM))) \
                && ((p_temp_queue_int[index]+p_temp_queue_int[index-1])/2 \
                < DeltaMax*(float)MAX_PRESSURE_COEFFICIENT_NOBPM))
            {
                *pPressureMax = pQueueS->pSteep[pQueueS->nLen-1].Ystart+20*10000;
            }
            else
            {
                *pPressureMax = pQueueS->pSteep[index].Ystart+20*10000;
            }
            return true;
        }
    }
    return false;
}

//int p_temp_queue_int[QUEUE_S_LEN];
static bool NIBPProcessQ(QUEUE_S* pQueueS)//I/O,会重新初始化pQueueS
{
    int index = 0;
    int Enable = 0;
    int DeltaMax = 0, DeltaMaxIndex = 0, startIndex = 0, endIndex = pQueueS->nLen-1;
    
    Queue_S_Init(&g_nibpQueueSnew);
    

    for(index = 0; index < pQueueS->nLen; index++)
		if(pQueueS->pSteep[index].Yend != pQueueS->pSteep[index].Ystart)
			Enable++;
    if(Enable <= 4)
        return false;

    for(index = 0; index < pQueueS->nLen; index++)
    {
        p_temp_queue_int[index] = pQueueS->pSteep[index].Yend - pQueueS->pSteep[index].Ystart;
    }
    
    DeltaMax = Max_Index_N(p_temp_queue_int, pQueueS->nLen, &DeltaMaxIndex);
    
    for(index = DeltaMaxIndex; index >= 0; index--)//计算前半部分
    {
        if((p_temp_queue_int[index] < DeltaMax*(float)FOR_BASELINE2) || (index == 0))
        {
            startIndex = index;
            break;
        }
    }
    
    for(index = DeltaMaxIndex; index < pQueueS->nLen; index++)//计算后半部分
    {
        if((p_temp_queue_int[index] < DeltaMax*(float)FOR_BASELINE2) || (index == pQueueS->nLen-1))
        {
            endIndex = index;
            break;
        }
    }
    
    if(p_temp_queue_int[index] >= DeltaMax*(float)FOR_BASELINE2)
    {
        return false;
    }

    memcpy(g_nibpQueueSnew.pSteep, pQueueS->pSteep+startIndex, sizeof(STEEP_UNIT)*(endIndex-startIndex+1));
    g_nibpQueueSnew.nLen = endIndex-startIndex+1;

    Queue_S_Init(pQueueS);
    for(index = 0; index < g_nibpQueueSnew.nLen; index++)
    {
        if(g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Xstart != 0)
        {
            pQueueS->pSteep[pQueueS->nLen].Xstart = g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Xstart;
            pQueueS->pSteep[pQueueS->nLen].Ystart = g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Ystart;
            pQueueS->pSteep[pQueueS->nLen].Xend = g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Xend;
            pQueueS->pSteep[pQueueS->nLen].Yend = g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Yend;
            pQueueS->pSteep[pQueueS->nLen].Mode = g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Mode;
            pQueueS->nLen++;
        }
    }
    return true;
}

static void ForEnergy(QUEUE_S* StepQueue)
{
    int MaxEnergy = 0, MaxIndex = 0, DataBKLast, DataBKNow;
    int ii;

	for(ii = 0; ii < StepQueue->nLen; ii++)
	{
		p_temp_queue_int[ii] = StepQueue->pSteep[ii].Yend - StepQueue->pSteep[ii].Ystart;
        if(p_temp_queue_int[ii] > MaxEnergy)
        {
            MaxEnergy = p_temp_queue_int[ii];
            MaxIndex = ii;
        }
    }
	if(StepQueue->nLen >= 3)
    {
		DataBKLast = p_temp_queue_int[0];
		for(ii = 1; ii < StepQueue->nLen-1; ii++)
        {
			DataBKNow = p_temp_queue_int[ii];
			if(ii != MaxIndex)
            {
				if(p_temp_queue_int[ii] > my_max(DataBKLast, p_temp_queue_int[ii+1]))
				{
					p_temp_queue_int[ii] = my_max(DataBKLast, p_temp_queue_int[ii+1]);
                }
				else if(p_temp_queue_int[ii] < my_min(DataBKLast, p_temp_queue_int[ii+1]))
				{
					p_temp_queue_int[ii] = my_min(DataBKLast, p_temp_queue_int[ii+1]);
                }
			} 
			DataBKLast = DataBKNow;
		}
		for(ii = MaxIndex+1; ii < StepQueue->nLen; ii++)
        {
			if(p_temp_queue_int[ii] > p_temp_queue_int[ii-1])
			{
			    p_temp_queue_int[ii] = p_temp_queue_int[ii-1];
            }
		}
		for(ii = MaxIndex-1; ii >=0; ii--)
        {
			if(p_temp_queue_int[ii] > p_temp_queue_int[ii+1])
			{
			    p_temp_queue_int[ii] = p_temp_queue_int[ii+1];
            }
		}
	}

	for(ii = 0; ii < StepQueue->nLen; ii++)
	{
		StepQueue->pSteep[ii].Yend = p_temp_queue_int[ii] + StepQueue->pSteep[ii].Ystart;
    }
}

//int p_temp_queue_int[QUEUE_S_LEN];
static void GetFeatureList(QUEUE_S* pQueueS, FEATURE_LIST* pFeatureList)
{
#if 0
    int index = 0, QIndex = 0, QAfterIndex = QIndex;
    bool flag = true;
    memset(p_temp_queue_int, 0, pQueueS->nLen*sizeof(int));
    memset(pQueuesDataDeltaUse, 0, pQueueS->nLen*sizeof(int));

    for(index = 0; index < pQueueS->nLen; index++)
    {
        p_temp_queue_int[index] = pQueueS->pSteep[index].Yend - pQueueS->pSteep[index].Ystart;
    }

	pQueuesDataDeltaUse[0] = p_temp_queue_int[0];
	pQueuesDataDeltaUse[pQueueS->nLen-1] = p_temp_queue_int[pQueueS->nLen-1];
    for(index = 1; index < pQueueS->nLen-1; index++)
    {
        if((p_temp_queue_int[index] >= p_temp_queue_int[index-1]) \
            || (p_temp_queue_int[index] >= p_temp_queue_int[index+1]))
        {
            pQueuesDataDeltaUse[index] = p_temp_queue_int[index];
        }
        else
        {
            pQueuesDataDeltaUse[index] = my_min(p_temp_queue_int[index+1], p_temp_queue_int[index-1]);
        }
    }

    for(index = 2; index < pQueueS->nLen-2; index++)
    {
        if((p_temp_queue_int[index] > p_temp_queue_int[index-1]) \
            && (p_temp_queue_int[index] > p_temp_queue_int[index+1]) \
            && ((p_temp_queue_int[index-1] < p_temp_queue_int[index-2]) \
            || (p_temp_queue_int[index+1] < p_temp_queue_int[index+2])))
        {
            pQueuesDataDeltaUse[index] = (float)(pQueuesDataDeltaUse[index] \
                +my_max(pQueuesDataDeltaUse[index+1], pQueuesDataDeltaUse[index-1]))/2+(float)0.5;
        }
    }
#else
    int index = 0, QIndex = 0, QAfterIndex = QIndex;
    bool flag = true;
    memset(pQueuesDataDeltaUse, 0, pQueueS->nLen*sizeof(int));

    ForEnergy(pQueueS);

    for(index = 0; index < pQueueS->nLen; index++)
    {
        pQueuesDataDeltaUse[index] = pQueueS->pSteep[index].Yend - pQueueS->pSteep[index].Ystart;
    }
#endif
    pFeatureList->nLen = 0;
    for(index = 0; index < FEATURE_LEN;)
    {
        pFeatureList->Pressure[index] = pQueueS->pSteep[0].Ystart \
            + (float)(pQueueS->pSteep[pQueueS->nLen-1].Ystart - pQueueS->pSteep[0].Ystart) \
            /FEATURE_LEN*index;
        if((index == 0) || (pFeatureList->Pressure[index] > pQueueS->pSteep[QAfterIndex].Ystart))
        {
            QIndex = QAfterIndex;
            for(QAfterIndex = QIndex+1;\
                ((pQueueS->pSteep[QAfterIndex].Ystart <= pQueueS->pSteep[QIndex].Ystart) \
                ||  (pFeatureList->Pressure[index] > pQueueS->pSteep[QAfterIndex].Ystart))\
                && (QAfterIndex<pQueueS->nLen-1);\
                QAfterIndex++);           
        }
        if(pQueueS->pSteep[QAfterIndex].Ystart > pQueueS->pSteep[QIndex].Ystart)
        {
            pFeatureList->Energy[index] = pQueuesDataDeltaUse[QIndex] \
                + (float)(pQueuesDataDeltaUse[QAfterIndex]-pQueuesDataDeltaUse[QIndex]) \
                /(pQueueS->pSteep[QAfterIndex].Ystart-pQueueS->pSteep[QIndex].Ystart) \
                *(pFeatureList->Pressure[index]-pQueueS->pSteep[QIndex].Ystart);
//            pFeatureList->Energy[index] = pQueuesDataDeltaUse[QIndex-1] \
//                + (float)(pQueuesDataDeltaUse[QPrevIndex-1]-pQueuesDataDeltaUse[QIndex-1]) \
//                /(pQueueS->pSteep[QPrevIndex].Ystart-pQueueS->pSteep[QIndex].Ystart) \
//                *(pFeatureList->Pressure[index]-pQueueS->pSteep[QIndex].Ystart);
            pFeatureList->nLen++;
            index++;
        }
    }
}

static void FilFeatureList(FEATURE_LIST* pFeatureList)
{
    int i, EnergyMax;
    Filter(fFir_5_1000hz_85s,
        NIBP_FEATURE_FILTER_SIZE,
        pFeatureList->Energy,
        pFeatureList->nLen,
        pFeatureList->nLen-NIBP_FEATURE_FILTER_SIZE+1,
        pFeatureList->Energy,
        0);
    pFeatureList->nLen = pFeatureList->nLen-NIBP_FEATURE_FILTER_SIZE+1;
    
    EnergyMax = Max_N(pFeatureList->Energy, pFeatureList->nLen);
    for(i=0; i < pFeatureList->nLen; i++)
    {
        pFeatureList->Energy[i] = (float)pFeatureList->Energy[i]/EnergyMax*10000+(float)0.5;
        pFeatureList->Pressure[i] = pFeatureList->Pressure[i+NIBP_FEATURE_FILTER_SIZE/2];
    }
}

static void NIBPfromFeature(FEATURE_LIST* pFeature_list,
                            int* pDP,
                            int *pDE,
                            int* pSP,
                            int *pSE,
                            int* pMAP,
                            int *pMAE,
                            int *pBPM)
{
    int MaxData = 0, index = 0, MAPindex = 0;
    float KdNow = 0, KsNow = 0, KsOT = 0, KdOT = 0;
    
    if((NULL == pFeature_list) || (pFeature_list->nLen < 0))
    {
        return;
    }

    /*******************平均压*******************/
    for(index = 0; index < pFeature_list->nLen; index++)
    {
        if(pFeature_list->Energy[index] > MaxData)
        {
            MAPindex = index;
            MaxData = pFeature_list->Energy[index];
        }
    }
    *pMAP = pFeature_list->Pressure[MAPindex];
    *pMAE = pFeature_list->Energy[MAPindex];

    KdNow = *pMAP * (float)KDP / 10000 + (float)KDPbase;
    KsNow = *pMAP * (float)KSP / 10000 + (float)KSPbase;

    /*******************收缩压*******************/
    for(index = MAPindex+1; index < pFeature_list->nLen; index++)
    {
        KsOT = my_max((float)0,(float)(pFeature_list->Pressure[index]-pFeature_list->Pressure[MAPindex])/10000\
            - (float)KOTSPbase)*(float)KOTSP;
        if ((pFeature_list->Energy[index] <= my_min((float)KSPmax, my_max((float)KSPmin, KsNow+KsOT))*100) \
            && ((pFeature_list->Pressure[index] - pFeature_list->Pressure[MAPindex]) \
            > my_min((float)50000, (float)pFeature_list->Pressure[MAPindex]/10)))
        {
            *pSP = pFeature_list->Pressure[index];
            *pSE = pFeature_list->Energy[index];
            break;
        }
    }
    if(index == pFeature_list->nLen)
    {
        *pSP = pFeature_list->Pressure[pFeature_list->nLen-1/*(int)(pFeature_list->nLen*0.95)*/];
        *pSE = pFeature_list->Energy[pFeature_list->nLen-1];
    }
    
    /*******************舒张压*******************/
    for(index = MAPindex-1; index >= 0; index--)
    {
        KdOT = (float)my_max(0, (float)(pFeature_list->Pressure[MAPindex]-pFeature_list->Pressure[index])/10000 - (float)KOTDPbase) \
            *(float)KOTDP;
        if((pFeature_list->Energy[index] <= my_min((float)KDPmax, my_max((float)KDPmin, KdNow+KdOT))*100) \
            && ((pFeature_list->Pressure[MAPindex] - pFeature_list->Pressure[index]) \
            > my_min((float)50000, (float)pFeature_list->Pressure[MAPindex]/10)))
        {
            *pDP = pFeature_list->Pressure[index];
            *pDE = pFeature_list->Energy[index];
            break;
        }
    }
    if(index < 0)
    {
        *pDP = pFeature_list->Pressure[(int)(pFeature_list->nLen * 0.1)];
        *pDE = pFeature_list->Energy[(int)(pFeature_list->nLen * 0.1)];
    }
    ForResult(  p_nibpAlgoIn->lastDP,
                p_nibpAlgoIn->lastSP,
                p_nibpAlgoIn->lastBPM,
                pDP, pSP, pBPM);
}

static void NIBPgetFallingQ(int* pDataIn,   //I/O，存放滤波前、后的数据
                            int* pDataFil,
                            int* pDataLen,  //I/O，滤波前、后的数据长度
                            QUEUE_S* pQueueS)
{

    int index = 0;
    int DataFilLen = *pDataLen-NIBP_DATA_FILTER_SIZE+1;
    //滤波算法为向后滤波
    Filter(fFir_6_100hz_35s, NIBP_DATA_FILTER_SIZE, pDataIn, *pDataLen, DataFilLen, pDataFil, 0);
    *pDataLen = DataFilLen;
    GetSteepFalling(pDataFil, *pDataLen, WAVELET_CUT, 0.3, 0.1, pQueueS); 
}

static void NIBPInit(QUEUE_S* QueueS, FEATURE_LIST* pFeatureList)
{
    Queue_S_Init(QueueS);
    pFeatureList->nLen = 0;
    //g_intListIndex = 0;
    //g_intMaxEnergyIndex = 0;
    //g_intMaxEnergy = 0;
}

//@return: 数据中的脉搏个数(实质为波峰个数)
static int StepFromWave(int *WaveIn,    //待处理数据
                        int WaveInLen,  //待处理数据长度
                        int cut1,
                        float cut2,
                        float cut3)
{
    int ii;
    int DeltaMax; 
    int StepNum = 0;
    int Mode = MODE_START;
    int MaxNow = 0;
    int MinNow = 0;
    int Start = 0;
    int End = 0;   
    int DeltaUpLast = 0;
    int DeltaDownLast = 0;
    int DeltaUpNow = 0;
    int DeltaDownNow = 0;

    if (WaveInLen > 0)
    {
        MaxNow = WaveIn[0];
        MinNow = WaveIn[0];
        for (ii = 0; ii < WaveInLen; ii++)
        {
            //更新当前上升、下降幅值
            if (WaveIn[ii] > MaxNow)
            {
                MaxNow = WaveIn[ii];
                DeltaUpNow = MaxNow - MinNow;
            }
            if (WaveIn[ii] < MinNow)
            {
                MinNow = WaveIn[ii];
                DeltaDownNow = MaxNow - MinNow;
            }
            
            DeltaMax = my_max(DeltaUpLast, DeltaDownLast);
            if ((Mode == MODE_START || Mode == MODE_DOWN) \
                && WaveIn[ii] < MaxNow - my_max(cut1+DeltaMax*cut3, DeltaMax*cut2))//当前值小于窗口上限
            {
                MinNow = WaveIn[ii];
                End = ii;
                if (Mode == MODE_DOWN && End - Start < MAX_STEP)
                {                 
                    StepNum++;
                }
                if (Mode == MODE_DOWN)
                    DeltaUpLast = DeltaUpNow;
                Mode = MODE_UP;
            }
            else if ((Mode == MODE_START || Mode == MODE_UP) \
                && WaveIn[ii] > MinNow + my_max(cut1+DeltaMax*cut3, DeltaMax*cut2))//当前值大于窗口下限
            {
                MaxNow = WaveIn[ii];
                Start = ii;
                if (Mode == MODE_UP)
                    DeltaDownLast = DeltaDownNow;
                Mode = MODE_DOWN;
            }
        }
    }
    return StepNum;
}

static int Sum_ABS_N(int* pt, int nLen)
{
    int sum = 0;
    int i;
    for (i = 0; i < nLen; i++)
    {
        sum += my_abs(pt[i]);
    }
    return sum;
}

static int NIBP_Step_Rising(int* pDataIn,
                            int DataLen,
                            int* pDataFil,
                            int* pDataFilLen,
                            QUEUE_S* pQueueS,
                            int* power,
                            int* pPulse_ms)
{
    int DataUseLen = DataLen-NIBP_DATA_FILTER_SIZE+1, index;
    Filter(fFir_6_100hz_35s, NIBP_DATA_FILTER_SIZE, pDataIn, DataLen, DataUseLen, pDataFil, 0);
    //数据删除前面一段,解决气泵停止出现的回落
//    for(ii=0; ii<DataUseLen-NIBP_RISING_CUT_LEN; ii++)
//    {
//        pDataFil[ii] = pDataFil[ii+NIBP_RISING_CUT_LEN];
//    }
//    DataUseLen -= NIBP_RISING_CUT_LEN;
	Queue_S_Init(&g_NIBP_QueuesNew2);   
    GetSteepFalling(pDataFil, DataUseLen, WAVELET_CUT, 0.3, 0.1, &g_NIBP_QueuesNew2);
    QerrCutFalling(&g_NIBP_QueuesNew2);
    GetFallingUnitList( pDataFil,
                        &DataUseLen,
                        pQueueS,
                        &g_NIBP_QueuesNew2,
                        g_DiffList,
                        g_CountList,
                        g_MeanList,
                        pPulse_ms);  

    *power = 0;
	for(index=0; index<g_NIBP_QueuesNew2.nLen; index++)
	{
		*power += (g_NIBP_QueuesNew2.pSteep[index].Yend - g_NIBP_QueuesNew2.pSteep[index].Ystart);
    }
	*power = *power/g_NIBP_QueuesNew2.nLen;

	return g_NIBP_QueuesNew2.nLen;
}

/*
输入:
    pDataIn - 压力数据
    DataLen - 压力数据长度
    Pressure - 当前阶梯稳态压力值
    First - 每次测试的首段数据标志
输出:
    pEnd - 上升阶段结束标志
    正数 - 下一阶梯调整至该压力
*/
static int NIBP_Rising_All( int* pDataIn,
                            int DataLen,
                            int* pDataFil,
                            int* pDataFilLen,
                            QUEUE_S* pQueueS,
                            int Pressure,
                            int *pPulse_ms,
                            char First,
                            char *pEnd)
{
    static int PowerMax, PowerMinH, PowerMinL, PressureMax, PressureMinH, PressureMinL, PressureBot, PressureTop;
    int StepNum, Power;
    
    StepNum = NIBP_Step_Rising(pDataIn, DataLen, pDataFil, pDataFilLen, pQueueS, &Power, pPulse_ms);
    
    if(First)
    {
        PressureMax = Pressure;
        PressureMinH = Pressure;
        PressureMinL = Pressure;
        PressureTop = Pressure;
        PowerMax = Power;
        PowerMinH = Power;
        PowerMinL = Power;
        PressureBot = 0;
        NIBPAlgo_DBG(">1 NIBP_Rising_All First %d",Pressure);
    }
    NIBPAlgo_DBG("Rising_mmHgCur=%d Power=%d maxPower=%d",Pressure, Power, PowerMax);
    
    if(StepNum == 0)
    {
        if(Pressure >= PressureMax)
        {
            *pEnd = 1;
            NIBPAlgo_DBG(">2 StepNum == 0 %d",Pressure);
            return Pressure;
        }
        else
        {
            *pEnd = 0;
            PressureBot = PressureTop;
            PressureTop += 40;
            NIBPAlgo_DBG(">3 StepNum == 0 %d",PressureTop);
            return PressureTop;
        }
    }
    else
    {
        if(PowerMax < Power)
        {
            PressureMax = Pressure;
            PowerMax = Power;
			if(PressureMax > PressureMinH){
				PressureMinH = PressureMax;
                PowerMinH = PowerMax;
			}
			else if(PressureMax < PressureMinL){
				PressureMinL = PressureMax;
                PowerMinL = PowerMax;
			}
        }
        else if(PowerMinH > Power && PressureMax < Pressure)
        {
            PressureMinH = Pressure;
            PowerMinH = Power;
        }
        else if(PowerMinL > Power && PressureMax > Pressure)
        {
            PressureMinL = Pressure;
            PowerMinL = Power;
        }
        
        if(PowerMinH <= PowerMax*0.4)
        {
            *pEnd = 1;
            NIBPAlgo_DBG(">4 PowerMax(%d) PowerMinH(%d) PowerMinL(%d) PressureBot(%d) @return=%d",
                PowerMax,PowerMinH,PowerMinL,PressureBot,10 + PressureMinH);
            return 10 + PressureMinH;
        }
        else if((PowerMinL < PowerMax * 0.8 && PressureMinL < PressureMax) \
            || (Pressure < PressureBot+15))
        {
            *pEnd = 0;
            PressureBot = PressureTop;
            PressureTop += 40;
            NIBPAlgo_DBG(">5 PowerMax(%d) PowerMinH(%d) PowerMinL(%d) PressureBot(%d) @return=%d",
                PowerMax,PowerMinH,PowerMinL,PressureBot,PressureTop);
            return PressureTop;
        }
        else
        {
            *pEnd = 0;
            NIBPAlgo_DBG(">6 PowerMax(%d) PowerMinH(%d) PowerMinL(%d) PressureBot(%d) @return=%d",
                PowerMax,PowerMinH,PowerMinL,PressureBot,Pressure - 10);
            return Pressure - 10; 
        }
    } 
}




bool NIBPProcessQWithEnd(QUEUE_S* pQueueS, int End)
{
    int index = 0;
    int Enable = 0;
    int DeltaMax = 0, DeltaMaxIndex = 0, startIndex = 0, endIndex = pQueueS->nLen-1;
    
    Queue_S_Init(&g_nibpQueueSnew);
    

    for(index = 0; index < pQueueS->nLen; index++)
		if(pQueueS->pSteep[index].Yend != pQueueS->pSteep[index].Ystart)
			Enable++;
    if(Enable <= 4)
        return false;

    for(index = 0; index < pQueueS->nLen; index++)
    {
        p_temp_queue_int[index] = pQueueS->pSteep[index].Yend - pQueueS->pSteep[index].Ystart;
    }
    
    DeltaMax = Max_Index_N(p_temp_queue_int, pQueueS->nLen, &DeltaMaxIndex);

    if(DeltaMaxIndex == 0 || DeltaMaxIndex == pQueueS->nLen-1)
    {
        return false;
    }

    for(index = DeltaMaxIndex; index >= 0; index--)//计算前半部分
    {
        if((p_temp_queue_int[index] < DeltaMax*(float)FOR_BASELINE2) || (index == 0))
        {
            startIndex = index;
            break;
        }
    }
    
    for(index = DeltaMaxIndex; index < pQueueS->nLen; index++)//计算后半部分
    {
        if((p_temp_queue_int[index] < DeltaMax*(float)FOR_BASELINE2) || (index == pQueueS->nLen-1))
        {
            endIndex = index;
            break;
        }
    }
    
    if(p_temp_queue_int[index] >= DeltaMax*(float)FOR_BASELINE2 && End != 1)
    {
        return false;
    }

    memcpy(g_nibpQueueSnew.pSteep, pQueueS->pSteep+startIndex, sizeof(STEEP_UNIT)*(endIndex-startIndex+1));
    g_nibpQueueSnew.nLen = endIndex-startIndex+1;

    Queue_S_Init(pQueueS);
    for(index = 0; index < g_nibpQueueSnew.nLen; index++)
    {
        if(g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Xstart != 0)
        {
            pQueueS->pSteep[pQueueS->nLen].Xstart = g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Xstart;
            pQueueS->pSteep[pQueueS->nLen].Ystart = g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Ystart;
            pQueueS->pSteep[pQueueS->nLen].Xend = g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Xend;
            pQueueS->pSteep[pQueueS->nLen].Yend = g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Yend;
            pQueueS->pSteep[pQueueS->nLen].Mode = g_nibpQueueSnew.pSteep[g_nibpQueueSnew.nLen-1-index].Mode;
            pQueueS->nLen++;
        }
    }
    NIBPAlgo_DBG("NIBPProcessQWithEnd===========ture!");
    return true;
}

static void NIBP_RisingAddUnitList(    int* pDataIn,       //input,输入原始数据
                                int  DataInLen,     //input,输入数据长度
                                int* pDataFil,
                                int  DataFilLen,
                                QUEUE_S* pQueueS,   //output,算法分析处理的重要数据
                                int *pPulse_ms)
{
    Queue_S_Init(&g_NIBP_QueuesNew2);
    
    GetSteepFalling(pDataFil, DataFilLen, WAVELET_CUT, 0.3, 0.1, &g_NIBP_QueuesNew2);
    QerrCutFalling(&g_NIBP_QueuesNew2);
    GetFallingUnitList( pDataIn,
                        &DataInLen,
                        pQueueS,
                        &g_NIBP_QueuesNew2,
                        g_DiffList,
                        g_CountList,
                        g_MeanList,
                        pPulse_ms);   
}


bool NIBP_FallingWithEnd(   int* pDataIn,  //input,输入原始数据
                            int* pDataFil,
                            int* pDataLen,  //input,输入数据长度
                            QUEUE_S* pQueueS,   //output,算法分析处理的重要数据
                            FEATURE_LIST* pFeatureList, //output,压力与能量列表
                            int* pDP,       //output,舒张压
                            int *pDE,
                            int* pSP,       //output,收缩压
                            int *pSE,
                            int* pMAP,      //output,平均压
                            int *pMAE,
                            int PMax,     //input,预充压最大值
                            int* pBPM,
                            int* pPulse_ms,
                            int End)
{
    int MaxIndex = 0, MaxData = 0, sum = 0, count = 0, index = 0, QcountBPM = 0;
    int i = 0, startIndex = 0, endIndex = 0, sortIndex = 0;

    Queue_S_Init(&g_NIBP_QueuesNew2);
    NIBPgetFallingQ(pDataIn, pDataFil, pDataLen, &g_NIBP_QueuesNew2);
    QerrCutFalling(&g_NIBP_QueuesNew2);
    GetFallingUnitList( pDataIn,
                        pDataLen,
                        pQueueS,
                        &g_NIBP_QueuesNew2,
                        g_DiffList,
                        g_CountList,
                        g_MeanList,
                        pPulse_ms);
    if(pDataIn[*pDataLen-1] < my_max((PMax)-35*10000, (PMax)*0.65))
    {
        QcountBPM = pQueueS->nLen;
        if(NIBPProcessQWithEnd(pQueueS, End))
        {
//<!-- BPM Start -->
            MaxData = Max_Index_N(g_MeanList, QcountBPM, &MaxIndex);
            startIndex = MaxIndex;
            endIndex = MaxIndex;
            //求左边界条件
            for(index = MaxIndex-1; index >= 0; index--)
            {
                if(g_MeanList[index] < MaxData*0.45)
                {
                    startIndex = index+1;
                    if(g_CountList[index] > 0)
                        break;
                }
            }
            //求右边界条件
            for(index = MaxIndex+1; index < QcountBPM; index++)
            {
                if(g_MeanList[index] < MaxData*0.45)
                {
                    endIndex = index-1;
                    if(g_CountList[index] > 0)
                        break;
                }
            }

            //计算总脉搏个数
            for(index=startIndex; index<=endIndex;index++)
            {
                count += g_CountList[index];
            }

            NIBPAlgo_DBG("===============count=%d,startIndex=%d,endIndex=%d,MaxIndex=%d",
                count,startIndex,endIndex,MaxIndex);
            for (index = 0; index < QcountBPM; index++)
            {
                NIBPAlgo_DBG("g_DiffList[%d]=%d g_CountList[%d]=%d",
                    index,g_DiffList[index],index,g_CountList[index]);
            }

            //计算脉搏
//            if(count > 4)
            {
                for(index=startIndex; index<=endIndex;index++)
                {
                    sum += g_DiffList[index] * g_CountList[index];
                }
                *pBPM = 6000.0*count/sum+0.5;//(1min)/(平均脉搏周期)
            }
//<!-- BPM End -->
            GetFeatureList(pQueueS, pFeatureList);
            FilFeatureList(pFeatureList);
            NIBPfromFeature(pFeatureList, pDP, pDE, pSP, pSE, pMAP, pMAE, pBPM);
            Reliability = ForReliability(g_MeanList, g_DiffList, g_CountList, \
                                         QcountBPM, startIndex, endIndex);
            return true;
        }
    }
    return false;
}


void NIBPInitV1000(void)
{
    memset(p_nibpAlgoOut, 0, sizeof(CNIBPOut));
    memset(p_nibpAlgoIn, 0, sizeof(CNIBPIn));
    p_nibpAlgoOut->nibpResult.isAllDone = false;
    p_nibpAlgoOut->data.stDataRaw.u32DataMax = NIBP_FILTER_WINDOW_LEN;
    p_nibpAlgoOut->data.stDataRaw.pData = p_NibpDataIn;
    p_nibpAlgoOut->data.stDataFilter.u32DataMax = NIBP_STEEP_WINDOW_LEN;
    p_nibpAlgoOut->data.stDataFilter.pData = p_NibpDataFil;
    p_nibpAlgoOut->nibpResult.isRisingDone = false;
    p_nibpAlgoOut->nibpResult.risingResult = 0;

    p_nibpAlgoIn->isOneStepDataReady = false;
    p_nibpAlgoIn->u32mmHgCur = STEP_RELE_ADC_TAG;
    
    NIBPInit(p_Queue_s, p_Feature_list);
    g_isRisingFirst = 1;
    g_pulse_average_ms = 0;
    Reliability = 0.0f;
}

void NIBPRunV1000(void)
{
    if (p_nibpAlgoOut->nibpResult.isAllDone) return;

    if (p_nibpAlgoIn->isOneStepDataReady) 
    {
        if (0 == p_nibpAlgoOut->data.stDataRaw.u32DataNum)
        {
            p_nibpAlgoIn->isOneStepDataReady = false; //handle one step data complete
            return;
        }
        
        if (!p_nibpAlgoOut->nibpResult.isRisingDone)//rising handle
        {
            char end_flag = 0;
            p_nibpAlgoOut->nibpResult.risingResult = \
                NIBP_Rising_All((int *)p_nibpAlgoOut->data.stDataRaw.pData,
                            p_nibpAlgoOut->data.stDataRaw.u32DataNum,
                            (int *)p_nibpAlgoOut->data.stDataFilter.pData,
                            (int *)&p_nibpAlgoOut->data.stDataFilter.u32DataNum,
                            p_Queue_s,
                            p_nibpAlgoIn->u32mmHgRising,
                            &p_nibpAlgoOut->nibpResult.pulse_ms,
                            g_isRisingFirst,
                            &end_flag);
            g_isRisingFirst = 0;
            if (end_flag)
            {
                p_nibpAlgoOut->nibpResult.isRisingDone = true;
                p_nibpAlgoOut->nibpResult.BPM = 0;
            }
        }
        else//falling handle
        {
            int isFastFalling;
            int DE,SE,MAE;
            isFastFalling = (p_nibpAlgoOut->data.stDataRaw.pData[0] < 100000) ? 1:0;//10mmHg
            p_nibpAlgoOut->nibpResult.isAllDone = \
                NIBP_FallingWithEnd((int *)p_nibpAlgoOut->data.stDataRaw.pData,
                            (int *)p_nibpAlgoOut->data.stDataFilter.pData,
                            (int *)&p_nibpAlgoOut->data.stDataRaw.u32DataNum,
                            p_Queue_s,
                            &Feature_list,
                            &p_nibpAlgoOut->nibpResult.DP,
                            &DE,
                            &p_nibpAlgoOut->nibpResult.SP,
                            &SE,
                            &p_nibpAlgoOut->nibpResult.MAP,
                            &MAE,
                            p_nibpAlgoIn->u32mmHgRising * 10000,
                            &p_nibpAlgoOut->nibpResult.BPM,
                            &p_nibpAlgoOut->nibpResult.pulse_ms,
                            isFastFalling);
            p_nibpAlgoOut->nibpResult.reliability = Reliability;
        }
        
        p_nibpAlgoIn->isOneStepDataReady = false; //handle one step data complete
        p_nibpAlgoOut->data.stDataRaw.u32DataNum = 0;
    }
}

#define MMHG_AMPLIFY_FACTOR     (10000)   //mmHg的值放大倍数

//作用：加在测量结果计算后，输出前。根据前一次测试结果，微调本次测试结果
//注意：病人释放时，前一次测量结果要清空
//DP_Last : 前一次舒张压测试结果
//SP_Last : 前一次收缩压测试结果
//BPM_Last : 前一次脉率测试结果
//*DP_Now : 本次舒张压测试结果
//*SP_Now : 本次收缩压测试结果
//*BPM_Now : 本次脉率测试结果
static void ForResult(int DP_Last, int SP_Last, int BPM_Last, int *DP_Now, int *SP_Now, int *BPM_Now)
{
    if( my_abs(DP_Last - *DP_Now) <= 12*MMHG_AMPLIFY_FACTOR \
        && my_abs(SP_Last - *SP_Now) <= 12*MMHG_AMPLIFY_FACTOR \
        && my_abs(BPM_Last - *BPM_Now) <= 5)
    {
        if(DP_Last - *DP_Now >= 3*MMHG_AMPLIFY_FACTOR)
            *DP_Now += 2*MMHG_AMPLIFY_FACTOR;
        else if(DP_Last - *DP_Now <= -3*MMHG_AMPLIFY_FACTOR)
            *DP_Now -= 2*MMHG_AMPLIFY_FACTOR;
        
        if(SP_Last - *SP_Now >= 3*MMHG_AMPLIFY_FACTOR)
            *SP_Now += 2*MMHG_AMPLIFY_FACTOR;
        else if(SP_Last - *SP_Now <= -3*MMHG_AMPLIFY_FACTOR)
            *SP_Now -= 2*MMHG_AMPLIFY_FACTOR;
        
        if(BPM_Last - *BPM_Now >= 2)
            *BPM_Now += 1;
        else if(BPM_Last - *BPM_Now <= -2)
            *BPM_Now -= 1;
    }
    NIBPAlgo_DBG("ForResult DP-SP-BPM Last:%d-%d-%d Now:%d-%d-%d",
        DP_Last,SP_Last,BPM_Last,*DP_Now, *SP_Now, *BPM_Now);
}


static float ForReliability(int *MeanH_List,
                            int *MeanD_List,
                            int *Count_List,
                            int ListLen,
                            int StartIndex,
                            int EndIndex)
{
	float Delta, Data, Out;
    int i, Mean, Count, MaxData, MaxIndex;
	Mean = 0;
	Count = 0;
	for(i=StartIndex; i<=EndIndex; i++)
    {
		if(Count_List[i] > 0)
        {
			Mean += MeanD_List[i];
			Count ++;
		}
	}
	if(0 == Count)//add by QWB
	//if(Count < 2 || EndIndex - StartIndex < 2)//delete by QWB
	{
	    NIBPAlgo_DBG("ForReliability>>1111111111111");
		return 100;
    }
	Mean = Mean / Count;
	Delta = 0;
	Count = 0;
	for(i=StartIndex; i<=EndIndex; i++)
    {
		if(Count_List[i] > 0)
        {
			Delta += my_abs(Mean - MeanD_List[i]) * my_abs(Mean - MeanD_List[i]);
	        NIBPAlgo_DBG("ForReliability>>1.1 Delta=%f Mean=%d MeanD_List[%d]=%d",Delta,Mean,i,MeanD_List[i]);
			Count ++;
		}
	}
	Delta = Delta / Count / Mean / Mean;
	NIBPAlgo_DBG("ForReliability>>22222222 Delta=%f",Delta);
	Out = Delta;

	MaxData = Max_Index_N(MeanH_List, ListLen, &MaxIndex);
	if(MaxIndex < StartIndex || MaxIndex > EndIndex || MaxIndex == 0 || MaxIndex == ListLen-1)
	{
	    NIBPAlgo_DBG("ForReliability>>3333333333333");
		return 100;
    }
	Delta = MeanH_List[MaxIndex] / my_max(MeanH_List[MaxIndex+1], MeanH_List[MaxIndex-1]);
	if(Delta >= 3.0f)
	{
        for (i=0; i < ListLen; i++) NIBPAlgo_DBG("MeanH_List[%d]=%d",i,MeanH_List[i]);
	    NIBPAlgo_DBG("ForReliability>>444444444444 Delta=%f [max+1]=%d, [max-1]=%d",
                      Delta,MeanH_List[MaxIndex+1], MeanH_List[MaxIndex-1]);
		return 100;
    }

    Delta = 0;
	Count = 0;
	for(i=StartIndex+1; i<=EndIndex-1; i++)
    {
		if(i != MaxIndex)
        {
			if(my_min(MeanH_List[i+1], MeanH_List[i-1]) > MeanH_List[i])
            {
				Data = (my_min(MeanH_List[i+1], MeanH_List[i-1]) - MeanH_List[i]) \
                        / my_max(MeanH_List[i+1], MeanH_List[i-1]);
				Delta += Data * Data;
			}
			else if(MeanH_List[i] > my_max(MeanH_List[i+1], MeanH_List[i-1]))
            {
				Data = (MeanH_List[i] - my_max(MeanH_List[i+1], MeanH_List[i-1])) \
                        / my_max(MeanH_List[i+1], MeanH_List[i-1]);
			    Delta += Data * Data;
			}
			Count ++;
		}
	}
    if (0 == Count)//add by QWB avoid "-nan" error
    {
	    NIBPAlgo_DBG("ForReliability>>7777777 Delta=%f",Delta);
        return 0;
    }
	Delta = Delta / Count;
	NIBPAlgo_DBG("ForReliability>>555555 Delta=%f",Delta);
	Out += Delta*2.5;
	NIBPAlgo_DBG("ForReliability>>>66666 Out=%f",Out);

	return Out;
}


