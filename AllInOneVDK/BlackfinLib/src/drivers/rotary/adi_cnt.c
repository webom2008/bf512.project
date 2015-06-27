/******************************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_cnt.c,v $
$Revision: 2095 $
$Date: 2010-02-18 17:08:05 -0500 (Thu, 18 Feb 2010) $

Description:
    This is the driver source code for the Rotary Counter.

*******************************************************************************/

/*******************************************************************************

Include files

********************************************************************************/

#include <services/services.h>               /* system service includes        */
#include <drivers/adi_dev.h>                 /* device manager includes        */
#include <drivers/rotary/adi_cnt.h>         /* rotary counter driver includes */

/* Rotary counter is available only for Moab and Kookaburra/Mockingbird class devices */
#if !defined(__ADSP_MOAB__)        \
 && !defined(__ADSP_KOOKABURRA__)  \
 && !defined(__ADSP_MOCKINGBIRD__) \
 && !defined(__ADSP_BRODIE__) \
 && !defined(__ADSP_MOY__)
    #error "Rotary Counter is available only on ADSP-BF54x, ADSP-BF52x, ADSP-BF51x & ADSP-BF50x"
#endif

/******************************************************************************

Enumerations and defines

********************************************************************************/

/* number of Rotary Counter (cnt) in the system */
#define ADI_CNT_NUM_DEVICES        (sizeof(Device)/sizeof(ADI_CNT))

/******************************************************************************

Macros to manipulate selected bits/fields in CNT MMRs

********************************************************************************/

/* Macros to manipulate bits/fields of CNT configuration register   */
/* Set CNTE (Enables Counter) */
#define CNT_CONFIG_CNTE_ENABLED          (pDevice->pRegisters->CntConfig  |= 0x0001)
/* Clear CNTE (Disables Counter) */
#define CNT_CONFIG_CNTE_DISABLED        (pDevice->pRegisters->CntConfig  &= 0xBF72)
/* Set DEBE (Enables Debounce) */
#define CNT_CONFIG_DEBE_ENABLED          (pDevice->pRegisters->CntConfig  |= 0x0002)
/* Clear DEBE (Disables Debounce) */
#define CNT_CONFIG_DEBE_DISABLED        (pDevice->pRegisters->CntConfig  &= 0xBF71)
/* Set ZMZC ( enables CZM Zeroes Counter)*/
#define CNT_CONFIG_ZMZC_ENABLED          (pDevice->pRegisters->CntConfig  |= 0x0800)
/* Clear ZMZC ( disables CZM Zeroes Counter) */
#define CNT_CONFIG_ZMZC_DISABLED        (pDevice->pRegisters->CntConfig  &= 0xB773)
/* Set INPDIS (disables CUD and CDG input ) */
#define CNT_CONFIG_INPDIS_DISABLED        (pDevice->pRegisters->CntConfig  |= 0x8000)
/* Clear INPDIS (enables CUD and CDG input ) */
#define CNT_CONFIG_INPDIS_ENABLED      (pDevice->pRegisters->CntConfig  &= 0x3F73)
/* Clear CNTMODE (Counter Operating Mode ) */
#define CNT_CONFIG_CLEAR_CNTMODE     (pDevice->pRegisters->CntConfig  &= 0xB873)
/* Set CNTMODE (Counter Operating Mode ) */
#define CNT_CONFIG_SET_CNTMODE(val)    (pDevice->pRegisters->CntConfig  |= (val << 8))
/* Clear BNDMODE (Boundary Mode ) */
#define CNT_CONFIG_CLEAR_BNDMODE     (pDevice->pRegisters->CntConfig  &= 0x8F73)
/* Set BNDMODE (Boundary Mode ) */
#define CNT_CONFIG_SET_BNDMODE(val)    (pDevice->pRegisters->CntConfig  |= (val << 12))




/* Macros to manipulate bits/fields of CNT command register */
/* Set W1ZCNT (Command to zero CNT_COUNTER) */
#define CNT_CMD_ZERO_COUNTER          (pDevice->pRegisters->CntCommand  |= 0x0001)
/* Set W1LMIN (Command to load CNT_COUNTER from CNT_MIN) */
#define CNT_CMD_LOAD_COUNTER_FROM_MIN (pDevice->pRegisters->CntCommand  |= 0x0004)
/* Set W1LMAX (Command to load CNT_COUNTER from CNT_MAX) */
#define CNT_CMD_LOAD_COUNTER_FROM_MAX (pDevice->pRegisters->CntCommand  |= 0x0008)
/* Set W1ZMIN (Command to zero CNT_MIN) */
#define CNT_CMD_ZERO_MIN              (pDevice->pRegisters->CntCommand  |= 0x0010)
/* Set W1CMIN (Command to capture CNT_COUNTER to CNT_MIN) */
#define CNT_CMD_LOAD_COUNTER_TO_MIN   (pDevice->pRegisters->CntCommand  |= 0x0020)
/* Set W1NMIN (Command to copy former CNT_MAX to new CNT_MIN) */
#define CNT_CMD_LOAD_MAX_TO_MIN       (pDevice->pRegisters->CntCommand  |= 0x0080)
/* Set W1ZMAX (Command to zero CNT_MAX) */
#define CNT_CMD_ZERO_MAX              (pDevice->pRegisters->CntCommand  |= 0x0100)
/* Set W1CMAX (Command to capture CNT_COUNTER to CNT_MAX) */
#define CNT_CMD_LOAD_COUNTER_TO_MAX   (pDevice->pRegisters->CntCommand  |= 0x0200)
/* Set W1NMAX (Command to copy former CNT_MIN to new CNT_MAX) */
#define CNT_CMD_LOAD_MIN_TO_MAX       (pDevice->pRegisters->CntCommand  |= 0x0400)
/* Set W1ZMONCE (Command to single Zero marker clear CNT_COUNTER) */
#define CNT_CMD_CZM_CLEAR_COUNTER_ONCE (pDevice->pRegisters->CntCommand  |= 0x1000)

