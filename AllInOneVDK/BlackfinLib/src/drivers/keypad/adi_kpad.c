/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  


Description:
            This is the driver source code for the KPAD peripheral.
            
*********************************************************************************/


/********************************************************************************

Include files

*********************************************************************************/


#include <services\services.h>          /* system service includes  */
#include <drivers\adi_dev.h>            /* device manager includes  */
#include <drivers\keypad\adi_kpad.h>    /* kpad driver includes     */

/* KPAD is available only for Moab class devices*/                               
#if !defined(__ADSP_MOAB__)                                                      
#error "KPAD is available only on ADSP-BF542,ADSP-BF548 & ADSP-BF549" 
#endif

/********************************************************************************

Enumerations and defines

*********************************************************************************/

/* number of KPAD devices in the system*/
#define ADI_KPAD_NUM_DEVICES    (sizeof(Device)/sizeof(ADI_KPAD))   

/********************************************************************************                         
                                                                                               
Macros to manipulate selected bits/fields in KPAD MMRs                                         
                                                                                               
*********************************************************************************/                         
/* Macros to manipulate bits/fields of KPAD control register */ 
/*Set KPAD_EN (Enables KPAD module)*/ 
#define ADI_KPAD_CTL_SET_KPAD_EN()      (pDevice->pRegisters->KPADCtrl  |= 0x0001)
/* Clear KPAD_EN (Disables KPAD module) */
#define ADI_KPAD_CTL_CLEAR_KPAD_EN()    (pDevice->pRegisters->KPADCtrl  &= 0xFC06)

/* Clear KPAD_IRQMODE bit fields */
#define ADI_KPAD_CTL_CLEAR_IRQMODE()    (pDevice->pRegisters->KPADCtrl  &= 0xFC01)
/* Set KPAD_IRQMODE bit fields */
#define ADI_KPAD_CTL_SET_IRQMODE(val)   (pDevice->pRegisters->KPADCtrl  |= (val << 1))


/* Clear KPAD_ROWEN bit fields */
#define ADI_KPAD_CTL_CLEAR_ROWEN()      (pDevice->pRegisters->KPADCtrl  &= 0xE007)
/* Set KPAD_ROWEN bit fields */
#define ADI_KPAD_CTL_SET_ROWEN(val)     (pDevice->pRegisters->KPADCtrl  |= (val << 10))
/* Clear KPAD_COLEN bit fields */
#define ADI_KPAD_CTL_CLEAR_COLEN()      (pDevice->pRegisters->KPADCtrl  &= 0x1C07)
/* Set KPAD_COLEN bit fields */
#define ADI_KPAD_CTL_SET_COLEN(val)     (pDevice->pRegisters->KPADCtrl  |= (val << 13))
/* Get KPAD_EN bit status */
#define ADI_KPAD_CTL_GET_KPAD_EN        (pDevice->pRegisters->KPADCtrl  &  0x0001)
/* Get KPAD_IRQMODE status */
#define ADI_KPAD_CTL_GET_KPAD_IRQMODE() ((pDevice->pRegisters->KPADCtrl &  0x0006) >> 1)                                                           
/* Macros to manipulate bits/fields of KPAD status register */ 
/* Clear KPAD_IRQ (clear interrupt pending bit) */
#define ADI_KPAD_STAT_CLEAR_KPAD_IRQ()  (pDevice->pRegisters->KPADStat  |= 0x0001)
/* Get KPAD_IRQ of the KPAD_STAT register */
#define ADI_KPAD_STAT_GET_KPAD_IRQ      (pDevice->pRegisters->KPADStat &  0x0001)                                                           
/* Get KPAD_MROWCOL of the KPAD_STAT register */
#define ADI_KPAD_STAT_GET_KPAD_MROWCOL() ((pDevice->pRegisters->KPADStat &  0x0006) >> 1)
/* Get KPAD_PRESSED of the KPAD_STAT register */
#define ADI_KPAD_STAT_GET_KPAD_PRESSED()((pDevice->pRegisters->KPADStat &  0x0008) >> 3)

