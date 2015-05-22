#ifndef SN74HC595_H_
#define SN74HC595_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern void AHCT595_Init(void);
extern void AHCT595_SetComADCChannel(COMADC_Channel_TypeDef channel);
extern void AHCT595_SetTempChannel(TEMP_Channel_TypeDef channel);
extern void AHCT595_SetIBPzero(EnableOrDisable status);

#ifdef __cplusplus
}
#endif

#endif /* SN74HC595_H_ */

