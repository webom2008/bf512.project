/*****************************************************************************
Copyright (c), 2009 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

$RCSfile: adi_adp5520.c,v $
$Revision: 2095 $
$Date: 2010-02-18 17:08:05 -0500 (Thu, 18 Feb 2010) $

Title:       ADP5520 Backlight Driver with I/O Expander

Description: This is the primary source file for ADP5520 Backlight Driver
             with I/O Expander.  The software driver supports Device access 
             commands to access ADP5520 registers.

Notes:       Access to the ADP5520 control registers is over the TWI port
             using the Device Access Service.

             Although the software driver enables use of all the ADP5520's
             features the ADP-BF527 EZ-Kit only uses the keypad decoder.

*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* system services */
#include <services/services.h>
/* device manager */
#include <drivers/adi_dev.h>
/* device access service (TWI/SPI-based register access) */
#include <drivers/deviceaccess/adi_device_access.h>
/* ADP5520 driver include */
#include <drivers/keypad/adi_adp5520.h>


/*=============  D E F I N E S  =============*/

/*
** ADI_ADP5520_NUM_DEVICES
**  - Number of ADP5520 devices in the system
*/
#define ADI_ADP5520_NUM_DEVICES (sizeof(ADP5520Device) / \
                                 sizeof(ADI_DD_ADP5520_DEF))

#define ADP5520_FINAL_REG_ADDRESS ADP5520_REG_KEY_STAT_2

/*
** ADI_ADP5520_REGISTER_READ
**  - ADP5520 register read request
*/
#define ADI_ADP5520_REGISTER_READ                1
/*
** ADI_ADP5520_REGISTER_WRITE
**  - ADP5520 register write request
*/
#define ADI_ADP5520_REGISTER_WRITE               0

#if defined(ADI_DEV_DEBUG)
#define SAVE_MODE_STATUS(dev) save_mode_status(dev)
#else
#define SAVE_MODE_STATUS(dev)
#endif
/*
**
** ADP5520-I TWI hardware
**
*/
typedef struct {
    /* TWI Device number used for ADP5520 register access */
    u8                          nTwiDevNumber;
    /* TWI address of the ADP5520 device */
    u8                          nTwiDevAddress;
    /* TWI Configuration Table specific to the application */
    ADI_DEV_CMD_VALUE_PAIR      *poTwiConfigTable;
} ADI_ADP5520_TWI;

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
/*
** ADI_ADP5520_TWI_DEVICE_NUM_INVALID
**  - Invalid TWI device number identifier
*/
#define     ADI_ADP5520_TWI_DEVICE_NUM_INVALID   0xFFU
/*
** ADI_ADP5520_TWI_DEVICE_ADDR_INVALID
**  - Invalid TWI device address identifier
*/
#define     ADI_ADP5520_TWI_DEVICE_ADDR_INVALID  0x00U

/*
** Identifiers for various ADP5520 features
*/
#define F_KEYPAD_KP (1 << 0)
#define F_KEYPAD_KR (1 << 1)
#define F_LED2      (1 << 2)
#define F_LED3      (1 << 3)
#define F_GPIO      (1 << 4)
#define F_BACKLIGHT (1 << 5)

/*
** Keypad event masks 
*/
#define KP_INT  (1 << ADP5520_POS_KP_INT)
#define KR_INT  (1 << ADP5520_POS_KR_INT)

/*
** ADI_DD_ADP5520_DEF
**  - ADP5520 Device instance structure
*/
#pragma pack(4)
typedef struct
{
    bool    bIsInUse;            /* Device status - true when device is open */
    void    *pCriticalRegionArg; /* Mutex information supplied by caller */

    ADI_ADP5520_INTERRUPT_INFO   /* ADP5520 and processor interrupt info */
            oIrqInfo;

    ADI_ADP5520_TWI              /* TWI addressing information for ADP5520 */
            oADP5520Twi;

    ADI_DEV_MANAGER_HANDLE       /* Application's device manager handle */
            hDeviceManager;

    ADI_DEV_DEVICE_HANDLE        /* This ADP5520's device handle */
            hDevice;

    ADI_DCB_HANDLE               /* Application's deferred callback handle */
            hDcbManager;

    ADI_DCB_CALLBACK_FN          /* Device Manager's callback function */
            pfDeviceManagerCallback;

    u32     enabled_features;    /* bitmap of currently enabled features */

#if defined(ADI_DEV_DEBUG)
    u8      mode_status;         /* contents of ADP5520_REG_MODE_STATUS */
#endif
} ADI_DD_ADP5520_DEF;
#pragma pack()

/*=============  D A T A  =============*/

/*
** ADP5520Device
**  - ADP5520 Device instance
*/
static ADI_DD_ADP5520_DEF    ADP5520Device[] =
{
    /* Device 0 */
    {
        false,                                 /* Device is not in use        */
        NULL,                                  /* No critical region arg      */

        {   /*  ADP5520 interrupt info */
            0,                                 /* ADP5520 interrupts disabled */
            ADI_FLAG_UNDEFINED,                /* No processor flag specified */
            (ADI_INT_PERIPHERAL_ID) 0,
        },

        /*  ADP5520 TWI */
        {
            ADI_ADP5520_TWI_DEVICE_NUM_INVALID, /* ADP5520 TWI device number  */
            ADI_ADP5520_TWI_DEVICE_ADDR_INVALID,/* ADP5520 TWI device address */
            NULL,                               /* Default config table       */
        },

        NULL,                                   /* Device Manager Handle      */
        NULL,                                   /* Device handle              */
        NULL,                                   /* DCB handle                 */
        NULL,                                   /* Device Manager callback fn */
    },
};


/*
**
**  Local Function Prototypes
**
*/

/* Opens ADP5520 device */
static u32  adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE      hDeviceManager,
    u32                         nDeviceNumber,
    ADI_DEV_DEVICE_HANDLE       hDevice,
    ADI_DEV_PDD_HANDLE          *phPhysicalDevice,
    ADI_DEV_DIRECTION           eDirection,
    void                        *pCriticalRegionArg,
    ADI_DMA_MANAGER_HANDLE      hDmaManager,
    ADI_DCB_HANDLE              hDcbManager,
    ADI_DCB_CALLBACK_FN         pfDeviceManagerCallback
);

/* Closes ADP5520 device */
static u32  adi_pdd_Close(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice
);

/* Not supported for ADP5520 device */
static u32  adi_pdd_Read(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poRxBuffer
);

/* Not supported for ADP5520 device */
static u32  adi_pdd_Write(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poTxBuffer
);

/* Not supported for ADP5520 device */
static u32  adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poSeqBuffer
);

/* Sets or senses device specific parameter/register */
static u32  adi_pdd_Control(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    u32                         nCommandID,
    void                        *pCommandValue
);

/* interrupt processing routines */
static void process_keys(
    ADI_DD_ADP5520_DEF  *pDevice,
    u32                 status_register,
    u32                 feature,
    u32                 event,
    u32                 status_field
);

static void process_keypad_interrupt(
    ADI_DD_ADP5520_DEF  *pDevice,
    u32                 status
);
static void process_gpio_interrupt(
    ADI_DD_ADP5520_DEF  *pDevice,
    u32                 status
);
static void process_light_sensor_interrupt(
    ADI_DD_ADP5520_DEF  *pDevice,
    u32                 status
);
static void process_overvoltage_interrupt(
    ADI_DD_ADP5520_DEF  *pDevice,
    u32                 status
);

