/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_usb_hdrc.c,v $
$Revision: 131 $
$Date: 2009-11-04 15:40:56 -0500 (Wed, 04 Nov 2009) $

Description:
             Hi-speed USB OTG dual-role controller driver source file.

*********************************************************************************/

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_HDRC_SECTION_CODE    section("adi_usb_hdrc_code")
#define __ADI_USB_HDRC_SECTION_DATA    section("adi_usb_hdrc_data")
#else
#define __ADI_USB_HDRC_SECTION_CODE
#define __ADI_USB_HDRC_SECTION_DATA
#endif 


#include <drivers\usb\controller\otg\adi\hdrc\adi_usb_hdrc.h>
#include <drivers\usb\usb_core\adi_usb_objects.h>
#include <drivers\usb\usb_core\adi_usb_core.h>
#include <services/services.h>
#include <drivers/adi_dev.h>
#include <drivers\usb\usb_core\adi_usb_debug.h>
#include <string.h>
#include <cycle_count_bf.h>

/* Configuration Macros */
#define DEBUG_USBOTG
#define USE_DMA

//#define ADI_HDRC_STATISTICS

/* Macros */
#define NUM_PHYSICAL_ENDPOINTS  8  /* including EP0 */


#define ADDR_LOW(x)        (((unsigned int)x) & 0xffff)
#define ADDR_HIGH(x)       ((((unsigned int)x) >> 16)  & 0xffff)
#define IS_ISONCHRONOUS(x) ((((u8_t)(x)) & 0x3) == 0x1)
#define IS_BULK(x)         ((((u8_t)(x)) & 0x3) == 0x2)
#define IS_INTERRUPT(x)    ((((u8_t)(x)) & 0x3) == 0x3)
#define DEV_OUT 0
#define DEV_IN  1

#ifdef DEBUG_USBOTG
void* DumpUsbOTGRegisters(void);
#define DUMP_REGISTERS  DumpUsbOTGRegisters
#else
#define DUMP_REGISTERS
#endif /* DUMP_REGISTERS */


#define FLUSH(P)           {asm volatile("FLUSH[%0++];":"+p"(P));}
#define FLUSHINV(P)        {asm volatile("FLUSHINV[%0++];":"+p"(P));}
#define SIMPLEFLUSHINV(P)  {ssync(); asm volatile("FLUSHINV[%0++];"::"#p"(P)); ssync();}

#define WARN_INT_MGR_MEMORY_ERROR(x)                                                           \
                         do {                                                                  \
                               if(x == ADI_INT_RESULT_NO_MEMORY)                              \
                               {                                                              \
                                   printf("ERROR: Insufficient Memory in the Interrupt Manager\n");\
                                   printf("USB requires memory for four secondary handlers [sizeof(ADI_INT_SECONDARY_MEMORY * 4)]\n");\
                                   printf("Supply more memory using adi_int_Init()\n");         \
                                }                                                              \
                             } while(0) 

/* Interrupt handler functions */
typedef ADI_INT_HANDLER_RESULT (*INTERRUPT_HANDLER_FUN)(void);

/**
 * Physical Endpoint structure. USB core operates at logical endpoint level.
 * once host selects a configuration and an interface the associated logical
 * endpoint objects of the currently active interface will be binded with the
 * physical endpoint objects. The bind holds good until host again sets or
 * resets the configuration and interface.
 */
typedef struct PhysicalEndpointObject
{
    ENDPOINT_OBJECT    *pLogicalEndpointObj;   /* logical endpoint object */
    s32_t              eEndpointState;         /* endpoint state          */
    s32_t              PhysicalEndpointID;     /* Physical endpoint ID    */
    s32_t              eIsDmaPending;          /* DMA pending */
    s32_t              DmaMode;               /* DMA operating mode used only in case of bulk */
}PHYSICAL_ENDPOINT_OBJECT;

/* USB OTG Device state-Internal to the Driver */
__ADI_USB_HDRC_SECTION_DATA
typedef enum DeviceState
{
    DEV_STATE_RESET=0,                        /* Device is in RESET state */
    DEV_STATE_OPEN_IN_PROCESS,                /* Application issued devOpen */
    DEV_STATE_OPENED,                         /* Device is Opened successfully */
    DEV_STATE_CLOSED,                         /* Device is Closed */
    DEV_STATE_STARTED,                        /* Device is Started, active transfers */
    DEV_STATE_CONFIGURED,                     /* Device configured enumeration finished*/
}DEVICE_STATE;


/* Device Enumeration state, only used in Host MODE */
__ADI_USB_HDRC_SECTION_DATA
typedef enum DevEnumState
{
   DEV_ENUM_RESET=0,
   DEV_ENUM_PENDING,
   DEV_ENUM_COMPLETE
}DEV_ENUM_STATE;

/* Queue */
__ADI_USB_HDRC_SECTION_DATA
typedef enum EpQueue
{
    FREE_RCV_QUEUE,                           /* queue used to fill an incoming packet */
    FREE_XMT_QUEUE,                           /* transmitted packets placed here       */
    PROC_RCV_QUEUE,                           /* pending receive operating buffer      */
    WAIT_XMT_QUEUE                            /* to be transmited packets              */
}EP_QUEUE;

/* HDRC Statistics */
#if defined(ADI_HDRC_STATISTICS)
__ADI_USB_HDRC_SECTION_DATA
typedef struct ADI_HDRC_Stats
{
    s32_t  dwRxZeroLengthPkts;
    s32_t  dwRxNAKTimeoutErrors;
    s32_t  dwRxStalls;
    s32_t  dwRxErrors;
    s32_t  dwUnexpectedShortPkts; 
    s32_t  dwRxPktInFIFO;
    s32_t  dwRxNoEpBuffer;
    s32_t  dwTxStalls;
    s32_t  dwTxNAKTimeoutErrors;
    s32_t  dwTxErrors;
}ADI_HDRC_STATS;
#endif /* ADI_HDRC_STATISTICS */

/* USB OTG Device data */
__ADI_USB_HDRC_SECTION_DATA
typedef struct UsbOTGDeviceData
{
    ADI_DEV_DEVICE_HANDLE     DeviceHandle;        /* Device handle    */
    ADI_DMA_MANAGER_HANDLE    DMAHandle;           /* DMA handle       */
    ADI_DCB_HANDLE            DCBHandle;           /* DCB handle       */
    ADI_DCB_CALLBACK_FN       DMCallback;          /* Callback routine */
    ADI_DEV_DIRECTION         Direction;           /* Device direction */
    void                      *pCriticalRegionArg; /* Storage to save interrupt mask */
    PHYSICAL_ENDPOINT_OBJECT  PhysicalEndpointObjects[NUM_PHYSICAL_ENDPOINTS];
    s32_t                     NumPhysicalEndpoints; /* No# of physical endpoints */
    bool                      Started;              /* Device Started  */
    DEVICE_STATE              State;                /* Device State    */
    ADI_INT_PERIPHERAL_ID     PeripheralID;         /* Peripheral ID   */
    interrupt_kind            IvgUSBINT0;           /* USB INT0 ivg    */
    interrupt_kind            IvgUSBINT1;           /* USB INT1 ivg RX */
    interrupt_kind            IvgUSBINT2;           /* USB INT2 ivg TX */
    interrupt_kind            IvgUSBDMAINT;         /* USB DMAINT ivg  */
    ADI_FLAG_ID               VbusDriveFlag;        /* flag used to drive VBUS */
    s32_t                     DeviceID;             /* Device ID       */
    s32_t                     DeviceAddress;        /* Device address-USB */
    DEVICE_OBJECT             *pDeviceObj;          /* Logical Device OBJ */
    bool                      Cache;                /* Cache on or off? */
    s32_t                     BufferPrefix;         /* Buffer prefix   */
    DEV_MODE                  Mode;                 /* Device MODE     */
    ADI_USB_DEVICE_SPEED      Speed;                /* Device SPEED    */
    INTERRUPT_HANDLER_FUN     RxInterruptHandler;   /* Function pointer to RX interrupt handler */
    INTERRUPT_HANDLER_FUN     TxInterruptHandler;   /* Function pointer to TX interrupt handler */
    DEV_ENUM_STATE            EnumerationState;     /* Device Enumeration state */
    s32_t                     RxDmaMode;            /* RX DMA mode */
    s32_t                     TxDmaMode;            /* TX DMA mode */ 
#if defined(ADI_HDRC_STATISTICS)
    ADI_HDRC_STATS            Stats;                /* Statistics */
#endif
}ADI_USBOTG_DEVICE;



/* USB OTG driver object */
__ADI_USB_HDRC_SECTION_DATA
static ADI_USBOTG_DEVICE UsbOtgDevice = {0};
__ADI_USB_HDRC_SECTION_DATA
static PHYSICAL_EP_INFO  PhysicalEpAdditionalInfo[] =
                                                       {
                              {.dwEpFifoSize = 64  },
                              {.dwEpFifoSize = 128 },
                              {.dwEpFifoSize = 128 },
                              {.dwEpFifoSize = 128 },
                              {.dwEpFifoSize = 128 },
                              {.dwEpFifoSize = 1024 },
                              {.dwEpFifoSize = 1024 },
                              {.dwEpFifoSize = 1024 }
                                                       };

/* USB OTG function prototypes */
static ADI_INT_HANDLER(UsbOtgInterruptHandler);
static ADI_INT_HANDLER(UsbRxInterruptHandler);
static ADI_INT_HANDLER(UsbTxInterruptHandler);
static ADI_INT_HANDLER(UsbDmaInterruptHandler);

static ADI_DEV_BUFFER* GetRcvBuffer(ADI_USBOTG_DEVICE *pDev,s32_t PhyEpNum);
static s32_t TransmitEpZeroBuffer(ADI_DEV_1D_BUFFER *pBuffer, USB_EP_INFO *pEpInfo);
static s32_t InitProcessorSpecificConfiguration(ADI_USBOTG_DEVICE*);
static ADI_INT_HANDLER_RESULT UsbInterruptHandler(void);
static void SetSessionBit(void);
static void ClearSessionBit(void);
static u32 StartOtgDevice(ADI_USBOTG_DEVICE *pDev);
static void DriveVbusOn(ADI_USBOTG_DEVICE *pDev);
static void DriveVbusOff(ADI_USBOTG_DEVICE *pDev);
static void HostModeReset(ADI_USBOTG_DEVICE *pDev);
static void ResetOnDisconnect(void);
static int TransmitDataBuffer(ADI_USBOTG_DEVICE *pDev,s32_t dwEpNum);
static int TxErrorHandler(ADI_USBOTG_DEVICE *pDev,s32_t dwEpNum);
static int RxErrorHandler(ADI_USBOTG_DEVICE *pDev,s32_t dwEpNum);

#if defined(ADI_HDRC_STATISTICS)
#define ADI_HDRC_STAT_INC(x) do { UsbOtgDevice.Stats.x++; } while(0)
#else
#define ADI_HDRC_STAT_INC(x)
#endif /* ADI_HDRC_STATISTICS */

/**
 * Flushes the contents of the cache from specified start address to the end address
 */
__ADI_USB_HDRC_SECTION_CODE
static void FlushArea(void *StartAddress, void *EndAddress)
{
    StartAddress = (void *)(((u32_t)StartAddress)&(~31));
    ssync();
    while (StartAddress < EndAddress)
        FLUSH(StartAddress);
    ssync();
}

/**
 * Flushes and invalidates the contents of cache from the  specified start address to
 * the end address
 */
__ADI_USB_HDRC_SECTION_CODE
static void FlushInvArea(void *StartAddress, void *EndAddress)
{
    StartAddress = (void *)(((u32_t)StartAddress)&(~31));
    ssync();
    while (StartAddress < EndAddress)
        FLUSHINV(StartAddress);
    ssync();
}
/**
 * InitProcessorSpecificConfiguration
 * Initializes the processor specific configuration items. This functions gets called
 * from the UsbOtgStart.
 */
__ADI_USB_HDRC_SECTION_CODE
s32_t InitProcessorSpecificConfiguration(ADI_USBOTG_DEVICE *pDev)
{
s32_t Result;

     Result = adi_int_SICGetIVG(ADI_INT_USB_INT0, (u32*)&pDev->IvgUSBINT0);
     USB_ASSERT(Result != 0);

     Result =  adi_int_SICEnable(ADI_INT_USB_INT0);
     USB_ASSERT(Result != 0);

     Result = adi_int_SICGetIVG(ADI_INT_USB_INT1, (u32*)&pDev->IvgUSBINT1);
     USB_ASSERT(Result != 0);

     Result =  adi_int_SICEnable(ADI_INT_USB_INT1);
     USB_ASSERT(Result != 0);

     Result = adi_int_SICGetIVG(ADI_INT_USB_INT2, (u32*)&pDev->IvgUSBINT2);
     USB_ASSERT(Result != 0);

     Result =  adi_int_SICEnable(ADI_INT_USB_INT2);
     USB_ASSERT(Result != 0);

     Result = adi_int_SICGetIVG(ADI_INT_USB_DMAINT, (u32*)&pDev->IvgUSBDMAINT);
     USB_ASSERT(Result != 0);

     Result =  adi_int_SICEnable(ADI_INT_USB_DMAINT);
     USB_ASSERT(Result != 0);

     /* Enable Nested Interrupts for this IVG level */
     Result = adi_int_CECHook(pDev->IvgUSBINT2,UsbOtgInterruptHandler,pDev,true);
     WARN_INT_MGR_MEMORY_ERROR(Result);
     USB_ASSERT(Result != 0);

     Result = adi_int_CECHook(pDev->IvgUSBINT1,UsbTxInterruptHandler,pDev,true);
     WARN_INT_MGR_MEMORY_ERROR(Result);
     USB_ASSERT(Result != 0);

     Result = adi_int_CECHook(pDev->IvgUSBINT0,UsbRxInterruptHandler,pDev,true);
     WARN_INT_MGR_MEMORY_ERROR(Result);
     USB_ASSERT(Result != 0);

     Result = adi_int_CECHook(pDev->IvgUSBDMAINT,UsbDmaInterruptHandler,pDev,true);
     WARN_INT_MGR_MEMORY_ERROR(Result);
     USB_ASSERT(Result != 0);
     return(Result);
}

/**
 * Sets default USB OTG configuration, like IVG levels etc.
 *
 * Default mappings are given below they can be changed by changing USB_GLOBALINTR register.
 * USB Interrupt (USB_INTRUSB, USB_OTG_VBUS_IRQ) --> USB_INT0
 * Rx Interrupts  (USB_INTRRX) --> USB_INT1
 * Tx Interrupts  (USB_INTRTX) --> USB_INT2
 *
 * @param pDev points to the OTG device driver data object
 * @return Upon success returns 1.
 *
 */
__ADI_USB_HDRC_SECTION_CODE
static int SetProcessorSpecificDefaultConfiguration(ADI_USBOTG_DEVICE *pDev)
{
    /* by default we use ivg11 for all USB interrupts */
    pDev->IvgUSBINT0   = ik_ivg11;
    pDev->IvgUSBINT1   = ik_ivg11;
    pDev->IvgUSBINT2   = ik_ivg11;
    pDev->IvgUSBDMAINT = ik_ivg11;

    /* by default we use EZ-KIT settings, if user has different settings they
       can be configured via a command */

#if defined(__ADSP_MOAB__)

    /* On BF548 EZ-KITs, PE7 powers VBUS when high and does not provide
       power to VBUS when low.  Power should only be supplied to VBUS
       when in OTG host mode, not when in device mode.  Therefore we
       default to device mode at this point and if we are to run as a
       host later we turn PE7 on at that time.

       Note that BF548 EZ-KITs rev 1.0 required PE7 to be high at all
       times in order for either device or OTG host mode to function.
       It is not recommended to set PE7 high when in device mode. Rev
       1.1 and higher BF548 EZ-KITs have the proper fix. */

    pDev->VbusDriveFlag = ADI_FLAG_PE7;

#elif defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

    /* VBUS goes to port PG13 on BF527 EZ-KIT: default to OFF for device mode,
       for host mode you must turn PG13 on which will be done later if needed */

    pDev->VbusDriveFlag = ADI_FLAG_PG13;

#else
#error *** Processor not supported ***
#endif

    /* open and configure flag for output then default to OFF */
    adi_flag_Open(pDev->VbusDriveFlag);
    adi_flag_SetDirection(pDev->VbusDriveFlag, ADI_FLAG_DIRECTION_OUTPUT);
#if defined (__ADSP_MOCKINGBIRD__)
    adi_flag_Set(pDev->VbusDriveFlag);
#else
    adi_flag_Clear(pDev->VbusDriveFlag);
#endif

     return(1);
}

/**
 * Waits for one milli second irrespective of processor speed
 */
__ADI_USB_HDRC_SECTION_CODE
static void WaitOneMilliSec(void)
{
volatile unsigned long long int cur,nd;

    _GET_CYCLE_COUNT(cur);

    nd = cur + (__PROCESSOR_SPEED__/1000);
    while (cur < nd) {
    _GET_CYCLE_COUNT(cur);
    }
}

/**
 * Wait for passed number of milli-seconds
 */
__ADI_USB_HDRC_SECTION_CODE
static int WaitMilliSec(unsigned int msec)
{
    while (msec != 0) {
           WaitOneMilliSec();
           msec--;
    }
    return(0);
    
}

/**
 *
 * Device Mode Reset to default values. Gets called blackfin operating only in device mode.
 * This function gets called when Host issues reset.
 *
 */
