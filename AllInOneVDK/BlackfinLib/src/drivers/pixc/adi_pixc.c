/**********************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software 
you agree to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_pixc.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
    This is the driver source code for the Pixel Compositor.
            
***********************************************************************/

/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>      /* system service includes          */
#include <drivers/adi_dev.h>        /* device manager includes          */
#include <drivers/pixc/adi_pixc.h>  /* Pixel Compositor driver includes */

/* PixC is available only for Moab class devices*/
#if !defined(__ADSP_MOAB__)
#error "PixC is available only on ADSP-BF542,ADSP-BF544,ADSP-BF548 & ADSP-BF549"
#endif

/*********************************************************************

Enumerations and defines

*********************************************************************/

/* number of Pixel Compositors (PIXC) in the system                 */
#define ADI_PIXC_NUM_DEVICES        (sizeof(Device)/sizeof(ADI_PIXC))

/*********************************************************************

Macros to manipulate selected bits/fields in PIXC MMRs

*********************************************************************/
/* Macros to manipulate bits/fields of PIXC control register                                */
/* Set PIXC_EN (Enables Pixel Compositor)                                                   */

#define ADI_PIXC_CTL_SET_PIXC_EN()          (pDevice->pRegisters->PixcCtrl  |= 0x0001)
/* Clear PIXC_EN (Disables Pixel Compositor)                                                */
#define ADI_PIXC_CTL_CLEAR_PIXC_EN()        (pDevice->pRegisters->PixcCtrl  &= 0x3FFE)
/* Set OVR_A_EN (Enables Overlay A)                                                         */
#define ADI_PIXC_CTL_SET_OVR_A_EN()         (pDevice->pRegisters->PixcCtrl  |= 0x0002)
/* Clear OVR_A_EN (Disables Overlay A)                                                      */
#define ADI_PIXC_CTL_CLEAR_OVR_A_EN()       (pDevice->pRegisters->PixcCtrl  &= 0x3FFD)
/* Set OVR_B_EN (Enables Overlay B)                                                         */
#define ADI_PIXC_CTL_SET_OVR_B_EN()         (pDevice->pRegisters->PixcCtrl  |= 0x0004)
/* Clear OVR_B_EN (Disables Overlay B)                                                      */
#define ADI_PIXC_CTL_CLEAR_OVR_B_EN()       (pDevice->pRegisters->PixcCtrl  &= 0x3FFB)
/* Set IMG_FORM (Sets Image Data Format to RGB)                                             */
#define ADI_PIXC_CTL_SET_IMG_FORM_RGB()     (pDevice->pRegisters->PixcCtrl  |= 0x0008)
/* Clear IMG_FORM (Sets Image Data Format to YUV)                                           */
#define ADI_PIXC_CTL_CLEAR_IMG_FORM_YUV()   (pDevice->pRegisters->PixcCtrl  &= 0x3FF7)
/* Set OVR_FORM (Sets Overlay Data Format to RGB)                                           */
#define ADI_PIXC_CTL_SET_OVR_FORM_RGB()     (pDevice->pRegisters->PixcCtrl  |= 0x0010)
/* Clear OVR_FORM (Sets Overlay Data Format to YUV)                                         */
#define ADI_PIXC_CTL_CLEAR_OVR_FORM_YUV()   (pDevice->pRegisters->PixcCtrl  &= 0x3FEF)
/* Set OUT_FORM (Sets Output Data Format to RGB)                                            */
#define ADI_PIXC_CTL_SET_OUT_FORM_RGB()     (pDevice->pRegisters->PixcCtrl  |= 0x0020)
/* Clear OUT_FORM (Sets Output Data Format to YUV)                                          */
#define ADI_PIXC_CTL_CLEAR_OUT_FORM_YUV()   (pDevice->pRegisters->PixcCtrl  &= 0x3FDF)
/* Set UDS_MOD (Sets Resampling mode to average for both up and down sampling)              */
#define ADI_PIXC_CTL_SET_UDS_MOD()          (pDevice->pRegisters->PixcCtrl  |= 0x0040)
/* Clear UDS_MOD (Sets Resampling mode to duplicate for both up and down sampling)          */
#define ADI_PIXC_CTL_CLEAR_UDS_MOD()        (pDevice->pRegisters->PixcCtrl  &= 0x3FBF)
/* Set TC_EN (Enables Transparent color)                                                    */
#define ADI_PIXC_CTL_SET_TC_EN()            (pDevice->pRegisters->PixcCtrl  |= 0x0080)
/* Clear TC_EN (Disables Transparent color)                                                 */
#define ADI_PIXC_CTL_CLEAR_TC_EN()          (pDevice->pRegisters->PixcCtrl  &= 0x3F7F)
/* Get IMG_STAT (Image FIFO Status)                                                         */
#define ADI_PIXC_CTL_GET_IMG_FIFO_STAT()    ((pDevice->pRegisters->PixcCtrl &  0x0300) >> 8)
/* Get OVR_STAT (Overlay FIFO Status)                                                       */
#define ADI_PIXC_CTL_GET_OVR_FIFO_STAT()    ((pDevice->pRegisters->PixcCtrl &  0x0C00) >> 10)
/* Get WM_LVL (Watermark Level)                                                             */
#define ADI_PIXC_CTL_GET_WM_LVL()           ((pDevice->pRegisters->PixcCtrl &  0x3000) >> 12)
/* Get Overlay A & B status                                                                 */
#define ADI_PIXC_CTL_GET_OVR_EN_STAT        (pDevice->pRegisters->PixcCtrl  &  0x0006)

/* Macros to manipulate bits/fields of PIXC Interrupt status register                       */
/* Set OVR_INT_EN & FRM_INT_EN(Enables Overlay comple and Frame complete IRQs)              */
#define ADI_PIXC_INTRSTAT_SET_INT_EN()          (pDevice->pRegisters->PixcIntrStat |= 0x0003)
/* Clear OVR_INT_EN & FRM_INT_EN(Disables Overlay complete and Frame complete IRQs)         */
#define ADI_PIXC_INTRSTAT_CLEAR_INT_EN()        (pDevice->pRegisters->PixcIntrStat &= ~0x0003)
/* Clear OVR_INT_STAT (clear Overlay Interrupt status)                                      */
#define ADI_PIXC_INTRSTAT_CLEAR_OVR_INT_STAT()  (pDevice->pRegisters->PixcIntrStat |= 0x0004)
/* Clear FRM_INT_STAT (clear Frame Interrupt status)                                        */
#define ADI_PIXC_INTRSTAT_CLEAR_FRM_INT_STAT()  (pDevice->pRegisters->PixcIntrStat |= 0x0008)
/* Get Overlay IRQ status                                                                   */
#define ADI_PIXC_INTRSTAT_GET_OVR_IRQ_STAT      (pDevice->pRegisters->PixcIntrStat & 0x0004)
/* Get Frame IRQ status                                                                     */
#define ADI_PIXC_INTRSTAT_GET_FRM_IRQ_STAT      (pDevice->pRegisters->PixcIntrStat & 0x0008)
/* Get OVR_INT_EN & FRM_INT_EN bit status                                                   */
#define ADI_PIXC_INTRSTAT_GET_INT_EN            (pDevice->pRegisters->PixcIntrStat & 0x0003)

/* Shift count to reach Image data format(IMG_FORM) bit in PIXC control register            */
#define ADI_PIXC_CTL_SHIFT_IMG_FORM         	3
/* Length of Transparent color component field in PIXC Transparency color value register    */
#define ADI_PIXC_TC_COMPONENT_FIELD_LEN     	8

/*********************************************************************

Macros to mask reserved/selected bits in PIXC MMRs

*********************************************************************/
/* Masks Transparent color component bit field in PIXC Transparent color value register     */
#define ADI_PIXC_MASK_TC_FIELD              0x000000FF