/* Macros to manipulate bits/fields of KPAD Multiplier Select register */ 
/* Clear DBON_SCALE bit fields */
#define ADI_KPAD_CTL_CLEAR_DBON()        (pDevice->pRegisters->KPADMsel  &= 0xFF00)
/* Set DEBON bit fields */
#define ADI_KPAD_CTL_SET_DBON(val)       (pDevice->pRegisters->KPADMsel  |= val)
/* Clear COLDRV_SCALE bit fields */
#define ADI_KPAD_CTL_CLEAR_COLDRV()      (pDevice->pRegisters->KPADMsel  &= 0x00FF)
/* Set COLDRV bit fields */
#define ADI_KPAD_CTL_SET_COLDRV(val)     (pDevice->pRegisters->KPADMsel  |= (val << 8))

#define ADI_KPAD_MSEL_SET_SCALE(delay)  \
((pDevice->pRegisters->KPADMsel |= ((((delay*10)-1) << 8) | (delay*10)-1) ))

/* KPAD_STATUS register key pressed indicator values */
#define ADI_KPAD_NOKEY_PRESSED              0
#define ADI_KPAD_SKEY_PRESSED               1
#define ADI_KPAD_MULTKEY_PRESSED            2
#define ADI_KPAD_MULT_ROW_COL_PRESSED       3
    
/* KPAD_CONTROL register interrupt mode */
#define ADI_KPAD_IRQ_DISABLE                0
#define ADI_KPAD_IRQ_SKEY_MODE              1
#define ADI_KPAD_IRQ_SMULTKEY_MODE          2

/*********************************************************************************

Macros to mask reserved/selected bits in KPAD MMRs

*********************************************************************************/
/* Macros to mask reserved bits in KPAD MMRs */
/* Masks reserved locations in KPAD control register */
#define ADI_MASK_KPAD_CTL                   0xFC07
/* Masks reserved locations in KPAD prescale register */
#define ADI_MASK_KPAD_PRESCALE              0x003F
/* Masks reserved locations in KPAD software evaluate register */
#define ADI_MASK_KPAD_SOFTEVAL              0x0001
/*********************************************************************************

Data Structures

*********************************************************************************/

/********************
    Moab  
********************/

#if defined(__ADSP_MOAB__)          /* KPAD register structure for Moab devices */

typedef struct {                    /* structure representing KPAD reg memory   */
    volatile u16    KPADCtrl;       /* Control register                         */
    u16             Padding0;
    volatile u16    KPADPrescale;   /* Pre-scale register                       */
    u16             Padding1;
    volatile u16    KPADMsel;       /* Multiplier Select register               */
    u16             Padding2;
    volatile u16    KPADRowCol;     /* Row Column register                      */
    u16             Padding3;
    volatile u16    KPADStat;       /* Status register                          */
    u16             Padding4;
    volatile u16    KPADSoftEval;   /* Software evaluate register               */
    u16             Padding5;
}ADI_KPAD_REGISTERS;
#endif                              /* End of Moab specific Data Structures     */

typedef struct {                                /* KPAD device structure        */
    ADI_KPAD_REGISTERS      *pRegisters;        /* base address SPORT regs      */
    u8                      InUseFlag;          /* in use flag (used when TRUE) */    
    void                    *pEnterCriticalArg; /* critical region argument     */    
    ADI_INT_PERIPHERAL_ID   KPADPeripheralID;   /* KPAD peripheral ID           */
    u32                     KPADIVG;            /* KPAD IVG                     */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;       /* Device manager handle        */
        void (*DMCallback) (                    /* Device manager Callback func */
        ADI_DEV_DEVICE_HANDLE   DeviceHandle,   /* device handle                */
        u32                     Event,          /* event ID                     */
        void                    *pArg);         /* argument pointer             */   
    u32                     SCLK;               /* SCLK (system clock) freqn Hz */
    u8                      KpadEnFlag;         /* KPAD enable/disable flag     */
    u32                     KpadIrqMode;        /* KPAD single/multi keyint mode*/
} ADI_KPAD;

ADI_KPAD_KEY_PRESSED_RESULT KpadResultBuf;

/*********************************************************************

Device specific data

*********************************************************************/

/***************************
    MOAB 
****************************/

#if defined(__ADSP_MOAB__)                                                      

