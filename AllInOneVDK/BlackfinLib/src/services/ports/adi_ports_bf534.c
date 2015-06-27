/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_ports_bf534.c,v $
$Revision: 3923 $
$Date: 2010-12-08 09:23:28 -0500 (Wed, 08 Dec 2010) $

Description:
			Port Configuration module for the System Services Library

*********************************************************************************/

#include <services/services.h>

// These macros can be used to pack/unpack the port control information into/from a 16 bit word.
// We can then replace all the tables with (anon) enums/macros


/*********************************************************************************
* Port index
*********************************************************************************/
#define ADI_FLAG_PORTF		0
#define	ADI_FLAG_PORTG		1
#define	ADI_FLAG_PORTH		2
#define	ADI_FLAG_PORTJ		3

/*********************************************************************************
* Port Mapping requiried for each peripheral type
*********************************************************************************/

#pragma alignment_region (2)
static u8 ppi_ports[]   = { ADI_FLAG_PORTF, ADI_FLAG_PORTG };
static u8 spi_ports[]   = { ADI_FLAG_PORTF };
static u8 sport_ports[] = { ADI_FLAG_PORTF, ADI_FLAG_PORTG };
static u8 uart_ports[]  = { ADI_FLAG_PORTF };
static u8 can_ports[]   = { ADI_FLAG_PORTJ };
static u8 tmr_ports[]   = { ADI_FLAG_PORTF };
static u8 gpio_ports[]  = { ADI_FLAG_PORTF, ADI_FLAG_PORTG, ADI_FLAG_PORTH };
#pragma alignment_region_end

#pragma alignment_region (4)
// PPI
static volatile u16 *ppi_portx_fer[]   = { pPORTF_FER, pPORTG_FER };

// SPI
static volatile u16 *spi_portx_fer[]   = { pPORTF_FER};

// SPORT
static volatile u16 *sport_portx_fer[]   = { pPORTF_FER, pPORTG_FER };

// UART
static volatile u16 *uart_portx_fer[]   = { pPORTF_FER };

// GP Timers
static volatile u16 *tmr_portx_fer[]   = { pPORTF_FER };

// GPIO flags
static volatile u16 *gpio_portx_fer[]   = { pPORTF_FER, pPORTG_FER, pPORTH_FER };
#pragma alignment_region_end

/*********************************************************************************
* The following arrays are used to determine if a pin has already been assigned
to another function
*********************************************************************************/
#define NUM_PINS_PORT 16
#define NUM_PORTS     4

/*********************************************************************************
* table to hold mask values for PORT_MUX and PORTF_FER registers based on the
* PPI_CONTROL DLEN bit field values
*********************************************************************************/
typedef struct {
	u16	PORT_MUXMask;			// PORT_MUX bits
	u16 PORTG_FERMask;			// PORTG_FER bits
} ADI_PORTS_PORT_CONFIG;

// PPI port config table
static ADI_PORTS_PORT_CONFIG PPIPortConfigTable[] = {
	{	0x0000, 0x00ff	},								// DLEN = 0 (8 bit data width)
	{	0x0200, 0x03ff	},								// DLEN = 1 (10 bit data width)
	{	0x0600, 0x07ff	},								// DLEN = 2 (11 bit data width)
	{	0x0600, 0x0fff	},								// DLEN = 3 (12 bit data width)
	{	0x0600, 0x1fff	},								// DLEN = 4 (13 bit data width)
	{	0x0e00, 0x3fff	},								// DLEN = 5 (14 bit data width)
	{	0x0e00, 0x7fff	},								// DLEN = 6 (15 bit data width)
	{	0x0e00, 0xffff	},								// DLEN = 7 (16 bit data width)
};


/*********************************************************************************
*							PRIVATE FUNCTION PROTOTYPES
*********************************************************************************/
static void assignPortMMR(volatile u16 *reg, u16 value);
static u32 GetNumFrameSyncs(void);
static void SetPortRegs(u8 *dev_ports, u32 nports, volatile u16 **dev_portx_fer,
u16 *port_masks, u16 port_mux_set, u16 port_mux_clr, u8 Enable);
static u16 GetDatalength(void);

static void *adi_ports_pEnterCriticalArg;

/*********************************************************************************
*							PUBLIC API FUNCTIONS
*********************************************************************************/

