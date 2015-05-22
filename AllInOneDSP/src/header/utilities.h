#ifndef __UTILITIES_H
#define __UTILITIES_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* usually define */
#define _START_UPLOAD_DATA_       //switch for uploading by uart
#define _UPLOAD_SELF_CHECK_
#define _UPLOAD_ECG_ADC_
#define _UPLOAD_RESP_ADC_
#define _UPLOAD_NIBP_ADC_
#define _UPLOAD_TEMP_ADC_
#define _UPLOAD_SpO2_ADC_
#define _UPLOAD_1HZ_DATA_

//#define _PRINT_ECG_SAMPLE_
//#define _PRINT_RESP_SAMPLE_
//#define _PRINT_NIBP_SAMPLE_

/* unusually define */
//#define _TEST_DATA_RANGE_
//#define _USE_IBP_FUNCTION_        //开启IBP功能
//#define _TIM1_TIM3_SAME_INTERRUPT_
//#define _NIBP_USED_WINDOWS_ //-DEBUG140117

#define DSP_SOFT_VERSION    0x04

#if 0
typedef unsigned char u8;
typedef unsigned long u32;
typedef unsigned short int u16;
typedef char s8;
typedef long int s32;
typedef short int s16;
#endif

typedef enum
{ 
    RESP2_channel   = 0,
    ECG1_channel    = 1,
    ECG2_channel    = 2,
    ECGV_channel    = 3,
} ECGADC_Channel_TypeDef;

typedef enum
{ 
    DIN_MODE    = 0,
    DRDY_MODE   = !DIN_MODE,
} ADCDataPinMode_TypeDef;

typedef enum
{ 
    ECG12_RA_LA_I   = 0x00,
    ECG12_RA_LL_II  = 0x01,
    ECG12_LA_LL_III = 0x02,
    ECG12_CAL       = 0x03,
} ECG12_Channel_TypeDef;

typedef enum
{ 
    ECGV_V1     = 0,
    ECGV_CAL    = 1,
} ECGV_Channel_TypeDef;

typedef enum
{ 
    ECG_DRV_RA  = 0x00,
    ECG_DRV_LA  = 0x01,
    ECG_DRV_LL  = 0x02,
    ECG_DRV_RL  = 0x03,
} ECG_DRV_Channel_TypeDef;

typedef enum
{ 
    ECG_3PROBE_I    = 0,
    ECG_3PROBE_II   = 1,
    ECG_3PROBE_III  = 2,
} ECG_3P_Channel_TypeDef;

typedef enum
{ 
    ECG_5PROBE_I    = 0,
    ECG_5PROBE_II   = 1,
    ECG_5PROBE_III  = 2,
    ECG_5PROBE_V    = 3,
} ECG_5P_Channel_TypeDef;

typedef enum
{ 
    RESP_RA_LL    = 0,//II
    RESP_RA_LA    = 1,//I
    RESP_AUTO     = 2,
} RESP_Channel_TypeDef;

typedef enum
{
    TRUE = 1,
    FALSE = 0,
} TRUE_OR_FALSE;

typedef enum
{
    RETURN_OK = 0,
    RETURN_ERROR = !RETURN_OK,
} RETURN_TypeDef;

typedef enum
{
    ECG_MODE_3PROBE = 1,
    ECG_MODE_5PROBE = 2,
} ECG_MODE_TypeDef;

typedef enum
{
    TIM3_ECG_ADC_R  = 0,
    TIM3_ECG_3Probe = 1,
} TIM3_HANDLER_TypeDef;
#define TIMER3_FUNC_LEN     2  //based on TIM3_HANDLER_TypeDef member

typedef enum
{
    TIM1_COM_ADC_R      = 0,
    TIM1_COM_ADC_START  = 1,
} TIM1_HANDLER_TypeDef;
#define TIMER1_FUNC_LEN     2  //based on TIM1_HANDLER_TypeDef member

