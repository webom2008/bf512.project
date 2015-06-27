/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
******************************************************************************
Copyright (c), 2009 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title: ADI Device Drivers and System Services

Description:
    Port configuration module for Delta (ADSP-BF59x) family of
    blackfin processors for System Services Library

*****************************************************************************/

/* System Services includes */
#include <services/services.h>

/*
**
** Array to hold the port control register addresses
**
*/

/* PORTx_MUX register addresses */
static volatile u16 *apPortMuxRegs[] =
{
    pPORTF_MUX,     /* Port F Multiplexer Control Register */
    pPORTG_MUX,     /* Port G Multiplexer Control Register */
};

/* PORTx_FER addresses */
static volatile u16 *apPortFuncEnableRegs[] =
{
    pPORTF_FER,     /* Port F Function Enable Register */
    pPORTG_FER,     /* Port G Function Enable Register */
};

/* Number of Port Multiplexer/Port Function Enable Registers */
#define ADI_PORTS_NUM_PORTS     (sizeof(apPortMuxRegs)/sizeof(apPortMuxRegs[0]))

/*
**
** Structure to hold initialisation data for port control service
**
*/
typedef struct ADI_PORTS_INSTANCE_DATA
{

    /* critical region parameter */
    void    *pEnterCriticalArg;

} ADI_PORTS_INSTANCE_DATA;

/* Instance to hold port control initialisation data */
static ADI_PORTS_INSTANCE_DATA  adi_ports_InstanceData;

/*
**
** Local function prototypes
**
*/

/* Debug Build only */
#if defined(ADI_SSL_DEBUG)

/* Validates given array of directives */
static ADI_PORTS_RESULT adi_ports_ValidateDirectives(
    ADI_PORTS_DIRECTIVE     *pDirectives,
    u32                     nDirectives
);

#endif

/*********************************************************************

    Function: adi_ports_Init

        Initialises port control service for Moy family of processors

    Parameters:
        pEnterCriticalArg  - parameter for critical region function

    Returns:
        ADI_PORTS_RESULT_SUCCESS
            - Successfully initialised Port control service

*********************************************************************/
ADI_PORTS_RESULT    adi_ports_Init(
    void    *pEnterCriticalArg
)
{
    /* save the critical region data */
    adi_ports_InstanceData.pEnterCriticalArg = pEnterCriticalArg;

    return (ADI_PORTS_RESULT_SUCCESS);
}

/*********************************************************************

    Function: adi_ports_Terminate

        Terminates the port control service

    Parameters:
        None

    Returns
        ADI_PORTS_RESULT_SUCCESS
            - Successfully terminated Port control service

*********************************************************************/
ADI_PORTS_RESULT    adi_ports_Terminate(
    void
)
{
    return (ADI_PORTS_RESULT_SUCCESS);
}

