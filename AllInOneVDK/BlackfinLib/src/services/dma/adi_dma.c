/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_dma.c,v $
$Revision: 2386 $
$Date: 2010-03-25 17:26:08 -0400 (Thu, 25 Mar 2010) $

Description:
            This is the main source file for the DMA Manager.

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/


#include <services\services.h>      // system service includes


/*********************************************************************

Data structure and bitfield enumerations for IRQ_STATUS register

*********************************************************************/


typedef enum ADI_DMA_RUN {
    ADI_DMA_RUN_DISABLED,
    ADI_DMA_RUN_ENABLED
} ADI_DMA_RUN;

typedef enum ADI_DMA_DFETCH {
    ADI_DMA_DFETCH_DISABLED,
    ADI_DMA_DFETCH_ENABLED
} ADI_DMA_DFETCH;

typedef enum ADI_DMA_ERR {
    ADI_DMA_ERR_NO_ERROR,
    ADI_DMA_ERR_ERROR
} ADI_DMA_ERR;

typedef enum ADI_DMA_DONE {
    ADI_DMA_DONE_NOT_DONE,
    ADI_DMA_DONE_DONE
} ADI_DMA_DONE;


#pragma pack(2)
typedef struct ADI_DMA_IRQ_STATUS_REG
{
    volatile u16 b_DMA_DONE:1;      //0     Completion
    volatile u16 b_DMA_ERR: 1;      //1     Error
    volatile u16 b_DFETCH:  1;      //2     Descriptor fetch
    volatile u16 b_DMA_RUN: 1;      //3     Running
} ADI_DMA_IRQ_STATUS_REG;
#pragma pack()




/*********************************************************************

Data structures for Traffic Control Period and Counter registers

*********************************************************************/


#pragma pack(2)
typedef struct ADI_DMA_TC_PER_REG {
    volatile u16 b_DCB_TRAFFIC_PERIOD:      4;  // bits 0-3     DCB period
    volatile u16 b_DEB_TRAFFIC_PERIOD:      4;  // bits 4-7     DEB period
    volatile u16 b_DAB_TRAFFIC_PERIOD:      3;  // bits 8-10    DAB period
    volatile u16 b_MDMA_ROUND_ROBIN_PERIOD: 5;  // bits 11-15   MDMA round robin period
} ADI_DMA_TC_PER_REG;
#pragma pack()


#pragma pack(2)
typedef struct ADI_DMA_TC_CNT_REG {
    volatile u16 b_DCB_TRAFFIC_COUNT:       4;  // bits 0-3     DCB count
    volatile u16 b_DEB_TRAFFIC_COUNT:       4;  // bits 4-7     DEB count
    volatile u16 b_DAB_TRAFFIC_COUNT:       3;  // bits 8-10    DAB count
    volatile u16 b_MDMA_ROUND_ROBIN_COUNT:  5;  // bits 11-15   MDMA round robin count
} ADI_DMA_TC_CNT_REG;
#pragma pack()




/*********************************************************************

DMA channel status

*********************************************************************/

typedef enum {                      // Data transfer mode
    ADI_DMA_STATUS_STOPPED,         // stopped  (not enabled)
    ADI_DMA_STATUS_PAUSED,          // paused   (enabled but no data to process)
    ADI_DMA_STATUS_RUNNING,         // running  (enabled and processing data)
} ADI_DMA_STATUS;


/*********************************************************************

DMA channel types

*********************************************************************/

typedef enum {                      // Channel type
    ADI_DMA_TYPE_UNKNOWN,               // unknown type
    ADI_DMA_TYPE_PERIPHERAL,            // peripheral DMA
    ADI_DMA_TYPE_MEMORY_SOURCE,         // memory DMA source channel
    ADI_DMA_TYPE_MEMORY_DESTINATION     // memory DMA destination channel
} ADI_DMA_CHANNEL_TYPE;


/*********************************************************************

Data Structures

*********************************************************************/


typedef struct ADI_DMA_CHANNEL {                // DMA Channel
    ADI_DMA_CONFIG_REG          *Config;            // configuration register
    ADI_DMA_DESCRIPTOR_UNION    **NextDescPtr;      // next descriptor pointer
    volatile void               **StartAddr;        // start address
    volatile u16                *XCount;            // XCOUNT
    volatile u16                *YCount;            // YCOUNT
    volatile s16                *XModify;           // XMODIFY
    volatile s16                *YModify;           // YMODIFY
    volatile void               *CurrAddr;          // current address
    volatile u16                *CurrXCount;        // current XCOUNT
    volatile u16                *CurrYCount;        // current YCOUNT
    ADI_DMA_IRQ_STATUS_REG      *IRQStatus;         // IRQ_STATUS
    volatile u16                *PeripheralMap;     // peripheral map

    struct ADI_DMA_MANAGER      *pManager;          // DMA manager this channel belongs to
    ADI_DMA_CHANNEL_ID          ChannelID;          // channel ID
    ADI_INT_PERIPHERAL_ID       PeripheralID;       // peripheral ID
    ADI_DMA_MODE                Mode;               // mode of operation
    ADI_DMA_STATUS              Status;             // channel status
    u8                          InUseFlag;          // channel open flag
    u8                          LoopbackFlag;       // loopback flag / flag indicating memory DMA is active
    u8                          StreamingFlag;      // streaming flag / flag indicating memory DMA
    u8                          BufferReadyFlag;    // single/circular buffer ready flag
    ADI_DMA_CHANNEL_TYPE        ChannelType;        // channel type
    struct ADI_DMA_CHANNEL      *pPairingChannel;   // source or dest channel ID this channel is paired with
    void                        *ClientHandle;      // client handle
    ADI_DMA_DESCRIPTOR_UNION    *pNextCallback;     // next descriptor that will trigger a callback
    ADI_DMA_DESCRIPTOR_UNION    *pActiveHead;       // head of live queue
    ADI_DMA_DESCRIPTOR_UNION    *pActiveTail;       // tail of live queue
    ADI_DMA_DESCRIPTOR_UNION    *pStandbyHead;      // head of standby queue
    ADI_DMA_DESCRIPTOR_UNION    *pStandbyTail;      // tail of standby queue
    ADI_DCB_HANDLE              DCBHandle;          // DCB service handle
    ADI_DCB_CALLBACK_FN         ClientCallback;     // client callback
} ADI_DMA_CHANNEL;


typedef struct ADI_DMA_MANAGER {    // DMA manager instance data
    u32             ChannelCount;       // number of channels in array
    void            *pEnterCriticalArg; // parameter for enter critical region
    ADI_DMA_CHANNEL *Channel;           // pointer to an array of channel entries
} ADI_DMA_MANAGER;


typedef struct {                            // Channel information
    u8                      *BaseAddress;       // base address for the channel's registers
    u32                     ControllerID;       // controller ID for the channel
    ADI_INT_PERIPHERAL_ID   PeripheralID;       // peripheral ID of the channel
    ADI_INT_PERIPHERAL_ID   ErrorPeripheralID;  // peripheral ID of the error interrupt for this channel
} ADI_DMA_CHANNEL_INFO;


typedef struct {                                    // structure for memory copies (used in simple memDMA functions only)
    volatile u8                 TransferInProgressFlag; // TRUE when a transfer is in progress
    ADI_DCB_CALLBACK_FN         ClientCallback;         // client callback
    void                        *ClientHandle;          // client handle
    ADI_DMA_DESCRIPTOR_LARGE    SourceDescriptor;       // source descriptor
    ADI_DMA_DESCRIPTOR_LARGE    DestinationDescriptor;  // destination descriptor
} ADI_DMA_MEMORY_COPY_DATA;


typedef struct {                                // Memory DMA channel info
    ADI_DMA_CHANNEL_ID          SourceChannelID;        // source channel ID
    ADI_DMA_CHANNEL_ID          DestinationChannelID;   // destination channel ID
} ADI_DMA_MEMORY_STREAM_INFO;


typedef struct {
    ADI_DMA_TC_PER_REG  *Period;    // pointer to the TC period register
    ADI_DMA_TC_CNT_REG  *Counter;   // pointer to the TC counter register
} ADI_DMA_TC_REGISTER_PAIR;



/*********************************************************************

Processor Specifics

*********************************************************************/

/********************
    Edinburgh
********************/

#if defined(__ADSP_EDINBURGH__)     // Edinburgh class devices

const static ADI_DMA_CHANNEL_INFO ChannelInfoTable[] = {    // Channel information
    {   (u8 *)0xffc00c00,   0,  ADI_INT_DMA0_PPI,       ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c40,   0,  ADI_INT_DMA1_SPORT0_RX, ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c80,   0,  ADI_INT_DMA2_SPORT0_TX, ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00cc0,   0,  ADI_INT_DMA3_SPORT1_RX, ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d00,   0,  ADI_INT_DMA4_SPORT1_TX, ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d40,   0,  ADI_INT_DMA5_SPI,       ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d80,   0,  ADI_INT_DMA6_UART_RX,   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00dc0,   0,  ADI_INT_DMA7_UART_TX,   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e00,   0,  ADI_INT_MDMAS0,         ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e40,   0,  ADI_INT_MDMAS0,         ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e80,   0,  ADI_INT_MDMAS1,         ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00ec0,   0,  ADI_INT_MDMAS1,         ADI_INT_DMA_ERROR   },
};

const static ADI_DMA_MEMORY_STREAM_INFO MemoryStreamInfoTable[] = { // Channel information for memory DMA channels
    {   ADI_DMA_MDMA_S0,    ADI_DMA_MDMA_D0 },                              // channels for memory stream 0
    {   ADI_DMA_MDMA_S1,    ADI_DMA_MDMA_D1 },                              // channels for memory stream 1
};


const static ADI_DMA_TC_REGISTER_PAIR TCTable[] = {     // traffic control registers indexed by controller number
    {   (ADI_DMA_TC_PER_REG *)0xffc00b0c,   (ADI_DMA_TC_CNT_REG *)0xffc00b10    },  // controller 0
};


#endif


/********************
    Braemar
********************/

#if defined(__ADSP_BRAEMAR__)       // Braemar class devices

const static ADI_DMA_CHANNEL_INFO ChannelInfoTable[] = {    // Channel information
    {   (u8 *)0xffc00c00,   0,  ADI_INT_DMA0_PPI,                   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c40,   0,  ADI_INT_DMA1_ETHERNET_RX_PORTH_A,   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c80,   0,  ADI_INT_DMA2_ETHERNET_TX_PORTH_B,   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00cc0,   0,  ADI_INT_DMA3_SPORT0_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d00,   0,  ADI_INT_DMA4_SPORT0_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d40,   0,  ADI_INT_DMA5_SPORT1_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d80,   0,  ADI_INT_DMA6_SPORT1_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00dc0,   0,  ADI_INT_DMA7_SPI,                   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e00,   0,  ADI_INT_DMA8_UART0_RX,              ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e40,   0,  ADI_INT_DMA9_UART0_TX,              ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e80,   0,  ADI_INT_DMA10_UART1_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00ec0,   0,  ADI_INT_DMA11_UART1_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f00,   0,  ADI_INT_MDMAS0,                     ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f40,   0,  ADI_INT_MDMAS0,                     ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f80,   0,  ADI_INT_MDMAS1,                     ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00fc0,   0,  ADI_INT_MDMAS1,                     ADI_INT_DMA_ERROR   },
};

const static ADI_DMA_MEMORY_STREAM_INFO MemoryStreamInfoTable[] = { // Channel information for memory DMA channels
    {   ADI_DMA_MDMA_S0,    ADI_DMA_MDMA_D0 },                              // channels for memory stream 0
    {   ADI_DMA_MDMA_S1,    ADI_DMA_MDMA_D1 },                              // channels for memory stream 1
};

const static ADI_DMA_TC_REGISTER_PAIR TCTable[] = {     // traffic control registers indexed by controller number
    {   (ADI_DMA_TC_PER_REG *)0xffc00b0c,   (ADI_DMA_TC_CNT_REG *)0xffc00b10    },  // controller 0
};


#endif


/********************
    Teton-Lite
********************/

#if defined(__ADSP_TETON__)         // Teton class devices

const static ADI_DMA_CHANNEL_INFO ChannelInfoTable[] = {    // Channel information
    {   (u8 *)0xffc01c00,   0,  ADI_INT_DMA1_0_PPI0,        ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01c40,   0,  ADI_INT_DMA1_1_PPI1,        ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01c80,   0,  ADI_INT_DMA1_2,             ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01cc0,   0,  ADI_INT_DMA1_3,             ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01d00,   0,  ADI_INT_DMA1_4,             ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01d40,   0,  ADI_INT_DMA1_5,             ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01d80,   0,  ADI_INT_DMA1_6,             ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01dc0,   0,  ADI_INT_DMA1_7,             ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01e00,   0,  ADI_INT_DMA1_8,             ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01e40,   0,  ADI_INT_DMA1_9,             ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01e80,   0,  ADI_INT_DMA1_10,            ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01ec0,   0,  ADI_INT_DMA1_11,            ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01f00,   0,  ADI_INT_MDMA1_S0,           ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01f40,   0,  ADI_INT_MDMA1_S0,           ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01f80,   0,  ADI_INT_MDMA1_S1,           ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc01fc0,   0,  ADI_INT_MDMA1_S1,           ADI_INT_DMA1_ERROR  },
    {   (u8 *)0xffc00c00,   1,  ADI_INT_DMA2_0_SPORT0_RX,   ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00c40,   1,  ADI_INT_DMA2_1_SPORT0_TX,   ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00c80,   1,  ADI_INT_DMA2_2_SPORT1_RX,   ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00cc0,   1,  ADI_INT_DMA2_3_SPORT1_TX,   ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00d00,   1,  ADI_INT_DMA2_4_SPI,         ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00d40,   1,  ADI_INT_DMA2_5_UART_RX,     ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00d80,   1,  ADI_INT_DMA2_6_UART_TX,     ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00dc0,   1,  ADI_INT_DMA2_7,             ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00e00,   1,  ADI_INT_DMA2_8,             ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00e40,   1,  ADI_INT_DMA2_9,             ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00e80,   1,  ADI_INT_DMA2_10,            ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00ec0,   1,  ADI_INT_DMA2_11,            ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00f00,   1,  ADI_INT_MDMA2_S0,           ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00f40,   1,  ADI_INT_MDMA2_S0,           ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00f80,   1,  ADI_INT_MDMA2_S1,           ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc00fc0,   1,  ADI_INT_MDMA2_S1,           ADI_INT_DMA2_ERROR  },
    {   (u8 *)0xffc01800,   1,  ADI_INT_IMDMA_S0,           ADI_INT_IMDMA_ERROR },
    {   (u8 *)0xffc01840,   1,  ADI_INT_IMDMA_S0,           ADI_INT_IMDMA_ERROR },
    {   (u8 *)0xffc01880,   1,  ADI_INT_IMDMA_S1,           ADI_INT_IMDMA_ERROR },
    {   (u8 *)0xffc018c0,   1,  ADI_INT_IMDMA_S1,           ADI_INT_IMDMA_ERROR },
};

const static ADI_DMA_MEMORY_STREAM_INFO MemoryStreamInfoTable[] = { // Channel information for memory DMA channels
    {   ADI_DMA_MDMA1_S0,   ADI_DMA_MDMA1_D0    },                              // channels for memory DMA controller 1 stream 0
    {   ADI_DMA_MDMA1_S1,   ADI_DMA_MDMA1_D1    },                              // channels for memory DMA controller 1 stream 1
    {   ADI_DMA_MDMA2_S0,   ADI_DMA_MDMA2_D0    },                              // channels for memory DMA controller 2 stream 0
    {   ADI_DMA_MDMA2_S1,   ADI_DMA_MDMA2_D1    },                              // channels for memory DMA controller 2 stream 1
    {   ADI_DMA_IMDMA_S0,   ADI_DMA_IMDMA_D0    },                              // channels for intermal memory DMA stream 0
    {   ADI_DMA_IMDMA_S1,   ADI_DMA_IMDMA_D1    },                              // channels for internal memory DMA stream 1
};

const static ADI_DMA_TC_REGISTER_PAIR TCTable[] = {     // traffic control registers indexed by controller number
    {   (ADI_DMA_TC_PER_REG *)0xffc01b0c,   (ADI_DMA_TC_CNT_REG *)0xffc01b10    },  // controller 0
    {   (ADI_DMA_TC_PER_REG *)0xffc00b0c,   (ADI_DMA_TC_CNT_REG *)0xffc00b10    },  // controller 1
};

#endif


/********************
    Stirling
********************/

#if defined(__ADSP_STIRLING__)      // Stirling class devices

const static ADI_DMA_CHANNEL_INFO ChannelInfoTable[] = {    // Channel information
    {   (u8 *)0xffc00c00,   0,  ADI_INT_DMA0_PPI,           ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00c40,   0,  ADI_INT_DMA1_SPORT0_RX,     ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00c80,   0,  ADI_INT_DMA2_SPORT0_TX,     ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00cc0,   0,  ADI_INT_DMA3_SPORT1_RX,     ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00d00,   0,  ADI_INT_DMA4_SPORT1_TX,     ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00d40,   0,  ADI_INT_DMA5_SPI0,          ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00d80,   0,  ADI_INT_DMA6_UART0_RX,      ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00dc0,   0,  ADI_INT_DMA7_UART0_TX,      ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00e00,   0,  ADI_INT_MDMA0_S0,           ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00e40,   0,  ADI_INT_MDMA0_S0,           ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00e80,   0,  ADI_INT_MDMA0_S1,           ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc00ec0,   0,  ADI_INT_MDMA0_S1,           ADI_INT_DMA0_ERROR      },
    {   (u8 *)0xffc01c00,   1,  ADI_INT_DMA8_SPORT2_RX,     ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01c40,   1,  ADI_INT_DMA9_SPORT2_TX,     ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01c80,   1,  ADI_INT_DMA10_SPORT3_RX,    ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01cc0,   1,  ADI_INT_DMA11_SPORT3_TX,    ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01d00,   1,  ADI_INT_DMA12,              ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01d40,   1,  ADI_INT_DMA13,              ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01d80,   1,  ADI_INT_DMA14_SPI1,         ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01dc0,   1,  ADI_INT_DMA15_SPI2,         ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01e00,   1,  ADI_INT_DMA16_UART1_RX,     ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01e40,   1,  ADI_INT_DMA17_UART1_TX,     ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01e80,   1,  ADI_INT_DMA18_UART2_RX,     ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01ec0,   1,  ADI_INT_DMA19_UART2_TX,     ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01f00,   1,  ADI_INT_MDMA1_S0,           ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01f40,   1,  ADI_INT_MDMA1_S0,           ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01f80,   1,  ADI_INT_MDMA1_S1,           ADI_INT_DMA1_ERROR      },
    {   (u8 *)0xffc01fc0,   1,  ADI_INT_MDMA1_S1,           ADI_INT_DMA1_ERROR      },
};

const static ADI_DMA_MEMORY_STREAM_INFO MemoryStreamInfoTable[] = { // Channel information for memory DMA channels
    {   ADI_DMA_MDMA0_S0,   ADI_DMA_MDMA0_D0    },                              // channels for controller 0 memory stream 0
    {   ADI_DMA_MDMA0_S1,   ADI_DMA_MDMA0_D1    },                              // channels for controller 0 memory stream 1
    {   ADI_DMA_MDMA1_S0,   ADI_DMA_MDMA1_D0    },                              // channels for controller 1 memory stream 0
    {   ADI_DMA_MDMA1_S1,   ADI_DMA_MDMA1_D1    },                              // channels for controller 1 memory stream 1
};

