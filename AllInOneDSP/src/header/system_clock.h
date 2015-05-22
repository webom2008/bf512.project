/*
 * system_clock.h
 *
 *  Created on: 2013-9-11
 *      Author: QiuWeibo
 */

#ifndef SYSTEM_CLOCK_H_
#define SYSTEM_CLOCK_H_

#ifdef __cplusplus
 extern "C" {
#endif 

extern void CLOCK_Init(void);
extern unsigned long getCodeCLK(void);
extern unsigned long getSystemCLK(void);
extern unsigned long getVCOFreq(void);

#define SYSTEM_CLKIN    12000000
#define CORE_CLOCK      (264000000)
#define SYSTEM_CLOCK    (132000000)

/* Select the clock frequencies from the definitions below */
/* If none are selected __240MHZ_CCLK_120MHZ_SCLK_20MHz_RSI__ is the default */
#define __264MHZ_CCLK_132MHZ_SCLK_22MHz_RSI__
//#define __300MHZ_CCLK_100MHZ_SCLK_25MHz_RSI__

#if defined (__300MHZ_CCLK_100MHZ_SCLK_25MHz_RSI__)
#define VRCTL_VALUE         0x0000
#define PLLCTL_VALUE        0x3200
#define PLLDIV_VALUE        0x0003
#define PLLLOCKCNT_VALUE    0x0000
#define PLLSTAT_VALUE       0x0000
#define RSICLK_DIV          0x0001//RSI_CLK=SCLK/(2x(CLKDIV+1))

#elif defined (__264MHZ_CCLK_132MHZ_SCLK_22MHz_RSI__)
#define VRCTL_VALUE         (0x70B0)//0x0000
#define PLLCTL_VALUE        0x2C00
#define PLLDIV_VALUE        0x0002
#define PLLLOCKCNT_VALUE    (0x0200)//0x0000
#define PLLSTAT_VALUE       0x0000
#define RSICLK_DIV          0x0002//RSI_CLK=SCLK/(2x(CLKDIV+1))

#else /* __240MHZ_CCLK_120MHZ_SCLK_20MHz_RSI__ */
#define VRCTL_VALUE         0x0000
#define PLLCTL_VALUE        0x2800
#define PLLDIV_VALUE        0x0002
#define PLLLOCKCNT_VALUE    0x0000
#define PLLSTAT_VALUE       0x0000
#define RSICLK_DIV          0x0002//RSI_CLK=SCLK/(2x(CLKDIV+1))
#endif

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_CLOCK_H_ */
