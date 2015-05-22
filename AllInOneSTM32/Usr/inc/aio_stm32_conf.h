/*****************************************************************************
 * aio_stm32_conf.h
 *****************************************************************************/
#ifndef AIO_STM32_CONF_H_
#define AIO_STM32_CONF_H_
 
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"//RCC_ClocksTypeDef

#include "utilities.h"
#include "main.h"
#include "uart.h"
#include "system_tick.h"
#include "system_check.h"
#include "system_clock.h"
#include "test_led_lamp.h"
#include "module_nibp.h"
#include "module_ecg.h"
#include "system_adc.h"
#include "system_nvic.h"
#include "watchdog.h"
#include "power.h"
#include "i2c_int.h"
#include "i2c_packet.h"


#ifdef __cplusplus
}
#endif
 
#endif /* AIO_STM32_CONF_H_ */