const static ADI_DMA_TC_REGISTER_PAIR TCTable[] = {     // traffic control registers indexed by controller number
    {   (ADI_DMA_TC_PER_REG *)0xffc00b0c,   (ADI_DMA_TC_CNT_REG *)0xffc00b10    },  // controller 0
    {   (ADI_DMA_TC_PER_REG *)0xffc01b0c,   (ADI_DMA_TC_CNT_REG *)0xffc01b10    },  // controller 1
};


#endif


/********************
    Moab
********************/

#if defined(__ADSP_MOAB__)      // BF54x

const static ADI_DMA_CHANNEL_INFO ChannelInfoTable[] = {    // Channel information
    {   (u8 *)0xffc00c00,   0,  ADI_INT_DMA0_SPORT0_RX,     ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00c40,   0,  ADI_INT_DMA1_SPORT0_TX,     ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00c80,   0,  ADI_INT_DMA2_SPORT1_RX,     ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00cc0,   0,  ADI_INT_DMA3_SPORT1_TX,     ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00d00,   0,  ADI_INT_DMA4_SPI0,          ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00d40,   0,  ADI_INT_DMA5_SPI1,          ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00d80,   0,  ADI_INT_DMA6_UART0_RX,      ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00dc0,   0,  ADI_INT_DMA7_UART0_TX,      ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00e00,   0,  ADI_INT_DMA8_UART1_RX,      ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00e40,   0,  ADI_INT_DMA9_UART1_TX,      ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00e80,   0,  ADI_INT_DMA10_ATAPI_RX,     ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00ec0,   0,  ADI_INT_DMA11_ATAPI_TX,     ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00f00,   0,  ADI_INT_MDMA_S0,            ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00f40,   0,  ADI_INT_MDMA_S0,            ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00f80,   0,  ADI_INT_MDMA_S1,            ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc00fc0,   0,  ADI_INT_MDMA_S1,            ADI_INT_DMAC0_ERROR     },
    {   (u8 *)0xffc01c00,   1,  ADI_INT_DMA12_EPPI0,        ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01c40,   1,  ADI_INT_DMA13_EPPI1,        ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01c80,   1,  ADI_INT_DMA14_EPPI2_HOST,   ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01cc0,   1,  ADI_INT_DMA15_PIXC_IMAGE,   ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01d00,   1,  ADI_INT_DMA16_PIXC_OVERLAY, ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01d40,   1,  ADI_INT_DMA17_PIXC_OUTPUT,  ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01d80,   1,  ADI_INT_DMA18_SPORT2_RX,    ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01dc0,   1,  ADI_INT_DMA19_SPORT2_TX,    ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01e00,   1,  ADI_INT_DMA20_SPORT3_RX,    ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01e40,   1,  ADI_INT_DMA21_SPORT3_TX,    ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01e80,   1,  ADI_INT_DMA22_SDH_NFC,      ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01ec0,   1,  ADI_INT_DMA23_SPI2,         ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01f00,   1,  ADI_INT_MDMA_S2,            ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01f40,   1,  ADI_INT_MDMA_S2,            ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01f80,   1,  ADI_INT_MDMA_S3,            ADI_INT_DMAC1_ERROR     },
    {   (u8 *)0xffc01fc0,   1,  ADI_INT_MDMA_S3,            ADI_INT_DMAC1_ERROR     },
};

const static ADI_DMA_MEMORY_STREAM_INFO MemoryStreamInfoTable[] = { // Channel information for memory DMA channels
    {   ADI_DMA_MDMA0_S0,   ADI_DMA_MDMA0_D0    },                              // channels for controller 0 memory stream 0
    {   ADI_DMA_MDMA0_S1,   ADI_DMA_MDMA0_D1    },                              // channels for controller 0 memory stream 1
    {   ADI_DMA_MDMA1_S0,   ADI_DMA_MDMA1_D0    },                              // channels for controller 1 memory stream 0
    {   ADI_DMA_MDMA1_S1,   ADI_DMA_MDMA1_D1    },                              // channels for controller 1 memory stream 1
};

const static ADI_DMA_TC_REGISTER_PAIR TCTable[] = {     // traffic control registers indexed by controller number
    {   (ADI_DMA_TC_PER_REG *)0xffc00b0c,   (ADI_DMA_TC_CNT_REG *)0xffc00b10    },  // controller 0
    {   (ADI_DMA_TC_PER_REG *)0xffc01b0c,   (ADI_DMA_TC_CNT_REG *)0xffc01b10    },  // controller 1
};


#endif


/********************
    Kookaburra/Mockingbird
********************/

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)   // Kookaburra/Mockingbird class devices

const static ADI_DMA_CHANNEL_INFO ChannelInfoTable[] = {    // Channel information
    {   (u8 *)0xffc00c00,   0,  ADI_INT_DMA0_PPI,                   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c40,   0,  ADI_INT_DMA1_MAC_RX,                ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c80,   0,  ADI_INT_DMA2_MAC_TX,                ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00cc0,   0,  ADI_INT_DMA3_SPORT0_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d00,   0,  ADI_INT_DMA4_SPORT0_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d40,   0,  ADI_INT_DMA5_SPORT1_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d80,   0,  ADI_INT_DMA6_SPORT1_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00dc0,   0,  ADI_INT_DMA7_SPI,                   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e00,   0,  ADI_INT_DMA8_UART0_RX,              ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e40,   0,  ADI_INT_DMA9_UART0_TX,              ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e80,   0,  ADI_INT_DMA10_UART1_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00ec0,   0,  ADI_INT_DMA11_UART1_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f00,   0,  ADI_INT_MDMA0,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f40,   0,  ADI_INT_MDMA0,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f80,   0,  ADI_INT_MDMA1,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00fc0,   0,  ADI_INT_MDMA1,                      ADI_INT_DMA_ERROR   },
};

const static ADI_DMA_MEMORY_STREAM_INFO MemoryStreamInfoTable[] = { // Channel information for memory DMA channels
    {   ADI_DMA_MDMA_S0,    ADI_DMA_MDMA_D0 },                              // channels for memory stream 0
    {   ADI_DMA_MDMA_S1,    ADI_DMA_MDMA_D1 },                              // channels for memory stream 1
};

const static ADI_DMA_TC_REGISTER_PAIR TCTable[] = {     // traffic control registers indexed by controller number
    {   (ADI_DMA_TC_PER_REG *)0xffc00b0c,   (ADI_DMA_TC_CNT_REG *)0xffc00b10    },  // controller 0
};


#endif

/********************
    Brodie
********************/

#if defined(__ADSP_BRODIE__)

const static ADI_DMA_CHANNEL_INFO ChannelInfoTable[] = {    // Channel information
    {   (u8 *)0xffc00c00,   0,  ADI_INT_DMA0_PPI,                   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c40,   0,  ADI_INT_DMA1_MAC_RX,                ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c80,   0,  ADI_INT_DMA2_MAC_TX,                ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00cc0,   0,  ADI_INT_DMA3_SPORT0_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d00,   0,  ADI_INT_DMA4_SPORT0_TX_AND_RSI,     ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d40,   0,  ADI_INT_DMA5_SPORT1_RX_AND_SPI1,    ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d80,   0,  ADI_INT_DMA6_SPORT1_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00dc0,   0,  ADI_INT_DMA7_SPI0,                  ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e00,   0,  ADI_INT_DMA8_UART0_RX,              ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e40,   0,  ADI_INT_DMA9_UART0_TX,              ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e80,   0,  ADI_INT_DMA10_UART1_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00ec0,   0,  ADI_INT_DMA11_UART1_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f00,   0,  ADI_INT_MDMA0,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f40,   0,  ADI_INT_MDMA0,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f80,   0,  ADI_INT_MDMA1,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00fc0,   0,  ADI_INT_MDMA1,                      ADI_INT_DMA_ERROR   },
};

const static ADI_DMA_MEMORY_STREAM_INFO MemoryStreamInfoTable[] = { // Channel information for memory DMA channels
    {   ADI_DMA_MDMA_S0,    ADI_DMA_MDMA_D0 },                              // channels for memory stream 0
    {   ADI_DMA_MDMA_S1,    ADI_DMA_MDMA_D1 },                              // channels for memory stream 1
};

const static ADI_DMA_TC_REGISTER_PAIR TCTable[] = {     // traffic control registers indexed by controller number
    {   (ADI_DMA_TC_PER_REG *)0xffc00b0c,   (ADI_DMA_TC_CNT_REG *)0xffc00b10    },  // controller 0
};


#endif

/********************
    Moy
********************/

/* DMA Channel information of ADSP-BF50x processors */
#if defined(__ADSP_MOY__)

const static ADI_DMA_CHANNEL_INFO ChannelInfoTable[] =
{
    {   (u8 *)0xffc00c00,   0,  ADI_INT_DMA0_PPI,                   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c40,   0,  ADI_INT_DMA1_RSI,                   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c80,   0,  ADI_INT_DMA2_SPORT0_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00cc0,   0,  ADI_INT_DMA3_SPORT0_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d00,   0,  ADI_INT_DMA4_SPORT1_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d40,   0,  ADI_INT_DMA5_SPORT1_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d80,   0,  ADI_INT_DMA6_SPI0,                  ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00dc0,   0,  ADI_INT_DMA7_SPI1,                  ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e00,   0,  ADI_INT_DMA8_UART0_RX,              ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e40,   0,  ADI_INT_DMA9_UART0_TX,              ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e80,   0,  ADI_INT_DMA10_UART1_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00ec0,   0,  ADI_INT_DMA11_UART1_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f00,   0,  ADI_INT_MDMA0,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f40,   0,  ADI_INT_MDMA0,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f80,   0,  ADI_INT_MDMA1,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00fc0,   0,  ADI_INT_MDMA1,                      ADI_INT_DMA_ERROR   },
};

const static ADI_DMA_MEMORY_STREAM_INFO MemoryStreamInfoTable[] = { // Channel information for memory DMA channels
    {   ADI_DMA_MDMA_S0,    ADI_DMA_MDMA_D0 },                      // channels for memory stream 0
    {   ADI_DMA_MDMA_S1,    ADI_DMA_MDMA_D1 },                      // channels for memory stream 1
};

const static ADI_DMA_TC_REGISTER_PAIR TCTable[] = {     // traffic control registers indexed by controller number
    {   (ADI_DMA_TC_PER_REG *)0xffc00b0c,   (ADI_DMA_TC_CNT_REG *)0xffc00b10    },  // controller 0
};


#endif

/********************
    Delta
********************/

/* DMA Channel information of ADSP-BF59x processors */
#if defined(__ADSP_DELTA__)

const static ADI_DMA_CHANNEL_INFO ChannelInfoTable[] =
{
    {   (u8 *)0xffc00c00,   0,  ADI_INT_DMA0_PPI,                   ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c40,   0,  ADI_INT_DMA1_SPORT0_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00c80,   0,  ADI_INT_DMA2_SPORT0_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00cc0,   0,  ADI_INT_DMA3_SPORT1_RX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d00,   0,  ADI_INT_DMA4_SPORT1_TX,             ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d40,   0,  ADI_INT_DMA5_SPI0,                  ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00d80,   0,  ADI_INT_DMA6_SPI1,                  ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00dc0,   0,  ADI_INT_DMA7_UART0_RX,              ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00e00,   0,  ADI_INT_DMA8_UART0_TX,              ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f00,   0,  ADI_INT_MDMA0,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f40,   0,  ADI_INT_MDMA0,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00f80,   0,  ADI_INT_MDMA1,                      ADI_INT_DMA_ERROR   },
    {   (u8 *)0xffc00fc0,   0,  ADI_INT_MDMA1,                      ADI_INT_DMA_ERROR   },
};

const static ADI_DMA_MEMORY_STREAM_INFO MemoryStreamInfoTable[] = { // Channel information for memory DMA channels
    {   ADI_DMA_MDMA_S0,    ADI_DMA_MDMA_D0 },                              // channels for memory stream 0
    {   ADI_DMA_MDMA_S1,    ADI_DMA_MDMA_D1 },                              // channels for memory stream 1
};

const static ADI_DMA_TC_REGISTER_PAIR TCTable[] = {     // traffic control registers indexed by controller number
    {   (ADI_DMA_TC_PER_REG *)0xffc00b0c,   (ADI_DMA_TC_CNT_REG *)0xffc00b10    },  // controller 0
};


#endif

/*********************************************************************

Static data for memory DMA transfers

*********************************************************************/

static ADI_DMA_MEMORY_COPY_DATA MemoryCopyData[sizeof(MemoryStreamInfoTable)/sizeof(ADI_DMA_MEMORY_STREAM_INFO)];

static const ADI_DMA_CONFIG_REG DefaultSourceConfig2D = // default value for source DMA config register
{
    ADI_DMA_EN_DISABLE,
    ADI_DMA_WNR_READ,
    ADI_DMA_WDSIZE_8BIT,
    ADI_DMA_DMA2D_2D,
    ADI_DMA_RESTART_RETAIN,
    ADI_DMA_DI_SEL_OUTER_LOOP,
    ADI_DMA_DI_EN_DISABLE,
    ADI_DMA_NDSIZE_STOP,
    ADI_DMA_FLOW_STOP
};

static const ADI_DMA_CONFIG_REG DefaultDestinationConfig2D = // default value for destination DMA config register
{
    ADI_DMA_EN_DISABLE,
    ADI_DMA_WNR_WRITE,
    ADI_DMA_WDSIZE_8BIT,
    ADI_DMA_DMA2D_2D,
    ADI_DMA_RESTART_RETAIN,
    ADI_DMA_DI_SEL_OUTER_LOOP,
    ADI_DMA_DI_EN_ENABLE,
    ADI_DMA_NDSIZE_STOP,
    ADI_DMA_FLOW_STOP
};



/*********************************************************************

Static functions

*********************************************************************/


static ADI_INT_HANDLER(DataHandler);        // interrupt handler for data traffic
static ADI_INT_HANDLER(ErrorHandler);       // interrupt handler for errors


static void DetermineNextDescriptorInterrupt(   // updates the channel with the next descriptor that will generate an interrupt
    ADI_DMA_CHANNEL             *pChannel,          // channel
    ADI_DMA_DESCRIPTOR_UNION    *pDescriptor);      // head of chain

static ADI_DMA_RESULT SetDataflow(  // enables/disables dataflow
    ADI_DMA_CHANNEL *pChannel,          // channel handle
    u32             Flag);              // enable/disable flag

static void SetChannelType(         // sets the channel type field for a channel
    ADI_DMA_CHANNEL     *pChannel,      // channel handle
    ADI_DMA_CHANNEL_ID  ChannelID);     // channel ID

static ADI_DMA_DESCRIPTOR_HANDLE SetConfigWord( // sets the bits in the config register for the chain being queued
    ADI_DMA_CHANNEL             *pChannel,          // channel handle
    ADI_DMA_DESCRIPTOR_HANDLE   DescriptorHandle);  // descriptor chain

static void AddChainToQueue(                        // adds a descriptor chain on the proper (standby or active) queue
    ADI_DMA_CHANNEL             *pChannel,              // channel handle
    ADI_DMA_DESCRIPTOR_HANDLE   FirstDescriptorHandle,  // first descriptor in chain
    ADI_DMA_DESCRIPTOR_HANDLE   LastDescriptorHandle);  // last descriptor in chain

static ADI_DMA_RESULT SyncSourceToDestination(      // syncs the source descriptor chain to the destination descriptor chain
    ADI_DMA_DESCRIPTOR_LARGE    *pSourceChain,          // source descriptor chain
    ADI_DMA_DESCRIPTOR_LARGE    *pDestinationChain);    // destination descriptor chain

static void ProcessDataInterrupt(   // processes data interrupts
    ADI_DMA_CHANNEL *pChannel,              // pointer to the channel
    u32             AllowCallbacksFlag      // flag indicating if callbacks should be allowed
);

static void MemoryDMACallback(void *ClientHandle, u32 Event, void *pArg);    // callback function for simple memory DMA transfers

#if defined(ADI_SSL_DEBUG)
static ADI_DMA_RESULT ValidateManagerHandle(ADI_DMA_MANAGER_HANDLE Handle);
static ADI_DMA_RESULT ValidateChannelID(ADI_DMA_CHANNEL_ID ChannelID);
static ADI_DMA_RESULT ValidateChannelHandle(ADI_DMA_CHANNEL_HANDLE Handle);
static ADI_DMA_RESULT ValidateStreamID(ADI_DMA_STREAM_ID StreamID);
static ADI_DMA_RESULT ValidateStreamHandle(ADI_DMA_STREAM_HANDLE Handle);
static ADI_DMA_RESULT ValidatePMAP(ADI_DMA_PMAP pmap);
static ADI_DMA_RESULT ValidateTransfer(ADI_DMA_CONFIG_REG Config, void *StartAddress, u16 XCount, s16 XModify, u16 YCount, s16 YModify);
static u32 ValidateIsChannelOpen(ADI_DMA_MANAGER_HANDLE ManagerHandle, ADI_DMA_CHANNEL_ID ChannelID);
static ADI_DMA_RESULT ValidateStreamDescriptors(ADI_DMA_DESCRIPTOR_LARGE *pSource, ADI_DMA_DESCRIPTOR_LARGE *pDestination);
static ADI_DMA_RESULT ValidateDescriptorChain(ADI_DMA_CHANNEL_HANDLE ChannelHandle, ADI_DMA_DESCRIPTOR_HANDLE DescriptorHandle);
#endif



/*********************************************************************

Macros local to the DMA Manager

*********************************************************************/

#define ADI_DMA_EXTRACT_PMAP_FIELD_FROM_REGISTER(Register)   (((Register) & 0xf000) >> 12)  // returns the pmap field from the PMAP register

// deposits pmap field into a PMAP register
#define ADI_DMA_DEPOSIT_PMAP_FIELD_INTO_REGISTER(Register,Field)    \
            ((Register) = ((Register) & 0x0fff) | ((Field) << 12))


/*********************************************************************

    Function:       adi_dma_Init

    Description:    Provides memory to the DMA manager to use.  This
                    function initializes the memory to support 'n' number
                    of channels.

*********************************************************************/

