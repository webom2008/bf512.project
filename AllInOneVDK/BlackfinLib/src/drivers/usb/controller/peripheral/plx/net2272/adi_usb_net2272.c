/*******************************************************************************

  Copyright(c) 2009 Analog Devices, Inc. All Rights Reserved.

  This software is proprietary and confidential.  By using this software you
  agree to the terms of the associated Analog Devices License Agreement.

  $RCSfile: adi_usb_net2272.c,v $ $Revision: 131 $ $Date: 2009-11-04 15:40:56 -0500 (Wed, 04 Nov 2009) $

Description: NET2272 USB peripheral controller driver.

*******************************************************************************/

#include <drivers/usb/controller/peripheral/plx/net2272/adi_usb_net2272.h>
#include <drivers/usb/usb_core/adi_usb_objects.h>
#include <drivers/usb/usb_core/adi_usb_core.h>
#include <drivers/usb/usb_core/adi_usb_debug.h>
#include <services/services.h>
#include <drivers/adi_dev.h>
#include <string.h>

/* Macros */
#define NUM_ENDPOINTS 4

#define DEBUG_NET2272

#ifdef DEBUG_NET2272
#define DUMP_REGISTERS(msg)  DumpNet2272Registers(msg)
#else
#define DUMP_REGISTERS(msg)
#endif /* DEBUG_NET2272 */

/* positive-logic assertions */
#define USB_CHECK(cond)  USB_ASSERT(!(cond))

#define DATA_FIFO (void*)EP_DATA_ADDR

#define EP_INTERRUPTS (EP_0_INTERRUPT | \
                       EP_A_INTERRUPT | \
                       EP_B_INTERRUPT | \
                       EP_C_INTERRUPT)

#define EP_BYTES_AVAIL ((u8_t)_inpb_d(EP_AVAIL0_ADDR) | \
                        (((u8_t)_inpb_d(EP_AVAIL1_ADDR) & 7u) << 8u))

#define USB_BUS_SPEED  ((_inpb_d(USBCTL1_ADDR) & USB_HIGH_SPEED) ? \
        ADI_USB_DEVICE_SPEED_HIGH : ADI_USB_DEVICE_SPEED_FULL)

/* Blackfin memory range definitions */
#define L1_MSB_MASK 0xFFF00000
#define COREA_L1_INSTRUCTION_SRAM_BASE_ADR  0xFFA00000
#define COREB_L1_INSTRUCTION_SRAM_BASE_ADR  0xFF600000
#define COREA_L1_DATA_BANK_B_SRAM_BASE_ADR  0xFF900000
#define COREA_L1_DATA_BANK_A_SRAM_BASE_ADR  0xFF800000
#define COREB_L1_DATA_BANK_B_SRAM_BASE_ADR  0xFF500000
#define COREB_L1_DATA_BANK_A_SRAM_BASE_ADR  0xFF400000
#define COREA_L1_SCRATCH_PAD_SRAM_BASE_ADR  0xFFB00000
#define COREB_L1_SCRATCH_PAD_SRAM_BASE_ADR  0xFF700000

/* minimum bytes to DMA: 4 is h/w minimum, 80 is a performance guess */
#define HW_MINIMUM_DMA_COUNT 4
#ifndef MINIMUM_DMA_COUNT
#define MINIMUM_DMA_COUNT 80
#endif

/* define USE_RX_DMA/USE_TX_DMA non-zero to use memory DMA for transfers
 * between data buffers and NET2272 FIFOs
 */
#ifndef USE_RX_DMA
#define USE_RX_DMA 1
#endif
#ifndef USE_TX_DMA
#define USE_TX_DMA 0
#endif
#if USE_RX_DMA || USE_TX_DMA
#define USE_DMA
#endif

#define DATA_WIDTH_16 (1 << 0)  /* in LOCCTL register */

/* Queue */
typedef enum EpQueue
{
    FREE_RCV_QUEUE,   /* queue used to fill an incoming packet */
    FREE_XMT_QUEUE,   /* transmitted packets placed here       */
    PROC_RCV_QUEUE,   /* pending receive operating buffer      */
    WAIT_XMT_QUEUE    /* to be transmited packets              */
} EP_QUEUE;

/******************************************************************************
 * Physical Endpoint structure. USB core operates at logical endpoint level.
 * once host selects a configuration and an interface the associated logical
 * endpoint objects of the currently active interface will be bound to the
 * physical endpoint objects. The binding holds good until host again sets or
 * resets the configuration and interface.
 */
typedef struct PhysicalEndpointObject
{
    ENDPOINT_OBJECT  *pLogicalEndpointObj;   /* logical endpoint object */
    int              eEndpointState;         /* endpoint state          */
    int              PhysicalEndpointID;     /* Physical endpoint ID    */
} PHYSICAL_ENDPOINT_OBJECT;

/* NET2272 Device state-Internal to the Driver */
typedef enum DeviceState
{
    DEV_STATE_RESET=0,
    DEV_STATE_OPEN_IN_PROCESS,
    DEV_STATE_OPENED,
    DEV_STATE_CLOSED,
    DEV_STATE_STARTED,
} DEVICE_STATE;

/* Statistics collection structure */
typedef struct Net2272Stats
{
    u32_t num_setup_ints;
    u32_t num_setup_completed;
    u32_t num_setup_with_data;
    u32_t num_rx_ints[NUM_ENDPOINTS];
    u32_t num_rx_zero_length[NUM_ENDPOINTS];
    u32_t num_rx_no_buffer[NUM_ENDPOINTS];
    u32_t num_rx_buffer_empty[NUM_ENDPOINTS];
    u32_t num_rx_completed[NUM_ENDPOINTS];
    u32_t num_tx_ints[NUM_ENDPOINTS];
    u32_t num_tx_completed[NUM_ENDPOINTS];
} NET2272_STATS;

/* NET2272 Core Device Structure */
typedef struct Net2272DeviceData
{
    ADI_DEV_DEVICE_HANDLE    DeviceHandle;
    ADI_DMA_MANAGER_HANDLE   DMAHandle;
    ADI_DCB_HANDLE           DCBHandle;
    ADI_DCB_CALLBACK_FN      DMCallback;
    ADI_DEV_DIRECTION        Direction;
    void                    *pCriticalRegionArg;
    bool                     Started;
    DEVICE_STATE             State;
    PHYSICAL_ENDPOINT_OBJECT PhysicalEndpointObjects[NUM_ENDPOINTS];
    bool                     EP0DataExpected;
    bool                     SetAddressExpected;
    int                      NumPhysicalEndpoints;
    ADI_DMA_STREAM_ID        DmaStreamID;
    u32                      DmaStreamIVG;
    ADI_DMA_STREAM_HANDLE    DmaStreamHandle;
    ADI_SEM_HANDLE           DmaSemaphore;
    bool                     UseDmaSemaphore;
    ADI_INT_PERIPHERAL_ID    PeripheralID;
    u32                      PeripheralIVG;
    int                      PF_Reset;
    int                      PF_Int;
    int                      PF_Clear;
    int                      PF_Set;
    int                      DeviceID;
    DEVICE_OBJECT           *pDeviceObj;
    NET2272_STATS            Stats;
    bool                     Cache;
    int                      BufferPrefix;
    ADI_USB_DEVICE_SPEED     Speed;
} ADI_NET2272_DEVICE;


/* Global Data  */
static ADI_NET2272_DEVICE    Net2272Device = {0};
 
static PHYSICAL_EP_INFO PhysicalEpAdditionalInfo[NUM_ENDPOINTS] =
{
    {.dwEpFifoSize =  128}, /* EP 0: 2 x  64 bytes */
    {.dwEpFifoSize = 1024}, /* EP A: 2 x 512 bytes */
    {.dwEpFifoSize = 1024}, /* EP B: 2 x 512 bytes */
    {.dwEpFifoSize = 1024}  /* EP C: 2 x 512 bytes */
};

#ifdef USE_DMA
static u8_t tmpBuffer[64*1024];     /* temporary memory DMA data buffer */

/* mapping of device IDs to interrupt IDs for memory DMA */
#ifdef __ADSP_BRODIE__
static enum ADI_INT_PERIPHERAL_ID dmaPeriphIDs[] = {
    ADI_INT_MDMA0,      /* ADI_DMA_MDMA_0 */
    ADI_INT_MDMA1       /* ADI_DMA_MDMA_1 */
};
#elif defined(__ADSP_EDINBURGH__)
static enum ADI_INT_PERIPHERAL_ID dmaPeriphIDs[] = {
    ADI_INT_MDMAS0,     /* ADI_DMA_MDMA_0 */
    ADI_INT_MDMAS1      /* ADI_DMA_MDMA_1 */
};
#elif defined(__ADSP_BRAEMAR__)
static enum ADI_INT_PERIPHERAL_ID dmaPeriphIDs[] = {
    ADI_INT_MDMAS0,     /* ADI_DMA_MDMA_0 */
    ADI_INT_MDMAS1      /* ADI_DMA_MDMA_1 */
};
#elif defined(__ADSP_TETON__)
static enum ADI_INT_PERIPHERAL_ID dmaPeriphIDs[] = {
    ADI_INT_MDMA1_S0,   /* ADI_DMA_MDMA1_0 */
    ADI_INT_MDMA1_S1,   /* ADI_DMA_MDMA1_1 */
    ADI_INT_MDMA2_S0,   /* ADI_DMA_MDMA2_0 */
    ADI_INT_MDMA2_S1    /* ADI_DMA_MDMA2_1 */
};
#else
#error Unsupported target processor
#endif /* processor targets */

#define MEMORY_STREAM_COUNT (sizeof(dmaPeriphIDs)/sizeof(dmaPeriphIDs[0]))
#endif /*def USE_DMA */

/* function prototypes */
static int InitProcessorSpecificConfiguration(ADI_NET2272_DEVICE *pDev);

static int SetProcessorSpecificDefaultConfiguration(ADI_NET2272_DEVICE *pDev);

static int TxDataToNet2272(ADI_NET2272_DEVICE* pDev,
                           u8_t *pu8Data,
                           u32_t ByteCount);

static int RxDataFromNet2272(ADI_NET2272_DEVICE* pDev,
                             u8_t *pu8Data,
                             u32_t ByteCount);

static void WriteEpDataWithPIO(u8_t *pByte, u32_t Count);

static void ReadEpDataWithPIO(u8_t *pByte, u32_t Count);

static ADI_INT_HANDLER_RESULT Net2272InterruptHandler(void);

static void DumpNet2272Registers(char* msg);

#ifdef DEBUG_NET2272
static struct _net2272_regs_t {
    char* message;
    u8 irqstat0;
    u8 irqstat1;
    u8 irqenb0;
    u8 irqenb1;
    u8 usbctl0;
    u8 usbctl1;
    u8 locctl;
    u8 locctl1;
    u8 setup[8];
    u8 pagesel;
    struct _ep_regs_t {
        u8 epstat0;
        u8 epstat1;
        u8 eptransfer0;
        u8 eptransfer1;
        u8 eptransfer2;
        u8 epirqenb;
        u8 epavail0;
        u8 epavail1;
        u8 eprspclr;
        u8 eprspset;
        u8 epmaxpkt0;
        u8 epmaxpkt1;
        u8 epcfg;
        u8 ephbw;
        u8 epbuffstates;
    }  _ep_regs[NUM_ENDPOINTS];
} _net2272_regs;

