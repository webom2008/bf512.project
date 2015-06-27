/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_tmr.c,v $
$Revision: 3916 $
$Date: 2010-12-06 11:39:55 -0500 (Mon, 06 Dec 2010) $

Description:
            This is the source code for the timer services.

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/
#include <services\services.h>      // system service includes


/*********************************************************************

Data structure to define a GP config register

*********************************************************************/

#pragma pack(2)                             // pack on 16 bit boundaries

typedef struct ADI_TMR_CONFIG_REG {
    volatile u16 b_TMODE:       2;  //0:1   timer mode
    volatile u16 b_PULSE_HI:    1;  //2     pulse_hi
    volatile u16 b_PERIOD_CNT:  1;  //3     period count
    volatile u16 b_IRQ_ENA:     1;  //4     interrupt request enable
    volatile u16 b_TIN_SEL:     1;  //5     timer input select
    volatile u16 b_OUT_DIS:     1;  //6     output pad disable
    volatile u16 b_CLK_SEL:     1;  //7     timer clock select
    volatile u16 b_TOGGLE_HI:   1;  //8     toggle hi
    volatile u16 b_EMU_RUN:     1;  //9     emulation behavior
    volatile u16 b_RESERVED:    4;  //10:13 reserved
    volatile u16 b_ERR_TYP:     2;  //13:15 error type
} ADI_TMR_CONFIG_REG;

#pragma pack()

/*********************************************************************

Data structures to define information for a core timer

*********************************************************************/

typedef struct {            // structure representing the core timer registers in memory
    volatile u32    Cntl;       // control register
    volatile u32    Period;     // period register
    volatile u32    Scale;      // scale register
    volatile u32    Count;      // count register
} ADI_TMR_CORE_REGISTERS;

/*********************************************************************

Data structures to define information for a watchdog timer

*********************************************************************/

typedef struct {            // structure representing the watchdog timer registers in memory
    volatile u16    Ctl;        // control register
    u16             Padding0;
    volatile u32    Cnt;        // count register
    volatile u32    Stat;       // status register
} ADI_TMR_WDOG_REGISTERS;

/*********************************************************************

Data structures to define information for a general purpose timer

*********************************************************************/

typedef struct {            // structure representing the GP timer registers in memory
    volatile u16    Config;     // config register
    u16             Padding0;
    volatile u32    Counter;    // counter register
    volatile u32    Period;     // period register
    volatile u32    Width;      // width register
} ADI_TMR_GP_REGISTERS;

typedef struct {            // structure representing group registers in memory
#if defined(__ADSP_EDINBURGH__)                 // Edinburgh class devices
    volatile u16    Enable;     // enable register
    u16             Padding0;
    volatile u16    Disable;    // disable register
    u16             Padding1;
    volatile u16    Status;     // status register
    u16             Padding2;
#endif
#if defined(__ADSP_BRAEMAR__)   // Braemar class devices
    volatile u16    Enable;     // enable register
    u16             Padding0;
    volatile u16    Disable;    // disable register
    u16             Padding1;
    volatile u32    Status;     // status register
#endif
/* BF52x, BF51x & BF50x class devices */
#if defined(__ADSP_KOOKABURRA__)  ||\
    defined(__ADSP_MOCKINGBIRD__) ||\
    defined(__ADSP_BRODIE__)      ||\
    defined(__ADSP_MOY__)         ||\
    defined(__ADSP_DELTA__)
    volatile u16    Enable;      // enable register
    u16             Padding0;
    volatile u16    Disable;     // disable register
    u16             Padding1;
    volatile u32    Status;      // status register
#endif
#if defined(__ADSP_TETON__)                     // Teton class devices
    volatile u32    Enable;     // enable register
    volatile u32    Disable;    // disable register
    volatile u32    Status;     // status register
#endif
#if defined(__ADSP_STIRLING__)                  // Stirling class devices
    volatile u16    Enable;     // enable register
    u16             Padding0;
    volatile u16    Disable;    // disable register
    u16             Padding1;
    volatile u16    Status;     // status register
    u16             Padding2;
#endif
#if defined(__ADSP_MOAB__)                      // Moab class devices
    volatile u16    Enable;     // enable register
    u16             Padding0;
    volatile u16    Disable;    // disable register
    u16             Padding1;
    volatile u32    Status;     // status register
#endif
} ADI_TMR_GROUP_REGISTERS;



/*********************************************************************

Data structures to define information for a timer

*********************************************************************/

typedef struct {                            // timer info
    union {
        ADI_TMR_CORE_REGISTERS  *Core;          // address of core registers in memory
        ADI_TMR_WDOG_REGISTERS  *Watchdog;      // address of watchdog registers in memory
        ADI_TMR_GP_REGISTERS    *GP;            // address of gp registers in memory
    } Registers;
    ADI_TMR_GROUP_REGISTERS *pGroupRegisters;   // address of group registers in memory
    ADI_INT_PERIPHERAL_ID   PeripheralID;       // peripheral ID for the timer
    ADI_DCB_HANDLE          DCBHandle;          // DCB handle for callbacks
    ADI_DCB_CALLBACK_FN     ClientCallback;     // client callback function for callbacks
    void                    *ClientHandle;      // client handle for callbacks
    u8                      GroupBit;           // bit in Enable/Disable register to enable/disable the timer
    u8                      IRQBit;             // bit in Status register to indicate IRQ
    u8                      ErrorBit;           // bit in Status register to indicate an error
    u8                      SlaveBit;           // bit in Status register to indicate slave status
    u32                     EnableDelay_Count;  // Number of System clocks before this timer can be enabled
    u32                     EnableDelay_Clocks; // Number of delays processed so far
} ADI_TMR_INFO;


/*********************************************************************

Processor specific data

*********************************************************************/

/********************
    Edinburgh
********************/

#if defined(__ADSP_EDINBURGH__) // gp timer info for Edinburgh class devices

static ADI_TMR_INFO TimerInfo[] = {     // timer info
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00600,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  4,  12  },  // timer 0
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00610,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER1,     NULL,   NULL,   NULL,   1,  1,  5,  13  },  // timer 1
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00620,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER2,     NULL,   NULL,   NULL,   2,  2,  6,  14  },  // timer 2
    {   (ADI_TMR_CORE_REGISTERS *)0xffe03000,   NULL,                                   ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  0,  0   },  // core timer
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00200,   NULL,                                   ADI_INT_WATCHDOG,   NULL,   NULL,   NULL,   0,  0,  0,  0   },  // watchdog timer
};

static ADI_TMR_GROUP_REGISTERS *GroupRegisters[] = {    // group registers
    (ADI_TMR_GROUP_REGISTERS *)0xffc00640,                  // group 1 (documentation convention of first is 1, not 0)
};

#endif


/********************
    Baemar
********************/

#if defined(__ADSP_BRAEMAR__) // gp timer info for Braemar class devices

static ADI_TMR_INFO TimerInfo[] = {     // timer info
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00600,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  4,  12  },  // timer 0
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00610,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER1,     NULL,   NULL,   NULL,   1,  1,  5,  13  },  // timer 1
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00620,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER2,     NULL,   NULL,   NULL,   2,  2,  6,  14  },  // timer 2
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00630,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER3,     NULL,   NULL,   NULL,   3,  3,  7,  15  },  // timer 3
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00640,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER4,     NULL,   NULL,   NULL,   4,  16, 20, 28  },  // timer 4
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00650,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER5,     NULL,   NULL,   NULL,   5,  17, 21, 29  },  // timer 5
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00660,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER6,     NULL,   NULL,   NULL,   6,  18, 22, 30  },  // timer 6
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00670,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER7,     NULL,   NULL,   NULL,   7,  19, 23, 31  },  // timer 7
    {   (ADI_TMR_CORE_REGISTERS *)0xffe03000,   NULL,                                   ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  0,  0   },  // core timer
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00200,   NULL,                                   ADI_INT_WATCHDOG,   NULL,   NULL,   NULL,   0,  0,  0,  0   },  // watchdog timer
};

static ADI_TMR_GROUP_REGISTERS *GroupRegisters[] = {    // group registers
    (ADI_TMR_GROUP_REGISTERS *)0xffc00680,                  // group 1 (documentation convention of first is 1, not 0)
};

#endif

/*************************************
    Kookaburra/Mockingbird/Brodie/Moy
*************************************/

/* GP timer info for BF52x, BF51x and BF50x class devices */
#if defined(__ADSP_KOOKABURRA__)  ||\
    defined(__ADSP_MOCKINGBIRD__) ||\
    defined(__ADSP_BRODIE__)      ||\
    defined(__ADSP_MOY__)

static ADI_TMR_INFO TimerInfo[] = {     // timer info
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00600,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  4,  12  },  // timer 0
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00610,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER1,     NULL,   NULL,   NULL,   1,  1,  5,  13  },  // timer 1
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00620,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER2,     NULL,   NULL,   NULL,   2,  2,  6,  14  },  // timer 2
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00630,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER3,     NULL,   NULL,   NULL,   3,  3,  7,  15  },  // timer 3
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00640,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER4,     NULL,   NULL,   NULL,   4,  16, 20, 28  },  // timer 4
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00650,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER5,     NULL,   NULL,   NULL,   5,  17, 21, 29  },  // timer 5
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00660,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER6,     NULL,   NULL,   NULL,   6,  18, 22, 30  },  // timer 6
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00670,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER7,     NULL,   NULL,   NULL,   7,  19, 23, 31  },  // timer 7
    {   (ADI_TMR_CORE_REGISTERS *)0xffe03000,   NULL,                                   ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  0,  0   },  // core timer
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00200,   NULL,                                   ADI_INT_WATCHDOG,   NULL,   NULL,   NULL,   0,  0,  0,  0   },  // watchdog timer
};