/* ****************************************************************************
 * Function: adi_ports_Init
 * Description: initializes the ports service (placeholder)
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_Init(
	void	*pCriticalRegionArg				// parameter for critical region function
)
{
	adi_ports_pEnterCriticalArg = pCriticalRegionArg;
	return ADI_PORTS_RESULT_SUCCESS;
}

/* ****************************************************************************
 * Function: adi_tmr_Terminate
 * Description: Terminates the ports service (placeholder)
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_Terminate(			// terminates the Ports service
	void
)
{
	return ADI_PORTS_RESULT_SUCCESS;
}


/* ****************************************************************************
 * Function: adi_ports_EnablePPI
 * Description: Configure GPIO pins For PPI use
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_EnablePPI(
				   u32 *Directives,
				   u32 nDirectives,
				   u32 Enable
				   )
{
	u32	i, Result;
	// port_masks array contains the accumulation  of changes to the PORTx_FER registers
	u16	port_masks[3] = {0,0,0};
	// port_mux_set contains the accumulation of bits to set in PORT_MUX to enable function
	u16	port_mux_set = 0;
	// port_mux_clr contains the accumulation of bits to clr in PORT_MUX to enable function
	u16	port_mux_clr  = 0;

#ifdef ADI_SSL_DEBUG
	// Test that array of directives is valid
	if (!Directives)
		return ADI_PORTS_RESULT_NULL_ARRAY;
#endif

	// Iterate around each Directive
	for (i=0;i<nDirectives;i++)
	{
		u32 tmrclk[] = { ADI_PORTS_DIR_TMR_CLK };
		u16 dlen;
		ADI_PORTS_PORT_CONFIG *pEntry;

		// Now we take action on each directive
		switch(Directives[i])
		{
#ifdef ADI_SSL_DEBUG
			// CASE (Directive not recognized for peripheral)
			default:
				return ADI_PORTS_RESULT_BAD_DIRECTIVE;

#endif

			// CASE ( Base PPI operation )
			case ADI_PORTS_DIR_PPI_BASE:
				switch ( GetNumFrameSyncs() ) {
					case 2:
						port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_PPI_FS2)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_PPI_FS2);
					case 1:
						port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_PPI_FS1)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_PPI_FS1);
						break;
				}
				// Enable PPI_CLK pin as input
				adi_ports_EnableTimer(tmrclk, 1, Enable);

				dlen = GetDatalength();
				pEntry = &PPIPortConfigTable[dlen];
				// Set bits in masks for the required number of bits
				port_mux_clr |= pEntry->PORT_MUXMask;
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_PPI_D0)] |= pEntry->PORTG_FERMask;
				break;

			// CASE ( Enable pin for 3rd PPI frame sync )
			case ADI_PORTS_DIR_PPI_FS3:
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_PPI_FS3)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_PPI_FS3);
				port_mux_set |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_PPI_FS3) ;
				break;
		}
	}

	// Set/clr bits in PORTx_FER and PORT_MUX registers
	SetPortRegs(ppi_ports, sizeof(ppi_ports)/sizeof(u8), ppi_portx_fer ,port_masks, port_mux_set, port_mux_clr, Enable);

	return ADI_PORTS_RESULT_SUCCESS;
}

/* ****************************************************************************
 * Function: adi_ports_EnableSPI
 * Description: Configure GPIO pins For SPI use
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_EnableSPI(
				   u32 *Directives,
				   u32 nDirectives,
				   u32 Enable
				   )
{
	u32	i,j;
	ADI_PORTS_RESULT Result;
	// port_masks array contains the accumulation  of changes to the PORTx_FER registers
	u16	port_masks[3] = {0,0,0};
	// port_mux_set contains the accumulation of bits to set in PORT_MUX to enable function
	u16	port_mux_set = 0;
	u16	port_mux_clr = 0;
	u8 spi_flg;

#ifdef ADI_SSL_DEBUG
	// Test that array of directives is valid
	if (!Directives)
		return ADI_PORTS_RESULT_NULL_ARRAY;
#endif

	// Iterate around each Directive
	for (i=0;i<nDirectives;i++)
	{
		// Now we take action on each directive
		switch(Directives[i])
		{
#ifdef ADI_SSL_DEBUG
			// CASE (Directive not recognized for peripheral)
			default:
				return ADI_PORTS_RESULT_BAD_DIRECTIVE;
#endif


			// CASE ( Base SPI operation )
			case ADI_PORTS_DIR_SPI_BASE:
				// Configure pins
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPI_SPISS)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPI_SPISS);
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPI_SCK)]   |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPI_SCK);
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPI_MOSI)]  |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPI_MOSI);
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPI_MISO)]  |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPI_MISO);

				// read in the current value of the SPI flag register
				// Fix applied to next line for TAR25519 (0x0e replaced with 0xfe)
				spi_flg = (u8)(*pSPI_FLG & 0xfe);

				// set port control for required bits
				if (spi_flg & 0x02) {
					port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPI_SSEL1)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPI_SSEL1);
				}
				if ((spi_flg & 0x04) || (spi_flg & 0x08)) {
					port_mux_set |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPI_SSEL2);
				}
				if (spi_flg & 0x10) {
					port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPI_SSEL4)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPI_SSEL4);
					port_mux_set |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPI_SSEL4);
				}
				if (spi_flg & 0x20) {
					port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPI_SSEL5)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPI_SSEL5);
					port_mux_set |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPI_SSEL5);
				}
				if (spi_flg & 0x40) {
					port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPI_SSEL6)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPI_SSEL6);
					port_mux_set |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPI_SSEL6);
				}
				if (spi_flg & 0x80) {
					port_mux_set |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPI_SSEL7);
				}

				break;

			// CASE ( Enable pin for SPI Slave Select #1,4,5,6 )
			case ADI_PORTS_DIR_SPI_SLAVE_SELECT_1:
			case ADI_PORTS_DIR_SPI_SLAVE_SELECT_4:
			case ADI_PORTS_DIR_SPI_SLAVE_SELECT_5:
			case ADI_PORTS_DIR_SPI_SLAVE_SELECT_6:
				port_masks[ADI_PORTS_GET_PORT(Directives[i])]  |= ADI_PORTS_GET_PIN_MASK(Directives[i]);
				port_mux_set |= ADI_PORTS_GET_MUX_MASK(Directives[i]);
				break;

			// CASE ( Enable pin for SPI Slave Select #2,3,7 )
			// these are on PORT J and so only require PORT_MUX settings
			case ADI_PORTS_DIR_SPI_SLAVE_SELECT_7:
				port_mux_clr = 0x0006;
			case ADI_PORTS_DIR_SPI_SLAVE_SELECT_2:
			case ADI_PORTS_DIR_SPI_SLAVE_SELECT_3:
				port_mux_set |= ADI_PORTS_GET_MUX_MASK(Directives[i]);
				break;

		}
	}

	// Set/clr bits in PORTx_FER and PORT_MUX registers
	SetPortRegs(spi_ports, sizeof(spi_ports)/sizeof(u8), spi_portx_fer ,port_masks, port_mux_set, port_mux_clr, Enable);

	return ADI_PORTS_RESULT_SUCCESS;
}


/* ****************************************************************************
 * Function: adi_ports_EnableSPORT
 * Description: Configure GPIO pins For SPORT use
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_EnableSPORT(
				   u32 *Directives,
				   u32 nDirectives,
				   u32 Enable
				   )
{
	u32	i;
	ADI_PORTS_RESULT Result;
	// port_masks array contains the accumulation  of changes to the PORTx_FER registers
	u16	port_masks[3] = {0,0,0};
	// port_mux_set contains the accumulation of bits to set in PORT_MUX to enable function
	u16	port_mux_set = 0;
	// port_mux_clr contains the accumulation of bits to clr in PORT_MUX to enable function
	u16	port_mux_clr  = 0;
	u16 sport_cr2_reg;

#ifdef ADI_SSL_DEBUG
	// Test that array of directives is valid
	if (!Directives)
		return ADI_PORTS_RESULT_NULL_ARRAY;
#endif

	// Iterate around each Directive
	for (i=0;i<nDirectives;i++)
	{
		// Now we take action on each directive
		switch(Directives[i])
		{
#ifdef ADI_SSL_DEBUG
			// CASE (Directive not recognized for peripheral)
			default:
				return ADI_PORTS_RESULT_BAD_DIRECTIVE;
#endif

			// CASE ( Base SPORT0 TX operation )
			case ADI_PORTS_DIR_SPORT0_BASE_TX:
				// The following covers requirements for TSCLK0 and DT0PRI
				port_mux_clr |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPORT0_TSCLK);
				// Configure secondary data pin if required
				sport_cr2_reg = *pSPORT0_TCR2;
				if ( (sport_cr2_reg&TXSE)==TXSE )
					port_mux_clr |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPORT0_DTSEC);
				break;

			// CASE ( Base SPORT0 RX operation )
			case ADI_PORTS_DIR_SPORT0_BASE_RX:
				// Configure secondary data pin if required
				sport_cr2_reg = *pSPORT0_RCR2;
				if ( (sport_cr2_reg&RXSE)==RXSE )
					port_mux_clr |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPORT0_DRSEC);
				break;

			// CASE ( Base SPORT1 RX operation )
			case ADI_PORTS_DIR_SPORT1_BASE_RX:
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPORT1_RSCLK)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPORT1_RSCLK);
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPORT1_RFS)]   |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPORT1_RFS);
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPORT1_DRPRI)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPORT1_DRPRI);
				// The following covers requirements for all three pins
				port_mux_set |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPORT1_RSCLK);
				// Configure secondary data pin if required
				sport_cr2_reg = *pSPORT1_RCR2;
				if ( (sport_cr2_reg&RXSE)==RXSE ) {
					port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPORT1_DRSEC)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPORT1_DRSEC);
					port_mux_set |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPORT1_DRSEC);
				}
				break;

			// CASE ( Base SPORT1 TX operation )
			case ADI_PORTS_DIR_SPORT1_BASE_TX:
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPORT1_TSCLK)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPORT1_TSCLK);
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPORT1_TFS)]   |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPORT1_TFS);
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPORT1_DTPRI)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPORT1_DTPRI);
				// The following covers requirements for all three pins
				port_mux_set |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPORT1_TSCLK);
				// Configure secondary data pin if required
				sport_cr2_reg = *pSPORT1_TCR2;
				if ( (sport_cr2_reg&TXSE)==TXSE ) {
					port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_SPORT1_DTSEC)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_SPORT1_DTSEC);
					port_mux_set |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPORT1_DTSEC);
				}
				break;

			// CASE ( Enable pin for SPORT0 RX secondary data )
			case ADI_PORTS_DIR_SPORT0_RXSE:
			// CASE ( Enable pin for SPORT0 TX secondary data )
			case ADI_PORTS_DIR_SPORT0_TXSE:
				port_mux_clr |= ADI_PORTS_GET_MUX_MASK(ADI_PORTS_SPORT0_DRSEC);
				break;

			// CASE ( Enable pin for SPORT1 RX secondary data )
			case ADI_PORTS_DIR_SPORT1_RXSE:
			// CASE ( Enable pin for SPORT1 TX secondary data )
			case ADI_PORTS_DIR_SPORT1_TXSE:
				port_masks[ADI_PORTS_GET_PORT(Directives[i])] |= ADI_PORTS_GET_PIN_MASK(Directives[i]);
				port_mux_set |= ADI_PORTS_GET_MUX_MASK(Directives[i]);
				break;
		}
	}

	// Set/clr bits in PORTx_FER and PORT_MUX registers
	SetPortRegs(sport_ports, sizeof(sport_ports)/sizeof(u8), sport_portx_fer ,port_masks, port_mux_set, port_mux_clr, Enable);

	return ADI_PORTS_RESULT_SUCCESS;
}

/* ****************************************************************************
 * Function: adi_ports_EnableUART
 * Description: Configure GPIO pins For UART use
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_EnableUART(
				   u32 *Directives,
				   u32 nDirectives,
				   u32 Enable
				   )
{
	u32	i;
	ADI_PORTS_RESULT Result;
	// port_masks array contains the accumulation  of changes to the PORTx_FER registers
	u16	port_masks[1] = {0};
	// port_mux_clr contains the accumulation of bits to clr in PORT_MUX to enable function
	u16	port_mux_clr  = 0;

#ifdef ADI_SSL_DEBUG
	// Test that array of directives is valid
	if (!Directives)
		return ADI_PORTS_RESULT_NULL_ARRAY;
#endif

	// Iterate around each Directive
	for (i=0;i<nDirectives;i++)
	{
#ifndef ADI_SSL_DEBUG
		port_mux_clr |= ADI_PORTS_GET_MUX_MASK(Directives[i]);
		port_masks[ADI_PORTS_GET_PORT(Directives[i])] |= ADI_PORTS_GET_PIN_MASK(Directives[i]);
#else
		switch(Directives[i])
		{
			// CASE (Directive not recognized for peripheral)
			default:
				return ADI_PORTS_RESULT_BAD_DIRECTIVE;


			// CASE ( UART0 RX operation )
			case ADI_PORTS_DIR_UART0_RX:
			// CASE ( UART1 RX operation )
			case ADI_PORTS_DIR_UART1_RX:
			// CASE ( UART0 TX operation )
			case ADI_PORTS_DIR_UART0_TX:
			// CASE ( UART1 TX operation )
			case ADI_PORTS_DIR_UART1_TX:
				port_mux_clr |= ADI_PORTS_GET_MUX_MASK(Directives[i]);
				port_masks[ADI_PORTS_GET_PORT(Directives[i])] |= ADI_PORTS_GET_PIN_MASK(Directives[i]);
				break;

		}
#endif
	}

	// Set/clr bits in PORTx_FER and PORT_MUX registers
	SetPortRegs(uart_ports, sizeof(uart_ports)/sizeof(u8), uart_portx_fer ,port_masks, 0, port_mux_clr, Enable);

	return ADI_PORTS_RESULT_SUCCESS;
}

/* ****************************************************************************
 * Function: adi_ports_EnableCAN
 * Description: Configure GPIO pins For CAN use
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_EnableCAN(
				   u32 *Directives,
				   u32 nDirectives,
				   u32 Enable
				   )
{
	u32	i;
	// port_mux_set contains the accumulation of bits to set in PORT_MUX to enable function
	u16	port_mux_set = 0;
	u16	port_mux_clr = 0;

#ifdef ADI_SSL_DEBUG
	// Test that array of directives is valid
	if (!Directives)
		return ADI_PORTS_RESULT_NULL_ARRAY;
#endif

	// Iterate around each Directive
	for (i=0;i<nDirectives;i++)
	{
#ifndef ADI_SSL_DEBUG
		port_mux_clr = 0x0006;
		port_mux_set |= ADI_PORTS_GET_MUX_MASK(Directives[i]);
#else
		switch(Directives[i])
		{
			// CASE (Directive not recognized for peripheral)
			default:
				return ADI_PORTS_RESULT_BAD_DIRECTIVE;

			// CASE ( CAN RX operation )
			case ADI_PORTS_DIR_CAN_RX:
			// CASE ( CAN TX operation )
			case ADI_PORTS_DIR_CAN_TX:
				port_mux_clr = 0x0006;
				port_mux_set |= ADI_PORTS_GET_MUX_MASK(Directives[i]);

				break;

		}
#endif
	}


	// Set/clr bits in PORTx_FER and PORT_MUX registers
	SetPortRegs(NULL, 0, 0, NULL, port_mux_set, port_mux_clr, Enable);

	return ADI_PORTS_RESULT_SUCCESS;
}

/* ****************************************************************************
 * Function: adi_ports_EnableTimer
 * Description: Configure a GPIO PIN For Timer use
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_EnableTimer(
				   u32 *Directives,			// array of directives
				   u32 nDirectives,			// number of directives
				   u32 Enable				// Enable(1)/Disable(0) flag
				   )
{
	u32	i;
	ADI_PORTS_RESULT Result;
	// port_masks array contains the accumulation  of changes to the PORTx_FER registers
	u16	port_masks[1] = {0};
	// port_mux_set contains the accumulation of bits to set in PORT_MUX to enable function
	u16	port_mux_set = 0;
	// port_mux_clr contains the accumulation of bits to clr in PORT_MUX to enable function
	u16	port_mux_clr  = 0;

#ifdef ADI_SSL_DEBUG
	// Test that array of directives is valid
	if (!Directives)
		return ADI_PORTS_RESULT_NULL_ARRAY;
#endif

	// Iterate around each Directive
	for (i=0;i<nDirectives;i++)
	{
		u16 itmr;
		switch(Directives[i]) {

#ifdef ADI_SSL_DEBUG
			default:
				return ADI_PORTS_RESULT_BAD_DIRECTIVE;
#endif
			// CASE ( TMR_CLK pin)
			case ADI_PORTS_DIR_TMR_CLK:
				// Set mask for TMRCLK pin
				port_masks[ADI_PORTS_GET_PORT(ADI_PORTS_TIMER_TMR_CLK)] |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_TIMER_TMR_CLK);
				// Set bits in PORTFIO_DIR and PORTFIO_INEN registers to enable TMRCLK pin for input
				// SAR-50067: DIR sense was reversed for timer clock as input
				*pPORTFIO_DIR &= ~(ADI_PORTS_GET_PIN_MASK(ADI_PORTS_TIMER_TMR_CLK));
				*pPORTFIO_INEN |= ADI_PORTS_GET_PIN_MASK(ADI_PORTS_TIMER_TMR_CLK);
				break;

			// CASE ( GP timers 0-1 )
			case ADI_PORTS_DIR_TMR_0:
			case ADI_PORTS_DIR_TMR_1:
				// No setting in PORT_MUX, but need to disable for GPIO
				break;

			// CASE ( GP timers 2-5 pin)
			case ADI_PORTS_DIR_TMR_2:
			case ADI_PORTS_DIR_TMR_3:
			case ADI_PORTS_DIR_TMR_4:
			case ADI_PORTS_DIR_TMR_5:
				port_mux_clr |= ADI_PORTS_GET_MUX_MASK(Directives[i]);
				break;
				// Setting required in PORT_MUX, and need to disable for GPIO

			// CASE ( GP timers 6-7 pin)
			case ADI_PORTS_DIR_TMR_6:
			case ADI_PORTS_DIR_TMR_7:
				// Setting required in PORT_MUX, and need to disable for GPIO
				port_mux_set |= ADI_PORTS_GET_MUX_MASK(Directives[i]);
				break;

#if 0
			// CASE ( Alternate Timer Clocks)
			case ADI_PORTS_DIR_TACLK_1:
			case ADI_PORTS_DIR_TACLK_2:
			case ADI_PORTS_DIR_TACLK_3:
			case ADI_PORTS_DIR_TACLK_4:
			case ADI_PORTS_DIR_TACLK_0:
			case ADI_PORTS_DIR_TACLK_5:
			case ADI_PORTS_DIR_TACLK_6:
			case ADI_PORTS_DIR_TACLK_7:
				// The alternate clock inputs are not gated by any Function Enable
				// or multiplexer register
				continue;

			case ADI_PORTS_DIR_TACI_0: // TACI0 is for CAN RX autobaud detect
			case ADI_PORTS_DIR_TACI_1: // TACI1 is for UART0 RX autobaud detect
			case ADI_PORTS_DIR_TACI_6: // TACI6 is for UART1 RX autobaud detect
				// The alternate caputure inputs are enabled as a result of enabling
				// the relevant periperal. eg, once UART0 RX pin is enabled, then the
				// signal can be simultaneously captured by Timer 1 through TACI1
				continue;
#endif
		}
		port_masks[ADI_PORTS_GET_PORT(Directives[i])] |= ADI_PORTS_GET_PIN_MASK(Directives[i]);
	}

	// Set/clr bits in PORTx_FER and PORT_MUX registers
	SetPortRegs(tmr_ports, sizeof(tmr_ports)/sizeof(u8), tmr_portx_fer ,port_masks, port_mux_set, port_mux_clr, Enable);

	return ADI_PORTS_RESULT_SUCCESS;
}


/* ****************************************************************************
 * Function: adi_ports_EnableGPIO
 * Description: Configure GPIO pins For GPIO use
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_EnableGPIO(
				   u32 *Directives,			// array of directives
				   u32 nDirectives,			// number of directives
				   u32 Enable				// Enable(1)/Disable(0) flag
				   )
{
	u32	i;
	// port_masks array contains the accumulation  of changes to the PORTx_FER registers
	u16	port_masks[3] = {0,0,0};

#ifdef ADI_SSL_DEBUG
	// Test that array of directives is valid
	if (!Directives)
		return ADI_PORTS_RESULT_NULL_ARRAY;
#endif

	// Iterate around each Directive
	for (i=0;i<nDirectives;i++)
	{
		port_masks[ADI_PORTS_GET_PORT(Directives[i])] |= ADI_PORTS_GET_PIN_MASK(Directives[i]);
	}

	// Set/clr bits in PORTx_FER and PORT_MUX registers
	// (note, Enable is inverted as the desired effect is to clear the PORTx_FER regs to enable GPIO)
	SetPortRegs(gpio_ports, sizeof(gpio_ports)/sizeof(u8), gpio_portx_fer ,port_masks, 0, 0, !Enable);

	return ADI_PORTS_RESULT_SUCCESS;
}

/* ****************************************************************************
 * Function: adi_ports_GetProfile
 * Description: Retrieves the PORTx_FER and PORT_MUX values into the given structure
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_GetProfile(		// Retrieves the profile information
				   ADI_PORTS_PROFILE *profile			// profile structure
				   )
{
	void *pExitCriticalArg;	// exit critical region parameter

#ifdef ADI_SSL_DEBUG
	// Test that profile structure is a valid address
	if (!profile)
		return ADI_PORTS_RESULT_FAILED;
#endif

	// protect this region
	pExitCriticalArg = adi_int_EnterCriticalRegion(adi_ports_pEnterCriticalArg);

	// Copy the MMR values
	profile->portf_fer = *pPORTF_FER;
	profile->portg_fer = *pPORTG_FER;
	profile->porth_fer = *pPORTH_FER;
	profile->port_mux  = *pPORT_MUX;

	// unprotect
	adi_int_ExitCriticalRegion(pExitCriticalArg);

	return ADI_PORTS_RESULT_SUCCESS;
}

/* ****************************************************************************
 * Function: adi_ports_SetProfile
 * Description: Applies the PORTx_FER and PORT_MUX values defined in the given structure
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_SetProfile(		// Applies a profile
				   ADI_PORTS_PROFILE *profile			// profile structure
				   )
{
	void *pExitCriticalArg;	// exit critical region parameter

#ifdef ADI_SSL_DEBUG
	// Test that profile structure is a valid address
	if (!profile)
		return ADI_PORTS_RESULT_FAILED;
#endif

	// protect this region
	pExitCriticalArg = adi_int_EnterCriticalRegion(adi_ports_pEnterCriticalArg);

	// Assign the MMR values
	// Fix applied in following for TAR 25451
	assignPortMMR(pPORTF_FER, profile->portf_fer);
	assignPortMMR(pPORTG_FER, profile->portg_fer);
	assignPortMMR(pPORTH_FER, profile->porth_fer);
	assignPortMMR(pPORT_MUX, profile->port_mux);

	// unprotect
	adi_int_ExitCriticalRegion(pExitCriticalArg);

	return ADI_PORTS_RESULT_SUCCESS;
}

/* ****************************************************************************
 * Function: adi_ports_ClearProfile
 * Description: Clears the PORTx_FER and PORT_MUX values defined in the given structure
 * ****************************************************************************
 */
