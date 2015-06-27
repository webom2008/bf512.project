/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_uart.c,v $
$Revision: 5506 $
$Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $

Description:
            This is the driver source code for the UART peripheral.  All MMR access
            is via macros.

            Use 'adi_uart_int.c' to build Interrupt driven UART driver
            Use 'adi_uart_dma.c' to build DMA driven UART driver

            Using this file (adi_uart.c) by itself builds Interrupt driven UART driver

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/


#include <services/services.h>          // system service includes
#include <drivers/adi_dev.h>            // device manager includes
#include <drivers/uart/adi_uart.h>      // uart driver includes

/*********************************************************************

Check to insure that either the dma-driven or interrupt-driven driver
is being built.

*********************************************************************/

/* IF (none of the UART driver build macro is defined) */
#ifndef ADI_UART_DMA
#ifndef ADI_UART_INT
#define ADI_UART_INT     /* Build an interrupt driven UART driver */
#endif
#endif

/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_UART_NUM_DEVICES    (sizeof(Device)/sizeof(ADI_UART))   // number of UART devices in the system

#define ADI_UART_AUTOBAUD_IN_PROGRESS   (0xffffffff)                // value used in baud rate field to indicate autobaud in progress

/*********************************************************************

Macros to manipulate MMRs

*********************************************************************/

/* MOAB and Moy has a completely different register structure than the other Blackfins */

#if defined(__ADSP_MOAB__) || defined (__ADSP_MOY__)

// line control register
#define ADI_UART_LCR_SET(Value)     (pDevice->pRegisters->lcr = (Value))        // sets the whole LCR register
#define ADI_UART_LCR_SET_WLS(Value) (pDevice->pRegisters->lcr = (pDevice->pRegisters->lcr & (0x00fc)) | (Value))    // sets word length
#define ADI_UART_LCR_SET_STB()      (pDevice->pRegisters->lcr |= 0x0004)        // set stop bits to 2 or 1 1/2
#define ADI_UART_LCR_CLEAR_STB()    (pDevice->pRegisters->lcr &= ~0x0004)       // set stop bits to 1
#define ADI_UART_LCR_SET_PEN()      (pDevice->pRegisters->lcr |= 0x0008)        // enable parity
#define ADI_UART_LCR_CLEAR_PEN()    (pDevice->pRegisters->lcr &= ~0x0008)       // disable parity
#define ADI_UART_LCR_SET_EPS()      (pDevice->pRegisters->lcr |= 0x0010)        // set even parity
#define ADI_UART_LCR_CLEAR_EPS()    (pDevice->pRegisters->lcr &= ~0x0010)       // set odd parity
#define ADI_UART_LCR_SET_STP()      (pDevice->pRegisters->lcr |= 0x0020)        // force parity
#define ADI_UART_LCR_CLEAR_STP()    (pDevice->pRegisters->lcr &= ~0x0020)       // no forced parity
#define ADI_UART_LCR_SET_SB()       (pDevice->pRegisters->lcr |= 0x0040)        // force Tx pin to 0
#define ADI_UART_LCR_CLEAR_SB()     (pDevice->pRegisters->lcr &= ~0x0040)       // no force of TX pin

// line status register
#define ADI_UART_LSR_GET()                      (pDevice->pRegisters->lsr)          // gets the LSR register
#define ADI_UART_LSR_IS_DR(RegisterValue)       ((RegisterValue) & 0x0001)          // tests data ready
#define ADI_UART_LSR_IS_OE(RegisterValue)       ((RegisterValue) & 0x0002)          // tests overrun error
#define ADI_UART_LSR_CLEAR_OE()                 (pDevice->pRegisters->lsr = 0x0002) // clears an overrun error
#define ADI_UART_LSR_IS_PE(RegisterValue)       ((RegisterValue) & 0x0004)          // tests parity error
#define ADI_UART_LSR_CLEAR_PE()                 (pDevice->pRegisters->lsr = 0x0004) // clears parity error
#define ADI_UART_LSR_IS_FE(RegisterValue)       ((RegisterValue) & 0x0008)          // tests framing error
#define ADI_UART_LSR_CLEAR_FE()                 (pDevice->pRegisters->lsr = 0x0008) // clears framing error
#define ADI_UART_LSR_IS_BI(RegisterValue)       ((RegisterValue) & 0x0010)          // tests break interrupt
#define ADI_UART_LSR_CLEAR_BI()                 (pDevice->pRegisters->lsr = 0x0010) // clear break interrupt
#define ADI_UART_LSR_IS_THRE(RegisterValue)     ((RegisterValue) & 0x0020)          // tests THR empty
#define ADI_UART_LSR_IS_TEMT(RegisterValue)     ((RegisterValue) & 0x0040)          // tests TSR and THR empty
#define ADI_UART_LSR_IS_ANY_LSE(RegisterValue)  ((RegisterValue) & 0x001e)          // tests for any line status error
#define ADI_UART_LSR_CLEAR_ALL_LSE()            (pDevice->pRegisters->lsr = 0x001e) // clears all line status errors

// transmit hold register
#define ADI_UART_THR_SET(Value) (pDevice->pRegisters->thr = (Value))    // stores data to THR

// receive buffer register
#define ADI_UART_RBR_GET()      (pDevice->pRegisters->rbr)              // reads data from RBR

// interrupt enable register set
#define ADI_UART_IER_SET(Value)     (pDevice->pRegisters->ier_set = (Value))    // sets the whole IER register
#define ADI_UART_IER_SET_ERBFI()    (pDevice->pRegisters->ier_set = 0x0001)     // enables Rx interrupts
#define ADI_UART_IER_SET_ETBEI()    (pDevice->pRegisters->ier_set = 0x0002)     // enables Tx interrupts
#define ADI_UART_IER_SET_ELSI()     (pDevice->pRegisters->ier_set = 0x0004)     // enables line status interrupts
#define ADI_UART_IER_SET_EDSSI()    (pDevice->pRegisters->ier_set = 0x0008)     // enables modem status interrupts
#define ADI_UART_IER_SET_EDTPTI()   (pDevice->pRegisters->ier_set = 0x0010)     // enables DMA TX peripheral interrupts
#define ADI_UART_IER_SET_ETFI()     (pDevice->pRegisters->ier_set = 0x0020)     // enables transmission finished interrupts
#define ADI_UART_IER_SET_ERFCI()    (pDevice->pRegisters->ier_set = 0x0040)     // enables receive FIFO interrupts

// interrupt enable register clear
#define ADI_UART_IER_CLEAR_ERBFI()  (pDevice->pRegisters->ier_clear = 0x0001)   // disables Rx interrupts
#define ADI_UART_IER_CLEAR_ETBEI()  (pDevice->pRegisters->ier_clear = 0x0002)   // disables Tx interrupts
#define ADI_UART_IER_CLEAR_ELSI()   (pDevice->pRegisters->ier_clear = 0x0004)   // disables line status interrupts
#define ADI_UART_IER_CLEAR_EDSSI()  (pDevice->pRegisters->ier_clear = 0x0008)   // disables modem status interrupts
#define ADI_UART_IER_CLEAR_EDTPTI() (pDevice->pRegisters->ier_clear = 0x0010)   // disables DMA TX peripheral interrupts
#define ADI_UART_IER_CLEAR_ETFI()   (pDevice->pRegisters->ier_clear = 0x0020)   // disables transmission finished interrupts
#define ADI_UART_IER_CLEAR_ERFCI()  (pDevice->pRegisters->ier_clear = 0x0040)   // disables receive FIFO interrupts
#define ADI_UART_IER_IS_ETBEI()     (pDevice->pRegisters->ier_clear & 0x0002)       // are Tx interrupts enabled

// divisor latch low and high
#define ADI_UART_DLL_SET(Value)     (pDevice->pRegisters->dll = (Value))    // sets DLL
#define ADI_UART_DLH_SET(Value)     (pDevice->pRegisters->dlh = (Value))    // sets DLH

// global control register
#define ADI_UART_GCTL_SET(Value)    (pDevice->pRegisters->gctl = (Value))       // sets the whole GCTL register
#define ADI_UART_GCTL_SET_UCEN()    (pDevice->pRegisters->gctl |= 0x0001)       // enables clocks
#define ADI_UART_GCTL_CLEAR_UCEN()  (pDevice->pRegisters->gctl &= ~0x0001)      // disables clocks

// modem control register
#define ADI_UART_MCR_SET(Value)                         (pDevice->pRegisters->mcr = (Value))    // sets the MCR register
#define ADI_UART_MCR_ENABLE_CTS_RTS()                   (pDevice->pRegisters->mcr |= 0x00c0)    // enables CTS/RTS
#define ADI_UART_MCR_DISABLE_CTS_RTS()                  (pDevice->pRegisters->mcr &= ~0x00c0)   // disables CTS/RTS
#define ADI_UART_MCR_SET_POSITIVE_CTS_RTS_POLARITY()    (pDevice->pRegisters->mcr |= 0x0020)    // sets positive CTS/RTS polarity
#define ADI_UART_MCR_SET_NEGATIVE_CTS_RTS_POLARITY()    (pDevice->pRegisters->mcr &= ~0x0020)   // sets negative CTS/RTS polarity
#define ADI_UART_MCR_SET_HIGH_CTS_RTS_THRESHOLD()       (pDevice->pRegisters->mcr |= 0x000c)    // sets high CTS/RTS threshold
#define ADI_UART_MCR_SET_LOW_CTS_RTS_THRESHOLD()        (pDevice->pRegisters->mcr &= ~0x000c)   // sets low CTS/RTS threshold


#else   /* all other Blackfins, including Kookaburra, Mockingbird and Brodie */

// line control register
#define ADI_UART_LCR_SET(Value)     (pDevice->pRegisters->lcr = (Value))        // sets the whole LCR register
#define ADI_UART_LCR_SET_WLS(Value) (pDevice->pRegisters->lcr = (pDevice->pRegisters->lcr & (0x00fc)) | (Value))    // sets word length
#define ADI_UART_LCR_SET_STB()      (pDevice->pRegisters->lcr |= 0x0004)        // set stop bits to 2 or 1 1/2
#define ADI_UART_LCR_CLEAR_STB()    (pDevice->pRegisters->lcr &= ~0x0004)       // set stop bits to 1
#define ADI_UART_LCR_SET_PEN()      (pDevice->pRegisters->lcr |= 0x0008)        // enable parity
#define ADI_UART_LCR_CLEAR_PEN()    (pDevice->pRegisters->lcr &= ~0x0008)       // disable parity
#define ADI_UART_LCR_SET_EPS()      (pDevice->pRegisters->lcr |= 0x0010)        // set even parity
#define ADI_UART_LCR_CLEAR_EPS()    (pDevice->pRegisters->lcr &= ~0x0010)       // set odd parity
#define ADI_UART_LCR_SET_STP()      (pDevice->pRegisters->lcr |= 0x0020)        // force parity
#define ADI_UART_LCR_CLEAR_STP()    (pDevice->pRegisters->lcr &= ~0x0020)       // no forced parity
#define ADI_UART_LCR_SET_SB()       (pDevice->pRegisters->lcr |= 0x0040)        // force Tx pin to 0
#define ADI_UART_LCR_CLEAR_SB()     (pDevice->pRegisters->lcr &= ~0x0040)       // no force of TX pin
#define ADI_UART_LCR_SET_DLAB()     (pDevice->pRegisters->lcr |= 0x0080)        // enable divisor latch access
#define ADI_UART_LCR_CLEAR_DLAB()   (pDevice->pRegisters->lcr &= ~0x0080)       // disable divisor latch access

// line status register
#define ADI_UART_LSR_GET()                      (pDevice->pRegisters->lsr)      // gets the LSR register
#define ADI_UART_LSR_IS_DR(RegisterValue)       ((RegisterValue) & 0x0001)      // tests data ready
#define ADI_UART_LSR_IS_OE(RegisterValue)       ((RegisterValue) & 0x0002)      // tests overrun error
#define ADI_UART_LSR_IS_PE(RegisterValue)       ((RegisterValue) & 0x0004)      // tests parity error
#define ADI_UART_LSR_IS_FE(RegisterValue)       ((RegisterValue) & 0x0008)      // tests framing error
#define ADI_UART_LSR_IS_BI(RegisterValue)       ((RegisterValue) & 0x0010)      // tests break interrupt
#define ADI_UART_LSR_IS_THRE(RegisterValue)     ((RegisterValue) & 0x0020)      // tests THR empty
#define ADI_UART_LSR_IS_TEMT(RegisterValue)     ((RegisterValue) & 0x0040)      // tests TSR and THR empty
#define ADI_UART_LSR_IS_ANY_LSE(RegisterValue)  ((RegisterValue) & 0x001e)      // tests for any line status error

