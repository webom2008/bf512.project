/*********************************************************************************

Copyright(c) 2009 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_sdh_reg.h,v $
$Revision: 3459 $
$Date: 2010-09-29 10:37:28 -0400 (Wed, 29 Sep 2010) $

Description:
    This header file defines the MMRs for the Secure Digital Host (SDH) interface.
    It includes both the MMR addresses and the bitfield access macros.

*********************************************************************************/

#ifndef __ADI_SDH_REG_H__
#define __ADI_SDH_REG_H__


/* system service header  */
#include <services/services.h>

/* ADSP-BF54x (MOAB)/ADSP-BF51x (BRODIE)/ADSP-BF50x (MOY) Families */
#if defined(__ADSP_MOAB__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)

/*********************************************************************

MMC Registers

*********************************************************************/

/*********************************************************************

MMC Card Status Register (CSR) Enumerations

*********************************************************************/
typedef enum
{	ADI_MMC_CSR_CURRENT_STATE_IDLE            = (0),
	ADI_MMC_CSR_CURRENT_STATE_READY           = (1),
	ADI_MMC_CSR_CURRENT_STATE_IDENTIFICATION  = (2),
	ADI_MMC_CSR_CURRENT_STATE_STANDBY         = (3),
	ADI_MMC_CSR_CURRENT_STATE_TRANSFER        = (4),
	ADI_MMC_CSR_CURRENT_STATE_DATA            = (5),
	ADI_MMC_CSR_CURRENT_STATE_RECEIVE         = (6),
	ADI_MMC_CSR_CURRENT_STATE_PROGRAM         = (7),
	ADI_MMC_CSR_CURRENT_STATE_DISCONNECT      = (8),
	ADI_MMC_CSR_CURRENT_STATE_BUS_TEST        = (9),
	ADI_MMC_CSR_CURRENT_STATE_SLEEP           = (10),
	ADI_MMC_CSR_CURRENT_STATE_RESERVED2       = (11),
	ADI_MMC_CSR_CURRENT_STATE_RESERVED3       = (12),
	ADI_MMC_CSR_CURRENT_STATE_RESERVED4       = (13),
	ADI_MMC_CSR_CURRENT_STATE_RESERVED5       = (14),
	ADI_MMC_CSR_CURRENT_STATE_IO_MODE         = (15)
} ADI_MMC_CSR_CURRENT_CARD_STATE;


typedef unsigned long ADI_RSI_MMC_CSR_REGISTER;

/****************************************************/
/*  MMC CSR Register Access Macros                  */
/****************************************************/
#define ADI_MMC_CSR_OUT_OF_RANGE         ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>31)
#define ADI_MMC_CSR_ADDRESS_ERROR        ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>30)
#define ADI_MMC_CSR_BLOCK_LEN_ERROR      ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>29)
#define ADI_MMC_CSR_ERASE_SEQ_ERROR      ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>28)
#define ADI_MMC_CSR_ERASE_PARAM          ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>27)
#define ADI_MMC_CSR_WP_VIOLATION         ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>26)
#define ADI_MMC_CSR_CARD_IS_LOCKED       ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>25)
#define ADI_MMC_CSR_LOCK_UNLOCK_FAILED   ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>24)
#define ADI_MMC_CSR_COM_CRC_ERROR        ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>23)
#define ADI_MMC_CSR_ILLEGAL_COMMAND      ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>22)
#define ADI_MMC_CSR_CARD_ECC_FAILED      ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>21)
#define ADI_MMC_CSR_CC_ERROR             ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>20)
#define ADI_MMC_CSR_ERROR                ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>19)
#define ADI_MMC_CSR_UNDERRUN             ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>18)
#define ADI_MMC_CSR_OVERRUN              ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>17)
#define ADI_MMC_CSR_CSD_OVERWRITE        ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>16)
#define ADI_MMC_CSR_WP_ERASE_SKIP        ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>15)
#define ADI_MMC_CSR_ERASE_RESET          ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>13)
#define ADI_MMC_CSR_CURRENT_STATE        ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>9)
#define ADI_MMC_CSR_READY_FOR_DATA       ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>8)
#define ADI_MMC_CSR_SWITCH_ERROR         ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>7)
#define ADI_MMC_CSR_APP_CMD              ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>5)



/****************************************************/
/*  MMC OCR Register                                */
/****************************************************/
/****************************************************/
/*  MMC OCR Register Bit Masks                      */
/****************************************************/
#define ADI_MMC_OCR_LOW_VOLTAGE_MASK          (1<<7)
#define ADI_MMC_OCR_VOLTAGE_20_21_MASK        (1<<8)
#define ADI_MMC_OCR_VOLTAGE_21_22_MASK        (1<<9)
#define ADI_MMC_OCR_VOLTAGE_22_23_MASK        (1<<10)
#define ADI_MMC_OCR_VOLTAGE_23_24_MASK        (1<<11)
#define ADI_MMC_OCR_VOLTAGE_24_25_MASK        (1<<12)
#define ADI_MMC_OCR_VOLTAGE_25_26_MASK        (1<<13)
#define ADI_MMC_OCR_VOLTAGE_26_27_MASK        (1<<14)
#define ADI_MMC_OCR_VOLTAGE_27_28_MASK        (1<<15)
#define ADI_MMC_OCR_VOLTAGE_28_29_MASK        (1<<16)
#define ADI_MMC_OCR_VOLTAGE_29_30_MASK        (1<<17)
#define ADI_MMC_OCR_VOLTAGE_30_31_MASK        (1<<18)
#define ADI_MMC_OCR_VOLTAGE_31_32_MASK        (1<<19)
#define ADI_MMC_OCR_VOLTAGE_32_33_MASK        (1<<20)
#define ADI_MMC_OCR_VOLTAGE_33_34_MASK        (1<<21)
#define ADI_MMC_OCR_VOLTAGE_34_35_MASK        (1<<22)
#define ADI_MMC_OCR_VOLTAGE_35_36_MASK        (1<<23)
#define ADI_MMC_OCR_CARD_CAPACITY_STATUS_MASK (1<<30)
#define ADI_MMC_OCR_CARD_POWER_UP_STATUS_MASK (1<<31)

typedef  unsigned long ADI_RSI_MMC_OCR_REGISTER;

/****************************************************/
/*  MMC OCR Register Bit Masks                      */
/****************************************************/
#define ADI_MMC_OCR_LOW_VOLTAGE             ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_LOW_VOLTAGE_MASK)>>7)
#define ADI_MMC_OCR_VOLTAGE_27_28           ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_VOLTAGE_27_28_MASK)>>15)
#define ADI_MMC_OCR_VOLTAGE_28_29           ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_VOLTAGE_28_29_MASK)>>16)
#define ADI_MMC_OCR_VOLTAGE_29_30           ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_VOLTAGE_29_31_MASK)>>17)
#define ADI_MMC_OCR_VOLTAGE_30_31           ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_VOLTAGE_30_31_MASK)>>18)
#define ADI_MMC_OCR_VOLTAGE_31_32           ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_VOLTAGE_31_32_MASK)>>19)
#define ADI_MMC_OCR_VOLTAGE_32_33           ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_VOLTAGE_32_33_MASK)>>20)
#define ADI_MMC_OCR_VOLTAGE_33_34           ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_VOLTAGE_33_34_MASK)>>21)
#define ADI_MMC_OCR_VOLTAGE_34_35           ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_VOLTAGE_34_35_MASK)>>22)
#define ADI_MMC_OCR_VOLTAGE_35_36           ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_VOLTAGE_35_36_MASK)>>23)
#define ADI_MMC_OCR_CARD_ACCESS_MODE        ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_CARD_CAPACITY_STATUS)>>29)
#define ADI_MMC_OCR_CARD_POWER_UP_STATUS    ((*pADI_SDH_RESPONSE_0 & ADI_MMC_OCR_CARD_POWER_UP_STATUS)>>31)
#define ADI_MMC_OCR_VOLTAGE_HIGH            ((*pADI_SDH_RESPONSE_0 & 0x0ffF8000)>>23)

/****************************************************/
/*  MMC CID Register                                */
/****************************************************/
typedef enum
{	ADI_MMC_CID_CBX_DEVICE_TYPE_CARD = 0,
	ADI_MMC_CID_CBX_DEVICE_TYPE_BGA = 1,
	ADI_MMC_CID_CBX_DEVICE_TYPE_RESERVED_0 = 2,
	ADI_MMC_CID_CBX_DEVICE_TYPE_RESERVED_1 = 3
} ADI_MMC_CID_CBX;
/****************************************************/
/*  MMC CID Register Structure                      */
/****************************************************/
typedef struct _ADI_RSI_MMC_CID_REGISTER
{
    unsigned char   mid;
    unsigned char   cbx;
    unsigned char   oid;
    char            pnm[6];
    unsigned char   prv;
    unsigned long   psn;
    unsigned short  mdt;
    unsigned char   crc;
} ADI_RSI_MMC_CID_REGISTER;

/****************************************************/
/*  MMC CID Register Access Macros                  */
/****************************************************/
#define ADI_MMC_CID_MID     ((*pADI_SDH_RESPONSE_0 & 0xFF000000)>>24)
#define ADI_MMC_CID_CBX     ((*pADI_SDH_RESPONSE_0 & 0x00030000)>>16)
#define ADI_MMC_CID_OID     ((*pADI_SDH_RESPONSE_0 & 0x0000FF00)>>8)
#define ADI_MMC_CID_PNM0    ((*pADI_SDH_RESPONSE_0 & 0x000000FF))
#define ADI_MMC_CID_PNM1    ((*pADI_SDH_RESPONSE_1 & 0xFF000000)>>24)
#define ADI_MMC_CID_PNM2    ((*pADI_SDH_RESPONSE_1 & 0x00FF0000)>>16)
#define ADI_MMC_CID_PNM3    ((*pADI_SDH_RESPONSE_1 & 0x0000FF00)>>8)
#define ADI_MMC_CID_PNM4    ((*pADI_SDH_RESPONSE_1 & 0x000000FF))
#define ADI_MMC_CID_PNM5    ((*pADI_SDH_RESPONSE_2 & 0xFF000000)>>24)
#define ADI_MMC_CID_PRV     ((*pADI_SDH_RESPONSE_2 & 0x00FF0000)>>16)
#define ADI_MMC_CID_PSN     (((*pADI_SDH_RESPONSE_2 & 0x0000FFFF)<<16) | ((*pADI_SDH_RESPONSE_3 & 0x7FFF8000)>>15))
#define ADI_MMC_CID_MDT     ((*pADI_SDH_RESPONSE_3 & 0x00007F80)>>7)
#define ADI_MMC_CID_CRC     ((*pADI_SDH_RESPONSE_3 & 0x0000007F))


/****************************************************/
/*  MMC CSD Register                                */
/****************************************************/
/****************************************************/
/*  MMC Card Status Register Enumerations           */
/****************************************************/
typedef enum
{
    ADI_MMC_CSD_STRUCTURE_VERSION_1_0           = 0,
    ADI_MMC_CSD_STRUCTURE_VERSION_1_1           = 1,
    ADI_MMC_CSD_STRUCTURE_VERSION_1_2           = 2,
    ADI_MMC_CSD_STRUCTURE_VERSION_IN_EXT_CSD    = 3
}ADI_MMC_CSD_STRUCTURE;

typedef enum
{	ADI_MMC_CSD_VERSION_ALLOCATED_BY_MMCA_0 = 0,
	ADI_MMC_CSD_VERSION_ALLOCATED_BY_MMCA_1 = 1,
	ADI_MMC_CSD_VERSION_ALLOCATED_BY_MMCA_2 = 2,
	ADI_MMC_CSD_VERSION_ALLOCATED_BY_MMCA_3 = 3,
	ADI_MMC_CSD_VERSION_4_1_TO_4_3 = 4,
	ADI_MMC_CSD_VERSION_RESERVED_0 = 5,
	ADI_MMC_CSD_VERSION_RESERVED_1 = 6,
	ADI_MMC_CSD_VERSION_RESERVED_2 = 7,
	ADI_MMC_CSD_VERSION_RESERVED_3 = 8,
	ADI_MMC_CSD_VERSION_RESERVED_4 = 9,
	ADI_MMC_CSD_VERSION_RESERVED_5 = 10,
	ADI_MMC_CSD_VERSION_RESERVED_6 = 11,
	ADI_MMC_CSD_VERSION_RESERVED_7 = 12,
	ADI_MMC_CSD_VERSION_RESERVED_8 = 13,
	ADI_MMC_CSD_VERSION_RESERVED_9 = 14,
	ADI_MMC_CSD_VERSION_RESERVED_10 = 15
} ADI_MMC_CSD_SPEC_VERS;

typedef enum
{
    ADI_MMC_CSD_VDD_CURR_MIN_0_5mA   = 0,
    ADI_MMC_CSD_VDD_CURR_MIN_1_0mA   = 1,
    ADI_MMC_CSD_VDD_CURR_MIN_5_0mA   = 2,
    ADI_MMC_CSD_VDD_CURR_MIN_10_0mA  = 3,
    ADI_MMC_CSD_VDD_CURR_MIN_25_0mA  = 4,
    ADI_MMC_CSD_VDD_CURR_MIN_35_0mA  = 5,
    ADI_MMC_CSD_VDD_CURR_MIN_60_0mA  = 6,
    ADI_MMC_CSD_VDD_CURR_MIN_100_0mA = 7,
    ADI_MMC_CSD_VDD_CURR_MIN_NOT_APPLICABLE = 8
} ADI_MMC_CSD_VDD_CURR_MIN;

typedef enum
{
    ADI_MMC_CSD_VDD_CURR_MAX_1_0mA   = 0,
    ADI_MMC_CSD_VDD_CURR_MAX_5_0mA   = 1,
    ADI_MMC_CSD_VDD_CURR_MAX_10_0mA  = 2,
    ADI_MMC_CSD_VDD_CURR_MAX_25_0mA  = 3,
    ADI_MMC_CSD_VDD_CURR_MAX_35_0mA  = 4,
    ADI_MMC_CSD_VDD_CURR_MAX_45_0mA  = 5,
    ADI_MMC_CSD_VDD_CURR_MAX_80_0mA  = 6,
    ADI_MMC_CSD_VDD_CURR_MAX_200_0mA = 7,
    ADI_MMC_CSD_VDD_CURR_MAX_NOT_APPLICABLE = 8
} ADI_MMC_CSD_VDD_CURR_MAX;

