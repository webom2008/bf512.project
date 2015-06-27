/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_ebiu_module.h,v $
$Revision: 2692 $
$Date: 2010-04-28 12:28:13 -0400 (Wed, 28 Apr 2010) $

Description:
			EBIU Management module header for use by module functions only

*********************************************************************************/

#ifndef __ADI_EBIU_MODULE_H__
#define __ADI_EBIU_MODULE_H__

#include <services/services.h>

#if defined(__ECC__) // VisualDSP C Compiler
#define __ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__
#endif

#if !defined(_LANGUAGE_ASM)


#if defined(__ADSP_MOAB__)                      /* BF54X */

/* Provide Chip ID to determine if this is a mobile or standard DDR part */
#define  pADI_EBIU_CHIPID ((volatile unsigned long *)0xFFC00014)	
#define MOBILE_DDR_CHIPID 0x327EA0CB	

#endif


/**********************************************************************************
  ASYNCHRONOUS MEMORY CONTROLLER 
***********************************************************************************/


/********************************************************************************
* EBIU AMC Global Control Register - EBIU_AMGCTL
*********************************************************************************/
typedef u16 ADI_EBIU_AMGCTL_REG;


#if defined(__ADSP_MOAB__)            /* BF54X */

#define ADI_EBIU_AMGCTL_RESET 0x0002


#else

#define ADI_EBIU_AMGCTL_RESET 0x00F2

#endif

/********************************************************************************
* EBIU AMC Memory Bank Control Register - EBIU_AMBCTL0
*********************************************************************************/
typedef u32 ADI_EBIU_AMBCTL0_REG;
#define ADI_EBIU_AMBCTL0_RESET 0xFFC2FFC2
	

/********************************************************************************
* EBIU AMC Memory Bank Control Register - EBIU_AMBCTL1
*********************************************************************************/

typedef u32 ADI_EBIU_AMBCTL1_REG;
#define ADI_EBIU_AMBCTL1_RESET 0xFFC2FFC2



/* THIS SECTION DEFINES CODE NOT FOR USE BY DDR EBIUs */
#if !defined (__ADI_EBIU_USE_DDR_MEMORY__)


/********************************************************************************
* EBIU SDRAM Global Control Register - EBIU_SDGCTL
*********************************************************************************/
#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__)

#if defined(__ADSP_TETON__) 

// TETON has four external banks.
typedef struct ADI_EBIU_SDBCTL_REG 
{
	u32 b_EB0E:1;
	u32 b_EB0SZ:2;
	u32 :1;
	u32 b_EB0CAW:2;
	u32 :2;
	u32 b_EB1E:1;
	u32 b_EB1SZ:2;
	u32 :1;
	u32 b_EB1CAW:2;
	u32 :2;
	u32 b_EB2E:1;
	u32 b_EB2SZ:2;
	u32 :1;
	u32 b_EB2CAW:2;
	u32 :2;
	u32 b_EB3E:1;
	u32 b_EB3SZ:2;
	u32 :1;
	u32 b_EB3CAW:2;
	u32 :2;
} ADI_EBIU_SDBCTL_REG; 

#endif  // Teton

#if defined(__ADSP_BRAEMAR__) || defined(__ADSP_KOOKABURRA__) 

// EBSZ is 3 bits

typedef struct ADI_EBIU_SDBCTL_REG {
	u16 b_EBE:1;
	u16 b_EBSZ:3;
	u16 b_EBCAW:2;
	u16 unused2:10;
} ADI_EBIU_SDBCTL_REG; 

#endif  // Braemar or Kook


#if !defined(__ADSP_BRAEMAR__) && !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_TETON__) 

// EBSZ is 2 bits
typedef struct ADI_EBIU_SDBCTL_REG {
	u16 b_EBE:1;
	u16 b_EBSZ:2;
	u16 unused1:1;
	u16 b_EBCAW:2;
	u16 unused2:10;
} ADI_EBIU_SDBCTL_REG;
 
#endif  


#else  // Bit Fields

typedef u16 ADI_EBIU_SDBCTL_REG;

#endif

#define ADI_EBIU_SDBCTL_RESET 0x0000


/********************************************************************************
* EBIU SDRAM Bank Control Register - EBIU_SDBCTL
*********************************************************************************/
#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__)

// Bitfield structure for the SDRAM Memory Global Control Register
typedef struct ADI_EBIU_SDGCTL_REG {
	u32 b_SCTLE:1;
#if !defined(__ADSP_TETON__)
	u32 unused1:1;
#else
	u32 b_SCK1E:1;
#endif
	u32 b_CL:2;
	u32 b_PASR:2;
	u32 b_TRAS:4;
	u32 unused2:1;
	u32 b_TRP:3;
	u32 unused3:1;
	u32 b_TRCD:3;
	u32 unused4:1;
	u32 b_TWR:2;
	u32 b_PUPSD:1;
	u32 b_PSM:1;
	u32 b_PSSE:1;
	u32 b_SRFS:1;
	u32 b_EBUFE:1;
	u32 b_FBBRW:1;
	u32 unused5:1;
	u32 b_EMREN:1;
	u32 b_TCSR:1;
	u32 b_CDDBG:1;
	u32 unused6:1;
} ADI_EBIU_SDGCTL_REG; 

#else

typedef u32 ADI_EBIU_SDGCTL_REG;

#endif

#if defined(__ADSP_BRAEMAR__)
#define ADI_EBIU_SDGCTL_RESET 0x8411998D
#elif defined(__ADSP_MOCKINGBIRD__)
#define ADI_EBIU_SDGCTL_RESET 0x0091190D
#else
#define ADI_EBIU_SDGCTL_RESET 0x0091998d
#endif

/********************************************************************************
* EBIU SDRAM Refresh Rate Control Register - EBIU_SDRRC
*********************************************************************************/
#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__)

typedef struct ADI_EBIU_SDRRC_REG {
	u16 b_RDIV:12;
	u16 unused:4;
} ADI_EBIU_SDRRC_REG; 

#else

typedef u16 ADI_EBIU_SDRRC_REG;

#endif



#if defined(__ADSP_KOOKABURRA__) 
#define ADI_EBIU_SDRRC_RESET 0x0406
#elif defined(__ADSP_BRAEMAR__)
#define ADI_EBIU_SDRRC_RESET 0x03A0
#elif defined(__ADSP_MOCKINGBIRD__)
#define ADI_EBIU_SDRRC_RESET 0x0026A
#else     /* other processors */
#define ADI_EBIU_SDRRC_RESET 0x01A0
#endif




/********************************************************************************
* EBIU SDRAM Control Status Register - EBIU_SDSTAT
*********************************************************************************/
//#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__)

typedef struct ADI_EBIU_SDSTAT_REG {
	u16 b_SDCI:1;
	u16 b_SDSRA:1;
	u16 b_SDPUA:1;
	u16 b_SDRS:1;
	u16 b_SDEASE:1;
	u16 b_BGSTAT:1;
	u16 unused:10;
} ADI_EBIU_SDSTAT_REG; 

//#else
//typedef u16 ADI_EBIU_SDSTAT_REG;
//#endif

#define ADI_EBIU_SDSTAT_RESET 0x0008



/********************************************************************************
*********************************************************************************
* Valid EBIU_SDSTAT field values
*********************************************************************************

*********************************************************************************/
/********************************************************************************
* SDCI
*********************************************************************************/
typedef enum ADI_EBIU_SDCI { 
	ADI_EBIU_SDCI_BUSY,
	ADI_EBIU_SDCI_IDLE,
} ADI_EBIU_SDCI;