ADI_DMA_RESULT adi_dma_Init(            // Initializes the DMA Manager
    void                    *pMemory,               // pointer to memory
    const size_t            MemorySize,             // size of memory (in bytes)
    u32                     *pMaxChannels,          // number of channels that can be supported
    ADI_DMA_MANAGER_HANDLE  *pManagerHandle,        // address where DMA manager will store the manager handle
    void                    *pEnterCriticalArg      // argument to pass to critical region function
) {

    ADI_DMA_MANAGER                     *pManager;  // pointer to the DMA Manager data
    ADI_DMA_CHANNEL                     *pChannel;  // pointer to a DMA channel
    u32                                 i;          // counter

    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if (ADI_DMA_CHANNEL_MEMORY != sizeof(ADI_DMA_CHANNEL)) {
        return (ADI_DMA_RESULT_BAD_CHANNEL_MEMORY_SIZE);
    }
#endif

    // insure we have enough memory for the DMA manager itself
    pManager = pMemory;
    if (MemorySize < sizeof(ADI_DMA_MANAGER)) {
        return (ADI_DMA_RESULT_NO_MEMORY);
    }
    *pManagerHandle = (ADI_DMA_MANAGER_HANDLE *)pManager;

    // determine how many DMA channels we can support and notify the application of such
    pManager->ChannelCount = (MemorySize - sizeof(ADI_DMA_MANAGER))/(sizeof(ADI_DMA_CHANNEL));
    *pMaxChannels = pManager->ChannelCount;
    if (pManager->ChannelCount == 0) {
        return (ADI_DMA_RESULT_NO_MEMORY);
    }
    pManager->Channel = (ADI_DMA_CHANNEL *)(pManager + 1);

    // save the parameter for critical region protection
    pManager->pEnterCriticalArg = pEnterCriticalArg;

    // initialize each channel
    for (i = pManager->ChannelCount, pChannel = pManager->Channel; i; i--, pChannel++) {
        pChannel->InUseFlag = FALSE;
        pChannel->pManager = pManager;
    }

    // return
    return (ADI_DMA_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       adi_dma_Terminate

    Description:    Closes down all channels and terminates the DMA manager

*********************************************************************/


ADI_DMA_RESULT adi_dma_Terminate(
        ADI_DMA_MANAGER_HANDLE  ManagerHandle       // DMA manager handle
) {

    int             i;                  // counter
    ADI_DMA_RESULT  Result;             // return code
    ADI_DMA_MANAGER *pManager;          // pointer to the DMA manager data
    ADI_DMA_CHANNEL *pChannel;          // pointer to the channel we're working on


    // assume success
    Result = ADI_DMA_RESULT_SUCCESS;

    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateManagerHandle(ManagerHandle)) != ADI_DMA_RESULT_SUCCESS) return(Result);
#endif

    // avoid casts
    pManager = (ADI_DMA_MANAGER *)ManagerHandle;

    // close all open channels
    for (i = pManager->ChannelCount, pChannel = pManager->Channel; i; i--, pChannel++) {
        if (pChannel->InUseFlag == TRUE) {
            if ((Result = adi_dma_Close(pChannel, FALSE)) != ADI_DMA_RESULT_SUCCESS) {
                break;
            }
        }
    }

    // return
    return(Result);
}



/*********************************************************************

    Function:       adi_dma_Open

    Description:    Opens a DMA channel for use

*********************************************************************/


ADI_DMA_RESULT adi_dma_Open(
        ADI_DMA_MANAGER_HANDLE  ManagerHandle,      // DMA manager handle
        ADI_DMA_CHANNEL_ID      ChannelID,          // channel ID
        void                    *ClientHandle,      // Client value
        ADI_DMA_CHANNEL_HANDLE  *pChannelHandle,    // address where the channel handle will be stored
        ADI_DMA_MODE            Mode,               // DMA mode
        ADI_DCB_HANDLE          DCBHandle,          // Callback service handle
        ADI_DCB_CALLBACK_FN     ClientCallback      // callback function
) {

    u32                         i;                  // counter
    u8                          *BaseAddress;       // base address of DMA registers
    ADI_DMA_MANAGER             *pManager;          // pointer to the DMA manager data
    u32                         DataIVG;            // IVG for the channel we're opening
    u32                         ErrorIVG;           // IVG for DMA error interrupts
    ADI_DMA_CHANNEL             *pChannel;          // pointer to the channel we're working on
    const ADI_DMA_CHANNEL_INFO  *pChannelInfo;      // point to the channel info for the channel
    ADI_DMA_RESULT              Result;             // return code
    void                        *pExitCriticalArg;  // parameter for exit critical


    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateManagerHandle(ManagerHandle)) != ADI_DMA_RESULT_SUCCESS) return(Result);
//  if (ClientCallback == NULL) return (ADI_DMA_RESULT_NO_CALLBACK_FUNCTION_SUPPLIED); Need to allow NULL for MemoryOpen

    // be sure the channel isn't already open
    if (ValidateIsChannelOpen(ManagerHandle, ChannelID) == TRUE) {
        return (ADI_DMA_RESULT_IN_USE);
    }

    // temporary till implemented
    if (Mode == ADI_DMA_MODE_DESCRIPTOR_ARRAY) return (ADI_DMA_RESULT_NOT_SUPPORTED);
#endif

    // avoid casts
    pManager = (ADI_DMA_MANAGER *)ManagerHandle;

    // a pessimist is never disappointed
    Result = ADI_DMA_RESULT_ALL_IN_USE;

    // find an open channel
    pExitCriticalArg = adi_int_EnterCriticalRegion(pManager->pEnterCriticalArg);
    for (i = pManager->ChannelCount, pChannel = pManager->Channel; i; i--, pChannel++) {
        if (pChannel->InUseFlag == FALSE) {
            pChannel->InUseFlag = TRUE;
            Result = ADI_DMA_RESULT_SUCCESS;
            break;
        }
    }
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // IF (we found a free channel)
    if (Result == ADI_DMA_RESULT_SUCCESS) {

        // point to the channel info
        pChannelInfo = &ChannelInfoTable[ChannelID];

        // populate the register addresses for the channel
        BaseAddress = pChannelInfo->BaseAddress;
        pChannel->NextDescPtr   = (ADI_DMA_DESCRIPTOR_UNION **) (BaseAddress);
        pChannel->StartAddr     = (volatile void **)            (BaseAddress + 0x04);
        pChannel->Config        = (ADI_DMA_CONFIG_REG *)        (BaseAddress + 0x08);
        pChannel->XCount        = (volatile u16 *)              (BaseAddress + 0x10);
        pChannel->XModify       = (volatile s16 *)              (BaseAddress + 0x14);
        pChannel->YCount        = (volatile u16 *)              (BaseAddress + 0x18);
        pChannel->YModify       = (volatile s16 *)              (BaseAddress + 0x1c);
        pChannel->CurrAddr      =                               (BaseAddress + 0x24);
        pChannel->IRQStatus     = (ADI_DMA_IRQ_STATUS_REG *)    (BaseAddress + 0x28);
        pChannel->PeripheralMap = (volatile u16 *)              (BaseAddress + 0x2c);
        pChannel->CurrXCount    = (volatile u16 *)              (BaseAddress + 0x30);
        pChannel->CurrYCount    = (volatile u16 *)              (BaseAddress + 0x38);

        // populate other entries for the channel
        pChannel->ChannelID = ChannelID;
        pChannel->Mode = Mode;
        pChannel->Status = ADI_DMA_STATUS_STOPPED;
        pChannel->LoopbackFlag = FALSE;
        pChannel->StreamingFlag = FALSE;
        pChannel->BufferReadyFlag = FALSE;
        pChannel->pNextCallback = NULL;
        pChannel->pActiveHead = NULL;
        pChannel->pActiveTail = NULL;
        pChannel->pStandbyHead = NULL;
        pChannel->pStandbyTail = NULL;
        pChannel->pPairingChannel = NULL;
        pChannel->PeripheralID = pChannelInfo->PeripheralID;
        pChannel->ClientHandle = ClientHandle;
        pChannel->DCBHandle = DCBHandle;
        pChannel->ClientCallback = ClientCallback;

        // set the channel type
        SetChannelType(pChannel, ChannelID);

        // store the DMA handle in the client provided location
        *pChannelHandle = pChannel;

        // get the IVG this channel is using for data and errors
        adi_int_SICGetIVG(pChannel->PeripheralID, &DataIVG);
        adi_int_SICGetIVG(pChannelInfo->ErrorPeripheralID, &ErrorIVG);

        // IF (we can hook the DMA error interrupt)
        if (adi_int_CECHook(ErrorIVG, ErrorHandler, pChannel, TRUE) == ADI_INT_RESULT_SUCCESS){

            // IF (we can hook the DMA data interrupt)
            if (adi_int_CECHook(DataIVG, DataHandler, pChannel, TRUE) == ADI_INT_RESULT_SUCCESS){

                // allow the DMA data interrupt to be passed to the CEC
                adi_int_SICWakeup(pChannel->PeripheralID, TRUE);
                adi_int_SICEnable(pChannel->PeripheralID);

                // allow the DMA error interrupt to be passed to the CEC
                adi_int_SICWakeup(pChannelInfo->ErrorPeripheralID, TRUE);
                adi_int_SICEnable(pChannelInfo->ErrorPeripheralID);

            // ELSE
            } else {

                // unhook the error interrupt
                adi_int_CECUnhook(ErrorIVG, ErrorHandler, pChannel);

                // return an error
                Result = ADI_DMA_RESULT_CANT_HOOK_INTERRUPT;

            // ENDIF
            }

        // ELSE
        } else {

            // return an error
            Result = ADI_DMA_RESULT_CANT_HOOK_INTERRUPT;

        // ENDIF
        }

        // make the channel available if we had any error
        if (Result != ADI_DMA_RESULT_SUCCESS) {
            pChannel->InUseFlag = FALSE;
        }

    // ENDIF
    }

    // return
    return (Result);
}




/*********************************************************************

    Function:       adi_dma_Close

    Description:    Closes down a DMA channel

*********************************************************************/


ADI_DMA_RESULT adi_dma_Close(
    ADI_DMA_CHANNEL_HANDLE  ChannelHandle,  // channel to close
    u32                     WaitFlag        // wait for idle flag
) {

    u32                         i;              // counter
    u32                         DataIVG;        // IVG for the channel we're closing
    u32                         ErrorIVG;       // IVG for DMA error interrupts
    ADI_DMA_RESULT              Result;         // return value
    ADI_DMA_CHANNEL             *pChannel;      // pointer to the channel we're working on
    const ADI_DMA_CHANNEL_INFO  *pChannelInfo;  // point to the channel info for the channel

    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateChannelHandle (ChannelHandle)) != ADI_DMA_RESULT_SUCCESS) return(Result);
#endif

    // avoid casts
    pChannel = (ADI_DMA_CHANNEL *)ChannelHandle;

    // be an optimist
    Result = ADI_DMA_RESULT_SUCCESS;

    // point to the channel info
    pChannelInfo = &ChannelInfoTable[pChannel->ChannelID];

    // get the data and error IVG of the channel we're closing
    adi_int_SICGetIVG(pChannel->PeripheralID, &DataIVG);
    adi_int_SICGetIVG(pChannelInfo->ErrorPeripheralID, &ErrorIVG);

    // wait if need be
    while (WaitFlag) {
        adi_dma_Control(ChannelHandle, ADI_DMA_CMD_GET_TRANSFER_STATUS, &WaitFlag);
    }

    // halt the DMA hardware
    pChannel->Config->b_DMA_EN = ADI_DMA_EN_DISABLE;

    // don't allow this channel's interrupt to wake us up or be passed to the core
    adi_int_SICDisable(pChannel->PeripheralID);
    adi_int_SICWakeup(pChannel->PeripheralID, FALSE);

    // IF (we can unhook the error handler)
    if (adi_int_CECUnhook(DataIVG, DataHandler, pChannel) == ADI_INT_RESULT_SUCCESS) {

        // IF (we can unhook the data handler)
        if (adi_int_CECUnhook(ErrorIVG, ErrorHandler, pChannel) == ADI_INT_RESULT_SUCCESS) {

            // mark the channel as closed
            pChannel->InUseFlag = FALSE;

        // ELSE
        } else {

            // return an error
            Result = ADI_DMA_RESULT_CANT_UNHOOK_INTERRUPT;

        // ENDIF
        }

    // ELSE
    } else {

        // return an error
        Result = ADI_DMA_RESULT_CANT_UNHOOK_INTERRUPT;

    // ENDIF
    }

    // return
    return (Result);
}



/*********************************************************************

    Function:       adi_dma_Queue

    Description:    Queues descriptors to a DMA channel.

                    Client needs to set WNR, WDSIZE and DMA2D only.  All
                    other fields within the config word are set by the DMA
                    Manager.

*********************************************************************/


ADI_DMA_RESULT adi_dma_Queue(
        ADI_DMA_CHANNEL_HANDLE      ChannelHandle,      // channel
        ADI_DMA_DESCRIPTOR_HANDLE   DescriptorHandle    // head of descriptor chain
) {

    ADI_DMA_RESULT              Result;             // return value
    ADI_DMA_CHANNEL             *pChannel;          // pointer to the channel we're working on
    ADI_DMA_DESCRIPTOR_HANDLE   pLastDescriptor;    // pointer to the last descriptor in the list passed in

    // avoid casts and assume we'll be successful
    pChannel = (ADI_DMA_CHANNEL *)ChannelHandle;
    Result = ADI_DMA_RESULT_SUCCESS;

    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)

    // validate the channel handle
    if ((Result = ValidateChannelHandle (ChannelHandle)) != ADI_DMA_RESULT_SUCCESS) return(Result);

    // validate the descriptor chain
    if ((Result = ValidateDescriptorChain(ChannelHandle, DescriptorHandle)) != ADI_DMA_RESULT_SUCCESS) {
        return (Result);
    }

    // can't allow descriptors to be submitted to live channels with loopback enabled
    if ((pChannel->Status == ADI_DMA_STATUS_RUNNING) && (pChannel->LoopbackFlag == TRUE)) {
        return (ADI_DMA_RESULT_ALREADY_RUNNING);
    }

#endif

    // set the configuration word for all descriptors in the chain and make note of the last descriptor in the chain
    pLastDescriptor = SetConfigWord(pChannel, DescriptorHandle);

    // place the descriptor chain on the proper queue (standby or active)
    AddChainToQueue(pChannel, DescriptorHandle, pLastDescriptor);

    // restart the channel if we're paused
    if (pChannel->Status == ADI_DMA_STATUS_PAUSED) {
        Result = adi_dma_Control(ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)TRUE);
    }

    // return
    return(Result);
}







/*********************************************************************

    Function:       adi_dma_Buffer

    Description:    Configures a DMA channel for circular/autobuffering
                    or a one-shot transfer

*********************************************************************/


ADI_DMA_RESULT adi_dma_Buffer(
        ADI_DMA_CHANNEL_HANDLE  ChannelHandle,  // channel
        void                    *StartAddress,  // start address of data
        ADI_DMA_CONFIG_REG      Config,         // config register value
        u16                     XCount,         // XCount
        s16                     XModify,        // XModify
        u16                     YCount,         // YCount
        s16                     YModify         // YModify
) {

    ADI_DMA_RESULT  Result;             // return value
    ADI_DMA_CHANNEL *pChannel;          // pointer to the channel we're working on
    void            *pExitCriticalArg;  // return value from EnterCritical

    // avoid casts
    pChannel = (ADI_DMA_CHANNEL *)ChannelHandle;

    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateChannelHandle (ChannelHandle)) != ADI_DMA_RESULT_SUCCESS) return(Result);
    if ((pChannel->Mode != ADI_DMA_MODE_CIRCULAR) && (pChannel->Mode != ADI_DMA_MODE_SINGLE)) return(ADI_DMA_RESULT_BAD_MODE);
    if (pChannel->Mode == ADI_DMA_MODE_CIRCULAR) {
        if ((XModify != 1) && (XModify != 2) && (XModify != 4)) return (ADI_DMA_RESULT_BAD_DATA_WIDTH);
    }
#endif

    // insure the channel is not actively transferring data
    if (pChannel->Status == ADI_DMA_STATUS_RUNNING) {
        return(ADI_DMA_RESULT_ALREADY_RUNNING);
    }

    // insure the reserved bit in the config register is cleared otherwise a DMA error
    // will be generated
    Config.b_RESERVED = 0;

    // IF (in circular mode)
    if (pChannel->Mode == ADI_DMA_MODE_CIRCULAR) {

        // set config for autobuffer
        Config.b_FLOW = ADI_DMA_FLOW_AUTOBUFFER;
        Config.b_DMA2D = ADI_DMA_DMA2D_2D;

        // set WDSIZE according the XModify value passed in
        Config.b_WDSIZE = XModify >> 1;

        // set YModify equal to XModify
        YModify = XModify;

    // ELSE (must be in single mode)
    } else {

        // set config for single mode
        Config.b_FLOW = ADI_DMA_FLOW_STOP;

    // ENDIF
    }

    // setup the remaining fields in the config register
    Config.b_NDSIZE = ADI_DMA_NDSIZE_STOP;
    Config.b_DMA_EN = ADI_DMA_EN_DISABLE;
    Config.b_RESTART = ADI_DMA_RESTART_RETAIN;

    // validate the transfer settings if debug is enabled
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateTransfer(Config, StartAddress, XCount, XModify, YCount, YModify)) != ADI_DMA_RESULT_SUCCESS) {
        return (Result);
    }
#endif

    // write the information to the DMA registers
    *pChannel->StartAddr = StartAddress;
    *pChannel->XCount = XCount;
    *pChannel->XModify = XModify;
    *pChannel->YCount = YCount;
    *pChannel->YModify = YModify;
    *pChannel->Config = Config;

    // update the flag indicating we have a circular buffer ready to go
    pChannel->BufferReadyFlag = TRUE;

    // restart the channel if we're paused
    if (pChannel->Status == ADI_DMA_STATUS_PAUSED) {
        Result = adi_dma_Control(ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)TRUE);
    }

    // return
    return (ADI_DMA_RESULT_SUCCESS);
}





/*********************************************************************

    Function:       adi_dma_Control

    Description:    Sets/senses DMA configuration and control parameters

*********************************************************************/


ADI_DMA_RESULT adi_dma_Control(
        ADI_DMA_CHANNEL_HANDLE  ChannelHandle,      // channel
        ADI_DMA_CMD             Command,            // command
        void                    *Value              // command specific value
) {

    ADI_DMA_RESULT          Result;             // return value
    ADI_DMA_CHANNEL         *pChannel;          // pointer to the channel we're working on
    ADI_DMA_CMD_VALUE_PAIR  *pPair;             // pointer to command pair
    void                    *pExitCriticalArg;  // return value from EnterCritical
    ADI_DMA_TC_SET          *pTCSet;            // pointer to a TC set structure
    ADI_DMA_TC_PER_REG      *pPeriod;           // pointer to TC period register
    ADI_DMA_TC_CNT_REG      *pCounter;          // pointer to TC counter register
    ADI_DMA_TC_GET          *pTCGet;            // pointer to a TC get structure

    // avoid casts and assume the worst
    pChannel = (ADI_DMA_CHANNEL *)ChannelHandle;
    Result = ADI_DMA_RESULT_FAILED;

    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateChannelHandle (ChannelHandle)) != ADI_DMA_RESULT_SUCCESS) return(Result);
