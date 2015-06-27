/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_pwr_module.h,v $
$Revision: 2386 $
$Date: 2010-03-25 17:26:08 -0400 (Thu, 25 Mar 2010) $

Description:
            Power Management Header for use by module functions only

*********************************************************************************/

#ifndef __ADI_PWR_MODULE_H__
#define __ADI_PWR_MODULE_H__
 
#include <services/services.h>

#if  !defined(__ADSP_TETON__) && !defined(__ADSP_EDINBURGH__) && !defined(__ADSP_BRAEMAR__) && !defined(__ADSP_STIRLING__)
#include <bfrom.h>
#endif

#if !defined(__ADSP_BRAEMAR__) && !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__)
#include <xcycle_count.h>
#else

#if !defined(_LANGUAGE_ASM)
extern volatile int _Processor_cycles_per_sec;
#endif

#endif

#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__) /* no EBIU */
#include "../ebiu/adi_ebiu_module.h"
#endif

#if !defined(_LANGUAGE_ASM)

#define U32_REG_CAST(V) (*(u32*)&(V))
#define U16_REG_CAST(V) (*(u16*)&(V))


/********************************************************************************
* Core Synchronisation
*********************************************************************************/

/********************************************************************************
* Synchronisation commands (not currently used)
* the 'or' of these means both 
*********************************************************************************/
#define ADI_PWR_CORE_SYNC_PLL  0
#define ADI_PWR_CORE_SYNC_EBIU 1
#define ADI_PWR_CORE_SYNC_EBIU_FIRST 3
#define ADI_PWR_CORE_SYNC_EBIU_LAST 5

/* maximum number of client callbacks that can be registered */
#define ADI_PWR_MAX_SCK_CALLBACKS 8

void
adi_pwr_SyncCoreB(testset_t *pLockVar);    // Synchronise with another core

void 
adi_pwr_ReleaseCoreB(testset_t *pLockVar); // releases core B to return to normal execution

void 
adi_pwr_SetLockVarPointer(
        void *pLockVar        // pointer to lock variable in L2 shared memory 
        );
        
#if defined(__ECC__) // VisualDSP C Compiler
#define __ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__
#endif

/********************************************************************************
* PLL Control Register (PLL_CTL)
*********************************************************************************/
#ifdef __ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__

typedef struct ADI_PWR_PLL_CTL_REG {
    u16 b_DF:1;
    u16 b_PLL_OFF:1;
    u16 bit2:1;
    u16 b_STOPCK:1;
    u16 bit4:1;
    u16 b_PDWN:1;
    u16 b_INPUT_DELAY:1;
    u16 b_OUTPUT_DELAY:1;
    u16 b_BYPASS:1;
    u16 b_MSEL:6;
    u16 bit15:1;
} ADI_PWR_PLL_CTL_REG; 

#else
typedef u16 ADI_PWR_PLL_CTL_REG;
#endif

/********************************************************************************
* PLL Divide Register (PLL_DIV)
*********************************************************************************/
#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__)
typedef struct ADI_PWR_PLL_DIV_REG {
    u16 b_SSEL:4;
    u16 b_CSEL:2;
    u16 unused:10;
} ADI_PWR_PLL_DIV_REG; 
#else
typedef u16 ADI_PWR_PLL_DIV_REG;
#endif

/********************************************************************************
* Voltage Regulator Control Register (VR_CTL)
*********************************************************************************/
#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__)

#if defined(__ADSP_BRAEMAR__) 

typedef struct ADI_PWR_VR_CTL_REG {
    u16 b_FREQ:2;
    u16 b_GAIN:2;
    u16 b_VLEV:4;
    u16 b_WAKE:1;
    u16 b_PHYWE:1;
    u16 b_CANWE:1;
    u16 unused:3;
    u16 b_CLKBUFOE:1;
    u16 b_CKELOW:1;
} ADI_PWR_VR_CTL_REG; 


#elif defined(__ADSP_MOAB__)


typedef struct ADI_PWR_VR_CTL_REG {
    u16 b_FREQ:2;
    u16 b_GAIN:2;
    u16 b_VLEV:4;
    u16 b_WAKE:1;
    u16 b_CANWE:1;    
    u16 b_GPWE_MXVRWE:1;  
    u16 b_USBWE:1;
    u16 b_KPADWE:1;
    u16 b_ROTWE:1;
    u16 b_CLKBUFOE:1;
    u16 b_CKELOW:1;
} ADI_PWR_VR_CTL_REG; 


#elif defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__)

typedef struct ADI_PWR_VR_CTL_REG {
    u16 b_VTRIM:4;
    u16 b_VLEV:4;
    u16 b_WAKE:1;
    u16 b_USBWE:1;
    u16 b_PHYWE:1;
    u16 unused:1;
    u16 b_FREQ:2;
    u16 b_CLKBUFOE:1;
    u16 b_CKELOW:1;
} ADI_PWR_VR_CTL_REG; 