/********************************************************************************
* SDSRA
*********************************************************************************/
typedef enum ADI_EBIU_SDSRA {
	ADI_EBIU_SDSRA_OFF,
	ADI_EBIU_SDSRA_ON,
} ADI_EBIU_SDSRA;

/********************************************************************************
* SDPUA
*********************************************************************************/
typedef enum ADI_EBIU_SDPUA {
	ADI_EBIU_SDPUA_INACTIVE,
	ADI_EBIU_SDPUA_ACTIVE,
} ADI_EBIU_SDPUA;

/********************************************************************************
* SDRS
*********************************************************************************/
typedef enum ADI_EBIU_SDRS {
	ADI_EBIU_SDRS_OFF,
	ADI_EBIU_SDRS_ON,
} ADI_EBIU_SDRS;

/********************************************************************************
* SDEASE
*********************************************************************************/
typedef enum ADI_EBIU_SDEASE { 
	ADI_EBIU_SDEASE_NO_ERROR,
	ADI_EBIU_SDEASE_ERROR,
} ADI_EBIU_SDEASE;

/********************************************************************************
* BGSTAT
*********************************************************************************/
typedef enum ADI_EBIU_BGSTAT {
	ADI_EBIU_BGSTAT_NOT_GRANTED,
	ADI_EBIU_BGSTAT_GRANTED,
} ADI_EBIU_BGSTAT;

/********************************************************************************
* EBIU Module Configuration structure
*********************************************************************************/
#pragma pack(4)
typedef struct ADI_EBIU_CONFIG {
	u32 cl_threshold;
	ADI_EBIU_TIME						tras_min;
	ADI_EBIU_TIME						trp_min;
	ADI_EBIU_TIME						trcd_min;
	ADI_EBIU_TIMING_VALUE				twr_min;
	ADI_EBIU_TIMING_VALUE				refresh;
	u32									InitializedFlag;
	ADI_EBIU_SDGCTL_REG					sdgctl;
	ADI_EBIU_SDBCTL_REG					sdbctl;
	ADI_EBIU_SDRRC_REG					sdrrc;
#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)
	u16									auto_sync_enabled;  // Auto synchronisation of cores
	u16									IVG_SuppInt0;		// IVG level for Core B supplemental interrupt 1.
	testset_t 							*pLockVar;			// pointer to lock variable
#endif
	void *								ClientHandle;
	u32									MHzFactor;
#if defined(__ADSP_TETON__) 
	u32									sdbctl_enable_mask;  // EBxE bits are set for each bank in use	
#endif

/* Asynch memory controller registers added May 2007 */
    ADI_EBIU_AMGCTL_REG                 amgctl;              /* Asynchronous Memory Global Control Register */
    ADI_EBIU_AMBCTL0_REG                ambctl0;             /* Memory Bank Control Register 0              */
    ADI_EBIU_AMBCTL1_REG                ambctl1;             /* Memory Bank Control Register 1              */  

} ADI_EBIU_CONFIG;
#pragma pack()


#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__) // eg. in VisualDSP C Compiler

#if 0 // template: copy and change
#define REGNAME_FIELD_GET(R)	( (R).b_FIELD )
#define REGNAME_FIELD_SET(R,V) 	( REGNAME_FIELD_GET(R) = (V) )
#endif

// EBIU_SDGCTL - SDRAM Memory Global Control Register

#define EBIU_SDGCTL_SCTLE_GET(R)	( (R).b_SCTLE )
#define EBIU_SDGCTL_SCTLE_SET(R,V) 	( EBIU_SDGCTL_SCTLE_GET(R) = (V) )

#define EBIU_SDGCTL_SCK1E_GET(R)	( (R).b_SCK1E )
#define EBIU_SDGCTL_SCK1E_SET(R,V) 	( EBIU_SDGCTL_SCK1E_GET(R) = (V) )

#define EBIU_SDGCTL_CL_GET(R)		( (R).b_CL )
#define EBIU_SDGCTL_CL_SET(R,V) 	( EBIU_SDGCTL_CL_GET(R) = (V) )

#define EBIU_SDGCTL_PASR_GET(R)		( (R).b_PASR )
#define EBIU_SDGCTL_PASR_SET(R,V) 	( EBIU_SDGCTL_PASR_GET(R) = (V) )

#define EBIU_SDGCTL_TRAS_GET(R)		( (R).b_TRAS )
#define EBIU_SDGCTL_TRAS_SET(R,V) 	( EBIU_SDGCTL_TRAS_GET(R) = (V) )

#define EBIU_SDGCTL_TRP_GET(R)		( (R).b_TRP )
#define EBIU_SDGCTL_TRP_SET(R,V) 	( EBIU_SDGCTL_TRP_GET(R) = (V) )

#define EBIU_SDGCTL_TRCD_GET(R)		( (R).b_TRCD )
#define EBIU_SDGCTL_TRCD_SET(R,V) 	( EBIU_SDGCTL_TRCD_GET(R) = (V) )

#define EBIU_SDGCTL_TWR_GET(R)		( (R).b_TWR )
#define EBIU_SDGCTL_TWR_SET(R,V) 	( EBIU_SDGCTL_TWR_GET(R) = (V) )

#define EBIU_SDGCTL_PUPSD_GET(R)	( (R).b_PUPSD )
#define EBIU_SDGCTL_PUPSD_SET(R,V) 	( EBIU_SDGCTL_PUPSD_GET(R) = (V) )

#define EBIU_SDGCTL_PSM_GET(R)		( (R).b_PSM )
#define EBIU_SDGCTL_PSM_SET(R,V) 	( EBIU_SDGCTL_PSM_GET(R) = (V) )

#define EBIU_SDGCTL_PSSE_GET(R)		( (R).b_PSSE )
#define EBIU_SDGCTL_PSSE_SET(R,V) 	( EBIU_SDGCTL_PSSE_GET(R) = (V) )

#define EBIU_SDGCTL_SRFS_GET(R)		( (R).b_SRFS )
#define EBIU_SDGCTL_SRFS_SET(R,V) 	( EBIU_SDGCTL_SRFS_GET(R) = (V) )

#define EBIU_SDGCTL_EBUFE_GET(R)	( (R).b_EBUFE )
#define EBIU_SDGCTL_EBUFE_SET(R,V) 	( EBIU_SDGCTL_EBUFE_GET(R) = (V) )

#define EBIU_SDGCTL_FBBRW_GET(R)	( (R).b_FBBRW )
#define EBIU_SDGCTL_FBBRW_SET(R,V) 	( EBIU_SDGCTL_FBBRW_GET(R) = (V) )

#define EBIU_SDGCTL_EMREN_GET(R)	( (R).b_EMREN )
#define EBIU_SDGCTL_EMREN_SET(R,V) 	( EBIU_SDGCTL_EMREN_GET(R) = (V) )

#define EBIU_SDGCTL_TCSR_GET(R)		( (R).b_TCSR )
#define EBIU_SDGCTL_TCSR_SET(R,V) 	( EBIU_SDGCTL_TCSR_GET(R) = (V) )

#define EBIU_SDGCTL_CDDBG_GET(R)	( (R).b_CDDBG )
#define EBIU_SDGCTL_CDDBG_SET(R,V) 	( EBIU_SDGCTL_CDDBG_GET(R) = (V) )

// EBIU_SDBCTL - SDRAM Bank control register

#define EBIU_SDBCTL_EBE_GET(R)		( (R).b_EBE )
#define EBIU_SDBCTL_EBE_SET(R,V) 	( EBIU_SDBCTL_EBE_GET(R) = (V) )

