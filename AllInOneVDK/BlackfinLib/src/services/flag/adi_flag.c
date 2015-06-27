/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_flag.c,v $
$Revision: 2386 $
$Date: 2010-03-25 17:26:08 -0400 (Thu, 25 Mar 2010) $

Description:
            Programmable flag services for the System Services Library

*********************************************************************************/



/*********************************************************************

Include files

*********************************************************************/
#include <services\services.h>      // system service includes



/*********************************************************************

Silicon anomaly macros

*********************************************************************/

#if defined(__ADSP_EDINBURGH__) || defined(__ADSP_TETON__)

#define ADI_FLAG_SILICON_ANOMALY_05000311   *pUART_SCR

#endif


/*********************************************************************

Data structures for the GPIO and GPIO interrupt registers on Moab

Note: the other Blackfins use a different approach to this

*********************************************************************/

typedef struct {            // replicates the GPIO registers in memory
    volatile u16 reg_FER;       // FER register
    u16 Padding0;               // padding
    volatile u16 reg_DATA;      // data register (r/w)
    u16 Padding1;               // padding
    volatile u16 reg_SET;       // set register (W1S)
    u16 Padding2;               // padding
    volatile u16 reg_CLEAR;     // clear register (W1C)
    u16 Padding3;               // padding
    volatile u16 reg_DIR_SET;   // direction set register (configures for output) (W1S)
    u16 Padding4;               // padding
    volatile u16 reg_DIR_CLEAR; // direction clear register (configures for input) (W1C)
    u16 Padding5;               // padding
    volatile u16 reg_INEN;      // input enable register (r/w)
    u16 Padding6;               // padding
    volatile u32 reg_MUX;       // mux register (r/w)
} ADI_FLAG_GPIO_REGISTERS;


typedef struct {                // replicates the GPIO interrupt registers in memory
    volatile u32 reg_MASK_SET;      // set mask register (W1S)
    volatile u32 reg_MASK_CLEAR;    // clear mask register (W1C)
    volatile u32 reg_IRQ;           // interrupt request register (W1C)
    volatile u32 reg_ASSIGN;        // interrupt assignment register (r/w)
    volatile u32 reg_EDGE_SET;      // set edge interrupts (W1S)
    volatile u32 reg_EDGE_CLEAR;    // clear edge interrupts (make level) (W1C)
    volatile u32 reg_INVERT_SET;    // set inverted polarity (W1S)
    volatile u32 reg_INVERT_CLEAR;  // clear inverted polarity (W1C)
    volatile u32 reg_PINSTATE;      // pin state register (RO)
    volatile u32 reg_LATCH;         // interrupt latch register (r/w)
} ADI_FLAG_INT_REGISTERS;



/*********************************************************************

Data structure to define the MMRs for the flags.  Note that this is more
complex than originally designed.  This is due to inconsistencies in
various processor implementations and changes to the underlying programming
model in the flags.

Note that Moab flags are different than other Blackfins.  As such a slightly
different data structure is used to control flags on this family.

*********************************************************************/

#if defined(__ADSP_EDINBURGH__)   \
 || defined(__ADSP_BRAEMAR__)     \
 || defined(__ADSP_TETON__)       \
 || defined(__ADSP_STIRLING__)    \
 || defined(__ADSP_KOOKABURRA__)  \
 || defined(__ADSP_MOCKINGBIRD__) \
 || defined(__ADSP_BRODIE__)      \
 || defined(__ADSP_MOY__)         \
 || defined(__ADSP_DELTA__)       

typedef struct ADI_FLAG_PORT_INFO { // information for a port that contains flags
    volatile u16 *Data;                 // flag data register
    volatile u16 *Clear;                // clear flag register (W1C)
    volatile u16 *Set;                  // set flag register (W1C)
    volatile u16 *Toggle;               // toggle flag register (W1C)
    volatile u16 *MaskAData;            // interrupt A mask data register
    volatile u16 *MaskAClear;           // interrupt A mask clear register (W1C)
    volatile u16 *MaskASet;             // interrupt A mask set register (W1C)
    volatile u16 *MaskBData;            // interrupt B mask data register
    volatile u16 *MaskBClear;           // interrupt B mask clear register (W1C)
    volatile u16 *MaskBSet;             // interrupt B mask set register (W1C)
    volatile u16 *Direction;            // flag direction register
    volatile u16 *Polarity;             // flag polarity register
    volatile u16 *Edge;                 // flag edge sensitive register
    volatile u16 *Both;                 // flag both edge register
    volatile u16 *InputEnable;          // flag input enable register
} ADI_FLAG_PORT_INFO;

#endif


#if defined(__ADSP_MOAB__)

typedef struct ADI_FLAG_PORT_INFO {     // information for a port that contains flags
    ADI_FLAG_GPIO_REGISTERS *pBaseAddress;  // base address for the port's GPIO registers
} ADI_FLAG_PORT_INFO;

typedef struct ADI_FLAG_INT_INFO {      // information for the flag interrupt registers
    ADI_FLAG_INT_REGISTERS *pBaseAddress;   // base address for the port's GPIO registers
    ADI_INT_PERIPHERAL_ID   PeripheralID;   // peripheral ID for this interrupt
    u8                      UsageCount[4];  // usage count for each byte in the PINTx_ASSIGN register
} ADI_FLAG_INT_INFO;

#endif

/*********************************************************************

Processor specifics

This section contains the processor specific data for flags.

*********************************************************************/

/********************
    Edinburg
********************/

#if defined(__ADSP_EDINBURGH__)     // processor specifics for BF531, BF532, BF533

static const ADI_FLAG_PORT_INFO PortInfo[] = {   // port information
    {                                               // port f
        pFIO_FLAG_D,    // flag data register
        pFIO_FLAG_C,    // clear flag register (W1C)
        pFIO_FLAG_S,    // set flag register (W1C)
        pFIO_FLAG_T,    // toggle flag register (W1C)
        pFIO_MASKA_D,   // interrupt A mask data register
        pFIO_MASKA_C,   // interrupt A mask clear register (W1C)
        pFIO_MASKA_S,   // interrupt A mask set register (W1C)
        pFIO_MASKB_D,   // interrupt B mask data register
        pFIO_MASKB_C,   // interrupt B mask clear register (W1C)
        pFIO_MASKB_S,   // interrupt B mask set register (W1C)
        pFIO_DIR,       // flag direction register
        pFIO_POLAR,     // flag polarity register
        pFIO_EDGE,      // flag edge sensitive register
        pFIO_BOTH,      // flag both edge register
        pFIO_INEN       // flag input enable register
    }
};

#endif

/********************
    Braemar
********************/

#if defined(__ADSP_BRAEMAR__)   // processor specifics for BF534, BF536, BF537

static const ADI_FLAG_PORT_INFO PortInfo[] = {   // port information
    {                                               // port f
        pPORTFIO,               // flag data register
        pPORTFIO_CLEAR,         // clear flag register (W1C)
        pPORTFIO_SET,           // set flag register (W1C)
        pPORTFIO_TOGGLE,        // toggle flag register (W1C)
        pPORTFIO_MASKA,         // interrupt A mask data register
        pPORTFIO_MASKA_CLEAR,   // interrupt A mask clear register (W1C)
        pPORTFIO_MASKA_SET,     // interrupt A mask set register (W1C)
        pPORTFIO_MASKB,         // interrupt B mask data register
        pPORTFIO_MASKB_CLEAR,   // interrupt B mask clear register (W1C)
        pPORTFIO_MASKB_SET,     // interrupt B mask set register (W1C)
        pPORTFIO_DIR,           // flag direction register
        pPORTFIO_POLAR,         // flag polarity register
        pPORTFIO_EDGE,          // flag edge sensitive register
        pPORTFIO_BOTH,          // flag both edge register
        pPORTFIO_INEN           // flag input enable register
    },
    {                                               // port g
        pPORTGIO,               // flag data register
        pPORTGIO_CLEAR,         // clear flag register (W1C)
        pPORTGIO_SET,           // set flag register (W1C)
        pPORTGIO_TOGGLE,        // toggle flag register (W1C)
        pPORTGIO_MASKA,         // interrupt A mask data register
        pPORTGIO_MASKA_CLEAR,   // interrupt A mask clear register (W1C)
        pPORTGIO_MASKA_SET,     // interrupt A mask set register (W1C)
        pPORTGIO_MASKB,         // interrupt B mask data register
        pPORTGIO_MASKB_CLEAR,   // interrupt B mask clear register (W1C)
        pPORTGIO_MASKB_SET,     // interrupt B mask set register (W1C)
        pPORTGIO_DIR,           // flag direction register
        pPORTGIO_POLAR,         // flag polarity register
        pPORTGIO_EDGE,          // flag edge sensitive register
        pPORTGIO_BOTH,          // flag both edge register
        pPORTGIO_INEN           // flag input enable register
    },
    {                                               // port h
        pPORTHIO,               // flag data register
        pPORTHIO_CLEAR,         // clear flag register (W1C)
        pPORTHIO_SET,           // set flag register (W1C)
        pPORTHIO_TOGGLE,        // toggle flag register (W1C)
        pPORTHIO_MASKA,         // interrupt A mask data register
        pPORTHIO_MASKA_CLEAR,   // interrupt A mask clear register (W1C)
        pPORTHIO_MASKA_SET,     // interrupt A mask set register (W1C)
        pPORTHIO_MASKB,         // interrupt B mask data register
        pPORTHIO_MASKB_CLEAR,   // interrupt B mask clear register (W1C)
        pPORTHIO_MASKB_SET,     // interrupt B mask set register (W1C)
        pPORTHIO_DIR,           // flag direction register
        pPORTHIO_POLAR,         // flag polarity register
        pPORTHIO_EDGE,          // flag edge sensitive register
        pPORTHIO_BOTH,          // flag both edge register
        pPORTHIO_INEN           // flag input enable register
    }
};

#endif


/********************
    Teton
********************/

#if defined(__ADSP_TETON__)         // processor specifics for BF561

static const ADI_FLAG_PORT_INFO PortInfo[] = {   // port information
    {                                               // port f
        pFIO0_FLAG_D,   // flag data register
        pFIO0_FLAG_C,   // clear flag register (W1C)
        pFIO0_FLAG_S,   // set flag register (W1C)
        pFIO0_FLAG_T,   // toggle flag register (W1C)
        pFIO0_MASKA_D,  // interrupt A mask data register
        pFIO0_MASKA_C,  // interrupt A mask clear register (W1C)
        pFIO0_MASKA_S,  // interrupt A mask set register (W1C)
        pFIO0_MASKB_D,  // interrupt B mask data register
        pFIO0_MASKB_C,  // interrupt B mask clear register (W1C)
        pFIO0_MASKB_S,  // interrupt B mask set register (W1C)
        pFIO0_DIR,      // flag direction register
        pFIO0_POLAR,    // flag polarity register
        pFIO0_EDGE,     // flag edge sensitive register
        pFIO0_BOTH,     // flag both edge register
        pFIO0_INEN      // flag input enable register
    },
    {                                               // port g
        pFIO1_FLAG_D,   // flag data register
        pFIO1_FLAG_C,   // clear flag register (W1C)
        pFIO1_FLAG_S,   // set flag register (W1C)
        pFIO1_FLAG_T,   // toggle flag register (W1C)
        pFIO1_MASKA_D,  // interrupt A mask data register
        pFIO1_MASKA_C,  // interrupt A mask clear register (W1C)
        pFIO1_MASKA_S,  // interrupt A mask set register (W1C)
        pFIO1_MASKB_D,  // interrupt B mask data register
        pFIO1_MASKB_C,  // interrupt B mask clear register (W1C)
        pFIO1_MASKB_S,  // interrupt B mask set register (W1C)
        pFIO1_DIR,      // flag direction register
        pFIO1_POLAR,    // flag polarity register
        pFIO1_EDGE,     // flag edge sensitive register
        pFIO1_BOTH,     // flag both edge register
        pFIO1_INEN      // flag input enable register
    },
    {                                               // port h
        pFIO2_FLAG_D,   // flag data register
        pFIO2_FLAG_C,   // clear flag register (W1C)
        pFIO2_FLAG_S,   // set flag register (W1C)
        pFIO2_FLAG_T,   // toggle flag register (W1C)
        pFIO2_MASKA_D,  // interrupt A mask data register
        pFIO2_MASKA_C,  // interrupt A mask clear register (W1C)
        pFIO2_MASKA_S,  // interrupt A mask set register (W1C)
        pFIO2_MASKB_D,  // interrupt B mask data register
        pFIO2_MASKB_C,  // interrupt B mask clear register (W1C)
        pFIO2_MASKB_S,  // interrupt B mask set register (W1C)
        pFIO2_DIR,      // flag direction register
        pFIO2_POLAR,    // flag polarity register
        pFIO2_EDGE,     // flag edge sensitive register
        pFIO2_BOTH,     // flag both edge register
        pFIO2_INEN      // flag input enable register
    }
};

#endif


/********************
    Stirling
********************/

#if defined(__ADSP_STIRLING__)      // processor specifics for BF538 and BF539