static void DumpNet2272Registers(char* msg)
{
    unsigned int imask = cli();
    int ep;
    int i;

    _net2272_regs.message  = msg; 

    _net2272_regs.irqstat0 = _inpb_d(IRQSTAT0_ADDR);
    _net2272_regs.irqstat1 = _inpb_d(IRQSTAT1_ADDR);
    _net2272_regs.irqenb0  = _inpb_ind(IRQENB0);
    _net2272_regs.irqenb1  = _inpb_ind(IRQENB1);
    _net2272_regs.usbctl0  = _inpb_d(USBCTL0_ADDR);
    _net2272_regs.usbctl1  = _inpb_d(USBCTL1_ADDR);
    _net2272_regs.locctl   = _inpb_ind(LOCCTL);
    _net2272_regs.locctl1  = _inpb_ind(LOCCTL1);

    i = 0;
    _net2272_regs.setup[i++] = _inpb_ind(SETUP0);
    _net2272_regs.setup[i++] = _inpb_ind(SETUP1);
    _net2272_regs.setup[i++] = _inpb_ind(SETUP2);
    _net2272_regs.setup[i++] = _inpb_ind(SETUP3);
    _net2272_regs.setup[i++] = _inpb_ind(SETUP4);
    _net2272_regs.setup[i++] = _inpb_ind(SETUP5);
    _net2272_regs.setup[i++] = _inpb_ind(SETUP6);
    _net2272_regs.setup[i++] = _inpb_ind(SETUP7);

    _net2272_regs.pagesel  = _inpb_d(PAGESEL_ADDR);
    for (ep = 0; ep < NUM_ENDPOINTS; ep += 1)
    {
        _outpb_d(PAGESEL_ADDR, ep);
        _net2272_regs._ep_regs[ep].epstat0      = _inpb_d(EP_STAT0_ADDR);
        _net2272_regs._ep_regs[ep].epstat1      = _inpb_d(EP_STAT1_ADDR);
        _net2272_regs._ep_regs[ep].eptransfer0  = _inpb_d(EP_TRANSFER0_ADDR);
        _net2272_regs._ep_regs[ep].eptransfer1  = _inpb_d(EP_TRANSFER1_ADDR);
        _net2272_regs._ep_regs[ep].eptransfer2  = _inpb_d(EP_TRANSFER2_ADDR);
        _net2272_regs._ep_regs[ep].epirqenb     = _inpb_d(EP_IRQENB_ADDR);
        _net2272_regs._ep_regs[ep].epavail0     = _inpb_d(EP_AVAIL0_ADDR);
        _net2272_regs._ep_regs[ep].epavail1     = _inpb_d(EP_AVAIL1_ADDR);
        _net2272_regs._ep_regs[ep].eprspclr     = _inpb_d(EP_RSPCLR_ADDR);
        _net2272_regs._ep_regs[ep].eprspset     = _inpb_d(EP_RSPSET_ADDR);
        _net2272_regs._ep_regs[ep].epmaxpkt0    = _inpb_ind(EP_MAXPKT0);
        _net2272_regs._ep_regs[ep].epmaxpkt1    = _inpb_ind(EP_MAXPKT1);
        _net2272_regs._ep_regs[ep].epcfg        = _inpb_ind(EP_CFG);
        _net2272_regs._ep_regs[ep].ephbw        = _inpb_ind(EP_HBW);
        _net2272_regs._ep_regs[ep].epbuffstates = _inpb_ind(EP_BUFF_STATES);

    }

    _outpb_d(PAGESEL_ADDR, _net2272_regs.pagesel);

    sti(imask);
}
#endif

/* Blackfin interrupt mask manipulation */

static bool disableIVG(unsigned int ivg)
{
    /* get current mask (and protect ourselves from interrupts) */
    unsigned int imask = cli();

    /* reenable all enabled interrupts except for specified IVG */
    sti(imask & ~(1 << ivg));

    /* return indication of whether specified IVG was enabled */
    return (imask & (1 << ivg)) != 0;
}

static void reenableIVG(unsigned int ivg)
{
    /* get current mask (and protect ourselves from interrupts) */
    unsigned int imask = cli();

    /* reenable all enabled interrupts plus specified IVG */
    sti(imask | (1 << ivg));
}

/* Buffer queue manipulation functions.
 *
 */
/*
 * enqueueBuffer appends the supplied buffer (chain) to the specified queue
 * for the specified endpoint. Assumes that the caller has masked off the USB
 * controller interrupt.
 */
static void enqueueBuffer(ADI_DEV_BUFFER *pBuffer,
                          PUSB_EP_INFO    pEpInfo,
                          EP_QUEUE        eQueue)
{
    ADI_DEV_BUFFER **pQueueHead;
    ADI_DEV_1D_BUFFER *tmpBuffer;

    USB_CHECK(eQueue == FREE_RCV_QUEUE ||
              eQueue == FREE_XMT_QUEUE ||
              eQueue == PROC_RCV_QUEUE ||
              eQueue == WAIT_XMT_QUEUE   );

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

/*
 * dequeueBuffer removes the first buffer from the specified queue for
 * the specified endpoint. Assumes that the caller has masked off the USB
 * controller interrupt. Returns a pointer to the removed buffer, or NULL
 * if the queue was empty.
 */
static ADI_DEV_BUFFER* dequeueBuffer(PUSB_EP_INFO    pEpInfo,
                                     EP_QUEUE        eQueue)
{
    ADI_DEV_BUFFER **pQueueHead;
    ADI_DEV_1D_BUFFER *tmpBuffer;

    USB_CHECK(eQueue == FREE_RCV_QUEUE ||
              eQueue == FREE_XMT_QUEUE ||
              eQueue == PROC_RCV_QUEUE ||
              eQueue == WAIT_XMT_QUEUE   );

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
    }

    if(*pQueueHead == NULL)
        return NULL;
    else
    {
        tmpBuffer = (ADI_DEV_1D_BUFFER*)*pQueueHead;
        *pQueueHead = (ADI_DEV_BUFFER*)tmpBuffer->pNext;

        tmpBuffer->pNext = NULL;
        return (ADI_DEV_BUFFER*)tmpBuffer;
    }
}

/* NET2272 Device Functions */

/*******************************************************************************
 *
 * Wait for the specified amount of time in msec.
 * TODO: implement using the current clock settings.
 */
static void delay(int mSec)
{
    volatile int i;
    for (i = 0; i < 0x6fffff; i++);
}


/*******************************************************************************
 *
 * Reset physical endpoints. On NET2272 we have four endpoints
 *
 * EP0 - Default control pipe - PAGESEL value --> 0x00
 * EPA - Data Endpoint -A     - PAGESEL value --> 0x01
 * EPB - Data Endpoint -B     - PAGESEL value --> 0x02
 * EPC - Data Endpoint -C     - PAGESEL value --> 0x03
 *
 * Most of the EP_xxx registers are directly addressable.
 */
static int ResetPhysicalEndPoints(ADI_NET2272_DEVICE *pDev)
{
    int ep;

    /* Reset physical endpoint object structures to zero */
    /* TODO: Reset the old bindings (logical - physical) if they exist */
    memset(pDev->PhysicalEndpointObjects,
           0,
           sizeof(pDev->PhysicalEndpointObjects));

    _outpb_ind(IRQENB0, (_inpb_ind(IRQENB0) & ~( (EP_A_INTERRUPT) |
                                                 (EP_B_INTERRUPT) |
                                                 (EP_C_INTERRUPT) |
                                                 (VIRTUAL_EP_INTERRUPT) )));

    _outpb_d(USBCTL1_ADDR, (_inpb_d(USBCTL1_ADDR) & ~(VIRTUAL_EP_INTERRUPT)));

    for (ep = 1; ep < NUM_ENDPOINTS; ep += 1)
    {
        /* select the EP */
        _outpb_d(PAGESEL_ADDR, ep);

        /* Reset configuration register values */
        _outpb_ind(EP_CFG, 0x00);

        /* clear all EPSTAT0 interrupts by writing 1 */
        _outpb_d(EP_STAT0_ADDR, EP_STAT0_INT_ALL);

        /* clear all EPSTAT1 status interrupts by writing 1 */
        _outpb_d(EP_STAT1_ADDR, EP_STAT1_INT_ALL);

        /* disable all interrupts */
        _outpb_d(EP_IRQENB_ADDR, 0x00);
    }

    return 1;
}

/*******************************************************************************
 *
 *  Resets critical NET2272 registers. This includes the Interrupt
 *  registers, control registers and status registers used to ack
 *  interrupts.
 */
static void ResetNet2272(ADI_NET2272_DEVICE *pDev)
{
    unsigned short tempReg;
    volatile unsigned int v;
    unsigned int uInterruptStatus = cli();
    
    v = _inpb_d(USBCTL0_ADDR);
    
    if (v == 0x20)
    {
        /* Root-Port Wakeup will be detected when there is USB activity on
         * the bus.
         */
      	sti(uInterruptStatus);
      	return;
    }

    /* clear any pending interrupts Write 1 to clear */
    _outpb_d(IRQSTAT0_ADDR, IRQSTAT0_ALL_INT);

    /* clear any pending interrupts Write 1 to clear */
    _outpb_d(IRQSTAT1_ADDR, IRQSTAT1_ALL_INT);

    /* Reset Interrupt Enable Registers */
    _outpb_ind(IRQENB0,0x00);
    _outpb_ind(IRQENB1,0x00);

    /* Reset USB Control registers */
    do {
        _outpb_d(USBCTL0_ADDR,0x00);
        delay(1);
        v = _inpb_d(USBCTL0_ADDR);
    } while (v != 0);
    
    /* Enable I/O Wake up and Root-Port wake up */
    _outpb_d(USBCTL0_ADDR,~(1 << 3));

    _outpb_d(USBCTL1_ADDR,0x00);

    /* Reset default device address to 0 */
    _outpb_ind(OURADDRESS,0x00);

    /* configure to 16-bit */
    _outpb_ind(LOCCTL,(_inpb_ind(LOCCTL) | DATA_WIDTH_16));

    /* Reset all data endpoints */
    ResetPhysicalEndPoints(pDev);

    sti(uInterruptStatus);

    /* now toggle the NET2272 reset line */

#ifdef __ADSP_BRODIE__
    *pPORTHIO_DIR |= pDev->PF_Reset;
    *pPORTHIO_CLEAR = pDev->PF_Reset;
#endif

#ifdef __ADSP_EDINBURGH__
    *pFIO_DIR |= pDev->PF_Reset;
    *pFIO_FLAG_C = pDev->PF_Reset;
#endif

#ifdef __ADSP_BRAEMAR__
    *pPORTFIO_DIR |= pDev->PF_Reset;
    *pPORTFIO_CLEAR = pDev->PF_Reset;
#endif

#ifdef __ADSP_TETON__
    *pFIO0_DIR |= pDev->PF_Reset;
    *pFIO0_FLAG_C = pDev->PF_Reset;
#endif

    /* delay...hold it low */
    for (v = 0; v < 0xfffff; v++)
        ;

#ifdef __ADSP_BRODIE__
    *pPORTHIO_SET = pDev->PF_Reset;
#endif

#ifdef __ADSP_EDINBURGH__
    *pFIO_FLAG_S = pDev->PF_Reset;
#endif

#ifdef __ADSP_BRAEMAR__
    *pPORTFIO_SET = pDev->PF_Reset;
#endif

#ifdef __ADSP_TETON__
    *pFIO0_FLAG_S = pDev->PF_Reset;
#endif

    /* delay...let it settle high */
    for (v = 0; v < 0xfffff; v++)
        ;

    return;
}

/*******************************************************************************
 *
 *  Check direct and indirect address register read/writes using
 *  the SCRATCH register and the CHIP Revision register.
 *
 */
static int CheckNet2272(unsigned int reg_addr)
{
    int i;
    unsigned char ucRet;

    /* read chipID register default value 0x11 */
    if (_inpb_ind(CHIPREV_2272) != 0x11) return -1;

    /* read legacy chip revision number */
    ucRet = _inpb_ind(CHIPREV_LEGACY);

    for (i = 0;i < 0xf; i++)
    {
        _outpb_d(reg_addr,i);
        ucRet = _inpb_d(reg_addr);
        if (ucRet != i)
            break;
    }

    if (i != 0xf)
        return -1;

    return 1;
}

/*******************************************************************************
 *
 * Enables NET2272 for data communication. USB interrupts will
 * start coming in once this function finishes execution.
 *
 */
