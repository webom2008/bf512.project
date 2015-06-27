/*********************************************************************************

Copyright(c) 2008 Analog Devices, Inc. All Rights Reserved. 


This software is proprietary and confidential. By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

File: 
    'adi_pwm.c'

    $RCSfile: adi_pwm.c,v $
    $Revision: 2095 $




Description:



 Source file containing Pulse Width Modulation Service in the System Services Library.  
 This file contains the following API functions:

  adi_pwm_Init 
  adi_pwm_Terminate
  adi_pwm_InstallCallback
  adi_pwm_RemoveCallback
  adi_pwm_Control
       
 These functions are only supported for ADSP_BF51X and the ADSP_BF50X Blackfin processors.

 This file contains the following internal functions
 
  adi_pwm_ValidateEventID
  adi_pwmSetDutyCycle
  adi_pwmGetDutyCycle

****************************************************************************/



/*********************************************************************

                                                     
  Include files                                         
                                                      
********************************************************************/

#include <services/services.h>       /* System Service includes  */
#include <services/pwm/adi_pwm.h>    /* PWM Service includes     */


/********************************************************************
                                                       
   Only BRODIE is currently supported so stub out API for others    
                                                    
********************************************************************/

#if !defined(__ADSP_BRODIE__) && !defined(__ADSP_MOY__)  


ADI_PWM_RESULT adi_pwm_Init(void *pCriticalRegionArg) 
{
    return(ADI_PWM_RESULT_SERVICE_NOT_SUPPORTED);
}

ADI_PWM_RESULT adi_pwm_Terminate(void) 
{
    return(ADI_PWM_RESULT_SERVICE_NOT_SUPPORTED);
}


ADI_PWM_RESULT adi_pwm_InstallCallback(ADI_PWM_EVENT_ID EventID, void *ClientHandle, ADI_DCB_HANDLE DCBHandle, ADI_DCB_CALLBACK_FN ClientCallback) 
{
    return(ADI_PWM_RESULT_SERVICE_NOT_SUPPORTED);
}

ADI_PWM_RESULT adi_pwm_RemoveCallback(ADI_PWM_EVENT_ID EventID) 
{
    return(ADI_PWM_RESULT_SERVICE_NOT_SUPPORTED);
}

ADI_PWM_RESULT adi_pwm_Control(void) 
{
    return(ADI_PWM_RESULT_SERVICE_NOT_SUPPORTED);
}

#else  /* Brodie or Moy products   */


    
/*******************************************************************
                                                    
 Register address for the Motor Control PWM                
                                                
********************************************************************/


#if defined (__ADSP_MOY__)
#define ADI_PWM_NUM_MUX_TABLE_ENTRIES 16
#elif defined (__ADSP_BRODIE__)
#define ADI_PWM_NUM_MUX_TABLE_ENTRIES 8
#endif

#if defined (__ADSP_MOY__)

/* Multiple PWMs - use an array of 'n' PWMs */

volatile u16 *pADI_PWM_ADDRESS_CTRL[ADI_NUM_PWM];   /* PWM Control Register     */
volatile u16 *pADI_PWM_ADDRESS_STAT[ADI_NUM_PWM];   /* PWM Status Register      */
volatile u16 *pADI_PWM_ADDRESS_TM[ADI_NUM_PWM];     /* PWM Period Register      */
volatile u16 *pADI_PWM_ADDRESS_DT[ADI_NUM_PWM];     /* PWM Dead Time Register   */
volatile u16 *pADI_PWM_ADDRESS_GATE[ADI_NUM_PWM];   /* PWM Chopping Control     */
volatile u16 *pADI_PWM_ADDRESS_CHA[ADI_NUM_PWM];    /* PWM Channel A Duty Control */
volatile u16 *pADI_PWM_ADDRESS_CHB[ADI_NUM_PWM];    /* PWM Channel B Duty Control */
volatile u16 *pADI_PWM_ADDRESS_CHC[ADI_NUM_PWM];    /* PWM Channel C Duty Control */
volatile u16 *pADI_PWM_ADDRESS_SEG[ADI_NUM_PWM];    /* PWM Crossover and Output Enable  */
volatile u16 *pADI_PWM_ADDRESS_SYNCWT[ADI_NUM_PWM]; /* PWM Sync pulse width control     */
volatile u16 *pADI_PWM_ADDRESS_CHAL[ADI_NUM_PWM];   /* PWM Channel AL Duty Control (SR mode only)     */
volatile u16 *pADI_PWM_ADDRESS_CHBL[ADI_NUM_PWM];   /* PWM Channel BL Duty Control (SR mode only)     */
volatile u16 *pADI_PWM_ADDRESS_CHCL[ADI_NUM_PWM];   /* PWM Channel CL Duty Control (SR mode only)     */
volatile u16 *pADI_PWM_ADDRESS_LSI[ADI_NUM_PWM];    /* PWM Low Side Invert (SR mode only) */
volatile u16 *pADI_PWM_ADDRESS_STAT2[ADI_NUM_PWM];  /* PWM Status2 Register  */
 
#else

/* Brodie PWM register Block */

#define pADI_PWM_ADDRESS_CTRL   ((volatile u16 *)0xFFC03700) /* PWM Control Register  */
#define pADI_PWM_ADDRESS_STAT   ((volatile u16 *)0xFFC03704) /* PWM Status Register   */
#define pADI_PWM_ADDRESS_TM    ((volatile u16 *)0xFFC03708) /* PWM Period Register   */
#define pADI_PWM_ADDRESS_DT    ((volatile u16 *)0xFFC0370C) /* PWM Period Register   */
#define pADI_PWM_ADDRESS_GATE   ((volatile u16 *)0xFFC03710) /* PWM Chopping Control  */
#define pADI_PWM_ADDRESS_CHA    ((volatile u16 *)0xFFC03714) /* PWM Channel A Duty Control */
#define pADI_PWM_ADDRESS_CHB    ((volatile u16 *)0xFFC03718) /* PWM Channel B Duty Control */
#define pADI_PWM_ADDRESS_CHC    ((volatile u16 *)0xFFC0371C) /* PWM Channel C Duty Control */
#define pADI_PWM_ADDRESS_SEG    ((volatile u16 *)0xFFC03720) /* PWM Crossover and Output Enable */
#define pADI_PWM_ADDRESS_SYNCWT ((volatile u16 *)0xFFC03724) /* PWM Sync pulse width control */
#define pADI_PWM_ADDRESS_CHAL   ((volatile u16 *)0xFFC03728) /* PWM Channel AL Duty Control (SR mode only)     */
#define pADI_PWM_ADDRESS_CHBL   ((volatile u16 *)0xFFC0372C) /* PWM Channel BL Duty Control (SR mode only)     */
#define pADI_PWM_ADDRESS_CHCL   ((volatile u16 *)0xFFC03730) /* PWM Channel CL Duty Control (SR mode only)     */
#define pADI_PWM_ADDRESS_LSI    ((volatile u16 *)0xFFC03734) /* PWM Low Side Invert (SR mode only) */
#define pADI_PWM_ADDRESS_STAT2  ((volatile u16 *)0xFFC03738) /* PWM Status Register  */

#endif

/*********************************************************************

 Declarations for internal functions
 
**********************************************************************/

 
ADI_PWM_RESULT adi_pwm_ValidateEventID (ADI_PWM_EVENT_ID EventID);    
ADI_PWM_RESULT adi_pwmSetDutyCycle (pADI_PWM_CHANNEL_DUTY_CYCLE pDutyCycle);
ADI_PWM_RESULT adi_pwmGetDutyCycle (pADI_PWM_CHANNEL_DUTY_CYCLE pDutyCycle);
ADI_PWM_RESULT adi_pwmCheckInitParameters (void);
    

/********************************************************************************

  Bit field structure used to read and write the PWM Control Register  
  
*********************************************************************************/  
  
typedef struct ADI_PWM_CTRL_REG
{
    volatile u32 Enable:1;        /* PWM Enable  0=disabled, 1=enabled reset by tripb def=0  */
    volatile u32 Sync_Enable:1;   /* PWM Sync Enable  0=disabled, 1=enabled def=0            */
    volatile u32 DoubleUp:1;      /* Double Update Mode 0=single, 1=double  def=0            */
    volatile u32 ExtSync:1;       /* External Sync  0=Internal sync used, 1=external def=0   */
    volatile u32 SyncSel:1;       /* External Sync Select  0=asynch, 1=sync def=1            */
    volatile u32 Polarity:1;      /* PWM output polarity  1=active HI , 0=active LO def=1    */
    volatile u32 SRMode:1;        /* PWM SR MODE  0=enabled , 1=disabled def=1               */
    volatile u32 TripIntEnable:1; /* Interrupt enable for trip  1=enabled , 0=disabled def=0 */
    volatile u32 SyncIntEnable:1; /* Interrupt enable for sync  1=enabled , 0=disabled def=0 */
    volatile u32 TripInputDisable:1; /* Disable for Trip Input  1=disabled, 0=enabled def=0  */
    volatile u32 unused0:6;      
} ADI_PWM_CTRL_REG, *pADI_PWM_CTRL_REG ;


/********************************************************************************

 Bit field structure used to read and write the PWM Status Register  

*********************************************************************************/

typedef struct ADI_PWM_STAT_REG
{
    volatile u32 Phase:1;    /* PWM phase, 0=1st half, 1=2nd half  def=0           */ 
    volatile u32 Polarity:1; /* PWM polarity  1=active high, 0=active low  def=0   */ 
    volatile u32 SRMode:1;   /* SR mode (PWM_SRMODEB) 0=active, 1-not active def=0 */
    volatile u32 Trip:1;     /* PWM Trip - pad_tripb pin def=0                     */
    volatile u32 unused1:4;    
    volatile u32 TripInt:1;  /* PWM Trip Interrupt  HW Pin, or SW  W1C, def=0      */
    volatile u32 SyncInt:1;  /* PWM Sync Interrupt W1C def=0                       */
    volatile u32 unused2:6; 
} ADI_PWM_STAT_REG, *pADI_PWM_STAT_REG;



/********************************************************************************

  structure used to read and write the PWM Crossover and Output Enable 
  
*********************************************************************************/
  
typedef struct ADI_PWM_SEG_REG
{
    volatile u32 CH_OutputEnable:1;    /* CH output enable 1=disabled, 0=enabled def=0  */
    volatile u32 CL_OutputEnable:1;    /* CL output enable RW 0 */
    volatile u32 BH_OutputEnable:1;    /* BH output enable RW 0 */
    volatile u32 BL_OutputEnable:1;    /* BL output enable RW 0 */
    volatile u32 AH_OutputEnable:1;    /* AH output enable RW 0 */
    volatile u32 AL_OutputEnable:1;    /* AL output enable RW 0 */
    volatile u32 C_OutputCrossover:1;  /* Channel C output Crossover 1=xovr, 0=not xovr  def=0  */
    volatile u32 B_OutputCrossover:1;  /* Channel B output Crossover 1=xovr, 0=not xovr  def=0  */
    volatile u32 A_OutputCrossover:1;  /* Channel A output Crossover 1=xovr, 0=not xovr  def=0  */
    volatile u32 unused5:7;
} ADI_PWM_SEG_REG, *pADI_PWM_SEG_REG;


/********************************************************************************

  Bit field structure used to read and write the PWM Low Side Invert (SR mode only) 

*********************************************************************************/

typedef struct ADI_PWM_LSI_REG
{
    volatile u32 SR_ModeLowSideInvertA:1;  /* PWM SR mode Low Side Invert Channel A def=0 */
    volatile u32 SR_ModeLowSideInvertB:1;  /* PWM SR mode Low Side Invert Channel B def=0 */
    volatile u32 SR_ModeLowSideInvertC:1;  /* PWM SR mode Low Side Invert Channel C def=0 */
    volatile u32 unused7:13;
} ADI_PWM_LSI_REG, *pADI_PWM_LSI_REG; 



/********************************************************************************

  Bit field structure used to read and write the PWM Chopping Control 
 
*********************************************************************************/

typedef struct ADI_PWM_GATE_CHOPPING
{
    volatile u32 GateChoppingPeriod:8;     /* PWM Gate Chopping Period (unsigned) def=0 */
    volatile u32 GateChoppingEnableHigh:1; /* PWM Gate Chopping Enable High Side def=0  */
    volatile u32 GateChoppingEnableLow:1;  /* PWM Gate Chopping Enable Low Side def=0   */
    volatile u32 unused4:6; 
} ADI_PWM_GATE_CHOPPING, *pADI_PWM_GATE_CHOPPING;


/********************************************************************************

  structure used to read and write the PWM Status 2 Register  
  
*********************************************************************************/
 
  
typedef struct ADI_PWM_STAT2_REG
{
    volatile u32 AL_Output:1;  /* al output signal for S/W observation  ~Polarity */
    volatile u32 AH_Output:1;  /* ah output signal for S/W observation  ~Polarity */
    volatile u32 BL_Output:1;  /* bl output signal for S/W observation  ~Polarity */
    volatile u32 BH_Output:1;  /* bh output signal for S/W observation  ~Polarity */
    volatile u32 CL_Output:1;  /* cl output signal for S/W observation  ~Polarity */
    volatile u32 CH_Output:1;  /* ch output signal for S/W observation  ~Polarity */
    volatile u32 unused8:3;
} ADI_PWM_STAT2_REG, *pADI_PWM_STAT2_REG;


/* Unions to pass register data to/from the MMR */
 
typedef union ADI_PWM_CTRL_REG_UNION 
{
    ADI_PWM_CTRL_REG   BitField;
    volatile u32      UnsignedValue;

} ADI_PWM_CTRL_REG_UNION, *pADI_PWM_CTRL_REG_UNION;


typedef union ADI_PWM_STAT_REG_UNION
{
    ADI_PWM_STAT_REG   BitField;
    volatile u32      UnsignedValue;

} ADI_PWM_STAT_REG_UNION, *pADI_PWM_STAT_REG_UNION;



typedef union ADI_PWM_GATE_REG_UNION
{
    ADI_PWM_GATE_CHOPPING   BitField;
    volatile u32          UnsignedValue;

} ADI_PWM_GATE_REG_UNION, *pADI_PWM_GATE_REG_UNION;


typedef union ADI_PWM_SEG_REG_UNION
{
    ADI_PWM_SEG_REG   BitField;
    volatile u32      UnsignedValue;

} ADI_PWM_SEG_REG_UNION, *pADI_PWM_SEG_REG_UNION;


typedef union ADI_PWM_LSI_REG_UNION
{
    ADI_PWM_LSI_REG    BitField;
    volatile u32      UnsignedValue;

} ADI_PWM_LSI_REG_UNION, *pADI_PWM_LSI_REG_UNION;

typedef union ADI_PWM_STAT2_REG_UNION
{
    ADI_PWM_STAT2_REG  BitField;
    volatile u32      UnsignedValue;

} ADI_PWM_STAT2_REG_UNION, *pADI_PWM_STAT2_REG_UNION; 




/******************************************************************************

 Static storage for register unions to be referenced bythe  config struct
  
*******************************************************************************/
#if defined(__ADSP_MOY__)  
    /* There is a set of registers for each PWM */
    static ADI_PWM_CTRL_REG_UNION   PwmCtrlReg[ADI_NUM_PWM];      /* storage PWM Control Register           */
    static ADI_PWM_STAT_REG_UNION   PwmStatReg[ADI_NUM_PWM];      /* PWM Status Register                    */
    static ADI_PWM_GATE_REG_UNION   PwmGateReg[ADI_NUM_PWM];      /* PWM Chopping Control                   */       
    static ADI_PWM_SEG_REG_UNION    PwmSegReg[ADI_NUM_PWM];       /* PWM Crossover and Output Enable        */
    static ADI_PWM_LSI_REG_UNION    PwmLowSideInvertReg[ADI_NUM_PWM]; /* PWM Low Side Invert (SR mode only) */
    static ADI_PWM_STAT2_REG_UNION  PwmStat2Reg[ADI_NUM_PWM];     /* PWM Status Register                    */ 
    
#else
    /* There is one set */
    static ADI_PWM_CTRL_REG_UNION   PwmCtrlReg;     /* storage PWM Control Register           */
    static ADI_PWM_STAT_REG_UNION   PwmStatReg;     /* PWM Status Register                    */
    static ADI_PWM_GATE_REG_UNION   PwmGateReg;     /* PWM Chopping Control                   */       
    static ADI_PWM_SEG_REG_UNION    PwmSegReg;      /* PWM Crossover and Output Enable        */
    static ADI_PWM_LSI_REG_UNION    PwmLowSideInvertReg; /* PWM Low Side Invert (SR mode only) */
    static ADI_PWM_STAT2_REG_UNION  PwmStat2Reg;    /* PWM Status Register                    */
   
#endif 

/*********************************************************************

 Callback entry structure used to build the callback list 
  
**********************************************************************/

typedef struct ADI_PWM_CALLBACK_ENTRY 
{
    void *ClientHandle;                  /* defined by the app; passed to callbacks */
    ADI_DCB_HANDLE DCBHandle;            /* handle to deferred callback service     */
    ADI_DCB_CALLBACK_FN ClientCallback;  /* name of client callback function        */
} ADI_PWM_CALLBACK_ENTRY;



/*********************************************************************

  Static storage to manage the PWM
  
**********************************************************************/

 
typedef struct ADI_PWM_Config    
{ 
    u8  InitFlag;            /* indicates the PWM service was initialized */
    u8  ClientCallbackCount;   /* number of callback events the client has */
    ADI_PWM_CALLBACK_ENTRY CallbackEntry[ADI_PWM_MAX_EVENTS];  /* data for the client callbacks */    

#if defined(__ADSP_MOY__) 
    /* Store the IVG level of the Trip and Sync ISR for each of the two PWMs */
    u32  TRIP_0_IVG;         /* IVG for Trip0 ISR       */
    u32  SYNC_0_IVG;         /* IVG for SYnc0 ISR       */      
    u32  TRIP_1_IVG;         /* IVG for Trip1 ISR       */
    u32  SYNC_1_IVG;         /* IVG for SYnc1 ISR       */      
#else   
   /* Store the IVG level of the Trip and Sync ISR      */
    u32  TRIP_IVG;           /* IVG for Trip ISR        */
    u32  SYNC_IVG;           /* IVG for SYnc ISR        */      
#endif    

   void      *pEnterCriticalArg;       /* value for entering crtical regions */

/******************************************************************************  
 
 Pointer to Register structure unions which reflect the hardware register values 
 
*******************************************************************************/   

#if defined(__ADSP_MOY__)  
    /* PWM 0 and 1 */
    pADI_PWM_CTRL_REG_UNION  pPwmCtrlReg[ADI_NUM_PWM];     /* storage PWM Control Register           */
    pADI_PWM_STAT_REG_UNION  pPwmStatReg[ADI_NUM_PWM];     /* PWM Status Register                    */
    pADI_PWM_GATE_REG_UNION  pPwmGateReg[ADI_NUM_PWM];     /* PWM Chopping Control                   */       
    pADI_PWM_SEG_REG_UNION   pPwmSegReg[ADI_NUM_PWM];      /* PWM Crossover and Output Enable        */
    pADI_PWM_LSI_REG_UNION   pPwmLowSideInvertReg[ADI_NUM_PWM]; /* PWM Low Side Invert (SR mode only) */
    pADI_PWM_STAT2_REG_UNION pPwmStat2Reg[ADI_NUM_PWM];    /* PWM Status Register                    */
           
#else    
    pADI_PWM_CTRL_REG_UNION  pPwmCtrlReg;     /* storage PWM Control Register           */
    pADI_PWM_STAT_REG_UNION  pPwmStatReg;     /* PWM Status Register                    */
    pADI_PWM_GATE_REG_UNION  pPwmGateReg;     /* PWM Chopping Control                   */       
    pADI_PWM_SEG_REG_UNION   pPwmSegReg;      /* PWM Crossover and Output Enable        */
    pADI_PWM_LSI_REG_UNION   pPwmLowSideInvertReg;  /* PWM Low Side Invert (SR mode only) */
    pADI_PWM_STAT2_REG_UNION pPwmStat2Reg;    /* PWM Status Register                    */
#endif    
    
/******************************************************************************

  These registers have only one field.  A 32 bit unsigned value will suffice
   
*******************************************************************************/
#if  defined(__ADSP_MOY__) 
          
    volatile u32    PwmTmReg[ADI_NUM_PWM];       /* PWM Period Register           */
    volatile u32    PwmDtReg[ADI_NUM_PWM];       /* PWM Dead Time Register        */
    volatile u32    PwmSyncWtReg[ADI_NUM_PWM];   /* PWM Sync Pulse Width Register */       
    volatile u32    PwmChAReg[ADI_NUM_PWM];      /* PWM Channel A Duty Control    */
    volatile u32    PwmChBReg[ADI_NUM_PWM];      /* PWM Channel B Duty Control    */
    volatile u32    PwmChCReg[ADI_NUM_PWM];      /* PWM Channel C Duty Control    */
    volatile u32    PwmChALReg[ADI_NUM_PWM];     /* PWM Channel AL Duty Control (SR mode only) */
    volatile u32    PwmChBLReg[ADI_NUM_PWM];     /* PWM Channel BL Duty Control (SR mode only) */
    volatile u32    PwmChCLReg[ADI_NUM_PWM];     /* PWM Channel CL Duty Control (SR mode only) */

 
#else   /* Brodie */
    volatile u32    PwmTmReg;       /* PWM Period Register            */
    volatile u32    PwmDtReg;       /* PWM Dead Time Register         */
    volatile u32    PwmSyncWtReg;   /* PWM Sync Pulse Width Register  */       
    volatile u32    PwmChAReg;      /* PWM Channel A Duty Control     */
    volatile u32    PwmChBReg;      /* PWM Channel B Duty Control     */
    volatile u32    PwmChCReg;      /* PWM Channel C Duty Control     */
    volatile u32    PwmChALReg;     /* PWM Channel AL Duty Control (SR mode only) */
    volatile u32    PwmChBLReg;     /* PWM Channel BL Duty Control (SR mode only) */
    volatile u32    PwmChCLReg;     /* PWM Channel CL Duty Control (SR mode only) */

#endif   

} ADI_PWM_Config, *pADI_PWM_Config;
 
