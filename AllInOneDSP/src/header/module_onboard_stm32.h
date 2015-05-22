/*
 * module_onboard_stm32.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef MODULE_ONBOARD_STM32_H_
#define MODULE_ONBOARD_STM32_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern unsigned char g_stm32_mask_count;
extern unsigned char g_is_aio_stm_error;

extern void OnBoardSTM32_Module_Init(void);
extern void STM32_I2C_Test(void);
extern void STM32_I2C_Handler(void);
extern int aio_stm32_writeByID(I2C_PacketID id, u8 *pWBuf, u8 nBufLen);
extern int aio_stm32_readByID(I2C_PacketID id, u8 *pRBuf, u8 nBufLen);
extern RETURN_TypeDef aio_stm32_UpdateHandle(UartProtocolPacket *pPacket);
extern int aio_stm32_setVerify(u8 state);

//<!-- STM32 I2C Api For Debug or Test Begin -->
extern int aio_stm32_readProtect(u8 *pindex, u16 *pmaxPress);
extern int aio_stm32_writeProtect(u8 *pindex, u16 *pmaxPress);
extern int aio_stm32_readNIBPState(u8 *pState);
extern int aio_stm32_writeNIBPState(u8 *pState);
//<!-- STM32 I2C Api For Debug or Test End -->


#ifdef __cplusplus
}
#endif

#endif /* MODULE_ONBOARD_STM32_H_ */
