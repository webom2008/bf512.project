/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_dcb_integrity.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			Deferred Callback (DCB) manager module - Integrity implementation

*********************************************************************************/


/*********************************************************************************
* Function:		adi_dcb_RegisterISR
* Description:	Registers the ISR with the Core Event Controller.
*				This function is replaced with one (that does nothing) in the 
*				underlying OS when in use.
*********************************************************************************/
 
void
adi_dcb_RegisterISR( 
		u16					IvgLevel,
		ADI_INT_HANDLER_FN	Dispatcher,
		ADI_DCB_HANDLE		hServer
		)
{
	// Replace this comment with Integrity implementation 
}

/*********************************************************************************
* Function:		adi_dcb_Forward
* Description:	Substitute internal function to masqueraded for one which 
*				forwards a DCB queue entry to the underlying OS, using the VDK
*				model.
*				This internal function simply raises the appropriate interrupt
*********************************************************************************/

void 
adi_dcb_Forward( 
		ADI_DCB_ENTRY_HDR	*Entry, 
		u16					IvgLevel,
		u16					Priority
		)
{
	// Replace this comment with Integrity implementation 
}

