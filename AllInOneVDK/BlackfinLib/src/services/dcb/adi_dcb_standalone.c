/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_dcb_standalone.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			Deferred Callback (DCB) manager module - standalone implementation

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
	if (hServer)
		adi_int_CECHook ( IvgLevel, Dispatcher, (void*)hServer, 1 );
	else
		adi_int_CECUnhook ( IvgLevel, Dispatcher, (void*)hServer );
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
	// Simply raise the interrupt for the queues level 
	switch(IvgLevel) {
	case ik_ivg14:
		raise_intr(14);
		return;
	case ik_ivg13:
		raise_intr(13);
		return;
	case ik_ivg12:
		raise_intr(12);
		return;
	case ik_ivg11:
		raise_intr(11);
		return;
	case ik_ivg10:
		raise_intr(10);
		return;
	case ik_ivg9:
		raise_intr(9);
		return;
	case ik_ivg8:
		raise_intr(8);
		return;
	case ik_ivg7:
		raise_intr(7);
		return;
	default:
		return;
	}
}