/* Get W1ZMONCE (Read W1ZMONCE bit of the CNT_COMMAND register ) */
#define CNT_CMD_GET_W1ZMONCE  (pDevice->pRegisters->CntCommand  & 0x1000) >> 12

/* Macros to manipulate bits/fields of CNT De-bounce register */
/* Get DPRESCALE (Read DPRESCALE bits of the CNT_DEBOUNCE register) */
#define CNT_CMD_GET_DPSCALE   (pDevice->pRegisters->CntDebounce  & 0x000F)


/* Shift count to reach Counter Operating Mode (CNTMODE) bits in CNT configuration register */
#define CNT_CONFIG_SHIFT_CNTMODE        8
/* Shift count to reach Boundary Register mode (BNDMODE) bits in CNT configuration register */
#define CNT_CONFIG_SHIFT_BNDMODE        12
/* Shift count to reach CDG Polarity Invert bit in CNT configuration register */
#define CNT_CONFIG_SHIFT_POLARITY       4

/******************************************************************************

Macros to mask reserved/selected bits in CNT MMRs

********************************************************************************/
#define CNT_MASK_CONFIG     0xBF73
#define CNT_MASK_IMASK      0x07FF
#define CNT_MASK_COMMAND    0x17BD
#if defined(__ADSP_MOAB__)
#define CNT_MASK_DEBOUNCE   0x000F
#else
#define CNT_MASK_DEBOUNCE   0x001F
#endif

/******************************************************************************

Tables for CNT Event ID's

********************************************************************************/
static ADI_CNT_CBSTATUS CbStatusBuf;


typedef struct {
    u32 EventID;
    ADI_CNT_CBSTATUS *pCntCbStatus;
}   ADI_CNT_CALLBACK_EVENT;


static ADI_CNT_CALLBACK_EVENT CBEvent[] = {
    { ADI_CNT_EVENT_ILLEGAL_CODE_INT, NULL      },  /* illegal gray/binary code int     */
    { ADI_CNT_EVENT_UPCOUNT_INT, &CbStatusBuf   },  /* upcount interrupt                */
    { ADI_CNT_EVENT_DOWNCOUNT_INT,&CbStatusBuf  },  /* downcount interrupt              */
    { ADI_CNT_EVENT_MINCOUNT_INT, &CbStatusBuf  },  /* CNT_COUNTER == CNT_MIN int       */
    { ADI_CNT_EVENT_MAXCOUNT_INT, &CbStatusBuf  },  /* CNT_COUNTER == CNT_MAX int       */
    { ADI_CNT_EVENT_OVERFLOW31_INT, NULL        },  /* Bit 31 overflow interrupt        */
    { ADI_CNT_EVENT_OVERFLOW15_INT, NULL        },  /* Bit 15 overflow interrupt        */
    { ADI_CNT_EVENT_COUNT_TO_ZERO_INT, NULL     },  /* CNT_COUNTER == 0x000.0000 int    */
    { ADI_CNT_EVENT_CZMPIN_INT, NULL            },  /* CZM pin/push button interrupt    */
    { ADI_CNT_EVENT_CZM_ERROR_INT, NULL         },  /* Zero marker error interrupt      */
    { ADI_CNT_EVENT_CZM_COUNTZERO_INT, NULL     },  /* Counter zeroes by zero marker int*/

};

#define NUM_OF_INT_EVENTS   11


/*******************************************************************************

Data Structures

********************************************************************************/

/* CNT register structure for Moab/Kookaburra/Mockingbird/Brodie/Moy devices      */
typedef struct {                /* structure representing CNT reg in memory     */
    volatile u16    CntConfig;  /* Configuration register                       */
    u16             Padding0;
    volatile u16    CntImask;   /* Interrupt mask register                      */
    u16             Padding1;
    volatile u16    CntStatus;  /* Status register                              */
    u16             Padding2;
    volatile u16    CntCommand; /* Command register                             */
    u16             Padding3;
    volatile u16    CntDebounce;/* Debounce register                            */
    u16             Padding4;
    volatile s32    CntCounter; /* Counter register                             */
    volatile s32    CntMax;     /* Maximal counter register                     */
    volatile s32    CntMin;     /* Minimal counter register                     */
}ADI_CNT_REGISTERS;