#elif defined(__ADSP_MOY__)
typedef struct ADI_PWR_VR_CTL_REG {
    u16 b_VTRIM:4;
    u16 b_unused1:4;  /* only on products which suppot internal VR */
    u16 b_WAKE:1;  /* bit 8 = wake */
    u16 b_unused2:1;  /* bit 9= reserved */
    u16 b_PHYWE:1;    /* bit 10 PHTWE */
    u16 unused3:1;    /* bit 11 reserved */
    u16 b_FREQ:2;      /* bits 12,13 freq bits */
    u16 b_CLKBUFOE:1;  /* bit 14 CLK BUFF OE */
    u16 b_CKELOW:1;    /* bit 15 SCKE low */
} ADI_PWR_VR_CTL_REG; 


#elif !defined(__ADSPBF535__)

typedef struct ADI_PWR_VR_CTL_REG {
    u16 b_FREQ:2;
    u16 b_GAIN:2;
    u16 b_VLEV:4;
    u16 b_WAKE:1;
    u16 unused:7;
} ADI_PWR_VR_CTL_REG; 

#endif

#else
typedef u16 ADI_PWR_VR_CTL_REG;
#endif

typedef u16 ADI_PWR_PLL_LOCKCNT_REG;

/********************************************************************************
* PLL Status Register (PLL_STAT) - READ-ONLY
*********************************************************************************/
#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__)
typedef struct ADI_PWR_PLL_STAT_REG {
    u16 b_ACTIVE_PLL_ENABLED:1;
    u16 b_FULL_ON:1;
    u16 b_ACTIVE_PLL_DISABLED:1;
    u16 b_DEEP_SLEEP:1;
    u16 b_SLEEP:1;
    u16 b_PLL_LOCKED:1;
    u16 b_CORE_IDLE:1;
    u16 b_VSTAT:1;
    u16 unused:8;
} ADI_PWR_PLL_STAT_REG; 
#else
typedef u16 ADI_PWR_PLL_STAT_REG;
#endif




/********************************************************************************
* POWER CONFIGURATION State structure
*  Revised March 7, 2007 - Initialized flag at head of structure for easy 
*  initialization when an instance is declared, of just the flag, rest of 
*  the structure is initialized at run time, in adi_pwr_init.
*********************************************************************************/

#pragma pack(4)
typedef struct ADI_PWR_CONFIG {
    u32                         Initialized;        // Flag to lock out further calls to adi_pwr_Init()
    ADI_PWR_PLL_CTL_REG         pll_ctl;            // Current PLL control reg settings - used only for Load/Save ops
    ADI_PWR_PLL_DIV_REG         pll_div;            // Current PLL divider reg settings - used only for Load/Save ops
    ADI_PWR_VR_CTL_REG            vr_ctl;             // Current VR control reg settings - used only for Load/Save ops
    ADI_PWR_PLL_STAT_REG        pll_stat;            // Current PLL status reg settings - used only for Load/Save ops
    ADI_PWR_PLL_LOCKCNT_REG        pll_lockcnt;        // Current PLL lock count reg settings
    ADI_PWR_PROC_KIND            proc_type;            // Processor type
    ADI_PWR_PACKAGE_KIND        package_type;        // Packaging - determines SCLK limits
    u32                         clkin;                // board CLKIN value
    
    ADI_PWR_VLEV                MaxCoreVoltage;        // Core voltage
    ADI_PWR_VDDEXT              ExternalVoltage;    // External voltage level
    u32                         IVG;
    u32                         MHzFactor;            // Factor to be applied to frequencies (1 if MHz is unit , 1000000 if Hz is unit)
    u32                         fsclk;                // Current SCLK frequency value

    u32                         ClientCLKCallbackCount; 
    ADI_PWR_CALLBACK_ENTRY      ClientCallbackTable[ADI_PWR_MAX_SCK_CALLBACKS];  
    
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__) /* no EBIU */
    ADI_EBIU_CONFIG                ebiu_config;        // EBIU settings
#endif
    
#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)
    u32                         auto_sync_enabled;  // Auto synchronisation of cores
    u32                         IVG_SuppInt0;        // IVG level for Core B supplemental interrupt.
    testset_t                   *pLockVar;            // pointer to lock variable
#endif
#if defined(__ADSP_BRAEMAR__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)
    u16                         PC133_compliant;    // flag to indicate CKE status on reset 
#endif
} ADI_PWR_CONFIG;


#pragma pack()

#endif   // !defined(_LANGUAGE_ASM)

// register MMR variables