/* declare a config structure to store the PWM status */
static ADI_PWM_Config AdiPwmConfig; 

/* Moy Ports Tables */

#if defined(__ADSP_MOY__) 
    /* primary group of port settings for PWM 0 */
    ADI_PORTS_DIRECTIVE PrimaryPortsTable[ADI_PWM_NUM_MUX_TABLE_ENTRIES] = 
    {
        ADI_PORTS_DIRECTIVE_PWM0_AH,
        ADI_PORTS_DIRECTIVE_PWM0_AL,  
        ADI_PORTS_DIRECTIVE_PWM0_BH,   
        ADI_PORTS_DIRECTIVE_PWM0_BL,   
        ADI_PORTS_DIRECTIVE_PWM0_CH,   
        ADI_PORTS_DIRECTIVE_PWM0_CL,    
        ADI_PORTS_DIRECTIVE_PWM0_TRIP_PF6,
        ADI_PORTS_DIRECTIVE_PWM0_SYNC_PF7,        
        ADI_PORTS_DIRECTIVE_PWM1_AH,
        ADI_PORTS_DIRECTIVE_PWM1_AL,  
        ADI_PORTS_DIRECTIVE_PWM1_BH,   
        ADI_PORTS_DIRECTIVE_PWM1_BL,   
        ADI_PORTS_DIRECTIVE_PWM1_CH,   
        ADI_PORTS_DIRECTIVE_PWM1_CL,   
        ADI_PORTS_DIRECTIVE_PWM1_SYNC, 
        ADI_PORTS_DIRECTIVE_PWM1_TRIP           
    }; 
 
    /* secondary group of port settings for PWM 0 
      Only the Trip and SYnc signals have secondary port mappings */    
    ADI_PORTS_DIRECTIVE SecondaryPortsTable[ADI_PWM_NUM_MUX_TABLE_ENTRIES] = 
    {
        ADI_PORTS_DIRECTIVE_PWM0_AH,
        ADI_PORTS_DIRECTIVE_PWM0_AL,  
        ADI_PORTS_DIRECTIVE_PWM0_BH,   
        ADI_PORTS_DIRECTIVE_PWM0_BL,   
        ADI_PORTS_DIRECTIVE_PWM0_CH,   
        ADI_PORTS_DIRECTIVE_PWM0_CL,   
        ADI_PORTS_DIRECTIVE_PWM0_TRIP_PF11,
        ADI_PORTS_DIRECTIVE_PWM0_SYNC_PF12,      
        ADI_PORTS_DIRECTIVE_PWM1_AH,
        ADI_PORTS_DIRECTIVE_PWM1_AL,  
        ADI_PORTS_DIRECTIVE_PWM1_BH,   
        ADI_PORTS_DIRECTIVE_PWM1_BL,   
        ADI_PORTS_DIRECTIVE_PWM1_CH,   
        ADI_PORTS_DIRECTIVE_PWM1_CL,   
        ADI_PORTS_DIRECTIVE_PWM1_SYNC, 
        ADI_PORTS_DIRECTIVE_PWM1_TRIP               
    };     
     
    
#else

/* Brodie - Table to configure the ports manager service for PWM usage */
    
    ADI_PORTS_DIRECTIVE PrimaryPortsTable[ADI_PWM_NUM_MUX_TABLE_ENTRIES] = 
    {

       ADI_PORTS_DIRECTIVE_PWM_PRI_AH,
       ADI_PORTS_DIRECTIVE_PWM_PRI_AL,  
       ADI_PORTS_DIRECTIVE_PWM_PRI_BH,   
       ADI_PORTS_DIRECTIVE_PWM_PRI_BL,   
       ADI_PORTS_DIRECTIVE_PWM_PRI_CH,   
       ADI_PORTS_DIRECTIVE_PWM_PRI_CL,   
       ADI_PORTS_DIRECTIVE_PWM_PRI_SYNC, 
       ADI_PORTS_DIRECTIVE_PWM_TRIPB 
    };
 
    ADI_PORTS_DIRECTIVE SecondaryPortsTable[ADI_PWM_NUM_MUX_TABLE_ENTRIES] = 
    {
       ADI_PORTS_DIRECTIVE_PWM_SEC_AH,
       ADI_PORTS_DIRECTIVE_PWM_SEC_AL,  
       ADI_PORTS_DIRECTIVE_PWM_SEC_BH,   
       ADI_PORTS_DIRECTIVE_PWM_SEC_BL,   
       ADI_PORTS_DIRECTIVE_PWM_SEC_CH,   
       ADI_PORTS_DIRECTIVE_PWM_SEC_CL,   
       ADI_PORTS_DIRECTIVE_PWM_SEC_SYNC, 
       ADI_PORTS_DIRECTIVE_PWM_TRIPB        
    }; 
#endif   /* Brodie Ports tables */    



#if !defined(__ADSP_MOY__)    


/*********************************************************************
**    Function:    PWM Trip Interrupt Handler for Brodie processor  **                
**                                                                  ** 
**    Description:  Called from the Interrupt Manager when the PWM  **
**               Trip Interrupt is serviced.                        **               
*********************************************************************/


static ADI_INT_HANDLER (PWMTripHandler)
{

    ADI_INT_HANDLER_RESULT  Result;           /* return code */
    u32          i; 
    ADI_PWM_CALLBACK_ENTRY  *pCallbackEntry;   /* callback entry */

    /* If nobody changes this, we will tell the Interrupt Manager that the interrupt was not for us */
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    /* Read in the PWM Status Register first  */
    PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;     
        
    /* IF PWM is asserting a trip interrupt -- Trip B resets the PWM !! */
    if (adi_int_SICInterruptAsserted(ADI_INT_PWM_TRIP) == ADI_INT_RESULT_ASSERTED) 
    {
       adi_pwm_Control(ADI_PWM_CMD_CLEAR_TRIP_INT, (void*) 1);
       
       /* check the callback table */
       /* IF (we have a deferred callback installed for the trip event) */
       i = (ADI_PWM_EVENT_TRIP-ADI_PWM_EVENT_START)-1;
       
       if ((AdiPwmConfig.CallbackEntry[i].ClientCallback)  != NULL)
       {               
          /* execute the deferred callback */
          if (AdiPwmConfig.CallbackEntry[i].DCBHandle)   
          {
             adi_dcb_Post(AdiPwmConfig.CallbackEntry[i].DCBHandle, 0, 
                 AdiPwmConfig.CallbackEntry[i].ClientCallback, AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM_EVENT_TRIP, NULL);
          } 
          else 
          {
             /* execute the immediate callback */                       
             (AdiPwmConfig.CallbackEntry[i].ClientCallback)(AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM_EVENT_TRIP, NULL);
          }
       }          
       /* indicate to the Interrupt Manager that the interrupt was processed */
       Result = ADI_INT_RESULT_PROCESSED;
    } 

   
    /* return the result to the Interrupt Manager */
    return(Result);
}

 
/*********************************************************************
**    Function:    PWM Sync Interrupt Handler for Brodie processor  **                
**                                                                  ** 
**    Description:  Called from the Interrupt Manager when the PWM  **
**               Sync Interrupt is serviced.                        **               
*********************************************************************/

static ADI_INT_HANDLER (PWMSyncHandler)
{

    ADI_INT_HANDLER_RESULT  Result;           /* return code */
    u32                 i; 
    ADI_PWM_CALLBACK_ENTRY  *pCallbackEntry;   /* callback entry */

    /* If nobody changes this, we will tell the Interrupt Manager that the interrupt was not for us */
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    /* Read in the PWM Status Register first  */
    PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;     
        

    /* IF PWM is asserting a sync interrupt */
    if (adi_int_SICInterruptAsserted(ADI_INT_PWM_SYNC) == ADI_INT_RESULT_ASSERTED)
    {       
        adi_pwm_Control(ADI_PWM_CMD_CLEAR_SYNC_INT, (void*) 1);
        
        /* check the callback table */  
        /* IF (we have a deferred callback installed for the sync event) */
        i = (ADI_PWM_EVENT_SYNC-ADI_PWM_EVENT_START)-1;
       
        if ((AdiPwmConfig.CallbackEntry[i].ClientCallback)  != NULL)
        {               
           /* execute the deferred callback */
           if (AdiPwmConfig.CallbackEntry[i].DCBHandle)   
           {
              adi_dcb_Post(AdiPwmConfig.CallbackEntry[i].DCBHandle, 0, AdiPwmConfig.CallbackEntry[i].ClientCallback, AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM_EVENT_SYNC, NULL);
           } 
           else 
           /* execute the immediate callback */          
           {
              (AdiPwmConfig.CallbackEntry[i].ClientCallback)(AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM_EVENT_SYNC, NULL);
           }
           /* indicate to the Interrupt Manager that the interrupt was processed */
        }
        Result = ADI_INT_RESULT_PROCESSED;
    }          

    /* return the result to the Interrupt Manager */
    return(Result);
}

/* End of Brodie Interrupt Handlers */
#else 
/* Moy Trip and Sync Handlers (PWM 0 and PWM 1) */

    
    
    
/*********************************************************************
**  Function:    PWM0 Trip Interrupt Handler for Moy processor      **        
**                                                                  **
**  Description:  Called from the Interrupt Manager when the PWM0   **
**             Trip Interrupt is serviced.                          **              
*********************************************************************/

static ADI_INT_HANDLER (PWM0TripHandler)
{

    ADI_INT_HANDLER_RESULT  Result;           /* return code */
    u32                 i; 
    ADI_PWM_CALLBACK_ENTRY  *pCallbackEntry;    /* callback entry */

    /* If nobody changes this, we will tell the Interrupt Manager that the interrupt was not for us */
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    /* Read in the PWM0 Status Register first  */
    PwmStatReg[ADI_PWM_0].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_0];     
        
    /* IF PWM0 is asserting a trip interrupt -- Trip B resets the PWM0 !! */
    if (adi_int_SICInterruptAsserted(ADI_INT_PWM0_TRIP) == ADI_INT_RESULT_ASSERTED) 
    {
       adi_pwm_Control(ADI_PWM_CMD_CLEAR_TRIP_INT, ADI_PWM_0);
       
       /* check the callback table */
       /* IF (we have a deferred callback installed for the trip event) */
       i = (ADI_PWM0_EVENT_TRIP-ADI_PWM_EVENT_START)-1;
       
       if ((AdiPwmConfig.CallbackEntry[i].ClientCallback) != NULL)
       {               
          /* execute the deferred callback */
          if (AdiPwmConfig.CallbackEntry[i].DCBHandle)   
          {
             adi_dcb_Post(AdiPwmConfig.CallbackEntry[i].DCBHandle, 0, AdiPwmConfig.CallbackEntry[i].ClientCallback, AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM0_EVENT_TRIP, NULL);
          } 
          else 
          {
             /* execute the immediate callback */                       
             (AdiPwmConfig.CallbackEntry[i].ClientCallback)(AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM0_EVENT_TRIP, NULL);
          }
       }          
       /* indicate to the Interrupt Manager that the interrupt was processed */
       Result = ADI_INT_RESULT_PROCESSED;
    } 

   
    /* return the result to the Interrupt Manager */
    return(Result);
}

    
/*********************************************************************
**  Function:    PWM1 Trip Interrupt Handler for Moy processor      **        
**                                                                  **
**  Description:  Called from the Interrupt Manager when the PWM1   **
**             Trip Interrupt is serviced.                          **              
*********************************************************************/

static ADI_INT_HANDLER (PWM1TripHandler)
{

    ADI_INT_HANDLER_RESULT  Result;           /* return code */
    u32                 i; 
    ADI_PWM_CALLBACK_ENTRY  *pCallbackEntry;    /* callback entry */

    /* If nobody changes this, we will tell the Interrupt Manager that the interrupt was not for us */
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    /* Read in the PWM1 Status Register first  */
    PwmStatReg[ADI_PWM_1].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_1];     
        
    /* IF PWM1 is asserting a trip interrupt -- Trip B resets the PWM1 !! */
    if (adi_int_SICInterruptAsserted(ADI_INT_PWM1_TRIP) == ADI_INT_RESULT_ASSERTED) 
    {
      adi_pwm_Control(ADI_PWM_CMD_CLEAR_TRIP_INT, (void*)ADI_PWM_1);
       
       /* check the callback table */
       /* IF (we have a deferred callback installed for the trip event) */
       i = (ADI_PWM1_EVENT_TRIP-ADI_PWM_EVENT_START)-1;
       
       if ((AdiPwmConfig.CallbackEntry[i].ClientCallback) != NULL)
       {               
          /* execute the deferred callback */
          if (AdiPwmConfig.CallbackEntry[i].DCBHandle)   
          {
             adi_dcb_Post(AdiPwmConfig.CallbackEntry[i].DCBHandle, 0, AdiPwmConfig.CallbackEntry[i].ClientCallback, AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM1_EVENT_TRIP, NULL);
          } 
          else 
          {
             /* execute the immediate callback */                       
             (AdiPwmConfig.CallbackEntry[i].ClientCallback)(AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM1_EVENT_TRIP, NULL);
          }
       }          
       /* indicate to the Interrupt Manager that the interrupt was processed */
       Result = ADI_INT_RESULT_PROCESSED;
    } 

   
    /* return the result to the Interrupt Manager */
    return(Result);
}



/***********************************************************************
**    Function:    PWM0 Sync Interrupt Handler for Moy processor      **            
**                                                                    ** 
**    Description:  Called from the Interrupt Manager when the PWM0   **
**               Sync Interrupt is serviced.                          **
***********************************************************************/

static ADI_INT_HANDLER (PWM0SyncHandler)
{

    ADI_INT_HANDLER_RESULT  Result;           /* return code */
    u32                 i; 
    ADI_PWM_CALLBACK_ENTRY  *pCallbackEntry;    /* callback entry */

    /* If nobody changes this, we will tell the Interrupt Manager that the interrupt was not for us */
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    /* Read in the PWM Status Register first  */
    PwmStatReg[ADI_PWM_0].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_0];     
        

    /* IF PWM is asserting a sync interrupt */
    if (adi_int_SICInterruptAsserted(ADI_INT_PWM0_SYNC) == ADI_INT_RESULT_ASSERTED)
    {       
       adi_pwm_Control(ADI_PWM_CMD_CLEAR_SYNC_INT, (void*) ADI_PWM_0);
        
       /* check the callback table */  
       /* IF (we have a deferred callback installed for the sync event) */
       i = (ADI_PWM0_EVENT_SYNC-ADI_PWM_EVENT_START)-1;
       
       if ((AdiPwmConfig.CallbackEntry[i].ClientCallback) != NULL)
       {               
          /* execute the deferred callback */
          if (AdiPwmConfig.CallbackEntry[i].DCBHandle)   
          {
             adi_dcb_Post(AdiPwmConfig.CallbackEntry[i].DCBHandle, 0, AdiPwmConfig.CallbackEntry[i].ClientCallback, AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM0_EVENT_SYNC, NULL);
          } 
          else 
          /* execute the immediate callback */          
          {
             (AdiPwmConfig.CallbackEntry[i].ClientCallback)(AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM0_EVENT_SYNC, NULL);
          }
          /* indicate to the Interrupt Manager that the interrupt was processed */
       }
       Result = ADI_INT_RESULT_PROCESSED;
    }          

    /* return the result to the Interrupt Manager */
    return(Result);
}

/***********************************************************************
**    Function:    PWM1 Sync Interrupt Handler for Moy processor      **            
**                                                                    ** 
**    Description:  Called from the Interrupt Manager when the PWM1   **
**               Sync Interrupt is serviced.                          **
***********************************************************************/

static ADI_INT_HANDLER (PWM1SyncHandler)
{

    ADI_INT_HANDLER_RESULT  Result;           /* return code */
    u32                 i; 
    ADI_PWM_CALLBACK_ENTRY  *pCallbackEntry;    /* callback entry */

    /* If nobody changes this, we will tell the Interrupt Manager that the interrupt was not for us */
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    /* Read in the PWM Status Register first  */
    PwmStatReg[ADI_PWM_1].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_1];     
        

    /* IF PWM is asserting a sync interrupt */
    if (adi_int_SICInterruptAsserted(ADI_INT_PWM1_SYNC) == ADI_INT_RESULT_ASSERTED)
    {       
       adi_pwm_Control(ADI_PWM_CMD_CLEAR_SYNC_INT, (void*) ADI_PWM_1);
        
       /* check the callback table */  
       /* IF (we have a deferred callback installed for the sync event) */
       i = (ADI_PWM1_EVENT_SYNC-ADI_PWM_EVENT_START)-1;
       
       if ((AdiPwmConfig.CallbackEntry[i].ClientCallback) != NULL)
       {               
          /* execute the deferred callback */
          if (AdiPwmConfig.CallbackEntry[i].DCBHandle)   
          {
             adi_dcb_Post(AdiPwmConfig.CallbackEntry[i].DCBHandle, 0, AdiPwmConfig.CallbackEntry[i].ClientCallback, AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM1_EVENT_SYNC, NULL);
          } 
          else 
          /* execute the immediate callback */          
          {
             (AdiPwmConfig.CallbackEntry[i].ClientCallback)(AdiPwmConfig.CallbackEntry[i].ClientHandle, ADI_PWM1_EVENT_SYNC, NULL);
          }
          /* indicate to the Interrupt Manager that the interrupt was processed */
       }
       Result = ADI_INT_RESULT_PROCESSED;
    }          

    /* return the result to the Interrupt Manager */
    return(Result);
}



#endif  /* End of Moy Interrupt Handlers */



/*********************************************************************
**    Function:    adi_pwm_Init                                     **                                                      
**    Description:  Initializes the Pulse Width Modulation Service  **                                                        
**********************************************************************/
   

/* These are used in debug mode, to insure that all necessary parameters 
   are set by the Init function  */

#if defined(ADI_SSL_DEBUG)

#if defined(__ADSP_MOY__)   
    /* These are used only for Moy PWM 0/1 */   
    static u8 PortMuxSet;
    static u8 PeriodSet[ADI_NUM_PWM];
    static u8 DeadSet[ADI_NUM_PWM];
    static u8 DutySetA[ADI_NUM_PWM];
    static u8 DutySetB[ADI_NUM_PWM];  
    static u8 DutySetC[ADI_NUM_PWM];   
    static u8 DutySetAL[ADI_NUM_PWM];
    static u8 DutySetBL[ADI_NUM_PWM];  
    static u8 DutySetCL[ADI_NUM_PWM];        
    static u8 PulseWidthSet[ADI_NUM_PWM];
    static u8 OperModeSet[ADI_NUM_PWM];    
    static u8 ChannelEnableSet[ADI_NUM_PWM];
    static u8 PolaritySet[ADI_NUM_PWM];
       
#else
    /* These are used by  Brodie */
    static u8 PortMuxSet;
    static u8 PeriodSet;
    static u8 DeadSet;
    static u8 DutySetA;
    static u8 DutySetB;  
    static u8 DutySetC;   
    static u8 DutySetAL;
    static u8 DutySetBL;  
    static u8 DutySetCL;        
    static u8 PulseWidthSet;
    static u8 OperModeSet;    
    static u8 ChannelEnableSet;
    static u8 PolaritySet;
      
#endif  /* Moy */  
#endif  /* Debug */
 


#if defined(ADI_SSL_DEBUG)            
/******************************************************************************
 ***  Function: adi_pwmCheckInitParameters()                                ***
 ***  Description: Checks that all required parameters have been passed     ***
 ***  for each enabled PWM.  Called from the end of the Init function and   ***
 ***  when a PWM is being enabled by a command.                             ***
 ******************************************************************************/
            
