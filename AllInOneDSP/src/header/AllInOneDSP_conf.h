/*****************************************************************************
 * AllInOneDSP_conf.h
 *****************************************************************************/
#ifndef ALL_IN_ONE_DSP_CONF_H_
#define ALL_IN_ONE_DSP_CONF_H_
 
#ifdef __cplusplus
extern "C" {
#endif

#if 0

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef signed long long s64;

#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;
typedef signed char sint8_t;
typedef signed short sint16_t;
typedef signed long sint32_t;
typedef signed long long sint64_t;


#define __SYSTEM_CHECK_C_
#define __SYSTEM_GPIO_C_
#define __SYSTEM_POWER_C_
#define __SYSTEM_SFLASH_C_
#define __SYSTEM_SPI_C_

#include <cdefBF512.h>
#include <ccblkfn.h>
#include <bfrom.h>
#include <sys/exception.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>


#include "utilities.h"

#include "AllInOneDSP.h"
#include "common_handle.h"
#include "eeprom.h"
#include "external_adc_com.h"
#include "external_adc_ecg.h"
#include "external_watchdog.h"
#include "module_ecg.h"
#include "module_ibp.h"
#include "module_nibp.h"
#include "module_onboard_stm32.h"
#include "module_resp.h"
#include "module_spo2_uart.h"
#include "module_temperature.h"
#include "probe_detect.h"
#include "sn74hc595.h"
#include "system_check.h"
#include "system_clock.h"
#include "system_gpio.h"
#include "system_power.h"
#include "system_sdram.h"
#include "system_spi.h"
#include "system_sflash.h"
#include "system_tick.h"
#include "system_twi.h"
#include "system_uart.h"
#include "test_led_lamp.h"
#include "uart_protocol.h"

//<!-- Algorithm include start -->
#include "AlgorithmUtilities.h"

#include "ECGAlgorithm.h"
#include "EcgUtilities.h"
#include "algorithm_ecg.h"

#include "RESPAlgorithm.h"
#include "RespUtilities.h"
#include "algorithm_resp.h"

#include "NIBPAlgorithm.h"
#include "NibpUtilities.h"
#include "algorithm_nibp.h"
//<!-- Algorithm include stop -->

//IVG(7~15)，数值越小，中断优先级越高
EX_INTERRUPT_HANDLER(CoreTimer_ISR);        //SystemTick的中断函数入口
EX_INTERRUPT_HANDLER(IVG8_ISR);             //ECG_ADC,ECG_500Hz中断函数入口
EX_INTERRUPT_HANDLER(IVG9_ISR);             //COM_ADC,PACE中断函数入口
EX_INTERRUPT_HANDLER(IVG10_ISR);            //TIM3的中断函数入口
EX_INTERRUPT_HANDLER(IVG11_SpO2_UART_ISR);  //SpO2 UART中断函数入口
EX_INTERRUPT_HANDLER(IVG12_AIO_UART_ISR);   //MCU UART中断函数入口
EX_INTERRUPT_HANDLER(IVG13_ISR);            //TIM1定时中断入口

#ifdef __cplusplus
}
#endif
 
#endif /* ALL_IN_ONE_DSP_CONF_H_ */

