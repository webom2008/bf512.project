/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_sport.c,v $
$Revision: 3578 $
$Date: 2010-10-19 09:36:04 -0400 (Tue, 19 Oct 2010) $

Description:
            This is the driver source code for the SPORT peripheral.

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/


#include <services/services.h>              // system service includes
#include <drivers/adi_dev.h>                // device manager includes
#include <drivers/sport/adi_sport.h>        // sport driver includes


/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_SPORT_NUM_DEVICES   (sizeof(Device)/sizeof(ADI_SPORT))  // number of SPORT devices in the system


/*********************************************************************

Data Structures to manage the device

*********************************************************************/

typedef struct {                                // structure representing the SPORT registers in memory
    ADI_SPORT_TX_CONFIG1_REG    TCR1;               // transmit config register 1
    u16                         Padding0;
    ADI_SPORT_TX_CONFIG2_REG    TCR2;               // transmit config register 2
    u16                         Padding1;
    volatile u16                TCLKDIV;            // transmit clock divide register
    u16                         Padding2;
    volatile u16                TFSDIV;             // transmit frame sync divide register
    u16                         Padding3;
    volatile u16                TX16;               // transmit data register (16 bit)
    u16                         Padding4;
    u16                         Padding5;
    u16                         Padding6;
    volatile u16                RX16;               // receive data register (16 bit)
    u16                         Padding7;
    u16                         Padding8;
    u16                         Padding9;
    ADI_SPORT_RX_CONFIG1_REG    RCR1;               // receive config register 1
    u16                         Padding10;
    ADI_SPORT_RX_CONFIG2_REG    RCR2;               // receive config register 2
    u16                         Padding11;
    volatile u16                RCLKDIV;            // receive clock divide register
    u16                         Padding12;
    volatile u16                RFSDIV;             // receive frame sync divide register
    u16                         Padding13;
    volatile u16                STAT;               // status register
    u16                         Padding14;
    volatile u16                CHNL;               // current channel register
    u16                         Padding15;
    volatile u16                MCMC1;              // multichannel config 1 register
    u16                         Padding16;
    volatile u16                MCMC2;              // multichannel config 2 register
    u16                         Padding17;
    volatile u32                MTCS0;              // multichannel transmit select 0 register
    volatile u32                MTCS1;              // multichannel transmit select 1 register
    volatile u32                MTCS2;              // multichannel transmit select 2 register
    volatile u32                MTCS3;              // multichannel transmit select 3 register
    volatile u32                MRCS0;              // multichannel receive select 0 register
    volatile u32                MRCS1;              // multichannel receive select 1 register
    volatile u32                MRCS2;              // multichannel receive select 2 register
    volatile u32                MRCS3;              // multichannel receive select 3 register
} ADI_SPORT_REGISTERS;

typedef struct {                                // information to manage a specific direction
    ADI_DMA_PMAP                MappingID;          // DMA peripheral mapping ID
    ADI_DCB_HANDLE              DCBHandle;          // callback handle
    ADI_DEV_DEVICE_HANDLE       DeviceHandle;       // device manager handle
    void (*DMCallback) (                            // callback function
        ADI_DEV_DEVICE_HANDLE DeviceHandle,                 // device handle
        u32 Event,                                          // event ID
        void *pArg);                                        // argument pointer
} ADI_SPORT_DIRECTION_INFO;


typedef struct {                            // SPORT device structure
    ADI_SPORT_REGISTERS         *pRegisters;            // base address of SPORT registers
    void                        *pEnterCriticalArg;     // critical region argument
    ADI_DEV_DIRECTION           Direction;              // direction
    u32                         SCLK;                   // SCLK value   (in Hz)
    ADI_INT_PERIPHERAL_ID       ErrorPeripheralID;      // error peripheral ID
    u16                         UsageCount;             // usage count
    u16                         ErrorReportingFlag;     // error reporting flag
    ADI_SPORT_DIRECTION_INFO    InboundInfo;            // inbound device information
    ADI_SPORT_DIRECTION_INFO    OutboundInfo;           // inbound device information

    /* Pin Mux mode for Moy (BF50x processors) */
#if defined(__ADSP_MOY__)

    ADI_SPORT_PIN_MUX_MODE      ePinMuxMode;

#endif

} ADI_SPORT;



/*********************************************************************

Device specific data

*********************************************************************/

/*********************
    Edinburgh
**********************/

#if defined(__ADSP_EDINBURGH__)                 // settings for Edinburgh class devices