#endif

    // assume success
    Result = ADI_DMA_RESULT_SUCCESS;

    // CASEOF (Command ID)
    switch (Command) {

        // CASE (command table)
        case ADI_DMA_CMD_TABLE:

            // avoid casts
            pPair = (ADI_DMA_CMD_VALUE_PAIR *)Value;

            // process each command pair
            while (pPair->CommandID != ADI_DMA_CMD_END) {
                if ((Result = adi_dma_Control(ChannelHandle, pPair->CommandID, pPair->Value)) != ADI_DMA_RESULT_SUCCESS) return(Result);
                pPair++;
            }
            break;

        // CASE (command table terminator)
        case ADI_DMA_CMD_END:
            break;

        // CASE (command pair)
        case ADI_DMA_CMD_PAIR:

            // avoid casts
            pPair = (ADI_DMA_CMD_VALUE_PAIR *)Value;

            // process and return
            return (adi_dma_Control(ChannelHandle, pPair->CommandID, pPair->Value));

        // CASE (loopback)
        case ADI_DMA_CMD_SET_LOOPBACK:

            // debug (can't set loopback if descriptors already queued)
#if defined(ADI_SSL_DEBUG)
            if ((pChannel->pActiveHead) || (pChannel->pStandbyHead)) {
                return (ADI_DMA_RESULT_INVALID_SEQUENCE);
            }
#endif

            // set the channel status but disallow if the dataflow is enabled
            if (pChannel->Status != ADI_DMA_STATUS_STOPPED) return(ADI_DMA_RESULT_ALREADY_RUNNING);
            pChannel->LoopbackFlag = (u32)Value;
            break;

        // CASE (loopback)
        case ADI_DMA_CMD_SET_STREAMING:

            // debug (can't set streaming if descriptors already queued)
#if defined(ADI_SSL_DEBUG)
            if ((pChannel->pActiveHead) || (pChannel->pStandbyHead)) {
                return (ADI_DMA_RESULT_INVALID_SEQUENCE);
            }
#endif

            // set the channel status but disallow if the dataflow is enabled
            if (pChannel->Status != ADI_DMA_STATUS_STOPPED) return(ADI_DMA_RESULT_ALREADY_RUNNING);
            pChannel->StreamingFlag = (u32)Value;
            break;

        // CASE (set dataflow)
        case ADI_DMA_CMD_SET_DATAFLOW:

            // do it
            return(SetDataflow(pChannel, (u32)Value));

        // CASE (get transfer status)
        case ADI_DMA_CMD_GET_TRANSFER_STATUS:

            // get it
            if (pChannel->IRQStatus->b_DMA_RUN == ADI_DMA_RUN_ENABLED) {
                *((u32 *)Value) = TRUE;
            } else {
                *((u32 *)Value) = FALSE;
            }
            break;

        // CASE (flush a channel of all buffers)
        case ADI_DMA_CMD_FLUSH:

            // flush all queues but check to be sure dataflow is stopped
            pExitCriticalArg = adi_int_EnterCriticalRegion(pChannel->pManager->pEnterCriticalArg);
            if (pChannel->Status != ADI_DMA_STATUS_STOPPED) {
                Result = ADI_DMA_RESULT_ALREADY_RUNNING;
            } else {
                pChannel->BufferReadyFlag = FALSE;
                pChannel->pNextCallback = NULL;
                pChannel->pActiveHead = NULL;
                pChannel->pActiveTail = NULL;
                pChannel->pStandbyHead = NULL;
                pChannel->pStandbyTail = NULL;
            }
            adi_int_ExitCriticalRegion(pExitCriticalArg);
            break;

        // CASE (set traffic control value)
        case ADI_DMA_CMD_SET_TC:

            // avoid casts
            pTCSet = (ADI_DMA_TC_SET *)Value;

            // check for the correct controller value
#if defined(ADI_SSL_DEBUG)
#if defined(__ADSP_TETON__) || defined(__ADSP_STIRLING__)
            if (pTCSet->ControllerID > 1) return (ADI_DMA_RESULT_BAD_CONTROLLER_ID);
#else
            if (pTCSet->ControllerID > 0) return (ADI_DMA_RESULT_BAD_CONTROLLER_ID);
#endif
#endif

            // point to the proper registers
            pPeriod = TCTable[pTCSet->ControllerID].Period;

            // set it
            switch (pTCSet->ParameterID) {
                case ADI_DMA_TC_DCB:
                    pPeriod->b_DCB_TRAFFIC_PERIOD = pTCSet->Value;
                    break;
                case ADI_DMA_TC_DEB:
                    pPeriod->b_DEB_TRAFFIC_PERIOD = pTCSet->Value;
                    break;
                case ADI_DMA_TC_DAB:
                    pPeriod->b_DAB_TRAFFIC_PERIOD = pTCSet->Value;
                    break;
                case ADI_DMA_TC_MDMA:
                    pPeriod->b_MDMA_ROUND_ROBIN_PERIOD = pTCSet->Value;
                    break;
#if defined(ADI_SSL_DEBUG)
                default:
                    return (ADI_DMA_RESULT_BAD_TC_PARAMETER);
#endif
            }
            break;

            // CASE (get traffic control counter values)
            case ADI_DMA_CMD_GET_TC:

            // avoid casts
            pTCGet = (ADI_DMA_TC_GET *)Value;

            // check for the correct controller value
#if defined(ADI_SSL_DEBUG)
#if defined(__ADSP_TETON__) || defined(__ADSP_STIRLING__)
            if (pTCGet->ControllerID > 1) return (ADI_DMA_RESULT_BAD_CONTROLLER_ID);
#else
            if (pTCGet->ControllerID > 0) return (ADI_DMA_RESULT_BAD_CONTROLLER_ID);
#endif
#endif

            // point to the proper registers
            pCounter = TCTable[pTCSet->ControllerID].Counter;

            // get it
            switch (pTCGet->ParameterID) {
                case ADI_DMA_TC_DCB:
                    *pTCGet->pValue = pCounter->b_DCB_TRAFFIC_COUNT;
                    break;
                case ADI_DMA_TC_DEB:
                    *pTCGet->pValue = pCounter->b_DEB_TRAFFIC_COUNT;
                    break;
                case ADI_DMA_TC_DAB:
                    *pTCGet->pValue = pCounter->b_DAB_TRAFFIC_COUNT;
                    break;
                case ADI_DMA_TC_MDMA:
                    *pTCGet->pValue = pCounter->b_MDMA_ROUND_ROBIN_COUNT;
                    break;
#if defined(ADI_SSL_DEBUG)
                default:
                    return (ADI_DMA_RESULT_BAD_TC_PARAMETER);
#endif
            }
            break;

            // CASE (gets DMA current address register value)
            case ADI_DMA_CMD_GET_CURRENT_ADDRESS:

                /* Read the current address register twice to fix Anomaly #05000199 */
                *((u32 *)Value) = *((u32 *)pChannel->CurrAddr);
                *((u32 *)Value) = *((u32 *)pChannel->CurrAddr);
                break;


        // CASEELSE
        default:

            // return an error
            return (ADI_DMA_RESULT_BAD_COMMAND);

    // ENDCASE
    }

    // return
    return (Result);
}



/*********************************************************************

    Function:       adi_dma_MemoryOpen

    Description:    Opens a memory DMA channel for use.  Because memory
                    DMA requires two channels, a source and destination
                    channel, we need to do things a bit differently.  As
                    the destination channel is the channel that completes
                    its operation last, it is the channel that the DMA
                    manager keys off of.  Interrupts are triggered by the
                    destination channel, not the source channel.

                    The loopback flag in the destination channel is overloaded
                    as an in-use flag.  It get's set when the client starts
                    a job and cleared when finished.  This prevents
                    multipled clients from queueing jobs simultaneously.

                    The streaming flag in the destination channel is overloaded
                    as a memory DMA flag.  It's set when the mode is SINGLE
                    and the channel is being used for memory DMA.

*********************************************************************/


ADI_DMA_RESULT adi_dma_MemoryOpen(
        ADI_DMA_MANAGER_HANDLE  ManagerHandle,      // DMA manager handle
        ADI_DMA_STREAM_ID       StreamID,           // memory stream ID
        void                    *ClientHandle,      // Client value
        ADI_DMA_STREAM_HANDLE   *pStreamHandle,     // address where the channel handle will be stored
        void                    *DCBHandle          // deferred callback service handle
) {

    ADI_DMA_RESULT  Result; // return code
    const ADI_DMA_MEMORY_STREAM_INFO    *pInfo;                 // pointer to a memory info entry
    ADI_DMA_MEMORY_COPY_DATA            *pMemoryCopyData;       // pointer to the memory copy data

    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateManagerHandle(ManagerHandle)) != ADI_DMA_RESULT_SUCCESS) return(Result);
    if ((Result = ValidateStreamID(StreamID)) != ADI_DMA_RESULT_SUCCESS) return(Result);
#endif

    // point to the info for this memory stream
    pInfo = &MemoryStreamInfoTable[StreamID];

    // point to the data structure for this stream
    pMemoryCopyData = &MemoryCopyData[StreamID];

    // IF (we can successfully open the stream via the MemoryQueueOpen function)
    if ((Result = adi_dma_MemoryQueueOpen(ManagerHandle, StreamID, pMemoryCopyData, pStreamHandle, DCBHandle, MemoryDMACallback)) == ADI_DMA_RESULT_SUCCESS) {

        // save the client handle in the memory copy structure
        pMemoryCopyData->ClientHandle = ClientHandle;

    // ENDIF
    }

    // return
    return (Result);
}



/*********************************************************************

    Function:       adi_dma_MemoryClose

    Description:    Closed down the source and destination channels.

*********************************************************************/


ADI_DMA_RESULT adi_dma_MemoryClose(
        ADI_DMA_STREAM_HANDLE   StreamHandle,   // stream handle
        u32                     WaitFlag        // wait flag
) {

    ADI_DMA_RESULT  Result; // return code

    // close the stream via the MemoryQueueClose function
    Result = adi_dma_MemoryQueueClose(StreamHandle, WaitFlag);

    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_dma_MemoryCopy

    Description:    Copies one dimensional data from one memory space
                    to another.

*********************************************************************/


ADI_DMA_RESULT adi_dma_MemoryCopy(          // copies memory in a 1D fashion
    ADI_DMA_STREAM_HANDLE   StreamHandle,       // stream handle
    void                    *pDest,             // memory destination address
    void                    *pSrc,              // memory source address
    u16                     ElementWidth,       // element width (in bytes)
    u16                     ElementCount,       // element count
    ADI_DCB_CALLBACK_FN     ClientCallback      // client callback function (Synchronous when NULL)
) {

    ADI_DMA_2D_TRANSFER Destination;
    ADI_DMA_2D_TRANSFER Source;

    // populate the source and destination structures
    Source.XCount = Destination.XCount = ElementCount;
    Source.XModify = Destination.XModify = ElementWidth;
    Source.YCount = Destination.YCount = 1;
    Source.YModify = Destination.YModify = 0;
    Source.StartAddress = pSrc;
    Destination.StartAddress = pDest;

    // perform the transfer via the 2D routine
    return (adi_dma_MemoryCopy2D(StreamHandle, &Destination, &Source, ElementWidth, ClientCallback));
}


/*********************************************************************

    Function:       adi_dma_MemoryCopy2D

    Description:    Copies two dimensional data from one memory space
                    to another.

                    NOTE: Loopback flag is used as a TransferInProgress
                    flag.

*********************************************************************/


ADI_DMA_RESULT adi_dma_MemoryCopy2D(    // copies memory in a 1D fashion
    ADI_DMA_STREAM_HANDLE   StreamHandle,   // stream handle
    ADI_DMA_2D_TRANSFER     *pDest,         // destination channel configuration
    ADI_DMA_2D_TRANSFER     *pSrc,          // source channel configuration
    u32                     ElementWidth,   // element width (in bytes)
    ADI_DCB_CALLBACK_FN     ClientCallback  // client callback function (Synchronous when NULL)
){

    ADI_DMA_RESULT                      Result;                 // return code
    ADI_DMA_CHANNEL                     *pDestinationChannel;   // pointer to the destination channel
    const ADI_DMA_MEMORY_STREAM_INFO    *pInfo;                 // pointer to a memory info entry
    ADI_DMA_CONFIG_REG                  SourceConfig;           // source config register
    ADI_DMA_CONFIG_REG                  DestinationConfig;      // source config register
    ADI_DMA_MEMORY_COPY_DATA            *pMemoryCopyData;       // pointer to the memory copy data
    u32                                 i;                      // counter
    void                                *pExitCriticalArg;      // return value from EnterCritical
    ADI_DMA_DESCRIPTOR_LARGE            *pSourceDescriptor;     // source descriptor
    ADI_DMA_DESCRIPTOR_LARGE            *pDestinationDescriptor;    // destination descriptor

    // assume we'll be successful
    Result = ADI_DMA_RESULT_SUCCESS;

    // validate the stream handle
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateStreamHandle (StreamHandle)) != ADI_DMA_RESULT_SUCCESS) return(Result);
#endif

    // avoid casts
    pDestinationChannel = (ADI_DMA_CHANNEL *)StreamHandle;

    // if synchronous and debug is active, verify that the current interrupt level is lower than the memDMA interrupt level
#if defined(ADI_SSL_DEBUG)
    u32 DMAIVG;
    u32 CurrentIVG;
    if (!ClientCallback) {
        adi_int_SICGetIVG(pDestinationChannel->PeripheralID, &DMAIVG);
        if (adi_int_GetCurrentIVGLevel(&CurrentIVG) == ADI_INT_RESULT_SUCCESS) {
            if (CurrentIVG < DMAIVG) {
                return (ADI_DMA_RESULT_INCOMPATIBLE_IVG_LEVEL);
            }
        }
    }
#endif


    // point to the data structure for this stream
    for (i = 0, pInfo = MemoryStreamInfoTable; i < sizeof(MemoryStreamInfoTable)/sizeof(ADI_DMA_MEMORY_STREAM_INFO); i++, pInfo++) {
        if (pInfo->DestinationChannelID == pDestinationChannel->ChannelID) {
            pMemoryCopyData = &MemoryCopyData[i];
            break;
        }
    }

    // try and allocate the stream
    pExitCriticalArg = adi_int_EnterCriticalRegion(pDestinationChannel->pManager->pEnterCriticalArg);
    if (pMemoryCopyData->TransferInProgressFlag) {
        Result = ADI_DMA_RESULT_IN_USE;
    } else {
        pMemoryCopyData->TransferInProgressFlag = TRUE;
    }
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // IF (we allocated the stream)
    if (Result == ADI_DMA_RESULT_SUCCESS) {

        // save the client callback
        pMemoryCopyData->ClientCallback = ClientCallback;

        // point to the descriptors
        pSourceDescriptor = &pMemoryCopyData->SourceDescriptor;
        pDestinationDescriptor = &pMemoryCopyData->DestinationDescriptor;

        // create the source descriptor
        SourceConfig = DefaultSourceConfig2D;
        SourceConfig.b_WDSIZE = ElementWidth >> 1;
        pSourceDescriptor->Config = SourceConfig;
        pSourceDescriptor->StartAddress = pSrc->StartAddress;
        pSourceDescriptor->XCount = pSrc->XCount;
        pSourceDescriptor->XModify = pSrc->XModify;
        pSourceDescriptor->YCount = pSrc->YCount;
        pSourceDescriptor->YModify = pSrc->YModify;

        // create the destination descriptor
        DestinationConfig = DefaultDestinationConfig2D;
        DestinationConfig.b_WDSIZE = ElementWidth >> 1;
        pDestinationDescriptor->Config = DestinationConfig;
        pDestinationDescriptor->StartAddress = pDest->StartAddress;
        pDestinationDescriptor->XCount = pDest->XCount;
        pDestinationDescriptor->XModify = pDest->XModify;
        pDestinationDescriptor->YCount = pDest->YCount;
        pDestinationDescriptor->YModify = pDest->YModify;

        // setup a callback on the destination channel so that our internal callback will always run
        pDestinationDescriptor->CallbackFlag = TRUE;

        // IF (we can successfully submit the job)
        if ((Result = adi_dma_MemoryQueue(StreamHandle, pSourceDescriptor, pDestinationDescriptor)) == ADI_DMA_RESULT_SUCCESS) {

            // IF (we can start the transfer)
            if ((Result = adi_dma_MemoryQueueControl(StreamHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)TRUE)) == ADI_DMA_RESULT_SUCCESS) {

                // IF (synchronous)
                if (ClientCallback == NULL) {

                    // wait for completion
                    while (pMemoryCopyData->TransferInProgressFlag == TRUE) {
                        i++;
                    }

                // ENDIF
                }

            // ENDIF
            }

        // ENDIF
        }

        // mark that the stream is free if we had any errors
        if (Result != ADI_DMA_RESULT_SUCCESS) {
            pMemoryCopyData->TransferInProgressFlag = FALSE;
        }

    // ENDIF
    }

    // return
    return (Result);
}




/*********************************************************************

    Function:       adi_dma_MemoryQueueOpen

    Description:    Opens a memory DMA channel for use when queueing
                    memory DMA descriptors.

                    Memory DMA requires two channels, a source channel
                    and a destination channel.  As the destination channel
                    always finishes last in any memory DMA transaction,
                    the destination channel is used for callback triggers
                    while the source channel should never generate any
                    callbacks.

                    When a stream is opened, here is the sequence:
                        o Source channel is opened first
                        o The client handle given to the source channel
                          is the real client handle that the caller gave us.
                        o The destination channel is then opened
                        o The client handle given to the destination channel is
                          is the real client handle that the caller gave us.
                        o The client is give the handle to the destination channel.

                    This allows us to treat both the source and destination
                    channels like any other DMA channel when interrupts from
                    these channels are processed.  It also forces the client
                    to give us the destiantion channel handle, from which we can
                    get the source channel handle.

*********************************************************************/


ADI_DMA_RESULT adi_dma_MemoryQueueOpen(
    ADI_DMA_MANAGER_HANDLE  ManagerHandle,      // DMA manager handle
    ADI_DMA_STREAM_ID       StreamID,           // memory stream ID
    void                    *ClientHandle,      // Client value
    ADI_DMA_STREAM_HANDLE   *pStreamHandle,     // address where the channel handle will be stored
    void                    *DCBHandle,         // deferred callback service handle
    ADI_DCB_CALLBACK_FN     ClientCallback      // client callback function
) {

    ADI_DMA_RESULT                      Result;             // return code
    const ADI_DMA_MEMORY_STREAM_INFO    *pInfo;             // pointer to info for the memory stream
    ADI_DMA_CHANNEL_HANDLE              SourceHandle;       // handle to the source channel
    ADI_DMA_CHANNEL_HANDLE              DestinationHandle;  // handle to the destination channel


    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateManagerHandle(ManagerHandle)) != ADI_DMA_RESULT_SUCCESS) return(Result);
    if ((Result = ValidateStreamID(StreamID)) != ADI_DMA_RESULT_SUCCESS) return(Result);
#endif

    // point to the info for this memory stream
    pInfo = &MemoryStreamInfoTable[StreamID];

    // open the source channel
    if ((Result = adi_dma_Open(ManagerHandle, pInfo->SourceChannelID, ClientHandle, &SourceHandle, ADI_DMA_MODE_DESCRIPTOR_LARGE, DCBHandle, ClientCallback)) != ADI_DMA_RESULT_SUCCESS) {
        return (Result);
    }

    // open the destination channel
    if ((Result = adi_dma_Open(ManagerHandle, pInfo->DestinationChannelID, ClientHandle, &DestinationHandle, ADI_DMA_MODE_DESCRIPTOR_LARGE, DCBHandle, ClientCallback)) != ADI_DMA_RESULT_SUCCESS) {
        adi_dma_Close (SourceHandle, FALSE);
        return (Result);
    }

    // give the client the handle to the stream (really the destination channel handle)
    *pStreamHandle = DestinationHandle;

    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_dma_MemoryQueueClose

    Description:    Closed down the source and destination channels.

*********************************************************************/