#if defined(__ECC__) || defined (__ghs__)
#define PLL_CTL_REG_MMR        (PLL_CTL)
#define PLL_DIV_REG_MMR        (PLL_DIV)
#define PLL_STAT_REG_MMR    (PLL_STAT)
#define PLL_LOCKCNT_REG_MMR (PLL_LOCKCNT)
#define VR_CTL_REG_MMR        (VR_CTL)

#if defined(__ADSPBF561__)
// System Reset Configuration Register (supplemental interrupt) MMR
#define SICB_SYSCR_MMR        (SICB_SYSCR)
#endif

#else // if defined(__GNUC__)

#define PLL_CTL_REG_MMR            //TBD
#define PLL_DIV_REG_MMR            //TBD
#define PLL_STAT_REG_MMR        //TBD
#define PLL_LOCKCNT_REG_MMR        //TBD
#define VR_CTL_REG_MMR            //TBD

#if defined(__ADSPBF561__)
// System Reset Configuration Register (supplemental interrupt) MMR
#define SICB_SYSCR_MMR            (0xFFC01104)
#endif

#endif

#if !defined(_LANGUAGE_ASM)

#if defined(__ADI_SERVICES_BITFIELD_STRUCTS_AVAILABLE__) // eg. in VisualDSP C Compiler

#if 0 // template: copy and change
#define REGNAME_FIELD_GET(R)    ( (R).b_FIELD) )
#define REGNAME_FIELD_SET(R,V)     ( REGNAME_FIELD_GET(R) = (V) )
#endif

// PLL CONTROL REGISTER

#define PLL_CTL_DF_GET(R)                ( (R).b_DF )
#define PLL_CTL_DF_SET(R,V)             ( PLL_CTL_DF_GET(R) = (V) )

#define PLL_CTL_PLL_OFF_GET(R)            ( (R).b_PLL_OFF )
#define PLL_CTL_PLL_OFF_SET(R,V)         ( PLL_CTL_PLL_OFF_GET(R) = (V) )

#define PLL_CTL_STOPCK_GET(R)            ( (R).b_STOPCK )
#define PLL_CTL_STOPCK_SET(R,V)         ( PLL_CTL_STOPCK_GET(R) = (V) )

#define PLL_CTL_PDWN_GET(R)                ( (R).b_PDWN )
#define PLL_CTL_PDWN_SET(R,V)             ( PLL_CTL_PDWN_GET(R) = (V) )

#define PLL_CTL_INPUT_DELAY_GET(R)        ( (R).b_INPUT_DELAY )
#define PLL_CTL_INPUT_DELAY_SET(R,V)     ( PLL_CTL_INPUT_DELAY_GET(R) = (V) )

#define PLL_CTL_OUTPUT_DELAY_GET(R)        ( (R).b_OUTPUT_DELAY )
#define PLL_CTL_OUTPUT_DELAY_SET(R,V)     ( PLL_CTL_OUTPUT_DELAY_GET(R) = (V) )

#define PLL_CTL_BYPASS_GET(R)            ( (R).b_BYPASS )
#define PLL_CTL_BYPASS_SET(R,V)         ( PLL_CTL_BYPASS_GET(R) = (V) )

#define PLL_CTL_MSEL_GET(R)                ( (R).b_MSEL )
#define PLL_CTL_MSEL_SET(R,V)             ( PLL_CTL_MSEL_GET(R) = (V) )

// PLL DIVIDE REGISTER

#define PLL_DIV_SSEL_GET(R)                ( (R).b_SSEL )
#define PLL_DIV_SSEL_SET(R,V)             ( PLL_DIV_SSEL_GET(R) = (V) )

#define PLL_DIV_CSEL_GET(R)                ( (R).b_CSEL )
#define PLL_DIV_CSEL_SET(R,V)             ( PLL_DIV_CSEL_GET(R) = (V) )

// PLL STATUS REGISTER

#define PLL_STAT_FULL_ON_GET(R)                ( (R).b_FULL_ON )
#define PLL_STAT_ACTIVE_PLL_ENABLED_GET(R)    ( (R).b_ACTIVE_PLL_ENABLED )
#define PLL_STAT_ACTIVE_PLL_DISABLED_GET(R)    ( (R).b_ACTIVE_PLL_DISABLED )
#define PLL_STAT_SLEEP_GET(R)                ( (R).b_SLEEP )
#define PLL_STAT_DEEP_SLEEP_GET(R)            ( (R).b_DEEP_SLEEP )
#define PLL_STAT_SSEL_GET(R)                ( (R).b_SSEL )
#define PLL_STAT_SSEL_GET(R)                ( (R).b_SSEL )

// VOLTAGE REGULATOR CONTROL REGISTER

