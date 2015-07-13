/*
 * U-boot - Configuration file for BF518F EZBrd board
 */

#ifndef __CONFIG_BF518F_EZBRD_H__
#define __CONFIG_BF518F_EZBRD_H__

#include <asm/config-pre.h>

#define __ADSPBF512__   //-QWB-20140212
#define __ADSPBF51x__

/*
 * Processor Settings
 */
#define CONFIG_BFIN_CPU             bf512-0.0
#define CONFIG_BFIN_BOOT_MODE       BFIN_BOOT_SPI_MASTER


/*
 * Clock Settings :VCO=256MHz,CCLK=256MHz,SCLK=128MHz
 *	CCLK = (CLKIN * VCO_MULT) / CCLK_DIV
 *	SCLK = (CLKIN * VCO_MULT) / SCLK_DIV
 */
/* CONFIG_CLKIN_HZ is any value in Hz					*/
#define CONFIG_CLKIN_HZ			16000000
/* CLKIN_HALF controls the DF bit in PLL_CTL      0 = CLKIN		*/
/*                                                1 = CLKIN / 2		*/
#define CONFIG_CLKIN_HALF		0
/* PLL_BYPASS controls the BYPASS bit in PLL_CTL  0 = do not bypass	*/
/*                                                1 = bypass PLL	*/
#define CONFIG_PLL_BYPASS		0
/* VCO_MULT controls the MSEL (multiplier) bits in PLL_CTL		*/
/* Values can range from 0-63 (where 0 means 64)			*/
#define CONFIG_VCO_MULT			16
/* CCLK_DIV controls the core clock divider				*/
/* Values can be 1, 2, 4, or 8 ONLY					*/
#define CONFIG_CCLK_DIV			1
/* SCLK_DIV controls the system clock divider				*/
/* Values can range from 1-15						*/
#define CONFIG_SCLK_DIV			2


/*
 * Memory Settings
 */
/* This board has a 16meg MT48LC16M8A2 */
#define CONFIG_MEM_ADD_WDTH	9
#define CONFIG_MEM_SIZE		16

#define CONFIG_EBIU_SDRRC_VAL	0x07C8 //SDRAM Refresh Rate Control Register
#define CONFIG_EBIU_SDGCTL_VAL	(SCTLE | CL_3 | PASR_ALL | TRAS_6 | TRP_3 | TRCD_3 | TWR_2 | PSS)

#if 0
#define CONFIG_EBIU_AMGCTL_VAL	(AMCKEN | AMBEN_ALL)
#define CONFIG_EBIU_AMBCTL0_VAL	(B1WAT_15 | B1RAT_15 | B1HT_3 | B1RDYPOL | B0WAT_15 | B0RAT_15 | B0HT_3 | B0RDYPOL)
#define CONFIG_EBIU_AMBCTL1_VAL	(B3WAT_15 | B3RAT_15 | B3HT_3 | B3RDYPOL | B2WAT_15 | B2RAT_15 | B2HT_3 | B2RDYPOL)
#else
#define CONFIG_EBIU_AMGCTL_VAL	(0x00F2)    //Reset Value
#define CONFIG_EBIU_AMBCTL0_VAL	(0xFFC2FFC2)//Reset Value
#define CONFIG_EBIU_AMBCTL1_VAL	(0xFFC2FFC2)//Reset Value
#endif

/*SDRAM BANK3
 *0x00D8 0000 ~ 0x00EF FFFF     :App In SDRAM
 *0x00F0 0000 ~ 0x00F7 FFFF     :U-boot malloc zone
 *0x00F8 0000 ~ 0x00FF FFFF     :U-boot In SDRAM
 */
#define CONFIG_SYS_MONITOR_LEN	(512 * 1024)
#define CONFIG_SYS_MALLOC_LEN	(384 * 1024)
#define CONFIG_SYS_LOAD_ADDR    0x00D80000 


/*
 * Network Settings
 */
#if !defined(__ADSPBF512__) && !defined(__ADSPBF514__)
#define ADI_CMDS_NETWORK	1
#define CONFIG_BFIN_MAC
#define CONFIG_BFIN_MAC_PINS \
	{ \
	P_MII0_ETxD0, \
	P_MII0_ETxD1, \
	P_MII0_ETxD2, \
	P_MII0_ETxD3, \
	P_MII0_ETxEN, \
	P_MII0_TxCLK, \
	P_MII0_PHYINT, \
	P_MII0_COL, \
	P_MII0_ERxD0, \
	P_MII0_ERxD1, \
	P_MII0_ERxD2, \
	P_MII0_ERxD3, \
	P_MII0_ERxDV, \
	P_MII0_ERxCLK, \
	P_MII0_CRS, \
	P_MII0_MDC, \
	P_MII0_MDIO, \
	0 }