ADI_DMA_RESULT adi_dma_MemoryQueueClose(
        ADI_DMA_STREAM_HANDLE   StreamHandle,   // stream handle
        u32                     WaitFlag        // wait flag
) {

    ADI_DMA_RESULT  SourceResult;           // return code
    ADI_DMA_RESULT  DestinationResult;      // return code
    ADI_DMA_CHANNEL *pDestinationChannel;   // pointer to the destination channel
    ADI_DMA_CHANNEL *pSourceChannel;        // pointer to the source channel


    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if ((DestinationResult = ValidateStreamHandle (StreamHandle)) != ADI_DMA_RESULT_SUCCESS) return(DestinationResult);
#endif

    // avoid casts
    pDestinationChannel = (ADI_DMA_CHANNEL *)StreamHandle;
    pSourceChannel = pDestinationChannel->pPairingChannel;

    // close the channels down
    DestinationResult = adi_dma_Close(pDestinationChannel, WaitFlag);
    SourceResult = adi_dma_Close(pSourceChannel, WaitFlag);

    // return (any error code else success)
    if (DestinationResult != ADI_DMA_RESULT_SUCCESS) return (DestinationResult);
    return (SourceResult);
}



/*********************************************************************

    Function:       adi_dma_MemoryQueueControl

    Description:    Sets/senses memory DMA configuration and control parameters

*********************************************************************/


ADI_DMA_RESULT adi_dma_MemoryQueueControl(  // Controls/configures a memory DMA stream
    ADI_DMA_STREAM_HANDLE   StreamHandle,       // stream handle
    ADI_DMA_CMD             Command,            // command ID
    void                    *Value              // command specific value
) {

    ADI_DMA_CHANNEL *pDestinationChannel;   // pointer to the destination channel
    ADI_DMA_CHANNEL *pSourceChannel;        // pointer to the source channel
    ADI_DMA_RESULT  Result;                 // return value

    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateStreamHandle (StreamHandle)) != ADI_DMA_RESULT_SUCCESS) return(Result);
#endif

    // avoid casts
    pDestinationChannel = (ADI_DMA_CHANNEL *)StreamHandle;
    pSourceChannel = pDestinationChannel->pPairingChannel;

    // pass the command to the source and destination channels
    if ((Result = adi_dma_Control(pSourceChannel, Command, Value)) != ADI_DMA_RESULT_SUCCESS) {
        return (Result);
    }
    if ((Result = adi_dma_Control(pDestinationChannel, Command, Value)) != ADI_DMA_RESULT_SUCCESS) {
        return (Result);
    }

    // return
    return(Result);
}




/*********************************************************************

    Function:       adi_dma_MemoryQueue

    Description:    Queues memory DMA descriptors to a memory stream

*********************************************************************/

ADI_DMA_RESULT adi_dma_MemoryQueue(
    ADI_DMA_STREAM_HANDLE       StreamHandle,               // stream handle
    ADI_DMA_DESCRIPTOR_LARGE    *pSourceDescriptor,         // source descriptor handle
    ADI_DMA_DESCRIPTOR_LARGE    *pDestinationDescriptor     // destination descriptor handle
) {

    ADI_DMA_RESULT      Result;                 // return code
    ADI_DMA_CHANNEL     *pDestinationChannel;   // pointer to the destination channel
    ADI_DMA_CHANNEL     *pSourceChannel;        // pointer to the source channel
    ADI_DMA_DESCRIPTOR_LARGE    *pLastSourceDescriptor;         // source descriptor handle
    ADI_DMA_DESCRIPTOR_LARGE    *pLastDestinationDescriptor;         // source descriptor handle

    // avoid casts
    pDestinationChannel = (ADI_DMA_CHANNEL *)StreamHandle;
    pSourceChannel = pDestinationChannel->pPairingChannel;

    // IF (debug version)
#   if defined(ADI_SSL_DEBUG)

        // validate the stream handle
        if ((Result = ValidateStreamHandle(StreamHandle)) != ADI_DMA_RESULT_SUCCESS) {
            return(Result);
        }

        // validate the descriptor streams
        if ((Result = ValidateDescriptorChain(pSourceChannel, (ADI_DMA_DESCRIPTOR_HANDLE)pSourceDescriptor)) != ADI_DMA_RESULT_SUCCESS) {
            return (Result);
        }
        if ((Result = ValidateDescriptorChain(pDestinationChannel, (ADI_DMA_DESCRIPTOR_HANDLE)pDestinationDescriptor)) != ADI_DMA_RESULT_SUCCESS) {
            return (Result);
        }

        // validate the source and destination streams
        if ((Result = ValidateStreamDescriptors(pSourceDescriptor, pDestinationDescriptor)) != ADI_DMA_RESULT_SUCCESS) {
            return (Result);
        }

    // ENDIF
#   endif

    // set the configuration word for all descriptors in the destination chain and make note of the last descriptor in the chain
    pLastDestinationDescriptor = (ADI_DMA_DESCRIPTOR_LARGE *)SetConfigWord(pDestinationChannel, (ADI_DMA_DESCRIPTOR_HANDLE)pDestinationDescriptor);

    // set the configuration word for all descriptors in the source chain chain and make note of the last descriptor in the chain
    pLastSourceDescriptor = (ADI_DMA_DESCRIPTOR_LARGE *)SetConfigWord(pSourceChannel, (ADI_DMA_DESCRIPTOR_HANDLE)pSourceDescriptor);

    // synchronize the source chain to the destination chain
    if ((Result = SyncSourceToDestination(pSourceDescriptor, pDestinationDescriptor)) != ADI_DMA_RESULT_SUCCESS) {
        return (Result);
    }

    // place the source chain on the proper queue (standby or active)
    AddChainToQueue(pSourceChannel, (ADI_DMA_DESCRIPTOR_HANDLE)pSourceDescriptor, (ADI_DMA_DESCRIPTOR_HANDLE)pLastSourceDescriptor);

    // place the destination chain on the proper queue (standby or active)
    AddChainToQueue(pDestinationChannel, (ADI_DMA_DESCRIPTOR_HANDLE)pDestinationDescriptor, (ADI_DMA_DESCRIPTOR_HANDLE)pLastDestinationDescriptor);

    // restart the source channel if we're paused
    if (pSourceChannel->Status == ADI_DMA_STATUS_PAUSED) {
        Result = adi_dma_Control(pSourceChannel, ADI_DMA_CMD_SET_DATAFLOW, (void *)TRUE);
    }

    // restart the destination channel if we're paused
    if (pDestinationChannel->Status == ADI_DMA_STATUS_PAUSED) {
        Result = adi_dma_Control(pDestinationChannel, ADI_DMA_CMD_SET_DATAFLOW, (void *)TRUE);
    }

    // return
    return(Result);
}








/*********************************************************************

    Function:       adi_dma_GetMapping

    Description:    Given a peripheral (a ADI_DMA_PMAP enumeration) this
                    function returns the ChannelID (a ADI_DMA_CHANNEL_ID
                    enumeration) to which the peripheral is mapped.

*********************************************************************/


ADI_DMA_RESULT adi_dma_GetMapping(      // Gets the channel ID to which a DMA compatible device is mapped
    ADI_DMA_PMAP        pmap,               // pmap
    ADI_DMA_CHANNEL_ID  *pChannelID         // location where the channel ID of the peripheral will be stored
) {

    int                         i;              // generic counter
    u16                         PeripheralMap;  // peripheral map register
    const ADI_DMA_CHANNEL_INFO  *pTable;        // pointer to channel info
    u32                         ControllerID;   // controller ID
    u32                         pmapField;      // pmap field
    ADI_DMA_RESULT              Result;         // return code

#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidatePMAP(pmap)) != ADI_DMA_RESULT_SUCCESS) return(ADI_DMA_RESULT_BAD_PERIPHERAL);
#endif

    // assume we won't find it mapped
    Result = ADI_DMA_RESULT_NOT_MAPPED;

    // determine the controller number and pmap field
    ControllerID = ADI_DMA_GET_CONTROLLER_ID(pmap);
    pmapField = ADI_DMA_GET_PMAP_FIELD(pmap);

    // FOR (each channel)
    for (i = 0, pTable = ChannelInfoTable; i < ADI_DMA_CHANNEL_COUNT; i++, pTable++) {

        // IF (this channel is on the proper controller)
        if (ControllerID == pTable->ControllerID) {

            // IF (the peripheral is mapped to this channel)
            PeripheralMap = *((volatile u16 *)(pTable->BaseAddress + 0x2c));
            if (ADI_DMA_EXTRACT_PMAP_FIELD_FROM_REGISTER(PeripheralMap) == pmapField) {

                // save the channel ID in the clients location and return
                *pChannelID = (ADI_DMA_CHANNEL_ID)i;
                Result = ADI_DMA_RESULT_SUCCESS;
                break;

            // ENDIF
            }

        // ENDIF
        }

    // ENDFOR
    }

    // return
    return (Result);
}



/*********************************************************************

    Function:       adi_dma_SetMapping

    Description:    Given a channel ID (a ADI_DMA_CHANNEL_ID enumeration)
                    sets the corresponding peripheral map register to the
                    specified peripheral (a ADI_DMA_PMAP enumeration).

*********************************************************************/