static const ADI_FLAG_PORT_INFO PortInfo[] = {   // port information
    {                                               // port c
        pPORTCIO,               // flag data register
        pPORTCIO_CLEAR,         // clear flag register (W1C)
        pPORTCIO_SET,           // set flag register (W1C)
        pPORTCIO_TOGGLE,        // toggle flag register (W1C)
        NULL,                   // interrupt A mask data register
        NULL,                   // interrupt A mask clear register (W1C)
        NULL,                   // interrupt A mask set register (W1C)
        NULL,                   // interrupt B mask data register
        NULL,                   // interrupt B mask clear register (W1C)
        NULL,                   // interrupt B mask set register (W1C)
        pPORTCIO_DIR,           // flag direction register
        NULL,                   // flag polarity register
        NULL,                   // flag edge sensitive register
        NULL,                   // flag both edge register
        pPORTCIO_INEN           // flag input enable register
    },
    {                                               // port d
        pPORTDIO,               // flag data register
        pPORTDIO_CLEAR,         // clear flag register (W1C)
        pPORTDIO_SET,           // set flag register (W1C)
        pPORTDIO_TOGGLE,        // toggle flag register (W1C)
        NULL,                   // interrupt A mask data register
        NULL,                   // interrupt A mask clear register (W1C)
        NULL,                   // interrupt A mask set register (W1C)
        NULL,                   // interrupt B mask data register
        NULL,                   // interrupt B mask clear register (W1C)
        NULL,                   // interrupt B mask set register (W1C)
        pPORTDIO_DIR,           // flag direction register
        NULL,                   // flag polarity register
        NULL,                   // flag edge sensitive register
        NULL,                   // flag both edge register
        pPORTDIO_INEN           // flag input enable register
    },
    {                                               // port e
        pPORTEIO,               // flag data register
        pPORTEIO_CLEAR,         // clear flag register (W1C)
        pPORTEIO_SET,           // set flag register (W1C)
        pPORTEIO_TOGGLE,        // toggle flag register (W1C)
        NULL,                   // interrupt A mask data register
        NULL,                   // interrupt A mask clear register (W1C)
        NULL,                   // interrupt A mask set register (W1C)
        NULL,                   // interrupt B mask data register
        NULL,                   // interrupt B mask clear register (W1C)
        NULL,                   // interrupt B mask set register (W1C)
        pPORTEIO_DIR,           // flag direction register
        NULL,                   // flag polarity register
        NULL,                   // flag edge sensitive register
        NULL,                   // flag both edge register
        pPORTEIO_INEN           // flag input enable register
    },
    {                                               // port f
        pPORTFIO,               // flag data register
        pPORTFIO_CLEAR,         // clear flag register (W1C)
        pPORTFIO_SET,           // set flag register (W1C)
        pPORTFIO_TOGGLE,        // toggle flag register (W1C)
        pPORTFIO_MASKA,         // interrupt A mask data register
        pPORTFIO_MASKA_CLEAR,   // interrupt A mask clear register (W1C)
        pPORTFIO_MASKA_SET,     // interrupt A mask set register (W1C)
        pPORTFIO_MASKB,         // interrupt B mask data register
        pPORTFIO_MASKB_CLEAR,   // interrupt B mask clear register (W1C)
        pPORTFIO_MASKB_SET,     // interrupt B mask set register (W1C)
        pPORTFIO_DIR,           // flag direction register
        pPORTFIO_POLAR,         // flag polarity register
        pPORTFIO_EDGE,          // flag edge sensitive register
        pPORTFIO_BOTH,          // flag both edge register
        pPORTFIO_INEN           // flag input enable register
    }
};

#endif


/********************
    Moab
********************/

#if defined(__ADSP_MOAB__) // processor specifics for the BF54x processors

static const ADI_FLAG_PORT_INFO PortInfo[] = {   // port information
    {                                               // port A
        (ADI_FLAG_GPIO_REGISTERS *)pPORTA_FER
    },
    {                                               // port B
        (ADI_FLAG_GPIO_REGISTERS *)pPORTB_FER
    },
    {                                               // port C
        (ADI_FLAG_GPIO_REGISTERS *)pPORTC_FER
    },
    {                                               // port D
        (ADI_FLAG_GPIO_REGISTERS *)pPORTD_FER
    },
    {                                               // port E
        (ADI_FLAG_GPIO_REGISTERS *)pPORTE_FER
    },
    {                                               // port F
        (ADI_FLAG_GPIO_REGISTERS *)pPORTF_FER
    },
    {                                               // port G
        (ADI_FLAG_GPIO_REGISTERS *)pPORTG_FER
    },
    {                                               // port H
        (ADI_FLAG_GPIO_REGISTERS *)pPORTH_FER
    },
    {                                               // port I
        (ADI_FLAG_GPIO_REGISTERS *)pPORTI_FER
    },
    {                                               // port J
        (ADI_FLAG_GPIO_REGISTERS *)pPORTJ_FER
    },
};


static ADI_FLAG_INT_INFO InterruptInfo[] = {    // interrupt information
    {                                           // interrupt 0
        (ADI_FLAG_INT_REGISTERS *) pPINT0_MASK_SET, // base register address
        ADI_INT_PINT0,                              // peripheral ID for this interrupt
        0,                                          // usage count for byte 0
        0,                                          // usage count for byte 1
        0,                                          // usage count for byte 2
        0                                           // usage count for byte 3
    },
    {                                           // interrupt 1
        (ADI_FLAG_INT_REGISTERS *) pPINT1_MASK_SET, // base register address
        ADI_INT_PINT1,                              // peripheral ID for this interrupt
        0,                                          // usage count for byte 0
        0,                                          // usage count for byte 1
        0,                                          // usage count for byte 2
        0                                           // usage count for byte 3
    },
    {                                           // interrupt 2
        (ADI_FLAG_INT_REGISTERS *) pPINT2_MASK_SET, // base register address
        ADI_INT_PINT2,                              // peripheral ID for this interrupt
        0,                                          // usage count for byte 0
        0,                                          // usage count for byte 1
        0,                                          // usage count for byte 2
        0                                           // usage count for byte 3
    },
    {                                           // interrupt 3
        (ADI_FLAG_INT_REGISTERS *) pPINT3_MASK_SET, // base register address
        ADI_INT_PINT3,                              // peripheral ID for this interrupt
        0,                                          // usage count for byte 0
        0,                                          // usage count for byte 1
        0,                                          // usage count for byte 2
        0                                           // usage count for byte 3
    }
};

#endif

/*************************************
    Kookaburra/Mockingbird/Brodie/Moy
*************************************/

/* processor specifics for the BF52x, BF51x & BF50x processors */
#if defined(__ADSP_KOOKABURRA__)  ||\
    defined(__ADSP_MOCKINGBIRD__) ||\
    defined(__ADSP_BRODIE__)      ||\
    defined(__ADSP_MOY__)

static const ADI_FLAG_PORT_INFO PortInfo[] = {   // port information
    {                                               // port f
        pPORTFIO,               // flag data register
        pPORTFIO_CLEAR,         // clear flag register (W1C)
        pPORTFIO_SET,           // set flag register (W1C)
        pPORTFIO_TOGGLE,        // toggle flag register (W1C)
        pPORTFIO_MASKA,         // interrupt A mask data register
        pPORTFIO_MASKA_CLEAR,   // interrupt A mask clear register (W1C)
        pPORTFIO_MASKA_SET,     // interrupt A mask set register (W1C)
        pPORTFIO_MASKB,         // interrupt B mask data register
        pPORTFIO_MASKB_CLEAR,   // interrupt B mask clear register (W1C)
        pPORTFIO_MASKB_SET,     // interrupt B mask set register (W1C)
        pPORTFIO_DIR,           // flag direction register
        pPORTFIO_POLAR,         // flag polarity register
        pPORTFIO_EDGE,          // flag edge sensitive register
        pPORTFIO_BOTH,          // flag both edge register
        pPORTFIO_INEN           // flag input enable register
    },
    {                                               // port g
        pPORTGIO,               // flag data register
        pPORTGIO_CLEAR,         // clear flag register (W1C)
        pPORTGIO_SET,           // set flag register (W1C)
        pPORTGIO_TOGGLE,        // toggle flag register (W1C)
        pPORTGIO_MASKA,         // interrupt A mask data register
        pPORTGIO_MASKA_CLEAR,   // interrupt A mask clear register (W1C)
        pPORTGIO_MASKA_SET,     // interrupt A mask set register (W1C)
        pPORTGIO_MASKB,         // interrupt B mask data register
        pPORTGIO_MASKB_CLEAR,   // interrupt B mask clear register (W1C)
        pPORTGIO_MASKB_SET,     // interrupt B mask set register (W1C)
        pPORTGIO_DIR,           // flag direction register
        pPORTGIO_POLAR,         // flag polarity register
        pPORTGIO_EDGE,          // flag edge sensitive register
        pPORTGIO_BOTH,          // flag both edge register
        pPORTGIO_INEN           // flag input enable register
    },
    {                                               // port h
        pPORTHIO,               // flag data register
        pPORTHIO_CLEAR,         // clear flag register (W1C)
        pPORTHIO_SET,           // set flag register (W1C)
        pPORTHIO_TOGGLE,        // toggle flag register (W1C)
        pPORTHIO_MASKA,         // interrupt A mask data register
        pPORTHIO_MASKA_CLEAR,   // interrupt A mask clear register (W1C)
        pPORTHIO_MASKA_SET,     // interrupt A mask set register (W1C)
        pPORTHIO_MASKB,         // interrupt B mask data register
        pPORTHIO_MASKB_CLEAR,   // interrupt B mask clear register (W1C)
        pPORTHIO_MASKB_SET,     // interrupt B mask set register (W1C)
        pPORTHIO_DIR,           // flag direction register
        pPORTHIO_POLAR,         // flag polarity register
        pPORTHIO_EDGE,          // flag edge sensitive register
        pPORTHIO_BOTH,          // flag both edge register
        pPORTHIO_INEN           // flag input enable register
    }
};

#endif

/******************************************
    Delta
*******************************************/

/* processor specifics for the BF59x processors */
#if defined(__ADSP_DELTA__)

static const ADI_FLAG_PORT_INFO PortInfo[] = {   // port information
    {                                               // port f
        pPORTFIO,               // flag data register
        pPORTFIO_CLEAR,         // clear flag register (W1C)
        pPORTFIO_SET,           // set flag register (W1C)
        pPORTFIO_TOGGLE,        // toggle flag register (W1C)
        pPORTFIO_MASKA,         // interrupt A mask data register
        pPORTFIO_MASKA_CLEAR,   // interrupt A mask clear register (W1C)
        pPORTFIO_MASKA_SET,     // interrupt A mask set register (W1C)
        pPORTFIO_MASKB,         // interrupt B mask data register
        pPORTFIO_MASKB_CLEAR,   // interrupt B mask clear register (W1C)
        pPORTFIO_MASKB_SET,     // interrupt B mask set register (W1C)
        pPORTFIO_DIR,           // flag direction register
        pPORTFIO_POLAR,         // flag polarity register
        pPORTFIO_EDGE,          // flag edge sensitive register
        pPORTFIO_BOTH,          // flag both edge register
        pPORTFIO_INEN           // flag input enable register
    },
    {                                               // port g
        pPORTGIO,               // flag data register
        pPORTGIO_CLEAR,         // clear flag register (W1C)
        pPORTGIO_SET,           // set flag register (W1C)
        pPORTGIO_TOGGLE,        // toggle flag register (W1C)
        pPORTGIO_MASKA,         // interrupt A mask data register
        pPORTGIO_MASKA_CLEAR,   // interrupt A mask clear register (W1C)
        pPORTGIO_MASKA_SET,     // interrupt A mask set register (W1C)
        pPORTGIO_MASKB,         // interrupt B mask data register
        pPORTGIO_MASKB_CLEAR,   // interrupt B mask clear register (W1C)
        pPORTGIO_MASKB_SET,     // interrupt B mask set register (W1C)
        pPORTGIO_DIR,           // flag direction register
        pPORTGIO_POLAR,         // flag polarity register
        pPORTGIO_EDGE,          // flag edge sensitive register
        pPORTGIO_BOTH,          // flag both edge register
        pPORTGIO_INEN           // flag input enable register
    }
};

#endif

/*********************************************************************

Mappings for processors with port control

*********************************************************************/

#if defined(__ADSP_STIRLING__)      // processor specifics for BF538 and BF539

typedef struct ADI_FLAG_PORT_MAPPING {  // maps Flag IDs to port control directives
    ADI_FLAG_ID         FlagID;     // Flag ID
    ADI_PORTS_DIRECTIVE Directive;  // port control directive
} ADI_FLAG_PORT_MAPPING;

