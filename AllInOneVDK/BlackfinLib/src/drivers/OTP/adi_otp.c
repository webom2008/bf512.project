/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_otp.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This is a preliminary device driver for OTP access

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/


#include <bfrom.h>
#include <drivers/adi_dev.h>		/* device manager includes */
#include <adi_otp.h>				/* device driver specific includes */


/*********************************************************************

Static functions that the device manager will call

NOTE: This section should not be changed

*********************************************************************/

static u32 adi_pdd_Open(			        	/* Open a device */
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,			/* device manager handle */
	u32 					DeviceNumber,			/* device number */
	ADI_DEV_DEVICE_HANDLE 	DeviceHandle,			/* device handle */
	ADI_DEV_PDD_HANDLE 		*pPDDHandle,			/* pointer to PDD handle location */
	ADI_DEV_DIRECTION 		Direction,				/* data direction */
	void					*pCriticalRegionArg,    /* critical region parameter */
	ADI_DMA_MANAGER_HANDLE	DMAHandle,				/* handle to the DMA manager */
	ADI_DCB_HANDLE			DCBHandle,				/* callback handle */
	ADI_DCB_CALLBACK_FN		DMCallback				/* device manager callback function */
);

static u32 adi_pdd_Close(		    /* Closes a device */
	ADI_DEV_PDD_HANDLE PDDHandle	    /* PDD handle */
);

static u32 adi_pdd_Read(		    /* Reads data or queues an inbound buffer to a device */
	ADI_DEV_PDD_HANDLE PDDHandle,	    /* PDD handle */
	ADI_DEV_BUFFER_TYPE	BufferType,	    /* buffer type */
	ADI_DEV_BUFFER *pBuffer			    /* pointer to buffer */
);

static u32 adi_pdd_Write(		    /* Writes data or queues an outbound buffer to a device */
	ADI_DEV_PDD_HANDLE PDDHandle,	    /* PDD handle */
	ADI_DEV_BUFFER_TYPE	BufferType,	    /* buffer type */
	ADI_DEV_BUFFER *pBuffer			    /* pointer to buffer */
);

static u32 adi_pdd_Control(		    /* Sets or senses a device specific parameter */
	ADI_DEV_PDD_HANDLE PDDHandle,	    /* PDD handle */
	u32 Command,				    	/* command ID */
	void *Value						    /* command specific value */
);


/*********************************************************************

Entry point for device manager to use to access this driver

NOTE: In this section, only the actual name of the entry point should change.

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADIOTPEntryPoint = {
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control
};


/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_OTP_NUM_DEVICES	(sizeof(Device)/sizeof(ADI_OTP))	/* number of OTP devices in the system */


/*********************************************************************

Data Structures

*********************************************************************/


typedef struct {				/* device structure */
    u8                      InUseFlag;              /* in use flag */
    u8                      DataFlowEnabledFlag;    /* dataflow enabled flag */
    u8                      ErrorReportingFlag;     /* error reporting flag */
	ADI_DCB_HANDLE			DCBHandle;				/* callback handle */
	ADI_DEV_DEVICE_HANDLE	DeviceHandle;			/* device manager handle */
	void (*DMCallback) (							/* device manager's callback function */
		ADI_DEV_DEVICE_HANDLE DeviceHandle,			/* device handle */
		u32 Event,									/* event ID */
		void *pArg);								/* argument pointer */
	u32						OTPTimingValue;
	u32						OTP_Base_Address;
} ADI_OTP;


/*********************************************************************

Device specific data

*********************************************************************/

/****************************

ADSP-BF54x

*****************************/

#if defined(__ADSPBF54x__)

static ADI_OTP Device[] = {	/* Actual OTP devices */
	FALSE,					/* in use flag */
	FALSE,					/* dataflow enabled flag */
	FALSE,					/* error reporting flag */
	NULL, 					/* callback handle */
	NULL, 					/* device manager handle */
	NULL,					/* device manager callback function */
	0x0A009464,				/* OTP default timing value with SCLK=100MHz/VDDINT=1.0v */
	0xFFC04300,				/* OTP base address */
};

#define OTP_TP3		0x0A008000	/* constant OTP_TP3 value */