ADI_PORTS_RESULT
adi_ports_ClearProfile(		// Clears a profile
				   ADI_PORTS_PROFILE *profile			// profile structure
				   )
{
	void *pExitCriticalArg;	// exit critical region parameter

#ifdef ADI_SSL_DEBUG
	// Test that profile structure is a valid address
	if (!profile)
		return ADI_PORTS_RESULT_FAILED;
#endif

	// protect this region
	pExitCriticalArg = adi_int_EnterCriticalRegion(adi_ports_pEnterCriticalArg);

	// Assign the MMR values as the
	// Fix applied in following for TAR 25451
	assignPortMMR(pPORTF_FER, ~profile->portf_fer);
	assignPortMMR(pPORTG_FER, ~profile->portg_fer);
	assignPortMMR(pPORTH_FER, ~profile->porth_fer);
	assignPortMMR(pPORT_MUX, ~profile->port_mux);

	// unprotect
	adi_int_ExitCriticalRegion(pExitCriticalArg);

	return ADI_PORTS_RESULT_SUCCESS;
}


/*********************************************************************************
*									PRIVATE FUNCTIONS
*********************************************************************************/


/* ****************************************************************************
 * Function: assignPortMMR
 * Description: Assigns either PORT_MUX or PORTx_FER regs (taking account of
 *				silicon anomaly - which is the reason to use a fn call here)
 * ****************************************************************************
 */