#define CONFIG_NETCONSOLE	1
#endif
#define CONFIG_HOSTNAME		bf518f-ezbrd
#define CONFIG_PHY_ADDR		3
/* Uncomment next line to use fixed MAC address */
/* #define CONFIG_ETHADDR	02:80:ad:20:31:e8 */


/*
 * Flash Settings
 */
#if 0
#define CONFIG_FLASH_CFI_DRIVER
#define CONFIG_SYS_FLASH_BASE		0x20000000
#define CONFIG_SYS_FLASH_CFI
#define CONFIG_SYS_FLASH_PROTECTION
#define CONFIG_SYS_MAX_FLASH_BANKS	1
#define CONFIG_SYS_MAX_FLASH_SECT	71
#else
#define CONFIG_SYS_NO_FLASH
#endif

/*
 * SPI Settings
 */
#define CONFIG_BFIN_SPI
#define CONFIG_ENV_SPI_MAX_HZ	        30000000
#define CONFIG_SF_DEFAULT_SPEED	        30000000
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SPI_BAUD_INITBLOCK       3   //-QWB- SPI_BAUD value

#define CONFIG_SPI_FLASH_SIZE           0x00200000 //Flash size
#define CONFIG_UBOOT_SPI_FLASH_SIZE     0x00080000 //Uboot size
#define CONFIG_USER_SPI_FLASH_SIZE      0x00180000 //(CONFIG_SPI_FLASH_SIZE - CONFIG_UBOOT_SPI_FLASH_SIZE))

/*
 * Env Storage Settings
 */
#if (CONFIG_BFIN_BOOT_MODE == BFIN_BOOT_SPI_MASTER)
#define CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_OFFSET	            0x10000
#define CONFIG_ENV_SIZE		            0x2000
#define CONFIG_ENV_SECT_SIZE	        0x10000
#else
#define CONFIG_ENV_IS_IN_FLASH
#define CONFIG_ENV_OFFSET	            0x4000
#define CONFIG_ENV_ADDR		            (CONFIG_SYS_FLASH_BASE + CONFIG_ENV_OFFSET)
#define CONFIG_ENV_SIZE		            0x2000
#define CONFIG_ENV_SECT_SIZE	        0x2000
#endif
#define CONFIG_ENV_IS_EMBEDDED_IN_LDR


/*
 * I2C Settings
 */
#define CONFIG_BFIN_TWI_I2C	            1
#define CONFIG_HARD_I2C		            1
#define CONFIG_SYS_I2C_SPEED            50000
#define CONFIG_SYS_I2C_SLAVE            0


/*
 * SDH Settings
 */
#if !defined(__ADSPBF512__)
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_BFIN_SDH
#endif


/*
 * Misc Settings
 */
/* #define CONFIG_BOARD_EARLY_INIT_F */
#define CONFIG_MISC_INIT_R
#define CONFIG_RTC_BFIN

/*
 * Console Setting
 */
#define CONFIG_UART_CONSOLE	            1
#define CONFIG_BAUDRATE                 230400 /* 256000 */
/*#define CONFIG_DEBUG_EARLY_SERIAL       1   //early serial*/

/*
 * Watchdog Setting
 */
/*#define CONFIG_HW_WATCHDOG*/

#define CONFIG_BOOTDELAY        1


/*
 * CVTE SeeCare AIO-BF512
 */
#define CONFIG_CHECK_AIO_BF512_UPDATE       1
#define CONFIG_BOOT_BUTTON                  1
#define CFG_BOOT_BUTTON_PIN                 GPIO_PF3
#define CONFIG_CHECK_LICENSE                1
#define CFG_DEFAULT_LICENSE                 0xFFFFFFFF
#define CONFIG_EXT_WATCHDOG                 1
#define CFG_EXT_WATCHDOG_WDI                GPIO_PF5
/*
 * Pull in common ADI header for remaining command/environment setup
 */
#include <configs/bfin_adi_common.h>

#endif
