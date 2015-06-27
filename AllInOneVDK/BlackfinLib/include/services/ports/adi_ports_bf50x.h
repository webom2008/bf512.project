/******************************************************************************
Copyright (c), 2010 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.

$Revision: 3492 $
$Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $

Title: ADI Device Drivers and System Services

Description:
    This file contains the defenitions and macros required by the
    Port control service for the Moy (ADSP-BF50x) family of Blackfin processors

*****************************************************************************/

#ifndef __ADI_PORTS_H__
#define __ADI_PORTS_H__

/* disable misra diagnostics as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#include <services/services_misra_suppress.h>
#endif /* _MISRA_RULES */

#if !defined(_LANGUAGE_ASM)

#if !defined(__ADSP_MOY__)
#error "Selected PORTS module not supported for the current processor family"
#endif

/*==============  D E F I N E S  ===============*/

/*

Example:

Enable the pins for PF0, PG12 and PH0

{
    ADI_PORTS_DIRECTIVE Directives[] = {
        ADI_PORTS_DIRECTIVE_PF0,    // Directive enumeration to enable PF0
        ADI_PORTS_DIRECTIVE_PG12,   // Directive enumeration to enable PG12
        ADI_PORTS_DIRECTIVE_PH0     // Directive enumeration to enable PH0
    };

    adi_ports_Configure(Directives, (sizeof(Directives)/sizeof(Directives[0]));
}

*/

/*
**
** Directive enumerations are concatenations of several pieces of information
** for the port
**
** A directive ID enumerator consists of:
**
**  o bit 0-8   - Port number (starting at 0 and incrementing by 1) (range 0-255)
**  o bit 8-15  - Bit position within the port function enable register for the given pin
**  o bit 16    - PORTx_FER value for this bit/pin (0 for GPIO, 1 for Peripheral)
**  o bit 17-18 - PORTx_MUX value to select the peripheral function mapped to the given pin
**  o bit 19-23 - PORTx_MUX bit position within the port mux register (5 bits, range 0-31)
**  o bit 24-31 - Not used
**
*/

/*
** Macros to create the actual directive enumerators and
** to extract each of the fields within the value.
*/

/* Macro to create the directive */
#define ADI_PORTS_CREATE_DIRECTIVE(Port,Bit,Function,MuxVal,MuxPos) \
    ( ((MuxPos) << 19) | ((MuxVal) << 17) | ((Function) << 16) | ((Bit) << 8) | (Port) )

/* Macros to get the individual field values in a given directive */
#define ADI_PORTS_GET_PORT(Directive)               (((u32)Directive) & 0xFF)
#define ADI_PORTS_GET_BIT_POSITION(Directive)      ((((u32)Directive) >> 8) & 0xFF)
#define ADI_PORTS_GET_PORT_FUNCTION(Directive)     ((((u32)Directive) >> 16) & 0x01)
#define ADI_PORTS_GET_PIN_MUX_VALUE(Directive)     ((((u32)Directive) >> 17) & 0x03)
#define ADI_PORTS_GET_PIN_MUX_POSITION(Directive)  ((((u32)Directive) >> 19) & 0x1F)

/*
**
** Directives
**
**  These directives are used by the drivers and services to configure the
**  pin muxing logic as appropriate.
**
**  Note:
**      'MuxVal' is ignored for all GPIO configuration directives
**
*/