typedef enum
{
    ADI_MMC_CSD_FILE_FORMAT_HD_WITH_PARTITION_TABLE          = 0,
    ADI_MMC_CSD_FILE_FORMAT_DOS_FAT_WITH_BOOT_SECTOR_ONLY    = 1,
    ADI_MMC_CSD_FILE_FORMAT_UNIVERSAL_FILE_FORMAT            = 2,
    ADI_MMC_CSD_FILE_FORMAT_OTHER_UNKNOWN                    = 3
} ADI_MMC_CSD_FILE_FORMAT;

typedef enum
{	ADI_MMC_CSD_ECC_NONE = 0,
	ADI_MMC_CSD_ECC_BCH_542_512 = 1,
	ADI_MMC_CSD_ECC_RESERVED_0 = 2,
	ADI_MMC_CSD_ECC_RESERVED_1 = 3
} ADI_MMC_CSD_ECC;

/********************************************************/
/*  MMC CSD Register Structure                          */
/********************************************************/
typedef struct _ADI_RSI_MMC_CSD_REGISTER
{
    ADI_MMC_CSD_STRUCTURE   csd_structure;
    ADI_MMC_CSD_SPEC_VERS   spec_vers;
    unsigned char       taac;
    unsigned char       nsac;
    unsigned char       tran_speed;
    unsigned short      ccc;
    unsigned char       read_bl_len;
    bool                read_bl_partial;
    bool                write_blk_misalign;
    bool                read_blk_misalign;
    bool                dsr_imp;
    unsigned short       c_size;
    ADI_MMC_CSD_VDD_CURR_MIN vdd_r_curr_min;
    ADI_MMC_CSD_VDD_CURR_MAX vdd_r_curr_max;
    ADI_MMC_CSD_VDD_CURR_MIN vdd_w_curr_min;
    ADI_MMC_CSD_VDD_CURR_MAX vdd_w_curr_max;
    unsigned short       c_size_mult;
    unsigned char        erase_grp_size;
    unsigned char        erase_grp_mult;
    unsigned char       wp_grp_size;
    bool                wp_grp_enable;
    ADI_MMC_CSD_ECC         default_ecc;
    unsigned char       r2w_factor;
    unsigned char       write_bl_length;
    bool                write_bl_partial;
    bool                content_prot_app;
    bool                file_format_grp;
    bool                copy;
    bool                perm_write_protect;
    bool                tmp_write_protect;
    ADI_MMC_CSD_FILE_FORMAT  file_format;
    ADI_MMC_CSD_ECC         ecc;
    unsigned char       crc;
}ADI_RSI_MMC_CSD_REGISTER;



/****************************************************/
/*  MMC Extended CSD Register                       */
/****************************************************/
/****************************************************/
/*  MMC Extended CSD Register Enumerations          */
/****************************************************/

/********************************************************/
/*  MMC EXTENDED CSD Register Structure                 */
/********************************************************/
typedef struct _ADI_RSI_MMC_EXTCSD_REGISTER
{
    unsigned char        s_cmd_set;
    unsigned char        boot_info;
    unsigned char        boot_size_mult;
    unsigned char        acc_size;
    unsigned char        hc_erase_grp_size;
    unsigned char        erase_timeout_mult;
    unsigned char        rel_wr_sec_c;
    unsigned char        hc_wp_grp_size;
    unsigned char        s_c_vcc;
    unsigned char        s_c_vccq;
    unsigned char        s_a_timeout;
    unsigned long        sec_count;
    unsigned char        min_perf_w_8_52;
    unsigned char        min_perf_r_8_52;
    unsigned char        min_perf_w_8_26_4_52;
    unsigned char        min_perf_r_8_26_4_52;
    unsigned char        min_perf_w_4_26;
    unsigned char        min_perf_r_4_26;
    unsigned char        pwr_cl_26_360;
    unsigned char        pwr_cl_52_360;
    unsigned char        pwr_cl_26_195;
    unsigned char        pwr_cl_52_195;
    unsigned char        card_type;
    unsigned char        csd_structure;
    unsigned char        ext_csd_rev;
    unsigned char        cmd_set;
    unsigned char        cmd_set_rev;
    unsigned char        power_class;
    unsigned char        hs_timing;
    unsigned char        bus_width;
    unsigned char        erased_mem_cont;
    unsigned char        boot_config;
    unsigned char        boot_bus_width;
    unsigned char        erase_group_def;
}ADI_RSI_MMC_EXTCSD_REGISTER;



/********************************************************/
/*  MMC RCA Register                                    */
/********************************************************/
typedef unsigned short ADI_RSI_MMC_RCA_REGISTER;

/********************************************************/
/*  MMC DSR Register                                    */
/********************************************************/
typedef unsigned short ADI_RSI_MMC_DSR_REGISTER;

/********************************************************/
/*  MMC Card Register Structure                         */
/********************************************************/
typedef struct _ADI_RSI_MMC_CARD_REGISTERS
{
    ADI_RSI_MMC_CSR_REGISTER csr;
    ADI_RSI_MMC_OCR_REGISTER ocr;
    ADI_RSI_MMC_CID_REGISTER cid;
    ADI_RSI_MMC_CSD_REGISTER csd;
    ADI_RSI_MMC_EXTCSD_REGISTER extcsd;
    ADI_RSI_MMC_RCA_REGISTER rca;
    ADI_RSI_MMC_DSR_REGISTER dsr;
} ADI_RSI_MMC_CARD_REGISTERS;





/****************************************************/
/*  SD Card Status Register                         */
/****************************************************/
/****************************************************/
/*  SD Card Status Register Enumerations            */
/****************************************************/
typedef enum {  ADI_SD_CSR_CURRENT_STATE_IDLE            = (0<<9),
                ADI_SD_CSR_CURRENT_STATE_READY           = (1<<9),
                ADI_SD_CSR_CURRENT_STATE_IDENTIFICATION  = (2<<9),
                ADI_SD_CSR_CURRENT_STATE_STANDBY         = (3<<9),
                ADI_SD_CSR_CURRENT_STATE_TRANSFER        = (4<<9),
                ADI_SD_CSR_CURRENT_STATE_DATA            = (5<<9),
                ADI_SD_CSR_CURRENT_STATE_RECEIVE         = (6<<9),
                ADI_SD_CSR_CURRENT_STATE_PROGRAM         = (7<<9),
                ADI_SD_CSR_CURRENT_STATE_DISCONNECT      = (8<<9),
                ADI_SD_CSR_CURRENT_STATE_RESERVED0       = (9<<9),
                ADI_SD_CSR_CURRENT_STATE_RESERVED1       = (10<<9),
                ADI_SD_CSR_CURRENT_STATE_RESERVED2       = (11<<9),
                ADI_SD_CSR_CURRENT_STATE_RESERVED3       = (12<<9),
                ADI_SD_CSR_CURRENT_STATE_RESERVED4       = (13<<9),
                ADI_SD_CSR_CURRENT_STATE_RESERVED5       = (14<<9),
                ADI_SD_CSR_CURRENT_STATE_IO_MODE         = (15<<9)
} ADI_SD_CSR_CURRENT_CARD_STATE;

/****************************************************/
/*  SD Card Status Register Bit Masks               */
/****************************************************/
#define ADI_SD_CSR_AKE_SEQ_ERROR        (1<<3)
#define ADI_SD_CSR_APP_CMD              (1<<5)
#define ADI_SD_CSR_READY_FOR_DATA       (1<<8)
#define ADI_SD_CSR_CURRENT_STATE        (15<<9)
#define ADI_SD_CSR_ERASE_RESET          (1<<13)
#define ADI_SD_CSR_CARD_ECC_DISABLED    (1<<14)
#define ADI_SD_CSR_WP_ERASE_SKIP        (1<<15)
#define ADI_SD_CSR_CSD_OVERWRITE        (1<<16)
#define ADI_SD_CSR_ERROR                (1<<19)
#define ADI_SD_CSR_CC_ERROR             (1<<20)
#define ADI_SD_CSR_CARD_ECC_FAILED      (1<<21)
#define ADI_SD_CSR_ILLEGAL_COMMAND      (1<<22)
#define ADI_SD_CSR_COM_CRC_ERROR        (1<<23)
#define ADI_SD_CSR_LOCK_UNLOCK_FAILED   (1<<24)
#define ADI_SD_CSR_CARD_IS_LOCKED       (1<<25)
#define ADI_SD_CSR_WP_VIOLATION         (1<<26)
#define ADI_SD_CSR_ERASE_PARAM          (1<<27)
#define ADI_SD_CSR_ERASE_SEQ_ERROR      (1<<28)
#define ADI_SD_CSR_BLOCK_LEN_ERROR      (1<<29)
#define ADI_SD_CSR_ADDRESS_ERROR        (1<<30)
#define ADI_SD_CSR_OUT_OF_RANGE         (1<<31)

typedef unsigned long ADI_RSI_SD_CSR_REGISTER;
/****************************************************/
/*  SD Status Register                              */
/****************************************************/

/****************************************************/
/*  SD Status Register Enumerations                 */
/****************************************************/
typedef enum {  ADI_SD_SSR_DAT_BUS_WIDTH_1           = 0,
                ADI_SD_SSR_DAT_BUS_WIDTH_RESERVED0   = 1,
                ADI_SD_SSR_DAT_BUS_WIDTH_4           = 2,
                ADI_SD_SSR_DAT_BUS_WIDTH_RESERVED1   = 3
} ADI_SD_SSR_DAT_BUS_WIDTH;

typedef enum {  ADI_SD_SSR_SECURED_MODE_DISABLED           = 0,
                ADI_SD_SSR_SECURED_MODE_ENABLED            = 1
} ADI_SD_SSR_SECURED_MODE;

typedef enum {  ADI_SD_SSR_SD_CARD_TYPE_REGULAR_RD_WR_CARD   = 0,
                ADI_SD_SSR_SD_CARD_TYPE_ROM                  = 1
} ADI_SD_SSR_SD_CARD_TYPE;

typedef enum {  ADI_SD_SSR_SPEED_CLASS_0 = 0,
                ADI_SD_SSR_SPEED_CLASS_2 = 1,
                ADI_SD_SSR_SPEED_CLASS_3 = 2,
                ADI_SD_SSR_SPEED_CLASS_6 = 3
} ADI_SD_SSR_SPEED_CLASS;

typedef enum {  ADI_SD_SSR_AU_SIZE_NOT_DEFINED   = 0,
                ADI_SD_SSR_AU_SIZE_16KB          = 1,
                ADI_SD_SSR_AU_SIZE_32KB          = 2,
                ADI_SD_SSR_AU_SIZE_64KB          = 3,
                ADI_SD_SSR_AU_SIZE_128KB         = 4,
                ADI_SD_SSR_AU_SIZE_256KB         = 5,
                ADI_SD_SSR_AU_SIZE_512KB         = 6,
                ADI_SD_SSR_AU_SIZE_1MB           = 7,
                ADI_SD_SSR_AU_SIZE_2MB           = 8,
                ADI_SD_SSR_AU_SIZE_4MB           = 9,
                ADI_SD_SSR_AU_SIZE_RESERVED0     = 10,
                ADI_SD_SSR_AU_SIZE_RESERVED1     = 11,
                ADI_SD_SSR_AU_SIZE_RESERVED2     = 12,
                ADI_SD_SSR_AU_SIZE_RESERVED3     = 13,
                ADI_SD_SSR_AU_SIZE_RESERVED4     = 14,
                ADI_SD_SSR_AU_SIZE_RESERVED5     = 15
}ADI_SD_SSR_AU_SIZE;


/****************************************************/
/*  SD Status Register Structure                    */
/****************************************************/
typedef struct _ADI_RSI_SD_SSR_REGISTER{
    ADI_SD_SSR_DAT_BUS_WIDTH dat_bus_width;
    ADI_SD_SSR_SECURED_MODE  secured_mode;
    ADI_SD_SSR_SD_CARD_TYPE  sd_card_type;
    unsigned long           size_of_protected_area;
    ADI_SD_SSR_SPEED_CLASS   speed_class;
    unsigned char           performance_move;
    ADI_SD_SSR_AU_SIZE       au_size;
    unsigned short          erase_size;
    unsigned char           erase_timeout;
    unsigned char           erase_offset;
}ADI_RSI_SD_SSR_REGISTER;

/****************************************************/
/*  SD OCR Register                                 */
/****************************************************/
/****************************************************/
/*  SD OCR Register Bit Masks                       */
/****************************************************/
#define ADI_SD_OCR_LOW_VOLTAGE          (1<<7)
#define ADI_SD_OCR_VOLTAGE_27_28        (1<<15)
#define ADI_SD_OCR_VOLTAGE_28_29        (1<<16)
#define ADI_SD_OCR_VOLTAGE_29_30        (1<<17)
#define ADI_SD_OCR_VOLTAGE_30_31        (1<<18)
#define ADI_SD_OCR_VOLTAGE_31_32        (1<<19)
#define ADI_SD_OCR_VOLTAGE_32_33        (1<<20)
#define ADI_SD_OCR_VOLTAGE_33_34        (1<<21)
#define ADI_SD_OCR_VOLTAGE_34_35        (1<<22)
#define ADI_SD_OCR_VOLTAGE_35_36        (1<<23)
#define ADI_SD_OCR_CARD_CAPACITY_STATUS (1<<30)
#define ADI_SD_OCR_CARD_POWER_UP_STATUS (1<<31)

typedef  unsigned long ADI_RSI_SD_OCR_REGISTER;