static void inline
assignPortMMR(volatile u16 *reg, u16 value)
{
	*reg = value;
#if (__SILICON_REVISION__==0xffff) || (__SILICON_REVISION__<0x0005)
	*reg = value;
#endif
}


/* ****************************************************************************
 * Function: GetDatalength
 * Description: Interrogates PPI_CONTROL MMR and determines the data length
 * ****************************************************************************
 */
static u16
GetDatalength(void)
{
	u16 ppi_control = *pPPI_CONTROL;
	u16 dlen = (ppi_control&DLEN_16)>>11;
	return dlen;
}

/* ****************************************************************************
 * Function: GetNumFrameSyncs
 * Description: Interrogates PPI_CONTROL MMR and determines the number of frame
 *				syncs required
 * ****************************************************************************
 */
static u32
GetNumFrameSyncs(void)
{
	u16 FrameSyncs;
	u16 ppi_control = *pPPI_CONTROL;
	u16 port_dir = (ppi_control&PORT_DIR)>>1;
	u16 port_cfg = (ppi_control&PORT_CFG)>>4;
	u16 xfr_type = (ppi_control&XFR_TYPE)>>2;
	// IF (PPI is configured for input)
	if (port_dir == 0) {

		// IF (non ITU-R 656 mode)
		if (xfr_type == 3) {

			// compute frame syncs based on PORT_CFG
			switch (port_cfg) {
				case 0:
					FrameSyncs = 1;
					break;
				case 3:
					FrameSyncs = 0;
					break;
				default:
					FrameSyncs = 2; // has to be 2 otherwise user would have commanded us to 3
					break;
			}

		// ELSE
		} else {

			// no frame syncs in ITU-R 656 mode)
			FrameSyncs = 0;

		// ENDIF
		}

	// ELSE (PPI is configured for output)
	} else {

		// IF (xfr_type says we're using frame syncs)
		if (xfr_type == 3) {

			// compute frame syncs based on PORT_CFG
			switch (port_cfg) {
				case 0:
					FrameSyncs = 1;
					break;
				case 1:
					FrameSyncs = 2;	// has to be 2 otherwise user would have commanded us to 3
					break;
				case 3:
					FrameSyncs = 2;  //********???? what's the right answer
					break;
			}

		// ELSE
		} else {

			// xfr_type says we're sync-less
			FrameSyncs = 0;

		// ENDIF
		}

	// ENDIF
	}

		return FrameSyncs;
}