typedef enum ADI_PORTS_DIRECTIVE
{
    /* Note: 50x has only 3 ports, F, G, and H, indexed as 0, 1 and 2 */

    /* PPI Peripheral Directives */                               /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_PPICLK_TMRCLK   = ADI_PORTS_CREATE_DIRECTIVE(  1,    0,     1,       1,       0  ),  /* Enable PPICLK/TMRCLK */
    ADI_PORTS_DIRECTIVE_PPI_FS1_TMR0    = ADI_PORTS_CREATE_DIRECTIVE(  1,    14,    1,       2,       14 ),  /* Enable PPI FS1/Timer 0 */
    ADI_PORTS_DIRECTIVE_PPI_FS2_TMR1    = ADI_PORTS_CREATE_DIRECTIVE(  1,    15,    1,       2,       14 ),  /* Enable PPI FS2/Timer 1 */
    ADI_PORTS_DIRECTIVE_PPI_FS3         = ADI_PORTS_CREATE_DIRECTIVE(  1,    1,     1,       1,       2  ),  /* Enable PPI FS3 */
    ADI_PORTS_DIRECTIVE_PPI_D0          = ADI_PORTS_CREATE_DIRECTIVE(  0,    15,    1,       2,       14 ),  /* Enable PPI D0 */
    ADI_PORTS_DIRECTIVE_PPI_D1          = ADI_PORTS_CREATE_DIRECTIVE(  0,    14,    1,       2,       14 ),  /* Enable PPI D1 */
    ADI_PORTS_DIRECTIVE_PPI_D2          = ADI_PORTS_CREATE_DIRECTIVE(  0,    13,    1,       2,       12 ),  /* Enable PPI D2 */
    ADI_PORTS_DIRECTIVE_PPI_D3          = ADI_PORTS_CREATE_DIRECTIVE(  0,    12,    1,       2,       10 ),  /* Enable PPI D3 */
    ADI_PORTS_DIRECTIVE_PPI_D4          = ADI_PORTS_CREATE_DIRECTIVE(  0,    11,    1,       2,       10 ),  /* Enable PPI D4 */
    ADI_PORTS_DIRECTIVE_PPI_D5          = ADI_PORTS_CREATE_DIRECTIVE(  0,    10,    1,       2,       10 ),  /* Enable PPI D5 */
    ADI_PORTS_DIRECTIVE_PPI_D6          = ADI_PORTS_CREATE_DIRECTIVE(  0,    9,     1,       2,       8  ),  /* Enable PPI D6 */
    ADI_PORTS_DIRECTIVE_PPI_D7          = ADI_PORTS_CREATE_DIRECTIVE(  0,    8,     1,       2,       8  ),  /* Enable PPI D7 */
    ADI_PORTS_DIRECTIVE_PPI_D8          = ADI_PORTS_CREATE_DIRECTIVE(  0,    2,     1,       2,       4  ),  /* Enable PPI D8 */
    ADI_PORTS_DIRECTIVE_PPI_D9          = ADI_PORTS_CREATE_DIRECTIVE(  0,    3,     1,       2,       4  ),  /* Enable PPI D9 */
    ADI_PORTS_DIRECTIVE_PPI_D10         = ADI_PORTS_CREATE_DIRECTIVE(  0,    4,     1,       2,       4  ),  /* Enable PPI D10 */
    ADI_PORTS_DIRECTIVE_PPI_D11         = ADI_PORTS_CREATE_DIRECTIVE(  0,    5,     1,       2,       4  ),  /* Enable PPI D11 */
    ADI_PORTS_DIRECTIVE_PPI_D12         = ADI_PORTS_CREATE_DIRECTIVE(  0,    6,     1,       2,       6  ),  /* Enable PPI D12 */
    ADI_PORTS_DIRECTIVE_PPI_D13         = ADI_PORTS_CREATE_DIRECTIVE(  0,    7,     1,       2,       6  ),  /* Enable PPI D13 */
    ADI_PORTS_DIRECTIVE_PPI_D14         = ADI_PORTS_CREATE_DIRECTIVE(  1,    13,    1,       2,       12 ),  /* Enable PPI D14 */
    ADI_PORTS_DIRECTIVE_PPI_D15         = ADI_PORTS_CREATE_DIRECTIVE(  1,    12,    1,       2,       12 ),  /* Enable PPI D15 */

    /* SPORT 0 Peripheral Directives */                           /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_SPORT0_DTPRI    = ADI_PORTS_CREATE_DIRECTIVE(  0,    2,     1,       0,       4  ),  /* Enable SPORT 0 DTPRI */
    ADI_PORTS_DIRECTIVE_SPORT0_TSCLK    = ADI_PORTS_CREATE_DIRECTIVE(  0,    0,     1,       0,       0  ),  /* Enable SPORT 0 TSCLK */
    ADI_PORTS_DIRECTIVE_SPORT0_TFS      = ADI_PORTS_CREATE_DIRECTIVE(  0,    3,     1,       0,       4  ),  /* Enable SPORT 0 TFS */
    ADI_PORTS_DIRECTIVE_SPORT0_DRPRI    = ADI_PORTS_CREATE_DIRECTIVE(  0,    5,     1,       0,       4  ),  /* Enable SPORT 0 DRPRI */
    ADI_PORTS_DIRECTIVE_SPORT0_RSCLK    = ADI_PORTS_CREATE_DIRECTIVE(  0,    1,     1,       0,       2  ),  /* Enable SPORT 0 RSCLK */
    ADI_PORTS_DIRECTIVE_SPORT0_RFS      = ADI_PORTS_CREATE_DIRECTIVE(  0,    4,     1,       0,       4  ),  /* Enable SPORT 0 RFS */
    ADI_PORTS_DIRECTIVE_SPORT0_DTSEC    = ADI_PORTS_CREATE_DIRECTIVE(  0,    8,     1,       1,       8  ),  /* Enable SPORT 0 DTSEC */
    ADI_PORTS_DIRECTIVE_SPORT0_DRSEC    = ADI_PORTS_CREATE_DIRECTIVE(  0,    9,     1,       1,       8  ),  /* Enable SPORT 0 DRSEC */

    /* SPORT 1 Peripheral Directives */                           /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_SPORT1_DTPRI    = ADI_PORTS_CREATE_DIRECTIVE(  2,    0,     1,       1,       0  ),  /* Enable SPORT 1 DTPRI */
    ADI_PORTS_DIRECTIVE_SPORT1_TSCLK    = ADI_PORTS_CREATE_DIRECTIVE(  2,    2,     1,       1,       4  ),  /* Enable SPORT 1 TSCLK */
    ADI_PORTS_DIRECTIVE_SPORT1_TFS      = ADI_PORTS_CREATE_DIRECTIVE(  2,    1,     1,       1,       2  ),  /* Enable SPORT 1 TFS */
    ADI_PORTS_DIRECTIVE_SPORT1_DRPRI    = ADI_PORTS_CREATE_DIRECTIVE(  1,    9,     1,       0,       10 ),  /* Enable SPORT 1 DRPRI */
    ADI_PORTS_DIRECTIVE_SPORT1_RSCLK    = ADI_PORTS_CREATE_DIRECTIVE(  1,    11,    1,       0,       10 ),  /* Enable SPORT 1 RSCLK */
    ADI_PORTS_DIRECTIVE_SPORT1_RFS      = ADI_PORTS_CREATE_DIRECTIVE(  1,    10,    1,       0,       10 ),  /* Enable SPORT 1 RFS */
    ADI_PORTS_DIRECTIVE_SPORT1_DTSEC    = ADI_PORTS_CREATE_DIRECTIVE(  1,    3,     1,       1,       6  ),  /* Enable SPORT 1 DTSEC */
    ADI_PORTS_DIRECTIVE_SPORT1_DRSEC_PG4= ADI_PORTS_CREATE_DIRECTIVE(  1,    4,     1,       1,       6  ),  /* Enable SPORT 1 DRSEC via PG4 */
    ADI_PORTS_DIRECTIVE_SPORT1_DRSEC_PG8= ADI_PORTS_CREATE_DIRECTIVE(  1,    8,     1,       0,       8  ),  /* Enable SPORT 1 DRSEC via PG8 */

    /* SPI 0 Peripheral Directives */                             /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_SPI0_SCK        = ADI_PORTS_CREATE_DIRECTIVE(  0,    10,    1,       0,       10 ),  /* Enable SPI 0 SCLK */
    ADI_PORTS_DIRECTIVE_SPI0_MISO       = ADI_PORTS_CREATE_DIRECTIVE(  0,    11,    1,       0,       10 ),  /* Enable SPI 0 MISO */
    ADI_PORTS_DIRECTIVE_SPI0_MOSI       = ADI_PORTS_CREATE_DIRECTIVE(  0,    12,    1,       0,       10 ),  /* Enable SPI 0 MOSI */
    ADI_PORTS_DIRECTIVE_SPI0_SEL1_SS    = ADI_PORTS_CREATE_DIRECTIVE(  0,    13,    1,       0,       12 ),  /* Enable SPI 0 SSEL 1/SS */
    ADI_PORTS_DIRECTIVE_SPI0_SEL2       = ADI_PORTS_CREATE_DIRECTIVE(  0,    14,    1,       0,       14 ),  /* Enable SPI 0 SSEL 2 */
    ADI_PORTS_DIRECTIVE_SPI0_SEL3_PF15  = ADI_PORTS_CREATE_DIRECTIVE(  0,    15,    1,       0,       14 ),  /* Enable SPI 0 SSEL 3 via PF15 */
    ADI_PORTS_DIRECTIVE_SPI0_SEL3_PH0   = ADI_PORTS_CREATE_DIRECTIVE(  2,    0,     1,       2,       0  ),  /* Enable SPI 0 SSEL 3 via PH0 */

    /* SPI 1 Peripheral Directives */                             /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_SPI1_SCK        = ADI_PORTS_CREATE_DIRECTIVE(  1,    3,     1,       0,       6  ),  /* Enable SPI 1 SCLK */
    ADI_PORTS_DIRECTIVE_SPI1_MISO       = ADI_PORTS_CREATE_DIRECTIVE(  1,    5,     1,       0,       6  ),  /* Enable SPI 1 MISO */
    ADI_PORTS_DIRECTIVE_SPI1_MOSI       = ADI_PORTS_CREATE_DIRECTIVE(  1,    4,     1,       0,       6  ),  /* Enable SPI 1 MOSI */
    ADI_PORTS_DIRECTIVE_SPI1_SEL1_SS    = ADI_PORTS_CREATE_DIRECTIVE(  1,    2,     1,       0,       4  ),  /* Enable SPI 1 SSEL 1/SS */
    ADI_PORTS_DIRECTIVE_SPI1_SEL2_PG1   = ADI_PORTS_CREATE_DIRECTIVE(  1,    1,     1,       0,       2  ),  /* Enable SPI 1 SSEL 2 via PG1 */
    ADI_PORTS_DIRECTIVE_SPI1_SEL2_PH2   = ADI_PORTS_CREATE_DIRECTIVE(  2,    2,     1,       2,       4  ),  /* Enable SPI 0 SSEL 2 via PH2 */
    ADI_PORTS_DIRECTIVE_SPI1_SEL3_PG0   = ADI_PORTS_CREATE_DIRECTIVE(  1,    0,     1,       0,       0  ),  /* Enable SPI 1 SSEL 3 via PG0 */
    ADI_PORTS_DIRECTIVE_SPI1_SEL3_PH1   = ADI_PORTS_CREATE_DIRECTIVE(  2,    1,     1,       2,       2  ),  /* Enable SPI 0 SSEL 3 via PH1 */

    /* UART 0 Peripheral Directives */                            /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_UART0F_TX       = ADI_PORTS_CREATE_DIRECTIVE(  0,    1,     1,       1,       2  ),  /* Enable UART 0 Tx via Port F */
    ADI_PORTS_DIRECTIVE_UART0F_RX       = ADI_PORTS_CREATE_DIRECTIVE(  0,    0,     1,       1,       0  ),  /* Enable UART 0 Rx via Port F */
    ADI_PORTS_DIRECTIVE_UART0G_TX       = ADI_PORTS_CREATE_DIRECTIVE(  1,    13,    1,       0,       12 ),  /* Enable UART 0 Tx via Port G */
    ADI_PORTS_DIRECTIVE_UART0G_RX       = ADI_PORTS_CREATE_DIRECTIVE(  1,    12,    1,       0,       12 ),  /* Enable UART 0 Rx via Port G */
    ADI_PORTS_DIRECTIVE_UART0_RTS       = ADI_PORTS_CREATE_DIRECTIVE(  1,    14,    1,       0,       14 ),  /* Enable UART 0 RTS */
    ADI_PORTS_DIRECTIVE_UART0_CTS       = ADI_PORTS_CREATE_DIRECTIVE(  1,    15,    1,       0,       14 ),  /* Enable UART 0 CTS */

    /* UART 1 Peripheral Directives */                            /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_UART1F_TX       = ADI_PORTS_CREATE_DIRECTIVE(  0,    6,     1,       0,       6  ),  /* Enable UART 1 Tx via Port F */
    ADI_PORTS_DIRECTIVE_UART1F_RX       = ADI_PORTS_CREATE_DIRECTIVE(  0,    7,     1,       0,       6  ),  /* Enable UART 1 Rx via Port F */
    ADI_PORTS_DIRECTIVE_UART1G_TX       = ADI_PORTS_CREATE_DIRECTIVE(  1,    3,     1,       2,       6  ),  /* Enable UART 1 Tx via Port G */
    ADI_PORTS_DIRECTIVE_UART1G_RX       = ADI_PORTS_CREATE_DIRECTIVE(  1,    0,     1,       2,       0  ),  /* Enable UART 1 Rx via Port G */
    ADI_PORTS_DIRECTIVE_UART1_RTS       = ADI_PORTS_CREATE_DIRECTIVE(  0,    8,     1,       0,       8  ),  /* Enable UART 1 RTS */
    ADI_PORTS_DIRECTIVE_UART1_CTS       = ADI_PORTS_CREATE_DIRECTIVE(  0,    9,     1,       0,       8  ),  /* Enable UART 1 CTS */

    /* PWM 0 Peripheral Directives */                             /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_PWM0_AH         = ADI_PORTS_CREATE_DIRECTIVE(  0,    14,    1,       1,       14 ),  /* Enable PWM 0 AH */
    ADI_PORTS_DIRECTIVE_PWM0_AL         = ADI_PORTS_CREATE_DIRECTIVE(  0,    15,    1,       1,       14 ),  /* Enable PWM 0 AL */
    ADI_PORTS_DIRECTIVE_PWM0_BH         = ADI_PORTS_CREATE_DIRECTIVE(  0,    2,     1,       1,       4  ),  /* Enable PWM 0 BH */
    ADI_PORTS_DIRECTIVE_PWM0_BL         = ADI_PORTS_CREATE_DIRECTIVE(  0,    3,     1,       1,       4  ),  /* Enable PWM 0 BL */
    ADI_PORTS_DIRECTIVE_PWM0_CH         = ADI_PORTS_CREATE_DIRECTIVE(  0,    4,     1,       1,       4  ),  /* Enable PWM 0 CH */
    ADI_PORTS_DIRECTIVE_PWM0_CL         = ADI_PORTS_CREATE_DIRECTIVE(  0,    5,     1,       1,       4  ),  /* Enable PWM 0 CL */
    ADI_PORTS_DIRECTIVE_PWM0_TRIP_PF6   = ADI_PORTS_CREATE_DIRECTIVE(  0,    6,     1,       1,       6  ),  /* Enable PWM 0 TRIP via PF6 */
    ADI_PORTS_DIRECTIVE_PWM0_SYNC_PF7   = ADI_PORTS_CREATE_DIRECTIVE(  0,    7,     1,       1,       6  ),  /* Enable PWM 0 SYNC via PF7 */
    ADI_PORTS_DIRECTIVE_PWM0_TRIP_PF11  = ADI_PORTS_CREATE_DIRECTIVE(  0,    11,    1,       1,       10 ),  /* Enable PWM 0 TRIP via PF11 */
    ADI_PORTS_DIRECTIVE_PWM0_SYNC_PF12  = ADI_PORTS_CREATE_DIRECTIVE(  0,    12,    1,       1,       10 ),  /* Enable PWM 0 TRIP via PF12 */

    /* PWM 1 Peripheral Directives */                             /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_PWM1_AH         = ADI_PORTS_CREATE_DIRECTIVE(  1,    6,     1,       2,       8  ),  /* Enable PWM 1 AH */
    ADI_PORTS_DIRECTIVE_PWM1_AL         = ADI_PORTS_CREATE_DIRECTIVE(  1,    7,     1,       2,       8  ),  /* Enable PWM 1 AL */
    ADI_PORTS_DIRECTIVE_PWM1_BH         = ADI_PORTS_CREATE_DIRECTIVE(  1,    8,     1,       2,       8  ),  /* Enable PWM 1 BH */
    ADI_PORTS_DIRECTIVE_PWM1_BL         = ADI_PORTS_CREATE_DIRECTIVE(  1,    9,     1,       2,       10 ),  /* Enable PWM 1 BL */
    ADI_PORTS_DIRECTIVE_PWM1_CH         = ADI_PORTS_CREATE_DIRECTIVE(  1,    10,    1,       2,       10 ),  /* Enable PWM 1 CH */
    ADI_PORTS_DIRECTIVE_PWM1_CL         = ADI_PORTS_CREATE_DIRECTIVE(  1,    11,    1,       2,       10 ),  /* Enable PWM 1 CL */
    ADI_PORTS_DIRECTIVE_PWM1_TRIP       = ADI_PORTS_CREATE_DIRECTIVE(  1,    5,     1,       2,       6  ),  /* Enable PWM 1 TRIP */
    ADI_PORTS_DIRECTIVE_PWM1_SYNC       = ADI_PORTS_CREATE_DIRECTIVE(  1,    4,     1,       2,       6  ),  /* Enable PWM 1 SYNC */

    /* SD/RSI Peripheral Directives */                             /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_SDH_CLK         = ADI_PORTS_CREATE_DIRECTIVE(  1,    11,    1,       1,       10 ),  /* Enable SD CLK */
    ADI_PORTS_DIRECTIVE_SDH_CMD         = ADI_PORTS_CREATE_DIRECTIVE(  1,    10,    1,       1,       10 ),  /* Enable SD CMD */
    ADI_PORTS_DIRECTIVE_SDH_D0          = ADI_PORTS_CREATE_DIRECTIVE(  1,    9,     1,       1,       10 ),  /* Enable SD Data 0 */
    ADI_PORTS_DIRECTIVE_SDH_D1          = ADI_PORTS_CREATE_DIRECTIVE(  1,    8,     1,       1,       8  ),  /* Enable SD Data 1 */
    ADI_PORTS_DIRECTIVE_SDH_D2          = ADI_PORTS_CREATE_DIRECTIVE(  1,    7,     1,       1,       8  ),  /* Enable SD Data 2 */
    ADI_PORTS_DIRECTIVE_SDH_D3          = ADI_PORTS_CREATE_DIRECTIVE(  1,    6,     1,       1,       8  ),  /* Enable SD Data 3 */
    ADI_PORTS_DIRECTIVE_SDH_D4          = ADI_PORTS_CREATE_DIRECTIVE(  1,    12,    1,       1,       12 ),  /* Enable SD Data 4 */
    ADI_PORTS_DIRECTIVE_SDH_D5          = ADI_PORTS_CREATE_DIRECTIVE(  1,    13,    1,       1,       12 ),  /* Enable SD Data 5 */
    ADI_PORTS_DIRECTIVE_SDH_D6          = ADI_PORTS_CREATE_DIRECTIVE(  1,    14,    1,       1,       14 ),  /* Enable SD Data 6 */
    ADI_PORTS_DIRECTIVE_SDH_D7          = ADI_PORTS_CREATE_DIRECTIVE(  1,    15,    1,       1,       14 ),  /* Enable SD Data 7 */

    /* ADC Control Module (ACM) Directives */                     /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_ACM_A0          = ADI_PORTS_CREATE_DIRECTIVE(  2,    2,     1,       0,       4  ),  /* Enable ACM A0 */
    ADI_PORTS_DIRECTIVE_ACM_A1          = ADI_PORTS_CREATE_DIRECTIVE(  2,    1,     1,       0,       2  ),  /* Enable ACM A2 */
    ADI_PORTS_DIRECTIVE_ACM_A2          = ADI_PORTS_CREATE_DIRECTIVE(  2,    0,     1,       0,       0  ),  /* Enable ACM A2 */
    ADI_PORTS_DIRECTIVE_ACM_RANGE       = ADI_PORTS_CREATE_DIRECTIVE(  1,    7,     1,       0,       8  ),  /* Enable ACM Range */
    ADI_PORTS_DIRECTIVE_ACM_SE_DIFF     = ADI_PORTS_CREATE_DIRECTIVE(  1,    6,     1,       0,       8  ),  /* Enable ACM SE DIFF */

    /* CAN Peripheral Directives */                               /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_CAN_TX          = ADI_PORTS_CREATE_DIRECTIVE(  1,    2,     1,       2,       4  ),  /* Enable CAN Tx */
    ADI_PORTS_DIRECTIVE_CAN_RX          = ADI_PORTS_CREATE_DIRECTIVE(  1,    1,     1,       2,       2  ),  /* Enable CAN Rx */

    /* Directives to enable Timers */                             /* Port, Bit, Function, MuxVal, MuxPos */
    /* Use 'ADI_PORTS_DIRECTIVE_PPICLK_TMRCLK' to enable Timer Clock (shared with PPI Clock) */
    /* Use 'ADI_PORTS_DIRECTIVE_PPI_FS1_TMR0' to enable Timer 0 (shared with PPI FS1) */
    /* Use 'ADI_PORTS_DIRECTIVE_PPI_FS2_TMR1' to enable Timer 1 (shared with PPI FS2) */
    ADI_PORTS_DIRECTIVE_TMR2            = ADI_PORTS_CREATE_DIRECTIVE(  0,    10,    1,       1,       10 ),  /* Enable Timer 2 */
    ADI_PORTS_DIRECTIVE_TMR3            = ADI_PORTS_CREATE_DIRECTIVE(  0,    13,    1,       1,       12 ),  /* Enable Timer 3 */
    ADI_PORTS_DIRECTIVE_TMR4            = ADI_PORTS_CREATE_DIRECTIVE(  1,    2,     1,       1,       4  ),  /* Enable Timer 4 */
    ADI_PORTS_DIRECTIVE_TMR5            = ADI_PORTS_CREATE_DIRECTIVE(  0,    1,     1,       2,       2  ),  /* Enable Timer 5 */
    ADI_PORTS_DIRECTIVE_TMR6            = ADI_PORTS_CREATE_DIRECTIVE(  0,    0,     1,       2,       0  ),  /* Enable Timer 6 */
    ADI_PORTS_DIRECTIVE_TMR7            = ADI_PORTS_CREATE_DIRECTIVE(  1,    5,     1,       1,       6  ),  /* Enable Timer 7 */

    /* Port F GPIO Directives */                                  /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_PF0             = ADI_PORTS_CREATE_DIRECTIVE(  0,    0,     0,       0,       0  ),  /* Enable PF0 GPIO */
    ADI_PORTS_DIRECTIVE_PF1             = ADI_PORTS_CREATE_DIRECTIVE(  0,    1,     0,       0,       2  ),  /* Enable PF1 GPIO */
    ADI_PORTS_DIRECTIVE_PF2             = ADI_PORTS_CREATE_DIRECTIVE(  0,    2,     0,       0,       4  ),  /* Enable PF2 GPIO */
    ADI_PORTS_DIRECTIVE_PF3             = ADI_PORTS_CREATE_DIRECTIVE(  0,    3,     0,       0,       4  ),  /* Enable PF3 GPIO */
    ADI_PORTS_DIRECTIVE_PF4             = ADI_PORTS_CREATE_DIRECTIVE(  0,    4,     0,       0,       4  ),  /* Enable PF4 GPIO */
    ADI_PORTS_DIRECTIVE_PF5             = ADI_PORTS_CREATE_DIRECTIVE(  0,    5,     0,       0,       4  ),  /* Enable PF5 GPIO */
    ADI_PORTS_DIRECTIVE_PF6             = ADI_PORTS_CREATE_DIRECTIVE(  0,    6,     0,       0,       6  ),  /* Enable PF6 GPIO */
    ADI_PORTS_DIRECTIVE_PF7             = ADI_PORTS_CREATE_DIRECTIVE(  0,    7,     0,       0,       6  ),  /* Enable PF7 GPIO */
    ADI_PORTS_DIRECTIVE_PF8             = ADI_PORTS_CREATE_DIRECTIVE(  0,    8,     0,       0,       8  ),  /* Enable PF8 GPIO */
    ADI_PORTS_DIRECTIVE_PF9             = ADI_PORTS_CREATE_DIRECTIVE(  0,    9,     0,       0,       8  ),  /* Enable PF9 GPIO */
    ADI_PORTS_DIRECTIVE_PF10            = ADI_PORTS_CREATE_DIRECTIVE(  0,    10,    0,       0,       10 ),  /* Enable PF10 GPIO */
    ADI_PORTS_DIRECTIVE_PF11            = ADI_PORTS_CREATE_DIRECTIVE(  0,    11,    0,       0,       10 ),  /* Enable PF11 GPIO */
    ADI_PORTS_DIRECTIVE_PF12            = ADI_PORTS_CREATE_DIRECTIVE(  0,    12,    0,       0,       10 ),  /* Enable PF12 GPIO */
    ADI_PORTS_DIRECTIVE_PF13            = ADI_PORTS_CREATE_DIRECTIVE(  0,    13,    0,       0,       12 ),  /* Enable PF13 GPIO */
    ADI_PORTS_DIRECTIVE_PF14            = ADI_PORTS_CREATE_DIRECTIVE(  0,    14,    0,       0,       14 ),  /* Enable PF14 GPIO */
    ADI_PORTS_DIRECTIVE_PF15            = ADI_PORTS_CREATE_DIRECTIVE(  0,    15,    0,       0,       14 ),  /* Enable PF15 GPIO */

    /* Port G GPIO Directives */                                  /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_PG0             = ADI_PORTS_CREATE_DIRECTIVE(  1,    0,     0,       0,       0  ),  /* Enable PG0 GPIO */
    ADI_PORTS_DIRECTIVE_PG1             = ADI_PORTS_CREATE_DIRECTIVE(  1,    1,     0,       0,       2  ),  /* Enable PG1 GPIO */
    ADI_PORTS_DIRECTIVE_PG2             = ADI_PORTS_CREATE_DIRECTIVE(  1,    2,     0,       0,       4  ),  /* Enable PG2 GPIO */
    ADI_PORTS_DIRECTIVE_PG3             = ADI_PORTS_CREATE_DIRECTIVE(  1,    3,     0,       0,       6  ),  /* Enable PG3 GPIO */
    ADI_PORTS_DIRECTIVE_PG4             = ADI_PORTS_CREATE_DIRECTIVE(  1,    4,     0,       0,       6  ),  /* Enable PG4 GPIO */
    ADI_PORTS_DIRECTIVE_PG5             = ADI_PORTS_CREATE_DIRECTIVE(  1,    5,     0,       0,       6  ),  /* Enable PG5 GPIO */
    ADI_PORTS_DIRECTIVE_PG6             = ADI_PORTS_CREATE_DIRECTIVE(  1,    6,     0,       0,       8  ),  /* Enable PG6 GPIO */
    ADI_PORTS_DIRECTIVE_PG7             = ADI_PORTS_CREATE_DIRECTIVE(  1,    7,     0,       0,       8  ),  /* Enable PG7 GPIO */
    ADI_PORTS_DIRECTIVE_PG8             = ADI_PORTS_CREATE_DIRECTIVE(  1,    8,     0,       0,       8  ),  /* Enable PG8 GPIO */
    ADI_PORTS_DIRECTIVE_PG9             = ADI_PORTS_CREATE_DIRECTIVE(  1,    9,     0,       0,       10 ),  /* Enable PG9 GPIO */
    ADI_PORTS_DIRECTIVE_PG10            = ADI_PORTS_CREATE_DIRECTIVE(  1,    10,    0,       0,       10 ),  /* Enable PG10 GPIO */
    ADI_PORTS_DIRECTIVE_PG11            = ADI_PORTS_CREATE_DIRECTIVE(  1,    11,    0,       0,       10 ),  /* Enable PG11 GPIO */
    ADI_PORTS_DIRECTIVE_PG12            = ADI_PORTS_CREATE_DIRECTIVE(  1,    12,    0,       0,       12 ),  /* Enable PG12 GPIO */
    ADI_PORTS_DIRECTIVE_PG13            = ADI_PORTS_CREATE_DIRECTIVE(  1,    13,    0,       0,       12 ),  /* Enable PG13 GPIO */
    ADI_PORTS_DIRECTIVE_PG14            = ADI_PORTS_CREATE_DIRECTIVE(  1,    14,    0,       0,       14 ),  /* Enable PG14 GPIO */
    ADI_PORTS_DIRECTIVE_PG15            = ADI_PORTS_CREATE_DIRECTIVE(  1,    15,    0,       0,       14 ),  /* Enable PG15 GPIO */

    /* Port H GPIO Directives */                                  /* Port, Bit, Function, MuxVal, MuxPos */
    ADI_PORTS_DIRECTIVE_PH0             = ADI_PORTS_CREATE_DIRECTIVE(  2,    0,     0,       0,       0  ),  /* Enable PH0 GPIO */
    ADI_PORTS_DIRECTIVE_PH1             = ADI_PORTS_CREATE_DIRECTIVE(  2,    1,     0,       0,       2  ),  /* Enable PH1 GPIO */
    ADI_PORTS_DIRECTIVE_PH2             = ADI_PORTS_CREATE_DIRECTIVE(  2,    2,     0,       0,       4  )   /* Enable PH2 GPIO */

} ADI_PORTS_DIRECTIVE;