/* Callback for ADP5520 nINT Interrupts */
static void ADP5520IntCallback(
    void    *phClient,
    u32     nEvent,
    void    *pArg
);

/* Sets up and actions call to device access service */
static u32 access_ADP5520_registers(
    ADI_DD_ADP5520_DEF  *pDevice,  /* pointer to the device we're working on  */
    u32                 nCommand,  /* Command ID from the client              */
    void                *Value     /* Command specific value                  */
);

/* enables some or all ADP5520 interrupt sources */
static u32 enable_ADP5520_interrupts(
    ADI_DD_ADP5520_DEF         *pDevice,   /* device we're working on         */
    ADI_ADP5520_INTERRUPT_INFO *poIrqInfo  /* holds interrupts to be enabled  */
);

/* disables some or all ADP5520 interrupt sources */
static u32 disable_ADP5520_interrupts(
    ADI_DD_ADP5520_DEF         *pDevice,   /* device we're working on         */
    ADI_ADP5520_INTERRUPT_INFO *poIrqInfo  /* holds interrupts to be disabled */
);

/* puts ADP5520 into quiescent state */
static void reset_ADP5520(
    ADI_DD_ADP5520_DEF  *pDevice   /* pointer to the device we're working on  */
);

/* sets up and enables keypad scanning */
static u32 enable_keypad(
    ADI_DD_ADP5520_DEF  *pDevice,  /* pointer to the device we're working on  */
    u32                 events     /* requested events (press and/or release) */
);

/* disables keypad scanning */
static u32 disable_keypad(
    ADI_DD_ADP5520_DEF  *pDevice   /* pointer to the device we're working on  */
);

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

/* Validates Physical Device Driver Handle */
static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      PDDHandle
);

/* reads ADP5520_REG_MODE_STATUS and saves value in device structure */
static void save_mode_status(
    ADI_DD_ADP5520_DEF  *pDevice   /* pointer to the device we're working on  */
);

#endif

/*********************************************************************
Table for Register Field Error check and Register field access

Table structure
    Count of registers containing individual fields
    Array of addresses of those registers
    Array of register field locations within those registers

Register field location
    Bit indicating least significant end of each field is 1
*********************************************************************/

/* ADP5520 registers containing bitfields (i.e. most of them) */
static u16 ADP5520_BitfieldRegs[] =
{
    ADP5520_REG_MODE_STATUS,
    ADP5520_REG_INTERRUPT_ENABLE,
    ADP5520_REG_BL_CONTROL,
    ADP5520_REG_BL_TIME,
    ADP5520_REG_BL_FADE,
    ADP5520_REG_DAYLIGHT_MAX,
    ADP5520_REG_DAYLIGHT_DIM,
    ADP5520_REG_OFFICE_MAX,
    ADP5520_REG_OFFICE_DIM,
    ADP5520_REG_DARK_MAX,
    ADP5520_REG_DARK_DIM,
    ADP5520_REG_BL_VALUE,
    ADP5520_REG_ALS_CMPR_CFG,
    ADP5520_REG_LED_CONTROL,
    ADP5520_REG_LED_TIME,
    ADP5520_REG_LED_FADE,
    ADP5520_REG_LED1_CURRENT,
    ADP5520_REG_LED2_CURRENT,
    ADP5520_REG_LED3_CURRENT,
    ADP5520_REG_GPIO_CFG_1,
    ADP5520_REG_GPIO_CFG_2,
    ADP5520_REG_GPIO_IN,
    ADP5520_REG_GPIO_OUT,
    ADP5520_REG_GPIO_INT_EN,
    ADP5520_REG_GPIO_INT_STAT,
    ADP5520_REG_GPIO_INT_LVL,
    ADP5520_REG_GPIO_DEBOUNCE,
    ADP5520_REG_GPIO_PULLUP,
    ADP5520_REG_KP_INT_STAT_1,
    ADP5520_REG_KP_INT_STAT_2,
    ADP5520_REG_KR_INT_STAT_1,
    ADP5520_REG_KR_INT_STAT_2,
    ADP5520_REG_KEY_STAT_1,
    ADP5520_REG_KEY_STAT_2,
};

/* A bitfield map for each entry in ADP5520_BitfieldRegs.  The lsb of each
** bitfield in a register is set to 1.
*/
static u16 ADP5520_BitfieldMap[] =
{
    0x00FF,
    0x003F,
    0x005F,
    0x0011,
    0x0011,
    0x0081,
    0x0081,
    0x0081,
    0x0081,
    0x0081,
    0x0081,
    0x0081,
    0x003F,
    0x007F,
    0x0055,
    0x0011,
    0x0041,
    0x0041,
    0x0041,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
    0x00FF,
};

static ADI_DEVICE_ACCESS_REGISTER_FIELD ADP5520_BitfieldTable[] = {
    /* count of bitfield-containing registers */
    sizeof(ADP5520_BitfieldRegs)/sizeof(ADP5520_BitfieldRegs[0]),
    /* array of addresses of bitfield-containing registers */
    ADP5520_BitfieldRegs,
    /* array of field locations in the bitfield-containing registers */
    ADP5520_BitfieldMap
};


/*********************************************************************
Table for reserved bit configuration

Table structure
    Count of registers containing reserved bits
    Array of addresses of those registers
    Array of reserved bit locations within those registers
    Array of reserved bit values within those registers

Reserved bit location and value
    Reserved bit locations are indicated by a '1'
    Reserved bit values are just a bit pattern of '1' and '0' as appropriate
*********************************************************************/

/* ADP5520 registers having reserved locations */
static u16 ADP5520_ReservedBitRegs[] =
{
    ADP5520_REG_INTERRUPT_ENABLE,
    ADP5520_REG_DAYLIGHT_MAX,
    ADP5520_REG_DAYLIGHT_DIM,
    ADP5520_REG_OFFICE_MAX,
    ADP5520_REG_OFFICE_DIM,
    ADP5520_REG_DARK_MAX,
    ADP5520_REG_DARK_DIM,
    ADP5520_REG_BL_VALUE,
    ADP5520_REG_LED_CONTROL,
    ADP5520_REG_LED1_CURRENT,
    ADP5520_REG_LED2_CURRENT,
    ADP5520_REG_LED3_CURRENT,
};

/* A reserved bit map for each entry in ADP5520_ReservedBitRegs.  Each reserved
** bit in a register is indicated by a 1.
*/
static u16 ADP5520_ReservedBitPos[] =
{
    0x00E0,
    0x0080,
    0x0080,
    0x0080,
    0x0080,
    0x0080,
    0x0080,
    0x0080,
    0x00C0,
    0x00C0,
    0x00C0,
    0x00C0,
};