static int EnableNet2272(ADI_NET2272_DEVICE *pDev)
{
    unsigned char ucStatus;
    unsigned int uIntStatus = cli();

    /* Setup the default EP0 physical object to hold the logical EP0 object */
    pDev->PhysicalEndpointObjects[EP0].pLogicalEndpointObj = 
        pDev->pDeviceObj->pEndpointZeroObj;

    /* Ensure EP0 max packet size is set up */
    pDev->pDeviceObj->pEndpointZeroObj->pEndpointDesc->wMaxPacketSize = 64;

    /* Enable EP0 interrupts */
    _outpb_d(PAGESEL_ADDR,EP0);
    _outpb_d(EP_RSPCLR_ADDR, EP_NAK_OUT_PACKETS_MODE |
                             EP_HIDE_STATUS_PHASE);

    /* Enable appropriate data transfer interrupts */
    _outpb_ind(IRQENB0, (EP_0_INTERRUPT | SETUP_PKT_INT));

    /* Enable appropriate bus event interrupts */
    _outpb_ind(IRQENB1,(ROOT_PORT_RESET_INT    |
                        SUSPEND_RQT_CHANGE_INT |
                        VBS_INT));
    _outpb_ind(IRQENB1,(_inpb_ind(IRQENB1) & (~(SUSPEND_RQT_INT))));

    /* ensure local bus width is 16 */
    ucStatus = _inpb_ind(LOCCTL);
    ucStatus |= DATA_WIDTH_16;
    _outpb_ind(LOCCTL, ucStatus);

    /* Set USBCTL0,enable all interrupts */
    _outpb_d(USBCTL0_ADDR, USBCTL0_ENABLE_ALL);
    delay(1);

    sti(uIntStatus);
    return 1;
}

/*******************************************************************************
 *
 * BusEventHandler deals with events signalled in IRQSTAT1 register.
 * These are ROOT_PORT_RESET, SUSPEND_REQUEST_CHANGE, VBUS.
 *
 */
static int BusEventHandler(ADI_NET2272_DEVICE *pDev, unsigned char intStatus)
{

    static unsigned char ucStatus;

    ADI_DCB_CALLBACK_FN epZeroCallback = pDev->
        PhysicalEndpointObjects[EP0].pLogicalEndpointObj->EPInfo.EpCallback;

    /* ensure that endpoint 0 has a callback installed */
    if (epZeroCallback == NULL)
        return -1;

    ucStatus = _inpb_d(USBCTL1_ADDR);

    if (intStatus & ROOT_PORT_RESET_INT)
    {
        epZeroCallback(pDev->DeviceHandle, ADI_USB_EVENT_ROOT_PORT_RESET, 0);
    }

    if (intStatus & SUSPEND_RQT_CHANGE_INT)
    {
        epZeroCallback(pDev->DeviceHandle, ADI_USB_EVENT_SUSPEND, 0);
    }

    if (intStatus & VBS_INT)
    {
        /* Check to see if device is connected */
        if ((ucStatus & 1) == 0)
        {
            epZeroCallback(pDev->DeviceHandle, ADI_USB_EVENT_DISCONNECT, 0);
        }           
    }

    return 0;
}

/*******************************************************************************
 *
 * Sets up endpoint's data pointer and transfer counts for a new transfer.
 *
 */
static void setupTransfer(PUSB_EP_INFO pEpInfo, EP_QUEUE eQueue)
{
    ADI_DEV_1D_BUFFER *pBuffer =
        (ADI_DEV_1D_BUFFER*)dequeueBuffer(pEpInfo, eQueue);

    if (pBuffer != NULL)
    {
        pEpInfo->TransferSize = pBuffer->ElementCount * pBuffer->ElementWidth;
    }
    pEpInfo->EpBytesProcessed = 0;
    pEpInfo->pTransferBuffer = (ADI_DEV_BUFFER*)pBuffer;
}

/*******************************************************************************
 * Return true if the supplied SETUP packet will be followed by OUT data 
 * packets, else return false.
 *
 */
static bool hasData(SETUP_PACKET* pkt)
{
    if ((pkt->bmRequestType & EP_DIR_IN) == 0)
        return pkt->wLength > 0;
    else
        return false;
}

/*******************************************************************************
 * Return true if the supplied SETUP packet is a SET ADDRESS, else return false.
 *
 */
static bool isSetAddress(SETUP_PACKET* pkt)
{
    const u8_t set_addr_rt = (USB_DIR_HOST_TO_DEVICE |
                              USB_TYPE_STANDARD      |
                              USB_RECIPIENT_DEVICE);

    if (pkt->bmRequestType == set_addr_rt && 
        pkt->bRequest == USB_STD_RQST_SET_ADDRESS &&
        pkt->wIndex == 0)
        return true;
    else
        return false;
}

/*******************************************************************************
 *
 * SetupPktHandler is responsible for handling SETUP packets on endpoint 0.
 *
 * Net2272 saves the 8 bytes from the data phase in registers SETUP0..7 .
 * This function takes a buffer from the receive queue, copies the setup
 * data into it and then calls the endpoint 0 callback.
 *
 */
static int SetupPktHandler(ADI_NET2272_DEVICE *pDev, unsigned char intStatus)
{
    ADI_DEV_1D_BUFFER *pBuf;
    unsigned char *pData;
    PUSB_EP_INFO pEpInfo = &pDev->
        PhysicalEndpointObjects[EP0].pLogicalEndpointObj->EPInfo;

    pDev->Stats.num_setup_ints++;

    /* look for a free receive buffer */
    pBuf = (ADI_DEV_1D_BUFFER*)dequeueBuffer(pEpInfo, FREE_RCV_QUEUE);

    /* if we were able to get hold of a free buffer copy the SETUP data to it */
    if (pBuf != NULL && ((pBuf->ElementCount * pBuf->ElementWidth) >= 8))
    {
        /* acknowledge any previous control status phase interrupt */
        _outpb_d(IRQSTAT1_ADDR, CONTROL_STATUS_INT);

        /* read the 8 bytes of SETUP packet data */
        pData = (unsigned char*)pBuf->Data;
        *pData++ = _inpb_ind(SETUP0);
        *pData++ = _inpb_ind(SETUP1);
        *pData++ = _inpb_ind(SETUP2);
        *pData++ = _inpb_ind(SETUP3);
        *pData++ = _inpb_ind(SETUP4);
        *pData++ = _inpb_ind(SETUP5);
        *pData++ = _inpb_ind(SETUP6);
        *pData++ = _inpb_ind(SETUP7);

        /* note whether to expect any associated incoming DATA packet(s)
         * since it affects how we treat the response phase of the STATUS
         * transaction
         */
        pDev->EP0DataExpected = hasData((SETUP_PACKET*)pBuf->Data);

        /* note whether this is a SET ADDRESS standard setup since its
         * acknowledgement must be delayed until after the core sends us
         * the command to change the address
         */
        pDev->SetAddressExpected = isSetAddress((SETUP_PACKET*)pBuf->Data);

        pBuf->ProcessedFlag = true;
        pBuf->ProcessedElementCount = 8 / pBuf->ElementWidth;

        /* ensure we are told about any associated incoming data */
        if (pDev->EP0DataExpected)
        {
            _outpb_d(PAGESEL_ADDR, EP0);
            _outpb_d(EP_IRQENB_ADDR, EP_DATA_RCVD_INT);

            pDev->Stats.num_setup_with_data++;
        }

        /* Invoke the EP zero callback handler */
        pEpInfo->EpCallback(pDev->DeviceHandle,
                            ADI_USB_EVENT_SETUP_PKT,
                            pBuf->CallbackParameter);

        pDev->Stats.num_setup_completed++;
    };

    return 0;
}


/*******************************************************************************
 *
 * Copies packets from the controller's FIFO to the buffers queued on the
 * endpoint and invokes the callback for each buffer.
 */
static int RxHandler(ADI_NET2272_DEVICE *pDev,
                     unsigned char       ucEpNum)
{
    PUSB_EP_INFO pEpInfo = &pDev->
        PhysicalEndpointObjects[ucEpNum].pLogicalEndpointObj->EPInfo;
    ADI_DEV_1D_BUFFER *pBuffer;
    unsigned int bytes_avail, byte_count, bytes_left;
    unsigned int epstat0, epstat1;
    u8_t *pData;
    bool bRequestSatisfied = false;

    USB_CHECK(pEpInfo != NULL);

    /* increment the rx interrupt counter */
    pDev->Stats.num_rx_ints[ucEpNum]++;

    /* get the current receive buffer */
    pBuffer = (ADI_DEV_1D_BUFFER*)pEpInfo->pTransferBuffer;

    /* if NULL, get the next free receive buffer for this EP */
    if (NULL == pBuffer)
    {
        setupTransfer(pEpInfo, FREE_RCV_QUEUE );
        pBuffer = (ADI_DEV_1D_BUFFER*)pEpInfo->pTransferBuffer;
    }

    /* if NULL we neither have active read nor pending read, but we got data;
     * inform application about read and see if it can give a buffer to fill;
     * if we can not get a buffer we will leave the data sitting in the fifo
     * and disable interrupts from this endpoint until the application calls
     * adi_dev_Read() again.
     */
    if (NULL == pBuffer)
    {
        /* alert the app that we have received data but there was not a free
         * buffer */
        pEpInfo->EpCallback(0,
                            ADI_USB_EVENT_PKT_RCVD_NO_BUFFER,
                            (void*)pBuffer->CallbackParameter);
        pBuffer = (ADI_DEV_1D_BUFFER*)pEpInfo->pTransferBuffer;

        if (!pBuffer)
        {
            /* still no read buffer: disable interrupt */
            _outpb_d(PAGESEL_ADDR, ucEpNum);
            _outpb_d(EP_IRQENB_ADDR, 0x00);

            /* count 'received data but no buffer available' */
            pDev->Stats.num_rx_no_buffer[ucEpNum]++;

            return 1;
        }
    }

    _outpb_d(PAGESEL_ADDR, ucEpNum);

    /* read endpoint's current status */
    epstat0 = _inpb_d(EP_STAT0_ADDR);
    epstat1 = _inpb_d(EP_STAT1_ADDR);

    /* avoid false RX interrupts (mainly EP 0) */
    if (epstat0 & EP_BUFFER_EMPTY)
    {
        pDev->Stats.num_rx_buffer_empty[ucEpNum]++;
        return 1;
    }

    /* find out how many bytes are available */
    bytes_avail = EP_BYTES_AVAIL;

    /* if there was no data in the buffer it may just be a zero-length
     * packet */
    if (0 == bytes_avail)
    {
        /* disable interrupt if there is no pending read */
        if (pEpInfo->pFreeRcvList == NULL)
            _outpb_d(EP_IRQENB_ADDR, 0x00);

        /* counts the zero length packets */
        pDev->Stats.num_rx_zero_length[ucEpNum]++;

        return 1;
    }

    /* this is how many bytes are left to transfer, if not specified just
     * get what's available */
    if (pEpInfo->TransferSize)
    {
        bytes_left = pEpInfo->TransferSize - pEpInfo->EpBytesProcessed;
    }
    else
    {
        pEpInfo->TransferSize = bytes_avail;
        bytes_left = bytes_avail;
    }
    
    /* acknowledge interrupt */
    _outpb_d(EP_STAT0_ADDR, (EP_DATA_RCVD_INT | EP_DATA_OUT_TOKEN_INT));

    /* set data pointer to proper location */
    pData = (u8_t*)pBuffer->Data + pEpInfo->EpBytesProcessed;

    /* make sure we have enough room in the buffer for all the available
     * bytes */
    if (bytes_avail > bytes_left)
        byte_count = bytes_left;
    else
        byte_count = bytes_avail;

    /* read data from NET2272 via memory DMA or PIO */
    RxDataFromNet2272(pDev, pData, byte_count);

    /* update processed byte count */
    pEpInfo->EpBytesProcessed += byte_count;

    /* re-initialise endpoint 0 transfer registers to avoid
     * erroneous pre-validation of subsequent control reads
     */
    if (ucEpNum == 0) {
        _outpb_d(EP_TRANSFER2_ADDR, 0x00);
        _outpb_d(EP_TRANSFER1_ADDR, 0x00);
        _outpb_d(EP_TRANSFER0_ADDR, 0x00);
    }

    /* check if the request has been satisfied */
    if (pEpInfo->EpBytesProcessed == pEpInfo->TransferSize)
    {
        bRequestSatisfied = true;
    }


    /* If requested transfer is finished or application does not care about the
     * length that is going to be received we return the buffer.
     */
    if (bRequestSatisfied || (EP_MODE_PACKET == pEpInfo->TransferMode))
    {
        pBuffer->ProcessedFlag = true;
        pBuffer->ProcessedElementCount = pEpInfo->EpBytesProcessed /
                                             pBuffer->ElementWidth;
        pEpInfo->pTransferBuffer = NULL;
        pEpInfo->TransferSize = pEpInfo->EpBytesProcessed;

        /* disable interrupt if EP0 or there is no pending read */
        if (ucEpNum == 0 || pEpInfo->pFreeRcvList == NULL)
        {
            _outpb_d(EP_IRQENB_ADDR, 0x00);
        }

        /* count 'number of completed rx requests' */
        pDev->Stats.num_rx_completed[ucEpNum]++;

        /* pass the buffer to the application */
        pEpInfo->EpCallback(pDev->DeviceHandle,
                            (ucEpNum == 0)? ADI_USB_EVENT_SETUP_PKT :
                            ADI_USB_EVENT_DATA_RX,
                            (void*)pBuffer->CallbackParameter);
    }

    return 1;
}

