


//#include "../../AlgCommonFunc/AlgLib.h"
#include "RESP_V1001.h"
#include <string.h>

//#include "../digitFilter.h"

#include <math.h>



//namespace V1001{

// ��������
static int pushCount;
static float TriAmp;
#pragma section("sdram0_bank2")
static float hisRespRate[60];
static int hisRespRateCount;
static int RespRate;
static FILTER_BUFFER_1 FilterBuffer;
static u8 FilterMode = 0;	// 0 -- ������ģʽ��1 -- ����ģʽ��
static float HighBoundary = 60;
static float LowBoundary = 10;
static float SuffocationAlertDelay = 20;
static u32 SuffocationSysTick = 0;
static bool isSuffocation = false;
#pragma section("sdram0_bank2")
static float hisR[120];	// ��������ʷ���ݶ��У�60�볤�ȣ�0.5�����һ�Σ�����120��λ���ȵĶ���
static int hisRCount = 0;	// ��������ʷ���ݶ��еĳ���
#pragma section("sdram0_bank2")
static CTriangle_Form TF[100];	// ���ǲ��б�
static int tfCount = 0;	// ���ǲ�����

#pragma section("sdram0_bank2")
u32 FindTriangleForm(int* pData, u32 nDataLen, int threshold, CTriangle_Form* pTFList, u32 nMaxLen);  // �����ǲ�
#pragma section("sdram0_bank2")
float FcoeH(float x, float B, float C);
#pragma section("sdram0_bank2")
float CalcRESPRate(CTriangle_Form* pTF, int nTFCount, int Fs);
#pragma section("sdram0_bank2")
float CalcRESPRate1(float* pHisRespRate, int nHisRespRateCount, float r);
#pragma section("sdram0_bank2")
void CheckAlert(CRESPAlertList *pStAlertList, u32 systick, int calDataLen, int Fs);
#pragma section("sdram0_bank2")
float CalcRESPByWave(CTriangle_Form* pTF, int nTFCount, int Fs, int calDataLen);
#pragma section("sdram0_bank2")
float CalcRESPByHisRESP(float* phisR, int *pPhisRCount, float curRESP, float updateTimeLen);
#pragma section("sdram0_bank2")
int AlertList_Append(CRESPAlertList* pList, CRESPAlertItem item);
#pragma section("sdram0_bank2")
void AlertList_Clear(CRESPAlertList* pList);
#pragma section("sdram0_bank2")
void RespAlgoInit(void);
#pragma section("sdram0_bank2")
void RespAlgoRun(  void* pdataIn,
                int* pRESP_Cal_Buf,
                u32 cal_data_len,
                u32 *pResp_calc_data_num,
                int update_data_len,
                int algorithm_input_fs,
                void* pdataOut,
                CRESPAlertList *pStAlertList);


void RespAlgoInit(void)
{
    pushCount = 0;
	TriAmp = 5000;

	hisRespRateCount = 0;

	RespRate = 0;

    memset(hisRespRate, 0, sizeof(float)*60);
}