/* Macros to mask reserved bits in PIXC MMRs                                                */
/* Masks reserved and read-only loactions in PIXC control register                          */
#define ADI_PIXC_MASK_CTL                   0x0FFF
/* Masks reserved loactions in PIXC Overlay A/B Horizontal start/end registers              */
#define ADI_PIXC_MASK_OVR_HORIZONTAL        0x0FFF
/* Masks reserved loactions in PIXC Overlay A/B Vertical start/end registers                */
#define ADI_PIXC_MASK_OVR_VERTICAL          0x03FF
/* Masks reserved loactions in PIXC Overlay A/B Transparency value registers                */
#define ADI_PIXC_MASK_OVR_TRANSPARENT       0x000F
/* Masks reserved loactions in PIXC Interrupt status register                               */
#define ADI_PIXC_MASK_INTRSTAT              0x0003
/* Masks reserved loactions in PIXC Color Conversion Coefficient registers                  */
#define ADI_PIXC_MASK_CC_COEFFICIENT        0x7FFFFFFF
/* Masks reserved loactions in PIXC Color Conversion Bias register                          */
#define ADI_PIXC_MASK_CC_BIAS               0x3FFFFFFF
/* Masks reserved loactions in PIXC Transparency color value register                       */
#define ADI_PIXC_MASK_TC                    0x00FFFFFF

/*********************************************************************

Data Structures

*********************************************************************/

/********************
    Moab  
********************/

#if defined(__ADSP_MOAB__)          /* PIXC register structure for Moab class devices       */

typedef struct {                    /* structure representing PIXC registers in memory      */
    volatile u16    PixcCtrl;       /* Control register                                     */
    u16             Padding0;
    volatile u16    PixcPpl;        /* Pixel per line                                       */
    u16             Padding1;
    volatile u16    PixcLpf;        /* Lines per frame                                      */
    u16             Padding2;
    volatile u16    PixcAHStart;    /* Horizantal start pixel info for overlay data set A   */
    u16             Padding3;
    volatile u16    PixcAHEnd;      /* Horizantal end pixel info for overlay data set A     */
    u16             Padding4;
    volatile u16    PixcAVStart;    /* Vertical start pixel info for overlay data set A     */
    u16             Padding5;
    volatile u16    PixcAVEnd;      /* Vertical end pixel info for overlay data set A       */
    u16             Padding6;
    volatile u16    PixcATransp;    /* Transparency ratio for overlay data set A            */
    u16             Padding7;
    volatile u16    PixcBHStart;    /* Horizantal start pixel info for overlay data set B   */
    u16             Padding8;
    volatile u16    PixcBHEnd;      /* Horizantal end pixel info for overlay data set B     */
    u16             Padding9;
    volatile u16    PixcBVStart;    /* Vertical start pixel info for overlay data set B     */
    u16             Padding10;
    volatile u16    PixcBVEnd;      /* Vertical end pixel info for overlay data set B       */
    u16             Padding11;
    volatile u16    PixcBTransp;    /* Transparency ratio for overlay data set B            */    
    u16             Padding13;
    u32             Padding14;
    u32             Padding15;
    volatile u16    PixcIntrStat;   /* Overlay interrupt configuration/status               */
    u16             Padding16;
    volatile s32    PixcRYCon;      /* R/Y coefficients for color space conversion matrix   */
    volatile s32    PixcGUCon;      /* G/U coefficients for color space conversion matrix   */
    volatile s32    PixcBVCon;      /* B/V coefficients for color space conversion matrix   */
    volatile s32    PixcCCBias;     /* Bias values for color space conversion matrix        */
    volatile u32    PixcTC;         /* Transperent color value                              */
}ADI_PIXC_REGISTERS;

#endif                              /* End of Moab specific Data Structures                 */

typedef struct {                                /* PIXC device structure                    */
    ADI_PIXC_REGISTERS      *pRegisters;        /* Base address of PIXC registers           */
    void                    *pEnterCriticalArg; /* critical region argument                 */
    u8                      InUseFlag;          /* in use flag (in use when TRUE)           */
    u8                      DataflowFlag;       /* Dataflow flag (TRUE when dataflow is on  */
    u8                      DeviceNumber;       /* This device number                       */
    u8						ITUR656EnableFlag;	/* Indicates ITU-R 656 support status		*/
    u8						CCTableIndex;		/* Color Conversion table index to use		*/
    ADI_INT_PERIPHERAL_ID   StatusPeripheralID; /* peripheral ID of PIXC's status interrupt */
    ADI_DEV_DMA_INFO        *pPixcInboundDma;   /* pointer to PIXC Inbound DMA channel info */
    ADI_DEV_DMA_INFO        *pPixcOutboundDma;  /* pointer to PIXC Outbound DMA channel info*/
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;       /* Device manager handle                    */
    void (*DMCallback) (                        /* Device manager's Callback function       */
        ADI_DEV_DEVICE_HANDLE DeviceHandle,     /* device handle                            */
        u32 Event,                              /* event ID                                 */
        void *pArg);                            /* argument pointer                         */
} ADI_PIXC;

/*********************************************************************

Device specific data

*********************************************************************/

/********************
    Moab  
********************/

#if defined(__ADSP_MOAB__)                  /* settings for Moab class devices              */

/* PIXC Devices                                                                             */
static ADI_PIXC Device[] = {                /* Actual PIXC devices                          */
    {                                       /* Device 0                                     */
        (ADI_PIXC_REGISTERS *)(0xffc04400), /* PIXC register base address                   */
        NULL,                               /* critical region argument                     */
        FALSE,                              /* in use flag                                  */
        FALSE,                              /* Dataflow is off by default                   */
        0,                                  /* This is Device 0                             */
        FALSE,								/* ITU-R 656 support disabled by default		*/
        3,									/* CC Table index points to special usage cases	*/
        ADI_INT_PIXC_ERROR,                	/* Peripheral ID of PIXC's status interrupt     */
        NULL,                               /* pointer to PIXC Inbound DMA channel info     */
        NULL,                               /* pointer to PIXC Outbound DMA channel info    */
        NULL,                               /* Device manager handle                        */
        NULL,                               /* Device manager Callback function             */
    },
};

/* PIXC DMA Channel Information Tables                                                      */
/* DMA channel information table for PIXC Image DMA (Inbound)                               */
static ADI_DEV_DMA_INFO PixcImageDma[]  = { /* Actual PIXC devices                          */
            {                               /* Device 0                                     */
                ADI_DMA_PMAP_PIXC_IMAGE,    /* PIXC Image DMA peripheral mapping ID         */
                NULL,                       /* DMA channel handle                           */
                FALSE,                      /* switch mode disabled by default              */
                NULL,                       /* pointer to switch queue head                 */
                NULL,                       /* pointer to switch queue tail                 */
                NULL,                       /* No further DMA channels to link              */
            },
};

/* DMA channel information table for PIXC Overlay DMA (Inbound)                             */
static ADI_DEV_DMA_INFO PixcOverlayDma[]  = {   /* Actual PIXC devices                      */
            {                                   /* Device 0                                 */
                ADI_DMA_PMAP_PIXC_OVERLAY,      /* PIXC Overlay DMA peripheral mapping ID   */
                NULL,                           /* DMA channel handle                       */
                FALSE,                          /* switch mode disabled by default          */
                NULL,                           /* pointer to switch queue head             */
                NULL,                           /* pointer to switch queue tail             */
                NULL,                           /* No further DMA channels to link          */
            },
};

/* DMA channel information table for PIXC Output DMA (Outbound)                             */
static ADI_DEV_DMA_INFO PixcOutputDma[]  = {/* Actual PIXC devices                          */
            {                               /* Device 0                                     */
                ADI_DMA_PMAP_PIXC_OUTPUT,   /* PIXC Output DMA peripheral mapping ID        */
                NULL,                       /* DMA channel handle                           */
                FALSE,                      /* switch mode disabled by default              */
                NULL,                       /* pointer to switch queue head                 */
                NULL,                       /* pointer to switch queue tail                 */
                NULL,                       /* No further DMA channels to link              */
            },
};

#endif                                      /* End of Moab specific data                    */

/*********************************************************************

Table of Color conversion coefficients for PIXC color conversion modes

*********************************************************************/
typedef struct {            /* Structure to hold color conversion coefficient/bias values   */
    s32     ry_coeff;       /* R/Y conversion coefficient                                   */
    s32     gu_coeff;       /* G/U conversion coefficient                                   */
    s32     bv_coeff;       /* B/V conversion coefficient                                   */
    s32     cc_bias;        /* Color conversion Bias                                        */
} ADI_PIXC_CC_VALUES;