static ADI_KPAD Device[] = {                    /* Actual KPAD devices          */
        (ADI_KPAD_REGISTERS *)(0xffc04100),     /* KPAD register base address   */
        FALSE,                                  /* in use flag                  */
        NULL,                                   /* critical region argument     */
        ADI_INT_KEYPAD,                         /* KPAD peripheral ID           */
        0,                                      /* KPAD IVG                     */
        NULL,                                   /* Device manager handle        */
        NULL,                                   /* Device manager Callback func */
        0,                                      /* SCLK frequency               */
        FALSE,                                  /* KPAD enable flag             */
        0,                                      /* KPAD_IRQMode                 */

};

#endif


    
/*********************************************************************************

Static functions

*********************************************************************************/
/* Open a device                */
static u32 adi_pdd_Open(                        
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* device manager handle        */
    u32                     DeviceNumber,       /* device number                */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* device handle                */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer PDD handle location  */
    ADI_DEV_DIRECTION       Direction,          /* data direction               */
    void                    *pCriticalRegionArg,/* critical imask location      */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager    */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle              */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback func */
);

static u32 adi_pdd_Close(                       /* Closes a device              */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle                   */
);

/* Reads data or queues an inbound buffer to a device */
static u32 adi_pdd_Read(                        
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                   */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* buffer type                  */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer            */
);

/* Writes data or queues an outbound buffer to a device */  
static u32 adi_pdd_Write(                       
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                   */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* buffer type                  */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer            */
);

/* queues a sequential buffer to a device */
static u32 adi_pdd_SequentialIO(                
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                   */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* buffer type                  */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer            */
);

/* Sets or senses a device specific parameter */    
static u32 adi_pdd_Control(                     
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                   */
    u32                     Command,            /* command ID                   */
    void                    *Value              /* command specific value       */
);

static u32 KeypadPortControl(ADI_KPAD *pDevice); /* configure pins for KPAD use */


static ADI_INT_HANDLER(InterruptHandler);       /* KPAD interrupt handler       */
    
#if defined(ADI_DEV_DEBUG)
static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);
#endif


/*********************************************************************************

Entry point for device manager

*********************************************************************************/
ADI_DEV_PDD_ENTRY_POINT ADIKPADEntryPoint = 
{
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO    
};

/*********************************************************************************

    Function:       adi_pdd_Open

    Description:    Opens a KPAD device for use

*********************************************************************************/


static u32 adi_pdd_Open(                        /* Open a device                */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* device manager handle        */
    u32                     DeviceNumber,       /* device number                */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* device handle                */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer PDD handle location  */
    ADI_DEV_DIRECTION       Direction,          /* data direction               */
    void                    *pEnterCriticalArg, /* enter critical region        */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager    */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle              */
    ADI_DCB_CALLBACK_FN     DMCallback          /* client callback function     */
) {

    /* Return value - assume we're going to be successful   */
    u32         Result = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on */
    ADI_KPAD        *pDevice;       
    /* exit critical region parameter */
    void            *pExitCriticalArg;  
    /* temporary value */
    u32         i;      
    /* sclk */
    u32         SCLK;       

    /* check for errors if required */
#ifdef ADI_DEV_DEBUG
    if (DeviceNumber >= ADI_KPAD_NUM_DEVICES) /* check the device number        */
    {   
        Result = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }
#endif
        /*  Continue only if the device number is valid */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* insure the device the client wants is available */
            Result = ADI_DEV_RESULT_DEVICE_IN_USE;
            pDevice = &Device[DeviceNumber];
            pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
            if (pDevice->InUseFlag == FALSE) {
                pDevice->InUseFlag = TRUE;
                Result = ADI_DEV_RESULT_SUCCESS;
            }
            adi_int_ExitCriticalRegion(pExitCriticalArg);
        
            /* Continue only when the KPAD device is available for use  */
            if (Result == ADI_DEV_RESULT_SUCCESS) 
            {    
                /* get SCLK (now returns values in Hz! */
                Result = adi_pwr_GetFreq(&i, &SCLK, &i);
            }

            /* Continue when the we know the SCLK */
            if (Result == ADI_DEV_RESULT_SUCCESS) 
            {
                /* initialize the device settings */
                pDevice->SCLK = SCLK;
                /* Pointer to critical region */
                pDevice->pEnterCriticalArg  = pEnterCriticalArg;
                /* Pointer to Device handle */
                pDevice->DeviceHandle       = DeviceHandle;
                /* Pointer to Device manager callback function */
                pDevice->DMCallback         = DMCallback;
    
                /* reset device default configuration       */
                /* 0.1ms(100usec) time base                 */
                pDevice->pRegisters->KPADPrescale = (SCLK/10000) >> 10;
                /* Debounce and column drive times are set to 1ms */
                pDevice->pRegisters->KPADMsel = 0x0909; 
                /*pDevice->pRegisters->KPADMsel = 0x1313; // 2ms */
    
                /*4 rows enabled, 4 columns enabled,single & multiple key interrupt enable */
                pDevice->pRegisters->KPADCtrl = 0x6c04; 
                pDevice->KpadIrqMode = 2;
        
                /* save the physical device handle in the client supplied location */
                *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;
            
                /* KPAD interrupt handle */
                /* get the KPAD IVG */
                adi_int_SICGetIVG(pDevice->KPADPeripheralID, &pDevice->KPADIVG);

                /* hook the interrupt handler  */
                if(adi_int_CECHook(pDevice->KPADIVG, InterruptHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS)
                {
                    pDevice->InUseFlag = FALSE;
                    Result = ADI_DEV_RESULT_CANT_HOOK_INTERRUPT;
                }

            }
        
        
            if (Result == ADI_DEV_RESULT_SUCCESS) 
            {
                /* allow this interrupt to wake us up and allow it to be passed to the core */
                adi_int_SICWakeup(pDevice->KPADPeripheralID, TRUE);
                adi_int_SICEnable(pDevice->KPADPeripheralID);
            }
        }
    
        /* return */
        return(Result);
}
        