typedef struct {                                    /* CNT device structure                     */
   ADI_CNT_REGISTERS            *pRegisters;        /* Base address of PIXC registers           */
    void                        *pEnterCriticalArg; /* critical region argument                 */
    u8                          InUseFlag;          /* in use flag (in use when TRUE)           */
    ADI_INT_PERIPHERAL_ID       CntPeripheralID;    /* peripheral ID of CNT's status interrupt  */
    ADI_DEV_DEVICE_HANDLE       DeviceHandle;       /* Device manager handle                    */
    void (*DMCallback) (                            /* Device manager's Callback function       */
        ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* device handle                            */
        u32                     Event,              /* event ID                                 */
        void                    *pArg);             /* argument pointer                         */
    u32                         SCLK;               /* SCLK (system clock) frequency in Hz      */
} ADI_CNT;

/*******************************************************************************

Device specific data

*********************************************************************************/

/***************************
    MOAB
****************************/

 /* settings for Moab class devices  */
 #if defined(__ADSP_MOAB__)
static ADI_CNT Device[] = {                 /* Actual CNT devices               */
        (ADI_CNT_REGISTERS *)(0xffc04200),  /* CNT register base address        */
        NULL,                               /* critical region argument         */
        FALSE,                              /* in use flag                      */
        ADI_INT_COUNTER,                    /* CNT peripheral ID                */
        NULL,                               /* Device manager handle            */
        NULL,                               /* Device manager Callback function */
        0,                                  /* SCLK frequency                   */
};
#endif                                      /*__ADSP_MOAB__                     */

/***************************
    KOOKABURRA/MOCKINGBIRD/BRODIE
****************************/

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__)
static ADI_CNT Device[] = {                 /* Actual CNT devices               */
        (ADI_CNT_REGISTERS *)(0xffc03500),  /* CNT register base address        */
        NULL,                               /* critical region argument         */
        FALSE,                              /* in use flag                      */
        ADI_INT_COUNTER,                    /* CNT peripheral ID                */
        NULL,                               /* Device manager handle            */
        NULL,                               /* Device manager Callback function */
        0,                                  /* SCLK frequency                   */
};
#endif                                      /* Kookaburra or Mockingbird        */

/* settings for Moy class devices  */
#if defined(__ADSP_MOY__)

/* Actual Up/Down/Rotary Counter devices  */
static ADI_CNT Device[] =
{
    {
        (ADI_CNT_REGISTERS *)(0xFFC03500),  /* CNT register base address        */
        NULL,                               /* critical region argument         */
        FALSE,                              /* in use flag                      */
        ADI_INT_COUNTER0,                   /* CNT peripheral ID                */
        NULL,                               /* Device manager handle            */
        NULL,                               /* Device manager Callback function */
        0,                                  /* SCLK frequency                   */
    },
    {
        (ADI_CNT_REGISTERS *)(0xFFC03300),  /* CNT register base address        */
        NULL,                               /* critical region argument         */
        FALSE,                              /* in use flag                      */
        ADI_INT_COUNTER1,                   /* CNT peripheral ID                */
        NULL,                               /* Device manager handle            */
        NULL,                               /* Device manager Callback function */
        0,                                  /* SCLK frequency                   */
    },
};
#endif                                      /*__ADSP_MOY__                      */

/*********************************************************************************

Static functions

*********************************************************************************/
static u32 adi_pdd_Open(                            /* Open a device                    */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          /* device manager handle            */
    u32                     DeviceNumber,           /* device number                    */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           /* device handle                    */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            /* pointer to PDD handle location   */
    ADI_DEV_DIRECTION       Direction,              /* data direction                   */
    void                    *pCriticalRegionArg,    /* critical region imask location   */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              /* handle to the DMA manager        */
    ADI_DCB_HANDLE          DCBHandle,              /* callback handle                  */
    ADI_DCB_CALLBACK_FN     DMCallback              /* device manager callback function */
);

static u32 adi_pdd_Close(                           /* Closes a device                  */
    ADI_DEV_PDD_HANDLE PDDHandle                    /* PDD handle                       */
);

static u32 adi_pdd_Read(                            /* Reads data inbound to device     */
    ADI_DEV_PDD_HANDLE      PDDHandle,              /* PDD handle                       */
    ADI_DEV_BUFFER_TYPE     BufferType,             /* buffer type                      */
    ADI_DEV_BUFFER          *pBuffer                /* pointer to buffer                */
);

static u32 adi_pdd_Write(                           /* Writes data outbound to a device */
    ADI_DEV_PDD_HANDLE      PDDHandle,              /* PDD handle                       */
    ADI_DEV_BUFFER_TYPE     BufferType,             /* buffer type                      */
    ADI_DEV_BUFFER          *pBuffer                /* pointer to buffer                */
);