__ADI_USB_HDRC_SECTION_CODE
static u32_t DeviceModeResetToDefault(ADI_USBOTG_DEVICE *pDev)
{
u16_t uStatus;

    USB_ASSERT(pDev->Mode != MODE_DEVICE);

    /* Set Index to 0 */
    *pUSB_INDEX = 0x0;

    /* Rest function address */
     *pUSB_FADDR = 0x0;

    /* Reset internal function address */
     pDev->DeviceAddress = 0x0;

    /* Flush FIFOs */
     *pUSB_CSR0 |= (1 << 8);
     ssync();

    /* Flush again */
     *pUSB_CSR0 |= (1 << 8);
     ssync();

     /* Reset EP0 CSR to zero */
     *pUSB_CSR0 = 0x0;

    /* Acknowledge any previously pending interrupts */
    uStatus = *pUSB_INTRUSB;
    *pUSB_INTRUSB = uStatus;

    uStatus = *pUSB_INTRRX;
    *pUSB_INTRRX = uStatus;

    uStatus = *pUSB_INTRTX;
    *pUSB_INTRTX = uStatus;

    /* Enable Reset and other connection interrupts */
    *pUSB_INTRUSBE    = (USB_INTRUSB_RESET_OR_BABBLE_B | USB_INTRUSB_CONN_B | USB_INTRUSB_DISCONN_B | USB_INTRUSB_SUSPEND_B);

    /* Disable all interrupts except EP0 */
    *pUSB_INTRTXE = 0x1;

    /* Disable all Rx Interrupts */
    *pUSB_INTRRXE = 0x0;

    pDev->State = DEV_STATE_RESET;

    return(0);
}
/*
 * Called only the case of OTG Host mode, when there is no connection detected.
 */
__ADI_USB_HDRC_SECTION_CODE
static u32_t HostModeResetToDefault(ADI_USBOTG_DEVICE *pDev)
{
volatile int i;

    USB_ASSERT(pDev->Mode != MODE_OTG_HOST);

   /* Acknowledge pending suspend interrupt */
   *pUSB_INTRUSB |= 0x1;

   /* Clear the pending session */
   *pUSB_OTG_DEV_CTL &= ~USB_OTG_DEV_CTL_SESSION_B;

   /* The for loop the call to WaitMilliSec replaced
    * suggested waiting 260 ms. This seems excessive and so
    * it is set to the more usual 100ms, instead

   WaitMilliSec(260);

   */
   WaitMilliSec(100);

   /* Reset power register */
   *pUSB_POWER  = 0x20;
   ssync(); csync();

   /* do not drive VBUS */
#if defined (__ADSP_MOCKINGBIRD__)
   adi_flag_Set(pDev->VbusDriveFlag);
#else
   adi_flag_Clear(pDev->VbusDriveFlag);
#endif

   pDev->EnumerationState = DEV_ENUM_RESET;

   return(1);
}

/**
 * Configures the USB Clock DIV register
 *
 */
__ADI_USB_HDRC_SECTION_CODE
static void SetClockDiv(void)
{
u32_t uCoreClock,uSystemClock,uVCO,uClockDivUsb;
u32_t Result;
u32_t uInterruptStatus = cli();

   Result = adi_pwr_GetFreq(&uCoreClock,&uSystemClock,&uVCO);
   if(Result == ADI_PWR_RESULT_SUCCESS)
   {
        uSystemClock = uSystemClock/1000;
        uClockDivUsb = ((uSystemClock/32 ) - 1);
        if (uClockDivUsb>0xFFF) uClockDivUsb = 0xFFF;
        *pUSB_SRP_CLKDIV |= (((u16_t)uClockDivUsb) & 0xfff);
        ssync();
   }
   sti(uInterruptStatus);
}
/**
 * Starts the OTG Device. Called from ADI_USB_ENABLE_USB I/O Control.
 */
__ADI_USB_HDRC_SECTION_CODE
static u32_t UsbOtgStart(ADI_USBOTG_DEVICE *pDev)
{
    u32_t Result=ADI_DEV_RESULT_SUCCESS;
    volatile unsigned int   v;

    /* Setup with the selected configuration */
    InitProcessorSpecificConfiguration(pDev);

    /* Setup the default EP0 physical object to hold the logical EP0 object */
    pDev->PhysicalEndpointObjects[EP0].pLogicalEndpointObj = pDev->pDeviceObj->pEndpointZeroObj;

    /* Configure PLL oscillator register */
    *pUSB_PLLOSC_CTRL = 0x30a8;
    ssync();

    /* Maximim packet size for EP0 Rx Endpoint */
    *pUSB_EP_NI0_RXMAXP = 64;

    /* Maximim packet size for EP0 Tx Endpoint */
    *pUSB_EP_NI0_TXMAXP = 64;

    /* set USB clock div in case we can get the current system clock value */
    SetClockDiv();

    /* for Host mode, set the SRP Counter Enable bit */
    if (pDev->Mode==MODE_OTG_HOST) {
        *pUSB_SRP_CLKDIV |= 0x8000;
    }

    /* Enable Reset and other connection interrupts */
    *pUSB_INTRUSBE    = (USB_INTRUSB_RESET_OR_BABBLE_B | USB_INTRUSB_CONN_B | USB_INTRUSB_DISCONN_B);
    if (pDev->Mode==MODE_OTG_HOST) {
        *pUSB_INTRUSBE    |= (USB_INTRUSB_SESSION_REQ_B | USB_INTRUSB_VBUSERROR_B);
    }

    /*
     * Enable USB and the data endpoint logic by setting appropriate bits in Global Control register
     * With the execution of below statement we are ready to receive and transmit setup packets on EP0
     */
    *pUSB_GLOBAL_CTL = (USB_GLOBAL_ENABLE_B | USB_GLOBAL_CTRL_ALL_TX_RX_B );

    /* enable CONN_B & SUSPEND interrupts */
    *pUSB_INTRUSBE |= USB_INTRUSB_DISCONN_B | USB_INTRUSB_CONN_B | USB_INTRUSB_SUSPEND_B;

    if (pDev->Mode==MODE_OTG_HOST) {
        *pUSB_INTRUSBE |= (USB_INTRUSB_SESSION_REQ_B | USB_INTRUSB_VBUSERROR_B);
    }

    /* enable DRIVE_VBUS_OFF interrupts */
    if (pDev->Mode!=MODE_OTG_HOST) {
        /* only set for device */
        *pUSB_OTG_VBUS_MASK |= 0x0002;
    }

    /* Enable Rx, Tx Interrupt on EP0 */
    *pUSB_INTRTXE = 0x1;

    /* set up to detect a session connection */
    if (pDev->Mode==MODE_OTG_HOST) {
        /* drive VBUS for host  */
        DriveVbusOn(pDev);
    }
    else {

        /* set restore bit - puts out of hibernate */
        *pUSB_APHY_CNTRL2  |= 0x2;

        /* set session bit for device */
        SetSessionBit();
    }

    /* At this point insertion of A or B plug and subsequent connection to a Mass storage device or a host
     * is handled asynchronously via interrupts
    */
    return(Result);
}

/**
 * ResetPhysicalEndPoints
 *
 * Resets all physical endpoints.
 *
 * @param pDev pointer to the USB Device object
 * @return 1 upon success
 *
 * @see ResetUsbOtgDevice
 */
__ADI_USB_HDRC_SECTION_CODE
static s32_t ResetPhysicalEndPoints(ADI_USBOTG_DEVICE *pDev)
{
s32_t i;
    /*
     * Enable the USBDRC and control logic for all endpoints.This is required to enable the PHY and access the
     * endpoint FIFOs See Global Control register def (page 24) of USB OTG specification.
     * TODO: Check if the below call is really required.
     */
    /* *pUSB_GLOBAL_CTL = (USB_GLOBAL_ENABLE_B | USB_GLOBAL_CTRL_ALL_TX_RX_B); */

    /* Disable all interrupts except EP0 */
    *pUSB_INTRTXE = 0x1;

    /* Disable all Rx Interrupts */
    *pUSB_INTRRXE = 0x0;

    /*
     * loop through all endpoints and reset critical registers
     * TODO: Peform any Endpoint specific reset actions
     */
     for(i=1;i<NUM_PHYSICAL_ENDPOINTS;i++)
     {
       *pUSB_INDEX = i;
     }

     *pUSB_INDEX = 0;
     return(1);
}

/**
 * ResetUsbOtgDevice  Internal Driver function
 * Performs reset functionality which includes the following.
 *  - FAddr initialized to zero
 *  - Index register set to zero
 *  - All endpoint FIFOs flushed
 *  - All CSRs cleared
 *  - All interrupts disabled. (different from that of hardware reset )
 */
__ADI_USB_HDRC_SECTION_CODE
static int ResetUsbOtgDevice(ADI_USBOTG_DEVICE *pDev)
{
u32_t  uInterruptStatus = cli();

    /* Acknowledge all USB interrupts and disable them */
    *pUSB_INTRUSB = (*pUSB_INTRUSB & 0xff);
    *pUSB_INTRUSBE = 0x0;

    /* Acknowledge all pending Tx interrupts and disable them */
    *pUSB_INTRTX = (*pUSB_INTRTX & 0xff);
    *pUSB_INTRTXE = 0x0;

    /* Acknowledge all pending Rx interrupts */
    *pUSB_INTRRX = (*pUSB_INTRRX & 0xff);
    *pUSB_INTRRXE = 0x0;

    /* Device Address set to zero */
    *pUSB_FADDR = 0x0;

    /* Endpoint Index register set to zero  */
    *pUSB_INDEX = 0x0;

    /* Reset all data endpoints */
    ResetPhysicalEndPoints(pDev);

    sti(uInterruptStatus);

    return(1);
}

/*
 * Requests IN tokens to get data on EP0
 */
__ADI_USB_HDRC_SECTION_CODE
static u32_t EpZeroOTGRequestINTokens(ADI_USBOTG_DEVICE *pDev)
{
s32_t dwSavedEpNum = *pUSB_INDEX;

    USB_ASSERT(pDev->Mode != MODE_OTG_HOST);

    *pUSB_INDEX = EP0;
    WaitMilliSec(3);
 
    /* Check if RX STALL is set if so clear stall */ 
    if(*pUSB_CSR0 & (1 << 2)) 
    {
       *pUSB_CSR0 &= ~(1 << 2); 
    }

    /* Set ReqPkt bit, Disable ping token bit.
     * TODO: for the multi-byte transfers for the last packet transfer set StatusPkt (bit:6) 
     */
    *pUSB_CSR0 = ((1 << 5) | (1<< 11));
    
    *pUSB_INDEX = dwSavedEpNum;
    
    ssync();
    {
       volatile int i=0;
       WaitMilliSec(13);
    }
    return(0);
}
/**
 *
 * We bind each logical Endpoint object with the physical Endpoint objects.The Endpoint
 * object that is passed in is the logical endpoint object present in the currently
 * active configuration.
 *
 * Operating Mode:  MODE_DEVICE
 */
__ADI_USB_HDRC_SECTION_CODE
static s32_t DevBindPhysicalEndpoints(ENDPOINT_OBJECT *pLogicalEp)
{
s32_t i=1,dwEpNum,dwSavedEpNum;
PHYSICAL_ENDPOINT_OBJECT *pPhyEpO;
ENDPOINT_OBJECT *pLogEp = pLogicalEp;
ENDPOINT_DESCRIPTOR *pEpD;
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;

    USB_ASSERT(pLogEp == NULL);

    dwSavedEpNum = *pUSB_INDEX;

    /* Get each logical endpoint */
    while(pLogEp != NULL)
    {
        /* Get the endpoint descriptor */
        pEpD = pLogEp->pEndpointDesc;

        /* Get the endpoint number */
        dwEpNum = pEpD->bEndpointAddress & 0x0F;

        /* Get the logical endpoint object associated for this endpoint. */
        pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj = pLogEp;

        *pUSB_INDEX = (unsigned char)(dwEpNum);
        i = dwEpNum;

        /* Set endpoint ID */
        pDev->PhysicalEndpointObjects[dwEpNum].PhysicalEndpointID  = pEpD->bEndpointAddress & 0x0F;

        USB_ASSERT( (pEpD->wMaxPacketSize == 0) || (pEpD->wMaxPacketSize > 0x7FF));

        switch((pEpD->bEndpointAddress >> 7) & 0x1)
        {
            case DEV_OUT: /* OUT */

                 if ( (pDev->RxDmaMode == DMA_MODE_0) && (IS_BULK(pEpD->bAttributes)) )
                     *pUSB_RX_MAX_PACKET = (pEpD->wMaxPacketSize >= 512) ? 1024 : 128; 
                 else
                     *pUSB_RX_MAX_PACKET = pEpD->wMaxPacketSize;


                    /* check if there are any packets in the FIFO if so flush */
                    if((*pUSB_RXCSR & 0x1) || (*pUSB_RXCSR & 0x2))
                    {
                       /* flush twice to handle double buffering */                    
                       *pUSB_RXCSR |= (1 << 4);
                       *pUSB_RXCSR |= (1 << 4);
                    }

	            /* set ClearDataToggle bit to keep endpoint in known state */
	            *pUSB_RXCSR |= (1 << 7);

                    /* Rest all */
                    *pUSB_RXCSR = 0x0;

                    /* Endpoint type is Isonchronous */
                    if(IS_ISONCHRONOUS(pEpD->bAttributes))
                         *pUSB_RXCSR  |= (1 << 14);

             break;

             case DEV_IN: /* IN */
                     /* configure max packet size from the endpoint */
                     *pUSB_TX_MAX_PACKET = pEpD->wMaxPacketSize;

                     /* check if there are any packets in the FIFO if so flush */
                     if((*pUSB_TXCSR & 0x1) || (*pUSB_TXCSR & 0x2))
		             {
                        /* flush twice to handle double buffering */                    
                        *pUSB_TXCSR |= (1 << 3);
                        *pUSB_TXCSR |= (1 << 3);
		             }

                     /* set ClearDataToggle bit to keep the endpoint in known state */
                     *pUSB_TXCSR |= (1 << 6);

                     /* Reset TXCSR to zero */
                      *pUSB_TXCSR = 0x0;

                     /* Endpoint type is Isonchronous */
                     if(IS_ISONCHRONOUS(pEpD->bAttributes))
                          *pUSB_TXCSR  |= (1 << 14);
                     ssync();
             break;
        }
       /* get the next logical endpoint */
       pLogEp = pLogEp->pNextActiveEpObj;
    }
    *pUSB_INDEX = (u16_t)dwSavedEpNum;
    pDev->State = DEV_STATE_CONFIGURED;

    return(1);
}

/**
 *
 * We bind each logical Endpoint object with the physical Endpoint objects.The Endpoint
 * object that is passed in is the logical endpoint object present in the currently
 * active configuration.
 *
 * Operating Mode:  MODE_OTG_HOST
 *
 */
__ADI_USB_HDRC_SECTION_CODE
static s32_t HostBindPhysicalEndpoints(ENDPOINT_OBJECT *pLogicalEp)
{
s32_t i=1,dwEpNum,dwSavedEpNum;
PHYSICAL_ENDPOINT_OBJECT *pPhyEpO;
ENDPOINT_OBJECT *pLogEp = pLogicalEp;
ENDPOINT_DESCRIPTOR *pEpD;
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;

    USB_ASSERT(pLogEp == NULL);

    dwSavedEpNum = *pUSB_INDEX;

    *pUSB_CSR0 |= (1 << 11);

    /* Get each logical endpoint */
    while(pLogEp != NULL)
    {
        /* Get the endpoint descriptor */
        pEpD = pLogEp->pEndpointDesc;

        /* For host side we use the same Endpoint number as that of the ID  and configure
         * the type register with the remote endpoint number
         */
        //dwEpNum = pEpD->bEndpointAddress & 0x0F;
        dwEpNum = pLogEp->ID;

        /* Get the logical endpoint object associated for this endpoint. */
        pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj = pLogEp;

        *pUSB_INDEX = (unsigned char)(dwEpNum);
        i = dwEpNum;

        /* Set endpoint ID */
        pDev->PhysicalEndpointObjects[dwEpNum].PhysicalEndpointID  = pEpD->bEndpointAddress & 0x0F;

        USB_ASSERT( (pEpD->wMaxPacketSize == 0) || (pEpD->wMaxPacketSize > 0x7FF));

    /* in host mode we OUT endpoint is used to write data and IN endpoint is used to read data
     */
        switch((pEpD->bEndpointAddress >> 7) & 0x1)
        {
            case DEV_OUT: /* OUT */
                 /* configure max packet size from the endpoint */
                 *pUSB_TX_MAX_PACKET = pEpD->wMaxPacketSize;

                 /* check if there are any packets in the FIFO if so flush */
                 if((*pUSB_TXCSR & 0x1) || (*pUSB_TXCSR & 0x2))
                 {
                      /* flush twice to handle double buffering */                    
                      *pUSB_TXCSR |= (1 << 3);
                      *pUSB_TXCSR |= (1 << 3);
                 }

                 /* set ClearDataToggle bit to keep the endpoint in known state */
                 *pUSB_TXCSR |= (1 << 6);

                 /* Reset TXCSR to zero */
                 *pUSB_TXCSR = 0x0;

                 /* Set Tx type */
                  *pUSB_TXTYPE = ((pEpD->bEndpointAddress & 0xf) | ((pEpD->bAttributes & 0x3) << 4) );

                  if(IS_BULK(pEpD->bAttributes))
                  {
                    *pUSB_TXINTERVAL = (pEpD->bInterval & 0xf);
                  }
                  else
                  {
                    *pUSB_TXINTERVAL = 1;
                  }

                  ssync();
             break;

             case DEV_IN: /* IN */                

                  if ( (pDev->RxDmaMode == DMA_MODE_0) && (IS_BULK(pEpD->bAttributes)) )
                      *pUSB_RX_MAX_PACKET = (pEpD->wMaxPacketSize >= 512) ? 1024 : 128; 
                  else
                      *pUSB_RX_MAX_PACKET = pEpD->wMaxPacketSize;

                   /* check if there are any packets in the FIFO if so flush */
                   if((*pUSB_RXCSR & 0x1) || (*pUSB_RXCSR & 0x2))
                   {
                       /* flush twice to handle double buffering */                    
                       *pUSB_RXCSR |= (1 << 4);
                       *pUSB_RXCSR |= (1 << 4);
                   }

                   /* set ClearDataToggle bit to keep endpoint in known state */
                  *pUSB_RXCSR |= (1 << 7);

                  /* Reset all */
                  *pUSB_RXCSR = 0x0;

                  /* RX TYPE */
                  *pUSB_RXTYPE = ((pEpD->bEndpointAddress & 0xf) | ((pEpD->bAttributes & 0x3) << 4) );

                  if(IS_BULK(pEpD->bAttributes))
                  {
                    *pUSB_RXINTERVAL = (pEpD->bInterval & 0xf);
                  }
                  else
                  {
                    *pUSB_RXINTERVAL = pEpD->bInterval; /* TODO: check for isonchrnous/interrupt */
                  }

                  ssync();
             break;
        }
       /* get the next logical endpoint */
       pLogEp = pLogEp->pNextActiveEpObj;
    }
    *pUSB_INDEX = (u16_t)dwSavedEpNum;
    pDev->State = DEV_STATE_CONFIGURED;
    return(1);
}


