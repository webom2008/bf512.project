/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree 
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_ports_bf538.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			Port Configuration module for the System Services Library

*********************************************************************************/

#include <services/services.h>

/*********************************************************************

data structures for the port registers

*********************************************************************/

static volatile u16 *PortRegisterAddresses[] = { // port register addresses
    pPORTCIO_FER,  // port C address
    pPORTDIO_FER,  // port D address
    pPORTEIO_FER   // port E address
};
    
// number of port registers
#define ADI_PORTS_NUM_PORTS  (sizeof(PortRegisterAddresses)/sizeof(PortRegisterAddresses[0]))
    
/*********************************************************************

Instance data for the port control service

*********************************************************************/

typedef struct ADI_PORTS_INSTANCE_DATA {		// instance data
	void    *pEnterCriticalArg;	// critical region parameter
} ADI_PORTS_INSTANCE_DATA;

static ADI_PORTS_INSTANCE_DATA adi_ports_InstanceData;	// instance data for port control


/*********************************************************************

static functions

*********************************************************************/

#if defined(ADI_SSL_DEBUG)

static ADI_PORTS_RESULT adi_ports_ValidateDirectives(		// validates an array of directives
	ADI_PORTS_DIRECTIVE	*pDirectives,
	u32                 nDirectives
);

#endif


/*********************************************************************

	Function:		adi_ports_Init

	Description:	Initializes the port control service

*********************************************************************/

ADI_PORTS_RESULT    adi_ports_Init(  	// initializes the port control service
	void    *pEnterCriticalArg	// parameter for critical region function
) {
	
	// save the critical region data
	adi_ports_InstanceData.pEnterCriticalArg = pEnterCriticalArg;

	// return
	return (ADI_PORTS_RESULT_SUCCESS);
}

	
/*********************************************************************

	Function:		adi_ports_Terminate

	Description:	Terminates the port control service

*********************************************************************/

ADI_PORTS_RESULT	adi_ports_Terminate( 	// terminates the port control service
	void
) {
	
	// return
	return (ADI_PORTS_RESULT_SUCCESS);
}
		
	

/*********************************************************************

	Function:		adi_ports_Configure

	Description:	Configures the pin muxing logic for an array of
	                directives

*********************************************************************/

ADI_PORTS_RESULT adi_ports_Configure(	// configures for an array of directives
	ADI_PORTS_DIRECTIVE	*pDirectives,
	u32                 nDirectives
){

    u32                 i;                                  // counter
    ADI_PORTS_RESULT    Result;                             // return code
	void                *pExitCriticalArg;	                // exit critical region parameter
	u16                 BitsToSet[ADI_PORTS_NUM_PORTS];     // bits that need to be set
	u16                 BitsToClear[ADI_PORTS_NUM_PORTS];   // bits that need to be cleared
	u16                 Data;                               // data from the port registers
    
    
	// validate directives
#if defined(ADI_SSL_DEBUG)
	if ((Result = adi_ports_ValidateDirectives(pDirectives, nDirectives))!= ADI_FLAG_RESULT_SUCCESS) {
		return (Result);
	}
#endif

    // ever the optimist
    Result = ADI_PORTS_RESULT_SUCCESS;
    
    // initialize the BitsToSet and BitsToClear registers
    for (i = 0; i <= ADI_PORTS_NUM_PORTS; i++) {
        BitsToSet[i] = 0;
        BitsToClear[i] = 0;
    }

    // FOR (each directive passed in)
    for (i = nDirectives; i; i--, pDirectives++) {
        
        // IF (the directive is configured to set a bit in the register)
        if (ADI_PORTS_GET_BIT_VALUE(*pDirectives) == 1) {
            
            // set the proper bit in the proper BitsToSet register
            BitsToSet[ADI_PORTS_GET_PORT(*pDirectives)] |= ADI_PORTS_GET_MASK(*pDirectives);
            
        // ELSE
        } else {
            
            // set the proper bit in the proper BitsToClear register
            BitsToClear[ADI_PORTS_GET_PORT(*pDirectives)] |= ADI_PORTS_GET_MASK(*pDirectives);
            
        // ENDIF
        }
        
    // ENDFOR
    }

    // FOR (each port register)  *** done this way to minimize the amount of time interrupts are disabled ***
    for (i = 0; i < ADI_PORTS_NUM_PORTS; i++) {
        
        // protect us
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_ports_InstanceData.pEnterCriticalArg);
        
        // read in the port register
        Data = *PortRegisterAddresses[i];
        
        // OR in the bits that need set
        Data |= BitsToSet[i];
        
        // AND in the bits that need cleared
        Data &= ~BitsToClear[i];
        
        // write out the port register
        *PortRegisterAddresses[i] = Data;
        
        // unprotect
    	adi_int_ExitCriticalRegion(pExitCriticalArg);
    	
    // ENDFOR
    }
    
    // return
	return (Result);
}


#ifdef ADI_SSL_DEBUG

/*********************************************************************

	Function:		adi_ports_ValidateDirectives

	Description:	Validates an array of directives

*********************************************************************/

ADI_PORTS_RESULT adi_ports_ValidateDirectives(	// validates an array of directives
	ADI_PORTS_DIRECTIVE	*pDirectives,
	u32                 nDirectives
){

    u32                 i;      // counter
    ADI_PORTS_RESULT    Result; // return code
    
    // ever the optimist
    Result = ADI_PORTS_RESULT_SUCCESS;

    // FOR (each directive passed in)
    for (i = nDirectives; i; i--, pDirectives++) {
        
        // insure the bit position is valid
        if (ADI_PORTS_GET_BIT_POSITION(*pDirectives) > 15) {
            Result = ADI_PORTS_RESULT_BAD_DIRECTIVE;
            break;
        }
        
        // insure the port index is valid
        if (ADI_PORTS_GET_PORT(*pDirectives) >= ADI_PORTS_NUM_PORTS) {
            Result = ADI_PORTS_RESULT_BAD_DIRECTIVE;
            break;
        }
        
    // ENDFOR
    }

	// return
	return (Result);
}

#endif  // ADI_SSL_DEBUG

	
