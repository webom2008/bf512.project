/**********************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software 
you agree to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_eppi.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
    This is the driver source code for the EPPI.
            
***********************************************************************/

/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>      /* system service includes          */
#include <drivers/adi_dev.h>        /* device manager includes          */
#include <drivers/eppi/adi_eppi.h>  /* EPPI driver includes             */

#if !defined(__ADSP_MOAB__)  /* EPPI is available only for Moab class devices*/
#error "EPPI is available only on ADSP-BF542,ADSP-BF544,ADSP-BF548 & ADSP-BF549"
#endif

/*********************************************************************

Enumerations and defines

*********************************************************************/

/* number of EPPIs (EPPI) in the system                                 */
#define ADI_EPPI_NUM_DEVICES            (sizeof(Device)/sizeof(ADI_EPPI))

/*********************************************************************

Macros to manipulate selected bits/fields in EPPI MMRs

*********************************************************************/
/* Macros to manipulate bits/fields of EPPI control register                                */
/* Set PORT_EN (EPPI Port Enable)                                                           */
#define ADI_EPPI_CTL_SET_PORT_EN()      (pDevice->pRegisters->EPPI_CONTROL  |= 0x0001)
/* Clear PORT_EN (EPPI Port Disable)                                                        */
#define ADI_EPPI_CTL_CLEAR_PORT_EN()    (pDevice->pRegisters->EPPI_CONTROL  &= 0x3FFE)

/* Get DLEN (Data Length)                                                                   */
#define ADI_EPPI_CONTROL_GET_DLEN(pDev) ((pDev->pRegisters->EPPI_CONTROL & 0x00038000) >> 15)
/* Get PORT_DIR (EPPI Port Direction)                                                       */
#define ADI_EPPI_CONTROL_GET_PORT_DIR() ((pDevice->pRegisters->EPPI_CONTROL & 0x02))
/* Get XFR_TYPE (is Transfer Type = ITU656 Entire Field?)                                   */
#define ADI_EPPI_CONTROL_GET_XFR_TYPE() ((pDevice->pRegisters->EPPI_CONTROL & 0x04))
/* Get ITU_TYPE (ITU Type)                                                                  */
#define ADI_EPPI_CONTROL_GET_ITU_TYPE() ((pDevice->pRegisters->EPPI_CONTROL & 0x40))
/* Get DMACFG (DMA Configuration Status)                                                    */
#define ADI_EPPI_CONTROL_GET_DMACFG(pDev)   ((pDev->pRegisters->EPPI_CONTROL & 0x02000000))

/* Macros to manipulate bits/fields of EPPI status register                                 */
/* Get Field Received status (FLD bit value)                                                */
#define ADI_EPPI_STATUS_GET_FLD()           ((pDevice->pRegisters->EPPI_STATUS & 0x8000) >> 15)
        
/*********************************************************************

Data Structures

*********************************************************************/

/*******************************
Moab  (BF542,BF544,BF548,BF549)
*******************************/

#if defined(__ADSP_MOAB__)          /* EPPI register structure for Moab class devices       */

typedef struct {                    /* structure representing EPPI registers in memory      */
    volatile u16    EPPI_STATUS;    /* Status register                                      */
    u16             Padding0;
    volatile u16    EPPI_HCOUNT;    /* Horizontal Transfer Count Register                   */
    u16             Padding1;
    volatile u16    EPPI_HDELAY;    /* Horizontal Delay Count Register                      */
    u16             Padding2;
    volatile u16    EPPI_VCOUNT;    /* Vertical Transfer Count Register                     */
    u16             Padding3;
    volatile u16    EPPI_VDELAY;    /* Vertical Delay Count Register                        */
    u16             Padding4;    
    volatile u16    EPPI_FRAME;     /* Lines Per Frame Register                             */
    u16             Padding5;
    volatile u16    EPPI_LINE;      /* Samples Per Line Register                            */
    u16             Padding6;
    volatile u16    EPPI_CLKDIV;    /* Clock Divide Register                                */
    u16             Padding7;
    volatile u32    EPPI_CONTROL;   /* Control Register                                     */
    volatile u32    EPPI_FS1W_HBL;  /* FS1 Width/Horizontal Blanking Samples per Line Reg   */
    volatile u32    EPPI_FS1P_AVPL; /* FS1 Period/Active Video Samples per Line Register    */
    volatile u32    EPPI_FS2W_LVB;  /* FS2 Width/Lines of Vertical Blanking Register        */
    volatile u32    EPPI_FS2P_LAVF; /* FS2 Period/Lines of Active Video per Field Register  */
    volatile u32    EPPI_CLIP;      /* Clipping Register                                    */
}ADI_EPPI_REGISTERS;

#endif                              /* End of Moab specific Data Structures                 */

typedef struct {                                /* EPPI device structure                    */
    ADI_EPPI_REGISTERS      *pRegisters;        /* Base address of EPPI registers           */
    void                    *pEnterCriticalArg; /* critical region argument                 */
    u8                      InUseFlag;          /* in use flag (in use when TRUE)           */
    u8                      DataflowFlag;       /* Dataflow flag (TRUE when dataflow is on) */
    u8                      DeviceNumber;       /* This device number                       */
    u8                      SharedDeviceNumber; /* Device Number that shares its
                                                   DMA channel with this device             */
    ADI_INT_PERIPHERAL_ID   ErrorPeripheralID;  /* peripheral ID of EPPI's error interrupt  */
    u8                      ErrorReportingFlag; /* EPPI error reporting flag                */
    ADI_DEV_DMA_INFO        *pEppiDma;          /* pointer to EPPI DMA channel info 
                                                   (common for Inbound & Outbound dataflow) */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;       /* Device manager handle                    */
    void (*DMCallback) (                        /* Device manager's Callback function       */
        ADI_DEV_DEVICE_HANDLE DeviceHandle,     /* device handle                            */
        u32 Event,                              /* event ID                                 */
        void *pArg);                            /* argument pointer                         */
} ADI_EPPI;

/*********************************************************************

Device specific data

*********************************************************************/

/********************
    Moab  
********************/

#if defined(__ADSP_MOAB__)                  /* settings for Moab class devices              */

static ADI_EPPI Device[] = {                /* Actual EPPI devices                          */
    {                                       /* Device 0 - EPPI0                             */
        (ADI_EPPI_REGISTERS *)(0xffc01000), /* EPPI0 register base address                  */
        NULL,                               /* critical region argument                     */
        FALSE,                              /* in use flag                                  */
        FALSE,                              /* Dataflow is off by default                   */
        0,                                  /* Device 0     								*/
        0,                                  /* This Device doesn't share or use shared DMA  */
        ADI_INT_EPPI0_ERROR,                /* Peripheral ID of EPPI0's Error interrupt  	*/
        FALSE,                              /* EPPI error reporting is disabled by default  */
        NULL,                               /* Pointer to EPPI0 DMA channel info            */
        NULL,                               /* Device manager handle                        */
        NULL,                               /* Device manager Callback function             */
    },
    {                                       /* Device 1 - EPPI1                             */
        (ADI_EPPI_REGISTERS *)(0xffc01300), /* EPPI1 register base address                  */
        NULL,                               /* critical region argument                     */
        FALSE,                              /* in use flag                                  */
        FALSE,                              /* Dataflow is off by default                   */
        1,                                  /* This is Device 1                             */
        2,                                  /* This Device share DMA channel(s) with EPPI2  */
        ADI_INT_EPPI1_ERROR,                /* Peripheral ID of EPPI1's Error interrupt  	*/
        FALSE,                              /* EPPI error reporting is disabled by default  */
        NULL,                               /* Pointer to EPPI1 DMA channel info            */
        NULL,                               /* Device manager handle                        */
        NULL,                               /* Device manager Callback function             */
    },
    {                                       /* Device 2 - EPPI2                             */
        (ADI_EPPI_REGISTERS *)(0xffc02900), /* EPPI2 register base address                  */
        NULL,                               /* critical region argument                     */
        FALSE,                              /* in use flag                                  */
        FALSE,                              /* Dataflow is off by default                   */
        2,                                  /* This is Device 2                             */
        1,                                  /* This Device share DMA channel(s) with EPPI1  */
        ADI_INT_EPPI2_ERROR,                /* Peripheral ID of EPPI2's Error interrupt  	*/
        FALSE,                              /* EPPI error reporting is disabled by default  */
        NULL,                               /* Pointer to EPPI2 DMA channel info            */
        NULL,                               /* Device manager handle                        */
        NULL,                               /* Device manager Callback function             */
    },
};

