/****************************************************************************
 Include Section
*****************************************************************************/

/*****************************************************************************
 Symbolic constants / definitions
******************************************************************************/

#define RDAC_STEP_VALUE_110 0x37 /* Digipot step value 0x37 gives us 1.10V */
#define RDAC_STEP_VALUE_115 0x2D /* Digipot step value 0x2D gives us 1.15V */
#define RDAC_STEP_VALUE_120 0x23 /* Digipot step value 0x23 gives us 1.20V */
#define RDAC_STEP_VALUE_125 0x1A /* Digipot step value 0x1A gives us 1.25V */
#define RDAC_STEP_VALUE_130 0x12 /* Digipot step value 0x12 gives us 1.30V */
#define RDAC_STEP_VALUE_135 0x0A /* Digipot step value 0x0A gives us 1.35V */
#define RDAC_STEP_VALUE_140 0x03 /* Digipot step value 0x03 gives us 1.40V */


/*****************************************************************************
 Dynamic Power Managment
*****************************************************************************/

/*************************************************
* To activate PLL and Voltage Regulator settings *
* uncomment the following definition             *
*************************************************/

//#define __ACTIVATE_DPM__


/********************************
* PLL Control Register Value    *
* Reset = 0x0A00                *
* ADSP-BF518F EZ-Board:         *
* CLKIN = 25MHz                 *
* MSEL = 12                     *
* --> VCO = 25MHz x 12 = 300MHz *
********************************/

#define PLL_CTL_VAL             (\
                               nBYPASS              |   /* Bypass the PLL */\
                               nOUT_DELAY           |   /* Add 200ps Delay To EBIU Output Signals */\
                               nIN_DELAY            |   /* Add 200ps Delay To EBIU Input Latches */\
                               nPDWN                |   /* Enter Deep Sleep Mode */\
                               nSTOPCK              |   /* Core Clock Off */\
                               nPLL_OFF             |   /* PLL Not Powered */\
                               nDF                  |   /* 0: PLL = CLKIN, 1: PLL = CLKIN/2 */\
                                SET_MSEL(12)        |   /* Set MSEL = 0-63 --> VCO = CLKIN*MSEL */\
                                0)

#define CLKIN_Hz                 12000000           /* CLKIN [Hz] */
#define VCO_MAX_Hz              400000000           /* VCO [Hz] */
#define VCO_MIN_Hz               50000000           /* VCO [Hz] */


/*********************************
* PLL Divide Register Value      *
* Reset = 0x0004                 *
* ADSP-BF518F EZ-Board:          *
* CSEL = 1                       *
* --> CCLK = VCO / CSEL          *
* --> CCLK = 300MHz / 1 = 300MHz *
* SSEL = 4                       *
* --> SCLK = VCO / SSEL          *
* --> SCLK = 300MHz / 4 = 75MHz  *
*********************************/

#define PLL_DIV_VAL             (\
                                SET_SSEL(4)         |   /* Set SSEL = 1-15 --> SCLK = VCO/SSEL */\
                                CSEL_DIV1           |   /* CCLK = VCO / 1 */\
                                0)

#define CCLK_MAX_Hz             VCO_MAX_Hz          /* Maximum Core Clock [Hz] */
#define SCLK_MAX_Hz             80000000            /* Maximum System Clock [Hz] */


/********************************
* PLL Lock Count Register Value *
* Reset = 0x0200                *
********************************/

#define PLL_LOCKCNT_VAL         0x0200


/****************************
* PLL Status Register Value *
* Reset = 0x00A2            *
****************************/

#define PLL_STAT_VAL            (\
                                VSTAT               |   /* Voltage Regulator Status: Regulator at programmed voltage */\
                                PLL_LOCKED          |   /* PLL Locked Status */\
                               nACTIVE_PLLDISABLED  |   /* Active Mode With PLL Disabled */\
                                FULL_ON             |   /* Full-On Mode */\
                               nACTIVE_PLLENABLED   |   /* Active Mode With PLL Enabled */\
                                0)