#define VR_CTL_FREQ_GET(R)                ( (R).b_FREQ )
#define VR_CTL_FREQ_SET(R,V)             ( VR_CTL_FREQ_GET(R) = (V) )

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__) 
#define VR_CTL_VTRIM_GET(R)                ( (R).b_VTRIM )
#define VR_CTL_VTRIM_SET(R,V)             ( VR_CTL_VTRIM_GET(R) = (V) )
#else
#define VR_CTL_GAIN_GET(R)                ( (R).b_GAIN )
#define VR_CTL_GAIN_SET(R,V)             ( VR_CTL_GAIN_GET(R) = (V) )
#endif

#define VR_CTL_VLEV_GET(R)                ( (R).b_VLEV )
#define VR_CTL_VLEV_SET(R,V)             ( VR_CTL_VLEV_GET(R) = (V) )

#define VR_CTL_WAKE_GET(R)                ( (R).b_WAKE )
#define VR_CTL_WAKE_SET(R,V)             ( VR_CTL_WAKE_GET(R) = (V) )

#if defined(__ADSP_BRAEMAR__)  || defined(__ADSP_MOAB__)
#define VR_CTL_CANWE_GET(R)                ( (R).b_CANWE )
#define VR_CTL_CANWE_SET(R,V)             ( VR_CTL_CANWE_GET(R) = (V) )
#endif

#if  defined(__ADSP_MOAB__)
#define VR_CTL_GPWE_MXVRWE_GET(R)                ( (R).b_GPWE_MXVRWE )
#define VR_CTL_GPWE_MXVRWE_SET(R,V)             ( VR_CTL_GPWE_MXVRWE_GET(R) = (V) )
#endif

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_MOAB__) 
#define VR_CTL_USBWE_GET(R)                ( (R).b_USBWE )
#define VR_CTL_USBWE_SET(R,V)             ( VR_CTL_USBWE_GET(R) = (V) )
#endif

#if defined(__ADSP_BRAEMAR__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__) 

#define VR_CTL_PHYWE_GET(R)                ( (R).b_PHYWE )
#define VR_CTL_PHYWE_SET(R,V)             ( VR_CTL_PHYWE_GET(R) = (V) )

#endif 

#if defined(__ADSP_BRAEMAR__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_MOAB__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__) 


#define VR_CTL_CLKBUFOE_GET(R)            ( (R).b_CLKBUFOE )
#define VR_CTL_CLKBUFOE_SET(R,V)         ( VR_CTL_CLKBUFOE_GET(R) = (V) )

#define VR_CTL_CKELOW_GET(R)            ( (R).b_CKELOW )
#define VR_CTL_CKELOW_SET(R,V)             ( VR_CTL_CKELOW_GET(R) = (V) )

#endif  /* Kook or Braemar or Moab */



#else // Compilers not supporting bitfield structs

#if 0 // template: copy and change
#define REGNAME_FIELD_MASK
#define REGNAME_FIELD_SHIFT
#define REGNAME_FIELD_SET(R,V)     ( ( (R) = (V) << REGNAME_FIELD_SHIFT ) | ( (R) & ~REGNAME_FIELD_MASK ) )
#define REGNAME_FIELD_GET(R)    ( ( (R) & REGNAME_FIELD_MASK ) >> REGNAME_FIELD_SHIFT )
#endif

// PLL CONTROL REGISTER

#define PLL_CTL_DF_MASK                    0x0001
#define PLL_CTL_DF_SHIFT                0
#define PLL_CTL_DF_SET(R,V)             ( (R) = ( (V) << PLL_CTL_DF_SHIFT ) | ( (R) & ~PLL_CTL_DF_MASK ) )
#define PLL_CTL_DF_GET(R)                ( ( (R) & PLL_CTL_DF_MASK ) >> PLL_CTL_DF_SHIFT )

#define PLL_CTL_PLL_OFF_MASK            0x0002
#define PLL_CTL_PLL_OFF_SHIFT            1
#define PLL_CTL_PLL_OFF_SET(R,V)         ( (R) = ( (V) << PLL_CTL_PLL_OFF_SHIFT ) | ( (R) & ~PLL_CTL_PLL_OFF_MASK ) )
#define PLL_CTL_PLL_OFF_GET(R)            ( ( (R) & PLL_CTL_PLL_OFF_MASK ) >> PLL_CTL_PLL_OFF_SHIFT )

#define PLL_CTL_STOPCK_MASK                0x0008
#define PLL_CTL_STOPCK_SHIFT            3
#define PLL_CTL_STOPCK_SET(R,V)         ( (R) = ( (V) << PLL_CTL_STOPCK_SHIFT ) | ( (R) & ~PLL_CTL_STOPCK_MASK ) )
#define PLL_CTL_STOPCK_GET(R)            ( ( (R) & PLL_CTL_STOPCK_MASK ) >> PLL_CTL_STOPCK_SHIFT )