/* Reserved bit values for each entry in ADP5520_ReservedBitRegs.  Each reserved
** bit location is either '0' or '1' as appropriate (all '0' for ADP5520).
*/
static u16 ADP5520_ReservedBitVal[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static ADI_DEVICE_ACCESS_RESERVED_VALUES ADP5520_ReservedBitTable[] = {
    /* count of reserved-bit-containing registers */
    sizeof(ADP5520_ReservedBitRegs)/sizeof(ADP5520_ReservedBitRegs[0]),
    /* array of addresses of reserved-bit-containing registers */
    ADP5520_ReservedBitRegs,
    /* array of reserved bit location maps */
    ADP5520_ReservedBitPos,
    /* array of reserved bit recommended values */
    ADP5520_ReservedBitVal
};


/*********************************************************************
Table for invalid register access checks

Structure - Count of invalid register addresses
            Array of invalid register addresses
            Count of read-only register addresses
            Array of read-only register addresses
*********************************************************************/

static u16 ADP5520_ReadOnlyRegs[] =
{
    ADP5520_REG_BL_VALUE,
    ADP5520_REG_GPIO_IN,
    ADP5520_REG_GPIO_INT_STAT,
    ADP5520_REG_KP_INT_STAT_1,
    ADP5520_REG_KP_INT_STAT_2,
    ADP5520_REG_KR_INT_STAT_1,
    ADP5520_REG_KR_INT_STAT_2,
    ADP5520_REG_KEY_STAT_1,
    ADP5520_REG_KEY_STAT_2
};

static ADI_DEVICE_ACCESS_VALIDATE_REGISTER ADP5520_ReadonlyTable[] =
{
    /* count of invalid register addresses */
    0,
    /* array of invalid register addresses */
    NULL,
    /* count of read-only registers */
    sizeof(ADP5520_ReadOnlyRegs)/sizeof(ADP5520_ReadOnlyRegs[0]),
    /* array of addresses of read-only registers */
    ADP5520_ReadOnlyRegs
};

/*
** ADIADP5520EntryPoint
**  - Device Manager Entry point for ADP5520 driver
*/
ADI_DEV_PDD_ENTRY_POINT ADIADP5520EntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

/*********************************************************************

    Function:       adi_pdd_Open

    Description:    Opens ADP5520 device

    Parameters:
     hDeviceManager     - Device Manager handle
     nDeviceNumber      - ADP5520 device number to open
     hDevice            - Device Handle allocated by the Device Manager
     phPhysicalDevice   - Pointer to Physical Device Handle
     eDirection         - Data Direction
     pCriticalRegionArg - Critical region parameter
     hDmaManager        - DMA Manager Handle
     hDcbManager        - Deferred Callback Handle
     pfDMCallback       - Pointer to Device Manager supplied
                          Callback function

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
    ADI_DEV_MANAGER_HANDLE      hDeviceManager,
    u32                         nDeviceNumber,
    ADI_DEV_DEVICE_HANDLE       hDevice,
    ADI_DEV_PDD_HANDLE          *phPhysicalDevice,
    ADI_DEV_DIRECTION           eDirection,
    void                        *pCriticalRegionArg,
    ADI_DMA_MANAGER_HANDLE      hDmaManager,
    ADI_DCB_HANDLE              hDcbManager,
    ADI_DCB_CALLBACK_FN         pfDMCallback
)
{
    /* Return value - assume we're going to be successful */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on */
    ADI_DD_ADP5520_DEF *poDevice;
    /* exit critical region parameter */
    void *pExitCriticalArg;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* check the device number */
    if (nDeviceNumber >= ADI_ADP5520_NUM_DEVICES)
    {
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }
#endif

    /* ensure the device the client wants is available  */
    nResult  = ADI_DEV_RESULT_DEVICE_IN_USE;
    /* ADP5520 device we're working on */
    poDevice = &ADP5520Device[nDeviceNumber];

    /* Protect this section of code - entering a critical region    */
    pExitCriticalArg = adi_int_EnterCriticalRegion(pCriticalRegionArg);

    /* Check the device usage status*/
    if (poDevice->bIsInUse == FALSE)
    {
        /* Device is not in use. Reserve the device for this client */
        poDevice->bIsInUse = TRUE;
        nResult = ADI_DEV_RESULT_SUCCESS;
    }

    /* Exit the critical region */
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    /* Continue only when the ADP5520 device is available for use  */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* ADP5520 device is reserved for this client ... */
        /* ... initialise the device settings             */

        /* Pointer to Device manager handle */
        poDevice->hDeviceManager               = hDeviceManager;
        /* Pointer to Device handle */
        poDevice->hDevice                      = hDevice;
        /* Pointer to Defered Callback manager handle */
        poDevice->hDcbManager                  = hDcbManager;
        /* Pointer to Device manager callback function  */
        poDevice->pfDeviceManagerCallback      = pfDMCallback;
        /* Save user-supplied critical region argument */
        poDevice->pCriticalRegionArg           = pCriticalRegionArg;
        /* Default TWI settings */
        poDevice->oADP5520Twi.nTwiDevNumber    = 0;
        poDevice->oADP5520Twi.nTwiDevAddress   = ADP5520_TWI_DEVICE_ADDRESS;
        poDevice->oADP5520Twi.poTwiConfigTable = DevAccessTWIConfig;

        /* save the physical device handle to client supplied location */
        *phPhysicalDevice = (ADI_DEV_PDD_HANDLE *)poDevice;
    }

    return nResult;
}

/*********************************************************************

    Function:       adi_pdd_Close

    Description:    Closes ADP5520 device

    Parameters:
     hPhysicalDevice    - Physical Device handle to ADP5520 device instance

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
     - Error code returned from remove flag callback function
     - Error code returned by device access function

*********************************************************************/
static u32  adi_pdd_Close(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice
)
{
    /* Return value - assume we're going to be successful   */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on */
    ADI_DD_ADP5520_DEF *poDevice;


/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given device handle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* Continue only if the given device handle is valid */
    if (nResult != ADI_DEV_RESULT_SUCCESS)
        return nResult;
#endif

    /* avoid casts */
    poDevice = (ADI_DD_ADP5520_DEF *)hPhysicalDevice;

    /* IF (processor interrupt is enabled for ADP5520) */
    if (poDevice->oIrqInfo.flagId != ADI_FLAG_UNDEFINED)
    {
        /* close the processor flag (which also disables interrupt callback) */
        nResult = adi_flag_Close(poDevice->oIrqInfo.flagId);

        poDevice->oIrqInfo.flagId = ADI_FLAG_UNDEFINED;
    }

    /* do soft reset of ADP5520 */
    reset_ADP5520(poDevice);

    /* mark this ADP5520 device as closed */
    poDevice->bIsInUse = FALSE;

    return nResult;
}

/*********************************************************************

    Function:       adi_pdd_Read

    Description:    Never called as ADP5520 uses Device Access commands
                    to access its internal registers

    Parameters:
     hPhysicalDevice    - Physical Device handle to ADP5520 device instance
     eBufferType        - Buffer type to queue
     poTxBuffer         - Pointer to receive buffer

    Returns:
     - ADI_DEV_RESULT_NOT_SUPPORTED as this function is not supported
       by this device

*********************************************************************/
static u32  adi_pdd_Read(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poRxBuffer
)
{
    /* return error (not supported) */
    return ADI_DEV_RESULT_NOT_SUPPORTED;
}

/*********************************************************************

    Function:       adi_pdd_Write

    Description:    Never called as ADP5520 uses Device Access commands
                    to access its internal registers

    Parameters:
     hPhysicalDevice    - Physical Device handle to ADP5520 device instance
     eBufferType        - Buffer type to queue
     poTxBuffer         - Pointer to receive buffer

    Returns:
     - ADI_DEV_RESULT_NOT_SUPPORTED as this function is not supported
       by this device

*********************************************************************/
static u32  adi_pdd_Write(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poTxBuffer
)
{
    /* return error (not supported) */
    return ADI_DEV_RESULT_NOT_SUPPORTED;
}