/*******************************************
* Voltage Regulator Control Register Value *
* Reset = 0x70B0                           *
*******************************************/

#define VR_CTL_VAL              (\
                               nSCKELOW         |   /* Enable Drive CKE Low During Reset */\
                               nWAKE            |   /* Enable RTC/Reset Wakeup From Hibernate */\
                               nPHYWE           |   /* Enable PHY Wakeup From Hibernate */\
                                CLKBUFOE        |   /* CLKIN Buffer Output Enable */\
                                0x00B0          |   /* Bits #7,6 set by default */\
                                FREQ            |   /* FREQ=00 indicates hibernate. FREQ=11 indicates non-Hibernate */\
                                0)


/*****************************
* External Voltage Regulator *
* Digipot Value              *
*****************************/

#define RDAC_STEP_VAL           RDAC_STEP_VALUE_140     /* Digipot step value for 1.40V */


/*****************************************************************************
 EBIU: Asynchronous Memory
*****************************************************************************/


/****************************************************
* Asynchronous Memory Global Control Register Value *
* Reset = 0x00F2                                    *
* --> Enabling all four Async Banks and Clock Out   *
****************************************************/

#define EBIU_AMGCTL_VAL         (\
                               nCDPRIO          |   /* Core/DMA Priority */\
                                AMCKEN          |   /* Enable CLKOUT */\
                                AMBEN_ALL       |   /* 4MB Asynchronous Memory */\
                                0)
                        

/****************************************************
* Asynchronous Memory Bank Control 0 Register Value *
* Reset = 0xFFC2 FFC2                               *
****************************************************/

#define EBIU_AMBCTL0_VAL        (\
                                B1WAT_15        |   /* B1 Write Access Time = 15 cycles */\
                                B1RAT_15        |   /* B1 Read Access Time = 15 cycles */\
                                B1HT_3          |   /* B1 Hold Time (nRead/Write to nAOE) = 3 cycles */\
                                B1ST_4          |   /* B1 Setup Time from AOE asserted to Read/Write asserted = 4 cycles */\
                                B1TT_4          |   /* B1 Transition Time (Read to Write) = 4 cycles */\
                                B1RDYPOL        |   /* B1 RDY Active High */\
                                B0WAT_15        |   /* B0 Write Access Time = 15 cycles */\
                                B0RAT_15        |   /* B0 Read Access Time = 15 cycles */\
                                B0HT_3          |   /* B0 Hold Time (nRead/Write to nAOE) = 3 cycles */\
                                B0ST_4          |   /* B0 Setup Time from AOE asserted to Read/Write asserted = 4 cycles */\
                                B0TT_4          |   /* B0 Transition Time (Read to Write) = 4 cycles */\
                                B0RDYPOL        |   /* B0 RDY Active High */\
                                0)


/****************************************************
* Asynchronous Memory Bank Control 1 Register Value *
* Reset = 0xFFC2 FFC2                               *
****************************************************/

#define EBIU_AMBCTL1_VAL        (\
                                B3WAT_15        |   /* B3 Write Access Time = 15 cycles */\
                                B3RAT_15        |   /* B3 Read Access Time = 15 cycles */\
                                B3HT_3          |   /* B3 Hold Time (nRead/Write to nAOE) = 3 cycles */\
                                B3ST_4          |   /* B3 Setup Time from AOE asserted to Read/Write asserted = 4 cycles */\
                                B3TT_4          |   /* B3 Transition Time (Read to Write) = 4 cycles */\
                                B3RDYPOL        |   /* B3 RDY Active High */\
                                B2WAT_15        |   /* B2 Write Access Time = 15 cycles */\
                                B2RAT_15        |   /* B2 Read Access Time = 15 cycles */\
                                B2HT_3          |   /* B2 Hold Time (nRead/Write to nAOE) = 3 cycles */\
                                B2ST_4          |   /* B2 Setup Time from AOE asserted to Read/Write asserted = 4 cycles */\
                                B2TT_4          |   /* B2 Transition Time (Read to Write) = 4 cycles */\
                                B2RDYPOL        |   /* B2 RDY Active High */\
                                0)