#define EBIU_SDBCTL_EBSZ_GET(R)		( (R).b_EBSZ )
#define EBIU_SDBCTL_EBSZ_SET(R,V) 	( EBIU_SDBCTL_EBSZ_GET(R) = (V) )

#if !defined(__ADSP_TETON__)
#define EBIU_SDBCTL_EBCAW_GET(R)	( (R).b_EBCAW )
#define EBIU_SDBCTL_EBCAW_SET(R,V) 	( EBIU_SDBCTL_EBCAW_GET(R) = (V) )
#else
#define EBIU_SDBCTL_EBCAW_GET(R)		( (R).b_EBCAW )
#define EBIU_SDBCTL_EBCAW_SET(R,B,V) 	( EBIU_SDBCTL_EBCAW_GET(R) = (V) )
#endif

// EBIU_SDSTAT register

#define EBIU_SDSTAT_SDCI_GET(R)		( (R).b_SDCI )

#define EBIU_SDSTAT_SDSRA_GET(R)	( (R).b_SDSRA )

#define EBIU_SDSTAT_SDPUA_GET(R)	( (R).b_SDPUA )

#define EBIU_SDSTAT_SDRS_GET(R)		( (R).b_SDRS )

#define EBIU_SDSTAT_SDEASE_GET(R)	( (R).b_SDEASE )
#define EBIU_SDSTAT_SDEASE_SET(R,V) ( EBIU_SDSTAT_SDEASE_GET(R) = (V) )

#define EBIU_SDSTAT_BGSTAT_GET(R)	( (R).b_BGSTAT )

// EBIU_SDRRC - SDRAM Refresh Rate Control register

#define EBIU_SDRRC_RDIV_GET(R)		( (R).b_RDIV )
#define EBIU_SDRRC_RDIV_SET(R,V) 	( EBIU_SDRRC_RDIV_GET(R) = (V) )


#else // Compilers not supporting bitfield structs

#if 0 // template: copy and change
#define REGNAME_FIELD_MASK
#define REGNAME_FIELD_SHIFT
#define REGNAME_FIELD_SET(R,V) 	( ( (R) = (V) << REGNAME_FIELD_SHIFT ) | ( (R) & ~REGNAME_FIELD_MASK ) )
#define REGNAME_FIELD_GET(R)	( ( (R) & REGNAME_FIELD_MASK ) >> REGNAME_FIELD_SHIFT )
#endif

// EBIU_SDGCTL - SDRAM Memory Global Control Register

#define EBIU_SDGCTL_SCTLE_MASK		0x00000001
#define EBIU_SDGCTL_SCTLE_SHIFT		0
#define EBIU_SDGCTL_SCTLE_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_SCTLE_SHIFT ) | ( (R) & ~EBIU_SDGCTL_SCTLE_MASK ) )
#define EBIU_SDGCTL_SCTLE_GET(R)	( ( (R) & EBIU_SDGCTL_SCTLE_MASK ) >> EBIU_SDGCTL_SCTLE_SHIFT )

#define EBIU_SDGCTL_SCK1E_MASK		0x00000002
#define EBIU_SDGCTL_SCK1E_SHIFT		1
#define EBIU_SDGCTL_SCK1E_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_SCK1E_SHIFT ) | ( (R) & ~EBIU_SDGCTL_SCK1E_MASK ) )
#define EBIU_SDGCTL_SCK1E_GET(R)	( ( (R) & EBIU_SDGCTL_SCK1E_MASK ) >> EBIU_SDGCTL_SCK1E_SHIFT )

#define EBIU_SDGCTL_CL_MASK			0x0000000C
#define EBIU_SDGCTL_CL_SHIFT		2
#define EBIU_SDGCTL_CL_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_CL_SHIFT ) | ( (R) & ~EBIU_SDGCTL_CL_MASK ) )
#define EBIU_SDGCTL_CL_GET(R)		( ( (R) & EBIU_SDGCTL_CL_MASK ) >> EBIU_SDGCTL_CL_SHIFT )

#define EBIU_SDGCTL_PASR_MASK		0x00000030
#define EBIU_SDGCTL_PASR_SHIFT		4
#define EBIU_SDGCTL_PASR_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_PASR_SHIFT ) | ( (R) & ~EBIU_SDGCTL_PASR_MASK ) )
#define EBIU_SDGCTL_PASR_GET(R)		( ( (R) & EBIU_SDGCTL_PASR_MASK ) >> EBIU_SDGCTL_PASR_SHIFT )

#define EBIU_SDGCTL_TRAS_MASK		0x000003C0
#define EBIU_SDGCTL_TRAS_SHIFT		6
#define EBIU_SDGCTL_TRAS_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_TRAS_SHIFT ) | ( (R) & ~EBIU_SDGCTL_TRAS_MASK ) )
#define EBIU_SDGCTL_TRAS_GET(R)		( ( (R) & EBIU_SDGCTL_TRAS_MASK ) >> EBIU_SDGCTL_TRAS_SHIFT )

#define EBIU_SDGCTL_TRP_MASK		0x00003800
#define EBIU_SDGCTL_TRP_SHIFT		11
#define EBIU_SDGCTL_TRP_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_TRP_SHIFT ) | ( (R) & ~EBIU_SDGCTL_TRP_MASK ) )
#define EBIU_SDGCTL_TRP_GET(R)		( ( (R) & EBIU_SDGCTL_TRP_MASK ) >> EBIU_SDGCTL_TRP_SHIFT )

#define EBIU_SDGCTL_TRCD_MASK		0x00038000
#define EBIU_SDGCTL_TRCD_SHIFT		15
#define EBIU_SDGCTL_TRCD_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_TRCD_SHIFT ) | ( (R) & ~EBIU_SDGCTL_TRCD_MASK ) )
#define EBIU_SDGCTL_TRCD_GET(R)		( ( (R) & EBIU_SDGCTL_TRCD_MASK ) >> EBIU_SDGCTL_TRCD_SHIFT )

#define EBIU_SDGCTL_TWR_MASK		0x00180000
#define EBIU_SDGCTL_TWR_SHIFT		19
#define EBIU_SDGCTL_TWR_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_TWR_SHIFT ) | ( (R) & ~EBIU_SDGCTL_TWR_MASK ) )
#define EBIU_SDGCTL_TWR_GET(R)		( ( (R) & EBIU_SDGCTL_TWR_MASK ) >> EBIU_SDGCTL_TWR_SHIFT )

#define EBIU_SDGCTL_PUPSD_MASK		0x00200000
#define EBIU_SDGCTL_PUPSD_SHIFT		21
#define EBIU_SDGCTL_PUPSD_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_PUPSD_SHIFT ) | ( (R) & ~EBIU_SDGCTL_PUPSD_MASK ) )
#define EBIU_SDGCTL_PUPSD_GET(R)	( ( (R) & EBIU_SDGCTL_PUPSD_MASK ) >> EBIU_SDGCTL_PUPSD_SHIFT )

#define EBIU_SDGCTL_PSM_MASK		0x00400000
#define EBIU_SDGCTL_PSM_SHIFT		22
#define EBIU_SDGCTL_PSM_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_PSM_SHIFT ) | ( (R) & ~EBIU_SDGCTL_PSM_MASK ) )
#define EBIU_SDGCTL_PSM_GET(R)		( ( (R) & EBIU_SDGCTL_PSM_MASK ) >> EBIU_SDGCTL_PSM_SHIFT )

