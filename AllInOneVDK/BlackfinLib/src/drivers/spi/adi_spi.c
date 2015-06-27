/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_spi.c,v $
$Revision: 5506 $
$Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $

Description:
            This is the driver source code for the SPI peripheral.

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/


#include <services\services.h>      // system service includes
#include <drivers\adi_dev.h>        // device manager includes
#include <drivers\spi\adi_spi.h>    // spi driver includes

/*********************************************************************

Check to insure that either the dma-driven or interrupt-driven driver
is being built.

*********************************************************************/

#ifndef ADI_SPI_INT
#ifndef ADI_SPI_DMA
#error "*** ERROR: Must build either adi_spi_int.c or adi_spi_dma.c, not adi_spi.c ***"
#endif
#endif

/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_SPI_NUM_DEVICES (sizeof(Device)/sizeof(ADI_SPI))    // number of SPI devices in the system


typedef enum {              // enumeration for dataflow status
    ADI_SPI_DATAFLOW_STOPPED,   // dataflow is stopped
    ADI_SPI_DATAFLOW_RUNNING,   // dataflow is actually occurring
    ADI_SPI_DATAFLOW_PAUSED     // dataflow is paused
} ADI_SPI_DATAFLOW_STATUS;



/*********************************************************************

Macros for various SPI control register fields

*********************************************************************/

#define ADI_SPI_CONTROL_TIMOD_RDBR  (0)
#define ADI_SPI_CONTROL_SIZE_16     (1)
#define ADI_SPI_CONTROL_SPE_ENABLE  (1)
#define ADI_SPI_CONTROL_SPE_DISABLE (0)
#define ADI_SPI_CONTROL_MASTER      (1)

#define ADI_SPI_STATUS_MASK_TXCOL   (0x0040)    // mask for transmit collision error bit
#define ADI_SPI_STATUS_MASK_RBSY    (0x0010)    // mask for receive error bit
#define ADI_SPI_STATUS_MASK_TXE     (0x0004)    // mask for transmission error bit
#define ADI_SPI_STATUS_MASK_MODF    (0x0002)    // mask for mode fault error bit


/*********************************************************************

Data Structures

*********************************************************************/

typedef struct adi_spi_reserved_area {      // define a structure for the reserved area at the top of ADI_DEV_1D_BUFFER
    u8                  *pCurrentDataPointer;   // pointer within the data where we are processing
    u32                 BytesRemaining;         // remaining number of bytes to receive or transmit
    ADI_DEV_DIRECTION   Direction;              // buffer direction (redundant but makes code faster)
} ADI_SPI_RESERVED_AREA;



typedef union ADI_SPI_CONTROL_REG_UNION
{

        ADI_SPI_CONTROL_REG   BitFields;
        volatile u16 Value16;
}  ADI_SPI_CONTROL_REG_UNION;


typedef union ADI_SPI_STATUS_REG_UNION
 {
        ADI_SPI_STATUS_REG   BitFields;
        volatile u16 Value16;
} ADI_SPI_STATUS_REG_UNION;

typedef struct {                            // SPI device structure
    ADI_SPI_CONTROL_REG_UNION   *SPIControl;        // SPI control register
    ADI_SPI_STATUS_REG_UNION    *SPIStatus;         // SPI status register
    volatile u16            *SPIBaud;           // SPI baud register
    volatile u16            *SPIFlag;           // SPI flag register
    volatile u16            *SPIReceiveData;    // SPI receive data register
    volatile u16            *SPITransmitData;   // SPI transmit data register
    volatile u16            *SPIShadowData;     // SPI shadow data register

    u32                     SCLK;               // SCLK (system clock) frequency in Hz
    u32                     InUseFlag;          // open flag
    u32                     ExclusiveFlag;      // exclusive use flag
    u32                     ErrorReportingFlag; // error reporting flag
    ADI_SPI_DATAFLOW_STATUS DataflowStatus;     // dataflow status
    ADI_DEV_MODE            DataflowMethod;     // Dataflow method
    void                    *pEnterCriticalArg; // critical region argument
    ADI_DCB_HANDLE          DCBHandle;          // callback handle
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;       // device manager handle
    void (*DMCallback) (                        // device manager's callback function
        ADI_DEV_DEVICE_HANDLE DeviceHandle,             // device handle
        u32 Event,                                      // event ID
        void *pArg);                                    // argument pointer
    ADI_INT_PERIPHERAL_ID   ErrorPeripheralID;  // error peripheral ID
#ifdef ADI_SPI_INT                              // additional settings for interrupt-driven device
    ADI_INT_PERIPHERAL_ID   SpiPeripheralID;        // SPI peripheral ID
    u32                     SpiIVG;                 // SPI IVG
    ADI_SPI_RESERVED_AREA   *pInboundHead;          // head of inbound queue
    ADI_SPI_RESERVED_AREA   *pInboundTail;          // tail of inbound queue
    ADI_SPI_RESERVED_AREA   *pInboundCurrent;       // current pointer of inbound queue
    ADI_SPI_RESERVED_AREA   *pOutboundHead;         // head of outbound queue
    ADI_SPI_RESERVED_AREA   *pOutboundTail;         // tail of outbound queue
    ADI_SPI_RESERVED_AREA   *pOutboundCurrent;      // current pointer of outbound queue
    u32                     TransferSize;           // transfer size (in bytes)
    u32                     BigEndian;              // big endian flag
#else                                           // additional settings for dma-driven device
    ADI_DMA_PMAP             MappingID;             // DMA peripheral mapping ID
#endif

/* Pin Mux mode for Moy  and Delta (BF50x, BF59x processors) */
#if defined(__ADSP_MOY__) || defined(__ADSP_DELTA__)

    ADI_SPI_PIN_MUX_MODE    ePinMuxMode;

#endif

} ADI_SPI;



/*********************************************************************

Device specific data

*********************************************************************/

/***************************
    Edinburgh
****************************/

#if defined(__ADSP_EDINBURGH__)         // settings for Edinburgh class devices

static ADI_SPI Device[] = {             // Actual SPI devices
    {                                   // device 0
        (ADI_SPI_CONTROL_REG_UNION *)   pSPI_CTL,   // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)    pSPI_STAT,  // SPI status register
        (volatile u16 *)pSPI_BAUD,          // SPI baud register
        (volatile u16 *)pSPI_FLG,           // SPI flag register
        (volatile u16 *)pSPI_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI_SHADOW,        // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI_ERROR,                  // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA5_SPI,                       // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI,                       // DMA peripheral mapping ID
#endif
    },
};


#endif


/********************
    Braemar
********************/

#if defined(__ADSP_BRAEMAR__)           // settings for Braemar class devices

static ADI_SPI Device[] = {             // Actual SPI devices
    {                                   // device 0
        (ADI_SPI_CONTROL_REG_UNION *)pSPI_CTL,  // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI_STAT,  // SPI status register
        (volatile u16 *)pSPI_BAUD,          // SPI baud register
        (volatile u16 *)pSPI_FLG,           // SPI flag register
        (volatile u16 *)pSPI_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI_SHADOW,        // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_PERIPHERAL_ERROR,           // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA7_SPI,                       // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI,                       // DMA peripheral mapping ID
#endif
    },
};


#endif


/********************
    Teton-Lite
********************/

#if defined(__ADSP_TETON__)              // settings for Teton class devices

static ADI_SPI Device[] = {             // Actual SPI devices
    {                                   // device 0
        (ADI_SPI_CONTROL_REG_UNION *)pSPI_CTL,  // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI_STAT,  // SPI status register
        (volatile u16 *)pSPI_BAUD,          // SPI baud register
        (volatile u16 *)pSPI_FLG,           // SPI flag register
        (volatile u16 *)pSPI_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI_SHADOW,        // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI_ERROR,                  // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA2_4_SPI,                     // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI,                       // DMA peripheral mapping ID
#endif
    },
};


#endif


/***************************
    Stirling
****************************/

#if defined(__ADSP_STIRLING__)          // settings for Stirling class devices

static ADI_SPI Device[] = {             // Actual SPI devices
    {                                   // device 0
        (ADI_SPI_CONTROL_REG_UNION *)pSPI0_CTL,   // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI0_STAT, // SPI status register
        (volatile u16 *)pSPI0_BAUD,         // SPI baud register
        (volatile u16 *)pSPI0_FLG,          // SPI flag register
        (volatile u16 *)pSPI0_RDBR,         // SPI receive data register
        (volatile u16 *)pSPI0_TDBR,         // SPI transmit data register
        (volatile u16 *)pSPI0_SHADOW,       // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI0_ERROR,                 // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA5_SPI0,                      // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI0,                      // DMA peripheral mapping ID
#endif
    },
    {                                   // device 1
        (ADI_SPI_CONTROL_REG_UNION *)pSPI1_CTL,   // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI1_STAT, // SPI status register
        (volatile u16 *)pSPI1_BAUD,         // SPI baud register
        (volatile u16 *)pSPI1_FLG,          // SPI flag register
        (volatile u16 *)pSPI1_RDBR,         // SPI receive data register
        (volatile u16 *)pSPI1_TDBR,         // SPI transmit data register
        (volatile u16 *)pSPI1_SHADOW,       // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI1_ERROR,                 // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA14_SPI1,                     // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI1,                      // DMA peripheral mapping ID
#endif
    },
    {                                   // device 2
        (ADI_SPI_CONTROL_REG_UNION *)pSPI2_CTL,   // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI2_STAT, // SPI status register
        (volatile u16 *)pSPI2_BAUD,         // SPI baud register
        (volatile u16 *)pSPI2_FLG,          // SPI flag register
        (volatile u16 *)pSPI2_RDBR,         // SPI receive data register
        (volatile u16 *)pSPI2_TDBR,         // SPI transmit data register
        (volatile u16 *)pSPI2_SHADOW,       // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI2_ERROR,                 // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA15_SPI2,                     // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI2,                      // DMA peripheral mapping ID
#endif
    },
};


#endif


/***************************

    Moab

****************************/

#if defined(__ADSP_MOAB__)   // Moab device class

static ADI_SPI Device[] = {             // Actual SPI devices
    {                                   // device 0
        (ADI_SPI_CONTROL_REG_UNION *)pSPI0_CTL,   // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI0_STAT, // SPI status register
        (volatile u16 *)pSPI0_BAUD,         // SPI baud register
        (volatile u16 *)pSPI0_FLG,          // SPI flag register
        (volatile u16 *)pSPI0_RDBR,         // SPI receive data register
        (volatile u16 *)pSPI0_TDBR,         // SPI transmit data register
        (volatile u16 *)pSPI0_SHADOW,       // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI0_STATUS,                // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA4_SPI0,                      // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI0,                      // DMA peripheral mapping ID
#endif
    },
    {                                   // device 1
        (ADI_SPI_CONTROL_REG_UNION *)pSPI1_CTL,   // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI1_STAT, // SPI status register
        (volatile u16 *)pSPI1_BAUD,         // SPI baud register
        (volatile u16 *)pSPI1_FLG,          // SPI flag register
        (volatile u16 *)pSPI1_RDBR,         // SPI receive data register
        (volatile u16 *)pSPI1_TDBR,         // SPI transmit data register
        (volatile u16 *)pSPI1_SHADOW,       // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI1_STATUS,                // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA5_SPI1,                      // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI1,                      // DMA peripheral mapping ID
#endif
    },
    {                                   // device 2
        (ADI_SPI_CONTROL_REG_UNION *)pSPI2_CTL,   // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI2_STAT, // SPI status register
        (volatile u16 *)pSPI2_BAUD,         // SPI baud register
        (volatile u16 *)pSPI2_FLG,          // SPI flag register
        (volatile u16 *)pSPI2_RDBR,         // SPI receive data register
        (volatile u16 *)pSPI2_TDBR,         // SPI transmit data register
        (volatile u16 *)pSPI2_SHADOW,       // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI2_STATUS,                // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA23_SPI2,                     // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI2,                      // DMA peripheral mapping ID
#endif
    },
};


#endif  // Moab class devices


/********************
    Kookaburra/Mockingbird
********************/

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

static ADI_SPI Device[] = {             // Actual SPI devices
    {                                   // device 0
        (ADI_SPI_CONTROL_REG_UNION *)pSPI_CTL,  // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI_STAT,  // SPI status register
        (volatile u16 *)pSPI_BAUD,          // SPI baud register
        (volatile u16 *)pSPI_FLG,           // SPI flag register
        (volatile u16 *)pSPI_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI_SHADOW,        // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI_STATUS,                 // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA7_SPI,                       // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI,                       // DMA peripheral mapping ID
#endif
    },

    {                                   // device "1" (virtually same as 0, but configures controls over PORTG MUX2
        (ADI_SPI_CONTROL_REG_UNION *)pSPI_CTL,  // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI_STAT,  // SPI status register
        (volatile u16 *)pSPI_BAUD,          // SPI baud register
        (volatile u16 *)pSPI_FLG,           // SPI flag register
        (volatile u16 *)pSPI_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI_SHADOW,        // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI_STATUS,                 // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA7_SPI,                       // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI,                       // DMA peripheral mapping ID
#endif
    },
};


#endif  // end Kookaburra/Mockingbird


/********************
    Brodie
********************/

#if defined(__ADSP_BRODIE__)

/* Brodie has two dedicated SPI controllers with two dedicated port interfaces */

static ADI_SPI Device[] = {             // SPI0
    {                                   // device 0
        (ADI_SPI_CONTROL_REG_UNION *)pSPI0_CTL, // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI0_STAT, // SPI status register
        (volatile u16 *)pSPI0_BAUD,          // SPI baud register
        (volatile u16 *)pSPI0_FLG,           // SPI flag register
        (volatile u16 *)pSPI0_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI0_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI0_SHADOW,        // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI0_STATUS,                // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA7_SPI0,                      // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI0,                  // DMA peripheral mapping ID
#endif
    },

    {                                   // SPI1
        (ADI_SPI_CONTROL_REG_UNION *)pSPI1_CTL, // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI1_STAT, // SPI status register
        (volatile u16 *)pSPI1_BAUD,          // SPI baud register
        (volatile u16 *)pSPI1_FLG,           // SPI flag register
        (volatile u16 *)pSPI1_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI1_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI1_SHADOW,        // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI1_STATUS,                // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA5_SPORT1_RX_AND_SPI1,    // SPI peripheral ID
        0,                                  // SPI IVG
        NULL,                               // head of inbound queue
        NULL,                               // tail of inbound queue
        NULL,                               // current of inbound queue
        NULL,                               // head of outbound queue
        NULL,                               // tail of outbound queue
        NULL,                               // current of outbound queue
        0,                                  // transfer size (in bytes)
        FALSE,                              // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPORT1_RX_AND_SPI1,    // DMA peripheral mapping ID
#endif
    },
};