/****************************************************/
/*  SD CID Register                                 */
/****************************************************/
/****************************************************/
/*  SD CID Register Structure                       */
/****************************************************/
typedef struct _ADI_RSI_SD_CID_REGISTER{
    unsigned char   mid;
    char            oid[2];
    char            pnm[5];
    unsigned char   prv;
    unsigned long   psn;
    unsigned short  mdt;
    unsigned char   crc;
} ADI_RSI_SD_CID_REGISTER;


/****************************************************/
/*  SD CSD Register Version 1.0 & 2.0               */
/****************************************************/
/****************************************************/
/*  SD CSD Register Version 1.0 & 2.0 Enumerations  */
/****************************************************/
typedef enum{
    ADI_SD_CSD_STRUCTURE_VERSION_1_0          = 0,
    ADI_SD_CSD_STRUCTURE_VERSION_2_0          = 1,
    ADI_SD_CSD_STRUCTURE_VERSION_RESERVED0    = 2,
    ADI_SD_CSD_STRUCTURE_VERSION_RESERVED1    = 3
}ADI_SD_CSD_STRUCTURE;

typedef enum{
    ADI_SD_CSD_VDD_CURR_MIN_0_5mA   = 0,
    ADI_SD_CSD_VDD_CURR_MIN_1_0mA   = 1,
    ADI_SD_CSD_VDD_CURR_MIN_5_0mA   = 2,
    ADI_SD_CSD_VDD_CURR_MIN_10_0mA  = 3,
    ADI_SD_CSD_VDD_CURR_MIN_25_0mA  = 4,
    ADI_SD_CSD_VDD_CURR_MIN_35_0mA  = 5,
    ADI_SD_CSD_VDD_CURR_MIN_60_0mA  = 6,
    ADI_SD_CSD_VDD_CURR_MIN_100_0mA = 7,
    ADI_SD_CSD_VDD_CURR_MIN_NOT_APPLICABLE = 8
}ADI_SD_CSD_VDD_CURR_MIN;

typedef enum{
    ADI_SD_CSD_VDD_CURR_MAX_1_0mA   = 0,
    ADI_SD_CSD_VDD_CURR_MAX_5_0mA   = 1,
    ADI_SD_CSD_VDD_CURR_MAX_10_0mA  = 2,
    ADI_SD_CSD_VDD_CURR_MAX_25_0mA  = 3,
    ADI_SD_CSD_VDD_CURR_MAX_35_0mA  = 4,
    ADI_SD_CSD_VDD_CURR_MAX_45_0mA  = 5,
    ADI_SD_CSD_VDD_CURR_MAX_80_0mA  = 6,
    ADI_SD_CSD_VDD_CURR_MAX_200_0mA = 7,
    ADI_SD_CSD_VDD_CURR_MAX_NOT_APPLICABLE = 8
}ADI_SD_CSD_VDD_CURR_MAX;

typedef enum{
    ADI_SD_CSD_FILE_FORMAT_HD_WITH_PARTITION_TABLE          = 0,
    ADI_SD_CSD_FILE_FORMAT_DOS_FAT_WITH_BOOT_SECTOR_ONLY    = 1,
    ADI_SD_CSD_FILE_FORMAT_UNIVERSAL_FILE_FORMAT            = 2,
    ADI_SD_CSD_FILE_FORMAT_OTHER_UNKNOWN                    = 3
}ADI_SD_CSD_FILE_FORMAT;

/********************************************************/
/*  SD CSD Register Version 1.0 and 2.0 Structure       */
/********************************************************/
/********************************************************/
/* For Version 2.0 support C_size has been extended     */
/* Vdd_curr_min and Vdd_curr_max values not applicable  */
/* C_SIZE_MULT not applicable                           */
/********************************************************/
typedef struct _ADI_RSI_SD_CSD_REGISTER{
    ADI_SD_CSD_STRUCTURE    csd_structure;
    unsigned char       taac;
    unsigned char       nsac;
    unsigned char       tran_speed;
    unsigned short      ccc;
    unsigned char       read_bl_len;
    bool                read_bl_partial;
    bool                write_blk_misalign;
    bool                read_blk_misalign;
    bool                dsr_imp;
    unsigned long       c_size;
    ADI_SD_CSD_VDD_CURR_MIN vdd_r_curr_min;
    ADI_SD_CSD_VDD_CURR_MAX vdd_r_curr_max;
    ADI_SD_CSD_VDD_CURR_MIN vdd_w_curr_min;
    ADI_SD_CSD_VDD_CURR_MAX vdd_w_curr_max;
    unsigned char       c_size_mult;
    bool                erase_blk_en;
    unsigned char       sector_size;
    unsigned char       wp_grp_size;
    bool                wp_grp_enable;
    unsigned char       r2w_factor;
    unsigned char       write_bl_length;
    bool                write_bl_partial;
    bool                file_format_grp;
    bool                copy;
    bool                perm_write_protect;
    bool                tmp_write_protect;
    ADI_SD_CSD_FILE_FORMAT  file_format;
    unsigned char       crc;
}ADI_RSI_SD_CSD_REGISTER;


/********************************************************/
/*  SD RCA Register                                     */
/********************************************************/    
typedef unsigned short ADI_RSI_SD_RCA_REGISTER;

/********************************************************/
/*  SD DSR Register                                     */
/********************************************************/    
typedef unsigned short ADI_RSI_SD_DSR_REGISTER;


/****************************************************/
/*  SD SCR Register                                 */
/****************************************************/
/****************************************************/
/*  SD SCR Register Enumerations                    */
/****************************************************/
typedef enum{
    ADI_SD_SCR_STRUCTURE_VERSION_NO_1_0    = 0,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED0  = 1,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED1  = 2,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED2  = 3,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED3  = 4,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED4  = 5,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED5  = 6,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED6  = 7,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED7  = 8,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED8  = 9,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED9  = 10,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED10 = 11,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED11 = 12,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED12 = 13,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED13 = 14,
    ADI_SD_SCR_STRUCTURE_VERSION_RESERVED14 = 15
}ADI_SD_SCR_STRUCTURE;

typedef enum{
    ADI_SD_SCR_SD_SPEC_VERSION_1_0_TO_1_01   = 0,
    ADI_SD_SCR_SD_SPEC_VERSION_1_10      = 1,
    ADI_SD_SCR_SD_SPEC_VERSION_2_00      = 2,
    ADI_SD_SCR_SD_SPEC_RESERVED0        = 3,
    ADI_SD_SCR_SD_SPEC_RESERVED1        = 4,
    ADI_SD_SCR_SD_SPEC_RESERVED2        = 5,
    ADI_SD_SCR_SD_SPEC_RESERVED3        = 6,
    ADI_SD_SCR_SD_SPEC_RESERVED4        = 7,
    ADI_SD_SCR_SD_SPEC_RESERVED5        = 8,
    ADI_SD_SCR_SD_SPEC_RESERVED6        = 9,
    ADI_SD_SCR_SD_SPEC_RESERVED7        = 10,
    ADI_SD_SCR_SD_SPEC_RESERVED8        = 11,
    ADI_SD_SCR_SD_SPEC_RESERVED9        = 12,
    ADI_SD_SCR_SD_SPEC_RESERVED10       = 13,
    ADI_SD_SCR_SD_SPEC_RESERVED11       = 14,
    ADI_SD_SCR_SD_SPEC_RESERVED12       = 15
}ADI_SD_SCR_SD_SPEC;


typedef enum{
    ADI_SD_SCR_SD_SECURITY_NO_SECURITY  = 0,
    ADI_SD_SCR_SD_SECURITY_NOT_USED     = 1,
    ADI_SD_SCR_SD_SECURITY_VERSION_1_01  = 2,
    ADI_SD_SCR_SD_SECURITY_VERSION_2_00  = 3,
    ADI_SD_SCR_SD_SECURITY_RESERVED0    = 4,
    ADI_SD_SCR_SD_SECURITY_RESERVED1    = 5,
    ADI_SD_SCR_SD_SECURITY_RESERVED2    = 6,
    ADI_SD_SCR_SD_SECURITY_RESERVED3    = 7
}ADI_SD_SCR_SD_SECURITY;

typedef enum{
    ADI_SD_SCR_SD_BUS_WIDTHS_1_BIT      = 0,
    ADI_SD_SCR_SD_BUS_WIDTHS_RESERVED0  = 1,
    ADI_SD_SCR_SD_BUS_WIDTHS_4_BIT      = 2,
    ADI_SD_SCR_SD_BUS_WIDTHS_RESERVED1  = 3,
    ADI_SD_SCR_SD_BUS_WIDTHS_1_BIT_AND_4_BIT = 5,
}ADI_SD_SCR_SD_BUS_WIDTHS;

/********************************************************/
/*  SD SCR Register Structure                           */
/********************************************************/

typedef struct _ADI_RSI_SD_SCR_REGISTER{
    ADI_SD_SCR_STRUCTURE        scr_structure;
    ADI_SD_SCR_SD_SPEC          sd_spec;
    bool                    data_stat_after_erase;
    ADI_SD_SCR_SD_SECURITY      sd_security;
    ADI_SD_SCR_SD_BUS_WIDTHS    sd_bus_widths;
}ADI_RSI_SD_SCR_REGISTER;


/********************************************************/
/*  SD Card Register Structure                          */
/********************************************************/
typedef struct _ADI_RSI_SD_CARD_REGISTERS{
    ADI_RSI_SD_CSR_REGISTER csr;
    ADI_RSI_SD_SSR_REGISTER ssr;
    ADI_RSI_SD_OCR_REGISTER ocr;
    ADI_RSI_SD_CID_REGISTER cid;
    ADI_RSI_SD_CSD_REGISTER csd;
    ADI_RSI_SD_RCA_REGISTER rca;
    ADI_RSI_SD_DSR_REGISTER dsr;
    ADI_RSI_SD_SCR_REGISTER scr;
}ADI_RSI_SD_CARD_REGISTERS;   


typedef struct _ADI_RSI_CARD_REGISTERS{
	ADI_RSI_MMC_CARD_REGISTERS mmc;
	ADI_RSI_SD_CARD_REGISTERS sd;
}ADI_RSI_CARD_REGISTERS;



/*********************************************************************************

Processor specific MMR defines for SDH

*********************************************************************************/

/* abstract a select few macros for processor independance
*/

#if defined (__ADSP_MOAB__)

/* Moab-specific abstractions
*/
#define ADI_DMA_PMAP_SDH_ABSTRACTION     ADI_DMA_PMAP_SDH_NFC
#define ADI_INT_SDH_ABSTRACTION          ADI_INT_SDH_INT0

#elif defined (__ADSP_BRODIE__)

/* Brodie-specific abstractions
*/
#define ADI_DMA_PMAP_SDH_ABSTRACTION     ADI_DMA_PMAP_SPORT0_TX_AND_RSI
#define ADI_INT_SDH_ABSTRACTION          ADI_INT_RSI0

#elif defined(__ADSP_MOY__)

/* Moy-specific abstractions */
#define ADI_DMA_PMAP_SDH_ABSTRACTION     ADI_DMA_PMAP_RSI
#define ADI_INT_SDH_ABSTRACTION          ADI_INT_RSI0

#else
#error "Definitions for this target are missing."

#endif

/* SDH registers/register offsets for Moab/Brodie/Moy class devices    */
/* NOTE: While Moab uses "ADI_SDH_xxx", Brodie would use "ADI_RSI_xxx", but we stick
 * with "ADI_SDH_xxx" for compatibility
 */

#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
#define ADI_SDH_REG_BASE                    0xFFC03800
#elif defined(__ADSP_MOAB__)
#define ADI_SDH_REG_BASE                    0xFFC03900
#else
#error "Definitions for this target are missing."
#endif

#define ADI_SDH_PWR_CTRL_REG_OFFSET             0x0000  /* SDH Power Control register offset                */
#define ADI_SDH_CLK_CTRL_REG_OFFSET             0x0004  /* SDH Clock Control register offset                */
#define ADI_SDH_ARGUMENT_REG_OFFSET             0x0008  /* SDH Argument register offset                     */
#define ADI_SDH_COMMAND_REG_OFFSET              0x000C  /* SDH Command register offset                      */
#define ADI_SDH_RESPONSE_COMMAND_REG_OFFSET     0x0010  /* SDH Response Command register offset             */
#define ADI_SDH_RESPONSE0_REG_OFFSET            0x0014  /* SDH Response 0 register offset                   */
#define ADI_SDH_RESPONSE1_REG_OFFSET            0x0018  /* SDH Response 1 register offset                   */
#define ADI_SDH_RESPONSE2_REG_OFFSET            0x001C  /* SDH Response 2 register offset                   */
#define ADI_SDH_RESPONSE3_REG_OFFSET            0x0020  /* SDH Response 3 register offset                   */
#define ADI_SDH_DATA_TIMER_REG_OFFSET           0x0024  /* SDH Data Timer register offset                   */
#define ADI_SDH_DATA_LGTH_REG_OFFSET            0x0028  /* SDH Data Length register offset                  */
#define ADI_SDH_DATA_CTRL_REG_OFFSET            0x002C  /* SDH Data Control register offset                 */
#define ADI_SDH_DATA_CNT_REG_OFFSET             0x0030  /* SDH Data Count register offset                   */
#define ADI_SDH_STATUS_REG_OFFSET               0x0034  /* SDH Status register offset                       */
#define ADI_SDH_STATUS_CLR_REG_OFFSET           0x0038  /* SDH Status Clear register offset                 */
#define ADI_SDH_INT_MASK0_REG_OFFSET            0x003C  /* SDH Interrupt Mask 0 register offset             */
#define ADI_SDH_INT_MASK1_REG_OFFSET            0x0040  /* SDH Interrupt Mask 1 register offset             */
#define ADI_SDH_FIFO_COUNTER_REG_OFFSET         0x0048  /* SDH FIFO Counter register offset                 */

#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
#define ADI_SDH_CEATA_REG_OFFSET                0x004C  /* SDH CEATA register offset                        */
#endif