/**
 * CheckUsbOtgDevice
 * Checks the OTG devices by looking at some predefined register values. This
 * function must be called before the driver changes any of the default reset
 * values.
 */
__ADI_USB_HDRC_SECTION_CODE
static int CheckUsbOtgDevice(void)
{
    /* Check Global Interrupt Control register Reset value : 0x0111 */
    if( (*pUSB_GLOBINTR != 0x111) || (*pUSB_INTRTXE != 0x00FF) || (*pUSB_INTRRXE != 0x00FE) )
        return(-1);

    return(1);
}
/**
 * QueueBuffer  driver internal function
 *
 * Queue buffer adds to the buffer to the specified queue and specified endpoint. This function gets
 * invoked if we are unable to send data because the EP FIFO is full. Once TX FIFO becomes empty we
 * will
 */
__ADI_USB_HDRC_SECTION_CODE
static void QueueBuffer(ADI_DEV_BUFFER *pBuffer,PUSB_EP_INFO pEpInfo,EP_QUEUE eQueue)
{
ADI_DEV_BUFFER **pQueueHead;
ADI_DEV_1D_BUFFER *tmpBuffer;

    switch(eQueue)
    {
        case FREE_RCV_QUEUE:
             pQueueHead = &pEpInfo->pFreeRcvList;
             break;
        case FREE_XMT_QUEUE:
             pQueueHead = &pEpInfo->pFreeXmtList;
             break;
        case PROC_RCV_QUEUE:
             pQueueHead = &pEpInfo->pProcessedRcvList;
             break;
        case WAIT_XMT_QUEUE:
             pQueueHead = &pEpInfo->pQueuedXmtList;
             break;
        default:
             break;
    }

    if(*pQueueHead == NULL)
        *pQueueHead = pBuffer;
    else
    {
        tmpBuffer = (ADI_DEV_1D_BUFFER*)*pQueueHead;

        while(tmpBuffer->pNext != NULL)
            tmpBuffer = tmpBuffer->pNext;
        tmpBuffer->pNext = (ADI_DEV_1D_BUFFER*)pBuffer;
    }
}

/**
 * QueueBufferAtHead  driver internal function
 *
 * Queue buffer adds to the buffer to the specified queue and specified endpoint. This function gets
 * invoked if we are unable to send data because the EP FIFO is full. Once TX FIFO becomes empty we
 * will
 */
__ADI_USB_HDRC_SECTION_CODE
static void QueueBufferAtHead(ADI_DEV_BUFFER *pBuffer,PUSB_EP_INFO pEpInfo,EP_QUEUE eQueue)
{
ADI_DEV_BUFFER **pQueueHead;
ADI_DEV_1D_BUFFER *tmpBuffer;

    switch(eQueue)
    {
        case FREE_RCV_QUEUE:
             pQueueHead = &pEpInfo->pFreeRcvList;
             break;
        case FREE_XMT_QUEUE:
             pQueueHead = &pEpInfo->pFreeXmtList;
             break;
        case PROC_RCV_QUEUE:
             pQueueHead = &pEpInfo->pProcessedRcvList;
             break;
        case WAIT_XMT_QUEUE:
             pQueueHead = &pEpInfo->pQueuedXmtList;
             break;
        default:
             break;
    }

    if(*pQueueHead == NULL)
        *pQueueHead = pBuffer;
    else
    {
        tmpBuffer = (ADI_DEV_1D_BUFFER*)*pQueueHead;
        *pQueueHead = pBuffer;

    ((ADI_DEV_1D_BUFFER*)pBuffer)->pNext = tmpBuffer;
    }
}

/*********************************************************************
*
*   Function:       pddOpen
*
*   Description:    Opens the Network Driver and does initialization.
*
*********************************************************************/

__ADI_USB_HDRC_SECTION_CODE
static u32 adi_pdd_Open(                       /* Open a device */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle, /* device manager handle */
    u32                     DeviceNumber,  /* device number */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,  /* device handle */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,   /* pointer to PDD handle location */
    ADI_DEV_DIRECTION       Direction,     /* data direction */
        void                    *pCriticalRegionArg,/* imask storage location */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,   /* handle to the DMA manager */
    ADI_DCB_HANDLE          DCBHandle,   /* callback handle */
    ADI_DCB_CALLBACK_FN     DMCallback   /* device manager callback function */
)
{
u32 Result= ADI_DEV_RESULT_DEVICE_IN_USE;
ADI_USBOTG_DEVICE *pDev;
void *pExitCriticalArg;
bool returnFlag = false;

    pExitCriticalArg = adi_int_EnterCriticalRegion(pCriticalRegionArg);
    pDev = &UsbOtgDevice;


    switch (pDev->State)
    {
        case DEV_STATE_RESET:
             pDev->State = DEV_STATE_OPEN_IN_PROCESS;

         if(pDev->Mode == MODE_OTG_HOST)
             pDev->EnumerationState = DEV_ENUM_RESET;
        break;

        case DEV_STATE_OPEN_IN_PROCESS:
             pDev->State = DEV_STATE_OPEN_IN_PROCESS;
             returnFlag = true;
        break;

        case DEV_STATE_OPENED:
             *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDev;
             Result = ADI_DEV_RESULT_SUCCESS;
             returnFlag = true;
        break;

        case DEV_STATE_STARTED:
             returnFlag = true;
        break;

        case DEV_STATE_CLOSED:
        break;

        case DEV_STATE_CONFIGURED:
             returnFlag = true;
        break;
       default:
       break;
    }

    adi_int_ExitCriticalRegion(pExitCriticalArg);

    if(returnFlag)
        return Result;

    Result = ADI_DEV_RESULT_SUCCESS;

    /* We come here only if we need to configure the device. */
    pDev->DeviceHandle = DeviceHandle;
    pDev->DMAHandle    = DMAHandle;
    pDev->DCBHandle    = DCBHandle;
    pDev->DMCallback   = DMCallback;
    pDev->Direction    = Direction;
    pDev->Started      = false;
    pDev->pCriticalRegionArg = pCriticalRegionArg;

    /* default cache is turned off. */
    pDev->Cache                = false;
    pDev->NumPhysicalEndpoints = NUM_PHYSICAL_ENDPOINTS;
    pDev->BufferPrefix         = 0;
    pDev->Mode                 = MODE_NONE;  /* Default configuration */
    pDev->Speed                = ADI_USB_DEVICE_SPEED_UNKNOWN;
    pDev->DeviceID             = adi_usb_CreateDevice(&pDev->pDeviceObj);

    /* default DMA operating modes */
#if (defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) ) 
    pDev->RxDmaMode            = DMA_MODE_0;
#else
    pDev->RxDmaMode            = DMA_MODE_1;
#endif /* __ADSP_KOOKABURRA__ or __ADSP_MOCKINGBIRD__ */

    pDev->TxDmaMode            = DMA_MODE_1;

    if(pDev->DeviceID == -1)
       return (ADI_DEV_RESULT_FAILED);

    /* Inform Core about the number of physical endpoints */
    adi_usb_SetPhysicalEndPointInfo(pDev->NumPhysicalEndpoints,PhysicalEpAdditionalInfo);

    /* Perform some primitive checking with the device */
    CheckUsbOtgDevice();

    /* we only support bidirectional traffic for this device */
    if ( pDev->Direction != ADI_DEV_DIRECTION_BIDIRECTIONAL )
         return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);

    *pPDDHandle = (ADI_DEV_PDD_HANDLE)pDev;

    /* Configures the default parameters. */
    SetProcessorSpecificDefaultConfiguration(pDev);

    /* Reset the OTG device */
    ResetUsbOtgDevice(pDev);

    pExitCriticalArg = adi_int_EnterCriticalRegion(pCriticalRegionArg);
    pDev->State = DEV_STATE_OPENED;
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    return(ADI_DEV_RESULT_SUCCESS);
}

/**
 * Schedules DMA mode -0 transfer.This function has to be called if packet is 
 * already present in the RX FIFO either in host mode or device mode.
 * Buffer is returned from the DMA completion.
 */
__ADI_USB_HDRC_SECTION_CODE
static s32_t ScheduleRxDMA0(ADI_USBOTG_DEVICE *pDev,s32_t dwEpNum)
{
ENDPOINT_OBJECT *pEpO=NULL;
USB_EP_INFO *pEpInfo;
char *pData;
ADI_DEV_1D_BUFFER *pCurrentBuffer;
s32_t dwSavedEpNum,*pDmaMode,wRxCount,dwNumReqBytes;


    dwSavedEpNum = *pUSB_INDEX;
    pEpO = pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj;
    USB_ASSERT(pEpO == NULL);

    pDmaMode = &pDev->PhysicalEndpointObjects[dwEpNum].DmaMode;
	pEpInfo = &pEpO->EPInfo;

	pCurrentBuffer = (ADI_DEV_1D_BUFFER*)pEpO->EPInfo.pTransferBuffer;

	dwNumReqBytes = pCurrentBuffer->ElementCount * pCurrentBuffer->ElementWidth;

	/* select current endpoint */
	*pUSB_INDEX = dwEpNum;

	pData = (char*)pCurrentBuffer->Data;
	*pDmaMode = 0;

	wRxCount = *pUSB_RXCOUNT;

	
	if(dwNumReqBytes < wRxCount)
   	    wRxCount = dwNumReqBytes;
   	

	if(wRxCount == 0) 
	{
        //USB_ASSERT(wRxCount == 0);
        return(0);
	}

	/* setup the data buffer */
	set_dma_addr(dwEpNum,(u16_t)ADDR_LOW(pData),(u16_t)ADDR_HIGH(pData));

	/* setup no# bytes to read */
	set_dma_count(dwEpNum,(u16_t)ADDR_LOW(wRxCount),(u16_t)ADDR_HIGH(wRxCount));

	/* enable dma and generate interrupt */
	set_dma_ctrl(dwEpNum,(u16_t)( USB_DMA_CTRL_DMA_INT_ENA_B | USB_DMA_CTRL_DMA_ENA_B | ((dwEpNum&0xf) << 4)));
	pDev->PhysicalEndpointObjects[dwEpNum].eIsDmaPending = 1;

	pEpInfo->EpBytesProcessed = wRxCount;                         

	/* acknowledge the receive interrupt */
	if(*pUSB_INTRRX & (1 << dwEpNum))
	   *pUSB_INTRRX  = (1 << dwEpNum);

	*pUSB_INDEX = dwSavedEpNum;

	return(1);
}

/**
 * Schedules a read operation.
 *
 */
__ADI_USB_HDRC_SECTION_CODE
static s32_t ReceiveDataBuffer(ADI_USBOTG_DEVICE *pDev,s32_t dwEpNum)
{
s32_t dwSavedEpNum,*pDmaMode,dwBytesToRcv=0;
ENDPOINT_OBJECT *pEpO=NULL;
ADI_DEV_1D_BUFFER *pCurrentBuffer;
u32_t uInterruptStatus=cli();
char *pData;
int dwShortPacketSize=0,dwMaxEpSize=0;

    dwSavedEpNum = *pUSB_INDEX;
    pEpO = pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj;

    if(pEpO->EPInfo.pTransferBuffer !=NULL)
    {
       sti(uInterruptStatus);
       return(0);
    }

    pCurrentBuffer = (ADI_DEV_1D_BUFFER*)GetRcvBuffer(pDev,dwEpNum);
    pEpO->EPInfo.pTransferBuffer = (ADI_DEV_BUFFER*)pCurrentBuffer;

    USB_ASSERT(pCurrentBuffer == NULL);

    /* get the total number of bytes to receive */
    dwBytesToRcv   = pCurrentBuffer->ElementCount * pCurrentBuffer->ElementWidth;

    if(pDev->RxDmaMode == DMA_MODE_0)
    {
        pDmaMode = &pDev->PhysicalEndpointObjects[dwEpNum].DmaMode;
        *pUSB_INDEX = dwEpNum;

        pEpO->EPInfo.EpBytesProcessed =0;

        *pUSB_INTRRXE |= (1 << dwEpNum);
        *pDmaMode = 0;
    
         /* we do not have packet yet, so host mode we send IN tokens */
         if(pDev->Mode == MODE_OTG_HOST)
	     {
            /* Send IN Tokens to get data */
             *pUSB_RXCSR |= (1 << 5); 
         }
    }
    else /* If DMA MODE 1 */
    {

       USB_ASSERT(dwBytesToRcv == 0);

       /* if the endpoint type is bulk and the requested number of bytes are more than
        * the maximum endpoint size then we schedule DMA mode 1 operation on that endpoint.
        * if the request number of bytes is less than the maximum endpoint packet size we
        * simply enable the rx interrupt and let the DevRxInterruptHandler transfer the data.
        */
        pDmaMode = &pDev->PhysicalEndpointObjects[dwEpNum].DmaMode;
	    *pUSB_INDEX = dwEpNum;
	
        if(IS_BULK(pEpO->pEndpointDesc->bAttributes) && (dwBytesToRcv > pEpO->pEndpointDesc->wMaxPacketSize))
        {
            pEpO->EPInfo.pTransferBuffer = (ADI_DEV_BUFFER*)pCurrentBuffer;
            pData = pCurrentBuffer->Data;

            USB_ASSERT(pData == NULL);
      
            dwMaxEpSize = pEpO->pEndpointDesc->wMaxPacketSize;

            USB_ASSERT(dwMaxEpSize == 0);

	        dwShortPacketSize =  (dwBytesToRcv % dwMaxEpSize);

            /* setup the data buffer */
            set_dma_addr(dwEpNum,(u16_t)ADDR_LOW(pData),(u16_t)ADDR_HIGH(pData));

            /* setup no# bytes to read */
            set_dma_count(dwEpNum,(u16_t)ADDR_LOW(dwBytesToRcv),(u16_t)ADDR_HIGH(dwBytesToRcv));

            /* setup dma mode 1 */
            set_dma_ctrl(dwEpNum,(u16_t)( USB_DMA_CTRL_DMA_INT_ENA_B | USB_DMA_CTRL_DMA_ENA_B | ((dwEpNum&0xf) << 4) | USB_DMA_CTRL_DMA_MODE_B));
       
       
            /* enable DMA mode 1 for multiple Rx packets */
            if(pDev->Mode == MODE_DEVICE)
            {
               *pUSB_INTRRXE &= ~(1 << dwEpNum);
               *pUSB_RXCSR |= ((1 << 13) | (1 << 15));
            }
            else
            {
                ENDPOINT_DESCRIPTOR *pEpD = pEpO->pEndpointDesc;

                *pUSB_RXTYPE = ((pEpD->bEndpointAddress & 0xf) | ((pEpD->bAttributes & 0x3) << 4) );         
                /* Enable receive interrupt for that endpoint */
                if(dwShortPacketSize) 
                    *pUSB_INTRRXE |= (1 << dwEpNum);

                *pUSB_RXCSR = ((1 << 5) | (1 << 11) | (1 << 13) | (1 << 15) | (1 << 14));
             }

             /* set dma mode to 1 */
              *pDmaMode = 1;

             USB_ASSERT(pDev->PhysicalEndpointObjects[dwEpNum].eIsDmaPending == 1);

             pDev->PhysicalEndpointObjects[dwEpNum].eIsDmaPending = 1;
        }
        else
        {
            /* RxPktRdy is set then we will schedule transfer right away */
            if(*pUSB_RXCSR & 0x1 || ((*pUSB_INTRRX) & (1 << dwEpNum)))
            {
                 ADI_HDRC_STAT_INC(dwRxPktInFIFO);

                 if(ScheduleRxDMA0(pDev,dwEpNum)) 
                 {
                     *pUSB_INDEX = dwSavedEpNum;
                      sti(uInterruptStatus);
                      return(1);
                  }
             }

	         /* we do not have packet yet, so host mode we send IN tokens */
	         if(pDev->Mode == MODE_OTG_HOST)
	         {
                 /* Send IN Tokens to get data */
                 *pUSB_RXCSR |= (1 << 5); 
	         }
	         else
	         {
                 if(*pUSB_RXCSR & (1 << 15))
                    *pUSB_RXCSR &= ~((1 << 13) | (1 << 15));
	         }

             /* Set Dma mode to 0 */
             *pDmaMode = 0;

             /* Enable receive interrupt for that endpoint */
             *pUSB_INTRRXE |= (1 << dwEpNum);
           }
    } /* DMA_MODE_0 */


    *pUSB_INDEX = dwSavedEpNum;
    sti(uInterruptStatus);
    return(1);
}
/*********************************************************************
*
*   Function:       pddRead
*
*   Description:    Gives list of read buffers to the driver
*
*********************************************************************/
__ADI_USB_HDRC_SECTION_CODE
static u32 adi_pdd_Read(                /* Reads data or queues an inbound buffer to a device */
    ADI_DEV_PDD_HANDLE PDDHandle,           /* PDD handle */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
ENDPOINT_OBJECT *pEpO=NULL;
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;
s32_t  dwEpNum,dwSavedEpNum;
u32_t  uInterruptStatus;
ADI_DEV_1D_BUFFER *pCurrentBuffer;
char *pData;

    USB_ASSERT(pBuffer == NULL);

    /* Get the endpoint number */
    dwEpNum =((ADI_DEV_1D_BUFFER*)pBuffer)->Reserved[4];

    USB_ASSERT((dwEpNum < 0) || (dwEpNum > 7));

    uInterruptStatus = cli();
    dwSavedEpNum = *pUSB_INDEX;

    /* We add the buffer to the top of the list, for bulk endpoints the ElementCount represents
     * the number of bytes to read for isonchrnouns and interrupt endpoints its the total buffer
     * size.
     */
    pEpO = pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj;

    USB_ASSERT(pEpO == NULL);

    /* make sure its OUT endpoint in case of Device mode
     * we do not check for EP0 as its bi-directional
     */
    if(dwEpNum != EP0)
    {
       if(pDev->Mode == MODE_DEVICE)
         USB_ASSERT(((pEpO->pEndpointDesc->bEndpointAddress >> 7) & 0x1) != 0);
       else
         USB_ASSERT(((pEpO->pEndpointDesc->bEndpointAddress >> 7) & 0x1) != 1);
    }

    pCurrentBuffer = (ADI_DEV_1D_BUFFER*)pBuffer;
    USB_ASSERT((pCurrentBuffer->ElementCount == 0) || (pCurrentBuffer->ElementWidth == 0));
   // USB_ASSERT(pCurrentBuffer->CallbackParameter != pCurrentBuffer);

    /* check if data cache is on */
    if(pDev->Cache)
    {
      char  *pData;
      s32_t dwNumBytes;
      ADI_DEV_1D_BUFFER *pDataBuffer = (ADI_DEV_1D_BUFFER*)pBuffer;

      while(pDataBuffer != NULL)
      {
          pData = (char*)pDataBuffer->Data;
          dwNumBytes = pDataBuffer->ElementCount * pDataBuffer->ElementWidth;

          FlushInvArea((void*)pData,(void*)(pData+dwNumBytes));
          pDataBuffer = pDataBuffer->pNext;
      }
    }	


    if(dwEpNum != EP0) {
        QueueBuffer(pBuffer,&pEpO->EPInfo,FREE_RCV_QUEUE);
        ReceiveDataBuffer(pDev,dwEpNum);
    }
    else {
        QueueBufferAtHead(pBuffer,&pEpO->EPInfo,FREE_RCV_QUEUE);
      
        if(pDev->Mode == MODE_OTG_HOST) {
            EpZeroOTGRequestINTokens(pDev);
        } else {
            *pUSB_INTRRXE |= 0x1;
        }
          
    }

    *pUSB_INDEX = dwSavedEpNum;
    sti(uInterruptStatus);

    return(ADI_DEV_RESULT_SUCCESS);
}
/*********************************************************************
*
*   Function:   pddWrite
*
*   Description:    Sends packet over the physical channel
*
*********************************************************************/
__ADI_USB_HDRC_SECTION_CODE
static u32 adi_pdd_Write(                  /* Writes data or queues an outbound buffer to a device */
    ADI_DEV_PDD_HANDLE PDDHandle,      /* PDD handle */
    ADI_DEV_BUFFER_TYPE BufferType,/* buffer type */
    ADI_DEV_BUFFER *pBuffer            /* pointer to buffer */
)
{
unsigned char ucEpNum;
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;
ADI_DCB_CALLBACK_FN  pEpCbFn;
PUSB_EP_INFO pEpInfo;
int i;
u32_t  uInterruptStatus;

    /* get the EP address */
    ucEpNum = ((ADI_DEV_1D_BUFFER*)pBuffer)->Reserved[BUFFER_RSVD_EP_ADDRESS] & 0xff;

    USB_ASSERT(ucEpNum > NUM_PHYSICAL_ENDPOINTS);
    USB_ASSERT(pBuffer == NULL);

    /* check if data cache is on */
    if(pDev->Cache)
    {
      ADI_DEV_1D_BUFFER *pDataBuffer = (ADI_DEV_1D_BUFFER*)pBuffer;
      char  *pData = (char*)pDataBuffer->Data;
      s32_t dwNumBytes = pDataBuffer->ElementCount * pDataBuffer->ElementWidth;

      FlushArea((void*)pData,(void*)(pData + dwNumBytes));
    }
    pEpInfo = &pDev->PhysicalEndpointObjects[ucEpNum].pLogicalEndpointObj->EPInfo;

    if(ucEpNum == EP0)
    {
        /* We try to send the buffer, if we can not (FIFO FULL) then we will queue it */
        if(!TransmitEpZeroBuffer((ADI_DEV_1D_BUFFER*)pBuffer,pEpInfo))
        {
            /* Get the associated logical Endpoint object */
             QueueBuffer(pBuffer,pEpInfo,WAIT_XMT_QUEUE);
             return(ADI_DEV_RESULT_FAILED);
        }
        else
        {
             pEpCbFn = pDev->PhysicalEndpointObjects[EP0].pLogicalEndpointObj->EPInfo.EpCallback;
             pEpCbFn(pDev->DeviceHandle,ADI_USB_EVENT_TX_COMPLETE,((ADI_DEV_1D_BUFFER*)pBuffer)->CallbackParameter);
        }
    }
    else
    {
        /* get the physical endpoint object */
        pEpInfo = &pDev->PhysicalEndpointObjects[ucEpNum].pLogicalEndpointObj->EPInfo;

        /* first queue the incoming transmit packet */
        QueueBuffer(pBuffer,pEpInfo,WAIT_XMT_QUEUE);

        TransmitDataBuffer(pDev,(s32_t)ucEpNum);
    }
    return(ADI_DEV_RESULT_SUCCESS);
}
/*
 *  Test mode, called for ADI_USB_CMD_ENTER_TEST_MODE i/o control
 */