ADI_DMA_RESULT adi_dma_SetMapping(      // Sets the channel ID to which a DMA compatible device is mapped
    ADI_DMA_PMAP        pmap,               // peripheral
    ADI_DMA_CHANNEL_ID  ChannelID           // channel ID to which the peripheral will be mapped
) {

    volatile u16    *pPeripheralMap;// peripheral map register
    u16             PeripheralMap;  // peripheral map register value
    u32             pmapField;      // pmap field value
    ADI_DMA_RESULT  Result;         // return code


#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidatePMAP(pmap)) != ADI_DMA_RESULT_SUCCESS) return(ADI_DMA_RESULT_BAD_PERIPHERAL);
    if ((Result = ValidateChannelID(ChannelID)) != ADI_DMA_RESULT_SUCCESS) return(ADI_DMA_RESULT_BAD_CHANNEL_ID);
#endif

    // get the address of the PMAP register for the channel
    pPeripheralMap = (volatile u16 *)(ChannelInfoTable[ChannelID].BaseAddress + 0x2c);

    // determine the pmap field value
    pmapField = ADI_DMA_GET_PMAP_FIELD(pmap);

    // read the register
    PeripheralMap = *pPeripheralMap;

    // set the mapping
    ADI_DMA_DEPOSIT_PMAP_FIELD_INTO_REGISTER(PeripheralMap, pmapField);

    // write the register
    *pPeripheralMap = PeripheralMap;

    // return
    return (ADI_DMA_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       adi_dma_GetPeripheralInterruptID

    Description:    Given a DMA channel, this function returns the
                    peripheral interrupt ID for the channel.

*********************************************************************/
ADI_DMA_RESULT adi_dma_GetPeripheralInterruptID(    // Gets the peripheral interrupt ID for a given DMA channel ID
    ADI_DMA_CHANNEL_ID      ChannelID,                  // DMA channel ID
    ADI_INT_PERIPHERAL_ID   *pPeripheralID              // location where the peripheral ID will be stored
) {

    ADI_DMA_RESULT  Result;     // return code

#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateChannelID(ChannelID)) != ADI_DMA_RESULT_SUCCESS) return(ADI_DMA_RESULT_BAD_CHANNEL_ID);
#endif

    // store the value in the location provided
    *pPeripheralID = ChannelInfoTable[ChannelID].PeripheralID;

    // return
    return (ADI_DMA_RESULT_SUCCESS);
}




/*********************************************************************

    Function:       DetermineNextDescriptorInterrupt

    Description:    Updates the pointer within a channel structure to point
                    to the next descriptor that will generate an interrupt on the
                    channel starting from the point in the chain indicated by the
                    given descriptor.  It's necessary to keep track of this as the
                    DMA hardware does not provide sufficient information to determine
                    which descriptor caused a DMA interrupt to trigger even when
                    the descriptor is configured to halt dataflow and generate an
                    interrupt.

                    Originally, the DI_EN bit was being checked exclusively,
                    however with the addition of queueing memDMA transfers, as the
                    DMA controller generates an error if the DI_EN bit is ever
                    set on a source channel, we need to special case the source
                    memDMA channel and check for the FLOW field being set to stop
                    rather than DI_EN.  Further, if streaming is enabled, the FLOW
                    field will never be set to stop so there is no easy way to
                    predict the descriptor on the source channel that correlates
                    to the interrupting descriptor on the destination channel.
                    This is why in the data interrupt handler, the source channel
                    is not processed if streaming is true.

*********************************************************************/

static void DetermineNextDescriptorInterrupt(
        ADI_DMA_CHANNEL             *pChannel,      // channel
        ADI_DMA_DESCRIPTOR_UNION    *pDescriptor    // head of chain
) {

    u32 UpperAddress;       // holder for the upper 16 bits of address for small descriptors

    // IF (the channel is a source memDMA channel AND streaming is enabled)
    if ((pChannel->ChannelType == ADI_DMA_TYPE_MEMORY_SOURCE) && (pChannel->StreamingFlag == TRUE)) {

        // there's no way to tell
        pChannel->pNextCallback = NULL;

    // ELSE
    } else {

        // CASEOF (descriptor type)
        switch (pChannel->Mode) {

            // CASE (large descriptors)
            case (ADI_DMA_MODE_DESCRIPTOR_LARGE):
            {

                // declare variables for this descriptor type
                ADI_DMA_DESCRIPTOR_LARGE    *pWorkingDescriptor;    // pointer to the descriptor we're working on

                // find the next descriptor in the chain that is generating an interrupt (watch for loopback)
                pWorkingDescriptor = &pDescriptor->Large;
                while (pWorkingDescriptor != NULL) {
                    if ((pWorkingDescriptor->Config.b_DI_EN == ADI_DMA_DI_EN_ENABLE) || (pWorkingDescriptor->Config.b_FLOW == ADI_DMA_FLOW_STOP)) {
                        break;
                    }
                    pWorkingDescriptor = pWorkingDescriptor->pNext;
                    if (pWorkingDescriptor == &pDescriptor->Large) pWorkingDescriptor = NULL;
                }

                // update the channel
                pChannel->pNextCallback = (ADI_DMA_DESCRIPTOR_UNION *)pWorkingDescriptor;
                break;
            }

            // CASE (small descriptors)
            case (ADI_DMA_MODE_DESCRIPTOR_SMALL):
            {

                // declare variables for this descriptor type
                ADI_DMA_DESCRIPTOR_SMALL    *pWorkingDescriptor;    // pointer to the descriptor we're working on

                // point to the first descriptor in the chain
                pWorkingDescriptor = &pDescriptor->Small;

                // in the small model, pNext is only a 16 bit address so when we walk the list
                // of descriptors, we need to insure we use the upper 16 bits of the first
                // descriptor that is passed in so that we properly walk the list
                UpperAddress = (u32)(pWorkingDescriptor) & 0xffff0000;

                // find the next descriptor in the chain that will generate an interrupt (watch for loopback)
                // and remember to only check the lower 16 bits cause we're small!
                // and watch for loopback
                while (((u32)pWorkingDescriptor & 0xffff) != 0) {
                    if ((pWorkingDescriptor->Config.b_DI_EN == ADI_DMA_DI_EN_ENABLE) || (pWorkingDescriptor->Config.b_FLOW == ADI_DMA_FLOW_STOP)) {
                        break;
                    }
                    pWorkingDescriptor = (ADI_DMA_DESCRIPTOR_SMALL *)((u32)pWorkingDescriptor->pNext | UpperAddress);
                    if (pWorkingDescriptor == &pDescriptor->Small) pWorkingDescriptor = NULL;
                }

                // update the channel
                pChannel->pNextCallback = (ADI_DMA_DESCRIPTOR_UNION *)pWorkingDescriptor;
                break;
            }

            // CASE (descriptor array)
            case (ADI_DMA_MODE_DESCRIPTOR_ARRAY):
            {

                // tbd ****************************
                break;
            }

        // ENDCASE
        }

    // ENDIF
    }

    // return
}



/*********************************************************************

    Function:       SetDataflow

    Description:    Enables or disables dataflow on a DMA channel

*********************************************************************/


static ADI_DMA_RESULT SetDataflow(
    ADI_DMA_CHANNEL *pChannel,  // channel
    u32             Flag        // true/false flag
) {

    ADI_DMA_RESULT  Result;             // return value
    void            *pExitCriticalArg;  // return value from EnterCritical

    // assume we'll be successful
    Result = ADI_DMA_RESULT_SUCCESS;

    // do error checking if enabled
#if defined(ADI_SSL_DEBUG)
    if (((pChannel->Mode == ADI_DMA_MODE_CIRCULAR) || (pChannel->Mode == ADI_DMA_MODE_CIRCULAR)) && (Flag == TRUE)) {       // insure we have a buffer ready
        if (pChannel->BufferReadyFlag != TRUE) {
            return(ADI_DMA_RESULT_NO_BUFFER);
        }
    }
#endif

    // IF (enabling dataflow)
    if (Flag == TRUE) {

        // protect this region
        pExitCriticalArg = adi_int_EnterCriticalRegion(pChannel->pManager->pEnterCriticalArg);

        // IF (the channel isn't already running)
        if (pChannel->Status != ADI_DMA_STATUS_RUNNING) {

            // CASEOF (operating mode)
            switch (pChannel->Mode) {

            // CASE (single buffer, circular buffers or descriptor array)
            case ADI_DMA_MODE_SINGLE:
            case ADI_DMA_MODE_CIRCULAR:
            case ADI_DMA_MODE_DESCRIPTOR_ARRAY:

                // enable the DMA channel and update our status to running
                pChannel->Config->b_DMA_EN = ADI_DMA_EN_ENABLE;
                pChannel->Status = ADI_DMA_STATUS_RUNNING;
                break;

            // CASE (large or small descriptors)
            case ADI_DMA_MODE_DESCRIPTOR_LARGE:
            case ADI_DMA_MODE_DESCRIPTOR_SMALL:

                // IF (streaming mode is enabled)
                if (pChannel->StreamingFlag) {

                    // set our state to paused and do nothing more if we have no buffers queued
                    if (pChannel->pActiveHead == NULL) {
                        pChannel->Status = ADI_DMA_STATUS_PAUSED;
                        break;
                    }

                // ELSE
                } else {

                    // set our state to paused and do nothing more if we have no buffers queued
                    if (pChannel->pStandbyHead == NULL) {
                        pChannel->Status = ADI_DMA_STATUS_PAUSED;
                        break;
                    }

                    // move the standby queue over to the active queue
                    pChannel->pActiveHead = pChannel->pStandbyHead;
                    pChannel->pActiveTail = pChannel->pStandbyTail;
                    pChannel->pStandbyHead = NULL;
                    pChannel->pStandbyTail = NULL;

                    // halt DMA after the last descriptor on the queue if loopback is not enabled
                    // and force an interrupt on the last descriptor so we can update our channel status
                    // upon completion of the last descriptor and restart if necessary
/* dkl
    Not sure we need to do this.  Commented out on 2/22/07 till regressions run.
*/
#ifdef xyzzy
                    if (pChannel->LoopbackFlag == FALSE) {
                        if (pChannel->Mode == ADI_DMA_MODE_DESCRIPTOR_SMALL) {
                            pChannel->pActiveTail->Small.Config.b_FLOW = ADI_DMA_FLOW_STOP;
                            pChannel->pActiveTail->Small.Config.b_NDSIZE = ADI_DMA_NDSIZE_STOP;
                            pChannel->pActiveTail->Small.Config.b_DI_EN = ADI_DMA_DI_EN_ENABLE;
                        } else {
                            pChannel->pActiveTail->Large.Config.b_FLOW = ADI_DMA_FLOW_STOP;
                            pChannel->pActiveTail->Large.Config.b_NDSIZE = ADI_DMA_NDSIZE_STOP;
                            pChannel->pActiveTail->Large.Config.b_DI_EN = ADI_DMA_DI_EN_ENABLE;
                        }
                    }
#endif

                // ENDIF (streaming is enabled)
                }

                // identify which descriptor will cause the next interrupt
                DetermineNextDescriptorInterrupt(pChannel, pChannel->pActiveHead);

                // load the descriptor into the DMA controller
                if (pChannel->Mode == ADI_DMA_MODE_DESCRIPTOR_SMALL) {
                    *pChannel->NextDescPtr = pChannel->pActiveHead;
                    *pChannel->StartAddr = (volatile void *)(((pChannel->pActiveHead->Small.StartAddressHigh) << 16) | pChannel->pActiveHead->Small.StartAddressLow);
                    *pChannel->XCount = pChannel->pActiveHead->Small.XCount;
                    *pChannel->XModify = pChannel->pActiveHead->Small.XModify;
                    *pChannel->YCount = pChannel->pActiveHead->Small.YCount;
                    *pChannel->YModify = pChannel->pActiveHead->Small.YModify;
                    *pChannel->Config = pChannel->pActiveHead->Small.Config;
                } else {
                    *pChannel->NextDescPtr = pChannel->pActiveHead;
                    *pChannel->StartAddr = pChannel->pActiveHead->Large.StartAddress;
                    *pChannel->XCount = pChannel->pActiveHead->Large.XCount;
                    *pChannel->XModify = pChannel->pActiveHead->Large.XModify;
                    *pChannel->YCount = pChannel->pActiveHead->Large.YCount;
                    *pChannel->YModify = pChannel->pActiveHead->Large.YModify;
                    *pChannel->Config = pChannel->pActiveHead->Large.Config;
                }

                // update our status to running
                pChannel->Status = ADI_DMA_STATUS_RUNNING;
                break;

            // ENDCASE
            }

        // ENDIF
        }

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ELSE (disabling dataflow)
    } else {

        // protect this region
        pExitCriticalArg = adi_int_EnterCriticalRegion(pChannel->pManager->pEnterCriticalArg);

        // halt DMA
        pChannel->Config->b_DMA_EN = ADI_DMA_EN_DISABLE;

        // clear both the active and standby queue
        pChannel->pActiveHead = NULL;
        pChannel->pActiveTail = NULL;
        pChannel->pStandbyHead = NULL;
        pChannel->pStandbyTail = NULL;

        // set our status flag
        pChannel->Status = ADI_DMA_STATUS_STOPPED;

        // clear any pending interrupts for the DMA channel (need to do this funky thing with the casts cause
        // we get a DMA error if we try and write to RO bits)
//#warning
//      *(u16 *)pChannel->IRQStatus = ADI_DMA_DONE_DONE;

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ENDIF
    }

    // return
    return(Result);
}




/*********************************************************************

    Function:       ErrorHandler

    Description:    Processes DMA error interrupts

*********************************************************************/

static ADI_INT_HANDLER(ErrorHandler)    // DMA error handler
{

    unsigned int            i;          // counter
    ADI_INT_HANDLER_RESULT  Result;     // result
    ADI_DMA_CHANNEL         *pChannel;  // pointer to the channel we're working on

    // avoid casts
    pChannel = (ADI_DMA_CHANNEL *)ClientArg;
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    // IF (this channel is asserting an error interrupt)
    if (pChannel->IRQStatus->b_DMA_ERR == ADI_DMA_ERR_ERROR) {

        // clear the interrupt (need to do this funky thing with the casts cause
        // we get a DMA error if we try and write to RO bits)
        *(u16 *)pChannel->IRQStatus = ADI_DMA_ERR_ERROR << 1;

        // notify the callback function that an error occurred (making sure there actually is a callback function)
        if (pChannel->ClientCallback) {
            if (pChannel->DCBHandle) {
                adi_dcb_Post(pChannel->DCBHandle, 0, pChannel->ClientCallback, pChannel->ClientHandle, ADI_DMA_EVENT_ERROR_INTERRUPT, NULL);
            } else {
                (pChannel->ClientCallback)(pChannel->ClientHandle, ADI_DMA_EVENT_ERROR_INTERRUPT, NULL);
            }
        }

        // indicate that we processed the interrupt
        Result = ADI_INT_RESULT_PROCESSED;

    // ENDIF
    }

    // return
    return(Result);
}



/*********************************************************************

    Function:       DataHandler

    Description:    Processes DMA channel interrupts

                    This function checks to verify the DMA channel that
                    is asserting, clears the hardware interrupt then
                    calls the ProcessDataInterrupt function to do the
                    bulk of the processing.

                    The primary reason for this two step approach is
                    due to memory DMA transfers.  When doing memory
                    DMA transfers, only the destination channel generates
                    interrupts; the controller generates a DMA error
                    if the source channel ever generates an interrupt.
                    As a result, when we process an interrupt from the
                    destination channel, we need to do virtually idential
                    processing of the source channel.  By breaking the
                    processing up into these two functions, we can call
                    the ProcessDataInterrupt function for the source
                    channel when the destination channel generates the
                    interrupt.

                    Processing is not called on the source memDMA channel
                    if streaming is active because there is no need to
                    call it and because there is no way we can predict
                    the next descriptor on the source channel that maps
                    to the corresponding descriptor on the destination
                    channel.

*********************************************************************/

static ADI_INT_HANDLER(DataHandler) // DMA handler
{


    unsigned int                i;                  // counter
    ADI_INT_HANDLER_RESULT      Result;             // result
    ADI_DMA_CHANNEL             *pChannel;          // pointer to the channel we're working on
    u32                         AssertedFlag;       // flag indicating whether or not the channel has an interrupt asserted

    // avoid casts
    pChannel = (ADI_DMA_CHANNEL *)ClientArg;
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    // assume the channel is not interrupting
    AssertedFlag = FALSE;

    // IF (this channel is asserting an interrupt cause DMA_DONE is set)
    if (pChannel->IRQStatus->b_DMA_DONE == ADI_DMA_DONE_DONE) {

        // clear the interrupt (need to do this funky thing with the casts cause
        // we get a DMA error if we try and write to RO bits)
        *(u16 *)pChannel->IRQStatus = ADI_DMA_DONE_DONE;

        // set flag to show this channel is interrupting
        AssertedFlag = TRUE;

    // ENDIF
    }

    // IF (this channel is interrupting)
    if (AssertedFlag) {

        // IF (this is a destination memory DMA channel)
        if (pChannel->ChannelType == ADI_DMA_TYPE_MEMORY_DESTINATION) {

            // IF (not streaming)
            if (pChannel->StreamingFlag == FALSE) {

                // process the source channel (but don't make any callbacks on the source channel)
                ProcessDataInterrupt((ADI_DMA_CHANNEL *)pChannel->pPairingChannel, FALSE);

            // ENDIF
            }

        // ENDIF
        }

        // process the channel
        ProcessDataInterrupt(pChannel, TRUE);

        // indicate that we processed the interrupt
        Result = ADI_INT_RESULT_PROCESSED;

    // ENDIF
    }

    // return
    return (Result);
}



/*********************************************************************

    Function:       ProcessDataInterrupt

    Description:    Processes DMA channel interrupts
                    Channels operating in circular and one-shots are
                    fairly straightforward.  The other modes are more
                    complex and depend on the state of loopback and
                    streaming mode.  The table below describes the
                    processing for each of the methods depending on
                    these modes:

                    Loopback    Streaming       Action taken
                    --------    ---------       ------------

                    False       False           purge active list
                                                append standby list
                                                reload next descriptor
                                                predict next callback from head of active
                                                make the callback that just triggered

                    False       True            purge active list
                                                predict next callback from head of active
                                                make the callback that just triggered

                    True        False           reload next descriptor
                                                predict next callback from pNext of current descriptor
                                                make the callback that just triggered

                    True        True            predict next callback from pNext of current descriptor
                                                make the callback that just triggered

*********************************************************************/

static void ProcessDataInterrupt(ADI_DMA_CHANNEL *pChannel, u32 AllowCallbacksFlag)
{


    ADI_DMA_DESCRIPTOR_UNION    *pDescriptor;       // pointer to a descriptor
    ADI_DMA_DESCRIPTOR_UNION    *pUpdateDescriptor; // pointer to the descriptor we'll predict from
    ADI_DMA_EVENT               Event;              // event to report to client
    void                        *pArg;              // void * parameter for callback function
    void                        *ClientHandle;      // client handle
    u32                         MakeCallbackFlag;   // flag indicating we need to make a callback
    void                        *pExitCriticalArg;  // return value from EnterCritical


    // point to the descriptor that we predicted should have generated the callback
    // this isn't used for circular or one-shots but does no harm and makes the code simpler
    pDescriptor = pChannel->pNextCallback;

    // setup the callback parameters
    ClientHandle = pChannel->ClientHandle;
    Event = ADI_DMA_EVENT_DESCRIPTOR_PROCESSED;
    pArg = pDescriptor;
    MakeCallbackFlag = TRUE;

    // CASEOF (operating mode)
    switch (pChannel->Mode) {

        // CASE (circular)
        case ADI_DMA_MODE_CIRCULAR:

            // determine if it's an inner loop interrupt or an outer loop interrupt
            if (pChannel->Config->b_DI_SEL == ADI_DMA_DI_SEL_OUTER_LOOP) {
                Event = ADI_DMA_EVENT_OUTER_LOOP_PROCESSED;
            } else {
                Event = ADI_DMA_EVENT_INNER_LOOP_PROCESSED;
            }

            // override the pArg callback parameter to be the buffer start address
            pArg = pChannel->StartAddr;
            break;

        // CASE (large descriptors)
        case ADI_DMA_MODE_DESCRIPTOR_LARGE:

            // IF (not streaming)
            if (pChannel->StreamingFlag == FALSE) {

                // IF (not using loopback)
                if (pChannel->LoopbackFlag == FALSE) {

                    // purge the active queue up through (and including) the descriptor that generated the callback
                    if ((pChannel->pActiveHead = (ADI_DMA_DESCRIPTOR_HANDLE)pDescriptor->Large.pNext) == NULL) {
                        pChannel->pActiveTail = NULL;
                    }

                    // append the standby queue to the active queue
                    if (pChannel->pStandbyHead != NULL) {
                        if (pChannel->pActiveHead == NULL) {
                            pChannel->pActiveHead = pChannel->pStandbyHead;
                        } else {
                            pChannel->pActiveTail->Large.pNext = &pChannel->pStandbyHead->Large;
                        }
                        pChannel->pActiveTail = pChannel->pStandbyTail;
                        pChannel->pStandbyHead = NULL;
                        pChannel->pStandbyTail = NULL;
                    }

                    // restart DMA and predict the next descriptor that will generate a callback with what's at the head of the active queue
                    pUpdateDescriptor = pChannel->pActiveHead;

                // ELSE (streaming FALSE, loopback TRUE)
                } else {

                    // predict the next descriptor that will generate a callback with what's at pNext from the current descriptor
                    pUpdateDescriptor = (ADI_DMA_DESCRIPTOR_UNION *)pDescriptor->Large.pNext;

                // ENDIF
                }

                // IF (we have a descriptor with which to restart DMA)
                if (pUpdateDescriptor) {

                    // load it into the controller
                    *pChannel->NextDescPtr  = pUpdateDescriptor;
                    *pChannel->StartAddr    = pUpdateDescriptor->Large.StartAddress;
                    *pChannel->XCount       = pUpdateDescriptor->Large.XCount;
                    *pChannel->XModify      = pUpdateDescriptor->Large.XModify;
                    *pChannel->YCount       = pUpdateDescriptor->Large.YCount;
                    *pChannel->YModify      = pUpdateDescriptor->Large.YModify;
                    *pChannel->Config       = pUpdateDescriptor->Large.Config;

                // ELSE
                } else {

                    // set the channel to paused
                    pChannel->Status = ADI_DMA_STATUS_PAUSED;

                // ENDIF
                }

            // ELSE (streaming TRUE)
            } else {

                // IF (not using loopback)
                if (pChannel->LoopbackFlag == FALSE) {

                    // purge the active queue up through (and including) the descriptor that generated the callback
                    if ((pChannel->pActiveHead = (ADI_DMA_DESCRIPTOR_HANDLE)pDescriptor->Large.pNext) == NULL) {
                        pChannel->pActiveTail = NULL;
                    }

                    // predict starting at the head of the active queue
                    pUpdateDescriptor = pChannel->pActiveHead;

                // ELSE (streaming TRUE, loopback TRUE)
                } else {

                    // predict starting at pNext from the current descriptor
                    pUpdateDescriptor = (ADI_DMA_DESCRIPTOR_UNION *)pDescriptor->Large.pNext;

                // ENDIF
                }

            // ENDIF
            }

            // determine if we should generate a callback
            MakeCallbackFlag = pDescriptor->Large.CallbackFlag;
            break;

        // CASE (small descriptors)
        case ADI_DMA_MODE_DESCRIPTOR_SMALL:

            // IF (not streaming)
            if (pChannel->StreamingFlag == FALSE) {

                // IF (not using loopback)
                if (pChannel->LoopbackFlag == FALSE) {

                    // purge the active queue up through (and including) the descriptor that generated the callback
                    if ((pChannel->pActiveHead = (ADI_DMA_DESCRIPTOR_HANDLE)pDescriptor->Small.pNext) == NULL) {
                        pChannel->pActiveTail = NULL;
                    }

                    // append the standby queue to the active queue
                    if (pChannel->pStandbyHead != NULL) {
                        if (pChannel->pActiveHead == NULL) {
                            pChannel->pActiveHead = pChannel->pStandbyHead;
                        } else {
                            pChannel->pActiveTail->Small.pNext = (u16)((u32)&pChannel->pStandbyHead->Small);
                        }
                        pChannel->pActiveTail = pChannel->pStandbyTail;
                        pChannel->pStandbyHead = NULL;
                        pChannel->pStandbyTail = NULL;
                    }

                    // predict the next descriptor that will generate a callback with what's at the head of the active queue
                    pUpdateDescriptor = pChannel->pActiveHead;

                // ELSE (streaming FALSE, loopback TRUE)
                } else {

                    // restart DMA and predict the next descriptor that will generate a callback with what's at pNext from the current descriptor
                    pUpdateDescriptor = (ADI_DMA_DESCRIPTOR_UNION *)((u32)pDescriptor->Small.pNext | ((u32)&pDescriptor->Small & 0xffff0000));

                // ENDIF
                }

                // IF (we have a descriptor with which to restart DMA)
                if (pUpdateDescriptor) {

                    // load it into the controller
                    *pChannel->NextDescPtr  = pUpdateDescriptor;
                    *pChannel->StartAddr    = (volatile void *)(((pUpdateDescriptor->Small.StartAddressHigh) << 16) | pUpdateDescriptor->Small.StartAddressLow);
                    *pChannel->XCount       = pUpdateDescriptor->Small.XCount;
                    *pChannel->XModify      = pUpdateDescriptor->Small.XModify;
                    *pChannel->YCount       = pUpdateDescriptor->Small.YCount;
                    *pChannel->YModify      = pUpdateDescriptor->Small.YModify;
                    *pChannel->Config       = pUpdateDescriptor->Small.Config;

                // ELSE
                } else {

                    // set the channel to paused
                    pChannel->Status = ADI_DMA_STATUS_PAUSED;

                // ENDIF
                }

            // ELSE (streaming TRUE)
            } else {

                // IF (not using loopback)
                if (pChannel->LoopbackFlag == FALSE) {

                    // purge the active queue up through (and including) the descriptor that generated the callback
                    if ((pChannel->pActiveHead = (ADI_DMA_DESCRIPTOR_HANDLE)pDescriptor->Small.pNext) == NULL) {
                        pChannel->pActiveTail = NULL;
                    }

                    // predict starting at the head of the active queue
                    pUpdateDescriptor = pChannel->pActiveHead;

                // ELSE (streaming TRUE, loopback TRUE)
                } else {

                    // predict starting at pNext from the current descriptor
                    pUpdateDescriptor = (ADI_DMA_DESCRIPTOR_UNION *)((u32)pDescriptor->Small.pNext | ((u32)&pDescriptor->Small & 0xffff0000));

                // ENDIF
                }

            // ENDIF
            }

            // determine if we should generate a callback
            MakeCallbackFlag = pDescriptor->Small.CallbackFlag;
            break;

        // CASE (descriptor array)
        case ADI_DMA_MODE_DESCRIPTOR_ARRAY:

            // tbd
            break;

        // CASE (one shot)
        case ADI_DMA_MODE_SINGLE:

            // check the config word to see if we need to generate a callback
            if (pChannel->Config->b_DI_EN == ADI_DMA_DI_EN_ENABLE) {
                MakeCallbackFlag = TRUE;
            } else {
                MakeCallbackFlag = FALSE;
            }

            // mark the channel as stopped
            pChannel->Status = ADI_DMA_STATUS_STOPPED;
            break;

    // ENDCASE
    }

    // identify which descriptor will cause the next interrupt
    DetermineNextDescriptorInterrupt((ADI_DMA_CHANNEL_HANDLE)pChannel, pUpdateDescriptor);

    // post the callback if this function thinks we should and the caller allows it (parameters are Client Handle, Event, pArg) if required
    if (MakeCallbackFlag && AllowCallbacksFlag) {
        if (pChannel->ClientCallback) {
            if (pChannel->DCBHandle) {
                adi_dcb_Post(pChannel->DCBHandle, 0, pChannel->ClientCallback, ClientHandle, Event, pArg);
            } else {
                (pChannel->ClientCallback)(ClientHandle, Event, pArg);
            }
        }
    }

    // return
}



/*********************************************************************

    Function:       SetChannelType

    Description:    Sets the channel type for a channel and for memory
                    DMA channels, it makes sure the destination channel
                    has the proper handle to the source channel and vice
                    versa

*********************************************************************/
static void SetChannelType(         // sets the channel type field for a channel
    ADI_DMA_CHANNEL     *pChannel,      // channel handle
    ADI_DMA_CHANNEL_ID  ChannelID)      // channel ID
{

    u32 i;
    const ADI_DMA_MEMORY_STREAM_INFO    *pInfo;             // pointer to info for the memory stream
    ADI_DMA_CHANNEL_ID                  PairingChannelID;   // ID of the pairing channel
    ADI_DMA_CHANNEL                     *pPairingChannel;   // pointer to the pairing channel

    // assume the channel is a peripheral DMA channel
    pChannel->ChannelType = ADI_DMA_TYPE_PERIPHERAL;

    // FOR (each entry in the memory stream table)
    for (i = 0, pInfo = MemoryStreamInfoTable; i < (sizeof(MemoryStreamInfoTable)/sizeof(MemoryStreamInfoTable[0])); i++, pInfo++) {

        // IF (it's a memory DMA source channel)
        if (ChannelID == pInfo->SourceChannelID) {

            // update the channel type
            pChannel->ChannelType = ADI_DMA_TYPE_MEMORY_SOURCE;

            // make note of the pairing channel
            PairingChannelID = pInfo->DestinationChannelID;

            // stop looking further
            break;

        // ENDIF
        }

        // IF (it's a memory DMA destinatin channel)
        if (ChannelID == pInfo->DestinationChannelID) {

            // update the channel type
            pChannel->ChannelType = ADI_DMA_TYPE_MEMORY_DESTINATION;

            // make note of the pairing channel
            PairingChannelID = pInfo->SourceChannelID;

            // stop looking further
            break;

        // ENDIF
        }

    // ENDFOR
    }

    // IF (the current channel is a memory DMA channel)
    if (pChannel->ChannelType != ADI_DMA_TYPE_PERIPHERAL) {

        // update the channel so that it has the channel handle of the pairing channel (if it's open) and vice versa
        for (i = pChannel->pManager->ChannelCount, pPairingChannel = pChannel->pManager->Channel; i; i--, pPairingChannel++) {
            if (pPairingChannel->InUseFlag == TRUE) {
                if (pPairingChannel->ChannelID == PairingChannelID) {
                    pPairingChannel->pPairingChannel = pChannel;
                    pChannel->pPairingChannel = pPairingChannel;
                    break;
                }
            }
        }

    // ENDIF
    }

    // return
}


/*********************************************************************

    Function:       adi_dma_SetConfigWord

    Description:    DMA Helper function - calls a local function (SetConfigWord)
                    to set the bits in the configuration word for a chain of
                    descriptors.
*********************************************************************/

ADI_DMA_RESULT adi_dma_SetConfigWord(               // Sets the bits in the configuration word for a chain of descriptors
    ADI_DMA_CHANNEL_HANDLE      ChannelHandle,      // channel handle
    ADI_DMA_DESCRIPTOR_HANDLE   DescriptorHandle    // descriptor chain
){

// do error checking if enabled
#if defined(ADI_SSL_DEBUG)

     ADI_DMA_RESULT Result;

    // validate the channel handle
    if ((Result = ValidateChannelHandle (ChannelHandle)) != ADI_DMA_RESULT_SUCCESS) {
        return(Result);
    }

    // validate the descriptor chain
    if ((Result = ValidateDescriptorChain(ChannelHandle, DescriptorHandle)) != ADI_DMA_RESULT_SUCCESS) {
        return (Result);
    }

#endif

    // pass the channel and descriptor info to the local function
    SetConfigWord((ADI_DMA_CHANNEL *)ChannelHandle,DescriptorHandle);

    return(ADI_DMA_RESULT_SUCCESS);
}

/*********************************************************************

    Function:       SetConfigWord

    Description:    Sets the bits in the configuration word for a chain of
                    descriptors.  This function should be called for all
                    peripheral DMA chains and all memory destination
                    chains but not memory source chains.

                    Returns the address of the last descriptor in the chain.

*********************************************************************/

static ADI_DMA_DESCRIPTOR_HANDLE SetConfigWord( // sets the bits in the config register for the chain being queued
    ADI_DMA_CHANNEL             *pChannel,          // channel handle
    ADI_DMA_DESCRIPTOR_HANDLE   DescriptorHandle)   // descriptor chain
{

    static ADI_DMA_CONFIG_REG LargeInterruptsAllowed[] = {   // table of config words for the large model when interrupts are allowed (peripheral and memory destination channels)
        //
        //  DMA_EN              WNR (set by client) WDSIZE (set by client)  DMA2D (set by client)   RESTART                 DI_SEL                      DI_EN                   NDSIZE                  FLOW                Reserved    Streaming   Loopback    Callback    Last in Q
        //
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     FALSE       FALSE       FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     FALSE       TRUE        FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     FALSE       TRUE        FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       TRUE        TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       TRUE        TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        FALSE       FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        FALSE       FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        FALSE       TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        FALSE       TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        TRUE        FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        TRUE        FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        TRUE        TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        TRUE        TRUE        TRUE
    };

    static ADI_DMA_CONFIG_REG LargeInterruptsNotAllowed[] = {   // table of config words for the large model when interrupts are not allowed (memory source channels)
        //
        //  DMA_EN              WNR (set by client) WDSIZE (set by client)  DMA2D (set by client)   RESTART                 DI_SEL                      DI_EN                   NDSIZE                  FLOW                Reserved    Streaming   Loopback    Callback    Last in Q
        //
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     FALSE       FALSE       FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     FALSE       TRUE        FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     FALSE       TRUE        FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       TRUE        TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       TRUE        TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        FALSE       FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        FALSE       FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        FALSE       TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        FALSE       TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        TRUE        FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        TRUE        FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        TRUE        TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_LARGE,   ADI_DMA_FLOW_LARGE, 0 }, //     TRUE        TRUE        TRUE        TRUE
    };

    static ADI_DMA_CONFIG_REG SmallInterruptsAllowed[] = {   // table of config words for the small model when interrupts are allowed (peripheral and memory destination channels)
        //
        //  DMA_EN              WNR (set by client) WDSIZE (set by client)  DMA2D (set by client)   RESTART                 DI_SEL                      DI_EN                   NDSIZE                  FLOW                Reserved    Streaming   Loopback    Callback    Last in Q
        //
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     FALSE       FALSE       FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     FALSE       TRUE        FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     FALSE       TRUE        FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       TRUE        TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       TRUE        TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        FALSE       FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        FALSE       FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        FALSE       TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        FALSE       TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        TRUE        FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        TRUE        FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        TRUE        TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_ENABLE,   ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        TRUE        TRUE        TRUE
    };

    static ADI_DMA_CONFIG_REG SmallInterruptsNotAllowed[] = {   // table of config words for the small model when interrupts are not allowed (memory source channels)
        //
        //  DMA_EN              WNR (set by client) WDSIZE (set by client)  DMA2D (set by client)   RESTART                 DI_SEL                      DI_EN                   NDSIZE                  FLOW                Reserved    Streaming   Loopback    Callback    Last in Q
        //
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     FALSE       FALSE       FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       FALSE       TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     FALSE       TRUE        FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     FALSE       TRUE        FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       TRUE        TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_FLOW_STOP,      ADI_DMA_NDSIZE_STOP,0 }, //     FALSE       TRUE        TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        FALSE       FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        FALSE       FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        FALSE       TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        FALSE       TRUE        TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        TRUE        FALSE       FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        TRUE        FALSE       TRUE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        TRUE        TRUE        FALSE
        {   ADI_DMA_EN_ENABLE,  0,                  0,                      0,                      ADI_DMA_RESTART_RETAIN, ADI_DMA_DI_SEL_OUTER_LOOP,  ADI_DMA_DI_EN_DISABLE,  ADI_DMA_NDSIZE_SMALL,   ADI_DMA_FLOW_SMALL, 0 }, //     TRUE        TRUE        TRUE        TRUE
    };

#define ADI_DMA_CONFIG_MASK (0x003e)                // mask to strip off all non-client fields in the config register
    u16                         ClientConfigWord;   // configuration word
    u16                         RealConfigWord;     // config word the DMA manager creates
    u32                         Index;              // index into table
    u32                         UpperAddress;       // holder for the upper 16 bits of address for small descriptors
    ADI_DMA_DESCRIPTOR_HANDLE   pLastDescriptor;    // last descriptor in the chain
    ADI_DMA_CONFIG_REG          *Table;             // table that is used to get the configuration word

    // CASEOF (descriptor type)
    switch (pChannel->Mode) {

        // CASE (large descriptors)
        case (ADI_DMA_MODE_DESCRIPTOR_LARGE):
        {

            // declare variables for this descriptor type
            ADI_DMA_DESCRIPTOR_LARGE    *pWorkingDescriptor;    // pointer to the descriptor we're working on

            // point to the proper config table
            if (pChannel->ChannelType == ADI_DMA_TYPE_MEMORY_SOURCE) {
                Table = LargeInterruptsNotAllowed;
            } else {
                Table = LargeInterruptsAllowed;
            }

            // FOR (each descriptor in the list passed in)
            for (pWorkingDescriptor = &DescriptorHandle->Large; pWorkingDescriptor != NULL; pWorkingDescriptor = pWorkingDescriptor->pNext) {

                // get the config word from the descriptor the client gave us
                ClientConfigWord = *((u16 *)&pWorkingDescriptor->Config);

                // strip off any fields that the DMA manager needs to control
                ClientConfigWord &= ADI_DMA_CONFIG_MASK;

                // calculate the index into the table
                if (pWorkingDescriptor->pNext) {
                    Index = (pChannel->StreamingFlag * 8) + (pChannel->LoopbackFlag * 4) + (pWorkingDescriptor->CallbackFlag * 2) + 0;
                } else {
                    Index = (pChannel->StreamingFlag * 8) + (pChannel->LoopbackFlag * 4) + (pWorkingDescriptor->CallbackFlag * 2) + 1;
                }

                // get the config word the DMA manager thinks it should be
                RealConfigWord = *((u16 *)(&Table[Index]));

                // add in the fields the client wants
                RealConfigWord |= ClientConfigWord;

                // update the descriptor
                pWorkingDescriptor->Config = *((ADI_DMA_CONFIG_REG *)&RealConfigWord);

                // make note of the last descriptor in the list
                pLastDescriptor = (ADI_DMA_DESCRIPTOR_HANDLE)pWorkingDescriptor;

            // ENDFOR
            }
        }
        break;

        // CASE (small descriptors)
        case (ADI_DMA_MODE_DESCRIPTOR_SMALL):
        {

            // declare variables for this descriptor type
            ADI_DMA_DESCRIPTOR_SMALL    *pWorkingDescriptor;    // pointer to the descriptor we're working on

            // point to the proper config table
            if (pChannel->ChannelType == ADI_DMA_TYPE_MEMORY_SOURCE) {
                Table = SmallInterruptsNotAllowed;
            } else {
                Table = SmallInterruptsAllowed;
            }

            // in the small model, pNext is only a 16 bit address so when we walk the list
            // of descriptors, we need to insure we use the upper 16 bits of the first
            // descriptor that is passed in so that we properly walk the list
            UpperAddress = (u32)(&DescriptorHandle->Small) & 0xffff0000;

            // point to the first descriptor in the chain
            pWorkingDescriptor = &DescriptorHandle->Small;

            // WHILE (there are more descriptors in the chain)
            while (((u32)pWorkingDescriptor & 0xffff) != 0) {

                // get the config word from the descriptor the client gave us
                ClientConfigWord = *((u16 *)&pWorkingDescriptor->Config);

                // strip off any fields that the DMA manager needs to control
                ClientConfigWord &= ADI_DMA_CONFIG_MASK;

                // calculate the index into the table
                if (pWorkingDescriptor->pNext) {
                    Index = (pChannel->StreamingFlag * 8) + (pChannel->LoopbackFlag * 4) + (pWorkingDescriptor->CallbackFlag * 2) + 0;
                } else {
                    Index = (pChannel->StreamingFlag * 8) + (pChannel->LoopbackFlag * 4) + (pWorkingDescriptor->CallbackFlag * 2) + 1;
                }

                // get the config word the DMA manager thinks it should be
                RealConfigWord = *((u16 *)(&Table[Index]));

                // add in the fields the client wants
                RealConfigWord |= ClientConfigWord;

                // update the descriptor
                pWorkingDescriptor->Config = *((ADI_DMA_CONFIG_REG *)&RealConfigWord);

                // make note of the last descriptor in the list
                pLastDescriptor = (ADI_DMA_DESCRIPTOR_HANDLE)pWorkingDescriptor;

                // get the address of next descriptor in the chain and remember to put in
                // the upper 16 bits
                pWorkingDescriptor = (ADI_DMA_DESCRIPTOR_SMALL *)((u32)(pWorkingDescriptor->pNext) | UpperAddress);

            // ENDWHILE
            }

        }
        break;

        // CASE (array of descriptors)
        case (ADI_DMA_MODE_DESCRIPTOR_ARRAY):

            // tbd
            pLastDescriptor = NULL;
            break;

        // CASEELSE
        default:

            // return that this is a bad mode
            pLastDescriptor = NULL;
            break;

    // ENDCASE
    }

    // return
    return(pLastDescriptor);
}