ADI_PWM_RESULT adi_pwmCheckInitParameters()
{        
        
    /* In debug mode, make sure all required initialization parameters were set */
#if defined(__ADSP_MOY__)      
    ADI_PWM_NUMBER i;        
    if (!(PortMuxSet))
            return ADI_PWM_RESULT_PORT_MUX_MAPPING_NOT_SET;
            
    for(i=ADI_PWM_0; i<ADI_NUM_PWM; i++)
    {
        if (AdiPwmConfig.pPwmCtrlReg[i]->BitField.Enable == ADI_PWM_ENABLE)
        {            
            /* PWM 0 is enabled, check that all PWM 0 parameters are set */
 
            if (!(PeriodSet[i]))
                return ADI_PWM_RESULT_PERIOD_NOT_SET;
                       
            if (!(DeadSet[i]))
                return ADI_PWM_RESULT_DEAD_TIME_NOT_SET;
        
            if (!(PulseWidthSet[i]))
                return ADI_PWM_RESULT_SYNC_PULSE_WIDTH_NOT_SET;
                  
            if (!(OperModeSet[i]))
                return ADI_PWM_RESULT_OPERATING_MODE_NOT_SET;

            if (!(ChannelEnableSet[i]))
                return ADI_PWM_RESULT_CHANNEL_ENABLE_NOT_SET;
            
            if (!(PolaritySet[i]))
                return ADI_PWM_RESULT_POLARITY_NOT_SET; 


        /* Check that all enabled channels have duty cycles set */
         if (((AdiPwmConfig.pPwmSegReg[i]->BitField.AH_OutputEnable == ADI_PWM_ENABLE)
             || (AdiPwmConfig.pPwmSegReg[i]->BitField.AL_OutputEnable == ADI_PWM_ENABLE))  && (!(DutySetA[i])))
            return ADI_PWM_RESULT_DUTY_CYCLE_A_NOT_SET;
            
        if (((AdiPwmConfig.pPwmSegReg[i]->BitField.BH_OutputEnable == ADI_PWM_ENABLE)
             || (AdiPwmConfig.pPwmSegReg[i]->BitField.BL_OutputEnable == ADI_PWM_ENABLE))  && (!(DutySetB[i])))
            return ADI_PWM_RESULT_DUTY_CYCLE_B_NOT_SET;
            
        if (((AdiPwmConfig.pPwmSegReg[i]->BitField.CH_OutputEnable == ADI_PWM_ENABLE)
             || (AdiPwmConfig.pPwmSegReg[i]->BitField.CL_OutputEnable == ADI_PWM_ENABLE))  && (!(DutySetC[i])))
            return ADI_PWM_RESULT_DUTY_CYCLE_C_NOT_SET;    
        } /* end if enabled */        
    } /* end for */
                                
#else  /* Not Moy */

/* Check that all enabled channels have duty cycles set */
    if (((AdiPwmConfig.pPwmSegReg->BitField.AH_OutputEnable == ADI_PWM_ENABLE)
         || (AdiPwmConfig.pPwmSegReg->BitField.AL_OutputEnable == ADI_PWM_ENABLE))  && (!(DutySetA)))
        return ADI_PWM_RESULT_DUTY_CYCLE_A_NOT_SET;
            
    if (((AdiPwmConfig.pPwmSegReg->BitField.BH_OutputEnable == ADI_PWM_ENABLE)
         || (AdiPwmConfig.pPwmSegReg->BitField.BL_OutputEnable == ADI_PWM_ENABLE))  && (!(DutySetB)))
        return ADI_PWM_RESULT_DUTY_CYCLE_B_NOT_SET;
            
    if (((AdiPwmConfig.pPwmSegReg->BitField.CH_OutputEnable == ADI_PWM_ENABLE)
         || (AdiPwmConfig.pPwmSegReg->BitField.CL_OutputEnable == ADI_PWM_ENABLE))  && (!(DutySetC)))
        return ADI_PWM_RESULT_DUTY_CYCLE_C_NOT_SET;
            

    if (!(PortMuxSet))
        return ADI_PWM_RESULT_PORT_MUX_MAPPING_NOT_SET;

    if (!(PeriodSet))
        return ADI_PWM_RESULT_PERIOD_NOT_SET;
                       
    if (!(DeadSet))
        return ADI_PWM_RESULT_DEAD_TIME_NOT_SET;
        
    if (!(PulseWidthSet))
        return ADI_PWM_RESULT_SYNC_PULSE_WIDTH_NOT_SET;
                  
    if (!(OperModeSet))
        return ADI_PWM_RESULT_OPERATING_MODE_NOT_SET;

    if (!(ChannelEnableSet))
        return ADI_PWM_RESULT_CHANNEL_ENABLE_NOT_SET;
            
    if (!(PolaritySet))
        return ADI_PWM_RESULT_POLARITY_NOT_SET;   

#endif      /* Moy */


    return ADI_PWM_RESULT_SUCCESS;
}

#endif /* debug */





/******************************************************************************
 ***  Function: adi_pwm_Init                                                ***
 ***  Description:  Main initialization function for the PWM service.       ***
 ***  Called  to initialize all parameters to use the PWM.                  ***
 *****************************************************************************/
 

ADI_PWM_RESULT adi_pwm_Init(const ADI_PWM_COMMAND_PAIR *table,    /* table of commands to configure the module */
       void *pCriticalRegionArg )         /* parameter for the critical region   */    
{

    ADI_PWM_CALLBACK_ENTRY  *pCallbackEntry;   
    ADI_PWM_RESULT        Result;          
    AdiPwmConfig.ClientCallbackCount = 0;                
              
    ADI_PWM_COMMAND_PAIR *cmd = (ADI_PWM_COMMAND_PAIR *)table;

#if defined(__ADSP_MOY__) 
ADI_PWM_NUMBER i;
#endif

#if defined(ADI_SSL_DEBUG)

#if defined(__ADSP_MOY__) 


    PortMuxSet     = 0;
    for(i=ADI_PWM_0; i<ADI_NUM_PWM; i++)
    { 

        PeriodSet[i]      = 0;
        DeadSet[i]        = 0;
        DutySetA[i]       = 0;
        DutySetB[i]       = 0;
        DutySetC[i]       = 0;    
        DutySetAL[i]      = 0;
        DutySetBL[i]      = 0;
        DutySetCL[i]      = 0;      
        PulseWidthSet[i]   = 0;
        OperModeSet[i]    = 0;
        ChannelEnableSet[i] = 0;
        PolaritySet[i]     = 0;
    }
#else
    PortMuxSet     = 0;
    PeriodSet      = 0;
    DeadSet        = 0;
    DutySetA       = 0;
    DutySetB       = 0;
    DutySetC       = 0;    
    DutySetAL      = 0;
    DutySetBL      = 0;
    DutySetCL      = 0;      
    PulseWidthSet  = 0;
    OperModeSet    = 0;
    ChannelEnableSet = 0;
    PolaritySet     = 0;    
#endif  /* Moy */
    /* make sure the service is not already initialized */
    if (AdiPwmConfig.InitFlag == TRUE)
    {
       return(ADI_PWM_RESULT_ALREADY_INITIALIZED);
    }
    
#endif /* Debug */

    AdiPwmConfig.InitFlag = TRUE;              


#if defined(__ADSP_MOY__)  
  
    u32 BaseAddress = 0xFFC03700;
    
    for (i=ADI_PWM_0; i< ADI_NUM_PWM; i++)
    {
        /* Moy PWM 0 register Block  (0xFFC03700 - 0xFFC037FF)        */                            

        pADI_PWM_ADDRESS_CTRL[i]   =      ((volatile u16 *)(u32)BaseAddress);        /*  Control Register                   */
        pADI_PWM_ADDRESS_STAT[i]   =      ((volatile u16 *)((u32)BaseAddress+4));    /*  Status Register                    */
        pADI_PWM_ADDRESS_TM[i]     =      ((volatile u16 *)((u32)BaseAddress+8));    /*  Period Register                    */
        pADI_PWM_ADDRESS_DT[i]     =      ((volatile u16 *)((u32)BaseAddress+0xC));  /*  Dead Time Register                 */
        pADI_PWM_ADDRESS_GATE[i]   =      ((volatile u16 *)((u32)BaseAddress+0x10)); /*  Chopping Control                   */
        pADI_PWM_ADDRESS_CHA[i]    =      ((volatile u16 *)((u32)BaseAddress+0x14)); /*  Channel A Duty Control             */
        pADI_PWM_ADDRESS_CHB[i]    =      ((volatile u16 *)((u32)BaseAddress+0x18)); /*  Channel B Duty Control             */
        pADI_PWM_ADDRESS_CHC[i]    =      ((volatile u16 *)((u32)BaseAddress+0x1C)); /*  Channel C Duty Control             */
        pADI_PWM_ADDRESS_SEG[i]    =      ((volatile u16 *)((u32)BaseAddress+0x20)); /*  Crossover and Output Enable        */
        pADI_PWM_ADDRESS_SYNCWT[i] =      ((volatile u16 *)((u32)BaseAddress+0x24)); /*  Sync pulse width control           */
        pADI_PWM_ADDRESS_CHAL[i]   =      ((volatile u16 *)((u32)BaseAddress+0x28)); /*  Channel AL Duty Control (SR mode only)  */
        pADI_PWM_ADDRESS_CHBL[i]   =      ((volatile u16 *)((u32)BaseAddress+0x2C)); /*  Channel BL Duty Control (SR mode only)  */
        pADI_PWM_ADDRESS_CHCL[i]   =      ((volatile u16 *)((u32)BaseAddress+0x30)); /*  Channel CL Duty Control (SR mode only)  */
        pADI_PWM_ADDRESS_LSI[i]    =      ((volatile u16 *)((u32)BaseAddress+0x34)); /*  Low Side Invert (SR mode only)     */
        pADI_PWM_ADDRESS_STAT2[i]  =      ((volatile u16 *)((u32)BaseAddress+0x38)); /*  Status Register                    */


        /* MOY - sense the values from the MMRs into the config struct */
        AdiPwmConfig.PwmTmReg[i] = *pADI_PWM_ADDRESS_TM[i];        /* PWM Period Register   */
        AdiPwmConfig.PwmDtReg[i] = *pADI_PWM_ADDRESS_DT[i];        /* PWM Dead Time Register*/
        AdiPwmConfig.PwmChALReg[i] = *pADI_PWM_ADDRESS_CHCL[i];    /* PWM Channel A Duty Control */
        AdiPwmConfig.PwmChBLReg[i] = *pADI_PWM_ADDRESS_CHBL[i];    /* PWM Channel B Duty Control */
        AdiPwmConfig.PwmChCLReg[i] = *pADI_PWM_ADDRESS_CHCL[i];    /* PWM Channel C Duty Control */
        AdiPwmConfig.PwmChAReg[i] = *pADI_PWM_ADDRESS_CHC[i];      /* PWM Channel A Duty Control */
        AdiPwmConfig.PwmChBReg[i] = *pADI_PWM_ADDRESS_CHB[i];      /* PWM Channel B Duty Control */
        AdiPwmConfig.PwmChCReg[i] = *pADI_PWM_ADDRESS_CHC[i];      /* PWM Channel C Duty Control */
        AdiPwmConfig.PwmSyncWtReg[i]= *pADI_PWM_ADDRESS_SYNCWT[i]; /* PWM Sync pulse width control */
    
        /* MOY - sense the values from the MMRs into the union structures. */
        PwmCtrlReg[i].UnsignedValue = *pADI_PWM_ADDRESS_CTRL[i];     /* PWM Control Register  */
        PwmStatReg[i].UnsignedValue = *pADI_PWM_ADDRESS_STAT[i];     /* PWM Status Register   */
        PwmGateReg[i].UnsignedValue  = *pADI_PWM_ADDRESS_GATE[i];    /* PWM Chopping Control  */
        PwmSegReg[i].UnsignedValue = *pADI_PWM_ADDRESS_SEG[i];       /* PWM Crossover and Output Enable */       
        PwmLowSideInvertReg[i].UnsignedValue  = *pADI_PWM_ADDRESS_LSI[i]; /* PWM Low Side Invert (SR mode only) */
        PwmStat2Reg[i].UnsignedValue = *pADI_PWM_ADDRESS_STAT2[i];    /* PWM Status Register  */
    
    
        /* MOY - From the union structures into the config struct */
        AdiPwmConfig.pPwmCtrlReg[i] = &PwmCtrlReg[i] ;     /* PWM Control Register  */
        AdiPwmConfig.pPwmStatReg[i]= &PwmStatReg[i] ;      /* PWM Status Register   */
        AdiPwmConfig.pPwmGateReg[i] = &PwmGateReg[i] ;      /* PWM Chopping Control  */      
        AdiPwmConfig.pPwmSegReg[i]  = &PwmSegReg[i] ;      /* PWM Crossover and Output Enable */
        AdiPwmConfig.pPwmLowSideInvertReg[i] = &PwmLowSideInvertReg[i] ;  /* PWM Low Side Invert (SR mode only) */
        AdiPwmConfig.pPwmStat2Reg[i] = &PwmStat2Reg[i] ;    /* PWM Status Register  */    
    
        /* Moy PWM 1 register Block  (0xFFC03000 - 0xFFC030FF) */        
        BaseAddress -= 0x0700;
    }    

    ADI_PWM_CHANNEL_STATUS  DisableAllPWM0Channels = { ADI_PWM_CHANNEL_ALL, ADI_PWM0_DISABLE };
    ADI_PWM_CHANNEL_STATUS  DisableAllPWM1Channels = { ADI_PWM_CHANNEL_ALL, ADI_PWM1_DISABLE };
    
    
#else    
    /* BRODIE - sense the values from the MMRs into the config struct */
    AdiPwmConfig.PwmTmReg  = *pADI_PWM_ADDRESS_TM;        /* PWM Period Register   */
    AdiPwmConfig.PwmDtReg  = *pADI_PWM_ADDRESS_DT;        /* PWM Dead Time Register*/
    AdiPwmConfig.PwmChALReg  = *pADI_PWM_ADDRESS_CHAL;    /* PWM Channel A Duty Control */
    AdiPwmConfig.PwmChBLReg  = *pADI_PWM_ADDRESS_CHBL;    /* PWM Channel B Duty Control */
    AdiPwmConfig.PwmChCLReg  = *pADI_PWM_ADDRESS_CHCL;    /* PWM Channel C Duty Control */
    AdiPwmConfig.PwmChAReg  = *pADI_PWM_ADDRESS_CHA;      /* PWM Channel A Duty Control */
    AdiPwmConfig.PwmChBReg  = *pADI_PWM_ADDRESS_CHB;      /* PWM Channel B Duty Control */
    AdiPwmConfig.PwmChCReg  = *pADI_PWM_ADDRESS_CHC;      /* PWM Channel C Duty Control */
       AdiPwmConfig.PwmSyncWtReg = *pADI_PWM_ADDRESS_SYNCWT;  /* PWM Sync pulse width control */

    
    /* BRODIE - sense the values from the MMRs into the union structures. */
    PwmCtrlReg.UnsignedValue = *pADI_PWM_ADDRESS_CTRL;     /* PWM Control Register  */
    PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;     /* PWM Status Register   */
    PwmGateReg.UnsignedValue  = *pADI_PWM_ADDRESS_GATE;    /* PWM Chopping Control  */
    PwmSegReg.UnsignedValue  = *pADI_PWM_ADDRESS_SEG;      /* PWM Crossover and Output Enable */
       
    PwmLowSideInvertReg.UnsignedValue  = *pADI_PWM_ADDRESS_LSI;      /* PWM Low Side Invert (SR mode only) */
    PwmStat2Reg.UnsignedValue = *pADI_PWM_ADDRESS_STAT2;    /* PWM Status Register  */
 
    /* BRODIE - From the union structures into the config struct */
    AdiPwmConfig.pPwmCtrlReg = &PwmCtrlReg;     /* PWM Control Register  */
    AdiPwmConfig.pPwmStatReg = &PwmStatReg;     /* PWM Status Register   */
    AdiPwmConfig.pPwmGateReg = &PwmGateReg;     /* PWM Chopping Control  */      
    AdiPwmConfig.pPwmSegReg  = &PwmSegReg;      /* PWM Crossover and Output Enable */
    AdiPwmConfig.pPwmLowSideInvertReg = &PwmLowSideInvertReg;      /* PWM Low Side Invert (SR mode only) */
    AdiPwmConfig.pPwmStat2Reg = &PwmStat2Reg;    /* PWM Status Register  */
    

    ADI_PWM_CHANNEL_STATUS  DisableAllChannels = { ADI_PWM_CHANNEL_ALL, ADI_PWM_DISABLE };

#endif  /* Brodie / Moy */


    /* save the critical region parameter */
    AdiPwmConfig.pEnterCriticalArg = pCriticalRegionArg;

    /* initialize that we have no client callbacks */
    AdiPwmConfig.ClientCallbackCount = 0;
   

    /* define a command pair table to pass to adi_pwm_Control to initialize the PWM */
    ADI_PWM_COMMAND_PAIR PwmInitTable[] =
     {
#if defined(__ADSP_MOY__)  
        /* Temporarily disable the PWMs */
        { ADI_PWM_CMD_SET_PWM_ENABLE, (void*)ADI_PWM0_DISABLE },
        { ADI_PWM_CMD_SET_PWM_ENABLE, (void*)ADI_PWM1_DISABLE },

        /* Temporarily disable the channels */
        { ADI_PWM_CMD_SET_CHANNEL_ENABLE, (void*)&DisableAllPWM0Channels },
        { ADI_PWM_CMD_SET_CHANNEL_ENABLE, (void*)&DisableAllPWM1Channels },
           
        /* disable the Sync on Output pin */
        { ADI_PWM_CMD_SET_SYNC_OUT_ENABLE, (void*)ADI_PWM0_DISABLE },
        { ADI_PWM_CMD_SET_SYNC_OUT_ENABLE, (void*)ADI_PWM1_DISABLE },

        /* disable the Trip interrupt */
        { ADI_PWM_CMD_SET_TRIP_INT_ENABLE, (void*)ADI_PWM0_DISABLE },
        { ADI_PWM_CMD_SET_TRIP_INT_ENABLE, (void*)ADI_PWM1_DISABLE },
       
        /* disable the Sync interrupt */
        { ADI_PWM_CMD_SET_SYNC_INT_ENABLE, (void*)ADI_PWM0_DISABLE },           
        { ADI_PWM_CMD_SET_SYNC_INT_ENABLE, (void*)ADI_PWM1_DISABLE }, 
                     
    
        /* clear the trip and sync interrupts */
        { ADI_PWM_CMD_CLEAR_TRIP_INT,  (void*)ADI_PWM_0 },
        { ADI_PWM_CMD_CLEAR_SYNC_INT,  (void*)ADI_PWM_0 }, 
        { ADI_PWM_CMD_CLEAR_TRIP_INT,  (void*)ADI_PWM_1 },
        { ADI_PWM_CMD_CLEAR_SYNC_INT,  (void*)ADI_PWM_1 }, 

        /* enable the trip input signals */
        { ADI_PWM_CMD_SET_TRIP_INPUT_ENABLE, (void*)ADI_PWM0_ENABLE },
        { ADI_PWM_CMD_SET_TRIP_INPUT_ENABLE, (void*)ADI_PWM1_ENABLE },
                                      
#else                      
        /* Temporarily disable the PWM */
        { ADI_PWM_CMD_SET_PWM_ENABLE, (void*)ADI_PWM_DISABLE },

        /* Temporarily disable the channels */
        { ADI_PWM_CMD_SET_CHANNEL_ENABLE, (void*)&DisableAllChannels },

        /* disable the Sync on Output pin */
        { ADI_PWM_CMD_SET_SYNC_OUT_ENABLE, (void*)ADI_PWM_DISABLE },

        /* disable the Trip interrupt */
        { ADI_PWM_CMD_SET_TRIP_INT_ENABLE, (void*)ADI_PWM_DISABLE },
       
        /* disable the Sync interrupt */
        { ADI_PWM_CMD_SET_SYNC_INT_ENABLE, (void*)ADI_PWM_DISABLE },
               
        /* clear the trip and sync interrupts */
        { ADI_PWM_CMD_CLEAR_TRIP_INT, (void*) 1 },
        { ADI_PWM_CMD_CLEAR_SYNC_INT, (void*) 1 }, 
           
        /* enable the trip input signal */
        { ADI_PWM_CMD_SET_TRIP_INPUT_ENABLE, (void*)ADI_PWM_ENABLE },
              

#endif       /* Moy */
   

        /* include the application's command table in this table */
        { ADI_PWM_CMD_TABLE, (void*)table },
       
        /* indicate the end of this command pair table */
        { ADI_PWM_CMD_END, (void*)0 }    
    
    };
    
    /* Pass both the PWM service initialization commands, and the application's command table, to adi_pwm_Control */
    Result = adi_pwm_Control(ADI_PWM_CMD_TABLE, (void*)PwmInitTable);
   
    
#if defined(ADI_SSL_DEBUG)
    /* If there was a failure, return the result code to the application, now */         
    if (Result != ADI_PWM_RESULT_SUCCESS) 
      return Result;
#endif                    

 
#if defined(ADI_SSL_DEBUG)

/* Interrupt Handling Initialization for debug mode, with error checking */

#if defined(__ADSP_MOY__)   

/* PWM 0 */
   /* get the IVGs for the PWM0 Trip Interrupt */
    if (adi_int_SICGetIVG(ADI_INT_PWM0_TRIP, &AdiPwmConfig.TRIP_0_IVG) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }       
    /* get the IVG for the PWM0 Sync Interrupt */
    if (adi_int_SICGetIVG(ADI_INT_PWM0_SYNC, &AdiPwmConfig.SYNC_0_IVG) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }    
    /* hook the handler for PWM0Trip Int */
    if (adi_int_CECHook(AdiPwmConfig.TRIP_0_IVG, PWM0TripHandler, NULL, TRUE) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }           
    /* hook the handler for PWM0Sync Int */
    if (adi_int_CECHook(AdiPwmConfig.SYNC_0_IVG, PWM0SyncHandler, NULL, TRUE) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }    

/* PWM 1 */

   /* get the IVGs for the PWM1 Trip Interrupt */
    if (adi_int_SICGetIVG(ADI_INT_PWM1_TRIP, &AdiPwmConfig.TRIP_1_IVG) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }    
    
    /* get the IVG for the PWM1 Sync Interrupt */
    if (adi_int_SICGetIVG(ADI_INT_PWM1_SYNC, &AdiPwmConfig.SYNC_1_IVG) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }    
          
    /* hook the handler for the PWM1Sync Int */
    if (adi_int_CECHook(AdiPwmConfig.SYNC_1_IVG, PWM1SyncHandler, NULL, TRUE) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }    

    /* hook the handler for the PWM1 Trip Int */
    if (adi_int_CECHook(AdiPwmConfig.TRIP_1_IVG, PWM1TripHandler, NULL, TRUE) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    } 
  
 
#else /* not Moy */

    /* get the IVGs for the PWM Trip Interrupt */
    if (adi_int_SICGetIVG(ADI_INT_PWM_TRIP, &AdiPwmConfig.TRIP_IVG) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }    
    
    /* get the IVG for the PWM Sync Interrupt */
    if (adi_int_SICGetIVG(ADI_INT_PWM_SYNC, &AdiPwmConfig.SYNC_IVG) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }    
          
    /* hook the handler for Sync Int */
    if (adi_int_CECHook(AdiPwmConfig.SYNC_IVG, PWMSyncHandler, NULL, TRUE) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }    

    /* hook the handler for Trip Int */
    if (adi_int_CECHook(AdiPwmConfig.TRIP_IVG, PWMTripHandler, NULL, TRUE) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }     
    
#endif    /* Moy */

    if ((Result = adi_pwmCheckInitParameters()) != ADI_PWM_RESULT_SUCCESS)
     
       return Result;
       
#else  /* Release */

/* Interrupt Handling Initialization for release mode, with no error checking */

#if defined(__ADSP_MOY__)  
 
    /* get the IVG for the PWM 0 Trip Int */
    adi_int_SICGetIVG(ADI_INT_PWM0_TRIP, &AdiPwmConfig.TRIP_0_IVG);   
    /* get the IVG for the PWM 0 Sync Int*/
    adi_int_SICGetIVG(ADI_INT_PWM0_SYNC, &AdiPwmConfig.SYNC_0_IVG);
    /* hook the handler for PWM 0 Trip Int */
    adi_int_CECHook(AdiPwmConfig.TRIP_0_IVG, PWM0TripHandler, NULL, TRUE);
       /* hook the handler for PWM 0 Sync Int */
    adi_int_CECHook(AdiPwmConfig.SYNC_0_IVG, PWM0SyncHandler, NULL, TRUE); 


    /* get the IVG for the PWM 1 Trip Int*/
    adi_int_SICGetIVG(ADI_INT_PWM1_TRIP, &AdiPwmConfig.TRIP_1_IVG);    
    /* get the IVG for the PWM Sync Int */
    adi_int_SICGetIVG(ADI_INT_PWM1_SYNC, &AdiPwmConfig.SYNC_1_IVG);
    /* hook the handler for PWM 1 Trip Int */
    adi_int_CECHook(AdiPwmConfig.TRIP_1_IVG, PWM1TripHandler, NULL, TRUE);            
    /* hook the handler for PWM 1 Sync Int */
    adi_int_CECHook(AdiPwmConfig.SYNC_1_IVG, PWM1SyncHandler, NULL, TRUE); 

    
#else  /* Not Moy */
    
    /* get the IVGs for the PWM */
    adi_int_SICGetIVG(ADI_INT_PWM_TRIP, &AdiPwmConfig.TRIP_IVG);
    
    /* get the IVG for the PWM */
    adi_int_SICGetIVG(ADI_INT_PWM_SYNC, &AdiPwmConfig.SYNC_IVG);
          
    /* hook the handler for Sync Int */
    adi_int_CECHook(AdiPwmConfig.SYNC_IVG, PWMSyncHandler, NULL, TRUE); 

    /* hook the handler for Trip Int */
    adi_int_CECHook(AdiPwmConfig.TRIP_IVG, PWMTripHandler, NULL, TRUE); 

#endif  /* Moy */
#endif  /* debug */
        

    /* and return to the caller */
    return ADI_PWM_RESULT_SUCCESS;
}



