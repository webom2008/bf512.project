/*
 * Original document:
 * Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved.
 *
 * This software is proprietary and confidential.  By using this software you
 * agree to the terms of the associated Analog Devices License Agreement.
 *
 * $RCSfile: adi_ssl_Init.c,v $
 * $Revision: 2095 $
 *
 * Changes:
 * Copyright (c) 2012 Thermo Fisher Scientific
 */

/*
 * This file contains a convenient mechanism to initialize and terminate
 * all system services and the device manager.  The application should
 * modify the sizings located in the adi_ssl_Init.h file as needed by their
 * application, then add this file to their list of source files for
 * their project.
 *
 * The application should then make one call to the function adi_ssl_Init(),
 * insuring the return value from the function call returns the value 0.
 * This function call initialized all services and the device manager
 * according to the sizings defined in adi_ssl_init.h.
 *
 * When no longer needed, the application can then one call to the function
 * adi_ssl_Terminate(), insuring the return value from the function call
 * returns the value 0.  This function call terminates all services and
 * the device manager.
 *
 * The handles to the DMA and Device Manager are stored in the global
 * variables adi_dma_ManagerHandle and adi_dev_ManagerHandle, respectively.
 * These handles can be passed to subsequent adi_dev_Open() calls as
 * necessary.
 *
 * DO NOT MODIFY ANYTHING IN THIS FILE  <-- Except to reflect your target
 */

/*
 * Include files
 */

#include <services/services.h>      // system service includes
#include <sys/pll.h>                // PLL 
#include <drivers/adi_dev.h>        // device driver includes
#include "adi_ssl_init.h"           // initialization sizings

/*
 * Handles
 *
 * This section provides storage for handles into the services and device
 * manager.  The application may use these handles into calls such as
 * adi_dev_Open() for opening device drivers, adi_dma_OpenChannel() for
 * opening DMA channels etc.
 */

ADI_DMA_MANAGER_HANDLE adi_dma_ManagerHandle;   // handle to the DMA manager
ADI_DEV_MANAGER_HANDLE adi_dev_ManagerHandle;   // handle to the device manager

 /*
 * Global storage data
 *
 * This section provides memory, based on the sizing defined above,
 * for each of the services.
 */

static  u8  InterruptServiceData        [ADI_INT_SECONDARY_MEMORY *
                                         ADI_SSL_INT_NUM_SECONDARY_HANDLERS];
static  u8  DeferredCallbackServiceData [ADI_DCB_QUEUE_SIZE *
                                         ADI_SSL_DCB_NUM_SERVERS];
static  u8  DMAServiceData              [ADI_DMA_BASE_MEMORY +
                         (ADI_DMA_CHANNEL_MEMORY *  ADI_SSL_DMA_NUM_CHANNELS)];
static  u8  FlagServiceData             [ADI_FLAG_CALLBACK_MEMORY *
                                         ADI_SSL_FLAG_NUM_CALLBACKS];
static  u8  SemaphoreServiceData        [ADI_SEM_SEMAPHORE_MEMORY *
                                         ADI_SSL_SEM_NUM_SEMAPHORES];
static  u8  DevMgrData                  [ADI_DEV_BASE_MEMORY +
                           (ADI_DEV_DEVICE_MEMORY * ADI_SSL_DEV_NUM_DEVICES)];

/*
 *   Table configuration for power, External Bus Interface Unit and SDRAM
 */
static  ADI_PWR_COMMAND_PAIR config_power[] = {
    { ADI_PWR_CMD_SET_PROC_VARIANT,(void*)ADI_PWR_PROC_BF512SBBC1300 }, // 300Mhz ADSP-BF512 variant 
    { ADI_PWR_CMD_SET_PACKAGE,     (void*)ADI_PWR_PACKAGE_MBGA       }, // in MBGA packaging, as on all EZ-KITS
    { ADI_PWR_CMD_SET_VDDEXT,      (void*)ADI_PWR_VDDEXT_330         }, /* external voltage supplied to the voltage regulator is 3.3V */
    { ADI_PWR_CMD_SET_VDDINT,      (void*)ADI_SSL_EXT_VDDINT         }, /* Externally supplied VDDINT (see adi_ssl_Init.h)*/
    { ADI_PWR_CMD_SET_CLKIN,       (void*)16000000                   }, /* 16 MHz */
    { ADI_PWR_CMD_END,              0   /* indicates end of table */ }
    };