static ADI_TMR_GROUP_REGISTERS *GroupRegisters[] = {    // group registers
    (ADI_TMR_GROUP_REGISTERS *)0xffc00680,                  // group 1 (documentation convention of first is 1, not 0)
};

#endif
/*************************************
    Delta
*************************************/

/* GP timer info for BF59x class devices */
#if defined(__ADSP_DELTA__)

static ADI_TMR_INFO TimerInfo[] = {     // timer info
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00600,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  4,  12  },  // timer 0
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00610,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER1,     NULL,   NULL,   NULL,   1,  1,  5,  13  },  // timer 1
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00620,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER2,     NULL,   NULL,   NULL,   2,  2,  6,  14  },  // timer 2
    {   (ADI_TMR_CORE_REGISTERS *)0xffe03000,   NULL,                                   ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  0,  0   },  // core timer
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00200,   NULL,                                   ADI_INT_WATCHDOG,   NULL,   NULL,   NULL,   0,  0,  0,  0   },  // watchdog timer
};

static ADI_TMR_GROUP_REGISTERS *GroupRegisters[] = {    // group registers
    (ADI_TMR_GROUP_REGISTERS *)0xffc00640,              // group 1 (documentation convention of first is 1, not 0)
};

#endif

/********************
    Teton
********************/

#if defined(__ADSP_TETON__) // gp timer info for Teton class devices

static ADI_TMR_INFO TimerInfo[] = {     // timer info
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00600,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  4,  12  },  // timer 0
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00610,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER1,     NULL,   NULL,   NULL,   1,  1,  5,  13  },  // timer 1
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00620,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER2,     NULL,   NULL,   NULL,   2,  2,  6,  14  },  // timer 2
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00630,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER3,     NULL,   NULL,   NULL,   3,  3,  7,  15  },  // timer 3
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00640,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER4,     NULL,   NULL,   NULL,   4,  16, 20, 28  },  // timer 4
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00650,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER5,     NULL,   NULL,   NULL,   5,  17, 21, 29  },  // timer 5
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00660,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER6,     NULL,   NULL,   NULL,   6,  18, 22, 30  },  // timer 6
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00670,   (ADI_TMR_GROUP_REGISTERS *)0xffc00680,  ADI_INT_TIMER7,     NULL,   NULL,   NULL,   7,  19, 23, 31  },  // timer 7
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01600,   (ADI_TMR_GROUP_REGISTERS *)0xffc01640,  ADI_INT_TIMER8,     NULL,   NULL,   NULL,   0,  0,  4,  12  },  // timer 8
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01610,   (ADI_TMR_GROUP_REGISTERS *)0xffc01640,  ADI_INT_TIMER9,     NULL,   NULL,   NULL,   1,  1,  5,  13  },  // timer 9
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01620,   (ADI_TMR_GROUP_REGISTERS *)0xffc01640,  ADI_INT_TIMER10,    NULL,   NULL,   NULL,   2,  2,  6,  14  },  // timer 10
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01630,   (ADI_TMR_GROUP_REGISTERS *)0xffc01640,  ADI_INT_TIMER11,    NULL,   NULL,   NULL,   3,  3,  7,  15  },  // timer 11
    {   (ADI_TMR_CORE_REGISTERS *)0xffe03000,   NULL,                                   ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  0,  0   },  // core timer
    {   (ADI_TMR_CORE_REGISTERS *)NULL,         NULL,                                   ADI_INT_WATCHDOG,   NULL,   NULL,   NULL,   0,  0,  0,  0   },  // watchdog timer
};

static ADI_TMR_GROUP_REGISTERS *GroupRegisters[] = {    // group registers
    (ADI_TMR_GROUP_REGISTERS *)0xffc00680,                  // group 1 (documentation convention of first is 1, not 0)
    (ADI_TMR_GROUP_REGISTERS *)0xffc01640,                  // group 2 (documentation convention of first is 1, not 0)
};

#endif


/********************
    Stirling
********************/

#if defined(__ADSP_STIRLING__) // gp timer info for Stirling class devices

static ADI_TMR_INFO TimerInfo[] = {     // timer info
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00600,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  4,  12  },  // timer 0
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00610,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER1,     NULL,   NULL,   NULL,   1,  1,  5,  13  },  // timer 1
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00620,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER2,     NULL,   NULL,   NULL,   2,  2,  6,  14  },  // timer 2
    {   (ADI_TMR_CORE_REGISTERS *)0xffe03000,   NULL,                                   ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  0,  0   },  // core timer
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00200,   NULL,                                   ADI_INT_WATCHDOG,   NULL,   NULL,   NULL,   0,  0,  0,  0   },  // watchdog timer
};

static ADI_TMR_GROUP_REGISTERS *GroupRegisters[] = {    // group registers
    (ADI_TMR_GROUP_REGISTERS *)0xffc00640,                  // group 1 (documentation convention of first is 1, not 0)
};

#endif


/********************
    Moab
********************/
#if defined(__ADSP_MOAB__) // gp timer info for Moab class devices

static ADI_TMR_INFO TimerInfo[] = {     // timer info
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01600,   (ADI_TMR_GROUP_REGISTERS *)0xffc01680,  ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  4,  12  },  // timer 0
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01610,   (ADI_TMR_GROUP_REGISTERS *)0xffc01680,  ADI_INT_TIMER1,     NULL,   NULL,   NULL,   1,  1,  5,  13  },  // timer 1
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01620,   (ADI_TMR_GROUP_REGISTERS *)0xffc01680,  ADI_INT_TIMER2,     NULL,   NULL,   NULL,   2,  2,  6,  14  },  // timer 2
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01630,   (ADI_TMR_GROUP_REGISTERS *)0xffc01680,  ADI_INT_TIMER3,     NULL,   NULL,   NULL,   3,  3,  7,  15  },  // timer 3
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01640,   (ADI_TMR_GROUP_REGISTERS *)0xffc01680,  ADI_INT_TIMER4,     NULL,   NULL,   NULL,   4,  16, 20, 28  },  // timer 4
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01650,   (ADI_TMR_GROUP_REGISTERS *)0xffc01680,  ADI_INT_TIMER5,     NULL,   NULL,   NULL,   5,  17, 21, 29  },  // timer 5
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01660,   (ADI_TMR_GROUP_REGISTERS *)0xffc01680,  ADI_INT_TIMER6,     NULL,   NULL,   NULL,   6,  18, 22, 30  },  // timer 6
    {   (ADI_TMR_CORE_REGISTERS *)0xffc01670,   (ADI_TMR_GROUP_REGISTERS *)0xffc01680,  ADI_INT_TIMER7,     NULL,   NULL,   NULL,   7,  19, 23, 31  },  // timer 7
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00600,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER8,     NULL,   NULL,   NULL,   0,  0,  4,  12  },  // timer 8
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00610,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER9,     NULL,   NULL,   NULL,   1,  1,  5,  13  },  // timer 9
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00620,   (ADI_TMR_GROUP_REGISTERS *)0xffc00640,  ADI_INT_TIMER10,    NULL,   NULL,   NULL,   2,  2,  6,  14  },  // timer 10
    {   (ADI_TMR_CORE_REGISTERS *)0xffe03000,   NULL,                                   ADI_INT_TIMER0,     NULL,   NULL,   NULL,   0,  0,  0,  0   },  // core timer
    {   (ADI_TMR_CORE_REGISTERS *)0xffc00200,   NULL,                                   ADI_INT_WATCHDOG,   NULL,   NULL,   NULL,   0,  0,  0,  0   },  // watchdog timer
};

static ADI_TMR_GROUP_REGISTERS *GroupRegisters[] = {    // group registers
    (ADI_TMR_GROUP_REGISTERS *)0xffc01680,                  // group 1 (documentation convention of first is 1, not 0)
    (ADI_TMR_GROUP_REGISTERS *)0xffc00640,                  // group 2
};

#endif  // end of all Moabs

#define ADI_TMR_GP_GROUP_COUNT (sizeof(GroupRegisters)/sizeof(ADI_TMR_GP_REGISTERS *))      // number of groups


/*********************************************************************

Tables for individual control/status register bit field settings

*********************************************************************/

typedef struct {        // structure to access bit fields in the configuration register
    u16 Mask;               // mask to access the field
    u16 StartBit;           // starting bit of the field within the register
} ADI_TMR_CONFIG_REGISTER_FIELDS;


// the information in table below must match exactly the order of the command IDs in
// adi_tmr.h.  These tables are indexed with the command ID so any changes in either the
// command IDs or the table below must be reflected in the other.

static ADI_TMR_CONFIG_REGISTER_FIELDS GPConfigRegisterFields[] = {
    {   0x3,    0   },  // TMODE
    {   0x1,    2   },  // PULSE_HI
    {   0x1,    3   },  // Period_CNT
    {   0x1,    4   },  // IRQ_ENA
    {   0x1,    5   },  // TIN_SEL
    {   0x1,    6   },  // OUT_DIS
    {   0x1,    7   },  // CLK_SEL
    {   0x1,    8   },  // TOGGLE_HI
    {   0x1,    9   },  // EMU_RUN
    {   0x3,    14  },  // ERR_TYPE
};

static ADI_TMR_CONFIG_REGISTER_FIELDS CoreConfigRegisterFields[] = {
    {   0x1,    0   },  // TMPWR
    {   0x1,    1   },  // TMREN
    {   0x1,    2   },  // TAUTORLD
    {   0x1,    3   },  // TINT
};

static ADI_TMR_CONFIG_REGISTER_FIELDS WatchdogConfigRegisterFields[] = {
    {   0x3,    1   },  // ICTL
    {   0xff,   4   },  // TMR_EN
    {   0x1,    15  },  // TRO
};



/*********************************************************************

Critical Region Data

*********************************************************************/

static void *adi_tmr_pCriticalRegionArg;