/* EPPI DMA Channel Information Tables                                                      */
/* Information table for EPPI Inbound/Outbound DMA channels                                 */
static ADI_DEV_DMA_INFO EppiDmaInfo[]  = {  /* Actual EPPI devices                          */
            {                               /* Device 0 DMA channel info                    */
                ADI_DMA_PMAP_EPPI0,       	/* EPPI0 DMA peripheral mapping ID          	*/
                NULL,                       /* DMA channel handle                           */
                FALSE,                      /* switch mode disabled by default              */
                NULL,                       /* pointer to switch queue head                 */
                NULL,                       /* pointer to switch queue tail                 */
                NULL,                       /* No further DMA channels to link              */
            },
            {                               /* Device 1 DMA channel info                    */
                ADI_DMA_PMAP_EPPI1,       	/* EPPI1 DMA peripheral mapping ID          	*/
                NULL,                       /* DMA channel handle                           */
                FALSE,                      /* switch mode disabled by default              */
                NULL,                       /* pointer to switch queue head                 */
                NULL,                       /* pointer to switch queue tail                 */
                NULL,                       /* No further DMA channels to link              */
            },
            {                               /* Device 2 DMA channel info                    */
                ADI_DMA_PMAP_EPPI2_HOST,  	/* EPPI2 DMA peripheral mapping ID          	*/
                NULL,                       /* DMA channel handle                           */
                FALSE,                      /* switch mode disabled by default              */
                NULL,                       /* pointer to switch queue head                 */
                NULL,                       /* pointer to switch queue tail                 */
                NULL,                       /* No further DMA channels to link              */
            },
};

/* Mask value for EPPI control register
   - start location of individual bit/field is set to 1, others to 0, reserved bits to 1    */
/* b#10101111111111001010111111010111 */
const static u32 EppiControlMask = 0xAFFCAFD7;

#endif                                      /* End of Moab specific data                    */

/*********************************************************************

Static functions

*********************************************************************/

static u32 adi_pdd_Open(                        /* Opens EPPI device                        */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle                    */
    u32                     DeviceNumber,       /* EPPI Device number to open               */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                            */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle location           */
    ADI_DEV_DIRECTION       Direction,          /* data direction                           */
    void                    *pEnterCriticalArg, /* critical region storage location         */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager                */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                          */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function         */
);

static u32 adi_pdd_Close(                       /* Closes a EPPI device                     */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the EPPI device to close   */
);

static u32 adi_pdd_Read(                        /* Reads data/queues inbound buffer to EPPI */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a EPPI device              */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);
    
static u32 adi_pdd_Write(                       /*Writes data/queues outbound buffer to EPPI*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a EPPI device              */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_SequentialIO(                /* Sequentially read/writes data to a device*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a EPPI device              */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_Control(                     /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a EPPI device              */
    u32                     Command,            /* Command ID                               */
    void                    *Value              /* Command specific value                   */
);

static u32 UpdateEppiDmaUsage(                  /* Updates EPPI DMA channel usage           */
    ADI_EPPI                *pDevice            /* pointer to the device we're working on   */
);

#if defined(__ADSP_MOAB__)                      /* Static functions for Moab class devices  */

static u32 EppiSetPortControl(                  /* configures the port control registers    */
    ADI_EPPI                *pDevice            /* pointer to the device we're working on   */
);

#endif                                          /* End of Moab specific Static functions    */

static ADI_INT_HANDLER(Eppi0IntHandler);        /* EPPI0 Error interrupt handler            */
static ADI_INT_HANDLER(Eppi1IntHandler);        /* EPPI1 Error interrupt handler            */
static ADI_INT_HANDLER(Eppi2IntHandler);        /* EPPI2 Error interrupt handler            */

static void ReportEppiError(                    /* Reports EPPI Error(s) to the client      */
    ADI_EPPI                *pDevice            /* pointer to the device we're working on   */
);

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

static u32 ValidatePDDHandle(                   /* Validates Physical Device Driver Handle  */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of a EPPI device              */
);

static u32 ValidateEppiUsage(                   /* Validates EPPI hardware status           */
    ADI_EPPI                *pDevice            /* pointer to the device we're working on   */
);

#endif

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADIEPPIEntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,    
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens a EPPI device for use
*
*********************************************************************/

static u32 adi_pdd_Open(                        /* Open EPPI device                 */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle            */
    u32                     DeviceNumber,       /* EPPI Device number to open       */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                    */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle location   */
    ADI_DEV_DIRECTION       Direction,          /* data direction                   */
    void                    *pEnterCriticalArg, /* critical region storage location */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager        */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                  */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function */
) 
{   
    /* Return value - assume we're going to be successful   */
    u32         Result = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on               */
    ADI_EPPI    *pDevice;
    /* pointer to the device that shares DMA channel with the Device we're working on */
    ADI_EPPI    *pTempDevice;
    /* exit critical region parameter                       */
    void        *pExitCriticalArg;
    ADI_EPPI_REGISTERS  *pRegisters;        /* pointer to ePPI registers */

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* check the direction */
    if (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL) 
    {	
	    Result = ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED;
    }

    if (DeviceNumber >= ADI_EPPI_NUM_DEVICES)       /* check the device number */
    {
        Result = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;  /* Invalid Device number */
    }
    
    /*  Continue only if the device number is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* insure the device the client wants is available  */
        Result  = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* EPPI device we're working on */
        pDevice = &Device[DeviceNumber];
    
        /* Protect this section of code - entering a critical region    */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
        /* Check the device usage status*/
        if (pDevice->InUseFlag == FALSE) 
        {
            /* Device is not in use. Reserve the device for this client */
            pDevice->InUseFlag = TRUE;
            Result = ADI_DEV_RESULT_SUCCESS;
        }
        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);
    
        /* Continue only when the EPPI device is available for use  */
        if (Result == ADI_DEV_RESULT_SUCCESS) 
        {    
            /* EPPI device is reserved for this client      */

            /* Reset the actual device                      */
            /* Configure EPPI MMRs to power-on/reset values */
            pRegisters = pDevice->pRegisters;
            pRegisters->EPPI_HCOUNT     = 0x0000;
            pRegisters->EPPI_HDELAY     = 0x0000;
            pRegisters->EPPI_VCOUNT     = 0x0000;
            pRegisters->EPPI_VDELAY     = 0x0000;
            pRegisters->EPPI_FRAME      = 0x0000;
            pRegisters->EPPI_LINE       = 0x0000;
            pRegisters->EPPI_CLKDIV     = 0x0000;
            pRegisters->EPPI_CONTROL    = 0x00000000;
            pRegisters->EPPI_FS1W_HBL   = 0x00000000;
            pRegisters->EPPI_FS1P_AVPL  = 0x00000000;
            pRegisters->EPPI_FS2W_LVB   = 0x00000000;
            pRegisters->EPPI_FS2P_LAVF  = 0x00000000;
            pRegisters->EPPI_CLIP       = 0xFF00FF00;
            pRegisters->EPPI_STATUS     = 0x01FF;
            
            /* Continue only if mainstream DMA channel of this Device is available for use */
            pTempDevice = &Device[pDevice->SharedDeviceNumber];
            /* check if the DMA shareable EPPI device is configured for Multiple DMA usage */
            if (ADI_EPPI_CONTROL_GET_DMACFG(pTempDevice))
            {
                /* Mainstream DMA channel of this Device is already in use by a shared peripheral */
                /* return error */                
                Result = ADI_EPPI_RESULT_DMA_SHARING_ERROR;
                /* mark this EPPI device as closed */
                pDevice->InUseFlag = FALSE;
            }
            else
            {
                /* Mainstream DMA channel is available. update EPPI DMA parameters */
                /* all mainstream DMA channels must be opened by default */    
                pDevice->pEppiDma       = &EppiDmaInfo[pDevice->DeviceNumber];
                pDevice->pEppiDma->pNext= NULL;
            
                /* initialize the device settings               */
                /* Pointer to critical region                   */
                pDevice->pEnterCriticalArg  = pEnterCriticalArg;
                /* Pointer to Device handle                     */
                pDevice->DeviceHandle       = DeviceHandle;
                /* Pointer to Device manager callback function  */
                pDevice->DMCallback         = DMCallback;
                /* Dataflow is disabled by default */
                pDevice->DataflowFlag       = FALSE;
                /* Error reporting is disabled by default */
                pDevice->ErrorReportingFlag = FALSE;
                /* save the physical device handle in the client supplied location */
                *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;
            }
        }
        
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)        
    }