/*******************************************************************************
 *
 * Copies packets from the buffers queued on the endpoint to the controller's
 * FIFO and invokes the callback for each buffer.
 */
static int TxHandler(ADI_NET2272_DEVICE *pDev,
                     unsigned char       ucEpNum)
{
    ENDPOINT_OBJECT *pEpO = pDev->
        PhysicalEndpointObjects[ucEpNum].pLogicalEndpointObj;
    PUSB_EP_INFO pEpInfo = &pEpO->EPInfo;
    ADI_DEV_1D_BUFFER *pBuffer;
    u8_t  *pData;
    unsigned int epstat0;
    unsigned int bytes_avail, byte_count, bytes_left;

    /* increment the tx interrupt counter */
    pDev->Stats.num_tx_ints[ucEpNum]++;

    /* get the buffer for this EP */
    pBuffer = (ADI_DEV_1D_BUFFER*) pEpInfo->pTransferBuffer;

    _outpb_d(PAGESEL_ADDR, ucEpNum);

    /* if app hasn't specified any data for transmission yet, there's not
     * much we can do but wait for the data to be available for a future IN
     * TOKEN
     */
    if (NULL == pBuffer)
    {
        /* acknowledge interrupt */
        _outpb_d(EP_STAT0_ADDR, EP_DATA_IN_TOKEN_INT);
        return 1;
    }

    /* this interrupt may be because the last data was just transferred */
    if (pEpInfo->EpBytesProcessed == pEpInfo->TransferSize)
    {
        /* clear xmtd and IN TOKEN interrupts */
        _outpb_d(EP_STAT0_ADDR,( EP_DATA_XMTD_INT     |
                                 EP_DATA_IN_TOKEN_INT |
                                 EP_SHORT_PKT_TFRD_INT));

        /* disable interrupts */
        _outpb_d(EP_IRQENB_ADDR, 0x00);

        /* check to see if there's any data in the buffer */
        epstat0 = _inpb_d(EP_STAT0_ADDR);
        if (!(epstat0 & EP_BUFFER_EMPTY))
        {
            /* if so we must validate the EP buffer */
            _outpb_d(EP_TRANSFER0_ADDR, 0x00);
        }

        pBuffer->ProcessedFlag = true;

        pEpInfo->pTransferBuffer = NULL;
        pEpInfo->TransferSize = 0;
        pEpInfo->EpBytesProcessed = 0;

        /* call the EP callback to indicate transfer is complete */
        pEpInfo->EpCallback(pDev->DeviceHandle,
                                ADI_USB_EVENT_DATA_TX,
                                (void*)pBuffer->CallbackParameter);

        pDev->Stats.num_tx_completed[ucEpNum]++;

        return 1;
    }

    /* enable data xmtd interrupt for this endpoint, do not enable
     * IN TOKEN interrupts at this point
     */
    _outpb_d(EP_IRQENB_ADDR, EP_DATA_XMTD_INT);

    /* loop until all the data is transmitted or the buffer is full */
    while (1)
    {
        /* acknowledge data xmtd interrupt */
        _outpb_d(EP_STAT0_ADDR, EP_DATA_XMTD_INT);

        /* check to see if there's any room in the buffer */
        epstat0 = _inpb_d(EP_STAT0_ADDR);
        if (epstat0 & EP_BUFFER_FULL)
        {
            /* there's no room, we'll get interrupted again when the host takes
             * some data; in the meantime clear IN TOKEN interrupt
             */
            _outpb_d(EP_STAT0_ADDR, EP_DATA_IN_TOKEN_INT);
            break;
        }

        /* this is how many bytes are left to transfer */
        bytes_left = pEpInfo->TransferSize - pEpInfo->EpBytesProcessed;

        /* find out how much room is available */
        bytes_avail = EP_BYTES_AVAIL;

        /* if there's not enough room for the remaining data only transfer
         * what's available
         */
        if (bytes_avail < bytes_left)
            byte_count = bytes_avail;
        else
            byte_count = bytes_left;

        /* set data pointer to proper location */
        pData = (u8_t*)pBuffer->Data + pEpInfo->EpBytesProcessed;

        /* transfer data to NET2272 */
        TxDataToNet2272(pDev, pData, byte_count);

        /* update processed byte count */
        pEpInfo->EpBytesProcessed += byte_count;

        /* if there is no more data to transfer */
        if (pEpInfo->EpBytesProcessed == pEpInfo->TransferSize)
        {
            /* if it's a short packet we need to validate the transfer so that
             * the NET2272 will send it
             */
            if (byte_count < pEpO->pEndpointDesc->wMaxPacketSize)
            {
                /* validate the EP buffer */
                _outpb_d(EP_TRANSFER0_ADDR, 0x00);
            }
        }
        break;
    }

    return 1;
}

/*******************************************************************************
 *
 * Called to handle data, mostly for the receive on the endpoints.
 * Appropriate handler gets called depending on the interrupts.
 * Note that Rx interrupts on EP0 must be serviced but Tx interrupts
 * should be ignored ( Tx is done synchronously in TransmitBufferEp0() ).
 *
 */
static int ProcessEpDataInterrupts(ADI_NET2272_DEVICE *pDev)
{
    unsigned char epIrqStatus;
    int ep,ret;

    for(ep = 0; ep < NUM_ENDPOINTS; ep += 1)
    {
    	_outpb_d(PAGESEL_ADDR, ep);
    	epIrqStatus = _inpb_d(EP_STAT0_ADDR);

    	if (epIrqStatus & EP_DATA_RCVD_INT)
    	    ret = RxHandler(pDev, ep);
    	else if (epIrqStatus & (EP_DATA_IN_TOKEN_INT | EP_DATA_XMTD_INT))
            if (ep != EP0)
    	        ret = TxHandler(pDev, ep);
    }

    return ret;
}

/*******************************************************************************
 *
 * Net2272InterruptHandler responsible for identifying the interrupt
 * and invoking the appropriate handler.
 *
 */
ADI_INT_HANDLER_RESULT Net2272InterruptHandler(void)
{
    unsigned char ucIrqStatus;
    ADI_NET2272_DEVICE *pDev=&Net2272Device;

    /* read IRQSTAT1 */
    ucIrqStatus = _inpb_d(IRQSTAT1_ADDR);

    /* process interrupts */
    if (ucIrqStatus)
        BusEventHandler(pDev, ucIrqStatus);

    /* acknowledge by setting 1 */
    _outpb_d(IRQSTAT1_ADDR, ucIrqStatus);

    /* read IRQSTAT0 */
    ucIrqStatus = _inpb_d(IRQSTAT0_ADDR);

    /* check for Control Read or Control Write transaction */
    if (ucIrqStatus & SETUP_PKT_INT)
        SetupPktHandler(pDev, ucIrqStatus);

    /* check for data transfer conditions to be actioned */
    if (ucIrqStatus & EP_INTERRUPTS)
    {
        ProcessEpDataInterrupts(pDev);
    }

    /* clear the acknowledgeable interrupt status */
    _outpb_d(IRQSTAT0_ADDR, ucIrqStatus & 0xE0);

    return ADI_INT_RESULT_PROCESSED;
}

