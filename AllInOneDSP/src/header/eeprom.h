/*
 * eeprom.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern EEPROM_StructureDef* pEeprom_struct;

extern void EEPROM_Init(void);
extern void EEPROM_ExamineAll(void);
extern RETURN_TypeDef EEPROM_ExamineHeadTail(void);
extern void EEPROM_TestInterface(const EEPROM_Debug_TypeDef type);
extern void EEPROM_LoopTest(void);
extern RETURN_TypeDef EEPROM_BufferWrite(u16 u16Addr, u8 *pData, u16 u8Len);
extern RETURN_TypeDef EEPROM_SequentialRead(const u16 u16Addr, u8 *pData, const u16 u8Len);

//App½Ó¿Ú
extern RETURN_TypeDef EEPROM_RefreshNIBPVerify(void);
extern RETURN_TypeDef EEPROM_saveSTM32NIBPVerify(void);
extern RETURN_TypeDef EEPROM_saveRAMSTM32NIBPVerify(u8 index, u16 *mmHg, u16 *adc);

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_H_ */