static u32 adi_pdd_SequentialIO(                    /* queue sequential buffer to device*/
    ADI_DEV_PDD_HANDLE      PDDHandle,              /* PDD handle                       */
    ADI_DEV_BUFFER_TYPE     BufferType,             /* buffer type                      */
    ADI_DEV_BUFFER          *pBuffer                /* pointer to buffer                */
);

static u32 adi_pdd_Control(                         /* Sets a device specific parameter */
    ADI_DEV_PDD_HANDLE      PDDHandle,              /* PDD handle                       */
    u32                     Command,                /* command ID                       */
    void                    *Value                  /* command specific value           */
);

static ADI_INT_HANDLER(InterruptHandler);           /* CNT interrupt handler            */

#if defined(__ADSP_MOAB__)
static u32 CounterPortControl( void);
#endif

#if defined(ADI_DEV_DEBUG)
static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);
#endif

/*******************************************************************************

Entry point for device manager

********************************************************************************/
ADI_DEV_PDD_ENTRY_POINT ADICNTEntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

/*******************************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens a CNT device for use
*
********************************************************************************/
static u32 adi_pdd_Open(                        /* Open CNT device              */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle        */
    u32                     DeviceNumber,       /* CNT Device number to open    */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer-PDD handle location  */
    ADI_DEV_DIRECTION       Direction,          /* data direction               */
    void                    *pEnterCriticalArg, /* critical region location     */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager    */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle              */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback func */
)
{

    u32         Result = ADI_DEV_RESULT_SUCCESS;/* Ret val - assume successful  */
    ADI_CNT     *pDevice;                       /* pointer to current device on */
    void        *pExitCriticalArg;              /* exit critical region paramer */
    u32         CntIvg;                         /* Counter IVG                  */

/* for Debug build only - check for errors if required*/
#if defined(ADI_DEV_DEBUG)
    if (DeviceNumber >= ADI_CNT_NUM_DEVICES)        /* check the device number  */
    {
        Result = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;  /* Invalid Device number    */
    }

    /*  Continue only if the device number is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif
        /* insure the device the client wants is available  */
        Result  = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* CNT device we're working on */
        pDevice = &Device[DeviceNumber];

        /* Protect this section of code - entering a critical region    */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
        /* Check the device usage status*/
        if (pDevice->InUseFlag == FALSE)
        {
            /* Device is not in use. Reserve the device for this client */
            pDevice->InUseFlag = TRUE;
            Result = ADI_DEV_RESULT_SUCCESS;

            /* CNT interrupt handle */
            /* get the CNT IVG      */
            adi_int_SICGetIVG(pDevice->CntPeripheralID, &CntIvg);

            /* hook the interrupt handler  */
            if(adi_int_CECHook(CntIvg, InterruptHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS)
            {
                pDevice->InUseFlag = FALSE;
                Result = ADI_DEV_RESULT_CANT_HOOK_INTERRUPT;
            }


        }
        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        /* Continue only when the CNT device is available for use  */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {

            /* Reset the actual device */
            /* Configure CNT MMRs to power-up default value */
            /* 16xcycles debounce filter time */
            pDevice->pRegisters->CntDebounce = 4;
            /* CUD & CDG input enable, QUAD_ENC mode, active high polarity,Debounce Enable */
            pDevice->pRegisters->CntConfig = 0x0002;
            /* enable zero-once to perform an initial reset of the counter value when zero marker is detected */
            pDevice->pRegisters->CntCommand = 0x1000;
            /* enable interrupt for CZM, Upcount and downcount */
            pDevice->pRegisters->CntImask = 0x0006;

            /* Pointer to critical region  */
            pDevice->pEnterCriticalArg  = pEnterCriticalArg;
            /* Pointer to Device handle */
            pDevice->DeviceHandle       = DeviceHandle;
            /* Pointer to Device manager callback function */
            pDevice->DMCallback         = DMCallback;

            /* save the physical device handle in the client supplied location */
            *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;

            /* allow this interrupt to wake us up and allow it to be passed to the core */
            adi_int_SICWakeup(pDevice->CntPeripheralID, TRUE);
            adi_int_SICEnable(pDevice->CntPeripheralID);


        }

#if defined(__ADSP_MOAB__)
        /* configure pins for Counter use */
        CounterPortControl();
#endif

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}

/*******************************************************************************
*
*   Function:       adi_pdd_Close
*
*   Description:    Closes down a CNT device
*
********************************************************************************/

static u32 adi_pdd_Close(                       /* Closes a CNT device          */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle CNT closed device */
)
{
    u32         Result = ADI_DEV_RESULT_SUCCESS;/* Ret val - assume successful  */
    ADI_CNT    *pDevice;                        /* pointers to the device       */
    u32         i;                              /* a temp location              */

#if defined(ADI_DEV_DEBUG)                      /* Debug only - check for errors*/
    Result = ValidatePDDHandle(PDDHandle);      /* Validate the given PDDHandle */
    if (Result == ADI_DEV_RESULT_SUCCESS)       /* Continue if PDDHandle OK     */
    {
#endif
        pDevice = (ADI_CNT *)PDDHandle;         /* avoid casts                  */

        /* Set CNT MMRs to power-on/reset values */
        pDevice->pRegisters->CntConfig      = 0;    /* disable counter  */
        pDevice->pRegisters->CntImask       = 0;    /* disable all interrupts */
        pDevice->pRegisters->CntStatus      = 0x7F; /* clear status */
        pDevice->pRegisters->CntCommand     = 0;
        pDevice->pRegisters->CntDebounce    = 0;
        pDevice->pRegisters->CntCounter     = 0;
        pDevice->pRegisters->CntMax         = 0;
        pDevice->pRegisters->CntMin         = 0;
        pDevice->InUseFlag                  = FALSE;

#if defined(ADI_DEV_DEBUG)                      /* for Debug build only         */
    }
#endif
    return(Result);                             /* return                       */

}

/*******************************************************************************

    Function:       adi_pdd_Read

    Description:    Processing of inbound buffer is not supported

********************************************************************************/

static u32 adi_pdd_Read(            /* Reads data to inbound buffer to a device */
    ADI_DEV_PDD_HANDLE  PDDHandle,  /* PDD handle                               */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type                              */
    ADI_DEV_BUFFER      *pBuffer    /* pointer to buffer                        */
){

    /* not supported  */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}




/*******************************************************************************

    Function:       adi_pdd_Write

    Description:    Processing of outbound buffer is not supported


********************************************************************************/

static u32 adi_pdd_Write(           /* Writes to outbound buffer to a device    */
    ADI_DEV_PDD_HANDLE  PDDHandle,  /* PDD handle                               */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type                              */
    ADI_DEV_BUFFER      *pBuffer    /* pointer to buffer                        */
){
    /* not supported  */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);


}