#define PLL_CTL_PDWN_MASK                0x0020
#define PLL_CTL_PDWN_SHIFT                5
#define PLL_CTL_PDWN_SET(R,V)             ( (R) = ( (V) << PLL_CTL_PDWN_SHIFT ) | ( (R) & ~PLL_CTL_PDWN_MASK ) )
#define PLL_CTL_PDWN_GET(R)                ( ( (R) & PLL_CTL_PDWN_MASK ) >> PLL_CTL_PDWN_SHIFT )

#define PLL_CTL_INPUT_DELAY_MASK        0x0040
#define PLL_CTL_INPUT_DELAY_SHIFT        6
#define PLL_CTL_INPUT_DELAY_SET(R,V)     ( (R) = ( (V) << PLL_CTL_INPUT_DELAY_SHIFT ) | ( (R) & ~PLL_CTL_INPUT_DELAY_MASK ) )
#define PLL_CTL_INPUT_DELAY_GET(R)        ( ( (R) & PLL_CTL_INPUT_DELAY_MASK ) >> PLL_CTL_INPUT_DELAY_SHIFT )

#define PLL_CTL_OUTPUT_DELAY_MASK        0x0080
#define PLL_CTL_OUTPUT_DELAY_SHIFT        7
#define PLL_CTL_OUTPUT_DELAY_SET(R,V)     ( (R) = ( (V) << PLL_CTL_OUTPUT_DELAY_SHIFT ) | ( (R) & ~PLL_CTL_OUTPUT_DELAY_MASK ) )
#define PLL_CTL_OUTPUT_DELAY_GET(R)        ( ( (R) & PLL_CTL_OUTPUT_DELAY_MASK ) >> PLL_CTL_OUTPUT_DELAY_SHIFT )

#define PLL_CTL_BYPASS_MASK                0x0100
#define PLL_CTL_BYPASS_SHIFT            8
#define PLL_CTL_BYPASS_SET(R,V)         ( (R) = ( (V) << PLL_CTL_BYPASS_SHIFT ) | ( (R) & ~PLL_CTL_BYPASS_MASK ) )
#define PLL_CTL_BYPASS_GET(R)            ( ( (R) & PLL_CTL_BYPASS_MASK ) >> PLL_CTL_BYPASS_SHIFT )

#define PLL_CTL_MSEL_MASK                0x7E00
#define PLL_CTL_MSEL_SHIFT                9
#define PLL_CTL_MSEL_SET(R,V)             ( (R) = ( (V) << PLL_CTL_MSEL_SHIFT ) | ( (R) & ~PLL_CTL_MSEL_MASK ) )
#define PLL_CTL_MSEL_GET(R)                ( ( (R) & PLL_CTL_MSEL_MASK ) >> PLL_CTL_MSEL_SHIFT )

// PLL DIVIDE REGISTER

#define PLL_DIV_SSEL_MASK                0x000F
#define PLL_DIV_SSEL_SHIFT                0
#define PLL_DIV_SSEL_SET(R,V)             ( (R) = ( (V) << PLL_DIV_SSEL_SHIFT ) | ( (R) & ~PLL_DIV_SSEL_MASK ) )
#define PLL_DIV_SSEL_GET(R)                ( ( (R) & PLL_DIV_SSEL_MASK ) >> PLL_DIV_SSEL_SHIFT )

#define PLL_DIV_CSEL_MASK                0x0030
#define PLL_DIV_CSEL_SHIFT                4
#define PLL_DIV_CSEL_SET(R,V)             ( (R) = ( (V) << PLL_DIV_CSEL_SHIFT ) | ( (R) & ~PLL_DIV_CSEL_MASK ) )
#define PLL_DIV_CSEL_GET(R)                ( ( (R) & PLL_DIV_CSEL_MASK ) >> PLL_DIV_CSEL_SHIFT )

// PLL STATUS REGISTER

#define PLL_STAT_ACTIVE_PLL_ENABLED_MASK    0x0001
#define PLL_STAT_ACTIVE_PLL_ENABLED_SHIFT    0
#define PLL_STAT_ACTIVE_PLL_ENABLED_GET(R)    ( ( (R) & PLL_STAT_ACTIVE_PLL_ENABLED_MASK ) >> PLL_STAT_ACTIVE_PLL_ENABLED_SHIFT )

#define PLL_STAT_FULL_ON_MASK                0x0002
#define PLL_STAT_FULL_ON_SHIFT                1
#define PLL_STAT_FULL_ON_GET(R)                ( ( (R) & PLL_STAT_FULL_ON_MASK ) >> PLL_STAT_FULL_ON_SHIFT )