static ADI_FLAG_PORT_MAPPING PortMappings[] = {   // port mappings
    {   ADI_FLAG_PC0,   ADI_PORTS_DIRECTIVE_PC0   },
    {   ADI_FLAG_PC1,   ADI_PORTS_DIRECTIVE_PC1   },
    /* PC 2,3 unused */
    {   ADI_FLAG_PC4,   ADI_PORTS_DIRECTIVE_PC4   },
    {   ADI_FLAG_PC5,   ADI_PORTS_DIRECTIVE_PC5   },
    {   ADI_FLAG_PC6,   ADI_PORTS_DIRECTIVE_PC6   },
    {   ADI_FLAG_PC7,   ADI_PORTS_DIRECTIVE_PC7   },
    {   ADI_FLAG_PC8,   ADI_PORTS_DIRECTIVE_PC8   },
    {   ADI_FLAG_PC9,   ADI_PORTS_DIRECTIVE_PC9   },
    /* PC 10-15 unused */
    {   ADI_FLAG_PD0,   ADI_PORTS_DIRECTIVE_PD0   },
    {   ADI_FLAG_PD1,   ADI_PORTS_DIRECTIVE_PD1   },
    {   ADI_FLAG_PD2,   ADI_PORTS_DIRECTIVE_PD2   },
    {   ADI_FLAG_PD3,   ADI_PORTS_DIRECTIVE_PD3   },
    {   ADI_FLAG_PD4,   ADI_PORTS_DIRECTIVE_PD4   },
    {   ADI_FLAG_PD5,   ADI_PORTS_DIRECTIVE_PD5   },
    {   ADI_FLAG_PD6,   ADI_PORTS_DIRECTIVE_PD6   },
    {   ADI_FLAG_PD7,   ADI_PORTS_DIRECTIVE_PD7   },
    {   ADI_FLAG_PD8,   ADI_PORTS_DIRECTIVE_PD8   },
    {   ADI_FLAG_PD9,   ADI_PORTS_DIRECTIVE_PD9   },
    {   ADI_FLAG_PD10,  ADI_PORTS_DIRECTIVE_PD10  },
    {   ADI_FLAG_PD11,  ADI_PORTS_DIRECTIVE_PD11  },
    {   ADI_FLAG_PD12,  ADI_PORTS_DIRECTIVE_PD12  },
    {   ADI_FLAG_PD13,  ADI_PORTS_DIRECTIVE_PD13  },
    /* PD 14, 15 unused */
    {   ADI_FLAG_PE0,   ADI_PORTS_DIRECTIVE_PE0   },
    {   ADI_FLAG_PE1,   ADI_PORTS_DIRECTIVE_PE1   },
    {   ADI_FLAG_PE2,   ADI_PORTS_DIRECTIVE_PE2   },
    {   ADI_FLAG_PE3,   ADI_PORTS_DIRECTIVE_PE3   },
    {   ADI_FLAG_PE4,   ADI_PORTS_DIRECTIVE_PE4   },
    {   ADI_FLAG_PE5,   ADI_PORTS_DIRECTIVE_PE5   },
    {   ADI_FLAG_PE6,   ADI_PORTS_DIRECTIVE_PE6   },
    {   ADI_FLAG_PE7,   ADI_PORTS_DIRECTIVE_PE7   },
    {   ADI_FLAG_PE8,   ADI_PORTS_DIRECTIVE_PE8   },
    {   ADI_FLAG_PE9,   ADI_PORTS_DIRECTIVE_PE9   },
    {   ADI_FLAG_PE10,  ADI_PORTS_DIRECTIVE_PE10  },
    {   ADI_FLAG_PE11,  ADI_PORTS_DIRECTIVE_PE11  },
    {   ADI_FLAG_PE12,  ADI_PORTS_DIRECTIVE_PE12  },
    {   ADI_FLAG_PE13,  ADI_PORTS_DIRECTIVE_PE13  },
    {   ADI_FLAG_PE14,  ADI_PORTS_DIRECTIVE_PE14  },
    {   ADI_FLAG_PE15,  ADI_PORTS_DIRECTIVE_PE15  }
};


#endif

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)   // processor specifics for BF52x

typedef struct ADI_FLAG_PORT_MAPPING {  // maps Flag IDs to port control directives
    ADI_FLAG_ID         FlagID;     // Flag ID
    ADI_PORTS_DIRECTIVE Directive;  // port control directive
} ADI_FLAG_PORT_MAPPING;

static ADI_FLAG_PORT_MAPPING PortMappings[] = {   // port mappings
    {   ADI_FLAG_PF0,   ADI_PORTS_DIRECTIVE_PF0   },
    {   ADI_FLAG_PF1,   ADI_PORTS_DIRECTIVE_PF1   },
    {   ADI_FLAG_PF2,   ADI_PORTS_DIRECTIVE_PF2   },
    {   ADI_FLAG_PF3,   ADI_PORTS_DIRECTIVE_PF3   },
    {   ADI_FLAG_PF4,   ADI_PORTS_DIRECTIVE_PF4   },
    {   ADI_FLAG_PF5,   ADI_PORTS_DIRECTIVE_PF5   },
    {   ADI_FLAG_PF6,   ADI_PORTS_DIRECTIVE_PF6   },
    {   ADI_FLAG_PF7,   ADI_PORTS_DIRECTIVE_PF7   },
    {   ADI_FLAG_PF8,   ADI_PORTS_DIRECTIVE_PF8   },
    {   ADI_FLAG_PF9,   ADI_PORTS_DIRECTIVE_PF9   },
    {   ADI_FLAG_PF10,  ADI_PORTS_DIRECTIVE_PF10  },
    {   ADI_FLAG_PF11,  ADI_PORTS_DIRECTIVE_PF11  },
    {   ADI_FLAG_PF12,  ADI_PORTS_DIRECTIVE_PF12  },
    {   ADI_FLAG_PF13,  ADI_PORTS_DIRECTIVE_PF13  },
    {   ADI_FLAG_PF14,  ADI_PORTS_DIRECTIVE_PF14  },
    {   ADI_FLAG_PF15,  ADI_PORTS_DIRECTIVE_PF15  },
    // hardwired - unconfigurable {   ADI_FLAG_PG0,   ADI_PORTS_DIRECTIVE_PG0   },
    {   ADI_FLAG_PG1,   ADI_PORTS_DIRECTIVE_PG1   },
    {   ADI_FLAG_PG2,   ADI_PORTS_DIRECTIVE_PG2   },
    {   ADI_FLAG_PG3,   ADI_PORTS_DIRECTIVE_PG3   },
    {   ADI_FLAG_PG4,   ADI_PORTS_DIRECTIVE_PG4   },
    {   ADI_FLAG_PG5,   ADI_PORTS_DIRECTIVE_PG5   },
    {   ADI_FLAG_PG6,   ADI_PORTS_DIRECTIVE_PG6   },
    {   ADI_FLAG_PG7,   ADI_PORTS_DIRECTIVE_PG7   },
    {   ADI_FLAG_PG8,   ADI_PORTS_DIRECTIVE_PG8   },
    {   ADI_FLAG_PG9,   ADI_PORTS_DIRECTIVE_PG9   },
    {   ADI_FLAG_PG10,  ADI_PORTS_DIRECTIVE_PG10  },
    {   ADI_FLAG_PG11,  ADI_PORTS_DIRECTIVE_PG11  },
    {   ADI_FLAG_PG12,  ADI_PORTS_DIRECTIVE_PG12  },
    {   ADI_FLAG_PG13,  ADI_PORTS_DIRECTIVE_PG13  },
    {   ADI_FLAG_PG14,  ADI_PORTS_DIRECTIVE_PG14  },
    {   ADI_FLAG_PG15,  ADI_PORTS_DIRECTIVE_PG15  },
    {   ADI_FLAG_PH0,   ADI_PORTS_DIRECTIVE_PH0   },
    {   ADI_FLAG_PH1,   ADI_PORTS_DIRECTIVE_PH1   },
    {   ADI_FLAG_PH2,   ADI_PORTS_DIRECTIVE_PH2   },
    {   ADI_FLAG_PH3,   ADI_PORTS_DIRECTIVE_PH3   },
    {   ADI_FLAG_PH4,   ADI_PORTS_DIRECTIVE_PH4   },
    {   ADI_FLAG_PH5,   ADI_PORTS_DIRECTIVE_PH5   },
    {   ADI_FLAG_PH6,   ADI_PORTS_DIRECTIVE_PH6   },
    {   ADI_FLAG_PH7,   ADI_PORTS_DIRECTIVE_PH7   },
    {   ADI_FLAG_PH8,   ADI_PORTS_DIRECTIVE_PH8   },
    {   ADI_FLAG_PH9,   ADI_PORTS_DIRECTIVE_PH9   },
    {   ADI_FLAG_PH10,  ADI_PORTS_DIRECTIVE_PH10  },
    {   ADI_FLAG_PH11,  ADI_PORTS_DIRECTIVE_PH11  },
    {   ADI_FLAG_PH12,  ADI_PORTS_DIRECTIVE_PH12  },
    {   ADI_FLAG_PH13,  ADI_PORTS_DIRECTIVE_PH13  },
    {   ADI_FLAG_PH14,  ADI_PORTS_DIRECTIVE_PH14  },
    {   ADI_FLAG_PH15,  ADI_PORTS_DIRECTIVE_PH15  }
};


#endif

#if defined(__ADSP_BRODIE__) // processor specifics for BF51x (slightly different than Kookaburra and Mockingbird)

typedef struct ADI_FLAG_PORT_MAPPING {  // maps Flag IDs to port control directives
    ADI_FLAG_ID         FlagID;     // Flag ID
    ADI_PORTS_DIRECTIVE Directive;  // port control directive
} ADI_FLAG_PORT_MAPPING;

static ADI_FLAG_PORT_MAPPING PortMappings[] = {   // port mappings
    {   ADI_FLAG_PF0,   ADI_PORTS_DIRECTIVE_PF0   },
    {   ADI_FLAG_PF1,   ADI_PORTS_DIRECTIVE_PF1   },
    {   ADI_FLAG_PF2,   ADI_PORTS_DIRECTIVE_PF2   },
    {   ADI_FLAG_PF3,   ADI_PORTS_DIRECTIVE_PF3   },
    {   ADI_FLAG_PF4,   ADI_PORTS_DIRECTIVE_PF4   },
    {   ADI_FLAG_PF5,   ADI_PORTS_DIRECTIVE_PF5   },
    {   ADI_FLAG_PF6,   ADI_PORTS_DIRECTIVE_PF6   },
    {   ADI_FLAG_PF7,   ADI_PORTS_DIRECTIVE_PF7   },
    {   ADI_FLAG_PF8,   ADI_PORTS_DIRECTIVE_PF8   },
    {   ADI_FLAG_PF9,   ADI_PORTS_DIRECTIVE_PF9   },
    {   ADI_FLAG_PF10,  ADI_PORTS_DIRECTIVE_PF10  },
    {   ADI_FLAG_PF11,  ADI_PORTS_DIRECTIVE_PF11  },
    {   ADI_FLAG_PF12,  ADI_PORTS_DIRECTIVE_PF12  },
    {   ADI_FLAG_PF13,  ADI_PORTS_DIRECTIVE_PF13  },
    {   ADI_FLAG_PF14,  ADI_PORTS_DIRECTIVE_PF14  },
    {   ADI_FLAG_PF15,  ADI_PORTS_DIRECTIVE_PF15  },
    {   ADI_FLAG_PG0,   ADI_PORTS_DIRECTIVE_PG0   }, /* note that PG0 IS configurable on Brodie */
    {   ADI_FLAG_PG1,   ADI_PORTS_DIRECTIVE_PG1   },
    {   ADI_FLAG_PG2,   ADI_PORTS_DIRECTIVE_PG2   },
    {   ADI_FLAG_PG3,   ADI_PORTS_DIRECTIVE_PG3   },
    {   ADI_FLAG_PG4,   ADI_PORTS_DIRECTIVE_PG4   },
    {   ADI_FLAG_PG5,   ADI_PORTS_DIRECTIVE_PG5   },
    {   ADI_FLAG_PG6,   ADI_PORTS_DIRECTIVE_PG6   },
    {   ADI_FLAG_PG7,   ADI_PORTS_DIRECTIVE_PG7   },
    {   ADI_FLAG_PG8,   ADI_PORTS_DIRECTIVE_PG8   },
    {   ADI_FLAG_PG9,   ADI_PORTS_DIRECTIVE_PG9   },
    {   ADI_FLAG_PG10,  ADI_PORTS_DIRECTIVE_PG10  },
    {   ADI_FLAG_PG11,  ADI_PORTS_DIRECTIVE_PG11  },
    {   ADI_FLAG_PG12,  ADI_PORTS_DIRECTIVE_PG12  },
    {   ADI_FLAG_PG13,  ADI_PORTS_DIRECTIVE_PG13  },
    {   ADI_FLAG_PG14,  ADI_PORTS_DIRECTIVE_PG14  },
    {   ADI_FLAG_PG15,  ADI_PORTS_DIRECTIVE_PG15  },
    {   ADI_FLAG_PH0,   ADI_PORTS_DIRECTIVE_PH0   },
    {   ADI_FLAG_PH1,   ADI_PORTS_DIRECTIVE_PH1   },
    {   ADI_FLAG_PH2,   ADI_PORTS_DIRECTIVE_PH2   },
    {   ADI_FLAG_PH3,   ADI_PORTS_DIRECTIVE_PH3   },
    {   ADI_FLAG_PH4,   ADI_PORTS_DIRECTIVE_PH4   },
    {   ADI_FLAG_PH5,   ADI_PORTS_DIRECTIVE_PH5   },
    {   ADI_FLAG_PH6,   ADI_PORTS_DIRECTIVE_PH6   },
    {   ADI_FLAG_PH7,   ADI_PORTS_DIRECTIVE_PH7   },
    // no upper order PortH bits (only 40 bits of GPIO total, including PG0)
};
#endif

/***********
    Moy
***********/

#if defined(__ADSP_MOY__) /* processor specifics for BF50x */

typedef struct ADI_FLAG_PORT_MAPPING {  // maps Flag IDs to port control directives
    ADI_FLAG_ID         FlagID;     // Flag ID
    ADI_PORTS_DIRECTIVE Directive;  // port control directive
} ADI_FLAG_PORT_MAPPING;