#endif  // end Brodie


/********************
    Moy (ADSP-BF50x)
********************/

#if defined(__ADSP_MOY__)


/*
** Note:
**  Moy has two physical SPI (SPI0 & SPI1)
**  SPI 0 offers following options
**      - use PF15 or PH1 to operate as Slave Select 3
**  SPI 1 offers following options
**      - use PG1 or PH2 to operate as Slave Select 2
**      - use PG0 or PH1 to operate as Slave Select 3
*/

static ADI_SPI Device[] = {             // SPI0
    {                                   // device 0
        (ADI_SPI_CONTROL_REG_UNION *)pSPI0_CTL, // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI0_STAT, // SPI status register
        (volatile u16 *)pSPI0_BAUD,          // SPI baud register
        (volatile u16 *)pSPI0_FLG,           // SPI flag register
        (volatile u16 *)pSPI0_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI0_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI0_SHADOW,        // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI0_STATUS,                // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA6_SPI0,                      // SPI peripheral ID
        0,                                      // SPI IVG
        NULL,                                   // head of inbound queue
        NULL,                                   // tail of inbound queue
        NULL,                                   // current of inbound queue
        NULL,                                   // head of outbound queue
        NULL,                                   // tail of outbound queue
        NULL,                                   // current of outbound queue
        0,                                      // transfer size (in bytes)
        FALSE,                                  // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI0,                  // DMA peripheral mapping ID
#endif
        ADI_SPI_PIN_MUX_MODE_0,             /* Default Pin Mux mode */
    },

    {                                   // SPI1
        (ADI_SPI_CONTROL_REG_UNION *)pSPI1_CTL, // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI1_STAT, // SPI status register
        (volatile u16 *)pSPI1_BAUD,          // SPI baud register
        (volatile u16 *)pSPI1_FLG,           // SPI flag register
        (volatile u16 *)pSPI1_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI1_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI1_SHADOW,        // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI1_STATUS,                // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA7_SPI1,                  // SPI peripheral ID
        0,                                  // SPI IVG
        NULL,                               // head of inbound queue
        NULL,                               // tail of inbound queue
        NULL,                               // current of inbound queue
        NULL,                               // head of outbound queue
        NULL,                               // tail of outbound queue
        NULL,                               // current of outbound queue
        0,                                  // transfer size (in bytes)
        FALSE,                              // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI1,                  // DMA peripheral mapping ID
#endif
        ADI_SPI_PIN_MUX_MODE_0,             /* Default Pin Mux mode */
    },
};


#endif  // end Moy

/********************
    Delta (ADSP-BF59x)
********************/

#if defined(__ADSP_DELTA__)


/*
** Note:
**  Delta has two physical SPI (SPI0 & SPI1)
**  SPI 1 offers following options
**      - use PF14 or PG14 to operate as Slave Select 4
**      - use PF15 or PG11 to operate as Slave Select 5
**      - use PG5  or PG15 to operate as Slave Select 6
*/

static ADI_SPI Device[] = {             // SPI0
    {                                   // device 0
        (ADI_SPI_CONTROL_REG_UNION *)pSPI0_CTL, // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI0_STAT, // SPI status register
        (volatile u16 *)pSPI0_BAUD,          // SPI baud register
        (volatile u16 *)pSPI0_FLG,           // SPI flag register
        (volatile u16 *)pSPI0_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI0_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI0_SHADOW,        // SPI shadow data register

        0,                                   // SCLK frequency
        FALSE,                               // open flag
        FALSE,                               // exclusive use flag
        FALSE,                               // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,            // dataflow status
        ADI_DEV_MODE_UNDEFINED,              // Dataflow method
        NULL,                                // critical region argument
        NULL,                                // callback handle
        NULL,                                // device manager handle
        NULL,                                // device manager callback function
        ADI_INT_SPI0_STATUS,                 // error peripheral ID
#ifdef ADI_SPI_INT                           // settings for interrupt driven device
        ADI_INT_DMA5_SPI0,                   // SPI peripheral ID
        0,                                   // SPI IVG
        NULL,                                // head of inbound queue
        NULL,                                // tail of inbound queue
        NULL,                                // current of inbound queue
        NULL,                                // head of outbound queue
        NULL,                                // tail of outbound queue
        NULL,                                // current of outbound queue
        0,                                   // transfer size (in bytes)
        FALSE,                               // Default as little endian
#else                                        // settings for dma-driven device
        ADI_DMA_PMAP_SPI0,                   // DMA peripheral mapping ID
#endif
        ADI_SPI_PIN_MUX_MODE_0,             /* Default Pin Mux mode */
    },

    {                                   // SPI1
        (ADI_SPI_CONTROL_REG_UNION *)pSPI1_CTL, // SPI control register
        (ADI_SPI_STATUS_REG_UNION *)pSPI1_STAT, // SPI status register
        (volatile u16 *)pSPI1_BAUD,          // SPI baud register
        (volatile u16 *)pSPI1_FLG,           // SPI flag register
        (volatile u16 *)pSPI1_RDBR,          // SPI receive data register
        (volatile u16 *)pSPI1_TDBR,          // SPI transmit data register
        (volatile u16 *)pSPI1_SHADOW,        // SPI shadow data register

        0,                                  // SCLK frequency
        FALSE,                              // open flag
        FALSE,                              // exclusive use flag
        FALSE,                              // error reporting flag
        ADI_SPI_DATAFLOW_STOPPED,           // dataflow status
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        NULL,                               // critical region argument
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_INT_SPI1_STATUS,                // error peripheral ID
#ifdef ADI_SPI_INT                          // settings for interrupt driven device
        ADI_INT_DMA6_SPI1,                  // SPI peripheral ID
        0,                                  // SPI IVG
        NULL,                               // head of inbound queue
        NULL,                               // tail of inbound queue
        NULL,                               // current of inbound queue
        NULL,                               // head of outbound queue
        NULL,                               // tail of outbound queue
        NULL,                               // current of outbound queue
        0,                                  // transfer size (in bytes)
        FALSE,                              // Default as little endian
#else                                       // settings for dma-driven device
        ADI_DMA_PMAP_SPI1,                  // DMA peripheral mapping ID
#endif
        ADI_SPI_PIN_MUX_MODE_0,             /* Default Pin Mux mode */
    },
};


#endif  // end Delta



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

static u32 adi_pdd_SequentialIO(// queues a sequential buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
);

static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    u32 Command,                    // command ID
    void *Value                     // command specific value
);

static void spiReportAndClearErrors(    // reports and clears errors
    ADI_SPI             *pDevice,       // device we're working on
    ADI_SPI_STATUS_REG_UNION    *StatusReg       // status register
);

#if defined(ADI_SPI_INT)

static ADI_INT_HANDLER(InterruptHandler);   // SPI interrupt handler

static void spiBufferComplete(              // processes completed buffers
    ADI_SPI             *pDevice,
    ADI_SPI_RESERVED_AREA   *pReservedArea,
    ADI_SPI_RESERVED_AREA   **pCurrent,
    ADI_SPI_RESERVED_AREA   **pHead,
    u32 CallbackEvent);

static ADI_DEV_BUFFER *SetupReservedArea(   // prepares the reserved area of buffers passed in
    ADI_SPI *pDevice,
    ADI_DEV_BUFFER *pBuffer,
    ADI_DEV_DIRECTION Direction);

static void AddBuffersToQueue(              // adds buffers to a queue
    ADI_SPI *pDevice,
    ADI_SPI_RESERVED_AREA **ppHead,
    ADI_SPI_RESERVED_AREA **ppTail,
    ADI_DEV_BUFFER *pFirstBuffer,
    ADI_DEV_BUFFER *pLastBuffer);

static void Restart(ADI_SPI *pDevice);      // restarts the SPI

static void LoadTransmitFromBuffer(         // loads the transmit register from a buffer
    ADI_SPI *pDevice,
    ADI_SPI_RESERVED_AREA *pReserved);

static void StoreIntoBuffer(                // stores the given data into a buffer
    ADI_SPI *pDevice,
    u16 Data,
    ADI_SPI_RESERVED_AREA *pReserved);

#endif


#if defined(ADI_SPI_DMA)
static ADI_INT_HANDLER(ErrorHandler);       // interrupt handler for errors
#endif

#if defined (__ADSP_BRAEMAR__)     ||\
    defined (__ADSP_STIRLING__)    ||\
    defined (__ADSP_MOAB__)        ||\
    defined (__ADSP_KOOKABURRA__)  ||\
    defined (__ADSP_MOCKINGBIRD__) ||\
    defined (__ADSP_BRODIE__)      ||\
    defined (__ADSP_MOY__)         ||\
    defined (__ADSP_DELTA__)

static u32 spiSetPortControl(ADI_SPI *pDevice, u32 SlaveSelectOnlyFlag);

#endif  // __ADSP_BRAEMAR__ || __ADSP_STIRLING__ || __ADSP_MOAB__



#if defined(ADI_DEV_DEBUG)

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);

#endif


/*********************************************************************

Entry point for device manager

*********************************************************************/

#if defined(ADI_SPI_INT)
ADI_DEV_PDD_ENTRY_POINT ADISPIIntEntryPoint =
#else
ADI_DEV_PDD_ENTRY_POINT ADISPIDMAEntryPoint =
#endif
{
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};





/*********************************************************************

    Function:       adi_pdd_Open

    Description:    Opens a SPI device for use

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

    u32             Result;                 // return value
    ADI_SPI         *pDevice;               // pointer to the device we're working on
    void            *pExitCriticalArg;      // exit critical region parameter
    u32             i;                      // temporary value
    u32             SCLK;                   // sclk

    // check for errors if required
#ifdef ADI_DEV_DEBUG
    if (DeviceNumber >= ADI_SPI_NUM_DEVICES) {      // check the device number
        return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
    }
#endif

#ifdef ADI_SPI_DMA
    if (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL) {
        return(ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED); // data direction not supported
    }
#endif

    // get SCLK (now returns values in Hz!)
    if ((Result = adi_pwr_GetFreq(&i, &SCLK, &i)) != ADI_PWR_RESULT_SUCCESS) {
        return (Result);
    }

    // insure the device the client wants is available
    Result = ADI_DEV_RESULT_DEVICE_IN_USE;
    pDevice = &Device[DeviceNumber];
    pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
    if (pDevice->InUseFlag == FALSE) {
        pDevice->InUseFlag = TRUE;
        Result = ADI_DEV_RESULT_SUCCESS;
    }
    adi_int_ExitCriticalRegion(pExitCriticalArg);
    if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);

    // reset the actual device and clear any error conditions

    pDevice->SPIControl->Value16 = 0x0400;

    *pDevice->SPIBaud = 0;
    *pDevice->SPIFlag = 0xff00;

    pDevice->SPIStatus->Value16 = 0;

    pDevice->SPIStatus->BitFields.modf = 1;
    pDevice->SPIStatus->BitFields.rbsy = 1; // mode fault error
    pDevice->SPIStatus->BitFields.txe = 1;  // transmission error
    pDevice->SPIStatus->BitFields.txcol = 1;
    pDevice->SPIStatus->BitFields.spif = 0; // SPI finished
    pDevice->SPIStatus->BitFields.tsx = 0;  // SPI_TDBR data buffer status
    pDevice->SPIStatus->BitFields.rxs = 0;  // RX data buffer status


    *pDevice->SPITransmitData = 0x0000;

    // initialize the device settings
    pDevice->DCBHandle = DCBHandle;
    pDevice->DMCallback = DMCallback;
    pDevice->DeviceHandle = DeviceHandle;
    pDevice->pEnterCriticalArg = pEnterCriticalArg;
    pDevice->ExclusiveFlag = FALSE;
    pDevice->SCLK = SCLK;
    pDevice->DataflowStatus = ADI_SPI_DATAFLOW_STOPPED;
    pDevice->ErrorReportingFlag = FALSE;

    // save the physical device handle in the client supplied location
    *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;

    // IF (this is the interrupt driven driver)
#if defined(ADI_SPI_INT)

        // initialize the device settings
        pDevice->SpiIVG = 0;

        // initialize data queues
        pDevice->pOutboundHead = NULL;
        pDevice->pOutboundTail = NULL;
        pDevice->pInboundHead = NULL;
        pDevice->pInboundTail = NULL;
        pDevice->pOutboundCurrent = NULL;
        pDevice->pInboundCurrent = NULL;
        pDevice->BigEndian = FALSE; // default as little endian(as blackfin memory is little endian)

        // get the SPI IVG
        adi_int_SICGetIVG(pDevice->SpiPeripheralID, &pDevice->SpiIVG);

        // hook the Rx interrupt handler
        if(adi_int_CECHook(pDevice->SpiIVG, InterruptHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS){
            pDevice->InUseFlag = FALSE;
            return (ADI_DEV_RESULT_CANT_HOOK_INTERRUPT);
        }

        // allow this interrupt to wake us up and allow it to be passed to the core
        adi_int_SICWakeup(pDevice->SpiPeripheralID, TRUE);
        adi_int_SICEnable(pDevice->SpiPeripheralID);

    // ENDIF
#endif

    // set port control if we're a Braemar/Stirling/Moab/Mockingbird/Kookaburra/Brodie/Moy class device
#if defined (__ADSP_BRAEMAR__)     ||\
    defined (__ADSP_STIRLING__)    ||\
    defined (__ADSP_MOAB__)        ||\
    defined (__ADSP_KOOKABURRA__)  ||\
    defined (__ADSP_MOCKINGBIRD__) ||\
    defined (__ADSP_BRODIE__)      ||\
    defined (__ADSP_MOY__)         ||\
    defined (__ADSP_DELTA__)
        if (ADI_DEV_RESULT_SUCCESS != (Result = spiSetPortControl(pDevice, FALSE)))
            return(Result);
#endif

    // return
    return(ADI_DEV_RESULT_SUCCESS);
}






/*********************************************************************

    Function:       adi_pdd_Close

    Description:    Closes down a SPI device

*********************************************************************/