/*********************************************************************

    Function:       adi_pdd_SequentialIO

    Description:    Never called as ADP5520 uses Device Access commands
                    to access its internal registers

    Parameters:
     hPhysicalDevice    - Physical Device handle to ADP5520 device instance
     eBufferType        - Buffer type to queue
     poTxBuffer         - Pointer to receive buffer

    Returns:
     - ADI_DEV_RESULT_NOT_SUPPORTED as this function is not supported
       by this device

*********************************************************************/
static u32  adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poSeqBuffer
)
{
    /* return error (not supported) */
    return ADI_DEV_RESULT_NOT_SUPPORTED;
}

/*********************************************************************

    Function:       adi_pdd_Control

    Description:    Sets or senses device specific parameter

    Parameters:
     hPhysicalDevice    - Physical Device handle to
                          ADP5520 device instance
     nCommandID         - Command ID
     pCommandValue      - Command specific value

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
*********************************************************************/
static u32  adi_pdd_Control(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    u32                         nCommandID,
    void                        *pCommandValue
)
{

    /* Pointer to the device we're working on*/
    ADI_DD_ADP5520_DEF *poDevice;
    /* Return value - assume we're going to be successful */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;
    /* u32 equivalent of command specific value */
    u32 nu32Value;

    /* avoid casts */
    poDevice = (ADI_DD_ADP5520_DEF *)hPhysicalDevice;
    /* void * value argument as a 32-bit unsigned value */
    nu32Value = (u32)pCommandValue;

    /* interrupt ports for flag and interrupt designations */
    ADI_ADP5520_INTERRUPT_INFO *poIrqInfo;

    /* work variable for testing flag combinations */
    u32 valid_bits;

/* for Debug build only - check for errors if required */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given PDDHandle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* Continue only if the given PDDHandle is valid */
    if (nResult != ADI_DEV_RESULT_SUCCESS)
        return nResult;
#endif

    /* CASEOF (Command ID)  */
    switch (nCommandID)
    {
        /*
        ** Device Manager specific commands
        */

        /* CASE (Control dataflow) */
        case ADI_DEV_CMD_SET_DATAFLOW:
        /* CASE (Set dataflow method) */
        case ADI_DEV_CMD_SET_DATAFLOW_METHOD:
            /* Do nothing & simply return for these commands */
            break;

        /* CASE (Query for processor DMA support) */
        case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:
            /* ADP5520 does not support DMA */
            *((u32 *)pCommandValue) = FALSE;
            break;

        /*
        ** TWI related commands
        */

        /* CASE (Set ADP5520 TWI Device Number to use) */
        case ADI_ADP5520_CMD_SET_TWI_DEVICE_NUMBER:
            /* Save the TWI device number to use */
            poDevice->oADP5520Twi.nTwiDevNumber = (u8)nu32Value;
            break;

        /* CASE (Set ADP5520 TWI Device Address to use) */
        case ADI_ADP5520_CMD_SET_TWI_DEVICE_ADDRESS:
            /* Save the TWI device address to use */
            poDevice->oADP5520Twi.nTwiDevAddress = (u8)nu32Value;
            break;

        /* CASE (Set TWI Configuration table specific to the application) */
        case ADI_ADP5520_CMD_SET_TWI_CONFIG_TABLE:
            /* Save the TWI configuration table to use */
            poDevice->oADP5520Twi.poTwiConfigTable =
                (ADI_DEV_CMD_VALUE_PAIR*)nu32Value;
            break;

        /*
        ** Interrupt related commands
        */

        /* CASE: (enable selected ADP5520 interrupts and processor interrupt) */
        case ADI_ADP5520_CMD_ENABLE_INTERRUPTS:

            poIrqInfo = (ADI_ADP5520_INTERRUPT_INFO*)pCommandValue;

            /* IF (nINT Interrupt monitoring already enabled and changing  */
            /*     associated processor flag pin [very unlikely])          */
            if (poDevice->oIrqInfo.flagId != ADI_FLAG_UNDEFINED &&
                poDevice->oIrqInfo.flagId != poIrqInfo->flagId)
            {
                /* Close the currently specified processor flag pin */
                /* (also removes our callback)                      */
                 adi_flag_Close(poDevice->oIrqInfo.flagId);
            }

            if (poIrqInfo->flagId != ADI_FLAG_UNDEFINED)
            {
                /* Open specified Blackfin flag */
                if ((nResult = adi_flag_Open (poIrqInfo->flagId)) !=
                        ADI_FLAG_RESULT_SUCCESS)
                {
                    break;
                }

                /* Set flag direction as input */
                nResult = adi_flag_SetDirection(poIrqInfo->flagId,
                                                ADI_FLAG_DIRECTION_INPUT);
                if (nResult != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;
                }

                /*
                ** Set flag trigger to sense interrupt & generate callback
                */

                /* Senses on Falling edge, Parameters are
                     Flag ID, Flag Peripheral interrupt ID,
                     Flag Trigger mode, Is this a wake-up flag,
                     Client Handle, Callback Manager, Callback Function */

                nResult = adi_flag_InstallCallback(
                            poIrqInfo->flagId,
                            poIrqInfo->flagIntId,
                            ADI_FLAG_TRIGGER_FALLING_EDGE,
                            true,
                            (void *)poDevice,
                            poDevice->hDcbManager,
                            ADP5520IntCallback);

                if (nResult != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;
                }
            }

            /* Save the Flag info */
            poDevice->oIrqInfo.flagId = poIrqInfo->flagId;
            poDevice->oIrqInfo.flagIntId = poIrqInfo->flagIntId;
    
            /* Set enable bits for requested ADP5520 interrupts */
            nResult = enable_ADP5520_interrupts(poDevice, poIrqInfo);

            break;

        /* CASE:(disable selected ADP5520 interrupts and processor interrupt) */
        case ADI_ADP5520_CMD_DISABLE_INTERRUPTS:

            poIrqInfo = (ADI_ADP5520_INTERRUPT_INFO*)pCommandValue;

            /* IF (nINT Interrupt monitoring already enabled and being     */
            /*     requested to disable it)                                */
            if (poDevice->oIrqInfo.flagId != ADI_FLAG_UNDEFINED &&
                poIrqInfo->flagId == ADI_FLAG_UNDEFINED)
            {
                /* Close the currently specified processor flag pin */
                /* (also removes our callback)                      */
                 adi_flag_Close(poDevice->oIrqInfo.flagId);

                 poDevice->oIrqInfo.flagId = ADI_FLAG_UNDEFINED;
            }

            nResult = disable_ADP5520_interrupts(poDevice, poIrqInfo);

            break;

        /* CASE: (block processor interrupt associated with nINT) */
        case ADI_ADP5520_CMD_BLOCK_INTERRUPT:
            nResult = adi_int_SICDisable(poDevice->oIrqInfo.flagIntId);
            break;

        /* CASE: (unblock processor interrupt associated with nINT) */
        case ADI_ADP5520_CMD_UNBLOCK_INTERRUPT:
            nResult = adi_int_SICEnable(poDevice->oIrqInfo.flagIntId);
            break;

        /*
        ** ADP5520 register access related commands
        */
        /* CASE: (Read an ADP5520 register) */
        case ADI_DEV_CMD_REGISTER_READ:
        /* CASE: (Write to an ADP5520 register) */
        case ADI_DEV_CMD_REGISTER_WRITE:
        /* CASE: (Read a field from an ADP5520 register) */
        case ADI_DEV_CMD_REGISTER_FIELD_READ:
        /* CASE: (Write to a field in an ADP5520 register) */
        case ADI_DEV_CMD_REGISTER_FIELD_WRITE:
        /* CASE: (Read block of ADP5520 registers) */
        case ADI_DEV_CMD_REGISTER_BLOCK_READ:
        /* CASE: (Write to a block of ADP5520 registers) */
        case ADI_DEV_CMD_REGISTER_BLOCK_WRITE:
        /* CASE: (Read a set of ADP5520 registers) */
        case ADI_DEV_CMD_REGISTER_TABLE_READ:
        /* CASE: (Write to a set of ADP5520 registers) */
        case ADI_DEV_CMD_REGISTER_TABLE_WRITE:
        /* CASE (Read a set of ADP5520 register fields) */
        case ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ:
        /* CASE (Write to a set of ADP5520 register fields) */
        case ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE:

            /* Access ADP5520 registers */
            nResult = access_ADP5520_registers(poDevice,
                                               nCommandID,
                                               pCommandValue);
            break;

        /*
        ** 'Macro' commands to set up an ADP5520 feature in a pre-defined
        ** configuration ('pre-defined' by this driver).
        */

        /* CASE: (enable keypad scanning) */
        case ADI_ADP5520_CMD_ENABLE_KEYPAD:
            /* ensure that no conflicting features are currently enabled */
            if (poDevice->enabled_features &
                    (F_KEYPAD_KP | F_KEYPAD_KR | F_LED2 | F_LED3 | F_GPIO))
            {
                nResult = ADI_ADP5520_RESULT_FEATURE_CONFLICT;
                break;
            }

            /* ensure that supplied parameter is valid */
            valid_bits = 
                (ADI_ADP5520_WANT_KEY_PRESSES | ADI_ADP5520_WANT_KEY_RELEASES);
            if ((nu32Value &  valid_bits) == 0 ||
                (nu32Value & ~valid_bits) != 0    )
            {
                nResult = ADI_ADP5520_RESULT_INVALID_KEY_REQUIREMENT;
                break;
            }

            /* set up and enable keypad scanning */
            nResult = enable_keypad(poDevice, nu32Value);
            break;

        /* CASE: (disable keypad scanning) */
        case ADI_ADP5520_CMD_DISABLE_KEYPAD:
            nResult = disable_keypad(poDevice);
            break;

        /*
        ** Default: other commands not supported
        */
        default:
            nResult = ADI_ADP5520_RESULT_CMD_NOT_SUPPORTED;
            break;
    }

    return nResult;
}