void RespAlgoRun(	void* pdataIn,
				int* pRESP_Cal_Buf,
				u32 cal_data_len,
				u32 *pResp_calc_data_num,
				int update_data_len,
				int algorithm_input_fs,
				void* pdataOut,
				CRESPAlertList *pStAlertList)
{
	pushCount++;

	CRESPIn dataIn = *((CRESPIn*)pdataIn);

	CRESPOut* pDataOut = (CRESPOut*)pdataOut;

	// ���ݾ����˲�
	int filteddata = 0;
	switch (FilterMode) {
	case 0:
		filteddata = resp_newborn_filter(&FilterBuffer, dataIn.s32RESP);
		break;
	case 1:
		filteddata = resp_adult_filter(&FilterBuffer, dataIn.s32RESP);
		break;
	}
    int currentIndex = pDataOut->u32RESPDataNum-1;
	pDataOut->pData[currentIndex].stDataRaw.s32RESP = dataIn.s32RESP;
	pDataOut->pData[currentIndex].stDataRaw.u32SysTick = dataIn.u32SysTick;
	pDataOut->pData[currentIndex].stDataFilter.s32RESP = filteddata;
	pDataOut->pData[currentIndex].stDataFilter.u32SysTick = dataIn.u32SysTick;

	// �˲�������buffer
	if (*pResp_calc_data_num < cal_data_len) {
		pRESP_Cal_Buf[*pResp_calc_data_num] = filteddata;
		(*pResp_calc_data_num) += 1;
	}
	
    AlertList_Clear(pStAlertList);
	
	if (pushCount >= update_data_len) {
		pushCount = 0;
		
		float timeLen = 4;
		if (RespRate >= 40 && RespRate <= 130) {
			timeLen = 4;
		} else if (RespRate > 0 && RespRate < 40) {
			timeLen = 8;
		}
		int cLen = algorithm_input_fs * timeLen;	
		float delta = Max_N(&pRESP_Cal_Buf[(*pResp_calc_data_num) - cLen], cLen) \
			- Min_N(&pRESP_Cal_Buf[(*pResp_calc_data_num) - cLen], cLen);

		
		if (delta < 10000) {
			//printf("delta: %f\r\n", delta);
			RespRate = 0;
			// ���������ʶ���
			if (hisRespRateCount == 60) {
				int i = 0;
				int maxCount = hisRespRateCount - 1;
				for (i = 0; i < maxCount; i++) {
					hisRespRate[i] = hisRespRate[i + 1];
				}
				hisRespRate[hisRespRateCount - 1] = RespRate;
			} else {
				hisRespRate[hisRespRateCount] = RespRate;
				hisRespRateCount++;
			}

		} else {
			// �㵱ǰ������ֵ
			float curAmp = 0.35 * delta;
		
			// ��ǰ��ֵ����ʷ��ֵ�ı���
			float di = my_abs(curAmp - TriAmp) / TriAmp;

			// ��ʷ��ֵ���Ŵ�ϵ��
			float coeH = FcoeH(di, 0.98, 2);

			// ���·�����ֵ
			TriAmp = (1.0 - coeH) * curAmp + coeH * TriAmp;
			//printf("TriAmp: %f   delta: %f\r\n", TriAmp, delta);

			// �����ǲ�
			tfCount = FindTriangleForm(pRESP_Cal_Buf, (*pResp_calc_data_num), TriAmp, TF, 100);

			// �㵱ǰ������
			float curRespRate = CalcRESPByWave(TF, tfCount, algorithm_input_fs, cal_data_len);
			curRespRate = (curRespRate < 0 || curRespRate > 400) ? 0 : curRespRate;

			// ���º�����
			RespRate = CalcRESPByHisRESP(hisR, &hisRCount, curRespRate, (float)update_data_len / algorithm_input_fs) + 0.5;

		}

		// ��������д�������
		pDataOut->pData[currentIndex].stResult.u32RESPRate = RespRate;
		pDataOut->pData[currentIndex].stResult.pAlertList = pStAlertList;

		// �������
		CheckAlert(pStAlertList, dataIn.u32SysTick, cal_data_len, algorithm_input_fs);

		// ȥ��һ������
		int moveLen = cal_data_len - update_data_len;
		int i = 0;
		for (i = 0; i < moveLen; i++) {
			pRESP_Cal_Buf[i] = pRESP_Cal_Buf[i + update_data_len];
		}
		(*pResp_calc_data_num) -= update_data_len;
		

	} else {
		// ��������д�������
		pDataOut->pData[currentIndex].stResult.u32RESPRate = RespRate;
		pDataOut->pData[currentIndex].stResult.pAlertList = pStAlertList;

		// �������
		CheckAlert(pStAlertList, dataIn.u32SysTick, cal_data_len, algorithm_input_fs);
	}
}





