__ADI_USB_HDRC_SECTION_CODE
static u32_t UsbOtgEnterTestMode(ADI_USBOTG_DEVICE *pDev,void *pArg)
{
   static char TestPacket[] =
   {
    /* 53 byte USB test packet (see USB 2.0 spec 7.1.20) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                   /* JKJKJKJK * 9         */
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,                         /* JJKKJJKK * 8         */
    0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,                         /* JJJJKKKK * 8         */
    0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* JJJJJJJKKKKKKK * 8   */
    0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD,                               /* JJJJJJJK * 8         */
    0xFC, 0x7E, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0x7E                    /* {JKKKKKKK * 10}, JK  */
    };
    int i = 0;
    u32_t Index,Result;
    u16_t wTest;
    s16_t  *pwData = (s16_t*)TestPacket;
    bool bExit = false;

    /* determine which test it is */
    Index = (u32)pArg >> 8;

    /* disable interrupts, the only way to exit a test mode is to cycle power */
    u32_t uInterruptStatus = cli();

    /* set the EP number */
    *pUSB_INDEX = 0;

    switch(Index)
    {
        /* TEST_J */
        case USB_TMODE_TEST_J:
            wTest = 0x2;
        break;

        /* TEST_K */
        case USB_TMODE_TEST_K:
             wTest = 0x4;
        break;

        /* TEST_SE0_NAK */
        case USB_TMODE_TEST_SE0_NAK:
             wTest = 0x1;
        break;

        /* TEST_PACKET */
        case USB_TMODE_TEST_PACKET:
        {
            wTest = 0x8;

            /* 53 bytes of test packet data */
            *pUSB_TX_MAX_PACKET = 0x35;
            *pUSB_TXCOUNT = 0x35;

            /* fill EP0 with test packet, 16-bits at a time */
            for( i = 0 ;i < 27; i++, pwData++)
               *pUSB_EP0_FIFO = *pwData;
            ssync();

            /* set TXPKTRDY */
            *pUSB_CSR0 |= 2;
            ssync();
        }
        break;

        /* TEST_FORCE_ENABLE */
        case USB_TMODE_TEST_FORCE_ENABLE:
             wTest = 0x10;
        break;

        default:
             Result = ADI_DEV_RESULT_FAILED;
        break;
    }

    /* write to the TESTMODE register to enter the test mode */
    *pUSB_TESTMODE = wTest;

    Result = ADI_DEV_RESULT_SUCCESS;

    /* we do not want to return, we don't want to leave the test mode */
    while ( bExit )
    {
       i++;
    }
    sti(uInterruptStatus);
    return(Result);
}


/*********************************************************************
*
*   Function:   pddControl
*
*   Description:    List of I/O control commands to the driver
*
*********************************************************************/
__ADI_USB_HDRC_SECTION_CODE
static u32 adi_pdd_Control(     /* Sets or senses a device specific parameter */
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle  */
    u32 Command,            /* command ID */
    void *pArg          /* pointer to argument */
)
{
u32 Result = ADI_DEV_RESULT_SUCCESS;
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;
u32  *pArgument;
int i = 0;
ENDPOINT_OBJECT *pEpO = NULL;
PUSB_EP_INFO  pEpInfo = &pDev->PhysicalEndpointObjects[EP0].pLogicalEndpointObj->EPInfo;

    switch (Command)
    {
        /*
         * dataflow method
         */
        case ADI_DEV_CMD_SET_DATAFLOW_METHOD:
            break;

        /*
         * dataflow
         */
        case ADI_DEV_CMD_SET_DATAFLOW:
            break;

        /*
         * query for processor DMA support
         */
        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

            /* no, we don't support it */
            (*(u32 *)pArg) = FALSE;
            break;

        /*
         * Returns the Device ID of the physical driver
         */
        case ADI_USB_CMD_GET_DEVICE_ID:
            (*(u32*)pArg) = pDev->DeviceID;
            break;

        /*
         * Enables the USB, in case of device mode, enumeration process starts and we supply the configuration
         * information to the host. In case of host we will start enumerating the device.
         */
        case ADI_USB_CMD_ENABLE_USB:
            Result = UsbOtgStart(pDev);
            break;

        /*
         * Disables USB
         */
        case ADI_USB_CMD_DISABLE_USB:
            Result = ADI_DEV_RESULT_FAILED;
            break;

        /*
         * Device Mode SET_STALL will send stall handshake to the host.
         * Device Mode CLEAR_STALL clears the stall condition.
         * These I/O controls are not applicable in case of Host mode.
         *
         * FIXME: We have to take EP number rather than EP Object
         */
        case ADI_USB_CMD_SET_STALL_EP:
        case ADI_USB_CMD_CLEAR_STALL_EP:
        {
            PENDPOINT_OBJECT pEpO = (PENDPOINT_OBJECT)pArg;
            USB_ASSERT(pEpO == NULL);
            s32_t dwEpNum = pEpO->ID;
            s32_t dwSavedEpNum;
            s16_t sControlReg;
			u32_t uInterruptStatus;

            USB_ASSERT( (dwEpNum < 0) || (dwEpNum > 7));

            if(pDev->Mode == MODE_DEVICE)
            {
                dwSavedEpNum = *pUSB_INDEX;
                
                uInterruptStatus = cli();

                /* select the endpoint */
                *pUSB_INDEX = dwEpNum;

                /* OUT EP in Device Mode - RXCSR*/
                if(((pEpO->pEndpointDesc->bEndpointAddress >> 7) & 0x1) == 0x0)
                {
                    sControlReg = *pUSB_RXCSR;
                    *pUSB_RXCSR = (Command == ADI_USB_CMD_SET_STALL_EP) ? (sControlReg | (1 << 5)) : (sControlReg & ~(1 << 5));
                }
                else /* IN EP Device Mode - TXCSR */
                {
                    sControlReg = *pUSB_TXCSR;
                    *pUSB_TXCSR = (Command == ADI_USB_CMD_SET_STALL_EP) ? (sControlReg | (1 << 4)) : (sControlReg & ~(1 << 4));
                }

                *pUSB_INDEX = dwSavedEpNum;
                
                sti(uInterruptStatus);
            }
        }
            break;

        /*
         * Sets the IVG levels of USB
         */
        case ADI_USB_CMD_SET_IVG:
        {
            /* In MOAB we expect the class driver or applicaiton to pass ADI_USB_HDRC_IVG_LEVELS structure */
            ADI_USB_HDRC_IVG_LEVELS *pUsbIvgLevels = (ADI_USB_HDRC_IVG_LEVELS*)pArg;

            USB_ASSERT(pUsbIvgLevels == NULL);
            USB_ASSERT( (pUsbIvgLevels->IvgUSBINT0 < ik_ivg7) || (pUsbIvgLevels->IvgUSBINT0 > ik_ivg15));
            USB_ASSERT( (pUsbIvgLevels->IvgUSBINT1 < ik_ivg7) || (pUsbIvgLevels->IvgUSBINT1 > ik_ivg15));
            USB_ASSERT( (pUsbIvgLevels->IvgUSBINT2 < ik_ivg7) || (pUsbIvgLevels->IvgUSBINT2 > ik_ivg15));
            USB_ASSERT( (pUsbIvgLevels->IvgUSBDMAINT < ik_ivg7) || (pUsbIvgLevels->IvgUSBDMAINT > ik_ivg15));

            pDev->IvgUSBINT0 = pUsbIvgLevels->IvgUSBINT0;
            pDev->IvgUSBINT1 = pUsbIvgLevels->IvgUSBINT1;
            pDev->IvgUSBINT2 = pUsbIvgLevels->IvgUSBINT2;
            pDev->IvgUSBDMAINT = pUsbIvgLevels->IvgUSBDMAINT;
        }
            break;

        /*
         * Returns the IVG levels of the USB
         */
        case ADI_USB_CMD_GET_IVG:
        {
            ADI_USB_HDRC_IVG_LEVELS *pUsbIvgLevels = (ADI_USB_HDRC_IVG_LEVELS*)pArg;
            USB_ASSERT(pUsbIvgLevels == NULL);
            pUsbIvgLevels->IvgUSBINT0 = pDev->IvgUSBINT0;
            pUsbIvgLevels->IvgUSBINT1 = pDev->IvgUSBINT1;
            pUsbIvgLevels->IvgUSBINT2 = pDev->IvgUSBINT2;
            pUsbIvgLevels->IvgUSBDMAINT = pDev->IvgUSBDMAINT;
        }
            break;

        /*
         * Sets the PF for VBUS
         */
        case ADI_USB_CMD_SET_PF:
        {
            pArgument = (u32*)pArg;
            pDev->VbusDriveFlag = (ADI_FLAG_ID)*pArgument;
        }
            break;

        /*
         * Returns the PF used for VBUS
         */
        case ADI_USB_CMD_GET_PF:
        {
            pArgument = (u32*)pArg;
            *pArgument = pDev->VbusDriveFlag;
        }
            break;

        /*
         * Used to reserve extra memory for the peripheral driver to use per buffer
         */
        case ADI_USB_CMD_GET_BUFFER_PREFIX:
        {
            pArgument = (u32*)pArg;
            *pArgument = pDev->BufferPrefix;
        }
            break;
        /*
         * Sets the buffer prefix for the USB buffers
         */
        case ADI_USB_CMD_SET_BUFFER_PREFIX:
        {
            pDev->BufferPrefix = (int)pArg;
        }
            break;

        /*
         * Enables Handshake signaling in the status of a transaction. Used by the core.
         */
        case ADI_USB_CMD_ENABLE_CNTRL_STATUS_HANDSHAKE:
        {
            if(pDev->Mode == MODE_OTG_HOST)
            {
                s32_t dwSavedEpNum = *pUSB_INDEX;

                *pUSB_INDEX = EP0;
                *pUSB_CSR0 = ((1 << 6) | (1 << 11)); 
                *pUSB_INDEX = dwSavedEpNum;
            }
        }
            break;

        /*
         * Sets the device address. In the device mode its the address given by the host. In the OTG host
         * mode core assigns address to the connected device
         */
        case ADI_USB_CMD_SET_DEV_ADDRESS:
        {
           /* Maximum value of function address is 7F */
            USB_ASSERT(pDev->DeviceAddress > 0x7F);
            /* Set address recovery interval, USB 2.0 specification 9.2.6.3 */
            if(pDev->Mode == MODE_DEVICE)
               WaitMilliSec(2);
            pDev->DeviceAddress = (int)((u32_t)pArg);
            *pUSB_FADDR = (u8_t)((u32_t)pArg & 0x7F);
            ssync();
        }
            break;

        /*
         * Updates the current bindings in the physical driver. In case of device mode core uses this call
         * when it recevies SET_CONFIGURATION or SET_INTERFACE requests. In case of OTG host this call is used
         * to set the class driver selected configuration.
         */
        case ADI_USB_CMD_UPDATE_ACTIVE_EP_LIST:
        {
            if(pDev->Mode == MODE_DEVICE)
            
                DevBindPhysicalEndpoints((ENDPOINT_OBJECT*)pArg);
                
            else
                HostBindPhysicalEndpoints((ENDPOINT_OBJECT*)pArg);
        }
            break;

        /*
         * Used by the core to request an IN token.
         */
        case ADI_USB_CMD_OTG_REQ_IN_TOKEN:
        EpZeroOTGRequestINTokens(pDev);
            break;

        /*
         * Used by the core to send zero length packet. Zero length packets are used by the in the
         * status phase to end a particular transaction.
         */
        case ADI_USB_CMD_OTG_SEND_ZERO_LEN_PKT:
        {
            s32_t dwSavedEpNum = *pUSB_INDEX;

            USB_ASSERT(pDev->Mode != MODE_OTG_HOST);

            *pUSB_INDEX = EP0;
            *pUSB_CSR0 = ((1 << 1) | (1 << 6) | (1 << 11));
            while(!(*pUSB_INTRTX & 0x1));
            *pUSB_INTRTX = 0x1;
            *pUSB_INDEX = dwSavedEpNum;
            ssync();
        }
            break;

        /*
         * Sets the device mode.
         */
        case ADI_USB_CMD_SET_DEV_MODE:
            pDev->Mode = (DEV_MODE)((u32_t)pArg);
            /* FIXME:
             * If user changes the mode during runtime or other end requests change via SRP
             * We have to change the operating mode
             */
            break;

        /*
         * Returns the currently operating device mode.
         */
        case ADI_USB_CMD_GET_DEV_MODE:
        {
            pArgument  = (u32*)pArg;
            *pArgument = pDev->Mode;
        }
            break;

        /*
         * If Data cache is enabled application or class driver has to issue cache on.
         */
        case ADI_USB_CMD_BUFFERS_IN_CACHE:
        {
            i = (int)(pArg);
            pDev->Cache = (i!=0);
        }
            break;

        /*
         * Returns the currently operating device mode.
         */
        case ADI_USB_CMD_GET_DEV_SPEED:
        {
            pArgument  = (u32*)pArg;
            *pArgument = pDev->Speed;
	    
        }
            break;

        /*
         * Forces to operate the device at a speed.
         */
        case ADI_USB_CMD_SET_DEV_SPEED:
        {
            ADI_USB_DEVICE_SPEED eDevSpeed = (ADI_USB_DEVICE_SPEED)pArg;
	        u32_t uInterruptStatus = cli();

            s16_t wPowerReg = *pUSB_POWER;

            *pUSB_POWER = (eDevSpeed == ADI_USB_DEVICE_SPEED_FULL) ? (wPowerReg & ~(1 <<5)) : (wPowerReg | (1 << 5)); 

            pDev->Speed = eDevSpeed;

            sti(uInterruptStatus);
        }
            break;

       /*
        * Enter a test mode.
        */
        case ADI_USB_CMD_ENTER_TEST_MODE:
             Result = UsbOtgEnterTestMode(pDev,pArg);
        break;

        case ADI_USB_CMD_PEEK_EP_FIFO:
        {
           s32_t *data = (s32_t*)pArg;
           s32_t dwSavedEpNum,rxcount=0;
           u32_t uInterruptStatus;

           /* Endpoint can not be less than 0 or greater than 7 on MOAB */
           USB_ASSERT( (*data < 0) || (*data > 7));

           uInterruptStatus = cli();

           dwSavedEpNum = *pUSB_INDEX;

           *pUSB_INDEX  = *data;

           rxcount = *pUSB_RXCOUNT;
           *data = rxcount;
           *pUSB_INDEX = dwSavedEpNum;
           sti(uInterruptStatus);

           Result = ADI_DEV_RESULT_SUCCESS;
        }
        break;

        case ADI_USB_CMD_STOP_EP_TOKENS:
        {
           s32_t *data = (s32_t*)pArg;
           s32_t dwSavedEpNum;
           u32_t uInterruptStatus;


           /* Endpoint can not be less than 0 or greater than 7 on MOAB */
           USB_ASSERT( (*data < 0) || (*data > 7));
           USB_ASSERT( pDev->Mode != MODE_OTG_HOST);

           uInterruptStatus = cli();

           dwSavedEpNum = *pUSB_INDEX;

           *pUSB_INDEX  = *data;

           /* if Req Pkt enabled */
           if(*pUSB_RXCSR & (1 <<5))
             *pUSB_RXCSR &= ~(1 << 5);

           *pUSB_INDEX = dwSavedEpNum;
           sti(uInterruptStatus);

           Result = ADI_DEV_RESULT_SUCCESS;
       }
       break;

       case ADI_USB_CMD_SETUP_RESPONSE:
        {
                  USB_ASSERT(pDev->Mode != MODE_DEVICE);

                  s32_t SetupResponse = (s32_t)pArg;

		  switch(SetupResponse)
		  {
			  /* ACK setup request packet and data packts */
			  case ADI_USB_SETUP_RESPONSE_ACK:
				  *pUSB_CSR0 |= (1 << 6);
			  break;

                          /* Send STALL as response to setup request */
			  case ADI_USB_SETUP_RESPONSE_STALL:
				  *pUSB_CSR0 |= (1 << 5);
			  break;

			  /*  ACK along with Data end */
			  case ADI_USB_SETUP_RESPONSE_ACK_DATA_END:
			  {
                  /* if send-stall is set then clear it */
                  if(*pUSB_CSR0 & (1 << 2))
                  {
                      *pUSB_CSR0 &= ~(1 << 2);
                       return(ADI_DEV_RESULT_SUCCESS);
                  }                 
                     /* if setup transaction is ended then ack it and return */
                  if(*pUSB_CSR0 &(1 << 4))
                  {
				      *pUSB_CSR0 |= (1 << 7);
				      /* Set ServicedRxPktRdy bit to ack setup pkt */
				      *pUSB_CSR0 |= (1 << 6);
				      
                      *pUSB_INTRTX = 0x1; 
                      return(ADI_DEV_RESULT_SUCCESS);                     
                  }
                  
			      if(pEpInfo->EpDir == USB_DIR_HOST_TO_DEVICE)
				      *pUSB_CSR0 |= ((1 << 6) | (1 << 3));
			      else
				      *pUSB_CSR0 |= (1 << 3);
				      				  
			   
				  
			  }
			  break;
		  }

		  ssync();
                  Result = ADI_DEV_RESULT_SUCCESS;
	     }
	break;

        case ADI_USB_CMD_HIBERNATE:
             *pUSB_APHY_CNTRL2 |= 0x1;
        break;

        case ADI_USB_CMD_RESTORE:
             *pUSB_APHY_CNTRL2  |= 0x2;
	break;

	/* sets the DMA mode for RX Channel - only applicable for bulk endpoints */
    case ADI_USB_CMD_SET_DMA_MODE:
	{
            ADI_USB_HDRC_DMA_CONFIG *pDmaConfig = (ADI_USB_HDRC_DMA_CONFIG*)pArg;

	    USB_ASSERT(pDmaConfig == NULL);

            Result = ADI_DEV_RESULT_SUCCESS;
	 
	    switch(pDmaConfig->DmaChannel)
	    {
            case  ADI_USB_HDRC_RX_CHANNEL:
                      pDev->RxDmaMode = pDmaConfig->DmaMode;
	        break;

            case  ADI_USB_HDRC_TX_CHANNEL:
                      //pDev->TxDmaMode = pDmaConfig->DmaMode;
                      Result = ADI_DEV_RESULT_NOT_SUPPORTED;
	        break;

            default:
                       Result = ADI_DEV_RESULT_FAILED;
	        break;
	    }
	}
        break;	

	case ADI_USB_CMD_GET_DMA_MODE:
	{
        ADI_USB_HDRC_DMA_CONFIG *pDmaConfig = (ADI_USB_HDRC_DMA_CONFIG*)pArg;

        USB_ASSERT(pDmaConfig == NULL);

        Result = ADI_DEV_RESULT_SUCCESS;

        switch(pDmaConfig->DmaChannel)
        {
            case  ADI_USB_HDRC_RX_CHANNEL:
                  pDmaConfig->DmaMode=pDev->RxDmaMode;
            break;

            case  ADI_USB_HDRC_TX_CHANNEL:
                   pDmaConfig->DmaMode = pDev->TxDmaMode;
            break;

            default:
                   Result = ADI_DEV_RESULT_FAILED;
	        break;
         }
     }
	 break;

     default:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
      break;
    }

    return(Result);
}