#endif
    
    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Close
*
*   Description:    Closes down a EPPI device
*
*********************************************************************/

static u32 adi_pdd_Close(                       /* Closes a EPPI device                     */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the EPPI device to close   */
)
{  
    /* Return value - assume we're going to be successful   */
    u32         Result = ADI_DEV_RESULT_SUCCESS;
    /* pointers to the device we're working on   */
    ADI_EPPI    *pDevice,*pTmpDevice;
    /* a temp location   */
    u32         i;  
    
/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* Validate the given PDDHandle */
    Result = ValidatePDDHandle(PDDHandle);
    
    /* Continue only if the given PDDHandle is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {    
#endif
       
        /* avoid casts */
        pDevice = (ADI_EPPI *)PDDHandle;
        
        /* disable EPPI dataflow */
        Result = adi_pdd_Control (PDDHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)FALSE);
        
        /* continue ony if EPPI dataflow is disabled */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {            
            /* mark this EPPI device as closed */
            pDevice->InUseFlag = FALSE;
            /* unhook the EPPI Error interrupt handler from the system */
            Result = adi_pdd_Control(PDDHandle, ADI_DEV_CMD_SET_ERROR_REPORTING, (void *)FALSE);
                    
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Read
*
*   Description:    Never called as EPPI uses DMA
*
*********************************************************************/
static u32 adi_pdd_Read(                /* Reads data/queues inbound buffer to EPPI */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a EPPI device              */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer                        */
){
    
    /* this device uses processor DMA so we should never get here */
    return(ADI_DEV_RESULT_FAILED);
}
 
/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Never called as EPPI uses DMA
*
*********************************************************************/
static u32 adi_pdd_Write(               /*Writes data/queues outbound buffer to EPPI*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a EPPI device              */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer                        */
){
    
    /* this device uses processor DMA so we should never get here */
    return(ADI_DEV_RESULT_FAILED);
}

/*********************************************************************
*
*   Function:       adi_pdd_SequentialIO
*
*   Description:    EPPI does not support this function
*
*********************************************************************/
static u32 adi_pdd_SequentialIO(        /* Never called as EPPI uses DMA            */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a EPPI device              */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer                        */
){
    
    /* this function is not supported by EPPI */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_Control
*
*   Description:    Senses or Configures EPPI device registers
*
*********************************************************************/
static u32 adi_pdd_Control(             /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE  PDDHandle,      /* PDD handle of a EPPI device              */
    u32                 Command,        /* Command ID                               */
    void                *Value          /* Command specific value                   */
){
    
    ADI_EPPI            *pDevice;       /* pointer to the device we're working on   */
    u32                 Result;         /* return value                             */
    volatile u32        *pu32Reg;       /* pointer to unsigned 32 bit reg           */
    u32                 u32Value;       /* u32 type to avoid casts/warnings etc.    */    
    u16                 u16Value;       /* u16 type to avoid casts/warnings etc.    */    
    u32                 u32Temp;        /* temp u32 location                        */        
    u16                 u16Temp;        /* temp u16 location                        */
    u32                 Mask;           /* Mask to configure individual bit/field   */        
    u8                  ShiftCount;     /*shift count to access individual bit/field*/
    u32                 ErrorIVG;       /* IVG for EPPI Error interrupts            */
    
    /* assume we're going to be successful */
    Result          = ADI_DEV_RESULT_SUCCESS;
        
    /* avoid casts */
    pDevice         = (ADI_EPPI *)PDDHandle;
    /* assign 16 and 32 bit values for the Value argument */
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
            
                /* Enable dataflow? */
                if (u16Value)
                {
/* for Debug build only - Validate all register values before enabling Dataflow */
#if defined(ADI_DEV_DEBUG)
                    /* Validates EPPI hardware status */
                    if ((Result = ValidateEppiUsage (pDevice))!= ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Continue only when EPPI hardware status & register values are valid */
                        break;
                    }
#endif

#if defined(__ADSP_MOAB__)  /* for Moab class devices  */
                    /* configure port control registers for EPPI operation */
                    if ((Result = EppiSetPortControl(pDevice))!= ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Continue only when Port Control results in success */
                        break;
                    }
#endif 
                    /* Set EPPI_EN (Enable EPPI) */
                    ADI_EPPI_CTL_SET_PORT_EN();
                    pDevice->DataflowFlag = TRUE;
                }
                else
                {
                    /* Clear EPPI_EN (Disable EPPI) */
                    ADI_EPPI_CTL_CLEAR_PORT_EN();
                    pDevice->DataflowFlag = FALSE;
                }
                break;
            
            /* CASE: query for processor DMA support */
            case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
        
                /* yes, we do support it */
                *((u32 *)Value) = TRUE;
                break;
        
            /* CASE: query for peripheral inbound DMA channel information */
            case (ADI_DEV_CMD_GET_INBOUND_DMA_INFO):
            /* CASE: query for peripheral inbound DMA channel information */
            case (ADI_DEV_CMD_GET_OUTBOUND_DMA_INFO):
                /* pass EPPI Inbound/Outbound DMA channel information to Device manager */
                /* EPPI is half-duplex and uses same DMA channel(s) for Inbound/Outbound operation */
                *((u32 *)Value) = (u32)pDevice->pEppiDma;
                break;

            /* CASE: Get Field Received status (value of FLD bit value in EPPI_STATUS register)*/
            case (ADI_EPPI_CMD_GET_FIELD_RECEIVED_STATUS):
                *((u8 *)Value) = ADI_EPPI_STATUS_GET_FLD();
                break;

            /* CASE: Get EPPI control register value */
            case (ADI_EPPI_CMD_GET_CONTROL_REG):
                *((u32 *)Value) = pDevice->pRegisters->EPPI_CONTROL;
                break;
                
            /* CASE: Enable EPPI Error status reporting */
            case (ADI_DEV_CMD_SET_ERROR_REPORTING):
                                
                /* get the EPPI Error interrupt IVG */
                adi_int_SICGetIVG(pDevice->ErrorPeripheralID, &ErrorIVG);
                /* IF (enabling) */
                if (u32Value == TRUE) 
                {
                    /* check if this IVG is already hooked to the system */
                    if (pDevice->ErrorReportingFlag)
                    {
                        /* EPPI error interrupt reporting is already enabled. do nothing */
                        break;  
                    }
                    /* hook the EPPI Error interrupt handler to the system */
                    if (pDevice->DeviceNumber == 0) /* EPPI 0 */
                    {
                        /* Hook EPPI 0 Interrupt Handler  */
                        Result = adi_int_CECHook(ErrorIVG, Eppi0IntHandler, pDevice, TRUE);
                    }
                    else if (pDevice->DeviceNumber == 1) /* EPPI 1 */
                    {
                        /* Hook EPPI 1 Interrupt Handler */
                        Result = adi_int_CECHook(ErrorIVG, Eppi1IntHandler, pDevice, TRUE);
                    }
                    else /* must be EPPI 2 */
                    {
                        /* Hook EPPI 2 Interrupt Handler */
                        Result = adi_int_CECHook(ErrorIVG, Eppi2IntHandler, pDevice, TRUE);
                    }
                    
                    if(Result != ADI_INT_RESULT_SUCCESS)
                    {
                        break;
                    }
                    adi_int_SICWakeup(pDevice->ErrorPeripheralID, TRUE);
                    adi_int_SICEnable(pDevice->ErrorPeripheralID);
                }
                else
                {   
                    /* check if this IVG is already unhooked from the system */
                    if (!pDevice->ErrorReportingFlag)
                    {
                        /* EPPI error interrupt reporting is already disabled. do nothing */
                        break;
                    }
                    /* unhook the EPPI status interrupt handler from the system */
                    adi_int_SICDisable(pDevice->ErrorPeripheralID);
                    adi_int_SICWakeup(pDevice->ErrorPeripheralID, FALSE);
                    
                    /* unhook the EPPI Error interrupt handler to the system */
                    if (pDevice->DeviceNumber == 0) /* EPPI 0 */
                    {
                        /* unhook EPPI 0 Interrupt Handler  */
                        Result = adi_int_CECUnhook(ErrorIVG, Eppi0IntHandler, pDevice);
                    }
                    else if (pDevice->DeviceNumber == 1) /* EPPI 1 */
                    {
                        /* unhook EPPI 1 Interrupt Handler */
                        Result = adi_int_CECUnhook(ErrorIVG, Eppi1IntHandler, pDevice);
                    }
                    else /* must be EPPI 2*/
                    {
                        /* unhook EPPI 2 Interrupt Handler */
                        Result = adi_int_CECUnhook(ErrorIVG, Eppi2IntHandler, pDevice);
                    }
               }
               break;
                    
            /* CASE: Configure EPPI registers? */
            default:
            
                /* Make sure that this command is ours */
                if ((Command >= ADI_EPPI_CMD_START) && (Command <= ADI_EPPI_CMD_END))
                {
                    /* Make sure to disable EPPI dataflow before configuring EPPI registers */
                    if (pDevice->DataflowFlag)
                    {
                        /* Clear EPPI_EN (Disable EPPI) */
                        ADI_EPPI_CTL_CLEAR_PORT_EN();
#if defined(__ADSP_MOAB__)  /* for Moab class devices  */
                        /* Disable EPPI hardware port by reseting port control registers */
                        if ((Result = EppiSetPortControl(pDevice)) != ADI_DEV_RESULT_SUCCESS)
                        {
                            break;
                        }
#endif
                    }
                }
                /* This command is not supported by us. return error */
                else
                {
                    /* EPPI doesn't understand this command */
                    Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
                }
                
                /* CASEOF (Command ID)  */
                switch (Command)
                {                       
                    /* CASE: Set EPPI control register (EPPI_CONTROL) */
                    case (ADI_EPPI_CMD_SET_CONTROL_REG):
        
                        pDevice->pRegisters->EPPI_CONTROL   = u32Value;
                        /* update DMA channel usage */
                        Result = UpdateEppiDmaUsage(pDevice);
                        break;

                    /* CASE: Set EPPI Lines per Frame register (EPPI_FRAME)    */
                    case (ADI_EPPI_CMD_SET_LINES_PER_FRAME):
                    
                        pDevice->pRegisters->EPPI_FRAME     = u16Value;
                        break;
        
                    /* CASE: Set EPPI Samples Per Line register (EPPI_LINE) */
                    case (ADI_EPPI_CMD_SET_SAMPLES_PER_LINE):
                    
                        pDevice->pRegisters->EPPI_LINE      = u16Value;
                        break;
                     
                     /* CASE: Set EPPI Vertical Delay register (EPPI_VDELAY) */
                    case (ADI_EPPI_CMD_SET_VERTICAL_DELAY):
                    
                        pDevice->pRegisters->EPPI_VDELAY    = u16Value;
                        break;
                        
                    /* CASE: Set EPPI Vertical Transfer Count register (EPPI_VCOUNT) */
                    case (ADI_EPPI_CMD_SET_VERTICAL_TX_COUNT):
                    
                        pDevice->pRegisters->EPPI_VCOUNT    = u16Value;
                        break;
                        
                    /* CASE: Set EPPI Horizontal Delay register (EPPI_HDELAY) */
                    case (ADI_EPPI_CMD_SET_HORIZONTAL_DELAY):
                    
                        pDevice->pRegisters->EPPI_HDELAY    = u16Value;
                        break;
                    
                    /* CASE: Set EPPI Horizontal Transfer Count register (EPPI_HCOUNT) */
                    case (ADI_EPPI_CMD_SET_HORIZONTAL_TX_COUNT):
                    
                        pDevice->pRegisters->EPPI_HCOUNT    = u16Value;
                        break;
                        
                    /* CASE: Set EPPI Clock Frequency*/
                    case (ADI_EPPI_CMD_SET_CLOCK_FREQ):
                    
                        /* Get the SCLK value to calculate EPPI_CLKDIV for given frequency */
	                    if ((Result = adi_pwr_GetFreq(&Mask, &u32Temp, &Mask)) != ADI_PWR_RESULT_SUCCESS)
                        {
                            break;
                        }
/* Debug build only - check for valid EPPI Clock frequency */
#if defined(ADI_DEV_DEBUG)
                        /* EPPI Clock cannot be greater than SCLK/2, also cannot be zero */
                        if ((u32Value > (u32Temp/2)) || (u32Value == 0))
                        {
                            Result = ADI_EPPI_RESULT_CLOCK_FREQ_INVALID;
                            break;
                        }
#endif
                        /* Calculate EPPI_CLKDIV value */
                        /* EPPI_CLK = SCLK / (2 * (EPPI_CLKDIV + 1)) */
                        /* EPPI_CLKDIV = (SCLK / (2 * EPPI_CLK)) - 1 */
                        u16Temp = (u16)(((u32Temp / (2 * u32Value)) - 1));
                        pDevice->pRegisters->EPPI_CLKDIV    = u16Temp;
                        break;
                        
                    /* CASE: Set EPPI Clock Divide register (EPPI_CLKDIV) */
                    case (ADI_EPPI_CMD_SET_CLOCK_DIV):
/* Debug build only - check for valid EPPI Clock Div */
#if defined(ADI_DEV_DEBUG)
                        /* EPPI Clock Div must be with in the range (0xFFFE to 0x0000) */
                        if ((u32Value > 0xFFFE) || (u32Value == 0))
                        {
                            Result = ADI_EPPI_RESULT_CLOCK_DIV_INVALID;
                            break;
                        }
#endif
                        pDevice->pRegisters->EPPI_CLKDIV    = u16Value;
                        break;
                        
                    /* CASE: Set EPPI Frame Sync 1 Width register (EPPI_FS1W_HBL) */
                    case (ADI_EPPI_CMD_SET_FS1_WIDTH):
                    /* CASE: Set Horizontal Blanking Samples per Line (lower 16 bits in EPPI_FS1W_HBL register) */
                    case (ADI_EPPI_CMD_SET_HORIZONTAL_BLANK_PER_LINE):
                                        
                        pDevice->pRegisters->EPPI_FS1W_HBL  = u32Value;
                        break;
                        
                    /* CASE: Set EPPI Frame Sync 2 Width register (EPPI_FS2W_LVB) */
                    case (ADI_EPPI_CMD_SET_FS2_WIDTH):
                    
                        pDevice->pRegisters->EPPI_FS2W_LVB  = u32Value;
                        break;
                        
                    /* CASE: Set EPPI Frame Sync 1 Period register (EPPI_FS1P_AVPL) */
                    case (ADI_EPPI_CMD_SET_FS1_PERIOD):
                    /* CASE: Set Vertical Blanking Samples per Line (lower 16 bits in EPPI_FS1P_AVPL register) */
                    case (ADI_EPPI_CMD_SET_VERTICAL_BLANK_PER_LINE):
                    
                        pDevice->pRegisters->EPPI_FS1P_AVPL = u32Value;
                        break;
                        
                    /* CASE: Set EPPI Frame Sync 2 Period register (EPPI_FS2P_LAVF) */
                    case (ADI_EPPI_CMD_SET_FS2_PERIOD):
                    
                        pDevice->pRegisters->EPPI_FS2P_LAVF = u32Value;
                        break;
                        
                    /* CASE: Set EPPI Clipping register (EPPI_CLIP) */
                    case (ADI_EPPI_CMD_SET_CLIPPING):
                    
                        pDevice->pRegisters->EPPI_CLIP      = u32Value;
                        break;
                    
                    /* Commands to configure individual bits/fields of EPPI control register (EPPI_CONTROL) */
                    /* CASE: Set EPPI Direction (PORT_DIR) */
                    case (ADI_EPPI_CMD_SET_PORT_DIRECTION):
                    /* CASE: Set EPPI Transfer type (XFR_TYPE) */
                    case (ADI_EPPI_CMD_SET_TRANSFER_TYPE):
                    /* CASE: Set Frame sync configuration (FS_CFG) */
                    case (ADI_EPPI_CMD_SET_FRAME_SYNC_CONFIG):
                    /* CASE: Set Field select/trigger (FLD_SEL) */
                    case (ADI_EPPI_CMD_SET_FIELD_SELECT_TRIGGER):
                    /* CASE: Set ITU Type (ITU_TYPE) */
                    case (ADI_EPPI_CMD_SET_ITU_TYPE):
                    /* CASE: Enable Blank/preamble generation (BLANKGEN) */
                    case (ADI_EPPI_CMD_ENABLE_BLANKGEN):
                    /* CASE: Enable Internal clock gen (ICLKGEN) */
                    case (ADI_EPPI_CMD_ENABLE_INTERNAL_CLOCK_GEN):
                    /* CASE: Set Enable Internal Frame Sync generation (IFSGEN) */
                    case (ADI_EPPI_CMD_ENABLE_INTERNAL_FS_GEN):                    
                    /* CASE: Set clock polarity (POLC) */
                    case (ADI_EPPI_CMD_SET_CLOCK_POLARITY):
                    /* CASE: Set Frame sync polarity (POLS) */
                    case (ADI_EPPI_CMD_SET_FRAME_SYNC_POLARITY):
                    /* CASE: Set Data length (DLEN) */
                    case (ADI_EPPI_CMD_SET_DATA_LENGTH):
                    /* CASE: Set Skip enable (SKIP_EN) */
                    case (ADI_EPPI_CMD_SET_SKIP_ENABLE):
                    /* CASE: Set Skip even or odd elements (SKIP_EO) */
                    case (ADI_EPPI_CMD_SET_SKIP_EVEN_ODD):
                    /* CASE: Enable DMA pack/unpack (PACKEN/UNPACKEN) */
                    case (ADI_EPPI_CMD_SET_PACK_UNPACK_ENABLE):
                    /* CASE: Set Swap enable (SWAPEN) */
                    case (ADI_EPPI_CMD_SET_SWAP_ENABLE):
                    /* CASE: Set Sign Extension/Split 16 (SIGN_EXT/SPLIT_16) */
                    case (ADI_EPPI_CMD_SET_SIGN_EXT_SPLIT16):
                    /* CASE: Set Split Even/Odd Samples(SPLT_EVEN_ODD) */
                    case (ADI_EPPI_CMD_SET_SPLIT_EVEN_ODD):
                    /* CASE: Enable Sub split odd (SUBSPLIT_ODD) */
                    case (ADI_EPPI_CMD_ENABLE_SUBSPLIT_ODD):
                    /* CASE: Set DMA channel mode (DMACFG) */
                    case (ADI_EPPI_CMD_SET_DMA_CHANNEL_MODE):
                    /* CASE: Enable RGB Formatting (RGB_FMT_EN) */
                    case (ADI_EPPI_CMD_ENABLE_RGB_FORMATTING):
                    /* CASE: Set FIFO Regular watermark(FIFO_RWM) */
                    case (ADI_EPPI_CMD_SET_FIFO_REGULAR_WATERMARK):
                    /* CASE: Set FIFO Urgent watermark (FIFO_UWM) */
                    case (ADI_EPPI_CMD_SET_FIFO_URGENT_WATERMARK):
                    
                    /* Commands to configure individual bits/fields of vertical blanking (EPPI_FS2W_LVB) register   */
                    /* CASE: Set number of lines of vertical blanking before Field 1 active data */
                    case (ADI_EPPI_CMD_SET_FIELD1_PRE_ACTIVE_DATA_VBLANK):
                    /* CASE: Set number of lines of vertical blanking after Field 1 active data */
                    case (ADI_EPPI_CMD_SET_FIELD1_POST_ACTIVE_DATA_VBLANK):
                    /* CASE: Set number of lines of vertical blanking before Field 2 active data */
                    case (ADI_EPPI_CMD_SET_FIELD2_PRE_ACTIVE_DATA_VBLANK):
                    /* CASE: Set number of lines of vertical blanking after Field 1 active data */
                    case (ADI_EPPI_CMD_SET_FIELD2_POST_ACTIVE_DATA_VBLANK):
                    
                     /* Commands to configure individual bits/fields of Lines of active video per frame (EPPI_FS2P_LAVF) register */
                    /* CASE: Set Number of lines of active data in Field 1 */
                    case (ADI_EPPI_CMD_SET_FIELD1_ACTIVE_DATA_LINES):
                    /* CASE: Set Number of lines of active data in Field 2 */
                    case (ADI_EPPI_CMD_SET_FIELD2_ACTIVE_DATA_LINES):
                    
                    /* Commands to configure individual bits/fields of clipping (EPPI_CLIP) register */
                    /* CASE: Set Lower clipping limit for odd bytes (Chroma) */
                    case (ADI_EPPI_CMD_SET_CHROMA_LOW_CLIP_LIMIT):
                    /* CASE: Set Higher clipping limit for odd bytes (Chroma) */
                    case (ADI_EPPI_CMD_SET_CHROMA_HIGH_CLIP_LIMIT):
                    /* CASE: Set Lower clipping limit for even bytes (Luma) */
                    case (ADI_EPPI_CMD_SET_LUMA_LOW_CLIP_LIMIT):
                    /* CASE: Set Higher clipping limit for even bytes (Luma) */
                    case (ADI_EPPI_CMD_SET_LUMA_HIGH_CLIP_LIMIT):
                                        
                        /* Get the command index & address of register to be configured */
                        /* is this EPPI_CONTROL configuration related command? */
                        if (Command < ADI_EPPI_CMD_SET_FIELD1_PRE_ACTIVE_DATA_VBLANK)
                        {
                            /* this command is to configure EPPI Control register */
                            /* command index */
                            u16Temp     = (Command - ADI_EPPI_CMD_SET_PORT_DIRECTION);
                            /* register to access */
                            pu32Reg     = (volatile u32 *) &pDevice->pRegisters->EPPI_CONTROL;                            
                            /* Initialise ShifCount to 1 as the first bit - EPPI enable/disable is not configured here */
                            ShiftCount = 1;
                            /* Calculate the Shift Count value to reach start of the field location to be configured */
                            while(u16Temp)
                            {
                                ShiftCount++;
                                /* is this start of new bit/field location? */
                                if ((EppiControlMask >> ShiftCount) & 0x01)
                                {
                                    u16Temp--;
                                }
                            }
                            /* Extract this field length. Field length must be atleast 1 */
                            u16Temp = 1;
                            /* load the mask value for remaining bit/field location(s) */
                            u32Temp = EppiControlMask >> (ShiftCount+1);
                            /* continue until bit 0 in u32Temp equals 1 (indicates start of next bit/field) */
                            while (!(u32Temp & 0x01))
                            {
                                u32Temp >>= 1;
                                u16Temp++;      /* increment the field length */
                            }
                        }
                        /* is this EPPI_FS2W_LVB configuration related command? */
                        else if (Command < ADI_EPPI_CMD_SET_FIELD1_ACTIVE_DATA_LINES)
                        {
                            /* command index */
                            u16Temp     = (Command - ADI_EPPI_CMD_SET_FIELD1_PRE_ACTIVE_DATA_VBLANK);
                             /* register to access */
                            pu32Reg     = (volatile u32 *) &pDevice->pRegisters->EPPI_FS2W_LVB;                            
                            /* Calculate shift count */
                            ShiftCount  = u16Temp * 8;
                            /* Field length is always 8 for this register */
                            u16Temp     = 8;
                        }
                        /* is this EPPI_FS2P_LAVF configuration related command? */
                        else if (Command < ADI_EPPI_CMD_SET_CHROMA_LOW_CLIP_LIMIT)
                        {
                            /* command index */
                            u16Temp     = (Command - ADI_EPPI_CMD_SET_FIELD1_ACTIVE_DATA_LINES);
                             /* register to access */
                            pu32Reg     = (volatile u32 *) &pDevice->pRegisters->EPPI_FS2P_LAVF;                            
                            /* Calculate shift count */
                            ShiftCount  = u16Temp * 16;
                            /* Field length is always 16 for this register */
                            u16Temp     = 16;
                        }
                        /* this must be EPPI_CLIP configuration related command */
                        else
                        {
                            /* command index */
                            u16Temp     = (Command - ADI_EPPI_CMD_SET_CHROMA_LOW_CLIP_LIMIT);
                             /* register to access */
                            pu32Reg     = (volatile u32 *) &pDevice->pRegisters->EPPI_CLIP;                            
                            /* Calculate shift count */
                            ShiftCount  = u16Temp * 8;
                            /* Field length is always 8 for this register */
                            u16Temp     = 8;
                        }                        
                        /* Generate the mask value for the location(s) to be configured */
                        /* 'ShiftCount' holds the shift count value to reach the corresponding bit/field, 'u16Temp' holds the field length*/
                        Mask = ((0xFFFFFFFF << (ShiftCount+u16Temp)) | (0xFFFFFFFF >> (32-ShiftCount)));
                        /* Get the new bit/field value */
                        u32Value = ((u32Value << ShiftCount) & ~Mask);
                        /* Clear the corresponding register bit/field location(s) to be configured */
                        u32Temp = (*pu32Reg & Mask);
                        /* Update the corresponding register with new bit/field value */
                        *pu32Reg = (u32Temp | u32Value);
                        
                        /* if is this EPPI_CONTROL configuration related command, update DMA channel usage */
                        if (Command < ADI_EPPI_CMD_SET_FIELD1_PRE_ACTIVE_DATA_VBLANK)
                        {
                            /* update DMA channel usage */
                            Result = UpdateEppiDmaUsage(pDevice);
                        }
                        break;
                /* ENDCASE for EPPI register configuration */
                }
                
                /* make sure to re-enable EPPI if we're enabled */
                if (pDevice->DataflowFlag == TRUE)
                {
/* for Debug build only - Validate all register values before enabling Dataflow */
#if defined(ADI_DEV_DEBUG)
                    /* Validates EPPI hardware status */
                    Result = ValidateEppiUsage (pDevice);
                    /* Continue only when EPPI register values are valid */
                    if (Result != ADI_DEV_RESULT_SUCCESS)
                    {
                        break;
                    }
#endif

#if defined(__ADSP_MOAB__)  /* for Moab class devices  */
                    /* re-configure port control registers for EPPI operation as EPPI configuration has changed */
                    if ((Result = EppiSetPortControl(pDevice))!= ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Continue only when Port Control results in success */
                        break;
                    }
#endif 
                    /* Set EPPI_EN (Enable EPPI) */
                    ADI_EPPI_CTL_SET_PORT_EN();
                }

                break;
        /* ENDCASE */
        }
        
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}

/*********************************************************************

    Function:       UpdateEppiDmaUsage

    Description:    Updates EPPI DMA channel usage status and
                    DMA channel information table

*********************************************************************/

static u32 UpdateEppiDmaUsage(      /* Updates EPPI DMA channel usage           */
    ADI_EPPI    *pDevice            /* pointer to the device we're working on   */
){
    /* Return value - assume we're going to be successful */
    u32 Result  = ADI_DEV_RESULT_SUCCESS;
    
    /* Get DMACFG status */
    if (ADI_EPPI_CONTROL_GET_DMACFG(pDevice))
    {
        /* Two DMA channel mode selected */
        /* Continue only if the device is presently using one DMA channel only */
        if (pDevice->pEppiDma->pNext == NULL)
        {
            /* Check if the shared DMA channel is available for use */
            if (Device[pDevice->SharedDeviceNumber].InUseFlag == TRUE)
            {
                /* The shareable DMA channel is already in use. Return error */
                Result = ADI_EPPI_RESULT_DMA_SHARING_ERROR;
            }
            else
            {
                /* Shared/Extensible DMA channel is available for use */
                /* request Device manager to open extensiable DMA for use */
                Result = adi_dev_Control(pDevice->DeviceHandle, ADI_DEV_CMD_OPEN_PERIPHERAL_DMA, (void *)&EppiDmaInfo[pDevice->SharedDeviceNumber]);
                if (Result == ADI_DEV_RESULT_SUCCESS)
                {
                    /* link the Shared/Extensible DMA to the mainstream DMA */
                    pDevice->pEppiDma->pNext = &EppiDmaInfo[pDevice->SharedDeviceNumber];
                    /* Terminate the shared DMA table */
                    EppiDmaInfo[pDevice->SharedDeviceNumber].pNext = NULL;
                }
            }
        }
    }
    else
    {
        /* One DMA channel mode selected */
        /* Continue only if the device is presently using Two DMA channels */
        if (pDevice->pEppiDma->pNext != NULL)
        {
            /* request Device manager to close the shared DMA channel */
            Result = adi_dev_Control(pDevice->DeviceHandle, ADI_DEV_CMD_CLOSE_PERIPHERAL_DMA, (void *)pDevice->pEppiDma->pNext);
            if (Result == ADI_DEV_RESULT_SUCCESS)
            {
                /* Remove the shared DMA information table from mainstream DMA list */
                pDevice->pEppiDma->pNext   = EppiDmaInfo[pDevice->SharedDeviceNumber].pNext;
            }
        }
    }
    
    /* return */
    return (Result);
}  

/******************* EPPI Error Interrupt handlers *******************/

/*********************************************************************

    Function:       Eppi0IntHandler

    Description:    Processes EPPI 0 Error interrupts

*********************************************************************/
static ADI_INT_HANDLER(Eppi0IntHandler)  /* EPPI 0 Error interrupt handler */
{
    ADI_EPPI    *pDevice;

    pDevice     = &Device[0];           /* Device that generated this interrupt */
        
    /* Call EPPI Error reporting routine */
    ReportEppiError(pDevice);
    
    return(ADI_INT_RESULT_PROCESSED);
}  

/*********************************************************************

    Function:       Eppi1IntHandler

    Description:    Processes EPPI 1 Error interrupts

*********************************************************************/
static ADI_INT_HANDLER(Eppi1IntHandler)  /* EPPI 1 Error interrupt handler */
{
    ADI_EPPI    *pDevice;

    pDevice     = &Device[1];           /* Device that generated this interrupt */    
    
    /* Call EPPI Error reporting routine */
    ReportEppiError(pDevice);
    
    return(ADI_INT_RESULT_PROCESSED);
}

/*********************************************************************

    Function:       Eppi2IntHandler

    Description:    Processes EPPI 2 Error interrupts

*********************************************************************/
static ADI_INT_HANDLER(Eppi2IntHandler)  /* EPPI 2 Error interrupt handler */
{
    ADI_EPPI    *pDevice;

    pDevice     = &Device[2];           /* Device that generated this interrupt */    
    
    /* Call EPPI Error reporting routine */
    ReportEppiError(pDevice);
    
    return(ADI_INT_RESULT_PROCESSED);
}

/*********************************************************************

    Function:       ReportEppiError

    Description:    Reports EPPI Error(s) to the client

*********************************************************************/
static void ReportEppiError(        /* Reports EPPI Error(s) to the client      */
    ADI_EPPI        *pDevice        /* pointer to the device we're working on   */
){

    /* Base error event */
    u32     BaseEvent = ADI_EPPI_EVENT_CHROMA_FIFO_ERROR;
    /* Temp loactions to hold EPPI_STATUS register value */
    u16     Status, TempStatus;
    u8      i;  

    /* Get the present EPPI Status register value */
    TempStatus = (pDevice->pRegisters->EPPI_STATUS & 0x407F);
	Status = TempStatus;
	
    /* Post callback for all status error interrupts */
    for (i=7; i ; i--)
    {
        /* check if this error bit is set */
        if (TempStatus & 1)
        {
            /* post callback for this error (parameters are device handle, BaseEvent, NULL) */
            (pDevice->DMCallback)(pDevice->DeviceHandle, BaseEvent, NULL);    
        }
        TempStatus >>= 1;   /* move to next status bit */
        BaseEvent++;        /* move to next status bit error interrupt event id */
    }
    
    TempStatus >>= 7;       /* move to Preamble Error status bit */
    
    /* check for Preamble Error */
    if (TempStatus & 1)
    {
        /* post callback for Preamble Error (parameters are device handle, BaseEvent, NULL) */
        (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_EPPI_EVENT_PREAMBLE_ERROR, NULL);  
    }

    /* Clear all EPPI status error interrupt bit locations */
    pDevice->pRegisters->EPPI_STATUS = Status;
        
    /* return */
    return;
}

#if defined(__ADSP_MOAB__)      /* Static functions for Moab class devices      */
/*********************************************************************

    Function:       EppiSetPortControl

    Description:    Configures the general purpose ports for Moab
                    class devices for EPPI usage.

*********************************************************************/
static u32 EppiSetPortControl(      /* configures the port control registers    */
    ADI_EPPI    *pDevice           	/* pointer to the device we're working on   */
){

    /* Pointer to list directives to enable EPPI Clock and Data ports 	*/
    ADI_PORTS_DIRECTIVE		*pClkDataDirectives;
    /* Pointer to list directives to enable EPPI Frame Sync ports 		*/
    ADI_PORTS_DIRECTIVE		*pFsDirectives;
    /* Number of directives to be passed */
    u32                     nDirectives;
	u32						Result;
    u32                     u32Temp;
    u8						Dlen;
    
    /* assume we're going to be successful */
    Result = ADI_PORTS_RESULT_SUCCESS;
	
    /* Directives to enable EPPI 0 Clock and Data ports */
    ADI_PORTS_DIRECTIVE		Eppi0ClkDataDirectives [] = {
        ADI_PORTS_DIRECTIVE_EPPI0_CLK,
        ADI_PORTS_DIRECTIVE_EPPI0_D0, ADI_PORTS_DIRECTIVE_EPPI0_D1, 
        ADI_PORTS_DIRECTIVE_EPPI0_D2, ADI_PORTS_DIRECTIVE_EPPI0_D3, 
        ADI_PORTS_DIRECTIVE_EPPI0_D4, ADI_PORTS_DIRECTIVE_EPPI0_D5,
        ADI_PORTS_DIRECTIVE_EPPI0_D6, ADI_PORTS_DIRECTIVE_EPPI0_D7, 
        ADI_PORTS_DIRECTIVE_EPPI0_D8, ADI_PORTS_DIRECTIVE_EPPI0_D9,
        ADI_PORTS_DIRECTIVE_EPPI0_D10,ADI_PORTS_DIRECTIVE_EPPI0_D11,
        ADI_PORTS_DIRECTIVE_EPPI0_D12,ADI_PORTS_DIRECTIVE_EPPI0_D13,
        ADI_PORTS_DIRECTIVE_EPPI0_D14,ADI_PORTS_DIRECTIVE_EPPI0_D15,
        ADI_PORTS_DIRECTIVE_EPPI0_D16,ADI_PORTS_DIRECTIVE_EPPI0_D17,
        ADI_PORTS_DIRECTIVE_EPPI0_D18,ADI_PORTS_DIRECTIVE_EPPI0_D19,
        ADI_PORTS_DIRECTIVE_EPPI0_D20,ADI_PORTS_DIRECTIVE_EPPI0_D21,
        ADI_PORTS_DIRECTIVE_EPPI0_D22,ADI_PORTS_DIRECTIVE_EPPI0_D23
    };
    
    /* Directives to enable EPPI 0 Frame sync ports */
	ADI_PORTS_DIRECTIVE		Eppi0FsDirectives [] = {
		ADI_PORTS_DIRECTIVE_EPPI0_FS1,
        ADI_PORTS_DIRECTIVE_EPPI0_FS2,
        ADI_PORTS_DIRECTIVE_EPPI0_FS3
	};
	    
    /* Directives to enable EPPI 1 Clock and Data ports */
    ADI_PORTS_DIRECTIVE		Eppi1ClkDataDirectives [] = {
        ADI_PORTS_DIRECTIVE_EPPI1_CLK,
        ADI_PORTS_DIRECTIVE_EPPI1_D0, ADI_PORTS_DIRECTIVE_EPPI1_D1, 
        ADI_PORTS_DIRECTIVE_EPPI1_D2, ADI_PORTS_DIRECTIVE_EPPI1_D3, 
        ADI_PORTS_DIRECTIVE_EPPI1_D4, ADI_PORTS_DIRECTIVE_EPPI1_D5,
        ADI_PORTS_DIRECTIVE_EPPI1_D6, ADI_PORTS_DIRECTIVE_EPPI1_D7, 
        ADI_PORTS_DIRECTIVE_EPPI1_D8, ADI_PORTS_DIRECTIVE_EPPI1_D9,
        ADI_PORTS_DIRECTIVE_EPPI1_D10,ADI_PORTS_DIRECTIVE_EPPI1_D11,
        ADI_PORTS_DIRECTIVE_EPPI1_D12,ADI_PORTS_DIRECTIVE_EPPI1_D13,
        ADI_PORTS_DIRECTIVE_EPPI1_D14,ADI_PORTS_DIRECTIVE_EPPI1_D15
    };

    /* Directives to enable EPPI 1 Frame sync ports */
	ADI_PORTS_DIRECTIVE		Eppi1FsDirectives [] = {
		ADI_PORTS_DIRECTIVE_EPPI1_FS1,
        ADI_PORTS_DIRECTIVE_EPPI1_FS2,
        ADI_PORTS_DIRECTIVE_EPPI1_FS3
	};
	    
    /* Directives to enable EPPI 2 Clock and Data ports */
    ADI_PORTS_DIRECTIVE		Eppi2ClkDataDirectives [] = {
        ADI_PORTS_DIRECTIVE_EPPI2_CLK,
        ADI_PORTS_DIRECTIVE_EPPI2_D0, ADI_PORTS_DIRECTIVE_EPPI2_D1, 
        ADI_PORTS_DIRECTIVE_EPPI2_D2, ADI_PORTS_DIRECTIVE_EPPI2_D3, 
        ADI_PORTS_DIRECTIVE_EPPI2_D4, ADI_PORTS_DIRECTIVE_EPPI2_D5,
        ADI_PORTS_DIRECTIVE_EPPI2_D6, ADI_PORTS_DIRECTIVE_EPPI2_D7 
    };

    /* Directives to enable EPPI 2 Frame sync ports */
	ADI_PORTS_DIRECTIVE		Eppi2FsDirectives [] = {
		ADI_PORTS_DIRECTIVE_EPPI2_FS1,
        ADI_PORTS_DIRECTIVE_EPPI2_FS2,
        ADI_PORTS_DIRECTIVE_EPPI2_FS3
	};
	    
    /* assume no directives are to be set */
    pClkDataDirectives 	= NULL;
    pFsDirectives		= NULL;
    /* EPPI device needs Clock & Data pins 0 to 7 by default */
    nDirectives = 9;

	/* Determine the Directive table to pass */   
    /* IF (configure ports for device 0) */
    if (pDevice->DeviceNumber == 0)
    {
		/* Pass EPPI 0 Directives */
        pClkDataDirectives 	= Eppi0ClkDataDirectives;
        pFsDirectives		= Eppi0FsDirectives;
    }
    /* ELSE IF (configure ports for device 1) */
	else if (pDevice->DeviceNumber == 1)
	{
		/* Pass EPPI 1 Directives */
        pClkDataDirectives 	= Eppi1ClkDataDirectives;
        pFsDirectives		= Eppi1FsDirectives;
    }
	/* ELSE (configure ports for device 2) */
	else if (pDevice->DeviceNumber == 2)
	{
		/* Pass EPPI 2 Directives */
        pClkDataDirectives 	= Eppi2ClkDataDirectives;
        pFsDirectives		= Eppi2FsDirectives;
    }
    
    /* read EPPI Control register */
    u32Temp = pDevice->pRegisters->EPPI_CONTROL;
    /* Extract EPPI Data length */
    /* any additional Data pins to enable? */
    Dlen = (u8)((u32Temp & DLEN) >> 15);
    /* if Data length > 8 bits) */
    if (Dlen)
    {
		nDirectives += (Dlen * 2);
    }
    /* if Data length > 18 bits) */
    if (Dlen > 5)
    {
        nDirectives += 4;
    }

    /* IF (any Clack and Data port Directives to set) */
    if (pClkDataDirectives)
    {
    	/* Pass EPPI Clock and Data port directives */
		Result = adi_ports_Configure(pClkDataDirectives, nDirectives);
    }
    	
	/* IF (port control results in success) */
	if (Result == ADI_PORTS_RESULT_SUCCESS)
	{
    	/* IF (EPPI configured in Non-ITU-R 656 / GP Mode) */
    	if (((u32Temp & XFR_TYPE) >> 2) == 3)    
    	{
    		/* IF (any Frame Sync Directives to set) */
    		if (pFsDirectives)    		
    		{
        		/* extract Frame Sync Count */
        		nDirectives = ((u32Temp & FS_CFG) >> 4);
    			/* Pass EPPI Frame sync port directives */
				Result = adi_ports_Configure(pFsDirectives, nDirectives);
    		}
    	}
    }
    
    /* return */
    return (Result);
}

#endif                          /* End of Moab specific Static functions        */

#if defined(ADI_DEV_DEBUG)  /* Debug build only */
/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of a EPPI device  */
)
{    
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     Result = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* index */
    u32     i;
    
    /* compare the given PDD handle with PDDHandle allocated to all EPPI devices in the list    */
    for (i = 0; i < ADI_EPPI_NUM_DEVICES; i++) 
    {        
        if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i])
        {
            /* Given PDDHandle is valid. quit this loop */
            Result = ADI_DEV_RESULT_SUCCESS;
            break;
        }
    }
    /* return */
    return (Result);
}