static ADI_FLAG_PORT_MAPPING PortMappings[] =   // port mappings
{
    {   ADI_FLAG_PF0,   ADI_PORTS_DIRECTIVE_PF0   },
    {   ADI_FLAG_PF1,   ADI_PORTS_DIRECTIVE_PF1   },
    {   ADI_FLAG_PF2,   ADI_PORTS_DIRECTIVE_PF2   },
    {   ADI_FLAG_PF3,   ADI_PORTS_DIRECTIVE_PF3   },
    {   ADI_FLAG_PF4,   ADI_PORTS_DIRECTIVE_PF4   },
    {   ADI_FLAG_PF5,   ADI_PORTS_DIRECTIVE_PF5   },
    {   ADI_FLAG_PF6,   ADI_PORTS_DIRECTIVE_PF6   },
    {   ADI_FLAG_PF7,   ADI_PORTS_DIRECTIVE_PF7   },
    {   ADI_FLAG_PF8,   ADI_PORTS_DIRECTIVE_PF8   },
    {   ADI_FLAG_PF9,   ADI_PORTS_DIRECTIVE_PF9   },
    {   ADI_FLAG_PF10,  ADI_PORTS_DIRECTIVE_PF10  },
    {   ADI_FLAG_PF11,  ADI_PORTS_DIRECTIVE_PF11  },
    {   ADI_FLAG_PF12,  ADI_PORTS_DIRECTIVE_PF12  },
    {   ADI_FLAG_PF13,  ADI_PORTS_DIRECTIVE_PF13  },
    {   ADI_FLAG_PF14,  ADI_PORTS_DIRECTIVE_PF14  },
    {   ADI_FLAG_PF15,  ADI_PORTS_DIRECTIVE_PF15  },

    {   ADI_FLAG_PG0,   ADI_PORTS_DIRECTIVE_PG0   },
    {   ADI_FLAG_PG1,   ADI_PORTS_DIRECTIVE_PG1   },
    {   ADI_FLAG_PG2,   ADI_PORTS_DIRECTIVE_PG2   },
    {   ADI_FLAG_PG3,   ADI_PORTS_DIRECTIVE_PG3   },
    {   ADI_FLAG_PG4,   ADI_PORTS_DIRECTIVE_PG4   },
    {   ADI_FLAG_PG5,   ADI_PORTS_DIRECTIVE_PG5   },
    {   ADI_FLAG_PG6,   ADI_PORTS_DIRECTIVE_PG6   },
    {   ADI_FLAG_PG7,   ADI_PORTS_DIRECTIVE_PG7   },
    {   ADI_FLAG_PG8,   ADI_PORTS_DIRECTIVE_PG8   },
    {   ADI_FLAG_PG9,   ADI_PORTS_DIRECTIVE_PG9   },
    {   ADI_FLAG_PG10,  ADI_PORTS_DIRECTIVE_PG10  },
    {   ADI_FLAG_PG11,  ADI_PORTS_DIRECTIVE_PG11  },
    {   ADI_FLAG_PG12,  ADI_PORTS_DIRECTIVE_PG12  },
    {   ADI_FLAG_PG13,  ADI_PORTS_DIRECTIVE_PG13  },
    {   ADI_FLAG_PG14,  ADI_PORTS_DIRECTIVE_PG14  },
    {   ADI_FLAG_PG15,  ADI_PORTS_DIRECTIVE_PG15  },

    {   ADI_FLAG_PH0,   ADI_PORTS_DIRECTIVE_PH0   },
    {   ADI_FLAG_PH1,   ADI_PORTS_DIRECTIVE_PH1   },
    {   ADI_FLAG_PH2,   ADI_PORTS_DIRECTIVE_PH2   },

    /* PH3 to PH15 not configurable */

};
#endif

/***********
    Delta
***********/

#if defined(__ADSP_DELTA__) /* processor specifics for BF59x */

typedef struct ADI_FLAG_PORT_MAPPING {  // maps Flag IDs to port control directives
    ADI_FLAG_ID         FlagID;     // Flag ID
    ADI_PORTS_DIRECTIVE Directive;  // port control directive
} ADI_FLAG_PORT_MAPPING;

static ADI_FLAG_PORT_MAPPING PortMappings[] =   // port mappings
{
    {   ADI_FLAG_PF0,   ADI_PORTS_DIRECTIVE_PF0   },
    {   ADI_FLAG_PF1,   ADI_PORTS_DIRECTIVE_PF1   },
    {   ADI_FLAG_PF2,   ADI_PORTS_DIRECTIVE_PF2   },
    {   ADI_FLAG_PF3,   ADI_PORTS_DIRECTIVE_PF3   },
    {   ADI_FLAG_PF4,   ADI_PORTS_DIRECTIVE_PF4   },
    {   ADI_FLAG_PF5,   ADI_PORTS_DIRECTIVE_PF5   },
    {   ADI_FLAG_PF6,   ADI_PORTS_DIRECTIVE_PF6   },
    {   ADI_FLAG_PF7,   ADI_PORTS_DIRECTIVE_PF7   },
    {   ADI_FLAG_PF8,   ADI_PORTS_DIRECTIVE_PF8   },
    {   ADI_FLAG_PF9,   ADI_PORTS_DIRECTIVE_PF9   },
    {   ADI_FLAG_PF10,  ADI_PORTS_DIRECTIVE_PF10  },
    {   ADI_FLAG_PF11,  ADI_PORTS_DIRECTIVE_PF11  },
    {   ADI_FLAG_PF12,  ADI_PORTS_DIRECTIVE_PF12  },
    {   ADI_FLAG_PF13,  ADI_PORTS_DIRECTIVE_PF13  },
    {   ADI_FLAG_PF14,  ADI_PORTS_DIRECTIVE_PF14  },
    {   ADI_FLAG_PF15,  ADI_PORTS_DIRECTIVE_PF15  },

    {   ADI_FLAG_PG0,   ADI_PORTS_DIRECTIVE_PG0   },
    {   ADI_FLAG_PG1,   ADI_PORTS_DIRECTIVE_PG1   },
    {   ADI_FLAG_PG2,   ADI_PORTS_DIRECTIVE_PG2   },
    {   ADI_FLAG_PG3,   ADI_PORTS_DIRECTIVE_PG3   },
    {   ADI_FLAG_PG4,   ADI_PORTS_DIRECTIVE_PG4   },
    {   ADI_FLAG_PG5,   ADI_PORTS_DIRECTIVE_PG5   },
    {   ADI_FLAG_PG6,   ADI_PORTS_DIRECTIVE_PG6   },
    {   ADI_FLAG_PG7,   ADI_PORTS_DIRECTIVE_PG7   },
    {   ADI_FLAG_PG8,   ADI_PORTS_DIRECTIVE_PG8   },
    {   ADI_FLAG_PG9,   ADI_PORTS_DIRECTIVE_PG9   },
    {   ADI_FLAG_PG10,  ADI_PORTS_DIRECTIVE_PG10  },
    {   ADI_FLAG_PG11,  ADI_PORTS_DIRECTIVE_PG11  },
    {   ADI_FLAG_PG12,  ADI_PORTS_DIRECTIVE_PG12  },
    {   ADI_FLAG_PG13,  ADI_PORTS_DIRECTIVE_PG13  },
    {   ADI_FLAG_PG14,  ADI_PORTS_DIRECTIVE_PG14  },
    {   ADI_FLAG_PG15,  ADI_PORTS_DIRECTIVE_PG15  },

};
#endif

/*********************************************************************

Instance data for the flag manager (used only if using callbacks on flag interrupts)

*********************************************************************/

typedef struct ADI_FLAG_CALLBACK_ENTRY {    // callback entry
    ADI_FLAG_ID             FlagID;             // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID;       // peripheral ID
    void                    *ClientHandle;      // client handle argument passed in callbacks
    ADI_DCB_HANDLE          DCBHandle;          // deferred callback service handle
    ADI_DCB_CALLBACK_FN     ClientCallback;     // client callback function
#if defined(__ADSP_MOAB__)          // Moab additions
    ADI_FLAG_INT_INFO       *pInterruptInfo;        // interrupt info for the flag
    u32                     BitToTouch;             // bit to touch within the LATCH etc registers
    u8                      ByteIndex;              // byte within the PINTx_ASSIGN register
#endif
} ADI_FLAG_CALLBACK_ENTRY;


typedef struct ADI_FLAG_INSTANCE_DATA {     // instance data
    u32                     NumEntries;         // number of callback handlers
    void                    *pEnterCriticalArg; // critical region parameter
    ADI_FLAG_CALLBACK_ENTRY *pCallbacks;        // pointer to array of callback handlers
} ADI_FLAG_INSTANCE_DATA;


ADI_FLAG_INSTANCE_DATA adi_flag_InstanceData;   // instance data for the flag manager


/*********************************************************************

static functions for all families

*********************************************************************/

static ADI_INT_HANDLER(FlagHandler);                // flag handler

static ADI_FLAG_RESULT  adi_flag_SetInterruptMask(  // sets/clears interrupt mask
    ADI_FLAG_ID             FlagID,                     // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID,               // peripheral ID
    u32                     EnableInterruptFlag         // enable/disable interrupt flag
);

static inline u32 adi_flag_GetInterruptMask(        // returns the interrupt mask for a flag (TRUE - enabled, FALSE - not enabled)
    ADI_FLAG_ID             FlagID,                     // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID                // peripheral ID
);

static ADI_FLAG_CALLBACK_ENTRY *adi_flag_GetCallbackEntry(  // returns the callback entry for a flag
    ADI_FLAG_ID             FlagID                              // Flag ID
);

#if defined(ADI_SSL_DEBUG)
static ADI_FLAG_RESULT adi_flag_ValidateFlagID(     // validates a flag ID
    ADI_FLAG_ID FlagID
);
#endif


/*********************************************************************

static functions for Stirling

*********************************************************************/

#if defined(__ADSP_STIRLING__)      // Stirling has flags not capable of interrupts

static ADI_FLAG_RESULT adi_flag_ValidateInterruptCapableFlagID(     // validates a flag ID is interrupt capable
    ADI_FLAG_ID FlagID
);


#endif



/*********************************************************************

static functions for Moab

*********************************************************************/

#if defined(__ADSP_MOAB__)     // Moab flags are real different from the other families

static ADI_FLAG_RESULT adi_flag_MapFlagToInterrupt(    // maps a flag to a peripheral interrupt ID
    ADI_FLAG_ID             FlagID,                     // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID,               // peripheral ID
    ADI_FLAG_INT_INFO       **pIntInfo,                 // address where the pointer to interrupt info will be stored
    u32                     *pBitToTouch,               // address where the bit to control the flag will be stored
    u8                      *pByteIndex                 // address where the byte index into the ASSIGN register will be stored
);

static ADI_FLAG_RESULT adi_flag_UnmapFlagFromInterrupt( // unmaps the given flag from the interrupt
    ADI_FLAG_ID             FlagID,                     // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID                // pointer to peripheral ID
);

#endif



/*********************************************************************

    Function:       adi_flag_Init

    Description:    Initializes the flag manager

*********************************************************************/