#define EBIU_SDGCTL_PSSE_MASK		0x00800000
#define EBIU_SDGCTL_PSSE_SHIFT		23
#define EBIU_SDGCTL_PSSE_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_PSSE_SHIFT ) | ( (R) & ~EBIU_SDGCTL_PSSE_MASK ) )
#define EBIU_SDGCTL_PSSE_GET(R)		( ( (R) & EBIU_SDGCTL_PSSE_MASK ) >> EBIU_SDGCTL_PSSE_SHIFT )

#define EBIU_SDGCTL_SRFS_MASK		0x01000000
#define EBIU_SDGCTL_SRFS_SHIFT		24
#define EBIU_SDGCTL_SRFS_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_SRFS_SHIFT ) | ( (R) & ~EBIU_SDGCTL_SRFS_MASK ) )
#define EBIU_SDGCTL_SRFS_GET(R)		( ( (R) & EBIU_SDGCTL_SRFS_MASK ) >> EBIU_SDGCTL_SRFS_SHIFT )

#define EBIU_SDGCTL_EBUFE_MASK		0x02000000
#define EBIU_SDGCTL_EBUFE_SHIFT		25
#define EBIU_SDGCTL_EBUFE_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_EBUFE_SHIFT ) | ( (R) & ~EBIU_SDGCTL_EBUFE_MASK ) )
#define EBIU_SDGCTL_EBUFE_GET(R)	( ( (R) & EBIU_SDGCTL_EBUFE_MASK ) >> EBIU_SDGCTL_EBUFE_SHIFT )

#define EBIU_SDGCTL_FBBRW_MASK		0x04000000
#define EBIU_SDGCTL_FBBRW_SHIFT		26
#define EBIU_SDGCTL_FBBRW_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_FBBRW_SHIFT ) | ( (R) & ~EBIU_SDGCTL_FBBRW_MASK ) )
#define EBIU_SDGCTL_FBBRW_GET(R)	( ( (R) & EBIU_SDGCTL_FBBRW_MASK ) >> EBIU_SDGCTL_FBBRW_SHIFT )

#define EBIU_SDGCTL_EMREN_MASK		0x10000000
#define EBIU_SDGCTL_EMREN_SHIFT		28
#define EBIU_SDGCTL_EMREN_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_EMREN_SHIFT ) | ( (R) & ~EBIU_SDGCTL_EMREN_MASK ) )
#define EBIU_SDGCTL_EMREN_GET(R)	( ( (R) & EBIU_SDGCTL_EMREN_MASK ) >> EBIU_SDGCTL_EMREN_SHIFT )

#define EBIU_SDGCTL_TCSR_MASK		0x20000000
#define EBIU_SDGCTL_TCSR_SHIFT		29
#define EBIU_SDGCTL_TCSR_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_TCSR_SHIFT ) | ( (R) & ~EBIU_SDGCTL_TCSR_MASK ) )
#define EBIU_SDGCTL_TCSR_GET(R)		( ( (R) & EBIU_SDGCTL_TCSR_MASK ) >> EBIU_SDGCTL_TCSR_SHIFT )

#define EBIU_SDGCTL_CDDBG_MASK		0x40000000
#define EBIU_SDGCTL_CDDBG_SHIFT		30
#define EBIU_SDGCTL_CDDBG_SET(R,V) 	( ( (R) = (V) << EBIU_SDGCTL_CDDBG_SHIFT ) | ( (R) & ~EBIU_SDGCTL_CDDBG_MASK ) )
#define EBIU_SDGCTL_CDDBG_GET(R)	( ( (R) & EBIU_SDGCTL_CDDBG_MASK ) >> EBIU_SDGCTL_CDDBG_SHIFT )


// EBIU_SDBCTL - SDRAM Bank control register

#define EBIU_SDBCTL_EBE_MASK		0x0001
#define EBIU_SDBCTL_EBE_SHIFT		0
#define EBIU_SDBCTL_EBE_SET(R,V) 	( ( (R) = (V) << EBIU_SDBCTL_EBE_SHIFT ) | ( (R) & ~EBIU_SDBCTL_EBE_MASK ) )
#define EBIU_SDBCTL_EBE_GET(R)		( ( (R) & EBIU_SDBCTL_EBE_MASK ) >> EBIU_SDBCTL_EBE_SHIFT )

#if defined(__ADSP_BRAEMAR__) || defined(__ADSP_KOOKABURRA__) 
#define EBIU_SDBCTL_EBSZ_MASK		0x000E
#else
#define EBIU_SDBCTL_EBSZ_MASK		0x0006
#endif
#define EBIU_SDBCTL_EBSZ_SHIFT		1
#define EBIU_SDBCTL_EBSZ_SET(R,V) 	( ( (R) = (V) << EBIU_SDBCTL_EBSZ_SHIFT ) | ( (R) & ~EBIU_SDBCTL_EBSZ_MASK ) )
#define EBIU_SDBCTL_EBSZ_GET(R)		( ( (R) & EBIU_SDBCTL_EBSZ_MASK ) >> EBIU_SDBCTL_EBSZ_SHIFT )

#define EBIU_SDBCTL_EBCAW_MASK		0x0030
#define EBIU_SDBCTL_EBCAW_SHIFT		4
#define EBIU_SDBCTL_EBCAW_SET(R,V) 	( ( (R) = (V) << EBIU_SDBCTL_EBCAW_SHIFT ) | ( (R) & ~EBIU_SDBCTL_EBCAW_MASK ) )
#define EBIU_SDBCTL_EBCAW_GET(R)	( ( (R) & EBIU_SDBCTL_EBCAW_MASK ) >> EBIU_SDBCTL_EBCAW_SHIFT )


// EBIU_SDSTAT - SDRAM control status register

#define EBIU_SDSTAT_SDCI_MASK		0x0001
#define EBIU_SDSTAT_SDCI_SHIFT		0
#define EBIU_SDSTAT_SDCI_GET(R)		( ( (R) & EBIU_SDSTAT_SDCI_MASK ) >> EBIU_SDSTAT_SDCI_SHIFT )

#define EBIU_SDSTAT_SDSRA_MASK		0x0002
#define EBIU_SDSTAT_SDSRA_SHIFT		1
#define EBIU_SDSTAT_SDSRA_GET(R)	( ( (R) & EBIU_SDSTAT_SDSRA_MASK ) >> EBIU_SDSTAT_SDSRA_SHIFT )

#define EBIU_SDSTAT_SDPUA_MASK		0x0004
#define EBIU_SDSTAT_SDPUA_SHIFT		2
#define EBIU_SDSTAT_SDPUA_GET(R)	( ( (R) & EBIU_SDSTAT_SDPUA_MASK ) >> EBIU_SDSTAT_SDPUA_SHIFT )

#define EBIU_SDSTAT_SDRS_MASK		0x0008
#define EBIU_SDSTAT_SDRS_SHIFT		3
#define EBIU_SDSTAT_SDRS_GET(R)		( ( (R) & EBIU_SDSTAT_SDRS_MASK ) >> EBIU_SDSTAT_SDRS_SHIFT )

#define EBIU_SDSTAT_SDEASE_MASK		0x0010
#define EBIU_SDSTAT_SDEASE_SHIFT	4
#define EBIU_SDSTAT_SDEASE_SET(R,V) ( ( (R) = (V) << EBIU_SDSTAT_SDEASE_SHIFT ) | ( (R) & ~EBIU_SDSTAT_SDEASE_MASK ) )
#define EBIU_SDSTAT_SDEASE_GET(R)	( ( (R) & EBIU_SDSTAT_SDEASE_MASK ) >> EBIU_SDSTAT_SDEASE_SHIFT )