static 	ADI_EBIU_TIMING_VALUE     twrmin       = {1,{6000, ADI_EBIU_TIMING_UNIT_PICOSEC}};   // set min TWR to 1 SCLK cycle + 6ns	
static	ADI_EBIU_TIMING_VALUE     refresh      = {4096,{64, ADI_EBIU_TIMING_UNIT_MILLISEC}}; // set refresh period to 4096 cycles in 64ms
static	ADI_EBIU_TIME             trasmin      = {42, ADI_EBIU_TIMING_UNIT_NANOSEC};         // set min TRAS to 42ns
static	ADI_EBIU_TIME             trpmin       = {18, ADI_EBIU_TIMING_UNIT_NANOSEC};	     // set min TRP to 18ns
static	ADI_EBIU_TIME             trcdmin      = {18, ADI_EBIU_TIMING_UNIT_NANOSEC}; 	     // set min TRCD to 18ns
//const   u32                       cl_threshold = 100u;                                        // set cl threshold to 100 Mhz
#define cl_threshold 100u
static  ADI_EBIU_SDRAM_BANK_VALUE bank_size    = {0, {size: ADI_EBIU_SDRAM_BANK_16MB }};      // set bank size to 16MB
static  ADI_EBIU_SDRAM_BANK_VALUE bank_width   = {0, {width: ADI_EBIU_SDRAM_BANK_COL_9BIT}}; // set column address width to 9-Bit

#if !defined(__ADSP_MOY__)  /* This processor does not have asynch memory */
    /* definitions for asynch memory controller commands */

    /*  global control register fields */
    /*  clkout enable */
static  ADI_EBIU_ASYNCH_CLKOUT clkout_enable = ADI_EBIU_ASYNCH_CLKOUT_ENABLE;
    /*  which banks to enable */
static  ADI_EBIU_ASYNCH_BANK_ENABLE banks_enable = ADI_EBIU_ASYNCH_BANK0_1_2_3;

    /* bank timing parameters - specified in cycles */
    /* transition times for the four banks */
static  ADI_EBIU_ASYNCH_BANK_TIMING asynch_bank_trans_time = {ADI_EBIU_BANK_ALL,
         { ADI_EBIU_ASYNCH_TT_4_CYCLES, { 0, ADI_EBIU_TIMING_UNIT_NANOSEC } } };

    /* time between Read Enable assertion to de-assertion */
static  ADI_EBIU_ASYNCH_BANK_TIMING asynch_bank_read_access_time = {
                                    ADI_EBIU_BANK_ALL,
                                    { 0xB, { 0, ADI_EBIU_TIMING_UNIT_NANOSEC } } };

    /* time between Write Enable  assertion to de-assertion */
static  ADI_EBIU_ASYNCH_BANK_TIMING asynch_bank_write_access_time = {
                                 ADI_EBIU_BANK_ALL,
                                 { 7, { 0, ADI_EBIU_TIMING_UNIT_NANOSEC } } };

    /* time from beginning of memory cycle to R/W-enable */
static  ADI_EBIU_ASYNCH_BANK_TIMING asynch_bank_setup_time = { ADI_EBIU_BANK_ALL,
         { ADI_EBIU_ASYNCH_ST_3_CYCLES, { 0, ADI_EBIU_TIMING_UNIT_NANOSEC } } };

    /* time from de-assertion  to end of memory cycle */
static  ADI_EBIU_ASYNCH_BANK_TIMING asynch_bank_hold_time = {ADI_EBIU_BANK_ALL,
         { ADI_EBIU_ASYNCH_HT_2_CYCLES, { 0, ADI_EBIU_TIMING_UNIT_NANOSEC } } };

    /* specify whether ARDY enabled (is used to insert extra wait states) */
