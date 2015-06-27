/*****************************************************************************
Copyright (c), 2002-2009 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

$RCSfile: adi_ad7147.c,v $
$Revision: 131 $
$Date: 2009-11-04 15:40:56 -0500 (Wed, 04 Nov 2009) $

Title: AD7147 CapTouch Controller Driver

Description: This is the primary source file for AD7147 CapTouch
             Controller Driver. The driver supports Device access commands
             to access AD7147 registers

*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* system service includes */
#include <services/services.h>

/* device manager includes */
#include <drivers/adi_dev.h>
#include <drivers/deviceaccess/adi_device_access.h>     // adi_device_access driver includes

/* AD7147 driver includes */
#include <drivers/captouch/adi_ad7147.h>



/*=============  D E F I N E S  =============*/

// Number of AD7147 devices in the system
#define ADI_AD7147_NUM_DEVICES (sizeof(gAD7147ArrayOfCapTouchDevices)/sizeof(AD7147_CAP_TOUCH_DEV_STRUCT))

// AD7147-I TWI hardware info struct
typedef struct {
	u32 twiDevIdx;                              // TWI Device inxed used for AD7147 register access
	u32 twiDevAddr;                             // TWI address of the AD7147 device
	ADI_DEV_CMD_VALUE_PAIR *pTwiConfigTable;    // TWI Configuration Table specific to the application
} AD7147_TWI_STRUCT;


// Invalid TWI device number identifier
#define     ADI_AD7147_TWI_DEVICE_NUM_INVALID   0xFFU


// Invalid TWI device address identifier
#define     ADI_AD7147_TWI_DEVICE_ADDR_INVALID   0x00U


// AD7147 Device instance structure
#pragma pack(4)
typedef struct {
    bool                         bDeviceOpen;         // Device usage status flag. true when device is in use
    bool                         bInterruptsEnabled;  // Interrupt enable flag tracking flag
    ADI_AD7147_INTERRUPT_PORT    pIrqStruct;          // Port info for AD7147 INT interrupt signal
    AD7147_TWI_STRUCT            pTwiStruct;          // TWI info specific for AD7147
    ADI_DEV_MANAGER_HANDLE       ppDevMgr;            // Device Manager Handle
    ADI_DEV_DEVICE_HANDLE        ppDevice;            // Device Handle allocated by the Device Manager
    ADI_DCB_HANDLE               ppDcbManager;        // Deferred Callback Manager Handle
    ADI_DCB_CALLBACK_FN          pfDevMgrCallback;    // Pointer to callback function supplied by the Device Manager
} AD7147_CAP_TOUCH_DEV_STRUCT;
#pragma pack()


/*=============  D A T A  =============*/

/*
** gAD7147ArrayOfCapTouchDevices
**  - AD7147 Device instance
*/
static AD7147_CAP_TOUCH_DEV_STRUCT    gAD7147ArrayOfCapTouchDevices[] = {

    // Device 0 (the singular 7147 device instance)
    {
        FALSE,                                      // Device is not in use
        FALSE,                                      // Interrupting is disabled

        /* ADI_AD7147_INTERRUPT_PORT... */
        {                                           // AD7147 INT interrupt port struct...
            ADI_FLAG_UNDEFINED,
            (ADI_INT_PERIPHERAL_ID) 0,
        },

        /* AD7147_TWI_STRUCT... */
        {                                          // AD7147 TWI struct...
            ADI_AD7147_TWI_DEVICE_NUM_INVALID,     // AD7147 TWI device number
            ADI_AD7147_TWI_DEVICE_ADDR_INVALID,    // AD7147 TWI device address
            NULL,                                  // Default configuration table
        },

        NULL,                                      // Device Manager Handle
        NULL,                                      // Device Handle allocated by the Device Manager
        NULL,                                      // Deferred Callback Manager Handle
        NULL,                                      // Pointer to callback function supplied by the Device Manager
    },
};


// fixed device access parameters, passed a/p/o AccessParams
static ADI_DEVICE_ACCESS_SELECT SelectParams = {

    0,                            // SPI chip select (not used here)
    ADI_DEVICE_ACCESS_LENGTH0,    // SPI global address length (not used here)
    ADI_DEVICE_ACCESS_LENGTH2,    // register address length is 16-bits (2-byte)
    ADI_DEVICE_ACCESS_LENGTH2,    // register data length is 16-bits (2-byte)
    ADI_DEVICE_ACCESS_TYPE_TWI    // generic TWI access
};



/*
**
**  Local Function Prototypes
**
*/