#define ADI_SDH_FIFO_REG_START_OFFSET           0x0080  /* SDH Data FIFO start register offset              */
#define ADI_SDH_E_STATUS_REG_OFFSET             0x00C0  /* SDH Exception Status register offset             */
#define ADI_SDH_E_MASK_REG_OFFSET               0x00C4  /* SDH Exception Mask register offset               */
#define ADI_SDH_CFG_REG_OFFSET                  0x00C8  /* SDH Configuration register offset                */
#define ADI_SDH_RD_WAIT_EN_OFFSET               0x00CC  /* SDH Read Wait Enable register offset             */

#if defined(__ADSP_MOAB__)

/* Moab-specific PID register offsets
*/
#define ADI_SDH_PID0_OFFSET                     0x00D0  /* SDH Peripheral Identification register 0 offset  */
#define ADI_SDH_PID1_OFFSET                     0x00D4  /* SDH Peripheral Identification register 1 offset  */
#define ADI_SDH_PID2_OFFSET                     0x00D8  /* SDH Peripheral Identification register 2 offset  */
#define ADI_SDH_PID3_OFFSET                     0x00DC  /* SDH Peripheral Identification register 3 offset  */
#define ADI_SDH_PID4_OFFSET                     0x00E0  /* SDH Peripheral Identification register 4 offset  */
#define ADI_SDH_PID5_OFFSET                     0x00E4  /* SDH Peripheral Identification register 5 offset  */
#define ADI_SDH_PID6_OFFSET                     0x00E8  /* SDH Peripheral Identification register 6 offset  */
#define ADI_SDH_PID7_OFFSET                     0x00EC  /* SDH Peripheral Identification register 7 offset  */

#elif defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)

/* Brodie-specific PID register offsets
*/
#define ADI_SDH_PID0_OFFSET                     0x0FE0  /* SDH Peripheral Identification register 0 offset  */
#define ADI_SDH_PID1_OFFSET                     0x0FE4  /* SDH Peripheral Identification register 1 offset  */
#define ADI_SDH_PID2_OFFSET                     0x0FE8  /* SDH Peripheral Identification register 2 offset  */
#define ADI_SDH_PID3_OFFSET                     0x0FEC  /* SDH Peripheral Identification register 3 offset  */
#define ADI_SDH_PID4_OFFSET                     0x0FF0  /* SDH Peripheral Identification register 4 offset  */
#define ADI_SDH_PID5_OFFSET                     0x0FF4  /* SDH Peripheral Identification register 5 offset  */
#define ADI_SDH_PID6_OFFSET                     0x0FF8  /* SDH Peripheral Identification register 6 offset  */
#define ADI_SDH_PID7_OFFSET                     0x0FFC  /* SDH Peripheral Identification register 7 offset  */

#else
#error "Definitions for this target are missing."

#endif                                          /* end of PID register offsets */

/*********************************************************************************

SDH Power Control Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_PWR_CTRL                        (ADI_SDH_REG_BASE + ADI_SDH_PWR_CTRL_REG_OFFSET)
#define pADI_SDH_PWR_CTRL                       ((volatile u16*)ADI_SDH_PWR_CTRL)

/* SDH Power Control - Power ON */
#define ADI_SDH_PWR_CTRL_PWN_ON_FIELD           0x0003
#define ADI_SDH_PWR_CTRL_PWN_ON_ON              (*pADI_SDH_PWR_CTRL |= ADI_SDH_PWR_CTRL_PWN_ON_FIELD)

/* SDH Power Control - Open Drain Output */
#define ADI_SDH_PWR_CTRL_CMD_OD_FIELD           0x0040
#define ADI_SDH_PWR_CTRL_CMD_OD_DISABLE         (*pADI_SDH_PWR_CTRL &= ~ADI_SDH_PWR_CTRL_CMD_OD_FIELD)
#define ADI_SDH_PWR_CTRL_CMD_OD_ENABLE          (*pADI_SDH_PWR_CTRL |= ADI_SDH_PWR_CTRL_CMD_OD_FIELD)

/* SDH Power Control - Rod Control */
#define ADI_SDH_PWR_CTRL_ROD_CTRL_FIELD         0x0080
#define ADI_SDH_PWR_CTRL_ROD_CTRL_DISABLE       (*pADI_SDH_PWR_CTRL &= ~ADI_SDH_PWR_CTRL_ROD_CTRL_FIELD)
#define ADI_SDH_PWR_CTRL_ROD_CTRL_ENABLE        (*pADI_SDH_PWR_CTRL |= ADI_SDH_PWR_CTRL_ROD_CTRL_FIELD)

/* SDH Power Control - Default settings */
#define ADI_SDH_PWR_CTRL_SET_DEFAULT            (*pADI_SDH_PWR_CTRL = 0)

/*********************************************************************************

SDH Clock Control Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_CLK_CTRL                        (ADI_SDH_REG_BASE + ADI_SDH_CLK_CTRL_REG_OFFSET)
#define pADI_SDH_CLK_CTRL                       ((volatile u16*)ADI_SDH_CLK_CTRL)

/* SDH Clock Control - SD Clock Divisor */
#define ADI_SDH_CLK_CTRL_FIELD                  0x00FF
#define ADI_SDH_CLK_CTRL_GET_CLKDIV             (*pADI_SDH_CLK_CTRL & ADI_SDH_CLK_CTRL_FIELD)
#define ADI_SDH_CLK_CTRL_SET_CLKDIV(V)          (*pADI_SDH_CLK_CTRL = ((*pADI_SDH_CLK_CTRL & ~ADI_SDH_CLK_CTRL_FIELD) | (V)))

/* SDH Clock Control - Bus Clock Enable */
#define ADI_SDH_CLK_CTRL_CLK_E_FIELD            0x0100
#define ADI_SDH_CLK_CTRL_CLK_DISABLE            (*pADI_SDH_CLK_CTRL &= ~ADI_SDH_CLK_CTRL_CLK_E_FIELD)
#define ADI_SDH_CLK_CTRL_CLK_ENABLE             (*pADI_SDH_CLK_CTRL |= ADI_SDH_CLK_CTRL_CLK_E_FIELD)

/* SDH Clock Control - Power Save Enable */
#define ADI_SDH_CLK_CTRL_PWR_SV_E_FIELD         0x0200
#define ADI_SDH_CLK_CTRL_PWR_SAVE_BUS_ACTIVE    (*pADI_SDH_CLK_CTRL |= ADI_SDH_CLK_CTRL_PWR_SV_E_FIELD)  /* Enable clock when bus active */
#define ADI_SDH_CLK_CTRL_PWR_SAVE_ALWAYS_ENABLE (*pADI_SDH_CLK_CTRL &= ~ADI_SDH_CLK_CTRL_PWR_SV_E_FIELD) /* Power control always enabled */

/* SDH Clock Control - Enable/Disable Divisor bypass */
#define ADI_SDH_CLK_CTRL_CLKDIV_BYPASS_FIELD    0x0400
#define ADI_SDH_CLK_CTRL_CLKDIV_BYPASS_DISABLE  (*pADI_SDH_CLK_CTRL &= ~ADI_SDH_CLK_CTRL_CLKDIV_BYPASS_FIELD)
#define ADI_SDH_CLK_CTRL_CLKDIV_BYPASS_ENABLE   (*pADI_SDH_CLK_CTRL |= ADI_SDH_CLK_CTRL_CLKDIV_BYPASS_FIELD)

/* SDH Clock Control - Enable/Disable Wide Bus Mode */
#define ADI_SDH_CLK_CTRL_WIDE_BUS_FIELD         0x0800
#define ADI_SDH_CLK_CTRL_WIDE_BUS_DISABLE       (*pADI_SDH_CLK_CTRL &= ~ADI_SDH_CLK_CTRL_WIDE_BUS_FIELD)
#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
#define ADI_SDH_CLK_CTRL_WIDE_BUS_ENABLE        (*pADI_SDH_CLK_CTRL = (*pADI_SDH_CLK_CTRL &~(ADI_SDH_CLK_CTRL_BYTE_BUS_FIELD|ADI_SDH_CLK_CTRL_WIDE_BUS_FIELD)) | ADI_SDH_CLK_CTRL_WIDE_BUS_FIELD)
#elif defined(__ADSP_MOAB__)
#define ADI_SDH_CLK_CTRL_WIDE_BUS_ENABLE        (*pADI_SDH_CLK_CTRL |= ADI_SDH_CLK_CTRL_WIDE_BUS_FIELD)
#else
#error "Definitions for this target are missing."
#endif
#define ADI_SDH_CLK_CTRL_IS_WIDE_BUS_ENABLED    (*pADI_SDH_CLK_CTRL & ADI_SDH_CLK_CTRL_WIDE_BUS_FIELD)   /* for port control - checks if wide bus mode is enabled or not */

#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
/* SDH Clock Control - Enable/Disable Byte Bus Mode */
#define ADI_SDH_CLK_CTRL_BYTE_BUS_FIELD         0x1000
#define ADI_SDH_CLK_CTRL_BYTE_BUS_DISABLE       (*pADI_SDH_CLK_CTRL &= ~ADI_SDH_CLK_CTRL_BYTE_BUS_FIELD)
// the following sets both BusMode bits, so need to clear wide mode bit as well, so we end up with '10'
#define ADI_SDH_CLK_CTRL_BYTE_BUS_ENABLE        (*pADI_SDH_CLK_CTRL = (*pADI_SDH_CLK_CTRL &~(ADI_SDH_CLK_CTRL_BYTE_BUS_FIELD|ADI_SDH_CLK_CTRL_WIDE_BUS_FIELD)) | ADI_SDH_CLK_CTRL_BYTE_BUS_FIELD)
#define ADI_SDH_CLK_CTRL_IS_BYTE_BUS_ENABLED    (*pADI_SDH_CLK_CTRL & ADI_SDH_CLK_CTRL_BYTE_BUS_FIELD)   /* for port control - checks if byte bus mode is enabled or not */
#endif

/* SDH Clock Control - Default settings (Disable SD CLK, disable SD CLK bypass, standard (1-bit) bus mode */
#define ADI_SDH_CLK_CTRL_SET_DEFAULT            (*pADI_SDH_CLK_CTRL = 0)

/*********************************************************************************

SDH Argument Register MMR

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_ARGUMENT                        (ADI_SDH_REG_BASE + ADI_SDH_ARGUMENT_REG_OFFSET)
#define pADI_SDH_ARGUMENT                       ((volatile u32*)ADI_SDH_ARGUMENT)

/* SDH Argument - set value */
#define ADI_SDH_ARGUMENT_SET_VALUE(V)           (*pADI_SDH_ARGUMENT = (V))

/*********************************************************************************

SDH Command Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_COMMAND                         (ADI_SDH_REG_BASE + ADI_SDH_COMMAND_REG_OFFSET)
#define pADI_SDH_COMMAND                        ((volatile u16*)ADI_SDH_COMMAND)

/* SDH Command - SD Command Index */
#define ADI_SDH_COMMAND_CMD_IDX_FIELD           0x003F
#define ADI_SDH_COMMAND_INDEX_SET(V)            (*pADI_SDH_COMMAND = ((*pADI_SDH_COMMAND & ~ADI_SDH_COMMAND_CMD_IDX_FIELD) | (V)))

/* SDH Command - Command Response */
#define ADI_SDH_COMMAND_CMD_RSP_FIELD           0x0040
#define ADI_SDH_COMMAND_NO_RESPONSE             (*pADI_SDH_COMMAND |= ADI_SDH_COMMAND_CMD_RSP_FIELD)     /* No Response, use CmdSend Flag    */
#define ADI_SDH_COMMAND_WAIT_FOR_RESPONSE       (*pADI_SDH_COMMAND &= ~ADI_SDH_COMMAND_CMD_RSP_FIELD)    /* Waits for response               */

/* SDH Command - Command Long/Short Response */
#define ADI_SDH_COMMAND_CMD_L_RSP_FIELD         0x0080
#define ADI_SDH_COMMAND_SHORT_RSP               (*pADI_SDH_COMMAND &= ~ADI_SDH_COMMAND_CMD_RSP_FIELD)    /* command receives a short response    */
#define ADI_SDH_COMMAND_LONG_RSP                (*pADI_SDH_COMMAND |= ADI_SDH_COMMAND_CMD_RSP_FIELD)     /* command receives a long response     */

/* SDH Command - Command Interrupt Enable */
#define ADI_SDH_COMMAND_CMD_INT_E_FIELD         0x0100
#define ADI_SDH_COMMAND_USE_INTERRUPT           (*pADI_SDH_COMMAND |= ADI_SDH_COMMAND_CMD_INT_E_FIELD)   /* Don't wait for interrupt, use command timer          */
#define ADI_SDH_COMMAND_USE_TIMER               (*pADI_SDH_COMMAND &= ~ADI_SDH_COMMAND_CMD_INT_E_FIELD)  /* Disable Command Timer, wait for interrupt request    */

/* SDH Command - Command Pending */
#define ADI_SDH_COMMAND_CMD_PEND_E_FIELD        0x0200
#define ADI_SDH_COMMAND_SEND_IMMEDIATELY        (*pADI_SDH_COMMAND &= ~ADI_SDH_COMMAND_CMD_RSP_FIELD)    /* wait for CmdPend flag before sending a command */
#define ADI_SDH_COMMAND_WAIT_FOR_CMDPEND        (*pADI_SDH_COMMAND |= ADI_SDH_COMMAND_CMD_PEND_E_FIELD)  /* Send command immediately */

/* SDH Command - Command Long/Short Response */
#define ADI_SDH_COMMAND_CMD_E_FIELD             0x0400
#define ADI_SDH_COMMAND_DISABLE                 (*pADI_SDH_COMMAND &= ~ADI_SDH_COMMAND_CMD_E_FIELD)      /* Disable commands */
#define ADI_SDH_COMMAND_ENABLE                  (*pADI_SDH_COMMAND |= ADI_SDH_COMMAND_CMD_E_FIELD)       /* Enable commands  */