#define PLL_STAT_ACTIVE_PLL_DISABLED_MASK    0x0004
#define PLL_STAT_ACTIVE_PLL_DISABLED_SHIFT    2
#define PLL_STAT_ACTIVE_PLL_DISABLED_GET(R)    ( ( (R) & PLL_STAT_ACTIVE_PLL_DISABLED_MASK ) >> PLL_STAT_ACTIVE_PLL_DISABLED_SHIFT )

#define PLL_STAT_DEEP_SLEEP_MASK            0x0008
#define PLL_STAT_DEEP_SLEEP_SHIFT            3
#define PLL_STAT_DEEP_SLEEP_GET(R)            ( ( (R) & PLL_STAT_DEEP_SLEEP_MASK ) >> PLL_STAT_DEEP_SLEEP_SHIFT )

#define PLL_STAT_SLEEP_MASK                    0x0010
#define PLL_STAT_SLEEP_SHIFT                4
#define PLL_STAT_SLEEP_GET(R)                ( ( (R) & PLL_STAT_SLEEP_MASK ) >> PLL_STAT_SLEEP_SHIFT )

// VOLTAGE REGULATOR CONTROL REGISTER






#if defined(__ADSP_MOAB__)

#define VR_CTL_KPADWE_MASK                0x1000
#define VR_CTL_KPADWE_SHIFT                12
#define VR_CTL_KPADWE_SET(R,V)             ( (R) = ( (V) << VR_CTL_KPADWE_SHIFT ) | ( (R) & ~VR_CTL_KPADWE_MASK ) )
#define VR_CTL_KPADWE_GET(R)                ( ( (R) & VR_CTL_KPADWE_MASK ) >> VR_CTL_KPADWE_SHIFT )

#define VR_CTL_ROTWE_MASK                0x2000
#define VR_CTL_ROTWE_SHIFT                13
#define VR_CTL_ROTWE_SET(R,V)             ( (R) = ( (V) << VR_CTL_ROTWE_SHIFT ) | ( (R) & ~VR_CTL_ROTWE_MASK ) )
#define VR_CTL_ROTWE_GET(R)                ( ( (R) & VR_CTL_ROTWE_MASK ) >> VR_CTL_ROTWE_SHIFT )

#define VR_CTL_USBWE_MASK                0x0800
#define VR_CTL_USBWE_SHIFT                11
#define VR_CTL_USBWE_SET(R,V)             ( (R) = ( (V) << VR_CTL_USBWE_SHIFT ) | ( (R) & ~VR_CTL_USBWE_MASK ) )
#define VR_CTL_USBWE_GET(R)                ( ( (R) & VR_CTL_USBWE_MASK ) >> VR_CTL_USBWE_SHIFT )

#define VR_CTL_GPWE_MXVRWE_MASK                0x0400
#define VR_CTL_GPWE_MXVRWE_SHIFT                10
#define VR_CTL_GPWE_MXVRWE_SET(R,V)                 ( (R) = ( (V) << VR_CTL_GPWE_MXVRWE_SHIFT ) | ( (R) & ~VR_CTL_GPWE_MXVRWE_MASK ) )
#define VR_CTL_GPWE_MXVRWE_GET(R)                ( ( (R) & VR_CTL_GPWE_MXVRWE_MASK ) >> VR_CTL_GPWE_MXVRWE_SHIFT )

#define VR_CTL_CANWE_MASK                0x0200
#define VR_CTL_CANWE_SHIFT                9
#define VR_CTL_CANWE_SET(R,V)             ( (R) = ( (V) << VR_CTL_CANWE_SHIFT ) | ( (R) & ~VR_CTL_CANWE_MASK ) )
#define VR_CTL_CANWE_GET(R)                ( ( (R) & VR_CTL_CANWE_MASK ) >> VR_CTL_CANWE_SHIFT )



#endif  /* MOAB */


#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)

#define VR_CTL_FREQ_MASK                0x3000
#define VR_CTL_FREQ_SHIFT                12
#define VR_CTL_VTRIM_MASK                0x000F 
#define VR_CTL_VTRIM_SHIFT                0        

#define VR_CTL_VTRIM_SET(R,V)             ( (R) = ( (V) << VR_CTL_VTRIM_SHIFT ) | ( (R) & ~VR_CTL_VTRIM_MASK ) )
#define VR_CTL_VTRIM_GET(R)                ( ( (R) & VR_CTL_VTRIM_MASK ) >> VR_CTL_VTRIM_SHIFT )

#else  /* FREQ and GAIN on the other processors are still in bit positions 0,1; 2,3 */

#define VR_CTL_FREQ_MASK                0x0003
#define VR_CTL_FREQ_SHIFT                0
#define VR_CTL_GAIN_MASK                0x000C
#define VR_CTL_GAIN_SHIFT                2