/*********************************************************************

Static functions

*********************************************************************/

static ADI_TMR_RESULT adi_tmr_Hook(     // hooks an interrupt handler for a timer
    u32                     TimerID,        // argument supplied to handler (TimerID)
    ADI_INT_PERIPHERAL_ID   PeripheralID,   // peripheral ID
    u32                     NestingFlag,    // interrupt nesting enable flag (TRUE/FALSE)
    u32                     WakeupFlag      // wakeup flag (TRUE/FALSE)
);

static ADI_TMR_RESULT adi_tmr_Unhook(   // unhooks an interrupt handler for a timer
    u32                     TimerID,        // argument supplied to handler (TimerID)
    ADI_INT_PERIPHERAL_ID   PeripheralID    // peripheral ID
);


static ADI_INT_HANDLER(TimerHandler);   // timer handler

#ifdef ADI_SSL_DEBUG

static ADI_TMR_RESULT adi_tmr_ValidateTimerID(  // validates any timer ID
    u32 TimerID
);

static ADI_TMR_RESULT adi_tmr_ValidateGPTimerID(    // validates a GP timer ID
    u32 TimerID
);

static ADI_TMR_RESULT adi_tmr_ValidateGPTimerIDs(   // validates a group of timer IDs
    u32 TimerIDs
);

static ADI_TMR_RESULT adi_tmr_ValidateCoreTimerID(  // validates a core timer ID
    u32 TimerID
);

static ADI_TMR_RESULT adi_tmr_ValidateWatchdogTimerID(  // validates a watchdog timer ID
    u32 TimerID
);
#endif

static void adi_tmr_PortControl(
    u32 TimerIDs                            // OR-ing of TimerIDs being enabled
);



/*********************************************************************

    Function:       adi_tmr_Init

    Description:    Initializes the timer services.

*********************************************************************/

ADI_TMR_RESULT adi_tmr_Init(        // Initializes the timer services
    void    *pCriticalRegionArg         // parameter for critical region function
) {


    // debug
#if defined(ADI_SSL_DEBUG)
#endif

    // save the critical region parameter
    adi_tmr_pCriticalRegionArg = pCriticalRegionArg;

    // must initialize watchdog timer address explicitly at run time on Teton class devices
#if defined(__ADSP_TETON__)
    TimerInfo[ADI_TMR_GET_TIMER_INDEX(ADI_TMR_WDOG_TIMER)].Registers.Watchdog = (ADI_TMR_WDOG_REGISTERS *)((*ADI_SRAM_BASE_ADDR_MMR == ADI_COREA)?(0xffc00200):(0xffc01200));
#endif

    // return
    return (ADI_TMR_RESULT_SUCCESS);
}

/*********************************************************************

    Function:       adi_tmr_Terminate

    Description:    Shuts down the timer services.

*********************************************************************/