/*********************************************************************************

    Function:       adi_pdd_Close

    Description:    Closes down a KPAD device

*********************************************************************************/


static u32 adi_pdd_Close(           /* Closes a device                          */
    ADI_DEV_PDD_HANDLE PDDHandle    /* PDD handle                               */
) {
    
    u32             Result;         /* return value                             */
    ADI_KPAD        *pDevice;       /* pointer to the device we're working on   */
    
    
    /* check for errors if required */
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    /* avoid casts */
    pDevice = (ADI_KPAD *)PDDHandle;
    
    /* Set KPAD MMRs to power-on/reset values  */
    pDevice->pRegisters->KPADCtrl		= 0;
    pDevice->pRegisters->KPADPrescale	= 0;
    pDevice->pRegisters->KPADMsel		= 0;
    pDevice->pRegisters->KPADRowCol		= 0;
    pDevice->pRegisters->KPADStat		= 0;
    pDevice->pRegisters->KPADSoftEval	= 0;

    pDevice->KpadEnFlag 	= FALSE;
    pDevice->KpadIrqMode 	= 0;
    /* mark the device as closed */
    pDevice->InUseFlag 		= FALSE;

    /* disable interrupts from the KPAD */
    adi_int_SICDisable(pDevice->KPADPeripheralID);
    adi_int_SICWakeup(pDevice->KPADPeripheralID, FALSE);
        
    /* unhook the handler */
    Result = adi_int_CECUnhook(pDevice->KPADIVG, InterruptHandler, pDevice);
    
    /* return */
    return(Result);
}

/*********************************************************************************

    Function:       adi_pdd_Read

    Description:    Processing of inbound buffer is not supported

*********************************************************************************/

static u32 adi_pdd_Read(                /* Reads to inbound buffer to a device  */
    ADI_DEV_PDD_HANDLE  PDDHandle,      /* PDD handle                           */
    ADI_DEV_BUFFER_TYPE BufferType,     /* buffer type                          */
    ADI_DEV_BUFFER      *pBuffer        /* pointer to buffer                    */
){

    /* not supported  */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}
        



/*********************************************************************************

    Function:       adi_pdd_Write
    
    Description:    Processing of outbound buffer is not supported


*********************************************************************************/