ADI_FLAG_RESULT adi_flag_Init(      // initializes the flag manager
    void            *pMemory,           // pointer to memory
    const size_t    MemorySize,         // size of memory (in bytes)
    u32             *pMaxEntries,       // number of callback entries supported
    void            *pEnterCriticalArg  // parameter for critical region function
) {

    int i, j;                           // generic counter
    ADI_FLAG_CALLBACK_ENTRY *pEntry;    // pointer to a flag entry

    // debug
#if defined(ADI_SSL_DEBUG)
    if (ADI_FLAG_CALLBACK_MEMORY != sizeof(ADI_FLAG_CALLBACK_ENTRY)) {
        return (ADI_FLAG_RESULT_BAD_CALLBACK_MEMORY_SIZE);
    }
#endif

    // determine how many flag interrupt handlers we can support and notify the application of such
    i = adi_flag_InstanceData.NumEntries = *pMaxEntries = MemorySize/(sizeof(ADI_FLAG_CALLBACK_ENTRY));

    // save the address of the data
    pEntry = adi_flag_InstanceData.pCallbacks = (ADI_FLAG_CALLBACK_ENTRY *)pMemory;

    // save the critical region data
    adi_flag_InstanceData.pEnterCriticalArg = pEnterCriticalArg;

    // initialize the callback handler list
    for (; i; i--, pEntry++) {
        pEntry->FlagID = ADI_FLAG_UNDEFINED;
    }

    // clear the usage counters for Moab flags
#if defined(__ADSP_MOAB__)
    for (i = 0; i < (sizeof(InterruptInfo)/sizeof(InterruptInfo[0])); i++) {
        for (j = 0; j < 4; j++) {
            InterruptInfo[i].UsageCount[j] = 0;
        }
    }
#endif

    // return
    return (ADI_FLAG_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       adi_flag_Terminate

    Description:    Terminates the flag manager

*********************************************************************/

ADI_FLAG_RESULT adi_flag_Terminate(     // terminates the flag manager
    void
) {


    int i;                              // generic counter
    ADI_FLAG_CALLBACK_ENTRY *pEntry;    // pointer to a flag entry
    ADI_FLAG_RESULT         Result;     // return code

    // ever the optimist
    Result = ADI_FLAG_RESULT_SUCCESS;

    // FOR (each entry in the callback list)
    for (i = adi_flag_InstanceData.NumEntries, pEntry = adi_flag_InstanceData.pCallbacks; i; i--, pEntry++) {

        // IF (entry is being used)
        if (pEntry->FlagID != ADI_FLAG_UNDEFINED) {

            // remove the callback
            if ((Result = adi_flag_RemoveCallback(pEntry->FlagID)) != ADI_FLAG_RESULT_SUCCESS) {
                break;
            }

        // ENDIF
        }

    // ENDFOR
    }

    // return
    return (Result);
}



/*********************************************************************

    Function:       adi_flag_Open

    Description:    Opens a flag for use

*********************************************************************/

ADI_FLAG_RESULT adi_flag_Open(  // opens a flag for use
    ADI_FLAG_ID FlagID              // flag ID
){

    // validate Flag ID
#if defined(ADI_SSL_DEBUG)
    if (adi_flag_ValidateFlagID(FlagID)!= ADI_FLAG_RESULT_SUCCESS) {
        return (ADI_FLAG_RESULT_INVALID_FLAG_ID);
    }
#endif

#if defined(__ADSP_BRAEMAR__)  // only need to do this for Braemar

    u32 Directive;  // directive to pass to port control

    // create the directive (*** a bit risky if port control definitions change asynchronously ***)
    // but keeping a mapping would be memory intensive
    Directive = ADI_PORTS_PIN_ENUM_VALUE( FlagID,   0,      0, ADI_PORTS_MUX_CMD_NONE );

    // set port control
    if (adi_ports_EnableGPIO(&Directive, 1, TRUE) != ADI_PORTS_RESULT_SUCCESS) {
        return (ADI_FLAG_RESULT_PORT_CONTROL_ERROR);
    }

#endif

#if defined(__ADSP_STIRLING__)  // only need to do this for Stirling

    ADI_FLAG_PORT_MAPPING   *pMapping;          // pointer to mappings
    u32                     PortControlFlag;    // flag indicating port control needed
    u32                     i;                  // counter

    // scan the mapping table to see if the flag needs port control
    for (PortControlFlag = FALSE, i = 0, pMapping = PortMappings; i < (sizeof(PortMappings)/sizeof(PortMappings[0])); i++, pMapping++) {
        if (pMapping->FlagID == FlagID) {
            PortControlFlag = TRUE;
            break;
        }
    }

    // IF (port control is needed)
    if (PortControlFlag == TRUE) {

        // configure the port
        if (adi_ports_Configure(&pMapping->Directive, 1) != ADI_PORTS_RESULT_SUCCESS) {
            return (ADI_FLAG_RESULT_PORT_CONTROL_ERROR);
        }

    // ENDIF
    }

#endif

#if defined(__ADSP_MOAB__) /* only need to do this for MOAB */

    u32 Directive;  /* directive to pass to port control */

    /* create the directive (*** a bit risky if port control definitions change asynchronously ***) */
    /* but keeping a mapping would be memory intensive */
                                         /*       Port,                     Bit,                 Function, MuxVal */
    Directive = ADI_PORTS_CREATE_DIRECTIVE( ADI_FLAG_GET_PORT(FlagID),  ADI_FLAG_GET_BIT(FlagID), 0,        0   );

    /* set port control */
    if (adi_ports_Configure((ADI_PORTS_DIRECTIVE*)&Directive, 1) != ADI_PORTS_RESULT_SUCCESS)
    {
        return (ADI_FLAG_RESULT_PORT_CONTROL_ERROR);
    }

#endif

#if defined(__ADSP_KOOKABURRA__)  ||\
    defined(__ADSP_MOCKINGBIRD__) ||\
    defined(__ADSP_BRODIE__)      ||\
    defined(__ADSP_MOY__)         ||\
    defined(__ADSP_DELTA__)   
// only need to do this for Kookaburra/Mockingbird/Brodie/Moy/Delta

    ADI_FLAG_PORT_MAPPING   *pMapping;          // pointer to mappings
    u32                     PortControlFlag;    // flag indicating port control needed
    u32                     i;                  // counter

    // scan the mapping table to see if the flag needs port control
    for (PortControlFlag = FALSE, i = 0, pMapping = PortMappings; i < (sizeof(PortMappings)/sizeof(PortMappings[0])); i++, pMapping++) {
        if (pMapping->FlagID == FlagID) {
            PortControlFlag = TRUE;
            break;
        }
    }

    // IF (port control is needed)
    if (PortControlFlag == TRUE) {

        // configure the port
        if (adi_ports_Configure(&pMapping->Directive, 1) != ADI_PORTS_RESULT_SUCCESS) {
            return (ADI_FLAG_RESULT_PORT_CONTROL_ERROR);
        }

    // ENDIF
    }

#endif

    /* return */
    return (ADI_FLAG_RESULT_SUCCESS);
}


/*********************************************************************

    Function:       adi_flag_Close

    Description:    Closes a flag from further use

*********************************************************************/

ADI_FLAG_RESULT adi_flag_Close(     // closes a flag from further use
    ADI_FLAG_ID FlagID                  // flag ID
){

    ADI_FLAG_RESULT             Result;             // return code
    const ADI_FLAG_PORT_INFO    *pPort;             // port info
    void                        *pExitCriticalArg;  // parameter for exit critical
    u16                         Mask;               // mask to clear the input driver register

    // validate Flag ID
#if defined(ADI_SSL_DEBUG)
    if (adi_flag_ValidateFlagID(FlagID)!= ADI_FLAG_RESULT_SUCCESS) {
        return (ADI_FLAG_RESULT_INVALID_FLAG_ID);
    }
#endif

    // uninstall the callback (don't worry if there is no callback installed, or it can't support interrupts)
    Result = adi_flag_RemoveCallback(FlagID);
    if ((Result == ADI_FLAG_RESULT_CALLBACK_NOT_INSTALLED) || (Result == ADI_FLAG_RESULT_NOT_CAPABLE))  {
        Result = ADI_FLAG_RESULT_SUCCESS;
    }

    // point to the port
    pPort = &PortInfo[ADI_FLAG_GET_PORT(FlagID)];

    // get the input driver bit that we're going to clear
    Mask = ADI_FLAG_GET_MASK(FlagID);

    // protect us
    pExitCriticalArg = adi_int_EnterCriticalRegion(adi_flag_InstanceData.pEnterCriticalArg);

    // IF (Moab)
#if defined(__ADSP_MOAB__)

        // disable the input driver
        pPort->pBaseAddress->reg_INEN &= ~Mask;

    // ELSE
#else

        // disable the input driver
        *pPort->InputEnable &= ~Mask;

    // ENDIF
#endif

    // unprotect
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_flag_SetDirection

    Description:    Sets the direction of a flag and enables it as an input

*********************************************************************/

ADI_FLAG_RESULT adi_flag_SetDirection(      // sets the direction of a flag
    ADI_FLAG_ID         FlagID,                 // flag ID
    ADI_FLAG_DIRECTION  Direction               // direction
){

    const ADI_FLAG_PORT_INFO    *pPort;             // port info
    u16                         Dir;                // direction register
    u16                         Inen;               // input enable register
    u16                         ORMask;             // mask to enable bits
    u16                         ANDMask;            // mask to clear bits
    void                        *pExitCriticalArg;  // parameter for exit critical

    // validate Flag ID
#if defined(ADI_SSL_DEBUG)
    if (adi_flag_ValidateFlagID(FlagID)!= ADI_FLAG_RESULT_SUCCESS) {
        return (ADI_FLAG_RESULT_INVALID_FLAG_ID);
    }
#endif

    // IF (Moab)
#if defined(__ADSP_MOAB__)

        // point to the port
        pPort = &PortInfo[ADI_FLAG_GET_PORT(FlagID)];

        // get the bit that we're going to set
        ORMask = ADI_FLAG_GET_MASK(FlagID);

        // protect us
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_flag_InstanceData.pEnterCriticalArg);

        // IF (configuring for inbound)
        if (Direction == ADI_FLAG_DIRECTION_INPUT) {

            // set the direction and input enable register
            pPort->pBaseAddress->reg_DIR_CLEAR = ORMask;
            pPort->pBaseAddress->reg_INEN |= ORMask;

        // ELSE
        } else {

            // set the direction and disable the input enable
            pPort->pBaseAddress->reg_DIR_SET = ORMask;
            pPort->pBaseAddress->reg_INEN &= ~ORMask;

        // ENDIF
        }

        // unprotect
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ELSE
#else

        // create the masks
        ORMask = ADI_FLAG_GET_MASK(FlagID);
        ANDMask = ~ORMask;

        // point to the port
        pPort = &PortInfo[ADI_FLAG_GET_PORT(FlagID)];

        // protect us
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_flag_InstanceData.pEnterCriticalArg);

        // get the registers
        Dir = *pPort->Direction;
        Inen = *pPort->InputEnable;

        // set the bits appropriately
        if (Direction) {
            Dir |= ORMask;
            Inen &= ANDMask;
        } else {
            Dir &= ANDMask;
            Inen |= ORMask;
        }

        // update the registers
        *pPort->Direction = Dir;
        *pPort->InputEnable = Inen;

        // unprotect
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ENDIF
#endif

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return (ADI_FLAG_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       adi_flag_Set

    Description:    Sets a flag to 1

*********************************************************************/

ADI_FLAG_RESULT adi_flag_Set(   // sets the flag value to 1
    ADI_FLAG_ID FlagID              // flag ID
){

    // validate Flag ID
#if defined(ADI_SSL_DEBUG)
    if (adi_flag_ValidateFlagID(FlagID)!= ADI_FLAG_RESULT_SUCCESS) {
        return (ADI_FLAG_RESULT_INVALID_FLAG_ID);
    }
#endif

    // IF (Moab)
#if defined(__ADSP_MOAB__)

        // set the bit for the flag
        PortInfo[ADI_FLAG_GET_PORT(FlagID)].pBaseAddress->reg_SET = ADI_FLAG_GET_MASK(FlagID);

    // ELSE
#else

        // set the bit for the flag
        *PortInfo[ADI_FLAG_GET_PORT(FlagID)].Set = ADI_FLAG_GET_MASK(FlagID);

    // ENDIF
#endif

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return (ADI_FLAG_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       adi_flag_Clear

    Description:    Sets a flag to 0

*********************************************************************/

ADI_FLAG_RESULT adi_flag_Clear( // sets the flag value to 0
    ADI_FLAG_ID FlagID              // flag ID
){

    // validate Flag ID
#if defined(ADI_SSL_DEBUG)
    if (adi_flag_ValidateFlagID(FlagID)!= ADI_FLAG_RESULT_SUCCESS) {
        return (ADI_FLAG_RESULT_INVALID_FLAG_ID);
    }
#endif

    // IF (Moab)
#if defined(__ADSP_MOAB__)

        // set the bit for the flag
        PortInfo[ADI_FLAG_GET_PORT(FlagID)].pBaseAddress->reg_CLEAR = ADI_FLAG_GET_MASK(FlagID);

    // ELSE
#else

        // clear the bit for the flag
        *PortInfo[ADI_FLAG_GET_PORT(FlagID)].Clear = ADI_FLAG_GET_MASK(FlagID);

    // ENDIF
#endif

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return (ADI_FLAG_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       adi_flag_Toggle

    Description:    Toggles a flag value

*********************************************************************/

ADI_FLAG_RESULT adi_flag_Toggle(    // toggles a flag value
    ADI_FLAG_ID FlagID                  // flag ID
){

    const ADI_FLAG_PORT_INFO    *pPort;             // port info
    void                        *pExitCriticalArg;  // parameter for exit critical
    u16                         Mask;               // bit corresponding to the flag

    // validate Flag ID
#if defined(ADI_SSL_DEBUG)
    if (adi_flag_ValidateFlagID(FlagID)!= ADI_FLAG_RESULT_SUCCESS) {
        return (ADI_FLAG_RESULT_INVALID_FLAG_ID);
    }
#endif

    // IF (Moab)
#if defined(__ADSP_MOAB__)

        // point to the port
        pPort = &PortInfo[ADI_FLAG_GET_PORT(FlagID)];

        // get the bit that we're going to set
        Mask = ADI_FLAG_GET_MASK(FlagID);

        // protect us
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_flag_InstanceData.pEnterCriticalArg);

        // IF (the flag is currently set)
        if (pPort->pBaseAddress->reg_DATA & Mask) {

            // clear it
            pPort->pBaseAddress->reg_DATA &= ~Mask;

        // ELSE
        } else {

            // set it
            pPort->pBaseAddress->reg_DATA |= Mask;

        // ENDIF
        }

        // unprotect
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ELSE
#else

        // toggle the bit for the flag
        *PortInfo[ADI_FLAG_GET_PORT(FlagID)].Toggle = ADI_FLAG_GET_MASK(FlagID);

    // ENDIF
#endif

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return (ADI_FLAG_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       adi_flag_Sense

    Description:    Senses a flag value

*********************************************************************/

ADI_FLAG_RESULT adi_flag_Sense(     // senses the flag value
    ADI_FLAG_ID FlagID,                 // flag ID
    u32         *pValue                 // location to store value
) {

    // validate Flag ID
#if defined(ADI_SSL_DEBUG)
    if (adi_flag_ValidateFlagID(FlagID)!= ADI_FLAG_RESULT_SUCCESS) {
        return (ADI_FLAG_RESULT_INVALID_FLAG_ID);
    }
#endif

    // IF (Moab)
#if defined(__ADSP_MOAB__)

        // sense the flag
        *pValue = (((PortInfo[ADI_FLAG_GET_PORT(FlagID)].pBaseAddress->reg_DATA) >> ADI_FLAG_GET_BIT(FlagID)) & 0x1);

    // ELSE
#else

        // sense the flag
        *pValue = (((*PortInfo[ADI_FLAG_GET_PORT(FlagID)].Data) >> ADI_FLAG_GET_BIT(FlagID)) & 0x1);

    // ENDIF
#endif

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return (ADI_FLAG_RESULT_SUCCESS);
}




/*********************************************************************

    Function:       adi_flag_InstallCallback

    Description:    Installs a callback for a flag

*********************************************************************/

ADI_FLAG_RESULT adi_flag_InstallCallback(   // installs a callback for a flag
    ADI_FLAG_ID             FlagID,             // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID,       // peripheral ID
    ADI_FLAG_TRIGGER        Trigger,            // trigger
    u32                     WakeupFlag,         // wakeup flag (TRUE/FALSE)
    void                    *ClientHandle,      // client handle argument passed in callbacks
    ADI_DCB_HANDLE          DCBHandle,          // deferred callback service handle
    ADI_DCB_CALLBACK_FN     ClientCallback      // client callback function
) {

    int                     i;                  // generic counter
    u32                     IVG;                // IVG for the flag
    ADI_FLAG_CALLBACK_ENTRY *pEntry;            // pointer to a flag entry
    void                    *pExitCriticalArg;  // parameter for exit critical
    ADI_FLAG_RESULT         FlagResult;         // result
    ADI_INT_RESULT          IntResult;          // result

    // validate Flag ID and on Stirling, validate that the flag supports interrupts
#if defined(ADI_SSL_DEBUG)
    if ((FlagResult = adi_flag_ValidateFlagID(FlagID))!= ADI_FLAG_RESULT_SUCCESS) {
        return (FlagResult);
    }
#if defined(__ADSP_STIRLING__)
    if ((FlagResult = adi_flag_ValidateInterruptCapableFlagID(FlagID))!= ADI_FLAG_RESULT_SUCCESS) {
        return (FlagResult);
    }
#endif

#endif

    // protect us
    pExitCriticalArg = adi_int_EnterCriticalRegion(adi_flag_InstanceData.pEnterCriticalArg);

    // FOR (each entry in the callback list)
    for (i = adi_flag_InstanceData.NumEntries, pEntry = adi_flag_InstanceData.pCallbacks; i; i--, pEntry++) {

        // IF (entry is free)
        if (pEntry->FlagID == ADI_FLAG_UNDEFINED) {

            // save the FlagID in the callback to indicate it's in use
            pEntry->FlagID = FlagID;

            // exit the loop
            break;

        // ENDIF
        }

    //  ENDFOR
    }

    // unprotect us
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // return if we don't have a space for the callback
    if (!i) {
        return(ADI_FLAG_RESULT_ALL_IN_USE);
    }

    // populate the rest of the callback structure
    pEntry->PeripheralID = PeripheralID;
    pEntry->ClientHandle = ClientHandle;
    pEntry->DCBHandle = DCBHandle;
    pEntry->ClientCallback = ClientCallback;

    // assume success
    FlagResult = ADI_FLAG_RESULT_SUCCESS;
    IntResult = ADI_INT_RESULT_SUCCESS;

    // make a convenient way to punch out on any error
    do {

        // IF (Moab)
#if defined(__ADSP_MOAB__)

            // map the flag to the interrupt and add the Moab additions to the callback entry
            if ((FlagResult = adi_flag_MapFlagToInterrupt(FlagID, PeripheralID, &pEntry->pInterruptInfo, &pEntry->BitToTouch, &pEntry->ByteIndex)) != ADI_FLAG_RESULT_SUCCESS) {
                break;
            }

        // ENDIF
#endif

        // set the transition type for the callback
        if ((FlagResult = adi_flag_SetTrigger(FlagID, Trigger)) != ADI_FLAG_RESULT_SUCCESS) {
            break;
        }

        // IF (Moab)
#if defined(__ADSP_MOAB__)

            // clear any interrupts already latched
            pEntry->pInterruptInfo->pBaseAddress->reg_LATCH = pEntry->BitToTouch;

#else   // ELSE (all other Blackfins)

            // clear any interrupts already latched
            *PortInfo[ADI_FLAG_GET_PORT(FlagID)].Clear = ADI_FLAG_GET_MASK(FlagID);

#endif

        // get the IVG to which the peripheral is mapped
        if ((IntResult = adi_int_SICGetIVG(PeripheralID, &IVG)) != ADI_INT_RESULT_SUCCESS) {
            break;
        }

        // set wakeup accordingly
        if ((IntResult = adi_int_SICWakeup(PeripheralID, WakeupFlag)) != ADI_INT_RESULT_SUCCESS) {
            break;
        }

        // hook the handler into the chain
        if ((IntResult = adi_int_CECHook(IVG, FlagHandler, (void *)pEntry, TRUE)) != ADI_INT_RESULT_SUCCESS) {
            break;
        }

        // enable the interrupt through the SIC
        if ((IntResult = adi_int_SICEnable(PeripheralID)) != ADI_INT_RESULT_SUCCESS) {
            break;
        }

        // enable the interrupt from the flag
        if ((FlagResult = adi_flag_SetInterruptMask(FlagID, PeripheralID, TRUE)) != ADI_FLAG_RESULT_SUCCESS) {
            break;
        }
        break;

    // end DO
    } while (0);

    // free up the entry if we had any error and return the result
    if ((FlagResult != ADI_FLAG_RESULT_SUCCESS) || (IntResult != ADI_INT_RESULT_SUCCESS)) {
        pEntry->FlagID = ADI_FLAG_UNDEFINED;
        if (IntResult != ADI_INT_RESULT_SUCCESS) {
            FlagResult = ADI_FLAG_RESULT_INTERRUPT_MANAGER_ERROR;
        }
    }

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return (FlagResult);
}



/*********************************************************************

    Function:       adi_flag_RemoveCallback

    Description:    Removes a callback for a flag.
                    Originally, we disabled the peripheral interrupt from
                    being passed to the CEC by calling adi_int_SICDisable()
                    here.  We don't do that anymore as if someone outside the
                    flag service is using that interrupt, we could inadvertantly
                    disable them. This is probably acceptable as the interrupts
                    from the flag are being disabled at the flag, so even if no
                    one else is using that interrupt, it shouldn't generate an
                    interrupt.

*********************************************************************/

ADI_FLAG_RESULT adi_flag_RemoveCallback(    // removes a callback for a flag
    ADI_FLAG_ID FlagID                          // flag ID
) {

    u32                     IVG;        // IVG for the flag
    ADI_FLAG_CALLBACK_ENTRY *pEntry;    // pointer to a flag entry
    ADI_FLAG_RESULT         Result;     // return code

    // validate Flag ID and on Stirling, validate that the flag supports interrupts
#if defined(ADI_SSL_DEBUG)
    if ((Result = adi_flag_ValidateFlagID(FlagID))!= ADI_FLAG_RESULT_SUCCESS) {
        return (Result);
    }
#if defined(__ADSP_STIRLING__)
    if ((Result = adi_flag_ValidateInterruptCapableFlagID(FlagID))!= ADI_FLAG_RESULT_SUCCESS) {
        return (Result);
    }
#endif

#endif

    // assume success
    Result = ADI_FLAG_RESULT_SUCCESS;

    // make a convenient way to punch out on any error
    do {

        // locate the callback entry for the flag
        if ((pEntry = adi_flag_GetCallbackEntry(FlagID)) == NULL) {
            Result = ADI_FLAG_RESULT_CALLBACK_NOT_INSTALLED;
            break;
        }

        // disable the interrupt from the flag
        if ((Result = adi_flag_SetInterruptMask(FlagID, pEntry->PeripheralID, FALSE)) != ADI_FLAG_RESULT_SUCCESS) {
            break;
        }

        // IF (Moab)
#if defined(__ADSP_MOAB__)

            // unmap the flag from the interrupt
            if ((Result = adi_flag_UnmapFlagFromInterrupt(FlagID, pEntry->PeripheralID)) != ADI_FLAG_RESULT_SUCCESS) {
                break;
            }

        // ENDIF
#endif

        // get the IVG to which the peripheral is mapped
        if (adi_int_SICGetIVG(pEntry->PeripheralID, &IVG) != ADI_INT_RESULT_SUCCESS) {
            Result = ADI_FLAG_RESULT_INTERRUPT_MANAGER_ERROR;
            break;
        }

        // unhook the handler from the chain
        if (adi_int_CECUnhook(IVG, FlagHandler, (void *)pEntry) != ADI_INT_RESULT_SUCCESS) {
            Result = ADI_FLAG_RESULT_INTERRUPT_MANAGER_ERROR;
            break;
        }

        // free the entry
        pEntry->FlagID = ADI_FLAG_UNDEFINED;

    // end DO
    } while (0);

    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_flag_SuspendCallbacks

    Description:    Temporarily suspends callbacks while leaving callback
                    function installed

*********************************************************************/

ADI_FLAG_RESULT adi_flag_SuspendCallbacks(  // suspends callbacks without removing the callback function
    ADI_FLAG_ID             FlagID,             // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID        // peripheral ID
) {

    ADI_FLAG_RESULT     Result;             // return code

    // validate Flag ID and on Stirling, validate that the flag supports interrupts
#if defined(ADI_SSL_DEBUG)
    if ((Result = adi_flag_ValidateFlagID(FlagID))!= ADI_FLAG_RESULT_SUCCESS) {
        return (Result);
    }
#if defined(__ADSP_STIRLING__)
    if ((Result = adi_flag_ValidateInterruptCapableFlagID(FlagID))!= ADI_FLAG_RESULT_SUCCESS) {
        return (Result);
    }
#endif

#endif

    // mask the interrupt
    return (adi_flag_SetInterruptMask(FlagID, PeripheralID, FALSE));
}


/*********************************************************************

    Function:       adi_flag_ResumeCallbacks

    Description:    Resumes callbacks

*********************************************************************/

ADI_FLAG_RESULT adi_flag_ResumeCallbacks(   // resumes callback processing
    ADI_FLAG_ID             FlagID,             // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID        // peripheral ID
) {

    ADI_FLAG_RESULT     Result;             // return code

    // validate Flag ID and on Stirling, validate that the flag supports interrupts
#if defined(ADI_SSL_DEBUG)
    if ((Result = adi_flag_ValidateFlagID(FlagID))!= ADI_FLAG_RESULT_SUCCESS) {
        return (Result);
    }
#if defined(__ADSP_STIRLING__)
    if ((Result = adi_flag_ValidateInterruptCapableFlagID(FlagID))!= ADI_FLAG_RESULT_SUCCESS) {
        return (Result);
    }
#endif

#endif

    // unmask the interrupt
    return (adi_flag_SetInterruptMask(FlagID, PeripheralID, TRUE));
}



/*********************************************************************

    Function:       adi_flag_SetTrigger

    Description:    Sets the type of trigger that generates a callback

*********************************************************************/
ADI_FLAG_RESULT adi_flag_SetTrigger(    // sets the type of trigger that generates a flag callback
    ADI_FLAG_ID         FlagID,             // flag ID
    ADI_FLAG_TRIGGER    Trigger             // trigger type
){

    void                        *pExitCriticalArg;  // parameter for exit critical
    ADI_FLAG_RESULT             Result;             // return code

    // debug
#if defined(ADI_SSL_DEBUG)
    // verify the flag ID
    if ((Result = adi_flag_ValidateFlagID(FlagID))!= ADI_FLAG_RESULT_SUCCESS) {
        return (Result);
    }
#if defined(__ADSP_STIRLING__)
    // verify the flag supports interrupt generation
    if ((Result = adi_flag_ValidateInterruptCapableFlagID(FlagID))!= ADI_FLAG_RESULT_SUCCESS) {
        return (Result);
    }
#endif
#if defined(__ADSP_MOAB__)
    // Moab doesn't support both edges
    if (Trigger == ADI_FLAG_TRIGGER_BOTH_EDGES) {
        return (ADI_FLAG_RESULT_TRIGGER_TYPE_NOT_SUPPORTED);
    }
#endif
#endif

    // IF (Moab)
#if defined(__ADSP_MOAB__)

        ADI_FLAG_INT_REGISTERS  *pBaseAddress;  // base address of the registers
        ADI_FLAG_CALLBACK_ENTRY *pEntry;        // pointer to a flag entry
        u32                      BitToTouch;     // bit to control the flag

        // assume the worst
        Result = ADI_FLAG_RESULT_CALLBACK_NOT_INSTALLED;

        // IF (we can get the callback entry for this flag)
        if ((pEntry = adi_flag_GetCallbackEntry(FlagID)) != NULL) {

            // make local copies to save code space
            pBaseAddress = pEntry->pInterruptInfo->pBaseAddress;
            BitToTouch = pEntry->BitToTouch;

            // protect us
            pExitCriticalArg = adi_int_EnterCriticalRegion(adi_flag_InstanceData.pEnterCriticalArg);

            // set/clear the proper bits within the proper registers
            switch (Trigger) {
                case ADI_FLAG_TRIGGER_LEVEL_HIGH:       // want 0 for polarity, 0 for edge
                    pBaseAddress->reg_INVERT_CLEAR = BitToTouch;
                    pBaseAddress->reg_EDGE_CLEAR = BitToTouch;
                    break;
                case ADI_FLAG_TRIGGER_LEVEL_LOW:        // want 1 for polarity, 0 for edge
                    pBaseAddress->reg_INVERT_SET = BitToTouch;
                    pBaseAddress->reg_EDGE_CLEAR = BitToTouch;
                    break;
                case ADI_FLAG_TRIGGER_RISING_EDGE:      // want 0 for polarity, 1 for edge
                    pBaseAddress->reg_INVERT_CLEAR = BitToTouch;
                    pBaseAddress->reg_EDGE_SET = BitToTouch;
                    break;
                case ADI_FLAG_TRIGGER_FALLING_EDGE:     // want 1 for polarity, 1 for edge
                    pBaseAddress->reg_INVERT_SET = BitToTouch;
                    pBaseAddress->reg_EDGE_SET = BitToTouch;
                    break;
            }

            // unprotect us
            adi_int_ExitCriticalRegion(pExitCriticalArg);

            // looks like we're successful
            Result = ADI_FLAG_RESULT_SUCCESS;

        // ENDIF
        }

    // ELSE
#else

        const ADI_FLAG_PORT_INFO    *pPort;             // port info
        u16                         Polarity;           // polarity
        u16                         Both;               // both
        u16                         Edge;               // edge
        u16                         ORMask;             // mask to OR with
        u16                         ANDMask;            // mask to AND with

        // assume success
        Result = ADI_FLAG_RESULT_SUCCESS;

        // point to the port
        pPort = &PortInfo[ADI_FLAG_GET_PORT(FlagID)];

        // create a mask that we'll OR with the register (when we want to set the flag's bit)
        ORMask = ADI_FLAG_GET_MASK(FlagID);

        // create mask that we'll AND with the register (when we want to clear the flag's bit)
        ANDMask = ~ORMask;

        // protect us
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_flag_InstanceData.pEnterCriticalArg);

        // read in the registers
        Polarity = *pPort->Polarity;
        Edge = *pPort->Edge;
        Both = *pPort->Both;

        // set/clear the proper bits
        switch (Trigger) {
            case ADI_FLAG_TRIGGER_LEVEL_HIGH:       // want 0 for polarity, 0 for edge and 0 for both
                Polarity    &= ANDMask;
                Edge        &= ANDMask;
                Both        &= ANDMask;
                break;
            case ADI_FLAG_TRIGGER_LEVEL_LOW:        // want 1 for polarity, 0 for edge and 0 for both
                Polarity    |= ORMask;
                Edge        &= ANDMask;
                Both        &= ANDMask;
                break;
            case ADI_FLAG_TRIGGER_RISING_EDGE:      // want 0 for polarity, 1 for edge and 0 for both
                Polarity    &= ANDMask;
                Edge        |= ORMask;
                Both        &= ANDMask;
                break;
            case ADI_FLAG_TRIGGER_FALLING_EDGE:     // want 1 for polarity, 1 for edge and 0 for both
                Polarity    |= ORMask;
                Edge        |= ORMask;
                Both        &= ANDMask;
                break;
            case ADI_FLAG_TRIGGER_BOTH_EDGES:       // want 0 for polarity, 1 for edge and 1 for both
                Polarity    &= ANDMask;
                Edge        |= ORMask;
                Both        |= ORMask;
                break;
        }

        // write the registers
        *pPort->Polarity = Polarity;
        *pPort->Edge = Edge;
        *pPort->Both = Both;

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ENDIF
#endif

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return (Result);
}



/*********************************************************************

    Function:       FlagHandler

    Description:    Processes flag interrupts and posts callbacks

*********************************************************************/

static ADI_INT_HANDLER(FlagHandler)     // flag handler
{

    ADI_FLAG_CALLBACK_ENTRY *pEntry;    // pointer to a flag entry
    ADI_INT_HANDLER_RESULT  Result;     // return code

    // assume the interrupt is not for us
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    // point to the entry for the flag
    pEntry = (ADI_FLAG_CALLBACK_ENTRY *)ClientArg;

    // IF (Moab)
#if defined(__ADSP_MOAB__)

        // IF (the flag is asserting an interrupt)
        if (pEntry->pInterruptInfo->pBaseAddress->reg_IRQ & pEntry->BitToTouch) {

            // IF (the flag has interrupts enabled for it)
            if (pEntry->pInterruptInfo->pBaseAddress->reg_MASK_SET & pEntry->BitToTouch) {

                // notify the callback function that the flag triggerred
                // parameters to the callback function are (ClientHandle, ADI_FLAG_EVENT_CALLBACK, FlagID)
                if (pEntry->DCBHandle) {
                    adi_dcb_Post(pEntry->DCBHandle, 0, pEntry->ClientCallback, pEntry->ClientHandle, ADI_FLAG_EVENT_CALLBACK, (void *)pEntry->FlagID);
                } else {
                    (pEntry->ClientCallback)(pEntry->ClientHandle, ADI_FLAG_EVENT_CALLBACK, (void *)pEntry->FlagID);
                }

                // clear the interrupt
                pEntry->pInterruptInfo->pBaseAddress->reg_IRQ = pEntry->BitToTouch;

                // the interrupt was for us
                Result = ADI_INT_RESULT_PROCESSED;

            // ENDIF
            }

        // ENDIF
        }

    // ELSE
#else

        // IF (the flag is asserting an interrupt)
        if (*PortInfo[ADI_FLAG_GET_PORT(pEntry->FlagID)].Data & ADI_FLAG_GET_MASK(pEntry->FlagID)) {

            // IF (the flag has interrupts enabled for it)
            if (adi_flag_GetInterruptMask(pEntry->FlagID, pEntry->PeripheralID)) {

                // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
                ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

                // notify the callback function that the flag triggerred
                // parameters to the callback function are (ClientHandle, ADI_FLAG_EVENT_CALLBACK, FlagID)
                if (pEntry->DCBHandle) {
                    adi_dcb_Post(pEntry->DCBHandle, 0, pEntry->ClientCallback, pEntry->ClientHandle, ADI_FLAG_EVENT_CALLBACK, (void *)pEntry->FlagID);
                } else {
                    (pEntry->ClientCallback)(pEntry->ClientHandle, ADI_FLAG_EVENT_CALLBACK, (void *)pEntry->FlagID);
                }

                // clear the interrupt
                *PortInfo[ADI_FLAG_GET_PORT(pEntry->FlagID)].Clear = ADI_FLAG_GET_MASK(pEntry->FlagID);

                // the interrupt was for us
                Result = ADI_INT_RESULT_PROCESSED;

            // ENDIF
            }

        // ENDIF
        }

    // ENDIF
#endif

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return(Result);
}




/*********************************************************************

    Function:       adi_flag_GetCallbackEntry

    Description:    Finds the callback entry for a flag

*********************************************************************/

static ADI_FLAG_CALLBACK_ENTRY *adi_flag_GetCallbackEntry(  // returns the callback entry for a flag
    ADI_FLAG_ID             FlagID                              // Flag ID
) {

    ADI_FLAG_CALLBACK_ENTRY *pEntry;    // pointer to a flag entry
    u32                     i;          // counter

    // FOR (each entry in the callback list)
    for (i = adi_flag_InstanceData.NumEntries, pEntry = adi_flag_InstanceData.pCallbacks; i; i--, pEntry++) {

        // IF (this is the entry)
        if (pEntry->FlagID == FlagID) {

            // don't look any further
            break;

        // ENDIF
        }

    // ENDFOR
    }

    // return NULL if not found
    if (i == 0) {
        pEntry = NULL;
    }

    // return
    return (pEntry);
}






/*********************************************************************

    Function:       adi_flag_SetInterruptMask

    Description:    Sets/clears the interrupt mask bit for a flag

*********************************************************************/

static ADI_FLAG_RESULT  adi_flag_SetInterruptMask(  // sets/clears interrupt mask
    ADI_FLAG_ID             FlagID,                     // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID,               // peripheral ID
    u32                     EnableInterruptFlag         // enable/disable interrupt flag
) {

    ADI_FLAG_RESULT     Result;     // return code

#if defined(__ADSP_MOAB__)  // BF54x

    ADI_FLAG_CALLBACK_ENTRY *pEntry;    // pointer to a flag entry

    // assume success
    Result = ADI_FLAG_RESULT_SUCCESS;

    // IF (we can get the entry for the callback)
    if ((pEntry = adi_flag_GetCallbackEntry(FlagID)) != NULL) {

        // mask the interrupt accordingly
        if (EnableInterruptFlag) {
            pEntry->pInterruptInfo->pBaseAddress->reg_MASK_SET = pEntry->BitToTouch;
        } else {
            pEntry->pInterruptInfo->pBaseAddress->reg_MASK_CLEAR = pEntry->BitToTouch;
        }

    // ELSE
    } else {

        // return error
        Result = ADI_FLAG_RESULT_CALLBACK_NOT_INSTALLED;

    // ENDIF
    }

#else

    volatile    u16                 *pRegister; // register to access
    const       ADI_FLAG_PORT_INFO  *pPort;     // port info

    // be optimistic
    Result = ADI_FLAG_RESULT_SUCCESS;

    // point to the port
    pPort = &PortInfo[ADI_FLAG_GET_PORT(FlagID)];

    // identify the register whose bit we want to set
#if defined(__ADSP_EDINBURGH__)         // BF531, BF532, BF533
    if (PeripheralID == ADI_INT_PFA) {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskASet;
        } else {
            pRegister = pPort->MaskAClear;
        }
    } else {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskBSet;
        } else {
            pRegister = pPort->MaskBClear;
        }
    }
#endif
#if defined(__ADSP_BRAEMAR__)           // BF534, BF536, BF537
    if ((PeripheralID == ADI_INT_PORTFG_A) || (PeripheralID == ADI_INT_DMA1_ETHERNET_RX_PORTH_A)) {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskASet;
        } else {
            pRegister = pPort->MaskAClear;
        }
    } else {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskBSet;
        } else {
            pRegister = pPort->MaskBClear;
        }
    }
#endif

/* BF52x, BF51x & BF50x */
#if defined(__ADSP_KOOKABURRA__)  ||\
    defined(__ADSP_MOCKINGBIRD__) ||\
    defined(__ADSP_BRODIE__)      ||\
    defined(__ADSP_MOY__)
    if (PeripheralID == ADI_INT_PORTF_INTA || PeripheralID == ADI_INT_PORTG_INTA || PeripheralID == ADI_INT_PORTH_INTA) {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskASet;
        } else {
            pRegister = pPort->MaskAClear;
        }
    } else  {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskBSet;
        } else {
            pRegister = pPort->MaskBClear;
        }
    }
#endif

/* BF59x */
#if defined(__ADSP_DELTA__)       
    if (PeripheralID == ADI_INT_PORTF_INTA || PeripheralID == ADI_INT_PORTG_INTA) {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskASet;
        } else {
            pRegister = pPort->MaskAClear;
        }
    } else  {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskBSet;
        } else {
            pRegister = pPort->MaskBClear;
        }
    }
#endif

#if defined(__ADSP_TETON__)             // BF561
    if ((PeripheralID == ADI_INT_PF0_15_A) || (PeripheralID == ADI_INT_PF16_31_A) || (PeripheralID == ADI_INT_PF32_47_A)) {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskASet;
        } else {
            pRegister = pPort->MaskAClear;
        }
    } else {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskBSet;
        } else {
            pRegister = pPort->MaskBClear;
        }
    }