#endif


/****************************

ADSP-BF523, ADSP-BF525, and ADSP-BF527

*****************************/

#if defined(__ADSPBF523__) || defined(__ADSPBF525__) || defined(__ADSPBF527__)

static ADI_OTP Device[] = {	/* Actual OTP devices */
	FALSE,					/* in use flag */
	FALSE,					/* dataflow enabled flag */
	FALSE,					/* error reporting flag */
	NULL, 					/* callback handle */
	NULL, 					/* device manager handle */
	NULL,					/* device manager callback function */
	0x0A548664,				/* OTP default timing value with 100 MHz SCLK */
	0xFFC03600,				/* OTP base address */
};

#define OTP_TP3		0x0A548000	/* constant OTP_TP3 value */

#endif


/****************************

ADSP-BF522, ADSP-BF524, and ADSP-BF526

*****************************/

#if defined(__ADSPBF522__) || defined(__ADSPBF524__) || defined(__ADSPBF526__)

static ADI_OTP Device[] = {	/* Actual OTP devices */
	FALSE,					/* in use flag */
	FALSE,					/* dataflow enabled flag */
	FALSE,					/* error reporting flag */
	NULL, 					/* callback handle */
	NULL, 					/* device manager handle */
	NULL,					/* device manager callback function */
	0x0A548664,				/* OTP default timing value with 100 MHz SCLK */
	0xFFC03600,				/* OTP base address */
};

#define OTP_TP3		0x0A548000	/* constant OTP_TP3 value */

#endif

/****************************

ADSP-BF512, ADSP-BF514, ADSP-BF516, and ASDP-BF518

*****************************/

#if defined(__ADSPBF512__) || defined(__ADSPBF514__) || defined(__ADSPBF516__) || defined(__ADSPBF518__)

static ADI_OTP Device[] = {	/* Actual OTP devices */
	FALSE,					/* in use flag */
	FALSE,					/* dataflow enabled flag */
	FALSE,					/* error reporting flag */
	NULL, 					/* callback handle */
	NULL, 					/* device manager handle */
	NULL,					/* device manager callback function */
	0x15548732,				/* OTP default timing value with 100 MHz SCLK */
	0xFFC03600,				/* OTP base address */
};

#define OTP_TP3		0x15548000	/* constant OTP_TP3 value */

#endif

/* All EZ-KITs have a 25Mhz Oscillator
   a custom board will have to change this depending on the value
   they use */
#define CLKIN_VAL	25


/*********************************************************************

Static functions that are used internally to the driver

*********************************************************************/
static u32 CleanupOTPRegs(ADI_OTP *pDev);

/*********************************************************************

Static functions that are used in the debug version of the driver

*********************************************************************/

#if defined(ADI_DEV_DEBUG)

static u32 ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);

#endif



/*********************************************************************
*
*	Function:		adi_pdd_Open
*
*	Description:	Opens an OTP device for use
*
*********************************************************************/