// transmit hold register
#define ADI_UART_THR_SET(Value) (pDevice->pRegisters->thr_rbr_dll = (Value))    // stores data to THR

// receive buffer register
#define ADI_UART_RBR_GET()      (pDevice->pRegisters->thr_rbr_dll)              // reads data from RBR

// interrupt ID register
#define ADI_UART_IIR_GET()      (pDevice->pRegisters->iir)                      // reads the IIR register

// interrupt enable register
#define ADI_UART_IER_SET(Value)     (pDevice->pRegisters->ier_dlh = (Value))    // sets the whole IER register
#define ADI_UART_IER_SET_ERBFI()    (pDevice->pRegisters->ier_dlh |= 0x0001)    // enables Rx interrupts
#define ADI_UART_IER_CLEAR_ERBFI()  (pDevice->pRegisters->ier_dlh &= ~0x0001)   // disables Rx interrupts
#define ADI_UART_IER_SET_ETBEI()    (pDevice->pRegisters->ier_dlh |= 0x0002)    // enables Tx interrupts
#define ADI_UART_IER_CLEAR_ETBEI()  (pDevice->pRegisters->ier_dlh &= ~0x0002)   // disables Tx interrupts
#define ADI_UART_IER_SET_ELSI()     (pDevice->pRegisters->ier_dlh |= 0x0004)    // enables line status interrupts
#define ADI_UART_IER_CLEAR_ELSI()   (pDevice->pRegisters->ier_dlh &= ~0x0004)   // disables line status interrupts
#define ADI_UART_IER_IS_ETBEI()     (pDevice->pRegisters->ier_dlh & 0x0002)     // are Tx interrupts enabled

// global control register
#define ADI_UART_GCTL_SET(Value)    (pDevice->pRegisters->gctl = (Value))       // sets the whole GCTL register
#define ADI_UART_GCTL_SET_UCEN()    (pDevice->pRegisters->gctl |= 0x0001)       // enables clocks
#define ADI_UART_GCTL_CLEAR_UCEN()  (pDevice->pRegisters->gctl &= ~0x0001)      // disables clocks

// divisor latch low
#define ADI_UART_DLL_SET(Value)     (pDevice->pRegisters->thr_rbr_dll = (Value))// sets DLL

// divisor latch high
#define ADI_UART_DLH_SET(Value)     (pDevice->pRegisters->ier_dlh = (Value))    // sets DLH

// modem control register
#define ADI_UART_MCR_SET(Value)     (pDevice->pRegisters->mcr = (Value))        // sets the MCR register

#endif

/*********************************************************************

Default register values

*********************************************************************/

#define ADI_UART_LCR_DEFAULT    ((u16)0x0000)   // default LCR value
#define ADI_UART_MCR_DEFAULT    ((u16)0x0000)   // default MCR value
#define ADI_UART_GCTL_DEFAULT   ((u16)0x0001)   // default GCTL value
#define ADI_UART_IER_DEFAULT    ((u16)0x0000)   // default IER value
#define ADI_UART_DLL_DEFAULT    ((u16)0x0001)   // default DLL value
#define ADI_UART_DLH_DEFAULT    ((u16)0x0000)   // default DLH value

/*********************************************************************

Data Structures

*********************************************************************/

#if defined(ADI_UART_INT)
typedef struct adi_uart_reserved_area {                 // define a structure for the reserved area at the top of ADI_DEV_1D_BUFFER
    struct adi_uart_reserved_area   *pNext;                 // next buffer in the chain
    u8                              *pCurrentDataPointer;   // pointer within the data where we are processing
    u32                             BytesRemaining;         // remaining number of bytes to receive or transmit
} ADI_UART_RESERVED_AREA;
#endif

/* MOAB and MOY has a completely different register structure than the other Blackfins */

#if defined(__ADSP_MOAB__) || defined (__ADSP_MOY__)

typedef struct {                // structure representing the UART registers in memory
    volatile u16    dll;            // DLL register
    u16             Padding0;
    volatile u16    dlh;            // DLH register
    u16             Padding1;
    volatile u16    gctl;           // GCTL register
    u16             Padding2;
    volatile u16    lcr;            // LCR register
    u16             Padding3;
    volatile u16    mcr;            // MCR register
    u16             Padding4;
    volatile u16    lsr;            // LSR register
    u16             Padding5;
    volatile u16    msr;            // MSR register
    u16             Padding6;
    volatile u16    scr;            // SCR register
    u16             Padding7;
    volatile u16    ier_set;        // IER_SET register
    u16             Padding8;
    volatile u16    ier_clear;      // IER_CLEAR register
    u16             Padding9;
    volatile u16    thr;            // THR register
    u16             Padding10;
    volatile u16    rbr;            // RBR register
    u16             Padding11;
} ADI_UART_REGISTERS;

#else   /* all other Blackfins, including Kookaburra, Mockingbird Brodie and Delta */

typedef struct {                // structure representing the UART registers in memory
    volatile u16    thr_rbr_dll;    // THR, RBR and DLL registers
    u16             Padding0;
    volatile u16    ier_dlh;        // DLH and IER registers
    u16             Padding1;
    volatile u16    iir;            // interrupt ID register
    u16             Padding2;
    volatile u16    lcr;            // line control register
    u16             Padding3;
    volatile u16    mcr;            // modem control register
    u16             Padding4;
    volatile u16    lsr;            // line status register
    u16             Padding5;
    u16             Padding6;
    u16             Padding7;
    volatile u16    scr;            // scratch register
    u16             Padding8;
    u16             Padding9;
    u16             Padding10;
    volatile u16    gctl;           // global control register
} ADI_UART_REGISTERS;

#endif  /* Moab vs non-Moab */


typedef struct {                        // UART device structure
    ADI_UART_REGISTERS      *pRegisters;            // base address of UART registers
    u32                     InUseFlag;              // device in use flag
    u32                     DeviceNumber;           // device number
    ADI_DEV_MODE            DataflowMethod;         // Dataflow method
    ADI_DEV_DIRECTION       Direction;              // direction
    u32                     SCLK;                   // SCLK value
    u32                     BaudRate;               // baud rate
    u32                     DataflowFlag;           // dataflow enabled flag
    u32                     LineStatusFlag;         // line status interrupt flag
    u32                     DivisorBits;            // divisor capture bits used in autobaud operation
    ADI_DMA_PMAP            InboundPMAP;            // inbound PMAP value
    ADI_DMA_PMAP            OutboundPMAP;           // outbound PMAP value
    ADI_INT_PERIPHERAL_ID   ErrorPeripheralID;      // error peripheral ID
    u32                     TimerID;                // timer ID used for autobaud
    u32                     TimerIVG;               // timer IVG for autobaud
    u32                     TransferMode;           // UART Transfer mode
#if defined(ADI_UART_INT)
    u32                     AutoCompleteFlag;       // auto complete on special character flag
    u32                     AutoCompleteChar;       // auto completion character
    ADI_UART_RESERVED_AREA  *pInboundHead;          // head of inbound queue
    ADI_UART_RESERVED_AREA  *pInboundTail;          // tail of inbound queue
    ADI_UART_RESERVED_AREA  *pOutboundHead;         // head of outbound queue
    ADI_UART_RESERVED_AREA  *pOutboundTail;         // tail of outbound queue
#endif
    void                    *pEnterCriticalArg;     // critical region argument
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;           // Device handle
    ADI_DCB_HANDLE          DCBHandle;              // callback handle
    void                    (*DMCallback) (         // device manager's callback function
                                ADI_DEV_DEVICE_HANDLE DeviceHandle, // Device handle
                                u32 Event,                          // event ID
                                void *pArg);                        // argument pointer

/* Pin Mux mode for Moy (BF50x processors) */
#if defined(__ADSP_MOY__)

    ADI_UART_PIN_MUX_MODE    ePinMuxMode;

#endif

} ADI_UART;



/*********************************************************************

Static data and processor specific macros

*********************************************************************/

/********************
    Edinburgh
********************/

#if defined(__ADSP_EDINBURGH__) // settings for Edinburgh class devices