ADI_TMR_RESULT adi_tmr_Terminate(
    void
) {

    ADI_TMR_RESULT  Result;     // result
    int             i;          // counter

    // debug
#if defined(ADI_SSL_DEBUG)
#endif

    // uninstall callbacks for all GP timers
    for (i = 0; i < ADI_TMR_GP_TIMER_COUNT; i++) {
        if ((Result = adi_tmr_RemoveCallback(ADI_TMR_CREATE_GP_TIMER_ID(i))) != ADI_TMR_RESULT_SUCCESS) {
            return (Result);
        }
    }

    // uninstall core timer callback
    if ((Result = adi_tmr_RemoveCallback(ADI_TMR_CORE_TIMER)) != ADI_TMR_RESULT_SUCCESS) {
        return (Result);
    }

    // uninstall watchdog timer callback
    Result = adi_tmr_RemoveCallback(ADI_TMR_WDOG_TIMER);

    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_tmr_Open

    Description:    Opens a general purpose, core or watchdog timer for use

*********************************************************************/

ADI_TMR_RESULT adi_tmr_Open(                // opens a general purpose, core or watchdog timer for use
    u32 TimerID                                 // timer ID
){

    ADI_TMR_RESULT  Result;                 // result

    // debug
#if defined(ADI_SSL_DEBUG)
    if ((Result = adi_tmr_ValidateTimerID(TimerID)) != ADI_TMR_RESULT_SUCCESS) {
        return (Result);
    }
#endif

    // return
    return (adi_tmr_Reset(TimerID));
}



/*********************************************************************

    Function:       adi_tmr_Close

    Description:    Closes a general purpose, core or watchdog timer from further use

*********************************************************************/

ADI_TMR_RESULT adi_tmr_Close(               // closes a general purpose, core or watchdog timer from further use
    u32 TimerID                                 // timer ID
){

    ADI_TMR_RESULT  Result;                 // result

    // debug
#if defined(ADI_SSL_DEBUG)
    if ((Result = adi_tmr_ValidateTimerID(TimerID)) != ADI_TMR_RESULT_SUCCESS) {
        return (Result);
    }
#endif

    // return
    return (adi_tmr_Reset(TimerID));
}



/*********************************************************************

    Function:       adi_tmr_InstallCallback

    Description:    Installs a callback for a general purpose, core or watchdog timer

*********************************************************************/

ADI_TMR_RESULT adi_tmr_InstallCallback( // installs a callback for a general purpose, core or watchdog timer
    u32                 TimerID,                // timer ID
    u32                 WakeupFlag,             // wakeup flag (TRUE/FALSE)
    void                *ClientHandle,          // client handle argument passed in callbacks
    ADI_DCB_HANDLE      DCBHandle,              // deferred callback service handle
    ADI_DCB_CALLBACK_FN ClientCallback          // client callback function
) {

    ADI_TMR_INFO    *pInfo;                 // pointer to a timer's info
    ADI_TMR_RESULT  Result;                 // result
    void            *pExitCriticalArg;      // return value from EnterCritical

    // debug
#if defined(ADI_SSL_DEBUG)
    if ((Result = adi_tmr_ValidateTimerID(TimerID)) != ADI_TMR_RESULT_SUCCESS) {
        return (Result);
    }
#endif

    // assume success
    Result = ADI_TMR_RESULT_SUCCESS;

    // get the info for the timer
    pInfo = &TimerInfo[ADI_TMR_GET_TIMER_INDEX(TimerID)];

    // protect this region
    pExitCriticalArg = adi_int_EnterCriticalRegion(adi_tmr_pCriticalRegionArg);

    // IF (there isn't a callback already there)
    if (pInfo->ClientCallback == NULL) {

        // save the callback information
        pInfo->ClientCallback = ClientCallback;
        pInfo->ClientHandle = ClientHandle;
        pInfo->DCBHandle = DCBHandle;

    // ELSE
    } else {

        // we only support a single callback
        Result = ADI_TMR_RESULT_CALLBACK_ALREADY_INSTALLED;

    // ENDIF
    }

    // unprotect us
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // IF (we have no error)
    if (Result == ADI_TMR_RESULT_SUCCESS) {

        // install the interrupt handler for the timer
        Result = adi_tmr_Hook(TimerID, pInfo->PeripheralID, TRUE, WakeupFlag);

    // ENDIF
    }

    // return
    return (Result);
}



/*********************************************************************

    Function:       adi_tmr_RemoveCallback

    Description:    Removes a callback for a general purpose, core or watchdog timer

*********************************************************************/

ADI_TMR_RESULT adi_tmr_RemoveCallback(  // removes a callback for a general purpose, core or watchdog timer
    u32 TimerID                             // timer ID
) {

    ADI_TMR_INFO    *pInfo;                 // pointer to a timer's info
    ADI_TMR_RESULT  Result;                 // result

    // debug
#if defined(ADI_SSL_DEBUG)
    if ((Result = adi_tmr_ValidateTimerID(TimerID)) != ADI_TMR_RESULT_SUCCESS) {
        return (Result);
    }
#endif

    // assume success
    Result = ADI_TMR_RESULT_SUCCESS;

    // get the info for the timer
    pInfo = &TimerInfo[ADI_TMR_GET_TIMER_INDEX(TimerID)];

    // unhook it if it exists and mark it as free
    if (pInfo->ClientCallback) {
        Result = adi_tmr_Unhook(TimerID, pInfo->PeripheralID);
        pInfo->ClientCallback = NULL;
    }

    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_tmr_Reset

    Description:    Resets a core, watchdog or general purpose timer to it's power up values

*********************************************************************/

ADI_TMR_RESULT adi_tmr_Reset(   // resets any timer to reset values
    u32 TimerID                     // timer ID
){

    ADI_TMR_RESULT  Result;     // return code
    ADI_TMR_INFO    *pInfo;     // pointer to timer's info

    // debug
#if defined(ADI_SSL_DEBUG)
    if ((Result = adi_tmr_ValidateTimerID(TimerID)) != ADI_TMR_RESULT_SUCCESS) {
        return (Result);
    }
#endif

    // get the info for the timer
    pInfo = &TimerInfo[ADI_TMR_GET_TIMER_INDEX(TimerID)];

    // CASEOF (timer type)
    switch (TimerID) {

        // case (core timer)
        case ADI_TMR_CORE_TIMER:

            // reset it
            pInfo->Registers.Core->Cntl = 0x8;
            break;

        // case (watchdog timer)
        case ADI_TMR_WDOG_TIMER:

            // disable the timer
            adi_tmr_WatchdogControl(ADI_TMR_WDOG_CMD_ENABLE_TIMER, (void *)FALSE);

            // clear the indication that it expired
            adi_tmr_WatchdogControl(ADI_TMR_WDOG_CMD_RESET_EXPIRED, NULL);

            // reset count to 0
            adi_tmr_WatchdogControl(ADI_TMR_WDOG_CMD_SET_COUNT, (void *)0);

            // set event to reset
            adi_tmr_WatchdogControl(ADI_TMR_WDOG_CMD_EVENT_SELECT, (void *)0);
            break;

        // case (GP timer)
        default:

            // disable the timer
            adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_ENABLE_TIMER, (void *)FALSE);
            adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_IMMEDIATE_HALT, NULL);

            // clear the timer's interrupt request
            adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_CLEAR_INTERRUPT, NULL);

            // clear the timer's error status
            adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_CLEAR_ERROR, NULL);

            // set the timer config register to 0x0000, period and width to 0
            pInfo->Registers.GP->Config = 0;
            pInfo->Registers.GP->Period = 0;
            pInfo->Registers.GP->Width = 0;
            break;

    // ENDCASE
    }

    // return
    return (ADI_TMR_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       adi_tmr_GPGetPeripheralID

    Description:    Provides the peripheral ID for the timer
                    (Note this function was inadvertantly included in
                    some 4.5 releases rather than the proper function
                    adi_tmr_GetPeripheralID().  It is left here for
                    to make sure we don't break existing code but it
                    is not described in the .h file and should be
                    removed from future releases.  Perhaps at the
                    first 5.0 update.

*********************************************************************/

ADI_TMR_RESULT adi_tmr_GPGetPeripheralID(   // gets the interrupt peripheral ID for a general purpose timer
    u32                     TimerID,            // timer ID
    ADI_INT_PERIPHERAL_ID   *pPeripheralID      // location where peripheral ID of timer will be stored
) {

    // return
    return (adi_tmr_GetPeripheralID(TimerID, pPeripheralID));
}



/*********************************************************************

    Function:       adi_tmr_GetPeripheralID

    Description:    Provides the peripheral ID for a GP or Watchdog timer

*********************************************************************/

ADI_TMR_RESULT adi_tmr_GetPeripheralID( // gets the interrupt peripheral ID for a general purpose timer
    u32                     TimerID,            // timer ID
    ADI_INT_PERIPHERAL_ID   *pPeripheralID      // location where peripheral ID of timer will be stored
) {

    // debug
#if defined(ADI_SSL_DEBUG)
    // flag it if trying to get the peripheral ID for a core timer
    if (adi_tmr_ValidateCoreTimerID(TimerID) == ADI_TMR_RESULT_SUCCESS) {
        return (ADI_TMR_RESULT_BAD_TIMER_TYPE);
    }
#endif

    // save it in the caller's spot
    *pPeripheralID = TimerInfo[ADI_TMR_GET_TIMER_INDEX(TimerID)].PeripheralID;

    // return
    return (ADI_TMR_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       adi_tmr_GPGroupEnable

    Description:    Enables/disables a group of timers simultaneously

*********************************************************************/
ADI_TMR_RESULT adi_tmr_GPGroupEnable(   // simultaneously enable/disable a group of registers
    u32 TimerIDs,                           // OR-ing of TimerIDs to enable/disable
    u32 EnableFlag                          // enable/disable flag (TRUE - enable, FALSE - disable)
){

    ADI_TMR_RESULT  Result;                             // return code
    u16             GroupValue[ADI_TMR_GP_GROUP_COUNT]; // group registers
    u16             GroupFlag[ADI_TMR_GP_GROUP_COUNT];  // flag for group
    ADI_TMR_INFO    *pInfo;                             // pointer to a timer's info
    u32             GroupID;                            // group ID
    int             i;                                  // counter
    u32             tmp_TimerIDs;                       // save TimerIDs for subsequent processing

    // debug
#if defined(ADI_SSL_DEBUG)
    if ((Result = adi_tmr_ValidateGPTimerIDs(TimerIDs)) != ADI_TMR_RESULT_SUCCESS) {
        return (Result);
    }
#endif

    // initialize the group registers
    for (i = 0; i < ADI_TMR_GP_GROUP_COUNT; i++) {
        GroupValue[i] = 0;
        GroupFlag[i] = FALSE;
    }

    if (EnableFlag) {
        adi_tmr_PortControl(TimerIDs);
    }

    // Some timers must be delayed before enabling
    // For example, the Frame Synch on the Kookaburra EzKit's LCD must assert 2 PPI clocks
    // before the Horizontal Synch (i.e. - 2 clocks of blanking data before the horizontal synch asserts
    u32 enableDelays = 0;
    tmp_TimerIDs = TimerIDs;

    // FOR (each gp timer)
    for (i = 0, pInfo = TimerInfo; i < ADI_TMR_GP_TIMER_COUNT; i++, pInfo++, TimerIDs >>= 1) {

        // IF (this timer is being enabled or disabled)
        if (TimerIDs & 0x1) {

            if( pInfo->EnableDelay_Count ) {
                enableDelays = 1;
            }

            // determine the group ID for this timer
            for (GroupID = 0; GroupID < ADI_TMR_GP_GROUP_COUNT; GroupID++) {
                if (pInfo->pGroupRegisters == GroupRegisters[GroupID]) break;
            }

            // update the group's flag and register value
            GroupFlag[GroupID] = TRUE;
            GroupValue[GroupID] |= (0x1 << pInfo->GroupBit);

        // ENDIF
        }

    // ENDFOR
    }


    if(enableDelays && EnableFlag) {

        while( enableDelays ) {
            TimerIDs = tmp_TimerIDs;
            enableDelays = false;
            for (i = 0, pInfo = TimerInfo; i < ADI_TMR_GP_TIMER_COUNT; i++, pInfo++, TimerIDs >>= 1) {

                // IF (this timer is being enabled )
                if (TimerIDs & 0x1) {

                    if( pInfo->EnableDelay_Count == 0 || (pInfo->EnableDelay_Count == pInfo->EnableDelay_Clocks)) {
                        pInfo->pGroupRegisters->Enable |= (0x1 << pInfo->GroupBit);
                        ssync();
                    } else {
                        pInfo->EnableDelay_Clocks++;
                        enableDelays = true;
                    }

                // ENDIF
                }
            }
        }


    } else {
        // affect the changes to the appropriate enable and disable registers
        for (i = 0; i < ADI_TMR_GP_GROUP_COUNT; i++) {
            if (GroupFlag[i] == TRUE) {
                if (EnableFlag) {
                    GroupRegisters[i]->Enable = GroupValue[i];
                } else {
                    GroupRegisters[i]->Disable = GroupValue[i];
                }
            }
        }
    }

    // return
    return (ADI_TMR_RESULT_SUCCESS);
}




/*********************************************************************

    Function:       adi_tmr_GPControl

    Description:    Controls a general purpose timer

*********************************************************************/

ADI_TMR_RESULT adi_tmr_GPControl(   // controls a general purpose timer
    u32             TimerID,            // timer ID
    ADI_TMR_GP_CMD  Command,            // command ID
    void            *Value              // command specific value
) {

    ADI_TMR_RESULT                  Result;             // return code
    ADI_TMR_INFO                    *pInfo;             // pointer to timer's info
    ADI_TMR_GP_CMD_VALUE_PAIR       *pPair;             // pointer to command pair
    u32                             u32Value;           // u32 type to avoid casts/warnings etc.
    ADI_TMR_CONFIG_REGISTER_FIELDS  *pFields;           // pointer into fields array
    u16                             tmp16;              // temporary storage
    ADI_TMR_GP_REGISTERS            *pRegs;             // pointer to GP registers
    void                            *pExitCriticalArg;  // return value from EnterCritical

    // debug
#if defined(ADI_SSL_DEBUG)
    if ((Result = adi_tmr_ValidateGPTimerID(TimerID)) != ADI_TMR_RESULT_SUCCESS) {
        return (Result);
    }
#endif

    // avoid casts and assume success
    u32Value = (u32)Value;
    Result = ADI_TMR_RESULT_SUCCESS;

    // get the info for the timer
    pInfo = &TimerInfo[ADI_TMR_GET_TIMER_INDEX(TimerID)];
    pRegs = pInfo->Registers.GP;

    // CASEOF (Command ID)
    switch (Command) {

    // CASE (command table)
    case ADI_TMR_GP_CMD_TABLE:

        // avoid casts
        pPair = (ADI_TMR_GP_CMD_VALUE_PAIR *)Value;

        // process each command pair
        while (pPair->CommandID != ADI_TMR_GP_CMD_END) {
            if ((Result = adi_tmr_GPControl(TimerID, pPair->CommandID, pPair->Value)) != ADI_TMR_RESULT_SUCCESS) return(Result);
            pPair++;
        }
        break;

    // CASE (command table terminator)
    case ADI_TMR_GP_CMD_END:
        break;

    // CASE (command pair)
    case ADI_TMR_GP_CMD_PAIR:

        // avoid casts
        pPair = (ADI_TMR_GP_CMD_VALUE_PAIR *)Value;

        // process and return
        return (adi_tmr_GPControl(TimerID, pPair->CommandID, pPair->Value));

    // CASE (set period register)
    case ADI_TMR_GP_CMD_SET_PERIOD:

        // do it
        pRegs->Period = u32Value;
        break;

    // CASE (set width register)
    case ADI_TMR_GP_CMD_SET_WIDTH:

        // do it
        pRegs->Width = u32Value;
        break;

    // CASE (get width register)
    case ADI_TMR_GP_CMD_GET_WIDTH:

        // do it
        *((u32 *)Value) = pRegs->Width;
        break;

    // CASE (get period register)
    case ADI_TMR_GP_CMD_GET_PERIOD:

        // do it
        *((u32 *)Value) = pRegs->Period;
        break;


    // CASE (get counter register)
    case ADI_TMR_GP_CMD_GET_COUNTER:

        // do it
        *((u32 *)Value) = pRegs->Counter;
        break;

    // CASE (set a config register field)
    case ADI_TMR_GP_CMD_SET_TIMER_MODE:
    case ADI_TMR_GP_CMD_SET_PULSE_HI:
    case ADI_TMR_GP_CMD_SET_COUNT_METHOD:
    case ADI_TMR_GP_CMD_SET_INTERRUPT_ENABLE:
    case ADI_TMR_GP_CMD_SET_INPUT_SELECT:
    case ADI_TMR_GP_CMD_SET_OUTPUT_PAD_DISABLE:
    case ADI_TMR_GP_CMD_SET_CLOCK_SELECT:
    case ADI_TMR_GP_CMD_SET_TOGGLE_HI:
    case ADI_TMR_GP_CMD_RUN_DURING_EMULATION:

        // protect this region
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_tmr_pCriticalRegionArg);

        // point to the proper entry in the config register table
        pFields = &GPConfigRegisterFields[Command - ADI_TMR_GP_CMD_SET_TIMER_MODE];

        // set the value in the register
        tmp16 = pRegs->Config;
        tmp16 &= ~(pFields->Mask << pFields->StartBit);
        tmp16 |= ((u32Value & pFields->Mask) << pFields->StartBit);
        pRegs->Config = tmp16;

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);
        break;

    // CASE (enable timer)
    case ADI_TMR_GP_CMD_ENABLE_TIMER:

        // enable/disable it
        Result = adi_tmr_GPGroupEnable (TimerID, (u32)Value);
        break;

    // CASE (get error type)
    case ADI_TMR_GP_CMD_GET_ERROR_TYPE:

        // store it in the caller's location
        *((u32 *)Value) = (pRegs->Config & 0xc000) >> 14;
        break;

    // CASE (is interrupt asserted)
    case ADI_TMR_GP_CMD_IS_INTERRUPT_ASSERTED:

        // store it in the caller's location
        *((u32 *)Value) = (pInfo->pGroupRegisters->Status >> pInfo->IRQBit) & 0x1;
        break;

    // CASE (clear interrupt)
    case ADI_TMR_GP_CMD_CLEAR_INTERRUPT:

        // clear it
        pInfo->pGroupRegisters->Status  = (0x1 << pInfo->IRQBit);
        break;

    // CASE (is error)
    case ADI_TMR_GP_CMD_IS_ERROR:

        // store it in the caller's location
        *((u32 *)Value) = (pInfo->pGroupRegisters->Status >> pInfo->ErrorBit) & 0x1;
        break;

    // CASE (clear error)
    case ADI_TMR_GP_CMD_CLEAR_ERROR:

        // clear it
        pInfo->pGroupRegisters->Status  = (0x1 << pInfo->ErrorBit);
        break;

    // CASE (is slave enabled)
    case ADI_TMR_GP_CMD_IS_SLAVE_ENABLED:

        // store it in the caller's location
        *((u32 *)Value) = (pInfo->pGroupRegisters->Status >> pInfo->SlaveBit) & 0x1;
        break;

    // CASE (immediate halt in PWM mode)
    case ADI_TMR_GP_CMD_IMMEDIATE_HALT:

        // halt!!
        pInfo->pGroupRegisters->Status  = (0x1 << pInfo->SlaveBit);
        break;


    // CASE
    case ADI_TMR_GP_CMD_SET_ENABLE_DELAY:
        pInfo->EnableDelay_Count = u32Value;
        pInfo->EnableDelay_Clocks = 0;
        break;

    // default
    default:

        // bad command
        Result = ADI_TMR_RESULT_BAD_COMMAND;
        break;

    // ENDCASE
    }

    // return
    return (Result);
}




/*********************************************************************

    Function:       adi_tmr_CoreControl

    Description:    Controls the core timer

*********************************************************************/

ADI_TMR_RESULT adi_tmr_CoreControl(         // controls the core timer
    ADI_TMR_CORE_CMD    Command,                // command ID
    void                *Value                  // command specific value
) {

    ADI_TMR_RESULT                  Result;     // return code
    ADI_TMR_CORE_CMD_VALUE_PAIR     *pPair;     // pointer to command pair
    u32                             u32Value;   // u32 type to avoid casts/warnings etc.
    ADI_TMR_CONFIG_REGISTER_FIELDS  *pFields;   // pointer into fields array
    u16                             tmp16;      // temporary storage
    ADI_TMR_INFO                    *pInfo;     // pointer to timer's info
    ADI_TMR_CORE_REGISTERS          *pRegs;     // pointer to core registers
    void                            *pExitCriticalArg;  // return value from EnterCritical

    // debug
#if defined(ADI_SSL_DEBUG)
#endif

    // avoid casts and assume success
    u32Value = (u32)Value;
    Result = ADI_TMR_RESULT_SUCCESS;
    pInfo = &TimerInfo[ADI_TMR_GET_TIMER_INDEX(ADI_TMR_CORE_TIMER)];
    pRegs = pInfo->Registers.Core;

    // CASEOF (Command ID)
    switch (Command) {

    // CASE (command table)
    case ADI_TMR_CORE_CMD_TABLE:

        // avoid casts
        pPair = (ADI_TMR_CORE_CMD_VALUE_PAIR *)Value;

        // process each command pair
        while (pPair->CommandID != ADI_TMR_CORE_CMD_END) {
            if ((Result = adi_tmr_CoreControl(pPair->CommandID, pPair->Value)) != ADI_TMR_RESULT_SUCCESS) return(Result);
            pPair++;
        }
        break;

    // CASE (command table terminator)
    case ADI_TMR_CORE_CMD_END:
        break;

    // CASE (command pair)
    case ADI_TMR_CORE_CMD_PAIR:

        // avoid casts
        pPair = (ADI_TMR_CORE_CMD_VALUE_PAIR *)Value;

        // process and return
        return (adi_tmr_CoreControl(pPair->CommandID, pPair->Value));

    // CASE (set a config register field)
    case ADI_TMR_CORE_CMD_SET_ACTIVE_MODE:
    case ADI_TMR_CORE_CMD_ENABLE_TIMER:
    case ADI_TMR_CORE_CMD_SET_AUTO_RELOAD:

        // point to the proper entry in the config register table
        pFields = &CoreConfigRegisterFields[Command - ADI_TMR_CORE_CMD_SET_ACTIVE_MODE];

        // protect this region
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_tmr_pCriticalRegionArg);

        // set the value in the register
        tmp16 = pRegs->Cntl;
        tmp16 &= ~(pFields->Mask << pFields->StartBit);
        tmp16 |= ((u32Value & pFields->Mask) << pFields->StartBit);
        pRegs->Cntl = tmp16;

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);
        break;

    // CASE (has interrupt occurred)
    case ADI_TMR_CORE_CMD_HAS_INTERRUPT_OCCURRED:

        // store it in the caller's location
        *((u32 *)Value) = (pRegs->Cntl >> 3) & 0x1;
        break;

    // CASE (clear interrupt occurred)
    case ADI_TMR_CORE_CMD_RESET_INTERRUPT_OCCURRED:

        // clear it
        pRegs->Cntl |= 0x8;
        break;

    // CASE (set count register)
    case ADI_TMR_CORE_CMD_SET_COUNT:

        // do it
        pRegs->Count = u32Value;
        break;

    // CASE (set period register)
    case ADI_TMR_CORE_CMD_SET_PERIOD:

        // do it
        pRegs->Period = u32Value;
        break;

    // CASE (set scale register)
    case ADI_TMR_CORE_CMD_SET_SCALE:

        // do it
        pRegs->Scale = u32Value & 0xff;
        break;

    // CASE (query core timer - tar 31844)
    case ADI_TMR_CORE_CMD_GET_COUNT:

        // do it
        *((u32 *)Value) = pRegs->Count;
        break;

    // default
    default:

        // bad command
        Result = ADI_TMR_RESULT_BAD_COMMAND;
        break;

    // ENDCASE
    }

    // return
    return (Result);
}