#define EBIU_SDSTAT_BGSTAT_MASK		0x0020
#define EBIU_SDSTAT_BGSTAT_SHIFT	5
#define EBIU_SDSTAT_BGSTAT_GET(R)	( ( (R) & EBIU_SDSTAT_BGSTAT_MASK ) >> EBIU_SDSTAT_BGSTAT_SHIFT )

// EBIU_SDRRC - SDRAM Refresh Rate Control register

#define EBIU_SDRRC_RDIV_MASK		0x0FFF
#define EBIU_SDRRC_RDIV_SHIFT		0
#define EBIU_SDRRC_RDIV_SET(R,V) 	( ( (R) = (V) << EBIU_SDRRC_RDIV_SHIFT ) | ( (R) & ~EBIU_SDRRC_RDIV_MASK ) )
#define EBIU_SDRRC_RDIV_GET(R)		( ( (R) & EBIU_SDRRC_RDIV_MASK ) >> EBIU_SDRRC_RDIV_SHIFT )

#endif /* bit fields not supported */

#if defined(__ADSP_TETON__)
#define ADI_CRITICAL_SEMAPHORE NULL
#else
#define ADI_CRITICAL_SEMAPHORE NULL
#endif


void adi_ebiu_SetKnownState(void);
// prototypes for assembler functions
void adi_ebiu_SelfRefreshEnable(void);



/* this section is compiled for processors with EBIU's that support DDR memory */
#else



/* Bitfield structures are used only if the compiler supports them */
#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__)

/* Bitfield structure for the DDR Memory Control Register 0 */

typedef struct ADI_EBIU_DDRCTL0_REG {
	u32 b_REFI:14;       /* refresh interval (0x0411) */
	u32 b_RFC:4;         /* auto refresh command period (0x0A)  */
	u32 b_RP:4;          /* (cycles) precharge to activate interval (0x03) */
	u32 b_RAS:4;         /* (cycles) activate to precharge interval - (0x06)*/
	u32 b_RC:4;          /* (cycles) interval between successive activate commands (0x02) */
	u32 unused:2;        /* RP + RAS = RC */
} ADI_EBIU_DDRCTL0_REG; 

    
typedef struct ADI_EBIU_DDRCTL1_REG {
	u32 b_RCD:4;            /* (default 4'b0011) cycles from active command to read/write assertion. */
	u32 b_MRD:4;            /* MRD mode register set to active (default 1'b10) */
	u32 b_WR:2;             /* write recovery time (default 2'b10) */
	u32 b_unused1:2;
	u32 b_DATA_WIDTH:2;     /* data width (bit) (default 10) 10 16-bit no other values allowed */
	u32 b_EXTERNAL_BANKS:2; /* external banks (bit) (default 01) 00=1, 01=2, 10,11=reserved */
	u32 b_DEVICE_WIDTH:2;   /* device width (bit) (default 10) 00=4, 01=8, 10=16, 11=reserved */
	u32 b_DEVICE_SIZE:2;    /* device size (Mbit) (default 00) 00=512, 01=64, 10=128, 11=256 */
	u32 b_unused2:8;
	u32 b_WTR:4;	        /* last write data until next read command (default 01) */	
} ADI_EBIU_DDRCTL1_REG; 



typedef struct ADI_EBIU_DDRCTL2_REG 
{
	u32 b_BURST_LENGTH:3;  /*  burst length (read-only) hard coded for 2 */
	u32 b_unused1:1; 
	u32 b_CAS:3;           /*  CAS latency (default 2 for standard; 3 for mobile) cycles from assertion of R/W until valid data */
	u32 b_unused2:1;
	u32 b_DLL_RESET:1;     /* (default 0) 0 - 0=normal 1=with DLL (Delay-Locked Loop) reset */
	u32 b_unused3:3;
	u32 b_REGE:1;          /* REGE - (default 0)  1= external registers inserted in control & address signals */
	u32 b_unused4:19;		
} ADI_EBIU_DDRCTL2_REG; 


/* in mobile mode  */

typedef struct ADI_EBIU_DDRCTL3_REG_MOBILE
{
	u32 b_PASR:3;            /* PASR partial array self-refresh */
	u32 b_TCSR:2;      
	u32 b_DS:2;              /* half strength - default = 01 */
	u32 b_unused2:25;		
} ADI_EBIU_DDRCTL3_REG_MOBILE;

/* in regular mode */

typedef struct ADI_EBIU_DDRCTL3_REG_REGULAR
{
	u32 b_DLL_DISABLE:1;                /* (default=1)  0=enable DLL; 1=disable DLL */
	u32 b_DS:1;                         /* drive strength = default = 01 (reduced) */
	u32 b_unused2:29;		
} ADI_EBIU_DDRCTL3_REG_REGULAR; 


/* both modes combined in a union */

typedef union ADI_EBIU_DDRCTL3_REG
{
    ADI_EBIU_DDRCTL3_REG_MOBILE ctlreg3_mobile;
    ADI_EBIU_DDRCTL3_REG_REGULAR ctlreg3_regular;
} ADI_EBIU_DDRCTL3_REG;



/* bit field structure for DDR control register 4
   also called the reset control register */
typedef struct ADI_EBIU_DDRCTL4_REG 
{
	u16 b_SOFT_RESET:1;                    /* SRESET DDR - controller soft reset */
	u16 b_unused1:1;     
	u16 b_PREFETCH_BUFFER_SOFT_RESET:1;	   /* Prefetch Buffer Soft reset */
	u16 b_SELF_REFRESH_REQUEST:1;	       /* Self refresh request */
	u16 b_SELF_REFRESH_ACK:1;		       /* Self-Refresh Ack */
	u16 b_MOBILE_DDR_ENABLE:1;             /* Mobile DDR Enable */
	u16 b_unused2:10;                      /* reserved field, do not modify the contents */
} ADI_EBIU_DDRCTL4_REG; 


/*  initial reset value 'b00010 - disables DDR */
   

/*  bit field structs are not supported so define registers accordingly */
#else  

typedef u32 ADI_EBIU_DDRCTL0_REG;
typedef u32 ADI_EBIU_DDRCTL1_REG;
typedef u32 ADI_EBIU_DDRCTL2_REG;
typedef u32 ADI_EBIU_DDRCTL3_REG;
typedef u16 ADI_EBIU_DDRCTL4_REG;


/* end of conditional compilation based on whether or not bit field structures are supported. */
#endif


/* Hardware Reset values for the MMRs */

#define ADI_EBIU_DDRCTL0_HW_RESET 0x098E8411
#define ADI_EBIU_DDRCTL1_HW_RESET 0x10026223
#define ADI_EBIU_DDRCTL2_HW_RESET 0x00000021
#define ADI_EBIU_DDRCTL3_HW_RESET 0x00000001
#define ADI_EBIU_DDRCTL4_HW_RESET 0x00000002 
 

/* 
   XML Reset Values for the MMRs
*/

#define ADI_EBIU_DDRCTL0_XML_RESET 0x218A8287 //  0x218A8411 // 0x23FE8287 
#define ADI_EBIU_DDRCTL1_XML_RESET 0x20022222 // 0x10022223
#define ADI_EBIU_DDRCTL2_XML_RESET 0x00000021 // 0x00000020
#define ADI_EBIU_DDRCTL3_XML_RESET 0x00000003