/* Color Conversion coefficient table derived from PixC HRM matrix */
/* The table will be used as default and supports YUV422(VYUY format) <-> RGB888 conversion */
const static ADI_PIXC_CC_VALUES DefaultColorConversion [] = 
{
    /* Imperative that the following CC coefficients remain in this exact order */
    /* YUV422 to RGB888 conversion coefficients         */
	{   0x4B300080, 0x7A5F5080, 0x40038880, 0x31E21F4D  },
	/* RGB888 to YUV422 conversion coefficients         */
    {   0x04A4B499, 0x0FFD5FAA, 0x3D7CA8FF, 0x08020000  },
    /* conversion coefficients - special usage cases    */
    {   1,          1,          1,          1           },
};

/* Color Conversion coefficient table to support ITU-R 656 type YUV422 data */
/* The table holds color conversion coefficients that support
   ITU-R 656 type (UYVY format) YUV422 <-> RGB888 conversion */
const static ADI_PIXC_CC_VALUES ITUR656ColorConversion [] = 
{
    /* Imperative that the following CC coefficients remain in this exact order */
    /* YUV422 to RGB888 conversion coefficients 
      (Swap colums 2 & 3 of Default CC Matrix)*/
	{   0x4002CC80, 0x7D4E9480, 0x4E200080, 0x31E21F4D  },
	/* RGB888 to YUV422 conversion coefficients 
	   (Swap Rows 2 & 3 of Default CC Matrix & CC Bias Matrix)  */
    {   0x04A4B499, 0x3D7CA8FF, 0x0FFD5FAA, 0x08020000  },
    /* conversion coefficients - special usage cases    */
    {   1,          1,          1,          1           },
};

/*********************************************************************

Tables for PIXC register field access

*********************************************************************/

typedef struct {            /* structure to access bit fields in PIXC registers             */
    u32 Mask;               /* mask to access the corresponding register field              */
    u16 ShiftCount;         /* Count to reach starting bit of the field within the register */
} ADI_PIXC_REG_FIELDS;

/* The information in table below must match exactly the order of command IDs in adi_pixc.h */
/* This table is indexed with the command ID. So any changes in either the command IDs or 
   the table below must be reflected in the other */

/* Table to access bit fields in Color conversion registers */
const static ADI_PIXC_REG_FIELDS CCfields[] = {
    {   0x000003FF, 0       },  /* R/Y, G/U, B/V, CC Bias registers element 1 field */
    {   0x000FFC00, 10      },  /* R/Y, G/U, B/V, CC Bias registers element 2 field */
    {   0x3FF00000, 20      },  /* R/Y, G/U, B/V, CC Bias registers element 3 field */
    {   0x40000000, 30      },  /* R/Y, G/U, B/V registers Multi 4 bit field        */
};

/*********************************************************************

Static functions

*********************************************************************/

static u32 adi_pdd_Open(                        /* Opens PIXC device                        */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle                    */
    u32                     DeviceNumber,       /* PIXC Device number to open               */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                            */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle location           */
    ADI_DEV_DIRECTION       Direction,          /* data direction                           */
    void                    *pEnterCriticalArg, /* critical region storage location         */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager                */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                          */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function         */
);

static u32 adi_pdd_Close(                       /* Closes a PIXC device                     */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the PIXC device to close   */
);

static u32 adi_pdd_Read(                        /* Reads data/queues inbound buffer to PIXC */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a PIXC device              */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);
    
