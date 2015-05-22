/**
  *(C) COPYRIGHT 2014 CVTE.SEECARE
  ******************************************************************************
  * @file    IAP/inc/commands.h
  * @author  CVTE.SEECARE.QiuWeibo
  * @version V1.0.0
  * @date    2014/01/21
  * @brief   Header file for commands.c
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __COMMANDS_H
#define __COMMANDS_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "includes.h"



/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
uint8_t Read_Opcode(void);
uint32_t Read_Add(void);
uint16_t Read_Byte_Page_Number(void);
void Read_Memory_Command(void);
void Write_Memory_Command(void);
void Erase_Page(uint32_t Add_Flash,uint16_t page_number);
void Erase_Page_Command(void);
void User_Space_Memory_Erase_Command(void);

#ifdef __cplusplus
}
#endif

#endif /* __COMMANDS_H */

/************************END OF FILE****/