static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle    // PDD handle
) {

    u32             Result;     // return value
    ADI_SPI         *pDevice;   // pointer to the device we're working on


    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    // avoid casts
    pDevice = (ADI_SPI *)PDDHandle;

    // disable dataflow
    if ((Result = adi_pdd_Control (PDDHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)FALSE)) != ADI_DEV_RESULT_SUCCESS) {
        return (Result);
    }

    // IF (this is the interrupt driven driver)
#if defined(ADI_SPI_INT)

        // disable interrupts from the SPI
        adi_int_SICDisable(pDevice->SpiPeripheralID);
        adi_int_SICWakeup(pDevice->SpiPeripheralID, FALSE);

        // unhook the handler
        if ((Result = adi_int_CECUnhook(pDevice->SpiIVG, InterruptHandler, pDevice)) != ADI_INT_RESULT_SUCCESS) {
            return (ADI_DEV_RESULT_CANT_UNHOOK_INTERRUPT);
        }

    // ENDIF
#endif

    // shut down error reporting
    Result = adi_pdd_Control(PDDHandle, ADI_DEV_CMD_SET_ERROR_REPORTING, (void *)FALSE);

    // mark the device as closed
    pDevice->InUseFlag = FALSE;

    // return
    return(Result);
}








/*********************************************************************

    Function:       adi_pdd_Read

    Description:    Has to exist for both driver types but should only
                    be called for the interrupt-driven driver.  The device
                    manager manages buffers for the DMA-driven version
                    of the driver.

*********************************************************************/

static u32 adi_pdd_Read(        // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER      *pBuffer    // pointer to buffer
){

    u32             Result;             // return value
    ADI_SPI         *pDevice;           // pointer to the device we're working on
    ADI_DEV_BUFFER  *pLastBuffer;       // pointer to the last buffer in the chain

    // assume we'll be successful
    Result = ADI_DEV_RESULT_SUCCESS;

    // avoid casts
    pDevice = (ADI_SPI *)PDDHandle;

    // IF (this is the DMA version of the driver)
#if defined(ADI_SPI_DMA)

        // return an error
        Result = ADI_DEV_RESULT_FAILED;

    // ELSE (this is the interrupt driven version)
#else

        // IF (debug version)
#if     defined(ADI_DEV_DEBUG)

            // validate the PDD handle
            if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);

            // validate that the proper buffer type is passed in
            if (BufferType != ADI_DEV_1D) return (ADI_DEV_RESULT_BUFFER_TYPE_INCOMPATIBLE);

            // validate the caller has us in normal I/O, not sequential
            if ((pDevice->DataflowMethod != ADI_DEV_MODE_CHAINED) && (pDevice->DataflowMethod != ADI_DEV_MODE_CHAINED_LOOPBACK)) {
                return (ADI_SPI_RESULT_RW_SEQIO_MISMATCH);
            }

        // ENDIF
#endif

        // setup reserved area (prepares and populates the reserved area)
        pLastBuffer = SetupReservedArea(pDevice, pBuffer, ADI_DEV_DIRECTION_INBOUND);

        // add the buffers to the proper queue
        AddBuffersToQueue(pDevice, &pDevice->pInboundHead, &pDevice->pInboundTail, pBuffer, pLastBuffer);

        // call the restart function to start/restart dataflow as required
        Restart(pDevice);

    // ENDIF
#endif

    // return
    return (Result);
}




/*********************************************************************

    Function:       adi_pdd_Write

    Description:    Has to exist for both driver types but should only
                    be called for the interrupt-driven driver.  The device
                    manager manages buffers for the DMA-driven version
                    of the driver.

*********************************************************************/

static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER      *pBuffer    // pointer to buffer
){

    u32             Result;             // return value
    ADI_SPI         *pDevice;           // pointer to the device we're working on
    ADI_DEV_BUFFER  *pLastBuffer;       // pointer to the last buffer in the chain

    // assume we'll be successful
    Result = ADI_DEV_RESULT_SUCCESS;

    // avoid casts
    pDevice = (ADI_SPI *)PDDHandle;

    // IF (this is the DMA version of the driver)
#if defined(ADI_SPI_DMA)

        // return an error
        Result = ADI_DEV_RESULT_FAILED;

    // ELSE (this is the interrupt driven version)
#else

        // IF (debug version)
#if     defined(ADI_DEV_DEBUG)

            // validate the PDD handle
            if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);

            // validate that the proper buffer type is passed in
            if (BufferType != ADI_DEV_1D) return (ADI_DEV_RESULT_BUFFER_TYPE_INCOMPATIBLE);

            // validate the caller has us in normal I/O, not sequential
            if ((pDevice->DataflowMethod != ADI_DEV_MODE_CHAINED) && (pDevice->DataflowMethod != ADI_DEV_MODE_CHAINED_LOOPBACK)) {
                return (ADI_SPI_RESULT_RW_SEQIO_MISMATCH);
            }

        // ENDIF
#endif

        // setup reserved area (prepares and populates the reserved area)
        pLastBuffer = SetupReservedArea(pDevice, pBuffer, ADI_DEV_DIRECTION_OUTBOUND);

        // add the buffers to the proper queue
        AddBuffersToQueue(pDevice, &pDevice->pOutboundHead, &pDevice->pOutboundTail, pBuffer, pLastBuffer);

        // call the restart function to start/restart dataflow as required
        Restart(pDevice);

    // ENDIF
#endif

    // return
    return (Result);
}




/*********************************************************************

    Function:       adi_pdd_SequentialIO

    Description:    Has to exist for both driver types but should only
                    be called for the interrupt-driven driver.  The device
                    manager manages buffers for the DMA-driven version
                    of the driver.

*********************************************************************/

static u32 adi_pdd_SequentialIO(    // provides buffer during sequential I/O
    ADI_DEV_PDD_HANDLE  PDDHandle,      // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType,     // buffer type
    ADI_DEV_BUFFER      *pBuffer        // pointer to buffer
){

    u32             Result;             // return value
    ADI_SPI         *pDevice;           // pointer to the device we're working on
    ADI_DEV_BUFFER  *pLastBuffer;       // pointer to the last buffer in the chain

    // assume we'll be successful
    Result = ADI_DEV_RESULT_SUCCESS;

    // avoid casts
    pDevice = (ADI_SPI *)PDDHandle;

    // IF (this is the DMA version of the driver)
#if defined(ADI_SPI_DMA)

        // return an error
        Result = ADI_DEV_RESULT_FAILED;

    // ELSE (this is the interrupt driven version)
#else

        // IF (debug version)
#if     defined(ADI_DEV_DEBUG)

            // validate the PDD handle
            if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);

            // validate that the proper buffer type is passed in
            if (BufferType != ADI_DEV_SEQ_1D) return (ADI_DEV_RESULT_BUFFER_TYPE_INCOMPATIBLE);

            // validate the caller has us in sequential I/O mode
            if ((pDevice->DataflowMethod != ADI_DEV_MODE_SEQ_CHAINED) && (pDevice->DataflowMethod != ADI_DEV_MODE_SEQ_CHAINED_LOOPBACK)) {
                return (ADI_SPI_RESULT_RW_SEQIO_MISMATCH);
            }

        // ENDIF
#endif

        // setup reserved area (prepares and populates the reserved area)
        pLastBuffer = SetupReservedArea(pDevice, pBuffer, ADI_DEV_DIRECTION_BIDIRECTIONAL);

        // add the buffers to the proper queue (in sequential I/O we just use the inbound queue)
        AddBuffersToQueue(pDevice, &pDevice->pInboundHead, &pDevice->pInboundTail, pBuffer, pLastBuffer);

        // call the restart function to start/restart dataflow as required
        Restart(pDevice);

    // ENDIF
#endif

    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_pdd_Control

    Description:    Configures the SPI device

*********************************************************************/


static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    u32                 Command,    // command ID
    void                *Value      // command specific value
) {

    ADI_SPI                     *pDevice;           // pointer to the device we're working on
    u16                         u16tmp;             // temporary storage
    ADI_SPI_CONTROL_REG_UNION   SPIControl;         // control register value
    u32                         Result;             // return value
    u32                         ErrorIVG;           // IVG for SPI error interrupts
    u32                         u32Value;           // u32 type to avoid casts/warnings etc.
    u16                         u16Value;           // u16 type to avoid casts/warnings etc.
    void                        *pExitCriticalArg;  // exit critical region parameter
    u32                         PortControl;        // Port Control Directive


    // avoid casts
    pDevice = (ADI_SPI *)PDDHandle;
    u32Value = (u32)Value;
    u16Value = ((u16)((u32)Value));

    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    // assume we're going to be successful
    Result = ADI_DEV_RESULT_SUCCESS;

    // CASEOF (Command ID)
    switch (Command) {

        // CASE (control dataflow)
        case (ADI_DEV_CMD_SET_DATAFLOW):

            // make sure we have a valid baud rate if debug is enabled and we're turning on dataflow
            // and make sure we're not re-enabling if we're enabled
#if defined(ADI_DEV_DEBUG)
            if (u32Value == TRUE) {
                // Baud rate is only valid if in Master mode - Fix for TAR 36774, DCE Jan 23 2009
                if (*pDevice->SPIBaud < 2 && (pDevice->SPIControl->BitFields.mstr == ADI_SPI_CONTROL_MASTER)) return(ADI_SPI_RESULT_BAD_BAUD_NUMBER);
                if (pDevice->DataflowStatus != ADI_SPI_DATAFLOW_STOPPED) return (ADI_SPI_RESULT_DATAFLOW_ENABLED);
            }
#endif

            // IF (this is the DMA driven driver)
#if         defined(ADI_SPI_DMA)

                // protect this region
                pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

                // update the dataflow flag
                if (u32Value == TRUE) {
                    pDevice->DataflowStatus = ADI_SPI_DATAFLOW_RUNNING;
                } else {
                    pDevice->DataflowStatus = ADI_SPI_DATAFLOW_STOPPED;
                }

                // set the SPI control register accordingly
                pDevice->SPIControl->BitFields.spe = u16Value;

                // unprotect
                adi_int_ExitCriticalRegion(pExitCriticalArg);

            // ELSE (this is interrupt driven driver)
#else

                // save the transfer size
                if (pDevice->SPIControl->BitFields.size == ADI_SPI_CONTROL_SIZE_16) {
                    pDevice->TransferSize = 2;
                } else {
                    pDevice->TransferSize = 1;
                }

                // IF (enabling dataflow)
                if (u32Value == TRUE) {

                    // override TIMOD to start transfer with a read of RDBR
                    // this allows us to use the same mechanism all the time and does not change functionality
                    pDevice->SPIControl->BitFields.timod = ADI_SPI_CONTROL_TIMOD_RDBR;

                    // set our status to paused
                    pDevice->DataflowStatus = ADI_SPI_DATAFLOW_PAUSED;

                    // call the restart function to kick things off
                    Restart(pDevice);

                // ELSE
                } else {

                    // protect this region
                    pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

                    // set our status to stopped
                    pDevice->DataflowStatus = ADI_SPI_DATAFLOW_STOPPED;

                    // clear the SPI enable bit
                    pDevice->SPIControl->BitFields.spe = ADI_SPI_CONTROL_SPE_DISABLE;

                    // unprotect
                    adi_int_ExitCriticalRegion(pExitCriticalArg);

                // ENDIF
                }

            // ENDIF
#endif
            break;

        // CASE (query for processor DMA support)
        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

            // the DMA-driven driver is supported by peripheral DMA, the interrupt-driven driver is not
#if defined(ADI_SPI_DMA)
            *((u32 *)Value) = TRUE;
#else
            *((u32 *)Value) = FALSE;
#endif
            break;

        // CASE ( error reporting)
        case (ADI_DEV_CMD_SET_ERROR_REPORTING):

            // don't do anything if nothing is being changed
            if (u32Value == pDevice->ErrorReportingFlag) {
                break;
            }

            // IF (this is the DMA version (only DMA version generates error interrupts)
#if defined(ADI_SPI_DMA)

                // get the error interrupt IVG
                adi_int_SICGetIVG(pDevice->ErrorPeripheralID, &ErrorIVG);

                // IF (enabling)
                if ((u32Value) == TRUE) {

                    // hook the SPI error interrupt handler into the system
                    if(adi_int_CECHook(ErrorIVG, ErrorHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS){
                        return (ADI_DEV_RESULT_CANT_HOOK_INTERRUPT);
                    }
                    adi_int_SICWakeup(pDevice->ErrorPeripheralID, TRUE);
                    adi_int_SICEnable(pDevice->ErrorPeripheralID);

                // ELSE
                } else {

                    // unhook the SPI error interrupt handler from the system
                    adi_int_SICDisable(pDevice->ErrorPeripheralID);
                    adi_int_SICWakeup(pDevice->ErrorPeripheralID, FALSE);
                    if (adi_int_CECUnhook(ErrorIVG, ErrorHandler, pDevice) != ADI_INT_RESULT_SUCCESS) {
                        return(ADI_DEV_RESULT_CANT_UNHOOK_INTERRUPT);
                    }

                // ENDIF
                }

            // ENDIF
#endif

            // update status
            pDevice->ErrorReportingFlag = (u32Value);
            break;

        // CASE (set SPI control register)
        case (ADI_SPI_CMD_SET_CONTROL_REG):

            // update the register and return
            pDevice->SPIControl->Value16 = u16Value;
            ssync();
            break;

        // CASE (set baud rate)
        case (ADI_SPI_CMD_SET_BAUD_RATE):

            // update the SPI_BAUD register based on the SCLK setting
            // SCK = SCLK / (2 * SPI_BAUD)
            *pDevice->SPIBaud = pDevice->SCLK / (2 * u32Value);
            break;

        // CASE (set flag register)
        case (ADI_SPI_CMD_SET_SLAVE_FLAG):

            // protect this region
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // update the real flag register
            *pDevice->SPIFlag = u16Value;

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);

// set port control if we're a Braemar/Stirling/Moab/Mockingbird/Kookaburra/Brodie/Moy class device
#if defined (__ADSP_BRAEMAR__)     ||\
    defined (__ADSP_STIRLING__)    ||\
    defined (__ADSP_MOAB__)        ||\
    defined (__ADSP_KOOKABURRA__)  ||\
    defined (__ADSP_MOCKINGBIRD__) ||\
    defined (__ADSP_BRODIE__)      ||\
    defined (__ADSP_MOY__)         ||\
    defined (__ADSP_DELTA__)
            spiSetPortControl(pDevice, TRUE);
#endif
            break;

        // CASE (set baud register)
        case (ADI_SPI_CMD_SET_BAUD_REG):
            *pDevice->SPIBaud = u16Value;
            break;

        // CASE (enable a slave select line)
        case ADI_SPI_CMD_ENABLE_SLAVE_SELECT:

#if defined(ADI_DEV_DEBUG)
#if defined (__ADSP_EDINBURGH__) || defined (__ADSP_BRAEMAR__) || defined (__ADSP_STIRLING__) || defined (__ADSP_TETON__) || defined (__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__) || defined(__ADSP_DELTA__)
            // validate the slave number
            if ((u32Value == 0) || (u32Value > 7)) return(ADI_SPI_RESULT_BAD_SLAVE_NUMBER);
#elif defined (__ADSP_MOAB__) || defined (__ADSP_MOY__)
            // validate the slave number
            if ((u32Value == 0) || (u32Value > 3)) return(ADI_SPI_RESULT_BAD_SLAVE_NUMBER);
#endif
#endif

            // protect this region
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // update the SPI Flag register accordingly
            *pDevice->SPIFlag |= (0x1 << u32Value);

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);

// set port control if we're a Braemar/Stirling/Moab/Mockingbird/Kookaburra/Brodie/Moy class device
#if defined (__ADSP_BRAEMAR__)     ||\
    defined (__ADSP_STIRLING__)    ||\
    defined (__ADSP_MOAB__)        ||\
    defined (__ADSP_KOOKABURRA__)  ||\
    defined (__ADSP_MOCKINGBIRD__) ||\
    defined (__ADSP_BRODIE__)      ||\
    defined (__ADSP_MOY__)         ||\
    defined (__ADSP_DELTA__)
            spiSetPortControl(pDevice, TRUE);
#endif

            break;

        // CASE (disable a slave select line)
        case ADI_SPI_CMD_DISABLE_SLAVE_SELECT:

#if defined(ADI_DEV_DEBUG)
#if defined (__ADSP_EDINBURGH__) || defined (__ADSP_BRAEMAR__) || defined (__ADSP_STIRLING__) || defined (__ADSP_TETON__) || defined (__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__) || defined(__ADSP_DELTA__)
            // validate the slave number
            if ((u32Value == 0) || (u32Value > 7)) return(ADI_SPI_RESULT_BAD_SLAVE_NUMBER);
#elif defined (__ADSP_MOAB__) || defined (__ADSP_MOY__)
            // validate the slave number
            if ((u32Value == 0) || (u32Value > 3)) return(ADI_SPI_RESULT_BAD_SLAVE_NUMBER);
#endif
#endif

            // protect this region
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // update the SPI Flag register accordingly
            *pDevice->SPIFlag &= ~(0x1 << u32Value);

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);
            break;

        // CASE (select or deselect a slave)
        case ADI_SPI_CMD_SELECT_SLAVE:
        case ADI_SPI_CMD_DESELECT_SLAVE:

#if defined(ADI_DEV_DEBUG)
#if defined (__ADSP_EDINBURGH__) || defined (__ADSP_BRAEMAR__) || defined (__ADSP_STIRLING__) || defined (__ADSP_TETON__) || defined (__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)  || defined(__ADSP_BRODIE__) || defined(__ADSP_DELTA__)
            // validate the slave number
            if ((u32Value == 0) || (u32Value > 7)) return(ADI_SPI_RESULT_BAD_SLAVE_NUMBER);
#elif defined (__ADSP_MOAB__) || defined (__ADSP_MOY__)
            // validate the slave number
            if ((u32Value == 0) || (u32Value > 3)) return(ADI_SPI_RESULT_BAD_SLAVE_NUMBER);
#endif
#endif

            // create the mask
            u16tmp = (0x0100 << u32Value);

            // protect this region
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // update the SPI Flag register accordingly
            if (Command == ADI_SPI_CMD_DESELECT_SLAVE) {
                *pDevice->SPIFlag |= u16tmp;
            } else {
                *pDevice->SPIFlag &= ~u16tmp;
            }

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);
            break;

        // CASE (set exclusive access)
        case ADI_SPI_CMD_SET_EXCLUSIVE_ACCESS:

            // protect this region
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // grant it if requesting and no one else has it
            // otherwise update the flag
            if (u32Value == TRUE) {
                if (pDevice->ExclusiveFlag == FALSE) {
                    pDevice->ExclusiveFlag = TRUE;
                } else {
                    Result = ADI_SPI_RESULT_ALREADY_EXCLUSIVE;
                }
            } else {
                pDevice->ExclusiveFlag = FALSE;
            }

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);
            break;

        // CASE (execute a dummy read (data is discarded))
        case ADI_SPI_CMD_EXECUTE_DUMMY_READ:

#if defined(ADI_DEV_DEBUG)
            // insure dataflow is stopped
            if (pDevice->SPIControl->BitFields.spe == 1) return(ADI_SPI_RESULT_DATAFLOW_ENABLED);

            // insure we're told 8 or 16 bits
            if ((u32Value != 8) && (u32Value != 16)) {
                return ADI_SPI_RESULT_BAD_WORD_SIZE;
            }
#endif

            // protect this region
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // save the contents of the control register
            SPIControl = *pDevice->SPIControl;

            // set control word for a single read of the requested number of bits
            pDevice->SPIControl->BitFields.timod = 0;
            pDevice->SPIControl->BitFields.size = (u32Value >> 4);

            // execute the read
            pDevice->SPIControl->BitFields.spe = 1;

            // clear any data that's sitting in the receive buffer and cause the transfer to start
            u16tmp = *pDevice->SPIReceiveData;

            // wait for the read to complete
            while (pDevice->SPIStatus->BitFields.rxs == 0) ;

            // clear the data (and pending interrupt)
            u16tmp = *pDevice->SPIReceiveData;

            // restore the control register
            *pDevice->SPIControl = SPIControl;

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);
            break;

        // CASE (set the transfer init mode)
        case ADI_SPI_CMD_SET_TRANSFER_INIT_MODE:

#if defined(ADI_DEV_DEBUG)
            // validate the transfer mode
            if ((Command == ADI_SPI_CMD_SET_TRANSFER_INIT_MODE) && (u32Value > 3)) return ADI_SPI_RESULT_BAD_TRANSFER_INIT_MODE;
#endif

            // protect this region
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // place the value in the SPI control register
            pDevice->SPIControl->BitFields.timod = (u16)(u32Value);

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);
            break;


        // CASE (pause/resume dataflow)
        case ADI_SPI_CMD_PAUSE_DATAFLOW:

            // protect this region
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // set the SPI enable bit appropriately
            if (u32Value) {
                pDevice->SPIControl->BitFields.spe = 0;
            } else {
                pDevice->SPIControl->BitFields.spe = 1;
            }

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);
            break;


        // CASE (set a field in the SPI control word, other than transfer init)
        case ADI_SPI_CMD_SEND_ZEROS:
        case ADI_SPI_CMD_SET_GET_MORE_DATA:
        case ADI_SPI_CMD_SET_PSSE:
        case ADI_SPI_CMD_SET_MISO:
        case ADI_SPI_CMD_SET_WORD_SIZE:
        case ADI_SPI_CMD_SET_LSB_FIRST:
        case ADI_SPI_CMD_SET_CLOCK_PHASE:
        case ADI_SPI_CMD_SET_CLOCK_POLARITY:
        case ADI_SPI_CMD_SET_MASTER:
        case ADI_SPI_CMD_SET_OPEN_DRAIN_MASTER:

#if defined(ADI_DEV_DEBUG)
            // set word size should be 8 or 16, everything else should be TRUE or FALSE
            if (Command == ADI_SPI_CMD_SET_WORD_SIZE) {
                if ((u32Value != 8) && (u32Value != 16)) {
                    return ADI_SPI_RESULT_BAD_WORD_SIZE;
                }
            } else {
                if ((u32Value != TRUE) && (u32Value != FALSE)) {
                    return ADI_SPI_RESULT_BAD_VALUE;
                }
            }
#endif

            // if setting the transfer word size, modify the value so that we can use a general solution here to set the proper bits in the register
            if (Command == ADI_SPI_CMD_SET_WORD_SIZE) {
                u32Value = ((u32Value == 8) ? 0 : 1);
            }

            // create a mask for the control register
            u16tmp = 1 << (u16)(Command - ADI_SPI_CMD_SET_TRANSFER_INIT_MODE);

            // protect this region
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // set/clear the appropriate bit in the SPI control register
            if (u32Value)
            {
                 pDevice->SPIControl->Value16 |= u16tmp;
            }
            else
            {
                 pDevice->SPIControl->Value16 &= ~u16tmp;
            }

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);

// set port control if we're a Braemar/Stirling/Moab/Mockingbird/Kookaburra/Brodie/Moy class device
#if defined (__ADSP_BRAEMAR__)     ||\
    defined (__ADSP_STIRLING__)    ||\
    defined (__ADSP_MOAB__)        ||\
    defined (__ADSP_KOOKABURRA__)  ||\
    defined (__ADSP_MOCKINGBIRD__) ||\
    defined (__ADSP_BRODIE__)      ||\
    defined (__ADSP_MOY__)         ||\
    defined (__ADSP_DELTA__)

            // pass PSSE command through ports configure again to activate the right MUX & FER bits
            if (ADI_SPI_CMD_SET_PSSE == Command) {
                if (ADI_DEV_RESULT_SUCCESS != (Result = spiSetPortControl(pDevice,pDevice->SPIControl->BitFields.mstr)))
                    return(Result);
            }
#endif
            break;

        // *******************
        // the following commands are only processed by the DMA-driven driver
#if defined(ADI_SPI_DMA)
        // CASE (query for our inbound or outbound DMA peripheral ID)
        case (ADI_DEV_CMD_GET_INBOUND_DMA_PMAP_ID):
        case (ADI_DEV_CMD_GET_OUTBOUND_DMA_PMAP_ID):

            // tell him what it is
            *((ADI_DMA_PMAP *)Value) = pDevice->MappingID;
            break;
#endif

        // *******************
        // the following commands are only processed by the interrupt driven driver
#if defined(ADI_SPI_INT)
        // CASE (set dataflow method)
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):

            // we only support chained descriptors with or without loopback
            switch ( (ADI_DEV_MODE)Value ) {
                case ADI_DEV_MODE_CHAINED :
                case ADI_DEV_MODE_CHAINED_LOOPBACK :
                case ADI_DEV_MODE_SEQ_CHAINED :
                case ADI_DEV_MODE_SEQ_CHAINED_LOOPBACK :
                    pDevice->DataflowMethod = ((ADI_DEV_MODE)Value);
                    break;
                default :
                    Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
            }
            break;

        //case (switch transfer data type between little & big endian)
        case (ADI_SPI_CMD_SET_TRANSFER_TYPE_BIG_ENDIAN):
            // true sets data transfer type as big endian
            // false sets data transfer type as little endian
            pDevice->BigEndian = u32Value;
            break;

#endif

        /* CASE (Set Pin Mux Mode) */
        case (ADI_SPI_CMD_SET_PIN_MUX_MODE):

/* For BF50x, BF59x Family */
#if defined (__ADSP_MOY__) || defined(__ADSP_DELTA__)
            /* Update pin mux mode selection */
            pDevice->ePinMuxMode = ((ADI_SPI_PIN_MUX_MODE) Value);
            /* Update SPI Port Mux */
            Result = spiSetPortControl(pDevice, FALSE);
#endif

            break;

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

    Function:       spiReportAndClearErrors

    Description:    Reports error to the client

*********************************************************************/

static void spiReportAndClearErrors(
    ADI_SPI                     *pDevice,       // device we're working on
    ADI_SPI_STATUS_REG_UNION    *StatusReg      // status register
) {

    ADI_DEV_1D_BUFFER   *pBuffer;        // pointer to a buffer
    ADI_SPI_STATUS_REG_UNION  ClearData;       // data to write back to the status register to clear the errors

    // clear the value we're going to write to the status register to clear all errors
    ClearData.Value16 = 0;

    // call the client's callback function for each reported error noting which bits need to be cleared in the status register
    if (StatusReg->BitFields.txcol) {
        (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_SPI_EVENT_TRANSMIT_COLLISION_ERROR, NULL);
        ClearData.BitFields.txcol = 1;
    }
    if (StatusReg->BitFields.rbsy) {
        (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_SPI_EVENT_RECEIVE_ERROR, NULL);
        ClearData.BitFields.rbsy = 1;
    }
    if (StatusReg->BitFields.txe) {
        (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_SPI_EVENT_TRANSMISSION_ERROR, NULL);
        ClearData.BitFields.txe = 1;
    }
    if (StatusReg->BitFields.modf) {
        (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_SPI_EVENT_MODE_FAULT_ERROR, NULL);
        ClearData.BitFields.modf = 1;
    }

    // clear the error bits in the status register
    pDevice->SPIStatus->Value16 = ClearData.Value16;

}



#if defined(ADI_SPI_INT)



/*********************************************************************

    Function:       SetupReservedArea

    Description:    Populates the various fields in the reserved area
                    of buffer structures

                    Returns the last buffer in the chain

*********************************************************************/