/*********************************************************************

    Function:       adi_tmr_WatchdogControl

    Description:    Controls the watchdog timer

*********************************************************************/

ADI_TMR_RESULT adi_tmr_WatchdogControl(     // controls the watchdog timer
    ADI_TMR_WDOG_CMD    Command,                // command ID
    void                *Value                  // command specific value
) {

    ADI_TMR_RESULT                  Result;     // return code
    ADI_TMR_WDOG_CMD_VALUE_PAIR     *pPair;     // pointer to command pair
    u32                             u32Value;   // u32 type to avoid casts/warnings etc.
    ADI_TMR_CONFIG_REGISTER_FIELDS  *pFields;   // pointer into fields array
    u16                             tmp16;      // temporary storage
    ADI_TMR_INFO                    *pInfo;     // pointer to timer's info
    ADI_TMR_WDOG_REGISTERS          *pRegs;     // pointer to watchdog registers
    void                            *pExitCriticalArg;  // return value from EnterCritical

    // debug
#if defined(ADI_SSL_DEBUG)
#endif

    // avoid casts and assume success
    u32Value = (u32)Value;
    Result = ADI_TMR_RESULT_SUCCESS;
    pInfo = &TimerInfo[ADI_TMR_GET_TIMER_INDEX(ADI_TMR_WDOG_TIMER)];
    pRegs = pInfo->Registers.Watchdog;

    // CASEOF (Command ID)
    switch (Command) {

    // CASE (command table)
    case ADI_TMR_WDOG_CMD_TABLE:

        // avoid casts
        pPair = (ADI_TMR_WDOG_CMD_VALUE_PAIR *)Value;

        // process each command pair
        while (pPair->CommandID != ADI_TMR_WDOG_CMD_END) {
            if ((Result = adi_tmr_WatchdogControl(pPair->CommandID, pPair->Value)) != ADI_TMR_RESULT_SUCCESS) return(Result);
            pPair++;
        }
        break;

    // CASE (command table terminator)
    case ADI_TMR_WDOG_CMD_END:
        break;

    // CASE (command pair)
    case ADI_TMR_WDOG_CMD_PAIR:

        // avoid casts
        pPair = (ADI_TMR_WDOG_CMD_VALUE_PAIR *)Value;

        // process and return
        return (adi_tmr_WatchdogControl(pPair->CommandID, pPair->Value));

    // CASE (set a config register field)
    case ADI_TMR_WDOG_CMD_EVENT_SELECT:
    case ADI_TMR_WDOG_CMD_ENABLE_TIMER:

        // have to special case the enable field
        if ((Command == ADI_TMR_WDOG_CMD_ENABLE_TIMER) && (u32Value == FALSE)) u32Value = 0xAD;

        // point to the proper entry in the config register table
        pFields = &WatchdogConfigRegisterFields[Command - ADI_TMR_WDOG_CMD_EVENT_SELECT];

        // protect this region
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_tmr_pCriticalRegionArg);

        // set the value in the register
        tmp16 = pRegs->Ctl;
        tmp16 &= ~(pFields->Mask << pFields->StartBit);
        tmp16 |= ((u32Value & pFields->Mask) << pFields->StartBit);
        pRegs->Ctl = tmp16;

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);
        break;

    // CASE (has watchdog expired)
    case ADI_TMR_WDOG_CMD_HAS_EXPIRED:

        // store it in the caller's location
        *((u32 *)Value) = (pRegs->Ctl >> 15) & 0x1;
        break;

    // CASE (reset expired indicator)
    case ADI_TMR_WDOG_CMD_RESET_EXPIRED:

        // clear it
        pRegs->Ctl |= 0x8000;
        break;

    // CASE (get status)
    case ADI_TMR_WDOG_CMD_GET_STATUS:

        // do it
        *((u32 *)Value) = pRegs->Stat;
        break;

    // CASE (set count register)
    case ADI_TMR_WDOG_CMD_SET_COUNT:

        // do it
        pRegs->Cnt = u32Value;
        break;

    // CASE (reload status register)
    case ADI_TMR_WDOG_CMD_RELOAD_STATUS:

        // do it
        pRegs->Stat = 0;
        break;

    // default
    default:

        // bad command
        Result = ADI_TMR_RESULT_BAD_COMMAND;
        break;

    // ENDCASE
    }

    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_tmr_Hook

    Description:    Hooks an interrupt handler for both peripheral (GP and watchdog) and core timers