#endif
#if defined(__ADSP_STIRLING__)      // BF538, BF539
    if (PeripheralID == ADI_INT_PFA) {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskASet;
        } else {
            pRegister = pPort->MaskAClear;
        }
    } else {
        if (EnableInterruptFlag) {
            pRegister = pPort->MaskBSet;
        } else {
            pRegister = pPort->MaskBClear;
        }
    }
#endif

    // write the register
    *pRegister = ADI_FLAG_GET_MASK(FlagID);

#endif  // non-Moab parts

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return (Result);
}




/*********************************************************************

    Function:       adi_flag_GetInterruptMask

    Description:    Returns the interrupt mask bit for a flag,
                        TRUE - interrupt enabled
                        FALSE - interrupt disabled

*********************************************************************/

static inline u32 adi_flag_GetInterruptMask(        // returns the interrupt mask
    ADI_FLAG_ID             FlagID,             // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID        // peripheral ID
) {

    u32 Result;     // result

#if defined(__ADSP_MOAB__)      // BF54x
    ADI_FLAG_CALLBACK_ENTRY *pEntry;    // pointer to a flag entry

    // IF (we can get the entry for the callback)
    if ((pEntry = adi_flag_GetCallbackEntry(FlagID)) != NULL) {

        // return the status
        Result = pEntry->pInterruptInfo->pBaseAddress->reg_MASK_SET & pEntry->BitToTouch;

    // ELSE
    } else {

        // couldn't find the entry so return false
        Result = FALSE;

    // ENDIF
    }

#else

    volatile    u16                 *pRegister; // register to access
    const       ADI_FLAG_PORT_INFO  *pPort;     // port info

    // point to the port
    pPort = &PortInfo[ADI_FLAG_GET_PORT(FlagID)];

    // identify the register whose bit we want to sense
#if defined(__ADSP_EDINBURGH__)         // BF531, BF532, BF533
    if (PeripheralID == ADI_INT_PFA) {
        pRegister = pPort->MaskAData;
    } else {
        pRegister = pPort->MaskBData;
    }
#endif
#if defined(__ADSP_BRAEMAR__)               // BF534, BF536, BF537
    if (PeripheralID == ADI_INT_PORTFG_A) {
        pRegister = pPort->MaskAData;
    } else {
        pRegister = pPort->MaskBData;
    }
#endif

/* BF52x, BF51x & BF50x */
#if defined(__ADSP_KOOKABURRA__)  ||\
    defined(__ADSP_MOCKINGBIRD__) ||\
    defined(__ADSP_BRODIE__)      ||\
    defined(__ADSP_MOY__)
    if (PeripheralID == ADI_INT_PORTF_INTA || PeripheralID == ADI_INT_PORTG_INTA || PeripheralID == ADI_INT_PORTH_INTA) {
        pRegister = pPort->MaskAData;
    } else {
        pRegister = pPort->MaskBData;
    }
#endif
/* BF59x */
#if defined(__ADSP_DELTA__)
    if (PeripheralID == ADI_INT_PORTF_INTA || PeripheralID == ADI_INT_PORTG_INTA) {
        pRegister = pPort->MaskAData;
    } else {
        pRegister = pPort->MaskBData;
    }
#endif
#if defined(__ADSP_TETON__)             // BF561
    if ((PeripheralID == ADI_INT_PF0_15_A) || (PeripheralID == ADI_INT_PF16_31_A) || (PeripheralID == ADI_INT_PF32_47_A)) {
        pRegister = pPort->MaskAData;
    } else {
        pRegister = pPort->MaskBData;
    }
#endif
#if defined(__ADSP_STIRLING__)      // BF538, BF539
    if (PeripheralID == ADI_INT_PFA) {
        pRegister = pPort->MaskAData;
    } else {
        pRegister = pPort->MaskBData;
    }
#endif

    // sense the bit
    Result = (*pRegister >> ADI_FLAG_GET_BIT(FlagID)) & 0x1;

#endif // non-Moab

    // silicon anomaly workaround 05-00-0311(if necessary)
#if defined(ADI_FLAG_SILICON_ANOMALY_05000311)
    ADI_FLAG_SILICON_ANOMALY_05000311;
#endif

    // return
    return (Result);

}