/********************************************************************
**    Function:    adi_pwm_Terminate                               **
**    Description:  Removes the Pulse Width Modulation Service .   **
********************************************************************/

ADI_PWM_RESULT adi_pwm_Terminate(void)
{
    u32 Result;
  
#if defined(ADI_SSL_DEBUG)

    /* make sure the service is initialized */
    if (AdiPwmConfig.InitFlag == FALSE)
    {
       return (ADI_PWM_RESULT_NOT_INITIALIZED);
    }
    
#endif    
    

#if defined(ADI_SSL_DEBUG)
#if defined(__ADSP_MOY__)  

    /* disable PWM interrupts with error checking */
    if (adi_int_SICDisable(ADI_INT_PWM0_SYNC) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }
    if (adi_int_SICDisable(ADI_INT_PWM0_TRIP)!= ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }
    /* disable PWM interrupts with error checking */
    if (adi_int_SICDisable(ADI_INT_PWM1_SYNC) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }    
    if (adi_int_SICDisable(ADI_INT_PWM1_TRIP)!= ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }
        
    /* unhook the handler for Sync Int with error checking */
    if (adi_int_CECUnhook(AdiPwmConfig.SYNC_0_IVG, PWM0SyncHandler, NULL) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }                
    /* unhook the handler for Trip Int with error checking */
    if (adi_int_CECUnhook(AdiPwmConfig.TRIP_0_IVG, PWM0TripHandler, NULL) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }
        
    /* unhook the handler for Sync Int with error checking */
    if (adi_int_CECUnhook(AdiPwmConfig.SYNC_1_IVG, PWM1SyncHandler, NULL) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }                
    /* unhook the handler for Trip Int with error checking */
    if (adi_int_CECUnhook(AdiPwmConfig.TRIP_1_IVG, PWM1TripHandler, NULL) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }    
    
#else /* Not Moy (Brodie)*/
    /* disable PWM interrupts with error checking */
    if (adi_int_SICDisable(ADI_INT_PWM_SYNC) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }
    
    if (adi_int_SICDisable(ADI_INT_PWM_TRIP)!= ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }
       
    
    /* unhook the handler for Sync Int with error checking */
    if (adi_int_CECUnhook(AdiPwmConfig.SYNC_IVG, PWMSyncHandler, NULL) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }
                

    /* unhook the handler for Trip Int with error checking */
    if (adi_int_CECUnhook(AdiPwmConfig.TRIP_IVG, PWMTripHandler, NULL) != ADI_INT_RESULT_SUCCESS) 
    {
       return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
    }

#endif /* Moy */  
  
#else

/* Not Debug */
#if defined(__ADSP_MOY__)  

    /* disable PWM0 interrupts without error checking */
    adi_int_SICDisable(ADI_INT_PWM0_SYNC);
    adi_int_SICDisable(ADI_INT_PWM0_TRIP);
    adi_int_SICDisable(ADI_INT_PWM1_SYNC);
    adi_int_SICDisable(ADI_INT_PWM1_TRIP);

    /* unhook the handler for Sync0 Int without error checking */
    adi_int_CECUnhook(AdiPwmConfig.SYNC_0_IVG, PWM0SyncHandler, NULL);
    
    /* unhook the handler for Trip0 Int without error checking */
    adi_int_CECUnhook(AdiPwmConfig.TRIP_0_IVG, PWM0TripHandler, NULL);
    
    /* unhook the handler for Sync1 Int without error checking */
    adi_int_CECUnhook(AdiPwmConfig.SYNC_1_IVG, PWM1SyncHandler, NULL);
    
    /* unhook the handler for Trip1 Int without error checking */
    adi_int_CECUnhook(AdiPwmConfig.TRIP_1_IVG, PWM0TripHandler, NULL);

#else
    /* Brodie */
    /* disable PWM interrupts without error checking */
    adi_int_SICDisable(ADI_INT_PWM_SYNC);
    adi_int_SICDisable(ADI_INT_PWM_TRIP);

    /* unhook the handler for Sync Int without error checking */
    adi_int_CECUnhook(AdiPwmConfig.SYNC_IVG, PWMSyncHandler, NULL);
    
    /* unhook the handler for Trip Int without error checking */
    adi_int_CECUnhook(AdiPwmConfig.TRIP_IVG, PWMTripHandler, NULL);
    
#endif /* Moy */
#endif /* Debug */
    
    /* clear the init flag */
    AdiPwmConfig.InitFlag = FALSE;
    
    /* return */
    return ADI_PWM_RESULT_SUCCESS;
}


/***********************************************************************
**    Function:    adi_pwm_InstallCallback                            **
**    Description:  Installs callback functionality for a given event **
***********************************************************************/