/* ****************************************************************************
 * Function: SetPortRegs
 * Description: Sets the PORT_MUX & PORTx_FER regs as required
 * ****************************************************************************
 */
static void
SetPortRegs(u8 *dev_ports, u32 nports, volatile u16 **dev_portx_fer,
u16 *port_masks, u16 port_mux_set, u16 port_mux_clr, u8 Enable)
{
	u32 i;
	void *pExitCriticalArg;	// exit critical region parameter
	// Copy the current PORTx_FER MMR contents
	u16	portx_fer[3];
	portx_fer[ADI_FLAG_PORTF] = *pPORTF_FER;
	portx_fer[ADI_FLAG_PORTG] = *pPORTG_FER;
	portx_fer[ADI_FLAG_PORTH] = *pPORTH_FER;

	// Copy the current PORT_MUX MMR contents
	u16 port_mux = *pPORT_MUX;

	// protect
	pExitCriticalArg = adi_int_EnterCriticalRegion(adi_ports_pEnterCriticalArg);

	// Set/Clr the required bits in the copy of PORT_MUX depending on whether device is enabled disabled
	if (Enable)
	{
		// clear the required bits in the copy of PORT_MUX
		port_mux &= ~port_mux_clr;
		// set the required bits in the copy of PORT_MUX
		port_mux |= port_mux_set;
		// Assign the PORT_MUX MMR
		assignPortMMR(pPORT_MUX,port_mux);
		// Change PORTx_FER values
		for (i=0;i<nports;i++) {
			portx_fer[dev_ports[i]] |= port_masks[dev_ports[i]];
			assignPortMMR(dev_portx_fer[dev_ports[i]],portx_fer[dev_ports[i]]);
		}
	}
	else
	{
		// clear the required bits in the copy of PORT_MUX
		port_mux |= port_mux_clr;
		// set the required bits in the copy of PORT_MUX
		port_mux &= ~port_mux_set;
		// Assign the PORT_MUX MMR
		assignPortMMR(pPORT_MUX,port_mux);
		// Change PORTx_FER values
		for (i=0;i<nports;i++) {
			portx_fer[dev_ports[i]] &= ~port_masks[dev_ports[i]];
			assignPortMMR(dev_portx_fer[dev_ports[i]],portx_fer[dev_ports[i]]);
		}
	}

	// unprotect
	adi_int_ExitCriticalRegion(pExitCriticalArg);
}