static ADI_DEV_BUFFER *SetupReservedArea(ADI_SPI *pDevice, ADI_DEV_BUFFER *pBuffer, ADI_DEV_DIRECTION Direction) {

    ADI_DEV_1D_BUFFER       *pWorkingBuffer;    // pointer to the buffer we're working on
    ADI_DEV_1D_BUFFER       *pLastBuffer;       // pointer to the last buffer in the chain
    ADI_SPI_RESERVED_AREA   *pReservedArea;     // pointer to the reserved area at the top of a buffer
    ADI_DEV_SEQ_1D_BUFFER   *pSeqBuffer;        // pointer to sequential buffer

    // FOR (each buffer in the chain)
    for (pWorkingBuffer = &pBuffer->OneD; pWorkingBuffer != NULL; pWorkingBuffer = pWorkingBuffer->pNext) {

        // point to the reserved area (avoids casts)
        pReservedArea = (ADI_SPI_RESERVED_AREA *)pWorkingBuffer->Reserved;

        // set the byte count for the data the buffer points to
        pReservedArea->BytesRemaining = pWorkingBuffer->ElementCount * pWorkingBuffer->ElementWidth;

        // point to the first piece of data in the buffer
        pReservedArea->pCurrentDataPointer = pWorkingBuffer->Data;

        // set the direction of the buffer (this is redundant with but makes the code faster)
        if (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL) {
            pSeqBuffer = (ADI_DEV_SEQ_1D_BUFFER *)pWorkingBuffer;
            pReservedArea->Direction = pSeqBuffer->Direction;
        } else {
            pReservedArea->Direction = Direction;
        }

        // update the last buffer pointer
        pLastBuffer = pWorkingBuffer;

    // ENDFOR
    }

    // return
    return ((ADI_DEV_BUFFER *)pLastBuffer);
}



/*********************************************************************

    Function:       AddBuffersToQueue

    Description:    Adds a buffer chain to a queue

*********************************************************************/

static void AddBuffersToQueue(ADI_SPI *pDevice, ADI_SPI_RESERVED_AREA **ppHead, ADI_SPI_RESERVED_AREA **ppTail, ADI_DEV_BUFFER *pFirstBuffer, ADI_DEV_BUFFER *pLastBuffer) {

    void    *pExitCriticalArg;  // exit critical region parameter

    // protect this region
    pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

    // IF (the queue is empty)
    if (*ppHead == NULL)
    {

        // set the head to the first buffer and the tail to the last
        *ppHead = (ADI_SPI_RESERVED_AREA *)pFirstBuffer->OneD.Reserved;
        *ppTail = (ADI_SPI_RESERVED_AREA *)pLastBuffer->OneD.Reserved;

        // IF (we're doing normal I/O)
        if ((pDevice->DataflowMethod == ADI_DEV_MODE_CHAINED) || (pDevice->DataflowMethod == ADI_DEV_MODE_CHAINED_LOOPBACK))
        {
            // IF (the first buffer is inbound buffer)
            if ((*ppHead)->Direction == ADI_DEV_DIRECTION_INBOUND)
            {
                // set our current inbound pointer to the head of the inbound queue
                pDevice->pInboundCurrent = *ppHead;
            }
            //ELSE
            else
            {
                // set our current outbound pointer to the head of the outbound queue
                pDevice->pOutboundCurrent = *ppHead;
            }
        }
        // ELSE (we're doing sequential I/O)
        else
        {
            // we only use the inbound queue for sequential
            // set our current inbound pointer to the head of the inbound queue
            pDevice->pInboundCurrent = *ppHead;
        }
    // ELSE
    }
    else
    {

        // insert the chain at the end of the existing chain
        ((ADI_DEV_1D_BUFFER *)*ppTail)->pNext = (ADI_DEV_1D_BUFFER *)pFirstBuffer;
        *ppTail = (ADI_SPI_RESERVED_AREA *)pLastBuffer->OneD.Reserved;

    // ENDIF
    }

    // unprotect
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // return
}



/*********************************************************************

    Function:       Restart

    Description:    Starts or restarts dataflow on the SPI port depending
                    on the current status of the SPI dataflow

                        o Disabled - then the user hasn't turned on dataflow
                        yet so we don't need to do anything

                        o Enabled - dataflow is currently active so again we
                        don't need to do anything

                        o Paused - the user has turned on dataflow but we
                        don't currently have the SPI enabled so we need to
                        start it if we have some data to process

*********************************************************************/

static void Restart(ADI_SPI *pDevice) {

    void                    *pExitCriticalArg;  // exit critical region parameter
    u32                     StartDataflowFlag;  // flag to start dataflow
    ADI_SPI_RESERVED_AREA   *pReservedArea;     // pointer to reserved area of buffer
    u16                     tmp;                // temporary to read register

    // protect this region
    pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

    // IF (dataflow is currently paused)
    if (pDevice->DataflowStatus == ADI_SPI_DATAFLOW_PAUSED) {

        // clear the start dataflow flag
        StartDataflowFlag = FALSE;

        // IF (we're doing normal I/O)
        if ((pDevice->DataflowMethod == ADI_DEV_MODE_CHAINED) || (pDevice->DataflowMethod == ADI_DEV_MODE_CHAINED_LOOPBACK)) {

            // IF (we have inbound buffers)
            if (pDevice->pInboundHead != NULL) {

                // set our current inbound pointer to the head of the inbound queue
                pDevice->pInboundCurrent = pDevice->pInboundHead;

                // set the start dataflow flag
                StartDataflowFlag = TRUE;

            // ENDIF
            }

            // IF (we have outbound buffers)
            if (pDevice->pOutboundHead != NULL) {

                // set our current outbound pointer to the head of the outbound queue
                pDevice->pOutboundCurrent = pDevice->pOutboundHead;

                // load the first word into the transmit register
                LoadTransmitFromBuffer(pDevice, pDevice->pOutboundCurrent);

                // set the start dataflow flag
                StartDataflowFlag = TRUE;

            // ENDIF
            }

        // ELSE (we're doing sequential I/O)
        } else {

            // IF (we have buffers on the queue) (we only use the inbound queue for sequential)
            if (pDevice->pInboundHead != NULL) {

                // set our current inbound pointer to the head of the inbound queue
                pDevice->pInboundCurrent = pDevice->pInboundHead;

                // point to the first buffer
                pReservedArea = pDevice->pInboundCurrent;

                // IF (the first buffer is outbound buffer)
                if (pReservedArea->Direction == ADI_DEV_DIRECTION_OUTBOUND) {

                    // load the first word into the transmit register
                    LoadTransmitFromBuffer(pDevice, pDevice->pInboundCurrent);

                // ENDIF
                }

                // set the start dataflow flag
                StartDataflowFlag = TRUE;

            // ENDIF
            }

        // ENDIF
        }

        // IF (we need to start dataflow)
        if (StartDataflowFlag) {

            // update our dataflow flag
            pDevice->DataflowStatus = ADI_SPI_DATAFLOW_RUNNING;

            // turn on the SPI
            pDevice->SPIControl->BitFields.spe = ADI_SPI_CONTROL_SPE_ENABLE;

            // read the receive register to start dataflow and discard the data
            tmp = *pDevice->SPIReceiveData;

        // ENDIF
        }

    // ENDIF
    }

    // unprotect
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // return
}


/*********************************************************************

    Function:       LoadTransmitFromBuffer

    Description:    Loads into the transmit register from the given buffer

*********************************************************************/

static void LoadTransmitFromBuffer (ADI_SPI *pDevice, ADI_SPI_RESERVED_AREA *pReserved) {

    u16 Data;       // data to transmit
    u16 NextByte;   // next byte of data

    // get the first byte of data and increment the data pointer
    Data = *pReserved->pCurrentDataPointer++;

    // IF (16 bit transfer)
    if (pDevice->TransferSize == 2) {

        // get the next byte of data and increment the data pointer
        NextByte = *pReserved->pCurrentDataPointer++;

        // add in the other byte and watch for endianness
        if (pDevice->BigEndian) {
            Data = (Data << 8) | NextByte;
        } else {
            Data = Data | (NextByte << 8);
        }

    // ENDIF
    }

    // decrement the bytes remaining count
    pReserved->BytesRemaining -= pDevice->TransferSize;

    // store the data into the transmit register
    *pDevice->SPITransmitData = Data;

    // return
}




/*********************************************************************

    Function:       StoreIntoBuffer

    Description:    Stores the given data into the given buffer

*********************************************************************/

static void StoreIntoBuffer (ADI_SPI *pDevice, u16 Data, ADI_SPI_RESERVED_AREA *pReserved) {

    u16 NextByte;   // next byte of data

    // IF (8 bit transfers)
    if (pDevice->TransferSize == 1) {

        // store the data into the buffer
        *pReserved->pCurrentDataPointer = Data;

    // ELSE
    } else {

        // store the data into the buffer
        *((u16 *)pReserved->pCurrentDataPointer) = Data;

    // ENDIF
    }

    // decrement the bytes remaining count
    pReserved->BytesRemaining -= pDevice->TransferSize;

    // increment the data pointer
    pReserved->pCurrentDataPointer += pDevice->TransferSize;

    // return
}



/*********************************************************************

    Function:       spiBufferComplete

    Description:    Does the processing necessary when a buffer has been
                    filled or sent out

                    Note we're called from the ISR so we don't have to worry
                    about critical regions

*********************************************************************/

static void spiBufferComplete(
    ADI_SPI                 *pDevice,       // device we're working on
    ADI_SPI_RESERVED_AREA   *pReservedArea, // pointer to the buffer that just completed
    ADI_SPI_RESERVED_AREA   **pCurrent,     // address of the current buffer pointer
    ADI_SPI_RESERVED_AREA   **pHead,        // address of the head buffer pointer
    u32 CallbackEvent

) {

    ADI_DEV_1D_BUFFER   *pBuffer;       // pointer to a buffer
    u32                 LoopbackFlag;   // loopback status flag

    // point to the buffer
    pBuffer = (ADI_DEV_1D_BUFFER *)pReservedArea;

    // update the processed fields in the buffer
    pBuffer->ProcessedFlag = TRUE;
    pBuffer->ProcessedElementCount = pBuffer->ElementCount;

    // bump our current pointer to the next one in the queue
    *pCurrent = (ADI_SPI_RESERVED_AREA *)(((ADI_DEV_1D_BUFFER *)pReservedArea)->pNext);

    // IF (not doing loopback)
    if ((pDevice->DataflowMethod != ADI_DEV_MODE_CHAINED_LOOPBACK) && (pDevice->DataflowMethod != ADI_DEV_MODE_SEQ_CHAINED_LOOPBACK) ) {

        // clear loopback status flag
        LoopbackFlag = FALSE;

        // flush the queue if it's now empty
        if (*pCurrent == NULL) {
            *pHead = NULL;
        }

    // ELSE
    } else {

        // set loopback status flag
        LoopbackFlag = TRUE;

    // ENDIF
    }

    // post a callback if one is requested
    // (parameters are Client Handle, CallbackEvent, CallbackParameter)
    if (pBuffer->CallbackParameter != NULL) {
        (pDevice->DMCallback)(pDevice->DeviceHandle, CallbackEvent, pBuffer->CallbackParameter);
    }

    // IF (doing loopback)
    if (LoopbackFlag) {

        // reset the counters/flags etc if we're doing loopback
        pReservedArea->pCurrentDataPointer = pBuffer->Data;
        pReservedArea->BytesRemaining = pBuffer->ElementCount * pBuffer->ElementWidth;
        pBuffer->ProcessedFlag = FALSE;
        pBuffer->ProcessedElementCount = 0;

    // ENDIF
    }

    // if we had nothing on the queue reset the current pointer in case the callback added something to the queue
    if (*pCurrent == NULL) {
        *pCurrent = *pHead;
    }

    // return
}




/*********************************************************************

    Function:       InterruptHandler

    Description:    Processes events in response to SPI interrupts

*********************************************************************/