/* SDH Command - SD Command Index */
#define ADI_SDH_COMMAND_CMD_IDX_FIELD           0x003F
#define ADI_SDH_COMMAND_INDEX_SET(V)            (*pADI_SDH_COMMAND = ((*pADI_SDH_COMMAND & ~ADI_SDH_COMMAND_CMD_IDX_FIELD) | (V)))

/* SDH Command - Default settings (disable commands) */
#define ADI_SDH_COMMAND_SET_DEFAULT             (*pADI_SDH_COMMAND = 0)

/* SDH Command - set value */
#define ADI_SDH_COMMAND_SET_VALUE(V)            (*pADI_SDH_COMMAND = (V))

/*********************************************************************************

SDH Response Command Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_RESPONSE_COMMAND                (ADI_SDH_REG_BASE + ADI_SDH_RESPONSE_COMMAND_REG_OFFSET)
#define pADI_SDH_RESPONSE_COMMAND               ((volatile u16*)ADI_SDH_RESPONSE_COMMAND)

/* SDH Response Command - Response Command Value */
#define ADI_SDH_RESPONSE_COMMAND_FIELD          0x003F
#define ADI_SDH_RESPONSE_COMMAND_SET(V)         (*pADI_SDH_RESPONSE_COMMAND = ((*pADI_SDH_RESPONSE_COMMAND & ~ADI_SDH_RESPONSE_COMMAND_FIELD) | (V)))

/*********************************************************************************

SDH Response 0 Register MMR

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_RESPONSE_0                      (ADI_SDH_REG_BASE + ADI_SDH_RESPONSE0_REG_OFFSET)
#define pADI_SDH_RESPONSE_0                     ((volatile u32*)ADI_SDH_RESPONSE_0)

/*********************************************************************************

SDH Response 1 Register MMR

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_RESPONSE_1                      (ADI_SDH_REG_BASE + ADI_SDH_RESPONSE1_REG_OFFSET)
#define pADI_SDH_RESPONSE_1                     ((volatile u32*)ADI_SDH_RESPONSE_1)

/*********************************************************************************

SDH Response 2 Register MMR

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_RESPONSE_2                      (ADI_SDH_REG_BASE + ADI_SDH_RESPONSE2_REG_OFFSET)
#define pADI_SDH_RESPONSE_2                     ((volatile u32*)ADI_SDH_RESPONSE_2)

/*********************************************************************************

SDH Response 3 Register MMR

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_RESPONSE_3                      (ADI_SDH_REG_BASE + ADI_SDH_RESPONSE3_REG_OFFSET)
#define pADI_SDH_RESPONSE_3                     ((volatile u32*)ADI_SDH_RESPONSE_3)

/*********************************************************************************

SDH Data Timer Register MMR

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_DATA_TIMER                      (ADI_SDH_REG_BASE + ADI_SDH_DATA_TIMER_REG_OFFSET)
#define pADI_SDH_DATA_TIMER                     ((volatile u32*)ADI_SDH_DATA_TIMER)

#define ADI_SDH_DATA_TIMER_SET(V)               (*pADI_SDH_DATA_TIMER = (V))

/*********************************************************************************

SDH Data Length Register MMR

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_DATA_LENGTH                     (ADI_SDH_REG_BASE + ADI_SDH_DATA_LGTH_REG_OFFSET)
#define pADI_SDH_DATA_LENGTH                    ((volatile u16*)ADI_SDH_DATA_LENGTH)

#define ADI_SDH_DATA_LENGTH_SET(V)              (*pADI_SDH_DATA_LENGTH = (V))

/*********************************************************************************

SDH Data Control Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_DATA_CTRL                       (ADI_SDH_REG_BASE + ADI_SDH_DATA_CTRL_REG_OFFSET)
#define pADI_SDH_DATA_CTRL                      ((volatile u16*)ADI_SDH_DATA_CTRL)

/* SDH Data Control - Enable/Disable Data Transfer */
#define ADI_SDH_DATA_CTRL_DTX_E_FIELD           0x0001
#define ADI_SDH_DATA_CTRL_DATA_TX_DISABLE       (*pADI_SDH_DATA_CTRL &= ~ADI_SDH_DATA_CTRL_DTX_E_FIELD)
#define ADI_SDH_DATA_CTRL_DATA_TX_ENABLE        (*pADI_SDH_DATA_CTRL |= ADI_SDH_DATA_CTRL_DTX_E_FIELD)

/* SDH Data Control - Data Transfer Direction */
#define ADI_SDH_DATA_CTRL_DTX_DIR_FIELD         0x0002
#define ADI_SDH_DATA_CTRL_DATA_TO_CARD          (*pADI_SDH_DATA_CTRL &= ~ADI_SDH_DATA_CTRL_DTX_DIR_FIELD)    /* Data Transfer - From Controller to Card  */
#define ADI_SDH_DATA_CTRL_DATA_FROM_CARD        (*pADI_SDH_DATA_CTRL |= ADI_SDH_DATA_CTRL_DTX_DIR_FIELD)     /* Data Transfer - From Card to Controller  */

/* SDH Data Control - Data Transfer Mode */
#define ADI_SDH_DATA_CTRL_DTX_MODE_FIELD         0x0004
#define ADI_SDH_DATA_CTRL_DATA_BLOCK_TX         (*pADI_SDH_DATA_CTRL &= ~ADI_SDH_DATA_CTRL_DTX_MODE_FIELD)   /* Block Transfer mode  */
#define ADI_SDH_DATA_CTRL_DATA_STREAM_TX        (*pADI_SDH_DATA_CTRL |= ADI_SDH_DATA_CTRL_DTX_MODE_FIELD)    /* Stream Transfer Mode */

/* SDH Data Control - Data Transfer DMA Enable/Disable */
#define ADI_SDH_DATA_CTRL_DTX_DMA_E_FIELD       0x0008
#define ADI_SDH_DATA_CTRL_DMA_DISABLE           (*pADI_SDH_DATA_CTRL &= ~ADI_SDH_DATA_CTRL_DTX_DMA_E_FIELD)
#define ADI_SDH_DATA_CTRL_DMA_ENABLE            (*pADI_SDH_DATA_CTRL |= ADI_SDH_DATA_CTRL_DTX_DMA_E_FIELD)

/* SDH Data Control - Data Transfer Block Length */
#define ADI_SDH_DATA_CTRL_DTX_BLK_LGTH_FIELD    0x00F0
#define ADI_SDH_DATA_CTRL_DTX_BLK_LGTH_SHIFT    4
#define ADI_SDH_DATA_CTRL_DTX_BLK_LGTH_SET(V)   (*pADI_SDH_DATA_CTRL = ((*pADI_SDH_DATA_CTRL & ~ADI_SDH_DATA_CTRL_DTX_BLK_LGTH_FIELD) | ((V) << ADI_SDH_DATA_CTRL_DTX_BLK_LGTH_SHIFT)))

/* SDH Data Control - Default settings */
#define ADI_SDH_DATA_CTRL_SET_DEFAULT           (*pADI_SDH_DATA_CTRL = 0)

/* SDH Data Control - set value */
#define ADI_SDH_DATA_CTRL_SET_VALUE(V)          (*pADI_SDH_DATA_CTRL = (V))

/*********************************************************************************

SDH Status Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_STATUS                          (ADI_SDH_REG_BASE + ADI_SDH_STATUS_REG_OFFSET)
#define pADI_SDH_STATUS                         ((volatile u32*)ADI_SDH_STATUS)

/* SDH Status - Command CRC Fail (0 - No CRC received on Command Response, 1 - CRC Fail on Command Response)*/
#define ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD       0x00000001
#define ADI_SDH_STATUS_CMD_CRC_FAIL             (*pADI_SDH_STATUS & ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD)

/* SDH Status - Data CRC Fail (0 - No CRC received on Data block Tx/Rx, 1 - CRC Fail on Data block Tx/Rx)   */
#define ADI_SDH_STATUS_DATA_CRC_FAIL_FIELD      0x00000002
#define ADI_SDH_STATUS_DATA_CRC_FAIL            (*pADI_SDH_STATUS & ADI_SDH_STATUS_DATA_CRC_FAIL_FIELD)

/* SDH Status - Command Time Out (0 - Command response not timed out, 1 - Command response time out)        */
#define ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD   0x00000004
#define ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT         (*pADI_SDH_STATUS & ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD)

/* SDH Status - Data Time Out (0 - Data not timed out, 1 - Data time out)                                   */
#define ADI_SDH_STATUS_DATA_TIMEOUT_FIELD        0x00000008
#define ADI_SDH_STATUS_DATA_TIMEOUT             (*pADI_SDH_STATUS & ADI_SDH_STATUS_DATA_TIMEOUT_FIELD)

/* SDH Status - Transmit FIFO underrun (0 - No error, 1 - Tx. FIFO under run error )                        */
#define ADI_SDH_STATUS_TX_FIFO_UNDERRUN_FIELD   0x00000010
#define ADI_SDH_STATUS_TX_FIFO_UNDERRUN         (*pADI_SDH_STATUS & ADI_SDH_STATUS_TX_FIFO_UNDERRUN_FIELD)

/* SDH Status - Receive FIFO Overrun (0 - No error, 1 - Rx. FIFO Over run error )                           */
#define ADI_SDH_STATUS_RX_FIFO_OVERRUN_FIELD    0x00000020
#define ADI_SDH_STATUS_RX_FIFO_OVERRUN          (*pADI_SDH_STATUS & ADI_SDH_STATUS_RX_FIFO_OVERRUN_FIELD)

/* SDH Status - Command Response End (0 - No response recived, 1 - Command response received & CRC passed)  */
#define ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD   0x00000040
#define ADI_SDH_STATUS_CMD_RESPONSE_END         (*pADI_SDH_STATUS & ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD)

/* SDH Status - Command Sent (0 - No Command sent, 1 - Command sent)                                        */
#define ADI_SDH_STATUS_CMD_SENT_FIELD           0x00000080
#define ADI_SDH_STATUS_CMD_SENT                 (*pADI_SDH_STATUS & ADI_SDH_STATUS_CMD_SENT_FIELD)

/* SDH Status - Data End (0 - No Data End, 1 - Data end (Data counter is zero))                             */
#define ADI_SDH_STATUS_DATA_END_FIELD           0x00000100
#define ADI_SDH_STATUS_DATA_END                 (*pADI_SDH_STATUS & ADI_SDH_STATUS_DATA_END_FIELD)

/* SDH Status - Start Bit Error (0 - No error, 1 - Start bit error (missing from a signal))                 */
#define ADI_SDH_STATUS_START_BIT_ERROR_FIELD    0x00000200
#define ADI_SDH_STATUS_START_BIT_ERROR          (*pADI_SDH_STATUS & ADI_SDH_STATUS_START_BIT_ERROR_FIELD)

/* SDH Status - Data Block End (0 - No End Detected, 1 - Data block Tx/Rx end (CRC passed))                 */
#define ADI_SDH_STATUS_DATA_BLOCK_END_FIELD     0x00000400
#define ADI_SDH_STATUS_DATA_BLOCK_END           (*pADI_SDH_STATUS & ADI_SDH_STATUS_DATA_BLOCK_END_FIELD)

/* SDH Status - Command Active (0 - No Command in progress, 1 - Command Transfer in progress)               */
#define ADI_SDH_STATUS_CMD_ACTIVE_FIELD         0x00000800
#define ADI_SDH_STATUS_IS_CMD_ACTIVE            (*pADI_SDH_STATUS & ADI_SDH_STATUS_CMD_ACTIVE_FIELD)

/* SDH Status - Transmit Active (0 - No transmit in progress, 1 - Data Transmit in progress)                */
#define ADI_SDH_STATUS_TX_ACTIVE_FIELD          0x00001000
#define ADI_SDH_STATUS_IS_TX_ACTIVE            (*pADI_SDH_STATUS & ADI_SDH_STATUS_TX_ACTIVE_FIELD)

/* SDH Status - Receive Active (0 - No receive in progress, 1 - Data Receive in progress)                   */
#define ADI_SDH_STATUS_RX_ACTIVE_FIELD          0x00002000
#define ADI_SDH_STATUS_IS_RX_ACTIVE             (*pADI_SDH_STATUS & ADI_SDH_STATUS_RX_ACTIVE_FIELD)

/* SDH Status - Tx FIFO Status (0 - No FIFO watermark detected, 1 - Tx FIFO Half empty)                     */
#define ADI_SDH_STATUS_TX_FIFO_STATUS_FIELD     0x00004000
#define ADI_SDH_STATUS_TX_FIFO_HALF_EMPTY       (*pADI_SDH_STATUS & ADI_SDH_STATUS_TX_FIFO_STATUS_FIELD)

/* SDH Status - Rx FIFO Status (0 - No FIFO watermark detected, 1 - Rx FIFO Half Full)                      */
#define ADI_SDH_STATUS_RX_FIFO_STATUS_FIELD     0x00008000
#define ADI_SDH_STATUS_RX_FIFO_HALF_FULL        (*pADI_SDH_STATUS & ADI_SDH_STATUS_RX_FIFO_STATUS_FIELD)

/* SDH Status - Tx FIFO Full (0 - Not Full, 1 - Tx FIFO Full)                                               */
#define ADI_SDH_STATUS_TX_FIFO_FULL_FIELD       0x00010000
#define ADI_SDH_STATUS_TX_FIFO_FULL             (*pADI_SDH_STATUS & ADI_SDH_STATUS_TX_FIFO_FULL_FIELD)

/* SDH Status - Rx FIFO Full (0 - Not Full, 1 - Rx FIFO Full)                                               */
#define ADI_SDH_STATUS_RX_FIFO_FULL_FIELD       0x00020000
#define ADI_SDH_STATUS_RX_FIFO_FULL             (*pADI_SDH_STATUS & ADI_SDH_STATUS_RX_FIFO_FULL_FIELD)

