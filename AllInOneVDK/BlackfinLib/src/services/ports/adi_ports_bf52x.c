/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_ports_bf52x.c,v $
$Revision: 5506 $
$Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $

Description:
			Port Configuration module for the System Services Library

*********************************************************************************/

#include <services/services.h>

/*********************************************************************

Data structures for the port registers

*********************************************************************/

/* PORTx_MUX register addresses */
static volatile u16 *PortMuxRegs[] = {
    pPORTF_MUX,     /* Port F Multiplexer Control Register */
    pPORTG_MUX,     /* Port G Multiplexer Control Register */
    pPORTH_MUX,     /* Port H Multiplexer Control Register */
};

/* PORTx_FER register addresses */
static volatile u16 *PortFerRegs[] = {
    pPORTF_FER,     /* Port F Function Enable Register      */
    pPORTG_FER,     /* Port G Function Enable Register      */
    pPORTH_FER,     /* Port H Function Enable Register      */
};

/* number of Port Multiplexer /  Port Function Enable Registers */
#define ADI_PORTS_NUM_PORTS     (sizeof(PortMuxRegs)/sizeof(PortMuxRegs[0]))

/*********************************************************************

Instance data for the port control service

*********************************************************************/

typedef struct ADI_PORTS_INSTANCE_DATA {    /* instance data                */
	void    *pEnterCriticalArg;	            /* critical region parameter    */
} ADI_PORTS_INSTANCE_DATA;

/* instance data for port control */
static ADI_PORTS_INSTANCE_DATA  adi_ports_InstanceData;

/*********************************************************************

static functions

*********************************************************************/

#if defined(ADI_SSL_DEBUG)

static ADI_PORTS_RESULT adi_ports_ValidateDirectives(   /* validates an array of directives     */
	ADI_PORTS_DIRECTIVE	    *pDirectives,               /* pointer to an array of directives    */
	u32                     nDirectives                 /* number of directives                 */
);

#endif

/*********************************************************************

	Function:		adi_ports_Init

	Description:	Initializes the port control service

*********************************************************************/

ADI_PORTS_RESULT    adi_ports_Init(  	/* initializes the port control service */
	void    *pEnterCriticalArg	        /* parameter for critical region function */
) {

    /* save the critical region data */
	adi_ports_InstanceData.pEnterCriticalArg = pEnterCriticalArg;

	/* return */
	return (ADI_PORTS_RESULT_SUCCESS);
}


/*********************************************************************

	Function:		adi_ports_Terminate

	Description:	Terminates the port control service

*********************************************************************/

ADI_PORTS_RESULT	adi_ports_Terminate( 	/* terminates the port control service */
	void
) {

	/* return */
	return (ADI_PORTS_RESULT_SUCCESS);
}