/* Opens AD7147 device */
static u32  adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE      ppDevMgr,
    u32                         devIndex,
    ADI_DEV_DEVICE_HANDLE       ppDevice,
    ADI_DEV_PDD_HANDLE          *ppPhyDev,
    ADI_DEV_DIRECTION           eDirection,
    void                        *pCriticalRegionArg,
    ADI_DMA_MANAGER_HANDLE      ppDmaManager,
    ADI_DCB_HANDLE              ppDcbManager,
    ADI_DCB_CALLBACK_FN         pfDevMgrCallback
);

/* Closes AD7147 device */
static u32  adi_pdd_Close(
    ADI_DEV_PDD_HANDLE          pPhyDev
);

/* Not supported for AD7147 device */
static u32  adi_pdd_Read(
    ADI_DEV_PDD_HANDLE          pPhyDev,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poRxBuffer
);

/* Not supported for AD7147 device */
static u32  adi_pdd_Write(
    ADI_DEV_PDD_HANDLE          pPhyDev,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poTxBuffer
);

/* Not supported for AD7147 device */
static u32  adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE          pPhyDev,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poSeqBuffer
);

/* Sets or senses device specific parameter */
static u32  adi_pdd_Control(
    ADI_DEV_PDD_HANDLE          pPhyDev,
    u32                         cmdID,
    void                        *pVal
);


/* Callback for AD7147 Interrupts */
static void AD7147InterruptCallback (
    void    *phClient,
    u32     nEvent,
    void    *pArg
);


/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

/* Validates Physical Device Driver Handle */
static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      ppdHandle
);
#endif


/*
** ADIAD7147EntryPoint
**  - Device Manager Entry point for AD7147 driver
*/
ADI_DEV_PDD_ENTRY_POINT     ADIAD7147EntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};


/*
    TWI Device access service configuration
*/
static ADI_DEV_CMD_VALUE_PAIR DevAccessTWIConfig[] = {

    /* Dataflow is always Seq IO    */
    { ADI_DEV_CMD_SET_DATAFLOW_METHOD,  (void *)ADI_DEV_MODE_SEQ_CHAINED    },

    /* Enable TWI Dataflow          */
    { ADI_DEV_CMD_SET_DATAFLOW,         (void *)TRUE                        },

     /* End of command table         */
    { ADI_DEV_CMD_END,                  (void *)NULL                        }
};


/*********************************************************************
Table for Register Field Error check and Register field access
Table structure -   'Count' of Register addresses containing individual fields
                    Register address containing individual fields,
                    Register field locations in the corresponding register,
                    Reserved bit locations in the corresponding register
Register field location - Bit indicating start of new field in a register will be 1
Reserved bit locations will be 1
*********************************************************************/

/* AD7147_ register address to perform Register Field Error check and Register field access */
static u16 AD7147_BitfieldRegArray[] = {

    // registers with bitfields...

    /* Bank 1 registers */
    AD7147_REG_PWR_CONTROL,
    AD7147_REG_STAGE_CAL_EN,
    AD7147_REG_AMB_COMP_CTRL0,
    AD7147_REG_AMB_COMP_CTRL1,
    AD7147_REG_AMB_COMP_CTRL2,
    AD7147_REG_STAGE_LOW_INT_EN,
    AD7147_REG_STAGE_HIGH_INT_EN,
    AD7147_REG_STAGE_COMPLETE_INT_EN,
    AD7147_REG_STAGE_LOW_INT_STATUS,
    AD7147_REG_STAGE_HIGH_INT_STATUS,
    AD7147_REG_STAGE_COMPLETE_INT_STATUS,
    AD7147_REG_DEVID,
    AD7147_REG_PROX_STATUS,

    /* Bank 2 registers */
    AD7147_REG_STG0_CIN0_6_SETUP,  AD7147_REG_STG0_CIN7_12_SETUP,  AD7147_REG_STG0_AFE_OFFSET,  AD7147_REG_STG0_SENSITIVITY,
    AD7147_REG_STG1_CIN0_6_SETUP,  AD7147_REG_STG1_CIN7_12_SETUP,  AD7147_REG_STG1_AFE_OFFSET,  AD7147_REG_STG1_SENSITIVITY,
    AD7147_REG_STG2_CIN0_6_SETUP,  AD7147_REG_STG2_CIN7_12_SETUP,  AD7147_REG_STG2_AFE_OFFSET,  AD7147_REG_STG2_SENSITIVITY,
    AD7147_REG_STG3_CIN0_6_SETUP,  AD7147_REG_STG3_CIN7_12_SETUP,  AD7147_REG_STG3_AFE_OFFSET,  AD7147_REG_STG3_SENSITIVITY,
    AD7147_REG_STG4_CIN0_6_SETUP,  AD7147_REG_STG4_CIN7_12_SETUP,  AD7147_REG_STG4_AFE_OFFSET,  AD7147_REG_STG4_SENSITIVITY,
    AD7147_REG_STG5_CIN0_6_SETUP,  AD7147_REG_STG5_CIN7_12_SETUP,  AD7147_REG_STG5_AFE_OFFSET,  AD7147_REG_STG5_SENSITIVITY,
    AD7147_REG_STG6_CIN0_6_SETUP,  AD7147_REG_STG6_CIN7_12_SETUP,  AD7147_REG_STG6_AFE_OFFSET,  AD7147_REG_STG6_SENSITIVITY,
    AD7147_REG_STG7_CIN0_6_SETUP,  AD7147_REG_STG7_CIN7_12_SETUP,  AD7147_REG_STG7_AFE_OFFSET,  AD7147_REG_STG7_SENSITIVITY,
    AD7147_REG_STG8_CIN0_6_SETUP,  AD7147_REG_STG8_CIN7_12_SETUP,  AD7147_REG_STG8_AFE_OFFSET,  AD7147_REG_STG8_SENSITIVITY,
    AD7147_REG_STG9_CIN0_6_SETUP,  AD7147_REG_STG9_CIN7_12_SETUP,  AD7147_REG_STG9_AFE_OFFSET,  AD7147_REG_STG9_SENSITIVITY,
    AD7147_REG_STG10_CIN0_6_SETUP, AD7147_REG_STG10_CIN7_12_SETUP, AD7147_REG_STG10_AFE_OFFSET, AD7147_REG_STG10_SENSITIVITY,
    AD7147_REG_STG11_CIN0_6_SETUP, AD7147_REG_STG11_CIN7_12_SETUP, AD7147_REG_STG11_AFE_OFFSET, AD7147_REG_STG11_SENSITIVITY
};