static  ADI_EBIU_ASYNCH_BANK_VALUE asynch_bank_ardy_enable = { ADI_EBIU_BANK_ALL,
                              { ardy_enable: ADI_EBIU_ASYNCH_ARDY_DISABLE } };

    /* specify whether ARDY is sampled low or high */
static  ADI_EBIU_ASYNCH_BANK_VALUE asynch_bank_ardy_polarity = { ADI_EBIU_BANK_ALL,
                        { ardy_polarity: ADI_EBIU_ASYNCH_ARDY_POLARITY_LOW } };
#endif

static  ADI_EBIU_COMMAND_PAIR       config_ram[] = {
    { ADI_EBIU_CMD_SET_SDRAM_BANK_SIZE,     (void*)&bank_size   },
    { ADI_EBIU_CMD_SET_SDRAM_BANK_COL_WIDTH,(void*)&bank_width  },
    { ADI_EBIU_CMD_SET_SDRAM_CL_THRESHOLD,  (void*)cl_threshold },
    { ADI_EBIU_CMD_SET_SDRAM_TRASMIN,       (void*)&trasmin     },
    { ADI_EBIU_CMD_SET_SDRAM_TRPMIN,        (void*)&trpmin      },
    { ADI_EBIU_CMD_SET_SDRAM_TRCDMIN,       (void*)&trcdmin     },
    { ADI_EBIU_CMD_SET_SDRAM_TWRMIN,        (void*)&twrmin      },
    { ADI_EBIU_CMD_SET_SDRAM_REFRESH,       (void*)&refresh     },
    /* Asynch Commands memory controller commands */
    { ADI_EBIU_CMD_SET_ASYNCH_CLKOUT_ENABLE,          (void*)&clkout_enable },
    { ADI_EBIU_CMD_SET_ASYNCH_BANK_ENABLE,            (void*)&banks_enable },
    { ADI_EBIU_CMD_SET_ASYNCH_BANK_TRANSITION_TIME,   (void*)&asynch_bank_trans_time },
    { ADI_EBIU_CMD_SET_ASYNCH_BANK_READ_ACCESS_TIME,  (void*)&asynch_bank_read_access_time },
    { ADI_EBIU_CMD_SET_ASYNCH_BANK_WRITE_ACCESS_TIME, (void*)&asynch_bank_write_access_time },
    { ADI_EBIU_CMD_SET_ASYNCH_BANK_SETUP_TIME,    (void*)&asynch_bank_setup_time },
    { ADI_EBIU_CMD_SET_ASYNCH_BANK_HOLD_TIME,     (void*)&asynch_bank_hold_time },
    { ADI_EBIU_CMD_SET_ASYNCH_BANK_ARDY_ENABLE,   (void*)&asynch_bank_ardy_enable },
    { ADI_EBIU_CMD_SET_ASYNCH_BANK_ARDY_POLARITY, (void*)&asynch_bank_ardy_polarity },
    { ADI_EBIU_CMD_END,                       0   /* indicates end of table */ }
};

 /*
 *     Function:       adi_ssl_Init
 *
 *     Description:    Initializes the system services and device manager.
 *                     Initialize everything but exit upon the first error
 */

