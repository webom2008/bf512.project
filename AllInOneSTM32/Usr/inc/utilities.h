/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UTILITIES_H
#define __UTILITIES_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "aio_stm32_conf.h"

//#define _PRINT_INTERNAL_ADC_
//#define _PRINT_ECG_RESULT_

#define __WATCHDOG_C__
#define __POWER_C__
#define __PACKET_C__
#define __I2C_INT_C__



/* Exported types ------------------------------------------------------------*/

typedef enum
{
    TRUE = 0,
    FALSE = !TRUE,
} TRUE_OR_FALSE;

typedef enum
{
    BUFFER_EMPTY,
    BUFFER_FULL,
    BUFFER_NORMAL,
} UART_BUFFER_STATUS;

typedef enum
{
    Ctrl_Idle = 0,
    Ctrl_Busy = !Ctrl_Idle,
} NIBP_CtrlStatus;

typedef enum
{
    RETURN_SUCCESS = 1,
    RETURN_ERROR   = -1,
} RETURN_Type;

typedef enum
{
    LA_OFF = 0,
    RA_OFF,
    LL_OFF,
    RL_OFF,
    V1_OFF,
    ECG1_OVERLOAD,
    ECG2_OVERLOAD,
    ECGV_OVERLOAD,
} ECG_SelectTypeDef;

typedef struct
{
    u16 RA_OFF_Val;
    u16 RL_OFF_Val;
    u16 LA_OFF_Val;
    u16 LL_OFF_Val;
    u16 V1_OFF_Val;
    u16 ECG1_OVERLOAD_Val;
    u16 ECG2_OVERLOAD_Val;
    u16 ECGV_OVERLOAD_Val;
} ECG_ResultTypeDef;

typedef struct
{
    __IO u16 DET_NIBP;
    __IO u16 DET_D3V3E;
    __IO u16 DET_VDDINT;
    __IO u16 DET_5VAE;
    __IO u16 DET_OVLD_OFF;
} InternalADCResult;

typedef struct
{
    u16 u16PressADC_Max;
    u16 u16PressADC_Min;
    char Index;  //0:default, >0:verified
} NIBP_ProtectDef;

/* golbal variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define SETBIT(x,y)     ((x)|=(1<<(y)))
#define RESETBIT(x,y)   ((x)&=~(1<<(y)))

/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /*__UTILITIES_H*/

