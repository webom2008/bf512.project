/*****************************************************************************
 * AlgLib.c
 *****************************************************************************/
#include "AlgLib.h"

int GetDirection(int data)
{
    if(data>0)return 1;
    else if(data==0)return 0;
    else return -1;
}

void Diff_N(int* pDin, int nLen, int* pDout)
{
    int i = 0;
    int nIndexMax = nLen - 1;

    for (i = 0; i < nIndexMax; i++)
    {
        pDout[i] = pDin[i + 1] - pDin[i];
    }
}

void Diff_F(float* pDin, int nLen, float* pDout)
{
    int i = 0;
    int nIndexMax = nLen - 1;

    for (i = 0; i < nIndexMax; i++)
    {
        pDout[i] = pDin[i + 1] - pDin[i];
    }
}

int Mean_N(int* pDin, int nLen)
{
    float nMean = 0;
    int i = 0;

    for (i = 0; i < nLen; i++)
    {
        nMean += pDin[i];
    }
    return (int)(nMean / nLen + 0.5); // 四舍五入
}

int Mean_ABS_N(int* pDin, int nLen)
{
    float nMean = 0;
    int i = 0;

    for (i = 0; i < nLen; i++)
    {
        nMean += my_abs(pDin[i]);
    }
    return (int)(nMean / nLen + 0.5); // 四舍五入
}

float Mean_F(float* pDin, int nLen)
{
    float nMean = 0;
    int i = 0;

    for (i = 0; i < nLen; i++)
    {
        nMean += pDin[i];
    }
    return (nMean / nLen);
}

float Fix(float fIn)
{
    return ((float)((int)(fIn)));
}

int Min_Index_N(int* pt, int nLen, int* pIndex)
{
    int index, i, min;
    min = pt[0];
    index = 0;
    for (i = 1; i < nLen; i++)
    {
        if (pt[i] < min)
        {
            index = i;
            min = pt[i];
        }
    }
    *pIndex = index;
    return min;
}

int Max_Index_N(int* pt, int nLen, int* pIndex)
{
    int index, i, max;
    max = pt[0];
    index = 0;
    for (i = 1; i < nLen; i++)
    {
        if (pt[i] > max)
        {
            index = i;
            max = pt[i];
        }
    }
    *pIndex = index;
    return max;
}

int Max_N(int* pt, int nLen)
{
    int i, max = pt[0];
    for (i = 1; i < nLen; i++)
    {
        if (pt[i] > max)
        {
            max = pt[i];
        }
    }
    return max;
}

int Min_N(int* pt, int nLen)
{
    int i, min = pt[0];
    for (i = 1; i < nLen; i++)
    {
        if (pt[i] < min)
        {
            min = pt[i];
        }
    }
    return min;
}

int Sum_N(int* pt, int nLen)
{
    int sum = 0;
    int i;
    for (i = 0; i < nLen; i++)
    {
        sum += pt[i];
    }
    return sum;
}

void Abs_N(int* pt, int nLen)
{
    int i;
    for (i = 0; i < nLen; i++)
    {
        pt[i] = my_abs(pt[i]);
    }
}

int Max_Index_Abs_N(int* pt, int nLen, int* pIndex)
{
    int index, i, max;
    int tmp;
    max = pt[0];
    index = 0;
    for (i = 1; i < nLen; i++)
    {
        tmp = my_abs(pt[i]);
        if (tmp > max)
        {
            index = i;
            max = tmp;
        }
    }
    if (pIndex != 0)
    {
        *pIndex = index;
    }
    return max;
}

int Max_Abs_N(int* pt, int nLen)
{
    int i, max;
    max = my_abs(pt[0]);
    for (i = 1; i < nLen; i++)
    {
        if (my_abs(pt[i]) > max)
        {
            max = my_abs(pt[i]);
        }
    }
    return max;
}

void GetDelta(int* din, int len, int* dout)
{
    int mean, i;
    if(len>0)
    {
        mean = Mean_N(din, len);
        for (i = 0; i < len; i++)
        {
            dout[i] = din[i] - mean;
        }
    }
}

void GetDeltaAbs(int* din, int len, int* dout)
{
    int mean, i;
    if(len>0)
    {
        mean = Mean_N(din, len);
        for (i = 0; i < len; i++)
        {
            dout[i] = my_abs(din[i] - mean);
        }
    }
}

void Zoom(int* din, int len, int Max, int* dout)
{
    int MaxDin, i;
    MaxDin = Max_Abs_N(din, len);
    if (MaxDin > 0)
    {
        for (i = 0; i < len; i++)
        {
            din[i] = din[i] * Max / MaxDin;
        }
    }
}

void ArrMulArr(int* din1, int* din2, int len, int* dout)
{
    int i;
    for (i = 0; i < len; i++)
    {
        dout[i] = din1[i] * din2[i];
    }
}

void ArrMulData(int* din1, int din2, int len, int* dout)
{
    int i;
    for (i = 0; i < len; i++)
    {
        dout[i] = din1[i] * din2;
    }
}

int IntCmp(const void *a , const void *b)
{
    return *(int *)a - *(int *)b;
}


float Max_F(float* pt, int nLen) 
{
	int i;
	float max = pt[0];
    for (i = 1; i < nLen; i++)
    {
        if (pt[i] > max)
        {
            max = pt[i];
        }
    }
    return max;
}



float Min_F(float* pt, int nLen)
{
	int i;
	float min = pt[0];
    for (i = 1; i < nLen; i++)
    {
        if (pt[i] < min)
        {
            min = pt[i];
        }
    }
    return min;
}

int GetMedian_N(int* Data, int DataLen, float cut)
{
    int DataB = 0, IndexB = -1, DataS = 0, IndexS = -1, IndexNow;
    int ii, jj;
    
    for(ii=0; ii<DataLen; ii++){
        if(IndexB == -1 || IndexS == -1 || (Data[ii] < DataB && Data[ii] > DataS)){
            IndexNow = 0;
            for(jj=0; jj<DataLen; jj++){
                if(Data[jj] < Data[ii])
                    IndexNow++;
            }
            if(IndexNow == (int)(DataLen*cut))
                return Data[ii];          
            else if((IndexNow < IndexB || IndexB == -1) && IndexNow > (int)(DataLen*cut)){
                IndexB = IndexNow;
                DataB = Data[ii];
            }
            else if((IndexNow > IndexS || IndexS == -1) && IndexNow < (int)(DataLen*cut)){
                IndexS = IndexNow;
                DataS = Data[ii];
            }
        }
    }
    return DataS;
}