typedef enum
{
    ENABLE  = 0,
    DISABLE = !ENABLE,
} EnableOrDisable;

typedef enum
{
    IDEL_FLAG,
    BUSY_FLAG,
    WORK_FLAG,
} StatusFlag_TypeDef;

typedef enum
{
    TEMP_channel   = 0,
    IBP1_channel    = 1,
    NIBP_channel    = 2,
    IBP2_channel    = 3,
} COMADC_Channel_TypeDef;

typedef enum
{
    TEMP_COM    = 0,
    TEMP_CH1    = 1,
    TEMP_CH2    = 2,
    TEMP_COM_R  = 3,
} TEMP_Channel_TypeDef;

typedef enum
{
    FAST_RELE    = 0,
    SLOW_RELE    = 1,
    PRESS_HOLD   = 2,
} NIBP_Release_TypeDef;

typedef enum
{ 
    AUTO_TEST           = 1,
    MANUAL_TEST,
    DEBUG_TEST,
    VENIPUNCTURE_TEST,
    CONTINUED_TEST,
} NIBPWorkMode_TypeDef;

typedef enum
{
    FMT_PACKET,
    FMT_ADC_DEC,
    FMT_ADC_HEX,
    FMT_VOLTAGE,
    FMT_CENTIGRADE,//摄氏温度
    FMT_FILTER_DATA,
    
    FMT_UPLOAD_OFF,
} UploadFormat_TypeDef;

typedef enum
{ 
    CH_PACE1    = 0,
    CH_PACE2    = 1,
    CH_PACEV    = 2,
} PACE_Channel_TypeDef;

typedef enum
{ 
    _EEPROM_TEST_ALL_W_,
    _EEPROM_TEST_ALL_R_,
    _EEPROM_RESTORE_ALL_,
    _EEPROM_TEST_BYTE_,
    _EEPROM_TEST_PAGE_WRITE_,
    _EEPROM_TEST_PAGE_READ_,
} EEPROM_Debug_TypeDef;

typedef enum
{ 
    ADULT     = 1,     
    CHILD     = 2,
    NEWBORN   = 3,
} PATIENT_TypeDef;

typedef enum
{ 
    SURGICAL     = 1,     
    MONITOR      = 2,
    DIAGNOSE     = 3,
} WorkMode_TypeDef;

typedef struct 
{
    u32 max;
    u32 min;
} u32Limits_TypeDef;