/* for mobile DDR */
#define ADI_EBIU_DDRCTL2M_XML_RESET 0x00000031 // 0x00000030
#define ADI_EBIU_DDRCTL3M_XML_RESET 0x00000020


/*********************************************************************************
  EBIU_CONFIG struct for EBIU's that support DDR memory 
**********************************************************************************/

#pragma pack(4)
typedef struct ADI_EBIU_CONFIG {
	u32	                    InitializedFlag; /* whether the service is initialized */		 
	u32                       cas;             /* CAS latency of 2 as in default config */
	ADI_EBIU_TIMING_VALUE     ras_min;         /* tRAS = 42ns min = 6 SCLKs */
	ADI_EBIU_TIMING_VALUE     rp_min;          /* tRP = 15ns min = 2 SCLKs */
	ADI_EBIU_TIMING_VALUE     rcd_min;         /* tRCD = 15ns min = 2 SCLKs */
	ADI_EBIU_TIMING_VALUE     wr_min;          /* tWR = 15ns min = 2 SCLKs */
	ADI_EBIU_TIMING_VALUE     rfc_min;         /* tRFC = 72ns min = 10 SCLKs */
	ADI_EBIU_TIMING_VALUE     rc_min;          /* tRC = 60ns min = 8 SCLKs */
	ADI_EBIU_TIMING_VALUE     mrd_min;         /* tMRD = 12ns min = 2 SCLKs */
	ADI_EBIU_TIMING_VALUE     refi;            /* tREFI = 7,8us max = 1037 SCLKs */	
	ADI_EBIU_TIMING_VALUE     wtr_min;         /* tWTR @ CL2 = 7.5ns min = 1 SCLK */
	ADI_EBIU_DDRCTL0_REG      ddrctl0;
	ADI_EBIU_DDRCTL1_REG      ddrctl1;
	ADI_EBIU_DDRCTL2_REG      ddrctl2;
	ADI_EBIU_DDRCTL3_REG      ddrctl3;	
	ADI_EBIU_DDRCTL4_REG      ddrctl4;	
	u32                       MHzFactor;
	
		
/* Asynch memory controller registers added May 2007 */
    ADI_EBIU_AMGCTL_REG                 amgctl;              /* Asynchronous Memory Global Control Register */
    ADI_EBIU_AMBCTL0_REG                ambctl0;             /* Memory Bank Control Register 0              */
    ADI_EBIU_AMBCTL1_REG                ambctl1;             /* Memory Bank Control Register 1              */  	
	
	
	
} ADI_EBIU_CONFIG;
#pragma pack()



/******************************************************************************
  Macros to get/set the fields within the MMR structures  
*******************************************************************************/

#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__)

/* This set of macros is for the compilation environment where bitfields are supported */

/* control register 0 commands */

#define EBIU_DDRCTL0_REFI_GET(R) ((R).b_REFI)
#define EBIU_DDRCTL0_REFI_SET(R,V) ( EBIU_DDRCTL0_REFI_GET(R) = (V) ) 	 
#define EBIU_DDRCTL0_RFC_GET(R)	((R).b_RFC) 
#define EBIU_DDRCTL0_RFC_SET(R,V) (EBIU_DDRCTL0_RFC_GET(R) = (V) ) 	 
#define EBIU_DDRCTL0_RP_GET(R) ((R).b_RP)
#define EBIU_DDRCTL0_RP_SET(R,V) (EBIU_DDRCTL0_RP_GET(R) = (V) )
#define EBIU_DDRCTL0_RAS_GET(R)	( (R).b_RAS )	 
#define EBIU_DDRCTL0_RAS_SET(R,V) (EBIU_DDRCTL0_RAS_GET(R) = (V) )
#define EBIU_DDRCTL0_RC_GET(R) ((R).b_RC)	 
#define EBIU_DDRCTL0_RC_SET(R,V) (EBIU_DDRCTL0_RC_GET(R) = V)
 
/* control register 1 commands */
 
#define EBIU_DDRCTL1_RCD_GET(R) ((R).b_RCD)
#define EBIU_DDRCTL1_RCD_SET(R,V) (EBIU_DDRCTL1_RCD_GET(R) = V) 	 
#define EBIU_DDRCTL1_MRD_GET(R)	((R).b_MRD) 
#define EBIU_DDRCTL1_MRD_SET(R,V) (EBIU_DDRCTL1_MRD_GET(R) = V) 	 
#define EBIU_DDRCTL1_WR_GET(R) ((R).b_WR)
#define EBIU_DDRCTL1_WR_SET(R,V) (EBIU_DDRCTL1_WR_GET(R) = V)
#define EBIU_DDRCTL1_DATA_WIDTH_GET(R) ((R).b_DATA_WIDTH)	 
#define EBIU_DDRCTL1_DATA_WIDTH_SET(R,V) (EBIU_DDRCTL1_DATA_WIDTH_GET(R) = V)
#define EBIU_DDRCTL1_EXTERNAL_BANKS_GET(R) ((R).b_EXTERNAL_BANKS)	 
#define EBIU_DDRCTL1_EXTERNAL_BANKS_SET(R,V) (EBIU_DDRCTL1_EXTERNAL_BANKS_GET(R) = V)
#define EBIU_DDRCTL1_DEVICE_WIDTH_GET(R) ((R).b_DEVICE_WIDTH)	 
#define EBIU_DDRCTL1_DEVICE_WIDTH_SET(R,V) (EBIU_DDRCTL1_DEVICE_WIDTH_GET(R) = V)
#define EBIU_DDRCTL1_DEVICE_SIZE_GET(R) ((R).b_DEVICE_SIZE)	 
#define EBIU_DDRCTL1_DEVICE_SIZE_SET(R,V) (EBIU_DDRCTL1_DEVICE_SIZE_GET(R) = V)
#define EBIU_DDRCTL1_WTR_GET(R)	((R).b_WTR)
#define EBIU_DDRCTL1_WTR_SET(R,V) (EBIU_DDRCTL1_WTR_GET(R) = V)
 
/* control register 2 commands (only setting of CAS is supported) */
 
#define EBIU_DDRCTL2_CAS_GET(R)	((R).b_CAS)	 
#define EBIU_DDRCTL2_CAS_SET(R,V) (EBIU_DDRCTL2_CAS_GET(R) = V)
  

/* no non-mobile control register 3 commands */

/*  only commands for mobile mode (PASR, DS) */
#define EBIU_DDRCTL3_PASR_GET(R)	((R).b_PASR)	 
#define EBIU_DDRCTL3_PASR_SET(R,V) (EBIU_DDRCTL3_PASR_GET(R) = V)
  
#define EBIU_DDRCTL3_DS_GET(R)	((R).b_DS)	 
#define EBIU_DDRCTL3_DS_SET(R,V) (EBIU_DDRCTL3_DS_GET(R) = V)
  

/* NOTE:  DS is not changeable for either mobile or regular mode */
/* TCSR is read only */

/* no commands for non-mobile mode */


/* control register 4 commands */

#define EBIU_DDRCTL4_SOFT_RESET_GET(R) ((R).b_SOFT_RESET)	 
#define EBIU_DDRCTL4_SOFT_RESET_SET(R,V) (EBIU_DDRCTL4_SOFT_RESET_GET(R) = V)
#define EBIU_DDRCTL4_SELF_REFRESH_REQUEST_GET(R) ((R).b_SELF_REFRESH_REQUEST)	 
#define EBIU_DDRCTL4_SELF_REFRESH_REQUEST_SET(R,V) (EBIU_DDRCTL4_SELF_REFRESH_REQUEST_GET(R) = V)
#define EBIU_DDRCTL4_MOBILE_DDR_ENABLE_GET(R) ((R).b_MOBILE_DDR_ENABLE)	 
#define EBIU_DDRCTL4_MOBILE_DDR_ENABLE_SET(R,V) (EBIU_DDRCTL4_MOBILE_DDR_ENABLE_GET(R) = V)