*********************************************************************/

static ADI_TMR_RESULT adi_tmr_Hook(             // hooks an interrupt handler for a timer
    u32                     TimerID,                // argument supplied to handler (TimerID)
    ADI_INT_PERIPHERAL_ID   PeripheralID,           // peripheral ID
    u32                     NestingFlag,            // interrupt nesting enable flag (TRUE/FALSE)
    u32                     WakeupFlag              // wakeup flag (TRUE/FALSE)
){

    u32             IVG;        // IVG for the timer
    ADI_TMR_RESULT  Result;     // result

    // IF (it's the core timer)
    if (TimerID == ADI_TMR_CORE_TIMER) {

        // note the IVG
        IVG = ADI_INT_IVG_CORE_TIMER;

    // ELSE (it's a GP or watchdog timer)
    } else {

        // get the IVG to which the peripheral is mapped
        if (adi_int_SICGetIVG(PeripheralID, &IVG) != ADI_INT_RESULT_SUCCESS) {
            return (ADI_TMR_RESULT_INTERRUPT_MANAGER_ERROR);
        }

        // set wakeup accordingly
        if (adi_int_SICWakeup(PeripheralID, WakeupFlag) != ADI_INT_RESULT_SUCCESS) {
            return (ADI_TMR_RESULT_INTERRUPT_MANAGER_ERROR);
        }

        // enable the interrupt through the SIC
        if (adi_int_SICEnable(PeripheralID) != ADI_INT_RESULT_SUCCESS) {
            return (ADI_TMR_RESULT_INTERRUPT_MANAGER_ERROR);
        }

    // ENDIF
    }

    // hook the handler into the chain
    if (adi_int_CECHook(IVG, TimerHandler, (void *)TimerID, NestingFlag) != ADI_INT_RESULT_SUCCESS) {
        return (ADI_TMR_RESULT_INTERRUPT_MANAGER_ERROR);
    }

    // return
    return(ADI_TMR_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       adi_tmr_Unhook

    Description:    Unhooks an interrupt handler for both peripheral (GP or watchdog) and core timers

*********************************************************************/

static ADI_TMR_RESULT adi_tmr_Unhook(   // unhooks an interrupt handler for a timer
    u32                     TimerID,        // argument supplied to handler (TimerID)
    ADI_INT_PERIPHERAL_ID   PeripheralID    // peripheral ID
){

    u32             IVG;                                // IVG for the timer
    ADI_TMR_RESULT  Result;                             // result

    // IF (it's the core timer)
    if (TimerID == ADI_TMR_CORE_TIMER) {

        // note the IVG
        IVG = ADI_INT_IVG_CORE_TIMER;

    // ELSE
    } else {

        // get the IVG to which the peripheral is mapped
        if (adi_int_SICGetIVG(PeripheralID, &IVG) != ADI_INT_RESULT_SUCCESS) {
            return (ADI_TMR_RESULT_INTERRUPT_MANAGER_ERROR);
        }

        // disable the interrupt through the SIC
        if (adi_int_SICDisable(PeripheralID) != ADI_INT_RESULT_SUCCESS) {
            return (ADI_TMR_RESULT_INTERRUPT_MANAGER_ERROR);
        }

    // ENDIF
    }

    // unhook the handler from the chain
    if (adi_int_CECUnhook(IVG, TimerHandler, (void *)TimerID) != ADI_INT_RESULT_SUCCESS) {
        return (ADI_TMR_RESULT_INTERRUPT_MANAGER_ERROR);
    }

    // return
    return(ADI_TMR_RESULT_SUCCESS);
}



/*********************************************************************

    Function:       TimerHandler

    Description:    Processes timer interrupts and posts callbacks

*********************************************************************/

static ADI_INT_HANDLER(TimerHandler)    // timer handler
{

    ADI_TMR_INFO    *pInfo;             // pointer to timer's info
    u32             TimerID;            // timer ID
    u32             asserted;           // assertion status of Timer

    // get the timer ID
    TimerID = (u32)ClientArg;

    // get the info for the timer
    pInfo = &TimerInfo[ADI_TMR_GET_TIMER_INDEX(TimerID)];

    // CASEOF (timer type)
    switch (TimerID){

        // CASE (core timer)
        case ADI_TMR_CORE_TIMER:

            // check that the core timer has expired. If not pass control along the chain
            adi_tmr_CoreControl(ADI_TMR_CORE_CMD_HAS_INTERRUPT_OCCURRED, (void*)&asserted);
            if (!asserted) {
                return ADI_INT_RESULT_NOT_PROCESSED;
            }

            // clear the status stating the core timer has expired
            adi_tmr_CoreControl(ADI_TMR_CORE_CMD_RESET_INTERRUPT_OCCURRED, NULL);
            break;

        // CASE (watchdog timer)
        case ADI_TMR_WDOG_TIMER:

            // check that the watchdog timer has asserted. If not pass control along the chain
            adi_tmr_WatchdogControl(ADI_TMR_WDOG_CMD_HAS_EXPIRED, (void*)&asserted);
            if (!asserted) {
                return ADI_INT_RESULT_NOT_PROCESSED;
            }

            // watchdog is usually a timer that only triggers when something typically bad happens so there's really not much
            // to do other than notify the client that it triggered
            break;

        // default (GP timer)
        default:

            // check that the TimerID has asserted. If not pass control along the chain
            adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_IS_INTERRUPT_ASSERTED, (void*)&asserted);
            if (!asserted) {
                return ADI_INT_RESULT_NOT_PROCESSED;
            }

            // clear the interrupt
            adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_CLEAR_INTERRUPT, NULL);
            break;

    // ENDCASE
    }

    // notify the callback function that the timer expired
    if (pInfo->DCBHandle) {
        adi_dcb_Post(pInfo->DCBHandle, 0, pInfo->ClientCallback, pInfo->ClientHandle, ADI_TMR_EVENT_TIMER_EXPIRED, (void *)TimerID);
    } else {
        (pInfo->ClientCallback)(pInfo->ClientHandle, ADI_TMR_EVENT_TIMER_EXPIRED, (void *)TimerID);
    }

    // return
    return(ADI_INT_RESULT_PROCESSED);
}



#if defined(__ADSP_BRAEMAR__)   // port control for Braemar class devices

/*********************************************************************

    Function:       adi_tmr_PortControl

    Description:    Configures port control for Braemar class devices

*********************************************************************/