ADI_PWM_RESULT adi_pwm_InstallCallback(    
    ADI_PWM_EVENT_ID    EventID,       /* ID of the PWM event                 */
    void             *ClientHandle,    /* client handle argument to callbacks */
    ADI_DCB_HANDLE     DCBHandle,      /* deferred callback service handle    */
    ADI_DCB_CALLBACK_FN ClientCallback  /* name of client callback function   */
) 
{

    void                *pExitCriticalArg;    /* argument returned from 'enter critical region' function */
    ADI_PWM_RESULT        Result;             /* result code to pas back to caller */    
    ADI_PWM_CALLBACK_ENTRY  *pCallbackEntry;  /* pointer to the callback entry */      
    
#if defined(ADI_SSL_DEBUG)

    /* make sure the service is initialized */
    if (AdiPwmConfig.InitFlag == FALSE)
    {
       return(ADI_PWM_RESULT_NOT_INITIALIZED);
    }
    
    /* validate the event ID */    
    if (adi_pwm_ValidateEventID(EventID) !=  ADI_PWM_RESULT_SUCCESS) 
    {
       return(ADI_PWM_RESULT_INVALID_EVENT_ID);
    }
   

#endif

    /* point to the callback entry */
    pCallbackEntry = &AdiPwmConfig.CallbackEntry[EventID - (ADI_PWM_EVENT_START + 1)];

    /* protect this region of code */
    pExitCriticalArg = adi_int_EnterCriticalRegion(AdiPwmConfig.pEnterCriticalArg);       

    /* IF (the event is free) */
    if (pCallbackEntry->ClientCallback == NULL) 
    {      
       /* OK to install the callback, saving the info to pass to the callback */
       pCallbackEntry->ClientHandle     = ClientHandle;
       pCallbackEntry->DCBHandle        = DCBHandle;
       pCallbackEntry->ClientCallback    = ClientCallback;
        
       /* increment the count of client callback events */
       AdiPwmConfig.ClientCallbackCount++;
       
       /* so far so good */
       Result = ADI_PWM_RESULT_SUCCESS;      
    } 
    else 
    {      
       /* the callback is already installed  */
       Result = ADI_PWM_RESULT_CALLBACK_ALREADY_INSTALLED;
    }

    /* unprotect this region of code */
    adi_int_ExitCriticalRegion(pExitCriticalArg);       
 
    /* IF (no errors) */
    if (Result == ADI_PWM_RESULT_SUCCESS) 
    {    
       /* CASEOF (EventID) */
       switch (EventID) 
       {

#if defined(__ADSP_MOY__)  
          /* enable multiple PWM event for Moy */
          case ADI_PWM0_EVENT_SYNC:          
             {
                /* ENABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_SYNC_INT_ENABLE, (void*) ADI_PWM0_ENABLE);          
             }
             break;
       
          case ADI_PWM0_EVENT_TRIP:          
             {
                /* ENABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_TRIP_INT_ENABLE, (void*) ADI_PWM0_ENABLE);    
             }
             break;

          case ADI_PWM1_EVENT_SYNC:          
             {
                /* ENABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_SYNC_INT_ENABLE, (void*) ADI_PWM1_ENABLE);          
             }
             break;
       
          case ADI_PWM1_EVENT_TRIP:          
             {
                /* ENABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_TRIP_INT_ENABLE, (void*) ADI_PWM1_ENABLE);    
             }
             break;

#else
          /* enable the event for Brodie */
          case ADI_PWM_EVENT_SYNC:          
             {
                /* ENABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_SYNC_INT_ENABLE, (void*) ADI_PWM_ENABLE);          
             }
             break;
       
          case ADI_PWM_EVENT_TRIP:          
             {
                /* ENABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_TRIP_INT_ENABLE, (void*) ADI_PWM_ENABLE);    
             }
             break;

#endif  /* Moy/Brodie */

          default:          
             break;
       }
    }
    return(Result);  
}


/**********************************************************************
**   Function:     adi_pwm_RemoveCallback                      **
**   Description:   Removes callback functionality for a given event **
**********************************************************************/

ADI_PWM_RESULT adi_pwm_RemoveCallback(ADI_PWM_EVENT_ID EventID)
{
    
    ADI_PWM_RESULT        Result;           /* result */
    ADI_PWM_CALLBACK_ENTRY  *pCallbackEntry;    /* pointer to the callback entry */      
    void                *pExitCriticalArg;    /* critical region parameter */
    u16                 Bit;             /* the event */
     
#if defined(ADI_SSL_DEBUG)

    /* make sure the service is initialized */
    if (AdiPwmConfig.InitFlag == FALSE)
    {
       return(ADI_PWM_RESULT_NOT_INITIALIZED);
    }
    
    /* validate the event ID */    
    if (adi_pwm_ValidateEventID(EventID) !=  ADI_PWM_RESULT_SUCCESS) 
    {
       return(ADI_PWM_RESULT_INVALID_EVENT_ID);
    }

#endif

    /* point to the callback entry */
    pCallbackEntry = &AdiPwmConfig.CallbackEntry[EventID - (ADI_PWM_EVENT_START + 1)];

    /* protect this region of code */
    pExitCriticalArg = adi_int_EnterCriticalRegion(AdiPwmConfig.pEnterCriticalArg);

    /* IF (the event is active) */
    if (pCallbackEntry->ClientCallback != NULL) 
    {
           
       /* remove the callback from the list */
       pCallbackEntry->ClientCallback    = NULL;
        
       /* decrement the count of client callback events */
       AdiPwmConfig.ClientCallbackCount--;
       
       Result = ADI_PWM_RESULT_SUCCESS;
       
    } 
    else 
    {  
       /* the event isn't installed */
       Result = ADI_PWM_RESULT_CALLBACK_NOT_INSTALLED;
    }

  /* IF (no errors) */
    if (Result == ADI_PWM_RESULT_SUCCESS) 
    {
    
       /* CASEOF (EventID) */
       switch (EventID) 
       {
           
          
#if defined(__ADSP_MOY__) 
          /* disable multiple PWM event for Moy */

          case ADI_PWM0_EVENT_SYNC:          
             {
                /* DISABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_SYNC_INT_ENABLE, (void*) ADI_PWM0_DISABLE);          
             }
             break;       
          case ADI_PWM0_EVENT_TRIP:          
             {
                /* DISABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_TRIP_INT_ENABLE, (void*) ADI_PWM0_DISABLE);    
             }
             break;
          case ADI_PWM1_EVENT_SYNC:          
             {
                /* DISABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_SYNC_INT_ENABLE, (void*) ADI_PWM1_DISABLE);          
             }
             break;
       
          case ADI_PWM1_EVENT_TRIP:          
             {
                /* DISABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_TRIP_INT_ENABLE, (void*) ADI_PWM1_DISABLE);    
             }
             break;
#else
         /* disable event for Brodie */

          case ADI_PWM_EVENT_SYNC:          
             {
                /* DISABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_SYNC_INT_ENABLE, (void*) ADI_PWM_DISABLE);          
             }
             break;
       
          case ADI_PWM_EVENT_TRIP:          
             {
                /* DISABLE the event */
                Result = adi_pwm_Control( ADI_PWM_CMD_SET_TRIP_INT_ENABLE, (void*) ADI_PWM_DISABLE);    
             }
             break;

#endif   /* Moy/Brodie */
          default:          
             break;
       }
    }

    /* unprotect this region of code */
    adi_int_ExitCriticalRegion(pExitCriticalArg);       
    
    /* return */
    return(Result);
}


    
/********************************************************************************
**  Function:     adi_pwm_Control                                              **
**  Description: Sets miscellaneous bits in control regs and queries status.   **
********************************************************************************/

/* global flags to control recursive behaviour */
static u32 RecursiveControlEntry=0;

ADI_PWM_RESULT adi_pwm_Control(ADI_PWM_COMMAND command, void *value)
{
    void    *pExitCriticalArg;    /* argument returned from 'enter critical region' function */    
    ADI_PWM_RESULT result; 
    
#if defined(__ADSP_MOY__)    
    /* Pointer to a struct used to pass in or receive values for a given PWM number */
    pADI_PWM_NUMBER_AND_VALUE pPwmNumValue;   
    pADI_PWM_NUMBER_AND_CHANNEL_STATUS pPwmNumChannelStatus;
    pADI_PWM_NUMBER_AND_ENABLE_STATUS pPwmNumStatus;  
    u8 PWM_NUM;     
#endif /* Moy */

   /* Pointer to a unsigned 32 bit value, used to pass in or receive values  */
    u32 *Value;

        
    RecursiveControlEntry++;
    switch(command) 
    {
        default:
            RecursiveControlEntry--;
            return ADI_PWM_RESULT_INVALID_COMMAND;

        case ADI_PWM_CMD_END:
            break;
        case ADI_PWM_CMD_PAIR:    
          {
             ADI_PWM_COMMAND_PAIR *cmd = (ADI_PWM_COMMAND_PAIR*)value;

#if defined(ADI_SSL_DEBUG)
             if ((result=adi_pwm_Control(cmd->kind, (void*)cmd->value))!=ADI_PWM_RESULT_SUCCESS)
                return result;
#else
             adi_pwm_Control(cmd->kind, (void*)cmd->value);
#endif
          }
          break;
         
        case ADI_PWM_CMD_TABLE:
          {
             ADI_PWM_COMMAND_PAIR *cmd = (ADI_PWM_COMMAND_PAIR *)value;
             while (cmd->kind != ADI_PWM_CMD_END)
             {
#if defined(ADI_SSL_DEBUG)
                if ((result=adi_pwm_Control(cmd->kind, (void*)cmd->value))!=ADI_PWM_RESULT_SUCCESS)
                    return result;
#else
                adi_pwm_Control(cmd->kind, (void*)cmd->value);                    
#endif
                cmd++;
             }
          }
          break;

        case ADI_PWM_CMD_SET_DUTY_CYCLE:
          {
             pADI_PWM_CHANNEL_DUTY_CYCLE pDutyCycle = (pADI_PWM_CHANNEL_DUTY_CYCLE)value;
         
             /* Sets the duty cycle in the config structure and hardware 
                Debug build returns error result if an out-of-range value is passed */
#if defined(ADI_SSL_DEBUG) 
                if ((result = adi_pwmSetDutyCycle(pDutyCycle)) != ADI_PWM_RESULT_SUCCESS)            
                return(result);
#else
             adi_pwmSetDutyCycle(pDutyCycle);
#endif /* debug */
          }
          break;

     

        case ADI_PWM_CMD_SET_PORT_MUX: 
          {                                           
              ADI_PORTS_DIRECTIVE PortMuxTable[ADI_PWM_NUM_MUX_TABLE_ENTRIES];
              u32 Value = *(u32*)value;
              u8 i;

              for (i=0; i<ADI_PWM_NUM_MUX_TABLE_ENTRIES; i++)
              {
                  if (((Value >> i) & 0x01) == 0)
                
                  {
                      PortMuxTable[i] = PrimaryPortsTable[i];
                  }
                  else
                  {
                      PortMuxTable[i] = SecondaryPortsTable[i];
                  }
              }
                adi_ports_Configure(PortMuxTable, ADI_PWM_NUM_MUX_TABLE_ENTRIES);


#if defined(ADI_SSL_DEBUG)
        /* indicate to debug mode, that the parameter has been set */
        PortMuxSet++;
#endif

          }
          break;  
           

        case ADI_PWM_CMD_SET_CHANNEL_ENABLE:        
            {
               /* Can either enable/disable an individual channel, channel pair, or All channels at once */
               /* Logical OR and XOR uses less code than setting the bit fields */
               /* NOTE regarding tar#58519: Previously only the Low Side Invert and Crossover mode commands 
                 supported the channel pair enumerations (like ADI_PWM_CHANNEL_A). Those functions only operate 
                 on channel pairs.  They do not operate on the high side and low side, individually. The PWM 
                 channel enable register is logically ANDed with ~(1<<n) where n = the channel enumeration (eg., 
                 ADI_PWM_CHANNEL_AL).  The enabling of a channel pair can be accomplished by logically ANDing 
                 the PWM channel enable register with ~(3<<n) where n=(2*(channel enumeration-6)).  */
         
               
               pADI_PWM_CHANNEL_STATUS pChannelValue = (pADI_PWM_CHANNEL_STATUS)value;
               
#if defined(ADI_SSL_DEBUG)               
#if defined(__ADSP_MOY__)  
            

                /* validate channel enum */
               if (pChannelValue->Status > ADI_PWM1_ENABLE) 
#else
                /* validate channel enum */
               if (pChannelValue->Status > ADI_PWM_ENABLE)
#endif /* Moy */
                  return ADI_PWM_RESULT_INVALID_ENABLE_STATUS;              
#endif /* debug */

               u32 MaskVal;
               
                if((pChannelValue->Channel==ADI_PWM_CHANNEL_C)      /*   channel pair C  */
                    || (pChannelValue->Channel==ADI_PWM_CHANNEL_B)   /*   channel pair B */    
                    || (pChannelValue->Channel==ADI_PWM_CHANNEL_A))  /*   channel pair A */
                {
                    /* mask for setting a channel pair */
                    MaskVal = (3 << (2*(pChannelValue->Channel-6)));        
                }       
             else
             {
                 /* if ALL channels, use the appropriate mask */
                   if (pChannelValue->Channel == ADI_PWM_CHANNEL_ALL)    
                   {
                      MaskVal = 0x03F;
                   }
                   else
                   {
                       /* Must be a single channel */
                      MaskVal = (1 << pChannelValue->Channel);
                   }
             }
             
             
#if defined(__ADSP_MOY__) 
                /* PWM 1 = 2, 3; PWM 0 = 0, 1 */
                PWM_NUM = (pChannelValue->Status & 0x02)/2;
                 /* PWM 0 enable = 1, 3; disable = 0, 2 */
               if (pChannelValue->Status & 0x01)
               {                   
                   /* enable the channel */
                   AdiPwmConfig.pPwmSegReg[PWM_NUM]->UnsignedValue &= ~MaskVal;               
               }                 
                else
                /* disable = 0, 2 */
                {                                                          
                    AdiPwmConfig.pPwmSegReg[PWM_NUM]->UnsignedValue |= MaskVal;                  
                }

               /* Write Register that selects Channel Crossover and Channel Output Enable */
               *pADI_PWM_ADDRESS_SEG[PWM_NUM] = AdiPwmConfig.pPwmSegReg[PWM_NUM]->UnsignedValue; 
                            
#if defined(ADI_SSL_DEBUG)                                 
               /* indicate to debug mode, that the parameter has been set */
               ChannelEnableSet[PWM_NUM]++;                  
#endif /* debug */                             
#else  /* Not Moy */                           
               /* As written to the register: 1=disabled, 0=enabled  */
               if (pChannelValue->Status == ADI_PWM_DISABLE)
               {                             
                  AdiPwmConfig.pPwmSegReg->UnsignedValue |= MaskVal;        
               }
               else
               {                   
                  /* enable the channel */
                  AdiPwmConfig.pPwmSegReg->UnsignedValue &= ~MaskVal;              
               }   
              
               /* Write Register that selects Channel Crossover and Channel Output Enable */
               *pADI_PWM_ADDRESS_SEG = AdiPwmConfig.pPwmSegReg->UnsignedValue; 
#if defined(ADI_SSL_DEBUG)                                 
                /* indicate to debug mode, that the parameter has been set */
               ChannelEnableSet++;                  
#endif /* debug */                            
#endif               
            }            
            break;                

        case ADI_PWM_CMD_SET_DEAD_TIME:
            {
#if defined(__ADSP_MOY__)

                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
#if defined(ADI_SSL_DEBUG)                
                if (pPwmNumValue->PwmNumber >ADI_PWM_1)
                  return ADI_PWM_RESULT_INVALID_PWM_NUMBER;
                                    
               /*   error checking */
               if (pPwmNumValue->Value > 0x3FF)          
                  return ADI_PWM_RESULT_INVALID_DEAD_TIME;
#endif    /* debug */              
                  

               /* Read in the PWM Status Register first  */
               PwmStatReg[ pPwmNumValue->PwmNumber ].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ pPwmNumValue->PwmNumber ];    
 
               /* if PWM is in switch reluctance mode then dead time is meaningless 
                    Switch Reluctance signal is the opposite of others (1 = disable) */
               if (AdiPwmConfig.pPwmStatReg[ pPwmNumValue->PwmNumber ]->BitField.SRMode == ADI_PWM_DISABLE)                                             
                  return ADI_PWM_RESULT_SWITCH_RELUCTANCE_IS_ACTIVE;
                  
              /* update the value in the config structure */    
               AdiPwmConfig.PwmDtReg[ pPwmNumValue->PwmNumber  ] = pPwmNumValue->Value;
          
               /* write to the hardware */
               *pADI_PWM_ADDRESS_DT[ pPwmNumValue->PwmNumber  ] = AdiPwmConfig.PwmDtReg[ pPwmNumValue->PwmNumber ];    
#if defined(ADI_SSL_DEBUG)    
                             
               /* indicate to debug mode, that the parameter has been set */
               DeadSet[ pPwmNumValue->PwmNumber ]++;
#endif    /* debug */                                                                      
                             
#else  /* not Moy */

               u32 DeadTimeValue = (u32)value;
               
#if defined(ADI_SSL_DEBUG)
               /* Read in the PWM Status Register first  */
               PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;    
  
               /* if in switch reluctance mode then dead time is meaningless 
                Switch Reluctance signal is the opposite of others (1 = disable) */
               if (AdiPwmConfig.pPwmStatReg->BitField.SRMode == ADI_PWM_DISABLE)                                             
                  return ADI_PWM_RESULT_SWITCH_RELUCTANCE_IS_ACTIVE;
             
               /*  add error checking */
               if (DeadTimeValue > 0x3FF)          
                  return ADI_PWM_RESULT_INVALID_DEAD_TIME;
                  
               /* indicate to debug mode, that the parameter has been set */                  
               DeadSet++;
#endif  /* debug */        
               AdiPwmConfig.PwmDtReg = DeadTimeValue;
          
               /* write to the hardware */
               *pADI_PWM_ADDRESS_DT = AdiPwmConfig.PwmDtReg;
               
#endif    /* Moy */    
    
            }
            break;        
         
        case ADI_PWM_CMD_SET_PERIOD:
            {
                                
#if defined(__ADSP_MOY__)                    
                ADI_PWM_NUMBER_AND_VALUE PeriodValue = *(pADI_PWM_NUMBER_AND_VALUE)value;
#if defined(ADI_SSL_DEBUG)                
                if (PeriodValue.PwmNumber >ADI_PWM_1)
                  return ADI_PWM_RESULT_INVALID_PWM_NUMBER;
                                    
               /*  add error checking */
               if((PeriodValue.Value > 0x0FFFF) || (PeriodValue.Value  < 2))        
                  return ADI_PWM_RESULT_INVALID_PERIOD;                  
#endif                                    

               AdiPwmConfig.PwmTmReg[PeriodValue.PwmNumber] = PeriodValue.Value;
               *pADI_PWM_ADDRESS_TM[PeriodValue.PwmNumber] = AdiPwmConfig.PwmTmReg[PeriodValue.PwmNumber];
#if defined(ADI_SSL_DEBUG)                  
               /* indicate to debug mode, that the parameter has been set */                  
               PeriodSet[PeriodValue.PwmNumber]++;                  
#endif                       

                
#else /* not Moy */                
            
                u32 PeriodValue = (u32)value;
    
#if defined(ADI_SSL_DEBUG)
               if((PeriodValue > 0x0FFFF) || (PeriodValue  < 2))
                  return ADI_PWM_RESULT_INVALID_PERIOD;
                  
               /* indicate to debug mode, that the parameter has been set */                  
               PeriodSet++;
#endif          
               AdiPwmConfig.PwmTmReg = PeriodValue;
               *pADI_PWM_ADDRESS_TM = AdiPwmConfig.PwmTmReg;
               
#endif /* Moy */               
               
            }
            break;        

                
        case ADI_PWM_CMD_SET_SYNC_PULSE_WIDTH:
            {
#if defined(__ADSP_MOY__)                

                ADI_PWM_NUMBER_AND_VALUE SyncPulseWidth = *(pADI_PWM_NUMBER_AND_VALUE)value;
#if defined(ADI_SSL_DEBUG)                
                if (SyncPulseWidth.PwmNumber >ADI_PWM_1)
                  return ADI_PWM_RESULT_INVALID_PWM_NUMBER;
                                    
               /*  add error checking */
               if (SyncPulseWidth.Value > 0x03FF)        
                  return ADI_PWM_RESULT_INVALID_PULSE_WIDTH;                  
              
#endif    /* debug */                                
    
               AdiPwmConfig.PwmSyncWtReg[SyncPulseWidth.PwmNumber]= SyncPulseWidth.Value;
               *pADI_PWM_ADDRESS_SYNCWT[SyncPulseWidth.PwmNumber] = AdiPwmConfig.PwmSyncWtReg[SyncPulseWidth.PwmNumber];
#if defined(ADI_SSL_DEBUG)                              
               /* indicate to debug mode, that the parameter has been set */
               PulseWidthSet[SyncPulseWidth.PwmNumber]++;                  
#endif    /* debug */                   
                                        
                                
#else /* not Moy */    
               u32 SyncPulseWidth = (u32)value;
               
#if defined(ADI_SSL_DEBUG)
          
               if (SyncPulseWidth > 0x3FF)
                  return ADI_PWM_RESULT_INVALID_PULSE_WIDTH;
                  
               /* indicate to debug mode, that the parameter has been set */
               PulseWidthSet++;
#endif  /* debug */      
               AdiPwmConfig.PwmSyncWtReg = SyncPulseWidth;   
               *pADI_PWM_ADDRESS_SYNCWT = AdiPwmConfig.PwmSyncWtReg; 
#endif /* Moy */
            }
            break;        
        
        case ADI_PWM_CMD_SET_CROSSOVER:
            {
                
               pADI_PWM_CHANNEL_STATUS pChannelValue = (pADI_PWM_CHANNEL_STATUS)value;
               u32 mask;           
             
#if defined(ADI_SSL_DEBUG)

#if defined(__ADSP_MOY__) 
               /* validate enable status  */
               if (pChannelValue->Status > ADI_PWM1_ENABLE) 
#else              
               /* status error checking */
               if (pChannelValue->Status > ADI_PWM_ENABLE)
#endif /* Moy */               
                  return ADI_PWM_RESULT_INVALID_CROSSOVER;  
                       
               /* channel error checking */
               if (pChannelValue->Channel > ADI_PWM_CHANNEL_ALL)
                  return ADI_PWM_RESULT_INVALID_CHANNEL;  
#endif    /* debug */   

               if (pChannelValue->Channel == ADI_PWM_CHANNEL_ALL)
               {
                /* mask for all three channel pairs */
                  mask = 0x01C0;
               }
               else
               {
                  /* mask for just one chnanel pair */
                  mask = 1 << pChannelValue->Channel;
               }

#if defined(__ADSP_MOY__) 

                PWM_NUM = (pChannelValue->Status & 2)/2;
                  
                if (pChannelValue->Status & 0x01)
                {
                     /* enable crossover for the channel(s) */
                     AdiPwmConfig.pPwmSegReg[PWM_NUM]->UnsignedValue |= mask;                     
                }
                else /* disable */
                {
                     /* disable crossover for the selected PWM 1 channel(s) */
                     AdiPwmConfig.pPwmSegReg[PWM_NUM]->UnsignedValue &= ~mask;
                }         
                *pADI_PWM_ADDRESS_SEG[PWM_NUM] = AdiPwmConfig.pPwmSegReg[PWM_NUM]->UnsignedValue;                         
                      

#else /* not Moy */               
               
               if (pChannelValue->Status == ADI_PWM_DISABLE)
               {
                  /* disable crossover for the channel(s) */
                  AdiPwmConfig.pPwmSegReg->UnsignedValue &= ~mask;
               }
               else
               {
                  /* enable crossover for the channel(s) */
                  AdiPwmConfig.pPwmSegReg->UnsignedValue |= mask;
               }
               *pADI_PWM_ADDRESS_SEG = AdiPwmConfig.pPwmSegReg->UnsignedValue;    
               
#endif /* Moy */                   
             }
            break;
                    

        case ADI_PWM_CMD_SET_SYNC_IVG:            
            {
                                
#if defined(__ADSP_MOY__)                             
                pADI_PWM_NUMBER_AND_VALUE pIVG_Value = (pADI_PWM_NUMBER_AND_VALUE)value;
                
#if defined(ADI_SSL_DEBUG)
               /* PWM number error checking */
               if (pIVG_Value->PwmNumber > ADI_PWM_1)
                   return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
                   
               if (pIVG_Value->PwmNumber == ADI_PWM_1)
               {

                   /* Set the IVG for the PWM Sync Interrupt */
                   if (adi_int_SICSetIVG(ADI_INT_PWM1_SYNC, pIVG_Value->Value) != ADI_INT_RESULT_SUCCESS) 
                   {
                      return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                   }     
               }
               else
               {
                   /* Set the IVG for the PWM Sync Interrupt */
                   if (adi_int_SICSetIVG(ADI_INT_PWM0_SYNC, pIVG_Value->Value) != ADI_INT_RESULT_SUCCESS) 
                   {
                      return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                   }     
               }               
                          
#else  /* not debug */
               if (pIVG_Value->PwmNumber == ADI_PWM_1)
                {
                   /* Call the functiun without checking the return values */
                   adi_int_SICSetIVG(ADI_INT_PWM1_SYNC, pIVG_Value->Value);
                }
                else
                {
                   /* Call the functiun without checking the return values */
                   adi_int_SICSetIVG(ADI_INT_PWM0_SYNC, pIVG_Value->Value);
                }

#endif  /* Debug */    
            
#else  /* not Moy */
            
               u32 IVG_value = (u32)value;

#if defined(ADI_SSL_DEBUG)

               /* Set the IVG for the PWM Sync Interrupt */
               if (adi_int_SICSetIVG(ADI_INT_PWM_SYNC, IVG_value) != ADI_INT_RESULT_SUCCESS) 
               {
                   return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
               }                
#else
               /* Call the functiun without checking the return values */
               adi_int_SICSetIVG(ADI_INT_PWM_SYNC, IVG_value);
#endif  /* Debug */
              
               /* Save the value of the IVG in the config structure */ 
               AdiPwmConfig.SYNC_IVG = IVG_value;

        
#endif    /* Moy */  
             }
            break;

        case ADI_PWM_CMD_SET_TRIP_IVG:            
            {            
            
#if defined(__ADSP_MOY__)

                pADI_PWM_NUMBER_AND_VALUE pIVG_value =  (ADI_PWM_NUMBER_AND_VALUE*)value;
 
#if defined(ADI_SSL_DEBUG)                
                
               if((pIVG_value->Value > 14) || (pIVG_value->Value < 7))
                    return ADI_PWM_RESULT_INVALID_IVG;    
                                
               /* PWM number error checking */
               if (pIVG_value->PwmNumber > ADI_PWM_1)
                   return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
                   
               if (pIVG_value->PwmNumber == ADI_PWM_1)
               {

                   /* Set the IVG for the PWM 1 Trip Interrupt */
                   if (adi_int_SICSetIVG(ADI_INT_PWM1_TRIP, pIVG_value->Value) != ADI_INT_RESULT_SUCCESS) 
                   {
                      return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                   }     
               }
               else
               {
                   /* Set the IVG for the PWM Trip Interrupt */
                   if (adi_int_SICSetIVG(ADI_INT_PWM0_TRIP, pIVG_value->Value) != ADI_INT_RESULT_SUCCESS) 
                   {
                      return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                   }     
               }               
                          

#else  /* not debug */
               if (pIVG_value->PwmNumber == ADI_PWM_1)
                {
                   /* Call the functiun without checking the return values */
                   adi_int_SICSetIVG(ADI_INT_PWM1_TRIP, pIVG_value->Value);
                }
                else
                {
                   /* Call the functiun without checking the return values */
                   adi_int_SICSetIVG(ADI_INT_PWM0_TRIP, pIVG_value->Value);
                }
#endif  /* Debug */    
                    

#else /* Not Moy */
    
               u32 IVG_value = (u32)value;
#if defined(ADI_SSL_DEBUG)

                if((IVG_value > 14) || (IVG_value < 7))
                {
                    return ADI_PWM_RESULT_INVALID_IVG;
                }
                    
               /* Set the IVG for the PWM Trip Interrupt */
               if (adi_int_SICSetIVG(ADI_INT_PWM_TRIP, IVG_value) != ADI_INT_RESULT_SUCCESS) 
               {
                  return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
               }                
#else
               /* Call the function without checking the return values */
               adi_int_SICSetIVG(ADI_INT_PWM_TRIP, IVG_value);
#endif  /* debug */
              
               /* Save the value of the IVG in the config structure */ 
               AdiPwmConfig.TRIP_IVG = IVG_value;
#endif /* Moy */               
             }
            break;    
            


        case ADI_PWM_CMD_SET_POLARITY:
            {
               ADI_PWM_POLARITY Polarity = (ADI_PWM_POLARITY)value;
               
#if defined(ADI_SSL_DEBUG)
#if defined(__ADSP_MOY__)                
               
               if (Polarity > ADI_PWM1_POLARITY_HIGH)
                  result = ADI_PWM_RESULT_INVALID_POLARITY;
#if defined(ADI_SSL_DEBUG)          
               /* indicate to debug mode, that the parameter has been set */                  
               PolaritySet[PWM_NUM]++;
#endif  
#else                  
               if (Polarity > ADI_PWM_POLARITY_HIGH)
                  result = ADI_PWM_RESULT_INVALID_POLARITY;
         
               /* indicate to debug mode, that the parameter has been set */                  
               PolaritySet++;
#endif    /* Moy */
#endif /* debug */        

#if defined(__ADSP_MOY__)
               PWM_NUM = (Polarity & 0x02)/2;                   

               AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.Polarity = Polarity;
               *pADI_PWM_ADDRESS_CTRL[PWM_NUM] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->UnsignedValue;      
                                      
#else /* not Moy */                                      
               AdiPwmConfig.pPwmCtrlReg->BitField.Polarity = Polarity;
               *pADI_PWM_ADDRESS_CTRL = AdiPwmConfig.pPwmCtrlReg->UnsignedValue;                 
#endif    /* Moy */           
             }
            break;    
                    
        /* set gate drive chopping frequency in PWMGATE (one value, no channel ID) */    
        case ADI_PWM_CMD_SET_GATE_CHOPPING_FREQ:   
            {
                
#if defined(__ADSP_MOY__)                             
                pADI_PWM_NUMBER_AND_VALUE  pChopFreq = (pADI_PWM_NUMBER_AND_VALUE)value;
                
#if defined(ADI_SSL_DEBUG)
               /* PWM number error checking */
               if (pChopFreq->PwmNumber > ADI_PWM_1)
                   return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
       
               if (pChopFreq->Value > 0x0FF)
                  return ADI_PWM_RESULT_INVALID_GATE_CHOPPING_FREQ;
#endif          
               AdiPwmConfig.pPwmGateReg[pChopFreq->PwmNumber]->BitField.GateChoppingPeriod = pChopFreq->Value; 
               *pADI_PWM_ADDRESS_GATE[pChopFreq->PwmNumber] = AdiPwmConfig.pPwmGateReg[pChopFreq->PwmNumber]->UnsignedValue;                     

#else /* not Moy */
               u32 ChopFreq = (u32)value;   
#if defined(ADI_SSL_DEBUG)         
               if (ChopFreq > 0x0FF)
                  return ADI_PWM_RESULT_INVALID_GATE_CHOPPING_FREQ;
#endif          
               AdiPwmConfig.pPwmGateReg->BitField.GateChoppingPeriod = ChopFreq; 
               *pADI_PWM_ADDRESS_GATE = AdiPwmConfig.pPwmGateReg->UnsignedValue;         
#endif /* not Moy */               
            }        
            break;    
                
       /* set gate chopping enable low status in PWMGATE 0-disable, 1-enable */ 
        case ADI_PWM_CMD_SET_GATE_ENABLE_LOW:     
            {
                
               ADI_PWM_ENABLE_STATUS GateEnable = (ADI_PWM_ENABLE_STATUS)value;
    
#if defined(ADI_SSL_DEBUG)
#if defined(__ADSP_MOY__) 
               /* validate enable status  */
               if (GateEnable> ADI_PWM1_ENABLE) 
#else /* not Moy */
               if (GateEnable > ADI_PWM_ENABLE)
#endif /* Moy */
                  return  ADI_PWM_RESULT_INVALID_GATE_ENABLE;
#endif /* debug */   

#if defined(__ADSP_MOY__)   
               PWM_NUM = (GateEnable & 0x02)/2;

               /* In this case, enable = 1 */
               AdiPwmConfig.pPwmGateReg[PWM_NUM ]->BitField.GateChoppingEnableLow = GateEnable;
               *pADI_PWM_ADDRESS_GATE[PWM_NUM ] = AdiPwmConfig.pPwmGateReg[ADI_PWM_1]->UnsignedValue;          
                    
#else /* not Moy */                
               /* In this case, enable = 1 */
               AdiPwmConfig.pPwmGateReg->BitField.GateChoppingEnableLow = GateEnable;
               *pADI_PWM_ADDRESS_GATE = AdiPwmConfig.pPwmGateReg->UnsignedValue;          
#endif /* Moy */               
            }
            
            break;

    
       /* set gate chopping enable low status in PWMGATE 0-disable, 1-enable */ 
        case ADI_PWM_CMD_SET_GATE_ENABLE_HIGH:     
            {
               ADI_PWM_ENABLE_STATUS GateEnable = (ADI_PWM_ENABLE_STATUS)value;    
               
#if defined(ADI_SSL_DEBUG)
#if defined(__ADSP_MOY__) 
               /* validate enable status  */
               if (GateEnable > ADI_PWM1_ENABLE)
#else /* not Moy */
               if (GateEnable > ADI_PWM_ENABLE)
#endif /* Moy */               
                  return  ADI_PWM_RESULT_INVALID_GATE_ENABLE;
#endif   /* debug */  

#if defined(__ADSP_MOY__)   
               PWM_NUM = (GateEnable & 0x02)/2;
                

               /* In this case, enable = 1 */
               AdiPwmConfig.pPwmGateReg[PWM_NUM]->BitField.GateChoppingEnableHigh = GateEnable;
               *pADI_PWM_ADDRESS_GATE[PWM_NUM] = AdiPwmConfig.pPwmGateReg[PWM_NUM]->UnsignedValue;          
                    
#else /* not Moy */     
               /* In this case, enable = 1 */
               AdiPwmConfig.pPwmGateReg->BitField.GateChoppingEnableHigh = GateEnable ;
               *pADI_PWM_ADDRESS_GATE = AdiPwmConfig.pPwmGateReg->UnsignedValue;
#endif /* moy */               
            }
            break;

        
        case ADI_PWM_CMD_SET_LOW_SIDE_INVERT:
            {    
               pADI_PWM_CHANNEL_STATUS pChannelValue = (pADI_PWM_CHANNEL_STATUS)value;        
             
#if defined(ADI_SSL_DEBUG)

               if ((pChannelValue->Channel < ADI_PWM_CHANNEL_C) 
                || (pChannelValue->Channel > ADI_PWM_CHANNEL_ALL)

#if defined(__ADSP_MOY__)                 
                || (pChannelValue->Status > ADI_PWM1_ENABLE))  
                    
                
#else   /* not Moy */   
                || (pChannelValue->Status >  ADI_PWM_ENABLE))  
#endif /* Moy */                
                   return ADI_PWM_RESULT_INVALID_LOW_SIDE_INVERT;                                                             
#endif    /* debug */                           
               

#if defined(__ADSP_MOY__)            
                  PWM_NUM = (pChannelValue->Status & 0x02)/2;
                  
               if (pChannelValue->Status & 0x01)  
               /* Enable */
               {
                    /* PWM 1 */ 
                   if ((pChannelValue->Channel == ADI_PWM_CHANNEL_A) 
                      || (pChannelValue->Channel == ADI_PWM_CHANNEL_ALL))
                   {
                      AdiPwmConfig.pPwmLowSideInvertReg[PWM_NUM]->BitField.SR_ModeLowSideInvertA = pChannelValue->Status;
                   }
           
                   if((pChannelValue->Channel == ADI_PWM_CHANNEL_B)
                     || (pChannelValue->Channel == ADI_PWM_CHANNEL_ALL))                
                   {
                      AdiPwmConfig.pPwmLowSideInvertReg[PWM_NUM]->BitField.SR_ModeLowSideInvertB = pChannelValue->Status;
                   }           
               
                   if((pChannelValue->Channel == ADI_PWM_CHANNEL_C)
                     || (pChannelValue->Channel == ADI_PWM_CHANNEL_ALL))                
                   {
                      AdiPwmConfig.pPwmLowSideInvertReg[PWM_NUM]->BitField.SR_ModeLowSideInvertC = pChannelValue->Status;
                   }               
                   *pADI_PWM_ADDRESS_LSI[PWM_NUM] = AdiPwmConfig.pPwmLowSideInvertReg[PWM_NUM]->UnsignedValue;                    
               }

               
#else /* Not Moy */               
               if ((pChannelValue->Channel == ADI_PWM_CHANNEL_A) 
                || (pChannelValue->Channel == ADI_PWM_CHANNEL_ALL))
               {
                  AdiPwmConfig.pPwmLowSideInvertReg->BitField.SR_ModeLowSideInvertA = pChannelValue->Status;
               }
                          
               if((pChannelValue->Channel == ADI_PWM_CHANNEL_B)
                || (pChannelValue->Channel == ADI_PWM_CHANNEL_ALL))                
               {
                  AdiPwmConfig.pPwmLowSideInvertReg->BitField.SR_ModeLowSideInvertB = pChannelValue->Status;
               }           
               
               if((pChannelValue->Channel == ADI_PWM_CHANNEL_C)
                || (pChannelValue->Channel == ADI_PWM_CHANNEL_ALL))                
               {
                  AdiPwmConfig.pPwmLowSideInvertReg->BitField.SR_ModeLowSideInvertC = pChannelValue->Status;
               }               
               *pADI_PWM_ADDRESS_LSI = AdiPwmConfig.pPwmLowSideInvertReg->UnsignedValue;
#endif /* Moy */               
            }
               
            break;        

                
        case ADI_PWM_CMD_SET_SWITCH_RELUCTANCE:
            {
               ADI_PWM_ENABLE_STATUS SRMode = (ADI_PWM_ENABLE_STATUS)value;
                              
#if defined(ADI_SSL_DEBUG)
#if defined(__ADSP_MOY__) 

                if (SRMode > ADI_PWM1_ENABLE)
                
#else /* not Moy */               
               if (SRMode > ADI_PWM_ENABLE)
#endif /* Moy */               
                  return ADI_PWM_RESULT_INVALID_SWITCH_RELUCTANCE;
#endif /* debug */  


#if defined(__ADSP_MOY__) 
               PWM_NUM = (SRMode & 0x02)/2;

                  /* switch reluctance is active if a 0 is written */
                  AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.SRMode = (SRMode+1)& 0x1;            
                  *pADI_PWM_ADDRESS_CTRL[PWM_NUM] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->UnsignedValue;

#else /* not Moy */                        
               /* switch reluctance is active if 0 is written */
               AdiPwmConfig.pPwmCtrlReg->BitField.SRMode = (SRMode+1)& 0x1;            
               *pADI_PWM_ADDRESS_CTRL = AdiPwmConfig.pPwmCtrlReg->UnsignedValue;
#endif /* Moy */                   
            }
            break;
            
            
                    
        case ADI_PWM_CMD_SET_SYNC_INT_ENABLE:        
            {
                
#if defined(__ADSP_MOY__) 
                /* to distinguish SYNC0 from SYNC1 */
                ADI_INT_PERIPHERAL_ID Event;
                             
#if defined(ADI_SSL_DEBUG)
 
               if ((ADI_PWM_ENABLE_STATUS) value > ADI_PWM1_ENABLE)            
               {
                  return ADI_PWM_RESULT_INVALID_SYNC_INT_ENABLE;
               }                
#endif          
                PWM_NUM = ((ADI_PWM_ENABLE_STATUS)value & 0x02)/2;
                if( PWM_NUM == ADI_PWM_1)
                {
                    Event = ADI_INT_PWM1_SYNC;
                }
                else
                {
                    Event = ADI_INT_PWM0_SYNC;
                }                    
 
                 /* 1 or 3 */
               if ((ADI_PWM_ENABLE_STATUS)value & 0x01)
               {
                   /* enable sync int   */
                   AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.SyncIntEnable = ADI_PWM_ENABLE;
                
#if defined(ADI_SSL_DEBUG)   
                            
                    
                  /* enable the sync interrupt and wakeup for PWM 0 (using debug return values) */
                  if (adi_int_SICEnable( Event ) != ADI_INT_RESULT_SUCCESS ) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }
                  if (adi_int_SICWakeup( Event, TRUE ) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }                  
                               
#else                            
                /* enable the sync interrupt and wakeup for PWM 0 (no return values)*/                
                  adi_int_SICEnable( Event);
                  adi_int_SICWakeup( Event, TRUE );
#endif
                    *pADI_PWM_ADDRESS_CTRL[PWM_NUM] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->UnsignedValue;                           

               }
                else
                /* 0 or 2 */               
               {    
                   /* Disable sync int  */
                   AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.SyncIntEnable = ADI_PWM_DISABLE;
                           
#if defined(ADI_SSL_DEBUG)
                  /* disable the sync interrupt and wakeup */                                
                  if (adi_int_SICDisable( Event ) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }
                  if (adi_int_SICWakeup( Event, FALSE ) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }                               
#else                    
                  /* disable the sync interrupt and wakeup */                
                  adi_int_SICDisable( Event );
                  adi_int_SICWakeup( Event, FALSE );               
#endif                
                    *pADI_PWM_ADDRESS_CTRL[PWM_NUM] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->UnsignedValue;                           
               }
                                                       
#else  /* NOT MOY */

#if defined(ADI_SSL_DEBUG)  
               if ((ADI_PWM_ENABLE_STATUS)value > ADI_PWM_ENABLE)                
               {
                  return ADI_PWM_RESULT_INVALID_SYNC_INT_ENABLE;
               } 
#endif               
               if ((AdiPwmConfig.pPwmCtrlReg->BitField.SyncIntEnable = (ADI_PWM_ENABLE_STATUS)value) == ADI_PWM_ENABLE)
               {
#if defined(ADI_SSL_DEBUG)   
                            
                  /* enable the sync interrupt and wakeup */
                  if (adi_int_SICEnable(ADI_INT_PWM_SYNC) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }
                  if (adi_int_SICWakeup(ADI_INT_PWM_SYNC, TRUE) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }
                               
#else                            
                /* enable the sync interrupt and wakeup */                
                  adi_int_SICEnable(ADI_INT_PWM_SYNC);
                  adi_int_SICWakeup(ADI_INT_PWM_SYNC, TRUE);
#endif

               }
               else
               {            
#if defined(ADI_SSL_DEBUG)
                  /* disable the sync interrupt and wakeup */                                
                  if (adi_int_SICDisable(ADI_INT_PWM_SYNC) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }
                  if (adi_int_SICWakeup(ADI_INT_PWM_SYNC, FALSE) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }                               
#else                    
                  /* disable the sync interrupt and wakeup */                
                  adi_int_SICDisable(ADI_INT_PWM_SYNC);
                  adi_int_SICWakeup(ADI_INT_PWM_SYNC, FALSE);               
#endif                                
               }
               *pADI_PWM_ADDRESS_CTRL = AdiPwmConfig.pPwmCtrlReg->UnsignedValue;    

#endif /* Moy or not */
            }            

            break;            
            
        case ADI_PWM_CMD_SET_SYNC_OUT_ENABLE:
            { 
                
              ADI_PWM_ENABLE_STATUS SyncOutEnable = (ADI_PWM_ENABLE_STATUS)value;                 
#if defined(ADI_SSL_DEBUG)
#if defined(__ADSP_MOY__)   
             if (SyncOutEnable > ADI_PWM1_ENABLE) 
#else /* not Moy */                
             if (SyncOutEnable > ADI_PWM_ENABLE)
#endif /* Moy */
                return ADI_PWM_RESULT_INVALID_SYNC_OUT_ENABLE;
#endif   /* debug */  

#if defined(__ADSP_MOY__) 
                PWM_NUM = (SyncOutEnable & 2)/2;

                /*  &1 = enabled */
                AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.Sync_Enable =  (SyncOutEnable & 1); 
                *pADI_PWM_ADDRESS_CTRL[PWM_NUM] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->UnsignedValue;  

#else /* Not Moy */                
                /* 1 = enabled */
                AdiPwmConfig.pPwmCtrlReg->BitField.Sync_Enable = SyncOutEnable;  
                *pADI_PWM_ADDRESS_CTRL = AdiPwmConfig.pPwmCtrlReg->UnsignedValue;                            
#endif /* Moy */   

            }
            break;

            
        case ADI_PWM_CMD_SET_TRIP_INPUT_ENABLE:
            {
               ADI_PWM_ENABLE_STATUS TripInEnable = (ADI_PWM_ENABLE_STATUS)value;                 
#if defined(ADI_SSL_DEBUG)
#if defined(__ADSP_MOY__) 
               if (TripInEnable > ADI_PWM1_ENABLE) 
#else /* not Moy */               
               if (TripInEnable > ADI_PWM_ENABLE)
#endif /* Moy */
                  return ADI_PWM_RESULT_INVALID_SYNC_OUT_ENABLE;
#endif    /* debug */    

#if defined(__ADSP_MOY__)   

                PWM_NUM = (TripInEnable & 0x02)/2;
                
                /* becomes 1 if enable for any PWM */
                TripInEnable &= 0x01;
               AdiPwmConfig.pPwmCtrlReg[PWM_NUM ]->BitField.TripInputDisable = (TripInEnable+1) & 1; 
               *pADI_PWM_ADDRESS_CTRL[PWM_NUM ] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM ]->UnsignedValue;    

#else /* Not Moy */                                      

               /* 0 = enabled */
               AdiPwmConfig.pPwmCtrlReg->BitField.TripInputDisable = (TripInEnable+1) & 1; 
               /* Do this immediately */
               *pADI_PWM_ADDRESS_CTRL = AdiPwmConfig.pPwmCtrlReg->UnsignedValue;
#endif                   
            }
            break;
            
        case ADI_PWM_CMD_SET_SYNC_SOURCE:
        
#if defined(__ADSP_MOY__) 

            /* External Sync (0 =Internal sync used for PWM0, 1= external sync used for PWM0)  
              (2 =Internal sync used for PWM1, 3= external sync used for PWM1)  */
            {
                ADI_PWM_SYNC_SOURCE SyncSource = (ADI_PWM_SYNC_SOURCE)value;
#if defined(ADI_SSL_DEBUG)
          
                if (SyncSource > ADI_PWM1_SYNC_SOURCE_EXTERNAL)
                    return ADI_PWM_RESULT_INVALID_SYNC_SOURCE;
#endif   
                PWM_NUM = (SyncSource & 0x02)/2;

                AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.ExtSync = SyncSource& 0x01;
                *pADI_PWM_ADDRESS_CTRL[PWM_NUM] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->UnsignedValue;    
             
            }

#else  /* Not Moy */

            /* External Sync (0 =Internal sync used, 1= external)  */
            {
                ADI_PWM_SYNC_SOURCE SyncSource = (ADI_PWM_SYNC_SOURCE)value;
#if defined(ADI_SSL_DEBUG)
          
                if (SyncSource > ADI_PWM_SYNC_SOURCE_EXTERNAL)
                   return ADI_PWM_RESULT_INVALID_SYNC_SOURCE;
#endif   
               AdiPwmConfig.pPwmCtrlReg->BitField.ExtSync = SyncSource;
             
               *pADI_PWM_ADDRESS_CTRL = AdiPwmConfig.pPwmCtrlReg->UnsignedValue;    
            }
            
#endif /* Moy or not */            
            break;        
            /* set external Sync Select 0=asynch, 1=sync to internal clock def=1 */
        case ADI_PWM_CMD_SET_SYNC_SEL:        
            {
                ADI_PWM_SYNC_SELECT SyncSelect = (ADI_PWM_SYNC_SELECT)value;
#if defined(ADI_SSL_DEBUG)  
#if defined(__ADSP_MOY__)
                if (SyncSelect > ADI_PWM1_SYNC_SYNCH)
#else /* Not Moy */                
                if (SyncSelect >  ADI_PWM_SYNC_SYNCH)
                    return ADI_PWM_RESULT_INVALID_SYNC_SEL;
#endif  /* Moy */
#endif  /* debug */  
       
#if defined(__ADSP_MOY__)
                PWM_NUM = (SyncSelect & 0x02)/2;

                AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.SyncSel = SyncSelect;             
                *pADI_PWM_ADDRESS_CTRL[PWM_NUM] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->UnsignedValue;    
                

#else /* Not Moy */
                AdiPwmConfig.pPwmCtrlReg->BitField.SyncSel = SyncSelect;             
                *pADI_PWM_ADDRESS_CTRL = AdiPwmConfig.pPwmCtrlReg->UnsignedValue;    
#endif  /* Moy */                
            }
        
            break;        
    
        case ADI_PWM_CMD_SET_TRIP_INT_ENABLE:        
            {
#if defined(__ADSP_MOY__) 
            
#if defined(ADI_SSL_DEBUG)  
               if ((ADI_PWM_ENABLE_STATUS)value > ADI_PWM1_ENABLE)     
                  return ADI_PWM_RESULT_INVALID_TRIP_INT_ENABLE;
#endif

               PWM_NUM = ((ADI_PWM_ENABLE_STATUS)value & 0x02)/2;
                
               if ((ADI_PWM_ENABLE_STATUS)value & 0x01) 
               {
                   /* enable Trip int  */
                   AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.TripIntEnable = ADI_PWM_ENABLE;
                   if (PWM_NUM == 0)
                   { 
#if defined(ADI_SSL_DEBUG)   
                          
                      /* enable the Trip interrupt and wakeup  */
                      if (adi_int_SICEnable(ADI_INT_PWM0_TRIP) != ADI_INT_RESULT_SUCCESS) 
                      {
                         return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                      }
                      if (adi_int_SICWakeup(ADI_INT_PWM0_TRIP, TRUE) != ADI_INT_RESULT_SUCCESS) 
                      {
                         return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                      }                  
                               
#else                            
                      /* enable the Trip interrupt and wakeup for PWM 0 */                
                       adi_int_SICEnable(ADI_INT_PWM0_TRIP);
                      adi_int_SICWakeup(ADI_INT_PWM0_TRIP, TRUE);
#endif
                  }
                  else
                  {

#if defined(ADI_SSL_DEBUG)   
                            
                      /* enable the trip interrupt and wakeup for PWM 1 */
                      if (adi_int_SICEnable(ADI_INT_PWM1_TRIP) != ADI_INT_RESULT_SUCCESS) 
                      {
                         return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                      }
                      if (adi_int_SICWakeup(ADI_INT_PWM1_TRIP, TRUE) != ADI_INT_RESULT_SUCCESS) 
                      {
                         return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                      }                  
                               
#else                            
                    /* enable the trip interrupt and wakeup for PWM 1 */                
                      adi_int_SICEnable(ADI_INT_PWM1_TRIP);
                      adi_int_SICWakeup(ADI_INT_PWM1_TRIP, TRUE);
#endif                          
                  }  /* which pwm */    
               }                                    
               else
                /* disable */
               {    
                   /* Disable trip int   */
                   AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.TripIntEnable = ADI_PWM_DISABLE;
                      if (PWM_NUM == 0)
                      { 
                           
#if defined(ADI_SSL_DEBUG)
                      /* disable the trip interrupt and wakeup */                                
                      if (adi_int_SICDisable(ADI_INT_PWM0_TRIP) != ADI_INT_RESULT_SUCCESS) 
                      {
                         return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                      }
                      if (adi_int_SICWakeup(ADI_INT_PWM0_TRIP, FALSE) != ADI_INT_RESULT_SUCCESS) 
                      {
                         return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                      }                               
#else                    
                      /* disable the trip interrupt and wakeup */                
                      adi_int_SICDisable(ADI_INT_PWM0_TRIP);
                      adi_int_SICWakeup(ADI_INT_PWM0_TRIP, FALSE);               
#endif   
                      }
                      else
                      {
                           
#if defined(ADI_SSL_DEBUG)
                      /* disable the trip interrupt and wakeup */                                
                      if (adi_int_SICDisable(ADI_INT_PWM1_TRIP) != ADI_INT_RESULT_SUCCESS) 
                      {
                         return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                      }
                      if (adi_int_SICWakeup(ADI_INT_PWM1_TRIP, FALSE) != ADI_INT_RESULT_SUCCESS) 
                      {
                         return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                      }                               
#else                    
                      /* disable the trip interrupt and wakeup */                
                      adi_int_SICDisable(ADI_INT_PWM1_TRIP);
                      adi_int_SICWakeup(ADI_INT_PWM1_TRIP, FALSE);               
#endif                
                     }
                     
               } /* enable or disable */                     
                     
            *pADI_PWM_ADDRESS_CTRL[PWM_NUM] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->UnsignedValue;                           
               
            
#else  /* NOT MOY */

#if defined(ADI_SSL_DEBUG) 
               if ((ADI_PWM_ENABLE_STATUS)value > ADI_PWM_ENABLE)        
                  return ADI_PWM_RESULT_INVALID_TRIP_INT_ENABLE;
#endif         
               if ((AdiPwmConfig.pPwmCtrlReg->BitField.TripIntEnable = (ADI_PWM_ENABLE_STATUS)value) == ADI_PWM_ENABLE)
               {
  
#if defined(ADI_SSL_DEBUG)   
                                
                  /* enable the trip interrupt and wakeup */
                  if (adi_int_SICEnable(ADI_INT_PWM_TRIP) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }
                  if (adi_int_SICWakeup(ADI_INT_PWM_TRIP, TRUE) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }                               
#else                            
                    /* enable the trip interrupt and wakeup */                
                  adi_int_SICEnable(ADI_INT_PWM_TRIP);
                  adi_int_SICWakeup(ADI_INT_PWM_TRIP, TRUE);
#endif
               }
               else
               {
#if defined(ADI_SSL_DEBUG)
                  /* disable the trip interrupt and wakeup */                                
                  if (adi_int_SICDisable(ADI_INT_PWM_TRIP) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }
                  if (adi_int_SICWakeup(ADI_INT_PWM_TRIP, FALSE) != ADI_INT_RESULT_SUCCESS) 
                  {
                     return ADI_PWM_RESULT_INTERRUPT_MANAGER_ERROR;
                  }
                
                
#else                     
                  /* disable the trip interrupt and wakeup */                
                  adi_int_SICDisable(ADI_INT_PWM_TRIP);
                  adi_int_SICWakeup(ADI_INT_PWM_TRIP, FALSE);                
#endif                               
               }            
               *pADI_PWM_ADDRESS_CTRL = AdiPwmConfig.pPwmCtrlReg->UnsignedValue;    

#endif /* Moy */
            }            
            break;    
    
            
        case ADI_PWM_CMD_SET_UPDATE_MODE:
            {
               ADI_PWM_UPDATE_MODE UpdateMode = (ADI_PWM_UPDATE_MODE)value;
             
#if defined(ADI_SSL_DEBUG)
#if defined(__ADSP_MOY__)

               if (UpdateMode > ADI_PWM1_DOUBLE_UPDATE)
#else     /* not Moy */                    
               if (UpdateMode > ADI_PWM_DOUBLE_UPDATE)
#endif /* Moy */               
                  return ADI_PWM_RESULT_INVALID_UPDATE_MODE;                  
#endif     

#if defined(__ADSP_MOY__)
               PWM_NUM = (UpdateMode & 0x02)/2;
               
               /* PWM 1 */
               AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.DoubleUp = UpdateMode;  
               *pADI_PWM_ADDRESS_CTRL[PWM_NUM] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->UnsignedValue;    
#if defined(ADI_SSL_DEBUG)                   
                  /* indicate to debug mode, that the parameter has been set */                  
               OperModeSet[PWM_NUM]++;
#endif                          
    
#else /* not Moy */     
               AdiPwmConfig.pPwmCtrlReg->BitField.DoubleUp = UpdateMode;  
               *pADI_PWM_ADDRESS_CTRL = AdiPwmConfig.pPwmCtrlReg->UnsignedValue;    
#if defined(ADI_SSL_DEBUG)                   
               /* indicate to debug mode, that the parameter has been set */                  
               OperModeSet++;
#endif /* debug */ 
#endif /* Moy */
            }
            break;        
        
        case ADI_PWM_CMD_SET_PWM_ENABLE:        
            {      
               ADI_PWM_ENABLE_STATUS Enabled = (ADI_PWM_ENABLE_STATUS)value;
               
#if defined(__ADSP_MOY__)
#if defined(ADI_SSL_DEBUG)                         
               if (Enabled > ADI_PWM1_ENABLE)
                 return ADI_PWM_RESULT_INVALID_PWM_ENABLE;
#endif                 
               PWM_NUM = (Enabled & 0x02)/2;                   

               if((Enabled & 0x01) == 1)
               {
                   AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.Enable = 1;    
#if defined(ADI_SSL_DEBUG)
                   
                   /* Make sure that all parameters have been set before enabling the PWM */               
                    if ((result = adi_pwmCheckInitParameters()) != ADI_PWM_RESULT_SUCCESS)
                        return result;
#endif                                       
                }    
               else
               {                              
                   /* The field is used to disable the PWM - enable = 1 */
                   AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->BitField.Enable = 0;                    
               }    
                *pADI_PWM_ADDRESS_CTRL[PWM_NUM] = AdiPwmConfig.pPwmCtrlReg[PWM_NUM]->UnsignedValue;       
                                  
#else  /* Not Moy */
#if defined(ADI_SSL_DEBUG)                         
               if (Enabled > ADI_PWM_ENABLE)
                 return ADI_PWM_RESULT_INVALID_PWM_ENABLE;
#endif
                                   
               /* The field is used to disable the PWM - enable = 1 */
               AdiPwmConfig.pPwmCtrlReg->BitField.Enable = Enabled;
             
               *pADI_PWM_ADDRESS_CTRL = AdiPwmConfig.pPwmCtrlReg->UnsignedValue;       
#endif  /* Moy */               
            }
            break;
                        


        case ADI_PWM_CMD_CLEAR_SYNC_INT:

        /* set W1C to clear SYNC in PWMSTAT    */

            {    

#if defined(__ADSP_MOY__)  
                 PWM_NUM = (ADI_PWM_NUMBER)value;
            
#if defined(ADI_SSL_DEBUG)  
               if ( PWM_NUM > ADI_PWM_1)     
                  return ADI_PWM_RESULT_INVALID_PWM_NUMBER;
#endif
#endif                
        
               /* protect this region of code */
               pExitCriticalArg = adi_int_EnterCriticalRegion(AdiPwmConfig.pEnterCriticalArg);

#if defined(__ADSP_MOY__)      
               /* Read in the PWM Status Register first  */
               PwmStatReg[PWM_NUM].UnsignedValue = *pADI_PWM_ADDRESS_STAT[PWM_NUM];    
               AdiPwmConfig.pPwmStatReg[PWM_NUM]->BitField.SyncInt = 1;
               *pADI_PWM_ADDRESS_STAT[PWM_NUM] = AdiPwmConfig.pPwmStatReg[PWM_NUM]->UnsignedValue; 
#else
     
               /* Read in the PWM Status Register first  */
               PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;    
               AdiPwmConfig.pPwmStatReg->BitField.SyncInt = 1;
               *pADI_PWM_ADDRESS_STAT = AdiPwmConfig.pPwmStatReg->UnsignedValue; 

#endif

               /* unprotect this region of code */
               adi_int_ExitCriticalRegion(pExitCriticalArg);  

            }
            break;        
        
        case ADI_PWM_CMD_CLEAR_TRIP_INT:
            {
    
#if defined(__ADSP_MOY__)  
                 PWM_NUM = (ADI_PWM_NUMBER)value;
            
#if defined(ADI_SSL_DEBUG)  
               if ( PWM_NUM  > ADI_PWM_1 )     
                  return ADI_PWM_RESULT_INVALID_PWM_NUMBER;
#endif
#endif                
  
               /* protect this region of code */
               pExitCriticalArg = adi_int_EnterCriticalRegion(AdiPwmConfig.pEnterCriticalArg);

#if defined(__ADSP_MOY__)    
               /* Read in the PWM Status Register first  */
               PwmStatReg[PWM_NUM].UnsignedValue = *pADI_PWM_ADDRESS_STAT[PWM_NUM];    
        
               AdiPwmConfig.pPwmStatReg[PWM_NUM]->BitField.TripInt = 1;
               *pADI_PWM_ADDRESS_STAT[PWM_NUM] = AdiPwmConfig.pPwmStatReg[PWM_NUM]->UnsignedValue;  
#else
               /* Read in the PWM Status Register first  */
               PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;    
        
               AdiPwmConfig.pPwmStatReg->BitField.TripInt = 1;
               *pADI_PWM_ADDRESS_STAT = AdiPwmConfig.pPwmStatReg->UnsignedValue; 
#endif
               /* unprotect this region of code */
               adi_int_ExitCriticalRegion(pExitCriticalArg);                
            }
            break;

            
    /* Following are all the commands which report the status of a PWM value  */
            
        case ADI_PWM_CMD_GET_DUTY_CYCLE:
          { 
             pADI_PWM_CHANNEL_DUTY_CYCLE pChannelValue = (pADI_PWM_CHANNEL_DUTY_CYCLE)value;
                                             
             /* Reads the duty cycle in the config structure and  
                returns it in the structure pointed to by the command argument */
#if defined(ADI_SSL_DEBUG)
             if ((result = adi_pwmGetDutyCycle(pChannelValue)) != ADI_PWM_RESULT_SUCCESS)                
                    return(result);
#else /* not debug */
                adi_pwmGetDutyCycle(pChannelValue);
#endif /* debug */
          }

          break;

 
       case ADI_PWM_CMD_GET_CHANNEL_ENABLE:        
            {   

#if defined(__ADSP_MOY__) 

                pPwmNumChannelStatus = (pADI_PWM_NUMBER_AND_CHANNEL_STATUS)value;

#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number (cannot select BOTH) */
                if (pPwmNumChannelStatus->PwmNumber >ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */
                /* unsigned value in which to store Seg Reg for the PWM # in question */
                u32 PwmSegReg;
                    
                /* get the Seg Reg for the PWM  */        
                PwmSegReg = AdiPwmConfig.pPwmSegReg[pPwmNumChannelStatus->PwmNumber]->UnsignedValue;
    
                
                /* return the enable status of the channel specified */
                if ((PwmSegReg && (1 <<  pPwmNumChannelStatus->ChannelStatus.Channel)) != 0)
                {
                    pPwmNumChannelStatus->ChannelStatus.Status = ADI_PWM_DISABLE;
                }
                else
                {
                    pPwmNumChannelStatus->ChannelStatus.Status = ADI_PWM_ENABLE;
                }

#else /* Not Moy */

                 
               pADI_PWM_CHANNEL_STATUS pChannelValue = (pADI_PWM_CHANNEL_STATUS)value;             

#if defined(ADI_SSL_DEBUG)
    
               /* Must specify an individual channel */                            
               if (pChannelValue->Channel > ADI_PWM_CHANNEL_AL) 
                  return ADI_PWM_RESULT_INVALID_CHANNEL;
        
#endif /* debug */
                        
               if (((AdiPwmConfig.pPwmSegReg->UnsignedValue) && (1 << pChannelValue->Channel)) != 0)
               {
                   pChannelValue->Status = ADI_PWM_DISABLE;
               }
               else
               {
                   pChannelValue->Status = ADI_PWM_ENABLE;
               }
 
#endif /* Moy */               
            }
            break;  


             /* return the value of the SR bit */        
        case ADI_PWM_CMD_GET_SWITCH_RELUCTANCE:
            {
                
#if defined(__ADSP_MOY__) 

                pPwmNumStatus = (pADI_PWM_NUMBER_AND_ENABLE_STATUS)value;
        

#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number (cannot select BOTH) */
                if (pPwmNumStatus->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */                
                
                if (pPwmNumStatus->PwmNumber == ADI_PWM_1)
                {                                
                   /* Read in the PWM 1 Status Register first  */
                   PwmStatReg[ADI_PWM_1].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_1];                        
                }
                else                               
                {                                
                   /* Read in the PWM 0 Status Register first  */
                   PwmStatReg[ADI_PWM_0].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_0]; 
                }    
               /* return the enable status of SR bit */                                                     
               pPwmNumStatus->Status = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmStatReg[pPwmNumStatus->PwmNumber]->BitField.SRMode;             
                
                
#else /* Not Moy */                
               /* Read in the PWM Status Register first  */
               PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;     
               *(ADI_PWM_ENABLE_STATUS*)value = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmStatReg->BitField.SRMode;             
                              
#endif /* Moy */               
            }
            break;    

 
        case ADI_PWM_CMD_GET_DEAD_TIME:        
        /* get dead time from PWMDT Register  */        
#if defined(__ADSP_MOY__)     
            {
                  /*  return dead time in value field of ADI_PWM_NUMBER_AND_VALUE struct */                        
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */                

               /* Read in the PWM 1 Dead Time */                
               pPwmNumValue->Value  = (u32)AdiPwmConfig.PwmDtReg[pPwmNumValue->PwmNumber];                                                    
            }        
#else /* Not Moy */        
    
            {                       
               *(u32*)value = (u32)AdiPwmConfig.PwmDtReg;                                    
            }
#endif /* Moy */
            
            break;
                           
 
        case ADI_PWM_CMD_GET_PERIOD:           
            /* get the period from PWMTM Register */
#if defined(__ADSP_MOY__)     
            {        
                /*  return period in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */                
                                                
               /* Read in the PWM 1 Dead Time */        
               pPwmNumValue->Value  = (u32)AdiPwmConfig.PwmTmReg[pPwmNumValue->PwmNumber];                                            
        
            }        
#else /* Not Moy */                            
            {
               *(u32*)value = (u32)AdiPwmConfig.PwmTmReg;                                                       
            }
#endif /* Moy */            
            
            break; 
                        
                  

        case ADI_PWM_CMD_GET_SYNC_PULSE_WIDTH:   
            /* get sync pulse width in PWMSYNCWT register */                
#if defined(__ADSP_MOY__)     
            {        
                /*  return sync pulse width in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */                                
                                
               /* Read in the PWM 1 sync width */        
               pPwmNumValue->Value  = (u32)AdiPwmConfig.PwmSyncWtReg[pPwmNumValue->PwmNumber];                                    
        
            }        
#else /* Not Moy */            
            {
               *(u32*)value = (u32)AdiPwmConfig.PwmSyncWtReg;                 
            }
#endif /* Moy */            
            break;  
                              
                                      
        case ADI_PWM_CMD_GET_POLARITY:         
            /* get PWM polarity - PWMSTAT 1=active hi, 0=active lo def=0 */             
#if defined(__ADSP_MOY__)     
            {        
                /*  return polarity in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    
 
                /* Read in the PWM 1 Polarity */        
               pPwmNumValue->Value = (u32)AdiPwmConfig.pPwmStatReg[pPwmNumValue->PwmNumber]->BitField.Polarity;                                    
        
            }        
#else /* Not Moy */                   
            {
               /* Read in the PWM Status Register first  */
               PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;                    
               *(u32*)value = (u32)AdiPwmConfig.pPwmStatReg->BitField.Polarity;                    
            }
#endif /* Moy */            
            break;   
            
        
        case ADI_PWM_CMD_GET_GATE_CHOPPING_FREQ: 
        /* get gate drive chopping frequency from PWMGATE     */ 
#if defined(__ADSP_MOY__)     
            {        
                /*  return gate drive chopping frequency in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    
                                        
                                
               /* Read in the PWM 1 Polarity */        
               pPwmNumValue->Value = (u32)AdiPwmConfig.pPwmGateReg[pPwmNumValue->PwmNumber]->BitField.GateChoppingPeriod;
            }        
#else /* Not Moy */                
          
            {
               *(u32*)value = (u32)AdiPwmConfig.pPwmGateReg->BitField.GateChoppingPeriod;
            }
#endif /* Moy */            
            break;  

                        
        case ADI_PWM_CMD_GET_GATE_ENABLE_LOW:                            
#if defined(__ADSP_MOY__) 
            {            
                /* do not use the channel field */
                pPwmNumStatus = (pADI_PWM_NUMBER_AND_ENABLE_STATUS)value;        

#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number (cannot select BOTH) */
                if (pPwmNumStatus->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */                
                
                                
                /* get gate chopping enable status for low side from PWMGATE */                  
                pPwmNumStatus->Status = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmGateReg[ pPwmNumStatus->PwmNumber ]->BitField.GateChoppingEnableLow;  
                    
            }                
#else /* Not Moy */                
        
        
            /* get gate chopping enable status for low side from PWMGATE */   
            {
               *(ADI_PWM_ENABLE_STATUS*)value = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmGateReg->BitField.GateChoppingEnableLow;                    
            }
#endif /* Moy */            
            break;            
                   
                       
        case ADI_PWM_CMD_GET_GATE_ENABLE_HIGH: 
            {                         
#if defined(__ADSP_MOY__) 
                /*  based on specified PWM number */
                pPwmNumStatus = (pADI_PWM_NUMBER_AND_ENABLE_STATUS)value;
        
#if defined(ADI_SSL_DEBUG)
                 /* validate PWM Number (cannot select BOTH) */
                if (pPwmNumStatus->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */                                            
                /* get gate chopping enable status for low side from PWMGATE */                  
                pPwmNumStatus->Status = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmGateReg[pPwmNumStatus->PwmNumber]->BitField.GateChoppingEnableHigh;  
                
#else /* Not Moy */   
           
                /* get gate chopping enable status for low side from PWMGATE */   
                *(ADI_PWM_ENABLE_STATUS*)value = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmGateReg->BitField.GateChoppingEnableHigh;
               
#endif /* Moy */
            
            }            
            break;        
            
                                
        case ADI_PWM_CMD_GET_LOW_SIDE_INVERT:           
        /* get Low Side Invert for a channel pair from PWMLSI register  */   
            {                
#if defined(__ADSP_MOY__)     
            /* also based on specified PWM number */                

                pPwmNumChannelStatus = (pADI_PWM_NUMBER_AND_CHANNEL_STATUS)value;  
                
#if defined(ADI_SSL_DEBUG)
               if (pPwmNumChannelStatus->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
                    
               if((pPwmNumChannelStatus->ChannelStatus.Channel < ADI_PWM_CHANNEL_C) 
                || ( pPwmNumChannelStatus->ChannelStatus.Channel> ADI_PWM_CHANNEL_A))
                  return ADI_PWM_RESULT_INVALID_CHANNEL;                                                                       
#endif /* debug */

               if (pPwmNumChannelStatus->ChannelStatus.Channel == ADI_PWM_CHANNEL_A)
               {
                   pPwmNumChannelStatus->ChannelStatus.Status = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmLowSideInvertReg[pPwmNumChannelStatus->PwmNumber]->BitField.SR_ModeLowSideInvertA;
               }
                          
               if (pPwmNumChannelStatus->ChannelStatus.Channel == ADI_PWM_CHANNEL_B)
               {
                   pPwmNumChannelStatus->ChannelStatus.Status = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmLowSideInvertReg[pPwmNumChannelStatus->PwmNumber]->BitField.SR_ModeLowSideInvertB;
               }           
               
               if (pPwmNumChannelStatus->ChannelStatus.Channel == ADI_PWM_CHANNEL_C)
               {
                   pPwmNumChannelStatus->ChannelStatus.Status  = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmLowSideInvertReg[pPwmNumChannelStatus->PwmNumber]->BitField.SR_ModeLowSideInvertC;
               }
                                   

#else /* Not Moy */
                
               pADI_PWM_CHANNEL_STATUS pChannelValue = (pADI_PWM_CHANNEL_STATUS)value;                    
               
#if defined(ADI_SSL_DEBUG)
               if((pChannelValue->Channel < ADI_PWM_CHANNEL_C) 
                 || (pChannelValue->Channel > ADI_PWM_CHANNEL_A))
                   return ADI_PWM_RESULT_INVALID_CHANNEL;                                               
#endif /* debug */     
               
               if (pChannelValue->Channel == ADI_PWM_CHANNEL_A)
               {
                   pChannelValue->Status = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmLowSideInvertReg->BitField.SR_ModeLowSideInvertA;
               }
                          
               if (pChannelValue->Channel == ADI_PWM_CHANNEL_B)
               {
                   pChannelValue->Status = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmLowSideInvertReg->BitField.SR_ModeLowSideInvertB;
               }           
               
               if (pChannelValue->Channel == ADI_PWM_CHANNEL_C)
               {
                   pChannelValue->Status = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmLowSideInvertReg->BitField.SR_ModeLowSideInvertC;
               }    
#endif  /* Moy */                           
            }
            break; 
                    

        case ADI_PWM_CMD_GET_CROSSOVER:        
            /* get crossover mode for a channel pair from PWMSEG register  */
             {
#if defined(__ADSP_MOY__) 
                /* also based on specified PWM number */                
                 pPwmNumChannelStatus = (pADI_PWM_NUMBER_AND_CHANNEL_STATUS)value;
        
#if defined(ADI_SSL_DEBUG)
                  /* validate PWM Number (cannot select BOTH) */
                  if (pPwmNumChannelStatus->PwmNumber > ADI_PWM_1)
                        return ADI_PWM_RESULT_INVALID_PWM_NUMBER;
                    
                  if((pPwmNumChannelStatus->ChannelStatus.Channel < ADI_PWM_CHANNEL_C) 
                     || (pPwmNumChannelStatus->ChannelStatus.Channel > ADI_PWM_CHANNEL_A))

                  return ADI_PWM_RESULT_INVALID_CHANNEL;                     
#endif /* debug */                

                  /* get crossover status for lpair */                  
                  pPwmNumChannelStatus->ChannelStatus.Status = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmGateReg[ pPwmNumChannelStatus->PwmNumber]->BitField.GateChoppingEnableHigh;  
                       
              }                
#else /* Not Moy */                    
                
                pADI_PWM_CHANNEL_STATUS pChannelValue = (pADI_PWM_CHANNEL_STATUS)value;               
             
#if defined(ADI_SSL_DEBUG)
                if((pChannelValue->Channel < ADI_PWM_CHANNEL_C) 
                  || (pChannelValue->Channel > ADI_PWM_CHANNEL_A))

                    return ADI_PWM_RESULT_INVALID_CHANNEL;                                               
#endif   /* debug */    
                if (AdiPwmConfig.pPwmSegReg->UnsignedValue & (1 << pChannelValue->Channel))
                {
                    pChannelValue->Status = ADI_PWM_ENABLE;                
                }
                else
                {
                    pChannelValue->Status = ADI_PWM_DISABLE;                
                } 
            }
#endif  /* Moy */             
            break;    
                        
                
                            
        case ADI_PWM_CMD_GET_SYNC_INT:    
        /* get sync interrupt from PWMSTAT */            
#if defined(__ADSP_MOY__)     
            {        
                /*  return sync interrupt in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number (cannot specify BOTH) */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    
                                        
                if (pPwmNumValue->PwmNumber == ADI_PWM_1) 
                {            
                   /* Read in the PWM Status Register first  */
                   PwmStatReg[ADI_PWM_1].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_1];     
                }        
                else
                {    
                   /* Read in the PWM Status Register first  */
                   PwmStatReg[ADI_PWM_1].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_0];                                                     
                }
               /* Read in the PWM Sync Int */                
               pPwmNumValue->Value = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmStatReg[pPwmNumValue->PwmNumber]->BitField.SyncInt;                                
                
            }        
#else /* Not Moy */                                                    
            {
               /* Read in the PWM Status Register first  */
               PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;                    
               *(ADI_PWM_ENABLE_STATUS*)value = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmStatReg->BitField.SyncInt;                                
            }
#endif  /* Moy */                             
            break; 
                    

                                       
        case ADI_PWM_CMD_GET_SYNC_SOURCE:       
        /* get sync pulse source (internal or external) PWMCTRL */                
#if defined(__ADSP_MOY__)     
            {        
                /*  return sync source enum in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number (cannot specify BOTH) */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    
                                        
       
               pPwmNumValue->Value = (ADI_PWM_SYNC_SOURCE)AdiPwmConfig.pPwmCtrlReg[pPwmNumValue->PwmNumber ]->BitField.ExtSync;                    
            }
        
#else /* Not Moy */        

            {
               *(ADI_PWM_SYNC_SOURCE*)value = (ADI_PWM_SYNC_SOURCE)AdiPwmConfig.pPwmCtrlReg->BitField.ExtSync;                    
            }
#endif  /* Moy */             
            break;   
            

                                    
        case ADI_PWM_CMD_GET_SYNC_SEL:         
        /* get extern Sync Select  0=asynch, 1=sync def=1 */
#if defined(__ADSP_MOY__)     
            {        
                /*  return sync source enum in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number (cannot specify BOTH) */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    

                 pPwmNumValue->Value = (ADI_PWM_SYNC_SELECT)AdiPwmConfig.pPwmCtrlReg[pPwmNumValue->PwmNumber]->BitField.SyncSel; 
                       
            }                    
                                                
#else /* Not Moy */            
            {
               *(ADI_PWM_SYNC_SELECT*)value = (ADI_PWM_SYNC_SELECT)AdiPwmConfig.pPwmCtrlReg->BitField.SyncSel;    
            }
#endif  /* Moy */             
            break;                


            
        case ADI_PWM_CMD_GET_TRIP_INT:    
        /* get trip interrupt in PWMSTAT HW Pin, or SW W1C, def=0   */ 
#if defined(__ADSP_MOY__)     
            {        
                /*  return trip interrupt in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
               /* validate PWM Number (cannot specify BOTH) */
               if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    
                                        
               if (pPwmNumValue->PwmNumber == ADI_PWM_1) 
               {            
                   /* Read in the PWM Status Register first  */
                   PwmStatReg[ADI_PWM_1].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_1];     
                                                       
                   /* Read in the PWM 1 Trip Int */        
               
                   if ((ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmStatReg[ADI_PWM_1]->BitField.TripInt== ADI_PWM_ENABLE)
                   {
                       pPwmNumValue->Value = ADI_PWM_DISABLE;      
                   }
                   else
                   {
                       pPwmNumValue->Value = ADI_PWM_ENABLE;      
                   }                                             
               }        
               else
               {    
                   /* Read in the PWM Status Register first  */
                   PwmStatReg[ADI_PWM_1].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_0];     
                                                
                   /* Read in the PWM 0 Trip Int */                
                   if ((ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmStatReg[ADI_PWM_0]->BitField.TripInt== ADI_PWM_ENABLE)
                   {
                       pPwmNumValue->Value = ADI_PWM_DISABLE;      
                   }
                   else
                   {
                       pPwmNumValue->Value = ADI_PWM_ENABLE;      
                   }                                
               }
           }        
#else /* Not Moy */                    
           {
               /* Read in the PWM Status Register first  */
               PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;     
               
               /* Enable = 1 in this case */
               if (AdiPwmConfig.pPwmStatReg->BitField.TripInt == ADI_PWM_ENABLE)
               {
                  *(ADI_PWM_ENABLE_STATUS*)value = ADI_PWM_DISABLE;
               }
               else
               {
                  *(ADI_PWM_ENABLE_STATUS*)value = ADI_PWM_ENABLE;
               }                                
            }
#endif  /* Moy */             
            break;                
        

        case ADI_PWM_CMD_GET_UPDATE_MODE:       
        /* get PWM Operating Mode from PWMSTAT- single/double update */ 
#if defined(__ADSP_MOY__)     
            {        
                /*  return trip interrupt in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number (cannot specify BOTH) */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    
        
               pPwmNumValue->Value = (ADI_PWM_UPDATE_MODE)AdiPwmConfig.pPwmCtrlReg[pPwmNumValue->PwmNumber]->BitField.DoubleUp;
    
            }        
#else /* Not Moy */                                        
        
            {             
               *(ADI_PWM_UPDATE_MODE*)value = (ADI_PWM_UPDATE_MODE)AdiPwmConfig.pPwmCtrlReg->BitField.DoubleUp;                            
            }
#endif /* Moy */            
            break;                

        case ADI_PWM_CMD_GET_PWM_ENABLE:        
        /* get software shutdown status (all 6 channel disable)     */
#if defined(__ADSP_MOY__)     
            {        
                /*  return trip interrupt in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number (cannot specify BOTH) */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    
                    
                pPwmNumValue->Value =  (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmCtrlReg[pPwmNumValue->PwmNumber]->BitField.Enable;   

            }        
#else /* Not Moy */                                        
                        
            {
                *(ADI_PWM_ENABLE_STATUS*)value = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmCtrlReg->BitField.Enable;                    
            }
#endif /* Moy */            
            break;                

        case ADI_PWM_CMD_GET_PHASE:            
        /* get PWM phase from PWMSTAT, 0=1st half, 1=2nd half  def=0 */
        
#if defined(__ADSP_MOY__)     
            {        
                /*  return trip interrupt in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
               /* validate PWM Number (cannot specify BOTH) */
               if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    
                                        
               if (pPwmNumValue->PwmNumber == ADI_PWM_1) 
               {            
                   /* Read in the PWM Status Register first  */
                   PwmStatReg[ADI_PWM_1].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_1];                    
               }        
               else
               {    
                   /* Read in the PWM Status Register first  */
                   PwmStatReg[ADI_PWM_0].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_0];                    
               }
               pPwmNumValue->Value 
                 = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmStatReg[pPwmNumValue->PwmNumber]->BitField.Phase;    
            }        
