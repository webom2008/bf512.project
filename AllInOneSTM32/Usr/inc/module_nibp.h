/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MODULE_NIBP_H
#define __MODULE_NIBP_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "aio_stm32_conf.h"

extern NIBP_ProtectDef* p_nibp_press;
extern char g_nibp_fast_rele_state;

extern void NIBP_Init(void);
extern void NIBP_Handler(void);
extern int NIBP_SetSafePress(u8 *pBuf, u8 nLen);
extern int NIBP_SetNIBPVerify(u8 state);
extern int NIBP_SetNIBPFastReleState(u8 state);

#ifdef __cplusplus
}
#endif
#endif /*__MODULE_NIBP_H*/