/* Register Field locations corresponding to AD7147_BitfieldRegArray (***LS bit of each bitfield set as 1***)*/
static u16 AD7147_BitfieldPosArray [] = {

    // bitfield positions (set bits mark LS field positon)...

    /* Bank 1 registers */
    0x5D15, 0x5FFF, 0xD111, 0x4101,
    0x0401, 0x5FFF, 0x0FFF, 0x1FFF,
    0x0FFF, 0x0FFF, 0x1FFF, 0x0011,
    0x0FFF,

    /* Bank 2 registers */
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111,
    0x1555, 0xD555, 0x8181, 0x1111
};

static ADI_DEVICE_ACCESS_REGISTER_FIELD AD7147_BitfieldRegTable[] = {

    sizeof(AD7147_BitfieldRegArray)/sizeof(AD7147_BitfieldRegArray[0]),    // 'Count' of Register addresses containing individual fields
    AD7147_BitfieldRegArray,                                               // array of register addresses containing bitfields
    AD7147_BitfieldPosArray                                                // array of register bitfield positions in the corresponding registers
};


/*********************************************************************
Table to configure the reserved bits in the device to its recommended values
Table structure -  'Count' of Register addresses containing Reserved Locations
     Register address containing Reserved Locations
     Recommended value for the Reserved Locations
*********************************************************************/
/* AD7147_ register address having reserved locations */
static u16 AD7147_ReservedBitRegArray [] =
{
    // registers with reserved fields...

    /* Bank 1 registers */
    AD7147_REG_PWR_CONTROL, AD7147_REG_STAGE_HIGH_INT_EN, AD7147_REG_STAGE_COMPLETE_INT_EN,
    AD7147_REG_STAGE_LOW_INT_STATUS, AD7147_REG_STAGE_HIGH_INT_STATUS, AD7147_REG_STAGE_COMPLETE_INT_STATUS,
    AD7147_REG_PROX_STATUS,

    /* Bank 2 registers */
    AD7147_REG_STG0_CIN0_6_SETUP,  AD7147_REG_STG0_AFE_OFFSET,  AD7147_REG_STG0_SENSITIVITY,
    AD7147_REG_STG1_CIN0_6_SETUP,  AD7147_REG_STG1_AFE_OFFSET,  AD7147_REG_STG1_SENSITIVITY,
    AD7147_REG_STG2_CIN0_6_SETUP,  AD7147_REG_STG2_AFE_OFFSET,  AD7147_REG_STG2_SENSITIVITY,
    AD7147_REG_STG3_CIN0_6_SETUP,  AD7147_REG_STG3_AFE_OFFSET,  AD7147_REG_STG3_SENSITIVITY,
    AD7147_REG_STG4_CIN0_6_SETUP,  AD7147_REG_STG4_AFE_OFFSET,  AD7147_REG_STG4_SENSITIVITY,
    AD7147_REG_STG5_CIN0_6_SETUP,  AD7147_REG_STG5_AFE_OFFSET,  AD7147_REG_STG5_SENSITIVITY,
    AD7147_REG_STG6_CIN0_6_SETUP,  AD7147_REG_STG6_AFE_OFFSET,  AD7147_REG_STG6_SENSITIVITY,
    AD7147_REG_STG7_CIN0_6_SETUP,  AD7147_REG_STG7_AFE_OFFSET,  AD7147_REG_STG7_SENSITIVITY,
    AD7147_REG_STG8_CIN0_6_SETUP,  AD7147_REG_STG8_AFE_OFFSET,  AD7147_REG_STG8_SENSITIVITY,
    AD7147_REG_STG9_CIN0_6_SETUP,  AD7147_REG_STG9_AFE_OFFSET,  AD7147_REG_STG9_SENSITIVITY,
    AD7147_REG_STG10_CIN0_6_SETUP, AD7147_REG_STG10_AFE_OFFSET, AD7147_REG_STG10_SENSITIVITY,
    AD7147_REG_STG11_CIN0_6_SETUP, AD7147_REG_STG11_AFE_OFFSET, AD7147_REG_STG11_SENSITIVITY

};