u32 FindTriangleForm(int* pData, u32 nDataLen, int threshold, CTriangle_Form* pTFList, u32 nMaxLen)
{
	// pData�ǲ������ݣ��������ǲ��Ķ���
	// threshold�������ǲ��ķ�����ֵ
	// pTFList���ҵ������ǲ���ŵ��б�
	// nMaxLen�����ǲ��б����󳤶�
	// ����ֵ�����ǲ��ĸ���

	if (pData == NULL)
	{
		return 0;
	}
	//printf("nDataLen %d\r\n", nDataLen);

	CCoordiate_Point stCPoint;
	CCoordiate_Point leftLow;
	CCoordiate_Point high;
	CCoordiate_Point rigthLow;
	CTriangle_Form TFTmp;
	s32 di = 0;
	u32 triCount = 0;
	int jstat = 0;
	CCoordiate_Point jPoint;

	int stat = 0;
	leftLow.u32X = 0;
	leftLow.u32Y = pData[leftLow.u32X];
	high = leftLow;
	rigthLow = leftLow;
	
	for (u32 i = 0; i < nDataLen; i++) {
		if (triCount >= nMaxLen) {
			break;
		}

		// ��ǰ������
		stCPoint.u32X = i;
		stCPoint.u32Y = pData[i];

		switch (stat) 
		{
		case 0: // ǰһ���״̬��������ͬһ����
			//printf("Test 245 case 0\r\n");
			if (stCPoint.u32Y <= leftLow.u32Y) {
				// �½�
				leftLow = stCPoint;
				high = leftLow;
				rigthLow = leftLow;
			} else {
				// ����
				stat = 1;
				high = stCPoint;
				rigthLow = stCPoint;
			}
			break;
		case 1: // �����ҵĹ���
			//printf("Test 259 case 1\r\n");
			if (stCPoint.u32Y >= high.u32Y)	{
				// ��������
				high = stCPoint;
				rigthLow = stCPoint;
			} else {
				// ���µĹյ�
				di = high.u32Y - stCPoint.u32Y;
				if (stCPoint.u32Y > leftLow.u32Y) {
					// �½���û������ߵĵ͵�
					if (di >= threshold) {
						// �ҵ�һ���������ǲ���ͷ������Ҫ�����ҵ��Ұ벿�ĵײ�
						stat = 2;
					}
					rigthLow = stCPoint;
				} else {
					// 
					if (di >= threshold && high.u32Y - leftLow.u32Y >= threshold) {
						// �ҵ�һ���������ǲ���ͷ������Ҫ�����ҵ��Ұ벿�ĵײ�
						stat = 2;
					} else {
						// �ص�����ͬһ�����״̬
						stat = 0;
						leftLow = stCPoint;
						high = stCPoint;
						rigthLow = stCPoint;
					}
				}
			}
			break;
		case 2: // �������Ұ벿�ײ�
			//printf("Test 290 case 2 i %d  threshold %d  (%d,%d)(%d,%d)(%d,%d)\r\n", i, threshold, leftLow.u32X, leftLow.u32Y, high.u32X, high.u32Y, rigthLow.u32X, rigthLow.u32Y);
			if (stCPoint.u32Y >= rigthLow.u32Y) {
				// �ҵ��ײ������һ�����ǲ��Ĳ���
				
				jstat = 1;
				for (int j = high.u32X; j >= 0; j--) {
					jPoint.u32X = j;
					jPoint.u32Y =	 pData[j];
					//printf("jPoint (%d, %d) %d threshold %d\r\n", jPoint.u32X, jPoint.u32Y, high.u32Y - jPoint.u32Y, threshold);
					if (jstat == 1)
					{
						if (high.u32Y - jPoint.u32Y >= threshold) {
							// �ҵ�������ֵ�ĵ㣬���������ҵ���벿�ĵײ�
							//printf("in           jPoint (%d, %d)\r\n", jPoint.u32X, jPoint.u32Y);
							jstat = 2;
						}
					}
					else
					{
						if (jPoint.u32Y >= pData[j + 1]) {
							// ���һ�����ǲ��Ĳ���
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
				// �ص�����ͬһ�����״̬
				stat = 0;
				leftLow = stCPoint;
				high = stCPoint;
				rigthLow = stCPoint;
			} else {
				// �����½�����������
				rigthLow = stCPoint;
			}
			break;
		}
	}

	return triCount;
}




float FcoeH(float x, float B, float C)
{
	// ����ֵ�Ŵ�ϵ��
	float a = 2 - 2 * B;
	float y;
	float p1 = 1.0 / (1.0 + exp((0.0 - C) * x));
	y = 1.0 - a + a * p1;
	return y;
}


float CalcRESPRate(CTriangle_Form* pTF, int nTFCount, int Fs) 
{
	// ������� 

	// nTFCount ���ܳ���100

	if (nTFCount == 1 || nTFCount > 100) {
		return 0.0;
	} else if (nTFCount == 2) {
		return 60.0 * Fs / (pTF[1].stPeak.u32X - pTF[0].stPeak.u32X);
	}

    //printf("========================\n");
	int y[100];
	int yCount = nTFCount - 1;
	for (int i = 0; i < yCount; i++) {
		y[i] = pTF[i + 1].stPeak.u32X - pTF[i].stPeak.u32X;
        //printf("%d\n", y[i]);
	}

	int blockLen = 100;
	int stepLen = blockLen / 2;
	int stepCount = blockLen * 3 / stepLen;
	int head = 0;
	int tail = 0;
	int summ = 0;
	int count = 0;
	int moveCount = 1;


	int bbCubeSum[50];	
	int bbCubeCount[50];
	int bbCubeC = 0;

	// ͳ��
	head = Max_N(y, yCount);
	tail = head - blockLen;
	while (moveCount < stepCount && bbCubeC < 50) {
		count = 0;
		summ = 0;
		for (int i = 0; i < yCount; i++) {
			if (y[i] <= head && y[i] > tail) {
				summ += y[i];
				count++;
			}
		}
		bbCubeSum[bbCubeC] = summ;
		bbCubeCount[bbCubeC] = count;
		bbCubeC++;

		head -= stepLen;
		tail = head - blockLen;
		moveCount++;
	}

	// �������ֵ
	int index = 0;
	Max_Index_N(bbCubeCount, bbCubeC, &index);

	// �������
	float rate = 60.0 * Fs / ((float)bbCubeSum[index] / (float)bbCubeCount[index]);

	return rate;
}

float CalcRESPRate1(float* pHisRespRate, int nHisRespRateCount, float r)
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
        while (i < nHisRespRateCount) {
            rate += (a + R * i) * pHisRespRate[i];
            i = i + 1;
	    }
    }
	return rate;
}

/*
void GetPara(float* paraList)
{
	FilterMode = paraList[RESP_PARAMETER_FILTER_MODE_INDEX];
	HighBoundary = paraList[RESP_PARAMETER_HIGH_BOUNDARY_INDEX];
	LowBoundary = paraList[RESP_PARAMETER_LOW_BOUNDARY_INDEX];
	SuffocationAlertDelay = paraList[RESP_PARAMETER_SUFFOCATION_ALERT_DELAY_TIME_INDEX];
}
*/

void CheckAlert(CRESPAlertList *pStAlertList, u32 systick, int calDataLen, int Fs)
{
	CRESPAlertItem item;
	

	if (RespRate > HighBoundary) {
		//sprintf_s(item.cItemName, "High Boundary\r\n");
		item.alertType = High_Boundary;
		item.u32Time = systick; // ��ǰsystick
		item.u32AlertIndex = 0;
		AlertList_Append(pStAlertList, item);
	} else if (RespRate < LowBoundary) {
		//sprintf_s(item.cItemName, "Low Boundary\r\n");
		item.alertType = Low_Boundary;
		item.u32Time = systick; // ��ǰsystick
		item.u32AlertIndex = 0;
		AlertList_Append(pStAlertList, item);
	}

	// ������һ�������뵱ǰʱ�䳬��4��
	if (tfCount != 0) {
		float timeLen = (calDataLen - TF[tfCount - 1].stPeak.u32X) / Fs;
		if (timeLen > 4) {
			RespRate = 0;
		}
	}

	if (RespRate == 0) {
		// ��Ϣ��
		if (isSuffocation == false) {
			// �տ�ʼ��Ϣ
			isSuffocation = true;
			SuffocationSysTick = systick;
		} else {
			// ������Ϣ�˶��
			u32 delayTime = (systick - SuffocationSysTick) / 10000;
			//printf("delayTime %d\r\n", delayTime);
			if (delayTime > SuffocationAlertDelay - 6) {
				//sprintf_s(item.cItemName, "Suffocation\r\n");
				item.alertType = Suffocation;
				item.u32Time = systick; // ��ǰsystick
				item.u32AlertIndex = 0;
				AlertList_Append(pStAlertList, item);
			}
		}
	} else {
		// û��Ϣ
		if (isSuffocation == true) {
			isSuffocation = false;
		}
	}
	
}


float CalcRESPByWave(CTriangle_Form* pTF, int nTFCount, int Fs, int calDataLen)
{
	// nTFCount ���ܳ���100

	if (nTFCount == 1 || nTFCount > 100 || nTFCount == 0) {
		return 0.0;
	} else if (nTFCount == 2) {
		return 60.0 * Fs / (pTF[1].stPeak.u32X - pTF[0].stPeak.u32X);
	}

    //printf("========================\n");
	float y[100];
	int yCount = nTFCount - 1;
	for (int i = 0; i < yCount; i++) {
		y[i] = pTF[i + 1].stPeak.u32X - pTF[i].stPeak.u32X;
		
        //printf("%d\n", y[i]);
	}

	float timeLen[3] = {10, 15, 20};	// �ֶε�ʱ�䳤��
	float Pi[3] = {4, 3, 1};	// ����
	int indexArr[3] = {0, 0, 0};	// �ֶ���ʼ�±�
	float Mi[3] = {0, 0, 0};	// ��ֵ
	float Vi[3] = {0, 0, 0};	// ����
	float Ai[3] = {0, 0, 0};	// ����
	float Ci[3] = {0, 0, 0};	// ���Ŷ�
	float RESP = 0;	// ������

	// ����ֶ���ʼ�±�
	//// calDataLen�����鳤�ȣ����ǲ��±���ֵ�������ǻ��������±꣬��ʱ����Բ�����
	for (int i = 0; i < 3; i++) {
		int len = timeLen[i] * Fs;
		int sum = 0;
		for (int j = yCount - 1; j >= 0; j--) {
			sum += y[j];
			if (sum >= len) {
				indexArr[i] = j;
				break;
			}
		}
	}

	// ����ת������
	for (int i = 0; i < yCount; i++) {
		y[i] = (60 * Fs) / y[i];
	}

	// ���ֵ
	for (int i = 0; i < 3; i++) {
		float sum = 0;
		for (int j = indexArr[i]; j < yCount; j++) {
			sum += y[j];
		}
		Mi[i] = sum / (yCount - indexArr[i]);
	}

	// �󷽲�
	for (int i = 0; i < 3; i++) {
		float sum = 0;
		for (int j = indexArr[i]; j < yCount; j++) {
			float di = y[j] - Mi[i];
			sum += di * di;
		}
		Vi[i] = sum / (yCount - indexArr[i]);
		Vi[i] = Vi[i] == 0 ? 0.00001 : Vi[i];
	}

	// �����
	float bsum = 0;
	float b[3];
	for (int i = 0; i < 3; i++) {
		b[i] = 1.0 / Vi[i];
		bsum += b[i];
	}
	for (int i = 0; i < 3; i++) {
		Ai[i] = b[i] / bsum;
	}


	// �����Ŷ�
	bsum = 0;
	for (int i = 0; i < 3; i++) {
		b[i] = Pi[i] * Ai[i];
		bsum += b[i];
	}
	for (int i = 0; i < 3; i++) {
		Ci[i] = b[i] / bsum;
	}

	// �������
	bsum = 0;
	for (int i = 0; i < 3; i++) {
		bsum += Ci[i] * Mi[i];
	}
	RESP = bsum;

#if 0
	printf("Mi:%f %f %f        ", Mi[0], Mi[1], Mi[2]);
	printf("Vi:%f %f %f\r\n", Vi[0], Vi[1], Vi[2]);
	printf("Ai:%f %f %f        ", Ai[0], Ai[1], Ai[2]);
	printf("Ci:%f %f %f\r\n", Ci[0], Ci[1], Ci[2]);
#endif

	return RESP;
}

float CalcRESPByHisRESP(float* phisR, int *pPhisRCount, float curRESP, float updateTimeLen)
{
	float RESP = 0;

	// ���µĺ����ʽ������
	if ((*pPhisRCount) >= 120) {
		int tailIndex = 120 - 1;
		for (int i = 0; i < tailIndex; i++) {
			phisR[i] = phisR[i + 1];
		}
		phisR[tailIndex] = curRESP;
	} else {
		phisR[(*pPhisRCount)] = curRESP;
		(*pPhisRCount) += 1;
	}


	float timeLen[3] = {2, 5, 20};	// �ֶε�ʱ�䳤�ȣ� ��λ��
	float Pi[3] = {10, 2, 1};	// ����
	int indexArr[3] = {0, 0, 0};	// �ֶ���ʼ�±�
	float Mi[3] = {0, 0, 0};	// ��ֵ
	float Vi[3] = {0, 0, 0};	// ����
	float Ai[3] = {0, 0, 0};	// ����
	float Ci[3] = {0, 0, 0};	// ���Ŷ�
	
	// ����ֶ���ʼ�±�
	for (int i = 0; i < 3; i++) {
		int count = timeLen[i] / updateTimeLen;
		count = (*pPhisRCount) - count;
		if (count < 0) {
			indexArr[i] = 0;
		} else if (count >= (*pPhisRCount)) {
			indexArr[i] = (*pPhisRCount) - 1;
		} else {
			indexArr[i] = count;
		}
	}

	// ���ֵ
	for (int i = 0; i < 3; i++) {
		float sum = 0;
		for (int j = indexArr[i]; j < (*pPhisRCount); j++) {
			sum += phisR[j];
		}
		Mi[i] = sum / ((*pPhisRCount) - indexArr[i]);
	}

	// �󷽲�
	for (int i = 0; i < 3; i++) {
		float sum = 0;
		for (int j = indexArr[i]; j < (*pPhisRCount); j++) {
			float di = phisR[j] - Mi[i];
			sum += di * di;
		}
		Vi[i] = sum / ((*pPhisRCount) - indexArr[i]);
		Vi[i] = Vi[i] == 0 ? 0.00001 : Vi[i];
	}

	// �����
	float bsum = 0;
	float b[3];
	for (int i = 0; i < 3; i++) {
		b[i] = 1.0 / Vi[i];
		bsum += b[i];
	}
	for (int i = 0; i < 3; i++) {
		Ai[i] = b[i] / bsum;
	}


	// �����Ŷ�
	bsum = 0;
	for (int i = 0; i < 3; i++) {
		b[i] = Pi[i] * Ai[i];
		bsum += b[i];
	}
	for (int i = 0; i < 3; i++) {
		Ci[i] = b[i] / bsum;
	}

#if 0
	printf("Mi:%f %f %f        ", Mi[0], Mi[1], Mi[2]);
	printf("Vi:%f %f %f\r\n", Vi[0], Vi[1], Vi[2]);
	printf("Ai:%f %f %f        ", Ai[0], Ai[1], Ai[2]);
	printf("Ci:%f %f %f\r\n", Ci[0], Ci[1], Ci[2]);
#endif
	

	// �������
	bsum = 0;
	for (int i = 0; i < 3; i++) {
		bsum += Ci[i] * Mi[i];
	}
	RESP = bsum;


	return RESP;
}


static int AlertList_Append(CRESPAlertList* pList, CRESPAlertItem item)
{
	if (pList->u32AlertCount >= RESP_ALERT_MAX_NUM)
	{
		int moveCount = pList->u32AlertCount - 1;
		for (int i = 0; i < moveCount; i++)
		{
			pList->alertList[i] = pList->alertList[i + 1];
		}
		pList->u32AlertCount--;
	}

	pList->alertList[pList->u32AlertCount] = item;
	pList->u32AlertCount++;
	return pList->u32AlertCount;
}


static void AlertList_Clear(CRESPAlertList* pList)
{
	pList->u32AlertCount = 0;
}

int RespAlgoDebugInterface(RESP_ALG_DEBUG_TypeDef type,
                                char* pBuf,
                                unsigned char nLen)
{
    switch(type)
    {
    case RESP_FILTER_SEL:
		if (0 == pBuf[0])
		{
        	FilterMode = 0;
		}
		else
		{
        	FilterMode = 1;
		}
        break;
	case RESP_SetHighBoundary:
		HighBoundary = pBuf[0];
		break;
	case RESP_SetLowBoundary:
		LowBoundary = pBuf[0];
		break;
	case RESP_SetSuffocationAlertDelay:
		SuffocationAlertDelay = pBuf[0];
		break;
    default:
        break;
    }
    return 0;
}

//};