/* read-only self-refresh ACKnowledge field */

#define EBIU_DDRCTL4_SELF_REFRESH_ACK_GET(R) ((R).b_SELF_REFRESH_ACK) 
 
	
/* Queue configuration commands and error register commands have been removed and will be added back 
  if deemed necessary */
 
 
/* The following set of macros is for compilation environments that do not support bitfield structures */

   
#else /* non-bitfield versions */

#if 0 // template: copy and change
#define REGNAME_FIELD_MASK
#define REGNAME_FIELD_SHIFT
#define REGNAME_FIELD_SET(R,V) 	( ( (R) = (V) << REGNAME_FIELD_SHIFT ) | ( (R) & ~REGNAME_FIELD_MASK ) )
#define REGNAME_FIELD_GET(R)	( ( (R) & REGNAME_FIELD_MASK ) >> REGNAME_FIELD_SHIFT )
#endif

/* Define the macros for the environments where bitfield structures are not supported */
/* Uses masking and shifting instead of accessing the bit fields within the structure. */

/* control register 0 commands */
#define EBIU_DDRCTL0_REFI_MASK     0x03FFF
#define EBIU_DDRCTL0_REFI_SHIFT    0
#define EBIU_DDRCTL0_REFI_GET(R)   ( ( (R) & EBIU_DDRCTL0_REFI_MASK) >> EBIU_DDRCTL0_REFI_SHIFT )
#define EBIU_DDRCTL0_REFI_SET(R,V) ( ( (R) = (V) << EBIU_DDRCTL0_REFI_SHIFT ) | ( (R) & ~EBIU_DDRCTL0_REFI_MASK ) )	 

#define EBIU_DDRCTL0_RFC_MASK      0x03C000
#define EBIU_DDRCTL0_RFC_SHIFT     14
#define EBIU_DDRCTL0_RFC_GET(R)    ( ( (R) &  EBIU_DDRCTL0_RFC_MASK) >> EBIU_DDRCTL0_RFC_SHIFT )
#define EBIU_DDRCTL0_RFC_SET(R,V)  ( ( (R) = (V) << EBIU_DDRCTL0_RFC_SHIFT ) | ( (R) & ~EBIU_DDRCTL0_RFC_MASK ) )	 

#define EBIU_DDRCTL0_RP_MASK       0x03C000
#define EBIU_DDRCTL0_RP_SHIFT      18
#define EBIU_DDRCTL0_RP_GET(R)     ( ( (R) & EBIU_DDRCTL0_RP_MASK >> EBIU_DDRCTL0_RP_SHIFT )
#define EBIU_DDRCTL0_RP_SET(R,V)   ( ( (R) = (V) << EBIU_DDRCTL0_RP_SHIFT ) | ( (R) & ~EBIU_DDRCTL0_RP_MASK ) )  

#define EBIU_DDRCTL0_RAS_MASK      0x03C0000
#define EBIU_DDRCTL0_RAS_SHIFT     22
#define EBIU_DDRCTL0_RAS_GET(R)    ( ( (R) &  EBIU_DDRCTL0_RAS_MASK >> EBIU_DDRCTL0_RAS_SHIFT )
#define EBIU_DDRCTL0_RAS_SET(R,V)  ( ( (R) = (V) << EBIU_DDRCTL0_RAS_SHIFT ) | ( (R) & ~EBIU_DDRCTL0_RAS_MASK ) )

#define EBIU_DDRCTL0_RC_MASK       0x03C00000
#define EBIU_DDRCTL0_RC_SHIFT      26
#define EBIU_DDRCTL0_RC_GET(R)     ( ( (R) & EBIU_DDRCTL0_RC_MASK) >> EBIU_DDRCTL0_RC_SHIFT )
#define EBIU_DDRCTL0_RC_SET(R,V)   ( ( (R) = (V) << EBIU_DDRCTL0_RC_SHIFT ) | ( (R) & ~EBIU_DDRCTL0_RC_MASK ) )

 
/* control register 1 commands */
 
#define EBIU_DDRCTL1_RCD_MASK      0x0F
#define EBIU_DDRCTL1_RCD_SHIFT     0
#define EBIU_DDRCTL1_RCD_GET(R)    ( ( (R) & EBIU_DDRCTL1_RCD_MASK) >> EBIU_DDRCTL1_RCD_SHIFT )
#define EBIU_DDRCTL1_RCD_SET(R,V)  ( ( (R) = (V) << EBIU_DDRCTL1_RCD_SHIFT ) | ( (R) & ~EBIU_DDRCTL1_RCD_MASK ) )	 

#define EBIU_DDRCTL1_MRD_MASK      0x0F0
#define EBIU_DDRCTL1_MRD_SHIFT     4
#define EBIU_DDRCTL1_MRD_GET(R)    ( ( (R) & EBIU_DDRCTL1_MRD_MASK) >> EBIU_DDRCTL1_MRD_SHIFT )
#define EBIU_DDRCTL1_MRD_SET(R,V)  ( ( (R) = (V) << EBIU_DDRCTL1_MRD_SHIFT ) | ( (R) & ~EBIU_DDRCTL1_MRD_MASK ) )	 

#define EBIU_DDRCTL1_WR_MASK       0x0F00
#define EBIU_DDRCTL1_WR_SHIFT      8
#define EBIU_DDRCTL1_WR_GET(R)     ( ( (R) & EBIU_DDRCTL1_WR_MASK) >> EBIU_DDRCTL1_WR_SHIFT )
#define EBIU_DDRCTL1_WR_SET(R,V)   ( ( (R) = (V) << EBIU_DDRCTL1_WR_SHIFT ) | ( (R) & ~EBIU_DDRCTL1_WR_MASK ) )

#define EBIU_DDRCTL1_DATA_WIDTH_MASK       0x0F000
#define EBIU_DDRCTL1_DATA_WIDTH_SHIFT      12
#define EBIU_DDRCTL1_DATA_WIDTH_GET(R)     ( ( (R) & EBIU_DDRCTL1_DATA_WIDTH_MASK) >> EBIU_DDRCTL1_DATA_WIDTH_SHIFT )
#define EBIU_DDRCTL1_DATA_WIDTH_SET(R,V)   ( ( (R) = (V) << EBIU_DDRCTL1_DATA_WIDTH_SHIFT ) | ( (R) & ~EBIU_DDRCTL1_DATA_WIDTH_MASK ) )

#define EBIU_DDRCTL1_EXTERNAL_BANKS_MASK      0x03C000
#define EBIU_DDRCTL1_EXTERNAL_BANKS_SHIFT     14
#define EBIU_DDRCTL1_EXTERNAL_BANKS_GET(R)    ( ( (R) & EBIU_DDRCTL1_EXTERNAL_BANKS_MASK) >> EBIU_DDRCTL1_EXTERNAL_BANKS_SHIFT	)
#define EBIU_DDRCTL1_EXTERNAL_BANKS_SET(R,V)  ( ( (R) = (V) << EBIU_DDRCTL1_EXTERNAL_BANKS_SHIFT ) | ( (R) & ~EBIU_DDRCTL1_EXTERNAL_BANKS_MASK ) )