u32 adi_ssl_Init(void) {

    u32 i;
    u32 Result;

    do {
        // initialize the interrupt manager, parameters are
        //      pointer to memory for interrupt manager to use
        //      memory size (in bytes)
        //      location where the number of secondary handlers that can be
        //          supported will be stored
        //      parameter for adi_int_EnterCriticalRegion
        //          (always NULL for VDK and standalone systems)
        Result = adi_int_Init(InterruptServiceData,
                              sizeof(InterruptServiceData),
                              &i, ADI_SSL_ENTER_CRITICAL);
        while ((ADI_INT_RESULT_SUCCESS != Result) \
            ||(ADI_SSL_INT_NUM_SECONDARY_HANDLERS != i))
        {
        }

       //  initialize the EBIU, parameters are
       //  address of table containing RAM parameters
       //  0 - reserved field, always 0
       //  Ignores result, so it can keep going if it's already initialized
        Result = adi_ebiu_Init(config_ram, 0);
        while ((ADI_EBIU_RESULT_SUCCESS != Result) &&
        (ADI_EBIU_RESULT_ALREADY_INITIALIZED != Result))
        {

        }

        // initialize power, parameters are
        //      address of table containing processor information
        // keep going if it's already initialized
        Result = adi_pwr_Init(config_power);
        while ((ADI_PWR_RESULT_SUCCESS != Result) &&
            (ADI_PWR_RESULT_ALREADY_INITIALIZED != Result))
        {

        }
        
        // U-boot done setFreq
        Result = adi_pwr_SetFreq (256000000, 256000000, ADI_PWR_DF_ON);
        while (ADI_PWR_RESULT_SUCCESS != Result)
        {

        }
        
        // initialize port control, parameters are
        //      parameter for adi_int_EnterCriticalRegion
        //          (always NULL for VDK and standalone systems)
        Result = adi_ports_Init(ADI_SSL_ENTER_CRITICAL);
        while (ADI_PORTS_RESULT_SUCCESS != Result)
        {

        }

        // initialize deferred callback service if needed, parameters are
        //      pointer to data
        //      size of data
        //      location where number of servers is stored
        //      parameter for adi_int_EnterCriticalRegion
        //          (always NULL for VDK and standalone systems)
#if     (ADI_SSL_DCB_NUM_SERVERS != 0)
        Result = adi_dcb_Init(DeferredCallbackServiceData,
                              sizeof(DeferredCallbackServiceData),
                              &i, ADI_SSL_ENTER_CRITICAL);
        while ((ADI_DCB_RESULT_SUCCESS != Result) \
            || (ADI_SSL_DCB_NUM_SERVERS != i))
        {

        }
#endif

        // initialize the dma manager if needed, parameters are
        //      pointer to memory for the DMA manager to use
        //      memory size (in bytes)
        //      parameter for adi_int_EnterCriticalRegion
        //          (always NULL for VDK and standalone systems)
#if     (ADI_SSL_DMA_NUM_CHANNELS != 0)
        Result = adi_dma_Init(DMAServiceData,
                              sizeof(DMAServiceData),
                              &i, &adi_dma_ManagerHandle, ADI_SSL_ENTER_CRITICAL);
        while ((Result != ADI_DMA_RESULT_SUCCESS) \
            || (ADI_SSL_DMA_NUM_CHANNELS != i))
        {

        }
#endif


        // initialize the flag manager, parameters are
        //      pointer to memory for the flag service to use
        //      memory size (in bytes)
        //      location where the number of flag callbacks that can be
        //          supported will be stored
        //      parameter for adi_int_EnterCriticalRegion
        //          (always NULL for VDK and standalone systems)
        Result = adi_flag_Init(FlagServiceData, sizeof(FlagServiceData),
                               &i, ADI_SSL_ENTER_CRITICAL);
        while ((Result != ADI_FLAG_RESULT_SUCCESS) \
            || (ADI_SSL_FLAG_NUM_CALLBACKS != i))
        {

        }

        // initialize the timer manager, parameters are
        //      parameter for adi_int_EnterCriticalRegion
        //          (always NULL for VDK and standalone systems)
        Result = adi_tmr_Init(ADI_SSL_ENTER_CRITICAL);
        while (ADI_TMR_RESULT_SUCCESS != Result)
        {

        }


#if !defined(ADI_SSL_RTC_NO_INIT)
        // initialize the RTC service
        //      parameter for adi_int_EnterCriticalRegion
        //          (always NULL for VDK and standalone systems)
        Result = adi_rtc_Init(ADI_SSL_ENTER_CRITICAL);
        while (ADI_RTC_RESULT_SUCCESS != Result)
        {

        }
#endif

        // initialize the semaphore service if needed, parameters are
        //      pointer to memory for the semaphore service to use
        //      memory size (in bytes)
        //      parameter for adi_int_EnterCriticalRegion
        //          (always NULL for VDK and standalone systems)
#if     (ADI_SSL_SEM_NUM_SEMAPHORES != 0)
        Result = adi_sem_Init(SemaphoreServiceData,
                              sizeof(SemaphoreServiceData),
                              &i, ADI_SSL_ENTER_CRITICAL);
        while ((ADI_SEM_RESULT_SUCCESS != Result) \
            || (ADI_SSL_SEM_NUM_SEMAPHORES != i))
        {

        }
#endif

        // initialize the device manager if needed, parameters are
        //      pointer to data for the device manager to use
        //      size of the data in bytes
        //      location where the number of devices that can be managed
        //          will be stored
        //      location where the device manager handle will be stored
        //      parameter for adi_int_EnterCriticalRegion() function
        //          (always NULL for standalone and VDK)
#if     (ADI_SSL_DEV_NUM_DEVICES != 0)
        Result = adi_dev_Init(DevMgrData, sizeof(DevMgrData),
                              &i,  &adi_dev_ManagerHandle,
                              ADI_SSL_ENTER_CRITICAL);
        while ((Result != ADI_DEV_RESULT_SUCCESS) \
            || (ADI_SSL_DEV_NUM_DEVICES != i))
        {

        }
#endif
     } while (0);     // WHILE (no errors or 1 pass complete)
    return (Result);
}
 /*
 *     Function:       adi_ssl_Terminate
 *
 *     Description:    Terminates the system services and device manager
 *                     Terminate everything but exit upon the first error
 */