/*********************************************************************

    Function:       ADP5520IntCallback

    Description:    Callback routine for ADP5520 nINT interrupt

    Parameters:
     phClient       - Client handle passed to flag service
     nEvent         - Callback Event
     pArg           - Callback Argument

*********************************************************************/
static void ADP5520IntCallback (
    void    *phClient,
    u32     nEvent,
    void    *pArg
)
{
    /* Retrive the device instance to which this callback belongs */
    ADI_DD_ADP5520_DEF *pDevice = (ADI_DD_ADP5520_DEF *)phClient;

    ADI_DEV_ACCESS_REGISTER reg;

/* Debug build only */
#ifdef ADI_DEV_DEBUG
    /* verify the Device handle */
    if (ValidatePDDHandle(pDevice) != ADI_DEV_RESULT_SUCCESS)
        return;
#endif

    SAVE_MODE_STATUS(pDevice);

    /*
    ** Discover why nINT was asserted, perform necessary actions to 
    ** acknowledge the conditions, post callbacks to client if necessary.
    */

    /* read the ADP5520's overall status register */
    reg.Address = ADP5520_REG_MODE_STATUS;
    reg.Data    = 0 ;
    access_ADP5520_registers(pDevice, ADI_DEV_CMD_REGISTER_READ, &reg);

    /* check for and process each interrupt source */
    process_keypad_interrupt      (pDevice, reg.Data);
    process_gpio_interrupt        (pDevice, reg.Data);
    process_light_sensor_interrupt(pDevice, reg.Data);
    process_overvoltage_interrupt (pDevice, reg.Data);

    SAVE_MODE_STATUS(pDevice);
}

/*********************************************************************

    Function:       process_keys

    Description:    Helper function for process_keypad_interrupt().

                    Does the work of acknowledging the key press or
                    release interrupt and posting client callback if
                    appropriate.

    Parameters:
     pDevice         The ADP5520 device instance
     status_register The first of the pair of interrupt status regs to check.
     feature         The feature identifier that controls client callback.
     event           The identifier of the event to raise.
     status_field    The flag position to be acknowledged in interrupt register.

*********************************************************************/
static void process_keys(
    ADI_DD_ADP5520_DEF  *pDevice,
    u32                 status_register,
    u32                 feature,
    u32                 event,
    u32                 status_field
)
{
    ADI_DEV_ACCESS_REGISTER_FIELD field;
    ADI_DEV_ACCESS_REGISTER_BLOCK interrupt_regs;
    u16 interrupt_vals[2];

    /* Event specifier and argument if invoking client callback */
    u32 client_event = 0;
    u32 client_arg   = 0;

    /* find which key presses are raising interrupts */
    interrupt_regs.Count   = 2;
    interrupt_regs.Address = status_register;
    interrupt_regs.pData   = interrupt_vals;
    access_ADP5520_registers(pDevice,
                             ADI_DEV_CMD_REGISTER_BLOCK_READ,
                             &interrupt_regs);

    client_arg = (interrupt_vals[1] << 8) | interrupt_vals[0];

    /* read again to clear status */
    access_ADP5520_registers(pDevice,
                             ADI_DEV_CMD_REGISTER_BLOCK_READ,
                             &interrupt_regs);

    client_arg |= (interrupt_vals[0] << 8) | interrupt_vals[1];

    /* cause client key press event (if requested) */
    if (pDevice->enabled_features & feature)
    {
        client_event = event;
        (pDevice->pfDeviceManagerCallback)
            (pDevice->hDevice, client_event, (void*)client_arg);
    }

    /* acknowledge interrupt source */
    field.Address = ADP5520_REG_MODE_STATUS;
    field.Field   = status_field;
    field.Data    = 1;
    access_ADP5520_registers(pDevice,
                             ADI_DEV_CMD_REGISTER_FIELD_WRITE,
                             &field);
}

/*********************************************************************

    Function:       process_keypad_interrupt

    Description:    Routine for servicing ADP5520 keypad interrupts.

                    Acknowledges any key press and/or release interrupt
                    and posts client callback if appropriate.

    Parameters:
     pDevice        The ADP5520 device instance
     status         Current value of ADP5520 register 0x00 (MODE STATUS)

*********************************************************************/
static void process_keypad_interrupt(
    ADI_DD_ADP5520_DEF  *pDevice,
    u32                 status
)
{
    /* has there been a keypad event? */
    if (!(status & (KP_INT | KR_INT)))
        return;

    /* process any key presses */
    if (status & KP_INT)
    {
        process_keys(pDevice,
                     ADP5520_REG_KP_INT_STAT_1,
                     F_KEYPAD_KP,
                     ADI_ADP5520_EVENT_KEYPAD_PRESS,
                     ADP5520_POS_KP_INT);
    }

    /* process any key releases */
    if (status & KR_INT)
    {
        process_keys(pDevice,
                     ADP5520_REG_KR_INT_STAT_1,
                     F_KEYPAD_KR,
                     ADI_ADP5520_EVENT_KEYPAD_RELEASE,
                     ADP5520_POS_KR_INT);
    }
}