/*********************************************************************

	Function:		adi_ports_Configure

	Description:	Configure GPIO pins for GPIO use or peripheral use,
	                depending on the Directive type passed.

*********************************************************************/
ADI_PORTS_RESULT adi_ports_Configure(   /* Configures GPIO pins for GPIO or Peripheral use  */
    ADI_PORTS_DIRECTIVE *pDirectives,   /* pointer to an array of directives                */
    u32                 nDirectives     /* number of directives                             */
){

    u32                 i,j;                    /* counter */
    ADI_PORTS_RESULT    Result;                 /* return code */
	void                *pExitCriticalArg;      /* exit critical region parameter       */
    /* To extract individual fields from given Directive */
    u8      Bit,Port;
    /*Shift count to reach Mux value position for this Bit/Pin */
    u8      MuxValShift;
    /* Arrays to update PORTx_MUX registers */
    u32     PortMuxMask[ADI_PORTS_NUM_PORTS];	/* PORTx_MUX masks  */
    u32     PortMuxVal[ADI_PORTS_NUM_PORTS];	/* PORTx_MUX values */
    /* Arrays to update PORTx_FER registers */
    u16     PortFerSet[ADI_PORTS_NUM_PORTS];	/* PORTx_FER bits to set    */
    u16     PortFerClear[ADI_PORTS_NUM_PORTS]; 	/* PORTx_FER bits to clear  */
    u16     UpdatePortFers = 0;     /* PORTx_FER's marked for update */
    u16     UpdatePortMuxs = 0;     /* PORTx_MUX registers marked for update */
    u32     u32Value;
    u16     u16Value;

    /* assume we're going to be successful */
    Result = ADI_PORTS_RESULT_SUCCESS;

/* Debug mode only */
#ifdef ADI_SSL_DEBUG
	/* Test that array of directives is valid */
	Result = adi_ports_ValidateDirectives(pDirectives,nDirectives);
	/* continue only if validation returns success */
	if (Result == ADI_PORTS_RESULT_SUCCESS)
	{
#endif

    /* initialize the Port register update arrays */
    for (i = 0; i < ADI_PORTS_NUM_PORTS; i++)
    {
        PortMuxMask[i]	= 0xFFFFFFFF;
        PortMuxVal[i] 	= 0;
        PortFerSet[i] 	= 0;
        PortFerClear[i] = 0;
    }

    /* FOR (each directive passed in) */
    for (i = nDirectives; i ; i--, *pDirectives++)
    {
        /* Port id for this bit/pin */
        Port = ADI_PORTS_GET_PORT(*pDirectives);

		/*	Skip intentionally undefined ports (marked as -1).
			Some I/O bypasses the port muxing and have dedicated pins
			(such as PortJ) and there is nothing to program.
			This test relies on knowledge of the port encodings.
		*/
		if (0xFF == Port)
			continue;

        /* PORTx_FER bit location for this pin */
        Bit = ADI_PORTS_GET_BIT_POSITION(*pDirectives);
        /*Shift count to reach Mux value position for this Bit/Pin */
        MuxValShift = ADI_PORTS_GET_PIN_MUX_POSITION(*pDirectives);

        /* IF (configure this bit/pin for peripheral use) */
        if (ADI_PORTS_GET_PORT_FUNCTION(*pDirectives))
        {
            /* Generate PORTx_FER for this Directive */
            PortFerSet[Port]    |= (1 << Bit);
            /* Generate PORTx_MUX mask for this bit/pin */
			PortMuxMask[Port]   &= (~(3 << MuxValShift));
            /* Generate PORTx_MUX value for this bit/pin */
            PortMuxVal[Port]    |= (ADI_PORTS_GET_PIN_MUX_VALUE(*pDirectives) << MuxValShift);
            /* Mark this port multiplexer control register for update */
            UpdatePortMuxs |= (1 << Port);
        }
        /* ELSE (configure this bit/pin as GPIO) */
        else
        {
            /* Generate PORTx_FER for this Directive */
            PortFerClear[Port]  |= (1 << Bit);
        }
        /* Mark this port function enable register for update */
        UpdatePortFers |= (1 << Port);
    }

    /* protect us */
    pExitCriticalArg = adi_int_EnterCriticalRegion(adi_ports_InstanceData.pEnterCriticalArg);

    /* This minimizes the amount of time interrupts are disabled */
    /* FOR (each port register) */
    for (i = 0; i < ADI_PORTS_NUM_PORTS; i++, UpdatePortFers >>=1,UpdatePortMuxs >>=1  )
    {
        /* IF (this port control multiplexer register is marked for update) */
        if (UpdatePortMuxs & 0x01)
        {
            /* Update PORTx_MUX register */
            /* Read Port Multiplexer Control Register */
            u32Value = *PortMuxRegs[i];
            /* Clear the Mux Field location(s) if needed */
            u32Value &= PortMuxMask[i];
            /* update the Mux Field locations */
            u32Value |= PortMuxVal[i];
            /* update Multiplexer Control Register with new value */
            *PortMuxRegs[i] = u32Value;
        }
        /* IF (this port function enable register is marked for update) */
        if (UpdatePortFers & 0x01)
        {
            /* Update PORTx_FER register */
            /* Read Port Function Enable Register */
            u16Value = *PortFerRegs[i];
            /* OR in the bits that need set */
            u16Value |= PortFerSet[i];
            /* AND in the bits that need cleared */
            u16Value &= ~PortFerClear[i];
            /* update Function Enable Register with new value */
            *PortFerRegs[i] = u16Value;
        }
    }

    /* unprotect */
    adi_int_ExitCriticalRegion(pExitCriticalArg);

/* Debug mode only */
#ifdef ADI_SSL_DEBUG
    }
#endif

    return (Result);
}

#ifdef ADI_SSL_DEBUG

/*********************************************************************

	Function:		adi_ports_ValidateDirectives

	Description:	Validates an array of directives

*********************************************************************/

ADI_PORTS_RESULT adi_ports_ValidateDirectives(
    ADI_PORTS_DIRECTIVE *pDirectives,   /* pointer to an array of directives    */
    u32                 nDirectives     /* number of directives                 */
){

    u32                 i;      /* counter */
    ADI_PORTS_RESULT    Result; /* return code */

    /* assume we're going to be successful */
    Result = ADI_PORTS_RESULT_SUCCESS;

	/* Test that array of directives is valid */
	if (!pDirectives)
	{
		Result = ADI_PORTS_RESULT_NULL_ARRAY;
    }
    /* validate each directive passed in */
    else
    {
        /* FOR (each directive passed in) */
        for (i = nDirectives; i; i--, pDirectives++)
        {
            /* insure the bit position is valid */
            if (ADI_PORTS_GET_BIT_POSITION(*pDirectives) > 15)
            {
                Result = ADI_PORTS_RESULT_BAD_DIRECTIVE;
                break;
            }

            /* insure the port index is valid */
            if (ADI_PORTS_GET_PORT(*pDirectives) >= ADI_PORTS_NUM_PORTS)
            {
                Result = ADI_PORTS_RESULT_BAD_DIRECTIVE;
                break;
            }
        }
    }

	/* return */
	return (Result);
}

#endif  /* ADI_SSL_DEBUG */

/*****/