static void adi_tmr_PortControl(
    u32 TimerIDs    // OR-ing of TimerIDs being enabled
) {
    // TMRx values
    u32 TMRx[8] = {ADI_PORTS_DIR_TMR_0, ADI_PORTS_DIR_TMR_1, ADI_PORTS_DIR_TMR_2, ADI_PORTS_DIR_TMR_3,
                   ADI_PORTS_DIR_TMR_4, ADI_PORTS_DIR_TMR_5, ADI_PORTS_DIR_TMR_6, ADI_PORTS_DIR_TMR_7 };
#if 0 // unsure this is required
    u32 TACLKx;                     // TACLKx value
    u32 TACIx;                      // TACIx value
#endif
    u32 TMRCLK = ADI_PORTS_DIR_TMR_CLK; // TMRCLK value
    int i;                          // counter
    ADI_TMR_INFO *pInfo;            // pointer to a timer's info
    ADI_TMR_CONFIG_REG  Config;     // config register of timer

    // FOR (each gp timer)
    for (i = 0, pInfo = TimerInfo/*, TMRx = ADI_PORTS_DIR_TMR_0 , TACLKx = ADI_PORTS_TACLK_0, TACIx = ADI_PORTS_TACI_0 */;
         i < ADI_TMR_GP_TIMER_COUNT;
         i++, pInfo++, TimerIDs >>= 1/*, TMRx++, TACLKx++, TACIx++*/) {

        // IF (this timer is being enabled)
        if (TimerIDs & 0x1) {

            // read the config register for the timer
            *((u16 *)&Config) = pInfo->Registers.GP->Config;

            // CASEOF (TMODE)
            switch (Config.b_TMODE) {

                // CASE (Internal)
                case 0:

                    // nothing to do
                    break;

                // CASE (PWM mode)
                case 1:

                    // IF (we need to enable the TMRx pad)
                    if (!Config.b_OUT_DIS) {

                        // enable TMRx
                        adi_ports_EnableTimer(&TMRx[i], 1, TRUE);

                    // ENDIF
                    }

                    // IF (we're using the PWM_CLK)
                    if (Config.b_CLK_SEL) {

                        // IF (input select says to use the TMRCLK pin)
                        if (Config.b_TIN_SEL) {

                            // enable TMRCLK
                            adi_ports_EnableTimer(&TMRCLK, 1, TRUE);
#if 0 // unsure this is required
                        // ELSE
                        } else {

                            // enable TACLKx
                            adi_ports_EnableTimer(&TACLKx, 1, TRUE);
#endif
                        // ENDIF
                        }

                    // ENDIF
                    }
                    break;

                // CASE (WDTH_CAP mode)
                case 2:

#if 0 // unsure this is required
                    // IF (input select says to use the TACIx pin)
                    if (Config.b_TIN_SEL) {

                        // enable TACIx
                        adi_ports_EnableTimer(&TACIx, 1, TRUE);

                    // ELSE
                    } else {
#else
                    if (!Config.b_TIN_SEL) {
#endif

                        // enable TMRx
                        adi_ports_EnableTimer(&TMRx[i], 1, TRUE);

                    // ENDIF
                    }
                    break;

                // CASE (EXT_CLK)
                case 3:

                    // enable TMRx
                    adi_ports_EnableTimer(&TMRx[i], 1, TRUE);
                    break;

            // ENDCASE
            }

        // ENDIF
        }

    // ENDFOR
    }

    // return
}

#elif defined(__ADSP_EDINBURGH__) || defined(__ADSP_STIRLING__) || defined(__ADSP_TETON__)
static void adi_tmr_PortControl(
    u32 TimerIDs
)
{
	/* no timer port control for these devices */
}
#else
/* Port control for all other class devices */
/*********************************************************************

    Function: adi_tmr_PortControl

        Configures port control for class devices

    Parameters:
        TimerIDs    - OR-ing of TimerIDs being enabled

*********************************************************************/
static void adi_tmr_PortControl(
    u32 TimerIDs
)
{
    /* Loop variable */
    u8                      i;
    /* Location to hold Timer config register value */
    ADI_TMR_CONFIG_REG      Config;
    /* pointer to a timer's info */
    ADI_TMR_INFO            *pInfo;

#if defined(__ADSP_MOY__)
    /* Port control directives for GP Timer pads */
    ADI_PORTS_DIRECTIVE     aeGPTmrTMRxDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_PPI_FS1_TMR0,
        ADI_PORTS_DIRECTIVE_PPI_FS2_TMR1,
        ADI_PORTS_DIRECTIVE_TMR2,
        ADI_PORTS_DIRECTIVE_TMR3,
        ADI_PORTS_DIRECTIVE_TMR4,
        ADI_PORTS_DIRECTIVE_TMR5,
        ADI_PORTS_DIRECTIVE_TMR6,
        ADI_PORTS_DIRECTIVE_TMR7
    };

    /* GPIO Flags for GP Timers TACLK source */
    ADI_FLAG_ID     aeGPTmrTACLKxFlags[] =
    {
        ADI_FLAG_PF4,
        ADI_FLAG_PF5,
        ADI_FLAG_PF11,
        ADI_FLAG_PH1,
        ADI_FLAG_UNDEFINED, /* CLKBUF for TMR 4 */
        ADI_FLAG_UNDEFINED, /* CLKBUF for TMR 5 */
        ADI_FLAG_PG4,
        ADI_FLAG_PG11
    };

    /* GPIO Flags for GP Timers Alternate Capture Input (TACI) */
    ADI_FLAG_ID     aeGPTmrTACIxFlags[] =
    {
        ADI_FLAG_UNDEFINED, /* CNT0 */
        ADI_FLAG_UNDEFINED, /* CNT1 */
        ADI_FLAG_UNDEFINED, /* UART0 RX on PG12 */
        ADI_FLAG_UNDEFINED, /* UART1 RX on PG0 */
        ADI_FLAG_UNDEFINED, /* UART1 RX on PF7 */
        ADI_FLAG_PG1,
        ADI_FLAG_PG10,
        ADI_FLAG_PH2
    };

    /* TMRCLK directive */
    ADI_PORTS_DIRECTIVE     eTMRClkDirective = ADI_PORTS_DIRECTIVE_PPICLK_TMRCLK;

#elif defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_KOOKABURRA__)
    /* Port control directives for GP Timer pads */
    ADI_PORTS_DIRECTIVE     aeGPTmrTMRxDirectives[] =
    {
        (ADI_PORTS_DIRECTIVE) -1, /* undefined */
        ADI_PORTS_DIRECTIVE_PPI_FS2_MUX0,
        ADI_PORTS_DIRECTIVE_TMR2,
        ADI_PORTS_DIRECTIVE_TMR3,
        ADI_PORTS_DIRECTIVE_TMR4,
        ADI_PORTS_DIRECTIVE_TMR5,
        ADI_PORTS_DIRECTIVE_TMR6,
        ADI_PORTS_DIRECTIVE_TMR7_MUX0
    };

    /* GPIO Flags for GP Timers TACLK source */
    ADI_FLAG_ID     aeGPTmrTACLKxFlags[] =
    {
        ADI_FLAG_PF4,
        ADI_FLAG_PF5,
        ADI_FLAG_PH8,
        ADI_FLAG_PH9,
        ADI_FLAG_UNDEFINED, /* CLKBUF for TMR 4 */
        ADI_FLAG_UNDEFINED, /* CLKBUF for TMR 5 */
        ADI_FLAG_UNDEFINED, /* CLKBUF for TMR 6 */
        ADI_FLAG_UNDEFINED  /* CLKBUF for TMR 7 */
    };

    /* GPIO Flags for GP Timers Alternate Capture Input (TACI) */
    ADI_FLAG_ID     aeGPTmrTACIxFlags[] =
    {
        ADI_FLAG_PF6,
        ADI_FLAG_PF7,
        ADI_FLAG_UNDEFINED, /* UART1 RX on PG6 */
        ADI_FLAG_UNDEFINED, /* UART1 RX on PG7 */
        ADI_FLAG_UNDEFINED, /* UART1 RX on PG8 */
        ADI_FLAG_PG9,
        ADI_FLAG_PG10,
        ADI_FLAG_UNDEFINED  /* COUNTER0 */
    };

    /* TMRCLK directive */
    ADI_PORTS_DIRECTIVE     eTMRClkDirective = ADI_PORTS_DIRECTIVE_PPICLK_TMRCLK;

#elif defined(__ADSP_BRODIE__)
    /* Port control directives for GP Timer pads */
    ADI_PORTS_DIRECTIVE     aeGPTmrTMRxDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_TMR0_MUX1,
        ADI_PORTS_DIRECTIVE_TMR1_MUX1,
        ADI_PORTS_DIRECTIVE_TMR2,
        ADI_PORTS_DIRECTIVE_TMR3,
        ADI_PORTS_DIRECTIVE_TMR4,
        ADI_PORTS_DIRECTIVE_TMR5,
        ADI_PORTS_DIRECTIVE_TMR6,
        ADI_PORTS_DIRECTIVE_TMR7
    };

    /* GPIO Flags for GP Timers TACLK source */
    ADI_FLAG_ID     aeGPTmrTACLKxFlags[] =
    {
        ADI_FLAG_PF3,
        ADI_FLAG_PF4,
        ADI_FLAG_PG11,
        ADI_FLAG_PG3,
        ADI_FLAG_UNDEFINED, /* CLKBUF for TMR 4 */
        ADI_FLAG_UNDEFINED, /* CLKBUF for TMR 5 */
        ADI_FLAG_PF0,
        ADI_FLAG_PF1
    };

    /* GPIO Flags for GP Timers Alternate Capture Input (TACI) */
    ADI_FLAG_ID     aeGPTmrTACIxFlags[] =
    {
        ADI_FLAG_PF5,
        ADI_FLAG_PF6,
        ADI_FLAG_UNDEFINED, /* UART1 RX on PH7 */
        ADI_FLAG_PF11,
        ADI_FLAG_UNDEFINED, /* UART0 RX on PG10 */
        ADI_FLAG_PG4,
        ADI_FLAG_PG8,
        ADI_FLAG_UNDEFINED  /* COUNTER0 */
    };

    /* TMRCLK directive */
    ADI_PORTS_DIRECTIVE     eTMRClkDirective = ADI_PORTS_DIRECTIVE_TMR_CLK_MUX1;

