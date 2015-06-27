/*********************************************************************************

Copyright(c) 2008 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

Description:
			This file implements a basic OTP driver.

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/

#ifdef __ADSPBF518__
	#include <cdefBF518.h>
#elif __ADSPBF526__
	#include <cdefBF526.h>
#elif __ADSPBF527__
	#include <cdefBF527.h>
#elif __ADSPBF548__
	#include <cdefBF548.h>
#elif __ADSPBF548M__
	#include <cdefBF548m.h>
#endif
#include <bfrom.h>				/* Blackfin ROM include file */
#include <services_types.h>		/* basic type defines */
#include <otp_basic.h>			/* otp driver specific includes */


/*********************************************************************

Enumerations and defines

*********************************************************************/

/*************************************************
* ADSP-BF54x
*************************************************/

#if defined(__ADSPBF54x__)

static u32 g_OTP_Timing_Value =	0x0A009464;	/* OTP default timing value with SCLK=100MHz/VDDINT=1.0v */

#define OTP_TP3		0x0A008000				/* constant OTP_TP3 value */

#endif


/*************************************************
* ADSP-BF52x
*************************************************/

#if defined(__ADSPBF52x__)

static u32 g_OTP_Timing_Value =	0x0A548664;	/* OTP default timing value with 100 MHz SCLK */

#define OTP_TP3		0x0A548000				/* constant OTP_TP3 value */

#endif


/*************************************************
* ADSP-BF512, ADSP-BF514, ADSP-BF516, and ADSP-BF518
*************************************************/

#if defined(__ADSPBF512__) || defined(__ADSPBF514__) || defined(__ADSPBF516__) || defined(__ADSPBF518__)

static u32 g_OTP_Timing_Value =	0x15548732;	/* OTP default timing value with 50 MHz SCLK */

#define OTP_TP3		0x15548000				/* constant OTP_TP3 value */

#endif


/* All EZ-KITs have a 25Mhz oscillator, a custom board will have to change this
   depending on the value they use */
#define CLKIN_VAL	25


/*********************************************************************
*
*	Function:		otp_basic_Open
*
*	Description:	Opens the OTP interface for use.
*
*********************************************************************/

u32 otp_basic_Open(void)
{
	u32 			Result;					/* return value */

	u16				u16CCLK = CLKIN_VAL * ((*pPLL_CTL >> 9) & 0x3F);	/* calculate CCLK */
	u16				u16SCLK = u16CCLK / *pPLL_DIV;						/* calculate SCLK */
	float 			sclk_period = (1/(float)u16SCLK) * 1000;			/* calculate SCLK period */
	u32				u32OTP_TP1 = ( 1000 / sclk_period );				/* TP1 of timing value */
	u32				u32OTP_TP2 = 0;										/* TP2 of timing value */
	u32				u32OTP_TP3 = OTP_TP3;								/* TP3 of timing value */

	/* be optimistic */
	Result = ADI_OTP_RESULT_SUCCESS;

#if defined(__ADSPBF54x__)

	/* calculate the timing value based on CCLK and SCLK */
	u32OTP_TP2 = ( 400 / (u32)( 2 * sclk_period ) ) << 8;

	/* if silicon rev is 0.0, OTP is not supported */
	if( *pBK_REVISION == 0xAD010001 )
		Result = ADI_OTP_SILICON_REV_NOT_SUPPORTED;

#endif

#if defined(__ADSPBF52x__)

	/* calculate the timing value based on CCLK and SCLK */
	u32OTP_TP2 = ( 120 / (u32)( 2 * sclk_period ) ) << 8;

#endif

#if defined(__ADSPBF512__) || defined(__ADSPBF514__) || defined(__ADSPBF516__) || defined(__ADSPBF518__)

	/* normally we calculate the timing value based on CCLK and SCLK,
		but for BF51x it should always be 0x7 */
	u32OTP_TP2 = 0x7 << 8;

#endif

	g_OTP_Timing_Value = u32OTP_TP3 | u32OTP_TP2 | u32OTP_TP1;

	/* return */
	return (Result);
}


/*********************************************************************
*
*	Function:		otp_basic_Close
*
*	Description:	Closes down the OTP interface.
*
*********************************************************************/

u32 otp_basic_Close(void)
{
	u32 			Result;		/* return value */

	/* be optimistic */
	Result = ADI_OTP_RESULT_SUCCESS;

    /* return */
	return (Result);
}


/*********************************************************************
*
*	Function:		otp_basic_Read
*
*	Description:	Called to read data from OTP.
*
*********************************************************************/

u32 otp_basic_Read(u32 page, u32 flags, u64 *pData)
{
	u32 				Result;				/* return value */

	Result = bfrom_OtpRead(page, flags, pData);

	/* return */
	return (Result);
}


/*********************************************************************
*
*	Function:		otp_basic_Write
*
*	Description:	Called to write data to OTP.
*
*********************************************************************/

u32 otp_basic_Write(u32 page, u32 flags, u64 *pData)
{
	u32 				Result;			/* return value */

#if defined(__ADSPBF523__) || defined(__ADSPBF525__) || defined(__ADSPBF527__)

	/* if silicon rev is 0.0, OTP writes are not supported */
	if( *pBK_REVISION == 0xAD020101 )
		Result = ADI_OTP_SILICON_REV_NOT_SUPPORTED;
	else
		Result = bfrom_OtpWrite(page, flags, pData);

#else

	Result = bfrom_OtpWrite(page, flags, pData);

#endif

	/* return */
	return (Result);
}


/*********************************************************************
*
*	Function:		otp_basic_Control
*
*	Description:	Configures the OTP interface.
*
*********************************************************************/

u32 otp_basic_Control(u32 Command, void *Value)
{
	u32     Result;			/* return value */
	u32     *pu32Value;		/* Value converted to a u32 type to avoid casts/warnings etc. */


	/* avoid casts */
	pu32Value = (u32 *)Value;

	/* be optimistic */
	Result = ADI_OTP_RESULT_SUCCESS;

	/* switch on the command */
	switch (Command)
	{
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
				bfrom_OtpCommand( OTP_INIT, g_OTP_Timing_Value );
			else
				/* send the OTP_INIT command with some value */
				Result = bfrom_OtpCommand( OTP_INIT, g_OTP_Timing_Value );

#else
				/* send the OTP_INIT command with some value */
				Result = bfrom_OtpCommand( OTP_INIT, g_OTP_Timing_Value );

#endif
			break;

		case (ADI_OTP_CMD_SET_TIMING):

			g_OTP_Timing_Value = *pu32Value;

#if defined(__ADSPBF523__) || defined(__ADSPBF525__) || defined(__ADSPBF527__)

			/* if silicon rev is 0.0, OTP init is not supported  */
			if( *pBK_REVISION == 0xAD020101 )
				Result = ADI_OTP_SILICON_REV_NOT_SUPPORTED;
			else
				/* send the OTP_INIT command with some value */
				Result = bfrom_OtpCommand( OTP_INIT, g_OTP_Timing_Value );

#else
				/* send the OTP_INIT command with some value */
				Result = bfrom_OtpCommand( OTP_INIT, g_OTP_Timing_Value );
#endif
			break;

		default:

			/* the driver doesn't support this command */
			Result = ADI_OTP_RESULT_NOT_SUPPORTED;
	}

	/* return */
	return(Result);
}