typedef enum
{
  
    /*----------- SpO2 Start ------------*/
    SpO2_RX_MODEL_VERSION_ID	 = 0x10,
    SpO2_RX_SELF_CHECK_ID		 = 0x12,
    SpO2_RX_PATIENT_SPEED_ID     = 0x13,
    SpO2_RX_WORK_MODE_ID         = 0x14,
    SpO2_RX_SOFTWARE_UPDATE_ID   = 0x15,
    SpO2_RX_MODEL_LOWPOWER_ID    = 0x16,//-QWB-

    SpO2_TX_MODEL_VERSION_ID     = 0x90,
    SpO2_TX_SELF_CHECK_ID        = 0x92,
    SpO2_TX_PATIENT_SPEED_ID     = 0x93,
    SpO2_TX_WORK_MODE_ID         = 0x94,
    SpO2_TX_SOFTWARE_UPDATE_ID   = 0x95,
    SpO2_TX_POWER_DETECT_ID  	 = 0x97,
    SpO2_TX_SPO2_REALTIME_ID  	 = 0x99,
    SpO2_TX_SPO2_NORMALIZED_ID 	 = 0x9A,//TX_SPO2_NORMALIZED_ID
    SpO2_TX_SPO2_CALC_ID 	 	 = 0x9B,
    /*----------- SpO2 Stop ------------*/

    /*----------- AIO Start ------------*/
    AIO_RX_ECG12_CHANNEL_ID	     = 0x20,
    AIO_RX_ECG_FILTER_SEL_ID	 = 0x21,
    AIO_RX_RESP_ASPHYXIA_TIME_ID = 0x24,
    AIO_RX_ECG_ST_SW_ID	         = 0x25,
    AIO_RX_ECG_ARRHYTHMIA_SW_ID	 = 0x26,
    AIO_RX_ECG_ST_MEASURE_ID	 = 0x27,
    AIO_RX_ECG_NOTCH_SW_ID	     = 0x28,
    AIO_RX_ECG_CAL_MODE_ID       = 0x29,
    AIO_RX_PROBE_MODE_ID	     = 0x2A,
    AIO_RX_ECG_PACE_SW_ID	     = 0x2B,
    AIO_RX_ECG_PACE_CHANNEL_ID	 = 0x2C,
    AIO_RX_ECG_PACE_OVERSHOOT_ID = 0x2D,
    AIO_RX_ECG_Alarm_ID          = 0x2E,
    AIO_RX_ECG_Debug_ID          = 0x2F,//Add for Debug ECG Module
        
    AIO_RX_RESP_UPLOAD_TYPE_ID   = 0x30,
    AIO_RX_RESP_THRESHOLD_ID     = 0x31,
    AIO_RX_RESP_CHANNEL_SEL_ID   = 0x32,
    AIO_RX_RESP_CARRIER_SW_ID    = 0x33,
    AIO_RX_RESP_Debug_ID         = 0x3E,//Add for Debug RESP Module
    AIO_RX_TEMP_Debug_ID         = 0x3F,//Add for Debug TEMP Module

    AIO_RX_NIBP_START_ID         = 0x40,
    AIO_RX_NIBP_STOP_ID          = 0x41,
    AIO_RX_NIBP_CYCLE_ID         = 0x42,
    AIO_RX_NIBP_VERIFY_ID        = 0x43,
    AIO_RX_NIBP_VERIFYING_ID     = 0x44,//不建议使用此命令，建议使用0x4F相关协议替换
    AIO_RX_NIBP_STM32_PRESS_ID   = 0x45,
    AIO_RX_NIBP_RESET_ID         = 0x46,
    AIO_RX_NIBP_PREPROCESS_PRESS_ID  = 0x47,
    AIO_RX_NIBP_GET_RESULT_ID    = 0x48,
    AIO_RX_NIBP_VENIPUNCTURE_ID  = 0x49,
    AIO_RX_NIBP_DUTY_CYCLE_ID    = 0x4B,
    AIO_RX_NIBP_CONTINUED_ID     = 0x4C,
    AIO_RX_NIBP_GAS_LEAK_ID      = 0x4D,
    AIO_RX_NIBP_Debug_ID         = 0x4F,//Add for Debug NIBP Module

    AIO_TX_ECG_REALTIME_ID       = 0xA0,
    AIO_TX_ECG_HR_RR_ID          = 0xA2,//心率HR和呼吸率RR
    AIO_TX_ARRHYTHMIA_RESULT_ID  = 0xA3,//心律失常分析结果
    AIO_TX_ECG_LEAD_INFO_ID      = 0xA7,
    AIO_TX_ECG_OVERLOAD_ID       = 0xA8,
    AIO_TX_ECG_ALARM_INFO_ID     = 0xAE,

    AIO_TX_RESP_REALTIME_ID      = 0xB0,
    AIO_TX_RESP_ASPHYXIA_ID	     = 0xB1,
        
    AIO_TX_TEMP_REALTIME_ID      = 0xB8,

    AIO_TX_NIBP_RESLULT_ID       = 0xC0,
    AIO_TX_NIBP_REALTIME_ID      = 0xC1,
    AIO_TX_NIBP_VERIFY_STATE_ID  = 0xC2,
    AIO_TX_NIBP_ACT_RESPOND_ID   = 0xC3,
    AIO_TX_NIBP_ALARM_ID         = 0xC4,
    AIO_TX_NIBP_MMHG_ID          = 0xC5,

    /*----------- common Start ------------*/
    COM_SOFTWARE_VERSION_ID      = 0xE0,
    COM_SELF_CHECK_ID            = 0xE1,
    COM_TX_PowerStatus_ID        = 0xE2,
    COM_TX_AbnormalReset_ID      = 0xE3,
    COM_PATIENT_TYPE_ID          = 0xE4,
    COM_PM_WORK_MODE_ID          = 0xE5,
    /*----------- common Stop ------------*/
    
    /*----------- AIO Stop ------------*/

    SF_SPO2_UPDATE               = 0xD1,
    SF_AIO_STM_UPDATE            = 0xD2,
    SF_AIO_DSP_UPDATE            = 0xD3,

    /*----------- Error Info Start ------------*/
    ERR_LICENSE_FAILED           = 0xF0,//U-Boot used
    /*----------- Error Info Stop ------------*/
    
} UART_PacketID;