/*********************************************************************

    Function: adi_ports_Configure

        Configure Port pins for GPIO use or peripheral use
        depending on the directive type passed

    Parameters:
        pDirectives - Pointer to array of directives
        nDirectives - Number of directives in the array

*********************************************************************/
ADI_PORTS_RESULT adi_ports_Configure(
    ADI_PORTS_DIRECTIVE *pDirectives,
    u32                 nDirectives
)
{
    /* Loop variable */
    u8                  i;
    /* To extract individual fields from given Directive */
    u8                  nBit,nPort;
    /*Shift count to reach Mux value position for this Bit/Pin */
    u8                  nMuxValShift;
    /* Arrays to update PORTx_MUX registers */
    u16                 anPortMuxMask[ADI_PORTS_NUM_PORTS];   /* PORTx_MUX masks  */
    u16                 anPortMuxVal[ADI_PORTS_NUM_PORTS];    /* PORTx_MUX values */
    /* Arrays to update PORTx_FER registers */
    u16                 anPortFerSet[ADI_PORTS_NUM_PORTS];    /* PORTx_FER bits to set  */
    u16                 anPortFerClear[ADI_PORTS_NUM_PORTS];  /* PORTx_FER bits to clear */
    /* PORTx_FER's marked for update */
    u16                 nUpdatePortFers = 0;
    /* PORTx_MUX registers marked for update */
    u16                 nUpdatePortMuxs = 0;
    /* Location to manipulate PORT_MUX values */
    u16                 nMuxRegValue;
    /* Location to manipulate PORT_FER values */
    u16                 nFuncEnableValue;
    /* exit critical region parameter */
    void                *pExitCriticalArg;
    /* return code */
    ADI_PORTS_RESULT    Result;

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

        /* Initialise the Port register update arrays */
        for (i = 0; i < ADI_PORTS_NUM_PORTS; i++)
        {
            anPortMuxMask[i]  = 0xFFFF;
            anPortMuxVal[i]   = 0;
            anPortFerSet[i]   = 0;
            anPortFerClear[i] = 0;
        }

        /* FOR (each directive passed in) */
        for (i = nDirectives; i ; i--, *pDirectives++)
        {
            /* Port id for this bit/pin */
            nPort = ADI_PORTS_GET_PORT(*pDirectives);
            /* PORTx_FER bit location for this pin */
            nBit = ADI_PORTS_GET_BIT_POSITION(*pDirectives);
            /*Shift count to reach Mux value position for this Bit/Pin */
            nMuxValShift = ADI_PORTS_GET_PIN_MUX_POSITION(*pDirectives);

            /* IF (configure this bit/pin for peripheral use) */
            if (ADI_PORTS_GET_PORT_FUNCTION(*pDirectives))
            {
                /* Generate PORTx_FER for this Directive */
                anPortFerSet[nPort] |= (1 << nBit);
                /* Generate PORTx_MUX mask for this bit/pin */
                anPortMuxMask[nPort]&= (~(3 << nMuxValShift));
                /* Generate PORTx_MUX value for this bit/pin */
                anPortMuxVal[nPort] |= (ADI_PORTS_GET_PIN_MUX_VALUE(*pDirectives) << nMuxValShift);
                /* Mark this port multiplexer control register for update */
                nUpdatePortMuxs     |= (1 << nPort);
            }
            /* ELSE (configure this bit/pin as GPIO) */
            else
            {
                /* Generate PORTx_FER for this Directive */
                anPortFerClear[nPort] |= (1 << nBit);
            }
            /* Mark this port function enable register for update */
            nUpdatePortFers |= (1 << nPort);
        }

        /* protect us */
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_ports_InstanceData.pEnterCriticalArg);

        /* This minimizes the amount of time interrupts are disabled */
        /* FOR (each port register) */
        for (i = 0;
             i < ADI_PORTS_NUM_PORTS;
             i++, nUpdatePortFers >>= 1, nUpdatePortMuxs >>= 1)
        {
            /* IF (this port control multiplexer register is marked for update) */
            if (nUpdatePortMuxs & 0x01)
            {
                /* Update PORTx_MUX register */
                /* Read Port Multiplexer Control Register */
                nMuxRegValue = *apPortMuxRegs[i];
                /* Clear the Mux Field location(s) if needed */
                nMuxRegValue &= anPortMuxMask[i];
                /* update the Mux Field locations */
                nMuxRegValue |= anPortMuxVal[i];
                /* update Multiplexer Control Register with new value */
                *apPortMuxRegs[i] = nMuxRegValue;
            }
            /* IF (this port function enable register is marked for update) */
            if (nUpdatePortFers & 0x01)
            {
                /* Update PORTx_FER register */
                /* Read Port Function Enable Register */
                nFuncEnableValue = *apPortFuncEnableRegs[i];
                /* OR in the bits that need set */
                nFuncEnableValue |= anPortFerSet[i];
                /* AND in the bits that need cleared */
                nFuncEnableValue &= ~anPortFerClear[i];
                /* update Function Enable Register with new value */
                *apPortFuncEnableRegs[i] = nFuncEnableValue;
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

/* Debug Build only */

#ifdef ADI_SSL_DEBUG

/*********************************************************************

    Function: adi_ports_ValidateDirectives

        Validates an array of directives

    Parameters:
        pDirectives - Pointer to array of directives
        nDirectives - Number of directives in the array

    Returns:
        ADI_PORTS_RESULT_SUCCESS
            - Successfully validated directives
        ADI_PORTS_RESULT_NULL_ARRAY
            - Given address to directives array is invalid
        ADI_PORTS_RESULT_BAD_DIRECTIVE
            - Given directive is invalid

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

    return (Result);
}

#endif  /* ADI_SSL_DEBUG */

/*****/

/*
**
** EOF: $
**
*/