/*********************************************************************
*
*   Function:       pddClose
*
*   Description:    Closes the driver and releases any memory
*
*********************************************************************/

__ADI_USB_HDRC_SECTION_CODE
static u32 adi_pdd_Close(       /* Closes a device */
    ADI_DEV_PDD_HANDLE PDDHandle    /* PDD handle */
)
{
u32_t Result;
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;
 
    /* Disable all pending Interrupts */
    *pUSB_INTRUSB = (*pUSB_INTRUSB & 0xff);
    *pUSB_INTRUSBE = 0;

    /* Disable TX interrupts */
    *pUSB_INTRTX = (*pUSB_INTRTX & 0xff);
    *pUSB_INTRTXE = 0;

    /* Disable RX interrupts */
    *pUSB_INTRRX = (*pUSB_INTRRX & 0xff);
    *pUSB_INTRRXE = 0;
    
    if(pDev->Mode == MODE_DEVICE)
    {
        /* Reset Soft connected bit */    	
        *pUSB_POWER &= ~(1 << 6);     
        *pUSB_POWER = 0x20;    
        /* reset device address */
        *pUSB_FADDR = 0x0;      
        *pUSB_GLOBINTR = 0x0111;
    }


    /* Diasble powet to USB */
    *pUSB_GLOBAL_CTL = 0;
    ssync();

    /* Clear Core data structures */
   adi_usb_ClearCoreData();
  

    /* Unhook all interrupt handlers */
    Result = adi_int_CECUnhook(pDev->IvgUSBINT2,UsbOtgInterruptHandler,pDev);

    if(Result == 0) 
    {
       Result = adi_int_CECUnhook(pDev->IvgUSBINT1,UsbTxInterruptHandler,pDev);

       if(Result == 0)
       {
          Result = adi_int_CECUnhook(pDev->IvgUSBINT0,UsbRxInterruptHandler,pDev);

          if(Result == 0)
          {
             Result = adi_int_CECUnhook(pDev->IvgUSBDMAINT,UsbDmaInterruptHandler,pDev);
          }
       }
    }
    
    /* Clear OTG data */
    memset(&UsbOtgDevice, 0, sizeof(ADI_USBOTG_DEVICE));
      

    if(Result != 0)
       return ADI_DEV_RESULT_FAILED;

    return(ADI_DEV_RESULT_SUCCESS);
}

/**************************************************************************
 *
 * Hi-speed USB OTG dual-role controller driver entry point
 *
 **************************************************************************/