static ADI_SPORT Device[] = {                   // Actual SPORT devices
    {                                               // device 0
        (ADI_SPORT_REGISTERS *)0xffc00800,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT0_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT0_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT0_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 1
        (ADI_SPORT_REGISTERS *)0xffc00900,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT1_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT1_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT1_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

};

#endif

/********************
    Braemar
********************/

#if defined(__ADSP_BRAEMAR__)                   // settings for Braemar class devices

static ADI_SPORT Device[] = {                   // Actual SPORT devices
    {                                               // device 0
        (ADI_SPORT_REGISTERS *)0xffc00800,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_PERIPHERAL_ERROR,                   // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT0_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT0_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 1
        (ADI_SPORT_REGISTERS *)0xffc00900,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_PERIPHERAL_ERROR,                   // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT1_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT1_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

};

#endif


/*********************
    Teton-Lite
**********************/

#if defined(__ADSP_TETON__)                     // settings for Teton class devices

static ADI_SPORT Device[] = {                   // Actual SPORT devices
    {                                               // device 0
        (ADI_SPORT_REGISTERS *)0xffc00800,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT0_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT0_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT0_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 1
        (ADI_SPORT_REGISTERS *)0xffc00900,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT1_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT1_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT1_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

};

#endif


/*********************
    Stirling
**********************/

#if defined(__ADSP_STIRLING__)              // settings for Stirling class devices

static ADI_SPORT Device[] = {                   // Actual SPORT devices
    {                                               // device 0
        (ADI_SPORT_REGISTERS *)0xffc00800,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT0_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT0_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT0_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 1
        (ADI_SPORT_REGISTERS *)0xffc00900,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT1_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT1_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT1_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 2
        (ADI_SPORT_REGISTERS *)0xffc02500,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT2_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT2_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT2_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 3
        (ADI_SPORT_REGISTERS *)0xffc02600,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT3_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT3_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT3_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

};

#endif



/*********************
    Moab
**********************/

#if defined(__ADSP_MOAB__)  // settings for Moab class devices

static ADI_SPORT Device[] = {                   // Actual SPORT devices
    {                                               // device 0
        (ADI_SPORT_REGISTERS *)0xffc00800,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT0_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT0_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT0_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 1
        (ADI_SPORT_REGISTERS *)0xffc00900,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT1_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT1_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT1_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 2
        (ADI_SPORT_REGISTERS *)0xffc02500,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT2_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT2_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT2_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 3
        (ADI_SPORT_REGISTERS *)0xffc02600,          // SPORT registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT3_ERROR,                       // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT3_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT3_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

};

#endif      // Moab class devices


/********************
    Kookaburra/Mockingbird
********************/

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

// Note: Kookaburra/Mockingbird has two physical sports (SPORT0 & SPORT1) shared
// between three I/O port mappings (two on PORTF and one on PORTG).
// We handle the second instance of SPORT0 (on POTRTG) as a third, virtual
// SPORT device.  Control register access for this virtual device will
// conflict if users attempt to open SPORTG0 twice on PORTF and PORTG.

static ADI_SPORT Device[] = {                   // Actual SPORT devices
    {                                               // device 0
        (ADI_SPORT_REGISTERS *)0xffc00800,          // SPORT0 registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT0_STATUS,                      // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT0_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT0_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 1
        (ADI_SPORT_REGISTERS *)0xffc00900,          // SPORT1 registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT1_STATUS,                  // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT1_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT1_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },

    {                                               // device 3
        (ADI_SPORT_REGISTERS *)0xffc00800,          // SPORT0 registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT0_STATUS,                      // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT0_RX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT0_TX,                     // DMA peripheral mapping ID
            NULL,                                       // callback handle
            NULL,                                       // device manager handle
            NULL,                                       // device manager callback function
        },
    },


};

#endif  // Kookaburra/Mockingbird class devices


/********************
    Brodie
********************/

#if defined(__ADSP_BRODIE__)

static ADI_SPORT Device[] = {                   // Actual SPORT devices
    {                                               // device 0
        (ADI_SPORT_REGISTERS *)0xffc00800,          // SPORT0 registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT0_STATUS,                      // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT0_RX,                 // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT0_TX_AND_RSI,         // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
    },

    {                                               // device 1
        (ADI_SPORT_REGISTERS *)0xffc00900,          // SPORT1 registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT1_STATUS,                      // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT1_RX_AND_SPI1,        // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT1_TX,                 // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
    },

};

#endif  // Brodie class devices

/********************
ADSP-BF50x (Moy)
********************/

#if defined(__ADSP_MOY__)

/*
** Note:
**  Moy has two physical sports (SPORT0 & SPORT1)
**  SPORT 1 offers the option to use PG4 or PG8 to operate as
**  Secondary Receive channel (DRSEC). SPORT device instance 1 should be
**  used if PG4 should act as DRSEC. SPORT device instance 2, a virtual device,
**  should be used when PG8 should act as DRSEC.
*/


/* Actual SPORT devices */
static ADI_SPORT Device[] =
{
    /* SPORT 0 */
    {
        (ADI_SPORT_REGISTERS *)0xFFC00800,          // SPORT0 registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT0_STATUS,                      // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT0_RX,                 // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT0_TX,                 // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
        ADI_SPORT_PIN_MUX_MODE_0,                   // Pin Mux mode not required for SPORT 0
    },

    /* SPORT 1 */
    {
        (ADI_SPORT_REGISTERS *)0xFFC00900,          // SPORT1 registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT1_STATUS,                      // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT1_RX,                 // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT1_TX,                 // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
        ADI_SPORT_PIN_MUX_MODE_0,                   // By default, use PG4 as Secondary Rx channel (DR1SEC)
    },

};

#endif  /* __ADSP_MOY__ */

/********************
    Delta
********************/

#if defined(__ADSP_DELTA__)

static ADI_SPORT Device[] = {                   // Actual SPORT devices
    {                                               // device 0
        (ADI_SPORT_REGISTERS *)0xffc00800,          // SPORT0 registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT0_STATUS,                      // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT0_RX,                 // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT0_TX,                 // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
    },

    {                                               // device 1
        (ADI_SPORT_REGISTERS *)0xffc00900,          // SPORT1 registers base address
        NULL,                                       // critical region argument
        ADI_DEV_DIRECTION_UNDEFINED,                // direction
        0,                                          // SCLK value
        ADI_INT_SPORT1_STATUS,                      // interrupt ID
        0,                                          // usage count
        FALSE,                                      // error reporting flag
        {                                           // inbound device info
            ADI_DMA_PMAP_SPORT1_RX,                 // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
        {                                           // outbound device info
            ADI_DMA_PMAP_SPORT1_TX,                 // DMA peripheral mapping ID
            NULL,                                   // callback handle
            NULL,                                   // device manager handle
            NULL,                                   // device manager callback function
        },
    },

};

#endif  // Delta class devices

/*********************************************************************

Control register reset data

*********************************************************************/

static u32 InboundResetCommands[] = {   // commands that reset all inbound SPORT parameters
    ADI_SPORT_CMD_SET_RCR1,
    ADI_SPORT_CMD_SET_RCR2,
    ADI_SPORT_CMD_SET_RCLKDIV,
    ADI_SPORT_CMD_SET_RFSDIV,
    ADI_SPORT_CMD_SET_MRCS0,
    ADI_SPORT_CMD_SET_MRCS1,
    ADI_SPORT_CMD_SET_MRCS2,
    ADI_SPORT_CMD_SET_MRCS3,
    ADI_SPORT_CMD_SET_MCMC1,
    ADI_SPORT_CMD_SET_MCMC2,
    ADI_SPORT_CMD_CLEAR_RX_ERRORS,
};

static u32 OutboundResetCommands[] = {  // commands that reset all outbound SPORT parameters
    ADI_SPORT_CMD_SET_TCR1,
    ADI_SPORT_CMD_SET_TCR2,
    ADI_SPORT_CMD_SET_TCLKDIV,
    ADI_SPORT_CMD_SET_TFSDIV,
    ADI_SPORT_CMD_SET_MTCS0,
    ADI_SPORT_CMD_SET_MTCS1,
    ADI_SPORT_CMD_SET_MTCS2,
    ADI_SPORT_CMD_SET_MTCS3,
    ADI_SPORT_CMD_SET_MCMC1,
    ADI_SPORT_CMD_SET_MCMC2,
    ADI_SPORT_CMD_CLEAR_TX_ERRORS,
};

/*********************************************************************

Tables for individual control/status register bit field settings

*********************************************************************/

typedef struct {        // structure to access bit fields in control registers
    u16 Mask;               // mask to access the field
    u16 StartBit;           // starting bit of the field within the register
} ADI_SPORT_REGISTER_FIELDS;


// the information in table below must match exactly the order of the command IDs in
// adi_sport.h.  This table is indexed with the command ID so any changes in either the
// command IDs or the table below must be reflected in the other.

static ADI_SPORT_REGISTER_FIELDS RegisterFields[] = {
    // TCR1 fields
    {   0x1,    1   },  // clock source
    {   0x3,    2   },  // data format
    {   0x1,    4   },  // bit order
    {   0x1,    9   },  // frame sync source
    {   0x1,    10  },  // frame sync required
    {   0x1,    11  },  // data based fs generation
    {   0x1,    12  },  // frame sync polarity
    {   0x1,    13  },  // frame sync timing
    {   0x1,    14  },  // edge selection

    // TCR2 fields
    {   0x1f,   0   },  // word length
    {   0x1,    8   },  // secondary TX channel
    {   0x1,    9   },  // stereo frame sync
    {   0x1,    10  },  // channel order

    // RCR1 fields
    {   0x1,    1   },  // clock source
    {   0x3,    2   },  // data format
    {   0x1,    4   },  // bit order
    {   0x1,    9   },  // frame sync source
    {   0x1,    10  },  // frame sync required
    {   0x1,    12  },  // frame sync polarity
    {   0x1,    13  },  // frame sync timing
    {   0x1,    14  },  // edge selection

    // RCR2 fields
    {   0x1f,   0   },  // word length
    {   0x1,    8   },  // secondary TX channel
    {   0x1,    9   },  // stereo frame sync
    {   0x1,    10  },  // channel order

    // MCMC1 fields
    {   0x3ff,  0   },  // window offset
    {   0xf,    12  },  // window size

    // MCMC2 fields
    {   0x3,    0   },  // 2X clock recovery mode
    {   0x1,    2   },  // transmit packing
    {   0x1,    3   },  // receive packing
    {   0x1,    4   },  // multichannel mode
    {   0x1,    7   },  // frame sync to data relationship
    {   0xf,    12  },  // delay between fs and first data bit

};

/*********************************************************************

Static functions

*********************************************************************/


static u32 adi_pdd_Open(                        // Open a device
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          // device manager handle
    u32                     DeviceNumber,           // device number
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           // device handle
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            // pointer to PDD handle location
    ADI_DEV_DIRECTION       Direction,              // data direction
    void                    *pCriticalRegionArg,    // critical region imask storage location
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              // handle to the DMA manager
    ADI_DCB_HANDLE          DCBHandle,              // callback handle
    ADI_DCB_CALLBACK_FN     DMCallback              // device manager callback function
);

static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle    // PDD handle
);

static u32 adi_pdd_Read(        // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
);

static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
);

static u32 adi_pdd_SequentialIO(// Reads and/or writes data in a sequential fashion
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
);

static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    u32 Command,                    // command ID
    void *Value                     // command specific value
);

static ADI_INT_HANDLER(ErrorHandler);       // interrupt handler for errors

static ADI_DEV_DIRECTION    FindDevice(     // finds a device and direction given a PDD handle
    ADI_DEV_PDD_HANDLE PDDHandle,               // handle to the device
    ADI_SPORT           **pDevice               // location where pointer to actual device will be stored
);

#if defined(__ADSP_BRAEMAR__)       // static functions for Braemar

static u32 sportSetPortControl(     // configures the port control registers
    ADI_SPORT               *pDevice,       // pointer to the device
    ADI_DEV_DIRECTION       Direction       // direction
);

#endif


#if defined(__ADSP_STIRLING__)          // static functions for Stirling

static u32 sportSetPortControl(     // configures the port control registers
    ADI_SPORT               *pDevice,       // pointer to the device
    ADI_DEV_DIRECTION       Direction       // direction
);

#endif

#if defined(__ADSP_MOAB__)              /* static functions for Moab */

static u32 sportSetPortControl(         /* configures the port control registers    */
    ADI_SPORT               *pDevice,       /* pointer to the device we're working on   */
    ADI_DEV_DIRECTION       Direction       /* direction */
);

#endif


#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__)   // static functions for Kookaburra/Mockingbird

static u32 sportSetPortControl(         // configures the port control registers
    ADI_SPORT               *pDevice,       // pointer to the device
    ADI_DEV_DIRECTION       Direction       // direction
);

#endif

#if defined(__ADSP_MOY__) || defined(__ADSP_DELTA__)          // static functions for Moy

static u32 sportSetPortControl(     // configures the port control registers
    ADI_SPORT               *pDevice,       // pointer to the device
    ADI_DEV_DIRECTION       Direction       // direction
);

#endif

#if defined(ADI_DEV_DEBUG)

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);

#endif


/*********************************************************************

Entry point for device manager

*********************************************************************/


ADI_DEV_PDD_ENTRY_POINT ADISPORTEntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO,
};



/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens a SPORT device for use
*
*********************************************************************/


static u32 adi_pdd_Open(                        // Open a device
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          // device manager handle
    u32                     DeviceNumber,           // device number
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           // device handle
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            // pointer to PDD handle location
    ADI_DEV_DIRECTION       Direction,              // data direction
    void                    *pEnterCriticalArg,     // enter critical region parameter
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              // handle to the DMA manager
    ADI_DCB_HANDLE          DCBHandle,              // callback handle
    ADI_DCB_CALLBACK_FN     DMCallback              // client callback function
) {

    ADI_INT_IMASK   IMask;                  // temporary storage of IMask register
    u32             Result;                 // return value
    ADI_SPORT       *pDevice;               // pointer to the device we're working on
    void            *pExitCriticalArg;      // exit critical region parameter
    u32             i;                      // counter
    u32             SCLK;                   // SCLK value from power service

    // check for errors if required
#ifdef ADI_DEV_DEBUG
    if (DeviceNumber >= ADI_SPORT_NUM_DEVICES) {        // check the device number
        return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
    }
#endif

    // get SCLK (now returns in values in Hz!)
    if ((Result = adi_pwr_GetFreq(&i, &SCLK, &i)) != ADI_PWR_RESULT_SUCCESS) {
        return (Result);
    }

    // point to the device that is being opened
    pDevice = &Device[DeviceNumber];

    // assume the worst
    Result = ADI_DEV_RESULT_DEVICE_IN_USE;

    // protect this region
    pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);

    // IF (the device is already in use and is either opened for bidirectional data or
    //      already operating in the mode the caller wants)
    if ((pDevice->UsageCount) && ((pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL) || (pDevice->Direction == Direction))) {

        // there's nothing we can do for the caller

    // ELSE
    } else {

        // the caller can use the device

        // update the device direction
        if (pDevice->UsageCount == 0) {
            pDevice->Direction = Direction;
        } else {
            pDevice->Direction = ADI_DEV_DIRECTION_BIDIRECTIONAL;
        }

        // mark the device as in use and successfully opened
        pDevice->UsageCount++;
        Result = ADI_DEV_RESULT_SUCCESS;

        // IF (caller wants inbound or bidirectional data)
        if ((Direction == ADI_DEV_DIRECTION_INBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

            // save the inbound info
            pDevice->InboundInfo.DCBHandle = DCBHandle;
            pDevice->InboundInfo.DeviceHandle = DeviceHandle;
            pDevice->InboundInfo.DMCallback = DMCallback;

        // ENDIF
        }

        // IF (caller wants outbound or bidirectional data)
        if ((Direction == ADI_DEV_DIRECTION_OUTBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

            // save the outbound info
            pDevice->OutboundInfo.DCBHandle = DCBHandle;
            pDevice->OutboundInfo.DeviceHandle = DeviceHandle;
            pDevice->OutboundInfo.DMCallback = DMCallback;

        // ENDIF
        }

    // ENDIF
    }

    // unprotect
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // return an error if we can't open the device
    if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);

    // reset the actual device by setting all appropriate registers to their reset value
    if ((Direction == ADI_DEV_DIRECTION_INBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {
        for (i = 0; i < sizeof(InboundResetCommands)/sizeof(InboundResetCommands[0]); i++) {
            adi_pdd_Control(pDevice, InboundResetCommands[i], (void *)0);
        }
    }
    if ((Direction == ADI_DEV_DIRECTION_OUTBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {
        for (i = 0; i < sizeof(OutboundResetCommands)/sizeof(OutboundResetCommands[0]); i++) {
            adi_pdd_Control(pDevice, OutboundResetCommands[i], (void *)0);
        }
    }

    // initialize the remaining device settings
    pDevice->SCLK = SCLK;
    pDevice->pEnterCriticalArg = pEnterCriticalArg;
    pDevice->ErrorReportingFlag = FALSE;

    // as the handle to the PDD, give the caller the following:
    //      - if opened for bidirectional data, give him the address of the device structure
    //      - if opened for inbound only, give him the address of the inbound info
    //      - if opened for outbound only, give him the address of the outbound info
    switch (Direction) {
        case ADI_DEV_DIRECTION_INBOUND:
            *pPDDHandle = (ADI_DEV_PDD_HANDLE *)&pDevice->InboundInfo;
            break;
        case ADI_DEV_DIRECTION_OUTBOUND:
            *pPDDHandle = (ADI_DEV_PDD_HANDLE *)&pDevice->OutboundInfo;
            break;
        case ADI_DEV_DIRECTION_BIDIRECTIONAL:
            *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;
            break;
    }

    // return
    return(ADI_DEV_RESULT_SUCCESS);
}






/*********************************************************************
*
*   Function:       adi_pdd_Close
*
*   Description:    Closes down a SPORT device
*
*********************************************************************/


static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle    // PDD handle
) {

    u32                 Result;             // return value
    ADI_DEV_DIRECTION   Direction;          // direction
    ADI_SPORT           *pDevice;           // pointer to the device we're working on
    void                *pExitCriticalArg;  // exit critical region parameter
    u32                 ErrorIVG;           // IVG for SPORT error interrupts


    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    // find the device
    Direction = FindDevice(PDDHandle, &pDevice);

    // disable dataflow on the device
    if ((Result = adi_pdd_Control (PDDHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)FALSE)) != ADI_DEV_RESULT_SUCCESS) {
        return (Result);
    }

    // turn off error reporting if this is going to completely shut down the device
    if (pDevice->UsageCount == 1) {
        if ((Result = adi_pdd_Control(PDDHandle, ADI_DEV_CMD_SET_ERROR_REPORTING, (void *)FALSE)) != ADI_DEV_RESULT_SUCCESS) {
            return (Result);
        }
    }

    // protect this region
    pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

    // decrement the usage count
    pDevice->UsageCount--;

    // IF (there is still another active handle into the device)
    if (pDevice->UsageCount) {

        // update the device so that only the other handle is open
        pDevice->Direction = ((Direction == ADI_DEV_DIRECTION_INBOUND)?ADI_DEV_DIRECTION_OUTBOUND:ADI_DEV_DIRECTION_INBOUND);

    // ENDIF
    }

    // unprotect
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // return
    return(Result);
}





/*********************************************************************
*
*   Function:       adi_pdd_Read
*
*   Description:    Never called as SPORT uses DMA
*
*********************************************************************/


static u32 adi_pdd_Read(        // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER      *pBuffer    // pointer to buffer
){

    // this device uses processor DMA so we should never get here
    return(ADI_DEV_RESULT_FAILED);
}





/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Never called as SPORT uses DMA
*
*********************************************************************/


static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER      *pBuffer    // pointer to buffer
){

    // this device uses processor DMA so we should never get here
    return(ADI_DEV_RESULT_FAILED);
}





/*********************************************************************
*
*   Function:       adi_pdd_SequentialIO
*
*   Description:    Never called as we don't support sequential IO
*
*********************************************************************/


static u32 adi_pdd_SequentialIO(    // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER      *pBuffer    // pointer to buffer
){

    // not supported
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}



/*********************************************************************
*
*   Function:       adi_pdd_Control
*
*   Description:    Configures the SPORT device
*
*********************************************************************/


static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    u32                 Command,    // command ID
    void                *Value      // command specific value
) {

    ADI_SPORT                   *pDevice;       // pointer to the device we're working on
    u16                         tmp16;          // temporary storage
    u32                         tmp32;          // temporary storage
    u32                         Result;         // return value
    u32                         ErrorIVG;       // IVG for SPORT error interrupts
    u32                         u32Value;       // u32 type to avoid casts/warnings etc.
    u16                         u16Value;       // u16 type to avoid casts/warnings etc.
    volatile u16                *p16;           // pointer to 16 bit register
    volatile u32                *p32;           // pointer to 32 bit register
    ADI_SPORT_REGISTER_FIELDS   *pFields;       // pointer into fields array
    ADI_DEV_DIRECTION           Direction;      // direction
    void                        *pExitCriticalArg;  // exit critical region parameter

    // avoid casts
    u32Value = (u32)Value;
    u16Value = ((u16)((u32)Value));

    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    // assume we're going to be successful
    Result = ADI_DEV_RESULT_SUCCESS;

    // find the device
    Direction = FindDevice(PDDHandle, &pDevice);

    // CASEOF (Command ID)
    switch (Command) {

        // CASE (control dataflow)
        case (ADI_DEV_CMD_SET_DATAFLOW):

            // set port control if we're a Braemar class device
#if defined(__ADSP_BRAEMAR__)
            if (ADI_DEV_RESULT_SUCCESS != (Result = sportSetPortControl(pDevice, Direction)))
                break;  // stop on sportSetPortControl failure
#endif

            // set port control if we're a Stirling class device
#if defined(__ADSP_STIRLING__)
            if (ADI_DEV_RESULT_SUCCESS != (Result = sportSetPortControl(pDevice, Direction)))
                break;  // stop on sportSetPortControl failure
#endif

            /* set port control if we're a Moab class device */
#if defined(__ADSP_MOAB__)
            if (ADI_DEV_RESULT_SUCCESS != (Result = sportSetPortControl(pDevice, Direction)))
                break;  // stop on sportSetPortControl failure
#endif

            /* set port control if we're a Kookaburra/Mockingbird/Brodie class device */
#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__)
            if (ADI_DEV_RESULT_SUCCESS != (Result = sportSetPortControl(pDevice, Direction)))
                break;  // stop on sportSetPortControl failure
#endif

            /* set port control if we're a Moy class device */
#if defined(__ADSP_MOY__) || defined(__ADSP_DELTA__)
            if (ADI_DEV_RESULT_SUCCESS != (Result = sportSetPortControl(pDevice, Direction)))
                break;  // stop on sportSetPortControl failure
#endif

            // IF (opened for outbound or bidirectional)
            if ((Direction == ADI_DEV_DIRECTION_OUTBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

                // enable or disable Tx accordingly
                pDevice->pRegisters->TCR1.tspen = u16Value;
            }
            // ENDIF

            // IF (opened for inbound or bidirectional)
            if ((Direction == ADI_DEV_DIRECTION_INBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

                // enable or disable Rx accordingly
                pDevice->pRegisters->RCR1.rspen = u16Value;
            }

            break;
            // ENDIF

        // CASE (query for processor DMA support)
        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

            // yes, we do support it
            *((u32 *)Value) = TRUE;
            break;

        // CASE (query for our inbound DMA peripheral ID)
        case (ADI_DEV_CMD_GET_INBOUND_DMA_PMAP_ID):

            // tell him what it is
            *((ADI_DMA_PMAP *)Value) = pDevice->InboundInfo.MappingID;
            break;

        // CASE (query for our outbound DMA peripheral ID)
        case (ADI_DEV_CMD_GET_OUTBOUND_DMA_PMAP_ID):

            // tell him what it is
            *((ADI_DMA_PMAP *)Value) = pDevice->OutboundInfo.MappingID;
            break;

        // CASE ( error reporting)
        case (ADI_DEV_CMD_SET_ERROR_REPORTING):

            // don't do anything if nothing is being changed
            if (((u32)Value) == pDevice->ErrorReportingFlag) {
                break;
            }

            // get the error interrupt IVG
            adi_int_SICGetIVG(pDevice->ErrorPeripheralID, &ErrorIVG);

            // IF (enabling)
            if (((u32)Value) == TRUE) {

                // hook the SPORT error interrupt handler into the system
                if(adi_int_CECHook(ErrorIVG, ErrorHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS){
                    return (ADI_DEV_RESULT_CANT_HOOK_INTERRUPT);
                }
                adi_int_SICWakeup(pDevice->ErrorPeripheralID, TRUE);
                adi_int_SICEnable(pDevice->ErrorPeripheralID);

            // ELSE
            } else {

                // unhook the SPORT error interrupt handler from the system
                adi_int_SICDisable(pDevice->ErrorPeripheralID);
                adi_int_SICWakeup(pDevice->ErrorPeripheralID, FALSE);
                if (adi_int_CECUnhook(ErrorIVG, ErrorHandler, pDevice) != ADI_INT_RESULT_SUCCESS) {
                    return(ADI_DEV_RESULT_CANT_UNHOOK_INTERRUPT);
                }

            // ENDIF
            }

            // update status
            pDevice->ErrorReportingFlag = ((u32)Value);
            break;

        // CASE (set a 16 bit register)
        case ADI_SPORT_CMD_SET_TCR1:
        case ADI_SPORT_CMD_SET_TCR2:
        case ADI_SPORT_CMD_SET_TCLKDIV:
        case ADI_SPORT_CMD_SET_TFSDIV:
        case ADI_SPORT_CMD_SET_TX16:
        case ADI_SPORT_CMD_SET_RCR1:
        case ADI_SPORT_CMD_SET_RCR2:
        case ADI_SPORT_CMD_SET_RCLKDIV:
        case ADI_SPORT_CMD_SET_RFSDIV:
        case ADI_SPORT_CMD_SET_MCMC1:
        case ADI_SPORT_CMD_SET_MCMC2:

            // we'll get an exception if trying to access TX16 and the slen is > 16 bits
#if defined(ADI_DEV_DEBUG)
            if ((Command == ADI_SPORT_CMD_SET_TX16) && (pDevice->pRegisters->TCR2.slen > 15)) {
                return (ADI_SPORT_RESULT_BAD_ACCESS_WIDTH);
            }
#endif

            // set it
            p16 = (volatile u16 *)pDevice->pRegisters + ((Command - ADI_SPORT_CMD_SET_TCR1) << 1);
            *p16 = u16Value;
            break;

        // CASE (get a 16 bit register)
        case ADI_SPORT_CMD_GET_STAT:
        case ADI_SPORT_CMD_GET_CHNL:
        case ADI_SPORT_CMD_GET_RX16:

            // we'll get an exception if trying to access RX16 and the slen is > 16 bits
#if defined(ADI_DEV_DEBUG)
            if ((Command == ADI_SPORT_CMD_GET_RX16) && (pDevice->pRegisters->RCR2.slen > 15)) {
                return (ADI_SPORT_RESULT_BAD_ACCESS_WIDTH);
            }
#endif

            // get it
            p16 = (volatile u16 *)pDevice->pRegisters + ((Command - ADI_SPORT_CMD_SET_TCR1) << 1);
            (*(u16 *)Value) = *p16;
            break;

        // CASE (set a 32 bit register)
        case ADI_SPORT_CMD_SET_MTCS0:
        case ADI_SPORT_CMD_SET_MTCS1:
        case ADI_SPORT_CMD_SET_MTCS2:
        case ADI_SPORT_CMD_SET_MTCS3:
        case ADI_SPORT_CMD_SET_MRCS0:
        case ADI_SPORT_CMD_SET_MRCS1:
        case ADI_SPORT_CMD_SET_MRCS2:
        case ADI_SPORT_CMD_SET_MRCS3:

            // set it
            p16 = (volatile u16 *)pDevice->pRegisters + ((Command - ADI_SPORT_CMD_SET_TCR1) << 1);
            *((u32 *)p16) = u32Value;
            break;

        // CASE (set TX32 data register)
        case ADI_SPORT_CMD_SET_TX32:

            // we'll get an exception if trying to access and the slen is < 16 bits
#if defined(ADI_DEV_DEBUG)
            if (pDevice->pRegisters->TCR2.slen < 16) {
                return (ADI_SPORT_RESULT_BAD_ACCESS_WIDTH);
            }
#endif

            // set it
            *((u32 *)&pDevice->pRegisters->TX16) = u32Value;
            break;

        // CASE (get RX32 data register)
        case ADI_SPORT_CMD_GET_RX32:

            // we'll get an exception if trying to access and the slen is < 16 bits
#if defined(ADI_DEV_DEBUG)
            if (pDevice->pRegisters->RCR2.slen < 16) {
                return (ADI_SPORT_RESULT_BAD_ACCESS_WIDTH);
            }
#endif

            // get it
            (*(u32 *)Value) = *((u32 *)&pDevice->pRegisters->RX16);
            break;

        // CASE (clear transmit errors)
        case ADI_SPORT_CMD_CLEAR_TX_ERRORS:

            // clear the appropriate bits
            pDevice->pRegisters->STAT = 0x0030;
            break;

        // CASE (clear receive errors)
        case ADI_SPORT_CMD_CLEAR_RX_ERRORS:

            // clear the appropriate bits
            pDevice->pRegisters->STAT = 0x0006;
            break;

        // CASE (set the transmit clock)
        case (ADI_SPORT_CMD_SET_TX_CLOCK_FREQ):

            // update the clock divider register based on the system clock and return
            //SPORTx_TCLK = (SCLK)/(2 x (SPORTx_TCLKDIV + 1))
            pDevice->pRegisters->TCLKDIV = (pDevice->SCLK / (2 * u32Value)) - 1;
            ssync();
            break;

        // CASE (set the transmit frame sync clock)
        case (ADI_SPORT_CMD_SET_TX_FRAME_SYNC_FREQ):

            // update the frame sync divider register based on the SPORT clock and return
            // SPORTx_TFS frequency = (SPORTx_TCLK)/(SPORTx_TFSDIV + 1)
            // SPORTx_TFSDIV = ((SPORTx_TCLK/SPORTx_TFS Frequency) - 1), where
            // SPORTx_TCLK = (SCLK)/(2 x (SPORTx_TCLKDIV + 1))
            pDevice->pRegisters->TFSDIV=((pDevice->SCLK/(2*(pDevice->pRegisters->TCLKDIV+1)))/u32Value)-1;
            ssync();
            break;

        // CASE (set the receive clock)
        case (ADI_SPORT_CMD_SET_RX_CLOCK_FREQ):

            // update the clock divider register based on the system clock and return
            //SPORTx_RCLK = (SCLK)/(2 x (SPORTx_RCLKDIV + 1))
            pDevice->pRegisters->RCLKDIV = (pDevice->SCLK / (2 * u32Value)) - 1;
            ssync();
            break;

        // CASE (set the receive frame sync clock)
        case (ADI_SPORT_CMD_SET_RX_FRAME_SYNC_FREQ):

            // update the frame sync divider register based on the SPORT clock and return
            // SPORTx_RFS frequency = (SPORTx_RCLK)/(SPORTx_RFSDIV + 1)
            // SPORTx_RFSDIV = ((SPORTx_RCLK/SPORTx_RFS Frequency) - 1), where
            // SPORTx_RCLK = (SCLK)/(2 x (SPORTx_RCLKDIV + 1))
            pDevice->pRegisters->RFSDIV = ((pDevice->SCLK/(2*(pDevice->pRegisters->RCLKDIV+1)))/u32Value)-1;
            ssync();
            break;

        // CASE (various field controls)
        case ADI_SPORT_CMD_SET_TX_CLOCK_SOURCE:
        case ADI_SPORT_CMD_SET_TX_DATA_FORMAT:
        case ADI_SPORT_CMD_SET_TX_BIT_ORDER:
        case ADI_SPORT_CMD_SET_TX_FS_SOURCE:
        case ADI_SPORT_CMD_SET_TX_FS_REQUIREMENT:
        case ADI_SPORT_CMD_SET_TX_FS_DATA_GEN:
        case ADI_SPORT_CMD_SET_TX_FS_POLARITY:
        case ADI_SPORT_CMD_SET_TX_FS_TIMING:
        case ADI_SPORT_CMD_SET_TX_EDGE_SELECT:

        case ADI_SPORT_CMD_SET_TX_WORD_LENGTH:
        case ADI_SPORT_CMD_SET_TX_SECONDARY_ENABLE:
        case ADI_SPORT_CMD_SET_TX_STEREO_FS_ENABLE:
        case ADI_SPORT_CMD_SET_TX_LEFT_RIGHT_ORDER:

        case ADI_SPORT_CMD_SET_RX_CLOCK_SOURCE:
        case ADI_SPORT_CMD_SET_RX_DATA_FORMAT:
        case ADI_SPORT_CMD_SET_RX_BIT_ORDER:
        case ADI_SPORT_CMD_SET_RX_FS_SOURCE:
        case ADI_SPORT_CMD_SET_RX_FS_REQUIREMENT:
        case ADI_SPORT_CMD_SET_RX_FS_POLARITY:
        case ADI_SPORT_CMD_SET_RX_FS_TIMING:
        case ADI_SPORT_CMD_SET_RX_EDGE_SELECT:

        case ADI_SPORT_CMD_SET_RX_WORD_LENGTH:
        case ADI_SPORT_CMD_SET_RX_SECONDARY_ENABLE:
        case ADI_SPORT_CMD_SET_RX_STEREO_FS_ENABLE:
        case ADI_SPORT_CMD_SET_RX_LEFT_RIGHT_ORDER:

        case ADI_SPORT_CMD_SET_MC_WINDOW_OFFSET:
        case ADI_SPORT_CMD_SET_MC_WINDOW_SIZE:

        case ADI_SPORT_CMD_SET_MC_CLOCK_RECOVERY_MODE:
        case ADI_SPORT_CMD_SET_MC_TRANSMIT_PACKING:
        case ADI_SPORT_CMD_SET_MC_RECEIVE_PACKING:
        case ADI_SPORT_CMD_SET_MC_MODE:
        case ADI_SPORT_CMD_SET_MC_FS_TO_DATA:
        case ADI_SPORT_CMD_SET_MC_FRAME_DELAY:

            // determine the register to address and point to the entry in the fields table
            if (Command <= ADI_SPORT_CMD_SET_TX_EDGE_SELECT) {
                p16 = (volatile u16 *)&pDevice->pRegisters->TCR1;
            } else {
                if (Command <= ADI_SPORT_CMD_SET_TX_LEFT_RIGHT_ORDER) {
                    p16 = (volatile u16 *)&pDevice->pRegisters->TCR2;
                } else {
                    if (Command <= ADI_SPORT_CMD_SET_RX_EDGE_SELECT) {
                        p16 = (volatile u16 *)&pDevice->pRegisters->RCR1;
                    } else {
                        if (Command <= ADI_SPORT_CMD_SET_RX_LEFT_RIGHT_ORDER) {
                            p16 = (volatile u16 *)&pDevice->pRegisters->RCR2;
                        } else {
                            if (Command <= ADI_SPORT_CMD_SET_MC_WINDOW_SIZE) {
                                p16 = (volatile u16 *)&pDevice->pRegisters->MCMC1;
                            } else {
                                p16 = (volatile u16 *)&pDevice->pRegisters->MCMC2;
                            }
                        }
                    }
                }
            }
            pFields = &RegisterFields[Command - ADI_SPORT_CMD_SET_TX_CLOCK_SOURCE];

            // set the value in the register
            tmp16 = *p16;
            tmp16 &= ~(pFields->Mask << pFields->StartBit);
            tmp16 |= ((u16Value & pFields->Mask) << pFields->StartBit);
            *p16 = tmp16;
            break;

        // CASE (enabling/disabling a channel in multichannel mode)
        case ADI_SPORT_CMD_MC_TX_CHANNEL_ENABLE:
        case ADI_SPORT_CMD_MC_TX_CHANNEL_DISABLE:
        case ADI_SPORT_CMD_MC_RX_CHANNEL_ENABLE:
        case ADI_SPORT_CMD_MC_RX_CHANNEL_DISABLE:

            // point to the register we're going to change
            if ((Command == ADI_SPORT_CMD_MC_TX_CHANNEL_ENABLE) || (Command == ADI_SPORT_CMD_MC_TX_CHANNEL_DISABLE)) {
                p32 = &pDevice->pRegisters->MTCS0;
            } else {
                p32 = &pDevice->pRegisters->MRCS0;
            }
            p32 += (u16Value >> 5);

            // read it in
            tmp32 = *p32;

            // set or clear the appropriate bit
            if ((Command == ADI_SPORT_CMD_MC_TX_CHANNEL_ENABLE) || (Command == ADI_SPORT_CMD_MC_RX_CHANNEL_ENABLE)) {
                tmp32 |= (1 << (0x1f & u16Value));
            } else {
                tmp32 &= ~(1 << (0x1f & u16Value));
            }

            // update the register
            *p32 = tmp32;
            break;

        /* CASE (Set Pin Mux Mode) */
        case (ADI_SPORT_CMD_SET_PIN_MUX_MODE):

/* For BF50x Family */
#if defined (__ADSP_MOY__)
            /* Update pin mux mode selection */
            pDevice->ePinMuxMode = ((ADI_SPORT_PIN_MUX_MODE) Value);
#endif

            break;
			
/* For BF53x and BF561 families */		
#if defined(__ADSP_EDINBURGH__) || defined(__ADSP_BRAEMAR__) || defined(__ADSP_TETON__)

        /* CASE (Enable/Disable SPORT Hysteresys) */
        case (ADI_SPORT_CMD_SET_HYSTERESIS_ENABLE):
        
            // protect this region
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // get the PLL_CTL register value
            tmp16 = *pPLL_CTL;
			
            // set or clear bit 15 in the PLL_CTL register
            if (u16Value) {
	            tmp16 |= (SPORT_HYS);
            } else {
	            tmp16 &= ~(SPORT_HYS);
            }

            // update the register
            *pPLL_CTL = tmp16;

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);

            break;
#endif
            
        // CASEELSE
        default:

            // we don't understand this command
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;

    // ENDCASE
    }

    // return
    return(Result);
}




/*********************************************************************

    Function:       ErrorHandler

    Description:    Processes SPORT error interrupts

*********************************************************************/

static ADI_INT_HANDLER(ErrorHandler)    // SPORT error handler
{

    ADI_SPORT_DIRECTION_INFO    *pDirectionInfo;    // direction info
    ADI_SPORT                   *pDevice;           // pointer to the device we're working on
    u16                         SPORTStatus;        // sport status register
    ADI_INT_HANDLER_RESULT      Result;             // result
    u32                         ProcessInboundFlag; // device processes inbound events flag
    u32                         ProcessOutboundFlag;// device processes outbound events flag

    // assume the interrupt is not for us
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    // get the pointer to the device
    pDevice = (ADI_SPORT *)ClientArg;

    // point to the direction info that we can use so that we can get the callback type and device handle that will be passed to the app
    switch (pDevice->Direction) {
        case ADI_DEV_DIRECTION_INBOUND:
            pDirectionInfo = &pDevice->InboundInfo;
            ProcessInboundFlag = TRUE;
            ProcessOutboundFlag = FALSE;
            break;
        case ADI_DEV_DIRECTION_OUTBOUND:
            pDirectionInfo = &pDevice->OutboundInfo;
            ProcessInboundFlag = FALSE;
            ProcessOutboundFlag = TRUE;
            break;
        default:
            pDirectionInfo = &pDevice->OutboundInfo;
            ProcessInboundFlag = TRUE;
            ProcessOutboundFlag = TRUE;
            break;
    }

    // read the SPORTStatus register to clear any pending interrupt
    SPORTStatus = pDevice->pRegisters->STAT;

    // IF (we have an inbound error and this device processes inbound events)
    if ((SPORTStatus & (ROVF | RUVF)) && ProcessInboundFlag) {

        // post the callback (parameters are device handle, ADI_SPORT_EVENT_ERROR_INTERRUPT, NULL)
        if (pDirectionInfo->DCBHandle) {
            adi_dcb_Post(pDirectionInfo->DCBHandle, 0, pDirectionInfo->DMCallback, pDirectionInfo->DeviceHandle, ADI_SPORT_EVENT_ERROR_INTERRUPT, NULL);
        } else {
            (pDirectionInfo->DMCallback)(pDirectionInfo->DeviceHandle, ADI_SPORT_EVENT_ERROR_INTERRUPT, NULL);
        }

        // clear the error
        pDevice->pRegisters->STAT = SPORTStatus & (ROVF | RUVF);

        // indicate that we processed the interrupt
        Result = ADI_INT_RESULT_PROCESSED;

    // ENDIF
    }

    // IF (we have an outbound error and this device processes outbound events)
    if ((SPORTStatus & (TOVF | TUVF)) && ProcessOutboundFlag) {

        // post the callback (parameters are device handle, ADI_SPORT_EVENT_ERROR_INTERRUPT, NULL)
        if (pDirectionInfo->DCBHandle) {
            adi_dcb_Post(pDirectionInfo->DCBHandle, 0, pDirectionInfo->DMCallback, pDirectionInfo->DeviceHandle, ADI_SPORT_EVENT_ERROR_INTERRUPT, NULL);
        } else {
            (pDirectionInfo->DMCallback)(pDirectionInfo->DeviceHandle, ADI_SPORT_EVENT_ERROR_INTERRUPT, NULL);
        }

        // clear the error
        pDevice->pRegisters->STAT = SPORTStatus & (TOVF | TUVF);

        // indicate that we processed the interrupt
        Result = ADI_INT_RESULT_PROCESSED;

    // ENDIF
    }

    // return
    return(Result);
}




/*********************************************************************

    Function:       FindDevice

    Description:    Because we allow the device to be opened separately for inbound
                    and outbound traffic, the PDDHandle may be one of three values:
                        o pointer to the whole device, if opened for bidirectional data
                        o pointer to the inbound data, if opened for inbound data
                        o pointer to the outbound data, if opened for outbound data

                    This function stores the address of the whole device in the
                    given location and returns whether the given handle is for the whole
                    device, or inbound or outbound only.

*********************************************************************/
static ADI_DEV_DIRECTION    FindDevice(     // finds a device and direction given a PDD handle
    ADI_DEV_PDD_HANDLE PDDHandle,               // handle to the device
    ADI_SPORT           **pDevice               // location where pointer to actual device will be stored
) {

    unsigned int        i;                  // counter
    ADI_SPORT           *pWorkingDevice;    // device we're working on
    ADI_DEV_DIRECTION   Direction;          // direction the handle is opened for

    // FOR (each SPORT)
    for (i = 0, pWorkingDevice = &Device[i]; i < ADI_SPORT_NUM_DEVICES; i++, pWorkingDevice++) {

        // IF (the SPORT is open)
        if (pWorkingDevice->UsageCount) {

            // check to see if the PDDHandle is the whole device, or either the inbound or outbound data
            if (PDDHandle == (ADI_DEV_PDD_HANDLE)pWorkingDevice){
                Direction = ADI_DEV_DIRECTION_BIDIRECTIONAL;
                break;
            }
            if (PDDHandle == (ADI_DEV_PDD_HANDLE)(&pWorkingDevice->InboundInfo)) {
                Direction = ADI_DEV_DIRECTION_INBOUND;
                break;
            }
            if (PDDHandle == (ADI_DEV_PDD_HANDLE)(&pWorkingDevice->OutboundInfo)) {
                Direction = ADI_DEV_DIRECTION_OUTBOUND;
                break;
            }

        // ENDIF
        }

    // ENDFOR
    }

    // save the device address in the caller location
    *pDevice = pWorkingDevice;

    // return
    return (Direction);
}



#if defined(__ADSP_BRAEMAR__)       // static functions for Braemar

/*********************************************************************

    Function:       sportSetPortControl

    Description:    Configures the general purpose ports for Braemar
                    class devices upon opening the device.

*********************************************************************/

static u32 sportSetPortControl( // configures the port control registers
    ADI_SPORT           *pDevice,       // pointer to the device
    ADI_DEV_DIRECTION   Direction       // direction to set
)
{
    u32 PortControl[2];     // Port Control Directives
    u32 lenPortControl = 0;     // Number of Port Control Directives

    // validate device pointer
    if (pDevice != &Device[0] && pDevice != &Device[1])
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;

    // IF (device is SPORT0)
    if (pDevice == &Device[0]) {

        // IF (opening for inbound or bidirectional traffic)
        if ((Direction == ADI_DEV_DIRECTION_INBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {
            PortControl[lenPortControl++] = ADI_PORTS_DIR_SPORT0_BASE_RX;
        }
        // IF (opening for outbound or bidirectional traffic)
        if ((Direction == ADI_DEV_DIRECTION_OUTBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

            PortControl[lenPortControl++] = ADI_PORTS_DIR_SPORT0_BASE_TX;
        }

    // ELSE (device is SPORT1)
    } else {

        // IF (opening for inbound or bidirectional traffic)
        if ((Direction == ADI_DEV_DIRECTION_INBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {
            PortControl[lenPortControl++] = ADI_PORTS_DIR_SPORT1_BASE_RX;
        }
        // IF (opening for outbound or bidirectional traffic)
        if ((Direction == ADI_DEV_DIRECTION_OUTBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

            PortControl[lenPortControl++] = ADI_PORTS_DIR_SPORT1_BASE_TX;
        }
    }

    // Configure port control
    adi_ports_EnableSPORT(PortControl,lenPortControl,TRUE);

    // return
    return ADI_DEV_RESULT_SUCCESS;
}

#endif      // __ADSP_BRAEMAR__


#if defined(__ADSP_STIRLING__)          // static functions for Stirling

/*********************************************************************

    Function:       sportSetPortControl

    Description:    Configures the general purpose ports for Stirling
                    class devices upon opening the device.

                    Only SPORT2 and 3 need port control.  SPORT0 and 1
                    do not need port control.

                    Pins enabled are as follows:

                            Inbound Outbound Bidirectional
                            ------- -------- -------------
                    RSCLKx     x                   x
                    RFSx       x                   x
                    DRxPRI     x                   x
                    DRxSEC  if RXSE             if RXSE
                    TSCLKx              x          x
                    TFSx                x          x
                    DTxPRI              x          x
                    DTxSEC           if TXSE    if TXSE


*********************************************************************/

static u32 sportSetPortControl( // configures the port control registers
    ADI_SPORT           *pDevice,       // pointer to the device
    ADI_DEV_DIRECTION   Direction       // direction to set
)
{

    ADI_PORTS_DIRECTIVE SPORT2InboundDirectives[] = {   // SPORT2 directives for inbound or bidirectional
        ADI_PORTS_DIRECTIVE_RSCLK2,
        ADI_PORTS_DIRECTIVE_RFS2,
        ADI_PORTS_DIRECTIVE_DR2PRI,
        ADI_PORTS_DIRECTIVE_DR2SEC
    };

    ADI_PORTS_DIRECTIVE SPORT2OutboundDirectives[] = {  // SPORT2 directives for outbound or bidirectional
        ADI_PORTS_DIRECTIVE_TSCLK2,
        ADI_PORTS_DIRECTIVE_TFS2,
        ADI_PORTS_DIRECTIVE_DT2PRI,
        ADI_PORTS_DIRECTIVE_DT2SEC
    };

    ADI_PORTS_DIRECTIVE SPORT3InboundDirectives[] = {   // SPORT3 directives for inbound or bidirectional
        ADI_PORTS_DIRECTIVE_RSCLK3,
        ADI_PORTS_DIRECTIVE_RFS3,
        ADI_PORTS_DIRECTIVE_DR3PRI,
        ADI_PORTS_DIRECTIVE_DR3SEC
    };

    ADI_PORTS_DIRECTIVE SPORT3OutboundDirectives[] = {  // SPORT3 directives for outbound or bidirectional
        ADI_PORTS_DIRECTIVE_TSCLK3,
        ADI_PORTS_DIRECTIVE_TFS3,
        ADI_PORTS_DIRECTIVE_DT3PRI,
        ADI_PORTS_DIRECTIVE_DT3SEC
    };
    ADI_PORTS_DIRECTIVE *pInboundDirectives;    // pointer to the table of inbound directives
    ADI_PORTS_DIRECTIVE *pOutboundDirectives;   // pointer to the table of outbound directives
    u32                 Count;                  // number of directives to pass to port control

    // validate device pointer
    if (pDevice != &Device[0] && pDevice != &Device[1] && pDevice != &Device[2] && pDevice != &Device[3])
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;

    // IF (not SPORT0 or 1)
    if ((pDevice != &Device[0]) && (pDevice != &Device[1])) {

        // point to the proper tables of directives
        if (pDevice == &Device[2]) {
            pInboundDirectives = SPORT2InboundDirectives;
            pOutboundDirectives = SPORT2OutboundDirectives;
        } else {
            pInboundDirectives = SPORT3InboundDirectives;
            pOutboundDirectives = SPORT3OutboundDirectives;
        }

        // IF (opened for inbound or bidirectional)
        if ((Direction == ADI_DEV_DIRECTION_INBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

            // do first 3 directives
            Count = 3;

            // IF (secondary channel enabled)
            if (pDevice->pRegisters->RCR2.rxse) {

                // do the secondary directive as well
                Count++;

            // ENDIF
            }

            // set port control
            adi_ports_Configure (pInboundDirectives, Count);

        // ENDIF
        }

        // IF (opened for outbound or bidirectional)
        if ((Direction == ADI_DEV_DIRECTION_INBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

            // do first 3 directives
            Count = 3;

            // IF (secondary channel enabled)
            if (pDevice->pRegisters->TCR2.txse) {

                // do the secondary directive as well
                Count++;

            // ENDIF
            }

            // set port control
            adi_ports_Configure (pOutboundDirectives, Count);

        // ENDIF
        }

    // ENDIF
    }

    // return
    return ADI_DEV_RESULT_SUCCESS;

}
#endif      // __ADSP_STIRLING__


#if defined(__ADSP_MOAB__)      /* static functions for Moab class devices */

/*********************************************************************

    Function:       sportSetPortControl

    Description:    Configures the general purpose ports for Stirling
                    class devices upon opening the device.

                    Only SPORT2 and 3 need port control.  SPORT0 and 1
                    do not need port control.

                    Pins enabled are as follows:

                            Inbound Outbound Bidirectional
                            ------- -------- -------------
                    RSCLKx     x                   x
                    RFSx       x                   x
                    DRxPRI     x                   x
                    DRxSEC  if RXSE             if RXSE
                    TSCLKx              x          x
                    TFSx                x          x
                    DTxPRI              x          x
                    DTxSEC           if TXSE    if TXSE


*********************************************************************/

static u32 sportSetPortControl(     /* configures the port control registers */
    ADI_SPORT           *pDevice,       /* pointer to the device */
    ADI_DEV_DIRECTION   Direction       /* direction to set */
){

    /* SPORT 0 Peripheral Directives */
    ADI_PORTS_DIRECTIVE SPORT0InboundDirectives[] = {   /* SPORT 0 directives for inbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT0_RFS,
        ADI_PORTS_DIRECTIVE_SPORT0_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT0_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0_DRSEC
    };
    ADI_PORTS_DIRECTIVE SPORT0OutboundDirectives[] = {  /* SPORT 0 directives for outbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT0_TFS,
        ADI_PORTS_DIRECTIVE_SPORT0_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT0_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0_DTSEC
    };

    /* SPORT 1 Peripheral Directives */
    ADI_PORTS_DIRECTIVE SPORT1InboundDirectives[] = {   /* SPORT 1 directives for inbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT1_RFS,
        ADI_PORTS_DIRECTIVE_SPORT1_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT1_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT1_DRSEC
    };
    ADI_PORTS_DIRECTIVE SPORT1OutboundDirectives[] = {  /* SPORT 1 directives for outbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT1_TFS,
        ADI_PORTS_DIRECTIVE_SPORT1_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT1_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT1_DTSEC
    };

    /* SPORT 2 Peripheral Directives */
    ADI_PORTS_DIRECTIVE SPORT2InboundDirectives[] = {   /* SPORT 2 directives for inbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT2_RFS,
        ADI_PORTS_DIRECTIVE_SPORT2_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT2_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT2_DRSEC
    };
    ADI_PORTS_DIRECTIVE SPORT2OutboundDirectives[] = {  /* SPORT 2 directives for outbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT2_TFS,
        ADI_PORTS_DIRECTIVE_SPORT2_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT2_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT2_DTSEC
    };

    /* SPORT 3 Peripheral Directives */
    ADI_PORTS_DIRECTIVE SPORT3InboundDirectives[] = {   /* SPORT 3 directives for inbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT3_RFS,
        ADI_PORTS_DIRECTIVE_SPORT3_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT3_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT3_DRSEC
    };
    ADI_PORTS_DIRECTIVE SPORT3OutboundDirectives[] = {  /* SPORT 3 directives for outbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT3_TFS,
        ADI_PORTS_DIRECTIVE_SPORT3_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT3_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT3_DTSEC
    };

    ADI_PORTS_DIRECTIVE *pInboundDirectives;    /* pointer to the table of inbound directives   */
    ADI_PORTS_DIRECTIVE *pOutboundDirectives;   /* pointer to the table of outbound directives  */
    u32                 nDirectives;            /* number of directives to pass to port control */

    /* assume no directives are to be set */
    pInboundDirectives  = NULL;
    pOutboundDirectives = NULL;

    // validate device pointer
    if (pDevice != &Device[0] && pDevice != &Device[1] && pDevice != &Device[2] && pDevice != &Device[3])
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;

    /* point to the proper tables of directives */
    if (pDevice == &Device[0]) {
        pInboundDirectives = SPORT0InboundDirectives;
        pOutboundDirectives = SPORT0OutboundDirectives;
    } else {
        if (pDevice == &Device[1]) {
            pInboundDirectives = SPORT1InboundDirectives;
            pOutboundDirectives = SPORT1OutboundDirectives;
        } else {
            if (pDevice == &Device[2]) {
                pInboundDirectives = SPORT2InboundDirectives;
                pOutboundDirectives = SPORT2OutboundDirectives;
            } else {
                pInboundDirectives = SPORT3InboundDirectives;
                pOutboundDirectives = SPORT3OutboundDirectives;
            }
        }
    }

    /* IF (opened for inbound or bidirectional) */
    if ((Direction == ADI_DEV_DIRECTION_INBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

        /* do first 3 directives */
        nDirectives = 3;

        /* IF (secondary channel enabled) */
        if (pDevice->pRegisters->RCR2.rxse) {

            /* do the secondary directive as well */
            nDirectives++;

        // ENDIF
        }

        /* set port control */
        adi_ports_Configure (pInboundDirectives, nDirectives);

    // ENDIF
    }

    /* IF (opened for outbound or bidirectional) */
    if ((Direction == ADI_DEV_DIRECTION_OUTBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

        /* do first 3 directives */
        nDirectives = 3;

        /* IF (secondary channel enabled) */
        if (pDevice->pRegisters->TCR2.txse) {

            /* do the secondary directive as well */
            nDirectives++;

        // ENDIF
        }

        /* set port control */
        adi_ports_Configure (pOutboundDirectives, nDirectives);

    // ENDIF
    }

    // return
    return ADI_DEV_RESULT_SUCCESS;
}

#endif      /* __ADSP_MOAB__ */



#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)       /* static functions for Kookaburra/Mockingbird class devices */

/*********************************************************************

    Function:       sportSetPortControl

    Description:    Configures the general purpose ports for Kookaburra
                    and Mockingbird class devices upon opening the device.

                    Each SPORT requires PORT config.
                        SPORT0 => PORTF - PORTF shares common SPORT0
                        SPORT1 => PORTF - dedicated PORTF for SPORT1
                        SPORT0 => PORTG - PORTG shares common SPORT0

                    Pins enabled are as follows:

                            Inbound Outbound Bidirectional
                            ------- -------- -------------
                    RSCLKx     x                   x
                    RFSx       x                   x
                    DRxPRI     x                   x
                    DRxSEC  if RXSE             if RXSE
                    TSCLKx              x          x
                    TFSx                x          x
                    DTxPRI              x          x
                    DTxSEC           if TXSE    if TXSE


*********************************************************************/

static u32 sportSetPortControl(     /* configures the port control registers */
    ADI_SPORT           *pDevice,       /* pointer to the device */
    ADI_DEV_DIRECTION   Direction       /* direction to set */
){

    /* SPORT 0 Peripheral Directives for PORT F */
    ADI_PORTS_DIRECTIVE SPORT0FInboundDirectives[] = {   /* SPORT 0 directives for inbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT0F_RFS,
        ADI_PORTS_DIRECTIVE_SPORT0F_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT0F_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0F_DRSEC
    };
    ADI_PORTS_DIRECTIVE SPORT0FOutboundDirectives[] = {  /* SPORT 0 directives for outbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT0F_TFS,
        ADI_PORTS_DIRECTIVE_SPORT0F_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT0F_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0F_DTSEC
    };

    /* SPORT 1 Peripheral Directives for PORT F */
    ADI_PORTS_DIRECTIVE SPORT1FInboundDirectives[] = {   /* SPORT 1 directives for inbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT1F_RFS,
        ADI_PORTS_DIRECTIVE_SPORT1F_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT1F_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT1F_DRSEC
    };
    ADI_PORTS_DIRECTIVE SPORT1FOutboundDirectives[] = {  /* SPORT 1 directives for outbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT1F_TFS,
        ADI_PORTS_DIRECTIVE_SPORT1F_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT1F_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT1F_DTSEC
    };

    /* SPORT 0 Peripheral Directives for PORT G */
    ADI_PORTS_DIRECTIVE SPORT0GInboundDirectives[] = {   /* SPORT 0 directives for inbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT0G_RFS,
        ADI_PORTS_DIRECTIVE_SPORT0G_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT0G_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0G_DRSEC
    };
    ADI_PORTS_DIRECTIVE SPORT0GOutboundDirectives[] = {  /* SPORT 0 directives for outbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT0G_TFS,
        ADI_PORTS_DIRECTIVE_SPORT0G_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT0G_TSCLK_MUX1,  // least potential conflict for SPORRT0 TSCLK is MUX1
      //ADI_PORTS_DIRECTIVE_SPORT0G_TSCLK_MUX0,  // MUX0 is an alternative choice for SPORT0 TSCLK
        ADI_PORTS_DIRECTIVE_SPORT0G_DTSEC
    };

    ADI_PORTS_DIRECTIVE *pInboundDirectives;    /* pointer to the table of inbound directives   */
    ADI_PORTS_DIRECTIVE *pOutboundDirectives;   /* pointer to the table of outbound directives  */
    u32                 nDirectives;            /* number of directives to pass to port control */

    /* assume no directives are to be set */
    pInboundDirectives  = NULL;
    pOutboundDirectives = NULL;

    // validate device pointer
    if (pDevice != &Device[0] && pDevice != &Device[1] && pDevice != &Device[2])
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;

    /* point to the proper tables of directives */
    if (pDevice == &Device[0]) {
        pInboundDirectives = SPORT0FInboundDirectives;
        pOutboundDirectives = SPORT0FOutboundDirectives;
    } else {
        if (pDevice == &Device[1]) {
            pInboundDirectives = SPORT1FInboundDirectives;
            pOutboundDirectives = SPORT1FOutboundDirectives;
        } else {
            pInboundDirectives = SPORT0GInboundDirectives;
            pOutboundDirectives = SPORT0GOutboundDirectives;
        }
    }

    /* IF (opened for inbound or bidirectional) */
    if ((Direction == ADI_DEV_DIRECTION_INBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

        /* do first 3 directives */
        nDirectives = 3;

        /* IF (secondary channel enabled) */
        if (pDevice->pRegisters->RCR2.rxse) {

            /* do the secondary directive as well */
            nDirectives++;

        // ENDIF
        }

        /* set port control */
        adi_ports_Configure (pInboundDirectives, nDirectives);

    // ENDIF
    }

    /* IF (opened for outbound or bidirectional) */
    if ((Direction == ADI_DEV_DIRECTION_OUTBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

        /* do first 3 directives */
        nDirectives = 3;

        /* IF (secondary channel enabled) */
        if (pDevice->pRegisters->TCR2.txse) {

            /* do the secondary directive as well */
            nDirectives++;

        // ENDIF
        }

        /* set port control */
        adi_ports_Configure (pOutboundDirectives, nDirectives);

    // ENDIF
    }

    // return
    return ADI_DEV_RESULT_SUCCESS;
}

#endif      /* __ADSP_KOOKABURRA__ or __ADSP_MOCKINGBIRD__ */


#if defined(__ADSP_BRODIE__)        /* static functions for Brodie class devices */

/*********************************************************************

    Function:       sportSetPortControl

    Description:    Configures the general purpose ports for Kookaburra
                    and Brodie class devices upon opening the device.

                    Each SPORT requires PORT config.
                        SPORT0 => PORTG - dedicated PORTG for SPORT0
                        SPORT1 => PORTF - dedicated PORTF for SPORT1
                        (no virtual ports, only 2 dedicated real ports)

                    Pins enabled are as follows:

                            Inbound Outbound Bidirectional
                            ------- -------- -------------
                    RSCLKx     x                   x
                    RFSx       x                   x
                    DRxPRI     x                   x
                    DRxSEC  if RXSE             if RXSE
                    TSCLKx              x          x
                    TFSx                x          x
                    DTxPRI              x          x
                    DTxSEC           if TXSE    if TXSE


*********************************************************************/

static u32 sportSetPortControl(     /* configures the port control registers */
    ADI_SPORT           *pDevice,       /* pointer to the device */
    ADI_DEV_DIRECTION   Direction       /* direction to set */
){

    /* SPORT 0 Peripheral Directives for PORT G */
    ADI_PORTS_DIRECTIVE SPORT0FInboundDirectives[] = {   /* SPORT 0 directives for inbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT0_RFS,
        ADI_PORTS_DIRECTIVE_SPORT0_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT0_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0_DRSEC
    };
    ADI_PORTS_DIRECTIVE SPORT0FOutboundDirectives[] = {  /* SPORT 0 directives for outbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT0_TFS,
        ADI_PORTS_DIRECTIVE_SPORT0_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT0_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0_DTSEC
    };

    /* SPORT 1 Peripheral Directives for PORT H */
    ADI_PORTS_DIRECTIVE SPORT1FInboundDirectives[] = {   /* SPORT 1 directives for inbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT1_RFS,
        ADI_PORTS_DIRECTIVE_SPORT1_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT1_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT1_DRSEC
    };
    ADI_PORTS_DIRECTIVE SPORT1FOutboundDirectives[] = {  /* SPORT 1 directives for outbound or bidirectional */
        ADI_PORTS_DIRECTIVE_SPORT1_TFS,
        ADI_PORTS_DIRECTIVE_SPORT1_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT1_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT1_DTSEC
    };

    ADI_PORTS_DIRECTIVE *pInboundDirectives;    /* pointer to the table of inbound directives   */
    ADI_PORTS_DIRECTIVE *pOutboundDirectives;   /* pointer to the table of outbound directives  */
    u32                 nDirectives;            /* number of directives to pass to port control */

    /* assume no directives are to be set */
    pInboundDirectives  = NULL;
    pOutboundDirectives = NULL;

    // validate device pointer
    if (pDevice != &Device[0] && pDevice != &Device[1])
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;

    /* point to the proper tables of directives */
    if (pDevice == &Device[0]) {
        pInboundDirectives = SPORT0FInboundDirectives;
        pOutboundDirectives = SPORT0FOutboundDirectives;
    } else {
        pInboundDirectives = SPORT1FInboundDirectives;
        pOutboundDirectives = SPORT1FOutboundDirectives;
    }

    /* IF (opened for inbound or bidirectional) */
    if ((Direction == ADI_DEV_DIRECTION_INBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

        /* do first 3 directives */
        nDirectives = 3;

        /* IF (secondary channel enabled) */
        if (pDevice->pRegisters->RCR2.rxse) {

            /* do the secondary directive as well */
            nDirectives++;

        // ENDIF
        }

        /* set port control */
        adi_ports_Configure (pInboundDirectives, nDirectives);

    // ENDIF
    }

    /* IF (opened for outbound or bidirectional) */
    if ((Direction == ADI_DEV_DIRECTION_OUTBOUND) || (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

        /* do first 3 directives */
        nDirectives = 3;

        /* IF (secondary channel enabled) */
        if (pDevice->pRegisters->TCR2.txse) {

            /* do the secondary directive as well */
            nDirectives++;

        // ENDIF
        }

        /* set port control */
        adi_ports_Configure (pOutboundDirectives, nDirectives);

    // ENDIF
    }

    // return
    return ADI_DEV_RESULT_SUCCESS;
}

#endif      /* __ADSP_BRODIE__ */

/* Static functions for Moy class devices */
#if defined(__ADSP_MOY__)
/*********************************************************************

    Function:       sportSetPortControl

        Configures the general purpose ports for Moy class devices upon
        opening the SPORT device.

        Each SPORT requires PORT config.
            Device 0 - SPORT0 => dedicated PORTF for SPORT0
            Device 1 - SPORT1 => signals split between PORTF & PORTH for SPORT1

            Pins enabled are as follows:

                    Inbound Outbound Bidirectional
                    ------- -------- -------------
            RSCLKx     x                   x
            RFSx       x                   x
            DRxPRI     x                   x
            DRxSEC  if RXSE             if RXSE
            TSCLKx              x          x
            TFSx                x          x
            DTxPRI              x          x
            DTxSEC           if TXSE    if TXSE

*********************************************************************/
static u32 sportSetPortControl(
    ADI_SPORT           *pDevice,
    ADI_DEV_DIRECTION   Direction
)
{

    /* SPORT 0 Peripheral Directives for inbound or bidirectional */
    ADI_PORTS_DIRECTIVE anSport0InboundDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_SPORT0_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0_RFS,
        ADI_PORTS_DIRECTIVE_SPORT0_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT0_DRSEC
    };

    /* SPORT 0 Peripheral Directives for outbound or bidirectional */
    ADI_PORTS_DIRECTIVE anSport0OutboundDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_SPORT0_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0_TFS,
        ADI_PORTS_DIRECTIVE_SPORT0_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT0_DTSEC
    };

    /* SPORT 1 Peripheral Directives for inbound or bidirectional */
    ADI_PORTS_DIRECTIVE anSport1InboundDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_SPORT1_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT1_RFS,
        ADI_PORTS_DIRECTIVE_SPORT1_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT1_DRSEC_PG4
    };

    /* SPORT 1 Peripheral Directives for outbound or bidirectional */
    ADI_PORTS_DIRECTIVE anSport1OutboundDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_SPORT1_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT1_TFS,
        ADI_PORTS_DIRECTIVE_SPORT1_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT1_DTSEC
    };


    /* pointer to the table of inbound directives   */
    ADI_PORTS_DIRECTIVE *pInboundDirectives;
    /* pointer to the table of outbound directives  */
    ADI_PORTS_DIRECTIVE *pOutboundDirectives;
    /* number of directives to pass to port control */
    u32                 nDirectives;
    /* return code - assume we're going to be successful */
    u32                 nResult = ADI_DEV_RESULT_SUCCESS;

    /* assume no directives are to be set */
    pInboundDirectives  = NULL;
    pOutboundDirectives = NULL;

    /* IF (Device instance 0) */
    if (pDevice == &Device[0])
    {
        /* Get address of array holding SPORT 0 directives */
        pInboundDirectives  = anSport0InboundDirectives;
        pOutboundDirectives = anSport0OutboundDirectives;
    }
    /* ELSE IF (Device instance 1) */
    else if (pDevice == &Device[1])
    {
        /* Get address of array holding SPORT 1 directives */
        pInboundDirectives  = anSport1InboundDirectives;
        pOutboundDirectives = anSport1OutboundDirectives;

        /* IF (Use Pin Mux mode 1) */
        if (pDevice->ePinMuxMode == ADI_SPORT_PIN_MUX_MODE_1)
        {
            /* Update SPORT 1 inbound directives table to use PG8 as DR1SEC */
            anSport1InboundDirectives[3] = ADI_PORTS_DIRECTIVE_SPORT1_DRSEC_PG8;
        }
        /* ELSE (other pin mux modes) */
            /* Use PG4 as DR1SEC */
    }
    /* ELSE (invalid device instance) */
    else
    {
        /* Report Failure (Device number invalid) */
        nResult = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }

    /* IF (Given device instance is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (opened for inbound or bidirectional) */
        if ((Direction == ADI_DEV_DIRECTION_INBOUND)||\
            (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL))
        {
            /* do first 3 directives */
            nDirectives = 3;

            /* IF (secondary channel enabled) */
            if (pDevice->pRegisters->RCR2.rxse)
            {
                /* do the secondary directive as well */
                nDirectives++;
            }

            /* Call port control to configure port pins for SPORT operation */
            nResult = (u32) adi_ports_Configure (pInboundDirectives, nDirectives);
        }
    }

    /* IF (Successfully configured port control for inbound) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (opened for outbound or bidirectional) */
        if ((Direction == ADI_DEV_DIRECTION_OUTBOUND) ||\
            (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL))
        {
            /* do first 3 directives */
            nDirectives = 3;

            /* IF (secondary channel enabled) */
            if (pDevice->pRegisters->TCR2.txse)
            {
                /* do the secondary directive as well */
                nDirectives++;
            }

            /* Call port control to configure port pins for SPORT operation */
            nResult = (u32) adi_ports_Configure (pOutboundDirectives, nDirectives);
        }
    }

    return (nResult);
}

#endif      /* __ADSP_MOY__ */

/* Static functions for Delta class devices */
#if defined(__ADSP_DELTA__) 
/*********************************************************************

    Function:       sportSetPortControl

        Configures the general purpose ports for Delta class devices upon
        opening the SPORT device.

        Each SPORT requires PORT config.
            Device 0 - SPORT0 => dedicated PORTF for SPORT0
            Device 1 - SPORT1 => dedicated PORTG for SPORT1

            Pins enabled are as follows:

                    Inbound Outbound Bidirectional
                    ------- -------- -------------
            RSCLKx     x                   x
            RFSx       x                   x
            DRxPRI     x                   x
            DRxSEC  if RXSE             if RXSE
            TSCLKx              x          x
            TFSx                x          x
            DTxPRI              x          x
            DTxSEC           if TXSE    if TXSE

*********************************************************************/
static u32 sportSetPortControl(
    ADI_SPORT           *pDevice,
    ADI_DEV_DIRECTION   Direction
)
{

    /* SPORT 0 Peripheral Directives for inbound or bidirectional */
    ADI_PORTS_DIRECTIVE anSport0InboundDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_SPORT0_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0_RFS,
        ADI_PORTS_DIRECTIVE_SPORT0_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT0_DRSEC
    };

    /* SPORT 0 Peripheral Directives for outbound or bidirectional */
    ADI_PORTS_DIRECTIVE anSport0OutboundDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_SPORT0_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT0_TFS,
        ADI_PORTS_DIRECTIVE_SPORT0_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT0_DTSEC
    };

    /* SPORT 1 Peripheral Directives for inbound or bidirectional */
    ADI_PORTS_DIRECTIVE anSport1InboundDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_SPORT1_RSCLK,
        ADI_PORTS_DIRECTIVE_SPORT1_RFS,
        ADI_PORTS_DIRECTIVE_SPORT1_DRPRI,
        ADI_PORTS_DIRECTIVE_SPORT1_DRSEC
    };

    /* SPORT 1 Peripheral Directives for outbound or bidirectional */
    ADI_PORTS_DIRECTIVE anSport1OutboundDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_SPORT1_TSCLK,
        ADI_PORTS_DIRECTIVE_SPORT1_TFS,
        ADI_PORTS_DIRECTIVE_SPORT1_DTPRI,
        ADI_PORTS_DIRECTIVE_SPORT1_DTSEC
    };


    /* pointer to the table of inbound directives   */
    ADI_PORTS_DIRECTIVE *pInboundDirectives;
    /* pointer to the table of outbound directives  */
    ADI_PORTS_DIRECTIVE *pOutboundDirectives;
    /* number of directives to pass to port control */
    u32                 nDirectives;
    /* return code - assume we're going to be successful */
    u32                 nResult = ADI_DEV_RESULT_SUCCESS;

    /* assume no directives are to be set */
    pInboundDirectives  = NULL;
    pOutboundDirectives = NULL;

    /* IF (Device instance 0) */
    if (pDevice == &Device[0])
    {
        /* Get address of array holding SPORT 0 directives */
        pInboundDirectives  = anSport0InboundDirectives;
        pOutboundDirectives = anSport0OutboundDirectives;
    }
    /* ELSE IF (Device instance 1) */
    else if (pDevice == &Device[1])
    {
        /* Get address of array holding SPORT 1 directives */
        pInboundDirectives  = anSport1InboundDirectives;
        pOutboundDirectives = anSport1OutboundDirectives;
            
    }
    /* ELSE (invalid device instance) */
    else
    {
        /* Report Failure (Device number invalid) */
        nResult = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }

    /* IF (Given device instance is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (opened for inbound or bidirectional) */
        if ((Direction == ADI_DEV_DIRECTION_INBOUND)||\
            (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL))
        {
            /* do first 3 directives */
            nDirectives = 3;

            /* IF (secondary channel enabled) */
            if (pDevice->pRegisters->RCR2.rxse)
            {
                /* do the secondary directive as well */
                nDirectives++;
            }

            /* Call port control to configure port pins for SPORT operation */
            nResult = (u32) adi_ports_Configure (pInboundDirectives, nDirectives);
        }
    }

    /* IF (Successfully configured port control for inbound) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (opened for outbound or bidirectional) */
        if ((Direction == ADI_DEV_DIRECTION_OUTBOUND) ||\
            (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL))
        {
            /* do first 3 directives */
            nDirectives = 3;

            /* IF (secondary channel enabled) */
            if (pDevice->pRegisters->TCR2.txse)
            {
                /* do the secondary directive as well */
                nDirectives++;
            }

            /* Call port control to configure port pins for SPORT operation */
            nResult = (u32) adi_ports_Configure (pOutboundDirectives, nDirectives);
        }
    }

    return (nResult);
}

#endif      /* __ADSP_DELTA__ */



#if defined(ADI_DEV_DEBUG)

/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {

    unsigned int i;
    ADI_SPORT *pDevice;

    // FOR (each SPORT)
    for (i = 0, pDevice = &Device[i]; i < ADI_SPORT_NUM_DEVICES; i++, pDevice++) {

        // check to see if the PDDHandle is the whole device, or either the inbound or outbound data
        if ((PDDHandle == (ADI_DEV_PDD_HANDLE)pDevice) ||
            (PDDHandle == (ADI_DEV_PDD_HANDLE)(&pDevice->InboundInfo)) ||
            (PDDHandle == (ADI_DEV_PDD_HANDLE)(&pDevice->OutboundInfo))) {
            return (ADI_DEV_RESULT_SUCCESS);
        }

    // ENDFOR
    }

    // return
    return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
    }

#endif /* ADI_DEV_DEBUG */


