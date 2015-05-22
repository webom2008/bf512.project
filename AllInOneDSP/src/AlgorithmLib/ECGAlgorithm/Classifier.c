
#include "ECGAlgorithm_conf.h"

int QuickRClassifier(int* pData, int len, int FS, float RHighThres, int *pIndex, int *pHigh)
{
    int i, j;

	// 返回：-1--输入参数不满足要求；
	//       0--不是R波；
	//       1--是R波

	// 输入的数据长度不能超过500, 不能小于100(针对500Hz采样率，长度范围为100--1000ms)
	if (len > 500 || len < 50) {
//		printf("QuickRClassifier input wrong  %d\r\n", len);
		return -1;
	}

	int bIs = 0;
	*pIndex = 0; 
	*pHigh = 0;

	// 整体形状相似度评价
	// // 最高点要在右边
	int mindex = 0;
	int maxD = Max_Index_N(pData, len, &mindex);
	if (mindex > len / 2) {
		//printf("max point on the right\r\n");
	} else {
//		printf("max point on the left, not R. mindex %d  lenght %d\r\n", mindex, len);
		return bIs;
	}

	*pIndex = mindex;

	// 检查顶点距离最新点超过一定距离
	if (len - mindex < 3) {
		//printf("len - mindex < 3, len %d, mindex %d\r\n", len, mindex);
		return bIs;
	}

	// 找底和斜边的连接部位
	float t1[500];
	int tlen = len - 2;
	int minD = Min_N(pData, len);
	float d = maxD - minD;
	d = d * d / 10000;
	int mean = 0;
	float sum = 0;
	int tmp = 0;
	for (i = 0; i < tlen; i++) {
		mean = (pData[i] + pData[i + 1] + pData[i + 2]) / 3;
		sum = 0;
		for (j = 0; j < 3; j++) {
			tmp = pData[i + j] - mean;
			sum += tmp * tmp;
		}
		t1[i] = sum / 3 / d;
	}

	int jointPointIndex = 0;
	int ii = mindex - 3;
#if 0
	while (ii > 0) {
		if (t1[ii - 1] < t1[ii]) {
			break;
		}
		ii--;
	}
#endif
	float thre = 4;
	while (ii > 5) {
		if (t1[ii] < thre && t1[ii - 1] < thre && t1[ii - 2] < thre && \
            t1[ii - 3] < thre && t1[ii - 4] < thre) {
			jointPointIndex = ii;
			break;
		}
		ii--;
	}
	if (jointPointIndex == 0) {
//		printf("jointPointIndex not found\r\n");
        return bIs;
	}

	// 斜边时间宽度，脉宽限制
	float halfWide = (float)(mindex - jointPointIndex) / FS * 1000;
	if (halfWide < 12.5 || halfWide > 75) {
//		printf("R wide is out of bounds , %f\r", halfWide);
        return bIs;
	}

	// 底
	int count1 = 0.083 * FS;
	int count2 = 0.028 * FS;
	if (jointPointIndex < count1 || jointPointIndex < count2) {
//		printf("jointPointIndex %d, count1 %d, count2%d\r\n", jointPointIndex, count1, count2);
		return bIs;
	}

	if (count1 > len || count2 > len) {
//		printf("len %d, count1 %d, count2%d\r\n", len, count1, count2);
		return bIs;
	}

	int dLen = count1 - count2;
	float dd = maxD - pData[jointPointIndex];
	float v4 = 0;
	float tmpf = 0;
	for (i = 0; i < dLen; i++) {
		tmpf = pData[jointPointIndex - count1 + i] - pData[jointPointIndex];
		tmpf = tmpf / dd;
		if (tmpf > v4) {
			v4 = tmpf;
		}
	}
	if (v4 > 22) { // 该段最高的点不能超过范围
//		printf("v4 > 22\r\n");
		return bIs;
	}

	// R波高度是否达标
	*pHigh = dd;
	//if (high < RHighThres) {
	//	return bIs;
	//}

	// 通过所有条件，判定为R波
	bIs = 1;
	return bIs;
}