__ADI_USB_HDRC_SECTION_DATA
ADI_DEV_PDD_ENTRY_POINT ADI_USBDRC_Entrypoint =
{
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

__ADI_USB_HDRC_SECTION_DATA
ADI_DEV_PDD_ENTRY_POINT ADI_USB_HDRC_Entrypoint =
{
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

/**
 *  Tx Error Handler
 *
 *  @param pDev Pointer the device object
 *  @param dwEpNum Endpoint number 
 */
__ADI_USB_HDRC_SECTION_CODE
static int TxErrorHandler(ADI_USBOTG_DEVICE *pDev,s32_t dwEpNum)
{
ENDPOINT_OBJECT *pEpO=NULL;
USB_EP_INFO *pEpInfo;
s32_t Ret=0;

pEpO = pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj;

	switch(pDev->Mode)
	{
		case MODE_OTG_HOST:
					
	    /* Check for TX Stall and let Class driver know
             * if we get one 
             */
            if(*pUSB_TXCSR & (1 << 5))
            {
                ADI_HDRC_STAT_INC(dwTxStalls);
                pEpO->EPInfo.pTransferBuffer = NULL; /* TODO: return the buffer */
                *pUSB_INTRTXE  &= ~(1 << dwEpNum);
                
                /* clear sent stall */
                *pUSB_TXCSR &= ~(1 << 5);
                (pEpO->EPInfo.EpCallback)(pDev->DeviceHandle,ADI_USB_EVENT_TX_STALL,(void*)dwEpNum);
                return(1);
            }

            if(*pUSB_TXCSR & (1 << 7))
            {                
                ADI_HDRC_STAT_INC(dwTxNAKTimeoutErrors);
                pEpO->EPInfo.pTransferBuffer = NULL;
                *pUSB_INTRTXE  &= ~(1 << dwEpNum); 
                /* incomplete tx */              
                *pUSB_TXCSR &= ~(1 << 7);                                
                (pEpO->EPInfo.EpCallback)(pDev->DeviceHandle,ADI_USB_EVENT_TX_NAK_TIMEOUT,(void*)dwEpNum);
                return(1);
            }

            /* Check for Error interrupt which means we tried to send a packet 
             * three times and the device did not take it 
             */
            if(*pUSB_TXCSR & (1 << 2))
            {               
                ADI_HDRC_STAT_INC(dwTxErrors);
                pEpO->EPInfo.pTransferBuffer = NULL;
                *pUSB_INTRTXE  &= ~(1 << dwEpNum);
                /* Tx under run */
                *pUSB_TXCSR &= ~(1 << 2);
                (pEpO->EPInfo.EpCallback)(pDev->DeviceHandle,ADI_USB_EVENT_TX_ERROR,(void*)dwEpNum);
                return(1);
            }
		
		break;
		
		case MODE_DEVICE:
		/* TODO: */
		break;
		
		default:
		break;		
	}
	return(0);
}
/**
 *  Handles Rx Errors
 *
 *  @param pDev Pointer the device object
 *  @param dwEpNum Endpoint number 
 */
__ADI_USB_HDRC_SECTION_CODE
static int RxErrorHandler(ADI_USBOTG_DEVICE *pDev,s32_t dwEpNum)
{
ENDPOINT_OBJECT *pEpO=NULL;
USB_EP_INFO *pEpInfo;
s32_t Ret=0;

pEpO = pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj;

	switch(pDev->Mode)
	{
		/* 
		 * OTG host mode Rx errors
		 */
		case MODE_OTG_HOST:
		{
			/* Notify the when a Rx STALL happens */
 			if(*pUSB_RXCSR & (1 << 6))
			{					
				pEpO->EPInfo.pTransferBuffer = NULL; /* TODO: we should return the buffer */
				*pUSB_INTRRXE  &= ~(1 << dwEpNum);				
				ADI_HDRC_STAT_INC(dwRxStalls);
				/* clear stall */
				*pUSB_RXCSR &= ~(1 << 6);
				*pUSB_INTRRX = (1 << dwEpNum);
				(pEpO->EPInfo.EpCallback)(pDev->DeviceHandle,ADI_USB_EVENT_RX_STALL,(void*)dwEpNum);
				return(1);			
			}

			/* Check for ERROR interrupt which means we tried to send a packet three times and
			 *  the device did not take it 
			 */
			if(*pUSB_RXCSR & (1 << 2))
			{					
				pEpO->EPInfo.pTransferBuffer = NULL;/* TODO: we should return the buffer */
				*pUSB_INTRRXE  &= ~(1 << dwEpNum);
				*pUSB_INTRRX = (1 << dwEpNum);
				
				ADI_HDRC_STAT_INC(dwRxErrors);
				/* only valid in case of Bulk or Interrupt */
				if(!IS_ISONCHRONOUS(pEpO->pEndpointDesc->bAttributes))
					*pUSB_RXCSR &= ~(1 << 2);
				(pEpO->EPInfo.EpCallback)(pDev->DeviceHandle,ADI_USB_EVENT_RX_ERROR,(void*)dwEpNum);
				return(1);		    
			}

			/* Check for NAK timeout interrupt which means we  exceeded our NAK timeout 
			 * value limit 
			 */
			if(*pUSB_RXCSR & (1 << 3))
			{
				pEpO = pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj;
				pEpO->EPInfo.pTransferBuffer = NULL;/* TODO: we should return the buffer */
				*pUSB_INTRRXE  &= ~(1 << dwEpNum);
				ADI_HDRC_STAT_INC(dwRxNAKTimeoutErrors);
				*pUSB_INTRRX = (1 << dwEpNum);
				
				/* clear Data error / timeout error */
				*pUSB_RXCSR &= ~(1 << 3);
				(pEpO->EPInfo.EpCallback)(pDev->DeviceHandle,ADI_USB_EVENT_RX_NAK_TIMEOUT,(void*)dwEpNum);
				return(1);
			}
			
		}
		break;

		/*
		 * In device mode
		 */
		case MODE_DEVICE:
	       /* TODO:  */
		break;
	}
	return(0);
}

/**
 *  DevRxDMAInterruptHandler gets called when DMA completion happens
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER_RESULT DevRxDMAInterruptHandler(void)
{
    return(ADI_INT_RESULT_PROCESSED);
}

/**
 * Peripheral Mode Receive Interrupt Handler
 *
 * This routine only gets called for the Data endpoints
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER_RESULT DevRxInterruptHandler(void)
{
char *pData;
volatile short intrRx = *pUSB_INTRRX;
int dwEpNum,wRxCount;
ENDPOINT_OBJECT *pEpO=NULL;
USB_EP_INFO *pEpInfo;
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;
ADI_DEV_1D_BUFFER    *pBuffer;
s32_t *pDmaMode=0,dwTotalBytes;
s32_t  DmaStartAddr, DmaEndAddress,dwBytesToRcv,dwSavedEpNum;
s32_t  dwRecvNoBufferFlag=0;
u32_t  uInterruptStatus;

    /* if no rx endpoint interrupt to service return - 0th bit is reserved */
    if(!(intrRx & 0xfe)) 
    {
       return ADI_INT_RESULT_NOT_PROCESSED;   
    }

    uInterruptStatus = cli();
    dwSavedEpNum = *pUSB_INDEX;
   /*
    * initiate dma for grabbing data from the currnetly triggered rx endpoints
    * TODO: Optimize this loop based on actually trigged EP interrupts in USB_INTRRX
    */
    for(dwEpNum=1; dwEpNum< NUM_PHYSICAL_ENDPOINTS; dwEpNum++)
    {
        if((intrRx >> dwEpNum ) & 0x1)
        {
           /* select the endpoint */
           *pUSB_INDEX  = dwEpNum;

   	   /* if there is an error we notify the application and continue */
           if(RxErrorHandler(pDev,dwEpNum))
           {
               /* Error interrupt handled continue, ack'd inside handler */
                continue;
           }
           

           if(pDev->RxDmaMode == DMA_MODE_0)
           {
               pEpO = pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj;
               pEpInfo = &pEpO->EPInfo;
               pBuffer = (ADI_DEV_1D_BUFFER*)pEpO->EPInfo.pTransferBuffer;

               pData =  (char*)pBuffer->Data + pEpInfo->EpBytesProcessed;

               wRxCount = *pUSB_RXCOUNT; 

                //USB_ASSERT(wRxCount == 0);
                if(wRxCount == 0) 
                {
                    ADI_HDRC_STAT_INC(dwRxZeroLengthPkts);
                   *pUSB_INTRRX = (1 << dwEpNum);
	           return ADI_INT_RESULT_PROCESSED;
                }
      
                *pUSB_INTRRX = (1 << dwEpNum);

	        pDev->PhysicalEndpointObjects[dwEpNum].eIsDmaPending = 1;
                set_dma_addr(dwEpNum,(u16_t)ADDR_LOW(pData),(u16_t)ADDR_HIGH(pData));

                /* setup no# bytes to read */
                set_dma_count(dwEpNum,(u16_t)ADDR_LOW(wRxCount),(u16_t)ADDR_HIGH(wRxCount));

                /* enable dma and generate interrupt */
                set_dma_ctrl(dwEpNum,(u16_t)( USB_DMA_CTRL_DMA_INT_ENA_B | USB_DMA_CTRL_DMA_ENA_B | ((dwEpNum&0xf) << 4)));


                pEpO->EPInfo.EpBytesProcessed += wRxCount;
	    }
	    else /* DMA_MODE_0 */
	    {
	       /* if interrupt is not enabled we do not want to continue */
               if(!(*pUSB_INTRRXE & (1 << dwEpNum)))
               {
                   /* Acknowledge Rx Interrupt */
         	   if(*pUSB_RXCOUNT > 0)
         	   {
                       ADI_HDRC_STAT_INC(dwUnexpectedShortPkts);
         	   }
         	   else
                   {
                       ADI_HDRC_STAT_INC(dwRxZeroLengthPkts);
                       *pUSB_INTRRX = (1 << dwEpNum);
                   }
		   continue;
                }
			
                pDmaMode = &pDev->PhysicalEndpointObjects[dwEpNum].DmaMode;

           /* Get the associated logical endpoint object */
           pEpO = pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj;
           pEpInfo = &pEpO->EPInfo;

           switch(*pDmaMode)
           {
                case DMA_MODE_0:
                {                  
                       wRxCount = *pUSB_RXCOUNT;

                       if(wRxCount == 0)
                       {
                          ADI_HDRC_STAT_INC(dwRxZeroLengthPkts);
                          continue;
                       }
                          
                       /* disable interrupt */
                       *pUSB_INTRRXE  &= ~(1 << dwEpNum);

                       pEpInfo->EpBytesProcessed = 0;
                       USB_ASSERT(pEpO == NULL);

                        /* make sure its OUT endpoint */
                        if(pDev->Mode == MODE_DEVICE)
                            USB_ASSERT(((pEpO->pEndpointDesc->bEndpointAddress >> 7) & 0x1) != 0);
                        else /* Host mode IN endpoint */
                            USB_ASSERT(((pEpO->pEndpointDesc->bEndpointAddress >> 7) & 0x1) == 0);

                        /* get the scheduled buffer */
                        pBuffer = (ADI_DEV_1D_BUFFER*)pEpO->EPInfo.pTransferBuffer;

                       /* We have no scheduled read but we got an interrupt means device sent more data then we
                        * asked for. This case may happen with mass storage like class driver(s) where status packet
                        * follows the data. So here we give application a chance to attach a buffer to the endpoint
                        */
                        if(!pBuffer)
                        {
                            *pUSB_INDEX = dwSavedEpNum;
                            (pEpO->EPInfo.EpCallback)(pDev->DeviceHandle,ADI_USB_EVENT_PKT_RCVD_NO_BUFFER,0);

                            /* check if callback has attached any buffer */
                            pBuffer = (ADI_DEV_1D_BUFFER*)pEpO->EPInfo.pTransferBuffer;

                            /* if we still could not get buffer from class driver or application we continue. 
                             * arrived data is left in the FIFO and next read from application will pick
                             * it up.
                             */
                             if(pBuffer  == NULL) 
                             { 
                                 ADI_HDRC_STAT_INC(dwRxNoEpBuffer);
                                 *pUSB_INTRRX  = (1 << dwEpNum);
                                 continue;
                             }
                        }

                       /* schedule DMA mode 0 transfer to unload the packet */
                        ScheduleRxDMA0(pDev,dwEpNum);

                        /* acknowledge the receive interrupt */
                        *pUSB_INTRRX  = (1 << dwEpNum);
              }
              break;

              /* We enter this block if we are coming from DMA mode 1 */
              case DMA_MODE_1:
                  {
                       /* in case of otg host mode if interrupt is not enabled means there is no short
                        * packet but the device has sent more data than we asked for. Typically it happens
                        * with mass storage class driver sending status, so we return no# bytes the one we
                        * asked for 
                        */
                        if((pDev->Mode == MODE_OTG_HOST) && (!(*pUSB_INTRRXE & (1 << dwEpNum) )))
                        {
                             ADI_HDRC_STAT_INC(dwUnexpectedShortPkts);
                             continue;
                        }

                        /* Switch back to DMA mode 0 */
                        *pDmaMode = 0x0;
                        
                        /* Acknowledge the receive interrupt */
                        *pUSB_INTRRX  = (1 << dwEpNum);
                        				
                        /* Disable Recive interrupt for that endpoint */
                        *pUSB_INTRRXE  &= ~(1 << dwEpNum);

                        /* enable dma and generate interrupt */
                        set_dma_ctrl(dwEpNum,(u16_t)( USB_DMA_CTRL_DMA_INT_ENA_B | USB_DMA_CTRL_DMA_ENA_B | ((dwEpNum&0xf) << 4)));                                                
                  }
                  break;

                  default:
                  break;
           }
	 } /* DMA_MODE_0 */
       } /* is there rx int */
    } /* for all eps */

    sti(uInterruptStatus);
    *pUSB_INDEX = dwSavedEpNum;

    return(ADI_INT_RESULT_PROCESSED);
}


/**
 * Peripheral Mode Transmit Interrupt Handler
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER_RESULT DevTxInterruptHandler()
{

ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;
unsigned short v =   *pUSB_INTRTX;

   if(pDev->DeviceAddress != 0)
   {
      *pUSB_FADDR = (short)(pDev->DeviceAddress & 0xffff);
      ssync();
   }
   if(!(*pUSB_CSR0 & 0x1))
     *pUSB_INTRTX = v;
   return(ADI_INT_RESULT_PROCESSED);
}
/**
 * Host Rx interrupt handler
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER_RESULT HostRxInterruptHandler()
{
   DevRxInterruptHandler();
   return(ADI_INT_RESULT_PROCESSED);
}

/**
 * Host Tx interrupt handler
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER_RESULT HostTxInterruptHandler()
{
unsigned short v =   *pUSB_INTRTX;
ENDPOINT_OBJECT *pEpO=NULL;
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;
int dwEpNum;

    for(dwEpNum=1; dwEpNum< NUM_PHYSICAL_ENDPOINTS; dwEpNum++)
    {
        if((v >> dwEpNum ) & 0x1)
        {        	
            if(TxErrorHandler(pDev,dwEpNum))
              continue;
        }
    }

    *pUSB_INTRTX = v;
   return(ADI_INT_RESULT_PROCESSED);
}

/**
 * Disconnect is detected we reset to default.
 */
__ADI_USB_HDRC_SECTION_CODE
static u32_t HostDisconnect(void)
{
   ResetUsbOtgDevice(&UsbOtgDevice);

   /* This should not be needed but adding to provide same as rev 1.19 */
#if defined (__ADSP_MOCKINGBIRD__)
   adi_flag_Set(UsbOtgDevice.VbusDriveFlag);
#else
   adi_flag_Clear(UsbOtgDevice.VbusDriveFlag);
#endif

   /* Reset the device address */
   *pUSB_FADDR = 0x0;

   /* Reset device index */
   *pUSB_INDEX = 0x0;

   /* Flush EP0 FIFO */
   *pUSB_CSR0 = (1 << 8);

   /* Reset EP0 Control and Status register */
   *pUSB_CSR0 = 0x0;

   /* Reset frame counter register */
   *pUSB_FRAME = 0x0;

   /* Reset USB interrupt enable register */
   *pUSB_INTRUSBE = 0x34;
    ssync();
    csync();

    /* Maximim packet size for EP0 Rx Endpoint */
    *pUSB_EP_NI0_RXMAXP = 64;

    /* Maximim packet size for EP0 Tx Endpoint */
    *pUSB_EP_NI0_TXMAXP = 64;

    /* set USB clock div in case we can get the current system clock value */
    SetClockDiv();

    /* Enable Reset and other connection interrupts */
    *pUSB_INTRUSBE    = (USB_INTRUSB_RESET_OR_BABBLE_B | USB_INTRUSB_CONN_B | USB_INTRUSB_DISCONN_B);

    /*
     * Enable USB and the data endpoint logic by setting appropriate bits in Global Control register
     * With the execution of below statement we are ready to receive and transmit setup packets on EP0
     */
     *pUSB_GLOBAL_CTL  = 0x0;

     /* Reset global control register to default */
    *pUSB_GLOBAL_CTL = 0x7fff;

    *pUSB_OTG_DEV_CTL = 0x80;

   return(0);
}


/**
 * UsbEventHandler
 *
 * Handles VBUS and USB interrupts. This is a Common handler for both host and the device mode.
 * USB_INTRUSB and USB_INTV
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER_RESULT UsbInterruptHandler()
{
    u16_t ucIrqStatus,ucDevCtrl;
    ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;

    /* Read USB interrupt status */
    ucIrqStatus = *pUSB_INTRUSB;

    /* if there is no interrupt to service return */
    if(!ucIrqStatus)
    {
       return ADI_INT_RESULT_NOT_PROCESSED;
    }

    /* If we get an interrupt that is not handled in DEVICE mode ignore it */
    if((pDev->Mode == MODE_DEVICE) && (!(ucIrqStatus & *pUSB_INTRUSBE)))
    {
        *pUSB_INTRUSB = ucIrqStatus;
        return ADI_INT_RESULT_PROCESSED;
    }

    /* IF (Session Request Interrupt) */
    if ( (ucIrqStatus&USB_INTRUSB_SESSION_REQ_B) && pDev->Mode==MODE_OTG_HOST )
    {
       /* Acknowledge pending SRP interrupt */
        *pUSB_INTRUSB = ucIrqStatus;
        /* Set session bit to configure controller as host */
        SetSessionBit();
        return ADI_INT_RESULT_PROCESSED;
    }

    /* IF (VBUS error Interrupt) */
    if ( (ucIrqStatus&USB_INTRUSB_VBUSERROR_B) && pDev->Mode==MODE_OTG_HOST )
    {
       /* Acknowledge pending VBUS error interrupt */
        *pUSB_INTRUSB = ucIrqStatus;
        DriveVbusOff(pDev);
        DriveVbusOn(pDev);
        SetSessionBit();
        return ADI_INT_RESULT_PROCESSED;
    }

    /* IF (SUSPEND interrupt)
     * This interrupted is generated when there is no micro A or micro B plug
     * detected and when it is inserted but not connected to a device.
     * In this event we set the SESSION bit again to keep the polling mechanism
     * alive.
    */
    if ( (ucIrqStatus&USB_INTRUSB_SUSPEND_B) && pDev->Mode==MODE_NONE )
    {
        /* ensure VBUS is off */
        u16 RegVal = *pUSB_OTG_DEV_CTL;
        DriveVbusOff(pDev);
        /* 100ms is a sensible time to wait before setting the SESSION bit */
        WaitMilliSec(100);
        /* Set SESSION bit to await plug insertion event */
        SetSessionBit();

        /* Acknowledge all interrupts */
        *pUSB_INTRUSB = ucIrqStatus;
        /* Nothing further to do */
        return ADI_INT_RESULT_PROCESSED;
    }

    /* Device RESET (peripheral mode) or a device is connected */

    if( !(ucIrqStatus & USB_INTRUSB_DISCONN_B) && ((ucIrqStatus & USB_INTRUSB_RESET_OR_BABBLE_B) || (ucIrqStatus & USB_INTRUSB_CONN_B) || ((ucIrqStatus&USB_INTRUSB_SOF_B) && (pDev->Mode == MODE_OTG_HOST))) )
    {
       /* Read the Dev Control register and change the device mode */
       ucDevCtrl = (unsigned char)(*pUSB_OTG_DEV_CTL & 0x0f);
       pDev->Mode = (ucDevCtrl & USB_OTG_DEV_CTL_HOST_MODE_B) ? MODE_OTG_HOST : MODE_DEVICE;

       /* Inform core layer about our operating mode */
       if(pDev->EnumerationState == DEV_ENUM_RESET)
       {
          adi_usb_SetDeviceMode(pDev->Mode);

          /* Reset all physical endpoints */
          ResetPhysicalEndPoints(pDev);
       }
       /* Upon the connection interrupt, enable start of frame interrupt and
        * reset the connected device. The controller will start generating frames
        * and the driver will take an interrupt on the first one, which it takes as its
        * signal to begin enumeration.
       */
       if(ucIrqStatus & USB_INTRUSB_CONN_B)
       {
            /* clear interrupts */
            *pUSB_INTRUSB = ucIrqStatus;

            /* set state machine to 'started' state */
            pDev->State = DEV_STATE_STARTED;

            /* perform device reset */
            HostModeReset(pDev);


#if defined(__ADSP_KOOKABURRA__)
            /* SOF interrupt will be next thing processed, so we will simply
             * Poll for the interrupt before moving on
            */
            void *pExitCriticalArg = adi_int_EnterCriticalRegion(pDev->pCriticalRegionArg);
            while ( !(*pUSB_INTRUSB&USB_INTRUSB_SOF_B) );
            adi_int_ExitCriticalRegion(pExitCriticalArg);
#endif
      }

    }

    if (pDev->Mode!=MODE_NONE)
    {
        /* Configure device speed */
        if(*pUSB_OTG_DEV_CTL & (USB_OTG_DEV_CTL_LS_DEVICE_B))
        {
           pDev->Speed = ADI_USB_DEVICE_SPEED_LOW;
        }
        else /* full or high speed */
        {
           pDev->Speed = (*pUSB_POWER & (USB_POWER_HS_MODE_B)) ? ADI_USB_DEVICE_SPEED_HIGH : ADI_USB_DEVICE_SPEED_FULL;
        }
    }
    switch(pDev->Mode)
    {
        /* In device mode or peripheral mode host sends series of setup packets as a part of device
         * enumeration on EP0. We will enable EP0 interrupts at this point
         */
        case MODE_DEVICE:
        {

             ADI_DCB_CALLBACK_FN epZeroCallback;

             epZeroCallback =  pDev->PhysicalEndpointObjects[EP0].pLogicalEndpointObj->EPInfo.EpCallback;

             USB_ASSERT(epZeroCallback == NULL);

             pDev->RxInterruptHandler = DevRxInterruptHandler;
             pDev->TxInterruptHandler = DevTxInterruptHandler;


             /* Check if there is a disconnect interrupt, device unplugged */
             if(ucIrqStatus & USB_INTRUSB_DISCONN_B)
             {
                 epZeroCallback(pDev->DeviceHandle,ADI_USB_EVENT_DISCONNECT,0);
                 DeviceModeResetToDefault(pDev);                 
             }

             if((ucIrqStatus & USB_INTRUSB_SUSPEND_B) && (pDev->State == DEV_STATE_CONFIGURED))
                  epZeroCallback(pDev->DeviceHandle,ADI_USB_EVENT_SUSPEND,0);
             else if(ucIrqStatus & USB_INTRUSB_RESUME_B)
                 epZeroCallback(pDev->DeviceHandle,ADI_USB_EVENT_RESUME,0);

             /* Device Reset Detected */
             if(ucIrqStatus & USB_INTRUSB_RESET_OR_BABBLE_B)
             {
                 epZeroCallback(pDev->DeviceHandle,ADI_USB_EVENT_ROOT_PORT_RESET,0);
                 DeviceModeResetToDefault(pDev);
             }
        }
        break;

        case MODE_OTG_HOST:
        {
            ADI_DCB_CALLBACK_FN epZeroCallback =  pDev->PhysicalEndpointObjects[EP0].pLogicalEndpointObj->EPInfo.EpCallback;
            USB_ASSERT(epZeroCallback == NULL);

            /* Check if there is a disconnect interrupt, device unplugged */
            if(ucIrqStatus & USB_INTRUSB_DISCONN_B)
            {
                epZeroCallback(pDev->DeviceHandle,ADI_USB_EVENT_DISCONNECT,0);

                /* clear interrupts */
                *pUSB_INTRUSB = ucIrqStatus;

                /* Reset OTG device */
                pDev->EnumerationState = DEV_ENUM_RESET;
                pDev->State = DEV_STATE_OPENED;
                *pUSB_FADDR = 0;

                /* Now we revert to the device detection phase
                */

                /* Enable VBus Error interrupt can be taken*/
                *pUSB_INTRUSBE |= USB_INTRUSB_VBUSERROR_B;

                /* We will not try to enumerate as device is disconnected */
                break;
            }

            /* Reset state machine to 'started' */
            pDev->State = DEV_STATE_STARTED;

            /* Assign RX/TX interrupt handlers */
            pDev->RxInterruptHandler = HostRxInterruptHandler;
            pDev->TxInterruptHandler = HostTxInterruptHandler;
            
#if 0
            /* Configure device speed */
            if(*pUSB_OTG_DEV_CTL & (USB_OTG_DEV_CTL_LS_DEVICE_B))
            {
               pDev->Speed = ADI_USB_DEVICE_SPEED_LOW;
            }
            else /* full or high speed */
            {
               pDev->Speed = (*pUSB_POWER & (USB_POWER_HS_MODE_B)) ? ADI_USB_DEVICE_SPEED_HIGH : ADI_USB_DEVICE_SPEED_FULL;
            }
#endif


            /* Device is not enumerated then let core start enumerating the device */
            if(pDev->EnumerationState == DEV_ENUM_RESET)
            {
                pDev->EnumerationState = DEV_ENUM_PENDING;
                /* Invoke the EP zero callback handler */
                epZeroCallback(pDev->DeviceHandle,ADI_USB_EVENT_START_DEV_ENUM,0);
            }
            }
            break;

        default:
        break;
    }

    /* Acknowledge all interrupts */
    *pUSB_INTRUSB = ucIrqStatus;
    /* Wait for some time for the reset to affect */
    return ADI_INT_RESULT_PROCESSED;
}