/* Reserved Bit locations corresponding to the entries in AD7147_ReservedBitRegArray */
static u16 AD7147_ReservedBitMaskArray [] = {

	// reserved bit mask (NOT bitfield positions)...

    /* Bank 1 registers */
    0x2000, 0xF000, 0xE000,
    0xF000, 0xF000, 0xE000,
    0xF000,

    /* Bank 2 registers */
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080,
    0xC000, 0x4040, 0x8080
};

/* Reserved Bit default values corresponding to the entries in AD7147_ReservedBitRegArray */
static u16 AD7147_ReservedBitValArray [] = {

    // reserved bit values are all zero

    /* Bank 1 registers */
    0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000,
    0x0000,

    /* Bank 2 registers */
    0x0000, 0x0000, 0x0000,
    0x0000, 0x0000,	0x0000,
    0x0000, 0x0000,	0x0000,
    0x0000, 0x0000,	0x0000,
    0x0000, 0x0000,	0x0000,
    0x0000, 0x0000,	0x0000,
    0x0000, 0x0000,	0x0000,
    0x0000, 0x0000,	0x0000,
    0x0000, 0x0000,	0x0000,
    0x0000, 0x0000,	0x0000,
    0x0000, 0x0000,	0x0000,
    0x0000, 0x0000,	0x0000,
};


static ADI_DEVICE_ACCESS_RESERVED_VALUES AD7147_ReservedBitTable[] = {

    sizeof(AD7147_ReservedBitRegArray)/sizeof(AD7147_ReservedBitRegArray[0]),    // 'Count' of reserved register addresses containing Reserved Locations
    AD7147_ReservedBitRegArray,                                                  // array of reserved register addresses containing bitfields
    AD7147_ReservedBitMaskArray,                                                 // array of reserved bitfield values in the corresponding registers
    AD7147_ReservedBitValArray                                                   // array of Recommended values for the Reserved Bit locations
};


/*********************************************************************
Table for error checking of invalid & read-only register(s) access
Structure - 'Count' of invalid Register addresses in AD7147_
            Invalid Register addresses in AD7147_
            'Count' of Read-only Register addresses in AD7147_
            Read-only Register addresses in AD7147_
*********************************************************************/

static u16 AD7147_InvalidRegs [] = {

    // Bank1 invalid register addresses
                                                    0x018,0x019,0x01A,0x01B,0x01C,0x01D,0x01E,0x01F,
    0x020,0x021,0x022,0x023,0x024,0x025,0x026,0x027,0x028,0x029,0x02A,0x02B,0x02C,0x02D,0x02E,0x02F,
    0x030,0x031,0x032,0x033,0x034,0x035,0x036,0x037,0x038,0x039,0x03A,0x03B,0x03C,0x03D,0x03E,0x03F,
    0x040,0x041,      0x043,0x044,0x045,0x046,0x047,0x048,0x049,0x04A,0x04B,0x04C,0x04D,0x04E,0x04F,
    0x050,0x051,0x052,0x053,0x054,0x055,0x056,0x057,0x058,0x059,0x05A,0x05B,0x05C,0x05D,0x05E,0x05F,
    0x060,0x061,0x062,0x063,0x064,0x065,0x066,0x067,0x068,0x069,0x06A,0x06B,0x06C,0x06D,0x06E,0x06F,
    0x070,0x071,0x072,0x073,0x074,0x075,0x076,0x077,0x078,0x079,0x07A,0x07B,0x07C,0x07D,0x07E,0x07F,

    // Bank3 invalid register addresses
    0x103, 0x127, 0x14B, 0x16F,
    0x193, 0x1B7, 0x1DB, 0x1FF,
    0x223, 0x247, 0x26B, 0x28F

};