/*********************************************************************

    Function:       process_gpio_interrupt

    Description:    Routine for servicing ADP5520 GPIO input interrupts.

                    Acknowledges any interrupt from the GPIO input pins
                    and posts client callback.

    Parameters:
     pDevice        The ADP5520 device instance
     status         Current value of ADP5520 register 0x00 (MODE STATUS)

*********************************************************************/
static void process_gpio_interrupt(
    ADI_DD_ADP5520_DEF  *pDevice,
    u32                 status
)
{
    const u32 GPI_INT = (1 << ADP5520_POS_GPI_INT);
    u32 client_data;

    ADI_DEV_ACCESS_REGISTER       reg1;
    ADI_DEV_ACCESS_REGISTER       reg2;
    ADI_DEV_ACCESS_REGISTER_FIELD field;

    /* has there been a GPIO input interrupt? */
    if (!(status & GPI_INT))
        return;

    /* read the state of the GPIO interrupt status twice (to clear) */
    reg1.Address = ADP5520_REG_ALS_CMPR_CFG;
    reg1.Data    = 0;
    access_ADP5520_registers(pDevice, ADI_DEV_CMD_REGISTER_READ, &reg1);
    reg2.Address = ADP5520_REG_ALS_CMPR_CFG;
    reg2.Data    = 0;
    access_ADP5520_registers(pDevice, ADI_DEV_CMD_REGISTER_READ, &reg2);

    /* post client callback passing state of GPIO interrupt status */
    client_data = reg1.Data | reg2.Data;

    (pDevice->pfDeviceManagerCallback)
        (pDevice->hDevice,
         ADI_ADP5520_EVENT_GPIO_INPUT,
         (void*)client_data);

    /* acknowledge interrupt source */
    field.Address = ADP5520_REG_MODE_STATUS;
    field.Field   = ADP5520_POS_GPI_INT;
    field.Data    = 1;
    access_ADP5520_registers(pDevice,
                             ADI_DEV_CMD_REGISTER_FIELD_WRITE,
                             &field);
}

/*********************************************************************

    Function:       process_light_sensor_interrupt

    Description:    Routine for servicing ADP5520 light sensor interrupts.

                    Acknowledges any interrupt from the ambient light 
                    sensor comparators and posts client callback.

    Parameters:
     pDevice        The ADP5520 device instance
     status         Current value of ADP5520 register 0x00 (MODE STATUS)

*********************************************************************/
static void process_light_sensor_interrupt(
    ADI_DD_ADP5520_DEF  *pDevice,
    u32                 status
)
{
    const u32 CMP_INT = (1 << ADP5520_POS_CMPR_INT);
    u32 client_data;

    ADI_DEV_ACCESS_REGISTER       reg;
    ADI_DEV_ACCESS_REGISTER_FIELD field;

    /* has there been a light sensor interrupt? */
    if (!(status & CMP_INT))
        return;

    /* read the state of the ambient light sensor comparators */
    reg.Address = ADP5520_REG_ALS_CMPR_CFG;
    reg.Data    = 0;
    access_ADP5520_registers(pDevice, ADI_DEV_CMD_REGISTER_READ, &reg);

    /* post client callback passing state of L3_OUT and L2_OUT flags */
    client_data = reg.Data &
                  ((1 << ADP5520_POS_L3_OUT) | (1 << ADP5520_POS_L2_OUT));

    (pDevice->pfDeviceManagerCallback)
        (pDevice->hDevice,
         ADI_ADP5520_EVENT_LIGHT_SENSOR_TRIGGER,
         (void*)client_data);

    /* acknowledge interrupt source */
    field.Address = ADP5520_REG_MODE_STATUS;
    field.Field   = ADP5520_POS_CMPR_INT;
    field.Data    = 1;
    access_ADP5520_registers(pDevice,
                             ADI_DEV_CMD_REGISTER_FIELD_WRITE,
                             &field);

    SAVE_MODE_STATUS(pDevice);
}

/*********************************************************************

    Function:       process_overvoltage_interrupt

    Description:    Routine for servicing ADP5520 overvoltage interrupts.

                    Acknowledges any interrupt from the overvoltage
                    protection feature and posts client callback.

    Parameters:
     pDevice        The ADP5520 device instance
     status         Current value of ADP5520 register 0x00 (MODE STATUS)

*********************************************************************/
static void process_overvoltage_interrupt(
    ADI_DD_ADP5520_DEF  *pDevice,
    u32                 status
)
{
    const u32 OVP_INT = (1 << ADP5520_POS_OVP_INT);

    ADI_DEV_ACCESS_REGISTER_FIELD field;

    /* has there been an overvoltage interrupt? */
    if (!(status & OVP_INT))
        return;

    /* post client callback */
    (pDevice->pfDeviceManagerCallback)
        (pDevice->hDevice, ADI_ADP5520_EVENT_OVERVOLTAGE_TRIGGER, NULL);

    /* acknowledge interrupt source */
    field.Address = ADP5520_REG_MODE_STATUS;
    field.Field   = ADP5520_POS_OVP_INT;
    field.Data    = 1;
    access_ADP5520_registers(pDevice,
                             ADI_DEV_CMD_REGISTER_FIELD_WRITE,
                             &field);
}


/*********************************************************************
*
* Function:  access_ADP5520_registers
*
* Description: Access ADP5520 Device registers via Device Access Service
*
*********************************************************************/

static u32 access_ADP5520_registers(
    ADI_DD_ADP5520_DEF  *pDevice,  /* pointer to the device we're working on  */
    u32                 nCommand,  /* Command ID from the client              */
    void                *Value     /* Command specific value                  */
){
    /* Return value - assume success */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* Device selection information passed to device access service */
    ADI_DEVICE_ACCESS_SELECT da_sel;

    /* Register access information passed to device access service */
    ADI_DEVICE_ACCESS_REGISTERS da_reg;

    /* Set up ADP5520 device selection information */
    da_sel.DeviceCS   = 0;                          /* (dummy for TWI) */
    da_sel.Gaddr_len  = ADI_DEVICE_ACCESS_LENGTH1;  /* (dummy for TWI) */
    da_sel.Raddr_len  = ADI_DEVICE_ACCESS_LENGTH1;  /* reg addrs are 8-bit */
    da_sel.Rdata_len  = ADI_DEVICE_ACCESS_LENGTH1;  /* reg data are 8-bit */
    da_sel.AccessType = ADI_DEVICE_ACCESS_TYPE_TWI; /* TWI-connected */

    /* Set up ADP5520 register access request information */
    da_reg.ManagerHandle    = pDevice->hDeviceManager;
    da_reg.ClientHandle     = NULL;
    da_reg.DeviceNumber     = pDevice->oADP5520Twi.nTwiDevNumber;
    da_reg.DeviceAddress    = pDevice->oADP5520Twi.nTwiDevAddress;
    da_reg.DCBHandle        = pDevice->hDcbManager;
    da_reg.DeviceFunction   = NULL;
    da_reg.Command          = nCommand;
    da_reg.Value            = Value;
    da_reg.FinalRegAddr     = ADP5520_FINAL_REG_ADDRESS;
    da_reg.RegisterField    = ADP5520_BitfieldTable;
    da_reg.ReservedValues   = ADP5520_ReservedBitTable;
    da_reg.ValidateRegister = ADP5520_ReadonlyTable;
    da_reg.ConfigTable      = pDevice->oADP5520Twi.poTwiConfigTable;
    da_reg.SelectAccess     = &da_sel;
    da_reg.pAdditionalinfo  = (void *)NULL;

    /* Access the requested register(s) or field(s) via device access service */
    nResult = adi_device_access(&da_reg);

    return nResult;
}