#else /* Not Moy */             
            {
               /* Read in the PWM Status Register first  */
               PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;                    
               *(ADI_PWM_ENABLE_STATUS*)value = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmStatReg->BitField.Phase;                                       
            }
#endif            
            break;                

        case ADI_PWM_CMD_GET_TRIP_PIN:         
        /* get trip pin value in PWMSTAT          */   
#if defined(__ADSP_MOY__)     
            {        
                /*  return trip interrupt in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number (cannot specify BOTH) */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    
                                        
                if (pPwmNumValue->PwmNumber == ADI_PWM_1) 
                {            
                   /* Read in the PWM Status Register first  */
                   PwmStatReg[ADI_PWM_1].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_1];                    
                }        
                else
                {    
                   /* Read in the PWM Status Register first  */
                   PwmStatReg[ADI_PWM_0].UnsignedValue = *pADI_PWM_ADDRESS_STAT[ADI_PWM_0];                                                                                 
                }
                pPwmNumValue->Value 
                  = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmStatReg[pPwmNumValue->PwmNumber]->BitField.Trip;                               
                
            }        
#else /* Not Moy */            
            {
               /* Read in the PWM Status Register first  */
               PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;                    
               *(ADI_PWM_ENABLE_STATUS*)value = (ADI_PWM_ENABLE_STATUS)AdiPwmConfig.pPwmStatReg->BitField.Trip;
            }
