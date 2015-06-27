/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_ebiu.c,v $
$Revision: 2692 $
$Date: 2010-04-28 12:28:13 -0400 (Wed, 28 Apr 2010) $

Description:
			EBIU Implementation Module

*********************************************************************************/

#include "adi_ebiu_module.h"
#include "../pwr/adi_pwr_module.h"


#define U32_REG_CAST(V) (*(u32*)&(V))
#define U16_REG_CAST(V) (*(u16*)&(V))
		
 
/********************************************************************************
* Configuration settings structure
*********************************************************************************/
//section("L1_data_a")
ADI_EBIU_CONFIG adi_ebiu_settings;


/********************************************************************************
* Global variables
*********************************************************************************/

/* Indicates that the Control function has been called by the Init Function, to 
   prevent applying the config in the Control function, deferring the decision 
   to the Init function  */
   
static u8 _adi_ebiu_ControlCalledFromInit = 0;

/* keeps track of Asynch commands processed by ebiu_Init and ebiu_Control */
static u32 AMC_cmd_count=0;


/* keeps track of recursive entries to adi_ebiu_Control */
static u32 RecursiveControlEntry=0;

/* keeps track of SDRAM commands processed by ebiu_Init and ebiu_Control */
static u32 cmd_count=0;


/* flag to say to apply the configuration to the hardware */
static u32 apply_config=0;




/**********************************************************************************
* Macro to compute clock cycles given a ADI_EBIU_TIME struct and fsclk value 
************************************************************************************/

#define TIMING_CYCLES(R,T,F) \
		R = ( (T).units==ADI_EBIU_TIMING_UNIT_MILLISEC ? \
					(T).value*(F)*1000 + 1 : \
					(T).value*(F)/(T).units + 1 );

					
#define TIMING_CYCLES_ROUNDED(R,T,F) \
        R = ( (T).units==ADI_EBIU_TIMING_UNIT_MILLISEC ? \
                    (int)((float)(T.value) * F * 1000 + .5) + 1 : \
                    (int)((float)(T.value) * F / T.units + .5) + 1 );


/********************************************************************************
* Core synchronization macros and interrupt handler prototype
*********************************************************************************/
#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)

#define SYNC_COREB \
	if (adi_ebiu_settings.auto_sync_enabled) \
		adi_pwr_SyncCoreB(adi_ebiu_settings.pLockVar);
#define RELEASE_COREB \
	if (adi_ebiu_settings.auto_sync_enabled) \
		adi_pwr_ReleaseCoreB(adi_ebiu_settings.pLockVar);

ADI_INT_HANDLER( adi_ebiu_CoreBSuppInt0Handler );
#endif
				


	
/***********************************************************************************
* Function:    adi_ebiu_SetInitFlag
* Description: Queries the initialized flag and sets it if unset.  Returns 1, if 
*			   it was already initialized, 0, if not.
************************************************************************************/
static int 
adi_ebiu_SetInitFlag(void)
{
	// mask interrupts to protect the Initialized flag from
	// being accessed elsewhere
	u32 imask = cli();
	
	/* Test if already initialized. Return 1 if so. */
	if (adi_ebiu_settings.InitializedFlag) 
	{
		sti(imask);
		return 1;
	}
	
	// Set Intitialized flag	
	adi_ebiu_settings.InitializedFlag = 1; 
	
	// unmask interrupts
	sti(imask);
	
	// return 0 to indicate success
	return 0;
}



				
/***************************************************************************************
* Function:    adi_ebiuSetAsynchReg
* 
*  Description: This function updates one of the Asynch memory control registers with 
*               the value passed in. The bank number determines which register to update,
*               the bank number, mask and shift values are used to move the data to the 
*               correct position within the register.  
*
*               ALSO-- increments the static counter AMC_cmd_count.
****************************************************************************************/
 	        
void adi_ebiuSetAsynchReg(
                               u32 Value,
                               u32 BankNumber, 
                               u32 Mask, 
                               u32 Shift )
{	

    /* if bank number is NOT evenly divisible by 2 (it is 1 or it is 3 ) */
    if(( BankNumber % 2 ) != 0 ) 
    {
        /* banks 1 and 3 are shifted into the upper word of the register */
        /* banks 0 and 2 are left in the lower word of the register */
       	Mask <<= 16;
    	Shift += 16;
    }
	
 
    /* See if the bank number is 0 or 1 */
    if( BankNumber < 2 )  
    {
	
        /* Bank numbers 0 and 1 - Get the current contents of the AMBCTL0 register, clear 
           out the bits we are going to change, shift the bits into the appropriate position 
           in the register, OR in the value */		        
	       
        adi_ebiu_settings.ambctl0 = (( adi_ebiu_settings.ambctl0 & ~Mask ) | (Value << Shift) );
    }
    else
    {
        /* Bank numbers 2 and 3 - Get the current contents of the AMBCTL1 register, clear 
           out the bits we are going to change, shift the bits into the appropriate position 
           in the register, OR in the value */		        
	       
        adi_ebiu_settings.ambctl1 = (( adi_ebiu_settings.ambctl1 & ~Mask ) | (Value << Shift) );

    }           	     	
    AMC_cmd_count++;	    
    return;	          
}



/***************************************************************************************
* Function:    adi_ebiuSetAsynchRegsForBanks
* 
*  Description: This function checks to see whether the command is for a specific asynch 
*               or for all of the banks.  In either case, it calls the function which updates 
*               the Asynch memory control registers with the value passed in. 
*               This function can be called from adi_ebiu_Init, from adi_ebiu_Control or 
*               from adi_ebiuSetAsynchTimingReg.  It returns a result in case an invalid 
*               bank number was passed.
****************************************************************************************/
 	
 
ADI_EBIU_RESULT adi_ebiuSetAsynchRegsForBanks(
                               u32 Value,
                               u32 BankNumber, 
                               u32 Mask, 
                               u32 Shift )
{
    ADI_EBIU_RESULT Result = ADI_EBIU_RESULT_SUCCESS;
 
 
#ifdef ADI_SSL_DEBUG          
        if(  BankNumber > ADI_EBIU_BANK_ALL )  
        {
            /* this is not a valid bank number */
            return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_NUMBER;  
        }
#endif
 
    if ( BankNumber == ADI_EBIU_BANK_ALL )
    {
        for( BankNumber = 0; BankNumber < ADI_EBIU_BANK_ALL;  BankNumber++)
        {
             adi_ebiuSetAsynchReg( Value,  BankNumber, Mask, Shift );
        }
    }
    else
    {
          adi_ebiuSetAsynchReg( Value,  BankNumber, Mask, Shift );       
    }
 
    return Result;
}
 

				
/***************************************************************************************
* Function:    adi_ebiuSetAsynchTimingReg
* 
*  Description: This function updates one of the Asynch memory control registers with the 
*     information  passed in. The bank number is found in the BANK_TIMING_VALUE structure, 
*     and it is used in conjunction with the mask and shift values that are passed in, to
*     provide the necessary information to update the correct register. The mask and shift 
*     values are specific to the command, and they are specified by the calling function. 
* 
****************************************************************************************/
 	        
ADI_EBIU_RESULT adi_ebiuSetAsynchTimingReg(
                               ADI_EBIU_ASYNCH_BANK_TIMING bank_timing_value, 
                               u32 Mask, 
                               u32 Shift )
{	

    u32 Cycles;
    

    /* Determine whether to use the Cycles value from the structure.  If the timing units field  
       is non-zero, calculate cycles based on the time value, timing units, and max (worst case)
       SCLKm using TIMING_CYCLES macro */

    if( bank_timing_value.bank_time.time.value != 0 )
    {
        TIMING_CYCLES(Cycles, bank_timing_value.bank_time.time, 133);
    }
    else
    {
        /* Get the cycles value from the structure and use it */   
        Cycles = bank_timing_value.bank_time.cycles;   
    }
        
    adi_ebiuSetAsynchRegsForBanks( Cycles,  bank_timing_value.bank_number, Mask, Shift );          	     	
	    
    return ADI_EBIU_RESULT_SUCCESS;	          
}




/************************************************************************************
* Function:    adi_ebiu_GetConfigSize
* Description: Returns the size of the EBIU module config structure
*************************************************************************************/

size_t 
adi_ebiu_GetConfigSize()
{
	return sizeof(ADI_EBIU_CONFIG);
}



/* external reference to assembly language function to apply asynch register values to HW */
void /* section("L1_code") */
adi_ebiu_ApplyAMConfig(
	u16 amgctl,
	u32 ambctl0,
	u32 ambctl1 );

#if !defined (__ADI_EBIU_USE_DDR_MEMORY__)

/* external reference to assembly language function to apply SDRAM register values to HW */	
void /* section("L1_code") */
adi_ebiu_ApplyConfig(
	u32 sdgctl,
	void *sdbctl,
	u16 sdrrc,
	int powerup
);

#endif

/*****************************************************************************************
* Function:    adi_ebiu_Control
*
* Description: Sets or queries various configuration settings for DDR, Asynch and SDRAM
*
*********************************************************************************************/