/* SDH Status - Tx FIFO Empty (0 - Not Empty, 1 - Tx FIFO Empty)                                            */
#define ADI_SDH_STATUS_TX_FIFO_EMPTY_FIELD      0x00040000
#define ADI_SDH_STATUS_TX_FIFO_EMPTY            (*pADI_SDH_STATUS & ADI_SDH_STATUS_TX_FIFO_EMPTY_FIELD)

/* SDH Status - Rx FIFO Empty (0 - Not Empty, 1 - Rx FIFO Empty)                                            */
#define ADI_SDH_STATUS_RX_FIFO_EMPTY_FIELD      0x00080000
#define ADI_SDH_STATUS_RX_FIFO_EMPTY            (*pADI_SDH_STATUS & ADI_SDH_STATUS_RX_FIFO_EMPTY_FIELD)

/* SDH Status - Tx Data Available (0 - No Data, 1 - Data Available in Tx FIFO)                              */
#define ADI_SDH_STATUS_TX_DATA_RDY_FIELD        0x00100000
#define ADI_SDH_STATUS_TX_DATA_RDY              (*pADI_SDH_STATUS & ADI_SDH_STATUS_TX_DATA_RDY_FIELD)

/* SDH Status - Rx Data Available (0 - No Data, 1 - Data Available in Rx FIFO)                              */
#define ADI_SDH_STATUS_RX_FIFO_RDY_FIELD        0x00200000
#define ADI_SDH_STATUS_RX_FIFO_RDY              (*pADI_SDH_STATUS & ADI_SDH_STATUS_RX_FIFO_RDY_FIELD)

/* Check error status bits related to Data transfer mode */
#define ADI_SDH_STATUS_IS_DATA_TX_ERROR         (*pADI_SDH_STATUS & (ADI_SDH_STATUS_DATA_CRC_FAIL_FIELD     | \
                                                                     ADI_SDH_STATUS_DATA_TIMEOUT_FIELD      | \
                                                                     ADI_SDH_STATUS_TX_FIFO_UNDERRUN_FIELD  | \
                                                                     ADI_SDH_STATUS_RX_FIFO_OVERRUN_FIELD   | \
                                                                     ADI_SDH_STATUS_START_BIT_ERROR_FIELD))


/*********************************************************************************

SDH Status Clear Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_STATUS_CLEAR                        (ADI_SDH_REG_BASE + ADI_SDH_STATUS_CLR_REG_OFFSET)
#define pADI_SDH_STATUS_CLEAR                       ((volatile u16*)ADI_SDH_STATUS_CLEAR)

/* SDH Status Clear - Clear Command CRC Fail status (write 1 to clear)                  */
#define ADI_SDH_STATUS_CLEAR_CMD_CRC_FAIL           (*pADI_SDH_STATUS_CLEAR = 0x0001)

/* SDH Status Clear - Clear Data CRC Fail status (write 1 to clear)                     */
#define ADI_SDH_STATUS_CLEAR_DATA_CRC_FAIL          (*pADI_SDH_STATUS_CLEAR = 0x0002)

/* SDH Status Clear - Clear Command Time Out status (write 1 to clear)                  */
#define ADI_SDH_STATUS_CLEAR_CMD_RESPONSE_TIMEOUT   (*pADI_SDH_STATUS_CLEAR = 0x0004)

/* SDH Status Clear - Clear Data Time Out status (write 1 to clear)                     */
#define ADI_SDH_STATUS_CLEAR_DATA_TIMEOUT           (*pADI_SDH_STATUS_CLEAR = 0x0008)

/* SDH Status Clear - Clear Transmit FIFO underrun status (write 1 to clear)            */
#define ADI_SDH_STATUS_CLEAR_TX_FIFO_UNDERRUN       (*pADI_SDH_STATUS_CLEAR = 0x0010)

/* SDH Status Clear - Clear Receive FIFO Overrun status (write 1 to clear)              */
#define ADI_SDH_STATUS_CLEAR_RX_FIFO_OVERRUN        (*pADI_SDH_STATUS_CLEAR = 0x0020)

/* SDH Status Clear - Clear Command Response End status (write 1 to clear)              */
#define ADI_SDH_STATUS_CLEAR_CMD_RESPONSE_END       (*pADI_SDH_STATUS_CLEAR = 0x0040)

/* SDH Status Clear - Clear Command Sent status (write 1 to clear)                      */
#define ADI_SDH_STATUS_CLEAR_CMD_SENT               (*pADI_SDH_STATUS_CLEAR = 0x0080)

/* SDH Status Clear - Clear Data End status (write 1 to clear)                          */
#define ADI_SDH_STATUS_CLEAR_DATA_END               (*pADI_SDH_STATUS_CLEAR = 0x0100)

/* SDH Status Clear - Clear Srart bit error status (write 1 to clear)                   */
#define ADI_SDH_STATUS_CLEAR_START_BIT_ERROR        (*pADI_SDH_STATUS_CLEAR = 0x0200)

/* SDH Status Clear - Clear Data Block End status (write 1 to clear)                    */
#define ADI_SDH_STATUS_CLEAR_DATA_BLOCK_END         (*pADI_SDH_STATUS_CLEAR = 0x0400)

/* SDH Status Clear - Clear all data transfer error status bits                         */
#define ADI_SDH_STATUS_CLEAR_DATA_TX_ERROR          (*pADI_SDH_STATUS_CLEAR = 0x023A)

/* SDH Status Clear - Clear all status bits                                             */
#define ADI_SDH_STATUS_CLEAR_ALL                    (*pADI_SDH_STATUS_CLEAR = 0x07FF)

/*********************************************************************************

SDH Interrupt Mask 0 Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_INT_MASK0                           (ADI_SDH_REG_BASE + ADI_SDH_INT_MASK0_REG_OFFSET)
#define pADI_SDH_INT_MASK0                          ((volatile u32*)ADI_SDH_INT_MASK0)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Command CRC Fail status bit */
#define ADI_SDH_INT_MASK0_CMD_CRC_FAIL_FIELD        0x00000001
#define ADI_SDH_INT_MASK0_CMD_CRC_FAIL_DISABLE      (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_CMD_CRC_FAIL_FIELD)     /* Disable interrupt    */
#define ADI_SDH_INT_MASK0_CMD_CRC_FAIL_ENABLE       (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_CMD_CRC_FAIL_FIELD)      /* Enable interrupt    */

/* SDH Interrupt Mask - Enable/Disable Interrupt for Data CRC Fail status bit */
#define ADI_SDH_INT_MASK0_DATA_CRC_FAIL_FIELD       0x00000002
#define ADI_SDH_INT_MASK0_DATA_CRC_FAIL_DISABLE     (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_DATA_CRC_FAIL_FIELD)
#define ADI_SDH_INT_MASK0_DATA_CRC_FAIL_ENABLE      (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_DATA_CRC_FAIL_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Command Time out status bit */
#define ADI_SDH_INT_MASK0_CMD_TIME_OUT_FIELD        0x00000004
#define ADI_SDH_INT_MASK0_CMD_TIME_OUT_DISABLE      (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_CMD_TIME_OUT_FIELD)
#define ADI_SDH_INT_MASK0_CMD_TIME_OUT_ENABLE       (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_CMD_TIME_OUT_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Data Time out status bit */
#define ADI_SDH_INT_MASK0_DATA_TIME_OUT_FIELD       0x00000008
#define ADI_SDH_INT_MASK0_DATA_TIME_OUT_DISABLE     (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_DATA_TIME_OUT_FIELD)
#define ADI_SDH_INT_MASK0_DATA_TIME_OUT_ENABLE      (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_DATA_TIME_OUT_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Transmit Underrun status bit */
#define ADI_SDH_INT_MASK0_TX_UNDERRUN_FIELD         0x00000010
#define ADI_SDH_INT_MASK0_TX_UNDERRUN_DISABLE       (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_TX_UNDERRUN_FIELD)
#define ADI_SDH_INT_MASK0_TX_UNDERRUN_ENABLE        (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_TX_UNDERRUN_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Receive Overrun status bit */
#define ADI_SDH_INT_MASK0_RX_OVERRUN_FIELD          0x00000020
#define ADI_SDH_INT_MASK0_RX_OVERRUN_DISABLE        (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_RX_OVERRUN_FIELD)
#define ADI_SDH_INT_MASK0_RX_OVERRUN_ENABLE         (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_TX_UNDERRUN_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Command Response End status bit */
#define ADI_SDH_INT_MASK0_CMD_RESPONSE_END_FIELD    0x00000040
#define ADI_SDH_INT_MASK0_CMD_RESPONSE_END_DISABLE  (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_CMD_RESPONSE_END_FIELD)
#define ADI_SDH_INT_MASK0_CMD_RESPONSE_END_ENABLE   (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_CMD_RESPONSE_END_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Command Sent status bit */
#define ADI_SDH_INT_MASK0_CMD_SENT_FIELD            0x00000080
#define ADI_SDH_INT_MASK0_CMD_SENT_DISABLE          (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_CMD_SENT_FIELD)
#define ADI_SDH_INT_MASK0_CMD_SENT_ENABLE           (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_CMD_SENT_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Data End status bit */
#define ADI_SDH_INT_MASK0_DATA_END_FIELD            0x00000100
#define ADI_SDH_INT_MASK0_DATA_END_DISABLE          (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_DATA_END_FIELD)
#define ADI_SDH_INT_MASK0_DATA_END_ENABLE           (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_DATA_END_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Start bit error status bit */
#define ADI_SDH_INT_MASK0_START_BIT_ERROR_FIELD     0x00000200
#define ADI_SDH_INT_MASK0_START_BIT_ERROR_DISABLE   (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_START_BIT_ERROR_FIELD)
#define ADI_SDH_INT_MASK0_START_BIT_ERROR_ENABLE    (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_START_BIT_ERROR_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Data block end status bit */
#define ADI_SDH_INT_MASK0_DATA_BLOCK_END_FIELD      0x00000400
#define ADI_SDH_INT_MASK0_DATA_BLOCK_END_DISABLE    (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_DATA_BLOCK_END_FIELD)
#define ADI_SDH_INT_MASK0_DATA_BLOCK_END_ENABLE     (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_DATA_BLOCK_END_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Command Active status bit */
#define ADI_SDH_INT_MASK0_CMD_ACTIVE_FIELD          0x00000800
#define ADI_SDH_INT_MASK0_CMD_ACTIVE_DISABLE        (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_CMD_ACTIVE_FIELD)
#define ADI_SDH_INT_MASK0_CMD_ACTIVE_ENABLE         (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_CMD_ACTIVE_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Transmit Active status bit */
#define ADI_SDH_INT_MASK0_TX_ACTIVE_FIELD           0x00001000
#define ADI_SDH_INT_MASK0_TX_ACTIVE_DISABLE         (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_TX_ACTIVE_FIELD)
#define ADI_SDH_INT_MASK0_TX_ACTIVE_ENABLE          (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_TX_ACTIVE_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Receive Active status bit */
#define ADI_SDH_INT_MASK0_RX_ACTIVE_FIELD           0x00002000
#define ADI_SDH_INT_MASK0_RX_ACTIVE_DISABLE         (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_RX_ACTIVE_FIELD)
#define ADI_SDH_INT_MASK0_RX_ACTIVE_ENABLE          (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_RX_ACTIVE_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Transmit FIFO watermark status bit */
#define ADI_SDH_INT_MASK0_TX_FIFO_STATUS_FIELD      0x00004000
#define ADI_SDH_INT_MASK0_TX_FIFO_STATUS_DISABLE    (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_TX_FIFO_STATUS_FIELD)
#define ADI_SDH_INT_MASK0_TX_FIFO_STATUS_ENABLE     (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_TX_FIFO_STATUS_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Receive FIFO watermark status bit */
#define ADI_SDH_INT_MASK0_RX_FIFO_STATUS_FIELD      0x00008000
#define ADI_SDH_INT_MASK0_RX_FIFO_STATUS_DISABLE    (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_RX_FIFO_STATUS_FIELD)
#define ADI_SDH_INT_MASK0_RX_FIFO_STATUS_ENABLE     (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_RX_FIFO_STATUS_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Transmit FIFO Full status bit */
#define ADI_SDH_INT_MASK0_TX_FIFO_FULL_FIELD        0x00010000
#define ADI_SDH_INT_MASK0_TX_FIFO_FULL_DISABLE      (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_TX_FIFO_FULL_FIELD)
#define ADI_SDH_INT_MASK0_TX_FIFO_FULL_ENABLE       (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_TX_FIFO_FULL_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Receive FIFO Full status bit */
#define ADI_SDH_INT_MASK0_RX_FIFO_FULL_FIELD        0x00020000
#define ADI_SDH_INT_MASK0_RX_FIFO_FULL_DISABLE      (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_RX_FIFO_FULL_FIELD)
#define ADI_SDH_INT_MASK0_RX_FIFO_FULL_ENABLE       (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_RX_FIFO_FULL_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Transmit FIFO Empty status bit */
#define ADI_SDH_INT_MASK0_TX_FIFO_EMPTY_FIELD       0x00040000
#define ADI_SDH_INT_MASK0_TX_FIFO_EMPTY_DISABLE     (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_TX_FIFO_EMPTY_FIELD)
#define ADI_SDH_INT_MASK0_TX_FIFO_EMPTY_ENABLE      (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_TX_FIFO_EMPTY_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Receive FIFO Empty status bit */
#define ADI_SDH_INT_MASK0_RX_FIFO_EMPTY_FIELD       0x00080000
#define ADI_SDH_INT_MASK0_RX_FIFO_EMPTY_DISABLE     (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_RX_FIFO_EMPTY_FIELD)
#define ADI_SDH_INT_MASK0_RX_FIFO_EMPTY_ENABLE      (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_RX_FIFO_EMPTY_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Transmit Data Available status bit */
#define ADI_SDH_INT_MASK0_TX_DATA_RDY_FIELD         0x00100000
#define ADI_SDH_INT_MASK0_TX_DATA_RDY_DISABLE       (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_TX_DATA_RDY_FIELD)
#define ADI_SDH_INT_MASK0_TX_DATA_RDY_ENABLE        (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_TX_DATA_RDY_FIELD)