static u32 adi_pdd_Write(           /* Writes data to outbound buffer           */
    ADI_DEV_PDD_HANDLE  PDDHandle,  /* PDD handle                               */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type                              */
    ADI_DEV_BUFFER      *pBuffer    /* pointer to buffer                        */
){
    /* not supported  */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);


}

/*********************************************************************************

    Function:       adi_pdd_SequentialIO

    Description:    Processing of sequential buffer is not supported

*********************************************************************************/

static u32 adi_pdd_SequentialIO(    /* provides buffer during sequential I/O    */
    ADI_DEV_PDD_HANDLE  PDDHandle,  /* PDD handle                               */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type                              */
    ADI_DEV_BUFFER      *pBuffer    /* pointer to buffer                        */
){

    /* not supported  */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);

}
        
/*********************************************************************************

    Function:       adi_pdd_Control

    Description:    Configures the KPAD device registers

*********************************************************************************/


static u32 adi_pdd_Control(                 /* Sets device specific parameter   */
    ADI_DEV_PDD_HANDLE  PDDHandle,          /* PDD handle                       */
    u32                 Command,            /* command ID                       */
    void                *Value              /* command specific value           */
) {
    
    ADI_KPAD            *pDevice;           /* pointer to the device            */
    u32                 Result;             /* return value                     */
    u32                 u32Value;           /* u32 type to avoid casts/warnings */
    u16                 u16Value;           /* u16 type to avoid casts/warnings */
    volatile            u16  *p16;          /* pointer to 16 bit register       */
    void                *pExitCriticalArg;  /* exit critical region parameter   */

    /* assume we're going to be successful */
    Result  = ADI_DEV_RESULT_SUCCESS;
            
    /* avoid casts */
    pDevice     = (ADI_KPAD *)PDDHandle;
    /* assign 16 and 32 bit values for the Value argument */
    u32Value        = (u32)Value;
    u16Value        = (u16)u32Value;

    
    /* check for errors if required */
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
    /* CASEOF (Command ID) */
    switch (Command) {
        
    /***************************************
    Device Manager commands
    ***************************************/

    /* CASE: control dataflow */
    case (ADI_DEV_CMD_SET_DATAFLOW):
    /* CASE: set dataflow method */
    case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
    /* Do nothing & simply return back for these commands */
        break;

    /* CASE( query for processor DMA support) */
    case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
        /* KPAD does not support DMA */
        *((u32 *)Value) = FALSE;   
        break;
                
     /* CASE: Configure KPAD registers? */
     default:

    /* Make sure that this command is ours */
    if ((Command <= ADI_KPAD_CMD_START) || (Command >= ADI_KPAD_CMD_END))
    {
        /* KPAD doesn't understand this command */
        Result = ADI_DEV_RESULT_NOT_SUPPORTED;
          break;
     }

     switch (Command) 
     {
        
        /* CASE (set KPAD control register) */
        case (ADI_KPAD_CMD_SET_CONTROL_REG):
            /* Mask the reserved bits */
            u16Value &= ADI_MASK_KPAD_CTL;
            pDevice->pRegisters->KPADCtrl = u16Value;
        
        break;
        
        /* CASE (set KPAD PRESCALE register) */
        case (ADI_KPAD_CMD_SET_PRESCALE_REG):
        /* CASE (set KPAD Prescale bits) */
        case (ADI_KPAD_CMD_SET_PRESCALE_VAL):       

             /* Mask the reserved bits */
             u16Value &= ADI_MASK_KPAD_PRESCALE;

            if(pDevice->KpadEnFlag == TRUE) 
            {
                /* disable KPAD before write to Prescale register */
                ADI_KPAD_CTL_CLEAR_KPAD_EN();
                pDevice->pRegisters->KPADPrescale = u16Value;
                /* enable KPAD */
                ADI_KPAD_CTL_SET_KPAD_EN();

            }
            else
            {
              pDevice->pRegisters->KPADPrescale = u16Value;
            }

            break;
        
        
        /* CASE (set KPAD MSEL register) */
        case (ADI_KPAD_CMD_SET_MSEL_REG):
            pDevice->pRegisters->KPADMsel = u16Value;

        break;
        
        /* CASE (set KPAD SOFTEVAL register) */
        case (ADI_KPAD_CMD_SET_SOFTEVAL_REG):
        /* CASE (set KPAD key programmable force evaluate bits) */
        case (ADI_KPAD_CMD_SET_SOFTEVAL):
             /* Mask the reserved bits */
             u16Value &= ADI_MASK_KPAD_SOFTEVAL;
            pDevice->pRegisters->KPADSoftEval = u16Value;

            break;
            

        /* CASE (get KPAD ROW COLUMN register) */
        case (ADI_KPAD_CMD_GET_ROWCOL_REG):
        /* CASE (Get KPAD STATUS register) */
        case (ADI_KPAD_CMD_GET_STAT_REG):
        
        /* get it */
        p16 = (volatile u16 *)pDevice->pRegisters + ((Command - ADI_KPAD_CMD_SET_CONTROL_REG) << 1);
        *((u16 *)Value) = *p16;/* return to client */

            
            break;      

        /* CASE (get KPAD control register) */
        case (ADI_KPAD_CMD_GET_CONTROL_REG):

            *((u16 *)Value) = pDevice->pRegisters->KPADCtrl;
        
        break;			
			
    
        /* CASE (set KPAD Enable bit) */
        case (ADI_KPAD_CMD_SET_KPAD_ENABLE):
			if(u16Value == TRUE)
			{
                /* enable KPAD */
                ADI_KPAD_CTL_SET_KPAD_EN();
                pDevice->KpadEnFlag = TRUE;
            }
			else
			{
			    /* disable KPAD */
                ADI_KPAD_CTL_CLEAR_KPAD_EN();
				pDevice->KpadEnFlag = FALSE;
			}

            break;
        
        
        /* CASE (set KPAD Multikey Press Interrupt Enable bits) */
        case (ADI_KPAD_CMD_SET_IRQMODE):
#if defined(ADI_DEV_DEBUG)
            if (u16Value > 2)
                {
                    Result = ADI_KPAD_RESULT_BAD_ACCESS_WIDTH;
                    break;
                }
#endif
            /* clear the bit field */
             ADI_KPAD_CTL_CLEAR_IRQMODE();

            /* disable KPAD before changing IRQMODE */
            if(pDevice->KpadEnFlag == TRUE) 
            {
                /* set the new IRQMODE value */
                ADI_KPAD_CTL_SET_IRQMODE(u16Value);
                /* enable KPAD */
                ADI_KPAD_CTL_SET_KPAD_EN();
            }
            else
            {
                /* set the new IRQMODE value */
                ADI_KPAD_CTL_SET_IRQMODE(u16Value);
            
            }
        
            break;
        
        /* CASE (set KPAD Row enable width bits) */
        case (ADI_KPAD_CMD_SET_ROW_NUMBER):
#if defined(ADI_DEV_DEBUG)
            if (u16Value > 7)
            {
                Result = ADI_KPAD_RESULT_BAD_ACCESS_WIDTH;
                break;
            }
#endif
            /* clear the bit field */
             ADI_KPAD_CTL_CLEAR_ROWEN();

            /* disable KPAD before changing ROWEN */
            if(pDevice->KpadEnFlag == TRUE) 
            {
                /* set the new ROWEN value */
                ADI_KPAD_CTL_SET_ROWEN(u16Value);
                /* enable KPAD */
                ADI_KPAD_CTL_SET_KPAD_EN();
            }
            else
            {
                /* set the new ROWEN value */
                ADI_KPAD_CTL_SET_ROWEN(u16Value);
            }
            break;
    
        
        /* CASE (set KPAD Column enable width bits) */
        case (ADI_KPAD_CMD_SET_COLUMN_NUMBER):
#if defined(ADI_DEV_DEBUG)
            if (u16Value > 7)
            {
                Result = ADI_KPAD_RESULT_BAD_ACCESS_WIDTH;
                break;
            }
#endif
            /* clear the bit field */
             ADI_KPAD_CTL_CLEAR_COLEN();

            /* disable KPAD before changing COLEN */
            if(pDevice->KpadEnFlag == TRUE) 
            {
                /* set the new COLEN value */
                ADI_KPAD_CTL_SET_COLEN(u16Value);
                /* enable KPAD */
                ADI_KPAD_CTL_SET_KPAD_EN();
            }
            else
            {
                /* set the new COLEN value */
                ADI_KPAD_CTL_SET_COLEN(u16Value);
            
            }
        
            break;
        
        
        /* CASE (set KPAD debounce multiplier bits) */
        case (ADI_KPAD_CMD_SET_DBON_SCALE):
             /* Mask the valid bits */
             u16Value &= 0x00FF;
            /* clear the bit field */   
            ADI_KPAD_CTL_CLEAR_DBON();      
            /* set the new value */ 
            ADI_KPAD_CTL_SET_DBON(u16Value);        

            break;
        
        /* CASE (set KPAD column drive multiplier bits) */
        case (ADI_KPAD_CMD_SET_COLDRV_SCALE):
             /* Mask the valid bits */
             u16Value &= 0xFF00;
            /* clear the bit field */   
            ADI_KPAD_CTL_CLEAR_COLDRV();        
            /* set the new value */ 
            ADI_KPAD_CTL_SET_COLDRV(u16Value);      
        
        break;
        
        /* CASE (set de-bounce and column drive delay time) */
        /* this command supports 10 resolutions ( 1 to 10 msec)*/
        case (ADI_KPAD_CMD_SET_DBON_COLDRV_TIME):
#if defined(ADI_DEV_DEBUG)
            if (u16Value == 0 || u16Value > 10)
            {
                Result = ADI_KPAD_RESULT_BAD_ACCESS_WIDTH;
                break;
            }
#endif

        /* Make sure to disable KPAD module before configuring register */
        if(pDevice->KpadEnFlag == TRUE) 
        {
            /* Clear KPAD_EN (Disable KPAD module) */
                ADI_KPAD_CTL_CLEAR_KPAD_EN();
            /* 0.1ms(100usec) time base  */
            pDevice->pRegisters->KPADPrescale = ((pDevice->SCLK)/10000) >> 10;

            /* clear MSEL-Register first */
            pDevice->pRegisters->KPADMsel = 0;
            ADI_KPAD_MSEL_SET_SCALE(u16Value);
        
            /* enable KPAD */
            ADI_KPAD_CTL_SET_KPAD_EN();
    
        }
        else
        {
            /* 0.1ms(100usec) time base  */
            pDevice->pRegisters->KPADPrescale = ((pDevice->SCLK)/10000) >> 10;

            /* clear MSEL-Register first */
            pDevice->pRegisters->KPADMsel = 0;
            ADI_KPAD_MSEL_SET_SCALE(u16Value);
  
        }


        break;
        /* ENDCASE of Configure KPAD registers*/
        }
        
        /* check if KPAD_EN bit is set */
            if (ADI_KPAD_CTL_GET_KPAD_EN )
            {
                pDevice->KpadEnFlag = TRUE;
                /* configure pins for KPAD */
                KeypadPortControl(pDevice);
            }
    /* ENDCASE */
    }
        
    /* return */
    return(Result);
}


