/*
 * module_ibp.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include "module_ibp.h"

#define IBP_HANDLE_PERIOD_MS    (500)

bool IsIBPSample,IsIBPzero;
unsigned long g_IBPadc_Val[2];//�ֱ��IBP1��IBP2��ֵ

//<!-- ����һ����ִ�еĺ�����ȫ��������SDRAM(����Ĭ�ϵ�DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void IBP_Init(void);
//<!-- ����һ����ִ�еĺ�����ȫ��������SDRAM(����Ĭ�ϵ�DATA SRAM) Stop -->

void IBP_Init(void)
{
    IsIBPSample = false;
    IsIBPzero = true;
    AHCT595_SetIBPzero(ENABLE);
}

void IBP_Handler(void)
{
#ifdef _USE_IBP_FUNCTION_
    static unsigned long ulNextChangeTime = 0;
    static char channelStatus = 0;
    if(IsOnTime(ulNextChangeTime)) // is on time or over time
    {
        //����ɼ����޸�״̬
        if (false == IsIBPSample)
        {
            if (0 == channelStatus)
            {
                IsIBPzero = true;
                AHCT595_SetIBPzero(ENABLE);
            }
            else
            {
                IsIBPzero = false;
                AHCT595_SetIBPzero(DISABLE);
            }
            IsIBPSample = true;
        }
        else//�²ɼ������ѵ��������ϴ���δ�ɼ���ɣ������澯��Ϣ
        {
            AIO_printf("\r\nIBP_Handler lost sample.warning.");
        }
        
        channelStatus = ~channelStatus;
        ulNextChangeTime += getTickNumberByMS(IBP_HANDLE_PERIOD_MS);
    }
#endif//_USE_IBP_FUNCTION_
}

void IBPReslultUpload(void)
{
#ifdef _USE_IBP_FUNCTION_
    long reslut[2];
    reslut[0] = s24_to_s32(g_IBPadc_Val[0]);
    reslut[1] = s24_to_s32(g_IBPadc_Val[1]);
    if (true == IsIBPzero)
    {
        AIO_printf("\r\nZERO>>IBP1:%d\tIBP2:%d", reslut[0],reslut[1]);
    }
    else
    {
        AIO_printf("\tNORMAL>>IBP1:%d\tIBP2:%d", reslut[0],reslut[1]);
    }
#endif//_USE_IBP_FUNCTION_
}

