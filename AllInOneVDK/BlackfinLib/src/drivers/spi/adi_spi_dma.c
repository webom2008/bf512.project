/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_spi_dma.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This is the DMA-driven driver source code for the SPI peripheral.  This
			file creates a macro that states that we are building the DMA-driven 
			version of the SPI driver.  It then includes the main SPI driver source
			code file.  
			
			See also adi_spi_int.c for the interrupt driven version of the driver.  
			
*********************************************************************************/


/*********************************************************************

DMA DRIVEN SPI macro define

*********************************************************************/

#define ADI_SPI_DMA
#include "adi_spi.c"      	// spi driver includes	

