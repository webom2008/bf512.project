/*
 * module_nibp.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef MODULE_NIBP_H_
#define MODULE_NIBP_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"
typedef struct PUMP_DEVICE
{
    u32 pump_avg_mmHg;      //mmHg * AMPLIFY_FACTOR
    u32 pump_prev_time;
    u32 pump_curr_time;
    u32 pump_prev_mmHg;     //mmHg * AMPLIFY_FACTOR
    u32 pump_curr_mmHg;     //mmHg * AMPLIFY_FACTOR
    u16 pump_curr_speed;
    //int (*write)(struct PUMP_DEVICE *pDev);
    //int (*read)(struct PUMP_DEVICE *pDev);
    
} pump_device;

typedef struct NIBP_DEVICE
{
    u32 mmHg_val;           //mmHg * AMPLIFY_FACTOR
    u32 adc_val;
    u32 tick;
    EnableOrDisable verifyStauts;
    EnableOrDisable waitForADCStatus;
    //int (*write)(struct NIBP_DEVICE *pDev);
    //int (*read)(struct NIBP_DEVICE *pDev);
    
} nibp_device;

typedef struct
{
    int BPM;
    int DP;
    int SP;
} NIBP_SimpleResult;

extern nibp_device *g_pNIBP_dev;
extern bool g_IsNIBP_Upload;
extern u16 g_Rising_mmHgCur;

extern void NIBP_Init(void);
extern void NIBP_Handler(void);
extern void NIBP_adc_store_data(void);
extern void NIBP_StartSample(void);
u16 NIBP_PopAlgoResult(u32* pu32Tick, u32* pu32mmHg, u32 *pu32adc);
extern RETURN_TypeDef NIBP_SetPatientType(u8 newType);

//<!-- onMessgae处理接口 Start -->
extern void NIBP_SampleOneTime(void);
extern void NIBP_ForceStop(void);
extern void NIBP_SetCycle(u8 cycler);
extern void NIBP_SetPWM_DutyCycles(u16 percent);
extern void NIBP_SetPrepressure(u16 u16mmHg);
extern void NIBP_VerifyBySimulator(const u8 mode);
extern void NIBP_VerifyAction(const u8 mmHg);
extern void NIBP_STM32VerifyAction(u8 *pBuf, u8 nLen);
extern void NIBP_resetModule(void);
extern void NIBP_getResult(UartProtocolPacket *pPacket);
extern void NIBP_setVenipuncture(UartProtocolPacket *pPacket);
extern void NIBP_setContinueMode(UartProtocolPacket *pPacket);
extern void NIBP_setGasLeakTest(UartProtocolPacket *pPacket);
extern RETURN_TypeDef NIBP_DebugInterface(UartProtocolPacket *pPacket);
//<!-- onMessgae处理接口 Stop -->

#ifdef __cplusplus
}
#endif

#endif /* MODULE_NIBP_H_ */