u32 adi_ssl_Terminate(void) {

    u32 Result;

    do {
#if     (ADI_SSL_DEV_NUM_DEVICES != 0) // terminate the device manager if needed
        Result = adi_dev_Terminate(adi_dev_ManagerHandle);
        if (Result != ADI_DEV_RESULT_SUCCESS)   { break; }
#endif

#if     (ADI_SSL_SEM_NUM_SEMAPHORES != 0)
        Result = adi_sem_Terminate();        // terminate the semaphore service
        if (Result != ADI_SEM_RESULT_SUCCESS)   { break; }
#endif

#if !defined(ADI_SSL_RTC_NO_INIT)
        Result = adi_rtc_Terminate();           // terminate the RTC service
        if (Result != ADI_RTC_RESULT_SUCCESS)   { break; }
#endif
        Result = adi_tmr_Terminate();           // terminate the timer manager
        if (Result != ADI_TMR_RESULT_SUCCESS)   { break; }

        Result = adi_flag_Terminate();          // terminate the flag manager
        if (Result != ADI_FLAG_RESULT_SUCCESS)  { break; }

#if     (ADI_SSL_DMA_NUM_CHANNELS != 0)   // terminate the dma manager if needed
        Result = adi_dma_Terminate(adi_dma_ManagerHandle);
        if (Result != ADI_DMA_RESULT_SUCCESS)   { break; }
#endif

#if     (ADI_SSL_DCB_NUM_SERVERS != 0)
        Result = adi_dcb_Terminate(); // terminate deferred callback service
        if (Result != ADI_DCB_RESULT_SUCCESS)   { break; }
#endif

        Result = adi_ports_Terminate();         // terminate port control
        if (Result != ADI_PORTS_RESULT_SUCCESS) { break; }

        Result = adi_pwr_Terminate();           // terminate power
        if (Result != ADI_PWR_RESULT_SUCCESS)   { break; }

        Result = adi_ebiu_Terminate();          // terminate the EBIU
        if (Result != ADI_EBIU_RESULT_SUCCESS)  { break; }

        Result = adi_int_Terminate();        // terminate the interrupt manager
        if (Result != ADI_INT_RESULT_SUCCESS)   { break; }

     } while (0);     // WHILE (no errors or 1 pass complete)
    return (Result);
}
