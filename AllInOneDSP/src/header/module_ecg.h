/*
 * module_ecg.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef MODULE_ECG_H_
#define MODULE_ECG_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

#define ECG_PACE_PIN                PF15

#define ECG_SAMPLE_FREQ             500
#define ECG_Sampling_Buffer_Len     (ECG_SAMPLE_FREQ+1)

typedef struct PACE_STRUCT
{
    PACE_Channel_TypeDef channel;
    bool sw;
    bool newState;
    bool oldState;
} CPACE_STRUCT;

extern TRUE_OR_FALSE g_3probe_run;
extern TRUE_OR_FALSE g_5probe_run;
extern ECG_3P_Channel_TypeDef g_3probe_channel;
extern ECG_5P_Channel_TypeDef g_5probe_ECG1, g_5probe_ECG2;
extern volatile unsigned long g_3PorbeResult[3];
extern volatile unsigned long g_5PorbeResult[4];//I,II,III,V
extern volatile unsigned long g_5Porbe_aVR, g_5Porbe_aVL, g_5Porbe_aVF;
extern bool g_IsECG_Upload;
extern CPACE_STRUCT g_pace_state;

extern void ECG_Init(void);
extern void ECG_Handler(void);
extern void ECG_CAL_DRV_signal(void);
extern void ECG_PrintResult(void);
extern void ECG_3ProbeChannelSelect(const ECG_3P_Channel_TypeDef channel);
extern void ECG_CAL_DRV_ModeSet(EnableOrDisable status);
extern void ECG_PACE_SetInterrupt(const EnableOrDisable status);
extern void ECG_SampleTimer_SetInterrupt(const EnableOrDisable status);
extern void ECG_adc_store_data(void);
extern u16 ECG_PopAlgoResult(u32* pu32Tick, s32* ps32I, s32* ps32II, s32* ps32V, char* pPace);
extern void ECG_ArrhythmiaUpload(u8 alert);
extern void ECG_NotchFilterHandle(u8 sw);
extern void ECG_SetDefibrillate(void);

//<!-- For test interface start -->
extern RETURN_TypeDef ECG_DebugInterface(UartProtocolPacket *pPacket);
extern RETURN_TypeDef ECG_setAlarm(UartProtocolPacket *pPacket);
extern void ECG_SetProbeMode(UartProtocolPacket *pPacket);
extern void ECG_SetECG12Channel(UartProtocolPacket *pPacket);
extern void ECG_RespondProbeMode(void);
extern void ECG_SetSTAnalyzeSW(UartProtocolPacket *pPacket);
extern void ECG_SetArrhythmiaSW(UartProtocolPacket *pPacket);
extern void ECG_SetNotchFilterSW(UartProtocolPacket *pPacket);
extern void ECG_SetPaceSW(UartProtocolPacket *pPacket);
extern void ECG_SetPaceChannel(UartProtocolPacket *pPacket);
extern void ECG_SetPaceOvershootSw(UartProtocolPacket *pPacket);
extern void ECG_SetSTMesureSetting(UartProtocolPacket *pPacket);

//<!-- For test interface stop -->

extern void Timer3_Configure(const unsigned int n1us, 
                            const TIM3_HANDLER_TypeDef target);

#ifdef __cplusplus
}
#endif

#endif /* MODULE_ECG_H_ */