static u32 UsbStartNet2272(ADI_NET2272_DEVICE *pDev)
{
    u32 Result;
    volatile unsigned int v;    /* delay */
    unsigned char ucStatus;
    enum ADI_INT_PERIPHERAL_ID dmaPeriphID;

    /* setup with the selected configuration */
    InitProcessorSpecificConfiguration(pDev);

    /* check if caller has supplied a specific IVG level for the NET2272
     * interrupt, otherwise just get the current setting
     */
    if (pDev->PeripheralIVG == 0)
    {
        /* get current setting */
        adi_int_SICGetIVG(pDev->PeripheralID, &pDev->PeripheralIVG);
    }
    else
    {
        /* set user-supplied IVG level */
        adi_int_SICSetIVG(pDev->PeripheralID,  pDev->PeripheralIVG);
    }

    /* hook the interrupt handler to actual IVG level */
    if (adi_int_CECHook(pDev->PeripheralIVG,
                        (ADI_INT_HANDLER_FN)Net2272InterruptHandler,
                        pDev,TRUE) != ADI_INT_RESULT_SUCCESS)
    {
        pDev->State = DEV_STATE_RESET;
        return ADI_DEV_RESULT_CANT_HOOK_INTERRUPT;
    }

#ifdef USE_DMA
    /* check if caller has supplied a specific IVG level for the memory DMA
     * interrupt, otherwise just get the current setting
     */
    USB_CHECK(pDev->DmaStreamID < MEMORY_STREAM_COUNT);
    dmaPeriphID = dmaPeriphIDs[pDev->DmaStreamID];

    if (pDev->DmaStreamIVG == 0)
    {   /* get current setting */
        adi_int_SICGetIVG(dmaPeriphID, &pDev->DmaStreamIVG);
    }
    else
    {   /* set user-supplied IVG level */
        adi_int_SICSetIVG(dmaPeriphID,  pDev->DmaStreamIVG);
    }

    /* open the memory DMA channel */
    Result = adi_dma_MemoryOpen(pDev->DMAHandle,         /* DMA Manager       */
                                pDev->DmaStreamID,       /* DMA stream ID     */
                                NULL,                    /* client value      */
                                &pDev->DmaStreamHandle,  /* stream handle     */
                                NULL);                   /* direct callback   */

    if (Result != ADI_DMA_RESULT_SUCCESS)
    {
        pDev->State = DEV_STATE_RESET;
        /* unhook the interrupt handler */
        adi_int_CECUnhook(pDev->PeripheralIVG,
                          (ADI_INT_HANDLER_FN)Net2272InterruptHandler,
                          pDev);
        return Result;
    }
    
#endif /* USE_DMA */

    pDev->State = DEV_STATE_STARTED;

    /* allow this interrupt to wake us up and allow it to be passed to the
     * core
     */
    adi_int_SICWakeup(pDev->PeripheralID, TRUE);
    adi_int_SICEnable(pDev->PeripheralID);

    EnableNet2272(pDev);

    /* get the configured speed */
    pDev->Speed = USB_BUS_SPEED;

    /* delay.give the system some time to configure */
    for (v = 0; v < 0x6ffff; v++)
        ;

    return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************************
 *
 * We bind each logical Endpoint object with the physical Endpoint objects.
 * The Endpoint object that is passed in is the logical endpoint object present
 * in the currently active configuration.
 *
 */
static int BindPhysicalEndpoints(ENDPOINT_OBJECT *pLogicalEp)
{
    int ep;
    PHYSICAL_ENDPOINT_OBJECT *pPhyEpO;
    ENDPOINT_OBJECT *pLogEp = pLogicalEp;
    ENDPOINT_DESCRIPTOR *pEpD;
    ADI_NET2272_DEVICE *pDev = &Net2272Device;
    unsigned char EpCfg;


    for (ep = 1; ((ep < NUM_ENDPOINTS) && (pLogEp != NULL)); ep += 1)
    {
        _outpb_d(PAGESEL_ADDR, ep);

        /* get the logical endpoint object associated for this endpoint */
        pDev->PhysicalEndpointObjects[ep].pLogicalEndpointObj = pLogEp;

        /* get the endpoint descriptor */
        pEpD = pLogEp->pEndpointDesc;

        /* set endpoint ID */
        pDev->PhysicalEndpointObjects[ep].PhysicalEndpointID  = 
            pEpD->bEndpointAddress & 0x0F;

        /* configure the Max packet size */
        _outpb_ind(EP_MAXPKT0,LOW_BYTE_LW(pEpD->wMaxPacketSize));
        _outpb_ind(EP_MAXPKT1,HIGH_BYTE_LW(pEpD->wMaxPacketSize));

        /* EP_CFG register settings */
        EpCfg = ((pEpD->bEndpointAddress & 0x0F) |              /* EP number */
                (((pEpD->bEndpointAddress >> 7) & 0x1) << 4) |  /* EP dir    */
                (pEpD->bAttributes << 5) |                      /* EP type   */
                1 << 7);                                        /* enable    */

        _outpb_ind(EP_CFG,EpCfg);

        /* disable all interrupts */
        _outpb_d(EP_IRQENB_ADDR,0x00);

        /* reset all status registers */
        _outpb_d(EP_STAT0_ADDR,_inpb_d(EP_STAT0_ADDR));
        _outpb_d(EP_STAT1_ADDR,_inpb_d(EP_STAT1_ADDR));

        /* check the direction if its 0x1 means IN endpoint (host perspective)
           i.e you are going to send data on it, since endpoints are
           uni-directional we really care about only one handler at a time
        */
        if( (pEpD->bEndpointAddress >> 7) & 0x1)
        {
            _outpb_d(EP_IRQENB_ADDR,EP_DATA_XMTD_INT);
        }
        else /* OUT endpoint, you receive data on it */
        {
            _outpb_d(EP_IRQENB_ADDR,EP_DATA_RCVD_INT | EP_DATA_OUT_TOKEN_INT );
            _outpb_d(EP_RSPCLR_ADDR,
                     (_inpb_d(EP_RSPCLR_ADDR) | EP_NAK_OUT_PACKETS_MODE));
        }

        /* get the next logical endpoint */
        pLogEp = pLogEp->pNextActiveEpObj;
    }

    /* TODO: If more logicalendpoint objects than the number of physical
     * endpoints then it's an error
     */

    /* enable interrupts on EPs 0, A, B, C */
    _outpb_ind(IRQENB0,(_inpb_ind(IRQENB0) | EP_INTERRUPTS));

    return 1;
}

/*******************************************************************************
 *
 *
 * Transmits buffer on EP0. Endpoint ID is specified in Reserved[4]
 * area of the buffer.  USB interrupts assumed to be disabled by caller.
 * EP0 transmits have to be synchronous with respect to the usb core so
 * control stays in this function until all the data is sent.
 */
static int TransmitBufferEp0(ADI_DEV_1D_BUFFER *pBuffer)
{
    ADI_NET2272_DEVICE *pDev=&Net2272Device;

    volatile unsigned char epStatus;
    u8_t *transfer_data = (u8_t*)pBuffer->Data;
    u32_t transfer_length = pBuffer->ElementCount * pBuffer->ElementWidth;
    u32_t packet_length, max_pkt, remainder;
    ENDPOINT_OBJECT *pEpO = pDev->
        PhysicalEndpointObjects[EP0].pLogicalEndpointObj;

    max_pkt = pEpO->pEndpointDesc->wMaxPacketSize;
    remainder = transfer_length;

    /* select endpoint 0 */
    _outpb_d(PAGESEL_ADDR, EP0);

    do
    {
        /* clear off old transmit complete status */
        _outpb_d(EP_STAT0_ADDR,  EP_DATA_XMTD_INT);

        /* wait until EP0's packet buffer is available */
        epStatus = _inpb_d(EP_STAT0_ADDR);
        while (!(epStatus & EP_BUFFER_EMPTY))
            epStatus = _inpb_d(EP_STAT0_ADDR);

        packet_length = (remainder <= max_pkt)? remainder : max_pkt;

        /* write data to the buffer */
        WriteEpDataWithPIO(transfer_data, packet_length);

        remainder -= packet_length;
        transfer_data += packet_length;

    } while (remainder > 0);

    /* validate the final transfer if less than max packet size */
    if (packet_length < max_pkt)
        _outpb_d(EP_TRANSFER0_ADDR, 0x00);

    /* set processed byte count */
    pEpO->EPInfo.EpBytesProcessed = transfer_length;

    pBuffer->ProcessedElementCount = pBuffer->ElementCount;
    pBuffer->ProcessedFlag  = true;

    /* clear off the Transmit complete and IN token status bits */
    _outpb_d(EP_STAT0_ADDR, EP_DATA_XMTD_INT | EP_DATA_IN_TOKEN_INT);

    /* if short packet indicator is set then clear it */
    epStatus = _inpb_d(EP_STAT0_ADDR);
    if (epStatus & EP_SHORT_PKT_TFRD_INT)
        _outpb_d(EP_STAT0_ADDR, EP_SHORT_PKT_TFRD_INT);

    return 1;
}

#ifdef USE_DMA
/*********************************************************************
 *
 *  Function:       DmaCallback
 *
 *  Description:    Called when DMA memory copies complete. Posts the
 *                  semaphore on which the driver is pending.
 *
 *********************************************************************/
static void DmaCallback(void* pHandle, u32 u32Arg, void* pArg)
{
    ADI_NET2272_DEVICE* pDev = &Net2272Device;

    adi_sem_Post(pDev->DmaSemaphore);
}

static void flush_cache(void* addr, int len, bool invalidate)
{
    /* cache-line-aligned pointer through area to be flushed */
    char* line = (char*)((unsigned int)addr & (~31));
    /* pointer to first byte beyond area to be flushed */
    char* end  = (char*)addr + len;  

    if (invalidate) {
        ssync();
        while (line < end) {
            asm volatile("FLUSHINV[%0++];":"+p"(line));
        }
        ssync();
    } else {
        ssync();
        while (line < end) {
            asm volatile("FLUSH[%0++];":"+p"(line));
        }
        ssync();
    }
}

/*********************************************************************
 *
 *  Function:       doMemoryDMA
 *
 *  Description:    Initiates a memory to memory DMA and optinally
 *                  pends on user-supplied semaphore.
 *
 *********************************************************************/
static void doMemoryDMA(ADI_NET2272_DEVICE* pDev,
                        void*               src,
                        void*               dst,
                        u32_t               count,
                        u32_t               units)
{
    ADI_DMA_2D_TRANSFER DMASource;      /* DMA source structure */
    ADI_DMA_2D_TRANSFER DMADestination; /* DMA destination structure */
    u32 Result;

    DMASource.StartAddress      = src;
    DMASource.XCount            = count;
    DMASource.XModify           = (src == DATA_FIFO)? 0 : units;
    DMASource.YCount            = 1;
    DMASource.YModify           = 0;

    DMADestination.StartAddress = dst;
    DMADestination.XCount       = count;
    DMADestination.XModify      = (dst == DATA_FIFO)? 0 : units;
    DMADestination.YCount       = 1;
    DMADestination.YModify      = 0;

    /* worry about src or dst in data cache? */
    if (pDev->Cache) {
        if (src != DATA_FIFO) {
            /* ensure src is flushed to memory before DMA */
            flush_cache(src, (count * units), false);
        }

        if (dst != DATA_FIFO) {
            /* ensure any dst cache lines are invalidated so that post-DMA */
            /* accesses go to memory                                       */
            flush_cache(dst, (count * units), true);
        }
    }

    if (pDev->UseDmaSemaphore) {
        /* asynchronous DMA copy - pend on supplied semaphore */
        Result = adi_dma_MemoryCopy2D(pDev->DmaStreamHandle,
                                      &DMADestination,
                                      &DMASource,
                                      units,
                                      DmaCallback);

        if (Result != ADI_DMA_RESULT_SUCCESS)
        {
            volatile int x = 1;
            while (x) ;
        }

        adi_sem_Pend(pDev->DmaSemaphore, 0);

    } else {
        /* synchronous DMA copy - no return until it is completed */
        Result = adi_dma_MemoryCopy2D(pDev->DmaStreamHandle,
                                      &DMADestination,
                                      &DMASource,
                                      units,
                                      NULL);

        if (Result != ADI_DMA_RESULT_SUCCESS)
        {
            volatile int x = 1;
            while (x) ;
        }
    }
}

#endif /* USE_DMA */

/*********************************************************************
 *
 *  Function:       TxDataToNet2272
 *
 *  Description:    Transfers data to NET2272 using DMA when possible
 *
 *********************************************************************/
static int TxDataToNet2272(ADI_NET2272_DEVICE   *pDev,
                           u8_t                 *pu8Data,
                           u32_t                 ByteCount)
{
    int pL1_MSB = (int)pu8Data & L1_MSB_MASK;   /* mask */
    u32_t WordCount = ByteCount >> 1;           /* number of 16-bit words to */
                                                /* DMA; if there is an odd   */
                                                /* number of bytes it will be*/
                                                /* handled down below */
    int data_addr_is_odd = (int)pu8Data & 1;

    USB_CHECK(ByteCount > 0);

#if !USE_TX_DMA
    WriteEpDataWithPIO(pu8Data, ByteCount);
#else
    if (ByteCount < HW_MINIMUM_DMA_COUNT || data_addr_is_odd)
    {
        WriteEpDataWithPIO(pu8Data, ByteCount);
        return 1;
    }

    /* if the source is L1 instruction memory */
#if defined(__ADSP_TETON__)
    if ((pL1_MSB == COREA_L1_INSTRUCTION_SRAM_BASE_ADR) ||
        (pL1_MSB == COREB_L1_INSTRUCTION_SRAM_BASE_ADR) ||
        (pL1_MSB == COREA_L1_DATA_BANK_A_SRAM_BASE_ADR) ||
        (pL1_MSB == COREA_L1_DATA_BANK_B_SRAM_BASE_ADR) ||
        (pL1_MSB == COREB_L1_DATA_BANK_A_SRAM_BASE_ADR) ||
        (pL1_MSB == COREB_L1_DATA_BANK_B_SRAM_BASE_ADR))
#else
    if (pL1_MSB == COREA_L1_INSTRUCTION_SRAM_BASE_ADR)
#endif
    {
        /* can't DMA directly to NET2272 from L1 instruction memory, we
         * first must DMA to another memory location, then to NET2272
         * (external memory)
         */

        /* 1-byte transfers in case of odd alignment of source */
        doMemoryDMA(pDev, (void*)pu8Data, (void*)tmpBuffer, ByteCount, 1);

        /* 2-byte transfers since alignment is OK */
        doMemoryDMA(pDev, (void*)tmpBuffer, DATA_FIFO, WordCount, 2);
    }
    else /* source is not L1 instruction memory */
    {
        /* if less than the minimum DMA count or if it's SCRATCHPAD memory
         * don't use DMA
         */
        if ((WordCount*2 < MINIMUM_DMA_COUNT) ||
            (pL1_MSB == COREA_L1_SCRATCH_PAD_SRAM_BASE_ADR))
        {
            WriteEpDataWithPIO(pu8Data, ByteCount);
            return 1;
        }
        else /* we can use DMA */
        {
            u8_t* startAddr = pu8Data;

            /* 2-byte DMA transfers */
            doMemoryDMA(pDev, (void*)startAddr, DATA_FIFO, WordCount, 2);
        }
    }

    /* if there were an odd number of bytes write the last one */
    if( ByteCount & 0x1 )
    {
        WriteEpDataWithPIO(pu8Data + ByteCount - 1, 1);
    }

#endif /* USE_TX_DMA */

    return 1;
}


/*********************************************************************
 *
 *  Function:       RxDataFromNet2272
 *
 *  Description:    Receives data from NET2272 using DMA when possible
 *
 *********************************************************************/
static int RxDataFromNet2272(ADI_NET2272_DEVICE   *pDev,
                             u8_t                 *pu8Data,
                             u32_t                 ByteCount)
{
    int pL1_MSB = (int)pu8Data & L1_MSB_MASK;   /* mask */
    u32_t WordCount = ByteCount >> 1;           /* number of 16-bit words to */
                                                /* DMA; if there is an odd */
                                                /* number of bytes it will be */
                                                /* handled down below */
    int data_addr_is_odd = (int)pu8Data & 1;

    USB_CHECK(ByteCount > 0);

#if !USE_RX_DMA
    ReadEpDataWithPIO(pu8Data, ByteCount);
#else
    if (ByteCount < HW_MINIMUM_DMA_COUNT || data_addr_is_odd)
    {
        ReadEpDataWithPIO(pu8Data, ByteCount);
        return 1;
    }

    /* if the destination is L1 instruction memory */
#if defined(__ADSP_TETON__)
    if ((pL1_MSB == COREA_L1_INSTRUCTION_SRAM_BASE_ADR) ||
        (pL1_MSB == COREB_L1_INSTRUCTION_SRAM_BASE_ADR) ||
        (pL1_MSB == COREA_L1_DATA_BANK_A_SRAM_BASE_ADR) ||
        (pL1_MSB == COREA_L1_DATA_BANK_B_SRAM_BASE_ADR) ||
        (pL1_MSB == COREB_L1_DATA_BANK_A_SRAM_BASE_ADR) ||
        (pL1_MSB == COREB_L1_DATA_BANK_B_SRAM_BASE_ADR))
#else
    if (pL1_MSB == COREA_L1_INSTRUCTION_SRAM_BASE_ADR)
#endif
    {
        /* can't DMA directly from NET2272 to L1 instruction memory, we
         * first must DMA to another memory location, then to L1
         * instruction memory
         */

        /* 2-byte transfers since alignment is OK */
        doMemoryDMA(pDev, DATA_FIFO, (void*)tmpBuffer, WordCount, 2);

        /* 1-byte transfers in case of alignment problems */
        doMemoryDMA(pDev, (void*)tmpBuffer, (void*)pu8Data, ByteCount, 1);
    }
    else /* else destination is not L1 instruction memory */
    {
        /* if less than the minimum DMA count or if it's SCRATCHPAD
         * memory don't use DMA
         */
        if ((WordCount*2 < MINIMUM_DMA_COUNT) ||
            (pL1_MSB == COREA_L1_SCRATCH_PAD_SRAM_BASE_ADR))
        {
            ReadEpDataWithPIO(pu8Data, ByteCount);
            return 1;
        }
        else /* we can use DMA */
        {
            u8_t* startAddr = pu8Data;

            /* 2-byte DMA transfers */
            doMemoryDMA(pDev, DATA_FIFO, (void*)startAddr, WordCount, 2);
        }
    }

    /* if there were an odd number of bytes read the last one */
    if( ByteCount & 0x1 )
    {
        ReadEpDataWithPIO(pu8Data + ByteCount - 1, 1);
    }

#endif /* USE_RX_DMA */

    return 1;
}

/*********************************************************************
 *
 *  Function:       WriteEpDataWithPIO
 *
 *  Description:    Transfers data to NET2272 with programmed IO
 *
 *********************************************************************/
void WriteEpDataWithPIO(u8_t *pByte, u32_t ByteCount)
{
    int i;                  /* counter */
    unsigned short *pWord;  /* word ptr */
    u32_t WordCount;        /* number of 16-bit words to write */

    WordCount = ByteCount >> 1;
    pWord = (u16_t*)pByte;

    if ((int)pByte & 0x1)
    {
        /* data has odd alignment: construct a 16-bit word each time round */
        u8_t* p = pByte;

        for (i = 0; i < WordCount; i += 1)
        {
            unsigned short Word = (p[1] << 8) | p[0];
            p += 2;
            _outpw_d(EP_DATA_ADDR, Word);
        }
    }
    else
    {
        /* data alignment OK: write directly from memory */
        for (i = 0; i < WordCount; i += 1)
        {
            _outpw_d(EP_DATA_ADDR, *(pWord++));
        }
    }
    ssync();

    /* if there was an odd number of bytes, handle the last one */
    if( ByteCount & 0x1 )
    {
        /* change LOCCTL, write 1 byte, restore LOCCTL */
        unsigned char ucLocControl = _inpb_ind(LOCCTL);
        u8_t LastByte = *(pByte + ByteCount - 1);
        _outpb_ind(LOCCTL, (ucLocControl & ~DATA_WIDTH_16));
        _outpb_d(EP_DATA_ADDR, LastByte);
        _outpb_ind(LOCCTL, ucLocControl);
    }
}


/*********************************************************************
 *
 *  Function:       ReadEpDataWithPIO
 *
 *  Description:    Receives data from NET2272 with programmed IO
 *
 *********************************************************************/
void ReadEpDataWithPIO(u8_t *pByte, u32_t ByteCount)
{
    int i;                  /* counter */
    unsigned short *pWord;  /* word ptr */
    u32_t WordCount;        /* number of 16-bit words to write */

    pWord = (u16_t*)pByte;
    WordCount = ByteCount >> 1;

    if ((int)pByte & 0x1)
    {
        /* data has odd alignment: split each 16-bit word each time round */
        u8_t* p = pByte;

        for (i = 0; i < WordCount; i += 1)
        {
            unsigned short Word = _inpw_d(EP_DATA_ADDR);

            p[0] = Word & 0xFF;
            p[1] = (Word >> 8) & 0xFF;
            p += 2;
        }
    }
    else
    {
        /* data alignment OK: read directly to memory */
        for (i = 0; i < WordCount; i += 1)
        {
            *(pWord++) = _inpw_d(EP_DATA_ADDR);
        }
    }
    ssync();

    /* if there was an odd number of bytes, handle the last one */
    if( ByteCount & 0x1 )
    {
        /* change LOCCTL, read 1 byte, restore LOCCTL */
        unsigned char ucLocControl = _inpb_ind(LOCCTL);
        u8_t *pLastByte = pByte + ByteCount - 1;
        _outpb_ind(LOCCTL, (ucLocControl & ~DATA_WIDTH_16));
        *pLastByte = _inpb_d(EP_DATA_ADDR);
        _outpb_ind(LOCCTL, ucLocControl);
    }
}


/*********************************************************************
 *
 *  Function:       pddOpen
 *
 *  Description:    Opens the driver and does initialization.
 *
 *********************************************************************/

static u32 adi_pdd_Open(
        ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* device manager handle */
        u32                     DeviceNumber,       /* device number */
        ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* device handle */
        ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle */
                                                    /* location */
        ADI_DEV_DIRECTION       Direction,          /* data direction */
        void                    *pCriticalRegionArg,/* imask storage location */
        ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA */
                                                    /* manager */
        ADI_DCB_HANDLE          DCBHandle,          /* callback handle */
        ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback*/
                                                    /* function */
        )
{
    u32 Result= ADI_DEV_RESULT_DEVICE_IN_USE;
    ADI_NET2272_DEVICE *pDev;
    void *pExitCriticalArg;
    bool returnFlag = false;
    short chipRev;


#ifdef ADI_NET2272_DEBUG
    if (DeviceNumber > 0)
        return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    if (Direction != ADI_DEV_DIRECTION_BIDIRECTIONAL)
        return ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED;
#endif /* ADI_NET2272_DEBUG */

    pExitCriticalArg = adi_int_EnterCriticalRegion(pCriticalRegionArg);
    pDev = &Net2272Device;

    switch (pDev->State)
    {
        case DEV_STATE_RESET:
            pDev->State = DEV_STATE_OPEN_IN_PROCESS;
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
        default:
            break;
    }

    adi_int_ExitCriticalRegion(pExitCriticalArg);

    if (returnFlag)
        return Result;

    Result = ADI_DEV_RESULT_SUCCESS;

    /* we come here only if we need to configure the device */

    pDev->DeviceHandle = DeviceHandle;
    pDev->DMAHandle    = DMAHandle;
    pDev->DCBHandle    = DCBHandle;
    pDev->DMCallback   = DMCallback;
    pDev->Direction    = Direction;
    pDev->Started      = false;

    pDev->pCriticalRegionArg = pCriticalRegionArg;

    /* by default cache is turned off */
    pDev->Cache = false;

    pDev->Speed = ADI_USB_DEVICE_SPEED_UNKNOWN;
    pDev->NumPhysicalEndpoints = NUM_ENDPOINTS;
    pDev->BufferPrefix = 0;
    pDev->UseDmaSemaphore = false;
    pDev->EP0DataExpected = false;
    pDev->SetAddressExpected = false;

    /* create a device object for use by the usb core */
    pDev->DeviceID = adi_usb_CreateDevice(&pDev->pDeviceObj);
    if(pDev->DeviceID == -1)
        return ADI_DEV_RESULT_FAILED;

    /* inform core we are a USB device rather than host */
    if (adi_usb_SetDeviceMode(MODE_DEVICE) != ADI_USB_RESULT_SUCCESS)
        return ADI_DEV_RESULT_FAILED;

    /* inform core about the number of physical endpoints and their */
    /* data buffer sizes in the NET2272                             */
    adi_usb_SetPhysicalEndPointInfo(pDev->NumPhysicalEndpoints,
                                    PhysicalEpAdditionalInfo);

    /* configure the default parameters */
    SetProcessorSpecificDefaultConfiguration(pDev);

    /* setup with the selected configuration */
    /* we do this here because the BF561 EZ-KIT requires that PF12 is low 
     * for communication with the NET2272.  Flags are also configurable
     * through command ADI_USB_CMD_SET_PF, but you need to open the driver
     * first to be able to do that.  If your hardware target is different you
     * may or may not want to remove this call depending on if your flags must
     * be setup prior to communicating with the NET2272.  You can also use the
     * ADI_USB_CMD_SET_PF command to configure your flags, and allow the flags
     * to be reconfigured when we enable USB as the last step.
     */
    InitProcessorSpecificConfiguration(pDev);

    /* we only support bidirectional traffic for this device */
    if ( pDev->Direction != ADI_DEV_DIRECTION_BIDIRECTIONAL )
        return ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED;

    ResetNet2272(pDev);
    if (CheckNet2272(SCRATCH_ADDR) < 0)
        return ADI_DEV_RESULT_FAILED;

    /* all is well, assign the handle to our device */
    *pPDDHandle = (ADI_DEV_PDD_HANDLE)pDev;

    /* set state to opened */
    pExitCriticalArg = adi_int_EnterCriticalRegion(pCriticalRegionArg);
    pDev->State = DEV_STATE_OPENED;
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    /* return result */
    return Result;
}



/*********************************************************************
 *
 *  Function:       pddClose
 *
 *  Description:    Closes the driver and releases any memory
 *
 *********************************************************************/

static u32 adi_pdd_Close(
        ADI_DEV_PDD_HANDLE PDDHandle    /* PDD handle */
        )
{
    /* TODO: detach device object, reset the state variables */
    return ADI_DEV_RESULT_SUCCESS;
}

/*********************************************************************
 *
 *  Function:       pddRead
 *
 *  Description:    Accepts list of read buffers and enables processing
 *                  of OUT tokens (if not already enabled).
 *
 *********************************************************************/
static u32 adi_pdd_Read(
        ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
        ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
        ADI_DEV_BUFFER *pBuffer         /* pointer to buffer list */
        )
{
    unsigned char ucEpNum;
    ADI_NET2272_DEVICE *pDev = &Net2272Device;
    PUSB_EP_INFO pEpInfo;
    ADI_DEV_1D_BUFFER *pNextBuffer;
    int ep;
    bool bReenableUSB;

    /* disable interrupts from NET2272 USB controller's IVG */
    bReenableUSB = disableIVG(pDev->PeripheralIVG);

    pNextBuffer = (ADI_DEV_1D_BUFFER*)pBuffer;
    while (pNextBuffer != NULL) {
        bool bEnableInterrupts = true;
        ADI_DEV_1D_BUFFER* pCurrentBuffer = pNextBuffer;

        pNextBuffer = pCurrentBuffer->pNext;
        pCurrentBuffer->pNext = NULL;

        /* get the EP address */
        ucEpNum = pCurrentBuffer->Reserved[BUFFER_RSVD_EP_ADDRESS] & 0xff;

        USB_CHECK(ucEpNum < NUM_ENDPOINTS);

        /* for now the read will be used to setup the transfer size for this
         * endpoint
         */
        for (ep = 0; ep < NUM_ENDPOINTS; ep++)
        {
            /* initialize EP transfer info */
            if (ucEpNum == pDev->PhysicalEndpointObjects[ep].PhysicalEndpointID)
            {
                pEpInfo =
                    &pDev->PhysicalEndpointObjects[ep].pLogicalEndpointObj->
                        EPInfo;

                USB_CHECK(pEpInfo != NULL);

                /* queue the read buffer */
                enqueueBuffer((ADI_DEV_BUFFER*)pCurrentBuffer,
                              pEpInfo,
                              FREE_RCV_QUEUE);

                if (pEpInfo->pTransferBuffer == NULL)
                {
                    /* No read currently in progress - start a new one */
                    setupTransfer(pEpInfo, FREE_RCV_QUEUE);
                    pCurrentBuffer =
                        (ADI_DEV_1D_BUFFER*)pEpInfo->pTransferBuffer;

                    USB_CHECK(pCurrentBuffer != NULL);
                }
                else
                {
                    /* let the current read progress normally */
                    bEnableInterrupts = false;
                }

                break;
            }
        }

        /* ensure supplied endpoint identifier was valid */
        if (ep == NUM_ENDPOINTS)
            return ADI_DEV_RESULT_FAILED;

        /* if we are scheduling new read, enable interrupt for the endpoint */
        if (bEnableInterrupts && ep != 0)
        {
            /* select EP */
            _outpb_d(PAGESEL_ADDR, ep);

            /* stop NAKing OUT tokens */
            _outpb_d(EP_RSPCLR_ADDR, EP_ALT_NAK_OUT_PACKETS);

            /* enable data packet received interrupt */
            _outpb_d(EP_IRQENB_ADDR, EP_DATA_RCVD_INT);
        }
    }

    /* re-enable USB interrupts if previously disabled */
    if (bReenableUSB)
        reenableIVG(pDev->PeripheralIVG);
    
    return ADI_DEV_RESULT_SUCCESS;
}

/*********************************************************************
 *
 *  Function:       pddWrite
 *
 *  Description:    Accepts list of write buffers and enables processing
 *                  of IN tokens (if not already enabled).
 *
 *********************************************************************/
static u32 adi_pdd_Write(
        ADI_DEV_PDD_HANDLE PDDHandle,       /* PDD handle */
        ADI_DEV_BUFFER_TYPE BufferType,     /* buffer type */
        ADI_DEV_BUFFER *pBuffer             /* pointer to buffer list */
        )
{
    unsigned char ucEpNum;
    ADI_NET2272_DEVICE *pDev = &Net2272Device;
    PUSB_EP_INFO pEpInfo;
    int ep;
    unsigned int bytes_avail, byte_count;
    bool bReenableUSB;
    ADI_DEV_1D_BUFFER *pNextBuffer;
    u32 result = ADI_DEV_RESULT_SUCCESS;

    /* disable interrupts from NET2272 USB controller's IVG */
    bReenableUSB = disableIVG(pDev->PeripheralIVG);

    pNextBuffer = (ADI_DEV_1D_BUFFER*)pBuffer;

    /* handle EP 0 specially */
    ucEpNum = pNextBuffer->Reserved[BUFFER_RSVD_EP_ADDRESS] & 0xff;
    if (ucEpNum == EP0) {
        if (!TransmitBufferEp0(pNextBuffer)) {
	    result = ADI_DEV_RESULT_FAILED;
        } else {
            pEpInfo = &pDev->PhysicalEndpointObjects[EP0].pLogicalEndpointObj->
                        EPInfo;
            USB_CHECK(pEpInfo != NULL);

            pEpInfo->EpCallback(pDev->DeviceHandle,
                                ADI_USB_EVENT_TX_COMPLETE,
                                pNextBuffer->CallbackParameter);
        }

        /* re-enable USB interrupts if previously disabled */
        if (bReenableUSB)
            reenableIVG(pDev->PeripheralIVG);
        return result;
    }

    /* handle normal data endpoints */
    while (pNextBuffer != NULL) {
        bool start_transfer = true;
        ADI_DEV_1D_BUFFER* pCurrentBuffer = pNextBuffer;

        pNextBuffer = pCurrentBuffer->pNext;
        pCurrentBuffer->pNext = NULL;

        /* get the EP address */
        ucEpNum = pCurrentBuffer->Reserved[BUFFER_RSVD_EP_ADDRESS] & 0xff;

        USB_CHECK(ucEpNum < NUM_ENDPOINTS);

        for (ep = 1; ep < NUM_ENDPOINTS; ep += 1 )
        {
            /* initialize EP transfer info */
            if (ucEpNum ==
                    pDev->PhysicalEndpointObjects[ep].PhysicalEndpointID)
            {
                pEpInfo =
                    &pDev->PhysicalEndpointObjects[ep].pLogicalEndpointObj->
                    EPInfo;
                USB_CHECK(pEpInfo != NULL);

                /* first queue the outgoing packet */
                enqueueBuffer((ADI_DEV_BUFFER*)pCurrentBuffer,
                              pEpInfo,
                              WAIT_XMT_QUEUE);

                /* if there is no pending xmt then try to send it */
                if (pEpInfo->pTransferBuffer == NULL)
                {
                    setupTransfer(pEpInfo, WAIT_XMT_QUEUE);
                    pCurrentBuffer =
                        (ADI_DEV_1D_BUFFER*)pEpInfo->pTransferBuffer;
                    USB_CHECK(pCurrentBuffer != NULL);
                }
                else /* let the current xmt proceed and do this one later */
                {
                    start_transfer = false;
                }
                break;
            }
        }

        /* ensure supplied endpoint identifier was valid */
        if (ep == NUM_ENDPOINTS)
            return ADI_DEV_RESULT_FAILED;

        if (!start_transfer)
            continue;   /* move to next supplied buffer (if any) */

        /* select EP */
        _outpb_d(PAGESEL_ADDR, ep);

        /* load the buffer with as much data as we can immediately so when
         * the IN TOKEN arrives we're ready to send
         */

        /* find out how much room is available */
        bytes_avail = EP_BYTES_AVAIL;

        if (bytes_avail > 0)
        {
            /* if there's not enough room for all the data only transfer
             * what will fit
             */
            if (bytes_avail < pEpInfo->TransferSize)
                byte_count = bytes_avail;
            else
                byte_count = pEpInfo->TransferSize;

            /* transfer data to NET2272 */
            TxDataToNet2272(pDev, (u8_t*)pCurrentBuffer->Data, byte_count);

            /* update processed byte count */
            pEpInfo->EpBytesProcessed += byte_count;
        }

        /* clear IN TOKEN, next one will interrupt us and be handled by the
         * handler
         */
        _outpb_d(EP_STAT0_ADDR, EP_DATA_IN_TOKEN_INT);

        /* enable IN TOKEN interrupt */
        _outpb_d(EP_IRQENB_ADDR, EP_DATA_IN_TOKEN_INT);
    }

    /* re-enable USB interrupts if previously disabled */
    if (bReenableUSB)
        reenableIVG(pDev->PeripheralIVG);
    
    return result;
}


/*********************************************************************
 *
 *  Function:       pddControl
 *
 *  Description:    List of I/O control commands to the driver
 *
 *********************************************************************/
static u32 adi_pdd_Control(
        ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
        u32 Command,                    /* command ID */
        void *pArg                      /* pointer to argument */
        )
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    ADI_NET2272_DEVICE *pDev = &Net2272Device;
    u32  *pArgument;
    int i = 0;
    volatile int x;
    ENDPOINT_OBJECT *pEp = NULL;
    u32 imask;
    char *msg;

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

        case ADI_USB_CMD_GET_DEVICE_ID:
            (*(u32*)pArg) = pDev->DeviceID;
            break;

        case ADI_USB_CMD_ENABLE_USB:
            Result = UsbStartNet2272(pDev);
            break;

        case ADI_USB_CMD_DISABLE_USB:
            /* TODO: implement disable */
            /*Result = Net2272_DisableUSB(pDev);*/
            break;

        case ADI_USB_CMD_BUFFERS_IN_CACHE:
            /* update 'worry about data cache' flag in device structure */
            pDev->Cache = (bool)pArg;
            break;

        case ADI_USB_CMD_SET_STALL_EP:
        case ADI_USB_CMD_CLEAR_STALL_EP:
            pEp = (ENDPOINT_OBJECT*)pArg;
            /* search for the logical EP */
            for (i = 0; i < NUM_ENDPOINTS; i++)
            {
                /* if we find the logical endpoint perform the set/clear halt */
                if (pEp == pDev->PhysicalEndpointObjects[i].pLogicalEndpointObj)
                {
                    _outpb_d(PAGESEL_ADDR, i);
                    if (ADI_USB_CMD_SET_STALL_EP == Command)
                    {
                        _outpb_d(EP_RSPSET_ADDR, EP_HALT);  /* set halt */
                        for (x=0; x < 0xffff; x++);
                        break;
                    }
                    else
                    {
                        _outpb_d(EP_RSPCLR_ADDR, EP_HALT);  /* clear halt */
                        for (x=0; x < 0xffff; x++);
                        break;
                    }
                }
            }

            break;

        case ADI_USB_CMD_SET_PF:
            pArgument = (u32*)pArg;
            pDev->PF_Reset = *pArgument;
            pDev->PF_Int = *(pArgument+1);
            pDev->PF_Clear = *(pArgument+2);
            pDev->PF_Set = *(pArgument+3);
            break;

        case ADI_USB_CMD_SET_IVG:
            pDev->PeripheralIVG = (u32)pArg;
            break;

        case ADI_USB_CMD_SET_DMA_CHANNEL:
            pDev->DmaStreamID = (ADI_DMA_STREAM_ID)pArg;
            break;

        case ADI_USB_CMD_SET_DMA_IVG:
            pDev->DmaStreamIVG = (u32)pArg;
            break;

        case ADI_USB_CMD_GET_PF:
            pArgument = (u32*)pArg;
            *pArgument = pDev->PF_Reset;
            *(pArgument+1) = pDev->PF_Int;
            *(pArgument+2) = pDev->PF_Clear;
            *(pArgument+3) = pDev->PF_Set;
            break;

        case ADI_USB_CMD_GET_IVG:
            pArgument = (u32*)pArg;
            *pArgument = pDev->PeripheralIVG;
            break;

        case ADI_USB_CMD_GET_DMA_CHANNEL:
            pArgument = (u32*)pArg;
            *pArgument = pDev->DmaStreamID;
            break;

        case ADI_USB_CMD_GET_DMA_IVG:
            pArgument = (u32*)pArg;
            *pArgument = pDev->DmaStreamIVG;
            break;

        /* Used to reserve extra memory for the
         * peripheral driver to use per buffer */
        case ADI_USB_CMD_GET_BUFFER_PREFIX:
            pArgument = (u32*)pArg;
            *pArgument = pDev->BufferPrefix;
            break;

        case ADI_USB_CMD_SET_BUFFER_PREFIX:
            pDev->BufferPrefix = (int)pArg;
            break;

        case ADI_USB_CMD_SETUP_RESPONSE:
            /* 
             * respond as requested to an EP0 Control Read or Control Write  
             */

            switch ((u32)pArg)
            {
                case ADI_USB_SETUP_RESPONSE_STALL:
                    /* USB Core can't deal with the Control Transfer - stall */

                    /* ensure ISR doesn't trounce us */
                    imask = cli();

                    /* select EP0 */
                    _outpb_d(PAGESEL_ADDR, EP0);
                    /* specify STALL for response */
                    _outpb_d(EP_RSPSET_ADDR, EP_HALT);
                    /* enable response */
                    _outpb_d(EP_RSPCLR_ADDR, EP_CONTROL_STATUS_PHASE_HS);
                    
                    pDev->EP0DataExpected = false;

                    /* allow ISR access again */
                    sti(imask);
                    break;

                case ADI_USB_SETUP_RESPONSE_ACK:
                    if (pDev->EP0DataExpected || pDev->SetAddressExpected)
                    {
                        /* (1) delay SETUP response if we are expecting
                         * data packets
                         * (2) delay SET_ADDRESS SETUP response until
                         * the STATUS_HANDSHAKE call (below) so that the new
                         * address will already have been set in OURADDRESS
                         */
                        break;
                    }
                    /* FALLTHROUGH */
                case ADI_USB_SETUP_RESPONSE_ACK_DATA_END:
                    /* ensure ISR doesn't trounce us */
                    imask = cli();

                    /* select EP0 */
                    _outpb_d(PAGESEL_ADDR, EP0);
                    /* enable status phase */
                    _outpb_d(EP_RSPCLR_ADDR, EP_CONTROL_STATUS_PHASE_HS);

                    /* disable further DATA received interrupts */
                    _outpb_d(EP_IRQENB_ADDR,
                            _inpb_d(EP_IRQENB_ADDR) & ~EP_DATA_RCVD_INT);
                    _outpb_d(EP_STAT0_ADDR,
                            (EP_DATA_RCVD_INT | EP_DATA_OUT_TOKEN_INT));

                    pDev->EP0DataExpected = false;

                    /* allow ISR access again */
                    sti(imask);
                    break;

                default:
                    Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
            }

            break;

        case ADI_USB_CMD_ENABLE_CNTRL_STATUS_HANDSHAKE:
            /* only requires action if SET ADDRESS status has been delayed
             */
            if (pDev->SetAddressExpected)
            {
                /* ensure ISR doesn't trounce us */
                imask = cli();

                /* select EP0 */
                _outpb_d(PAGESEL_ADDR, EP0);
                /* enable status phase */
                _outpb_d(EP_RSPCLR_ADDR, EP_CONTROL_STATUS_PHASE_HS);

                pDev->SetAddressExpected = false;

                /* allow ISR access again */
                sti(imask);
            }
            break;

        case ADI_USB_CMD_SET_DEV_ADDRESS:
            _outpb_ind(OURADDRESS,(char)((int)pArg & 0x7f));
            break;

        case ADI_USB_CMD_UPDATE_ACTIVE_EP_LIST:
            BindPhysicalEndpoints((ENDPOINT_OBJECT*)pArg);
            break;

        case ADI_USB_CMD_SET_DEV_MODE:
            /* We support only one mode i.e MODE_DEVICE */
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            break;

        case ADI_USB_CMD_GET_DEV_MODE:
            pArgument = (u32*)pArg;
            *pArgument = MODE_DEVICE;
            break;

        /*
         * Returns the currently operating device mode.
         */
        case ADI_USB_CMD_GET_DEV_SPEED:
            /* get the configured speed */
            pDev->Speed = USB_BUS_SPEED;
            pArgument  = (u32*)pArg;
            *pArgument = pDev->Speed;
            break;

        /*
         * Forces to operate the device at a speed.
         */
        case ADI_USB_CMD_SET_DEV_SPEED:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            break;

        /*
         * Set/Get SEM handle to support asynchronous memory DMA.
         */
        case ADI_USB_CMD_SET_DRIVER_SEM_HANDLE:
            pDev->DmaSemaphore = (ADI_SEM_HANDLE)pArg;
            pDev->UseDmaSemaphore = TRUE;
            break;

        case ADI_USB_CMD_GET_DRIVER_SEM_HANDLE:
            pArgument = (u32*)pArg;
            *pArgument = (u32)pDev->DmaSemaphore;
            break;

        case ADI_USB_CMD_ENTER_TEST_MODE:
            {
                static char TestPacket[] = {
                    /* 53 byte USB test packet (see USB 2.0 spec 7.1.20) */
                    /* JKJKJKJK * 9         */
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    /* JJKKJJKK * 8         */
                    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                    /* JJJJKKKK * 8         */
                    0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
                    /* JJJJJJJKKKKKKK * 8   */
                    0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF, 0xFF, 0xFF, 
                    /* JJJJJJJK * 8         */
                    0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD,
                    /* {JKKKKKKK * 10}, JK  */
                    0xFC, 0x7E, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0x7E
                };

                int i;
                volatile int j;
                u32_t Index;
                s16_t  *pwData = (s16_t*)TestPacket;

                /* determine which test it is */
                Index = (u32)pArg >> 8;

                /* disable interrupts, the only way to exit a test mode is to
                 * cycle power
                 */
                _outpb_ind(IRQENB0, 0x00);
                _outpb_ind(IRQENB1, 0x00);

                /* set the EP number and force high speed */
                _outpb_d(PAGESEL_ADDR, EP0);
                _outpb_ind(XCVRDIAG, 0x08);

                /* clean it up */
                _outpb_d(EP_STAT0_ADDR, EP_DATA_XMTD_INT);
                _outpb_d(EP_RSPCLR_ADDR, (EP_CONTROL_STATUS_PHASE_HS |
                                          EP_HIDE_STATUS_PHASE));
                _outpb_ind(EP_CFG, 0x10);
                _outpb_d(EP_STAT1_ADDR, EP_BUFFER_FLUSH);

                /* delay a little */
                j = 600;
                while (j--)
                    ;

                /* write the test value to USBTEST */
                _outpb_ind(USBTEST, Index);

                /* if TEST_PACKET test */
                if (0x4 == Index)
                {
                    /* write in byte mode */
                    volatile unsigned char ucLocControl = 0;
                    ucLocControl = _inpb_ind(LOCCTL);
                    _outpb_ind(LOCCTL, ( ucLocControl & 0xfe ));

                    /* fill EP0 with test packet, 8 bits at a time */
                    for (i = 0 ; i < 53; i++)
                        _outpb_d(EP_DATA_ADDR, TestPacket[i]);
                    ssync();

                    /* validate the transfer */
                    _outpb_d(EP_TRANSFER0_ADDR, 0x00);
                }

                /* we do not want to return since the only way out of test
                 * mode for a device is to be power-cycled
                 */
                while (true)
                    ;
            }
            /* break;  unreachable */

            /*
             * unknown command, return result that we don't support this
             * command
             */
        default:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            break;
    }
    return Result;
}