/*********************************************************************
*
* Function:  enable_ADP5520_interrupts
*
* Description: Enable the ADP5520 interrupt sources specified in
*              the 'poIrqInfo' parameter to raise the nINT signal
*              and thus cause a processor interrupt.
*
*********************************************************************/

static u32 enable_ADP5520_interrupts(
    ADI_DD_ADP5520_DEF         *pDevice,   /* device we're working on         */
    ADI_ADP5520_INTERRUPT_INFO *poIrqInfo  /* holds interrupts to be enabled  */
){
    u32 nResult = ADI_DEV_RESULT_SUCCESS;
    u32 ndx;
    ADI_DEV_ACCESS_REGISTER_FIELD field;

    field.Address = ADP5520_REG_INTERRUPT_ENABLE;
    field.Data    = 1;
    for (ndx = 0; ndx < 4; ndx += 1)
    {
        if (poIrqInfo->interruptMask & (1 << ndx))
        {
            field.Field = ndx;
            nResult = access_ADP5520_registers(pDevice,
                                               ADI_DEV_CMD_REGISTER_FIELD_WRITE,
                                               &field);
            if (nResult != ADI_DEV_RESULT_SUCCESS)
                return nResult;

            pDevice->oIrqInfo.interruptMask |= (1 << ndx);
        }
    }

    return nResult;
}

/*********************************************************************
*
* Function:  disable_ADP5520_interrupts
*
* Description: Disable the ADP5520 interrupt sources specified in
*              the 'poIrqInfo' parameter from being able to raise
*              the nINT signal.
*
*********************************************************************/

static u32 disable_ADP5520_interrupts(
    ADI_DD_ADP5520_DEF         *pDevice,   /* device we're working on         */
    ADI_ADP5520_INTERRUPT_INFO *poIrqInfo  /* holds interrupts to be disabled */
){
    u32 nResult = ADI_DEV_RESULT_SUCCESS;
    u32 ndx;
    ADI_DEV_ACCESS_REGISTER_FIELD field;

    field.Address = ADP5520_REG_INTERRUPT_ENABLE;
    field.Data = 0;
    for (ndx = 0; ndx < 4; ndx += 1)
    {
        if (poIrqInfo->interruptMask & (1 << ndx))
        {
            field.Field = ndx;
            nResult = access_ADP5520_registers(pDevice,
                                               ADI_DEV_CMD_REGISTER_FIELD_WRITE,
                                               &field);
            if (nResult != ADI_DEV_RESULT_SUCCESS)
                return nResult;

            pDevice->oIrqInfo.interruptMask &= ~(1 << ndx);
        }
    }

    return nResult;
}

/*********************************************************************
*
* Function:  reset_ADP5520
*
* Description: Perform a 'soft' reset of the ADP5520 by disabling
*              interrupts and putting it into standby mode.
*              The device does have a 'hard' reset signal (nRST)
*              but this is assumed to be connected to system-wide
*              reset functionality.
*
*********************************************************************/

static void reset_ADP5520(
    ADI_DD_ADP5520_DEF  *pDevice   /* pointer to the device we're working on  */
){
    static const u8 config_regs[] = {
        ADP5520_REG_BL_CONTROL,         ADP5520_REG_BL_TIME,
        ADP5520_REG_BL_FADE,            ADP5520_REG_DAYLIGHT_MAX,
        ADP5520_REG_DAYLIGHT_DIM,       ADP5520_REG_OFFICE_MAX,
        ADP5520_REG_OFFICE_DIM,         ADP5520_REG_DARK_MAX,
        ADP5520_REG_DARK_DIM,           ADP5520_REG_L2_TRIP,
        ADP5520_REG_L2_HYS,             ADP5520_REG_L3_TRIP,
        ADP5520_REG_L3_HYS,             ADP5520_REG_LED_CONTROL,
        ADP5520_REG_LED_TIME,           ADP5520_REG_LED_FADE,
        ADP5520_REG_LED1_CURRENT,       ADP5520_REG_LED2_CURRENT,
        ADP5520_REG_LED3_CURRENT,       ADP5520_REG_GPIO_CFG_1,
        ADP5520_REG_GPIO_CFG_2,         ADP5520_REG_GPIO_OUT,
        ADP5520_REG_GPIO_INT_EN,        ADP5520_REG_GPIO_INT_LVL,
        ADP5520_REG_GPIO_DEBOUNCE,      ADP5520_REG_GPIO_PULLUP
    };

    ADI_ADP5520_INTERRUPT_INFO oIrqInfo;
    ADI_DEV_ACCESS_REGISTER reg;
    u32 ndx;

    /* Disable ADP5520 interrupt sources */
    oIrqInfo.interruptMask = ADP5520_MSK_CMPR_IEN |
                             ADP5520_MSK_OVP_IEN  |
                             ADP5520_MSK_KR_IEN   |
                             ADP5520_MSK_KP_IEN;
    disable_ADP5520_interrupts(pDevice, &oIrqInfo);
    pDevice->oIrqInfo.interruptMask = 0;

    /* Reset most configuration registers */
    for (ndx = 0; ndx < sizeof(config_regs)/sizeof(config_regs[0]); ndx += 1)
    {
        reg.Address = config_regs[ndx];
        reg.Data = 0;
        access_ADP5520_registers(pDevice,
                                 ADI_DEV_CMD_REGISTER_WRITE,
                                 &reg);
    }

    /* Put ADP5520 into standby mode */
    reg.Address = ADP5520_REG_MODE_STATUS;
    reg.Data = 0;
    access_ADP5520_registers(pDevice, ADI_DEV_CMD_REGISTER_WRITE, &reg);
}