/*****************************************************************************
 EBIU: SDRAM Memory

       SDRAM Setup according to ADSP-BF518F EZ-Board

       SDRAM Device = MT48LC32M16A2P-75
 
       NRA  = 8192
       tRef = 64ms
       tRAS = 44ns
       tRP  = 20ns

       always round up tRAS and tRP values
       RDIV = ( ( SCLK[MHz] * tREF[ms] ) / NRA ) - ( tRAS + tRP ) [clock cycles]
 
       Note: with a 25MHz crystal the default SCLK rate is 50MHz on the EZ-Board.
*****************************************************************************/


/********************************************
* SDRAM Refresh Rate Control Register Value *
* Reset = 0x081A                            *
* ADSP-BF518-proc.xml = 0x026B              *
* here = 0x026B, SCLK @ 75MHz               *
* Hint: The refresh rate is calculated at   *
* runtime in the current implementation.    *
* Uncomment the definition below to set     *
* a fixed pre-defined value                 *
********************************************/

//#define EBIU_SDRRC_VAL          0x026B

#define tREF                    64                  /* [ms] */
#define NRA                     4096                /* [int] */


/*********************************************
* SDRAM Memory Global Control Register Value *
* Reset = 0xE008 8849                        *
* ADSP-BF518-proc.xml = 0xC091 1109          *
* here = 0xC091 1109                         *
* Additional Requirement:                    *
* tRAS + tRP >= max(tRC,tRFC,tXSR)           *
* EBIU internal calculation:                 *
* tRC = tRAS + tRP                           *
* tRFC = tRC                                 *
* tXSR = tRC                                 *
*********************************************/

#define EBIU_SDGCTL_VAL         (\
                                0x80000000      |   /* Reset: bit#31 = 1 */\
                                0x40000000      |   /* Reset: bit#30 = 1 */\
                               nTCSR            |   /* Temp-Compensated Self-Refresh Value (85/45* Deg C) */\
                               nEMREN           |   /* Extended Mode Register Enable */\
                               nFBBRW           |   /* Enable Fast Back-To-Back Read To Write */\
                               nEBUFE           |   /* Enable External Buffering Timing */\
                               nSRFS            |   /* Enable SDRAM Self-Refresh Mode */\
                               nPSM             |   /* Power-Up Sequence (Mode Register Before/After* Refresh) */\
                               nPUPSD           |   /* Power-Up Start Delay (15 SCLK Cycles Delay) */\
                                SCTLE           |   /* Enable SDRAM Signals */\
                                PSS             |   /* Enable Power-Up Sequence on Next SDRAM Access */\
                                TWR_2           |   /* SDRAM tWR = 2 cycle */\
                                TRCD_3          |   /* SDRAM tRCD = 3 cycles */\
                                TRP_3           |   /* SDRAM tRP = 3 cycles */\
                                TRAS_6          |   /* SDRAM tRAS = 6 cycles */\
                                PASR_ALL        |   /* All 4 SDRAM Banks Refreshed In Self-Refresh */\
                                CL_3            |   /* SDRAM CAS Latency = 3 cycles */\
                                0)


/*******************************************
* SDRAM Memory Bank Control Register Value *
* Reset = 0x0000                           *
* ADSP-BF518-proc.xml = 0x0025             *
* here = 0x0025                            *
*******************************************/

#define EBIU_SDBCTL_VAL         (\
                                EBE             |   /* Enable SDRAM External Bank */\
                                EBSZ_16         |   /* SDRAM External Bank Size = 16MB */\
                                EBCAW_9        |   /* SDRAM External Bank Column Address Width = 9 Bits */\
                                0)


/**********************************************************
* BMODE_SPIMEM: Boot from SPI memory                      *
* This definition will modify the SPI_BAUD register value *
* and can in/decrease boot speed                          *
* Boot Rom default: 0x85                                  *
**********************************************************/

#define SPI_BAUD_VAL            0x05//0x85,-QWB131122-modify for booting time.


/****************************************************************************
 EOF
*****************************************************************************/