/*********************************************************************

    Function:       ValidateEppiUsage

    Description:    Validates EPPI Hardware status before enabling 
                    EPPI dataflow

*********************************************************************/

static u32 ValidateEppiUsage(
    ADI_EPPI     *pDevice       /* pointer to the device we're working on               */
){
    /* Return value - assume we're going to be successful */
    u32         Result  = ADI_DEV_RESULT_SUCCESS;
    /* TRUE when validation is complete & success */
    u8          ValidateComplete = FALSE;   
    /* pointer to a device we're validating on */
    ADI_EPPI    *pTempDevice;   
    
    /* Validate EPPI Hardware status */        
    /* Switch: DeviceNumber */
    switch (pDevice->DeviceNumber)
    {
        /* Case: EPPI0 */
        case 0:
            /* Validate with EPPI1 operating status */
            pTempDevice = &Device[1];   /* point to the device we're validating with */
            /* If EPPI1 is in use, then EPPI0 can't be operated in DLEN = 24bits */
            if ((pTempDevice->InUseFlag == TRUE) && (pTempDevice->DataflowFlag == TRUE))
            {
                /* EPPI1 is in use. Get EPPI0 DLEN value */
                if (ADI_EPPI_CONTROL_GET_DLEN(pDevice) > 5)
                {
                    /* EPPI0 is set for DLEN = 24. Return error */
                    Result = ADI_EPPI_RESULT_PORT_SHARING_ERROR;
                }
            }
            break;
        /* Case: EPPI1 */
        case 1:            
            /* Validate with EPPI0 operating status */
            pTempDevice = &Device[0];   /* point to the device we're validating with */
            /* If EPPI0 is in use & operated in 24 bit DLEN, then EPPI1 can't used */
            if ((pTempDevice->InUseFlag == TRUE) && (pTempDevice->DataflowFlag == TRUE))
            {                    
                /* EPPI0 is in use. Get EPPI0 DLEN value */
                if (ADI_EPPI_CONTROL_GET_DLEN(pTempDevice) > 5)
                {
                    /* EPPI0 is set for DLEN = 24 bits. Return error */
                    Result = ADI_EPPI_RESULT_PORT_SHARING_ERROR;
                    break;
                }
            }
            /* Validate with EPPI2 operating status */
            pTempDevice = &Device[2];   /* point to the device we're validating with */                
            /* If EPPI2 is in use, then EPPI1 DLEN can't be more than 8bits */
            if ((pTempDevice->InUseFlag == TRUE) && (pTempDevice->DataflowFlag == TRUE))
            { 
                /* EPPI2 is in use. Get EPPI1 DLEN value */
                if (ADI_EPPI_CONTROL_GET_DLEN(pDevice))
                {
                    /* EPPI1 is set for DLEN > 8 bits. Return error */
                    Result = ADI_EPPI_RESULT_PORT_SHARING_ERROR;
                }
            }
            break;
                
        /* Case: EPPI2 */
        case 2:
            /* Validate with EPPI1 operating status */
            pTempDevice = &Device[1];   /* point to the device we're validating with */
                
            /* If EPPI1 is in use, then EPPI2 can be used only when EPPI1 DLEN is set for 8 bits) */
            if ((pTempDevice->InUseFlag == TRUE) && (pTempDevice->DataflowFlag == TRUE))
            { 
                /* EPPI1 is in use. Get EPPI1 DLEN value */
                if (ADI_EPPI_CONTROL_GET_DLEN(pTempDevice))
                {
                    /* EPPI1 is set for DLEN > 8 bits. Return error */
                    Result = ADI_EPPI_RESULT_PORT_SHARING_ERROR;
                }
            }
            break;
            
        default:
            break;
    }
    
    /* return */
    return (Result);
}
#endif