#define NIBP_VERIFY_NUM     6   //nibp verify total num:0,50,100,150,200,290

//EERPOM的结构体定义
typedef struct
{
    char HeadOfEeprom[8];// = "Head";
    u16 nibp_Verify_mmHg[NIBP_VERIFY_NUM];//nibp verify total num:0,50,100,150,200,290
    u32 nibp_Verify_Val[NIBP_VERIFY_NUM];//nibp verify total num:0,50,100,150,200,290
    TRUE_OR_FALSE nibp_IsVerify;

    u16 nibp_protect_mmHg[2];           //array[0]=150mmHg,array[1]=300mmHg
    u16 nibp_protect_adc[2];
    char chIsProtectVerify;
    
    char TailOfEeprom[8];// = "Tail";
} EEPROM_StructureDef;

//串口协议包格式定义
typedef struct 
{ 
    u8 DR_Addr;             //目的地址 
    u8 SR_Addr;             //源地址 
    u8 PacketNum;           //包编号 
    u8 PacketID;            //包标识
    u8 Length;              //数据长度 
    u8 DataAndCRC[251];     //数据内容和CRC校验值
} UartProtocolPacket;
#define PACKET_FIXED_LENGHT    6   //Packet中固有元素的总长度

//<!-- define for STM32 I2C protocol packet start -->
#define I2C_PACKET_VAL_MAX_LEN  (62)
typedef struct
{
    unsigned char ID;   //bit0:1,Read;0,Write; bit1-7:ID
    unsigned char Lenght;
    unsigned char pValue[I2C_PACKET_VAL_MAX_LEN];
} I2CProtocolPacket;
typedef enum//range at 0x00 to 0x7F
{
    POWER_STATUS        = 0x00,
    SELF_CHECK          = 0x01,
    ECG_PROBE_RESULT    = 0x02,
    PROTECT_PRESSURE    = 0x03,
    CURRENT_PRESSURE    = 0x04,
    SOFTWARE_UPDATE,
    NIBP_VERIFY,
    NIBP_PROTECT_STATE,
} I2C_PacketID;

//<!-- define for STM32 I2C protocol packet stop -->

#define SETBIT(x,y)         ((x) |= (1<<(y)))
#define RESETBIT(x,y)       ((x) &= ~(1<<(y)))
#define countof(a)          (sizeof(a) / sizeof(*(a)))

#define ADC_MUX_CHANGE_DELAY    250// < 2043.5*tosc = 2043.5 * 129ns(@7.7647 MHz) = 264us
#define ADC_DRDY_PERIOD         5   //ADS1254,DRDY持续时间

#define UART_AIO_ADDR   0x55
#define UART_MCU_ADDR   0xAA
#define UART_SpO2_ADDR  0xCC

#ifdef __cplusplus
}
#endif

#endif /*__UTILITIES_H*/