/*******************************************************************************

    Function:       adi_pdd_SequentialIO

    Description:    Processing of sequential buffer is not supported

********************************************************************************/

static u32 adi_pdd_SequentialIO(        /* provides buffer during sequential I/O*/
    ADI_DEV_PDD_HANDLE  PDDHandle,      /* PDD handle                           */
    ADI_DEV_BUFFER_TYPE BufferType,     /* buffer type                          */
    ADI_DEV_BUFFER      *pBuffer        /* pointer to buffer                    */
){

    /* not supported  */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);

}


/*******************************************************************************

    Function:       adi_pdd_Control

    Description:    Configures the CNT device registers

********************************************************************************/
static u32 adi_pdd_Control(             /* Sets device specific parameter       */
    ADI_DEV_PDD_HANDLE  PDDHandle,      /* PDD handle of a CNT device           */
    u32                 Command,        /* Command ID                           */
    void                *Value          /* Command specific value               */
){

    ADI_CNT            *pDevice;        /* pointer to the device                */
    u32                 Result;         /* return value                         */
    volatile s32        *ps32Reg;       /* pointer to signed 32 bit reg         */
    s32                 s32Value;       /* s32 type to avoid casts/warnings etc.*/
    u32                 u32Value;       /* u32 type to avoid casts/warnings etc.*/
    u16                 u16Value;       /* u16 type to avoid casts/warnings etc.*/
    s32                 s32temp;        /* temp s32 location                    */
    u32                 u32temp;        /* temp u32 location                    */
    u16                 u16temp;        /* temp u16 location                    */
    u32                 StatusIVG;      /* IVG for CNT status interrupts        */

    /* assume we're going to be successful */
    Result          = ADI_DEV_RESULT_SUCCESS;

    /* avoid casts */
    pDevice         = (ADI_CNT *)PDDHandle;
    /* assign 16 and 32 bit values for the Value argument */
    s32Value        = (s32)Value;
    u32Value        = (u32)Value;
    u16Value        = (u16)u32Value;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* Validate the given PDDHandle */
    Result = ValidatePDDHandle(PDDHandle);

    /* Continue only if the given PDDHandle is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* CASEOF (Command ID)  */
        switch (Command)
        {
            /* CASE: control dataflow */
            case (ADI_DEV_CMD_SET_DATAFLOW):
            /* CASE: set dataflow method */
            case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
                /* Do nothing & simply return back for these commands */
                break;
            /* CASE( query for processor DMA support) */
            case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
                /* CNT does not support DMA */
                *((u32 *)Value) = FALSE;
                break;
            /* CASE: Configure CNT registers? */
            default:
               /* Make sure that this command is ours */
                if ((Command <= ADI_CNT_CMD_START) || (Command >= ADI_CNT_CMD_END))
                {
                    /* CNT doesn't understand this command */
                    Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
                }
                /* CASEOF (Command ID)  */
                switch (Command)
                {
                    /* CASE: Set CNT configuration register (CNT_CONFIG) */
                    case (ADI_CNT_CMD_SET_CONFIG_REG):
                        pDevice->pRegisters->CntConfig       = (u16Value & CNT_MASK_CONFIG);
                        break;
                    /* CASE: Set CNT Interrupt enable register (CNT_IMASK) */
                    case (ADI_CNT_CMD_SET_IMASK_REG):
                        pDevice->pRegisters->CntImask       = (u16Value & CNT_MASK_IMASK);
                        break;
                    /* CASE: Set CNT Command register (CNT_COMMAND) */
                    case (ADI_CNT_CMD_SET_COMMAND_REG):
                        pDevice->pRegisters->CntCommand       = (u16Value & CNT_MASK_COMMAND);
                        break;

                   /* CASE: Set CNT De-bounce register (CNT_DEBOUNCE) */
                    case (ADI_CNT_CMD_SET_DEBOUNCE_REG):
                        pDevice->pRegisters->CntDebounce       = (u16Value & CNT_MASK_DEBOUNCE);
                        break;
                    /* CASE: Set CNT Max count register (CNT_MAX) */
                    case (ADI_CNT_CMD_SET_MAX_REG):
                        pDevice->pRegisters->CntMax       = s32Value;
                        break;
                    /* CASE: Set CNT Min count register (CNT_MIN) */
                    case (ADI_CNT_CMD_SET_MIN_REG):
                        pDevice->pRegisters->CntMin       = s32Value;
                        break;
                    /* CASE: Set CNT Enable bit */
                    case (ADI_CNT_CMD_SET_CNT_ENABLE):
                        if (u16Value == TRUE)
                        {
                            /* Enable Counter */
                            CNT_CONFIG_CNTE_ENABLED;
                        }
                        else
                        {
                            /* Disable Counter */
                            CNT_CONFIG_CNTE_DISABLED;
                        }
                        break;

                    /* CASE: Set CNT Debounce enable bit */
                    case (ADI_CNT_CMD_SET_DEBOUNCE_ENABLE):
                        if (u16Value == TRUE)
                        {
                            /* Enable debounce */
                            CNT_CONFIG_DEBE_ENABLED;
                        }
                        else
                        {
                            /* Disable Counter */
                            CNT_CONFIG_DEBE_DISABLED;
                        }
                        break;

                    /* CASE: disable CUD and CDG input */
                    case (ADI_CNT_CMD_CUD_CDG_DISABLE):
                        if (u16Value == TRUE)
                        {
                            /* CUD and CDG input disable */
                            CNT_CONFIG_INPDIS_DISABLED;
                        }
                        else
                        {
                            /* enable */
                            CNT_CONFIG_INPDIS_ENABLED;
                        }
                        break;

                    /* CASE: Set CZM zeroes counter enable bit */
                    case (ADI_CNT_CMD_SET_ZMZC_ENABLE):
                        if (u16Value == TRUE)
                        {
                            /* enable active CZM pin zeroes CNT_COUNTER */
                            CNT_CONFIG_ZMZC_ENABLED;
                        }
                        else
                        {
                            /* disable */
                            CNT_CONFIG_ZMZC_DISABLED;
                        }
                        break;


                    /* CASE: Set CDG pin polarity to active low,falling edge */
                    case (ADI_CNT_CMD_SET_CDG_POL_ALOW):
                    /* CASE: Set CUD pin polarity to active low,falling edge */
                    case (ADI_CNT_CMD_SET_CUD_POL_ALOW):
                    /* CASE: Set CZM pin polarity to active low,falling edge */
                    case (ADI_CNT_CMD_SET_CZM_POL_ALOW):
                        /* generate shift count to reach the corresponding bit location in CNT_CONFIG */
                        u16temp = ((Command - ADI_CNT_CMD_SET_CDG_POL_ALOW) + CNT_CONFIG_SHIFT_POLARITY);
                        /* set the corresponding CNT_CONFIG bit */
                        pDevice->pRegisters->CntConfig  |= (1 << u16temp);

                        break;
                    /* CASE: Set CDG pin polarity to active high, rising edge */
                    case (ADI_CNT_CMD_SET_CDG_POL_AHIGH):
                    /* CASE: Set CUD pin polarity to active high, rising edge */
                    case (ADI_CNT_CMD_SET_CUD_POL_AHIGH):
                    /* CASE: Set CZM pin polarity to active high, rising edge */
                    case (ADI_CNT_CMD_SET_CZM_POL_AHIGH):
                        /* generate shift count to reach the corresponding bit location in CNT_CONFIG */
                        u16temp = ((Command - ADI_CNT_CMD_SET_CDG_POL_AHIGH) + CNT_CONFIG_SHIFT_POLARITY);
                        /* clear the corresponding CNT_CONFIG bit */
                        pDevice->pRegisters->CntConfig  &= (~(1 << u16temp));

                        break;

                    /* CASE: Set Counter Operating Mode */
                     case (ADI_CNT_CMD_SET_CNTMODE):
                            /* check for errors if required */
#if defined(ADI_DEV_DEBUG)
                            if (u16Value == 3 || u16Value > ADI_CNT_DIR_TMR )
                            {
                                Result = ADI_CNT_RESULT_CNTMODE_INVALID;
                                break;
                            }
#endif
                        /* clear the Counter Mode bits field */
                        CNT_CONFIG_CLEAR_CNTMODE;

                        /* set the Counter Mode of CNT_CONFIG register*/
                        CNT_CONFIG_SET_CNTMODE(u16Value);
                        break;


                    /* CASE: Set Boundary register mode */
                   case (ADI_CNT_CMD_SET_BNDMODE):
                        /* check for errors if required */
                    #if defined(ADI_DEV_DEBUG)
                        if (u16Value > ADI_CNT_BND_AEXT )
                        {
                            Result = ADI_CNT_RESULT_CNTMODE_INVALID;
                            break;
                        }
                    #endif
                     /* clear the Boundary Mode bits field */
                    CNT_CONFIG_CLEAR_BNDMODE;

                    /* set the Boundary Mode of CNT_CONFIG register*/
                    CNT_CONFIG_SET_BNDMODE(u16Value);
                    break;


                    case (ADI_CNT_CMD_ILLEGAL_CODE_INT_EN): /* Illegal gray/binary code interrupt enable */
                    case (ADI_CNT_CMD_UPCOUNT_INT_EN):      /* Upcount interrupt enable     */
                    case (ADI_CNT_CMD_DOWNCOUNT_INT_EN):    /* Downcount interrupt enable   */
                    case (ADI_CNT_CMD_MINCOUNT_INT_EN):     /* Min count interrupt enable   */
                    case (ADI_CNT_CMD_MAXCOUNT_INT_EN):     /* Max count interrupt enable   */
                    case (ADI_CNT_CMD_OVERFLOW31_INT_EN):
                    case (ADI_CNT_CMD_OVERFLOW15_INT_EN):
                    case (ADI_CNT_CMD_COUNTZERO_INT_EN):
                    case (ADI_CNT_CMD_CZMPIN_INT_EN):
                    case (ADI_CNT_CMD_CZM_ERROR_INT_EN):
                    case (ADI_CNT_CMD_CZM_COUNTZERO_INT_EN):
                        /* generate shift count to reach the corresponding bit location in CNT_IMASK */
                        u16temp = (Command - ADI_CNT_CMD_ILLEGAL_CODE_INT_EN);
                        /* set the corresponding CNT_IMASK bit */
                        pDevice->pRegisters->CntImask  |= (1 << u16temp);

                        if(Command == ADI_CNT_CMD_CZM_COUNTZERO_INT_EN)
                        {
                            CNT_CONFIG_ZMZC_ENABLED;
                        }

                        break;

                    case (ADI_CNT_CMD_ILLEGAL_CODE_INT_DIS):/* Illegal gray/binary code interrupt disable */
                    case (ADI_CNT_CMD_UPCOUNT_INT_DIS):     /* Upcount interrupt disable    */
                    case (ADI_CNT_CMD_DOWNCOUNT_INT_DIS):   /* Downcount interrupt disable  */
                    case (ADI_CNT_CMD_MINCOUNT_INT_DIS):    /* Min count interrupt disable  */
                    case (ADI_CNT_CMD_MAXCOUNT_INT_DIS):    /* Max count interrupt disable  */
                    case (ADI_CNT_CMD_OVERFLOW31_INT_DIS):
                    case (ADI_CNT_CMD_OVERFLOW15_INT_DIS):
                    case (ADI_CNT_CMD_COUNTZERO_INT_DIS):
                    case (ADI_CNT_CMD_CZMPIN_INT_DIS):
                    case (ADI_CNT_CMD_CZM_ERROR_INT_DIS):
                    case (ADI_CNT_CMD_CZM_COUNTZERO_INT_DIS):
                        /* generate shift count to reach the corresponding bit location in CNT_IMASK */
                        u16temp = (Command - ADI_CNT_CMD_ILLEGAL_CODE_INT_DIS);
                        /* clear the corresponding CNT_IMASK bit */
                        pDevice->pRegisters->CntImask  &= (~(1 << u16temp));

                        if(Command == ADI_CNT_CMD_CZM_COUNTZERO_INT_DIS)
                        {
                            CNT_CONFIG_ZMZC_DISABLED;
                         }

                         break;

                             /* command to perform actions of CNT_COMMAND register */
                    case (ADI_CNT_CMD_ZERO_CNT_COUNTER):
                         CNT_CMD_ZERO_COUNTER;
                         break;

                    case (ADI_CNT_CMD_LOAD_MIN_TO_COUNTER):
                    case (ADI_CNT_CMD_LOAD_MAX_TO_COUNTER):
                    case (ADI_CNT_CMD_ZERO_CNT_MIN):
                    case (ADI_CNT_CMD_COUNTER_TO_CNT_MIN):
                        /* generate shift count to reach the corresponding bit location in CNT_COMMAND */
                        u16temp = (Command - ADI_CNT_CMD_LOAD_MIN_TO_COUNTER) + 2;
                        /* clear the corresponding CNT_COMMAND bit */
                        pDevice->pRegisters->CntCommand  |= (1 << u16temp);
                        break;

                    case (ADI_CNT_CMD_CNT_MAX_TO_CNT_MIN):
                    case (ADI_CNT_CMD_ZERO_CNT_MAX):
                    case (ADI_CNT_CMD_COUNTER_TO_CNT_MAX):
                    case (ADI_CNT_CMD_CNT_MIN_TO_CNT_MAX):
                        /* generate shift count to reach the corresponding bit location in CNT_COMMAND */
                        u16temp = (Command - ADI_CNT_CMD_CNT_MAX_TO_CNT_MIN) + 7;
                        /* clear the corresponding CNT_COMMAND bit */
                        pDevice->pRegisters->CntCommand  |= (1 << u16temp);
                        break;

                    case (ADI_CNT_CMD_CZM_CLEAR_COUNTER_ONCE):
                    CNT_CMD_CZM_CLEAR_COUNTER_ONCE;
                    break;


                    /* CASE: (get a 16 bit STATUS register) */
                    case (ADI_CNT_CMD_GET_STATUS_REG):
                    /* get it */
                    *(u16 *)Value = (pDevice->pRegisters->CntStatus  &= 0x07FF);
                    break;

                    /* CASE: (get a 32 bit COUNTER register) */
                    case (ADI_CNT_CMD_GET_COUNTER_REG):
                    /* get it */
                    *(u32 *)Value = (pDevice->pRegisters->CntCounter);
                    break;

                    /* CASE: (get a 32 bit MAX register) */
                    case (ADI_CNT_CMD_GET_MAX_REG):
                    /* get it */
                    *(u32 *)Value = (pDevice->pRegisters->CntMax);
                    break;

                    /* CASE: (get a 32 bit MIN register) */
                    case (ADI_CNT_CMD_GET_MIN_REG):
                    /* get it */
                    *(u32 *)Value = (pDevice->pRegisters->CntMin);
                    break;



                /* ENDCASE for CNT register configuration Command switch*/
                }

                break;
        /* ENDCASE*/
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}


/*******************************************************************************

    Function:       InterruptHandler

    Description:    Processes events in response to CNT interrupts

********************************************************************************/

static ADI_INT_HANDLER(InterruptHandler)
{
    ADI_CNT *pDevice; /* CNT device */
    bool PostCallback = false;
    ADI_INT_HANDLER_RESULT Result;
    u16 u16temp;
    u32 i,a;

    /* assume the interrupt is not for us */
    Result = ADI_INT_RESULT_NOT_PROCESSED;
    /* point to the CNT */
    pDevice = (ADI_CNT *)ClientArg;
    /* IF (this device is asserting an interrupt) */
        if (adi_int_SICInterruptAsserted(pDevice->CntPeripheralID) == ADI_INT_RESULT_ASSERTED)
        {
            /* clear the callback buffer */
            CbStatusBuf.CntCounter = 0;

            /* temporary store for counter status */
            u16temp = pDevice->pRegisters->CntStatus;
            /* mask off unwanted status bits with the imask reg */
            u16temp = u16temp & pDevice->pRegisters->CntImask;

            for ( i = 0; i <(NUM_OF_INT_EVENTS) ; i++)
            {
                /* check each of the interrupt pending bits */
                if (u16temp & 0x0001)
                {
                    /* clear the status bit(write-1-to-clear) */
                    pDevice->pRegisters->CntStatus |= ( 1 << i);
                    /* get the Event ID and update Event result */
                    if ( CBEvent[i].pCntCbStatus != NULL)
                    {
                        CbStatusBuf.CntCounter = pDevice->pRegisters->CntCounter;
                    }
                    (pDevice->DMCallback)(pDevice->DeviceHandle, CBEvent[i].EventID, CBEvent[i].pCntCbStatus);
                }
                /* if no more interrupt event to process */
                if (u16temp == 0)
                break;
                /* shift the next bit */
                u16temp = u16temp >> 1;
            }


    }
    /* return */
    return (Result);
}

/*******************************************************************************

    Function:       CounterPortControl

    Description:    Set port D to configure PD8-PD15 as Keypad pins(Row0-3, Col0-3)


********************************************************************************/

#if defined(__ADSP_MOAB__)

static u32 CounterPortControl( void)
{
    u32 Result;         /* return code */

   /* Directives to enable Counter device */
   ADI_PORTS_DIRECTIVE CntDirectives[] =
   {
    ADI_PORTS_DIRECTIVE_CNT_CUD,
    ADI_PORTS_DIRECTIVE_CNT_CDG,
    ADI_PORTS_DIRECTIVE_CNT_CZM
   };

    /* assume we're going to be successful */
    Result = ADI_PORTS_RESULT_SUCCESS;

    /* Call port control to enable CNT inputs */
    Result = adi_ports_Configure(&CntDirectives[0],3);


    /* return */
    return(Result);
}

#endif

#if defined(ADI_DEV_DEBUG)

/*******************************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

********************************************************************************/
static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle)
{
    unsigned int i, Result;

    Result = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    for (i = 0; i < ADI_CNT_NUM_DEVICES; i++)
    {
        if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i])
        {
            Result = ADI_DEV_RESULT_SUCCESS;
        }
    }

    return (Result);
}

#endif