int FullRClassifier(int* pData, int len, int FS, float RHighThres, int *pIndex, int *pHigh)
{
	// 返回：-1--输入参数不满足要求；
	//       0--不是R波；
	//       1--是R波

	// 输入的数据长度不能超过500, 不能小于100(针对500Hz采样率，长度范围为200--1000ms)
	if (len > 500 || len < 100) {
//		printf("FullRClassifier input len wrong\r\n");
		return -1;
	}

	int bIs = 0;
	*pIndex = 0; 
	*pHigh = 0;

	// 切割出来一段来给快速R识别，最高点向左要120ms，向右要20ms
	int mindex = 0;
	int maxD = Max_Index_N(&pData[len / 4], len / 2, &mindex);
	mindex = mindex + len / 4;
	int leftWide = FS * 0.15;
	int rightWide = FS * 0.02;
	if (mindex < leftWide || mindex + rightWide >= len) {
//		printf("data is too short, %d %d %d\r\n", leftWide, rightWide, mindex);
		return bIs;
	}
	bIs = QuickRClassifier(&pData[mindex - leftWide], leftWide + rightWide, \
                            FS, RHighThres, pIndex, pHigh);
	if (bIs == 0) {
//		printf("QuickRClassifier deem it not R\r\n");
		return bIs;
	}

	// 右边部分
	int me = Mean_N(&pData[mindex], len - mindex);
	float v1 = (maxD - me) * 100.0 / *pHigh;
	if (v1 < 20) {
		bIs = 0;
//		printf("right side is too high.Not R\r\n");
		return bIs;
	}

	*pIndex = mindex;
	return bIs;

}

int PVCClassifier(int* pData, int len, int FS, int *pIndex)
{
    int i;
	// 返回：-1--输入参数不满足要求；
	//       0--不是PVC；
	//       1--左PVC；
	//       2--右PVC

	// 输入的数据长度不能超过500, 不能小于200(针对500Hz采样率，长度范围为400--1000ms)
	if (len > 500 || len < 200) {
//		printf("PVC input len wrong\r\n");
		return -1;
	}

	int result = 0;
	float wide = 0;
	*pIndex = 0;

	int maxI, maxV, minI, minV;
	maxV = Max_Index_N(pData, len, &maxI);
	minV = Min_Index_N(pData, len, &minI);
	int midLine = (maxV + minV) / 2;
	int di = maxV - minV;
	int upLine = midLine + 0.45 * di;
	int downLine = midLine - 0.45 * di;
	int flag = 0;
	int p1, p2, p3;
	if (maxI < minI) {
		// 突起在左边的情况
		for (i = 0; i < len - 1; i++) {
			if (pData[i] <= upLine && pData[i + 1] >= upLine) {
				flag = 1;
				p1 = i;
			} else if (pData[i] >= upLine && pData[i + 1] <= upLine) {
				if (flag == 1) {
					flag = 2;
					p2 = i + 1;
				}
			} else if (pData[i] >= downLine && pData[i + 1] <= downLine) {
				if (flag == 2) {
					flag = 3;
					p3 = i;
					break;
				}
			}
		}
		if (flag == 3) {
			result = 1;
			wide = (float)(p2 - p1) / FS * 1000;
		}
	} else {
		// 突起在右边的情况
		for (i = 0; i < len - 1; i++) {
			if (pData[i] >= downLine && pData[i + 1] <= downLine) {
				flag = 1;
				p1 = i;
			} else if (pData[i] <= downLine && pData[i + 1] >= downLine) {
				if (flag == 1) {
					flag = 2;
					p2 = i + 1;
				}
			} else if (pData[i] <= upLine && pData[i + 1] >= upLine) {
				if (flag == 2) {
					flag = 3;
					p3 = i;
					break;
				}
			}
		}
		if (flag == 3) {
			result = 2;
			wide = (float)(p2 - p1) / FS * 1000;
		}
	}

	if (wide < 9 || wide > 46) {
//		printf("not PVC wide %f\r\n", wide);
		result = 0;
	}
	
	*pIndex = maxI;
	return result;

}

