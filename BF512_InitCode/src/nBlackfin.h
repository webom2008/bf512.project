/************************************************************************
* nBlackfin.h                                                           *
************************************************************************/


/* *********************  PLL MASKS  ************************ */
#ifndef nBYPASS
/* PLL_CTL Masks */
#define nDF                     0x0         /* 0: PLL = CLKIN, 1: PLL = CLKIN/2 */
#define nPLL_CLKIN_DIV2         nDF         /* Pass CLKIN/2 to PLL */
#define nPLL_OFF                0x0         /* Shut off PLL clocks */
#define nSTOPCK_OFF             0x0         /* Core clock off */
#define nSTOPCK                 0x0         /* Core Clock Off */
#define nPDWN                   0x0         /* Put the PLL in a Deep Sleep state */

#define nIN_DELAY               0x0         /* EBIU Input Delay Select */
#define nOUT_DELAY              0x0         /* EBIU Output Delay Select */
#define nBYPASS                 0x0         /* Bypass the PLL */


/* PLL_STAT Masks */
#define nVSTAT                  0x0         /* Voltage Regulator Status: Regulator not at programmed voltage */
#define nCORE_IDLE              0x0         /* processor is in the IDLE operating mode */
#define nPLL_LOCKED             0x0         /* PLL_LOCKCNT Has Been Reached */
#define nSLEEP                  0x0         /* processor is in the Sleep operating mode */
#define nDEEP_SLEEP             0x0         /* processor is in the Deep Sleep operating mode */
#define nACTIVE_PLLENABLED      0x0         /* Processor In Active Mode With PLL Enabled */
#define nFULL_ON                0x0         /* Processor In Full On Mode */
#define nACTIVE_PLLDISABLED     0x0         /* Processor In Active Mode With PLL Disabled */



/* VR_CTL Masks */
#define nFREQ                   0x0         /* Switching Oscillator Frequency For Regulator */
#define nHIBERNATE              0x0         /* Powerdown/Bypass On-Board Regulation */
#define nSCKELOW                0x0         /* Do Not Drive SCKE High During Reset After Hibernate */
#define nWAKE                   0x0         /* Enable RTC/Reset Wakeup From Hibernate */
#define nPHYWE                  0x0         /* Enable PHY Wakeup From Hibernate */
#define nCANWE                  0x0         /* Enable CAN Wakeup From Hibernate */
#define nGPWE                   0x0         /* Enable General-Purpose Wakeup From Hibernate */
#define nUSBWE                  0x0         /* Enable USB Wakeup From Hibernate */
#define nVTRIM                  0x0         /* VTRIM [3:0]: Not yet documented */
#endif /* nBYPASS */


/* *********************  ASYNCHRONOUS MEMORY CONTROLLER MASKS  ************* */
#ifndef nAMCKEN
/* EBIU_AMGCTL Masks */
#define nAMCKEN                 0x0         /* Enable CLKOUT */
#define nCDPRIO                 0x0         /* Core has priority over DMA for external accesses */
#endif /* nAMCKEN */


/* **********************  SDRAM CONTROLLER MASKS  *************************** */
#ifndef nEBE
/* EBIU_SDBCTL Masks */
#define nEBE                    0x0         /* Enable SDRAM external bank */
#define nEBSZ_32                0x0         /* SDRAM External Bank Size = 32MB */
#define nEBSZ_64                0x0         /* SDRAM External Bank Size = 64MB */

/* EBIU_SDGCTL Masks */
#define nSCTLE                  0x0         /* Enable SCLK[0], /SRAS, /SCAS, /SWE, SDQM[3:0] */
#define nCL_2                   0x0         /* SDRAM CAS latency = 2 cycles */
#define nCL_3                   0x0         /* SDRAM CAS latency = 3 cycles */
#define nPFE                    0x0         /* Enable SDRAM prefetch */
#define nPFP                    0x0         /* Prefetch has priority over AMC requests */
#define nPUPSD                  0x0         /* Power-up start delay */
#define nPSM                    0x0         /* SDRAM power-up sequence = Precharge, mode register set, 8 CBR refresh cycles */
#define nPSS                    0x0         /* enable SDRAM power-up sequence on next SDRAM access */
#define nSRFS                   0x0         /* Start SDRAM self-refresh mode */
#define nEBUFE                  0x0         /* Enable external buffering timing */
#define nFBBRW                  0x0         /* Fast back-to-back read write enable */
#define nEMREN                  0x0         /* Extended mode register enable */
#define nTCSR                   0x0         /* Temp compensated self refresh value 85 deg C */
#define nCDDBG                  0x0         /* Tristate SDRAM controls during bus grant */
#endif /* nEBE */


/****************************************************************************
 EOF
*****************************************************************************/
