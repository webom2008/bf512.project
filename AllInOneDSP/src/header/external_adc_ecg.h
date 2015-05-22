/*
 * external_adc_ecg.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef EXTERNAL_ADC_ECG_H_
#define EXTERNAL_ADC_ECG_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern void ADC_CLK_Init(void);
extern void ADC_Synchronize(void);
extern void ECG_ADC_Init(void);
extern void ECG_ADC_ChannelSelect(ECGADC_Channel_TypeDef channel);
extern void ECG_ADC_SaveData(void);
extern void ECG_ADC_3Probe_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* EXTERNAL_ADC_ECG_H_ */