ADI_EBIU_RESULT 
adi_ebiu_Control(ADI_EBIU_COMMAND Command, void *Value)
{

    ADI_EBIU_RESULT Result;
    
/* Make sure that the service is initialized, first the SDRAM way */        
#if !defined (__ADI_EBIU_USE_DDR_MEMORY__)  
 
	if ( adi_ebiu_settings.InitializedFlag==0) // || !EBIU_SDGCTL_SCTLE_GET(adi_ebiu_settings.sdgctl) )
#else

/* Then the DDR way */
	if ( adi_ebiu_settings.InitializedFlag == 0 ) 

#endif
		return ADI_EBIU_RESULT_NOT_INITIALIZED;
		
		
	if (RecursiveControlEntry)
		cmd_count++;
	else
		cmd_count=0;

	apply_config=1;
	RecursiveControlEntry++;
	switch (Command)
	{
		default:
			RecursiveControlEntry--;
			return ADI_EBIU_RESULT_BAD_COMMAND;

		case ADI_EBIU_CMD_END:
			break;

		case ADI_EBIU_CMD_PAIR:
			{
				ADI_EBIU_COMMAND_PAIR *cmd = (ADI_EBIU_COMMAND_PAIR *)Value;
				ADI_EBIU_RESULT result;
				if ( (result=adi_ebiu_Control(cmd->kind, (void*)cmd->value))!=ADI_EBIU_RESULT_SUCCESS )
					return result;
			}
			break;
			
		case ADI_EBIU_CMD_TABLE:
			{	
				ADI_EBIU_COMMAND_PAIR *cmd = (ADI_EBIU_COMMAND_PAIR *)Value;
				while (cmd->kind != ADI_EBIU_CMD_END)
				{
					ADI_EBIU_RESULT result;
					if ( (result=adi_ebiu_Control(cmd->kind, (void*)cmd->value))!=ADI_EBIU_RESULT_SUCCESS )
						return result;
					cmd++;
				}
			}
			break;
			
#if defined (__ADI_EBIU_USE_DDR_MEMORY__)

		case ADI_EBIU_CMD_DDR_SELF_REFRESH_REQUEST:
			{
			    
				u32 SelfRefresh = (u32)Value;
			    
                EBIU_DDRCTL4_SELF_REFRESH_REQUEST_SET( adi_ebiu_settings.ddrctl4, (u16)SelfRefresh );			    
			    if( SelfRefresh == 0 )
			    {
			        adi_ebiu_DDRSelfRefreshDisable();
			    }
			    else
			    {
			        adi_ebiu_DDRSelfRefreshEnable();
			    }			    
			    break;
			}
#endif			

/****************************************************************/
/*********  Commands for non-DDR SDRAM based EBIU interface *****/
/****************************************************************/						
#if !defined (__ADI_EBIU_USE_DDR_MEMORY__)
	
			
		case ADI_EBIU_CMD_SET_SDRAM_ENABLE:
			{
				ADI_EBIU_SDRAM_ENABLE ebe = (ADI_EBIU_SDRAM_ENABLE)Value;
#ifdef ADI_SSL_DEBUG
				if (ebe > ADI_EBIU_SDRAM_EBE_ENABLE)
					return ADI_EBIU_RESULT_INVALID_SDRAM_EBE;
#endif
				apply_config=0;
#if !defined(__ADSP_TETON__) 

				EBIU_SDBCTL_EBE_SET(adi_ebiu_settings.sdbctl,ebe);

				*pEBIU_SDBCTL = *(u16*)&adi_ebiu_settings.sdbctl;
#else
                if (ebe == ADI_EBIU_SDRAM_EBE_ENABLE) {
				    *(u32*)&adi_ebiu_settings.sdbctl |= adi_ebiu_settings.sdbctl_enable_mask;
                } else {
				    *(u32*)&adi_ebiu_settings.sdbctl &= ~(adi_ebiu_settings.sdbctl_enable_mask);
                }
				*pEBIU_SDBCTL = *(u32*)&adi_ebiu_settings.sdbctl;
#endif				
			}
			break;

		case ADI_EBIU_CMD_SET_SDRAM_SRFS:
			{
				ADI_EBIU_SDRAM_SRFS srfs = (ADI_EBIU_SDRAM_SRFS)Value;
#ifdef ADI_SSL_DEBUG
				if (srfs > ADI_EBIU_SDRAM_SRFS_ENABLE)
					return ADI_EBIU_RESULT_INVALID_SDRAM_SRFS;
#endif
				if ( srfs == ADI_EBIU_SDRAM_SRFS_ENABLE)  {
					// Place the SDRAM in self-refresh mode and
					// wait for it to take effect
					apply_config=0;
					adi_ebiu_SelfRefreshEnable();
				} else {
					// SDRAM will come out of self-refresh at next access,
					// and the following will ensure it stays that way
					apply_config=1;
					EBIU_SDGCTL_SRFS_SET(adi_ebiu_settings.sdgctl,0);
				}
			}
			break;

		case ADI_EBIU_CMD_SET_SDRAM_PUPSD:
#ifdef ADI_SSL_DEBUG
			{
				ADI_EBIU_SDRAM_PUPSD pupsd = (ADI_EBIU_SDRAM_PUPSD)Value;
				if ( pupsd > ADI_EBIU_SDRAM_PUPSD_15CYCLES )
					return ADI_EBIU_RESULT_INVALID_SDRAM_PUPSD;
			}
#endif
			EBIU_SDGCTL_PUPSD_SET(adi_ebiu_settings.sdgctl,(ADI_EBIU_SDRAM_PUPSD)Value);
			break;

		case ADI_EBIU_CMD_SET_SDRAM_PSM:
#ifdef ADI_SSL_DEBUG
			{
				ADI_EBIU_SDRAM_PSM psm = (ADI_EBIU_SDRAM_PSM)Value;
				if (  psm > ADI_EBIU_SDRAM_PSM_REFRESH_LAST )
					return ADI_EBIU_RESULT_INVALID_SDRAM_PSM;
			}
#endif
			EBIU_SDGCTL_PSM_SET(adi_ebiu_settings.sdgctl,(ADI_EBIU_SDRAM_PSM)Value);
			break;

		case ADI_EBIU_CMD_SET_SDRAM_EBUFE:
#ifdef ADI_SSL_DEBUG
			{
				ADI_EBIU_SDRAM_EBUFE ebufe = (ADI_EBIU_SDRAM_EBUFE)Value;
				if ( ebufe > ADI_EBIU_SDRAM_EBUFE_ENABLE )
					return ADI_EBIU_RESULT_INVALID_SDRAM_EBUFE;
			}
#endif
			EBIU_SDGCTL_EBUFE_SET(adi_ebiu_settings.sdgctl,(ADI_EBIU_SDRAM_EBUFE)Value);
			break;

		case ADI_EBIU_CMD_SET_SDRAM_FBBRW:
#ifdef ADI_SSL_DEBUG
			{
				ADI_EBIU_SDRAM_FBBRW fbbrw = (ADI_EBIU_SDRAM_FBBRW)Value;
				if ( fbbrw > ADI_EBIU_SDRAM_FBBRW_ENABLE )
					return ADI_EBIU_RESULT_INVALID_SDRAM_FBBRW;
			}
#endif
			EBIU_SDGCTL_FBBRW_SET(adi_ebiu_settings.sdgctl,(ADI_EBIU_SDRAM_FBBRW)Value);
			break;

		case ADI_EBIU_CMD_SET_SDRAM_CDDBG:
#ifdef ADI_SSL_DEBUG
			{
				ADI_EBIU_SDRAM_CDDBG cddbg = (ADI_EBIU_SDRAM_CDDBG)Value;
				if ( cddbg > ADI_EBIU_SDRAM_CDDBG_ENABLE )
					return ADI_EBIU_RESULT_INVALID_SDRAM_CDDBG;
			}
#endif
			EBIU_SDGCTL_CDDBG_SET(adi_ebiu_settings.sdgctl,(ADI_EBIU_SDRAM_CDDBG)Value);
			break;
			

/**************************************************************************
  End of commands for non-DDR SDRAM based EBIU interface 
 **************************************************************************/			
#endif
			
/**************************************************************************

ADI_EBIU_ASYNCH_BANK_TIMING: 
 
      bank_number
      bank_time _____cycles         
              |
              |_____time_____value  
	                 |
                     |_____units
                                           
*************************************************************************/


/**************************************************************************
  Commands for BOTH DDR and non-DDR based EBIU interface		        			
  Asynch Memory Controller commands 
 **************************************************************************/			

		case ADI_EBIU_CMD_SET_ASYNCH_BANK_TRANSITION_TIME:   /* set delay to avoid bus contention  */
		    {
		        ADI_EBIU_ASYNCH_BANK_TIMING  bank_timing_value = *(ADI_EBIU_ASYNCH_BANK_TIMING *)Value;
#ifdef ADI_SSL_DEBUG 
		            
                if( bank_timing_value.bank_time.time.value == 0 )
                {
                /* The structure can either specify cycles or timing units but not both */
		            if (  bank_timing_value.bank_time.cycles > ADI_EBIU_ASYNCH_TT_3_CYCLES )
		            
		                return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_TRANSITION_TIME;
                }
#endif      

    	        /* The function iuses the info in the structure to set the correct register accordingly */
		         /* also checks for valid bank number  */    	        
		        if( ( Result = adi_ebiuSetAsynchTimingReg( bank_timing_value, 0x0C, 2 ) ) != ADI_EBIU_RESULT_SUCCESS )
		            return Result;
		            
		        break;
		    }
					
		
		case ADI_EBIU_CMD_SET_ASYNCH_BANK_READ_ACCESS_TIME:  /* set time from R-enable assertion to de-assertion time */
		
		    {
		        ADI_EBIU_ASYNCH_BANK_TIMING bank_timing_value = *(ADI_EBIU_ASYNCH_BANK_TIMING *)Value;
		        
#ifdef ADI_SSL_DEBUG 
		            
                if( bank_timing_value.bank_time.time.value == 0 )
                {
                /* The structure can either specify cycles or timing units but not both */
		            if ((  bank_timing_value.bank_time.cycles > 0xF )  || (  bank_timing_value.bank_time.cycles < 1 ))
		            
		                return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_READ_ACCESS_TIME;
                }
#endif      
		        
		            
		        /* The function iuses the info in the structure to set the correct register accordingly */
		         /* also checks for valid bank number  */
		        adi_ebiuSetAsynchTimingReg( bank_timing_value, 0xF00, 8 );
		        break;
		    }

		
		case ADI_EBIU_CMD_SET_ASYNCH_BANK_WRITE_ACCESS_TIME: /* set time fromW-enable assertion to de-assertion time */		
		    {
		        ADI_EBIU_ASYNCH_BANK_TIMING bank_timing_value = *(ADI_EBIU_ASYNCH_BANK_TIMING *)Value;
#ifdef ADI_SSL_DEBUG 
		            
                if( bank_timing_value.bank_time.time.value == 0 )
                {
                /* The structure can either specify cycles or timing units but not both */
		            if ((  bank_timing_value.bank_time.cycles > 0xF )  || (  bank_timing_value.bank_time.cycles < 1 ))
		            
		                return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_WRITE_ACCESS_TIME;
                }
#endif  		        
		       		        /* The function iuses the info in the structure to set the correct register accordingly */
                /* also checks for valid bank number  */
		        
		        adi_ebiuSetAsynchTimingReg( bank_timing_value, 0xF000, 12 );
		        break;
		    }				    
		    		
		
		case ADI_EBIU_CMD_SET_ASYNCH_BANK_SETUP_TIME:        /* set time from start of memory cycle to R/W-enable     */
		
		    {
		        ADI_EBIU_ASYNCH_BANK_TIMING bank_timing_value = *(ADI_EBIU_ASYNCH_BANK_TIMING *)Value;
		        
#ifdef ADI_SSL_DEBUG 
		            
                if( bank_timing_value.bank_time.time.value == 0 )
                {
                /* The structure can either specify cycles or timing units but not both */
		            if (  bank_timing_value.bank_time.cycles > ADI_EBIU_ASYNCH_ST_3_CYCLES )
		            
		                return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_SETUP_TIME;
                }
#endif 		        
		        
		        /* The function iuses the info in the structure to set the correct register accordingly */
		        /* also checks for valid bank number */
		        adi_ebiuSetAsynchTimingReg( bank_timing_value, 0x30, 4 );
		        break;
		    }		
			
		
		case ADI_EBIU_CMD_SET_ASYNCH_BANK_HOLD_TIME:         /* set time from R/W-enable de-assertion to end of memory cycle */
		    {
		        ADI_EBIU_ASYNCH_BANK_TIMING bank_timing_value = *(ADI_EBIU_ASYNCH_BANK_TIMING *)Value;
		        
#ifdef ADI_SSL_DEBUG 
		            
                if( bank_timing_value.bank_time.time.value == 0 )
                {
                /* The structure can either specify cycles or timing units but not both */
		            if (  bank_timing_value.bank_time.cycles > ADI_EBIU_ASYNCH_ST_3_CYCLES )
		            
		                return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_HOLD_TIME;
                }
#endif 
		        /* The function iuses the info in the structure to set the correct register accordingly */
		        /* also checks for valid ban number */
		        adi_ebiuSetAsynchTimingReg( bank_timing_value, 0xC0, 6 );
		        break;
		    }									
	
	}

	RecursiveControlEntry--;
	
#if !defined (__ADI_EBIU_USE_DDR_MEMORY__)	    

	/* Avoid calling ApplyConfig more than once.
	 ApplyConfig is not called if the call to control is made from the Init function */
	if( !_adi_ebiu_ControlCalledFromInit && !RecursiveControlEntry  && (cmd_count>1 || apply_config) )
	{
	    
	    	    
#if !defined(__ADSP_TETON__)
		adi_ebiu_ApplyConfig( 
					U32_REG_CAST(adi_ebiu_settings.sdgctl), 
					(void*)&U16_REG_CAST(adi_ebiu_settings.sdbctl), 
					0, 
					0);
#else
		SYNC_COREB
		
		u32 sdbctl_reg = U32_REG_CAST(adi_ebiu_settings.sdbctl) | adi_ebiu_settings.sdbctl_enable_mask;
		adi_ebiu_ApplyConfig( 
					U32_REG_CAST(adi_ebiu_settings.sdgctl), 
					(void*)&sdbctl_reg, 
					0, 
					0);

		RELEASE_COREB
		
/* End of Teton case */		
#endif
	}
/* End of non-DDR case */
#endif



	/* Asynch Memory commands are processed in 'adi_ebiu_Control' can be called from 'adi_ebiu_Init' 
	   or it can be called by a user's application, so we have to check to see which it is.  If we are 
	   returning to the user's app, we should apply changes here, if returning to 'adi_ebiu_Init' we can 
	   wait on applying the changes. */
	 	  
	if(!_adi_ebiu_ControlCalledFromInit && AMC_cmd_count != 0 )
	{
	    AMC_cmd_count = 0;
	    adi_ebiu_ApplyAMConfig( adi_ebiu_settings.amgctl, adi_ebiu_settings.ambctl0, adi_ebiu_settings.ambctl1 );
	}

	
	return ADI_EBIU_RESULT_SUCCESS;

}



/***************************************************************************
*
*     The following section includes functions to support non-DDR SDRAM   
*
***************************************************************************/
#if !defined (__ADI_EBIU_USE_DDR_MEMORY__)



/********************************************************************************
* cast pointers to SDRAM MMRs
*********************************************************************************/
static volatile ADI_EBIU_SDGCTL_REG *ebiu_sdgctl = (volatile ADI_EBIU_SDGCTL_REG *)(EBIU_SDGCTL);


/********************************************************************************
* Core Identification macro
*********************************************************************************/
#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)
#define core_ident (*ADI_SRAM_BASE_ADDR_MMR)
#endif

/********************************************************************************
* static functions
*********************************************************************************/
#if !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__) && !defined(__ADSP_MOAB__)  && !defined(__ADSP_BRODIE__) 
/* This function is no Longer supported, starting with Moab and Kookaburra */  
static int adi_ebiu_set_ezkit_values(ADI_EBIU_EZKIT revno,ADI_EBIU_SDBCTL_REG *sdbctl);
#endif
static int adi_ebiu_set_sdram_values(u32 bankno,u32 module_id,ADI_EBIU_SDBCTL_REG *sdbctl);



// prototypes
static void adi_ebiu_SetLockVarPointer(
		void *pLockVar		// pointer to lock variable in L2 shared memory 
		);

/***********************************************************************************
***********************************************************************************

								API Functions

***********************************************************************************
************************************************************************************/

/************************************************************************************
* Function:    adi_ebiu_Init
* Description: Initialises the EBIU Module.
*************************************************************************************/
void /* section("L1_code") */
adi_ebiu_ApplyConfig(
	u32 sdgctl,
	void *sdbctl,
	u16 sdrrc,
	int powerup
);

ADI_EBIU_RESULT 
adi_ebiu_Init(
		const ADI_EBIU_COMMAND_PAIR *table, 
		const u16 Reserved
)