/**************************************************************************
 *
 * NET2272 entry point
 *
 **************************************************************************/
ADI_DEV_PDD_ENTRY_POINT ADI_USB_NET2272_Entrypoint =
{
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

static int SetProcessorSpecificDefaultConfiguration(ADI_NET2272_DEVICE *pDev)
{
    /* default settings, one should be able to change through
     * device controls before issuing USB_CMD_START
    */

#if defined(__ADSP_BRODIE__)
    pDev->DmaStreamID   = ADI_DMA_MDMA_0;
    pDev->PeripheralID  = ADI_INT_PORTH_INTA;
    pDev->PF_Reset      = PH5;
    pDev->PF_Int        = PH3;
    pDev->PF_Clear      = 0;
    pDev->PF_Set        = 0;
    pDev->PeripheralIVG = 0;
    pDev->DmaStreamIVG  = 0;
#elif defined(__ADSP_EDINBURGH__)
    pDev->DmaStreamID   = ADI_DMA_MDMA_0;
    pDev->PeripheralID  = ADI_INT_PFA;
    pDev->PF_Reset      = PF11;
    pDev->PF_Int        = PF10;
    pDev->PF_Clear      = 0;
    pDev->PF_Set        = 0;
    pDev->PeripheralIVG = 0;
    pDev->DmaStreamIVG  = 0;
#elif defined(__ADSP_BRAEMAR__)
    pDev->DmaStreamID   = ADI_DMA_MDMA_0;
    pDev->PeripheralID  = ADI_INT_PORTFG_A;
    pDev->PF_Reset      = PF6;
    pDev->PF_Int        = PF7;
    pDev->PF_Clear      = 0;
    pDev->PF_Set        = 0;
    pDev->PeripheralIVG = 0;
    pDev->DmaStreamIVG  = 0;
#elif defined(__ADSP_TETON__)
    pDev->DmaStreamID   = ADI_DMA_MDMA2_0;
    pDev->PeripheralID  = ADI_INT_PF0_15_A;
    pDev->PF_Reset      = PF11;
    pDev->PF_Int        = PF10;
    pDev->PF_Clear      = PF12;  /* PF12 used by USB EZ-EXTENDER to indicate */
                                 /* we are connected to BF561 EZ-KIT */
    pDev->PF_Set        = 0;
    pDev->PeripheralIVG = 0;
    pDev->DmaStreamIVG  = 0;
#else
#error *** Processor not supported ***
#endif
    return 0;
}

static int InitProcessorSpecificConfiguration(ADI_NET2272_DEVICE *pDev)
{
    unsigned short usValue;
    volatile unsigned int v;

#if defined(__ADSP_BRODIE__)
    /* configure interrupt flag as level sensitive input from NET2272 */
    usValue = *pPORTHIO_DIR;
    *pPORTHIO_DIR = usValue & ~(pDev->PF_Int);

     usValue = *pPORTHIO_INEN;
    *pPORTHIO_INEN = usValue | pDev->PF_Int;
        
    usValue = *pPORTHIO_EDGE;
    *pPORTHIO_EDGE = usValue & ~(pDev->PF_Int);
    usValue = *pPORTHIO_MASKA;
    *pPORTHIO_MASKA = usValue | pDev->PF_Int;
    usValue = *pPORTHIO_POLAR;
    *pPORTHIO_POLAR = usValue | pDev->PF_Int;
    
    /* check which async memory bank NET2272 is mapped to */
#if   USB_BASE_ADDR == 0x20300000
    /* asynch bank 3: ~AMS3 is multiplexed with PG15 (and other functions) */
    usValue = *pPORTG_MUX;
    /* select ~AMS3 function in its multiplex */
    *pPORTG_MUX = ((usValue & 0x3FFF) | 0x8000);
    /* enable ~AMS3 function by disabling PG15 */
    *pPORTG_FER |= PG15; 
    delay(1);
#elif USB_BASE_ADDR == 0x20200000
    /* asynch bank 2: ~AMS2 is multiplexed with PG11 (and other functions) */
    usValue = *pPORTG_MUX;
    /* select ~AMS2 function in its multiplex */
    *pPORTG_MUX = ((usValue & 0xCFFF) | 0x1000);
    /* enable ~AMS2 function by disabling PG11 */
    *pPORTG_FER |= PG11; 
    delay(1);
#else
    /* ~AMS1 and ~AMS0 are not multiplexed */
#endif

#elif defined(__ADSP_EDINBURGH__)
    /* configure interrupt flag as level sensitive input from NET2272 */
    usValue = *pFIO_INEN;
    *pFIO_INEN = usValue | pDev->PF_Int;
    usValue = *pFIO_DIR;
    *pFIO_DIR = usValue & ~(pDev->PF_Int);
    usValue = *pFIO_EDGE;
    *pFIO_EDGE = usValue & ~(pDev->PF_Int);
    usValue = *pFIO_MASKA_D;
    *pFIO_MASKA_D = usValue | pDev->PF_Int;
    usValue = *pFIO_POLAR;
    *pFIO_POLAR = usValue | pDev->PF_Int;

    /* configure the SET and CLEAR flags as outputs and perform SET or CLEAR */
    usValue = *pFIO_DIR;
    *pFIO_DIR = usValue | pDev->PF_Set | pDev->PF_Clear;
    *pFIO_FLAG_C = pDev->PF_Clear;
    *pFIO_FLAG_S = pDev->PF_Set;

#elif defined(__ADSP_BRAEMAR__)
    /* configure interrupt flag as level sensitive input from NET2272 */
    usValue = *pPORTFIO_INEN;
    *pPORTFIO_INEN = usValue | pDev->PF_Int;
    usValue = *pPORTFIO_DIR;
    *pPORTFIO_DIR = usValue & ~(pDev->PF_Int);
    usValue = *pPORTFIO_EDGE;
    *pPORTFIO_EDGE = usValue & ~(pDev->PF_Int);
    usValue = *pPORTFIO_MASKA;
    *pPORTFIO_MASKA = usValue | pDev->PF_Int;
    usValue = *pPORTFIO_POLAR;
    *pPORTFIO_POLAR = usValue | pDev->PF_Int;

    /* configure the SET and CLEAR flags as outputs and perform SET or CLEAR */
    usValue = *pPORTFIO_DIR;
    *pPORTFIO_DIR = usValue | pDev->PF_Set | pDev->PF_Clear;
    *pPORTFIO_CLEAR = pDev->PF_Clear;
    *pPORTFIO_SET = pDev->PF_Set;

#elif defined(__ADSP_TETON__)
    /* configure interrupt flag as level sensitive input from NET2272 */
    usValue = *pFIO0_INEN;
    *pFIO0_INEN = usValue | pDev->PF_Int;
    usValue = *pFIO0_DIR;
    *pFIO0_DIR = usValue & ~(pDev->PF_Int);
    usValue = *pFIO0_EDGE;
    *pFIO0_EDGE = usValue & ~(pDev->PF_Int);
    usValue = *pFIO0_MASKA_D;
    *pFIO0_MASKA_D = usValue | pDev->PF_Int;
    usValue = *pFIO0_POLAR;
    *pFIO0_POLAR = usValue | pDev->PF_Int;

    /* configure the SET and CLEAR flags as outputs and perform SET or CLEAR */
    usValue = *pFIO0_DIR;
    *pFIO0_DIR = usValue | pDev->PF_Set | pDev->PF_Clear;
    *pFIO0_FLAG_C = pDev->PF_Clear;
    *pFIO0_FLAG_S = pDev->PF_Set;

#else
#error *** Processor not supported ***
#endif

    /* delay for a bit */
    for (v = 0; v < 0xfffff; v++)
        ;

    return 0;
}