/*********************************************************************************

    Function:       InterruptHandler

    Description:    Processes events in response to KPAD interrupts

*********************************************************************************/

static ADI_INT_HANDLER(InterruptHandler)
{
    ADI_KPAD *pDevice; /* KPAD device */
    
    bool PostCallback = false;
    
    ADI_INT_HANDLER_RESULT Result;
    
    /* assume the interrupt is not for us */
    Result = ADI_INT_RESULT_NOT_PROCESSED;
    /* clear result buffer */
    KpadResultBuf.rowcolpos = 0;
    KpadResultBuf.mrowcol = 0;
    KpadResultBuf.keypressed = 0;


    /* point to the KPAD */
    pDevice = (ADI_KPAD *)ClientArg;
    
    /* IF (this device is asserting an interrupt) */
    if (adi_int_SICInterruptAsserted(pDevice->KPADPeripheralID) == ADI_INT_RESULT_ASSERTED) {
        
        /* IF (KPAD_IRQ bit of KPAD_STAT is set) */
        if ( ADI_KPAD_STAT_GET_KPAD_IRQ ) {
            /* IF (user wants callback) */
            if ((pDevice->KpadIrqMode == ADI_KPAD_IRQ_SKEY_MODE)&& 
            ( ADI_KPAD_STAT_GET_KPAD_MROWCOL() == ADI_KPAD_SKEY_PRESSED))
            { /* single key press */
                PostCallback = true;    
            }
            else if (pDevice->KpadIrqMode == ADI_KPAD_IRQ_SMULTKEY_MODE)
            {/* single or multiple key press  */
                PostCallback = true;    
            }
        }   
            
    /* IF (callback request) */
    if (PostCallback)
    {           
         /* update the key pressed status */
          KpadResultBuf.rowcolpos = pDevice->pRegisters->KPADRowCol;
          KpadResultBuf.mrowcol = ADI_KPAD_STAT_GET_KPAD_MROWCOL();
          KpadResultBuf.keypressed = ADI_KPAD_STAT_GET_KPAD_PRESSED();
     
      (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_KPAD_EVENT_KEYPRESSED, &KpadResultBuf);

        }
        
        /* clear the interrupt */
        ADI_KPAD_STAT_CLEAR_KPAD_IRQ(); /* sticky bit, write 1 to clear it. */

    }
    /* return */
    return (Result);
}
    
   
#if defined(ADI_DEV_DEBUG)