{
	ADI_EBIU_COMMAND_PAIR *cmd = (ADI_EBIU_COMMAND_PAIR *)table;
	ADI_EBIU_SDGCTL_REG sdgctl;
	ADI_EBIU_SDBCTL_REG sdbctl;
	u32 fcclk,fsclk,fvco;
	ADI_EBIU_RESULT Result;
	

	if (adi_ebiu_SetInitFlag())
		return ADI_EBIU_RESULT_ALREADY_INITIALIZED;

		

		
	// set to XML reset values
	*(u32*)&adi_ebiu_settings.sdgctl = ADI_EBIU_SDGCTL_RESET;
	*(u16*)&adi_ebiu_settings.sdbctl = ADI_EBIU_SDBCTL_RESET;
	*(u16*)&adi_ebiu_settings.sdrrc = ADI_EBIU_SDRRC_RESET;
	adi_ebiu_settings.MHzFactor		= 1000000;


	// set Asynch cached registers to reset values
	*(u16*)&adi_ebiu_settings.amgctl = ADI_EBIU_AMGCTL_RESET;
	*(u32*)&adi_ebiu_settings.ambctl0 = ADI_EBIU_AMBCTL0_RESET;
	*(u32*)&adi_ebiu_settings.ambctl1 = ADI_EBIU_AMBCTL1_RESET;
		

#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)
	// Supplemental interrupt 0 - default value 7
	adi_ebiu_settings.IVG_SuppInt0 = 7;
	// By default, auto synchronization is switched off - so that its embedded use within the 
	// power management module needs no special treatment.
	adi_ebiu_settings.auto_sync_enabled = 0;
#endif
	
#ifdef ADI_SSL_DEBUG
	u8 use_sdram = 1;
	u8 bank_size_set = 0;
	u8 bank_width_set = 0;
	u8 cas_latency_set = 0;
	u8 trasmin_set = 0;
	u8 trpmin_set = 0;
	u8 trcdmin_set = 0;
	u8 twrmin_set = 0;
	u8 refresh_set = 0;
	
	

	

	//ADI_EBIU_SDRAM_MODULE_SPEED_GRADE speed_grade;
	if (table==0)
		return ADI_EBIU_RESULT_FAILED;
	// only permit one initialisation per core	
#endif

	AMC_cmd_count = 0;

	for ( ; cmd->kind != ADI_EBIU_CMD_END ; cmd++)
	{
		switch (cmd->kind)
		{
			default:
				return ADI_EBIU_RESULT_BAD_COMMAND;


			case ADI_EBIU_CMD_SET_SDRAM_SCTLE:
				{
					ADI_EBIU_SDRAM_SCTLE sctle = (ADI_EBIU_SDRAM_SCTLE)cmd->value;
#ifdef ADI_SSL_DEBUG
					if (sctle > ADI_EBIU_SDRAM_SCTLE_ENABLE)
						return ADI_EBIU_RESULT_INVALID_SDRAM_SCTLE;
					use_sdram = (u8)(sctle);
#endif
					EBIU_SDGCTL_SCTLE_SET(adi_ebiu_settings.sdgctl,sctle);
				}
				break;

#if defined(__ADSP_TETON__)
			case ADI_EBIU_CMD_SET_SDRAM_SCK1E:
				{
					ADI_EBIU_SDRAM_SCK1E sck1e = (ADI_EBIU_SDRAM_SCK1E)cmd->value;
#ifdef ADI_SSL_DEBUG
					if (sck1e > ADI_EBIU_SDRAM_SCTLE_ENABLE)
						return ADI_EBIU_RESULT_INVALID_SDRAM_SCK1E;
#endif
					EBIU_SDGCTL_SCK1E_SET(adi_ebiu_settings.sdgctl,sck1e);
				}
				break;
#endif

			
#if !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__) && !defined(__ADSP_MOAB__) && !defined(__ADSP_BRODIE__)   
/* this command is no longer supported for these products */ 
			case ADI_EBIU_CMD_SET_EZKIT:
#ifndef ADI_SSL_DEBUG
				adi_ebiu_set_ezkit_values((ADI_EBIU_EZKIT)cmd->value,&adi_ebiu_settings.sdbctl);
#else
				if (!adi_ebiu_set_ezkit_values((ADI_EBIU_EZKIT)cmd->value,&adi_ebiu_settings.sdbctl))
					return ADI_EBIU_RESULT_INVALID_EZKIT;
				bank_size_set++;
				bank_width_set++;
				cas_latency_set++;
				trasmin_set++;
				trpmin_set++;
				trcdmin_set++;
				twrmin_set++;
				refresh_set++;
#endif
				break;
#endif
				
			case ADI_EBIU_CMD_SET_SDRAM_MODULE:				// sets the timings etc for a given (Micron) SDRAM module
					//ADI_EBIU_SDRAM_BANK_VALUE *module = (ADI_EBIU_SDRAM_BANK_VALUE*)cmd->value;

#ifndef ADI_SSL_DEBUG
//				adi_ebiu_set_sdram_values(0,module->value.module,&adi_ebiu_settings.sdbctl);
				adi_ebiu_set_sdram_values(0,(u32)cmd->value,&adi_ebiu_settings.sdbctl);
#else
				//if (speed_grade && ADI_EBIU_SDRAM_GET_SPEED_GRADE(module->value.module)!= speed_grade)
				//	return ADI_EBIU_RESULT_INCONSISTENT_SPEED_GRADE;

				if (!adi_ebiu_set_sdram_values(0,(u32)cmd->value,&adi_ebiu_settings.sdbctl))
//				if (!adi_ebiu_set_sdram_values(0,module->value.module,&adi_ebiu_settings.sdbctl))

					return ADI_EBIU_RESULT_INVALID_SDRAM_MODULE;
				bank_width_set++;
				cas_latency_set++;
				trasmin_set++;
				trpmin_set++;
				trcdmin_set++;
				twrmin_set++;
				refresh_set++;
#endif

#if defined(__ADSP_TETON__)
				// For TETON there are 4 external banks. We make the restriction here that if
				// a memory module is defined, then then the same applies to all banks.
				// At this point, we do not know which banks are occupied but as lomg as the 
				// EBxEBE bits are not set it makes no difference if we assign them
				adi_ebiu_set_sdram_values(1,(u32)cmd->value,&adi_ebiu_settings.sdbctl);
				adi_ebiu_set_sdram_values(2,(u32)cmd->value,&adi_ebiu_settings.sdbctl);
				adi_ebiu_set_sdram_values(3,(u32)cmd->value,&adi_ebiu_settings.sdbctl);
#endif

				break;

			case ADI_EBIU_CMD_SET_SDRAM_BANK_SIZE:
				{
					ADI_EBIU_SDRAM_BANK_VALUE *sz = (ADI_EBIU_SDRAM_BANK_VALUE*)cmd->value;
#ifdef ADI_SSL_DEBUG
#if defined(__ADSP_TETON__)
					if (sz->bankno > 3)
						return ADI_EBIU_RESULT_INVALID_SDRAM_BANK;
#endif


#if defined(__ADSP_BRAEMAR__) || defined(__ADSP_KOOKABURRA__)

					if (sz->value.size > ADI_EBIU_SDRAM_BANK_512MB)
						return ADI_EBIU_RESULT_INVALID_SDRAM_BANK_SIZE;

#else


					if (sz->value.size > ADI_EBIU_SDRAM_BANK_128MB)
						return ADI_EBIU_RESULT_INVALID_SDRAM_BANK_SIZE;
#endif
					bank_size_set++;

#endif
#if !defined(__ADSP_TETON__)
					EBIU_SDBCTL_EBSZ_SET(adi_ebiu_settings.sdbctl,sz->value.size);
#else
					*(u32*)&adi_ebiu_settings.sdbctl &= ~(3<<(8*(sz->bankno) + 1));
					*(u32*)&adi_ebiu_settings.sdbctl |=  (sz->value.size)<<(8*sz->bankno + 1);
					adi_ebiu_settings.sdbctl_enable_mask |= 1<<(8*(sz->bankno));
#endif					
				}
				break;

			case ADI_EBIU_CMD_SET_SDRAM_BANK_COL_WIDTH:
				{
					ADI_EBIU_SDRAM_BANK_VALUE *w = (ADI_EBIU_SDRAM_BANK_VALUE*)cmd->value;
#ifdef ADI_SSL_DEBUG
#if defined(__ADSP_TETON__)
					if (w->bankno > 3)
						return ADI_EBIU_RESULT_INVALID_SDRAM_BANK;
#endif
					if (w->value.width > ADI_EBIU_SDRAM_BANK_COL_11BIT)
						return ADI_EBIU_RESULT_INVALID_SDRAM_COL_WIDTH;
					bank_width_set++;
#endif
#if !defined(__ADSP_TETON__)
					EBIU_SDBCTL_EBCAW_SET(adi_ebiu_settings.sdbctl,w->value.width);
#else
					*(u32*)&adi_ebiu_settings.sdbctl &= ~(3<<(8*(w->bankno) + 4));
					*(u32*)&adi_ebiu_settings.sdbctl |=  (w->value.size)<<(8*w->bankno + 4);
#endif					
				}
				break;

			case ADI_EBIU_CMD_SET_FREQ_AS_MHZ:
				adi_ebiu_settings.MHzFactor = 1;
				break;

			case ADI_EBIU_CMD_SET_SDRAM_CL_THRESHOLD:
#ifdef ADI_SSL_DEBUG
				cas_latency_set++;
#endif
#define _FIX_FOR_TAR27618
#if defined(_FIX_FOR_TAR27618)
				adi_ebiu_settings.cl_threshold = (u32)cmd->value;
#else
				adi_ebiu_settings.cl_threshold = (u32)cmd->value*adi_ebiu_settings.MHzFactor;
#endif
				break;

			case ADI_EBIU_CMD_SET_SDRAM_TRASMIN:
#ifdef ADI_SSL_DEBUG
				trasmin_set++;
#endif
				adi_ebiu_settings.tras_min = *(ADI_EBIU_TIME*)cmd->value;
				break;

			case ADI_EBIU_CMD_SET_SDRAM_TRPMIN:
#ifdef ADI_SSL_DEBUG
				trpmin_set++;
#endif
				adi_ebiu_settings.trp_min = *(ADI_EBIU_TIME*)cmd->value;
				break;

			case ADI_EBIU_CMD_SET_SDRAM_TRCDMIN:
#ifdef ADI_SSL_DEBUG
				trcdmin_set++;
#endif
				adi_ebiu_settings.trcd_min = *(ADI_EBIU_TIME*)cmd->value;
				break;

			case ADI_EBIU_CMD_SET_SDRAM_TWRMIN:
#ifdef ADI_SSL_DEBUG
				if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->time.units<ADI_EBIU_TIMING_UNIT_NANOSEC)
					return ADI_EBIU_RESULT_INVALID_SDRAM_TWRMIN;
				twrmin_set++;
#endif
				adi_ebiu_settings.twr_min = *(ADI_EBIU_TIMING_VALUE*)cmd->value;
				break;

			case ADI_EBIU_CMD_SET_SDRAM_REFRESH:
#ifdef ADI_SSL_DEBUG
				refresh_set++;
#endif
				adi_ebiu_settings.refresh = *(ADI_EBIU_TIMING_VALUE*)cmd->value;
				break;

			case ADI_EBIU_CMD_SET_SDGCTL_REG:
				*(u32*)&adi_ebiu_settings.sdgctl = (u32)cmd->value;
				break;

			case ADI_EBIU_CMD_SET_SDBCTL_REG:
				*(u16*)&adi_ebiu_settings.sdbctl = (u32)cmd->value;
				break;

			case ADI_EBIU_CMD_SET_SDRAM_EMREN:
#ifdef ADI_SSL_DEBUG
				{
					ADI_EBIU_SDRAM_EMREN emren = (ADI_EBIU_SDRAM_EMREN)cmd->value;
					if (emren > ADI_EBIU_SDRAM_EMREN_ENABLE)
						return ADI_EBIU_RESULT_INVALID_SDRAM_EMREN;
				}
#endif
				EBIU_SDGCTL_EMREN_SET(adi_ebiu_settings.sdgctl,(ADI_EBIU_SDRAM_EMREN)cmd->value);
				break;

			case ADI_EBIU_CMD_SET_SDRAM_PASR:
#ifdef ADI_SSL_DEBUG
				{				    
					ADI_EBIU_PASR pasr = (ADI_EBIU_PASR)cmd->value;
					if (pasr > ADI_EBIU_PASR_INT0_ONLY)
						return ADI_EBIU_RESULT_INVALID_SDRAM_PASR;
				}
#endif
				EBIU_SDGCTL_PASR_SET(adi_ebiu_settings.sdgctl,(ADI_EBIU_PASR)cmd->value);
				break;

			case ADI_EBIU_CMD_SET_SDRAM_TCSR:
#ifdef ADI_SSL_DEBUG
				{
					ADI_EBIU_SDRAM_TCSR tcsr = (ADI_EBIU_SDRAM_TCSR)cmd->value;
					if (tcsr > ADI_EBIU_SDRAM_BANK_COL_11BIT)
						return ADI_EBIU_RESULT_INVALID_SDRAM_TCSR;
				}
#endif
				EBIU_SDGCTL_TCSR_SET(adi_ebiu_settings.sdgctl,(ADI_EBIU_SDRAM_TCSR)cmd->value);
				break;


/////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)
			case ADI_EBIU_CMD_SET_COREB_SUPP_INT0_IVG:
				{
#ifdef ADI_SSL_DEBUG
					u16 ivg = (u16)(u32)cmd->value;
					if (ivg < 7 || ivg > 15)
						return ADI_EBIU_RESULT_INVALID_IVG;
#endif
					adi_ebiu_settings.IVG_SuppInt0 = (u16)(u32)cmd->value;
				}
				break;

			case ADI_EBIU_CMD_SET_AUTO_SYNC_ENABLED:
				adi_ebiu_settings.auto_sync_enabled = 1;
				break;


			case  ADI_EBIU_CMD_SET_SYNC_LOCK_VARIABLE:
				adi_ebiu_SetLockVarPointer(cmd->value);
				break;

#endif
/**************************************************************************************************************/
			case ADI_EBIU_CMD_SET_SDRAM_PUPSD:         /* power up start delay                    */
			case ADI_EBIU_CMD_SET_SDRAM_PSM:           /* power up sequence                       */
			case ADI_EBIU_CMD_SET_SDRAM_EBUFE:         /* external buffering timing               */
			case ADI_EBIU_CMD_SET_SDRAM_FBBRW:         /* fast back to back R/W enable/disable    */
			case ADI_EBIU_CMD_SET_SDRAM_CDDBG:         /*   disable contro during bus grant       */
			
                {  
		    
/**************************************************************************************************************/
		    /* The above- commands will be passed on to adi_ebiu_Control */

				_adi_ebiu_ControlCalledFromInit = 1;
				adi_ebiu_Control(cmd->kind,(void*)cmd->value);
				_adi_ebiu_ControlCalledFromInit = 0;
				break;
		        }
			
  
			case ADI_EBIU_CMD_SET_ASYNCH_BANK_TRANSITION_TIME:         /* to avoid bus contention */
			case ADI_EBIU_CMD_SET_ASYNCH_BANK_READ_ACCESS_TIME:        /* Read enable time        */
			case ADI_EBIU_CMD_SET_ASYNCH_BANK_WRITE_ACCESS_TIME:       /* WRite enable time       */
			case ADI_EBIU_CMD_SET_ASYNCH_BANK_SETUP_TIME:       /*  memory cycle to R/W-enable    */
			case ADI_EBIU_CMD_SET_ASYNCH_BANK_HOLD_TIME: /* R/W enable deassert-end memory cycle  */    		    
                {  
		    
/**************************************************************************************************************/
		    /* The above- commands will be passed on to adi_ebiu_Control */

				    _adi_ebiu_ControlCalledFromInit = 1;
				    Result = adi_ebiu_Control(cmd->kind,(void*)cmd->value);
#ifdef ADI_SSL_DEBUG				    
				    if( Result != ADI_EBIU_RESULT_SUCCESS)
				        return Result;
#endif				    
				    _adi_ebiu_ControlCalledFromInit = 0;
				    break;
		        }
					 
#if defined(__ADSP_TETON__)
		    case ADI_EBIU_CMD_SET_ASYNCH_BANK_16_BIT_PACKING_ENABLE:
		        {
		            ADI_EBIU_ASYNCH_BANK_VALUE bank_value = *(ADI_EBIU_ASYNCH_BANK_VALUE *)cmd->value;

#ifdef ADI_SSL_DEBUG		            
		            if( bank_value.value.data_path > ADI_EBIU_ASYNCH_BANK_DATA_PATH_16 )
		                return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_16_BIT_PACKING_ENABLE;
		                
		            if( bank_value.bank_number > ADI_EBIU_BANK_ALL )
		                return 	ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_NUMBER;	                
#endif		                
                    /* Bits 4,5,6,7 correspond to the data path for the four banks. */
                    /* if data path is 16 bit then packing is enabled, we set the bit */
                    if( bank_value.value.data_path == ADI_EBIU_ASYNCH_BANK_DATA_PATH_16 )                   
                    {
                        if( bank_value.bank_number == ADI_EBIU_BANK_ALL ) 
                        {
                            /* set all 4 bit positions */
                            adi_ebiu_settings.amgctl |= 0x00F0;
                        }
                        else
                        {
                            /* shift the value by <4+bank_number> bit positions */
                            adi_ebiu_settings.amgctl |= ( 0x10 << bank_value.bank_number );
                        }
                    }
                    /* 32bit data path - clear the bit */
                    else
                    {
                        if( bank_value.bank_number == ADI_EBIU_BANK_ALL ) 
                        {
                            /* clear all 4 bit positions */
                            adi_ebiu_settings.amgctl &= 0xFF0F;
                        }
                        else
                        {
                            /* clear the position of the bit shifted by <4+bank_number> bit positions */
                            adi_ebiu_settings.amgctl &= ~( 0x10 << bank_value.bank_number );
                        }                        

                    }
                    
                    AMC_cmd_count++;
				    break;        
		        }
#endif


		    case ADI_EBIU_CMD_SET_ASYNCH_BANK_ARDY_ENABLE:		    
		        {
		            ADI_EBIU_ASYNCH_BANK_VALUE bank_value = *(ADI_EBIU_ASYNCH_BANK_VALUE *)cmd->value;
#ifdef ADI_SSL_DEBUG		            
		            if( bank_value.value.ardy_enable > ADI_EBIU_ASYNCH_ARDY_ENABLE )
		                return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_ARDY_ENABLE;
#endif	
    	            /* The function iuses the info in the structure to set the correct register accordingly */
		            Result = adi_ebiuSetAsynchRegsForBanks( bank_value.value.ardy_enable, bank_value.bank_number, 0x1, 0x0 );
		            if( Result != ADI_EBIU_RESULT_SUCCESS )
		                    
		                return Result;

		            break;		        
		        
		        }	
		    	   
		    case ADI_EBIU_CMD_SET_ASYNCH_BANK_ARDY_POLARITY:
		        {
		            ADI_EBIU_ASYNCH_BANK_VALUE bank_value = *(ADI_EBIU_ASYNCH_BANK_VALUE *)cmd->value;
#ifdef ADI_SSL_DEBUG		            
		            if( bank_value.value.ardy_polarity > ADI_EBIU_ASYNCH_ARDY_POLARITY_HIGH)
		                return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_ARDY_POLARITY;
#endif 
    	            /* The function iuses the info in the structure to set the correct register accordingly */
		            Result = adi_ebiuSetAsynchRegsForBanks( bank_value.value.ardy_polarity, bank_value.bank_number, 0x2, 0x1 );
		            if( Result != ADI_EBIU_RESULT_SUCCESS )
		                    
		                return Result;
		                
		            break;		        
		        
		        }		    		    
		        
		    case ADI_EBIU_CMD_SET_ASYNCH_CLKOUT_ENABLE:         /*  set CLKOUT field of the global control register */
		    
		        {  			
                    ADI_EBIU_ASYNCH_CLKOUT clkout_value = *(ADI_EBIU_ASYNCH_CLKOUT *)cmd->value;
                    
#ifdef ADI_SSL_DEBUG		        
                    if(  clkout_value > ADI_EBIU_ASYNCH_CLKOUT_ENABLE ) 
                    {
                        /* this is not a valid bank number */
                        return ADI_EBIU_RESULT_INVALID_ASYNCH_CLKOUT_ENABLE;  
                    }                
#endif                	
				    adi_ebiu_settings.amgctl = ( adi_ebiu_settings.amgctl & 0xFFFE ) | U16_REG_CAST(clkout_value);
				    AMC_cmd_count++;
				    break;
		        }		    
		    
		    case ADI_EBIU_CMD_SET_ASYNCH_BANK_ENABLE:           /* selects which banks will or will not be enabled */
		    
		        {  			
                    ADI_EBIU_ASYNCH_BANK_ENABLE bank_enable_value = *(ADI_EBIU_ASYNCH_BANK_ENABLE *)cmd->value;
                    
#ifdef ADI_SSL_DEBUG		        
                    if(  bank_enable_value > ADI_EBIU_ASYNCH_BANK0_1_2_3 ) 
                    {
                        /* this is not a valid bank number */
                        return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_ENABLE;  
                    }                
#endif                	
				    bank_enable_value <<= 1;
				    adi_ebiu_settings.amgctl = ( adi_ebiu_settings.amgctl & 0xFFF1 ) | U16_REG_CAST(bank_enable_value);
				    AMC_cmd_count++;
				    break;
		        }		    
		    

		    case ADI_EBIU_CMD_SET_ASYNCH_AMGCTL:                 /* set the global asynch control register */ 
		        {  			
				
				    U16_REG_CAST(adi_ebiu_settings.amgctl) = U16_REG_CAST(cmd->value);
				    AMC_cmd_count++;
				    break;
		        }
	
		    case ADI_EBIU_CMD_SET_ASYNCH_AMBCTL0:                /* set the asynch bank control register 0     */
		    
		        {              
				    *(u32*)&adi_ebiu_settings.ambctl0 = (u32)cmd->value;
				    AMC_cmd_count++;                    
				    break;
		        }		    
		    
		    case ADI_EBIU_CMD_SET_ASYNCH_AMBCTL1:                /* set the asynch bank control register 1     */	
		    
		        {              
				    *(u32*)&adi_ebiu_settings.ambctl1 = (u32)cmd->value;
				    AMC_cmd_count++;                    
				    break;
		        }

		}
	}