/*********************************************************************
*
* Function:  enable_keypad
*
* Description: Sets up the ADP5520 keypad scanning feature in a fixed
*              configuration and then enables it to deliver key press
*              and/or release interrupts depending on caller's 
*              requirement.
*
*********************************************************************/
static u32 enable_keypad(
    ADI_DD_ADP5520_DEF  *pDevice,  /* pointer to the device we're working on  */
    u32                 events     /* requested events (press and/or release) */
) {
    ADI_DEV_ACCESS_REGISTER_FIELD fields[3];
    ADI_DEV_ACCESS_REGISTER reg;
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* set R3 and C3 to GPIO mode rather than LED mode */
    fields[0].Address = ADP5520_REG_LED_CONTROL;
    fields[0].Field   = ADP5520_POS_R3_MODE;
    fields[0].Data    = ADP5520_VAL_R3_MODE_GPIO;
    fields[1].Address = ADP5520_REG_LED_CONTROL;
    fields[1].Field   = ADP5520_POS_C3_MODE;
    fields[1].Data    = ADP5520_VAL_C3_MODE_GPIO;
    fields[2].Address = ADI_DEV_REGEND;
    fields[2].Field   = 0;
    fields[2].Data    = 0;

    nResult = access_ADP5520_registers(
                    pDevice,
                    ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE,
                    fields);

    if (nResult != ADI_DEV_RESULT_SUCCESS)
        return nResult;

    /* enable row pull-ups using a whole-register write */
    reg.Address = ADP5520_REG_GPIO_PULLUP;
    reg.Data    = 0x0Fu;    /* R0-3 ON, C0-3 OFF */
    nResult = access_ADP5520_registers(
                    pDevice,
                    ADI_DEV_CMD_REGISTER_WRITE,
                    &reg);

    if (nResult != ADI_DEV_RESULT_SUCCESS)
        return nResult;

    /* enable scanning */
    reg.Address = ADP5520_REG_GPIO_CFG_1;
    reg.Data    = 0xFFu;    /* 4 rows x 4 columns */
    nResult = access_ADP5520_registers(
                    pDevice,
                    ADI_DEV_CMD_REGISTER_WRITE,
                    &reg);

    if (nResult != ADI_DEV_RESULT_SUCCESS)
        return nResult;

    /* if the backlight is not yet enabled arrange for key presses to 
       turn on the backlight if it is enabled later
    */
    if (!(pDevice->enabled_features & F_BACKLIGHT))
    {
        fields[0].Address = ADP5520_REG_BL_CONTROL;
        fields[0].Field   = ADP5520_POS_KP_BL_EN;
        fields[0].Data    = ADP5520_VAL_KP_BL_EN_ON;
        fields[1].Address = ADI_DEV_REGEND;
        fields[1].Field   = 0;
        fields[1].Data    = 0;

        nResult = access_ADP5520_registers(
                        pDevice,
                        ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE,
                        fields);

        if (nResult != ADI_DEV_RESULT_SUCCESS)
            return nResult;
    }

    /* the driver must enable both key press and release interrupts regardless
       of the events that the caller wants to receive - if the driver only
       enabled key press interrupts then the release of the last press in a
       series would go unacknowledged which would prevent the automatic
       backlight feature re-arming itself
    */
    fields[0].Address = ADP5520_REG_INTERRUPT_ENABLE;
    fields[0].Field   = ADP5520_POS_KP_IEN;
    fields[0].Data    = ADP5520_VAL_KP_IEN;
    fields[1].Address = ADP5520_REG_INTERRUPT_ENABLE;
    fields[1].Field   = ADP5520_POS_KR_IEN;
    fields[1].Data    = ADP5520_VAL_KR_IEN;
    fields[2].Address = ADI_DEV_REGEND;
    fields[2].Field   = 0;
    fields[2].Data    = 0;

    nResult = access_ADP5520_registers(
                    pDevice,
                    ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE,
                    fields);

    if (nResult != ADI_DEV_RESULT_SUCCESS)
        return nResult;

    /* ensure keypad processing starts off in a clean state */
    process_keypad_interrupt(pDevice, KP_INT | KR_INT);

    /* record that keypad scanning is enabled */
    if (events & ADI_ADP5520_WANT_KEY_PRESSES)
        pDevice->enabled_features |= F_KEYPAD_KP;
    if (events & ADI_ADP5520_WANT_KEY_RELEASES)
        pDevice->enabled_features |= F_KEYPAD_KR;

    return nResult;
}

/*********************************************************************
*
* Function:  disable_keypad
*
* Description: Turns off all keypad scanning in the ADP5520.
*
*********************************************************************/
static u32 disable_keypad(
    ADI_DD_ADP5520_DEF  *pDevice   /* pointer to the device we're working on  */
) {
    ADI_DEV_ACCESS_REGISTER_FIELD fields[3];
    ADI_DEV_ACCESS_REGISTER reg;
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* disable key press and release interrupts */
    fields[0].Address = ADP5520_REG_INTERRUPT_ENABLE;
    fields[0].Field   = ADP5520_POS_KP_IEN;
    fields[0].Data    = 0;
    fields[1].Address = ADP5520_REG_INTERRUPT_ENABLE;
    fields[1].Field   = ADP5520_POS_KR_IEN;
    fields[1].Data    = 0;
    fields[2].Address = ADI_DEV_REGEND;
    fields[2].Field   = 0;
    fields[2].Data    = 0;

    nResult = access_ADP5520_registers(
                    pDevice,
                    ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE,
                    fields);

    if (nResult != ADI_DEV_RESULT_SUCCESS)
        return nResult;

    /* disable scanning */
    reg.Address = ADP5520_REG_GPIO_CFG_1;
    reg.Data    = 0;
    nResult = access_ADP5520_registers(
                    pDevice,
                    ADI_DEV_CMD_REGISTER_WRITE,
                    &reg);

    if (nResult != ADI_DEV_RESULT_SUCCESS)
        return nResult;

    /* disable all pull-ups */
    reg.Address = ADP5520_REG_GPIO_PULLUP;
    reg.Data    = 0;
    nResult = access_ADP5520_registers(
                    pDevice,
                    ADI_DEV_CMD_REGISTER_WRITE,
                    &reg);

    if (nResult != ADI_DEV_RESULT_SUCCESS)
        return nResult;

    /* return R3 and C3 to LED mode rather than GPIO mode */
    fields[0].Address = ADP5520_REG_LED_CONTROL;
    fields[0].Field   = ADP5520_POS_R3_MODE;
    fields[0].Data    = ADP5520_VAL_R3_MODE_LED;
    fields[1].Address = ADP5520_REG_LED_CONTROL;
    fields[1].Field   = ADP5520_POS_C3_MODE;
    fields[1].Data    = ADP5520_VAL_C3_MODE_LED;
    fields[2].Address = ADI_DEV_REGEND;
    fields[2].Field   = 0;
    fields[2].Data    = 0;

    nResult = access_ADP5520_registers(
                    pDevice,
                    ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE,
                    fields);

    if (nResult != ADI_DEV_RESULT_SUCCESS)
        return nResult;

    /* record that the keypad feature is not in use */
    pDevice->enabled_features &= ~(F_KEYPAD_KP | F_KEYPAD_KR);

    /* clear away any left-over keypad status */
    process_keypad_interrupt(pDevice, KP_INT | KR_INT);

    return nResult;
}

#if defined(ADI_DEV_DEBUG)  /* Debug build only */
/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      PDDHandle   /* PDD handle of a ADP5520 device  */
)
{
    /* Return value - assume the worst */
    u32 nResult = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* general index */
    u32 i;

    /* compare the given PDD handle with PDDHandle of all ADP5520 devices */
    for (i = 0; i < ADI_ADP5520_NUM_DEVICES; i++)
    {
        if (PDDHandle == (ADI_DEV_PDD_HANDLE)&ADP5520Device[i])
        {
            /* Given PDDHandle is valid. quit this loop */
            nResult = ADI_DEV_RESULT_SUCCESS;
            break;
        }
    }
    /* return */
    return nResult;
}

/* reads ADP5520_REG_MODE_STATUS and saves value in device structure */
static void save_mode_status(
    ADI_DD_ADP5520_DEF  *pDevice   /* pointer to the device we're working on  */
)
{
    ADI_DEV_ACCESS_REGISTER reg;

    /* read the ADP5520's overall status register */
    reg.Address = ADP5520_REG_MODE_STATUS;
    reg.Data    = 0 ;
    access_ADP5520_registers(pDevice, ADI_DEV_CMD_REGISTER_READ, &reg);

    /* save its value in the device structure */
    pDevice->mode_status = (u8)reg.Data;
}
#endif /* Debug build only */

/*****/