static ADI_INT_HANDLER(InterruptHandler)
{
    ADI_SPI_RESERVED_AREA   *pInboundCurrent;       // current outbound buffer
    ADI_SPI_RESERVED_AREA   *pOutboundCurrent;      // current inbound buffer
    ADI_SPI                 *pDevice;               // SPI device
    u32                     SequentialFlag;         // indicates the device is in sequential operation
    u32                     BufferCompleteFlag;     // indicates the buffer is complete
    u32                     PauseDataflowFlag;      // indicates that we need to pause dataflow
    u16                     InboundData;            // data in from register
    ADI_SPI_STATUS_REG_UNION  StatusReg;              // status register

    ADI_INT_HANDLER_RESULT Result;

    // assume the interrupt is not for us
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    // point to the device for this handler
    pDevice = (ADI_SPI *)ClientArg;

    // IF (this device is asserting an interrupt)
    if (adi_int_SICInterruptAsserted(pDevice->SpiPeripheralID) == ADI_INT_RESULT_ASSERTED) {

        // point to the current buffers being processed
        if ((pDevice->DataflowMethod == ADI_DEV_MODE_CHAINED) || (pDevice->DataflowMethod == ADI_DEV_MODE_CHAINED_LOOPBACK)) {
            pInboundCurrent = pDevice->pInboundCurrent;
            pOutboundCurrent = pDevice->pOutboundCurrent;
            SequentialFlag = FALSE;
        } else {
            if (pDevice->pInboundCurrent) {
                if (pDevice->pInboundCurrent->Direction == ADI_DEV_DIRECTION_INBOUND) {
                    pInboundCurrent = pDevice->pInboundCurrent;
                    pOutboundCurrent = NULL;
                } else {
                    pInboundCurrent = NULL;
                    pOutboundCurrent = pDevice->pInboundCurrent;
                }
            } else {
                pInboundCurrent = NULL;
                pOutboundCurrent = NULL;
            }
            SequentialFlag = TRUE;
        }

        // assume we won't complete a buffer
        BufferCompleteFlag = FALSE;

        // IF (we have a inbound buffer)
        if (pInboundCurrent) {

            // read the receive data from the shadow register
            InboundData = *pDevice->SPIShadowData;

            // save the data into the buffer
            StoreIntoBuffer (pDevice, InboundData, pInboundCurrent);

            // IF (the buffer is now complete)
            if (pInboundCurrent->BytesRemaining == 0) {

                // call buffer complete to do callbacks, update queues etc. (note sequential and normal are basically
                // the same (except for callback event) cause we use the inbound queue for sequential)
                if (SequentialFlag) {
                    spiBufferComplete(pDevice, pInboundCurrent, &pDevice->pInboundCurrent, &pDevice->pInboundHead, ADI_SPI_EVENT_SEQ_BUFFER_PROCESSED);
                } else {
                    spiBufferComplete(pDevice, pInboundCurrent, &pDevice->pInboundCurrent, &pDevice->pInboundHead, ADI_SPI_EVENT_READ_BUFFER_PROCESSED);
                }

                // update our flag
                BufferCompleteFlag = TRUE;

            // ENDIF
            }

        // ENDIF
        }

        // IF (we have a outbound buffer)
        if (pOutboundCurrent) {

            // IF (the buffer is now complete)
            if (pOutboundCurrent->BytesRemaining == 0) {

                // call buffer complete to do callbacks, update queues etc. (note sequential and normal are different cause
                // we have a different callback event and we always use the inbound queue for sequential)
                if (SequentialFlag) {
                    spiBufferComplete(pDevice, pOutboundCurrent, &pDevice->pInboundCurrent, &pDevice->pInboundHead, ADI_SPI_EVENT_SEQ_BUFFER_PROCESSED);
                } else {
                    spiBufferComplete(pDevice, pOutboundCurrent, &pDevice->pOutboundCurrent, &pDevice->pOutboundHead, ADI_SPI_EVENT_WRITE_BUFFER_PROCESSED);
                }

                // update our flag
                BufferCompleteFlag = TRUE;

            // ENDIF
            }

        // ENDIF
        }

        // IF (we completed the buffer)
        if (BufferCompleteFlag) {

            // update our current buffer pointers cause we just finished the buffer(s) we were working on
            if (SequentialFlag == FALSE) {
                pInboundCurrent = pDevice->pInboundCurrent;
                pOutboundCurrent = pDevice->pOutboundCurrent;
            } else {
                if (pDevice->pInboundCurrent) {
                    if (pDevice->pInboundCurrent->Direction == ADI_DEV_DIRECTION_INBOUND) {
                        pInboundCurrent = pDevice->pInboundCurrent;
                        pOutboundCurrent = NULL;
                    } else {
                        pInboundCurrent = NULL;
                        pOutboundCurrent = pDevice->pInboundCurrent;
                    }
                } else {
                    pInboundCurrent = NULL;
                    pOutboundCurrent = NULL;
                }
            }

        // ENDIF
        }

        // assume we're going to pause dataflow
        PauseDataflowFlag = TRUE;

        // IF (we have a buffer to receive into)
        if (pInboundCurrent) {

            // don't pause dataflow
            PauseDataflowFlag = FALSE;

        // ENDIF
        }

        // IF (we have a buffer to transmit from
        if (pOutboundCurrent) {

            // transmit from the buffer
            LoadTransmitFromBuffer (pDevice, pOutboundCurrent);

            // don't pause dataflow
            PauseDataflowFlag = FALSE;

        // ENDIF
        }

        // IF (pausing dataflow)
        if (PauseDataflowFlag) {

            // clear the SPI enable bit
            pDevice->SPIControl->BitFields.spe = ADI_SPI_CONTROL_SPE_DISABLE;

            // set dataflow status to paused
            pDevice->DataflowStatus = ADI_SPI_DATAFLOW_PAUSED;

        // ENDIF
        }

        // read the receive register to clear the interrupt
        InboundData = *pDevice->SPIReceiveData;

        // looks like the interrupt was for us
        Result = ADI_INT_RESULT_PROCESSED;

        // IF (error reporting is enabled)
        if (pDevice->ErrorReportingFlag) {

            // read the status register (need to do this as all error interrupts aren't generated by the
            // controller when not using DMA
            // Further, we should suppress TXE errors as they will occur in normal operation and would generate
            // unnecessary callbacks.  TXE errors are generated when no new data has been written to the TDBR
            // register.  We intentially do this in both normal I/O and sequential I/O when doing only inbound
            // transfers.
            StatusReg.Value16 = pDevice->SPIStatus->Value16;
            StatusReg.Value16 &= (ADI_SPI_STATUS_MASK_TXCOL | ADI_SPI_STATUS_MASK_RBSY | ADI_SPI_STATUS_MASK_MODF);

            // IF (there are errors)
            if (StatusReg.Value16 != 0) {

                // report and clear them
                spiReportAndClearErrors(pDevice, &StatusReg);

            // ENDIF
            }

        // ENDIF
        }

    // ENDIF
    }

    // return
    return (Result);
}


#endif  // interrupt driven SPI



#if defined(ADI_SPI_DMA)

/*********************************************************************

    Function:       ErrorHandler

    Description:    Processes SPI error interrupts

*********************************************************************/

static ADI_INT_HANDLER(ErrorHandler)    // DMA error handler
{

    ADI_SPI                 *pDevice;   // pointer to the device we're working on
    ADI_SPI_STATUS_REG_UNION      *StatusReg;   // spi status register
    ADI_INT_HANDLER_RESULT  Result;     // result

    // assume the interrupt is not for us
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    // point to the device for this handler
    pDevice = (ADI_SPI *)ClientArg;

    // IF (this device is asserting an interrupt)
    if (adi_int_SICInterruptAsserted(pDevice->ErrorPeripheralID) == ADI_INT_RESULT_ASSERTED) {

        // read the status register (clears the interrupt)
        StatusReg = pDevice->SPIStatus;

        // IF (there are errors)
        if ( StatusReg->BitFields.txcol || StatusReg->BitFields.rbsy
            || StatusReg->BitFields.txe || StatusReg->BitFields.modf )
        {

            // report and clear them
            spiReportAndClearErrors( pDevice, StatusReg );

         // ENDIF
         }

        // indicate that we processed the interrupt
        Result = ADI_INT_RESULT_PROCESSED;

    // ENDIF
    }

    // return
    return(Result);
}

#endif  // DMA version





#if defined(__ADSP_BRAEMAR__)

/*********************************************************************

    Function:       spiSetPortControl

    Description:    Sets port control for Braemar

*********************************************************************/

static u32 spiSetPortControl(ADI_SPI *pDevice, u32 SlaveSelectOnlyFlag) {

    u32 PossibleDirectives[] = {
        ADI_PORTS_DIR_SPI_SLAVE_SELECT_1,
        ADI_PORTS_DIR_SPI_SLAVE_SELECT_2,
        ADI_PORTS_DIR_SPI_SLAVE_SELECT_3,
        ADI_PORTS_DIR_SPI_SLAVE_SELECT_4,
        ADI_PORTS_DIR_SPI_SLAVE_SELECT_5,
        ADI_PORTS_DIR_SPI_SLAVE_SELECT_6,
        ADI_PORTS_DIR_SPI_SLAVE_SELECT_7
    };
    u32 Directives[(sizeof(PossibleDirectives)/sizeof(PossibleDirectives[0]))]; // directives to send to port control
    u32 PossibleDirectiveIndex; // index into possible directives
    u32 DirectiveCount;         // number of directives to send to port control
    u16 SlaveSelectMask;        // mask for slave select flag reister
    u16 FlagRegister;           // flag register

    // assume no directives are to be set
    DirectiveCount = 0;

    // IF (setting slave selects)
    if (SlaveSelectOnlyFlag) {

        // read in the flag register
        FlagRegister = *pDevice->SPIFlag;

        // FOR (each slave select enable)
        for (PossibleDirectiveIndex = 0, SlaveSelectMask = 0x2, DirectiveCount = 0;
            PossibleDirectiveIndex < (sizeof(PossibleDirectives)/sizeof(PossibleDirectives[0]));
            PossibleDirectiveIndex++,SlaveSelectMask <<= 1) {

            // IF (slave select is enabled)
            if (FlagRegister & SlaveSelectMask) {

                // add the directive to the list to send
                Directives[DirectiveCount] = PossibleDirectives[PossibleDirectiveIndex];
                DirectiveCount++;

            // ENDIF
            }

        // ENDFOR
        }


     // ELSE
    } else {

        // enable the base SPI pins
        Directives[0] = ADI_PORTS_DIR_SPI_BASE;
        DirectiveCount = 1;

    // ENDIF
    }

    // call port if we have any directives to set
    if (DirectiveCount) {
        adi_ports_EnableSPI(Directives, DirectiveCount,TRUE);
    }

    // return
    return ADI_DEV_RESULT_SUCCESS;
}


#endif // __ADSP_BRAEMAR__


#if defined(__ADSP_STIRLING__)

/*********************************************************************

    Function:       spiSetPortControl

    Description:    Sets port control for Stirling

*********************************************************************/

static u32 spiSetPortControl(ADI_SPI *pDevice, u32 SlaveSelectOnlyFlag) {

    ADI_PORTS_DIRECTIVE Directives1[] = {
        ADI_PORTS_DIRECTIVE_MOSI1,
        ADI_PORTS_DIRECTIVE_MISO1,
        ADI_PORTS_DIRECTIVE_SCK1,
        ADI_PORTS_DIRECTIVE_SPI1SS,
        ADI_PORTS_DIRECTIVE_SPI1SEL
    };
    ADI_PORTS_DIRECTIVE Directives2[] = {
        ADI_PORTS_DIRECTIVE_MOSI2,
        ADI_PORTS_DIRECTIVE_MISO2,
        ADI_PORTS_DIRECTIVE_SCK2,
        ADI_PORTS_DIRECTIVE_SPI2SS,
        ADI_PORTS_DIRECTIVE_SPI2SEL
    };

    u32 DirectiveCount;                 // number of directives to send to port control
    u16 FlagRegister;                   // flag register
    ADI_PORTS_DIRECTIVE *pDirectives;   // pointer to the directives

    // assume no directives are to be set
    DirectiveCount = 0;

    // validate device pointer
    if (pDevice != &Device[0] && pDevice != &Device[1] && pDevice != &Device[2])
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;

    // IF (SPI being touched is not device 0 (no port required for device 0, which has dedicated pins))
    if (pDevice != &Device[0]) {

        // point to the proper directive set
        if (pDevice == &Device[1]) {
            pDirectives = Directives1;
        } else {
            pDirectives = Directives2;
        }

        // IF (set slave selects)
        if (SlaveSelectOnlyFlag) {

            // read in the flag register
            FlagRegister = *pDevice->SPIFlag;

            // IF (the slave select is enabled)
            if (FlagRegister & 0x2) {

                // point to the proper directive and set count to 1 directive only
                pDirectives += 4;
                DirectiveCount = 1;

            // ENDIF
            }

        // ELSE
        } else {

            // enable the base SPI pins
            DirectiveCount = 4;

        // ENDIF
        }

        // IF (there are directives to set)
        if (DirectiveCount) {

            // set them
            adi_ports_Configure(pDirectives, DirectiveCount);

        // ENDIF
        }

    // ENDIF
    }

    // return
    return ADI_DEV_RESULT_SUCCESS;
}


#endif // __ADSP_STIRLING__

#if defined(__ADSP_MOAB__)  /* Static function for Moab class devices */

/*********************************************************************

    Function:       spiSetPortControl

    Description:    Sets port control for Moab

*********************************************************************/

static u32 spiSetPortControl(ADI_SPI *pDevice, u32 SlaveSelectOnlyFlag) {

    /* SPI 0 Peripheral Directives */
    ADI_PORTS_DIRECTIVE SPI0Directives[] = {
        ADI_PORTS_DIRECTIVE_SPI0_SCK,
        ADI_PORTS_DIRECTIVE_SPI0_MISO,
        ADI_PORTS_DIRECTIVE_SPI0_MOSI,
        ADI_PORTS_DIRECTIVE_SPI0_SS,
        ADI_PORTS_DIRECTIVE_SPI0_SEL1,
        ADI_PORTS_DIRECTIVE_SPI0_SEL2,
        ADI_PORTS_DIRECTIVE_SPI0_SEL3
    };

    /* SPI 1 Peripheral Directives */
    ADI_PORTS_DIRECTIVE SPI1Directives[] = {
        ADI_PORTS_DIRECTIVE_SPI1_SCK,
        ADI_PORTS_DIRECTIVE_SPI1_MISO,
        ADI_PORTS_DIRECTIVE_SPI1_MOSI,
        ADI_PORTS_DIRECTIVE_SPI1_SS,
        ADI_PORTS_DIRECTIVE_SPI1_SEL1,
        ADI_PORTS_DIRECTIVE_SPI1_SEL2,
        ADI_PORTS_DIRECTIVE_SPI1_SEL3
    };

    /* SPI 2 Peripheral Directives */
    ADI_PORTS_DIRECTIVE SPI2Directives[] = {
        ADI_PORTS_DIRECTIVE_SPI2_SCK,
        ADI_PORTS_DIRECTIVE_SPI2_MISO,
        ADI_PORTS_DIRECTIVE_SPI2_MOSI,
        ADI_PORTS_DIRECTIVE_SPI2_SS,
        ADI_PORTS_DIRECTIVE_SPI2_SEL1,
        ADI_PORTS_DIRECTIVE_SPI2_SEL2,
        ADI_PORTS_DIRECTIVE_SPI2_SEL3
    };

    /* directives to send to port control */
    ADI_PORTS_DIRECTIVE Directives[(sizeof(SPI0Directives)/sizeof(SPI0Directives[0]))];
    u32 PossibleDirectiveIndex;     /* index into possible directives */
    u32 DirectiveCount;             /* number of directives to send to port control */
    u16 SlaveSelectMask;            /* mask for slave select flag reister */
    u16 FlagRegister;               /* flag register */
    ADI_PORTS_DIRECTIVE *pDirectives;   /* pointer to the directives */

    /* assume no directives are to be set */
    DirectiveCount = 0;
    pDirectives = NULL;

    // validate device pointer
    if (pDevice != &Device[0] && pDevice != &Device[1] && pDevice != &Device[2])
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;

    /* point to the proper directive set */
    /* IF (the device we're working on is SPI 0) */
    if (pDevice == &Device[0]) {
        pDirectives = SPI0Directives;
    } else {
        if (pDevice == &Device[1]) {
            pDirectives = SPI1Directives;
        } else {
            pDirectives = SPI2Directives;
        }
    }

    /* IF (set slave selects) */
    if (SlaveSelectOnlyFlag) {

        /* read in the flag register */
        FlagRegister = *pDevice->SPIFlag;

        /* FOR (each slave select enable) */
        for (PossibleDirectiveIndex = 4, SlaveSelectMask = 0x2, DirectiveCount = 0;
            PossibleDirectiveIndex < (sizeof(SPI0Directives)/sizeof(SPI0Directives[0]));
            PossibleDirectiveIndex++,SlaveSelectMask <<= 1)
        {

            /* IF (slave select is enabled) */
            if (FlagRegister & SlaveSelectMask)
            {
                /* add the directive to the list to send */
                Directives[DirectiveCount] = *(pDirectives+PossibleDirectiveIndex);
                DirectiveCount++;
            }

        /* ENDFOR */
        }

        /* update the pointer to directive set to pass */
        pDirectives = Directives;

    /* ELSE */
    } else {

        /* enable the base SPI pins */
        DirectiveCount = 3;
        /* SPI slave select (PSSE) enabled? */
        if (pDevice->SPIControl->BitFields.psse){
            DirectiveCount++;   /* include SPI slave select directive */
        }

    /* ENDIF */
    }

    /* IF (there are directives to set) */
    if (DirectiveCount) {

        /* pass them to Port control */
        adi_ports_Configure(pDirectives, DirectiveCount);

    /* ENDIF */
    }

    /* return */
    return ADI_DEV_RESULT_SUCCESS;
}