/*********************************************************************

    Function:       AddChainToQueue

    Description:    Adds a descriptor chain to the proper queue (standby
                    or active) for the given channel.

*********************************************************************/


static void AddChainToQueue(
        ADI_DMA_CHANNEL             *pChannel,          // channel
        ADI_DMA_DESCRIPTOR_HANDLE   FirstDescriptorHandle,  // first descriptor in the chain
        ADI_DMA_DESCRIPTOR_HANDLE   LastDescriptorHandle    // last descriptor in the chain
) {

    void                        *pExitCriticalArg;  // return value from EnterCritical
    ADI_DMA_DESCRIPTOR_UNION    **Head;             // pointer to the location that stores the head of the active or standby list
    ADI_DMA_DESCRIPTOR_UNION    **Tail;             // pointer to the location that stores the tail of the active or standby list

    // point to the head and tail pointers
    if (pChannel->StreamingFlag) {
        Head = &pChannel->pActiveHead;
        Tail = &pChannel->pActiveTail;
    } else {
        Head = &pChannel->pStandbyHead;
        Tail = &pChannel->pStandbyTail;
    }

    // CASEOF (descriptor type)
    switch (pChannel->Mode) {

        // CASE (large descriptors)
        case (ADI_DMA_MODE_DESCRIPTOR_LARGE):
        {

            // place the descriptors on the proper queue then update the last descriptor for loopback
            pExitCriticalArg = adi_int_EnterCriticalRegion(pChannel->pManager->pEnterCriticalArg);
            if (*Head == NULL) {
                *Head = FirstDescriptorHandle;
                *Tail = LastDescriptorHandle;
            } else {
                (*Tail)->Large.pNext = &FirstDescriptorHandle->Large;
                *Tail = LastDescriptorHandle;
            }
            if (pChannel->LoopbackFlag) {
                (*Tail)->Large.pNext = (ADI_DMA_DESCRIPTOR_LARGE *)*Head;
            }
            adi_int_ExitCriticalRegion(pExitCriticalArg);
        }
        break;

        // CASE (small descriptors)
        case (ADI_DMA_MODE_DESCRIPTOR_SMALL):
        {

            // place the descriptors on the proper queue then update the last descriptor for loopback
            pExitCriticalArg = adi_int_EnterCriticalRegion(pChannel->pManager->pEnterCriticalArg);
            if (*Head == NULL) {
                *Head = FirstDescriptorHandle;
                *Tail = LastDescriptorHandle;
            } else {
                (*Tail)->Small.pNext = (u16)((u32)(&FirstDescriptorHandle->Small));
                *Tail = LastDescriptorHandle;
            }
            if (pChannel->LoopbackFlag) {
                (*Tail)->Small.pNext = (u16)((u32)*Head);
            }
            adi_int_ExitCriticalRegion(pExitCriticalArg);
        }
        break;

        // CASE (array of descriptors)
        case (ADI_DMA_MODE_DESCRIPTOR_ARRAY):

            // tbd
            break;

        // CASEELSE
        default:

            // bad mode
            break;

    // ENDCASE
    }

    // return
}



/*********************************************************************

    Function:       SyncSourceToDestination

    Description:    Synchronizes configuration word settings for the
                    source channel based on the destination channel
                    settings

*********************************************************************/
static ADI_DMA_RESULT SyncSourceToDestination(  // syncs the source descriptor chain to the destination descriptor chain
    ADI_DMA_DESCRIPTOR_LARGE    *pSourceChain,      // source descriptor chain
    ADI_DMA_DESCRIPTOR_LARGE    *pDestinationChain) // destination descriptor chain
{

    ADI_DMA_DESCRIPTOR_LARGE    *pWorkingSourceDescriptor;      // descriptor under test
    ADI_DMA_DESCRIPTOR_LARGE    *pWorkingDestinationDescriptor; // descriptor under test
    u32                         SourceElements;                 // source element count
    u32                         DestinationElements;            // destination element count

    // set working source descriptor to first source descriptor
    pWorkingSourceDescriptor = pSourceChain;

    // set working destination descriptor to first destination descriptor
    pWorkingDestinationDescriptor = pDestinationChain;

    // WHILE (more destination descriptors)
    while (pWorkingDestinationDescriptor) {

        // total destination count = current destination count
        if (pWorkingDestinationDescriptor->Config.b_DMA2D == ADI_DMA_DMA2D_LINEAR) {
            DestinationElements = pWorkingDestinationDescriptor->XCount;
        } else {
            DestinationElements = (pWorkingDestinationDescriptor->XCount * pWorkingDestinationDescriptor->YCount);
        }

        // total source count = current source count
        if (pWorkingSourceDescriptor->Config.b_DMA2D == ADI_DMA_DMA2D_LINEAR) {
            SourceElements = pWorkingSourceDescriptor->XCount;
        } else {
            SourceElements = (pWorkingSourceDescriptor->XCount * pWorkingSourceDescriptor->YCount);
        }

        // WHILE (total source count < total destination count)
        while (SourceElements < DestinationElements) {

            // set current source descriptor to continue
            pWorkingSourceDescriptor->Config.b_NDSIZE = ADI_DMA_NDSIZE_LARGE;
            pWorkingSourceDescriptor->Config.b_FLOW = ADI_DMA_FLOW_LARGE;

            // point to the next source descriptor
            pWorkingSourceDescriptor = pWorkingSourceDescriptor->pNext;

            // total source count += current source count
            if (pWorkingSourceDescriptor->Config.b_DMA2D == ADI_DMA_DMA2D_LINEAR) {
                SourceElements += pWorkingSourceDescriptor->XCount;
            } else {
                SourceElements += (pWorkingSourceDescriptor->XCount * pWorkingSourceDescriptor->YCount);
            }

        // ENDWHILE
        }

        // WHILE (total source count > total destination count)
        while (SourceElements > DestinationElements) {

            // IF (current destination has a stop)
            if (pWorkingDestinationDescriptor->Config.b_DI_EN == ADI_DMA_DI_EN_ENABLE) {

                // flag error
                return (ADI_DMA_RESULT_INCOMPATIBLE_CALLBACK);

            // ENDIF
            }

            // point to the next destination descriptor
            pWorkingDestinationDescriptor = pWorkingDestinationDescriptor->pNext;

            // total destination count += current destination count
            if (pWorkingDestinationDescriptor->Config.b_DMA2D == ADI_DMA_DMA2D_LINEAR) {
                DestinationElements += pWorkingDestinationDescriptor->XCount;
            } else {
                DestinationElements += (pWorkingDestinationDescriptor->XCount * pWorkingDestinationDescriptor->YCount);
            }

        // ENDWHILE
        }

        // IF (total source count == total destination count)
        if (SourceElements == DestinationElements) {

            // set current source descriptor to the current destination descriptor value
            pWorkingSourceDescriptor->Config.b_NDSIZE = pWorkingDestinationDescriptor->Config.b_NDSIZE;
            pWorkingSourceDescriptor->Config.b_FLOW = pWorkingDestinationDescriptor->Config.b_FLOW;

            // point to the next source descriptor
            pWorkingSourceDescriptor = pWorkingSourceDescriptor->pNext;

            // point to the next destination descriptor
            pWorkingDestinationDescriptor = pWorkingDestinationDescriptor->pNext;

        // ENDIF
        }

    // ENDWHILE
    }

    // return
    return (ADI_DMA_RESULT_SUCCESS);
}