#define VR_CTL_GAIN_SET(R,V)             ( (R) = ( (V) << VR_CTL_GAIN_SHIFT ) | ( (R) & ~VR_CTL_GAIN_MASK ) )
#define VR_CTL_GAIN_GET(R)                ( ( (R) & VR_CTL_GAIN_MASK ) >> VR_CTL_GAIN_SHIFT )

#endif  /* FREQ and GAIN */

#define VR_CTL_FREQ_SET(R,V)             ( (R) = ( (V) << VR_CTL_FREQ_SHIFT ) | ( (R) & ~VR_CTL_FREQ_MASK ) )
#define VR_CTL_FREQ_GET(R)                ( ( (R) & VR_CTL_FREQ_MASK ) >> VR_CTL_FREQ_SHIFT )
 
#define VR_CTL_VLEV_MASK                0x00F0
#define VR_CTL_VLEV_SHIFT                4

#define VR_CTL_VLEV_SET(R,V)             ( (R) = ( (V) << VR_CTL_VLEV_SHIFT ) | ( (R) & ~VR_CTL_VLEV_MASK ) )
#define VR_CTL_VLEV_GET(R)                ( ( (R) & VR_CTL_VLEV_MASK ) >> VR_CTL_VLEV_SHIFT )

#define VR_CTL_WAKE_MASK                0x0100
#define VR_CTL_WAKE_SHIFT                8
#define VR_CTL_WAKE_SET(R,V)             ( (R) = ( (V) << VR_CTL_WAKE_SHIFT ) | ( (R) & ~VR_CTL_WAKE_MASK ) )
#define VR_CTL_WAKE_GET(R)                ( ( (R) & VR_CTL_WAKE_MASK ) >> VR_CTL_WAKE_SHIFT )

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)


#define VR_CTL_PHYWE_MASK                0x0400
#define VR_CTL_PHYWE_SHIFT                10
#define VR_CTL_PHYWE_SET(R,V)             ( (R) = ( (V) << VR_CTL_PHYWE_SHIFT ) | ( (R) & ~VR_CTL_PHYWE_MASK ) )
#define VR_CTL_PHYWE_GET(R)                ( ( (R) & VR_CTL_PHYWE_MASK ) >> VR_CTL_PHYWE_SHIFT )
#define VR_CTL_USBWE_MASK                0x0200
#define VR_CTL_USBWE_SHIFT                9
#define VR_CTL_USBWE_SET(R,V)             ( (R) = ( (V) << VR_CTL_USBWE_SHIFT ) | ( (R) & ~VR_CTL_USBWE_MASK ) )
#define VR_CTL_USBWE_GET(R)                ( ( (R) & VR_CTL_USBWE_MASK ) >> VR_CTL_USBWE_SHIFT )
#endif  /* Kook */

#if defined(__ADSP_BRAEMAR__) 

#define VR_CTL_PHYWE_MASK                0x0200
#define VR_CTL_PHYWE_SHIFT                9
#define VR_CTL_PHYWE_SET(R,V)             ( (R) = ( (V) << VR_CTL_PHYWE_SHIFT ) | ( (R) & ~VR_CTL_PHYWE_MASK ) )
#define VR_CTL_PHYWE_GET(R)                ( ( (R) & VR_CTL_PHYWE_MASK ) >> VR_CTL_PHYWE_SHIFT )
#define VR_CTL_CANWE_MASK                0x0400
#define VR_CTL_CANWE_SHIFT                10
#define VR_CTL_CANWE_SET(R,V)             ( (R) = ( (V) << VR_CTL_CANWE_SHIFT ) | ( (R) & ~VR_CTL_CANWE_MASK ) )
#define VR_CTL_CANWE_GET(R)                ( ( (R) & VR_CTL_CANWE_MASK ) >> VR_CTL_CANWE_SHIFT )

#endif  /* Braemar */

#if defined(__ADSP_BRAEMAR__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_MOAB__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__) 

/* These bits exist in the same position for the above processors */

#define VR_CTL_CLKBUFOE_MASK            0x4000
#define VR_CTL_CLKBUFOE_SHIFT            14
#define VR_CTL_CLKBUFOE_SET(R,V)         ( (R) = ( (V) << VR_CTL_CLKBUFOE_SHIFT ) | ( (R) & ~VR_CTL_CLKBUFOE_MASK ) )
#define VR_CTL_CLKBUFOE_GET(R)            ( ( (R) & VR_CTL_CLKBUFOE_MASK ) >> VR_CTL_CLKBUFOE_SHIFT )
#define VR_CTL_CKELOW_MASK                0x8000
#define VR_CTL_CKELOW_SHIFT                15
#define VR_CTL_CKELOW_SET(R,V)             ( (R) = ( (V) << VR_CTL_CKELOW_SHIFT ) | ( (R) & ~VR_CTL_CKELOW_MASK ) )
#define VR_CTL_CKELOW_GET(R)            ( ( (R) & VR_CTL_CKELOW_MASK ) >> VR_CTL_CKELOW_SHIFT )