#define EBIU_DDRCTL1_DEVICE_WIDTH_MASK        0x0F0000
#define EBIU_DDRCTL1_DEVICE_WIDTH_SHIFT       16
#define EBIU_DDRCTL1_DEVICE_WIDTH_GET(R)	    ( ( (R) & EBIU_DDRCTL1_DEVICE_WIDTH_MASK) >> EIU_DDRCTL1_DEVICE_WIDTH_SHIFT )
#define EBIU_DDRCTL1_DEVICE_WIDTH_SET(R,V)    ( ( (R) = (V) << EBIU_DDRCTL1_DEVICE_WIDTH_SHIFT ) | ( (R) & ~EBIU_DDRCTL1_DEVICE_WIDTH_MASK ) )

#define EBIU_DDRCTL1_DEVICE_SIZE_MASK         0x0C0000
#define EBIU_DDRCTL1_DEVICE_SIZE_SHIFT        18
#define EBIU_DDRCTL1_DEVICE_SIZE_GET(R)       ( ( (R) & EBIU_DDRCTL1_DEVICE_SIZE_MASK) >> EBIU_DDRCTL1_DEVICE_SIZE_SHIFT )
#define EBIU_DDRCTL1_DEVICE_SIZE_SET(R,V)     ( ( (R) = (V) << EBIU_DDRCTL1_DEVICE_SIZE_SHIFT ) | ( (R) & ~EBIU_DDRCTL1_DEVICE_SIZE_MASK ) )

#define EBIU_DDRCTL1_WTR_MASK                 0x30000000
#define EBIU_DDRCTL1_WTR_SHIFT                28
#define EBIU_DDRCTL1_WTR_GET(R)               ( ( (R) & EBIU_DDRCTL1_WTR_MASK) >> EBIU_DDRCTL1_WTR_SHIFT )
#define EBIU_DDRCTL1_WTR_SET(R,V)             ( ( (R) = (V) << EBIU_DDRCTL1_WTR_SHIFT ) | ( (R) & ~EBIU_DDRCTL1_WTR_MASK ) )

 
/* control register 2 commands (support only setting of CAS) */
 
#define EBIU_DDRCTL2_CAS_MASK            0x070
#define EBIU_DDRCTL2_CAS_SHIFT           4
#define EBIU_DDRCTL2_CAS_GET(R)          ( ( (R) & EBIU_DDRCTL2_CAS_MASK) >> EBIU_DDRCTL2_CAS_SHIFT )
#define EBIU_DDRCTL2_CAS_SET(R,V)        ( ( (R) = (V) << EBIU_DDRCTL2_CAS_SHIFT ) | ( (R) & ~EBIU_DDRCTL2_CAS_MASK ) )



 
/*   commands for mobile mode */


#define EBIU_DDRCTL3_PASR_MASK            0x07
#define EBIU_DDRCTL3_PASR_SHIFT           0
#define EBIU_DDRCTL3_PASR_GET(R)          ( ( (R) & EBIU_DDRCTL3_PASR_MASK) >> EBIU_DDRCTL3_PASR_SHIFT )
#define EBIU_DDRCTL3_PASR_SET(R,V)        ( ( (R) = (V) << EBIU_DDRCTL3_PASR_SHIFT ) | ( (R) & ~EBIU_DDRCTL3_PASR_MASK ) )


 
/* NOTE:  DS is not changeable for either mobile or regular mode */

/* commands for non-mobile mode */

#define EBIU_DDRCTL3_DLL_DISABLE_MASK                1
#define EBIU_DDRCTL3_DLL_DISABLE_SHIFT               0
#define EBIU_DDRCTL3_DLL_DISABLE_GET(R)              ( ( (R) & EBIU_DDRCTL3_DLL_DISABLE_MASK) >> EBIU_DDRCTL3_DLL_DISABLE_SHIFT )
#define EBIU_DDRCTL3_DLL_DISABLE_SET(R,V)            ( ( (R) = (V) << EBIU_DDRCTL3_DLL_DISABLE_SHIFT ) | ( (R) & ~EBIU_DDRCTL3_DLL_DISABLE_MASK ) )

/* control register 4 (reset control register) commands */

#define EBIU_DDRCTL4_SOFT_RESET_MASK                 0x01
#define EBIU_DDRCTL4_SOFT_RESET_SHIFT                0
#define EBIU_DDRCTL4_SOFT_RESET_GET(R)               ( ( (R) & EBIU_DDRCTL4_SOFT_RESET_MASK) >> EBIU_DDRCTL4_SOFT_RESET_SHIFT )
#define EBIU_DDRCTL4_SOFT_RESET_SET(R,V)             ( ( (R) = (V) << EBIU_DDRCTL4_SOFT_RESET_SHIFT ) | ( (R) & ~EBIU_DDRCTL4_SOFT_RESET_MASK ) )

#define EBIU_DDRCTL4_SELF_REFRESH_REQUEST_MASK       0x08
#define EBIU_DDRCTL4_SELF_REFRESH_REQUEST_SHIFT      3
#define EBIU_DDRCTL4_SELF_REFRESH_REQUEST_GET(R)     ( ( (R) & EBIU_DDRCTL4_SELF_REFRESH_REQUEST_MASK) >> EBIU_DDRCTL4_SELF_REFRESH_REQUEST_SHIFT )
#define EBIU_DDRCTL4_SELF_REFRESH_REQUEST_SET(R,V)   ( ( (R) = (V) << EBIU_DDRCTL4_SELF_REFRESH_REQUEST_SHIFT ) | ( (R) & ~EBIU_DDRCTL4_SELF_REFRESH_REQUEST_MASK ) )

/* read-only self-refresh ACK register */
#define EBIU_DDRCTL4_SELF_REFRESH_ACK_MASK           0x010
#define EBIU_DDRCTL4_SELF_REFRESH_ACK_SHIFT          4
#define EBIU_DDRCTL4_SELF_REFRESH_ACK_GET(R)         ( ( (R) & EBIU_DDRCTL4_SELF_REFRESH_ACK_MASK) >> EBIU_DDRCTL4_SELF_REFRESH_ACK_SHIFT )
 

#define EBIU_DDRCTL4_MOBILE_DDR_ENABLE_MASK      0x020
#define EBIU_DDRCTL4_MOBILE_DDR_ENABLE_SHIFT     5
#define EBIU_DDRCTL4_MOBILE_DDR_ENABLE_GET(R)	 ( ( (R) & EBIU_DDRCTL4_MOBILE_DDR_ENABLE_MASK) >> EBIU_DDRCTL4_MOBILE_DDR_ENABLE_SHIFT )
#define EBIU_DDRCTL4_MOBILE_DDR_ENABLE_SET(R,V)  ( ( (R) = (V) << EBIU_DDRCTL4_MOBILE_DDR_ENABLE_SHIFT ) | ( (R) & ~EBIU_DDRCTL4_MOBILE_DDR_ENABLE_MASK ) )
	
/* Queue configuration commands and Error register commands have been removed and will be added back
  if deemed necessary */

 
 
#endif /* whether or not bit fields supported */



#define ADI_CRITICAL_SEMAPHORE NULL

void adi_ebiu_SetKnownState(void);

// prototypes for assembler functions
void adi_ebiu_DDRSelfRefreshEnable(void);
void adi_ebiu_DDRSelfRefreshDisable(void);



/* This ends the conditional compilation based on whether DDR memory is supported */
#endif   /* DDR memory */




#endif //  !defined(_LANGUAGE_ASM)

#endif // __ADI_EBIU_MODULE_H__