/* SDH Interrupt Mask - Enable/Disable Interrupt for Receive Data Available status bit */
#define ADI_SDH_INT_MASK0_RX_DATA_RDY_FIELD         0x00200000
#define ADI_SDH_INT_MASK0_RX_DATA_RDY_DISABLE       (*pADI_SDH_INT_MASK0 &= ~ADI_SDH_INT_MASK0_RX_DATA_RDY_FIELD)
#define ADI_SDH_INT_MASK0_RX_DATA_RDY_ENABLE        (*pADI_SDH_INT_MASK0 |= ADI_SDH_INT_MASK0_RX_DATA_RDY_FIELD)

#define ADI_SDH_INT_MASK0_ALL                       (*pADI_SDH_INT_MASK0 = 0)   /* Masks all status interrupts  */

/* Enable Data transfer mode status bit Interrupts */
#define ADI_SDH_INT_MASK0_ENABLE_DATA_TX_INT        (*pADI_SDH_INT_MASK0 |= ( ADI_SDH_INT_MASK0_DATA_CRC_FAIL_FIELD   | \
                                                                              ADI_SDH_INT_MASK0_DATA_TIME_OUT_FIELD   | \
                                                                              ADI_SDH_INT_MASK0_TX_UNDERRUN_FIELD     | \
                                                                              ADI_SDH_INT_MASK0_RX_OVERRUN_FIELD      | \
                                                                              ADI_SDH_INT_MASK0_START_BIT_ERROR_FIELD | \
                                                                              ADI_SDH_INT_MASK0_DATA_END_FIELD))

/* Disable Data transfer mode status bit Interrupts */
#define ADI_SDH_INT_MASK0_DISABLE_DATA_TX_INT       (*pADI_SDH_INT_MASK0 &= ~(ADI_SDH_INT_MASK0_DATA_CRC_FAIL_FIELD   | \
                                                                              ADI_SDH_INT_MASK0_DATA_TIME_OUT_FIELD   | \
                                                                              ADI_SDH_INT_MASK0_TX_UNDERRUN_FIELD     | \
                                                                              ADI_SDH_INT_MASK0_RX_OVERRUN_FIELD      | \
                                                                              ADI_SDH_INT_MASK0_START_BIT_ERROR_FIELD | \
                                                                              ADI_SDH_INT_MASK0_DATA_END_FIELD))

/*********************************************************************************

SDH Interrupt Mask 1 Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_INT_MASK1                           (ADI_SDH_REG_BASE + ADI_SDH_INT_MASK1_REG_OFFSET)
#define pADI_SDH_INT_MASK1                          ((volatile u32*)ADI_SDH_INT_MASK1)

/*********************************************************************************

SDH FIFO Counter Register MMR

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_FIFO_COUNTER                        (ADI_SDH_REG_BASE + ADI_SDH_FIFO_COUNTER_REG_OFFSET)
#define pADI_SDH_FIFO_COUNTER                       ((volatile u16*)ADI_SDH_FIFO_COUNTER)

#define ADI_SDH_FIFO_COUNTER_GET                    (*pADI_SDH_FIFO_COUNTER & 0x7FFF)

/*********************************************************************************

SDH Data FIFO start Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_FIFO_START                          (ADI_SDH_REG_BASE + ADI_SDH_FIFO_REG_START_OFFSET)
#define pADI_SDH_FIFO_START                         ((volatile u32*)ADI_SDH_FIFO_START)

/*********************************************************************************

SDH Exception Status Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_E_STATUS                            (ADI_SDH_REG_BASE + ADI_SDH_E_STATUS_REG_OFFSET)
#define pADI_SDH_E_STATUS                           ((volatile u16*)ADI_SDH_E_STATUS)

/* SDH Exception Status - SDIO Interrupt Detect (0 - No Interrupt, 1 - Interrupt Detected)  */
#define ADI_SDH_E_STATUS_SDIO_INT_DETECT_FIELD      0x0002
#define ADI_SDH_E_STATUS_IS_SDIO_INT_DETECTED       (*pADI_SDH_E_STATUS & ADI_SDH_E_STATUS_SDIO_INT_DETECT_FIELD)
#define ADI_SDH_E_STATUS_SDIO_INT_CLEAR             (*pADI_SDH_E_STATUS &= ADI_SDH_E_STATUS_SDIO_INT_DETECT_FIELD)   /* clear SDIO Interrupt detect status bit   */

/* SDH Exception Status - SD Card Detect (0 - No Card Detect, 1 - Card Detected)  */
#define ADI_SDH_E_STATUS_SD_CARD_DETECT_FIELD       0x0010
#define ADI_SDH_E_STATUS_IS_SD_CARD_DETECTED        (*pADI_SDH_E_STATUS & ADI_SDH_E_STATUS_SD_CARD_DETECT_FIELD)
#define ADI_SDH_E_STATUS_SD_CARD_DETECT_CLEAR       (*pADI_SDH_E_STATUS &= ADI_SDH_E_STATUS_SD_CARD_DETECT_FIELD)   /* clear SD card detect Interrupt status bit */

/* Clear all exception status bits */
#define ADI_SDH_E_STATUS_CLEAR_ALL                  (*pADI_SDH_E_STATUS = 0x0012)

/*********************************************************************************

SDH Exception Mask Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_E_STATUS_MASK                           (ADI_SDH_REG_BASE + ADI_SDH_E_MASK_REG_OFFSET)
#define pADI_SDH_E_STATUS_MASK                          ((volatile u16*)ADI_SDH_E_STATUS_MASK)

/* SDH Exception Mask - Mask SDIO Interrupt Detect (0 - Mask SDIO interrupt, 1 - Enable SDIO Interrupt)  */
#define ADI_SDH_E_STATUS_MASK_SDIO_INT_DETECT_FIELD     0x0002
#define ADI_SDH_E_STATUS_MASK_SDIO_INT_DETECT_DISABLE   (*pADI_SDH_E_STATUS_MASK &= ~ADI_SDH_E_STATUS_MASK_SDIO_INT_DETECT_FIELD)
#define ADI_SDH_E_STATUS_MASK_SDIO_INT_DETECT_ENABLE    (*pADI_SDH_E_STATUS_MASK |= ADI_SDH_E_STATUS_MASK_SDIO_INT_DETECT_FIELD)

/* SDH Exception Mask - Mask SD Card Detect (0 - Disable SD Card detect interrupt, 1 - Enable SD Card detect Interrupt)  */
#if defined(__ADSP_MOAB__)
#define ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_FIELD      0x0040
#elif defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
#define ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_FIELD      0x0010
#else
#error "Definitions for this target are missing."
#endif
#define ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_DISABLE    (*pADI_SDH_E_STATUS_MASK &= ~ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_FIELD)
#define ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_ENABLE     (*pADI_SDH_E_STATUS_MASK |= ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_FIELD)
#define ADI_SDH_E_STATUS_IS_SD_CARD_DETECT_ENABLED      (*pADI_SDH_E_STATUS_MASK & ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_FIELD)

/* Mask all exception status interrupt bits */
#define ADI_SDH_E_STATUS_MASK_ALL                       (*pADI_SDH_E_STATUS_MASK = 0)

/*********************************************************************************

SDH Configuration Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_CFG                                 (ADI_SDH_REG_BASE + ADI_SDH_CFG_REG_OFFSET)
#define pADI_SDH_CFG                                ((volatile u16*)ADI_SDH_CFG)

/* SDH Configuration - Enable/Disable SDH Clocks  */
#define ADI_SDH_CFG_CLOCKS_EN_FIELD                 0x0001
#define ADI_SDH_CFG_CLOCKS_EN_DISABLE               (*pADI_SDH_CFG &= ~ADI_SDH_CFG_CLOCKS_EN_FIELD)
#define ADI_SDH_CFG_CLOCKS_EN_ENABLE                (*pADI_SDH_CFG |= ADI_SDH_CFG_CLOCKS_EN_FIELD)

/* SDH Configuration - Enable/Disable SDIO 4-bit  */
#define ADI_SDH_CFG_SDIO_4BIT_FIELD                 0x0004
#define ADI_SDH_CFG_SDIO_4BIT_DISABLE               (*pADI_SDH_CFG &= ~ADI_SDH_CFG_SDIO_4BIT_FIELD)
#define ADI_SDH_CFG_SDIO_4BIT_ENABLE                (*pADI_SDH_CFG |= ADI_SDH_CFG_SDIO_4BIT_FIELD)
#define ADI_SDH_CFG_IS_SDIO_4BIT_ENABLED            (*pADI_SDH_CFG & 0x0004)   /* for port control - checks if SDIO 4Bit is enabled or not */

/* SDH Configuration - Enable/Disable Moving Window  */
#define ADI_SDH_CFG_MOVING_WINDOW_FIELD             0x0008
#define ADI_SDH_CFG_MOVING_WINDOW_DISABLE           (*pADI_SDH_CFG &= ~ADI_SDH_CFG_MOVING_WINDOW_FIELD)
#define ADI_SDH_CFG_MOVING_WINDOW_ENABLE            (*pADI_SDH_CFG |= ADI_SDH_CFG_MOVING_WINDOW_FIELD)

/* SDH Configuration - SDMMC Reset  */
#define ADI_SDH_CFG_SDMMC_RESET_FIELD               0x0010
#define ADI_SDH_CFG_SDMMC_RESET_DISABLE             (*pADI_SDH_CFG &= ~ADI_SDH_CFG_SDMMC_RESET_FIELD)
#define ADI_SDH_CFG_SDMMC_RESET_ENABLE              (*pADI_SDH_CFG |= ADI_SDH_CFG_SDMMC_RESET_FIELD)

/* SDH Configuration - Enable/Disable pull-up on SD DATA 2-0 and SD CMD */
#define ADI_SDH_CFG_PUP_SDCMD_SDDAT2_0_FIELD        0x0020
#define ADI_SDH_CFG_PUP_SDCMD_SDDAT2_0_DISABLE      (*pADI_SDH_CFG &= ~ADI_SDH_CFG_PUP_SDCMD_SDDAT2_0_FIELD)
#define ADI_SDH_CFG_PUP_SDCMD_SDDAT2_0_ENABLE       (*pADI_SDH_CFG |= ADI_SDH_CFG_PUP_SDCMD_SDDAT2_0_FIELD)

/* SDH Configuration - Enable/Disable pull-up on SD DATA 3 */
#define ADI_SDH_CFG_PUP_SDDAT3_FIELD                0x0040
#define ADI_SDH_CFG_PUP_SDDAT3_DISABLE              (*pADI_SDH_CFG &= ~ADI_SDH_CFG_PUP_SDDAT3_FIELD)
#define ADI_SDH_CFG_PUP_SDDAT3_ENABLE               (*pADI_SDH_CFG |= ADI_SDH_CFG_PUP_SDDAT3_FIELD)

/* SDH Configuration - Enable/Disable pull-down on SD DATA 3 */
#define ADI_SDH_CFG_PD_SDDAT3_FIELD                 0x0080
#define ADI_SDH_CFG_PD_SDDAT3_DISABLE               (*pADI_SDH_CFG &= ~ADI_SDH_CFG_PD_SDDAT3_FIELD)
#define ADI_SDH_CFG_PD_SDDAT3_ENABLE                (*pADI_SDH_CFG |= ADI_SDH_CFG_PD_SDDAT3_FIELD)

/* SDH Configuration - Default settings (Disable SDH Clocks, Disable SDIO 4-bit, Disable pull-up/pulldown */
#define ADI_SDH_CFG_SET_DEFAULT                     (*pADI_SDH_CFG = 0)

/* SDH Configuration - set register value */
#define ADI_SDH_CFG_SET_VALUE(V)                     (*pADI_SDH_CFG = (V))

/*********************************************************************************

SDH Wait Enable Register MMR and bitfield manipulation

*********************************************************************************/

/* Address and pointer variable */
#define ADI_SDH_RD_WAIT_EN                          (ADI_SDH_REG_BASE + ADI_SDH_RD_WAIT_EN_REG_OFFSET)
#define pADI_SDH_RD_WAIT_EN                         ((volatile u16*)ADI_SDH_RD_WAIT_EN)

/* SDH Configuration - Enable/Disable SDH Clocks  */
#define ADI_SDH_RD_WAIT_EN_RD_WAIT_REQ_FIELD        0x0001
#define ADI_SDH_RD_WAIT_EN_RD_WAIT_REQ_DISABLE      (*pADI_SDH_RD_WAIT_EN &= ~ADI_SDH_RD_WAIT_EN_RD_WAIT_REQ_FIELD)
#define ADI_SDH_RD_WAIT_EN_RD_WAIT_REQ_ENABLE       (*pADI_SDH_RD_WAIT_EN |= ADI_SDH_RD_WAIT_EN_RD_WAIT_REQ_FIELD)

/*********************************************************************************

SDH Peripheral Identification Registers MMR

*********************************************************************************/

/* Address and pointer variable - Peripheral Identification register 0 */
#define ADI_SDH_PID0                                (ADI_SDH_REG_BASE + ADI_SDH_PID0_OFFSET)
#define pADI_SDH_PID0                               ((volatile u16*)ADI_SDH_PID0)

/* Address and pointer variable - Peripheral Identification register 1 */
#define ADI_SDH_PID1                                (ADI_SDH_REG_BASE + ADI_SDH_PID1_OFFSET)
#define pADI_SDH_PID1                               ((volatile u16*)ADI_SDH_PID1)

/* Address and pointer variable - Peripheral Identification register 2 */
#define ADI_SDH_PID2                                (ADI_SDH_REG_BASE + ADI_SDH_PID2_OFFSET)
#define pADI_SDH_PID2                               ((volatile u16*)ADI_SDH_PID2)