/*
**
** Return codes
**
*/

typedef enum ADI_PORTS_RESULT
{
    /* Generic success */
    ADI_PORTS_RESULT_SUCCESS = 0,
    /* Generic failure */
    ADI_PORTS_RESULT_FAILED  = 1,

    /* Insure this order remains */
    ADI_PORTS_RESULT_START = ADI_PORTS_ENUMERATION_START,
    /* (0x90001) A directive is invalid for the specified peripheral */
    ADI_PORTS_RESULT_BAD_DIRECTIVE,
    /* (0x90002) Array passed is NULL */
    ADI_PORTS_RESULT_NULL_ARRAY

} ADI_PORTS_RESULT;

/*=======  P U B L I C   P R O T O T Y P E S  ========*/
/*            (globally-scoped functions)             */

#ifdef __cplusplus
extern "C" {
#endif

/*
**  Function: adi_ports_Init
**
**      Initialises Port control service
**
**  Parameters:
**      pCriticalRegionArg - Parameter for critical region function
**
*/
ADI_PORTS_RESULT adi_ports_Init(
    void                *pCriticalRegionArg
);

/*
**  Function: adi_ports_Init
**
**      Terminates Port control service
**
**  Parameters:
**      None
**
*/
ADI_PORTS_RESULT adi_ports_Terminate(
    void
);

/*
**  Function: adi_ports_Configure
**
**      Configures port pins for GPIO or Peripheral use
**
**  Parameters:
**      pDirectives  - Pointer to an array of directives
**      nDirectives  - Number of directives in the given array
**
*/
ADI_PORTS_RESULT adi_ports_Configure(
    ADI_PORTS_DIRECTIVE *pDirectives,
    u32                 nDirectives
);

#ifdef __cplusplus
}
#endif

#endif /* Not ASM */

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif /* __ADI_PORTS_H__ */