static u16 AD7147_ReadOnlyRegs [] = {

    // only Bank1 has read-only registers
    AD7147_REG_STAGE_LOW_INT_STATUS,
    AD7147_REG_STAGE_HIGH_INT_STATUS,
    AD7147_REG_STAGE_COMPLETE_INT_STATUS,
    AD7147_REG_CDCRESULT_S0,
    AD7147_REG_CDCRESULT_S1,
    AD7147_REG_CDCRESULT_S2,
    AD7147_REG_CDCRESULT_S3,
    AD7147_REG_CDCRESULT_S4,
    AD7147_REG_CDCRESULT_S5,
    AD7147_REG_CDCRESULT_S6,
    AD7147_REG_CDCRESULT_S7,
    AD7147_REG_CDCRESULT_S8,
    AD7147_REG_CDCRESULT_S9,
    AD7147_REG_CDCRESULT_S10,
    AD7147_REG_CDCRESULT_S11,
    AD7147_REG_DEVID,
    AD7147_REG_PROX_STATUS
};


static ADI_DEVICE_ACCESS_VALIDATE_REGISTER AD7147_ValidateTable [] = {

    sizeof(AD7147_InvalidRegs)/sizeof(AD7147_InvalidRegs[0]),      // 'Count' of invalid register addresses
    AD7147_InvalidRegs,                                            // Pointer to array of invalid register addresses

    sizeof(AD7147_ReadOnlyRegs)/sizeof(AD7147_ReadOnlyRegs[0]),    // 'Count' of read-only register addresses
    AD7147_ReadOnlyRegs                                            // Pointer to array of read-only register addresses
};


/*********************************************************************

    Function:       adi_pdd_Open

    Description:    Opens AD7147 device

    Parameters:
     ppDevMgr              - Device Manager handle
     devIndex              - AD7147 device number to open
     ppDevice              - Device Handle allocated by the Device Manager
     ppPhyDev              - Physical Device Handle
     eDirection            - Data Direction
     pCriticalRegionArg    - Critical region parameter
     ppDmaManager          - DMA Manager Handle
     ppDcbManager          - DCB Manager Handle
     pfDMCallback          - Pointer to Device Manager supplied Callback function

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
     - ADI_DEV_RESULT_BAD_DEVICE_NUMBER when the given
       device number is invalid
     - ADI_DEV_RESULT_BAD_MANAGER_HANDLE when the given
       device manager handle/DMA manager handle is invalid
     - ADI_DEV_RESULT_DEVICE_IN_USE if the device number is
       already in use

*********************************************************************/
static u32 adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE      ppDevMgr,
    u32                         devIndex,
    ADI_DEV_DEVICE_HANDLE       ppDevice,
    ADI_DEV_PDD_HANDLE          *ppPhyDev,
    ADI_DEV_DIRECTION           eDirection,
    void                        *pCriticalRegionArg,
    ADI_DMA_MANAGER_HANDLE      ppDmaManager,
    ADI_DCB_HANDLE              ppDcbManager,
    ADI_DCB_CALLBACK_FN         pfDMCallback
)
{
    /* Return value - assume we're going to be successful */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* local pointer to the device entry we're opening */
    AD7147_CAP_TOUCH_DEV_STRUCT *pCapTouchDev;

    /* exit critical region parameter */
    void *pRestoreState;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* check the device number */
    if (devIndex >= ADI_AD7147_NUM_DEVICES) {

        /* Invalid Device number */
        Result = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }

    /*  Continue only if the device number is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS) {
#endif

        /* insure the device the client wants is available  */
        Result = ADI_DEV_RESULT_DEVICE_IN_USE;

        // get a pointer to the correct global static array member
        pCapTouchDev = &gAD7147ArrayOfCapTouchDevices[devIndex];

        /* Protect this section of code against interrupts - entering a critical region    */
        pRestoreState = adi_int_EnterCriticalRegion(pCriticalRegionArg);

        /* Check the device usage status */
        if (pCapTouchDev->bDeviceOpen == FALSE) {

            /* Device is not in use. Reserve the device for this client */
            pCapTouchDev->bDeviceOpen = TRUE;
            Result = ADI_DEV_RESULT_SUCCESS;
        }

        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pRestoreState);

        /* Continue only when the AD7147 device is available for use  */
        if (Result == ADI_DEV_RESULT_SUCCESS) {

            /* initialize the device settings */
            pCapTouchDev->bInterruptsEnabled = FALSE;         // start off with interrupts disabled
            pCapTouchDev->ppDevMgr           = ppDevMgr;      // Pointer to Device manager handle
            pCapTouchDev->ppDevice           = ppDevice;      // Pointer to Device handle
            pCapTouchDev->ppDcbManager       = ppDcbManager;  // Deffered Callback manager handle
            pCapTouchDev->pfDevMgrCallback   = pfDMCallback;  // Pointer to Device manager callback function

            /* store it to client-supplied handle for subsequent access */
            *ppPhyDev = (ADI_DEV_PDD_HANDLE *)pCapTouchDev;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(Result);
}