static u32 adi_pdd_Open(						/* Open a device */
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,			/* device manager handle */
	u32 					DeviceNumber,			/* device number */
	ADI_DEV_DEVICE_HANDLE	DeviceHandle,			/* device handle */
	ADI_DEV_PDD_HANDLE 		*pPDDHandle,			/* pointer to PDD handle location */
	ADI_DEV_DIRECTION 		Direction,				/* data direction */
	void					*pEnterCriticalArg,		/* enter critical region parameter */
	ADI_DMA_MANAGER_HANDLE	DMAHandle,				/* handle to the DMA manager */
	ADI_DCB_HANDLE			DCBHandle,				/* callback handle */
	ADI_DCB_CALLBACK_FN		DMCallback				/* client callback function */
) {

	u32 			Result;					/* return value */
	ADI_OTP         *pDevice;				/* pointer to the device we're working on */
	void			*pExitCriticalArg;		/* exit critical region parameter */
	u16				u16CCLK = CLKIN_VAL * ((*pPLL_CTL >> 9) & 0x3F);	/* calculate CCLK */
	u16				u16SCLK = u16CCLK / *pPLL_DIV;						/* calculate SCLK */
	float 			sclk_period = (1/(float)u16SCLK) * 1000;			/* calculate SCLK period */
	u32				u32OTP_TP1 = ( 1000 / sclk_period );				/* TP1 of timing value */
	u32				u32OTP_TP2 = 0;										/* TP2 of timing value */
	u32				u32OTP_TP3 = OTP_TP3;								/* TP3 of timing value */
	
	/* be optimistic */
	Result = ADI_DEV_RESULT_SUCCESS;

	/* check for errors if required */
#if defined(ADI_DEV_DEBUG)
	if (DeviceNumber >= ADI_OTP_NUM_DEVICES) {		/* check the device number */
		return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	}
#endif

	/* insure the device the client wants is available */
	Result = ADI_DEV_RESULT_DEVICE_IN_USE;
	pDevice = &Device[DeviceNumber];
	pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
	if (pDevice->InUseFlag == FALSE) {
		pDevice->InUseFlag = TRUE;
		Result = ADI_DEV_RESULT_SUCCESS;
	}
	adi_int_ExitCriticalRegion(pExitCriticalArg);

	/* IF (the device was available for use) */
	if (Result == ADI_DEV_RESULT_SUCCESS) {

    	/* save the physical device handle in the client supplied location */
	    *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;

	/* ENDIF */
    }

#if defined(__ADSPBF54x__)

	/* calculate the timing value based on CCLK and SCLK */
	u32OTP_TP2 = ( 400 / (u32)( 2 * sclk_period ) ) << 8;

	/* if silicon rev is 0.0, OTP is not supported */
	if( *pBK_REVISION == 0xAD010001 )
		Result = ADI_OTP_SILICON_REV_NOT_SUPPORTED;
		
#endif

#if defined(__ADSPBF523__) || defined(__ADSPBF525__) || defined(__ADSPBF527__)

	/* calculate the timing value based on CCLK and SCLK */
	u32OTP_TP2 = ( 120 / (u32)( 2 * sclk_period ) ) << 8;
		
#endif

#if defined(__ADSPBF522__) || defined(__ADSPBF524__) || defined(__ADSPBF526__)

	/* calculate the timing value based on CCLK and SCLK */
	u32OTP_TP2 = ( 120 / (u32)( 2 * sclk_period ) ) << 8;

#endif

#if defined(__ADSPBF512__) || defined(__ADSPBF514__) || defined(__ADSPBF516__) || defined(__ADSPBF518__)

	/* normally we calculate the timing value based on CCLK and SCLK, 
		but for BF51x it should always be 0x7 */
	u32OTP_TP2 = 0x7 << 8;
	
#endif
	
	pDevice->OTPTimingValue = u32OTP_TP3 | u32OTP_TP2 | u32OTP_TP1;

	/* return */
	return(Result);
}




/*********************************************************************
*
*	Function:		adi_pdd_Close
*
*	Description:	Closes down an OTP device
*
*********************************************************************/


static u32 adi_pdd_Close(		/* Closes a device */
	ADI_DEV_PDD_HANDLE PDDHandle	/* PDD handle */
) {

	u32 			Result;		/* return value */
	ADI_OTP			*pDevice;	/* pointer to the device we're working on */

	/* be optimistic */
	Result = ADI_DEV_RESULT_SUCCESS;

	/* check for errors if required */
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	/* avoid casts */
	pDevice = (ADI_OTP *)PDDHandle;

	/* let everyone know this device can be used */
	pDevice->InUseFlag = FALSE;

	/* shut down error reporting */
	Result = adi_pdd_Control(PDDHandle, ADI_OTP_CMD_CLOSE, (void *)NULL);

    /* return */
	return(Result);
}





/*********************************************************************
*
*	Function:		adi_pdd_Read
*
*	Description:	Called to receive data in from the device
*
*********************************************************************/