/* Address and pointer variable - Peripheral Identification register 3 */
#define ADI_SDH_PID3                                (ADI_SDH_REG_BASE + ADI_SDH_PID3_OFFSET)
#define pADI_SDH_PID3                               ((volatile u16*)ADI_SDH_PID3)

/* Address and pointer variable - Peripheral Identification register 4 */
#define ADI_SDH_PID4                                (ADI_SDH_REG_BASE + ADI_SDH_PID4_OFFSET)
#define pADI_SDH_PID4                               ((volatile u16*)ADI_SDH_PID4)

/* Address and pointer variable - Peripheral Identification register 5 */
#define ADI_SDH_PID5                                (ADI_SDH_REG_BASE + ADI_SDH_PID5_OFFSET)
#define pADI_SDH_PID5                               ((volatile u16*)ADI_SDH_PID5)

/* Address and pointer variable - Peripheral Identification register 6 */
#define ADI_SDH_PID6                                (ADI_SDH_REG_BASE + ADI_SDH_PID6_OFFSET)
#define pADI_SDH_PID6                               ((volatile u16*)ADI_SDH_PID6)

/* Address and pointer variable - Peripheral Identification register 7 */
#define ADI_SDH_PID7                                (ADI_SDH_REG_BASE + ADI_SDH_PID7_OFFSET)
#define pADI_SDH_PID7                               ((volatile u16*)ADI_SDH_PID7)



/*********************************************************************

Macros specific to SD/SDIO/Multimedia card configuration

*********************************************************************/

/* Supported operating condition (card operating voltage range) for ADS-BF548 Ez-Kit Lite */
#define     ADI_SD_MMC_OPERATING_CONDITION_BF548_EZKIT              0x00FF8000      /* range - 2.7 to 3.6 volts (used as default) */
/* Supported operating condition (card operating voltage range) for ADS-BF548 Ez-Kit Lite Rev 1.1 (macro left for backward compatibility)   */
#define     ADI_SD_MMC_OPERATING_CONDITION_BF548_EZKIT_REV_1_1      0x00FF8000      /* range - 2.7 to 3.6 volts (used as default) */


/*********************************************************************

SD/SDIO/Multimedia card commands - in terms of command index

*********************************************************************/

/******** Commands common to SD/SDIO/Multimedia cards ********/

#define     ADI_SD_MMC_CMD0      0      /* Go Idle state                                                    */
#define     ADI_SD_MMC_CMD2      2      /* Get CID of all cards in the bus                                  */
#define     ADI_SD_MMC_CMD3      3      /* Send relative address(SD/SDIO/SDHC), Set relative address (MMC)  */
#define     ADI_SD_MMC_CMD7      7      /* Select/De-select card                                            */
#define     ADI_SD_MMC_CMD8      8      /* Send SD Memory card interface condition (SDHC)                   */
#define     ADI_SD_MMC_CMD9      9      /* Send Card Specific Data (CSD)                                    */
#define     ADI_SD_MMC_CMD10    10      /* Send Card Identification Data (CID)                              */
#define     ADI_SD_MMC_CMD12    12      /* Stop Transmission                                                */
#define     ADI_SD_MMC_CMD13    13      /* Send Status                                                      */
#define     ADI_SD_MMC_CMD15    15      /* Go Inactive State                                                */
#define     ADI_SD_MMC_CMD16    16      /* Set Block Length                                                 */
#define     ADI_SD_MMC_CMD17    17      /* Read Single Block                                                */
#define     ADI_SD_MMC_CMD18    18      /* Read Multiple Blocks                                             */
#define     ADI_SD_MMC_CMD24    24      /* Write Single Block                                               */
#define     ADI_SD_MMC_CMD25    25      /* Write Multiple Blocks                                            */
#define     ADI_SD_MMC_CMD27    27      /* Program CSD                                                      */
#define     ADI_SD_MMC_CMD28    28      /* Set Write Protect                                                */
#define     ADI_SD_MMC_CMD29    29      /* Clear Write Protect                                              */
#define     ADI_SD_MMC_CMD30    30      /* Send Write Protect                                               */
#define     ADI_SD_MMC_CMD32    32      /* Erase write block start                                          */
#define     ADI_SD_MMC_CMD33    33      /* Erase write block end                                            */
#define     ADI_SD_MMC_CMD38    38      /* Erase selected blocks                                            */

/******** Commands specific to Multimedia card ********/

#define     ADI_MMC_CMD1         1      /* Reads MMC OCR value                                              */
#define     ADI_MMC_CMD6         6      /* Switch mode                                                      */
#define     ADI_MMC_CMD34       34      /* Untag block (sector for MMC) selected for erase                  */
#define     ADI_MMC_CMD40       40      /* Go Interrupt request state                                       */

/******** Commands specific to SD/SDIO cards ********/

#define     ADI_SD_CMD6          6      /* Checks switchable mode & switch card function                    */
#define     ADI_SD_CMD42        42      /* Lock/Unlock card (set/reset card password)                       */
#define     ADI_SD_CMD55        55      /* Set for application specific command                             */
#define     ADI_SD_ACMD6         6      /* Set bus width                                                    */
#define     ADI_SD_ACMD13       13      /* Reads SD memory card status                                      */
#define     ADI_SD_ACMD22       22      /* Reads number of written write blocks in SD memory card           */
#define     ADI_SD_ACMD41       41      /* Reads SD memory card OCR value                                   */
#define     ADI_SD_ACMD42       42      /* Connect/disconnect pullup on DAT3 line                           */
#define     ADI_SD_ACMD51       51      /* Reads SD Configuration register                                  */

/******** Commands specific to SDIO cards ********/
#define     ADI_SDIO_CMD5        5      /* Enquire SDIO support                                             */
#define     ADI_SDIO_CMD52      52      /* IO read/write direct - SDIO command                              */
#define     ADI_SDIO_CMD53      53      /* Command used for SDIO block operations                           */

/*********************************************************************

SD/SDIO/MM card commands - in terms of command classes

*********************************************************************/

/******** Basic Commands (Command Class 0)********/

#define     ADI_SD_MMC_CMD_GO_IDLE_STATE            ADI_SD_MMC_CMD0     /* Go Idle state                                                    */
#define     ADI_SD_MMC_CMD_GET_ALL_CID              ADI_SD_MMC_CMD2     /* Get CID of all cards in the bus                                  */
#define     ADI_MMC_CMD_GET_OCR_VALUE               ADI_MMC_CMD1        /* Reads MMC OCR value                                              */
#define     ADI_SD_MMC_CMD_GET_SET_RELATIVE_ADDR    ADI_SD_MMC_CMD3     /* Send relative address (SD/SDIO/SDHC), Set relative address (MMC) */
#define     ADI_SD_MMC_CMD_SELECT_CARD              ADI_SD_MMC_CMD7     /* Select card                                                      */
#define     ADI_SD_MMC_CMD_DESELECT_CARD            ADI_SD_MMC_CMD7     /* De-select card                                                   */
#define     ADI_SDHC_CMD_SEND_IF_COND               ADI_SD_MMC_CMD8     /* Send SD Card interface condition (SDHC)                          */
#define     ADI_MMC_CMD_SEND_EXT_CSD                ADI_SD_MMC_CMD8     /* Send Extended CSD Register                                       */
#define     ADI_SD_MMC_CMD_SEND_CSD                 ADI_SD_MMC_CMD9     /* Send Card Specific Data (CSD)                                    */
#define     ADI_SD_MMC_CMD_SEND_CID                 ADI_SD_MMC_CMD10    /* Send Card Identification Data (CID)                              */
#define     ADI_SD_MMC_CMD_STOP_TRANSMISSION        ADI_SD_MMC_CMD12    /* Stop Transmission                                                */
#define     ADI_SD_MMC_CMD_SEND_STATUS              ADI_SD_MMC_CMD13    /* read addressed card's Status register                            */
#define     ADI_SD_MMC_CMD_GO_INACTIVE              ADI_SD_MMC_CMD15    /* Go Inactive State                                                */
#define     ADI_SD_MMC_CMD_SET_BLOCK_LENGTH         ADI_SD_MMC_CMD16    /* Set Read/Write Block Length                                      */

/******** Block Read Commands (Command Class 2)********/
#define     ADI_SD_MMC_CMD_READ_SINGLE_BLOCK        ADI_SD_MMC_CMD17    /* Read Single Block                                                */
#define     ADI_SD_MMC_CMD_READ_MULTIPLE_BLOCKS     ADI_SD_MMC_CMD18    /* Read Multiple Blocks                                             */

/******** Block Write Commands (Command Class 4)********/
#define     ADI_SD_MMC_CMD_WRITE_SINGLE_BLOCK       ADI_SD_MMC_CMD24    /* Write Single Block                                               */
#define     ADI_SD_MMC_CMD_WRITE_MULTIPLE_BLOCKS    ADI_SD_MMC_CMD25    /* Write Multiple Blocks                                            */
#define     ADI_SD_MMC_CMD_PROGRAM_CSD              ADI_SD_MMC_CMD27    /* Program CSD                                                      */

/******** Erase Commands (Command Class 5)********/
#define     ADI_SD_MMC_CMD_ERASE_BLOCK_START        ADI_SD_MMC_CMD32    /* Erase write block start                                          */
#define     ADI_SD_MMC_CMD_ERASE_BLOCK_END          ADI_SD_MMC_CMD33    /* Erase write block end                                            */
#define     ADI_SD_MMC_CMD_ERASE_SELECTED_BLOCKS    ADI_SD_MMC_CMD38    /* Erase selected blocks                                            */
#define     ADI_MMC_CMD_UNTAG_BLOCK                 ADI_MMC_CMD34       /* Untag block selected for erase                                   */

/******** Write Protection Commands (Command Class 6)********/
#define     ADI_SD_MMC_CMD_SET_WRITE_PROTECT        ADI_SD_MMC_CMD28    /* Set Write Protect                                                */
#define     ADI_SD_MMC_CMD_CLEAR_WRITE_PROTECT      ADI_SD_MMC_CMD29    /* Clear Write Protect                                              */
#define     ADI_SD_MMC_CMD_SEND_WRITE_PROTECT       ADI_SD_MMC_CMD30    /* Send Write Protect                                               */

/******** Lock Card Commands (Command Class 7)********/
#define     ADI_SD_CMD_SET_DATA_BLOCK_LENGTH        ADI_SD_MMC_CMD16    /* Set lock/unlock data block length                                */
#define     ADI_SD_CMD_SET_PASSWORD                 ADI_SD_CMD42        /* Lock card (set card password)                                    */
#define     ADI_SD_CMD_CLEAR_PASSWORD               ADI_SD_CMD42        /* Unlock card (Clear card password)                                */

/******** Application Specific Commands (Command Class 8)********/

#define     ADI_SD_CMD_SET_FOR_APP_SPECIFIC_CMD     ADI_SD_CMD55        /* Set SD card for application specific command                     */
#define     ADI_SD_CMD_SET_BUS_WIDTH                ADI_SD_ACMD6        /* Set bus width                                                    */
#define     ADI_SD_CMD_GET_MEMORY_STATUS            ADI_SD_ACMD13       /* Reads SD memory card status                                      */
#define     ADI_SD_CMD_GET_WRITTEN_BLOCK_COUNT      ADI_SD_ACMD22       /* Reads number of written write blocks in SD memory card           */
#define     ADI_SD_CMD_GET_OCR_VALUE                ADI_SD_ACMD41       /* Reads SD memory card OCR value                                   */
#define     ADI_SD_CMD_DISCONNECT_DAT3_PULLUP       ADI_SD_ACMD42       /* disconnect pullup on DAT3 line                                   */
#define     ADI_SD_CMD_GET_CONFIG_REG               ADI_SD_ACMD51       /* Reads SD Configuration register                                  */

/******** I/O Mode Commands (Command Class 9)********/

#define     ADI_SDIO_CMD_GET_SUPPORT                ADI_SDIO_CMD5       /* Enquire SDIO support                                             */
#define     ADI_SDIO_CMD_READ_DIRECT                ADI_SDIO_CMD52      /* SDIO read direct                                                 */
#define     ADI_SDIO_CMD_WRITE_DIRECT               ADI_SDIO_CMD52      /* SDIO write direct                                                */
#define     ADI_SDIO_CMD_BLOCK                      ADI_SDIO_CMD53      /* Command used for SDIO block operations                           */
#define     ADI_MMC_CMD_GO_IRQ_STATE                ADI_MMC_CMD40       /* Go Interrupt request state                                       */

/******** Switch Commands (Command Class 10)********/
#define     ADI_SD_CMD_GET_SWITCH_MODE_INFO         ADI_SD_CMD6         /* Checks switchable mode & switch card function                    */
#define     ADI_MMC_CMD_SWITCH                      ADI_MMC_CMD6        /* Checks switchable mode & switch card function                    */

 /*********************************************************************

Maximum SD CLKs to wait after a SD/SDIO/MMC configuration/response

*********************************************************************/

#define     ADI_SDH_WAIT_PERIOD_SLACK                5                  /* Extra slack time to be added with actual wait period             */
#define     ADI_SDH_WAIT_PERIOD_NRC                  8                  /* Clocks to wait between response end to send next command         */
#define     ADI_SDH_WAIT_PERIOD_NCC                  8                  /* Clocks to wait between command sequences                         */
#define     ADI_SDH_WAIT_PERIOD_POWER_UP            74                  /* Clocks to wait before starting card identification sequence      */

/*********************************************************************

Maximum SD CLKs to wait for a command response (TIME OUT value)

*********************************************************************/

#define     ADI_SDH_TIMEOUT_SLACK                   10                  /* Extra slack time to be added with actual Time out                */
#define     ADI_SDH_TIMEOUT_NID                      5                  /* Card Identification Timeout                                      */
#define     ADI_SDH_TIMEOUT_NCR                     64                  /* Set/Get relative address / Data transfer mode Timeout            */

#else
#error "The RSDH/RSI driver is not available for the current target processor"
#endif



#endif      /* __ADI_SDH_REG_H__   */

/*****/
