/*****************************************************************************
 * queue.h
 *****************************************************************************/
#ifndef _QUEUE_H_
#define _QUEUE_H_
 
#ifdef __cplusplus
extern "C" {
#endif

void Queue_S_Init(QUEUE_S* pQueueS);
int Queue_S_Remove(QUEUE_S* pQueueS, int index, int nLen);
void Queue_S_RemoveAtLast(QUEUE_S* pQueueS, int nLen);
void Queue_S_Append(QUEUE_S* pQueueS, STEEP_UNIT data);
void Queue_S_Append_Syn(QUEUE_S* pQueueS, STEEP_UNIT data, int Syn);
void Queue_S_Insert(QUEUE_S* pQueueS, int index, STEEP_UNIT data);
STEEP_UNIT Get_STEEP_UNIT(int Xstart, int Ystart, int Xend, int Yend, int Mode, int eva);
void Queue_S_2_IntArr(QUEUE_S QueueS, int* pXstart, int* pYstart, int* pXend, int* pYend, int* pMode, int* peva);

#ifdef __cplusplus
}
#endif

#endif //_QUEUE_H_