static u32 adi_pdd_Read(		/* reads data in or queues inbound buffers to a device */
	ADI_DEV_PDD_HANDLE 	PDDHandle,	/* PDD handle */
	ADI_DEV_BUFFER_TYPE	BufferType,	/* buffer type */
	ADI_DEV_BUFFER 		*pBuffer	/* pointer to buffer */
){

	u32 				Result;		/* return value */
	ADI_OTP				*pDevice;	/* pointer to the device we're working on */
	ADI_OTP_INFO		*pOTPParams;/* parameters needed for read/write */
	ADI_DEV_1D_BUFFER 	*pBuff1D;			/* buffer pointer */
	u64					*page_content;
	u32 				parity_field = 0;
	u32 				ecc_code = 0;

	/* be optimistic */
	Result = ADI_DEV_RESULT_SUCCESS;

	/* check for errors if required */
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	/* avoid casts */
	pDevice = (ADI_OTP *)PDDHandle;

	/* cast our buffer to a 1D buffer */
	pBuff1D = (ADI_DEV_1D_BUFFER*)pBuffer;

	/* get a pointer to our page and flag parameters */
	pOTPParams = (ADI_OTP_INFO *)pBuff1D->pAdditionalInfo;

	/* get a pointer to our data */
	page_content = (u64 *)pBuff1D->Data;

	Result = bfrom_OtpRead(pOTPParams->page, pOTPParams->flags, page_content);

	/* return */
	return(Result);
}



/*********************************************************************
*
*	Function:		adi_pdd_Write
*
*	Description:	Called to transmit data out through the device
*
*********************************************************************/

static u32 adi_pdd_Write(		/* writes data out or queues outbound buffers to a device */
	ADI_DEV_PDD_HANDLE 	PDDHandle,	/* PDD handle */
	ADI_DEV_BUFFER_TYPE	BufferType,	/* buffer type */
	ADI_DEV_BUFFER 		*pBuffer	/* pointer to buffer */
){

	u32 				Result;			/* return value */
	ADI_OTP				*pDevice;		/* pointer to the device we're working on */
	ADI_OTP_INFO		*pOTPParams;	/* parameters needed for read/write */
	ADI_DEV_1D_BUFFER 	*pBuff1D;		/* buffer pointer */
	u64					*page_content;
	u32 write_timing = 0;
	u32 parity_field = 0;
	u64 temp_storage = 0;
	u32 ones_mask = 1;
	u32 ones_count = 0;
	u32 i = 0;
	u32 ecc_value = 0;

	/* be optimistic */
	Result = ADI_DEV_RESULT_SUCCESS;
		
	/* check for errors if required */
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	/* avoid casts */
	pDevice = (ADI_OTP *)PDDHandle;

	/* cast our buffer to a 1D buffer */
	pBuff1D = (ADI_DEV_1D_BUFFER*)pBuffer;

	/* get a pointer to our page and flag parameters */
	pOTPParams = (ADI_OTP_INFO *)pBuff1D->pAdditionalInfo;

	/* get a pointer to our data */
	page_content = (u64 *)pBuff1D->Data;

#if defined(__ADSPBF523__) || defined(__ADSPBF525__) || defined(__ADSPBF527__)

	/* if silicon rev is 0.0, OTP writes are not supported  */
	if( *pBK_REVISION == 0xAD020101 )
		Result = ADI_OTP_SILICON_REV_NOT_SUPPORTED;
	else
		Result = bfrom_OtpWrite(pOTPParams->page, pOTPParams->flags, page_content);
		
#else

	Result = bfrom_OtpWrite(pOTPParams->page, pOTPParams->flags, page_content);
	
#endif
	
	/* return */
	return(Result);
}


/*********************************************************************
*
*	Function:		adi_pdd_Control
*
*	Description:	Configures the OTP device
*
*********************************************************************/