#endif /* Moy */            
            break;                

        case ADI_PWM_CMD_GET_OUTPUT:           
        /* get output signal from PWMSTAT2                      */ 
#if defined(__ADSP_MOY__)     
            {        
                /*  return trip interrupt in value field of ADI_PWM_NUMBER_AND_VALUE struct */
                pPwmNumValue = (pADI_PWM_NUMBER_AND_VALUE)value;
                                        
#if defined(ADI_SSL_DEBUG)
                /* validate PWM Number (cannot specify BOTH) */
                if (pPwmNumValue->PwmNumber > ADI_PWM_1)
                    return ADI_PWM_RESULT_INVALID_PWM_NUMBER; 
#endif /* debug */    
                                        
                if (pPwmNumValue->PwmNumber == ADI_PWM_1) 
                {            
                    PwmStat2Reg[ADI_PWM_1].UnsignedValue = *pADI_PWM_ADDRESS_STAT2[ADI_PWM_1];    
                }        
                else
                {    
                    PwmStat2Reg[ADI_PWM_0].UnsignedValue = *pADI_PWM_ADDRESS_STAT2[ADI_PWM_0];    
                }
                pPwmNumValue->Value  = (u32)AdiPwmConfig.pPwmStat2Reg[pPwmNumValue->PwmNumber]->UnsignedValue;                                                                             
                                                             
                 
            }        