#elif defined(__ADSP_MOAB__)
    /* Port control directives for GP Timer pads */
    ADI_PORTS_DIRECTIVE     aeGPTmrTMRxDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_TMR0, 
        ADI_PORTS_DIRECTIVE_TMR1,
        ADI_PORTS_DIRECTIVE_TMR2,
        ADI_PORTS_DIRECTIVE_TMR3,
        ADI_PORTS_DIRECTIVE_TMR4,
        ADI_PORTS_DIRECTIVE_TMR5,
        ADI_PORTS_DIRECTIVE_TMR6,
        ADI_PORTS_DIRECTIVE_TMR7,
        ADI_PORTS_DIRECTIVE_TMR8,
        ADI_PORTS_DIRECTIVE_TMR9,
        ADI_PORTS_DIRECTIVE_TMR10
    };

    /* GPIO Flags for GP Timers TACLK source */
    ADI_FLAG_ID     aeGPTmrTACLKxFlags[] =
    {
        ADI_FLAG_PA7,
        ADI_FLAG_PA8,
        ADI_FLAG_PA10,
        ADI_FLAG_PA11,
        ADI_FLAG_PA12,
        ADI_FLAG_PA13,
        ADI_FLAG_PA14,
        ADI_FLAG_PA15,
        ADI_FLAG_PH5,
        ADI_FLAG_PH6,
        ADI_FLAG_PH7
    };

    /* GPIO Flags for GP Timers Alternate Capture Input (TACI) */
    ADI_FLAG_ID     aeGPTmrTACIxFlags[] =
    {
        ADI_FLAG_UNDEFINED, /* UART0 RX on PE8 */
        ADI_FLAG_UNDEFINED, /* UART1 RX on PH1 */
        ADI_FLAG_UNDEFINED, /* UART2 RX on PB5 */
        ADI_FLAG_UNDEFINED, /* UART3 RX on PB7 */
        ADI_FLAG_UNDEFINED, /* CAN0 RX on PG13 */
        ADI_FLAG_UNDEFINED, /* CAN1 RX on PG15 */
        ADI_FLAG_UNDEFINED, /* COUNTER0 */
        ADI_FLAG_PA15,
        ADI_FLAG_PH5,
        ADI_FLAG_PH6,
        ADI_FLAG_PH7
    };

    /* TMRCLK directive */
    ADI_PORTS_DIRECTIVE     eTMRClkDirective = ADI_PORTS_DIRECTIVE_EPPI0_CLK;

#elif defined(__ADSP_DELTA__)
    /* Port control directives for GP Timer pads */
    ADI_PORTS_DIRECTIVE     aeGPTmrTMRxDirectives[] =
    {
        ADI_PORTS_DIRECTIVE_PPI_FS1_TMR0, 
        ADI_PORTS_DIRECTIVE_PPI_FS2_TMR1,
        ADI_PORTS_DIRECTIVE_TMR2
    };

    /* GPIO Flags for GP Timers TACLK source */
    ADI_FLAG_ID     aeGPTmrTACLKxFlags[] =
    {
        ADI_FLAG_UNDEFINED,
        ADI_FLAG_PG14,
        ADI_FLAG_PG15
    };

    /* GPIO Flags for GP Timers Alternate Capture Input (TACI) */
    ADI_FLAG_ID     aeGPTmrTACIxFlags[] =
    {
        ADI_FLAG_UNDEFINED, /* UART0 RX on PF12 */
        ADI_FLAG_UNDEFINED, /* UART0 RX on PF12 */
        ADI_FLAG_UNDEFINED  /* UART0 RX on PF12 */
    };

    /* TMRCLK directive */
    ADI_PORTS_DIRECTIVE     eTMRClkDirective = (ADI_PORTS_DIRECTIVE) -1; /* undefined */

#endif

    /* FOR (each GP timer) */
    for (i = 0, pInfo = TimerInfo;
         i < ADI_TMR_GP_TIMER_COUNT;
         i++, pInfo++, TimerIDs >>= 1)
    {
        /* IF (This timer is being enabled) */
        if (TimerIDs & 0x1)
        {
            /* read the config register for the timer */
            *((u16 *)&Config) = pInfo->Registers.GP->Config;

            /* CASEOF (TMODE) */
            switch (Config.b_TMODE)
            {
                /* CASE (Unused state) */
                case 0:

                    /* nothing to do */
                    break;

                /* CASE (PWM mode) */
                case 1:

                    /* IF (we need to enable the TMRx pad) */
                    if (!Config.b_OUT_DIS)
                    {
                        /* Enable TMRx pin */
                        adi_ports_Configure(&aeGPTmrTMRxDirectives[i], 1);
                    }

                    /* IF (we're using the PWM_CLK) */
                    if (Config.b_CLK_SEL)
                    {
                        /* IF (Input select says to use the TMRCLK pin) */
                        if (Config.b_TIN_SEL)
                        {
                            /* Enable TMRCLK */
                            adi_ports_Configure(&eTMRClkDirective, 1);
                        }
                        /* ELSE (Use TACLKx) */
                        else
                        {
                            /* IF (TACLKx directive is valid) */
                            if (aeGPTmrTACLKxFlags[i] != (ADI_PORTS_DIRECTIVE) -1)
                            {
                                /* Open this flag */
                                adi_flag_Open(aeGPTmrTACLKxFlags[i]);
                                /* Set this flag direction as input */
                                adi_flag_SetDirection (aeGPTmrTACLKxFlags[i], ADI_FLAG_DIRECTION_INPUT);
                            }

                        } /* End of if (Input select says to use the TMRCLK pin) else */

                    } /* End of if (we're using the PWM_CLK) */

                    break;

                /* CASE (WDTH_CAP mode) */
                case 2:

                    /* IF (Input select says to use the TACIx pin) */
                    if (Config.b_TIN_SEL)
                    {
                        /* IF (TACIx directive is valid) */
                        if (aeGPTmrTACIxFlags[i] != (ADI_PORTS_DIRECTIVE) -1)
                        {
                            /* Open this flag */
                            adi_flag_Open(aeGPTmrTACIxFlags[i]);
                            /* Set this flag direction as input */
                            adi_flag_SetDirection (aeGPTmrTACIxFlags[i], ADI_FLAG_DIRECTION_INPUT);
                        }
                    }
                    /* ELSE (Use TMRx pin) */
                    else
                    {
                        /* Enable TMRx pin */
                        adi_ports_Configure(&aeGPTmrTMRxDirectives[i], 1);
                    }
                    break;

                /* CASE (EXT_CLK) */
                case 3:

                    /* Enable TMRx pin */
                    adi_ports_Configure(&aeGPTmrTMRxDirectives[i], 1);
                    break;

            } /* End CASEOF (TMODE) */

        } /* End of if (This timer is being enabled) */

    } /* End of for (each GP timer) */

}

#endif /* port control */

#ifdef ADI_SSL_DEBUG

/*********************************************************************

    Function:       adi_tmr_ValidateTimerID

    Description:    Validates a gp timer ID

*********************************************************************/


static ADI_TMR_RESULT adi_tmr_ValidateTimerID(  // validates any timer ID
    u32 TimerID
) {

    // verify it's a core, watchdog or GP timer ID
    if  ((adi_tmr_ValidateCoreTimerID(TimerID) == ADI_TMR_RESULT_SUCCESS) ||
         (adi_tmr_ValidateWatchdogTimerID(TimerID) == ADI_TMR_RESULT_SUCCESS) ||
         (adi_tmr_ValidateGPTimerID(TimerID) == ADI_TMR_RESULT_SUCCESS)) {
            return (ADI_TMR_RESULT_SUCCESS);
    }
    return (ADI_TMR_RESULT_BAD_TIMER_ID);
}


/*********************************************************************

    Function:       adi_tmr_ValidateGPTimerID

    Description:    Validates a gp timer ID

*********************************************************************/


static ADI_TMR_RESULT adi_tmr_ValidateGPTimerID(    // validates a GP timer ID
    u32 TimerID
) {

    int i;                  // counter

    // check the timer ID against all of our timer IDs
    for (i = 0; i < ADI_TMR_GP_TIMER_COUNT; i++) {
        if (TimerID == ADI_TMR_CREATE_GP_TIMER_ID(i)) {
            return (ADI_TMR_RESULT_SUCCESS);
        }
    }

    // return
    return (ADI_TMR_RESULT_BAD_TIMER_ID);
}



/*********************************************************************

    Function:       adi_tmr_ValidateGPTimerIDs

    Description:    Validates a group of timer IDs

*********************************************************************/


static ADI_TMR_RESULT adi_tmr_ValidateGPTimerIDs(   // validates a group of GP timer IDs
    u32 TimerIDs
) {

    int i;                          // counter
    u32             Mask;           // mask to compute TimerIDs
    u32             BitUnderTest;   // bit we're testing

    // initialize our mask
    Mask = 0;

    // FOR (each gp timer)
    for (i = 0, BitUnderTest = 0x1; i < ADI_TMR_GP_TIMER_COUNT; i++, BitUnderTest <<= 1) {

        // IF (this timer is in the group)
        if (TimerIDs & BitUnderTest) {

            // add this timer ID into the mask
            Mask |= ADI_TMR_CREATE_GP_TIMER_ID(i);

        // ENDIF
        }

    // ENDFOR
    }

    // insure the mask is the same as the TimerIDs
    if (Mask == TimerIDs) {
        return (ADI_TMR_RESULT_SUCCESS);
    }

    // return
    return (ADI_TMR_RESULT_BAD_TIMER_IDS);
}



/*********************************************************************

    Function:       adi_tmr_ValidateCoreTimerID

    Description:    Validates a core timer ID

*********************************************************************/


static ADI_TMR_RESULT adi_tmr_ValidateCoreTimerID(  // validates a core timer ID
    u32 TimerID
) {

    // verify it's a core ID
    if (TimerID == ADI_TMR_CORE_TIMER) return (ADI_TMR_RESULT_SUCCESS);
    return (ADI_TMR_RESULT_BAD_TIMER_ID);
}


/*********************************************************************

    Function:       adi_tmr_ValidateWatchdogTimerID

    Description:    Validates a watchdog timer ID

*********************************************************************/


static ADI_TMR_RESULT adi_tmr_ValidateWatchdogTimerID(  // validates a watchdog timer ID
    u32 TimerID
) {

    // verify it's a watchdog ID
    if (TimerID == ADI_TMR_WDOG_TIMER) return (ADI_TMR_RESULT_SUCCESS);
    return (ADI_TMR_RESULT_BAD_TIMER_ID);
}


#endif