#if defined(_FIX_FOR_TAR27618)
	/* If CAS Latency Threshold is less than one million, assume it's meant to be MHz and adjust accordingly */
	if ( (adi_ebiu_settings.MHzFactor > 1) && ( adi_ebiu_settings.cl_threshold < 1000000/* 1 MHz */) )		
		adi_ebiu_settings.cl_threshold	*= adi_ebiu_settings.MHzFactor;
#endif

#ifdef ADI_SSL_DEBUG
	// check to see that all mandatory settings have been supplied
	if (use_sdram && !(bank_size_set && bank_width_set && cas_latency_set && trasmin_set && trpmin_set 
		  && trcdmin_set && twrmin_set &&  refresh_set) )
	{
		return ADI_EBIU_RESULT_FAILED;
	}
#endif


#if !defined(__ADSP_TETON__) 
	// Check status of SDRAM
	u16 isEnabled = (*pEBIU_SDBCTL&0x0001);

	if (!isEnabled) 
	{
		// If SDRAM is not already enabled, then it is imperative to apply
		// the XML values here and initiate the power up sequence. This way
		// the application will be set up in all scenarios: IDDE + XML, IDDE-XML, 
		// PROM boot.
		adi_ebiu_ApplyConfig(
					U32_REG_CAST(adi_ebiu_settings.sdgctl),
					&U16_REG_CAST(adi_ebiu_settings.sdbctl),
					U16_REG_CAST(adi_ebiu_settings.sdrrc),
					1  // Power Up
					);
					
	}
#endif

#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)

	if( (core_ident != ADI_COREA ) && (adi_ebiu_settings.auto_sync_enabled) )
	{
		// register supplemental interrupt handler function
		adi_int_CECHook(adi_ebiu_settings.IVG_SuppInt0,adi_ebiu_CoreBSuppInt0Handler,NULL,0);
		adi_int_SICSetIVG(ADI_INT_SI0,adi_ebiu_settings.IVG_SuppInt0);
		adi_int_SICWakeup(ADI_INT_SI0, 1);
		adi_int_SICEnable(ADI_INT_SI0);
	}
#endif



	/* Some Asynch Memory commands may have been processed by 'adi_ebiu_Control' but we apply the changes here,
	   after returning from 'adi_ebiu_Control', and after all the commands have been processed.  */
	 	  
	if( AMC_cmd_count != 0 )
	{
	    AMC_cmd_count = 0;
	    adi_ebiu_ApplyAMConfig( adi_ebiu_settings.amgctl, adi_ebiu_settings.ambctl0, adi_ebiu_settings.ambctl1 );
	}

	return ADI_EBIU_RESULT_SUCCESS;

}

/************************************************************************************
* Function:    adi_ebiu_AdjustSDRAM
* Description: Adjusts the SDRAM timing values and refresh rate to be optimal for the
*			   given SCLK frequency.
*************************************************************************************/


#define TRAS_MAX 	15
#define TRP_MAX 	7
#define TRCD_MAX 	7
#define TWR_MAX 	3
					
ADI_EBIU_RESULT
adi_ebiu_AdjustSDRAM(u32 fsclk2)
{
	u16 rdiv;
	u32 tras, trp, trcd, twr;
	// Here the granularity of Mhz is sufficiently accurate
	u32 fsclk = fsclk2/adi_ebiu_settings.MHzFactor;

	if ( adi_ebiu_settings.InitializedFlag==0 || !EBIU_SDGCTL_SCTLE_GET(adi_ebiu_settings.sdgctl) )
		return ADI_EBIU_RESULT_NOT_INITIALIZED;

	TIMING_CYCLES(tras,adi_ebiu_settings.tras_min,fsclk)
	if (tras > TRAS_MAX) 
    {
	    tras = TRAS_MAX;
    }
    TIMING_CYCLES_ROUNDED(trp, adi_ebiu_settings.trp_min,fsclk)
	if (trp > TRP_MAX)
    {
        trp = TRP_MAX;
    }
	TIMING_CYCLES(trcd,adi_ebiu_settings.trcd_min,fsclk)
	if (trcd > TRCD_MAX)
    {
        trcd = TRCD_MAX;
    }
     
    if( adi_ebiu_settings.twr_min.time.value == 0 )
    {
    	twr = 0;
    }
    else
    {
        TIMING_CYCLES(twr, adi_ebiu_settings.twr_min.time,fsclk)

	/* The TWR field has a max value of 3 as it is 2 bits */
        if ((adi_ebiu_settings.twr_min.cycles + twr) > TWR_MAX)
        {
            twr = TWR_MAX - adi_ebiu_settings.twr_min.cycles;
        }
    }

	// calculate and populate the register settings
	// Following line corrected so that the same units are used.
	EBIU_SDGCTL_CL_SET   ( adi_ebiu_settings.sdgctl, (fsclk2 <= adi_ebiu_settings.cl_threshold ? 2 : 3) );

	EBIU_SDGCTL_TRAS_SET ( adi_ebiu_settings.sdgctl, tras );
	EBIU_SDGCTL_TRP_SET  ( adi_ebiu_settings.sdgctl, trp  );
	EBIU_SDGCTL_TRCD_SET ( adi_ebiu_settings.sdgctl, trcd );
 
	EBIU_SDGCTL_TWR_SET ( adi_ebiu_settings.sdgctl, adi_ebiu_settings.twr_min.cycles + twr );

	if (adi_ebiu_settings.refresh.time.units==ADI_EBIU_TIMING_UNIT_MILLISEC)
	{
		u32 x = fsclk*1000*adi_ebiu_settings.refresh.time.value;
		rdiv = (u16)(x/adi_ebiu_settings.refresh.cycles);
	} 
	else 
	{
		u32 x = fsclk*adi_ebiu_settings.refresh.time.value;
		u32 a = adi_ebiu_settings.refresh.cycles*adi_ebiu_settings.refresh.time.units;
		rdiv = (u16)(x/a);
	}
	rdiv -= (tras + trp);
	if (rdiv > 0x0FFF) 
		rdiv = 0x0FFF;
	
	*(u16*)&adi_ebiu_settings.sdrrc = rdiv;

	u16 PowerUpReqd = (*pEBIU_SDSTAT & SDRS);
	if (!PowerUpReqd) 
		EBIU_SDGCTL_PSSE_SET(adi_ebiu_settings.sdgctl,0);
		
#if !defined(__ADSP_TETON__) 
		adi_ebiu_ApplyConfig(
				U32_REG_CAST(adi_ebiu_settings.sdgctl),
				&U16_REG_CAST(adi_ebiu_settings.sdbctl),
				U16_REG_CAST(adi_ebiu_settings.sdrrc),
				0/*do not enforce_powerup*/);
#else
		SYNC_COREB

		u32 sdbctl_reg = U32_REG_CAST(adi_ebiu_settings.sdbctl) | adi_ebiu_settings.sdbctl_enable_mask;
		adi_ebiu_ApplyConfig(
				U32_REG_CAST(adi_ebiu_settings.sdgctl),
				&sdbctl_reg,
				U16_REG_CAST(adi_ebiu_settings.sdrrc),
				0/*do not enforce_powerup*/);

		RELEASE_COREB
#endif
		
	return ADI_EBIU_RESULT_SUCCESS;
}

 


