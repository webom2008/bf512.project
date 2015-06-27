/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

Description:
            This is the include file for the AD7477A ADC SPI driver.  
            
*********************************************************************************/

#ifndef __ADI_AD7477A_H__
#define __ADI_AD7477A_H__

/* disable misra diagnostics as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#include <services/services_misra_suppress.h>
#endif /* _MISRA_RULES */

/* ****************************************************************************
 *
 *
 * (c) Copyright Analog Devices Inc 2005
 *
 * ****************************************************************************
 */
 
/* ****************************************************************************
 * RESULT CODES
 * ****************************************************************************
 */
enum { 
    ADI_AD7477A_RESULT_START=ADI_AD7477A_ENUMERATION_START  // insure this order remains
};
 
 
/* ****************************************************************************
 * COMMAND CODES
 * ****************************************************************************
 */
 
enum { 
    ADI_AD7477A_CMD_START=ADI_AD7477A_ENUMERATION_START,    
    ADI_AD7477A_CMD_SET_SAMPLE_RATE,                // set sample rate in sample/s                  (Value = address of value containing samples/sec)
    ADI_AD7477A_CMD_SET_OPERATION_MODE,             // set mode of operation                        (Value = ADI_AD7477A_OPERATION_MODE enumeration)
    ADI_AD7477A_CMD_DECLARE_SLAVE_SELECT,           // tells the driver which slave select # to use (Value = 1-7)
    ADI_AD7477A_CMD_SET_QUIET_PERIOD_FACTOR         // replaces default factor with requested       (Value = default factor (default uses 1062)
};
 
/* ****************************************************************************
 * COMMAND VALUES
 * ****************************************************************************
 */
 
// Enum to describe the range of valid values for the 
// ADI_AD7477A_CMD_SET_OPERATION_MODE command
enum {
    ADI_AD7477A_MODE_POWERDOWN,
    ADI_AD7477A_MODE_NORMAL
};
 

// entry point
#ifndef ADI_AD7477A_MODULE
extern ADI_DEV_PDD_ENTRY_POINT ADI_AD7477A_EntryPoint;
#endif

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif /* __ADI_AD7477A_H__ */

