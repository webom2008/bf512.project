/**********************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software 
you agree to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_uart_int.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This is the Interrupt-driven driver source code for the 
			UART peripheral.  This file creates a macro that states that 
			we are building the Interrupt-driven version of the UART driver.  
			It then includes the main UART driver source code file (adi_uart.c)
            
            See also adi_uart_dma.c for DMA-driven UART driver.
            
***********************************************************************/

/***********************************************************************

Interrupt DRIVEN UART macro define

***********************************************************************/

#define ADI_UART_INT
#include "adi_uart.c"      	/* uart driver source include */

