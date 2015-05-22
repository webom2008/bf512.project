/*
 * external_adc_com.h
 *
 *  Created on: 2013-10-17
 *      Author: QiuWeibo
 */

#ifndef EXTERNAL_ADC_COM_H_
#define EXTERNAL_ADC_COM_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

#define COM_ADC_SCLK_PIN    PF1
#define COM_ADC_DATA_PIN    PF2

#define COM_ADC_SCLK_LOW()      (*pPORTFIO_CLEAR = COM_ADC_SCLK_PIN)
#define COM_ADC_SCLK_HIGH()     (*pPORTFIO_SET = COM_ADC_SCLK_PIN)

extern COMADC_Channel_TypeDef  g_COMADC_channel;
extern char  g_COMADC_state;
extern TIM1_HANDLER_TypeDef g_Tim1_target;

extern void COM_ADC_Init(void);
extern void COM_ADC_DataPin_Init(ADCDataPinMode_TypeDef mode);
extern void Timer1_SetInterrupt(const EnableOrDisable status);
extern void (*Tim1_HandlerSET[TIMER1_FUNC_LEN])(void);

#ifdef __cplusplus
}
#endif

#endif /* EXTERNAL_ADC_COM_H_ */