/**
 * Returns a receive buffer from the associated logical endpoint
 * TODO: Error checking code for invalid Endpoints.
 *
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_DEV_BUFFER* GetRcvBuffer(ADI_USBOTG_DEVICE *pDev,s32_t PhyEpNum)
{
ADI_DEV_1D_BUFFER *pBuffer;
unsigned int uInterruptStatus = cli();

    pBuffer  = (ADI_DEV_1D_BUFFER*)pDev->PhysicalEndpointObjects[PhyEpNum].pLogicalEndpointObj->EPInfo.pFreeRcvList;

    if( (PhyEpNum == EP0) && (pDev->Cache == true) && (pBuffer != NULL))
    {
         char *pData = pBuffer->Data;
         s32_t dwNumBytes = pBuffer->ElementCount * pBuffer->ElementWidth;

         FlushInvArea((void*)pData,(void*)(pData + dwNumBytes));
    }

    if(pBuffer != NULL)
    {
      pDev->PhysicalEndpointObjects[PhyEpNum].pLogicalEndpointObj->EPInfo.pFreeRcvList = (ADI_DEV_BUFFER*)pBuffer->pNext;
      pBuffer->pNext = NULL;
    }


    sti(uInterruptStatus);
    return((ADI_DEV_BUFFER*)pBuffer);
}

/**
 *
 * Returns a transmit buffer from the associated logical endpoint
 * TODO: Error checking code for invalid Endpoints.
 *
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_DEV_BUFFER * GetXmtBuffer(ADI_USBOTG_DEVICE *pDev,int PhyEpNum)
{
    ADI_DEV_1D_BUFFER *pBuffer;
    unsigned int uInterruptStatus = cli();

    pBuffer  = (ADI_DEV_1D_BUFFER*)pDev->PhysicalEndpointObjects[PhyEpNum].pLogicalEndpointObj->EPInfo.pQueuedXmtList;

    if(pBuffer != NULL)
    {
      pDev->PhysicalEndpointObjects[PhyEpNum].pLogicalEndpointObj->EPInfo.pQueuedXmtList = (ADI_DEV_BUFFER*)pBuffer->pNext;
      pBuffer->pNext = NULL;
    }
    sti(uInterruptStatus);
    return((ADI_DEV_BUFFER*)pBuffer);
}

/**
 * ProcessEpZeroData
 * processes the EpZero data, primiarly it will be the setup packet.
 */
__ADI_USB_HDRC_SECTION_CODE
static int ProcessEpZeroData(ADI_USBOTG_DEVICE *pDev)
{
ADI_DEV_1D_BUFFER *pBuf;
u16_t *pData;
s32_t wRxCount=0,i;
ADI_DCB_CALLBACK_FN epZeroCallback =  pDev->PhysicalEndpointObjects[EP0].pLogicalEndpointObj->EPInfo.EpCallback;

    USB_ASSERT(epZeroCallback == NULL);

    pBuf = (ADI_DEV_1D_BUFFER*) GetRcvBuffer(pDev,EP0);

    /* we were able to get hold of a free buffer fill
     * the setup frame
     */
    if(pBuf != NULL)
    {
       pData = (unsigned short*)pBuf->Data;

       /* set index register to EP0 */
       *pUSB_INDEX = EP0;

       /* no# of bytes received */
       wRxCount  = *pUSB_COUNT0;

#ifdef USE_DMA
       /* setup the data buffer */
       set_dma_addr(EP0,(u16_t)ADDR_LOW(pData),(u16_t)ADDR_HIGH(pData));

       /* setup no# bytes to read */
       set_dma_count(EP0,(u16_t)ADDR_LOW(wRxCount),(u16_t)ADDR_HIGH(wRxCount));

       /* enable dma and generate interrupt */
       set_dma_ctrl(EP0,(u16_t)( USB_DMA_CTRL_DMA_INT_ENA_B | USB_DMA_CTRL_DMA_ENA_B ));

       /* wait until dma is completed */
       while(!(*pUSB_DMA_INTERRUPT & 0x1));

       /* acknowledge dma interrupt */
       *pUSB_DMA_INTERRUPT &= 0x1;

       /* Reset DMA */
       set_dma_ctrl(EP0,(u16_t)(0));

       ssync();
#else /* Programmed I/O */
       wRxCount >>=1;

       for(i=0;i<wRxCount; i++)
           *pData++ = *pUSB_EP0_FIFO;
       ssync();
#endif /* USE_DMA */

       /* set data end and serviced rx pkt ready bits */

       if(pDev->Mode == MODE_DEVICE)
       {
          //*pUSB_CSR0 = ((1<< 6));
          ssync();
       }
       else /* MODE_OTG_HOST */
       {
          *pUSB_CSR0 &= ~(1 << 5);
	  *pUSB_CSR0 |= (1 << 11);
          WaitMilliSec(2);
       }

       while(!(*pUSB_INTRTX & 0x1));

       /* set the number of bytes received */
       pBuf->ProcessedElementCount = wRxCount;
       pBuf->ProcessedFlag         = true;

       /* acknowledge receive interrupt */
       *pUSB_INTRTX = 0x1;
       ssync();

      /* Invoke the EP zero callback handler */
      epZeroCallback(pDev->DeviceHandle,ADI_USB_EVENT_SETUP_PKT,pBuf->CallbackParameter);
    }
    return(0);
}

/**
 * Transmit - loads the short packet
 */
__ADI_USB_HDRC_SECTION_CODE
static s32_t LoadShortPacket(ADI_DEV_1D_BUFFER *pBuffer,s32_t dwProcessedBytes,s32_t dwEpNum)
{
ADI_USBOTG_DEVICE *pDev= &UsbOtgDevice;
u8_t *pData = ((u8_t*)pBuffer->Data) + dwProcessedBytes;
 PUSB_EP_INFO pEpInfo  = &pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj->EPInfo;
s32_t dwBytesRemaining = pEpInfo->TransferSize - dwProcessedBytes;

  USB_ASSERT(dwBytesRemaining == 0);
  
   reset_dma(dwEpNum);

  /* setup the data buffer */
  set_dma_addr(dwEpNum,(u16_t)ADDR_LOW(pData),(u16_t)ADDR_HIGH(pData));

  /* setup no# bytes to send */
  set_dma_count(dwEpNum,(u16_t)ADDR_LOW(dwBytesRemaining),(u16_t)ADDR_HIGH(dwBytesRemaining));

  /* enable dma and generate interrupt */
   set_dma_ctrl(dwEpNum,(u16_t)(USB_DMA_CTRL_DMA_ENA_B |
                                USB_DMA_CTRL_DMA_DIR_B |
                                USB_DMA_CTRL_DMA_INT_ENA_B | ((dwEpNum&0xf) << 4)));

   /* wait for the short packet to be loaded*/
   while(!(*pUSB_DMA_INTERRUPT & (1 << dwEpNum)));

    /* acknowledge dma interrupt */
    *pUSB_DMA_INTERRUPT = (1 << dwEpNum);

   return(0);

}

/**
 * DevDmaInterruptHandler()
 * Handles DMA interrupt
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER_RESULT DevDmaInterruptHandler(ADI_USBOTG_DEVICE *pDev)
{
s32_t dwEpNum;
volatile s16_t intrDMA = *pUSB_DMA_INTERRUPT;
s32_t *pDmaMode=0,dwMaxEpSize=0;
bool isOut;
ENDPOINT_OBJECT *pEpO=NULL;
ADI_DEV_1D_BUFFER    *pBuffer;
s32_t dwDmaEndAddress,dwDmaStartAddress,dwProcessedBytes,dwSavedEpNum;
PUSB_EP_INFO pEpInfo;


    dwSavedEpNum = *pUSB_INDEX;

    /* TODO: optimize loop as we only care about the bits thats set in DMA */
    for(dwEpNum=1; dwEpNum< NUM_PHYSICAL_ENDPOINTS; dwEpNum++)
    {
        if((intrDMA >> dwEpNum ) & 0x1)
        {
            /* select the endpoint */
            *pUSB_INDEX  = dwEpNum;

            pDmaMode = &pDev->PhysicalEndpointObjects[dwEpNum].DmaMode;
            pEpO     = pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj;
            pBuffer  = (ADI_DEV_1D_BUFFER*)pEpO->EPInfo.pTransferBuffer;
            pEpInfo  = &pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj->EPInfo;
            dwMaxEpSize = pEpO->pEndpointDesc->wMaxPacketSize;

            USB_ASSERT(dwMaxEpSize == 0);
            USB_ASSERT(pDev->PhysicalEndpointObjects[dwEpNum].eIsDmaPending != 1);
            USB_ASSERT(pBuffer == NULL);

            /* Reset DMA pending to zero */
            pDev->PhysicalEndpointObjects[dwEpNum].eIsDmaPending = 0;

            /* acknowledge dma interrupt */
            *pUSB_DMA_INTERRUPT = (1 << dwEpNum);

            /* Get the buffer start address */
            dwDmaStartAddress = (s32_t)pBuffer->Data;

            /* Get the buffer end address */
            get_dma_addr(dwEpNum,(unsigned int*)&dwDmaEndAddress);

            /* Check if the endpoint is OUT or IN endpoint so that we can reset
             * the appropriate CSR registers.
             *
             * Device Mode - OUT - RXCSR
             * Device Mode - IN  - TXCSR
             *
             * Host Mode   - OUT - TXCSR
             * Host Mode   - IN  - RXCSR
             */
             if(UsbOtgDevice.Mode == MODE_DEVICE)
                isOut = (((pEpO->pEndpointDesc->bEndpointAddress >> 7) & 0x1) == 0) ? true : false;
             else
                isOut = (((pEpO->pEndpointDesc->bEndpointAddress >> 7) & 0x1) == 0) ? false : true;

            if(isOut)
            {
                 if(pDev->RxDmaMode == DMA_MODE_0)
                 {
                      s32_t dwTotalBytes = pBuffer->ElementWidth * pBuffer->ElementCount;

                      *pUSB_RXCSR &= ~(1 << 0);

                      if(pEpInfo->EpBytesProcessed < dwTotalBytes)
                      {
                          /* we do not have packet yet, so host mode we send IN tokens */
                          if(pDev->Mode == MODE_OTG_HOST)
                          {
                              /* Send IN Tokens to get data */
                               *pUSB_RXCSR |= (1 << 5); 
                          }
                          continue;
                       }

                       *pUSB_INTRRXE  &= ~(1 << dwEpNum);

                       pBuffer->ProcessedElementCount = pEpInfo->EpBytesProcessed;
                       pBuffer->ProcessedFlag = true;
                       pEpO->EPInfo.pTransferBuffer = NULL;
                       pDev->PhysicalEndpointObjects[dwEpNum].eIsDmaPending = 0;
                       reset_dma(dwEpNum);

                       /* return the received packet */
                       (pEpO->EPInfo.EpCallback)(pDev->DeviceHandle,ADI_USB_EVENT_DATA_RX,(void*)pBuffer->CallbackParameter);
                      /* if there are pending receive operations schedule it */
                      if(pEpO->EPInfo.pFreeRcvList != NULL)
                      {
                           ReceiveDataBuffer(pDev,dwEpNum);
                      }
                  } /* DMA_MODE_0 */
		          else
                  {
                      /* Get total number of received bytes */
                      dwProcessedBytes = dwDmaEndAddress - dwDmaStartAddress;

                      USB_ASSERT(dwProcessedBytes <=0);

                      /* Set number of received bytes */
                      pBuffer->ProcessedElementCount = dwProcessedBytes;

                      /* Isonchronous or Interrupt endpoint */
                      if( !(IS_BULK(pEpO->pEndpointDesc->bAttributes)))
                      {
                          /* Clear PktRdy bit */
                          *pUSB_RXCSR &= ~(1 << 0);
                      }
                      else /* BULK */
                      {
                          /* if we are in mode 0 or in device mode we reset RXCSR. In hostmode extra 
                           * IN tokens that are going out may fetch data from device clearing RXCSR 
                           * may result in loss of packet. 
                           * 
                           */		      
                           if(((pDev->Mode == MODE_DEVICE) && (dwProcessedBytes % dwMaxEpSize) && (*pDmaMode == 1)) 
				                || (*pDmaMode == 0))
                                    *pUSB_RXCSR  = 0;

                           *pDmaMode = 0;
                      }

                      pBuffer->ProcessedFlag = true;
                      pEpO->EPInfo.pTransferBuffer = NULL;

                      reset_dma(dwEpNum);

                      /* return the received packet */
                      (pEpO->EPInfo.EpCallback)(pDev->DeviceHandle,ADI_USB_EVENT_DATA_RX,(void*)pBuffer->CallbackParameter);

                      /* if there are pending receive operations schedule it */
                      if(pEpO->EPInfo.pFreeRcvList != NULL)
                      {
                           ReceiveDataBuffer(pDev,dwEpNum);
                      }
                 } /* DMA_MODE_0 */
           } /* isOut */
           else /* IN endpoint TX */
           {
                 /* Get total number of received bytes */
                 dwProcessedBytes = dwDmaEndAddress - dwDmaStartAddress;

                 s32_t dwTotalTransferSize = pEpInfo->TransferSize;
                 s32_t dwBytesRemaining = dwTotalTransferSize - dwProcessedBytes;
		 
                 /* Set number of received bytes */
                 pBuffer->ProcessedElementCount = dwProcessedBytes;
                 
                 if(*pDmaMode == 0)
                 {
                     /* Set TxPktRdy bit in the CSR register */
                      if(!IS_ISONCHRONOUS(pEpO->pEndpointDesc->bAttributes))
                         *pUSB_TXCSR  = 0x1;
                      else
                         *pUSB_TXCSR  |= 0x1; /* do not overwrite ISO bit */

                      while(!(*pUSB_INTRTX & (1<< dwEpNum))) ;
                      *pUSB_INTRTX = (1 << dwEpNum);


                 }

                  /* if there is a  short packet at the end, we have to manually set TxPktRdy bit */
                 if(IS_BULK(pEpO->pEndpointDesc->bAttributes) && dwBytesRemaining)
                 {
                           /* this condition may happen rarely */
                        if(*pUSB_TXCSR & (1 << 1))
                        {
                            while(*pUSB_TXCSR & (1 << 1)) ;
                        }
                          LoadShortPacket(pBuffer,dwProcessedBytes,dwEpNum);
                         /* set the Tx Ready bit for the short packet to go */
                          *pUSB_TXCSR = 0x1;
                                                  
                         /* wait for the short packet to go */
                          while(!(*pUSB_INTRTX & (1<< dwEpNum))) ;
                          pBuffer->ProcessedElementCount += dwBytesRemaining;
                  }

                 /* reset dma mode */
                 *pDmaMode = 0;

                 if((*pUSB_INTRTX & (1<< dwEpNum)))
                   *pUSB_INTRTX = (1 << dwEpNum);

                pBuffer->ProcessedFlag = true;
                reset_dma(dwEpNum);
                pEpO->EPInfo.pTransferBuffer = NULL;

                /* return the transmitted buffer */
                (pEpO->EPInfo.EpCallback)(pDev->DeviceHandle,ADI_USB_EVENT_DATA_TX,(void*)pBuffer->CallbackParameter);

                /* if there are pending transmit then schedule it */
                if(pEpO->EPInfo.pQueuedXmtList != NULL)
                {
                    TransmitDataBuffer(pDev,dwEpNum);
                }
           }
       }
    } /* for */

   *pUSB_INDEX = dwSavedEpNum;

   return(ADI_INT_RESULT_PROCESSED);
}

/**
 * USB Rx Interrupt Handler
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER(UsbRxInterruptHandler)
{
ADI_INT_HANDLER_RESULT ret = ADI_INT_RESULT_NOT_PROCESSED;
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;

    if(pDev->RxInterruptHandler != NULL)
    ret = pDev->RxInterruptHandler();

   return(ret);
}

/**
 * USB Tx Interrupt Handler
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER(UsbTxInterruptHandler)
{
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;
ADI_INT_HANDLER_RESULT ret = ADI_INT_RESULT_NOT_PROCESSED;
u8_t ucIrqStatus = *pUSB_INTRTX;

    /* if there is no tx endpoint interrupt to service return */
    if(!(ucIrqStatus & 0xff))
    {
      return ADI_INT_RESULT_NOT_PROCESSED;
    }

    if (pDev->Mode != MODE_NONE)
    {
      u16_t RxCount = *pUSB_COUNT0;
    
            /* if endpoint 0 interrupt */
            if((ucIrqStatus & 0x01) && (*pUSB_CSR0 & 0x1)) 
            {
                ProcessEpZeroData(pDev);
		ret = ADI_INT_RESULT_PROCESSED;
            }
            /* else other endpointinterrupt */
            else 
            {
               if(pDev->TxInterruptHandler != NULL)
                 ret = pDev->TxInterruptHandler();
            }
    }
    return(ret);
}

/**
 * USB Dma Interrupt Handler
 */
__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER(UsbDmaInterruptHandler)
{
ADI_USBOTG_DEVICE *pDev = &UsbOtgDevice;
ADI_INT_HANDLER_RESULT ret = ADI_INT_RESULT_NOT_PROCESSED;

   ret = DevDmaInterruptHandler(pDev);
   return(ret);
}
/**
 *
 * USB OTG Interrupt Handler.  By default all USB interrupts were mapped to IVG11.
 * Rx and Tx handlers will be plugged based on the * operating mode of the device.
 * But there is only one interrupt handler (both host/device) for the USB interrupt.
 *
 *  USB_INT0 - INTRRX
 *  USB_INT1 - INTRTX
 *  USB_INT2 - INTRUSB
 *  USB_DMA  - DMA Interrupt
 */