#endif /* __ADSP_MOAB__ */


#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)  /* Static function for Kookaburra/Mockingbird class devices */

/*********************************************************************

    Function:       spiSetPortControl

    Description:    Sets port control for Kookaburra and Mockingbird

*********************************************************************/

static u32 spiSetPortControl(ADI_SPI *pDevice, u32 SlaveSelectOnlyFlag) {

    // NOTE: Kookaburra/Mockingbird offers choice of SPI control of the singular
    // SPI device over two MUX choices on PORTG.  So we have a virtual
    // "second" SPI device.  First-come, first-serve...

    /* SPI Peripheral Directives */
    ADI_PORTS_DIRECTIVE SPIMux0Directives[] = {
        // This "device" configures controls on MUX0
        ADI_PORTS_DIRECTIVE_SPI_SCK_MUX0,
        ADI_PORTS_DIRECTIVE_SPI_MISO_MUX0,
        ADI_PORTS_DIRECTIVE_SPI_MOSI_MUX0,
        ADI_PORTS_DIRECTIVE_SPI_SS,
        ADI_PORTS_DIRECTIVE_SPI_SEL1,
        ADI_PORTS_DIRECTIVE_SPI_SEL2,
        ADI_PORTS_DIRECTIVE_SPI_SEL3,
        ADI_PORTS_DIRECTIVE_SPI_SEL4,
        ADI_PORTS_DIRECTIVE_SPI_SEL5,
        ADI_PORTS_DIRECTIVE_SPI_SEL6,
        ADI_PORTS_DIRECTIVE_SPI_SEL7,
    };

    ADI_PORTS_DIRECTIVE SPIMux2Directives[] = {
        // This "device" configures controls on MUX2
        ADI_PORTS_DIRECTIVE_SPI_SCK_MUX2,
        ADI_PORTS_DIRECTIVE_SPI_MISO_MUX2,
        ADI_PORTS_DIRECTIVE_SPI_MOSI_MUX2,
        ADI_PORTS_DIRECTIVE_SPI_SS,
        ADI_PORTS_DIRECTIVE_SPI_SEL1,
        ADI_PORTS_DIRECTIVE_SPI_SEL2,
        ADI_PORTS_DIRECTIVE_SPI_SEL3,
        ADI_PORTS_DIRECTIVE_SPI_SEL4,
        ADI_PORTS_DIRECTIVE_SPI_SEL5,
        ADI_PORTS_DIRECTIVE_SPI_SEL6,
        ADI_PORTS_DIRECTIVE_SPI_SEL7,
    };

    /* directives to send to port control */
    ADI_PORTS_DIRECTIVE Directives[(sizeof(SPIMux0Directives)/sizeof(SPIMux0Directives[0]))];
    u32 PossibleDirectiveIndex;     /* index into possible directives */
    u32 DirectiveCount;             /* number of directives to send to port control */
    u16 SlaveSelectMask;            /* mask for slave select flag reister */
    u16 FlagRegister;               /* flag register */
    ADI_PORTS_DIRECTIVE *pDirectives;   /* pointer to the directives */

    /* assume no directives are to be set */
    DirectiveCount = 0;
    pDirectives = NULL;

    // validate device pointer
    if (pDevice != &Device[0] && pDevice != &Device[1])
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;

    /* point to the proper directive set */
    /* IF (the device we're working on is SPI 0) */
    if (pDevice == &Device[0]) {
        pDirectives = SPIMux0Directives;
    } else {
        pDirectives = SPIMux2Directives;
    }

    /* IF (set slave selects) */
    if (SlaveSelectOnlyFlag) {

        /* read in the flag register */
        FlagRegister = *pDevice->SPIFlag;

        /* FOR (each slave select enable) */
        for (PossibleDirectiveIndex = 4, SlaveSelectMask = 0x2, DirectiveCount = 0;
            PossibleDirectiveIndex < (sizeof(SPIMux0Directives)/sizeof(SPIMux0Directives[0]));
            PossibleDirectiveIndex++,SlaveSelectMask <<= 1)
        {

            /* IF (slave select is enabled) */
            if (FlagRegister & SlaveSelectMask)
            {
                /* add the directive to the list to send */
                Directives[DirectiveCount] = *(pDirectives+PossibleDirectiveIndex);
                DirectiveCount++;
            }

        /* ENDFOR */
        }

        /* update the pointer to directive set to pass */
        pDirectives = Directives;

    /* ELSE */
    } else {

        /* enable the base SPI pins */
        DirectiveCount = 3;
        /* SPI slave select (PSSE) enabled? */
        if (pDevice->SPIControl->BitFields.psse){
            DirectiveCount++;   /* include SPI slave select directive */
        }

    /* ENDIF */
    }

    /* IF (there are directives to set) */
    if (DirectiveCount) {

        /* pass them to Port control */
        adi_ports_Configure(pDirectives, DirectiveCount);

    /* ENDIF */
    }

    /* return */
    return ADI_DEV_RESULT_SUCCESS;
}

#endif // end Kookaburra/Mockingbird


#if defined(__ADSP_BRODIE__)  /* Static function for Brodie class devices */

/*********************************************************************

    Function:       spiSetPortControl

    Description:    Sets port control for Brodie

*********************************************************************/

static u32 spiSetPortControl(ADI_SPI *pDevice, u32 SlaveSelectOnlyFlag) {

    /* Brodie only uses sel1:sel5 with SPI0 missing sel4! */

    /* SPI Peripheral Directives */
    ADI_PORTS_DIRECTIVE SPI0Directives[] = {
        // This "device" configures controls on SPI0
        ADI_PORTS_DIRECTIVE_SPI0_SCK,
        ADI_PORTS_DIRECTIVE_SPI0_MISO,
        ADI_PORTS_DIRECTIVE_SPI0_MOSI,
        ADI_PORTS_DIRECTIVE_SPI0_SS,
        ADI_PORTS_DIRECTIVE_SPI0_SEL1,
        ADI_PORTS_DIRECTIVE_SPI0_SEL2,
        ADI_PORTS_DIRECTIVE_SPI0_SEL3,
      /*ADI_PORTS_DIRECTIVE_SPI0_SEL4,  !reserved! */
        ADI_PORTS_DIRECTIVE_SPI0_SEL5,
        (ADI_PORTS_DIRECTIVE)0,  /* filler to make up for missing sel4 */
    };

    ADI_PORTS_DIRECTIVE SPI1Directives[] = {
        // This "device" configures controls on SPI1
        ADI_PORTS_DIRECTIVE_SPI1_SCK,
        ADI_PORTS_DIRECTIVE_SPI1_MISO,
        ADI_PORTS_DIRECTIVE_SPI1_MOSI,
        ADI_PORTS_DIRECTIVE_SPI1_SS,
        ADI_PORTS_DIRECTIVE_SPI1_SEL1,
        ADI_PORTS_DIRECTIVE_SPI1_SEL2,
        ADI_PORTS_DIRECTIVE_SPI1_SEL3,
        ADI_PORTS_DIRECTIVE_SPI1_SEL4,
        ADI_PORTS_DIRECTIVE_SPI1_SEL5,
    };

    /* directives to send to port control */
    ADI_PORTS_DIRECTIVE Directives[(sizeof(SPI0Directives)/sizeof(SPI0Directives[0]))];
    u32 PossibleDirectiveIndex;     /* index into possible directives */
    u32 DirectiveCount;             /* number of directives to send to port control */
    u16 SlaveSelectMask;            /* mask for slave select flag reister */
    u16 FlagRegister;               /* flag register */
    ADI_PORTS_DIRECTIVE *pDirectives;   /* pointer to the directives */

    /* assume no directives are to be set */
    DirectiveCount = 0;
    pDirectives = NULL;

    // validate device pointer
    if (pDevice != &Device[0] && pDevice != &Device[1])
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;

    /* point to the proper directive set */
    /* IF (the device we're working on is SPI 0) */
    if (pDevice == &Device[0]) {
        pDirectives = SPI0Directives;
    } else {
        pDirectives = SPI1Directives;
    }

    /* IF (set slave selects) */
    if (SlaveSelectOnlyFlag) {

        /* read in the flag register */
        FlagRegister = *pDevice->SPIFlag;

        /* FOR (each slave select enable) */
        for (PossibleDirectiveIndex = 4, SlaveSelectMask = 0x2, DirectiveCount = 0;
            //PossibleDirectiveIndex < (sizeof(SPI0Directives)/sizeof(SPI0Directives[0]) && 0 != pDirectives[PossibleDirectiveIndex]); /* watch for gap from missing sel4 */
            PossibleDirectiveIndex < (sizeof(SPI1Directives)/sizeof(SPI1Directives[0]));
            PossibleDirectiveIndex++,SlaveSelectMask <<= 1)
        {

            /* IF (slave select is enabled) */
            if (FlagRegister & SlaveSelectMask)
            {
                /* add the directive to the list to send */
                Directives[DirectiveCount] = *(pDirectives+PossibleDirectiveIndex);
                DirectiveCount++;
            }

        /* ENDFOR */
        }

        /* update the pointer to directive set to pass */
        pDirectives = Directives;

    /* ELSE */
    } else {

        /* enable the base SPI pins */
         DirectiveCount = 3;
        /* SPI slave select (PSSE) enabled? */
        if (pDevice->SPIControl->BitFields.psse){
            DirectiveCount++;   /* include SPI slave select directive */
        }

    /* ENDIF */
    }

    /* IF (there are directives to set) */
    if (DirectiveCount) {

        /* pass them to Port control */
        adi_ports_Configure(pDirectives, DirectiveCount);

    /* ENDIF */
    }

    /* return */
    return ADI_DEV_RESULT_SUCCESS;
}

#endif // end Brodie

#if defined(__ADSP_MOY__)  /* Static function for Moy class devices */

/*********************************************************************

    Function:       spiSetPortControl

    Description:    Sets port control for Moy

*********************************************************************/