ADI_EBIU_RESULT 
adi_ebiu_LoadConfig(ADI_EBIU_CONFIG_HANDLE hConfig, size_t szConfig)
{

	// Not relevant if module not initialized
	// However, we will not limit it to whether SCTLE is enabled
	if ( adi_ebiu_settings.InitializedFlag==0 )
		return ADI_EBIU_RESULT_NOT_INITIALIZED;


	else 
	{
		ADI_EBIU_CONFIG *config = (ADI_EBIU_CONFIG*)hConfig;
		adi_ebiu_settings.cl_threshold = config->cl_threshold;
		adi_ebiu_settings.InitializedFlag = config->InitializedFlag;
		adi_ebiu_settings.refresh = config->refresh;
		adi_ebiu_settings.tras_min = config->tras_min;
		adi_ebiu_settings.trcd_min = config->trcd_min;
		adi_ebiu_settings.trp_min = config->trp_min;
		adi_ebiu_settings.twr_min = config->twr_min;
		adi_ebiu_settings.sdrrc  = config->sdrrc;
		adi_ebiu_settings.sdbctl = config->sdbctl;
		adi_ebiu_settings.sdgctl = config->sdgctl;

#if !defined(__ADSP_TETON__) 
		adi_ebiu_ApplyConfig(
					U32_REG_CAST(adi_ebiu_settings.sdgctl),
					(void*)&U16_REG_CAST(adi_ebiu_settings.sdbctl),
					U16_REG_CAST(adi_ebiu_settings.sdrrc),
					0);
#else
		SYNC_COREB
		
		u32 sdbctl_reg = U32_REG_CAST(adi_ebiu_settings.sdbctl) | adi_ebiu_settings.sdbctl_enable_mask;
		adi_ebiu_ApplyConfig(
					U32_REG_CAST(adi_ebiu_settings.sdgctl),
					(void*)&sdbctl_reg,
					U16_REG_CAST(adi_ebiu_settings.sdrrc),
					0);
		RELEASE_COREB
#endif
	}

	
	return ADI_EBIU_RESULT_SUCCESS;
}

/************************************************************************************
* Function:    adi_ebiu_SaveConfig
* Description: Saves the current EBIU settings to the memory location provided
*************************************************************************************/

ADI_EBIU_RESULT 
adi_ebiu_SaveConfig(ADI_EBIU_CONFIG_HANDLE hConfig, size_t szConfig)
{
	// Not relevant if module not initialized
	// However, we will not limit it to whether SCTLE is enabled
	if ( adi_ebiu_settings.InitializedFlag==0 )
		return ADI_EBIU_RESULT_NOT_INITIALIZED;

	else 
	{
		ADI_EBIU_CONFIG *config = (ADI_EBIU_CONFIG*)hConfig;

		config->sdgctl = adi_ebiu_settings.sdgctl;
		config->sdbctl = adi_ebiu_settings.sdbctl;
		config->sdrrc  = adi_ebiu_settings.sdrrc;
		config->cl_threshold = adi_ebiu_settings.cl_threshold;
		config->InitializedFlag = adi_ebiu_settings.InitializedFlag;
		config->refresh = adi_ebiu_settings.refresh;
		config->tras_min = adi_ebiu_settings.tras_min;
		config->trcd_min = adi_ebiu_settings.trcd_min;
		config->trp_min = adi_ebiu_settings.trp_min;
		config->twr_min = adi_ebiu_settings.twr_min;
	}

	return ADI_EBIU_RESULT_SUCCESS;
}



/************************************************************************************
* Function:    adi_ebiu_set_ezkit_values
* Description: Sets the EBIU module for the EZ-KIT board
*************************************************************************************/
#if !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__) && !defined(__ADSP_MOAB__)  && !defined(__ADSP_BRODIE__)   
/* This function is no longer supported for MOab and Kookaburra */  
static int
adi_ebiu_set_ezkit_values(
	ADI_EBIU_EZKIT revno,
	ADI_EBIU_SDBCTL_REG *sdbctl
)
{
	u32 mask;
	switch (revno) 
	{
#if defined(__ADSP_EDINBURGH__)
		case ADI_EBIU_EZKIT_BF533:
			// set the CAW and timing settings for the Micron MT48LC16M16A2-75 memory modules
			adi_ebiu_set_sdram_values(0,(u32)ADI_EBIU_SDRAM_MODULE_MT48LC16M16A2_75,sdbctl);
			// The bank size is the total amount of memory provided in the external bank
			EBIU_SDBCTL_EBSZ_SET ( *sdbctl, ADI_EBIU_SDRAM_BANK_32MB     );
			// set default values (as in ADSP-BF533-proc.xml)
			*(u32*)&adi_ebiu_settings.sdgctl = 0x0091998d;
			*(u16*)&adi_ebiu_settings.sdrrc = 0x01A0;
			break;
		case ADI_EBIU_EZKIT_BF533_REV1_7:
			// set the CAW and timing settings for the Micron MT48LC32M16A2-75 memory modules
			adi_ebiu_set_sdram_values(0,(u32)ADI_EBIU_SDRAM_MODULE_MT48LC32M16A2_75,sdbctl);
			// The bank size is the total amount of memory provided in the external bank
			EBIU_SDBCTL_EBSZ_SET ( *sdbctl, ADI_EBIU_SDRAM_BANK_64MB     );
			// set default values (as in ADSP-BF533-proc.xml)
			*(u32*)&adi_ebiu_settings.sdgctl = 0x0091998D;
			*(u16*)&adi_ebiu_settings.sdrrc = 0x0397;
			break;
#endif
#if defined(__ADSP_BRAEMAR__)
		case ADI_EBIU_EZKIT_BF537:
			// set the CAW and timing settings for the Micron MT48LC32M8A2-75 memory modules
			adi_ebiu_set_sdram_values(0,(u32)ADI_EBIU_SDRAM_MODULE_MT48LC32M8A2_75,sdbctl);
			// The bank size is the total amount of memory provided in the external bank
			EBIU_SDBCTL_EBSZ_SET ( *sdbctl, ADI_EBIU_SDRAM_BANK_64MB      );
			// set default values (as in ADSP-BF537-proc.xml)
			*(u32*)&adi_ebiu_settings.sdgctl = 0x0091998d;
			*(u16*)&adi_ebiu_settings.sdrrc = 0x03A0;
			break;
#endif

#if defined(__ADSP_TETON__)
		case ADI_EBIU_EZKIT_BF561:
			// Only one external bank of 64MB is used on the ADSP-BF651 EZ-Kit
			*(u32*)&adi_ebiu_settings.sdbctl = 0;
			// set the CAW and timing settings for the Micron MT48LC16M16A2-75 memory modules
			adi_ebiu_set_sdram_values(0,(u32)ADI_EBIU_SDRAM_MODULE_MT48LC16M16A2_75,sdbctl);
			// set total size of bank to 64MB
			*(u32*)&adi_ebiu_settings.sdbctl &= ~(3<<1);
			*(u32*)&adi_ebiu_settings.sdbctl |= (ADI_EBIU_SDRAM_BANK_64MB<<1);  
			adi_ebiu_settings.sdbctl_enable_mask = 1;
			// set default values (as in ADSP-BF561-proc.xml)
			*(u32*)&adi_ebiu_settings.sdgctl = 0x0091998d;
			*(u16*)&adi_ebiu_settings.sdrrc = 0x01D0;
			break;
#endif


#if defined(__ADSP_STIRLING__)

		case ADI_EBIU_EZKIT_BF538:
			// set the CAW and timing settings for the Micron MT48LC16M16A2-75 memory modules
			adi_ebiu_set_sdram_values(0,(u32)ADI_EBIU_SDRAM_MODULE_MT48LC16M16A2_75,sdbctl);
			// The bank size is the total amount of memory provided in the external bank
			
			
	adi_ebiu_set_sdram_values(0,(u32)ADI_EBIU_SDRAM_MODULE_MT48LC32M8A2_75,sdbctl);
			// The bank size is the total amount of memory provided in the external bank
			EBIU_SDBCTL_EBSZ_SET ( *sdbctl, ADI_EBIU_SDRAM_BANK_64MB ); 	
			// set default values 
			*(u32*)&adi_ebiu_settings.sdgctl = 0x0091998D;
			*(u16*)&adi_ebiu_settings.sdrrc = 0x03A0;
			break;

#endif



		default:
			return 0;

	}
	return 1;
}
#endif

/************************************************************************************
* Function:    adi_ebiu_set_sdram_values
* Description: Sets the EBIU module for Micron SDRAM modules
*************************************************************************************/

static int
adi_ebiu_set_sdram_values(
	u32 bankno,
	u32 module_id,
	ADI_EBIU_SDBCTL_REG *sdbctl
)
{
	ADI_EBIU_SDRAM_BANK_COL_WIDTH caw = ADI_EBIU_SDRAM_GET_CAW(module_id);
	ADI_EBIU_SDRAM_MODULE_SPEED_GRADE sg = ADI_EBIU_SDRAM_GET_SPEED_GRADE(module_id);
	ADI_EBIU_SDRAM_REFRESH_CYCLES refresh_cycles = ADI_EBIU_SDRAM_GET_REFRESH_CYCLES(module_id);

#ifdef ADI_SSL_DEBUG
	if (caw > ADI_EBIU_SDRAM_BANK_COL_11BIT)
		return 0;
	if (sg > ADI_EBIU_SDRAM_MODULE_8E)
		return 0;
	if (refresh_cycles > ADI_EBIU_SDRAM_REFRESH_CYCLES_8K)
		return 0;
#endif
#if !defined(__ADSP_TETON__)

	EBIU_SDBCTL_EBCAW_SET( *sdbctl, caw );

#else

	*(u32*)sdbctl &= ~(3<<(bankno*8 + 4));
	*(u32*)sdbctl |= caw<<(bankno*8 + 4);

	// the following is only set up for Bank 0 - 
	if (bankno)
		return 1;
#endif



	switch(sg)
	{
		case ADI_EBIU_SDRAM_MODULE_6:
			adi_ebiu_settings.cl_threshold			= 0;  // ==> CL=3 always
			adi_ebiu_settings.tras_min.value		= 42;
			adi_ebiu_settings.tras_min.units		= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.trp_min.value			= 18;
			adi_ebiu_settings.trp_min.units			= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.trcd_min.value		= 18;
			adi_ebiu_settings.trcd_min.units		= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.twr_min.cycles		= 1;
			adi_ebiu_settings.twr_min.time.value	= 6000;
			adi_ebiu_settings.twr_min.time.units	= ADI_EBIU_TIMING_UNIT_PICOSEC;
			adi_ebiu_settings.refresh.cycles		= (4<<refresh_cycles)*1024;
			adi_ebiu_settings.refresh.time.value	= 64;
			adi_ebiu_settings.refresh.time.units	= ADI_EBIU_TIMING_UNIT_MILLISEC;
			break;
		case ADI_EBIU_SDRAM_MODULE_6A:
			adi_ebiu_settings.cl_threshold			= 0;  // ==> CL=3 always
			adi_ebiu_settings.tras_min.value		= 42;
			adi_ebiu_settings.tras_min.units		= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.trp_min.value			= 18;
			adi_ebiu_settings.trp_min.units			= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.trcd_min.value		= 18;
			adi_ebiu_settings.trcd_min.units		= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.twr_min.cycles		= 1;
			adi_ebiu_settings.twr_min.time.value	= 7000;
			adi_ebiu_settings.twr_min.time.units	= ADI_EBIU_TIMING_UNIT_PICOSEC;
			adi_ebiu_settings.refresh.cycles		= (4<<refresh_cycles)*1024;
			adi_ebiu_settings.refresh.time.value	= 64;
			adi_ebiu_settings.refresh.time.units	= ADI_EBIU_TIMING_UNIT_MILLISEC;
			break;
		case ADI_EBIU_SDRAM_MODULE_7E:
			adi_ebiu_settings.cl_threshold			= 0;  // ==> CL=3 always
			adi_ebiu_settings.tras_min.value		= 37;
			adi_ebiu_settings.tras_min.units		= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.trp_min.value			= 15;
			adi_ebiu_settings.trp_min.units			= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.trcd_min.value		= 15;
			adi_ebiu_settings.trcd_min.units		= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.twr_min.cycles		= 1;
			adi_ebiu_settings.twr_min.time.value	= 7000;
			adi_ebiu_settings.twr_min.time.units	= ADI_EBIU_TIMING_UNIT_PICOSEC;
			adi_ebiu_settings.refresh.cycles		= (4<<refresh_cycles)*1024;
			adi_ebiu_settings.refresh.time.value	= 64;
			adi_ebiu_settings.refresh.time.units	= ADI_EBIU_TIMING_UNIT_MILLISEC;
			break;
		case ADI_EBIU_SDRAM_MODULE_75:
			adi_ebiu_settings.cl_threshold			= 100*adi_ebiu_settings.MHzFactor;
			adi_ebiu_settings.tras_min.value		= 44;
			adi_ebiu_settings.tras_min.units		= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.trp_min.value			= 20;
			adi_ebiu_settings.trp_min.units			= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.trcd_min.value		= 20;
			adi_ebiu_settings.trcd_min.units		= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.twr_min.cycles		= 1;
			adi_ebiu_settings.twr_min.time.value	= 7500;
			adi_ebiu_settings.twr_min.time.units	= ADI_EBIU_TIMING_UNIT_PICOSEC;
			adi_ebiu_settings.refresh.cycles		= (4<<refresh_cycles)*1024;
			adi_ebiu_settings.refresh.time.value	= 64;
			adi_ebiu_settings.refresh.time.units	= ADI_EBIU_TIMING_UNIT_MILLISEC;
			break;
		case ADI_EBIU_SDRAM_MODULE_8E:
			adi_ebiu_settings.cl_threshold			= 100*adi_ebiu_settings.MHzFactor;  
			adi_ebiu_settings.tras_min.value		= 50;
			adi_ebiu_settings.tras_min.units		= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.trp_min.value			= 20;
			adi_ebiu_settings.trp_min.units			= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.trcd_min.value		= 20;
			adi_ebiu_settings.trcd_min.units		= ADI_EBIU_TIMING_UNIT_NANOSEC;
			adi_ebiu_settings.twr_min.cycles		= 1;
			adi_ebiu_settings.twr_min.time.value	= 7000;
			adi_ebiu_settings.twr_min.time.units	= ADI_EBIU_TIMING_UNIT_PICOSEC;
			adi_ebiu_settings.refresh.cycles		= (4<<refresh_cycles)*1024;
			adi_ebiu_settings.refresh.time.value	= 64;
			adi_ebiu_settings.refresh.time.units	= ADI_EBIU_TIMING_UNIT_MILLISEC;
			break;
	}
	return 1;
}