static u32 adi_pdd_Write(                       /*Writes data/queues outbound buffer to PIXC*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a PIXC device              */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_SequentialIO(                /* Sequentially read/writes data to a device*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a PIXC device              */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_Control(                     /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a PIXC device              */
    u32                     Command,            /* Command ID                               */
    void                    *Value              /* Command specific value                   */
);

static u32 PixcUpdateCCmode(                    /* Updates PIXC Color conversion Table index*/
    ADI_PIXC_CONVERSION_MODE *CCmode,           /* pointer to structure holding CC mode info*/
    ADI_PIXC                 *pDevice           /* pointer to the device we're working on   */
);

static void PixcUpdateCCRegs(                   /* Updates PIXC color conversion registers 	*/
    ADI_PIXC                 *pDevice           /* pointer to the device we're working on   */
);
                    	
static u32 UpdatePixcDmaUsage(                  /* Updates PIXC DMA channel usage           */
    ADI_PIXC                *pDevice            /* pointer to the device we're working on   */
);

static ADI_INT_HANDLER(PixcIntHandler);         /* PIXC Frame and Overlay interrupt handler */

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

static u32 ValidatePDDHandle(                   /* Validates Physical Device Driver Handle  */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of a PIXC device              */
);

static u32 ValidatePixcRegs(                    /* Validates PIXC register values           */
    ADI_PIXC                *pDevice            /* pointer to the device we're working on   */
);

#endif

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADIPIXCEntryPoint = {
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
*   Description:    Opens a PIXC device for use
*
*********************************************************************/

static u32 adi_pdd_Open(                        /* Open PIXC device                 */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle            */
    u32                     DeviceNumber,       /* PIXC Device number to open       */
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
    ADI_PIXC    *pDevice;
    /* exit critical region parameter                       */
    void        *pExitCriticalArg;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    if (DeviceNumber >= ADI_PIXC_NUM_DEVICES)       /* check the device number */
    {
        Result = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;  /* Invalid Device number */
    }
    
    /*  Continue only if the device number is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* insure the device the client wants is available  */
        Result  = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* PIXC device we're working on */
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
        
        /* Continue only when the PIXC device is available for use  */
        if (Result == ADI_DEV_RESULT_SUCCESS) 
        {    
            /* PIXC device is reserved for this client      */

            /* Reset the actual device                      */
            /* Configure PIXC MMRs to power-on/reset value  */
            pDevice->pRegisters->PixcCtrl		= 0;
            pDevice->pRegisters->PixcPpl		= 0;
			pDevice->pRegisters->PixcLpf		= 0;
			pDevice->pRegisters->PixcAHStart	= 0;
			pDevice->pRegisters->PixcAHEnd		= 0;
			pDevice->pRegisters->PixcAVStart	= 0;
			pDevice->pRegisters->PixcAVEnd		= 0;
			pDevice->pRegisters->PixcATransp	= 0;
			pDevice->pRegisters->PixcBHStart	= 0;
			pDevice->pRegisters->PixcBHEnd		= 0;
			pDevice->pRegisters->PixcBVStart	= 0;
			pDevice->pRegisters->PixcBVEnd		= 0;
			pDevice->pRegisters->PixcBTransp	= 0;
			pDevice->pRegisters->PixcIntrStat	= 0;
			pDevice->pRegisters->PixcRYCon		= 0;
			pDevice->pRegisters->PixcGUCon		= 0;
			pDevice->pRegisters->PixcBVCon		= 0;
			pDevice->pRegisters->PixcCCBias		= 0;
			pDevice->pRegisters->PixcTC			= 0;

            /* update PIXC DMA channel information */
            /* all mainstream DMA channels must be opened by default */    
            /* Mainstream Inbound DMA - PIXC Output DMA */            
            pDevice->pPixcInboundDma        = &PixcOutputDma[pDevice->DeviceNumber];
            pDevice->pPixcInboundDma->pNext = NULL;
            /* Mainstream Outbound DMA - PIXC Image DMA */
            pDevice->pPixcOutboundDma       = &PixcImageDma[pDevice->DeviceNumber];
            pDevice->pPixcOutboundDma->pNext= NULL;
            
            /* initialize the device settings               */
            /* Pointer to critical region                   */
            pDevice->pEnterCriticalArg  = pEnterCriticalArg;
            /* Pointer to Device handle                     */
            pDevice->DeviceHandle       = DeviceHandle;
            /* Pointer to Device manager callback function  */
            pDevice->DMCallback         = DMCallback;
        
            /* save the physical device handle in the client supplied location */
            *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;
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
*   Description:    Closes down a PIXC device
*
*********************************************************************/

static u32 adi_pdd_Close(                       /* Closes a PIXC device                     */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the PIXC device to close   */
)
{  
    /* Return value - assume we're going to be successful   */
    u32         Result = ADI_DEV_RESULT_SUCCESS;
    /* pointers to the device we're working on   */
    ADI_PIXC    *pDevice,*pTmpDevice;
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
        pDevice = (ADI_PIXC *)PDDHandle;
        
        /* disable PIXC dataflow */
        Result = adi_pdd_Control (PDDHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)FALSE);
        
        /* continue ony if PIXC dataflow is disabled */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {            
            /* mark this PIXC device as closed */
            pDevice->InUseFlag = FALSE;
            /* unhook the PIXC status interrupt handler from the system */
            Result = adi_pdd_Control(PDDHandle, ADI_PIXC_CMD_ENABLE_STATUS_REPORT, (void *)FALSE);
                    
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
*   Description:    Never called as PIXC uses DMA
*
*********************************************************************/
static u32 adi_pdd_Read(                /* Reads data/queues inbound buffer to PIXC */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a PIXC device              */
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
*   Description:    Never called as PIXC uses DMA
*
*********************************************************************/
static u32 adi_pdd_Write(               /*Writes data/queues outbound buffer to PIXC*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a PIXC device              */
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
*   Description:    PIXC does not support this function
*
*********************************************************************/
static u32 adi_pdd_SequentialIO(        /* Never called as PIXC uses DMA            */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a PIXC device              */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer                        */
){
    
    /* this function is not supported by PIXC */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_Control
*
*   Description:    Senses or Configures PIXC device registers
*
*********************************************************************/
static u32 adi_pdd_Control(             /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE  PDDHandle,      /* PDD handle of a PIXC device              */
    u32                 Command,        /* Command ID                               */
    void                *Value          /* Command specific value                   */
){
    
    ADI_PIXC            *pDevice;       /* pointer to the device we're working on   */
    u32                 Result;         /* return value                             */
    volatile s32        *ps32Reg;       /* pointer to signed 32 bit reg             */    
    s32                 s32Value;       /* s32 type to avoid casts/warnings etc.    */
    u32                 u32Value;       /* u32 type to avoid casts/warnings etc.    */    
    u16                 u16Value;       /* u16 type to avoid casts/warnings etc.    */
    s32                 s32temp;        /* temp s32 location                        */
    u32                 u32temp;        /* temp u32 location                        */        
    u16                 u16temp;        /* temp u16 location                        */    
    u8                  UpdateDMAFlag;  /* DMA status update indicator              */
    u32                 StatusIVG;      /* IVG for PIXC status interrupts           */
    
    /* assume we're going to be successful */
    Result          = ADI_DEV_RESULT_SUCCESS;
        
    /* avoid casts */
    pDevice         = (ADI_PIXC *)PDDHandle;
    /* assign 16 and 32 bit values for the Value argument */
    s32Value        = (s32)Value; 
    u32Value        = (u32)Value;
    u16Value        = (u16)u32Value;
    /* Default - No need to update DMA channel list */
    UpdateDMAFlag   = FALSE;

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
                    /* validate PIXC register values */
                    Result = ValidatePixcRegs (pDevice);
                    /* Continue only when PIXC register values are valid */
                    if (Result != ADI_DEV_RESULT_SUCCESS)
                    {
                      break;
                    }
#endif
                    /* Set PIXC_EN (Enable Pixel Compositor) */
                    ADI_PIXC_CTL_SET_PIXC_EN();
                    pDevice->DataflowFlag = TRUE;
                }
                else
                {
                    /* Clear PIXC_EN (Disable Pixel Compositor) */
                    ADI_PIXC_CTL_CLEAR_PIXC_EN();
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
                          
                /* pass PIXC Inbound DMA channel information to Device manager */
                *((u32 *)Value) = (u32)pDevice->pPixcInboundDma;
                break;

            /* CASE: query for peripheral outbound DMA channel information */
            case (ADI_DEV_CMD_GET_OUTBOUND_DMA_INFO):
                          
                /* pass PIXC Outbound DMA channel information to Device manager */
                *((u32 *)Value) = (u32)pDevice->pPixcOutboundDma;
                break;
                            
            /* CASE: Configure PIXC registers? */
            default:
            
               /* Make sure that this command is ours */
                if ((Command >= ADI_PIXC_CMD_START) && (Command <= ADI_PIXC_CMD_END))
                {
                    /* Make sure to disable PIXC dataflow before configuring PIXC registers */
                    /* Also make sure not to disable PIXC dataflow while servicing an interrupt */
                    if ((pDevice->DataflowFlag) && (ADI_PIXC_INTRSTAT_GET_OVR_IRQ_STAT) && (ADI_PIXC_INTRSTAT_GET_FRM_IRQ_STAT))
                    {
                        /* Clear PIXC_EN (Disable Pixel Compositor) */
                        ADI_PIXC_CTL_CLEAR_PIXC_EN();
                    }
                }
                /* This command is not supported by us. return error */
                else
                {
                    /* PIXC doesn't understand this command */
                    Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
                }

                /* CASEOF (Command ID)  */
                switch (Command)
                {
                    /* CASE: Update PIXC Color conversion table index and set CC registers in selected color conversion mode */
                    case (ADI_PIXC_CMD_SET_COLOR_CONVERSION_MODE):
            
                        /* Update PIXC color conversion table index */
                        if ((Result = PixcUpdateCCmode((ADI_PIXC_CONVERSION_MODE *)Value,pDevice)) != ADI_DEV_RESULT_SUCCESS)
                        {
                            break;
                        }
                        /* update DMA channel usage */
                        Result = UpdatePixcDmaUsage(pDevice);
						break;
						
					/* CASE: Set PIXC registers to support color conversion of 
    						 ITU-R 656 (UYVY) type YUV422 frame to/from RGB888 frame */
                    case (ADI_PIXC_CMD_ENABLE_ITUR656_SUPPORT):

                    	/* update the ITU-R 656 enable flag status */
                    	pDevice->ITUR656EnableFlag = (u8)u32Value;
                    	/* update PIXC color conversion registers */
                    	PixcUpdateCCRegs(pDevice);
                    	break;

                    /* CASE: Set PIXC control register (PIX_CTL) */
                    case (ADI_PIXC_CMD_SET_CONTROL_REG):
        
                        pDevice->pRegisters->PixcCtrl       = (u16Value & ADI_PIXC_MASK_CTL);
                        /* update DMA channel usage */
                        Result = UpdatePixcDmaUsage(pDevice);
                    break;

                    /* CASE: Set PIXC Pixel Per Line register (PIXC_PLL)    */
                    case (ADI_PIXC_CMD_SET_PIXELS_PER_LINE):
                    
                        pDevice->pRegisters->PixcPpl       = u16Value;
                        break;
        
                    /* CASE: Set PIXC Lines Per Frame register (PIXC_LPF) */
                    case (ADI_PIXC_CMD_SET_LINES_PER_FRAME):
                    
                        pDevice->pRegisters->PixcLpf       = u16Value;
                        break;
                     
                    /* CASE: Set PIXC Overlay A Horizontal start (PIXC_AHSTART) */
                    case (ADI_PIXC_CMD_SET_OVERLAY_A_HSTART):
                    /* CASE: Set PIXC Overlay A Horizontal end (PIXC_AHEND) */
                    case (ADI_PIXC_CMD_SET_OVERLAY_A_HEND):
                    /* CASE: Set PIXC Overlay B Horizontal start (PIXC_BHSTART) */
                    case (ADI_PIXC_CMD_SET_OVERLAY_B_HSTART):
                    /* CASE: Set PIXC Overlay B Horizontal end (PIXC_BHEND) */
                    case (ADI_PIXC_CMD_SET_OVERLAY_B_HEND):
                    
                        /* Mask the reserved bits */
                        u16Value &= ADI_PIXC_MASK_OVR_HORIZONTAL;
                        /* update the corresponding register with new value */
                        if (Command == ADI_PIXC_CMD_SET_OVERLAY_A_HSTART)
                        {
                            pDevice->pRegisters->PixcAHStart   = u16Value;
                        }
                        else if (Command == ADI_PIXC_CMD_SET_OVERLAY_A_HEND)
                        {
                            pDevice->pRegisters->PixcAHEnd     = u16Value;
                        }
                        else if (Command == ADI_PIXC_CMD_SET_OVERLAY_B_HSTART)
                        {
                            pDevice->pRegisters->PixcBHStart   = u16Value;
                        }
                        else
                        {
                            pDevice->pRegisters->PixcBHEnd     = u16Value;
                        }                                                
                        break;
         
                    /* CASE: Set PIXC Overlay A Vertical start (PIXC_AVSTART)  */
                    case (ADI_PIXC_CMD_SET_OVERLAY_A_VSTART):
                    /* CASE: Set PIXC Overlay A Vertical end (PIXC_AVEND) */
                    case (ADI_PIXC_CMD_SET_OVERLAY_A_VEND):
                    /* CASE: Set PIXC Overlay B Vertical start (PIXC_BVSTART)  */
                    case (ADI_PIXC_CMD_SET_OVERLAY_B_VSTART):
                    /* CASE: Set PIXC Overlay B Vertical end (PIXC_BVEND) */
                    case (ADI_PIXC_CMD_SET_OVERLAY_B_VEND):
                    
                        /* Mask the reserved bits */
                        u16Value &= ADI_PIXC_MASK_OVR_VERTICAL;
                        /* update the corresponding register with new value */
                        if (Command == ADI_PIXC_CMD_SET_OVERLAY_A_VSTART)
                        {
                            pDevice->pRegisters->PixcAVStart   = u16Value;
                        }
                        else if (Command == ADI_PIXC_CMD_SET_OVERLAY_A_VEND)
                        {
                            pDevice->pRegisters->PixcAVEnd     = u16Value;
                        }
                        else if (Command == ADI_PIXC_CMD_SET_OVERLAY_B_VSTART)
                        {
                            pDevice->pRegisters->PixcBVStart   = u16Value;
                        }
                        else
                        {
                            pDevice->pRegisters->PixcBVEnd     = u16Value;
                        }                                                
                        break;

                    /* CASE: Set PIXC Overlay A Transparency value (PIXC_ATRANSP) */
                    case (ADI_PIXC_CMD_SET_OVERLAY_A_TRANSPARENCY):
                    /* CASE: Set PIXC Overlay B Transparency value (PIXC_BTRANSP) */
                    case (ADI_PIXC_CMD_SET_OVERLAY_B_TRANSPARENCY):

                        /* Mask the reserved bits */
                        u16Value &= ADI_PIXC_MASK_OVR_TRANSPARENT;
                        /* update the corresponding register with new value */
                        if (Command == ADI_PIXC_CMD_SET_OVERLAY_A_TRANSPARENCY)
                        {
                            pDevice->pRegisters->PixcATransp   = u16Value;
                        }
                        else
                        {
                            pDevice->pRegisters->PixcBTransp   = u16Value;
                        }
                        break;
  
                    /* CASE: Set R/Y coefficients for color space conversion matrix (PIXC_RYCON) */
                    case (ADI_PIXC_CMD_SET_RY_CONVERSION_COEFFICIENT):
                    /* CASE: Set G/U coefficients for color space conversion matrix (PIXC_GUCON) */
                    case (ADI_PIXC_CMD_SET_GU_CONVERSION_COEFFICIENT):
                    /* CASE: Set B/V coefficients for color space conversion matrix (PIXC_BVCON) */
                    case (ADI_PIXC_CMD_SET_BV_CONVERSION_COEFFICIENT):

                        /* Mask the reserved bits */
                        s32Value &= ADI_PIXC_MASK_CC_COEFFICIENT;
                        /* update the corresponding register with new value */
                        if (Command == ADI_PIXC_CMD_SET_RY_CONVERSION_COEFFICIENT)
                        {
                            pDevice->pRegisters->PixcRYCon     = s32Value;
                        }
                        else if (Command == ADI_PIXC_CMD_SET_GU_CONVERSION_COEFFICIENT)
                        {
                            pDevice->pRegisters->PixcGUCon     = s32Value;
                        }
                        else
                        {
                            pDevice->pRegisters->PixcBVCon     = s32Value;
                        }
                        break;

                    /* CASE: Set color conversion bias for color conversion matrix (PIXC_CCBIAS) */
                    case (ADI_PIXC_CMD_SET_COLOR_CONVERSION_BIAS):
        
                        pDevice->pRegisters->PixcCCBias    = (s32Value & ADI_PIXC_MASK_CC_BIAS);
                        break;

                    /* CASE: Set Transparency color value register */
                    case (ADI_PIXC_CMD_SET_TRANSPARENCY_COLOR):
        
                        pDevice->pRegisters->PixcTC        = (u32Value & ADI_PIXC_MASK_TC);
                        break;
   
                    /* CASE: Set Overlay A Enable */
                    case (ADI_PIXC_CMD_SET_OVERLAY_A_ENABLE):
    
                        if (u16Value == TRUE)
                        {
                            /* Enable Overlay A */
                            ADI_PIXC_CTL_SET_OVR_A_EN();
                        }
                        else
                        {
                            /* Disable Overlay A */
                            ADI_PIXC_CTL_CLEAR_OVR_A_EN();
                        }
                        /* update DMA channel usage */
                        Result = UpdatePixcDmaUsage(pDevice);
                        break;
        
                    /* CASE: Set Overlay B Enable */
                    case (ADI_PIXC_CMD_SET_OVERLAY_B_ENABLE):
    
                        if (u16Value == TRUE)
                        {
                            /* Enable Overlay B */
                            ADI_PIXC_CTL_SET_OVR_B_EN();                 
                        }
                        else
                        {
                            /* Disable Overlay B */
                            ADI_PIXC_CTL_CLEAR_OVR_B_EN();
                        }
                        /* update DMA channel usage */
                        Result = UpdatePixcDmaUsage(pDevice);
                        break;
           
                    /* CASE: Set Transparency color enable */
                    case (ADI_PIXC_CMD_SET_TRANSPARENCY_COLOR_ENABLE):
    
                        if (u16Value == TRUE)
                        {
                            /* Enable Transparency */
                            ADI_PIXC_CTL_SET_TC_EN();
                        }
                        else
                        {
                            /* Disable Transparency */
                            ADI_PIXC_CTL_CLEAR_TC_EN();
                        }
                        break;  
           
                    /* CASE: Set Image Data format to YUV */
                    case (ADI_PIXC_CMD_SET_IMAGE_DATA_YUV):
                    /* CASE: Set Overlay Data format to YUV */
                    case (ADI_PIXC_CMD_SET_OVERLAY_DATA_YUV):
                    /* CASE: Set Output Data format to YUV */
                    case (ADI_PIXC_CMD_SET_OUTPUT_DATA_YUV):
                    /* CASE: Set resampling mode to duplicate for up and downsampling */
                    case (ADI_PIXC_CMD_SET_RESAMPLE_MODE_DUPLICATE):

                        /* generate shift count to reach the corresponding bit location in PIXC_CTL */        
                        u16temp = ((Command - ADI_PIXC_CMD_SET_IMAGE_DATA_YUV) + ADI_PIXC_CTL_SHIFT_IMG_FORM);
                        /* clear the corresponding PIXC_CTL field */
                        pDevice->pRegisters->PixcCtrl  &= (~(1 << u16temp));        
                        break;
        
                    /* CASE: Set Image Data format to RGB */
                    case (ADI_PIXC_CMD_SET_IMAGE_DATA_RGB):
                    /* CASE: Set Overlay Data format to RGB */
                    case (ADI_PIXC_CMD_SET_OVERLAY_DATA_RGB):
                    /* CASE: Set Output Data format to RGB */
                    case (ADI_PIXC_CMD_SET_OUTPUT_DATA_RGB):
                    /* CASE: Set resampling mode to average for up and downsampling */
                    case (ADI_PIXC_CMD_SET_RESAMPLE_MODE_AVERAGE):
    
                        /* generate shift count to reach the corresponding bit location in PIXC_CTL */        
                        u16temp = ((Command - ADI_PIXC_CMD_SET_IMAGE_DATA_RGB) + ADI_PIXC_CTL_SHIFT_IMG_FORM);
                        /* set the corresponding PIXC_CTL field */
                        pDevice->pRegisters->PixcCtrl  |= (1 << u16temp);        
                        break;             

                    /* CASE: Set RY conversion coefficient element 1  (A11) */
                    case (ADI_PIXC_CMD_SET_RY_CONVERSION_ELEMENT1):
                    /* CASE: Set RY conversion coefficient element 2  (A12) */
                    case (ADI_PIXC_CMD_SET_RY_CONVERSION_ELEMENT2):
                    /* CASE: Set RY conversion coefficient element 3  (A13) */
                    case (ADI_PIXC_CMD_SET_RY_CONVERSION_ELEMENT3):
                    /* CASE: Set RY multiply by 4 factor enable             */
                    case (ADI_PIXC_CMD_SET_RY_MULTIPLY4_ENABLE):
                    /* CASE: Set GU conversion coefficient element 1  (A21) */
                    case (ADI_PIXC_CMD_SET_GU_CONVERSION_ELEMENT1):
                    /* CASE: Set GU conversion coefficient element 2  (A22) */
                    case (ADI_PIXC_CMD_SET_GU_CONVERSION_ELEMENT2):
                    /* CASE: Set GU conversion coefficient element 3  (A23) */
                    case (ADI_PIXC_CMD_SET_GU_CONVERSION_ELEMENT3):
                    /* CASE: Set GU multiply by 4 factor enable             */
                    case (ADI_PIXC_CMD_SET_GU_MULTIPLY4_ENABLE):
                    /* CASE: Set BV conversion coefficient element 1  (A31) */
                    case (ADI_PIXC_CMD_SET_BV_CONVERSION_ELEMENT1):
                    /* CASE: Set BV conversion coefficient element 2  (A32) */
                    case (ADI_PIXC_CMD_SET_BV_CONVERSION_ELEMENT2):
                    /* CASE: Set BV conversion coefficient element 3  (A33) */
                    case (ADI_PIXC_CMD_SET_BV_CONVERSION_ELEMENT3):
                    /* CASE: Set BV multiply by 4 factor enable             */
                    case (ADI_PIXC_CMD_SET_BV_MULTIPLY4_ENABLE):
                    /* CASE: Set BV conversion coefficient element 1  (A14) */
                    case (ADI_PIXC_CMD_SET_CONVERSION_BIAS_VECTOR1):
                    /* CASE: Set BV conversion coefficient element 2  (A24) */
                    case (ADI_PIXC_CMD_SET_CONVERSION_BIAS_VECTOR2):
                    /* CASE: Set BV conversion coefficient element 3  (A34) */
                    case (ADI_PIXC_CMD_SET_CONVERSION_BIAS_VECTOR3):
                                                
                        /* determine the register to address and point to the entry in the CCfields table */
                        if (Command <= ADI_PIXC_CMD_SET_RY_MULTIPLY4_ENABLE) 
                        {
                            /* register to access */
                            ps32Reg     = (volatile s32 *) &pDevice->pRegisters->PixcRYCon;
                            /* point to the entry in the CCfields table */
                            u16temp     = (Command - ADI_PIXC_CMD_SET_RY_CONVERSION_ELEMENT1);
                        }
                        else if (Command <= ADI_PIXC_CMD_SET_GU_MULTIPLY4_ENABLE) 
                        {
                            /* register to access */
                            ps32Reg     = (volatile s32 *) &pDevice->pRegisters->PixcGUCon;
                            /* point to the entry in the CCfields table */
                            u16temp     = (Command - ADI_PIXC_CMD_SET_GU_CONVERSION_ELEMENT1);
                        }
                        else if (Command <= ADI_PIXC_CMD_SET_BV_MULTIPLY4_ENABLE) 
                        {
                            /* register to access */
                            ps32Reg     = (volatile s32 *) &pDevice->pRegisters->PixcBVCon;
                            /* point to the entry in the CCfields table */
                            u16temp     = (Command - ADI_PIXC_CMD_SET_BV_CONVERSION_ELEMENT1);
                        }
                        else
                        {
                            /* register to access */
                            ps32Reg     = (volatile s32 *) &pDevice->pRegisters->PixcCCBias;
                            /* point to the entry in the CCfields table */
                            u16temp     = (Command - ADI_PIXC_CMD_SET_CONVERSION_BIAS_VECTOR1);
                        }
                
                        /* clear other bit field locations in the given client value */
                        s32Value    = ((s32Value << CCfields[u16temp].ShiftCount) & CCfields[u16temp].Mask);
                        /* extract the value of remaining bit fields */
                        s32temp     = (*ps32Reg & ~CCfields[u16temp].Mask);
                        /* update corresponding register with client value */
                        *ps32Reg    = (s32Value | s32temp);                
                        break;
                
                    /* CASE: Set Transparent color for RY component */
                    case (ADI_PIXC_CMD_SET_TRANSPARENT_COLOR_RY):
                    /* CASE: Set Transparent color for GU component */
                    case (ADI_PIXC_CMD_SET_TRANSPARENT_COLOR_GU):
                    /* CASE: Set Transparent color for BV component */
                    case (ADI_PIXC_CMD_SET_TRANSPARENT_COLOR_BV):
    
                        /* generate shift count to reach the corresponding bit location in PIXC_TC */        
                        u16temp     = ((Command - ADI_PIXC_CMD_SET_TRANSPARENT_COLOR_RY) * ADI_PIXC_TC_COMPONENT_FIELD_LEN);
                        /* clear other bit field locations in the given client value */
                        u32Value    = ((u32Value & ADI_PIXC_MASK_TC_FIELD) << u16temp);
                        /* extract the value of remaining bit fields */
                        u32temp     = (pDevice->pRegisters->PixcTC & ~(ADI_PIXC_MASK_TC_FIELD << u16temp));
                        /* update PIXC_TC register field with client value */
                        pDevice->pRegisters->PixcTC    = (u32Value | u32temp);
                        break;  


                    /* CASE: Enable PIXC interrupt status report */
                    case (ADI_PIXC_CMD_ENABLE_STATUS_REPORT):
                                
                        /* get the PIXC Status interrupt IVG */
                        adi_int_SICGetIVG(pDevice->StatusPeripheralID, &StatusIVG);
                    
                        /* IF (enabling) */
                        if (u32Value == TRUE) 
                        {
                            /* check if this IVG is already hooked to the system */
                            if (ADI_PIXC_INTRSTAT_GET_INT_EN)
                            {
                                /* PIXC interrupt status is already enabled. IVG must be already hooked to the system */
                                break;  
                            }
                            /* hook the PIXC Status interrupt handler to the system */
                            if((Result = adi_int_CECHook(StatusIVG, PixcIntHandler, pDevice, TRUE)) != ADI_INT_RESULT_SUCCESS)
                            {
                                break;
                            }
                            adi_int_SICWakeup(pDevice->StatusPeripheralID, TRUE);
                            adi_int_SICEnable(pDevice->StatusPeripheralID);
                            /* Enable Overlay complete and Frame Complete IRQs  */
                            ADI_PIXC_INTRSTAT_SET_INT_EN();
                        }
                        else
                        {   
                            /* check if this IVG is already unhooked from the system */
                            if (!ADI_PIXC_INTRSTAT_GET_INT_EN)
                            {
                                /* PIXC interrupt status is already disabled. IVG must be already unhooked from the system */
                                break;
                            }                        
                            /* Disable Overlay complete and Frame complete IRQs  */
                            ADI_PIXC_INTRSTAT_CLEAR_INT_EN();
                            /* unhook the PIXC status interrupt handler from the system */
                            adi_int_SICDisable(pDevice->StatusPeripheralID);
                            adi_int_SICWakeup(pDevice->StatusPeripheralID, FALSE);
                            /* unhook the PIXC Status interrupt handler from the system */
                            Result = adi_int_CECUnhook(StatusIVG, PixcIntHandler, pDevice);
                        }
                        break;
                /* ENDCASE for PIXC register configuration Command switch*/
                }
                
                /* make sure to re-enable PIXC if we're enabled */
                if (pDevice->DataflowFlag == TRUE)
                {
/* for Debug build only - Validate all register values before enabling Dataflow */
#if defined(ADI_DEV_DEBUG)
                    /* validate PIXC register values */
                    Result = ValidatePixcRegs (pDevice);
                    /* Continue only when PIXC register values are valid */
                    if (Result != ADI_DEV_RESULT_SUCCESS)
                    {
                        break;
                    }
#endif                    
                    /* Set PIXC_EN (Enable Pixel Compositor) */
                    ADI_PIXC_CTL_SET_PIXC_EN();
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

/*********************************************************************

    Function:       UpdatePixcDmaUsage

    Description:    Updates PIXC DMA channel usage status and
                    DMA channel information table

*********************************************************************/

static u32 UpdatePixcDmaUsage(      /* Updates PIXC DMA channel usage           */
    ADI_PIXC        *pDevice        /* pointer to the device we're working on   */
){
    /* Return value - assume we're going to be successful */
    u32 Result  = ADI_DEV_RESULT_SUCCESS;

    /* update DMA Channel status */
    /* check if overlay is enabled */
    if (ADI_PIXC_CTL_GET_OVR_EN_STAT && (PixcOverlayDma[pDevice->DeviceNumber].ChannelHandle == NULL))
    {
        /* request Device manager to open Overlay DMA for use */
        Result = adi_dev_Control(pDevice->DeviceHandle, ADI_DEV_CMD_OPEN_PERIPHERAL_DMA, (void *)&PixcOverlayDma[pDevice->DeviceNumber]);
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* Link this overlay DMA information table to Image DMA (Inbound DMA list) */
            PixcImageDma[pDevice->DeviceNumber].pNext   = &PixcOverlayDma[pDevice->DeviceNumber];
            /* Terminate the Overlay DMA table */
            PixcOverlayDma[pDevice->DeviceNumber].pNext = NULL;
        }
    } 
    /* check if overlay is disabled */
    else if ((!ADI_PIXC_CTL_GET_OVR_EN_STAT) && (PixcOverlayDma[pDevice->DeviceNumber].ChannelHandle != NULL))
    {
        /* request Device manager to close Overlay DMA */
        Result = adi_dev_Control(pDevice->DeviceHandle, ADI_DEV_CMD_CLOSE_PERIPHERAL_DMA, (void *)&PixcOverlayDma[pDevice->DeviceNumber]);
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* Remove the overlay DMA information table from Image DMA (Inbound DMA list) */
            PixcImageDma[pDevice->DeviceNumber].pNext   = PixcOverlayDma[pDevice->DeviceNumber].pNext;
        }
    }
            
    /* return */
    return (Result);
}  

/*********************************************************************

    Function:       PixcUpdateCCmode

    Description:    Updates PIXC color conversion table index (CCTableIndex) 
                    to match the client's color conversion mode

*********************************************************************/

static u32 PixcUpdateCCmode(               /* Updates PIXC Color conversion mode       */
    ADI_PIXC_CONVERSION_MODE    *CCmode,        /* pointer to structure holding CC mode info*/
    ADI_PIXC                    *pDevice        /* pointer to the device we're working on   */
){
    
    /* temp location to determine Overlay Frame type */
    ADI_PIXC_FRAME_TYPE  OverlayType;
    /* assume we're going to be successful */
    u32     Result      = ADI_DEV_RESULT_SUCCESS;

    /* check for valid Input/Output frame type */
    if ((CCmode->InputFrame >= ADI_PIXC_FRAME_DISABLE) ||
        (CCmode->OutputFrame >= ADI_PIXC_FRAME_DISABLE))
    {
        /* Invalid frame type */
        Result = ADI_PIXC_RESULT_FRAME_TYPE_INVALID;
    }
    /* check overlay frame conversion mode */ 
    else if ((CCmode->OverlayAFrame == ADI_PIXC_FRAME_DISABLE) && 
             (CCmode->OverlayBFrame == ADI_PIXC_FRAME_DISABLE))
    {
        /* Disable both Overlays */
        OverlayType = ADI_PIXC_FRAME_DISABLE;
        /* Clear OVR_A_EN and OVR_B_EN bits in PIXC_CTL register */
        ADI_PIXC_CTL_CLEAR_OVR_A_EN();
        ADI_PIXC_CTL_CLEAR_OVR_B_EN();
    }
    else if (CCmode->OverlayAFrame == ADI_PIXC_FRAME_DISABLE)
    {
        /* Disable Overlay A, Enable Overlay B */
        OverlayType = CCmode->OverlayBFrame;
        /* Clear OVR_A_EN bit in PIXC_CTL register */
        ADI_PIXC_CTL_CLEAR_OVR_A_EN();
        /* Set OVR_B_EN bit in PIXC_CTL register */
        ADI_PIXC_CTL_SET_OVR_B_EN();
    }    
    else if (CCmode->OverlayBFrame == ADI_PIXC_FRAME_DISABLE)
    {
        /* Enable Overlay A, Disable Overlay B */
        OverlayType = CCmode->OverlayAFrame;
        /* Set OVR_A_EN bit in PIXC_CTL register */
        ADI_PIXC_CTL_SET_OVR_A_EN();
        /* Clear OVR_B_EN bit in PIXC_CTL register */
        ADI_PIXC_CTL_CLEAR_OVR_B_EN();        
    }
    else if (CCmode->OverlayAFrame != CCmode->OverlayBFrame)
    {
        /* Invalid frame type */
        Result = ADI_PIXC_RESULT_FRAME_TYPE_INVALID;
    }
    else
    {
        /* Enable Overlay A and Overlay B */
        OverlayType = CCmode->OverlayAFrame;
        /* Set OVR_A_EN and OVR_B_EN bits in PIXC_CTL register */
        ADI_PIXC_CTL_SET_OVR_A_EN();
        ADI_PIXC_CTL_SET_OVR_B_EN();
    }

    /* Continue only when the given frame types are valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
        /* determine the color conversion mode */
        /* update PIXC_CTL register Image format bit */
        if (CCmode->InputFrame == ADI_PIXC_FRAME_YUV422)
        {
            /* Set PIXC_CTL Image format bit to YUV422 */
            ADI_PIXC_CTL_CLEAR_IMG_FORM_YUV();
        }
        else
        {
            /* Set PIXC_CTL Image format bit to RGB888 */
            ADI_PIXC_CTL_SET_IMG_FORM_RGB();
        }
            
        /* update PIXC_CTL register Overlay format bit */
        /* This bit is not valid when both overlays are disabled */
        if (OverlayType == ADI_PIXC_FRAME_YUV422)
        {
            /* Set PIXC_CTL Overlay format bit to YUV422 */
            ADI_PIXC_CTL_CLEAR_OVR_FORM_YUV();
        }
        else
        {
            /* Set PIXC_CTL Overlay format bit to RGB888 */
            ADI_PIXC_CTL_SET_OVR_FORM_RGB();
        }
            
        /* update PIXC_CTL register Output format bit */
        if (CCmode->OutputFrame == ADI_PIXC_FRAME_YUV422)
        {
            /* Set PIXC_CTL Output format bit to YUV422 */
            ADI_PIXC_CTL_CLEAR_OUT_FORM_YUV();
        }
        else
        {
            /* Set PIXC_CTL Output format bit to RGB888 */
            ADI_PIXC_CTL_SET_OUT_FORM_RGB();
        }
        
        /* Check if the conversion mode falls in to special usage case */
        if ((CCmode->InputFrame != ADI_PIXC_FRAME_YUV444) && 
            (OverlayType        != ADI_PIXC_FRAME_YUV444) &&
            (CCmode->OutputFrame!= ADI_PIXC_FRAME_YUV444))
        {
            /* This conversion mode is not a special case */          
            /* By default, point CC Field Index to YUV422 to RGB888 conversion coefficients */
            pDevice->CCTableIndex = 0;
                    
            /* check if Image and Output Frames are of same type */
            if (CCmode->InputFrame != CCmode->OutputFrame)
            {
                /* Conversion mode is RGB888 to YUV422 on Image Frame */
                if (CCmode->InputFrame == ADI_PIXC_FRAME_RGB888)
                {                    
                     /* Point CC Field Index to RGB888 to YUV422 conversion coefficients */
                    pDevice->CCTableIndex = 1;
                }
                /* else: Conversion mode is YUV422 to RGB888 on Image Frame */
            }
            /* Image and Output Frames are same type */
            else if ((OverlayType != CCmode->OutputFrame) && (OverlayType != ADI_PIXC_FRAME_DISABLE))
            {
                /* Conversion mode is RGB888 to YUV422 on Overlay Frame */
                if (OverlayType == ADI_PIXC_FRAME_RGB888)
                {                    
                    /* Point CC Field Index to RGB888 to YUV422 conversion coefficients */
                    pDevice->CCTableIndex = 1;
                }
                /* else: Conversion mode is YUV422 to RGB888 on Overlay Frame */
            }
            /* No color conversion required */
            else
            {
                /* Point CC Field Index to special usage case conversion coefficients */
                pDevice->CCTableIndex = 2;
            }
        }
        /* This conversion mode is a special usage case */
        else
        {
            /* Point CC Field Index to special usage case conversion coefficients */
            pDevice->CCTableIndex = 2;
            /* Validate the special usage case */
            if (CCmode->OutputFrame == ADI_PIXC_FRAME_RGB888)
            {
                /* Special usage doesnot support RGB888 output for YUV444 input(s). return error */
                Result = ADI_PIXC_RESULT_FRAME_TYPE_INVALID;
            }
            /* check if this is special usage case of YUV444 input image, no overlay and YUV422 output */
            else if ((OverlayType == ADI_PIXC_FRAME_DISABLE) && (CCmode->OutputFrame != ADI_PIXC_FRAME_YUV422))
            {
                /* YUV444 to YUV444 conversion? - not supported. return error */
                Result = ADI_PIXC_RESULT_FRAME_TYPE_INVALID;
            }
            /* check if this is special usage case of YUV444 input, YUV444 overlay and YUV444 output */
            else if ((CCmode->InputFrame == ADI_PIXC_FRAME_YUV444) && (OverlayType == ADI_PIXC_FRAME_YUV444))
            {
                /* change PIXC_CTL Output frame format bit to YUV422 */
                ADI_PIXC_CTL_CLEAR_OUT_FORM_YUV();
            }
        }
    }
    
    /* IF (the given Frame types are valid) */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
    	/* update color conversion coefficient registers */
    	PixcUpdateCCRegs(pDevice);
        /* Update UDS_MOD as per PIXC HRM recommendations */
        if ((CCmode->InputFrame     == ADI_PIXC_FRAME_YUV422) && 
            (OverlayType            == ADI_PIXC_FRAME_YUV422) &&
            (CCmode->OutputFrame    == ADI_PIXC_FRAME_YUV422))
        {
            /* for YUV422 to YUV422 conversion, UDS_MOD is recommended to be configured for 
            duplicating-dropping mode to prevent a low pass effect on the images */
            ADI_PIXC_CTL_CLEAR_UDS_MOD();
        }
    }    
    
    /* return */
    return (Result);
}    

/*********************************************************************

    Function:       PixcUpdateCCRegs

    Description:    Updates PIXC color conversion registers 
                    (RY/GU/BV coefficients, CC Bias & PIXC_CTL registers)
                    to match the client's color conversion mode

*********************************************************************/
static void PixcUpdateCCRegs(                    /* Updates PIXC color conversion registers 	*/
    ADI_PIXC                 *pDevice           /* pointer to the device we're working on   */
){

	/* IF (ITU-R 656 support is enabled) */
	if(pDevice->ITUR656EnableFlag)
	{
		/* Load coefficient values from 'ITUR656ColorConversion' table */
        /* Update  R/Y coefficients for color space conversion matrix (PIXC_RYCON) */
        pDevice->pRegisters->PixcRYCon     = ITUR656ColorConversion[pDevice->CCTableIndex].ry_coeff;
        /* Update G/U coefficients for color space conversion matrix (PIXC_GUCON) */
        pDevice->pRegisters->PixcGUCon     = ITUR656ColorConversion[pDevice->CCTableIndex].gu_coeff;
        /* Update B/V coefficients for color space conversion matrix (PIXC_BVCON) */
        pDevice->pRegisters->PixcBVCon     = ITUR656ColorConversion[pDevice->CCTableIndex].bv_coeff;
        /* Update color conversion bias for color conversion matrix (PIXC_CCBIAS) */
        pDevice->pRegisters->PixcCCBias    = ITUR656ColorConversion[pDevice->CCTableIndex].cc_bias;
	}
	/* ELSE (ITU-R 656 support is disabled) */
	else
	{
		/* Load coefficient values from 'DefaultColorConversion' table */
        /* Update  R/Y coefficients for color space conversion matrix (PIXC_RYCON) */
        pDevice->pRegisters->PixcRYCon     = DefaultColorConversion[pDevice->CCTableIndex].ry_coeff;
        /* Update G/U coefficients for color space conversion matrix (PIXC_GUCON) */
        pDevice->pRegisters->PixcGUCon     = DefaultColorConversion[pDevice->CCTableIndex].gu_coeff;
        /* Update B/V coefficients for color space conversion matrix (PIXC_BVCON) */
        pDevice->pRegisters->PixcBVCon     = DefaultColorConversion[pDevice->CCTableIndex].bv_coeff;
        /* Update color conversion bias for color conversion matrix (PIXC_CCBIAS) */
        pDevice->pRegisters->PixcCCBias    = DefaultColorConversion[pDevice->CCTableIndex].cc_bias;
	}
	
	return;
}

/*********************************************************************

    Function:       PixcIntHandler

    Description:    Processes PIXC status interrupts

*********************************************************************/
static ADI_INT_HANDLER(PixcIntHandler)  /* PIXC status interrupt handler */
{

    u32                     i;          /* counter  */
    ADI_PIXC                *pDevice;   /* pointer to the device we're working on */
    ADI_INT_HANDLER_RESULT  Result;     /* result */

    /* assume this interrupt is not for us */
    Result = ADI_INT_RESULT_NOT_PROCESSED;
    
    /* check IRQ status for every device and service all IRQs */
    for (i = 0, pDevice = Device; i < (sizeof (Device)/sizeof (ADI_PIXC)); i++, pDevice++) 
    {
        /* IF (the device is open) */
        if (pDevice->InUseFlag == TRUE) 
        {
            /* check for Overlay complete interrupt */
            if (ADI_PIXC_INTRSTAT_GET_OVR_IRQ_STAT)
            {
                /* Overlay complete interrupt has occured. Clear this interrupt */
                ADI_PIXC_INTRSTAT_CLEAR_OVR_INT_STAT();
                /* post callback (parameters are device handle, ADI_PIXC_EVENT_OVERLAY_COMPLETE, NULL) */
                (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_PIXC_EVENT_OVERLAY_COMPLETE, NULL);
                /* This interrupt is for us. indicate that we processed the interrupt */
                Result = ADI_INT_RESULT_PROCESSED;
            }
            /* check for Frame complete interrupt */
            if (ADI_PIXC_INTRSTAT_GET_FRM_IRQ_STAT)
            {
                /* Frame complete interrupt has occured. Clear this interrupt */
                ADI_PIXC_INTRSTAT_CLEAR_FRM_INT_STAT();
                /* post callback (parameters are device handle, ADI_PIXC_EVENT_FRAME_COMPLETE, NULL) */
                (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_PIXC_EVENT_FRAME_COMPLETE, NULL);
                /* This interrupt is for us. indicate that we processed the interrupt */
                Result = ADI_INT_RESULT_PROCESSED;
            }
        }
    }
    
    /* return */
    return(Result);
}

#if defined(ADI_DEV_DEBUG)  /* Debug build only */
/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of a PIXC device  */
)
{    
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     Result = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* a temporary location */
    u32     i;
    
    /* compare the given PDD handle with PDDHandle allocated to all PIXC devices in the list    */
    for (i = 0; i < ADI_PIXC_NUM_DEVICES; i++) 
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

    Function:       ValidatePixcRegs

    Description:    Validates Pixel Compositor Register values
                    before enabling PIXC dataflow

*********************************************************************/

static u32 ValidatePixcRegs(                    /* Validates PIXC register values           */
    ADI_PIXC                 *pDevice           /* pointer to the device we're working on   */
){
    /* Return value - assume we're going to be successful */
    u32     Result  = ADI_DEV_RESULT_SUCCESS;
    
    /* validate PIXC registers */
    if (pDevice->pRegisters->PixcPpl == 0)  /* is Pixel Per Line valid? */
    {
        Result = ADI_PIXC_RESULT_PIXEL_PER_LINE_INVALID;
    }
    else if (pDevice->pRegisters->PixcLpf == 0)  /* is Lines Per Frame valid? */
    {
        Result = ADI_PIXC_RESULT_LINES_PER_FRAME_INVALID;    
    }
    
    /* return */
    return (Result);
}
#endif