#endif  /* Braemar or Kook, Mock, Moy, Moab or Brodie */

#endif

// global pointers to the Dynamic Power management MMR's
static volatile ADI_PWR_PLL_CTL_REG        *pll_ctl_mmr     = (volatile ADI_PWR_PLL_CTL_REG *)(PLL_CTL_REG_MMR);
static volatile ADI_PWR_PLL_DIV_REG        *pll_div_mmr     = (volatile ADI_PWR_PLL_DIV_REG *)(PLL_DIV_REG_MMR);
static volatile ADI_PWR_PLL_STAT_REG    *pll_stat_mmr     = (volatile ADI_PWR_PLL_STAT_REG *)(PLL_STAT_REG_MMR);
static volatile ADI_PWR_PLL_LOCKCNT_REG *pll_lockcnt_mmr = (volatile ADI_PWR_PLL_LOCKCNT_REG *)(PLL_LOCKCNT_REG_MMR);
static volatile ADI_PWR_VR_CTL_REG        *vr_ctl_mmr         = (volatile ADI_PWR_VR_CTL_REG *)(VR_CTL_REG_MMR);

#if defined(__ADSPBF561__)
//global pointer to Core B's 
static volatile u16                        *sicb_syscr_mmr     = (volatile u16*)(SICB_SYSCR_MMR);
#endif

u16 adi_pwr_FindMaxFreqForVolt(ADI_PWR_VLEV VLEV_value, u16 data_MSEL);
void adi_pwr_program_pll( ADI_PWR_CONFIG *config,u32 IDLE_flag );
void adi_pwr_PLLProgrammingSequence(u16 pll_ctl);
u32 adi_pwr_GetSCLK(void);

void 
adi_pwr_SetPLL( 
        u32                msel,
        ADI_PWR_DF        df,
        ADI_PWR_CSEL    csel,
        u32                ssel,
        ADI_PWR_VLEV    vlev
);

void 
adi_pwr_ApplyConfig(u32 fcclk, u32 fsclk, u16 StartUp); 

void adi_pwr_pll_programming_sequence(void);
int adi_pwr_SetInitFlag(void);


typedef struct ADI_PWR_PKG_LIMITS {
    u32 sclk_max_level1;  // VDDINT < threshold
    u32 sclk_max_level2;  // VDDINT >= threshold
}ADI_PWR_PKG_LIMITS;

// MACRO to set the Cycle Count global for the RTL
#define SET_PROCESSOR_CYCLES_PER_SEC(C)        \
    if (adi_pwr_active_config.MHzFactor==1) \
        _Processor_cycles_per_sec = C * 1000000; \
    else \
        _Processor_cycles_per_sec = C; \

        
        
#else  // END OF C/C++ SECTION

/*********************************************************************************
* Assembler Section
*********************************************************************************/
#if defined(__ADSP_TETON__) || defined(__ADSP_EDINBURGH__) || defined(__ADSP_BRAEMAR__) || defined(__ADSP_STIRLING__)


// mask to set FULL ON mode
#define ADI_PWR_FULL_ON_MASK            0x7EC1
#define ADI_PWR_HIBERNATE_MASK            0xFFFC

// Register bitfields
// PLL_CTL
#define bBYPASS  8
#define bPLL_OFF 1
#define bSTOPCK  3
#define bPWDN    5

// VR_CTL
#define bFREQ0   0
#define bFREQ1   1
#define bWAKE    8
#if defined(__ADSP_BRAEMAR__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)
#define bCKELOW  15
#endif

// prototypes
.extern _adi_pwr_FindMaxFreqForVolt;
.type _adi_pwr_FindMaxFreqForVolt,STT_FUNC;

.extern _adi_pwr_PLLProgrammingSequence;
.type _adi_pwr_PLLProgrammingSequence,STT_FUNC;

.extern _adi_pwr_program_pll;
.type _adi_pwr_program_pll,STT_FUNC;

.extern _adi_pwr_SetInitFlag;
.type _adi_pwr_SetInitFlag,STT_FUNC;

.extern _adi_pwr_SyncCoreB;
.type _adi_pwr_SyncCoreB,STT_FUNC;

.extern _adi_pwr_GetSCLK;
.type _adi_pwr_GetSCLK,STT_FUNC;

#endif // processors with assembler API
#endif // END OF ASSEMBLER SECTION        
        
        
 
#endif // __ADI_PWR_MODULE_H__