/************************************************************************************

* Function:    adi_ebiu_Terminate

* Description: Resets the initialized flag, and unhooks supplemental interrupt (BF561)

*************************************************************************************/

ADI_EBIU_RESULT
adi_ebiu_Terminate(void)
{
	adi_ebiu_settings.InitializedFlag = 0;

#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)
	if( core_ident != ADI_COREA && adi_ebiu_settings.auto_sync_enabled )
	{
		// deregister supplemental interrupt handler function
		adi_int_SICDisable(ADI_INT_SI0);
		adi_int_CECUnhook(adi_ebiu_settings.IVG_SuppInt0,adi_ebiu_CoreBSuppInt0Handler, NULL);
	}
#endif
	return ADI_EBIU_RESULT_SUCCESS;
}

#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)

extern void adi_ebiu_SyncCoreA(void);

static void
adi_ebiu_SetLockVarPointer(
		void *pLockVar		// pointer to lock variable in L2 shared memory 
		)
{
	adi_ebiu_settings.pLockVar = (testset_t*)pLockVar;
}


/************************************************************************************
* Function:    adi_ebiu_CoreBSuppInt1Handler
* Description: Handles the SuppInt1 interrupt raised by Core A to bring core B to 
*              a safe state before updating the EBIU settings
*************************************************************************************/
ADI_INT_HANDLER( adi_ebiu_CoreBSuppInt0Handler )
{
	// Test for Interrupt
	// This is required as we are in an Interrupt Manager handler which
	// could've been chained.
	if ( (*pSICB_SYSCR&0x0080) != 0x0080)
		return ADI_INT_RESULT_NOT_PROCESSED;

	// test for correct command
//	if  ( (adi_sync_GetSemaphore() & ADI_SYNC_EBIU_ONLY ) != ADI_SYNC_EBIU_ONLY) 
//		return ADI_INT_RESULT_NOT_PROCESSED;


	// critical region (L1 ASM routine);
	adi_ebiu_SyncCoreA();


	return ADI_INT_RESULT_PROCESSED;

}

#endif

/**********************************************************************************

 The following section is for EBIUs which support DDR memory 

**********************************************************************************/
#else /* DDR */


 
/********************************************************************************

  DDR Memory Control Registers are defined in 'adi_ebiu_module.h'
    
*********************************************************************************/ 

static int adi_ebiu_set_ddr_values( u32 module_id, ADI_EBIU_DDRCTL0_REG *,  ADI_EBIU_DDRCTL1_REG *,  ADI_EBIU_DDRCTL2_REG * );


/************************************************************************************
* External reference to assembly routine adi_ebiu_ApplyDDRConfig - in adi_ebiu_asm.asm   
*************************************************************************************/


void adi_ebiu_ApplyDDRConfig( 
	u32 ddrctl0, u32 ddrctl1, u32 ddrctl2
	 );

static u32 ChipID;
	 			
/************************************************************************************
* Function:    adi_ebiu_Init
* Description:  
*************************************************************************************/

ADI_EBIU_RESULT adi_ebiu_Init(
		const ADI_EBIU_COMMAND_PAIR *table, 
		const u16 Reserved
)
{	
	
    ADI_EBIU_COMMAND_PAIR *cmd = (ADI_EBIU_COMMAND_PAIR *)table;
	
    u32 fcclk,fsclk,fvco;
    ADI_EBIU_RESULT Result;

    /* If the EBIU has already been initialized, ignore this call, otherwise, set the flag. */	
    if (adi_ebiu_SetInitFlag())
        return ADI_EBIU_RESULT_ALREADY_INITIALIZED;
        
    /* read the current contents of this register as it contains correction values */
    *(u32*)&adi_ebiu_settings.ddrctl4 = *pEBIU_RSTCTL;

    /* read the Chip ID to determine whether this is a mobile DDR ("M") part or standard DDR. */          
    ChipID = *pADI_EBIU_CHIPID;
	if( ChipID == MOBILE_DDR_CHIPID  )
	{
	    /* mobile DDR part - Set the mobile DDR bit in case a SW reset had cleared it */	
        EBIU_DDRCTL4_MOBILE_DDR_ENABLE_SET(adi_ebiu_settings.ddrctl4,(u32)1);
        
        *(u32*)&adi_ebiu_settings.ddrctl2 = ADI_EBIU_DDRCTL2M_XML_RESET;
        *(u32*)&adi_ebiu_settings.ddrctl3 = ADI_EBIU_DDRCTL3M_XML_RESET;        
    }
    else
    {      
	    /* Not mobile DDR - clear the mobile DDR bit just to make sure it is correct */	
        EBIU_DDRCTL4_MOBILE_DDR_ENABLE_SET(adi_ebiu_settings.ddrctl4,(u32)0);

        *(u32*)&adi_ebiu_settings.ddrctl2 = ADI_EBIU_DDRCTL2_XML_RESET;
        *(u32*)&adi_ebiu_settings.ddrctl3 = ADI_EBIU_DDRCTL3_XML_RESET;        
    }        
                    
    /* set the other two DDR control registers (which do not depend on mobile status) to XML reset values */
    *(u32*)&adi_ebiu_settings.ddrctl0 = ADI_EBIU_DDRCTL0_XML_RESET;
    *(u32*)&adi_ebiu_settings.ddrctl1 = ADI_EBIU_DDRCTL1_XML_RESET;

    /* update the value of the mobile DDR bit so it coincides with the Chip ID */
    *pEBIU_RSTCTL = *(u32*)&adi_ebiu_settings.ddrctl4; 
     
    
    adi_ebiu_settings.MHzFactor	= 1000000;

	// set Asynch cached registers to reset values
	*(u16*)&adi_ebiu_settings.amgctl = ADI_EBIU_AMGCTL_RESET;
	*(u32*)&adi_ebiu_settings.ambctl0 = ADI_EBIU_AMBCTL0_RESET;
	*(u32*)&adi_ebiu_settings.ambctl1 = ADI_EBIU_AMBCTL1_RESET;
	
		
#ifdef ADI_SSL_DEBUG
    u8 data_width_set = 0;
    u8 device_width_set = 0;
    u8 device_size_set = 0;
    u8 external_banks_set = 0;
    u8 cas_set = 0;
    u8 ras_min_set = 0;
    u8 rp_min_set = 0;
    u8 rcd_min_set = 0;
    u8 wr_min_set = 0;
    u8 rfc_min_set = 0;
    u8 rc_min_set = 0;
    u8 mrd_min_set = 0;
    u8 refi_set = 0;	
    u8 wtr_min_set = 0;	
	

    /* Make sure that the table is not empty */
    if (table == NULL ) 
        return ADI_EBIU_RESULT_FAILED;
 
#endif

    AMC_cmd_count = 0;
    

    /* parse the command pair table */
    for ( ; cmd->kind != ADI_EBIU_CMD_END ; cmd++)
    {
    	        	
        switch (cmd->kind)
        {
            default:
                return ADI_EBIU_RESULT_BAD_COMMAND;

                
            case ADI_EBIU_CMD_SET_FREQ_AS_MHZ:
                {
                    adi_ebiu_settings.MHzFactor = 1;
                    break;
                }
				
            /* sets the timings etc for a given (Micron) DDR memory */
            case ADI_EBIU_CMD_SET_DDR_MODULE: 
                {
                	
#ifndef ADI_SSL_DEBUG

                    adi_ebiu_set_ddr_values((u32)cmd->value, 
                        &adi_ebiu_settings.ddrctl0, &adi_ebiu_settings.ddrctl1,  &adi_ebiu_settings.ddrctl2 );		

			
 				
#else                /* we are in debug config so check the return values */
 
		            if( !adi_ebiu_set_ddr_values((u32)cmd->value, 
                        &adi_ebiu_settings.ddrctl0, &adi_ebiu_settings.ddrctl1,  &adi_ebiu_settings.ddrctl2 ) )
                            return ADI_EBIU_RESULT_INVALID_DDR_MODULE;  
 
                    		
                    data_width_set++;
                    device_width_set++;
                    device_size_set++;
                    external_banks_set++;
                    cas_set++;
                    ras_min_set++;
                    rp_min_set++;
                    rcd_min_set++;
                    wr_min_set++;
                    rfc_min_set++;
                    rc_min_set++;
                    mrd_min_set++;
                    refi_set++;	
                    wtr_min_set++;	

  
                
#endif  /* debug */             
                    break;	
                }
                
            case ADI_EBIU_CMD_SET_DDRCTL0:
                {
                	
#ifdef ADI_SSL_DEBUG             
                    refi_set++;
                    rfc_min_set++;                    						
                    rp_min_set++;
                    rc_min_set++;                    
                    ras_min_set++;                     
#endif                	
                    *(u32*)&adi_ebiu_settings.ddrctl0 = (u32)cmd->value;
                    break;
                }	
                
            case ADI_EBIU_CMD_SET_DDRCTL1:
                {
#ifdef ADI_SSL_DEBUG             
                    rcd_min_set++;
                    mrd_min_set++;                    						
                    wr_min_set++;
                    data_width_set++;                    
                    external_banks_set++;
                    device_width_set++;                    
                    device_size_set++; 
                    wtr_min_set++;                    
                                                                              
#endif                   	
                    *(u32*)&adi_ebiu_settings.ddrctl1 = (u32)cmd->value;
                    break;
                }
            case ADI_EBIU_CMD_SET_DDRCTL2:
                {
#ifdef ADI_SSL_DEBUG                 	
                    cas_set++;                    
#endif                	
                    *(u32*)&adi_ebiu_settings.ddrctl2 = (u32)cmd->value;
                    break;
                }

  
            /* set RAS delay - cycles between ACTIVE and PRECHARGE command (default 0x06)  */
            case ADI_EBIU_CMD_SET_DDR_RAS:
                {

#ifdef ADI_SSL_DEBUG               
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->time.units<ADI_EBIU_TIMING_UNIT_NANOSEC)
					    return ADI_EBIU_RESULT_INVALID_DDR_RAS;
					    
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->cycles>0xf )
					    return ADI_EBIU_RESULT_INVALID_DDR_RAS;	
					    					
                    ras_min_set++;	                           			
#endif
                    adi_ebiu_settings.ras_min = *(ADI_EBIU_TIMING_VALUE*)cmd->value ;
                     	                                 
                    break;		
                }

                
            /* RP = 4 bits - PRECHARGE_to_ACTIVE time (default 0x03) */
            /* Clock cycles needed for SDRAM to recover from a PRECHARGE command */
            /* and be ready to accept next ACTIVE command. */
                		
            /* set precharge command period */
            case ADI_EBIU_CMD_SET_DDR_RP:
                {
#ifdef ADI_SSL_DEBUG     
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->time.units<ADI_EBIU_TIMING_UNIT_NANOSEC)
					    return ADI_EBIU_RESULT_INVALID_DDR_RP;
					    
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->cycles>0xf )
					    return ADI_EBIU_RESULT_INVALID_DDR_RP; 
               
                    rp_min_set++;  						
#endif
				    adi_ebiu_settings.rp_min = *(ADI_EBIU_TIMING_VALUE*)cmd->value;
                    	                                 
                    break;		
                }
		
            /* set active to active delay */
            case ADI_EBIU_CMD_SET_DDR_RC:
                {
#ifdef ADI_SSL_DEBUG  
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->time.units<ADI_EBIU_TIMING_UNIT_NANOSEC)
					    return ADI_EBIU_RESULT_INVALID_DDR_RC;
					    
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->cycles>0xf )
					    return ADI_EBIU_RESULT_INVALID_DDR_RC; 
                                 
                    rc_min_set++;  						
#endif
				    adi_ebiu_settings.rc_min = *(ADI_EBIU_TIMING_VALUE*)cmd->value;
                    break;		
                }
               
            /* set delay between auto-refresh command and active command */
            case ADI_EBIU_CMD_SET_DDR_RFC:
                {
#ifdef ADI_SSL_DEBUG    
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->time.units<ADI_EBIU_TIMING_UNIT_NANOSEC)
					    return ADI_EBIU_RESULT_INVALID_DDR_RFC;
					
		            if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->cycles > 0xf )
					    return ADI_EBIU_RESULT_INVALID_DDR_RFC; 
                               					    
                               
                    rfc_min_set++;		
#endif
				    adi_ebiu_settings.rfc_min = *(ADI_EBIU_TIMING_VALUE*)cmd->value; 
                    break;		
                }

            /* set average refresh interval (from one refresh command to next) */
            case ADI_EBIU_CMD_SET_DDR_REFI:
                {
#ifdef ADI_SSL_DEBUG      
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->time.units<ADI_EBIU_TIMING_UNIT_NANOSEC)
					    return ADI_EBIU_RESULT_INVALID_DDR_REFI;
					    
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->cycles>16383 )
					    return ADI_EBIU_RESULT_INVALID_DDR_REFI; 
                             
                    refi_set++;  						
#endif
				    adi_ebiu_settings.refi = *(ADI_EBIU_TIMING_VALUE*)cmd->value; 

                    break;		
                }
		                                								
                
            /* set active to read/write delay */
            case ADI_EBIU_CMD_SET_DDR_RCD:
                {
#ifdef ADI_SSL_DEBUG   
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->time.units<ADI_EBIU_TIMING_UNIT_NANOSEC)
					    return ADI_EBIU_RESULT_INVALID_DDR_RCD;
					    
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->cycles > 0xf )
					    return ADI_EBIU_RESULT_INVALID_DDR_RCD; 
                                
                    rcd_min_set++;  						