/*********************************************************************

    Function:       MemoryDMACallback

    Description:    Called when the simple memory DMA transfers complete

*********************************************************************/

void static MemoryDMACallback(void *ClientHandle, u32 Event, void *pArg) {

    ADI_DMA_MEMORY_COPY_DATA            *pMemoryCopyData;       // pointer to the memory copy data

    // point to the info for this transfer
    pMemoryCopyData = (ADI_DMA_MEMORY_COPY_DATA *)ClientHandle;

    // CASEOF (event)
    switch(Event) {

        // CASE (transfer complete)
        case ADI_DMA_EVENT_DESCRIPTOR_PROCESSED:

            // clear the transfer in progress flag
            pMemoryCopyData->TransferInProgressFlag = FALSE;

            // callback the client if necessary (don't need to worry about deferred because we'd already be deferred)
            // parameters are (ClientHandle, Event, Destination Start Address)
            if (pMemoryCopyData->ClientCallback) {
                (pMemoryCopyData->ClientCallback)(pMemoryCopyData->ClientHandle, Event, pMemoryCopyData->DestinationDescriptor.StartAddress);
            }
            break;

    // ENDCASE
    }

    // return
}



#if defined(ADI_SSL_DEBUG)      // *********** debug routines

/*********************************************************************

    Function:       ValidateManagerHandle

    Description:    Attempts to verify the manager handle is valid

*********************************************************************/

static ADI_DMA_RESULT ValidateManagerHandle(ADI_DMA_MANAGER_HANDLE Handle) {

    ADI_DMA_MANAGER *pManager;

    // avoid casts
    pManager = (ADI_DMA_MANAGER *)Handle;

    // at least make sure it's not NULL
    if (pManager == NULL) return(ADI_DMA_RESULT_BAD_HANDLE);

    // return
    return(ADI_DMA_RESULT_SUCCESS);
}


/*********************************************************************

    Function:       ValidateChannelID

    Description:    Attempts to verify the channel ID

*********************************************************************/

static ADI_DMA_RESULT ValidateChannelID(ADI_DMA_CHANNEL_ID ChannelID) {

    // check it
    if (ChannelID < ADI_DMA_CHANNEL_COUNT) return(ADI_DMA_RESULT_SUCCESS);

    // return
    return(ADI_DMA_RESULT_BAD_CHANNEL_ID);
}


/*********************************************************************

    Function:       ValidateIsChannelOpen

    Description:    Determines if a DMA channel is opened

*********************************************************************/

static u32 ValidateIsChannelOpen(ADI_DMA_MANAGER_HANDLE ManagerHandle,      // DMA manager handle
                                 ADI_DMA_CHANNEL_ID ChannelID) {

    ADI_DMA_CHANNEL *pChannel;      // pointer to a DMA channel
    ADI_DMA_MANAGER *pManager;      // pointer to the DMA manager data
    int             i;              // counter

    // avoid casts
    pManager = (ADI_DMA_MANAGER *)ManagerHandle;

    // FOR (each potential channel)
    for (i = pManager->ChannelCount, pChannel = pManager->Channel; i; i--, pChannel++) {

        // IF (the channel is open)
        if (pChannel->InUseFlag == TRUE) {

            // IF (the channel is the same ChannelID as the given one)
            if (pChannel->ChannelID == ChannelID) {

                // return TRUE
                return (TRUE);

            // ENDIF
            }

        // ENDIF
        }

    // ENDFOR
    }

    // return FALSE
    return (FALSE);
}


/*********************************************************************

    Function:       ValidateChannelHandle

    Description:    Attempts to verify the channel handle is valid

*********************************************************************/

static ADI_DMA_RESULT ValidateChannelHandle(ADI_DMA_CHANNEL_HANDLE Handle) {

    ADI_DMA_CHANNEL *pChannel;

    // avoid casts
    pChannel = (ADI_DMA_CHANNEL *)Handle;

    // make sure the channel handle is not NULL
    if (pChannel == NULL) return(ADI_DMA_RESULT_BAD_HANDLE);

    // return by verifying the manager address
    return(ValidateManagerHandle(pChannel->pManager));
}


/*********************************************************************

    Function:       ValidateStreamID

    Description:    Attempts to verify the memory stream ID

*********************************************************************/

static ADI_DMA_RESULT ValidateStreamID(ADI_DMA_STREAM_ID StreamID) {

    // check it
    if (StreamID < ADI_DMA_MEMORY_STREAM_COUNT) return(ADI_DMA_RESULT_SUCCESS);

    // return
    return(ADI_DMA_RESULT_BAD_MEMORY_STREAM_ID);
}


/*********************************************************************

    Function:       ValidateStreamHandle

    Description:    Attempts to verify the memory stream handle is valid

*********************************************************************/

static ADI_DMA_RESULT ValidateStreamHandle(ADI_DMA_STREAM_HANDLE Handle) {

    ADI_DMA_CHANNEL *pChannel;

    // avoid casts (stream handle is really a channel handle)
    pChannel = (ADI_DMA_CHANNEL *)Handle;

    // return by validating the channel handle
    return(ValidateChannelHandle(pChannel));
}


/*********************************************************************

    Function:       ValidatePMAP

    Description:    Attempts to verify the pmap value

*********************************************************************/

static ADI_DMA_RESULT ValidatePMAP(ADI_DMA_PMAP pmap) {

    u32 ControllerID;

    // verify the controller ID is 0 or 1
    ControllerID = ADI_DMA_GET_CONTROLLER_ID(pmap);
    if (ControllerID > 1) {
        return(ADI_DMA_RESULT_BAD_PERIPHERAL);
    }

    // return
    return(ADI_DMA_RESULT_SUCCESS);
}


/*********************************************************************

    Function:       ValidateTransfer

    Description:    This function attempts to verify the parameters for
                    a DMA transfer are compatible and will not generate
                    any errors from the controller.  These tests come
                    from the list of potential DMA errors in the hardware
                    reference manual.

*********************************************************************/

static ADI_DMA_RESULT ValidateTransfer(ADI_DMA_CONFIG_REG Config, void *StartAddress, u16 XCount, s16 XModify, u16 YCount, s16 YModify) {

    u32 Elements;   // number of elements to transfer

    // compute how many elements are getting transferred
    if (Config.b_DMA2D == ADI_DMA_DMA2D_LINEAR) {
        Elements = XCount;
    } else {
        Elements = XCount * YCount;
    }

    // insure WDSIZE, alignment and transfer byte count are correct
    switch (Config.b_WDSIZE) {
        case ADI_DMA_WDSIZE_8BIT:
            break;
        case ADI_DMA_WDSIZE_16BIT:
            if ((u32)StartAddress & 0x1) return (ADI_DMA_RESULT_ALIGNMENT_ERROR);
            if (XModify & 0x1) return (ADI_DMA_RESULT_ALIGNMENT_ERROR);
            if ((Config.b_DMA2D == ADI_DMA_DMA2D_2D) && (YModify & 0x1)) return (ADI_DMA_RESULT_ALIGNMENT_ERROR);
            break;
        case ADI_DMA_WDSIZE_32BIT:
            if ((u32)StartAddress & 0x3) return (ADI_DMA_RESULT_ALIGNMENT_ERROR);
            if (XModify & 0x3) return (ADI_DMA_RESULT_ALIGNMENT_ERROR);
            if ((Config.b_DMA2D == ADI_DMA_DMA2D_2D) && (YModify & 0x3)) return (ADI_DMA_RESULT_ALIGNMENT_ERROR);
            break;
        default:
            return (ADI_DMA_RESULT_ALIGNMENT_ERROR);
    }

    // insure XCOUNT is not equal to 1 if doing 2D DMA
    if (Config.b_DMA2D == ADI_DMA_DMA2D_2D) {
        if (XCount == 1) return (ADI_DMA_RESULT_BAD_XCOUNT);
    }

    // as far as we can tell, everything looks good
    return (ADI_DMA_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       ValidateStreamDescriptors

    Description:    This function attempts to verify that the given
                    memory DMA descriptor chains are valid.

                    The checks are as follows:
                        o have at least one source and destination descriptor
                        o transfer element widths are identical for source and destination
                        o number of transfer elements are identical for the chains passed in
                        o wnr bit is set properly
                        o only allow callbacks on the destination channel

*********************************************************************/

static ADI_DMA_RESULT ValidateStreamDescriptors(ADI_DMA_DESCRIPTOR_LARGE *pSource, ADI_DMA_DESCRIPTOR_LARGE *pDestination) {

    ADI_DMA_DESCRIPTOR_LARGE    *pWorkingSourceDescriptor;      // descriptor under test
    ADI_DMA_DESCRIPTOR_LARGE    *pWorkingDestinationDescriptor; // descriptor under test
    u32                         TransferSize;                   // transfer size
    u32                         SourceElements;                 // source element count
    u32                         DestinationElements;            // destination element count


    // insure we have at least a single source and destination descriptor
    if ((pSource == NULL) || (pDestination == NULL)) {
        return (ADI_DMA_RESULT_NULL_DESCRIPTOR);
    }

    // set working source descriptor to first source descriptor
    pWorkingSourceDescriptor = pSource;

    // set working destination descriptor to first destination descriptor
    pWorkingDestinationDescriptor = pDestination;

    // WHILE (working destination descriptor is != NULL)
    while (pWorkingDestinationDescriptor != NULL) {

        // set current width to working destination descriptor value
        TransferSize = pWorkingDestinationDescriptor->Config.b_WDSIZE;

        // set total destination element count to working destination descriptor value
        if (pWorkingDestinationDescriptor->Config.b_DMA2D == ADI_DMA_DMA2D_LINEAR) {
            DestinationElements = pWorkingDestinationDescriptor->XCount;
        } else {
            DestinationElements = (pWorkingDestinationDescriptor->XCount * pWorkingDestinationDescriptor->YCount);
        }

        // set working destination descriptor to the next destination descriptor
        pWorkingDestinationDescriptor = pWorkingDestinationDescriptor->pNext;

        // set total source element count to zero
        SourceElements = 0;

        // WHILE (destination element count != source element count)
        while (DestinationElements != SourceElements) {

            // WHILE (destination element count < source element count)
            while (DestinationElements < SourceElements) {

                // verify we have another destination descriptor
                if (pWorkingDestinationDescriptor == NULL) {
                    return(ADI_DMA_RESULT_INCOMPATIBLE_TRANSFER_SIZE);
                }

                // verify the transfer size is consistent
                if (TransferSize != pWorkingDestinationDescriptor->Config.b_WDSIZE) {
                    return (ADI_DMA_RESULT_INCOMPATIBLE_WDSIZE);
                }

                // sum working destination element count into total destination element count
                if (pWorkingDestinationDescriptor->Config.b_DMA2D == ADI_DMA_DMA2D_LINEAR) {
                    DestinationElements += pWorkingDestinationDescriptor->XCount;
                } else {
                    DestinationElements += (pWorkingDestinationDescriptor->XCount * pWorkingDestinationDescriptor->YCount);
                }

                // set working destination descriptor to the next destination descriptor
                pWorkingDestinationDescriptor = pWorkingDestinationDescriptor->pNext;

            // ENDWHILE
            }

            // WHILE (destination element count > source element count)
            while (DestinationElements > SourceElements) {

                // verify we have another source descriptor
                if (pWorkingSourceDescriptor == NULL) {
                    return(ADI_DMA_RESULT_INCOMPATIBLE_TRANSFER_SIZE);
                }

                // verify the transfer size is consistent
                if (TransferSize != pWorkingSourceDescriptor->Config.b_WDSIZE) {
                    return (ADI_DMA_RESULT_INCOMPATIBLE_WDSIZE);
                }

                // sum working source element count into total source element count
                if (pWorkingSourceDescriptor->Config.b_DMA2D == ADI_DMA_DMA2D_LINEAR) {
                    SourceElements += pWorkingSourceDescriptor->XCount;
                } else {
                    SourceElements += (pWorkingSourceDescriptor->XCount * pWorkingSourceDescriptor->YCount);
                }

                // set working source descriptor to the next source descriptor
                pWorkingSourceDescriptor = pWorkingSourceDescriptor->pNext;

            // ENDWHILE
            }

        // ENDWHILE
        }

    // ENDWHILE
    }

    // return
    return (ADI_DMA_RESULT_SUCCESS);
}


/*********************************************************************

    Function:       ValidateDescriptorChain

    Description:    This function attempts to verify that the given
                    descriptor chain is valid.

                    The checks are as follows:
                        o validates the descriptor handle is not NULL
                        o validates the chain is NULL terminated
                        o validates the transfer of each descriptor is valid
                          by calling the ValidateTransfer function
                        o For memory DMA channels
                            - insures the wnr bit is set appropriately
                            - insures no callbacks are set for descriptors
                              on source channels

*********************************************************************/

static ADI_DMA_RESULT ValidateDescriptorChain(
    ADI_DMA_CHANNEL_HANDLE      ChannelHandle,
    ADI_DMA_DESCRIPTOR_HANDLE   DescriptorHandle
){


    ADI_DMA_CHANNEL *pChannel;  // pointer to the channel we're working on
    ADI_DMA_RESULT  Result;     // result

    // avoid casts
    pChannel = (ADI_DMA_CHANNEL *)ChannelHandle;

    // validate the descriptor handle is non-NULL
    if (DescriptorHandle == NULL) {
         return(ADI_DMA_RESULT_BAD_DESCRIPTOR);
    }

    // CASEOF (channel mode)
    switch(pChannel->Mode) {

        // CASE (large descriptors)
        case (ADI_DMA_MODE_DESCRIPTOR_LARGE):
        {

            ADI_DMA_DESCRIPTOR_LARGE    *pWorkingDescriptor;    // pointer to the descriptor we're working on
            ADI_DMA_DESCRIPTOR_LARGE    *pLastDescriptor;       // pointer to the last descriptor in the list passed in

            // FOR (each descriptor in the chain)
            for (pWorkingDescriptor = &DescriptorHandle->Large; pWorkingDescriptor != NULL; pWorkingDescriptor = pWorkingDescriptor->pNext) {

                // validate the transfer itself
                if ((Result = ValidateTransfer(pWorkingDescriptor->Config, pWorkingDescriptor->StartAddress, pWorkingDescriptor->XCount, pWorkingDescriptor->XModify, pWorkingDescriptor->YCount, pWorkingDescriptor->YModify)) != ADI_DMA_RESULT_SUCCESS) {
                    return (Result);
                }

                // validate the chain is NULL terminated
                if (pWorkingDescriptor->pNext == &DescriptorHandle->Large) {
                    return (ADI_DMA_RESULT_NON_TERMINATED_CHAIN);
                }

                // CASEOF (channel type)
                switch (pChannel->ChannelType) {

                    // CASE (memory DMA source)
                    case ADI_DMA_TYPE_MEMORY_SOURCE:

                        // validate the WNR bit
                        if (pWorkingDescriptor->Config.b_WNR != ADI_DMA_WNR_READ) {
                            return (ADI_DMA_RESULT_BAD_DIRECTION);
                        }

                        // validate that no callbacks are to be generated
                        if (pWorkingDescriptor->CallbackFlag) {
                            return (ADI_DMA_RESULT_CALLBACKS_DISALLOWED_ON_SOURCE);
                        }
                        break;

                    // CASE (memory DMA destination)
                    case ADI_DMA_TYPE_MEMORY_DESTINATION:

                        // validate the WNR bit
                        if (pWorkingDescriptor->Config.b_WNR != ADI_DMA_WNR_WRITE) {
                            return (ADI_DMA_RESULT_BAD_DIRECTION);
                        }
                        break;

                // ENDCASE
                }

            // ENDFOR
            }

        // ENDBLOCK
        }
        break;

        // CASE (small descriptors)
        case (ADI_DMA_MODE_DESCRIPTOR_SMALL):
        {

            ADI_DMA_DESCRIPTOR_SMALL    *pWorkingDescriptor;    // pointer to the descriptor we're working on
            ADI_DMA_DESCRIPTOR_SMALL    *pLastDescriptor;       // pointer to the last descriptor in the list passed in
            u32                         UpperAddress;           // upper 16 bits of address

            // in the small model, pNext is only a 16 bit address so when we walk the list
            // of descriptors, we need to insure we use the upper 16 bits of the first
            // descriptor that is passed in so that we properly walk the list
            UpperAddress = (u32)(&DescriptorHandle->Small) & 0xffff0000;

            // point to the first descriptor in the chain
            pWorkingDescriptor = &DescriptorHandle->Small;

            // WHILE (there are more descriptors in the chain)
            while (((u32)pWorkingDescriptor & 0xffff) != 0) {

                // validate the transfer itself
                if ((Result = ValidateTransfer(pWorkingDescriptor->Config, (void *)(((u32)pWorkingDescriptor->StartAddressHigh << 16) & ((u32)pWorkingDescriptor->StartAddressLow)), pWorkingDescriptor->XCount, pWorkingDescriptor->XModify, pWorkingDescriptor->YCount, pWorkingDescriptor->YModify)) != ADI_DMA_RESULT_SUCCESS) {
                    return (Result);
                }

                // validate the chain is NULL terminated
                if (pWorkingDescriptor->pNext == (u16)((u32)(&DescriptorHandle->Small) & 0xffff)) {
                    return (ADI_DMA_RESULT_NON_TERMINATED_CHAIN);
                }

                // CASEOF (channel type)
                switch (pChannel->ChannelType) {

                    // CASE (memory DMA source)
                    case ADI_DMA_TYPE_MEMORY_SOURCE:

                        // validate the WNR bit
                        if (pWorkingDescriptor->Config.b_WNR != ADI_DMA_WNR_READ) {
                            return (ADI_DMA_RESULT_BAD_DIRECTION);
                        }

                        // validate that no callbacks are to be generated
                        if (pWorkingDescriptor->CallbackFlag) {
                            return (ADI_DMA_RESULT_CALLBACKS_DISALLOWED_ON_SOURCE);
                        }
                        break;

                    // CASE (memory DMA destination)
                    case ADI_DMA_TYPE_MEMORY_DESTINATION:

                        // validate the WNR bit
                        if (pWorkingDescriptor->Config.b_WNR != ADI_DMA_WNR_WRITE) {
                            return (ADI_DMA_RESULT_BAD_DIRECTION);
                        }
                        break;

                // ENDCASE
                }

                // get the address of next descriptor in the chain and remember to put in
                // the upper 16 bits
                pWorkingDescriptor = (ADI_DMA_DESCRIPTOR_SMALL *)((u32)(pWorkingDescriptor->pNext) | UpperAddress);

            // ENDWHILE
            }

        // ENDBLOCK
        }
        break;

        // CASE (arrays)
        case (ADI_DMA_MODE_DESCRIPTOR_ARRAY):

            // don't support this
            return (ADI_DMA_RESULT_BAD_MODE);

    // ENDCASE
    }

    // return
    return (ADI_DMA_RESULT_SUCCESS);
}



#endif