/*********************************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) 
{
    unsigned int i, Result;
    
    Result = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    for (i = 0; i < ADI_KPAD_NUM_DEVICES; i++) 
    {
        if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) 
        {
            Result = ADI_DEV_RESULT_SUCCESS;
        }
    }
    
    return (Result);
}


#endif


/*********************************************************************************

    Function:       kpadPortControl

    Description:    Set port D to configure PD8-PD15 as Keypad pins(Row0-3, Col0-3)


*********************************************************************************/

static u32 KeypadPortControl(ADI_KPAD *pDevice)
{
    u32 nDirectives;    /* # directives to pass */
    u16 Control;        /* Temp location to hold Keypad control register value */    
    u32 Result;         /* return code */
    
    /* Directives to enable Keypad Rows */
    ADI_PORTS_DIRECTIVE RowDirectives[] = {
            ADI_PORTS_DIRECTIVE_KEYPAD_ROW0,
            ADI_PORTS_DIRECTIVE_KEYPAD_ROW1,
            ADI_PORTS_DIRECTIVE_KEYPAD_ROW2,
            ADI_PORTS_DIRECTIVE_KEYPAD_ROW3,
            ADI_PORTS_DIRECTIVE_KEYPAD_ROW4,
            ADI_PORTS_DIRECTIVE_KEYPAD_ROW5,
            ADI_PORTS_DIRECTIVE_KEYPAD_ROW6,
            ADI_PORTS_DIRECTIVE_KEYPAD_ROW7
    };

    /* Directives to enable Keypad Columns */
    ADI_PORTS_DIRECTIVE ColDirectives[] = {
        ADI_PORTS_DIRECTIVE_KEYPAD_COL0,
        ADI_PORTS_DIRECTIVE_KEYPAD_COL1,
        ADI_PORTS_DIRECTIVE_KEYPAD_COL2,
        ADI_PORTS_DIRECTIVE_KEYPAD_COL3,
        ADI_PORTS_DIRECTIVE_KEYPAD_COL4,
        ADI_PORTS_DIRECTIVE_KEYPAD_COL5,
        ADI_PORTS_DIRECTIVE_KEYPAD_COL6,
        ADI_PORTS_DIRECTIVE_KEYPAD_COL7
    };
        
    /* assume we're going to be successful */
    Result = ADI_PORTS_RESULT_SUCCESS;
    
    /* Read Keypad Control Register */
    Control = pDevice->pRegisters->KPADCtrl;
    
    /* load Directive count with number of rows to enable */
    nDirectives = (((Control & 0x1C00) >> 10)+1);
    
    /* Call port control to enable # rows */
    Result = adi_ports_Configure(&RowDirectives[0],nDirectives);
    
    /* IF (Port control results in success) */
    if (Result == ADI_PORTS_RESULT_SUCCESS)
    {
        /* load Directive count with number of Columns to enable */
        nDirectives = (((Control & 0xE000) >> 13)+1);
    
        /* Call port control to enable # columns */
        Result = adi_ports_Configure(&ColDirectives[0],nDirectives);
    }
    
    /* return */
    return(Result);
}

