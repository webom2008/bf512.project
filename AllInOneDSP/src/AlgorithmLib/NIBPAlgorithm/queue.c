/*****************************************************************************
 * queue.c
 *****************************************************************************/
#include "NIBPAlgorithm_conf.h"

void Queue_S_Init(QUEUE_S* pQueueS)
{
    pQueueS->nLen = 0;
}

int Queue_S_Remove(QUEUE_S* pQueueS, int index, int nLen)
{
    if (index < pQueueS->nLen)
    {
        pQueueS->nLen=my_max(index, pQueueS->nLen-nLen);
        memcpy( &(pQueueS->pSteep[index]),
            &(pQueueS->pSteep[index+nLen]),
            (pQueueS->nLen-index)*sizeof(STEEP_UNIT));
        return 1;
    }
    return -1;
}

void Queue_S_RemoveAtLast(QUEUE_S* pHead, int nLen)
{
    pHead->nLen=my_max(0, pHead->nLen-nLen);
}

void Queue_S_Append(QUEUE_S* pQueueS, STEEP_UNIT data)
{
    if (pQueueS->nLen >= QUEUE_S_LEN)
    {
        Queue_S_Remove(pQueueS,0,1);
        pQueueS->nLen--;
    }
    pQueueS->pSteep[pQueueS->nLen] = data;
    pQueueS->nLen++;
}

void Queue_S_Append_Syn(QUEUE_S* pQueueS, STEEP_UNIT data, int Syn)
{
    if(pQueueS->nLen > 0)
    {
        if(data.Xstart - pQueueS->pSteep[pQueueS->nLen-1].Xend <= Syn)
        {
            pQueueS->pSteep[pQueueS->nLen-1].Xend = data.Xend;
            pQueueS->pSteep[pQueueS->nLen-1].Yend = data.Yend;
        }
        else if(pQueueS->pSteep[pQueueS->nLen-1].Xend \
            - pQueueS->pSteep[pQueueS->nLen-1].Xstart \
            <= Syn)
        {
            pQueueS->pSteep[pQueueS->nLen-1] = data;
        }
        else
        {
            Queue_S_Append(pQueueS, data);
        }
    }
    else
    {
        Queue_S_Append(pQueueS, data);
    }
}

void Queue_S_Insert(QUEUE_S* pQueueS, int index, STEEP_UNIT data)
{
    if(index < pQueueS->nLen)
    {
        if (pQueueS->nLen >= QUEUE_S_LEN)//队列已满,删除第一个
        {
            memcpy(&pQueueS->pSteep[0],&pQueueS->pSteep[1],(index-1)*sizeof(STEEP_UNIT));
        }
        else
        {
            memcpy(&pQueueS->pSteep[index+1],&pQueueS->pSteep[index],\
                (pQueueS->nLen-index)*sizeof(STEEP_UNIT));
        }
        pQueueS->pSteep[index] = data;
    }
    else if(index == pQueueS->nLen)
    {
        Queue_S_Append(pQueueS, data);
    }
}

STEEP_UNIT Get_STEEP_UNIT(int Xstart, int Ystart, int Xend, int Yend, int Mode, int eva)
{
    STEEP_UNIT steep;
    steep.Xstart=Xstart;
    steep.Ystart=Ystart;
    steep.Xend=Xend;
    steep.Yend=Yend;
    steep.Mode=Mode;
    steep.eva=eva;
    return steep;
}

void Queue_S_2_IntArr(QUEUE_S QueueS, int* pXstart, int* pYstart, int* pXend, int* pYend, int* pMode, int* peva)
{
    int i;
    for(i=0; i<QUEUE_S_LEN; i++)
    {
        pXstart[i] = QueueS.pSteep[i].Xstart;
        pYstart[i] = QueueS.pSteep[i].Ystart;
        pXend[i] = QueueS.pSteep[i].Xend;
        pYend[i] = QueueS.pSteep[i].Yend;
        pMode[i] = QueueS.pSteep[i].Mode;
        peva[i] = QueueS.pSteep[i].eva;
    }
}