#endif
				    adi_ebiu_settings.rcd_min = *(ADI_EBIU_TIMING_VALUE*)cmd->value;
                    break;		
                }
  

           /* set write-to-read (cycles between last write-data and the next read) */
            case ADI_EBIU_CMD_SET_DDR_WTR:
                {
#ifdef ADI_SSL_DEBUG        
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->time.units<ADI_EBIU_TIMING_UNIT_NANOSEC)
					    return ADI_EBIU_RESULT_INVALID_DDR_WTR;
					    
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->cycles > 0xf )
					    return ADI_EBIU_RESULT_INVALID_DDR_WTR; 
                           
                    wtr_min_set++;  						
#endif
				    adi_ebiu_settings.wtr_min = *(ADI_EBIU_TIMING_VALUE*)cmd->value;
                    break;		
                }

 
            /* set write-recovery-time (cycles to recover from write before next precharge) */
            case ADI_EBIU_CMD_SET_DDR_WR:
                {
#ifdef ADI_SSL_DEBUG       
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->time.units<ADI_EBIU_TIMING_UNIT_NANOSEC)
					    return ADI_EBIU_RESULT_INVALID_DDR_WR;
					    
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->cycles > 3 )
					    return ADI_EBIU_RESULT_INVALID_DDR_WR; 
                            
                    wr_min_set++;  						
#endif
				    adi_ebiu_settings.wr_min = *(ADI_EBIU_TIMING_VALUE*)cmd->value;
                    break;		
                }


            /* set  load-mode-register cycle time */
            case ADI_EBIU_CMD_SET_DDR_MRD:
                {
#ifdef ADI_SSL_DEBUG       
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->time.units<ADI_EBIU_TIMING_UNIT_NANOSEC)
					    return ADI_EBIU_RESULT_INVALID_DDR_MRD;
					    
				    if ( ((ADI_EBIU_TIMING_VALUE*)cmd->value)->cycles > 0xf )
					    return ADI_EBIU_RESULT_INVALID_DDR_MRD; 
                            
                    mrd_min_set++;
#endif
				    adi_ebiu_settings.mrd_min = *(ADI_EBIU_TIMING_VALUE*)cmd->value;
                    break;		
                }

						
            /* size of device */		
            case ADI_EBIU_CMD_SET_DDR_DEVICE_SIZE:     
                {
#ifdef ADI_SSL_DEBUG    
					    
				    if ( cmd->value > (void*)0x3 )
					    return ADI_EBIU_RESULT_INVALID_DDR_DEVICE_SIZE; 
                               
                    device_size_set++;
#endif
        
                    EBIU_DDRCTL1_DEVICE_SIZE_SET(adi_ebiu_settings.ddrctl1,(u32)cmd->value);          	
                    break;		
                }

            
            /* width of device */ 
            case ADI_EBIU_CMD_SET_DDR_DEVICE_WIDTH:       
                {
#ifdef ADI_SSL_DEBUG        
				    if ( cmd->value > (void*)0x3 )
					    return ADI_EBIU_RESULT_INVALID_DDR_DEVICE_WIDTH; 
            
                    device_width_set++;
#endif
                    EBIU_DDRCTL1_DEVICE_WIDTH_SET(adi_ebiu_settings.ddrctl1,(u32)cmd->value);          	
                    break;		
                }

            
            /* set number of external banks */    
            case ADI_EBIU_CMD_SET_DDR_EXTERNAL_BANKS:   
                {
#ifdef ADI_SSL_DEBUG  
				    if ( cmd->value > (void*)0x3 )
					    return ADI_EBIU_RESULT_INVALID_DDR_EXTERNAL_BANKS; 
                  
                    external_banks_set++;
#endif
                    EBIU_DDRCTL1_EXTERNAL_BANKS_SET(adi_ebiu_settings.ddrctl1,(u32)cmd->value);          	
                    break;		
                }
            
             /* set data width */             
            case ADI_EBIU_CMD_SET_DDR_DATA_WIDTH:       
                {
#ifdef ADI_SSL_DEBUG                 
				    if ( cmd->value > (void*)0x3 )
					    return ADI_EBIU_RESULT_INVALID_DDR_DATA_WIDTH; 
               
                    data_width_set++;
#endif
                    EBIU_DDRCTL1_DATA_WIDTH_SET(adi_ebiu_settings.ddrctl1,(u32)cmd->value);          	
                    break;		
                }

            /* set memory drive strength */
            case ADI_EBIU_CMD_SET_DDR_DS:
                {
				    /* If in mobile DDR mode */
					    
                    ChipID = *pADI_EBIU_CHIPID;
	                if( ChipID == MOBILE_DDR_CHIPID  )			    				    
				   // if( EBIU_DDRCTL4_MOBILE_DDR_ENABLE_GET (adi_ebiu_settings.ddrctl4) == 1)
				    {
				             
#ifdef ADI_SSL_DEBUG            
				        if ( cmd->value > (void*)ADI_EBIU_DDR_DS_8 )
					        return ADI_EBIU_RESULT_INVALID_DDR_DS; 
#endif					    
                        EBIU_DDRCTL3_DS_SET(adi_ebiu_settings.ddrctl3.ctlreg3_mobile,(u32)cmd->value);          	

				    }
				    else
				    {
				        /* non-mobile DDR */
#ifdef ADI_SSL_DEBUG 				        
           
				        if ( cmd->value > (void*) ADI_EBIU_DDR_DS_REDUCED )
					        return ADI_EBIU_RESULT_INVALID_DDR_DS; 
#endif					    

					    *pEBIU_DDRCTL3 = U16_REG_CAST(adi_ebiu_settings.ddrctl3.ctlreg3_regular);
                        break;	           
                
				    }                
                    break;		
                }
            
           case ADI_EBIU_CMD_SET_DDR_PASR:
           
#ifdef ADI_SSL_DEBUG      
                    ChipID = *pADI_EBIU_CHIPID;
                    
                    /* invalid command if not MOBILE DDR */
	                if( ChipID != MOBILE_DDR_CHIPID  )
					    return ADI_EBIU_RESULT_BAD_COMMAND; 				    
           
				    if ( cmd->value > (void*)ADI_EBIU_DDR_PASR_16 )
					    return ADI_EBIU_RESULT_INVALID_DDR_PASR; 
#endif            
                    EBIU_DDRCTL3_PASR_SET(adi_ebiu_settings.ddrctl3.ctlreg3_mobile,(u32)cmd->value);          	
					*pEBIU_DDRCTL3 = U16_REG_CAST(adi_ebiu_settings.ddrctl3.ctlreg3_mobile);
                    break;	           
                
            /* cycles from read/write signal until first valid data */ 
            /* 101 = 1.5 cycles; 010 = 2 cycles; 110 = 2.5 cycles; 011= 3 cycles */       
            case ADI_EBIU_CMD_SET_DDR_CAS:            
                {
#ifdef ADI_SSL_DEBUG                    
					    
				    if ( cmd->value > (void*)7 )
					    return ADI_EBIU_RESULT_INVALID_DDR_CAS; 

                    cas_set++;
#endif
				    adi_ebiu_settings.cas = (u32)cmd->value;
                     	
                    break;		
                }

                    
            /* issue a soft reset to DDR controller*/				        
            case ADI_EBIU_CMD_SET_DDR_SOFT_RESET: 
                {
                                          
                    EBIU_DDRCTL4_SOFT_RESET_SET(adi_ebiu_settings.ddrctl4,(u32)1);
                    break;		
                }
                
 
            /* reset and enable mobile or regular mode */				        
            case ADI_EBIU_CMD_MOBILE_DDR_ENABLE: 
                {   
                    
#ifdef ADI_SSL_DEBUG                      
                    ChipID = *pADI_EBIU_CHIPID;
                    
                    /* invalid command if not MOBILE DDR */
	                if( ChipID != MOBILE_DDR_CHIPID  )
					    return ADI_EBIU_RESULT_BAD_COMMAND;                     
                    
#endif                    
                                            	
                    EBIU_DDRCTL4_MOBILE_DDR_ENABLE_SET(adi_ebiu_settings.ddrctl4,(u32)1);
                    break;		
                }		

			case ADI_EBIU_CMD_SET_ASYNCH_BANK_TRANSITION_TIME:         /* to avoid bus contention */
			case ADI_EBIU_CMD_SET_ASYNCH_BANK_READ_ACCESS_TIME:        /* Read enable time        */
			case ADI_EBIU_CMD_SET_ASYNCH_BANK_WRITE_ACCESS_TIME:       /* WRite enable time       */
			case ADI_EBIU_CMD_SET_ASYNCH_BANK_SETUP_TIME:       /*  memory cycle to R/W-enable    */
			case ADI_EBIU_CMD_SET_ASYNCH_BANK_HOLD_TIME: /* R/W enable deassert-end memory cycle  */    		    
                {  
		    
/**************************************************************************************************************/
		    /* The above- commands will be passed on to adi_ebiu_Control */

				    _adi_ebiu_ControlCalledFromInit = 1;
				    Result = adi_ebiu_Control(cmd->kind,(void*)cmd->value);
#ifdef ADI_SSL_DEBUG				    
				    if( Result != ADI_EBIU_RESULT_SUCCESS)
				        return Result;
#endif				    
				    _adi_ebiu_ControlCalledFromInit = 0;
				    break;
		        }


		    case ADI_EBIU_CMD_SET_ASYNCH_BANK_ARDY_ENABLE:		    
		        {
		            ADI_EBIU_ASYNCH_BANK_VALUE bank_value = *(ADI_EBIU_ASYNCH_BANK_VALUE *)cmd->value;
#ifdef ADI_SSL_DEBUG		            
		            if( bank_value.value.ardy_enable > ADI_EBIU_ASYNCH_ARDY_ENABLE )
		                return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_ARDY_ENABLE;
#endif		            

    	            /* The function iuses the info in the structure to set the correct register accordingly */
		            Result = adi_ebiuSetAsynchRegsForBanks( bank_value.value.ardy_enable, bank_value.bank_number, 0x1, 0x0 );
		            if( Result != ADI_EBIU_RESULT_SUCCESS )
		                    
		                return Result;

		            break;		        
		        
		        }	
		    	   
		    case ADI_EBIU_CMD_SET_ASYNCH_BANK_ARDY_POLARITY:
		        {
		            ADI_EBIU_ASYNCH_BANK_VALUE bank_value = *(ADI_EBIU_ASYNCH_BANK_VALUE *)cmd->value;
#ifdef ADI_SSL_DEBUG		            
		            if( bank_value.value.ardy_polarity > ADI_EBIU_ASYNCH_ARDY_POLARITY_HIGH)
		                return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_ARDY_POLARITY;
#endif  
    	            /* The function iuses the info in the structure to set the correct register accordingly */
		            Result = adi_ebiuSetAsynchRegsForBanks( bank_value.value.ardy_polarity, bank_value.bank_number, 0x2, 0x1 );
		            if( Result != ADI_EBIU_RESULT_SUCCESS )
		                    
		                return Result;
		                
		            break;		        
		        
		        }		    
		        
		    case ADI_EBIU_CMD_SET_ASYNCH_CLKOUT_ENABLE:         /*  set CLKOUT field of the global control register */
		    
		        {  			
                    ADI_EBIU_ASYNCH_CLKOUT clkout_value = *(ADI_EBIU_ASYNCH_CLKOUT *)cmd->value;
                    
#ifdef ADI_SSL_DEBUG		        
                    if(  clkout_value > ADI_EBIU_ASYNCH_CLKOUT_ENABLE ) 
                    {
                        /* this is not a valid bank number */
                        return ADI_EBIU_RESULT_INVALID_ASYNCH_CLKOUT_ENABLE;  
                    }                
#endif                	
				    adi_ebiu_settings.amgctl = ( adi_ebiu_settings.amgctl & 0xFFFE ) | U16_REG_CAST(clkout_value);
				    AMC_cmd_count++;
				    break;
		        }		    
		    
		    case ADI_EBIU_CMD_SET_ASYNCH_BANK_ENABLE:           /* selects which banks will or will not be enabled */
		    
		        {  			
                    ADI_EBIU_ASYNCH_BANK_ENABLE bank_enable_value = *(ADI_EBIU_ASYNCH_BANK_ENABLE *)cmd->value;
#ifdef ADI_SSL_DEBUG		        
                    if(  bank_enable_value > ADI_EBIU_ASYNCH_BANK0_1_2_3 ) 
                    {
                        /* this is not a valid bank number */
                        return ADI_EBIU_RESULT_INVALID_ASYNCH_BANK_ENABLE;  
                    }                
#endif                	
				    bank_enable_value <<= 1;
				    adi_ebiu_settings.amgctl = ( adi_ebiu_settings.amgctl & 0xFFF1 ) | U16_REG_CAST(bank_enable_value);
				    AMC_cmd_count++;
				    break;
		        }	


            case ADI_EBIU_CMD_SET_ASYNCH_AMGCTL:                 /* set the global asynch control register */ 
                {  
				    U16_REG_CAST(adi_ebiu_settings.amgctl) = U16_REG_CAST(cmd->value);
			        AMC_cmd_count++;
			        break;
                }
	
            case ADI_EBIU_CMD_SET_ASYNCH_AMBCTL0:                /* set the asynch bank control register 0     */
		    
                {              
			        *(u32*)&adi_ebiu_settings.ambctl0 = (u32)cmd->value;
			        AMC_cmd_count++;                    
			        break;
                }		    
		    
            case ADI_EBIU_CMD_SET_ASYNCH_AMBCTL1:                /* set the asynch bank control register 1     */	
		    
                {              
			        *(u32*)&adi_ebiu_settings.ambctl1 = (u32)cmd->value;
			        AMC_cmd_count++;                    
			        break;
                }

        }
    }
    