#else /* Not Moy */                  
            {
                PwmStat2Reg.UnsignedValue = *pADI_PWM_ADDRESS_STAT2;    
                *(u32*)value = (u32)AdiPwmConfig.pPwmStat2Reg->UnsignedValue;                                                                             
            }
 
                      
#endif /* Moy */
             break;     
                                                            
     }          
         
    RecursiveControlEntry--;
       
    /* return */
    return(ADI_PWM_RESULT_SUCCESS);
}



    
/******************************************************************************
 **  Function:    adi_pwm_ValidateEventID                               **
 **  Description:    Returns error if Event ID does not exist              **
 *****************************************************************************/

ADI_PWM_RESULT adi_pwm_ValidateEventID(ADI_PWM_EVENT_ID EventID)      
{
    
    if ((EventID-ADI_PWM_EVENT_START) > ADI_PWM_MAX_EVENTS)
    {
       return ADI_PWM_RESULT_INVALID_EVENT_ID;
    }
    return ADI_PWM_RESULT_SUCCESS;
}


/*********************************************************************************
 **  Function:     adi_pwmGetDutyCycle                                    **
 **  Description: Sense the duty cycle given a channel and PWM number         **
 ********************************************************************************/  

ADI_PWM_RESULT adi_pwmGetDutyCycle(pADI_PWM_CHANNEL_DUTY_CYCLE pChannelValue)
{
     
    ADI_PWM_CHANNEL Channel = pChannelValue->Channel;
    
    switch(Channel)
    {
        
#if defined(ADI_SSL_DEBUG)
        default:
          return ADI_PWM_RESULT_INVALID_CHANNEL;    
#endif                      
      
        case ADI_PWM_CHANNEL_A:
            {
#if defined(__ADSP_MOY__)  
                pChannelValue->Value = AdiPwmConfig. PwmChAReg[pChannelValue->Number];
#else  /* Not Moy */                
                pChannelValue->Value = AdiPwmConfig.PwmChAReg;
#endif /* Moy */                
            }
            break;
          
         case ADI_PWM_CHANNEL_B:
            {
#if defined(__ADSP_MOY__)  
                pChannelValue->Value = AdiPwmConfig.PwmChBReg[pChannelValue->Number];
#else  /* Not Moy */                
                pChannelValue->Value = AdiPwmConfig.PwmChBReg;
#endif /* Moy */                
            }
            break;     
             
         case ADI_PWM_CHANNEL_C:
            {
#if defined(__ADSP_MOY__)  
                pChannelValue->Value = AdiPwmConfig.PwmChBReg[pChannelValue->Number];                
#else  /* Not Moy */                
                pChannelValue->Value = AdiPwmConfig.PwmChCReg;
#endif /* Moy */                
            }
            break;  
          
          
        case ADI_PWM_CHANNEL_AL:
            {
#if defined(__ADSP_MOY__)  
                pChannelValue->Value = AdiPwmConfig.PwmChALReg[pChannelValue->Number];
#else  /* Not Moy */                
                pChannelValue->Value = AdiPwmConfig.PwmChALReg;
#endif                
            }
            break;
          
         case ADI_PWM_CHANNEL_BL:
            {

#if defined(__ADSP_MOY__)  
                pChannelValue->Value = AdiPwmConfig.PwmChBLReg[pChannelValue->Number];
#else  /* Not Moy */
                pChannelValue->Value = AdiPwmConfig.PwmChBLReg;
#endif /* Moy */                                
            }
            break;  
                 
         case ADI_PWM_CHANNEL_CL:
            {
#if defined(__ADSP_MOY__)  
                pChannelValue->Value = AdiPwmConfig.PwmChCLReg[pChannelValue->Number];
#else  /* Not Moy */                
                pChannelValue->Value = AdiPwmConfig.PwmChCLReg;
#endif                
            }
            break;   
                       
    }    
          
    return ADI_PWM_RESULT_SUCCESS;
}
        
    
/*************************************************************************************
 **  Function:    adi_pwmSetDutyCycle                                               **
 **  Description: These functions perform error checking on the values, return the  **
 **            appropriateresult code, update the configuration structure and       **
 **            write to the actual hardware.                                        **
 ************************************************************************************/
   
ADI_PWM_RESULT adi_pwmSetDutyCycle(pADI_PWM_CHANNEL_DUTY_CYCLE pDutyCycle)
{
        
#if defined(ADI_SSL_DEBUG)

    u32  DeadTime, Period;
    
    /* Cannot set duty for low channel if SR is active */    
    if((pDutyCycle->Channel == ADI_PWM_CHANNEL_AL) || (pDutyCycle->Channel == ADI_PWM_CHANNEL_BL) || (pDutyCycle->Channel == ADI_PWM_CHANNEL_CL))
    {
        
#if defined(__ADSP_MOY__)  

        /* check the status of the PWM being set */ 
        /* Read in the PWM Status Register first  */
            PwmStatReg[pDutyCycle->Number].UnsignedValue  = *pADI_PWM_ADDRESS_STAT[pDutyCycle->Number];    
             
          /* if not in switch reluctance mode cannot set the low side */                       
        if (AdiPwmConfig.pPwmStatReg[pDutyCycle->Number]->BitField.SRMode == ADI_PWM_DISABLE) 
           return ADI_PWM_RESULT_SWITCH_RELUCTANCE_NOT_ACTIVE;     
               
#else   /* Not Moy */

    /* Read in the PWM Status Register first  */
    PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;    
             
    /* if not in switch reluctance mode cannot set the low side */                       
    if (AdiPwmConfig.pPwmStatReg->BitField.SRMode == ADI_PWM_DISABLE)              
        return ADI_PWM_RESULT_SWITCH_RELUCTANCE_NOT_ACTIVE;    
        
#endif /* Moy */    
    }    
    /* Make sure the duty cycle is within bounds according to the dead time and Period */

#if defined(__ADSP_MOY__)     
    /* get dead time from PWM1 DT Register  */                    
    DeadTime = (u32)AdiPwmConfig.PwmDtReg[pDutyCycle->Number];

       /* get the period from PWM1 TM Register */
       Period = (u32)AdiPwmConfig.PwmTmReg[pDutyCycle->Number];            

#else /* Not Moy */

    /* get dead time from PWMDT Register  */                    
    DeadTime = (u32)AdiPwmConfig.PwmDtReg;

    /* get the period from PWMTM Register */
    Period = (u32)AdiPwmConfig.PwmTmReg;                                                       

#endif /* Moy */

    if (abs(pDutyCycle->Value) > ((Period/2) + DeadTime)) 
    {
       return ADI_PWM_RESULT_INVALID_DUTY_CYCLE;
    }
    
#endif /*debug   */
    

    switch(pDutyCycle->Channel)
    {
          
#if defined(ADI_SSL_DEBUG)
 
       default:        

          return ADI_PWM_RESULT_INVALID_CHANNEL;      
       
#endif           
//       case ADI_PWM_CHANNEL_AH:
       case ADI_PWM_CHANNEL_AL:
          {


#if defined(__ADSP_MOY__) 
                    
               AdiPwmConfig.PwmChALReg[pDutyCycle->Number]  =  pDutyCycle->Value;    
               *pADI_PWM_ADDRESS_CHAL[pDutyCycle->Number] = AdiPwmConfig.PwmChALReg[pDutyCycle->Number];
#if defined(ADI_SSL_DEBUG)  
               
               DutySetAL[pDutyCycle->Number]++;
#endif                      
                                                        
#else  
  
  /* not Moy */   
#if defined(ADI_SSL_DEBUG)  
               
               DutySetAL++;
#endif  
                AdiPwmConfig.PwmChALReg  = pDutyCycle->Value;    
                *pADI_PWM_ADDRESS_CHAL = AdiPwmConfig.PwmChALReg;
#endif /* Moy */

          }         
          break;          
          
       case ADI_PWM_CHANNEL_A:    
          { 
              
              
#if defined(__ADSP_MOY__) 
               AdiPwmConfig.PwmChAReg[pDutyCycle->Number]  =  pDutyCycle->Value;    
              *pADI_PWM_ADDRESS_CHA[pDutyCycle->Number] = AdiPwmConfig.PwmChAReg[pDutyCycle->Number];
#if defined(ADI_SSL_DEBUG)            
              DutySetA[pDutyCycle->Number]++;
#endif                                                         
                             
#else     /* not Moy */                
#if defined(ADI_SSL_DEBUG)            
             DutySetA++;
#endif                  
             AdiPwmConfig.PwmChAReg  =  pDutyCycle->Value;    
             *pADI_PWM_ADDRESS_CHA = AdiPwmConfig.PwmChAReg;
#endif /* Moy */                 
                  
          }
          break; 
              
                       
//       case ADI_PWM_CHANNEL_BH:
       case ADI_PWM_CHANNEL_BL:
          {
                                     
#if defined(__ADSP_MOY__) 
              AdiPwmConfig.PwmChBLReg[pDutyCycle->Number]  =  pDutyCycle->Value;    
              *pADI_PWM_ADDRESS_CHBL[pDutyCycle->Number] = AdiPwmConfig.PwmChBLReg[pDutyCycle->Number];
#if defined(ADI_SSL_DEBUG)
           
              DutySetBL[pDutyCycle->Number]++;            
#endif                       

                                                         
#else     /* not Moy */   
#if defined(ADI_SSL_DEBUG)
           
              DutySetBL++;            
#endif                            
              AdiPwmConfig.PwmChBLReg  =  pDutyCycle->Value;  
              *pADI_PWM_ADDRESS_CHBL = AdiPwmConfig.PwmChBLReg;
#endif /* Moy */                 
                 
          }
          break;          
       
       case ADI_PWM_CHANNEL_B:
          {
                             
#if defined(__ADSP_MOY__) 

              AdiPwmConfig.PwmChBReg[pDutyCycle->Number]  =  pDutyCycle->Value;    
              *pADI_PWM_ADDRESS_CHB[pDutyCycle->Number] = AdiPwmConfig.PwmChBReg[pDutyCycle->Number];
#if defined(ADI_SSL_DEBUG)            
              DutySetB[pDutyCycle->Number]++;
#endif                                
                                
                             
#else     /* not Moy */                       
#if defined(ADI_SSL_DEBUG)            
              DutySetB++;
#endif                          
              AdiPwmConfig.PwmChBReg  =  pDutyCycle->Value;   
              *pADI_PWM_ADDRESS_CHB = AdiPwmConfig.PwmChBReg;
#endif /* Moy */                  
                  
                 
          }
          break;          

       case ADI_PWM_CHANNEL_CL:
          {                 

#if defined(__ADSP_MOY__) 

               AdiPwmConfig.PwmChCLReg[pDutyCycle->Number]  =  pDutyCycle->Value;    
               *pADI_PWM_ADDRESS_CHCL[pDutyCycle->Number] = AdiPwmConfig.PwmChCLReg[pDutyCycle->Number];
#if defined(ADI_SSL_DEBUG)
               DutySetCL[pDutyCycle->Number]++;            
#endif                                                        
                          
#else     /* not Moy */  
#if defined(ADI_SSL_DEBUG)
           
               DutySetCL++;            
#endif                             
               AdiPwmConfig.PwmChCLReg  = pDutyCycle->Value;
               *pADI_PWM_ADDRESS_CHCL = AdiPwmConfig.PwmChCLReg;  
#endif /* Moy */                       
             
           }
           break;          
       
       case ADI_PWM_CHANNEL_C:       
           {
#if defined(__ADSP_MOY__)
 
               AdiPwmConfig.PwmChCReg[pDutyCycle->Number] =  pDutyCycle->Value;    
               *pADI_PWM_ADDRESS_CHC[pDutyCycle->Number] = AdiPwmConfig.PwmChCReg[pDutyCycle->Number];
#if defined(ADI_SSL_DEBUG)            
               DutySetC[pDutyCycle->Number]++;
#endif                          

                                       
#else     /* not Moy */                   
#if defined(ADI_SSL_DEBUG)            
             DutySetC++;
#endif                   
                AdiPwmConfig.PwmChCReg  = pDutyCycle->Value;
                *pADI_PWM_ADDRESS_CHC = AdiPwmConfig.PwmChCReg;    
#endif /* Moy */ 
                   
            }
            break;          
           
       case ADI_PWM_CHANNEL_ALL:    
            {
                 
#if defined(__ADSP_MOY__) 

                
                AdiPwmConfig. PwmChAReg[pDutyCycle->Number] = pDutyCycle->Value;
                AdiPwmConfig.PwmChBReg[pDutyCycle->Number]= pDutyCycle->Value;
                AdiPwmConfig.PwmChCReg[pDutyCycle->Number] = pDutyCycle->Value;
                *pADI_PWM_ADDRESS_CHA[pDutyCycle->Number] = AdiPwmConfig. PwmChAReg[pDutyCycle->Number];
                *pADI_PWM_ADDRESS_CHB[pDutyCycle->Number] = AdiPwmConfig.PwmChBReg[pDutyCycle->Number];      
                *pADI_PWM_ADDRESS_CHC[pDutyCycle->Number] = AdiPwmConfig.PwmChCReg[pDutyCycle->Number];
#if defined(ADI_SSL_DEBUG)
                DutySetA[pDutyCycle->Number]++;
                DutySetB[pDutyCycle->Number]++;
                DutySetC[pDutyCycle->Number]++;                                       
#endif                     
    
                /* if in switch reluctance mode also set the low side */                                                
                if (AdiPwmConfig.pPwmStatReg[pDutyCycle->Number]->BitField.SRMode == ADI_PWM_ENABLE)              
                {
                     AdiPwmConfig.PwmChALReg[pDutyCycle->Number] = pDutyCycle->Value;
                     AdiPwmConfig.PwmChBLReg[pDutyCycle->Number] = pDutyCycle->Value;
                     AdiPwmConfig.PwmChCLReg[pDutyCycle->Number] = pDutyCycle->Value;
                     *pADI_PWM_ADDRESS_CHAL[pDutyCycle->Number] = AdiPwmConfig. PwmChAReg[pDutyCycle->Number];
                     *pADI_PWM_ADDRESS_CHBL[pDutyCycle->Number]  = AdiPwmConfig.PwmChBReg[pDutyCycle->Number];      
                     *pADI_PWM_ADDRESS_CHCL[pDutyCycle->Number] = AdiPwmConfig.PwmChCReg[pDutyCycle->Number]; 
                    
#if defined(ADI_SSL_DEBUG)
                     DutySetAL[pDutyCycle->Number]++;
                     DutySetBL[pDutyCycle->Number]++;
                     DutySetCL[pDutyCycle->Number]++;             
#endif                                        
                 }
                         
                        
                             
#else     /* not Moy */                   
                          
                 AdiPwmConfig.PwmChAReg = pDutyCycle->Value;
                 AdiPwmConfig.PwmChBReg = pDutyCycle->Value;
                 AdiPwmConfig.PwmChCReg = pDutyCycle->Value;
                 *pADI_PWM_ADDRESS_CHA = AdiPwmConfig.PwmChAReg;
                 *pADI_PWM_ADDRESS_CHB = AdiPwmConfig.PwmChBReg;      
                 *pADI_PWM_ADDRESS_CHC = AdiPwmConfig.PwmChCReg;
#if defined(ADI_SSL_DEBUG)
                 DutySetA++;
                 DutySetB++;
                 DutySetC++;                                       
#endif                          
            /* Read in the PWM Status Register first  */
                PwmStatReg.UnsignedValue = *pADI_PWM_ADDRESS_STAT;    
    
             /* if in switch reluctance mode also set the low side */                                                
             if (AdiPwmConfig.pPwmStatReg->BitField.SRMode == ADI_PWM_ENABLE)              
                 {
                    AdiPwmConfig.PwmChALReg = pDutyCycle->Value;
                    AdiPwmConfig.PwmChBLReg = pDutyCycle->Value;
                    AdiPwmConfig.PwmChCLReg = pDutyCycle->Value;
                    *pADI_PWM_ADDRESS_CHAL = AdiPwmConfig.PwmChAReg;
                    *pADI_PWM_ADDRESS_CHBL = AdiPwmConfig.PwmChBReg;      
                    *pADI_PWM_ADDRESS_CHCL = AdiPwmConfig.PwmChCReg;  
                    
#if defined(ADI_SSL_DEBUG)
                    DutySetAL++;
                    DutySetBL++;
                    DutySetCL++;             
#endif            
                 }    
                       
#endif /* Moy */                                                        

              break;    
                        
        } /* end switch */
    } /* end if not trying to set low side */
    return(ADI_PWM_RESULT_SUCCESS);
}


          
#endif  /* BF51X parts */