__ADI_USB_HDRC_SECTION_CODE
static ADI_INT_HANDLER(UsbOtgInterruptHandler)
{
u8_t ucIrqStatus;
ADI_USBOTG_DEVICE *pDev=&UsbOtgDevice;
ADI_INT_HANDLER_RESULT ret = ADI_INT_RESULT_NOT_PROCESSED;

    /* IF (Drive VBUS off interrupt) */
    /* This interrupt is generated when a device is plugged into the type A receptacle.
     * what we now do is drive VBUS off as requested and set the Session bit again.
     * This results in the immediate recognition of the Type A connection, whereupon
     * we drive VBUS again. Next interrupt to happen is the CONN_B interrupt as the device
     * is connected at this point.
    */
    u16 intrusb = *pUSB_OTG_VBUS_IRQ;
    if ( (intrusb&0x0002) && (pDev->Mode!=MODE_OTG_HOST) )
    {
        ucIrqStatus = *pUSB_INTRUSB;
        /* Acknowledge the Interrupt to clear it */
        *pUSB_OTG_VBUS_IRQ |= 0x0002;
        /* Turn VBUS off*/
        DriveVbusOff(pDev);
        /* Start detecting a session again */
        SetSessionBit();
        /* 100ms is a sensible time to wait for the Type A connection to be detected */
        WaitMilliSec(100);
        /* read session type and drive VBUS if host */
        u16 RegVal = *pUSB_OTG_DEV_CTL;
        if ((RegVal&USB_OTG_DEV_CTL_B_DEVICE_B)!=USB_OTG_DEV_CTL_B_DEVICE_B )
        {
            DriveVbusOn(pDev);
        }
        /* Acknowledge all interrupts */
        *pUSB_INTRUSB = ucIrqStatus;
        /* Nothing further to do */
        return  ADI_INT_RESULT_PROCESSED;
    }

    ret = UsbInterruptHandler();
    return (ret);
}

/**
 * Transmit Data buffer. This function is used to setup DMA for any Data Endpoint.
 */
__ADI_USB_HDRC_SECTION_CODE
static int TransmitDataBuffer(ADI_USBOTG_DEVICE *pDev,s32_t dwEpNum)
{
ADI_DEV_1D_BUFFER *pBuffer=NULL;
s16_t  *pData;
s32_t wTxCount,*pDmaMode,dwSavedEpNum;
u32_t uInterruptStatus=cli();
PUSB_EP_INFO pEpInfo=NULL;
ENDPOINT_OBJECT *pEpO=NULL;
s32_t dwShortPacketLength = 0;

       pEpO = pDev->PhysicalEndpointObjects[dwEpNum].pLogicalEndpointObj;

      /* If there is a pending write we just return, Once dma completes we
       * process the queued tx packet
       */
       if(pEpO->EPInfo.pTransferBuffer !=NULL)
       {
           sti(uInterruptStatus);
           return(0);
       }
     
       /* If we get here we must have a queued write */
       pBuffer = (ADI_DEV_1D_BUFFER*)GetXmtBuffer(pDev,dwEpNum);
       USB_ASSERT(pBuffer == NULL);

       pEpO->EPInfo.pTransferBuffer = (ADI_DEV_BUFFER*)pBuffer;
       pData  = (short*)pBuffer->Data;
       USB_ASSERT(pData == NULL);

       wTxCount = pBuffer->ElementCount * pBuffer->ElementWidth;
       pEpO->EPInfo.TransferSize = wTxCount;
       USB_ASSERT(wTxCount == 0);
       
       dwShortPacketLength = wTxCount % pEpO->pEndpointDesc->wMaxPacketSize;

              
       /* if we have a short packet at the end we schedule it seperately */
       if( (wTxCount > pEpO->pEndpointDesc->wMaxPacketSize) && (dwShortPacketLength > 0))
       {
         wTxCount -= dwShortPacketLength; 
       }

       dwSavedEpNum = *pUSB_INDEX;

       /* set the EP number */
       *pUSB_INDEX = dwEpNum;
       
       /* clear any underrun */
       *pUSB_TXCSR &= ~(1 << 2);

       /* check if FIFO is empty or not */
       if(*pUSB_TXCSR & (1 << 1))
       {
           /* only for the bulk endpoints we will wait */
	   if(IS_BULK(pEpO->pEndpointDesc->bAttributes))
                  while(*pUSB_TXCSR & (1 << 1)) ;
	   else
               *pUSB_TXCSR |= (1 << 3);

       }

       if(!IS_ISONCHRONOUS(pEpO->pEndpointDesc->bAttributes))
          *pUSB_TXCSR  = 0x0;

       pDmaMode = &pDev->PhysicalEndpointObjects[dwEpNum].DmaMode;
       pEpInfo = &pEpO->EPInfo;
       pEpInfo->pTransferBuffer = (ADI_DEV_BUFFER*)pBuffer;

       /* If transmit buffer size is greater than the max endpoint size then switch to DMA mode 1*/
       *pDmaMode = (wTxCount > *pUSB_TX_MAX_PACKET) ? 1 : 0;

       pDev->PhysicalEndpointObjects[dwEpNum].eIsDmaPending = 1;

       /* setup the data buffer */
       set_dma_addr(dwEpNum,(u16_t)ADDR_LOW(pData),(u16_t)ADDR_HIGH(pData));

       /* setup no# bytes to send */
       set_dma_count(dwEpNum,(u16_t)ADDR_LOW(wTxCount),(u16_t)ADDR_HIGH(wTxCount));

       if(IS_BULK(pEpO->pEndpointDesc->bAttributes))
       {
            if(*pDmaMode == 0)
            {
                /* enable dma and generate interrupt */
                set_dma_ctrl(dwEpNum,(u16_t)(USB_DMA_CTRL_DMA_ENA_B |
                                             USB_DMA_CTRL_DMA_DIR_B |
                                             USB_DMA_CTRL_DMA_INT_ENA_B | ((dwEpNum&0xf) << 4)));

            }
            else /* DMA mode 1 */
            {
               /* enable dma,generate interrupt and set dma mode 1 */
                set_dma_ctrl(dwEpNum,(u16_t)(USB_DMA_CTRL_DMA_ENA_B |
                                             USB_DMA_CTRL_DMA_DIR_B |
                                             USB_DMA_CTRL_DMA_INT_ENA_B | ((dwEpNum&0xf) << 4) |
                                             USB_DMA_CTRL_DMA_MODE_B));

               /* Set DMA request enable and DMA mode enable bits */
                *pUSB_TXCSR  |= ((1 << 10) | (1 << 12) | (1 << 15));
            }
       }
       else /* For Isonchronous and Interrupt endpoint */
       {
           /* enable dma and generate interrupt */
           set_dma_ctrl(dwEpNum,(u16_t)(USB_DMA_CTRL_DMA_ENA_B |
                                        USB_DMA_CTRL_DMA_DIR_B |
                                        USB_DMA_CTRL_DMA_INT_ENA_B | ((dwEpNum&0xf) << 4)));
       }

       ssync();
       *pUSB_INDEX = dwSavedEpNum;

      sti(uInterruptStatus);
      return(1);
}

/**
 *
 * Transmits data on endpoint zero.
 */
__ADI_USB_HDRC_SECTION_CODE
static s32_t TransmitEpZeroBuffer(ADI_DEV_1D_BUFFER *pBuffer, USB_EP_INFO *pEpInfo)
{
        ADI_USBOTG_DEVICE *pDev=&UsbOtgDevice;

        u8_t   ucEpNum  = (pBuffer->Reserved[4] & 0xff);
        s16_t  *pData   = (s16_t*)pBuffer->Data;
        s32_t  wTxCount;
        u32_t  uInterruptStatus;

        wTxCount = pBuffer->ElementCount * pBuffer->ElementWidth;
        pEpInfo->pTransferBuffer = (ADI_DEV_BUFFER*)pBuffer;
        pEpInfo->EpBytesProcessed += wTxCount;

        /* set the EP number */
        *pUSB_INDEX = ucEpNum;

        if( (pDev->Mode == MODE_OTG_HOST) && (*pUSB_INTRTX & 0x1) &&
                (*pUSB_CSR0 & 0x1) && (*pUSB_COUNT0 == 0) )
        {
            /* FLUSH FIFO */
            *pUSB_CSR0 |= ((1 << 8) | (1 << 11));
        }
#ifdef USE_DMA
        uInterruptStatus = cli();
                      
        /* Reset DMA */
        set_dma_ctrl(EP0,(u16_t)(0));

        /* setup the data buffer */
        set_dma_addr(EP0,(u16_t)ADDR_LOW(pData),(u16_t)ADDR_HIGH(pData));

        /* setup no# bytes to send */
        set_dma_count(EP0,(u16_t)ADDR_LOW(wTxCount),(u16_t)ADDR_HIGH(wTxCount));

        /* enable dma and generate interrupt */
        set_dma_ctrl(EP0,(u16_t)(USB_DMA_CTRL_DMA_ENA_B | USB_DMA_CTRL_DMA_DIR_B | USB_DMA_CTRL_DMA_INT_ENA_B));

        /* wait until dma is completed, synchronous for EP0 */
        while(!(*pUSB_DMA_INTERRUPT & 0x1));

        /* acknowledge dma interrupt */
        *pUSB_DMA_INTERRUPT &= 0x1;

        /* Reset DMA */
        set_dma_ctrl(EP0,(u16_t)(0));
           
#else /* programmed I/O */

        /* convert no# of bytes to transfer to no# of words */
        wTxCount >>=1;
        for(i=0;i<wTxCount;i++)
            *pUSB_EP0_FIFO = *pData++;
        ssync();
#endif /* USE_DMA */

        if(pDev->Mode == MODE_DEVICE)
        {
#if 0
            /* set TxPktReady & DATAEND if last packet */
            if (pEpInfo->EpBytesProcessed==pEpInfo->TransferSize ) {
                *pUSB_CSR0 |= (1 << 1) | (1 << 3);
            } else {
                *pUSB_CSR0 |= (1 << 1);
            }
#endif
            *pUSB_CSR0 |= (1 << 1);
            ssync();

            /* loop until data is gone */
            while(*pUSB_CSR0 & 0x2);

            *pUSB_INTRTX = 0x1;

        }
        else /* MODE_OTG_HOST */
        {
            ssync();
	        /* set setup pkt bit and txpktrdy bit */
            *pUSB_CSR0 = 0xA;
            ssync();

	        while(*pUSB_CSR0 & 0xA);

            while(!(*pUSB_INTRTX & 0x1)) ;
            *pUSB_INTRTX= 0x1;

	        /* flush the fifo once setup packet is sent
	         */
	         *pUSB_CSR0 |= ((1 << 8) | (1 << 1));

	        /* Acknowledge error */
	        if(*pUSB_CSR0 & (1 << 4)) 
	        {
               *pUSB_CSR0 &= ~(1 << 4);
               *pUSB_INTRTX= 0x1;
	        }

	        /* TODO: If stall issued terminate the transaction */
	        if(*pUSB_CSR0 & (1 << 2)) 
	        {
		       *pUSB_CSR0 &= ~(1 << 2); 
	        }
        }

        sti(uInterruptStatus);

        return(1);
}


/**
 *
 * Dump registers into the memory. This function is used
 * for the debugging purpose.
 *
 */
#define DUMP_REG_SIZE 512
__ADI_USB_HDRC_SECTION_DATA
static int DumpRegArea[DUMP_REG_SIZE];
__ADI_USB_HDRC_SECTION_CODE
static void* DumpUsbOTGRegisters()
{
int i=0,*pDA = DumpRegArea;

       *pDA++ = *pUSB_FADDR;          // 0
       *pDA++ = *pUSB_POWER;          // 1
       *pDA++ = *pUSB_INTRTX;         // 2
       *pDA++ = *pUSB_INTRRX;         // 3
       *pDA++ = *pUSB_INTRTXE;        // 4
       *pDA++ = *pUSB_INTRRXE;        // 5
       *pDA++ = *pUSB_INTRUSB;        // 6
       *pDA++ = *pUSB_INTRUSBE;       // 7
       *pDA++ = *pUSB_FRAME;          // 8
       *pDA++ = *pUSB_INDEX;          // 9
       *pDA++ = *pUSB_TESTMODE;       // 10
       *pDA++ = *pUSB_GLOBINTR;       // 11
       *pDA++ = *pUSB_GLOBAL_CTL;     // 12
       *pDA++ = *pUSB_CSR0;           // 13
       *pDA++ = *pUSB_COUNT0;         // 14

       return((void*)DumpRegArea);
}

/**
 * StartOtgDevice
 *
 * Configures the OTG as a peripheral
 *
 * @param pDev pointer to the USB Device object
 * @return 1 upon success
 *
 */
__ADI_USB_HDRC_SECTION_CODE
static u32 StartOtgDevice(ADI_USBOTG_DEVICE *pDev)
{
    pDev->Mode = MODE_DEVICE;

    *pUSB_POWER &=  ~(USB_POWER_SOFT_CONN_B);

    /* check if application forced us for a specifc speed */
    if((pDev->Speed == ADI_USB_DEVICE_SPEED_UNKNOWN) || (pDev->Speed == ADI_USB_DEVICE_SPEED_HIGH) )
        *pUSB_POWER |=  (USB_POWER_SOFT_CONN_B | USB_POWER_HS_ENABLE_B);
    else
    {
        /* disable high speed mode */
        *pUSB_POWER &=  ~(USB_POWER_HS_ENABLE_B);
        *pUSB_POWER |=  (USB_POWER_SOFT_CONN_B);
    }
    ssync();

    /* enable additional interrupts that are valid in peripheral mode */
    *pUSB_INTRUSBE    |= (USB_INTRUSB_SUSPEND_B | USB_INTRUSB_RESUME_B);

    /* get the device speed */
    pDev->Speed = (*pUSB_POWER & (USB_POWER_HS_MODE_B)) ? ADI_USB_DEVICE_SPEED_HIGH : ADI_USB_DEVICE_SPEED_FULL;

    /* Mark the device state as started */
    pDev->State = DEV_STATE_STARTED;

    return 1;
}

/**
 * SetSessionBit
 *
 * Configures the OTG controller to detect insertion of A or B lead
 *
 */
__ADI_USB_HDRC_SECTION_CODE
static void SetSessionBit(void)
{
    /* drive VBUS in OTG host mode */

    /* Set up USB to listen on USB_ID */
    *pUSB_OTG_DEV_CTL |= 1;
    //wait 1ms - USB 2.0 specification
    WaitOneMilliSec();
}

/**
 * ClearSessionBit
 *
 * Terminates the current session
 *
 */
__ADI_USB_HDRC_SECTION_CODE
static void ClearSessionBit(void)
{
    /* Set up USB to stop listening on USB_ID */
    *pUSB_OTG_DEV_CTL = 0;
    //wait 1ms - USB 2.0 specification
    WaitOneMilliSec();

}

__ADI_USB_HDRC_SECTION_CODE
static void DriveVbusOn(ADI_USBOTG_DEVICE *pDev)
{
#if defined (__ADSP_MOCKINGBIRD__)
    adi_flag_Clear(pDev->VbusDriveFlag);
#else
    adi_flag_Set(pDev->VbusDriveFlag);
#endif
    /* Wait until VBUS goes high */
    while( (*pUSB_OTG_DEV_CTL&0x0018)!=0x0018) ;
}

__ADI_USB_HDRC_SECTION_CODE
static void DriveVbusOff(ADI_USBOTG_DEVICE *pDev)
{
#if defined (__ADSP_MOCKINGBIRD__)
    adi_flag_Set(pDev->VbusDriveFlag);
#else
    adi_flag_Clear(pDev->VbusDriveFlag);
#endif

    if(pDev->Mode == MODE_OTG_HOST)
    {
      u16 frame = *pUSB_FRAME;
      /* Wait until VBUS goes low */
      while((*pUSB_FRAME)==frame && (*pUSB_OTG_DEV_CTL&0x0018)>1) ;
    }
}

__ADI_USB_HDRC_SECTION_CODE
static void HostModeReset(ADI_USBOTG_DEVICE *pDev)
{
    u16 uHighSpeedEnableB = 0x0;

    /* Enable High Speed and suspend modes - device will negotiate down if it cannot support it
    */
    if( (pDev->Speed == ADI_USB_DEVICE_SPEED_UNKNOWN) || (pDev->Speed == ADI_USB_DEVICE_SPEED_HIGH))
    {
      uHighSpeedEnableB = USB_POWER_HS_ENABLE_B;
    }
    *pUSB_POWER = ( (USB_POWER_ENABLE_SUSPENDM_B) | (uHighSpeedEnableB));

    /* perform the host mode reset sequence to initiate the device
    */
    *pUSB_POWER |= USB_POWER_RESET_B ;
    WaitMilliSec(20);
    *pUSB_POWER &= ~USB_POWER_RESET_B;
}

__ADI_USB_HDRC_SECTION_CODE
static void ResetOnDisconnect(void)
{
   /* Reset the device address */
   *pUSB_FADDR = 0x0;

   /* Reset device index */
   *pUSB_INDEX = 0x0;

   /* Flush EP0 FIFO */
    *pUSB_CSR0 |= (1 << 8);
    ssync();

    /* Flush again */
    *pUSB_CSR0 |= (1 << 8);
    ssync();

   /* Reset EP0 Control and Status register */
   *pUSB_CSR0 = 0x0;

   /* Reset power register */
   *pUSB_POWER &= ~0x0040;

   /* Reset interrupt registers */
   u16 uStatus = *pUSB_INTRRX;
   *pUSB_INTRRX  = uStatus;
   uStatus = *pUSB_INTRTX;
   *pUSB_INTRTX  = uStatus;
   uStatus = *pUSB_INTRUSB;
   *pUSB_INTRUSB = uStatus;

   /* Reset frame counter register */
   *pUSB_FRAME = 0x0;

   *pUSB_GLOBAL_CTL = 0;
   WaitMilliSec(60);
   *pUSB_GLOBAL_CTL = 1;

}