static u32 adi_pdd_Control(		/* Sets or senses a device specific parameter */
	ADI_DEV_PDD_HANDLE 	PDDHandle,	/* PDD handle */
	u32 				Command,	/* command ID */
	void 				*Value		/* command specific value */
) {

	ADI_OTP *pDevice;		/* pointer to the device we're working on */
	u32     Result;			/* return value */
	u32     *pu32Value;		/* Value converted to a u32 type to avoid casts/warnings etc. */

	/* check for errors if required */
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	/* avoid casts */
	pDevice = (ADI_OTP *)PDDHandle;
	pu32Value = (u32 *)Value;

	/* assume we're going to be successful */
	Result = ADI_DEV_RESULT_SUCCESS;

	/* CASEOF (Command ID) */
	switch (Command) {

		/* CASE ( error reporting) */
		case (ADI_DEV_CMD_SET_ERROR_REPORTING):

		    /* don't do anything if error reporting is not changing */
			if (*pu32Value == pDevice->ErrorReportingFlag) {
    			break;
			}

			/* enable/disable error reporting as appropriate for the device */

			/* update status */
			pDevice->ErrorReportingFlag = *pu32Value;
			break;

		/* CASE (control dataflow) */
		case (ADI_DEV_CMD_SET_DATAFLOW):

			/* IF (really changing the dataflow) */
			if (pDevice->DataFlowEnabledFlag != *pu32Value) {

    			/* set dataflow flag */
    			pDevice->DataFlowEnabledFlag = *pu32Value;
			}
    		/* ENDIF */
			break;

		case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
        	/* Do nothing & simply return back for these commands */
            break;

		/* CASE (query for processor DMA support) */
		case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

			/* uncomment the proper response; the driver is supported (TRUE) or is not supported (FALSE) */
			 *((u32 *)Value) = FALSE;
			break;

		case (ADI_OTP_CMD_CLOSE):
		
#if defined(__ADSPBF523__) || defined(__ADSPBF525__) || defined(__ADSPBF527__)

			/* if silicon rev is 0.0, OTP close is not supported  
			   we make the call anyway and don't use the return value.
			   The call just does an RTS	*/
			if( *pBK_REVISION == 0xAD020101 )
				bfrom_OtpCommand( OTP_CLOSE, *pu32Value );
			else
				/* send the OTP_CLOSE command with some value */
				Result = bfrom_OtpCommand( OTP_CLOSE, *pu32Value );
				
#else
			/* send the OTP_CLOSE command with some value */
			Result = bfrom_OtpCommand( OTP_CLOSE, *pu32Value );
			
#endif
			break;

		case (ADI_OTP_CMD_SET_ACCESS_MODE):

#if defined(__ADSPBF523__) || defined(__ADSPBF525__) || defined(__ADSPBF527__)

			/* if silicon rev is 0.0, OTP init is not supported  
			   we make the call anyway and don't use the return value.
			   The call just does an RTS	*/
			if( *pBK_REVISION == 0xAD020101 )
				bfrom_OtpCommand( OTP_INIT, pDevice->OTPTimingValue );
			else
				/* send the OTP_INIT command with some value */
				Result = bfrom_OtpCommand( OTP_INIT, pDevice->OTPTimingValue );
				
#else
				/* send the OTP_INIT command with some value */
				Result = bfrom_OtpCommand( OTP_INIT, pDevice->OTPTimingValue );
				
#endif
			break;

		case (ADI_OTP_CMD_SET_TIMING):

			pDevice->OTPTimingValue = *pu32Value;

#if defined(__ADSPBF523__) || defined(__ADSPBF525__) || defined(__ADSPBF527__)

			/* if silicon rev is 0.0, OTP init is not supported  */
			if( *pBK_REVISION == 0xAD020101 )
				Result = ADI_OTP_SILICON_REV_NOT_SUPPORTED;
			else
				/* send the OTP_INIT command with some value */
				Result = bfrom_OtpCommand( OTP_INIT, pDevice->OTPTimingValue );
				
#else
				/* send the OTP_INIT command with some value */
				Result = bfrom_OtpCommand( OTP_INIT, pDevice->OTPTimingValue );
#endif
			break;

		/* CASEELSE */
		default:

			/* the driver doesn't support this command */
			Result = ADI_DEV_RESULT_NOT_SUPPORTED;

	/* ENDCASE */
	}

	/* return */
	return(Result);
}



#if defined(ADI_DEV_DEBUG)

/*********************************************************************

	Function:		ValidatePDDHandle

	Description:	Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {

    u32 i;  /* counter; */

    /* assume the worst */
    Result = ADI_DEV_RESULT_BAD_PDD_HANDLE;

    /* validate that the handle is indeed a device entry */
	for (i = 0; i < ADI_OTP_NUM_DEVICES; i++) {
		if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
			Result = ADI_DEV_RESULT_SUCCESS;
			break;
		}
	}

	/* return the result */
	return (Result);
}

#endif