#ifdef ADI_SSL_DEBUG
	/* check to see that all mandatory settings have been supplied */	
	
	if (!( data_width_set))
		return ADI_EBIU_RESULT_INVALID_DDR_DATA_WIDTH;	

	if (!( device_width_set))
		return ADI_EBIU_RESULT_INVALID_DDR_DEVICE_WIDTH;	

	if (!( device_size_set))
		return ADI_EBIU_RESULT_INVALID_DDR_DEVICE_SIZE;

	if (!( external_banks_set))
		return ADI_EBIU_RESULT_INVALID_DDR_EXTERNAL_BANKS;	

	if (!( cas_set ))
		return ADI_EBIU_RESULT_INVALID_DDR_CAS;	

	if (!( ras_min_set ))
		return ADI_EBIU_RESULT_INVALID_DDR_RAS;	

	if (!( rp_min_set ))
		return ADI_EBIU_RESULT_INVALID_DDR_RP;	

	if (!( rcd_min_set ))
		return ADI_EBIU_RESULT_INVALID_DDR_RCD;	

	if (!( wr_min_set ))
		return ADI_EBIU_RESULT_INVALID_DDR_WR;	

	if (!( rfc_min_set ))
		ADI_EBIU_RESULT_INVALID_DDR_RFC;	

	if (!( rc_min_set ))
		return ADI_EBIU_RESULT_INVALID_DDR_RC;	

	if (!( mrd_min_set ))
		return ADI_EBIU_RESULT_INVALID_DDR_MRD;	

	if (!( refi_set ))
		return ADI_EBIU_RESULT_INVALID_DDR_REFI;	

	if (!( wtr_min_set ))
		return ADI_EBIU_RESULT_INVALID_DDR_WTR;	
 
#endif
    
    
    /* Check status of the DDR Controller soft reset bit */
	
    u16 isEnabled = (*pEBIU_RSTCTL & 0x0001);
    
    if( isEnabled )
    {
         /* make sure the config settings match the status of the reset control register */
         EBIU_DDRCTL4_SOFT_RESET_SET(adi_ebiu_settings.ddrctl4,(u16)1);
    }
	else
	{
		/* If SDRAM is not already enabled, then it is imperative to apply
		   the default values register values here, so we can configure the registers. */
				
        EBIU_DDRCTL4_SOFT_RESET_SET(adi_ebiu_settings.ddrctl4,1);
		
 			
        /* write DDRCTL3 and EBIU_RSTCTL */
	    u32 imask = cli();

        *pEBIU_RSTCTL =  *(u32*)&adi_ebiu_settings.ddrctl4;

        /* EBIU_RSTCTL must be written first on mobile DDR board, emulator has not enabled EBIU already */
        *pEBIU_DDRCTL3 =  *(u32*)&adi_ebiu_settings.ddrctl3;
	    
         sti(imask);
       
       /* Apply the timing parameters */
        adi_ebiu_ApplyDDRConfig(
            U32_REG_CAST(adi_ebiu_settings.ddrctl0),
            U32_REG_CAST(adi_ebiu_settings.ddrctl1),
            U32_REG_CAST(adi_ebiu_settings.ddrctl2)           
        );                   
        
        
         		
	}


	/* Some Asynch Memory commands are processed in 'adi_ebiu_Control' but we apply the changes here,
	   after all commands have been processed  */
	 	  
	if( AMC_cmd_count>1 )
	{
	    AMC_cmd_count = 0;
	    adi_ebiu_ApplyAMConfig( adi_ebiu_settings.amgctl, adi_ebiu_settings.ambctl0, adi_ebiu_settings.ambctl1 );
	}
	
	return ADI_EBIU_RESULT_SUCCESS;
 			
}

	

/************************************************************************************
* Function:    adi_ebiu_AdjustSDRAM
* Description: Adjusts the SDRAM timing values and refresh rate to be optimal for the
*		 given SCLK frequency.  Uses the TIMING_CYCLES macro.
*************************************************************************************/
 

#define RAS_MAX 15   /* default 0x06 */
#define RP_MAX 	15   /* default 0x03 */
#define RCD_MAX 15   /* default 0x03 */
#define WR_MAX 	3    /* default 0x02 */
#define RFC_MAX 15
#define RC_MAX  15
#define MRD_MAX  15
#define REFI_MAX 16383
#define WTR_MAX  15

					
ADI_EBIU_RESULT
adi_ebiu_AdjustSDRAM(u32 fsclk2)
{
	u16 rdiv;
	
	u32 ras, rp, rcd, wr, rfc, rc, mrd, refi;
	
	// Here the granularity of Mhz is sufficiently accurate
	u32 fsclk = fsclk2 / adi_ebiu_settings.MHzFactor;


	if ( (adi_ebiu_settings.InitializedFlag == 0 ) || !EBIU_DDRCTL4_SOFT_RESET_GET(adi_ebiu_settings.ddrctl4) )
		return ADI_EBIU_RESULT_NOT_INITIALIZED;
				
/* Use new SCLK to get the the '_min' values from 'adi_ebiu_settings' in cycles */
/* Then cap at max value */


	TIMING_CYCLES(ras, adi_ebiu_settings.ras_min.time, fsclk)
	if (ras > RAS_MAX) 
	{
	    ras = RAS_MAX;
	}

	TIMING_CYCLES(rp, adi_ebiu_settings.rp_min.time, fsclk)
	if (rp > RP_MAX)
	{
	    rp = RP_MAX;
	}
    
	TIMING_CYCLES(rcd, adi_ebiu_settings.rcd_min.time, fsclk)
	if (rcd > RCD_MAX)
	{
	    rcd = RCD_MAX;
	}
     
	if( adi_ebiu_settings.wr_min.cycles == 0 )
	{
	    wr = 0;
	}
	else
	{ 	
 	
	    TIMING_CYCLES(wr, adi_ebiu_settings.wr_min.time,fsclk)
	    if (wr > WR_MAX)
	    {
	        wr = WR_MAX;
	    }
	}
    
    
	TIMING_CYCLES(rfc, adi_ebiu_settings.rfc_min.time, fsclk)
	if (rfc > RFC_MAX)
	{
	    rfc = RFC_MAX;
	}
       
        
	TIMING_CYCLES(rc, adi_ebiu_settings.rc_min.time, fsclk)
	if (rc > RC_MAX)
	{
            rc = RC_MAX;
	}

    
	TIMING_CYCLES(mrd, adi_ebiu_settings.mrd_min.time, fsclk)
	if (mrd > MRD_MAX)
	{
            mrd = MRD_MAX;
	}
       
      
	TIMING_CYCLES(refi, adi_ebiu_settings.refi.time, fsclk)
	if (refi > REFI_MAX)
	{
            refi = REFI_MAX;
	}

	/* calculate and populate the register settings */

	//EBIU_DDRCTL2_CAS_SET ( adi_ebiu_settings.ddrctl2, (fsclk2 <= adi_ebiu_settings.cl_threshold ? 2 : 3) );
	EBIU_DDRCTL0_RAS_SET ( adi_ebiu_settings.ddrctl0, ras );
	EBIU_DDRCTL0_RP_SET  ( adi_ebiu_settings.ddrctl0, rp  ); 
	EBIU_DDRCTL0_RFC_SET ( adi_ebiu_settings.ddrctl0, rfc );
	EBIU_DDRCTL0_REFI_SET ( adi_ebiu_settings.ddrctl0, refi );
	EBIU_DDRCTL0_RC_SET  ( adi_ebiu_settings.ddrctl0, rc  );
	EBIU_DDRCTL1_RCD_SET ( adi_ebiu_settings.ddrctl1, rcd ); 
	EBIU_DDRCTL1_MRD_SET ( adi_ebiu_settings.ddrctl1, mrd );
	EBIU_DDRCTL1_WR_SET  ( adi_ebiu_settings.ddrctl1, wr );  
	
 	
	adi_ebiu_ApplyDDRConfig(
            U32_REG_CAST(adi_ebiu_settings.ddrctl0),
            U32_REG_CAST(adi_ebiu_settings.ddrctl1),
            U32_REG_CAST(adi_ebiu_settings.ddrctl2)       
        );

		
	return ADI_EBIU_RESULT_SUCCESS;
}

				
 
 

/**************************************
	adi_ebiu_LoadConfig
******************************************/	

ADI_EBIU_RESULT 
adi_ebiu_LoadConfig(ADI_EBIU_CONFIG_HANDLE hConfig, size_t szConfig)
{
	/*Not relevant if module not initialized */
	if ( adi_ebiu_settings.InitializedFlag==0 )
		return ADI_EBIU_RESULT_NOT_INITIALIZED;

	else 
	{
		
		ADI_EBIU_CONFIG *config = (ADI_EBIU_CONFIG*)hConfig;
		
		adi_ebiu_settings.ddrctl0 = config->ddrctl0;
		adi_ebiu_settings.ddrctl1 = config->ddrctl1;
		adi_ebiu_settings.ddrctl2 = config->ddrctl2;
		adi_ebiu_settings.ddrctl4 = config->ddrctl4;
	    adi_ebiu_settings.ras_min = config->ras_min;  /* tRAS = 42ns min = 6 SCLKs */
		adi_ebiu_settings.rp_min = config->rp_min;     /* tRP = 15ns min = 2 SCLKs */
		adi_ebiu_settings.rcd_min = config->rcd_min;   /* tRCD = 15ns min = 2 SCLKs */
		adi_ebiu_settings.wr_min = config->wr_min;     /* tWR = 15ns min = 2 SCLKs */
		adi_ebiu_settings.rfc_min = config->rfc_min;   /* tRFC = 72ns min = 10 SCLKs */
		adi_ebiu_settings.rc_min = config->rc_min;     /* tRC = 60ns min = 8 SCLKs */
		adi_ebiu_settings.mrd_min = config->mrd_min;   /* tMRD = 12ns min = 2 SCLKs */
		adi_ebiu_settings.refi = config->refi;         /* tREFI = 7.8us max = 1037 SCLKs */	
		adi_ebiu_settings.wtr_min = config->wtr_min;   /* tWTR @ CL2 = 7.5ns min = 1 SCLK */
		adi_ebiu_settings.cas = config->cas;           
		adi_ebiu_settings.MHzFactor = config->MHzFactor;		
		

        adi_ebiu_ApplyDDRConfig(
            U32_REG_CAST(adi_ebiu_settings.ddrctl0),
            U32_REG_CAST(adi_ebiu_settings.ddrctl1),
            U32_REG_CAST(adi_ebiu_settings.ddrctl2)           
        );		
	}
	
	
	return ADI_EBIU_RESULT_SUCCESS;
}
	


/************************************************************************************
* Function:    adi_ebiu_SaveConfig
* Description: Saves the current EBIU settings to the memory location provided
*************************************************************************************/

ADI_EBIU_RESULT 
adi_ebiu_SaveConfig(ADI_EBIU_CONFIG_HANDLE hConfig, size_t szConfig)
{
	/* Not relevant if module not initialized */
	if (adi_ebiu_settings.InitializedFlag==0 )
		return ADI_EBIU_RESULT_NOT_INITIALIZED;

	else 
	{
		
		ADI_EBIU_CONFIG *config = (ADI_EBIU_CONFIG*)hConfig;


		config->ddrctl0 = adi_ebiu_settings.ddrctl0;
		config->ddrctl1 = adi_ebiu_settings.ddrctl1;
		config->ddrctl2 = adi_ebiu_settings.ddrctl2;
		config->ddrctl4 = adi_ebiu_settings.ddrctl4;
		config->ras_min  = adi_ebiu_settings.ras_min;  /* tRAS = 42ns min = 6 SCLKs */
		config->rp_min = adi_ebiu_settings.rp_min;     /* tRP = 15ns min = 2 SCLKs */
		config->rcd_min = adi_ebiu_settings.rcd_min;   /* tRCD = 15ns min = 2 SCLKs */
		config->wr_min = adi_ebiu_settings.wr_min;     /* tWR = 15ns min = 2 SCLKs */
		config->rfc_min = adi_ebiu_settings.rfc_min;   /* tRFC = 72ns min = 10 SCLKs */
		config->rc_min = adi_ebiu_settings.rc_min;     /* tRC = 60ns min = 8 SCLKs */
		config->mrd_min = adi_ebiu_settings.mrd_min;   /* tMRD = 12ns min = 2 SCLKs */
		config->refi = adi_ebiu_settings.refi;         /* tREFI = 7,8us max = 1037 SCLKs */	
		config->wtr_min = adi_ebiu_settings.wtr_min;   /* tWTR @ CL2 = 7.5ns min = 1 SCLK */
		config->cas = adi_ebiu_settings.cas;           /* tWTR @ CL2 = 7.5ns min = 1 SCLK */
		config->MHzFactor = adi_ebiu_settings.MHzFactor;		
		 		
	}

	return ADI_EBIU_RESULT_SUCCESS;
}

/************************************************************************************
* Function:    adi_ebiu_set_ddr_values
* Description: Sets the EBIU module for Micron DDR memory
*************************************************************************************/


static int
adi_ebiu_set_ddr_values(
	u32 module_id,
	ADI_EBIU_DDRCTL0_REG *ddrctl0,
	ADI_EBIU_DDRCTL1_REG *ddrct1l,
	ADI_EBIU_DDRCTL2_REG *ddrctl2		
)	
	 
{

	switch(module_id)
	{
		 
	//	case ADI_EBIU_SDRAM_MODULE_MT46V32M16P_6T_F:
		case ADI_EBIU_SDRAM_MODULE_MT46V32M16P_5B_F:
			adi_ebiu_settings.ras_min.cycles	= 6;    
			adi_ebiu_settings.rp_min.cycles		= 2;    
			adi_ebiu_settings.rcd_min.cycles	= 2;    
			adi_ebiu_settings.wr_min.cycles		= 2;    
			adi_ebiu_settings.refi.cycles =     1037;
			break;
 
	}
	return 1;
}

 	

/************************************************************************************

* Function:    adi_ebiu_Terminate

* Description: Resets the initialized flag 

*************************************************************************************/

ADI_EBIU_RESULT
adi_ebiu_Terminate(void)
{
	adi_ebiu_settings.InitializedFlag = 0;
	return ADI_EBIU_RESULT_SUCCESS;
}



/* end of DDR memory section */
#endif 