static u32 spiSetPortControl(
    ADI_SPI     *pDevice,
    u32         SlaveSelectOnlyFlag
)
{
    /* SPI 0 Peripheral Directives */
    ADI_PORTS_DIRECTIVE     aeSPI0Directives[] =
    {
        ADI_PORTS_DIRECTIVE_SPI0_SCK,
        ADI_PORTS_DIRECTIVE_SPI0_MISO,
        ADI_PORTS_DIRECTIVE_SPI0_MOSI,
        ADI_PORTS_DIRECTIVE_SPI0_SEL1_SS,
        ADI_PORTS_DIRECTIVE_SPI0_SEL2,
        ADI_PORTS_DIRECTIVE_SPI0_SEL3_PF15
    };

    /* SPI 1 Peripheral Directives */
    ADI_PORTS_DIRECTIVE     aeSPI1Directives[] =
    {
        ADI_PORTS_DIRECTIVE_SPI1_SCK,
        ADI_PORTS_DIRECTIVE_SPI1_MISO,
        ADI_PORTS_DIRECTIVE_SPI1_MOSI,
        ADI_PORTS_DIRECTIVE_SPI1_SEL1_SS,
        ADI_PORTS_DIRECTIVE_SPI1_SEL2_PG1,
        ADI_PORTS_DIRECTIVE_SPI1_SEL3_PG0
    };

    /* index into possible directives */
    u8                      nPossibleDirectiveIndex;
    /* mask for slave select flag reister */
    u16                     nSlaveSelectMask;
    /* flag register */
    u16                     nFlagRegister;
    /* number of directives to send to port control */
    u32                     nDirectiveCount;
    /* directives to send to port control */
    ADI_PORTS_DIRECTIVE     aeSPIDirectives[(sizeof(aeSPI0Directives)/sizeof(aeSPI0Directives[0]))];
    /* pointer to the directives */
    ADI_PORTS_DIRECTIVE     *pDirectives;

    /* assume no directives are to be set */
    nDirectiveCount = 0;
    pDirectives = NULL;

    /* point to the proper directive set */
    /* IF (the device we're working on is SPI 0) */
    if (pDevice == &Device[0])
    {
        pDirectives = aeSPI0Directives;
    }
    /* ELSE IF (work with SPI 1) */
    else if (pDevice == &Device[1])
    {
        pDirectives = aeSPI1Directives;
    }
    /* ELSE (Device must be invalid) */
    else
    {
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }

    /* IF (set slave selects) */
    if (SlaveSelectOnlyFlag)
    {
        /* IF (the device we're working on is SPI 0) */
        if (pDevice == &Device[0])
        {
            /* IF (Pin Mux Mode is to set PF15 as SPI0 SSEL3) */
            if ((pDevice->ePinMuxMode == ADI_SPI_PIN_MUX_MODE_0) ||\
                (pDevice->ePinMuxMode == ADI_SPI_PIN_MUX_MODE_2))
            {
                /* Update directives table */
                aeSPI0Directives[5] = ADI_PORTS_DIRECTIVE_SPI0_SEL3_PF15;
            }
            /* ELSE (Pin Mux Mode is to set PH0 as SPI0 SSEL3) */
            else
            {
                /* Update directives table */
                aeSPI0Directives[5] = ADI_PORTS_DIRECTIVE_SPI0_SEL3_PH0;
            }
        }
        /* ELSE (the device we're working on is SPI 1) */
        else
        {
            /* CASE OF (Pin Mux Mode) */
            switch (pDevice->ePinMuxMode)
            {
                /* CASE (Mode 1) */
                case (ADI_SPI_PIN_MUX_MODE_1):
                    /* PG1 as Slave Select 2 (SPI1 SSEL2) */
                    aeSPI1Directives[4] = ADI_PORTS_DIRECTIVE_SPI1_SEL2_PG1;
                    /* PH1 as Slave Select 3 (SPI1 SSEL3) */
                    aeSPI1Directives[5] = ADI_PORTS_DIRECTIVE_SPI1_SEL3_PH1;
                    break;

                /* CASE (Mode 2) */
                case (ADI_SPI_PIN_MUX_MODE_2):
                    /* PH2 as Slave Select 2 (SPI1 SSEL2) */
                    aeSPI1Directives[4] = ADI_PORTS_DIRECTIVE_SPI1_SEL2_PH2;
                    /* PG0 as Slave Select 3 (SPI1 SSEL3) */
                    aeSPI1Directives[5] = ADI_PORTS_DIRECTIVE_SPI1_SEL3_PG0;
                    break;

                /* CASE (Mode 3) */
                case (ADI_SPI_PIN_MUX_MODE_3):
                    /* PH2 as Slave Select 2 (SPI1 SSEL2) */
                    aeSPI1Directives[4] = ADI_PORTS_DIRECTIVE_SPI1_SEL2_PH2;
                    /* PH1 as Slave Select 3 (SPI1 SSEL3) */
                    aeSPI1Directives[5] = ADI_PORTS_DIRECTIVE_SPI1_SEL3_PH1;
                    break;

                /* default (Mode 0) */
                default:
                    /* PG1 as Slave Select 2 (SPI1 SSEL2) */
                    aeSPI1Directives[4] = ADI_PORTS_DIRECTIVE_SPI1_SEL2_PG1;
                    /* PG0 as Slave Select 3 (SPI1 SSEL3) */
                    aeSPI1Directives[5] = ADI_PORTS_DIRECTIVE_SPI1_SEL3_PG0;
                    break;
            }
        }

        /* read in the flag register */
        nFlagRegister = *pDevice->SPIFlag;

        /* FOR (each slave select enable) */
        for (nPossibleDirectiveIndex = 3, nSlaveSelectMask = 0x2, nDirectiveCount = 0;
             nPossibleDirectiveIndex < (sizeof(aeSPI0Directives)/sizeof(aeSPI0Directives[0]));
             nPossibleDirectiveIndex++, nSlaveSelectMask <<= 1)
        {
            /* IF (Slave Select is enabled) */
            if (nFlagRegister & nSlaveSelectMask)
            {
                /* Call Port control to update pin mux configuration */
                adi_ports_Configure((pDirectives + nPossibleDirectiveIndex), 1);
            }
        }
    }
    /* ELSE (Enable base SPI pins) */
    else
    {
        /* Enable the base SPI pins */
        nDirectiveCount = 3;

        /* IF (SPI slave select (PSSE) enabled?) */
        if (pDevice->SPIControl->BitFields.psse)
        {
            /* include SPI slave select directive */
            nDirectiveCount++;
        }

        /* Pass directives to Port control */
        adi_ports_Configure(pDirectives, nDirectiveCount);
    }

    /* return */
    return ADI_DEV_RESULT_SUCCESS;
}

#endif /* End Moy */


#if defined(__ADSP_DELTA__)  /* Static function for Moy class devices */

/*********************************************************************

    Function:       spiSetPortControl

    Description:    Sets port control for Delta

*********************************************************************/

static u32 spiSetPortControl(
    ADI_SPI     *pDevice,
    u32         SlaveSelectOnlyFlag
)
{
    /* SPI 0 Peripheral Directives */
    ADI_PORTS_DIRECTIVE     aeSPI0Directives[] =
    {
        ADI_PORTS_DIRECTIVE_SPI0_SCK,
        ADI_PORTS_DIRECTIVE_SPI0_MISO,
        ADI_PORTS_DIRECTIVE_SPI0_MOSI,
        ADI_PORTS_DIRECTIVE_SPI0_SEL1_SS,
        ADI_PORTS_DIRECTIVE_SPI0_SEL2,
        ADI_PORTS_DIRECTIVE_SPI0_SEL3,
        ADI_PORTS_DIRECTIVE_SPI0_SEL4,
        ADI_PORTS_DIRECTIVE_SPI0_SEL5,
        ADI_PORTS_DIRECTIVE_SPI0_SEL6,
        ADI_PORTS_DIRECTIVE_SPI0_SEL7

    };

    /* SPI 1 Peripheral Directives */
    ADI_PORTS_DIRECTIVE     aeSPI1Directives[] =
    {
        ADI_PORTS_DIRECTIVE_SPI1_SCK,
        ADI_PORTS_DIRECTIVE_SPI1_MISO,
        ADI_PORTS_DIRECTIVE_SPI1_MOSI,
        ADI_PORTS_DIRECTIVE_SPI1_SEL1_SS,
        ADI_PORTS_DIRECTIVE_SPI1_SEL2,
        ADI_PORTS_DIRECTIVE_SPI1_SEL3,
        ADI_PORTS_DIRECTIVE_SPI1_SEL4_PF14,
        ADI_PORTS_DIRECTIVE_SPI1_SEL5_PF15,
        ADI_PORTS_DIRECTIVE_SPI1_SEL6_PG5,
        ADI_PORTS_DIRECTIVE_SPI1_SEL7

    };

    /* index into possible directives */
    u8                      nPossibleDirectiveIndex;
    /* mask for slave select flag reister */
    u16                     nSlaveSelectMask;
    /* flag register */
    u16                     nFlagRegister;
    /* number of directives to send to port control */
    u32                     nDirectiveCount;
    /* directives to send to port control */
    ADI_PORTS_DIRECTIVE     aeSPIDirectives[(sizeof(aeSPI0Directives)/sizeof(aeSPI0Directives[0]))];
    /* pointer to the directives */
    ADI_PORTS_DIRECTIVE     *pDirectives;

    /* assume no directives are to be set */
    nDirectiveCount = 0;
    pDirectives = NULL;

    /* point to the proper directive set */
    /* IF (the device we're working on is SPI 0) */
    if (pDevice == &Device[0])
    {
        pDirectives = aeSPI0Directives;
    }
    /* ELSE IF (work with SPI 1) */
    else if (pDevice == &Device[1])
    {
        pDirectives = aeSPI1Directives;
    }
    /* ELSE (Device must be invalid) */
    else
    {
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }

    /* IF (set slave selects) */
    if (SlaveSelectOnlyFlag)
    {
        /* IF (the device we're working on is SPI 0) */
        if (pDevice == &Device[1])
        {
            /* CASE OF (Pin Mux Mode) */
            switch (pDevice->ePinMuxMode)
            {
                /* CASE (Mode 1) */
                case (ADI_SPI_PIN_MUX_MODE_1):
                    /* PF14 as Slave Select 4 (SPI1 SSEL4) */
                    aeSPI1Directives[6] = ADI_PORTS_DIRECTIVE_SPI1_SEL4_PF14;
                    /* PH15 as Slave Select 5 (SPI1 SSEL5) */
                    aeSPI1Directives[7] = ADI_PORTS_DIRECTIVE_SPI1_SEL5_PF15;
                    /* PG15 as Slave Select 6 (SPI1 SSEL6) */
                    aeSPI1Directives[8] = ADI_PORTS_DIRECTIVE_SPI1_SEL6_PG15;
                    break;

                /* CASE (Mode 2) */
                case (ADI_SPI_PIN_MUX_MODE_2):
                    /* PF14 as Slave Select 4 (SPI1 SSEL4) */
                    aeSPI1Directives[6] = ADI_PORTS_DIRECTIVE_SPI1_SEL4_PF14;
                    /* PG11 as Slave Select 5 (SPI1 SSEL5) */
                    aeSPI1Directives[7] = ADI_PORTS_DIRECTIVE_SPI1_SEL5_PG11;
                    /* PG5  as Slave Select 6 (SPI1 SSEL6) */
                    aeSPI1Directives[8] = ADI_PORTS_DIRECTIVE_SPI1_SEL6_PG5;
                  break;

                /* CASE (Mode 3) */
                case (ADI_SPI_PIN_MUX_MODE_3):
                    /* PF14 as Slave Select 4 (SPI1 SSEL4) */
                    aeSPI1Directives[6] = ADI_PORTS_DIRECTIVE_SPI1_SEL4_PF14;
                    /* PG11 as Slave Select 5 (SPI1 SSEL5) */
                    aeSPI1Directives[7] = ADI_PORTS_DIRECTIVE_SPI1_SEL5_PG11;
                    /* PG15 as Slave Select 6 (SPI1 SSEL6) */
                    aeSPI1Directives[8] = ADI_PORTS_DIRECTIVE_SPI1_SEL6_PG15;
                    break;

                /* CASE (Mode 4) */
                case (ADI_SPI_PIN_MUX_MODE_4):
                    /* PG14 as Slave Select 4 (SPI1 SSEL4) */
                    aeSPI1Directives[6] = ADI_PORTS_DIRECTIVE_SPI1_SEL4_PG14;
                    /* PF15 as Slave Select 5 (SPI1 SSEL5) */
                    aeSPI1Directives[7] = ADI_PORTS_DIRECTIVE_SPI1_SEL5_PF15;
                    /* PG5 as Slave Select 6 (SPI1 SSEL6) */
                    aeSPI1Directives[8] = ADI_PORTS_DIRECTIVE_SPI1_SEL6_PG5;
                    break;

                /* CASE (Mode 5) */
                case (ADI_SPI_PIN_MUX_MODE_5):
                    /* PG14 as Slave Select 4 (SPI1 SSEL4) */
                    aeSPI1Directives[6] = ADI_PORTS_DIRECTIVE_SPI1_SEL4_PG14;
                    /* PF15 as Slave Select 5 (SPI1 SSEL5) */
                    aeSPI1Directives[7] = ADI_PORTS_DIRECTIVE_SPI1_SEL5_PF15;
                    /* PG15 as Slave Select 6 (SPI1 SSEL6) */
                    aeSPI1Directives[8] = ADI_PORTS_DIRECTIVE_SPI1_SEL6_PG15;
                    break;

                /* CASE (Mode 6) */
                case (ADI_SPI_PIN_MUX_MODE_6):
                    /* PG14 as Slave Select 4 (SPI1 SSEL4) */
                    aeSPI1Directives[6] = ADI_PORTS_DIRECTIVE_SPI1_SEL4_PG14;
                    /* PG11 as Slave Select 5 (SPI1 SSEL5) */
                    aeSPI1Directives[7] = ADI_PORTS_DIRECTIVE_SPI1_SEL5_PG11;
                    /* PG5 as Slave Select 6 (SPI1 SSEL6) */
                    aeSPI1Directives[8] = ADI_PORTS_DIRECTIVE_SPI1_SEL6_PG5;
                    break;

                /* CASE (Mode 7) */
                case (ADI_SPI_PIN_MUX_MODE_7):
                    /* PG14 as Slave Select 4 (SPI1 SSEL4) */
                    aeSPI1Directives[6] = ADI_PORTS_DIRECTIVE_SPI1_SEL4_PG14;
                    /* PG11 as Slave Select 5 (SPI1 SSEL5) */
                    aeSPI1Directives[7] = ADI_PORTS_DIRECTIVE_SPI1_SEL5_PG11;
                    /* PG15 as Slave Select 6 (SPI1 SSEL6) */
                    aeSPI1Directives[8] = ADI_PORTS_DIRECTIVE_SPI1_SEL6_PG15;

                    break;

                /* default (Mode 0) */
                default:
                    /* PF14 as Slave Select 4 (SPI1 SSEL4) */
                    aeSPI1Directives[6] = ADI_PORTS_DIRECTIVE_SPI1_SEL4_PF14;
                    /* PH15 as Slave Select 5 (SPI1 SSEL5) */
                    aeSPI1Directives[7] = ADI_PORTS_DIRECTIVE_SPI1_SEL5_PF15;
                    /* PG5  as Slave Select 6 (SPI1 SSEL6) */
                    aeSPI1Directives[8] = ADI_PORTS_DIRECTIVE_SPI1_SEL6_PG5;
                    break;
            }
        }


        /* read in the flag register */
        nFlagRegister = *pDevice->SPIFlag;

        /* FOR (each slave select enable) */
        for (nPossibleDirectiveIndex = 3, nSlaveSelectMask = 0x2, nDirectiveCount = 0;
             nPossibleDirectiveIndex < (sizeof(aeSPI0Directives)/sizeof(aeSPI0Directives[0]));
             nPossibleDirectiveIndex++, nSlaveSelectMask <<= 1)
        {
            /* IF (Slave Select is enabled) */
            if (nFlagRegister & nSlaveSelectMask)
            {
                /* Call Port control to update pin mux configuration */
                adi_ports_Configure((pDirectives + nPossibleDirectiveIndex), 1);
            }
        }
    }
    /* ELSE (Enable base SPI pins) */
    else
    {
        /* Enable the base SPI pins */
        nDirectiveCount = 3;

        /* IF (SPI slave select (PSSE) enabled?) */
        if (pDevice->SPIControl->BitFields.psse)
        {
            /* include SPI slave select directive */
            nDirectiveCount++;
        }

        /* Pass directives to Port control */
        adi_ports_Configure(pDirectives, nDirectiveCount);
    }

    /* return */
    return ADI_DEV_RESULT_SUCCESS;
}

#endif /* End Delta */


#if defined(ADI_DEV_DEBUG)

/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {
    unsigned int i;
    for (i = 0; i < ADI_SPI_NUM_DEVICES; i++) {
        if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
            return (ADI_DEV_RESULT_SUCCESS);
        }
    }
    return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
    }


#endif