/*********************************************************************

    Function:       adi_pdd_Close

    Description:    Closes AD7147 device

    Parameters:
     pPhyDev        - Physical Device pointer to AD7147 device instance

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
     - Error code returned from remove flag callback function
     - Error code returned by device access function

*********************************************************************/
static u32  adi_pdd_Close(
    ADI_DEV_PDD_HANDLE          pPhyDev
)
{
    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* pointer to the device we're working on */
    AD7147_CAP_TOUCH_DEV_STRUCT *pCapTouchDev;


/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given device handle */
    Result = ValidatePDDHandle(pPhyDev);

    /* Continue only if the given device handle is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS) {

#endif

        /* avoid casts */
        pCapTouchDev = (AD7147_CAP_TOUCH_DEV_STRUCT *)pPhyDev;

        /* IF (INT interrupt is enabled & monitored by this driver) */
        if (pCapTouchDev->bInterruptsEnabled == TRUE) {

            /* Disable INT Interrupt Flag callback  */
            Result = adi_flag_RemoveCallback(pCapTouchDev->pIrqStruct.eFlagId);

        }

        if (Result == ADI_DEV_RESULT_SUCCESS) {

            /* Disable AD7147 */
            /* Clear the interrupt enable indicator */
            pCapTouchDev->bInterruptsEnabled = FALSE;

            /* mark this AD7147 device as closed */
            pCapTouchDev->bDeviceOpen = FALSE;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(Result);
}

/*********************************************************************

    Function:       adi_pdd_Read

    Description:    Never called as AD7147 uses Device Access commands
                    to access its internal registers

    Parameters:
     pPhyDev        - Physical Device handle to AD7147 device instance
     eBufferType    - Buffer type to queue
     pRxBuffer      - Pointer to receive buffer

    Returns:
     - ADI_DEV_RESULT_NOT_SUPPORTED as this function is not supported
       by this device

*********************************************************************/
static u32  adi_pdd_Read(
    ADI_DEV_PDD_HANDLE          pPhyDev,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *pRxBuffer
)
{
    /* return error (not supported) */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function:       adi_pdd_Write

    Description:    Never called as AD7147 uses Device Access commands
                    to access its internal registers

    Parameters:
     pPhyDev        - Physical Device handle to AD7147 device instance
     eBufferType    - Buffer type to queue
     pTxBuffer      - Pointer to transmit buffer

    Returns:
     - ADI_DEV_RESULT_NOT_SUPPORTED as this function is not supported
       by this device

*********************************************************************/
static u32  adi_pdd_Write(
    ADI_DEV_PDD_HANDLE          pPhyDev,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *pTxBuffer
)
{
    /* return error (not supported) */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function:       adi_pdd_SequentialIO

    Description:    Never called as AD7147 uses Device Access commands
                    to access its internal registers

    Parameters:
     pPhyDev        - Physical Device handle to AD7147 device instance
     eBufferType    - Buffer type to queue
     pSeqBuffer     - Pointer to the sequential buffer

    Returns:
     - ADI_DEV_RESULT_NOT_SUPPORTED as this function is not supported
       by this device

*********************************************************************/
static u32  adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE          pPhyDev,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *pSeqBuffer
)
{
    /* return error (not supported) */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function:       adi_pdd_Control

    Description:    Sets or senses device specific parameter

    Parameters:
     pPhyDev    - Physical Device handle to AD7147 device instance
     cmdID      - Command ID
     pVal       - Command specific value

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
*********************************************************************/
static u32  adi_pdd_Control(
    ADI_DEV_PDD_HANDLE          pPhyDev,
    u32                         cmdID,
    void                        *pVal
)
{

    /* Return value - assume we're going to be successful */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* local pointer to avoid casts */
    AD7147_CAP_TOUCH_DEV_STRUCT *pCapTouchDev = (AD7147_CAP_TOUCH_DEV_STRUCT *) pPhyDev;

    // Structure passed to device access service (to be filled in)
    ADI_DEVICE_ACCESS_REGISTERS AccessParams;

    /* interrupt ports for flag and interrupt designations */
    ADI_AD7147_INTERRUPT_PORT *pIrqStruct;

/* for Debug build only - check for errors if required */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given PDDHandle */
    Result = ValidatePDDHandle(pPhyDev);

    /* Continue only if the given PDDHandle is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS) {
#endif

        /* CASEOF (Command ID)  */
        switch (cmdID) {

            /* CASE (Control dataflow) */
            case ADI_DEV_CMD_SET_DATAFLOW:
            case ADI_DEV_CMD_SET_DATAFLOW_METHOD:

                /* Do nothing & simply return back for these commands */
                break;


            /* CASE (Query for processor DMA support) */
            case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:

                /* AD7147 does not support DMA */
                *((u32 *)pVal) = FALSE;
                break;


            /* CASE (Set AD7147 TWI Device Number to use) */
            case ADI_AD7147_CMD_SET_TWI_DEVICE_NUMBER:

                /* Save the TWI device number to use */
                pCapTouchDev->pTwiStruct.twiDevIdx   = (u8)((u32)pVal);
                break;


            /* CASE (Set AD7147 TWI Device Address to use) */
            case ADI_AD7147_CMD_SET_TWI_DEVICE_ADDRESS:

                /* Save the TWI device number to use */
                pCapTouchDev->pTwiStruct.twiDevAddr   = (u8)((u32)pVal);
                break;


            /* CASE (Set TWI Configuration table specific to the application) */
            case ADI_AD7147_CMD_SET_TWI_CONFIG_TABLE:

                /* Save the TWI command table pointer */
                pCapTouchDev->pTwiStruct.pTwiConfigTable   = (ADI_DEV_CMD_VALUE_PAIR*)pVal;
                break;


            /* CASE: (Set driver to monitor INT interrupts) */
            case ADI_AD7147_CMD_INSTALL_INTIRQ:

                /* IF (interrupts are alreayd configured) */
                if (pCapTouchDev->pIrqStruct.eFlagId != ADI_FLAG_UNDEFINED ) {

                    /* Remove existing callback */
                     adi_flag_RemoveCallback(pCapTouchDev->pIrqStruct.eFlagId);

                    /* Interrupt monitoring is disabled for this flag pin */
                     pCapTouchDev->bInterruptsEnabled = FALSE;
                }

                /* Map CDCIRQ Interrupt to this Flag */
                pIrqStruct = (ADI_AD7147_INTERRUPT_PORT*)pVal;


                /* IF (INT interrupt monitoring is disabled) */
                if (pCapTouchDev->bInterruptsEnabled == FALSE) {

                    /* Open corresponding Blackfin flag */
                    if ((Result = adi_flag_Open (pIrqStruct->eFlagId)) != ADI_FLAG_RESULT_SUCCESS) {

                        /* exit on error */
                        break;
                    }

                    /* Set flag direction as input */
                    if ((Result = adi_flag_SetDirection(pIrqStruct->eFlagId, ADI_FLAG_DIRECTION_INPUT)) != ADI_FLAG_RESULT_SUCCESS) {

                        /* exit on error */
                        break;
                    }

                    // install new callback...

                    /* Senses on Falling edge, Parameters are
                        - Flag ID, Flag Peripheral interrupt ID,
                          Flag Trigger mode, Is this a wake-up flag,
                          Client Handle, Callback Manager, Callback Function */

                    Result = adi_flag_InstallCallback (pIrqStruct->eFlagId,
                                                       pIrqStruct->eFlagIntId,
                                                       ADI_FLAG_TRIGGER_FALLING_EDGE,
                                                       true,
                                                       (void *)pCapTouchDev,
                                                       pCapTouchDev->ppDcbManager,
                                                       AD7147InterruptCallback);
                }

                /* IF (Successfully installed callback on interrupt flag) */
                if (Result == ADI_DEV_RESULT_SUCCESS) {

                    pCapTouchDev->bInterruptsEnabled = TRUE;

                    /* Save the Flag info */
                    pCapTouchDev->pIrqStruct.eFlagId = pIrqStruct->eFlagId;
                    pCapTouchDev->pIrqStruct.eFlagIntId = pIrqStruct->eFlagIntId;

                }
                break;


            /* CASE (Unistall INTIRQ (Remove INTIRQ from monitoring)( */
            case ADI_AD7147_CMD_UNINSTALL_INTIRQ:

                if (pCapTouchDev->bInterruptsEnabled == TRUE) {

                    /* Remove callback installed on the
                       interrupt monitoring flag pin */
                    Result = adi_flag_RemoveCallback(pCapTouchDev->pIrqStruct.eFlagId);
                    pCapTouchDev->bInterruptsEnabled = FALSE;
                }

                break;


            /* CASE: disable INTIRQ interrupt */
            case ADI_AD7147_CMD_DISABLE_INTIRQ:

                Result = adi_int_SICDisable(pCapTouchDev->pIrqStruct.eFlagIntId);
                break;


            /* CASE: Reenable INTIRQ interrupt */
            case ADI_AD7147_CMD_REENABLE_INTIRQ:
                Result = adi_int_SICEnable(pCapTouchDev->pIrqStruct.eFlagIntId);
                break;


            /*
            ** Device access commands to Access AD7147 registers
            */
            case ADI_DEV_CMD_REGISTER_READ:
            case ADI_DEV_CMD_REGISTER_WRITE:
            case ADI_DEV_CMD_REGISTER_FIELD_READ:
            case ADI_DEV_CMD_REGISTER_FIELD_WRITE:
            case ADI_DEV_CMD_REGISTER_BLOCK_READ:
            case ADI_DEV_CMD_REGISTER_BLOCK_WRITE:
            case ADI_DEV_CMD_REGISTER_TABLE_READ:
            case ADI_DEV_CMD_REGISTER_TABLE_WRITE:
            case ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ:
            case ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE:

                AccessParams.ManagerHandle    = pCapTouchDev->ppDevMgr;               /* device manager handle */
                AccessParams.ClientHandle     = pCapTouchDev;                         /* client handle - passed to the internal 'Device' specific function */
                AccessParams.DeviceNumber     = pCapTouchDev->pTwiStruct.twiDevIdx;   /* TWI device number */
                AccessParams.DeviceAddress    = pCapTouchDev->pTwiStruct.twiDevAddr;  /* TWI address of ADV717x Device */
                AccessParams.DCBHandle        = pCapTouchDev->ppDcbManager;           /* handle to the callback manager */
                AccessParams.DeviceFunction   = NULL;                                 /* DA callback to modify device modes for specific reg address */
                AccessParams.Command          = cmdID;                                /* command ID */
                AccessParams.Value            = pVal;                                 /* command specific value */
                AccessParams.FinalRegAddr     = ADI_AD7147_END_REG_ADDR;              /* Address of the last register in ADV717x */
                AccessParams.RegisterField    = AD7147_BitfieldRegTable;              /* table of ADV717x Register Field Error check and Register field access */
                AccessParams.ReservedValues   = AD7147_ReservedBitTable;              /* table to configure reserved bits in ADV717x to recommended values */
                AccessParams.ValidateRegister = AD7147_ValidateTable;                 /* table containing invalid and read-only validation registers in ADV717x */
                AccessParams.ConfigTable      = pCapTouchDev->pTwiStruct.pTwiConfigTable;  /* TWI configuration table */
                AccessParams.SelectAccess     = &SelectParams;                        /* Device Access type */
                AccessParams.pAdditionalinfo  = (void *)NULL;                         /* No Additional info */

               /* call device access */
                Result = adi_device_access (&AccessParams);
                break;

            /* DEFAULT: Command not supported */
            default:

                Result = ADI_AD7147_RESULT_CMD_NOT_SUPPORTED;
                break;

        }
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(Result);
}

/*********************************************************************

    Function:       AD7147InterruptCallback

    Description:    Callback routine for AD7147 PENIRQ/INT Interrupt

    Parameters:
     phClient       - Client handle passed to flag service
     nEvent         - Callback Event
     pArg           - Callback Argument

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
*********************************************************************/
static void AD7147InterruptCallback (
    void    *phClient,
    u32     nEvent,
    void    *pArg
)
{
    /* Retrive the device instance to which this callback belongs */
    AD7147_CAP_TOUCH_DEV_STRUCT *pCapTouchDev = (AD7147_CAP_TOUCH_DEV_STRUCT *)phClient;

/* Debug build only */
#ifdef ADI_DEV_DEBUG
    /* verify the Device handle */
    if (ValidatePDDHandle(phClient) == ADI_DEV_RESULT_SUCCESS) {
#endif

       /* post callback to client indicating that INTIRQ has occurred */
       /* parameters are Device Handle, Event, NULL */
         (pCapTouchDev->pfDevMgrCallback)(pCapTouchDev->ppDevice, ADI_AD7147_EVENT_INT_IRQ, NULL);

      //adi_int_SICEnable(pCapTouchDev->poIrqPort->eFlagIntId);

/* Debug build only */
#ifdef ADI_DEV_DEBUG
    }
#endif

}



#if defined(ADI_DEV_DEBUG)  /* Debug build only */
/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      handle   /* pointer to a specific AD7147 device instance */
)
{
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     Result = ADI_DEV_RESULT_BAD_PDD_HANDLE;

    /* index */
    u32     i;

    /* compare the given PDD handle with PDDHandle allocated to all EPPI devices in the list    */
    for (i = 0; i < ADI_AD7147_NUM_DEVICES; i++) {

        if (handle == (ADI_DEV_PDD_HANDLE)&gAD7147ArrayOfCapTouchDevices[i]) {

            /* Given pointer is valid. quit this loop */
             Result = ADI_DEV_RESULT_SUCCESS;
            break;
        }
    }

    /* return */
    return (Result);
}
#endif