#if defined(__ADSP_MOAB__) // Moab only
/*********************************************************************

    Function:       adi_flag_MapFlagToInterrupt

    Description:    Maps the given FlagID to the given peripheral
                    Interrupt ID

*********************************************************************/

static ADI_FLAG_RESULT adi_flag_MapFlagToInterrupt( // maps flag to interrupt
    ADI_FLAG_ID             FlagID,                     // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID,               // pointer to peripheral ID
    ADI_FLAG_INT_INFO       **pIntInfo,                 // pointer to where the address of the interrupt info will be stored
    u32                     *pBitToTouch,               // pointer to where the bit to control the flag will be stored
    u8                      *pByteIndex                 // pointer to where the byte index into the ASSIGN register will be stored
) {

    ADI_FLAG_INT_INFO   *pInterruptInfo;    // pointer to the interrupt info
    u32                 i, j;               // counters
    u32                 PINTxMask;          // mask of PINTx registers the flag can use
    u32                 PINTxByteMask;      // mask of bytes within the PINTx registers the flag can use
    u8                  Value;              // value within the byte of the PINTxASSIGN register for this flag
    u32                 FlagBit;            // bit for this flag
    u8                  BitToTouch;         // bit within the PINTxEDGE etc. registers that we need to touch
    ADI_FLAG_RESULT     Result;             // return code
    void                *pExitCriticalArg;  // parameter for exit critical

    // get the mask for the PINTx registers the flag can use
    PINTxMask = ADI_FLAG_GET_PINTX(FlagID);

    // get the mask for the bytes within the PINTx registers the flag can use
    PINTxByteMask = ADI_FLAG_GET_PINTXBYTE(FlagID);

    // get the bit for this flag
    FlagBit = ADI_FLAG_GET_BIT(FlagID);

    // get the value within the PINTxASSIGN register for this flag
    Value = ADI_FLAG_GET_VALUE(FlagID);

    // assume we can't map the flag to the peripheral ID
    Result = ADI_FLAG_RESULT_CANT_MAP_FLAG_TO_INTERRUPT;

    // FOR (each PINTxASSIGN register)
    for (i = 0, pInterruptInfo = InterruptInfo; i < (sizeof(InterruptInfo)/sizeof(InterruptInfo[0])); i++, PINTxMask >>= 1, pInterruptInfo++) {

        // IF (this is the peripheral ID that the caller wants)
        if (pInterruptInfo->PeripheralID == PeripheralID) {

            // IF (the flag can use this PINTx register)
            if (PINTxMask & 0x1) {

                // save the address of the interrupt info
                *pIntInfo = pInterruptInfo;

                // get the mask for the bytes within the PINTx registers the flag can use
                PINTxByteMask = ADI_FLAG_GET_PINTXBYTE(FlagID);

                // NOTE: in this first loop we try to piggy back onto a byte that's already active
                // FOR (each byte within the PINTxASSIGN register)
                for (j = 0; j < 4; j++, PINTxByteMask >>= 1) {

                    // IF (the flag can use this byte within the register)
                    if (PINTxByteMask & 0x1) {

                        // protect us
                        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_flag_InstanceData.pEnterCriticalArg);

                        // IF (the byte is active)
                        if (pInterruptInfo->UsageCount[j] != 0) {

                            // IF (this byte has the value this flag requires)
                            if (Value == ((pInterruptInfo->pBaseAddress->reg_ASSIGN >> (j<<3)) & 0xff)) {

                                // determine the bit for this flag within the PINTx_EDGE, MASK etc register
                                if (FlagBit < 8) {
                                    *pBitToTouch = 1 << ((j<<3) + FlagBit);
                                } else {
                                    *pBitToTouch = 1 << ((j<<3) + (FlagBit - 8));
                                }

                                // save the byte index
                                *pByteIndex = j;

                                // increment the usage counter
                                pInterruptInfo->UsageCount[j]++;

                                // indicate success
                                Result = ADI_FLAG_RESULT_SUCCESS;

                            // ENDIF
                            }

                        // ENDIF
                        }

                        // unprotect us
                        adi_int_ExitCriticalRegion(pExitCriticalArg);

                        // stop searching if we hooked ourselves in
                        if (Result == ADI_FLAG_RESULT_SUCCESS) {
                            break;
                        }

                    // ENDIF
                    }

                // ENDFOR
                }

                // IF (we couldn't piggy back onto a byte that was already active)
                if (Result != ADI_FLAG_RESULT_SUCCESS) {

                    // get the mask for the bytes within the PINTx registers the flag can use
                    PINTxByteMask = ADI_FLAG_GET_PINTXBYTE(FlagID);

                    // NOTE: in this second loop we try to find an inactive byte that we can use
                    // FOR (each byte within the PINTxASSIGN register)
                    for (j = 0; j < 4; j++, PINTxByteMask >>= 1) {

                        // IF (the flag can use this byte within the register)
                        if (PINTxByteMask & 0x1) {

                            // protect us
                            pExitCriticalArg = adi_int_EnterCriticalRegion(adi_flag_InstanceData.pEnterCriticalArg);

                            // IF (the byte is inactive)
                            if (pInterruptInfo->UsageCount[j] == 0) {

                                // determine the bit for this flag within the PINTx_EDGE, MASK etc register
                                if (FlagBit < 8) {
                                    *pBitToTouch = 1 << ((j<<3) + FlagBit);
                                } else {
                                    *pBitToTouch = 1 << ((j<<3) + (FlagBit - 8));
                                }

                                // save the byte index
                                *pByteIndex = j;

                                // set the value for this peripheral into the byte of the PINTxASSIGN register
                                pInterruptInfo->pBaseAddress->reg_ASSIGN  = (pInterruptInfo->pBaseAddress->reg_ASSIGN & ~(0xff << (j<<3))) | (Value << (j<<3));

                                // increment the usage counter
                                pInterruptInfo->UsageCount[j]++;

                                // indicate success
                                Result = ADI_FLAG_RESULT_SUCCESS;

                            // ENDIF
                            }

                            // unprotect us
                            adi_int_ExitCriticalRegion(pExitCriticalArg);

                            // stop searching if we hooked ourselves in
                            if (Result == ADI_FLAG_RESULT_SUCCESS) {
                                break;
                            }

                        // ENDIF
                        }

                    // ENDFOR
                    }

                // ENDIF
                }

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

    Function:       adi_flag_UnmapFlagFromInterrupt

    Description:    Unmaps the given FlagID from the given peripheral
                    Interrupt ID

*********************************************************************/

static ADI_FLAG_RESULT adi_flag_UnmapFlagFromInterrupt( // unmaps the given flag from the interrupt
    ADI_FLAG_ID             FlagID,                     // flag ID
    ADI_INT_PERIPHERAL_ID   PeripheralID                // pointer to peripheral ID
) {

    u8                      Value;              // value within the byte of the PINTxASSIGN register for this flag
    void                    *pExitCriticalArg;  // parameter for exit critical
    ADI_FLAG_RESULT         Result;             // return code
    ADI_FLAG_CALLBACK_ENTRY *pEntry;            // pointer to a flag entry

    // assume the worst
    Result = ADI_FLAG_RESULT_NOT_MAPPED_TO_INTERRUPT;

    // IF (we can get the callback entry)
    if ((pEntry = adi_flag_GetCallbackEntry(FlagID)) != NULL) {

        // get the value within the PINTxASSIGN register for this flag
        Value = ADI_FLAG_GET_VALUE(FlagID);

        // protect this region
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_flag_InstanceData.pEnterCriticalArg);

        // need to double-check to insure thread-safe operation
        // IF (the value is correct and the usage counter indicates the mapping is active)
        if ((Value == ((pEntry->pInterruptInfo->pBaseAddress->reg_ASSIGN >> (pEntry->ByteIndex<<3)) & 0xff)) &&
            (pEntry->pInterruptInfo->UsageCount[pEntry->ByteIndex] != 0)) {

            // decrement the usage counter
            pEntry->pInterruptInfo->UsageCount[pEntry->ByteIndex]--;

            // indicate success
            Result = ADI_FLAG_RESULT_SUCCESS;

        // ENDIF
        }

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ENDIF
    }

    // return
    return (Result);
}


#endif      // Moab



#if defined(ADI_SSL_DEBUG)

/*********************************************************************

    Function:       adi_flag_ValidateFlagID

    Description:    Tests the flag ID value for a legitimate value by
                    validating that the port index is less than or equal
                    to the number of ports and that the upper 16 bits of
                    the FlagID is equal to the bit number for the flag.

*********************************************************************/

static ADI_FLAG_RESULT adi_flag_ValidateFlagID(     // validates a flag ID
    ADI_FLAG_ID FlagID
) {

    ADI_FLAG_RESULT Result;     // return code
    u16             PortIndex;  // port index according to the FlagID
    u16             MaxBit=16;  // parameterize port bit
    // assume the worst
    Result = ADI_FLAG_RESULT_INVALID_FLAG_ID;

    // get the port index
    PortIndex = ADI_FLAG_GET_PORT(FlagID);

    // IF (the port index is valid)
    if (PortIndex < (sizeof(PortInfo)/sizeof(PortInfo[0]))) {

#if defined(__ADSP_BRODIE__)
        // Brodie only supports lower 8-bits of PortH...
        if (PortIndex+1 == sizeof(PortInfo)/sizeof(PortInfo[0]))
            MaxBit -= 8;
#endif

#if defined(__ADSP_MOY__)
        // Moy only supports lower 3-bits of PortH...
        if (PortIndex+1 == sizeof(PortInfo)/sizeof(PortInfo[0]))
            MaxBit -= 13;
#endif

        // IF (the bit is less than the max allowed)
        if (ADI_FLAG_GET_BIT(FlagID) < MaxBit) {

            // looks like the FlagID is valid
            Result = ADI_FLAG_RESULT_SUCCESS;

        // ENDIF
        }

    // ENDIF
    }

    // return
    return (Result);
}


#if defined(__ADSP_STIRLING__)      // Stirling has flags not capable of interrupts

/*********************************************************************

    Function:       adi_flag_ValidateInterruptCapableFlagID

    Description:    Tests the flag ID value to be sure it can support
                    interrupts.

*********************************************************************/
static ADI_FLAG_RESULT adi_flag_ValidateInterruptCapableFlagID(     // validates a flag ID is interrupt capable
    ADI_FLAG_ID FlagID
){

    ADI_FLAG_RESULT Result;     // return code

    // assume success
    Result = ADI_FLAG_RESULT_SUCCESS;

    // only port f supports interrupts so compare the port of the given flag to that of ADI_FLAG_PF0
    // which we know is on that port
    if (ADI_FLAG_GET_PORT(FlagID) != ADI_FLAG_GET_PORT(ADI_FLAG_PF0)) {
        Result = ADI_FLAG_RESULT_NOT_CAPABLE;
    }

    // return
    return (Result);
}

#endif  // Stirling





#endif  // ADI_SSL_DEBUG