static ADI_UART Device[] = {                // Actual UART devices
    {                                   // device 0
        (ADI_UART_REGISTERS *)(0xffc00400), // register base address
        FALSE,                              // in use flag
        0,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART_RX,               // inbound PMAP
        ADI_DMA_PMAP_UART_TX,               // outbound PMAP
        ADI_INT_UART_ERROR,                 // error peripheral ID
        ADI_TMR_GP_TIMER_1,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
};


#endif


/********************
    Braemar
********************/

#if defined(__ADSP_BRAEMAR__)  // settings for Braemar class devices

static ADI_UART Device[] = {                // Actual UART devices
    {                                   // device 0
        (ADI_UART_REGISTERS *)(0xffc00400), // register base address
        FALSE,                              // in use flag
        0,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART0_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART0_TX,              // outbound PMAP
        ADI_INT_PERIPHERAL_ERROR,           // error peripheral ID
        ADI_TMR_GP_TIMER_1,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
    {                                   // device 1
        (ADI_UART_REGISTERS *)(0xffc02000), // register base address
        FALSE,                              // in use flag
        1,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART1_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART1_TX,              // outbound PMAP
        ADI_INT_PERIPHERAL_ERROR,           // error peripheral ID
        ADI_TMR_GP_TIMER_6,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
};


#endif


/********************
    Teton-Lite
********************/

#if defined(__ADSP_TETON__) // settings for Teton class devices

static ADI_UART Device[] = {                // Actual UART devices
    {                                   // device 0
        (ADI_UART_REGISTERS *)(0xffc00400), // register base address
        FALSE,                              // in use flag
        0,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        0,                                  // divisor capture bits
        ADI_DMA_PMAP_UART_RX,               // inbound PMAP
        ADI_DMA_PMAP_UART_TX,               // outbound PMAP
        ADI_INT_UART_ERROR,                 // error peripheral ID
        ADI_TMR_GP_TIMER_1,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
};


#endif


/********************
    Stirling
********************/

#if defined(__ADSP_STIRLING__)  // settings for Stirling class devices

static ADI_UART Device[] = {                // Actual UART devices
    {                                   // device 0
        (ADI_UART_REGISTERS *)(0xffc00400), // register base address
        FALSE,                              // in use flag
        0,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART0_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART0_TX,              // outbound PMAP
        ADI_INT_UART0_ERROR,                // error peripheral ID
        ADI_TMR_GP_TIMER_1,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
    {                                   // device 1
        (ADI_UART_REGISTERS *)(0xffc02000), // register base address
        FALSE,                              // in use flag
        1,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART1_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART1_TX,              // outbound PMAP
        ADI_INT_UART1_ERROR,                // error peripheral ID
        ADI_TMR_GP_TIMER_0,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
    {                                   // device 2
        (ADI_UART_REGISTERS *)(0xffc02100), // register base address
        FALSE,                              // in use flag
        2,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART2_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART2_TX,              // outbound PMAP
        ADI_INT_UART2_ERROR,                // error peripheral ID
        ADI_TMR_GP_TIMER_0,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
};


#endif


/*********************
    Moab
**********************/

#if defined(__ADSP_MOAB__)  // settings for BF54x

static ADI_UART Device[] = {                // Actual UART devices
    {                                   // device 0
        (ADI_UART_REGISTERS *)(0xffc00400), // register base address
        FALSE,                              // in use flag
        0,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART0_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART0_TX,              // outbound PMAP
        ADI_INT_UART0_STATUS,               // error peripheral ID
        ADI_TMR_GP_TIMER_0,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
    {                                   // device 1
        (ADI_UART_REGISTERS *)(0xffc02000), // register base address
        FALSE,                              // in use flag
        1,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART1_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART1_TX,              // outbound PMAP
        ADI_INT_UART1_STATUS,               // error peripheral ID
        ADI_TMR_GP_TIMER_1,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
    {                                   // device 2
        (ADI_UART_REGISTERS *)(0xffc02100), // register base address
        FALSE,                              // in use flag
        2,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART2_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART2_TX,              // outbound PMAP
        ADI_INT_UART2_STATUS,               // error peripheral ID
        ADI_TMR_GP_TIMER_2,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
    {                                   // device 3
        (ADI_UART_REGISTERS *)(0xffc03100), // register base address
        FALSE,                              // in use flag
        3,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART3_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART3_TX,              // outbound PMAP
        ADI_INT_UART3_STATUS,               // error peripheral ID
        ADI_TMR_GP_TIMER_3,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
};

#endif



/********************
    Kookaburra/Mockingbird
********************/

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)  // settings for Kookaburra/Mockingbird class devices

static ADI_UART Device[] = {                // Actual UART devices
    {                                   // device 0 on PortG (UART0G)
        (ADI_UART_REGISTERS *)(0xffc00400), // register base address
        FALSE,                              // in use flag
        0,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART0_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART0_TX,              // outbound PMAP
        ADI_INT_UART0_STATUS,               // error peripheral ID (no dedicated error interrupt)
        ADI_TMR_GP_TIMER_4,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
    {                                   // device 1 on PortG (UART1G)
        (ADI_UART_REGISTERS *)(0xffc02000), // register base address
        FALSE,                              // in use flag
        1,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART1_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART1_TX,              // outbound PMAP
        ADI_INT_UART1_STATUS,               // error peripheral ID (no dedicated error interrupt)
        ADI_TMR_GP_TIMER_2,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
    {                                   // device 2 on PortF (UART1F)
        (ADI_UART_REGISTERS *)(0xffc02000), // register base address
        FALSE,                              // in use flag
        2,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART1_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART1_TX,              // outbound PMAP
        ADI_INT_UART1_STATUS,               // error peripheral ID (no dedicated error interrupt)
        ADI_TMR_GP_TIMER_3,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
};


#endif  // end Kookaburra or Mockingbird


/********************
    Brodie
********************/

#if defined(__ADSP_BRODIE__)  // settings for Brodie class devices

static ADI_UART Device[] = {                // Actual UART devices
    {                                   // device 0 on PortG (UART0)
        (ADI_UART_REGISTERS *)(0xffc00400), // register base address
        FALSE,                              // in use flag
        0,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART0_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART0_TX,              // outbound PMAP
        ADI_INT_UART0_STATUS,               // error peripheral ID (no dedicated error interrupt)
        ADI_TMR_GP_TIMER_4,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
    {                                   // device 1 on PortH (UART1)
        (ADI_UART_REGISTERS *)(0xffc02000), // register base address
        FALSE,                              // in use flag
        1,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART1_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART1_TX,              // outbound PMAP
        ADI_INT_UART1_STATUS,               // error peripheral ID (no dedicated error interrupt)
        ADI_TMR_GP_TIMER_2,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
};


#endif  // end Brodie


/*********************
    Moy
**********************/

#if defined(__ADSP_MOY__)  // settings for BF50x

static ADI_UART Device[] = {                // Actual UART devices
    {                                   // device 0
        (ADI_UART_REGISTERS *)(0xffc00400), // register base address
        FALSE,                              // in use flag
        0,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART0_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART0_TX,              // outbound PMAP
        ADI_INT_UART0_STATUS,               // error peripheral ID
        ADI_TMR_GP_TIMER_2,                 // timer ID for autobaud
        0,                                  // not used
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
        ADI_UART_PIN_MUX_MODE_0,            // Use Port F for Tx & Rx
    },
    {                                   // device 1
        (ADI_UART_REGISTERS *)(0xffc02000), // register base address
        FALSE,                              // in use flag
        1,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        7,                                  // divisor capture bits
        ADI_DMA_PMAP_UART1_RX,              // inbound PMAP
        ADI_DMA_PMAP_UART1_TX,              // outbound PMAP
        ADI_INT_UART1_STATUS,               // error peripheral ID
        ADI_TMR_GP_TIMER_3,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
        ADI_UART_PIN_MUX_MODE_0,            // Use Port F for Tx & Rx
    },
};

#endif

/********************
    Delta
********************/

#if defined(__ADSP_DELTA__) // settings for BF59x class devices

static ADI_UART Device[] = {                // Actual UART devices
    {                                   // device 0
        (ADI_UART_REGISTERS *)(0xffc00400), // register base address
        FALSE,                              // in use flag
        0,                                  // device number
        ADI_DEV_MODE_UNDEFINED,             // Dataflow method
        ADI_DEV_DIRECTION_INBOUND,          // direction
        0,                                  // SCLK
        0,                                  // baud rate
        FALSE,                              // dataflow enabled flag
        FALSE,                              // line status interrupt flag
        0,                                  // divisor capture bits
        ADI_DMA_PMAP_UART0_RX,               // inbound PMAP
        ADI_DMA_PMAP_UART0_TX,               // outbound PMAP
        ADI_INT_UART0_STATUS,               // error peripheral ID
        ADI_TMR_GP_TIMER_0,                 // timer ID for autobaud
        0,                                  // timer IVG
        0,                                  // Full-duplex transfer when operated in bi-directional mode
#if defined(ADI_UART_INT)
        FALSE,                              // auto complete on special character flag
        '\n',                               // completion character
        NULL,                               // inbound head
        NULL,                               // inbound tail
        NULL,                               // outbound head
        NULL,                               // outbound tail
#endif
        NULL,                               // critical region argument
        NULL,                               // Device handle
        NULL,                               // DCB handle
        NULL,                               // device manager callback function
    },
};


#endif


/*********************************************************************

Static functions

*********************************************************************/


static u32 adi_pdd_Open(                        // Open a device
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          // device manager handle
    u32                     DeviceNumber,           // device number
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           // device handle
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            // pointer to PDD handle location
    ADI_DEV_DIRECTION       Direction,              // data direction
    void                    *pCriticalRegionArg,    // critical region imask storage location
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              // handle to the DMA manager
    ADI_DCB_HANDLE          DCBHandle,              // callback handle
    ADI_DCB_CALLBACK_FN     DMCallback              // device manager callback function
);

static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle    // PDD handle
);

static u32 adi_pdd_Read(        // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
);

static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
);

/* Sequentially read/writes data to a device*/
static u32 adi_pdd_SequentialIO(        /* Never called as UART doesnot support this function  */
    ADI_DEV_PDD_HANDLE      PDDHandle,
    ADI_DEV_BUFFER_TYPE     BufferType,
    ADI_DEV_BUFFER          *pBuffer
);

static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    u32 Command,                    // command ID
    void *pArg                      // pointer to argument
);

static ADI_INT_HANDLER(InterruptHandler);       // interrupt handler

static ADI_INT_HANDLER(TimerHandler);           // timer interrupt handler

static void uartUpdateDividers(     // updates dividers for baud rate/SLCK changes
    ADI_UART    *pDevice                // device
);

static void AutobaudCallback(   // autobaud callback function
    void *ClientHandle,             // pointer to device structure
    u32  Event,                     // event ID
    void *pArg                      // not used
);

static u32 uartSetTimer(            // configures appropriate timer for autobaud
    ADI_UART    *pDevice                    // device
);

static void uartAutobaudComplete(   // update UART Registers after autobaud & unhook timer interrupt
    ADI_UART *pDevice                   // device
);

/* Only for Interrupt driven driver */
#if defined(ADI_UART_INT)
static void uartBufferComplete(         // processes completed buffers
    ADI_UART                *pDevice,
    ADI_UART_RESERVED_AREA  *pReservedArea,
    ADI_UART_RESERVED_AREA  **pHead,
    ADI_UART_RESERVED_AREA  **pTail
);
#endif

/*************************************************************/
/* static functions for processor families with port control */
/*************************************************************/
#if defined (__ADSP_BRAEMAR__)     ||\
    defined (__ADSP_STIRLING__)    ||\
    defined (__ADSP_MOAB__)        ||\
    defined (__ADSP_KOOKABURRA__)  ||\
    defined (__ADSP_MOCKINGBIRD__) ||\
    defined (__ADSP_BRODIE__)      ||\
    defined (__ADSP_MOY__)         ||\
    defined (__ADSP_DELTA__)

static void uartSetPortControl(         // configures the port control registers
    ADI_UART    *pDevice,                   // pointer to the device
    u32         CTSRTSFlag                  // CTS/RTS flag (TRUE if enabling CTS/RTS only)
);

#endif



/*************************************************************/
/* static debug functions                                    */
/*************************************************************/
#if defined(ADI_DEV_DEBUG)

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);

#endif


/*********************************************************************

Global data

*********************************************************************/

#if defined(ADI_UART_DMA)
ADI_DEV_PDD_ENTRY_POINT ADIUARTDmaEntryPoint =
#else
ADI_DEV_PDD_ENTRY_POINT ADIUARTEntryPoint =
#endif
{
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens a UART for use
*
*********************************************************************/


static u32 adi_pdd_Open(                // Open a device
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          // device manager handle
    u32                     DeviceNumber,           // device number
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           // device handle
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            // pointer to PDD handle location
    ADI_DEV_DIRECTION       Direction,              // data direction
    void                    *pEnterCriticalArg,     // enter critical region parameter
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              // handle to the DMA manager
    ADI_DCB_HANDLE          DCBHandle,              // callback handle
    ADI_DCB_CALLBACK_FN     DMCallback              // client callback function
) {

    u32                     Result;                 // return value
    u16                     rbr;                    // variable to clear any pending interrupts
    u16                     lsr;                    // variable to clear any pending interrupts
    u16                     iir;                    // variable to clear any pending interrupts
    ADI_UART                *pDevice;               // pointer to the device we're working on
    ADI_INT_PERIPHERAL_ID   PeripheralID;       // peripheral ID
    ADI_DMA_CHANNEL_ID      DMAChannelID;       // DMA channel ID
    u32                     IVG;                // IVG
    u32                     i;                      // temporary
    u32                     SCLK;                   // SCLK
    void                    *pExitCriticalArg;      // exit critical region parameter
    u32                     Directive;              // port control directive


    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if (DeviceNumber >= ADI_UART_NUM_DEVICES) {     // check the device number
        return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
    }
#endif

    // get SCLK (now returns values in Hz!)
    if ((Result = adi_pwr_GetFreq(&i, &SCLK, &i)) != ADI_PWR_RESULT_SUCCESS) {
        return (Result);
    }

    // insure the device the client wants is available
    Result = ADI_DEV_RESULT_DEVICE_IN_USE;
    pDevice = &Device[DeviceNumber];
    pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
    if (pDevice->InUseFlag == FALSE) {
        pDevice->InUseFlag = TRUE;
        Result = ADI_DEV_RESULT_SUCCESS;
    }
    adi_int_ExitCriticalRegion(pExitCriticalArg);
    if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);

    // save the physical device handle in the client supplied location
    *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;

    // initialize the device settings
    pDevice->DeviceNumber = DeviceNumber;
    pDevice->Direction = Direction;
    pDevice->DeviceHandle = DeviceHandle;
    pDevice->DCBHandle = DCBHandle;
    pDevice->DMCallback = DMCallback;
    pDevice->pEnterCriticalArg = pEnterCriticalArg;
    pDevice->DataflowFlag = FALSE;
    pDevice->LineStatusFlag = FALSE;
    pDevice->SCLK = SCLK;
    // number of captured bits for default autobaud character ('@' character)
    pDevice->DivisorBits = 7;
    // UART Transfer mode - Full-duplex transfer when operated in bi-directional mode
    pDevice->TransferMode = 0;

/* Only for Interrupt driven driver */
#if defined(ADI_UART_INT)
    pDevice->AutoCompleteFlag = FALSE;
    // initialize data queues
    pDevice->pInboundHead = NULL;
    pDevice->pInboundTail = NULL;
    pDevice->pOutboundHead = NULL;
    pDevice->pOutboundTail = NULL;
#endif

    // set port control if we're a family that needs port control
#if defined (__ADSP_BRAEMAR__)     ||\
    defined (__ADSP_STIRLING__)    ||\
    defined (__ADSP_MOAB__)        ||\
    defined (__ADSP_KOOKABURRA__)  ||\
    defined (__ADSP_MOCKINGBIRD__) ||\
    defined (__ADSP_BRODIE__)      ||\
    defined (__ADSP_MOY__)         ||\
    defined (__ADSP_DELTA__)
    uartSetPortControl(pDevice, FALSE);
#endif

    // turn on the UART clocks, reset the device to default values and clear any pending interrupts
#if defined(__ADSP_MOAB__) || defined (__ADSP_MOY__)
    ADI_UART_LCR_SET(ADI_UART_LCR_DEFAULT);
    ADI_UART_MCR_SET(ADI_UART_MCR_DEFAULT);
    ADI_UART_GCTL_SET(ADI_UART_GCTL_DEFAULT);
    ADI_UART_IER_SET(ADI_UART_IER_DEFAULT);
    ADI_UART_DLL_SET(ADI_UART_DLL_DEFAULT);
    ADI_UART_DLH_SET(ADI_UART_DLH_DEFAULT);
    ADI_UART_LSR_CLEAR_ALL_LSE();
#else
    ADI_UART_LCR_SET(ADI_UART_LCR_DEFAULT);
    ADI_UART_MCR_SET(ADI_UART_MCR_DEFAULT);
    ADI_UART_GCTL_SET(ADI_UART_GCTL_DEFAULT);
    ADI_UART_IER_SET(ADI_UART_IER_DEFAULT);
    ADI_UART_LCR_SET_DLAB();
    ADI_UART_DLL_SET(ADI_UART_DLL_DEFAULT);
    ADI_UART_DLH_SET(ADI_UART_DLH_DEFAULT);
    ADI_UART_LCR_CLEAR_DLAB();
    rbr = ADI_UART_RBR_GET();
    lsr = ADI_UART_LSR_GET();
    iir = ADI_UART_IIR_GET();
#endif

    // make a convenient way to punch out on any error
    do {

        // IF (device is opened for inbound or bidirectional traffic)
        if ((pDevice->Direction == ADI_DEV_DIRECTION_INBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

            // get the DMA channel for the UART traffic
            if (adi_dma_GetMapping(pDevice->InboundPMAP, &DMAChannelID) != ADI_DMA_RESULT_SUCCESS) {
                Result = ADI_UART_RESULT_NOT_MAPPED_TO_DMA_CHANNEL;
                break;
            }

            // get the peripheral ID for the UART
            if (adi_dma_GetPeripheralInterruptID(DMAChannelID, &PeripheralID) != ADI_DMA_RESULT_SUCCESS) {
                Result = ADI_UART_RESULT_DMA_CHANNEL_NOT_MAPPED_TO_INTERRUPT;
                break;
            }

            // get the IVG
            if (adi_int_SICGetIVG(PeripheralID, &IVG) != ADI_INT_RESULT_SUCCESS) {
                Result = ADI_DEV_RESULT_BAD_IVG;
                break;
            }

/* Only for Interrupt driven driver */
#if defined(ADI_UART_INT)

            // hook the interrupt handler
            if(adi_int_CECHook(IVG, InterruptHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS){
                Result = ADI_DEV_RESULT_CANT_HOOK_INTERRUPT;
                break;
            }

            // allow this interrupt to wake us up and allow it to be passed to the core
            adi_int_SICWakeup(PeripheralID, TRUE);
            adi_int_SICEnable(PeripheralID);

#endif

        // ENDIF
        }

        // IF (device is opened for outbound or bidirectional traffic)
        if ((pDevice->Direction == ADI_DEV_DIRECTION_OUTBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

            // get the DMA channel for the UART traffic
            if (adi_dma_GetMapping(pDevice->OutboundPMAP, &DMAChannelID) != ADI_DMA_RESULT_SUCCESS) {
                Result = ADI_UART_RESULT_NOT_MAPPED_TO_DMA_CHANNEL;
                break;
            }

            // get the peripheral ID for the UART
            if (adi_dma_GetPeripheralInterruptID(DMAChannelID, &PeripheralID) != ADI_DMA_RESULT_SUCCESS) {
                Result = ADI_UART_RESULT_DMA_CHANNEL_NOT_MAPPED_TO_INTERRUPT;
                break;
            }

            // get the IVG
            if (adi_int_SICGetIVG(PeripheralID, &IVG) != ADI_INT_RESULT_SUCCESS) {
                Result = ADI_DEV_RESULT_BAD_IVG;
                break;
            }

/* Only for Interrupt driven driver */
#if defined(ADI_UART_INT)

            // hook the interrupt handler
            if(adi_int_CECHook(IVG, InterruptHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS){
                Result = ADI_DEV_RESULT_CANT_HOOK_INTERRUPT;
                break;
            }

            // allow this interrupt to wake us up and allow it to be passed to the core
            adi_int_SICWakeup(PeripheralID, TRUE);
            adi_int_SICEnable(PeripheralID);

#endif

        // ENDIF
        }

    // end DO
    } while (0);

    // free the device if we had any errors
    if (Result != ADI_DEV_RESULT_SUCCESS) {
        pDevice->InUseFlag = FALSE;
    }

    // return
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Close
*
*   Description:    Closes down a UART
*
*********************************************************************/


static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle            // PDD handle
) {

    u32         Result;     // return value
    ADI_UART    *pDevice;   // pointer to the device we're working on
    ADI_INT_PERIPHERAL_ID   PeripheralID;       // peripheral ID
    ADI_DMA_CHANNEL_ID      DMAChannelID;       // DMA channel ID
    u32                     IVG;                // IVG

    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    // avoid casts
    pDevice = (ADI_UART *)PDDHandle;

    // make a convenient way to punch out on any error
    do {

        // disable dataflow and line status interrupt
        if ((Result = adi_pdd_Control (PDDHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)FALSE)) != ADI_DEV_RESULT_SUCCESS) {
            break;
        }

        // disable dataflow and line status interrupt
        if ((Result = adi_pdd_Control (PDDHandle, ADI_UART_CMD_SET_LINE_STATUS_EVENTS, (void *)FALSE)) != ADI_DEV_RESULT_SUCCESS) {
            break;
        }

        // IF (device is opened for inbound or bidirectional traffic)
        if ((pDevice->Direction == ADI_DEV_DIRECTION_INBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

            // get the DMA channel for the UART traffic
            if (adi_dma_GetMapping(pDevice->InboundPMAP, &DMAChannelID) != ADI_DMA_RESULT_SUCCESS) {
                Result = ADI_UART_RESULT_NOT_MAPPED_TO_DMA_CHANNEL;
                break;
            }

            // get the peripheral ID for the UART
            if (adi_dma_GetPeripheralInterruptID(DMAChannelID, &PeripheralID) != ADI_DMA_RESULT_SUCCESS) {
                Result = ADI_UART_RESULT_DMA_CHANNEL_NOT_MAPPED_TO_INTERRUPT;
                break;
            }

            // get the IVG
            if (adi_int_SICGetIVG(PeripheralID, &IVG) != ADI_INT_RESULT_SUCCESS) {
                Result = ADI_DEV_RESULT_BAD_IVG;
                break;
            }

/* Only for Interrupt driven driver */
#if defined(ADI_UART_INT)

            // disable interrupts and disallow wakeup
            adi_int_SICDisable(PeripheralID);
            adi_int_SICWakeup(PeripheralID, FALSE);

            // unhook the interrupt handler
            if(adi_int_CECUnhook(IVG, InterruptHandler, pDevice) != ADI_INT_RESULT_SUCCESS){
                Result = ADI_DEV_RESULT_CANT_UNHOOK_INTERRUPT;
                break;
            }

#endif

        // ENDIF
        }

        // IF (device is opened for outbound or bidirectional traffic)
        if ((pDevice->Direction == ADI_DEV_DIRECTION_OUTBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

            // get the DMA channel for the UART traffic
            if (adi_dma_GetMapping(pDevice->OutboundPMAP, &DMAChannelID) != ADI_DMA_RESULT_SUCCESS) {
                Result = ADI_UART_RESULT_NOT_MAPPED_TO_DMA_CHANNEL;
                break;
            }

            // get the peripheral ID for the UART
            if (adi_dma_GetPeripheralInterruptID(DMAChannelID, &PeripheralID) != ADI_DMA_RESULT_SUCCESS) {
                Result = ADI_UART_RESULT_DMA_CHANNEL_NOT_MAPPED_TO_INTERRUPT;
                break;
            }

            // get the IVG
            if (adi_int_SICGetIVG(PeripheralID, &IVG) != ADI_INT_RESULT_SUCCESS) {
                Result = ADI_DEV_RESULT_BAD_IVG;
                break;
            }

/* IF (this is the Interrupt version of the driver) */
#if defined(ADI_UART_INT)

            // disable interrupts and disallow wakeup
            adi_int_SICDisable(PeripheralID);
            adi_int_SICWakeup(PeripheralID, FALSE);

            // unhook the interrupt handler
            if(adi_int_CECUnhook(IVG, InterruptHandler, pDevice) != ADI_INT_RESULT_SUCCESS){
                Result = ADI_DEV_RESULT_CANT_UNHOOK_INTERRUPT;
                break;
            }

#endif

        // ENDIF
        }

    // end DO
    } while (0);

    // free the device and stop the clocks if we had no errors
    if (Result == ADI_DEV_RESULT_SUCCESS) {
        ADI_UART_GCTL_CLEAR_UCEN();
        pDevice->InUseFlag = FALSE;
    }

    // return
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Read
*
*   Description:    Provides buffers to store data when data is received
*                   from the UART
*
*********************************************************************/


static u32 adi_pdd_Read(            // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,      // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType,     // buffer type
    ADI_DEV_BUFFER      *pBuffer        // pointer to buffer
){

    u32         Result;             // return value

/* IF (this is the DMA version of the driver) */
#if defined(ADI_UART_DMA)

        // return an error
        Result = ADI_DEV_RESULT_FAILED;

/* ELSE (this is the interrupt driven version) */
#else

    ADI_UART                *pDevice;           // pointer to the device we're working on
    ADI_DEV_1D_BUFFER       *pWorkingBuffer;    // pointer to the buffer we're working on
    ADI_DEV_1D_BUFFER       *pLastBuffer;       // pointer to the last buffer in the chain
    ADI_UART_RESERVED_AREA  *pReservedArea;     // pointer to the reserved area at the top of a buffer
    void                    *pExitCriticalArg;  // exit critical region parameter

    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
    if (BufferType != ADI_DEV_1D) {
        return (ADI_DEV_RESULT_BUFFER_TYPE_INCOMPATIBLE);
    }
#endif

    // avoid casts
    pDevice = (ADI_UART *)PDDHandle;
    Result = ADI_DEV_RESULT_SUCCESS;

    // Set our data pointers and byte counts in the reserved area of each buffer
    // and find the last buffer in the chain
    for (pWorkingBuffer = &pBuffer->OneD; pWorkingBuffer != NULL; pWorkingBuffer = pWorkingBuffer->pNext) {
        pReservedArea = (ADI_UART_RESERVED_AREA *)pWorkingBuffer->Reserved;
        pReservedArea->pNext = (ADI_UART_RESERVED_AREA *)(pWorkingBuffer->pNext)->Reserved;
        pReservedArea->pCurrentDataPointer = pWorkingBuffer->Data;
        pReservedArea->BytesRemaining = pWorkingBuffer->ElementCount * pWorkingBuffer->ElementWidth;
        pLastBuffer = pWorkingBuffer;
    }
    ((ADI_UART_RESERVED_AREA *)pLastBuffer->Reserved)->pNext = NULL;

    // add the buffers to the end of the inbound queue
    pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);
    if (pDevice->pInboundHead == NULL) {
        pDevice->pInboundHead = (ADI_UART_RESERVED_AREA *)pBuffer->OneD.Reserved;
        pDevice->pInboundTail = (ADI_UART_RESERVED_AREA *)pLastBuffer->Reserved;
    } else {
        pDevice->pInboundTail->pNext = (ADI_UART_RESERVED_AREA *)pBuffer->OneD.Reserved;
        pDevice->pInboundTail = (ADI_UART_RESERVED_AREA *)pLastBuffer->Reserved;
    }
    adi_int_ExitCriticalRegion(pExitCriticalArg);

#endif

    // return
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Provides buffers containing data to be transmitted
*                   out through the UART
*
*********************************************************************/


static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,          // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType,         // buffer type
    ADI_DEV_BUFFER      *pBuffer            // pointer to buffer
){

    u32                     Result;             // return value

/* IF (this is the DMA version of the driver) */
#if defined(ADI_UART_DMA)

        // return an error
        Result = ADI_DEV_RESULT_FAILED;

/* ELSE (this is the interrupt driven version) */
#else

    ADI_UART                *pDevice;           // pointer to the device we're working on
    ADI_DEV_1D_BUFFER       *pWorkingBuffer;    // pointer to the buffer we're working on
    ADI_DEV_1D_BUFFER       *pLastBuffer;       // pointer to the last buffer in the chain
    ADI_UART_RESERVED_AREA  *pReservedArea;     // pointer to the reserved area at the top of a buffer
    u8                      Data;               // buffer to hold data if we need to restart transmission
    void                    *pExitCriticalArg;  // exit critical region parameter

    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
    if (BufferType != ADI_DEV_1D) {
        return (ADI_DEV_RESULT_BUFFER_TYPE_INCOMPATIBLE);
    }
#endif

    // avoid casts
    pDevice = (ADI_UART *)PDDHandle;
    Result = ADI_DEV_RESULT_SUCCESS;

    // Set our data pointers and byte counts in the reserved area of each buffer
    // and find the last buffer in the chain
    for (pWorkingBuffer = &pBuffer->OneD; pWorkingBuffer != NULL; pWorkingBuffer = pWorkingBuffer->pNext) {
        pReservedArea = (ADI_UART_RESERVED_AREA *)pWorkingBuffer->Reserved;
        pReservedArea->pNext = (ADI_UART_RESERVED_AREA *)(pWorkingBuffer->pNext)->Reserved;
        pReservedArea->pCurrentDataPointer = pWorkingBuffer->Data;
        pReservedArea->BytesRemaining = pWorkingBuffer->ElementCount * pWorkingBuffer->ElementWidth;
        pLastBuffer = pWorkingBuffer;
    }
    ((ADI_UART_RESERVED_AREA *)pLastBuffer->Reserved)->pNext = NULL;

    // add the buffers to the end of the outbound queue
    pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);
    if (pDevice->pOutboundHead == NULL) {
        pDevice->pOutboundHead = (ADI_UART_RESERVED_AREA *)pBuffer->OneD.Reserved;
        pDevice->pOutboundTail = (ADI_UART_RESERVED_AREA *)pLastBuffer->Reserved;
    } else {
        pDevice->pOutboundTail->pNext = (ADI_UART_RESERVED_AREA *)pBuffer->OneD.Reserved;
        pDevice->pOutboundTail = (ADI_UART_RESERVED_AREA *)pLastBuffer->Reserved;
    }
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // enable transmit interrupts
    ADI_UART_IER_SET_ETBEI();

#endif

    // return
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_SequentialIO
*
*   Description:    UART does not support this function
*
*********************************************************************/
static u32 adi_pdd_SequentialIO(        /* Never called as UART doesnot support this function  */
    ADI_DEV_PDD_HANDLE      PDDHandle,
    ADI_DEV_BUFFER_TYPE     BufferType,
    ADI_DEV_BUFFER          *pBuffer
){

    /* this function is not supported by UART */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_Control
*
*   Description:    Configures the UART
*
*********************************************************************/


static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE  PDDHandle,          // PDD handle
    u32                 Command,            // command ID
    void                *Value              // command specific value
) {

    ADI_UART                    *pDevice;           // pointer to the device we're working on
    u32                         Result;             // return value
    u32                         ErrorIVG;           // error IVG
    void                        *pExitCriticalArg;  // exit critical region parameter
    u16                         u16Value;           // u16 type to avoid casts/warnings etc.
    u16                         tmp;                // temp value for calculations
/* IF (this is the Interrupt version of the driver) */
#if defined(ADI_UART_INT)
    ADI_UART_RESERVED_AREA      *pReservedArea;     // pointer to reserved area of buffer
#endif
    ADI_DEV_1D_BUFFER           *pBuffer;           // pointer to buffer

    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    // avoid casts
    pDevice = (ADI_UART *)PDDHandle;
    u16Value = ((u16)((u32)Value));

    // assume we're going to be successful
    Result = ADI_DEV_RESULT_SUCCESS;

    // CASEOF (Command ID)
    switch (Command) {

        // CASE (control dataflow)
        case (ADI_DEV_CMD_SET_DATAFLOW):

            // insure we have a valid baud rate if we're not autobauding and we're enabling data flow
#if defined(ADI_DEV_DEBUG)
            if ((pDevice->BaudRate == 0) && ((u32)Value == TRUE)) {
                Result = ADI_UART_RESULT_BAD_BAUD_RATE;
                break;
            }
#endif

            // protect us
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // IF (opened for outbound or bidirectional)
            if ((pDevice->Direction == ADI_DEV_DIRECTION_OUTBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

                // enable or disable the transmit interrupt accordingly
                if (((u32)Value) == TRUE) {
                    ADI_UART_IER_SET_ETBEI();
                } else {
                    ADI_UART_IER_CLEAR_ETBEI();
                }

            // ENDIF
            }

            // IF (opened for inbound or bidirectional)
            if ((pDevice->Direction == ADI_DEV_DIRECTION_INBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)) {

                // enable or disable the receive interrupt accordingly
                // clear any pending receive interrupts before enabling
                if (((u32)Value) == TRUE) {
                    tmp = ADI_UART_RBR_GET();
                    ADI_UART_IER_SET_ERBFI();
                } else {
                    ADI_UART_IER_CLEAR_ERBFI();
                }

            // ENDIF
            }

            /* IF (Bi-directional dataflow) */
            if (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)
            {
                /* IF (disable UART Traffic or enable UART Transfer only) */
                if (pDevice->TransferMode & 1)
                {
                    /* Disable Receive interrupt */
                    ADI_UART_IER_CLEAR_ERBFI();
                }

                /* IF (disable UART Traffic or enable UART Receive only) */
                if (pDevice->TransferMode & 2)
                {
                    /* Disable Transmit interrupt */
                    ADI_UART_IER_CLEAR_ETBEI();
                }
            }

            // update the dataflow flag
            pDevice->DataflowFlag = ((u32)Value);

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);
            break;

        // CASE (query for processor DMA support)
        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

/* IF (this is the DMA version of the driver) */
#if defined(ADI_UART_DMA)
            // yes, we support DMA
            (*(u32 *)Value) = TRUE;
/* ELSE (this is the interrupt version of the driver) */
#else
            // no, we don't support it
            (*(u32 *)Value) = FALSE;
#endif
            break;

/* IF (this is the DMA version of the driver) */
#if defined(ADI_UART_DMA)
        // CASE (query for our inbound DMA peripheral ID)
        case (ADI_DEV_CMD_GET_INBOUND_DMA_PMAP_ID):

            // pass inbound DMA channel id to the device manager
            *((ADI_DMA_PMAP *)Value) = pDevice->InboundPMAP;
            break;

        // CASE (query for our outbound DMA peripheral ID)
        case (ADI_DEV_CMD_GET_OUTBOUND_DMA_PMAP_ID):
            // pass outbound DMA channel id to the device manager
            *((ADI_DMA_PMAP *)Value) = pDevice->OutboundPMAP;
            break;
#endif

        // CASE (set # of data bits, number of bits is the argument)
        case (ADI_UART_CMD_SET_DATA_BITS):

            // update the LCR accordingly
            ADI_UART_LCR_SET_WLS(u16Value - 5);
            break;

        // CASE (set # of stop bits, number of bits is the argument)
        case (ADI_UART_CMD_SET_STOP_BITS):

            // update the LCR accordingly
            if (u16Value == 1) {
                ADI_UART_LCR_CLEAR_STB();
            } else {
                ADI_UART_LCR_SET_STB();
            }
            break;

        // CASE (enable/disable parity)
        case (ADI_UART_CMD_ENABLE_PARITY):

            // update the LCR accordingly
            if (u16Value) {
                ADI_UART_LCR_SET_PEN();
            } else {
                ADI_UART_LCR_CLEAR_PEN();
            }
            break;

        // CASE (set parity, arg even means parity set even, arg odd means parity set odd)
        case (ADI_UART_CMD_SET_PARITY):

            // update the LCR accordingly
            if (u16Value & 0x1) {
                ADI_UART_LCR_CLEAR_EPS();
            } else {
                ADI_UART_LCR_SET_EPS();
            }
            break;

        // CASE (enable line status events (same as error reporting))
        case (ADI_UART_CMD_SET_LINE_STATUS_EVENTS):
        case (ADI_DEV_CMD_SET_ERROR_REPORTING):

            // make sure we're actually changing the line event status
            if ((u32)Value != pDevice->LineStatusFlag) {

                // get the error IVG
                adi_int_SICGetIVG(pDevice->ErrorPeripheralID, &ErrorIVG);

                // IF (enabling error interrupts)
                if ((u32)Value == TRUE) {

                    // hook the error interrupt handler
                    if(adi_int_CECHook(ErrorIVG, InterruptHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS){
                        return (ADI_DEV_RESULT_CANT_HOOK_INTERRUPT);
                    }

                    // allow the error interrupt to wake us up and allow it to be passed to the core
                    adi_int_SICWakeup(pDevice->ErrorPeripheralID, TRUE);
                    adi_int_SICEnable(pDevice->ErrorPeripheralID);

                    // tell the UART to generate line status interrupts
                    ADI_UART_IER_SET_ELSI();

                // ELSE
                } else {

                    // tell the UART not to generate line status errors
                    ADI_UART_IER_CLEAR_ELSI();

                    // don't allow the error interrupt to wake us up or allow it to be passed to the core
                    adi_int_SICWakeup(pDevice->ErrorPeripheralID, FALSE);
                    adi_int_SICDisable(pDevice->ErrorPeripheralID);

                    // unhook the error interrupt handler
                    if ((Result = adi_int_CECUnhook(ErrorIVG, InterruptHandler, pDevice)) != ADI_INT_RESULT_SUCCESS) {
                        return (ADI_DEV_RESULT_CANT_UNHOOK_INTERRUPT);
                    }

                // ENDIF
                }

                // update our line status flag
                pDevice->LineStatusFlag = (u32)Value;

            // ENDIF
            }
            break;

        // CASE (set baud rate)
        case (ADI_UART_CMD_SET_BAUD_RATE):

            // update the device with the baud rate value
            pDevice->BaudRate = (u32)Value;

            // update the dividers
            uartUpdateDividers(pDevice);
            break;

        // CASE (sense TEMT (are TSR and UART_THR both empty))
        case (ADI_UART_CMD_GET_TEMT):

            // sense and return the value
            tmp = ADI_UART_LSR_GET();
            if (ADI_UART_LSR_IS_TEMT(tmp)) {
                *((u32 *)Value) = TRUE;
            } else {
                *((u32 *)Value) = FALSE;
            }
            break;

        // CASE (set Divisor capture bits used in Autobaud operation)
        case (ADI_UART_CMD_SET_DIVISOR_BITS):

            // save the bits
            pDevice->DivisorBits = (u32)Value;
            break;

        // CASE (set Autobaud initiation character used in Autobaud operation)
        case (ADI_UART_CMD_SET_AUTOBAUD_CHAR):

            // calculate divisor bits from the character value
            tmp = 0x01;                     // used to check first occurance of 1 in character
            pDevice->DivisorBits = 0x01;    // initialise as 1 (start bit)

            // check each bit of the character for first occurance of 1
            while (!(u16Value & tmp)){
                tmp <<= 1;
                pDevice->DivisorBits +=1;
                if (pDevice->DivisorBits == 9) // character set is a NULL character
                    break;
            }
            break;

        // CASE (Start Autobaud detection)
        case (ADI_UART_CMD_AUTOBAUD):

            // start timer
            Result = uartSetTimer(pDevice);
            break;

/* IF (this is the Interrupt version of the driver) */
#if defined(ADI_UART_INT)
        // CASE (query for 2D support)
        case (ADI_DEV_CMD_GET_2D_SUPPORT):


            // no, we don't support it
            (*(u32 *)Value) = FALSE;
            break;

        // CASE (set dataflow method)
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):

            // we only support chained descriptors with or without loopback
            if (((ADI_DEV_MODE)Value == ADI_DEV_MODE_CHAINED) || ((ADI_DEV_MODE)Value == ADI_DEV_MODE_CHAINED_LOOPBACK)) {
                pDevice->DataflowMethod = ((ADI_DEV_MODE)Value);
            } else {
                Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            }
            break;

        // CASE (enable auto completion)
        case (ADI_UART_CMD_ENABLE_AUTO_COMPLETE):

            // enable it and save the character
            pDevice->AutoCompleteFlag = TRUE;
            pDevice->AutoCompleteChar = (u8)u16Value;
            break;

        // CASE (disable auto complete)
        case (ADI_UART_CMD_DISABLE_AUTO_COMPLETE):

            // disable it
            pDevice->AutoCompleteFlag = FALSE;
            break;

        // CASE (get processed count of current buffer)
        case (ADI_UART_CMD_GET_INBOUND_PROCESSED_ELEMENT_COUNT):
        case (ADI_UART_CMD_GET_OUTBOUND_PROCESSED_ELEMENT_COUNT):

            // protect us
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // point to the reserved area of the currrent buffer
            if (Command == ADI_UART_CMD_GET_INBOUND_PROCESSED_ELEMENT_COUNT) {
                pReservedArea = pDevice->pInboundHead;
            } else {
                pReservedArea = pDevice->pOutboundHead;
            }

            // IF (no buffer)
            if (pReservedArea == NULL) {

                // return 0 for the count and that there is no buffer
                *((u32 *)Value) = 0;
                Result = ADI_UART_RESULT_NO_BUFFER;

            // ELSE
            } else {

                // calculate how much has been processed
                pBuffer = (ADI_DEV_1D_BUFFER *)pReservedArea;
                *((u32 *)Value) = (pBuffer->ElementCount * pBuffer->ElementWidth) - pReservedArea->BytesRemaining;

            // ENDIF
            }

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);
            break;

        // CASE (complete processing of current buffer)
        case (ADI_UART_CMD_COMPLETE_INBOUND_BUFFER):
        case (ADI_UART_CMD_COMPLETE_OUTBOUND_BUFFER):

            // protect us
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

            // complete processing of the buffer if we have a current buffer
            if (Command == ADI_UART_CMD_COMPLETE_INBOUND_BUFFER) {
                if (pDevice->pInboundHead == NULL) {
                    Result = ADI_UART_RESULT_NO_BUFFER;
                } else {
                    uartBufferComplete(pDevice, pDevice->pInboundHead, &pDevice->pInboundHead, &pDevice->pInboundTail);
                }
            } else {
                if (pDevice->pOutboundHead == NULL) {
                    Result = ADI_UART_RESULT_NO_BUFFER;
                } else {
                    uartBufferComplete(pDevice, pDevice->pOutboundHead, &pDevice->pOutboundHead, &pDevice->pOutboundTail);
                }
            }

            // unprotect
            adi_int_ExitCriticalRegion(pExitCriticalArg);
            break;
#endif

#if defined(__ADSP_MOAB__) || defined (__ADSP_MOY__) // these commands are only supported on Moab and Moy devices

        // CASE (enable/disable CTS/RTS)
        case (ADI_UART_CMD_ENABLE_CTS_RTS):

            // IF (enabling)
            if (u16Value) {

                // enable port control
                uartSetPortControl(pDevice, TRUE);

                // enable CTS/RTS
                ADI_UART_MCR_ENABLE_CTS_RTS();

            // ELSE
            } else {

                // disable CTS/RTS
                ADI_UART_MCR_DISABLE_CTS_RTS();

            // ENDIF
            }
            break;

        // CASE (set CTS/RTS polarity)
        case (ADI_UART_CMD_SET_CTS_RTS_POLARITY):

            // set it
            if (u16Value) {
                ADI_UART_MCR_SET_POSITIVE_CTS_RTS_POLARITY();
            } else {
                ADI_UART_MCR_SET_NEGATIVE_CTS_RTS_POLARITY();
            }
            break;

        // CASE (set CTS/RTS threshold)
        case (ADI_UART_CMD_SET_CTS_RTS_THRESHOLD):

            // set it
            if (u16Value) {
                ADI_UART_MCR_SET_HIGH_CTS_RTS_THRESHOLD();
            } else {
                ADI_UART_MCR_SET_LOW_CTS_RTS_THRESHOLD();
            }
            break;

#endif // MOAB only commands

        // CASE (sets UART data transfer mode)
        case (ADI_UART_CMD_SET_TRANSFER_MODE):

            /* Save UART Transfer mode */
            pDevice->TransferMode = (u32)Value;

            /* IF (Bi-directional dataflow & Dataflow is enabled) */
            if ((pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL) && (pDevice->DataflowFlag == TRUE))
            {
                /* protect us */
                pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

                /* Enable Transmit interrupt */
                ADI_UART_IER_SET_ETBEI();
                /* Enable Receive interrupt */
                /* clear any pending receive interrupts before enabling */
                tmp = ADI_UART_RBR_GET();
                ADI_UART_IER_SET_ERBFI();

                /* IF (disable UART Traffic or enable UART Transfer only) */
                if (pDevice->TransferMode & 1)
                {
                    /* Disable Receive interrupt */
                    ADI_UART_IER_CLEAR_ERBFI();
                }

                /* IF (disable UART Traffic or enable UART Receive only) */
                if (pDevice->TransferMode & 2)
                {
                    /* Disable Transmit interrupt */
                    ADI_UART_IER_CLEAR_ETBEI();
                }

                /* unprotect */
                adi_int_ExitCriticalRegion(pExitCriticalArg);
            }
            break;

        /* CASE (Set Pin Mux Mode) */
        case (ADI_UART_CMD_SET_PIN_MUX_MODE):

/* For BF50x Family */
#if defined (__ADSP_MOY__)
            /* Update pin mux mode selection */
            pDevice->ePinMuxMode = ((ADI_UART_PIN_MUX_MODE) Value);
            /* Update port control settings */
            uartSetPortControl(pDevice, FALSE);
#endif

            break;

        // CASEELSE
        default:

            // we don't understand this command
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;

    // ENDCASE
    }

    // return
    return(Result);
}

#if defined(__ADSP_MOAB__) || defined (__ADSP_MOY__) /* Moab & Moy devices */

/*********************************************************************
*
*   Function:       InterruptHandler (Moab devices)
*
*   Description:    Processes events in response to UART interrupts
*
*********************************************************************/

static ADI_INT_HANDLER(InterruptHandler)            // UART interrupt handler
{

    u16 lsr;
    ADI_UART *pDevice;
    u8 Data;
    ADI_INT_HANDLER_RESULT Result;
#if defined(ADI_UART_INT)
    ADI_UART_RESERVED_AREA *pReservedArea;
#endif

    // assume the interrupt is not for us
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    // point to the UART
    pDevice = (ADI_UART *)ClientArg;

    // read the LSR register
    lsr = ADI_UART_LSR_GET();

/* IF (this is the Interrupt version of the driver) */
#if defined(ADI_UART_INT)
    // IF (a character has been received)
    if (ADI_UART_LSR_IS_DR(lsr)) {

        // get the character and clear the interrupt
        Data = ADI_UART_RBR_GET();

        // IF (we have a buffer to store it in)
        if ((pReservedArea = pDevice->pInboundHead) != NULL) {

            // add the new character into the buffer and update our processed count
            *pReservedArea->pCurrentDataPointer++ = Data;
            pReservedArea->BytesRemaining--;

            // IF (buffer is now fully processed or auto-complete character received)
            if ((pReservedArea->BytesRemaining == 0) || (pDevice->AutoCompleteFlag && (pDevice->AutoCompleteChar == Data))) {

                // make any callbacks, remove from queue etc.
                uartBufferComplete(pDevice, pReservedArea, &pDevice->pInboundHead, &pDevice->pInboundTail);

            // ENDIF
            }

        // ENDIF
        }

        // looks like the interrupt was for us
        Result = ADI_INT_RESULT_PROCESSED;

    // ENDIF
    }

    // IF (the LSR register is telling us a THR empty interrupt triggered and Tx interrupts are enabled)
    if (ADI_UART_LSR_IS_THRE(lsr) && ADI_UART_IER_IS_ETBEI()) {

        // IF (there is a buffer at the head of the queue)
        if ((pReservedArea = pDevice->pOutboundHead) != NULL) {

            // IF (buffer has been completely sent out)
            if (pReservedArea->BytesRemaining == 0) {

                // make any callbacks, remove from queue etc.
                uartBufferComplete(pDevice, pReservedArea, &pDevice->pOutboundHead, &pDevice->pOutboundTail);

            // ENDIF
            }

            // IF (there is still a buffer to transmit)
            if ((pReservedArea = pDevice->pOutboundHead) != NULL) {

                // send the next character in the buffer
                ADI_UART_THR_SET(*pReservedArea->pCurrentDataPointer);

                // increment our data pointer and decrement the number of bytes still to transmit
                *pReservedArea->pCurrentDataPointer++;
                pReservedArea->BytesRemaining--;

            // ELSE
            } else {

                // stop Tx interrupts
                // (was originally reading IIR to clear the TX interrupt but issue
                // 05000215 doesn't seem to be getting fixed
                ADI_UART_IER_CLEAR_ETBEI();

            // ENDIF
            }

        // ELSE
        } else {

            // stop Tx interrupts
            // (was originally reading IIR to clear the TX interrupt but issue
            // 05000215 doesn't seem to be getting fixed
            ADI_UART_IER_CLEAR_ETBEI();

        // ENDIF
        }

        // looks like the interrupt was for us
        Result = ADI_INT_RESULT_PROCESSED;

    // ENDIF
    }

#endif

    // generate events for any line status interrupts if they're enabled
    if ((pDevice->LineStatusFlag == TRUE) && ADI_UART_LSR_IS_ANY_LSE(lsr)) {
        if (ADI_UART_LSR_IS_OE(lsr)) {
            (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_UART_EVENT_OVERRUN_ERROR, NULL);
        }
        if (ADI_UART_LSR_IS_PE(lsr)) {
            (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_UART_EVENT_PARITY_ERROR, NULL);
        }
        if (ADI_UART_LSR_IS_FE(lsr)) {
            (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_UART_EVENT_FRAMING_ERROR, NULL);
        }
        if (ADI_UART_LSR_IS_BI(lsr)) {
            (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_UART_EVENT_BREAK_INTERRUPT, NULL);
        }
        Result = ADI_INT_RESULT_PROCESSED;
    }

    // return
    return (Result);
}

#else   /* non-Moab/Moy devices */

/*********************************************************************
*
*   Function:       InterruptHandler (non-Moab/Moy devices)
*
*   Description:    Processes events in response to UART interrupts
*
*********************************************************************/

static ADI_INT_HANDLER(InterruptHandler)            // UART interrupt handler
{

    u16 lsr;
    ADI_UART *pDevice;
    u8 Data;
    ADI_INT_HANDLER_RESULT Result;

/* IF (this is the Interrupt version of the driver) */
#if defined(ADI_UART_INT)
    ADI_UART_RESERVED_AREA *pReservedArea;
#endif

    // assume the interrupt is not for us
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    // point to the UART
    pDevice = (ADI_UART *)ClientArg;

    // read the LSR register
    lsr = ADI_UART_LSR_GET();

/* IF (this is the Interrupt version of the driver) */
#if defined(ADI_UART_INT)
    // IF (a character has been received)
    if (ADI_UART_LSR_IS_DR(lsr)) {

        // get the character and clear the interrupt
        Data = ADI_UART_RBR_GET();

        // IF (we have a buffer to store it in)
        if ((pReservedArea = pDevice->pInboundHead) != NULL) {

            // add the new character into the buffer and update our processed count
            *pReservedArea->pCurrentDataPointer++ = Data;
            pReservedArea->BytesRemaining--;

            // IF (buffer is now fully processed or auto-complete character received)
            if ((pReservedArea->BytesRemaining == 0) || (pDevice->AutoCompleteFlag && (pDevice->AutoCompleteChar == Data))) {

                // make any callbacks, remove from queue etc.
                uartBufferComplete(pDevice, pReservedArea, &pDevice->pInboundHead, &pDevice->pInboundTail);

            // ENDIF
            }

        // ENDIF
        }

        // looks like the interrupt was for us
        Result = ADI_INT_RESULT_PROCESSED;

    // ENDIF
    }

    // IF (the LSR register is telling us a THR empty interrupt triggered and Tx interrupts are enabled)
    if (ADI_UART_LSR_IS_THRE(lsr) && ADI_UART_IER_IS_ETBEI()) {

        // IF (there is a buffer at the head of the queue)
        if ((pReservedArea = pDevice->pOutboundHead) != NULL) {

            // IF (buffer has been completely sent out)
            if (pReservedArea->BytesRemaining == 0) {

                // make any callbacks, remove from queue etc.
                uartBufferComplete(pDevice, pReservedArea, &pDevice->pOutboundHead, &pDevice->pOutboundTail);

            // ENDIF
            }

            // IF (there is still a buffer to transmit)
            if ((pReservedArea = pDevice->pOutboundHead) != NULL) {

                // send the next character in the buffer
                ADI_UART_THR_SET(*pReservedArea->pCurrentDataPointer);

                // increment our data pointer and decrement the number of bytes still to transmit
                *pReservedArea->pCurrentDataPointer++;
                pReservedArea->BytesRemaining--;

            // ELSE
            } else {

                // stop Tx interrupts
                // (was originally reading IIR to clear the TX interrupt but issue
                // 05000215 doesn't seem to be getting fixed
                ADI_UART_IER_CLEAR_ETBEI();

            // ENDIF
            }

        // ELSE
        } else {

            // stop Tx interrupts
            // (was originally reading IIR to clear the TX interrupt but issue
            // 05000215 doesn't seem to be getting fixed
            ADI_UART_IER_CLEAR_ETBEI();

        // ENDIF
        }

        // looks like the interrupt was for us
        Result = ADI_INT_RESULT_PROCESSED;

    // ENDIF
    }

#endif

    // generate events for any line status interrupts if they're enabled
    if ((pDevice->LineStatusFlag == TRUE) && ADI_UART_LSR_IS_ANY_LSE(lsr)) {
        if (ADI_UART_LSR_IS_OE(lsr)) {
            (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_UART_EVENT_OVERRUN_ERROR, NULL);
        }
        if (ADI_UART_LSR_IS_PE(lsr)) {
            (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_UART_EVENT_PARITY_ERROR, NULL);
        }
        if (ADI_UART_LSR_IS_FE(lsr)) {
            (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_UART_EVENT_FRAMING_ERROR, NULL);
        }
        if (ADI_UART_LSR_IS_BI(lsr)) {
            (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_UART_EVENT_BREAK_INTERRUPT, NULL);
        }
        Result = ADI_INT_RESULT_PROCESSED;
    }

    // return
    return (Result);
}

#endif /* Moab vs. non-Moab devices */



/*********************************************************************

    Function:       Autobaud Callback

    Description:    Performs the processing required when the autobaud
                    timer generates an interrupt.

*********************************************************************/

static void AutobaudCallback(   // autobaud callback function
    void *ClientHandle,             // pointer to device structure
    u32  Event,                     // event ID
    void *pArg)                     // not used
{

    ADI_TMR_GP_CMD_VALUE_PAIR ConfigurationTable [] = { // configuration table for the timer
        { ADI_TMR_GP_CMD_SET_TIMER_MODE,            (void *)1       },
        { ADI_TMR_GP_CMD_SET_INTERRUPT_ENABLE,      (void *)TRUE    },
        { ADI_TMR_GP_CMD_SET_INPUT_SELECT,          (void *)FALSE   },
        { ADI_TMR_GP_CMD_ENABLE_TIMER,              (void *)TRUE    },
        { ADI_TMR_GP_CMD_END,                       NULL            },
    };

    u32         Width;      // width detected by the timer
    ADI_UART    *pDevice;   // device pointer

    // avoid casts
    pDevice = ClientHandle;

    // IF (we haven't determined the baud rate yet)
    if (pDevice->BaudRate == ADI_UART_AUTOBAUD_IN_PROGRESS) {

        // get the width value detected by the timer
        adi_tmr_GPControl(pDevice->TimerID, ADI_TMR_GP_CMD_GET_WIDTH, &Width);

        // calculate the baud rate
        pDevice->BaudRate = (pDevice->SCLK * pDevice->DivisorBits)/Width;

        // disable the timer
        adi_tmr_GPControl(pDevice->TimerID, ADI_TMR_GP_CMD_ENABLE_TIMER, (void *)FALSE);

        // enable the timer in PWM mode
        adi_tmr_GPControl(pDevice->TimerID, ADI_TMR_GP_CMD_TABLE, ConfigurationTable);

    // ELSE
    } else {

        // call autobaud complete
        uartAutobaudComplete(pDevice);

    // ENDIF
    }

    // return
}


/* IF (this is the Interrupt version of the driver) */
#if defined(ADI_UART_INT)
/*********************************************************************
*
*   Function:       uartBufferComplete
*
*   Description:    Does the processing necessary when a buffer has been
*                   filled or sent out
*
*********************************************************************/

static void uartBufferComplete(
    ADI_UART                *pDevice,
    ADI_UART_RESERVED_AREA  *pReservedArea,
    ADI_UART_RESERVED_AREA  **pHead,
    ADI_UART_RESERVED_AREA  **pTail
) {

    ADI_DEV_1D_BUFFER       *pBuffer;           // pointer to a buffer
    void                    *pExitCriticalArg;  // exit critical region parameter


    // point to the buffer
    pBuffer = (ADI_DEV_1D_BUFFER *)pReservedArea;

    // update the processed fields in the buffer
    pBuffer->ProcessedFlag = TRUE;
    pBuffer->ProcessedElementCount = pBuffer->ElementCount - pReservedArea->BytesRemaining;

    // protect us
    pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

    // take the buffer out of the queue
    *pHead = pReservedArea->pNext;
    if (*pHead == NULL) {
        *pTail = NULL;
    }

    // unprotect us
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // post a callback if one is requested
    // (parameters are Client Handle, ADI_DEV_EVENT_BUFFER_PROCESSED, CallbackParameter)
    if (pBuffer->CallbackParameter != NULL) {
        (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, pBuffer->CallbackParameter);
    }

    // IF (loopback is enabled)
    if (pDevice->DataflowMethod == ADI_DEV_MODE_CHAINED_LOOPBACK) {

        // reset the data pointer, byte counters etc.
        pReservedArea->pCurrentDataPointer = pBuffer->Data;
        pReservedArea->BytesRemaining = pBuffer->ElementCount * pBuffer->ElementWidth;
        pBuffer->ProcessedFlag = FALSE;
        pBuffer->ProcessedElementCount = 0;

        // protect us
        pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);

        // place the buffer back in at the end of the queue
        if (*pHead == NULL) {
            *pHead = pReservedArea;
            *pTail = pReservedArea;
        } else {
            (*pTail)->pNext = pReservedArea;
        }

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ENDIF
    }

    // return
}

#endif

/*********************************************************************
*
*   Function:       uartUpdateDividers
*
*   Description:    Updates the dividers upon SCLK/baud rate changes
*
*********************************************************************/

static void uartUpdateDividers(     // updates dividers for baud rate/SLCK changes
    ADI_UART    *pDevice                // device
) {

    u32 tmp;        // temporary calculation value

    // calculate divisor values
    if (pDevice->BaudRate != 0) {
        tmp = (pDevice->SCLK / (16.0 * (pDevice->BaudRate))) + 0.5;
    }

    // update divisor values
#if defined(__ADSP_MOAB__) || defined (__ADSP_MOY__)
    ADI_UART_DLL_SET(tmp & 0xff);
    ADI_UART_DLH_SET(tmp >> 8);
#else
    ADI_UART_LCR_SET_DLAB();
    ADI_UART_DLL_SET(tmp & 0xff);
    ADI_UART_DLH_SET(tmp >> 8);
    ADI_UART_LCR_CLEAR_DLAB();
#endif

    // return
}


/*********************************************************************

    Function:       uartSetTimer

    Description:    Configures a timer autobaud detection

*********************************************************************/

static u32 uartSetTimer(    //Configures timer registers for Autobaud detection
    ADI_UART    *pDevice        // device
){

    ADI_TMR_GP_CMD_VALUE_PAIR ConfigurationTable [] = { // configuration table for the timer
        { ADI_TMR_GP_CMD_SET_TIMER_MODE,            (void *)2       },
        { ADI_TMR_GP_CMD_SET_INTERRUPT_ENABLE,      (void *)TRUE    },
        { ADI_TMR_GP_CMD_SET_INPUT_SELECT,          (void *)TRUE    },
        { ADI_TMR_GP_CMD_ENABLE_TIMER,              (void *)TRUE    },
        { ADI_TMR_GP_CMD_END,                       NULL            },
    };

    // reset the timer
    if (adi_tmr_Reset(pDevice->TimerID) != ADI_TMR_RESULT_SUCCESS) {
        return (ADI_UART_RESULT_TIMER_ERROR);
    }

    // clear out the baud rate value for the device so we can tell where we are when we get the callback
    pDevice->BaudRate = ADI_UART_AUTOBAUD_IN_PROGRESS;

    // install the callback
    if (adi_tmr_InstallCallback(pDevice->TimerID, TRUE, pDevice, pDevice->DCBHandle, AutobaudCallback) != ADI_TMR_RESULT_SUCCESS) {
        return (ADI_UART_RESULT_TIMER_ERROR);
    }

    // configure the timer for width capture mode and enable it
    if (adi_tmr_GPControl(pDevice->TimerID, ADI_TMR_GP_CMD_TABLE, ConfigurationTable) != ADI_TMR_RESULT_SUCCESS) {
        return (ADI_UART_RESULT_TIMER_ERROR);
    }

    // return
    return (ADI_DEV_RESULT_SUCCESS);
}




/*********************************************************************
*
* Function:  uartAutobaudComplete
*
* Description: Service Routine to update UART Divisor values
*               & to unhook Timer Interrupt
*********************************************************************/

static void uartAutobaudComplete(
    ADI_UART *pDevice
)
{

    // disable the timer
    adi_tmr_GPControl(pDevice->TimerID, ADI_TMR_GP_CMD_ENABLE_TIMER, (void *)FALSE);

    // remove the callback
    adi_tmr_RemoveCallback(pDevice->TimerID);

    // set baud rate
     uartUpdateDividers(pDevice);

    // post a callback that returns the detected Baud rate Divisor value (DL)
    (pDevice->DMCallback) (pDevice->DeviceHandle, ADI_UART_EVENT_AUTOBAUD_COMPLETE, (void *)&pDevice->BaudRate);

    // return
}



#if defined(__ADSP_BRAEMAR__)       // static functions for Braemar class devices

/*********************************************************************

    Function:       uartSetPortControl

    Description:    Configures the general purpose ports for Braemar
                    class devices upon opening the device.
                    Called when the device is first opened.

*********************************************************************/

static void uartSetPortControl(         // configures the port control registers
    ADI_UART    *pDevice,                   // pointer to the device
    u32         CTSRTSFlag                  // CTS/RTS flag (TRUE if enabling CTS/RTS only)
){

    u32 Directives[] = {            // port control directives
        ADI_PORTS_DIR_UART0_RX,
        ADI_PORTS_DIR_UART0_TX,
        ADI_PORTS_DIR_UART1_RX,
        ADI_PORTS_DIR_UART1_TX
    };
    u32 Count;                      // directive count
    u32 Index;                      // index into directive table

    // CASEOF (direction)
    switch (pDevice->Direction) {

        // CASE (Inbound only)
        case ADI_DEV_DIRECTION_INBOUND:

            // only pass the RX directive
            Count = 1;
            Index = 0;
            break;

        // CASE (Outbound only)
        case ADI_DEV_DIRECTION_OUTBOUND:

            // only pass the TX directive
            Count = 1;
            Index = 1;
            break;

        // CASEELSE (Bidirectional)
        default:

            // pass both RX and TX directives
            Count = 2;
            Index = 0;
            break;

    // ENDCASE
    }

    // adjust if operating on the second UART
    if (pDevice->DeviceNumber == 1) {
        Index += 2;
    }

    // tell port control to configure the UART
    adi_ports_EnableUART(&Directives[Index], Count, TRUE);

    // return
}

#endif      // __ADSP_BRAEMAR__


#if defined(__ADSP_STIRLING__)          // static functions for Stirling class devices

/*********************************************************************

    Function:       uartSetPortControl

    Description:    Configures the general purpose ports for Stirling
                    class devices upon opening and closing the device.
                    Called when the device is first opened.

                    Only UART1 and 2 need port control.  UART0 does not.

*********************************************************************/

static void uartSetPortControl(         // configures the port control registers
    ADI_UART    *pDevice,                   // pointer to the device
    u32         CTSRTSFlag                  // CTS/RTS flag (TRUE if enabling CTS/RTS only)
){

    ADI_PORTS_DIRECTIVE Directives[] = {    // port control directives
        ADI_PORTS_DIRECTIVE_RX1,
        ADI_PORTS_DIRECTIVE_TX1,
        ADI_PORTS_DIRECTIVE_RX2,
        ADI_PORTS_DIRECTIVE_TX2
    };
    u32 Count;                      // directive count
    u32 Index;                      // index into directive table

    // IF (not UART0)
    if (pDevice->DeviceNumber != 0) {

        // CASEOF (direction)
        switch (pDevice->Direction) {

            // CASE (Inbound only)
            case ADI_DEV_DIRECTION_INBOUND:

                // only pass the RX directive
                Count = 1;
                Index = 0;
                break;

            // CASE (Outbound only)
            case ADI_DEV_DIRECTION_OUTBOUND:

                // only pass the TX directive
                Count = 1;
                Index = 1;
                break;

            // CASEELSE (Bidirectional)
            default:

                // pass both RX and TX directives
                Count = 2;
                Index = 0;
                break;

        // ENDCASE
        }

        // adjust if operating on the second UART
        if (pDevice->DeviceNumber == 2) {
            Index += 2;
        }

        // tell port control to configure the UART
        adi_ports_Configure(&Directives[Index], Count);

    // ENDIF
    }

    // return
}

#endif      // __ADSP_STIRLING__


#if defined(__ADSP_MOAB__)          // static functions for Moab class devices

/*********************************************************************

    Function:       uartSetPortControl

    Description:    Configures the general purpose ports for Moab
                    class devices upon opening the device. Called when
                    the device is first opened.

                    All UARTs need port control.  But UART0 and 3 do not
                    support RTS/CTS.

*********************************************************************/

static void uartSetPortControl(         // configures the port control registers
    ADI_UART    *pDevice,                   // pointer to the device
    u32         CTSRTSFlag                  // CTS/RTS flag (TRUE if enabling CTS/RTS only)
){

    static ADI_PORTS_DIRECTIVE BaseDirectives[] = {   // port control directives
        ADI_PORTS_DIRECTIVE_UART0_TX,
        ADI_PORTS_DIRECTIVE_UART0_RX,
        ADI_PORTS_DIRECTIVE_UART1_TX,
        ADI_PORTS_DIRECTIVE_UART1_RX,
        ADI_PORTS_DIRECTIVE_UART2_TX,
        ADI_PORTS_DIRECTIVE_UART2_RX,
        ADI_PORTS_DIRECTIVE_UART3_TX,
        ADI_PORTS_DIRECTIVE_UART3_RX,
    };
    static ADI_PORTS_DIRECTIVE CTSRTSDirectives[] = { // port control directives
        ADI_PORTS_DIRECTIVE_UART1_CTS,
        ADI_PORTS_DIRECTIVE_UART1_RTS,
        ADI_PORTS_DIRECTIVE_UART3_CTS,
        ADI_PORTS_DIRECTIVE_UART3_RTS,
    };
    u32 Count;                      // directive count
    u32 Index;                      // index into directive table
    ADI_PORTS_DIRECTIVE *pDirectives;   // pointer to the directives table

    // IF (CTS/RTS only)
    if (CTSRTSFlag) {

        // point to the directives table
        pDirectives = CTSRTSDirectives;

        // calculate the index and count into the table
        // only have to do devices 1 and 3
        Index = 0;
        Count = 0;
        if (pDevice->DeviceNumber == 1) {
            Count = 2;
        }
        if (pDevice->DeviceNumber == 3) {
            Index = 2;
            Count = 2;
        }

    // ELSE
    } else {

        // point to the directives table
        pDirectives = BaseDirectives;

        // determine how many directives to pass to port control
        if (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL) {
            Count = 2;
        } else {
            Count = 1;
        }

        // calculate the offset into the table
        Index = pDevice->DeviceNumber << 1;

     // ENDIF
    }

    // tell port control to configure the UART
    adi_ports_Configure(&pDirectives[Index], Count);

    // return
}

#endif      // __ADSP_MOAB__


#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)       // static functions for Kookaburra/Mockingbird class devices

/*********************************************************************

    Function:       uartSetPortControl

    Description:    Configures the general purpose ports for Kookaburra
                    and Mockingbird class devices upon opening the device.
                    Called when the device is first opened.

*********************************************************************/

static void uartSetPortControl(         // configures the port control registers
    ADI_UART    *pDevice,                   // pointer to the device
    u32         CTSRTSFlag                  // CTS/RTS flag (TRUE if enabling CTS/RTS only)
){

    ADI_PORTS_DIRECTIVE Directives[] = {    // port control directives
        ADI_PORTS_DIRECTIVE_UART0G_RX,
        ADI_PORTS_DIRECTIVE_UART0G_TX,
        ADI_PORTS_DIRECTIVE_UART1G_RX,
        ADI_PORTS_DIRECTIVE_UART1G_TX,
        ADI_PORTS_DIRECTIVE_UART1F_RX,
        ADI_PORTS_DIRECTIVE_UART1F_TX
    };
    u32 Count;                      // directive count
    u32 Index;                      // index into directive table

    // CASEOF (direction)
    switch (pDevice->Direction) {

        // CASE (Inbound only)
        case ADI_DEV_DIRECTION_INBOUND:

            // only pass the RX directive
            Count = 1;
            Index = 0;
            break;

        // CASE (Outbound only)
        case ADI_DEV_DIRECTION_OUTBOUND:

            // only pass the TX directive
            Count = 1;
            Index = 1;
            break;

        // CASEELSE (Bidirectional)
        default:

            // pass both RX and TX directives
            Count = 2;
            Index = 0;
            break;

    // ENDCASE
    }

    // adjust if operating on the second or third UART
    if (pDevice->DeviceNumber == 1) {
        Index += 2;
    } else if (pDevice->DeviceNumber == 2) {
        Index += 4;
    }

    // tell port control to configure the UART
    adi_ports_Configure(&Directives[Index], Count);

    // return
}

#endif      // __ADSP_KOOKABURRA__ or __ADSP_MOCKINGBIRD__


#if defined(__ADSP_BRODIE__)        // static functions for Brodie class devices

/*********************************************************************

    Function:       uartSetPortControl

    Description:    Configures the general purpose ports for Kookaburra
                    and Mockingbird class devices upon opening the device.
                    Called when the device is first opened.

*********************************************************************/

static void uartSetPortControl(         // configures the port control registers
    ADI_UART    *pDevice,                   // pointer to the device
    u32         CTSRTSFlag                  // CTS/RTS flag (TRUE if enabling CTS/RTS only)
){

    ADI_PORTS_DIRECTIVE Directives[] = {    // port control directives
        ADI_PORTS_DIRECTIVE_UART0_RX,
        ADI_PORTS_DIRECTIVE_UART0_TX,
        ADI_PORTS_DIRECTIVE_UART1_RX,
        ADI_PORTS_DIRECTIVE_UART1_TX,
    };
    u32 Count;                      // directive count
    u32 Index;                      // index into directive table

    // CASEOF (direction)
    switch (pDevice->Direction) {

        // CASE (Inbound only)
        case ADI_DEV_DIRECTION_INBOUND:

            // only pass the RX directive
            Count = 1;
            Index = 0;
            break;

        // CASE (Outbound only)
        case ADI_DEV_DIRECTION_OUTBOUND:

            // only pass the TX directive
            Count = 1;
            Index = 1;
            break;

        // CASEELSE (Bidirectional)
        default:

            // pass both RX and TX directives
            Count = 2;
            Index = 0;
            break;

    // ENDCASE
    }

    // adjust if operating on the second UART
    if (pDevice->DeviceNumber == 1) {
        Index += 2;
    }

    // tell port control to configure the UART
    adi_ports_Configure(&Directives[Index], Count);

    // return
}

#endif      // __ADSP_BRODIE

#if defined(__ADSP_MOY__)          // static functions for Moy class devices

/*********************************************************************

    Function:       uartSetPortControl

    Description:    Configures the general purpose ports for Moab
                    class devices upon opening the device. Called when
                    the device is first opened.

                    All UARTs need port control

*********************************************************************/

static void uartSetPortControl(         // configures the port control registers
    ADI_UART    *pDevice,                   // pointer to the device
    u32         CTSRTSFlag                  // CTS/RTS flag (TRUE if enabling CTS/RTS only)
){

    static ADI_PORTS_DIRECTIVE BaseDirectivesPortF[] = {   // port control directives for UART on Port F
        ADI_PORTS_DIRECTIVE_UART0F_TX,
        ADI_PORTS_DIRECTIVE_UART0F_RX,
        ADI_PORTS_DIRECTIVE_UART1F_TX,
        ADI_PORTS_DIRECTIVE_UART1F_RX
    };

    static ADI_PORTS_DIRECTIVE BaseDirectivesPortG[] = {   // port control directives for UART on Port G
        ADI_PORTS_DIRECTIVE_UART0G_TX,
        ADI_PORTS_DIRECTIVE_UART0G_RX,
        ADI_PORTS_DIRECTIVE_UART1G_TX,
        ADI_PORTS_DIRECTIVE_UART1G_RX
    };
 
    static ADI_PORTS_DIRECTIVE DefaultDirectivesPortF[] = {   // Default port control directives for UART Rx/Tx lines on Port F
        ADI_PORTS_DIRECTIVE_PF0,                              // Pin used for UART 0 Rx
        ADI_PORTS_DIRECTIVE_PF1,							  // Pin used for UART 0 Tx
        ADI_PORTS_DIRECTIVE_PF6,                              // Pin used for UART 1 Tx
        ADI_PORTS_DIRECTIVE_PF7                               // Pin used for UART 1 Rx
    };
    
    static ADI_PORTS_DIRECTIVE CTSRTSDirectives[] = { // port control directives
        ADI_PORTS_DIRECTIVE_UART0_CTS,
        ADI_PORTS_DIRECTIVE_UART0_RTS,
        ADI_PORTS_DIRECTIVE_UART1_CTS,
        ADI_PORTS_DIRECTIVE_UART1_RTS
    };
    u32 Count;                      // directive count
    u32 Index;                      // index into directive table
    ADI_PORTS_DIRECTIVE *pDirectives;   // pointer to the directives table

    /* Default index */
    Index = pDevice->DeviceNumber << 1;

    /* IF (Enable CTS/RTS only) */
    if (CTSRTSFlag)
    {
        /* point to the directives table */
        pDirectives = CTSRTSDirectives;
        /* Number of directives to pass */
        Count = 2;
    }
    /* ELSE (Enable UART base pins) */
    else
    {
        /* IF (Pix mux mode set to use Port G for Tx & Rx) */
        if (pDevice->ePinMuxMode == ADI_UART_PIN_MUX_MODE_1)
        {
            /* Use Port G directives table */
            pDirectives = BaseDirectivesPortG;
            /* IF (Device 1) */
            if (pDevice->DeviceNumber == 1)
            {
                /* Use Timer 4 for Auto-baud */
                pDevice->TimerID = ADI_TMR_GP_TIMER_4;
            }
        }
        /* ELSE (Pix mux mode set to use Port F for Tx & Rx) */
        else
        {
            /* Use Port F directives table */
            pDirectives = BaseDirectivesPortF;
            /* IF (Device 1) */
            if (pDevice->DeviceNumber == 1)
            {
                /* Use Timer 3 for Auto-baud */
                pDevice->TimerID = ADI_TMR_GP_TIMER_3;
            }
        }

        /* IF (UART opened for bi-directional flow) */
        if (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)
        {
            /* Enable both Tx & Rx ports */
            Count = 2;
        }
        else
        {
            /* Enable only one port */
            Count = 1;

            /* IF (UART opened for inbound data) */
            if (pDevice->Direction == ADI_DEV_DIRECTION_INBOUND)
            {
                /* Enable Rx port only */
                Index++;
            }
        }

    } /* End of if (Enable CTS/RTS only) else*/

    /* tell port control to configure the UART */
    adi_ports_Configure(&pDirectives[Index], Count);

    
    /* When UART Rx and Tx lines are mapped from PortF to PortG,
       chage back the PortF pins (used for UART Rx/Tx) to default GPIO mode */
    if(pDirectives == BaseDirectivesPortG)
    {
    	adi_ports_Configure(&DefaultDirectivesPortF[Index], Count);
    }
    
    /* return */
 }

#endif      /* __ADSP_MOY__ */


#if defined(__ADSP_DELTA__)        // static functions for Delta class devices

/*********************************************************************

    Function:       uartSetPortControl

    Description:    Configures the general purpose ports for Delta
                    class devices upon opening the device.
                    Called when the device is first opened.

*********************************************************************/

static void uartSetPortControl(         // configures the port control registers
    ADI_UART    *pDevice,               // pointer to the device
    u32         CTSRTSFlag              // CTS/RTS flag (TRUE if enabling CTS/RTS only)
){

    ADI_PORTS_DIRECTIVE Directives[] = {    // port control directives
        ADI_PORTS_DIRECTIVE_UART0F_RX,
        ADI_PORTS_DIRECTIVE_UART0F_TX
    };
    u32 Count;                      // directive count
    u32 Index;                      // index into directive table

    // CASEOF (direction)
    switch (pDevice->Direction) {

        // CASE (Inbound only)
        case ADI_DEV_DIRECTION_INBOUND:

            // only pass the RX directive
            Count = 1;
            Index = 0;
            break;

        // CASE (Outbound only)
        case ADI_DEV_DIRECTION_OUTBOUND:

            // only pass the TX directive
            Count = 1;
            Index = 1;
            break;

        // CASEELSE (Bidirectional)
        default:

            // pass both RX and TX directives
            Count = 2;
            Index = 0;
            break;

    // ENDCASE
    }

    // adjust if operating on the second UART
    if (pDevice->DeviceNumber == 1) {
        Index += 2;
    }

    // tell port control to configure the UART
    adi_ports_Configure(&Directives[Index], Count);

    // return
}

#endif      // __ADSP_DELTA



#if defined(ADI_DEV_DEBUG)

/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {
    unsigned int i;
    for (i = 0; i < ADI_UART_NUM_DEVICES; i++) {
        if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
            return (ADI_DEV_RESULT_SUCCESS);
        }
    }
    return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
    }


#endif



