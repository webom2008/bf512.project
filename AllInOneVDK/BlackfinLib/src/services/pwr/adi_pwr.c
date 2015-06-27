/*********************************************************************************
* 
* Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. ADI Confidential. 
*
* adi_pwr.c
*
* Description:    Dynamic Power Management Module
* Copyright:    Analog Devices (c) 2004
* Owner:        Analog Devices

$RCSfile: adi_pwr.c,v $
$Revision: 5506 $
$Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $



*
*********************************************************************************/

#include "adi_pwr_module.h"



/***********************************************************************************
***********************************************************************************

                        static values

***********************************************************************************
************************************************************************************/

/********************************************************************************
* Permitted transitions between power modes
* Note:
* These truth values are slightly different from those found in the hardware reference
* manual, as they indicate the desired behaviour in the adi_pwr_SetPowerMode function.
* For example, on wakeup, SLEEP will transition automatically to either FULL_ON or  
* ACTIVE depending on the value ot the BYPASS bit - there is no need to call 
* adi_pwr_SetPowerMode in this case.
*********************************************************************************/
u32 ADI_ALLOWED_TRANSITIONS[ADI_PWR_NUM_STATES][3] = {
    /*            To   From:            FULL_ON    ACTIVE(EN)  ACTIVE(DIS)*/
    /* ADI_PWR_MODE_FULL_ON */            FALSE,    TRUE,    TRUE, 
    /* ADI_PWR_MODE_ACTIVE */             TRUE,     FALSE,   TRUE,         
    /* ADI_PWR_MODE_ACTIVE_PLLDISABLED */ TRUE,     TRUE,    FALSE,     
    /* ADI_PWR_MODE_SLEEP */              TRUE,     TRUE,    FALSE,     
    /* ADI_PWR_MODE_DEEP_SLEEP */         TRUE,     TRUE,    TRUE,     
    /* ADI_PWR_MODE_HIBERNATE */          TRUE,     TRUE,    TRUE     
};


/* MMR Pointer definitions */
#define pADI_PWR_PLL_CTL     (volatile u16*) (0xFFC00000)
#define pADI_PWR_PLL_DIV     (volatile u16*) (0xFFC00004)
#define pADI_PWR_VR_CTL      (volatile u16*) (0xFFC00008)
#define pADI_PWR_PLL_STAT    (volatile u16*) (0xFFC0000C)
#define pADI_PWR_PLL_LOCKCNT (volatile u16*) (0xFFC00010)


/*  presently supports the bfrom_SysControl ROM function. */
#if defined(__ADSP_MOAB__)  
/* Flags that determine what SysControl actions to take */
static u16  SysControlActionFlag;


/* This is the SysControl structure which contains the register values */
ADI_SYSCTRL_VALUES adi_pwr_SysControlSettings;
ADI_SYSCTRL_VALUES* pAdi_pwr_SysControlSettings = &adi_pwr_SysControlSettings;


#endif   


/* This function will call SysControl ot adi_pwr_program_pll or both, depending on the proc type and action flags */
u32 adi_pwr_WritePMRegs(u16);

/* This function executes any user callbacks that are installed, to indicate that SCLK has changed */
void adi_pwr_ExecuteCallbacks(u32);


/********************************************************************************
*********************************************************************************
*
*                    DATASHEET LIMITS ON SCLK AND CCLK
*
*********************************************************************************
*********************************************************************************/

#if defined(__ADSP_EDINBURGH__)

/*                        ===============================
                        EDINBURGH CORES - 531, 532, 533
                        ===============================
*/

/********************************************************************************
* Minimum VCO frequency
*********************************************************************************/

#define ADI_PWR_FVCO_MIN  50
#define ADI_PWR_CLKIN_MIN 10000000
#define ADI_PWR_CLKIN_MAX 40000000

/********************************************************************************
* SCLK limits due to packaging, external and internal voltage levels
*********************************************************************************/

/* point at which we switch from level 1 to 2 */
#define ADI_PWR_VLEV_SCLK_THRESHOLD ADI_PWR_VLEV_115

static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits_datasheet[ADI_PWR_NUM_PKG_KINDS][2] = {
     /*        VDDEXT=3.3V          2.5V     */
    /* MBGA */   { {101, 134}, {101, 134} },
    /* LQFP */   { {84,  134}, {84,  134} } 
    /* Values are datasheet MHz values + 1, to enable rounding to yield max values */
};

/********************************************************************************
* Maximum Core Clock Speed versus Voltage Level of internal voltage regulator
*********************************************************************************/

/* datasheet values */

static u32 adi_pwr_cclk_vlev_datasheet [ADI_PWR_TOT_VLEVS][5] = {
    /*              SKBC750 SKBC600  SBBC500    531/2   SKBC-6V   */
    /* ADI_PWR_VLEV_085 */    { 250,    250,    250,    250,    250    },
    /* ADI_PWR_VLEV_090 */    { 376,    376,    333,    308,    376    },
    /* ADI_PWR_VLEV_095 */    { 426,    426,    400,    333,    426    },
    /* ADI_PWR_VLEV_100 */    { 426,    426,    400,    333,    426    },
    /* ADI_PWR_VLEV_105 */    { 476,    476,    400,    333,    476    },
    /* ADI_PWR_VLEV_110 */    { 476,    476,    444,    364,    476    },
    /* ADI_PWR_VLEV_115 */  { 476,    476,    500,    400,    476    },
    /* ADI_PWR_VLEV_120 */    { 600,    600,    500,    400,     500    },
    /* ADI_PWR_VLEV_125 */    { 600,    600,    500,    400,    500    },
    /* ADI_PWR_VLEV_130 */  { 600,    600,    500,    400,    600    },
    /* ADI_PWR_VLEV_135 */  { 600,    600,    500,    400,    600    },
    /* ADI_PWR_VLEV_140 */  { 756,    600,    500,    400,    600    }
};



#elif defined(__ADSP_TETON__)

/*                    ==========
                    BF561 CORE
                    ==========
*/


#define SYNC_COREB \
    if (adi_pwr_active_config.auto_sync_enabled) \
        adi_pwr_SyncCoreB(adi_pwr_active_config.pLockVar);
#define RELEASE_COREB \
    if (adi_pwr_active_config.auto_sync_enabled) \
        adi_pwr_ReleaseCoreB(adi_pwr_active_config.pLockVar);

/********************************************************************************
* Minimum VCO frequency
*********************************************************************************/

#define ADI_PWR_FVCO_MIN  50
#define ADI_PWR_CLKIN_MIN 10000000
#define ADI_PWR_CLKIN_MAX 40000000

/********************************************************************************
* SCLK limits due to packaging, external and internal voltage levels
*********************************************************************************/

/* point at which we switch from level 1 to 2 */
#define ADI_PWR_VLEV_SCLK_THRESHOLD ADI_PWR_VLEV_115

static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits_datasheet[ADI_PWR_NUM_PKG_KINDS][2] = {

    /*           VDDEXT=3.3V      2.5V    */

    /* MBGA */   { {101, 134}, {101, 134} },
    /* LQFP */   { {84,  134}, {84,  134} } 

};

/********************************************************************************
* Maximum Core Clock Speed versus Voltage Level of internal voltage regulator
*********************************************************************************/
/* datasheet values */

static u32 adi_pwr_cclk_vlev_datasheet [ADI_PWR_TOT_VLEVS][4] = {

/*       BF561SKBCZ-6A    BF561SKBCZ500X    BF561SKBCZ600X    BF561SBB600 */
     /* ADI_PWR_VLEV_085 */    {250,    250,    300,    250     },
     /* ADI_PWR_VLEV_090 */    {313,    300,    375,    300     },
     /* ADI_PWR_VLEV_095 */    {370,    313,    400,    313     },
     /* ADI_PWR_VLEV_100 */    {370,    350,    425,    350     },
     /* ADI_PWR_VLEV_105 */    {444,    400,    450,    400     },
     /* ADI_PWR_VLEV_110 */    {444,    444,    475,    444     },
     /* ADI_PWR_VLEV_115 */    {444,    450,    475,    450     },
     /* ADI_PWR_VLEV_120 */    {533,    475,    500,    475     },
     /* ADI_PWR_VLEV_125 */    {600,    500,    533,    500     },
     /* ADI_PWR_VLEV_130 */    {600,    500,    600,    600     },
};

/* core identification marcro */

#define core_ident (*ADI_SRAM_BASE_ADDR_MMR)

#elif defined(__ADSP_BRAEMAR__)

/*                =============================
                BREAMAR CORES - 534, 536, 537
                =============================
*/

/********************************************************************************
* Minimum VCO frequency
*********************************************************************************/

#define ADI_PWR_FVCO_MIN  50
#define ADI_PWR_CLKIN_MIN 10000000
#define ADI_PWR_CLKIN_MAX 50000000

/********************************************************************************
* SCLK limits due to packaging, external and internal voltage levels
*********************************************************************************/

/* point at which we switch from level 1 to 2 */
#define ADI_PWR_VLEV_SCLK_THRESHOLD ADI_PWR_VLEV_115

static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits_datasheet[ADI_PWR_NUM_PKG_KINDS][2] = {
    /*            VDDEXT=3.3V     2.5V     */

    /* MBGA */   { {101, 134}, {101, 134} },
    /* LQFP */   { {84,  134}, {84,  134} } 
};

static u32 adi_pwr_cclk_vlev_datasheet [ADI_PWR_TOT_VLEVS][5] = {
    /*                BF537SKBC1600    BF537SBBC1500    BF536SBBC1400    BF536SBBC1300 BF537BBCZ_5AV  */
    /* ADI_PWR_VLEV_085 */    {    250,    250,    250,    250,    250,    },
    /* ADI_PWR_VLEV_090 */    {    376,    334,    280,    300,    334,    },
    /* ADI_PWR_VLEV_095 */    {    426,    364,    301,    300,    364,    },
    /* ADI_PWR_VLEV_100 */    {    426,    400,    334,    300,    400,    },
    /* ADI_PWR_VLEV_105 */    {    476,    400,    334,    300,    400,    },
    /* ADI_PWR_VLEV_110 */    {    476,    444,    364,    300,    444,    },
    /* ADI_PWR_VLEV_115 */  {    476,    476,    400,    300,    476,    },
    /* ADI_PWR_VLEV_120 */    {    500,    500,    400,    300,    500,    },
    /* ADI_PWR_VLEV_125 */    {    533,    500,    400,    300,    533,    },
    /* ADI_PWR_VLEV_130 */  {    600,    500,    400,    300,    533,    },
    /* ADI_PWR_VLEV_135 */  {    600,    500,    400,    300,    533,    },
    /* ADI_PWR_VLEV_140 */  {    600,    500,    400,    300,    533,    }
};

#elif defined(__ADSP_STIRLING__)

/*                        =========================
                        STIRLING CORES - 538, 539
                        =========================
*/


/********************************************************************************
* Minimum VCO frequency
*********************************************************************************/

#define ADI_PWR_FVCO_MIN  50
#define ADI_PWR_CLKIN_MIN 10000000
#define ADI_PWR_CLKIN_MAX 50000000

/********************************************************************************
* SCLK limits due to packaging, external and internal voltage levels
*********************************************************************************/

/* point at which we switch from level 1 to 2 */
#define ADI_PWR_VLEV_SCLK_THRESHOLD ADI_PWR_VLEV_115

static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits_datasheet[ADI_PWR_NUM_PKG_KINDS][2] = {
/*  external voltage - VDDEXT  3.3V   2.5V   */
/*       only package type is MBGA */ 

            { {134, 134}, {134, 134} }

/* Values are datasheet MHz values + 1, to enable rounding to yield max values */

};

/********************************************************************************
* Maximum Core Clock Speed versus Voltage Level of internal voltage regulator
*********************************************************************************/

static u32 adi_pwr_cclk_vlev_datasheet [ADI_PWR_TOT_VLEVS][4] = {
/*    BF538BBCZ500    BF538BBCZ400    BF539BBCZ500    BF539BBCZ400     */
/* ADI_PWR_VLEV_080 */    {    250,    250,    250,    250    },
/* ADI_PWR_VLEV_085 */    {    334,    334,    334,    334    },
/* ADI_PWR_VLEV_090 */    {    376,    376,    376,    376    },
/* ADI_PWR_VLEV_095 */    {    400,    400,    400,    400    },
/* ADI_PWR_VLEV_100 */    {    422,    400,    422,    400    },
/* ADI_PWR_VLEV_105 */    {    445,    400,    445,    400    },
/* ADI_PWR_VLEV_110 */    {    463,    400,    463,    463    },
/* ADI_PWR_VLEV_115 */  {    481,    400,    481,    400    },
/* ADI_PWR_VLEV_120 */    {    500,    400,    500,    400    },
/* ADI_PWR_VLEV_125 */    {    500,    400,    500,    400    },
/* ADI_PWR_VLEV_130 */  {    500,    400,    500,    400    },
/* ADI_PWR_VLEV_135 */  {    500,    400,    500,    400    },
/* ADI_PWR_VLEV_140 */  {    500,    400,    500,    400    }
};






#elif defined(__ADSP_MOAB__)

/* **************************************************************************
 * MOAB CORES - 541, 542, 544, 547, 548, 549
 ***************************************************************************/


/********************************************************************************
* Minimum VCO frequency
*********************************************************************************/

#define ADI_PWR_FVCO_MIN  50
#define ADI_PWR_CLKIN_MIN 10000000
#define ADI_PWR_CLKIN_MAX 50000000

/********************************************************************************
* SCLK limits due to packaging, external and internal voltage levels
*********************************************************************************/

/* point at which we switch from level 1 to 2 */
#define ADI_PWR_VLEV_SCLK_THRESHOLD ADI_PWR_VLEV_120

static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits_datasheet[ADI_PWR_NUM_PKG_KINDS][2] =  
{
/* external voltage - VDDEXT   3.3V     2.5V     */
/* package type is MBGA                          */ 
                          { {101, 134}, {101, 134} }
/* Values are datasheet MHz values + 1, to enable rounding to yield max values */
                          
};
                     
                             
static u32 adi_pwr_cclk_vlev_datasheet [ADI_PWR_TOT_VLEVS][3] = 
{
/*   Internal VLEV     BF54XSKBC1600  BF54XSBBC1533  BF54XSBBC1400  */
/************* using preliminary values until data sheet is updated */

/* supporting 600MHz with external voltage regulation */

  

/* ADI_PWR_VLEV_095 */    { 376,  334,  301 },
/* ADI_PWR_VLEV_100 */    { 400,  400,  334 },
/* ADI_PWR_VLEV_105 */    { 423,  423,  350 },
/* ADI_PWR_VLEV_110 */    { 444,  444,  364 },
/* ADI_PWR_VLEV_115 */    { 476,  464,  376 },
/* ADI_PWR_VLEV_120 */  { 500,  500,  400 },
/* ADI_PWR_VLEV_125 */    { 533,  533,  400 },


/* with external VR */
/* ADI_PWR_VLEV_130 */  { 533,  533,  400 },
/* ADI_PWR_VLEV_135 */  { 600,  533,  400 },
/* ADI_PWR_VLEV_140 */  { 600,  533,  400 },


};



#elif defined(__ADSP_KOOKABURRA__)

/* **************************************************************************
 * KOOKABURRA CORES -   523,  525,   527
 ***************************************************************************/


/********************************************************************************
* Minimum VCO frequency
*********************************************************************************/

#define ADI_PWR_FVCO_MIN  50
#define ADI_PWR_CLKIN_MIN 10000000
#define ADI_PWR_CLKIN_MAX 50000000

/********************************************************************************
* SCLK limits due to packaging, external and internal voltage levels
*********************************************************************************/

/* point at which we switch from level 1 to 2 */
#define ADI_PWR_VLEV_SCLK_THRESHOLD ADI_PWR_VLEV_115

static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits_datasheet[ADI_PWR_NUM_PKG_KINDS][3] =  
{
/* external voltage - VDDEXT   3.3V     2.5V   1.8V  */
/* package type is MBGA                          */ 
                          { {101, 134}, {101, 134}, {101, 101} }
/* Values are datasheet MHz values + 1, to enable rounding to yield max values */
                          
};
                     
                             
static u32 adi_pwr_cclk_vlev_datasheet [ADI_PWR_TOT_VLEVS][2] = 
{

/*   Internal VLEV    BF527SBBC1600  BF527SBBC1533 BF527SBBC1400  */


/* ADI_PWR_VLEV_100 */    {  400,   400   },
/* ADI_PWR_VLEV_105 */    {  426,   426   },
/* ADI_PWR_VLEV_110 */    {  500,   500   },
/* ADI_PWR_VLEV_115 */    {  533,   533   },
/* ADI_PWR_VLEV_120 */  {  600,   533   },


};



#elif defined(__ADSP_MOCKINGBIRD__)

/* **************************************************************************
 * MOCKINGBIRD CORES - 522, 524,  526, 
 ***************************************************************************/


/********************************************************************************
* Minimum VCO frequency
*********************************************************************************/

#define ADI_PWR_FVCO_MIN  50
#define ADI_PWR_CLKIN_MIN 10000000
#define ADI_PWR_CLKIN_MAX 50000000

/********************************************************************************
* SCLK limits due to packaging, external and internal voltage levels
*********************************************************************************/

/* point at which we switch from level 1 to 2 - still TBD at this point */
#define ADI_PWR_VLEV_SCLK_THRESHOLD ADI_PWR_VLEV_115  

static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits_datasheet[ADI_PWR_NUM_PKG_KINDS][3] =  
{
/* external voltage - VDDEXT   3.3V     2.5V   1.8V   */
/* package type is MBGA                          */ 
                          { {81, 101}, {81, 101 }, {81, 81} }
/* Values are datasheet MHz values + 1, to enable rounding to yield max values */
                          
};
                     
                             
static u32 adi_pwr_cclk_vlev_datasheet [ADI_PWR_TOT_VLEVS][1] = 
{
/*   Internal VLEV     BF526SBBC1400 */

 
/* ADI_PWR_VLEV_115 */    {  276 },
/* ADI_PWR_VLEV_120 */  {  300 },
/* ADI_PWR_VLEV_125 */    {  334 },
/* ADI_PWR_VLEV_130 */  {  350 },
/* ADI_PWR_VLEV_135 */  {  376 },
/* ADI_PWR_VLEV_140 */  {  400 },
};



#elif defined(__ADSP_BRODIE__)

/* **************************************************************************
 * BRODIE CORES -   512,  514,   516
 ***************************************************************************/


/********************************************************************************
* Minimum VCO frequency
*********************************************************************************/

#define ADI_PWR_FVCO_MIN  50
#define ADI_PWR_CLKIN_MIN 10000000
#define ADI_PWR_CLKIN_MAX 50000000

/********************************************************************************
* SCLK limits due to packaging, external and internal voltage levels
*********************************************************************************/

/* point at which we switch from level 1 to 2 */
#define ADI_PWR_VLEV_SCLK_THRESHOLD ADI_PWR_VLEV_135

static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits_datasheet[ADI_PWR_NUM_PKG_KINDS][2] =  
{
/* external voltage - VDDEXT   3.3V/2.5V       1.8V   */
                              {{81, 101},      {81, 81}},
                              {{81, 101},      {81, 81}}
                          
/* Values are datasheet MHz values + 1, to enable rounding to yield max values */
                          
};
                     
                             
static u32 adi_pwr_cclk_vlev_datasheet [ADI_PWR_TOT_VLEVS][2] = 
{

/*   Internal VDDINT    BF512SBBC1300 BF512SBBC1400 */


/* ADI_PWR_VLEV_115 */    {  250,   276 },
/* ADI_PWR_VLEV_120 */    {  250,   300 },
/* ADI_PWR_VLEV_125 */    {  250,   334 },
/* ADI_PWR_VLEV_130 */    {  276,   376 },
/* ADI_PWR_VLEV_135 */    {  300,   400 },
/* ADI_PWR_VLEV_140 */    {  300,   400 },

};



#elif defined(__ADSP_MOY__)

/* **************************************************************************
 * MOY CORES - 504; 504F; 506F
 ***************************************************************************/


/********************************************************************************
* Minimum VCO frequency
*********************************************************************************/

#define ADI_PWR_FVCO_MIN  50
#define ADI_PWR_CLKIN_MIN 10000000
#define ADI_PWR_CLKIN_MAX 50000000

/********************************************************************************
* SCLK limits due to packaging, external and internal voltage levels
*********************************************************************************/

/* point at which we switch from level 1 to 2 - still TBD at this point */
#define ADI_PWR_VLEV_SCLK_THRESHOLD ADI_PWR_VLEV_115  

static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits_datasheet[ADI_PWR_NUM_PKG_KINDS][2] =  
{
/* package types for MOY are LFCSP, LQFP */
/* Maximum System Clock for all package types for all VDDEXT is 100 MHz */

                          { {101, 101}, {101, 101 }},
                          
/* Values are datasheet MHz values + 1, to enable rounding to yield max values */
                          { {101, 101}, {101, 101 } }
                                                    
};
                     
                             
static u32 adi_pwr_cclk_vlev_datasheet [ADI_PWR_TOT_VLEVS][1] = 
{
/*   Internal VLEV     BF506SBBC1400 */

 
/* ADI_PWR_VLEV_120 */    {  276 },
/* ADI_PWR_VLEV_125 */    {  300 },
/* ADI_PWR_VLEV_120 */    {  334 },
/* ADI_PWR_VLEV_135 */    {  376 },
/* ADI_PWR_VLEV_140 */    {  400 },

};


#elif defined(__ADSP_DELTA__)

/* **************************************************************************
 * DELTA CORES - 592
 ***************************************************************************/


/********************************************************************************
* Minimum VCO frequency
*********************************************************************************/

#define ADI_PWR_FVCO_MIN  50
#define ADI_PWR_CLKIN_MIN 10000000
#define ADI_PWR_CLKIN_MAX 50000000

/********************************************************************************
* SCLK limits due to packaging, external and internal voltage levels
*********************************************************************************/

/* point at which we switch from level 1 to 2 - still TBD at this point */
#define ADI_PWR_VLEV_SCLK_THRESHOLD ADI_PWR_VLEV_115  

static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits_datasheet[ADI_PWR_NUM_PKG_KINDS][2] =  
{
/* external voltage - VDDEXT   3.3V     2.5V   (1.8V???)   */
/* package type is MBGA                          */ 
                          { {84, 84}, {84, 84 } }
/* Values are datasheet MHz values + 1, to enable rounding to yield max values */
                          
};
                     
                             
static u32 adi_pwr_cclk_vlev_datasheet [ADI_PWR_TOT_VLEVS][1] = 
{
/*   Internal VLEV     BF506SBBC1400 */

 
/* ADI_PWR_VLEV_120 */    {  276 },
/* ADI_PWR_VLEV_125 */    {  300 },
/* ADI_PWR_VLEV_120 */    {  334 },
/* ADI_PWR_VLEV_135 */    {  376 },
/* ADI_PWR_VLEV_140 */    {  400 },

};

#endif  /* which set of cores */


/* optimal values derived in adi_pwr_Init */
static u32 adi_pwr_cclk_vlev [ADI_PWR_NUM_VLEVS];
static struct ADI_PWR_PKG_LIMITS adi_pwr_sclk_limits[ADI_PWR_NUM_PKG_KINDS][2];






/********************************************************************************
* POWER CONFIGURATION State instance
*  - we now only need to set the Initialized flag  
*********************************************************************************/
ADI_PWR_CONFIG adi_pwr_active_config = {
        0,                                      /* Initialized */
        /* ************************************************************************** */
        /* The remaining structure fields do not need to be initialised at this point */
        /* ************************************************************************** */
};



/***********************************************************************************
***********************************************************************************

                        static function prototypes

***********************************************************************************
************************************************************************************/
#if !defined(__ADSP_BRODIE__) && !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__) && !defined(__ADSP_MOAB__) && !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)
static int 
adi_pwr_set_ezkit_values(
        ADI_PWR_EZKIT revno
        );
#endif

static u32
adi_pwr_SelectOptimalMSEL(u32 *fvco, ADI_PWR_DF *df);

static u32
adi_pwr_SelectOptimalSSEL1(u32 fvco, u32 *fsclk);

static u32
adi_pwr_SelectOptimalSSEL2(u32 *fvco, u32 *fsclk);

static u32
adi_pwr_PowerSaving(u32 fcclk,ADI_PWR_VLEV vlev);

#if defined(__ADSP_TETON__)

/* The following definitions are required for dual core implementation 
   to synchronise the cores when changing the PLL settings */

/* address of System Reset Configuration register for Core B */
#define ADI_PWR_SICB_SYSCR ((volatile u16 *)0xFFC01104) 

/* Lock variable for core synchronization */
extern testset_t adi_pwr_lockvar;

/* prototype for Supplemental Interrupt Handler on core B */
ADI_INT_HANDLER(adi_pwr_CoreBSuppInt0Handler);

#endif

/***********************************************************************************
***********************************************************************************

                                MACROS

***********************************************************************************
************************************************************************************/
#define ADI_PWR_VOLTAGE_REGULATOR_IS_OFF    \
            (VR_CTL_FREQ_GET(adi_pwr_active_config.vr_ctl)==ADI_PWR_VR_FREQ_POWERDOWN)

#define ADI_PWR_CCLK_VIDX(V)                (V - ADI_PWR_VLEV_MIN)

#define ADI_PWR_CSEL_VALUE(C)                (1<<C)

/***********************************************************************************
***********************************************************************************

                                API Functions

***********************************************************************************
************************************************************************************/

/* *********************************************************************************
 * Function:    adi_pwr_Control    
 * Description:    Sets miscellaneous bits in control regs and queries status 
 * *********************************************************************************/

/* global flags to control recursive behaviour */

 
static u16 AdjustPLL;

static u16 PLL_prog_sequence;
static u32 RecursiveControlEntry=0;
static u16 vddint_set=0;

ADI_PWR_RESULT 
adi_pwr_Control(ADI_PWR_COMMAND command, void *value)
{
#ifdef ADI_SSL_DEBUG    
    ADI_PWR_RESULT result; 
#endif
    u32 ReturnCode = 0;
    AdjustPLL=FALSE;
    PLL_prog_sequence=0;
    
    /* miscellaneous counter */
    u32 i;
    
#if defined(__ADSP_TETON__)
    if (core_ident == ADI_COREA )    {
#endif

#if defined(__ADSP_MOAB__)

    SysControlActionFlag  = 0;      
    
#endif

    RecursiveControlEntry++;
    switch(command) 
    {
        default:
            RecursiveControlEntry--;
            return ADI_PWR_RESULT_BAD_COMMAND;

        case ADI_PWR_CMD_END:
            break;
        case ADI_PWR_CMD_PAIR:
            {
                ADI_PWR_RESULT result; 
                ADI_PWR_COMMAND_PAIR *cmd = (ADI_PWR_COMMAND_PAIR *)value;
#ifndef ADI_SSL_DEBUG
                adi_pwr_Control(cmd->kind, (void*)cmd->value);
#else
                if ( (result=adi_pwr_Control(cmd->kind, (void*)cmd->value))!=ADI_PWR_RESULT_SUCCESS )
                    return result;
#endif
            }
            break;
        case ADI_PWR_CMD_TABLE:
            {
                ADI_PWR_COMMAND_PAIR *cmd = (ADI_PWR_COMMAND_PAIR *)value;
                while (cmd->kind != ADI_PWR_CMD_END)
                {
#ifndef ADI_SSL_DEBUG
                    adi_pwr_Control(cmd->kind, (void*)cmd->value);
#else
                    if ( (result=adi_pwr_Control(cmd->kind, (void*)cmd->value))!=ADI_PWR_RESULT_SUCCESS )
                        return result;
#endif
                    cmd++;
                }
            }
            break;

        case ADI_PWR_CMD_SET_INPUT_DELAY:
            {
#ifdef ADI_SSL_DEBUG
                u16 in_delay = (u16)(u32)value;
                if (in_delay > ADI_PWR_INPUT_DELAY_ENABLE)
                    return ADI_PWR_RESULT_INVALID_INPUT_DELAY;
#endif
                PLL_CTL_INPUT_DELAY_SET(adi_pwr_active_config.pll_ctl,(u16)(u32)value);
            }
#if defined(__ADSP_MOAB__)
            SysControlActionFlag |= SYSCTRL_PLLCTL;
#endif            
            AdjustPLL=TRUE;
            break;

        case ADI_PWR_CMD_SET_OUTPUT_DELAY:
            {
#ifdef ADI_SSL_DEBUG
                u16 out_delay = (u16)(u32)value;
                if (out_delay > ADI_PWR_OUTPUT_DELAY_ENABLE)
                    return ADI_PWR_RESULT_INVALID_OUTPUT_DELAY;
#endif
                PLL_CTL_OUTPUT_DELAY_SET(adi_pwr_active_config.pll_ctl,(u16)(u32)value);
            }
#if defined(__ADSP_MOAB__) 
            SysControlActionFlag |= SYSCTRL_PLLCTL;
#endif            
            AdjustPLL=TRUE;
            break;

        case ADI_PWR_CMD_SET_PLL_LOCKCNT:
            {
#ifdef ADI_SSL_DEBUG
                u16 count = (u16)(u32)value;
                if (count > 65535 )
                    return ADI_PWR_RESULT_INVALID_LOCKCNT;
#endif
                adi_pwr_active_config.pll_lockcnt = (ADI_PWR_PLL_LOCKCNT_REG)(u32)value;
            }
#if defined(__ADSP_MOAB__) 
            SysControlActionFlag |= SYSCTRL_LOCKCNT;
#endif            
            AdjustPLL=TRUE;
            break;

        case ADI_PWR_CMD_GET_VDDINT:
            *(u32*)value = adi_pwr_active_config.MaxCoreVoltage;
            break;
            
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)
        case ADI_PWR_CMD_GET_VR_VLEV:
            *(ADI_PWR_VLEV*)value = (ADI_PWR_VLEV)VR_CTL_VLEV_GET(adi_pwr_active_config.vr_ctl);
            break;
#endif

        case ADI_PWR_CMD_GET_VR_FREQ:
            *(ADI_PWR_VR_FREQ*)value = (ADI_PWR_VR_FREQ)VR_CTL_FREQ_GET(adi_pwr_active_config.vr_ctl);
            break;    
                    
#if !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__) && !defined(__ADSP_BRODIE__) && !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)
            
        case ADI_PWR_CMD_GET_VR_GAIN:
            *(ADI_PWR_VR_GAIN*)value = (ADI_PWR_VR_GAIN)VR_CTL_GAIN_GET(adi_pwr_active_config.vr_ctl);
            break;
#endif
            

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_MOAB__)    
        case ADI_PWR_CMD_GET_VR_USBWE:
            *(ADI_PWR_VR_USBWE*)value = (ADI_PWR_VR_USBWE)VR_CTL_USBWE_GET(adi_pwr_active_config.vr_ctl);
            break;
#endif

        case ADI_PWR_CMD_GET_VR_WAKE:
            *(ADI_PWR_VR_WAKE*)value = (ADI_PWR_VR_WAKE)VR_CTL_WAKE_GET(adi_pwr_active_config.vr_ctl);
            break;
            
#if defined(__ADSP_BRODIE__) || defined(__ADSP_BRAEMAR__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_MOY__) /*FIXME || defined(__ADSP_DELTA__)*/
        case ADI_PWR_CMD_GET_VR_PHYWE:
            *(ADI_PWR_VR_PHYWE*)value = (ADI_PWR_VR_PHYWE)VR_CTL_PHYWE_GET(adi_pwr_active_config.vr_ctl);
            break;
#endif  // __ADSP_BRAEMAR__ or KOOK, MOCK, MOY, BRODIE
            

#if  defined(__ADSP_BRODIE__) || defined(__ADSP_BRAEMAR__) || defined(__ADSP_MOAB__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) /*FIXME || defined(__ADSP_MOY__)*/
        case ADI_PWR_CMD_GET_VR_CLKBUFOE:
            *(ADI_PWR_VR_CLKBUFOE*)value = (ADI_PWR_VR_CLKBUFOE)VR_CTL_CLKBUFOE_GET(adi_pwr_active_config.vr_ctl);
            break;
        case ADI_PWR_CMD_GET_VR_CKELOW:
            *(ADI_PWR_VR_CKELOW*)value = (ADI_PWR_VR_CKELOW)VR_CTL_CKELOW_GET(adi_pwr_active_config.vr_ctl);
            break;
#endif


#if defined(__ADSP_BRAEMAR__) || defined(__ADSP_MOAB__)
        case ADI_PWR_CMD_GET_VR_CANWE:
            *(ADI_PWR_VR_CANWE*)value = (ADI_PWR_VR_CANWE)VR_CTL_CANWE_GET(adi_pwr_active_config.vr_ctl);
            break;
#endif
#if defined(__ADSP_MOAB__)
        case ADI_PWR_CMD_GET_VR_GPWE_MXVRWE:
            *(ADI_PWR_VR_GPWE_MXVRWE*)value = (ADI_PWR_VR_GPWE_MXVRWE)VR_CTL_GPWE_MXVRWE_GET(adi_pwr_active_config.vr_ctl);
            break;
#endif        

        case ADI_PWR_CMD_GET_PLL_LOCKCNT:
            *(ADI_PWR_PLL_LOCKCNT_REG*)value = adi_pwr_active_config.pll_lockcnt;
            break;
            
            
            case ADI_PWR_CMD_INSTALL_CLK_CLIENT_CALLBACK:
                {
                    
#ifdef ADI_SSL_DEBUG    
                    /* If we already have the maximum number of callbacks installed */                
                    if( adi_pwr_active_config.ClientCLKCallbackCount == ADI_PWR_MAX_SCK_CALLBACKS)
                        return     ADI_PWR_RESULT_EXCEEDED_MAX_CALLBACKS;
#endif                        
                    /* Fill in the address of the Callback function */
                    pADI_PWR_CALLBACK_ENTRY pCallbackEntry = (pADI_PWR_CALLBACK_ENTRY)value;
                    adi_pwr_active_config.ClientCallbackTable[adi_pwr_active_config.ClientCLKCallbackCount].ClientCallback 
                    = (ADI_PWR_CALLBACK_FN)pCallbackEntry->ClientCallback;
                    
                   /* Fill in the client handle of the Callback function and increment the number of callbacks installed*/                    
                    adi_pwr_active_config.ClientCallbackTable[adi_pwr_active_config.ClientCLKCallbackCount++].ClientHandle 
                    = (ADI_PWR_CALLBACK_FN)pCallbackEntry->ClientHandle;
                    
                    
                    
                }
                break;

            // CASE (remove the client callback)
            case ADI_PWR_CMD_REMOVE_CLK_CLIENT_CALLBACK:
                {
                    for(i=0; i<adi_pwr_active_config.ClientCLKCallbackCount; i++ )
                    {
                        if( adi_pwr_active_config.ClientCallbackTable[i].ClientCallback == (ADI_PWR_CALLBACK_FN)value )
                        {
                            adi_pwr_active_config.ClientCallbackTable[i].ClientCallback  = NULL;
                            break;
                        }
                    }    
#ifdef ADI_SSL_DEBUG    
                    /* If client callback was not found in the table */                
                    if( i == adi_pwr_active_config.ClientCLKCallbackCount )
                        return ADI_PWR_RESULT_NO_CALLBACK_INSTALLED;
#endif                     
                              
                }
                break;

            // CASE (SET client handle to pass to callbacks - not used at this time)
//            case ADI_PWR_CMD_SET_CLIENT_HANDLE:
//                adi_pwr_active_config.ClientHandle = value;
//                break;            
            
        
            
    }

    /* only update the PLL_CTL MMR after all values are set. */
    RecursiveControlEntry--;
    if (AdjustPLL && !RecursiveControlEntry) 
    {
        /* synchronize the cores before programming the PLL */
#if defined(__ADSP_TETON__)
        SYNC_COREB
#endif


    /* the 0 flag tells _adi_pwr_program_pll not to IDLE 
       (input delay and output delay never set the flag PLL_prog_sequence */

    adi_pwr_WritePMRegs(0);

#if defined(__ADSP_TETON__)
        RELEASE_COREB
#endif
    }
    
#if defined(__ADSP_TETON__)
    } 
#endif


    return ADI_PWR_RESULT_SUCCESS;
}

/************************************************************************************
* Function:    adi_pwr_GetConfigSize
* Description: Returns the size in bytes of the configuration structure
*************************************************************************************/

size_t 
adi_pwr_GetConfigSize(void)
{
    return sizeof(ADI_PWR_CONFIG);
}

/*************************************************************************************
* Function:    adi_pwr_GetPowerMode
* Description: Returns the processor state (Full On | Active (PLL enabled) | Active (PLL disabled)
*               Note: wouldn't have access to this function if in Sleep, Deep Sleep, or Hibernate 
***************************************************************************************/

ADI_PWR_MODE  
adi_pwr_GetPowerMode(void)
{


    /* tar 33518 - If the STOPCK bit is still, we have just 
       transitioned from SLEEP - query control register */

    adi_pwr_active_config.pll_ctl = *pll_ctl_mmr;
        
        
    /* query status register */
    adi_pwr_active_config.pll_stat = *pll_stat_mmr;


    /* If we have transitioned from SLEEP mode the STOPCK bit will 
       need to be manually cleared */

    if ( PLL_CTL_STOPCK_GET(adi_pwr_active_config.pll_ctl) )
    {
        PLL_CTL_STOPCK_SET(adi_pwr_active_config.pll_ctl, 0);
        
#if 0 // defined(__ADSP_MOAB__) 
          pAdi_pwr_SysControlSettings->uwPllCtl = U16_REG_CAST(adi_pwr_active_config.pll_ctl);
        u32 ReturnCode = bfrom_SysControl(SYSCTRL_WRITE | SYSCTRL_PLLCTL, pAdi_pwr_SysControlSettings, 0);
    
        /* If SysControl fails, OTP may not be programmed */
        if( ReturnCode != 0 )
        {
            adi_pwr_PLLProgrammingSequence(*(u16*)&adi_pwr_active_config.pll_ctl );
            ReturnCode = 0;
        }


#else    
        // Just do a PLL programming sequence in L1 away with no use of stack
        adi_pwr_PLLProgrammingSequence(*(u16*)&adi_pwr_active_config.pll_ctl );
#endif


    }

    if ( PLL_STAT_FULL_ON_GET(adi_pwr_active_config.pll_stat) )
        return ADI_PWR_MODE_FULL_ON;

    else if ( PLL_STAT_ACTIVE_PLL_ENABLED_GET(adi_pwr_active_config.pll_stat) )
        return ADI_PWR_MODE_ACTIVE;

    else if ( PLL_STAT_ACTIVE_PLL_DISABLED_GET(adi_pwr_active_config.pll_stat) )
        return ADI_PWR_MODE_ACTIVE_PLLDISABLED;
    else 
        return ADI_PWR_NUM_STATES;
}

/************************************************************************************
* Function:    adi_pwr_GetPowerSaving
* Description: Returns the current power saving value
*************************************************************************************/

u32
adi_pwr_GetPowerSaving(void)
{
    u32 fcclk,fsclk,fvco;
    u32 power_saving;
#if defined(__ADSP_TETON__)
    if (core_ident == ADI_COREA )    {
#endif
    adi_pwr_GetFreq(&fcclk,&fsclk,&fvco);
    
#if defined(__ADSP_MOY__) || defined(__ADSP_DELTA__) 
    power_saving = adi_pwr_PowerSaving(
                        fcclk,
                        (ADI_PWR_VLEV)adi_pwr_active_config.MaxCoreVoltage
                   );    
#else    
    power_saving = adi_pwr_PowerSaving(
                        fcclk,
                        (ADI_PWR_VLEV)VR_CTL_VLEV_GET(adi_pwr_active_config.vr_ctl)
                   );
#endif                   
    return power_saving;
#if defined(__ADSP_TETON__)
    } else
        return 0;
#endif
}



/***********************************************************************************
* Function:    adi_pwr_GetFreq
* Description: Returns the core system clock and VCO frequencies.
************************************************************************************/

ADI_PWR_RESULT
adi_pwr_GetFreq(
                 u32 *f_cclk, 
                 u32 *f_sclk,
                 u32 *f_vco
)
{
 
    /* evaluate the three pointer arguments as call-by-value, creating internal 
       copies of the three arguments, so we will not disturb the calling environment */

    u32 fcclk;
    u32 fsclk;
    u32 fvco;
       
    u16 msel, df, csel, ssel;
     
    /* if power management has not been initialized this function is invalid */
    if (!adi_pwr_active_config.Initialized)
        return ADI_PWR_RESULT_NOT_INITIALIZED;


#if defined(__ADSP_TETON__)

    /* Some special rules apply for dual core: For Core B to behave the same as Core A, 
       or the same as single core processors, auto-synch must be enabled.  If auto-synch 
       is not enabled, Core B must read the values directly from the MMRs */

    if ( (core_ident != ADI_COREA ) && ( !(adi_pwr_active_config.auto_sync_enabled )))
    {
        msel = PLL_CTL_MSEL_GET( adi_pwr_active_config.pll_ctl );
        df   = PLL_CTL_DF_GET(adi_pwr_active_config.pll_ctl);
        csel = PLL_DIV_CSEL_GET(adi_pwr_active_config.pll_div);
        ssel = PLL_DIV_SSEL_GET(adi_pwr_active_config.pll_div);
        fvco = msel*adi_pwr_active_config.clkin/(df+1);
        fcclk = (fvco)/ADI_PWR_CSEL_VALUE(csel);
        fsclk = (fvco)/ssel;
    }
    else
    {
    /* For Core A, or for Core B when auto-synch is enabled, proceed as usual */

#endif


    /* If PLL not used CCLK and SCLK are CLKIN */
    if (PLL_CTL_BYPASS_GET( adi_pwr_active_config.pll_ctl) || PLL_CTL_PLL_OFF_GET(adi_pwr_active_config.pll_ctl) )
    {
        fvco = adi_pwr_active_config.clkin;
        fcclk = adi_pwr_active_config.clkin;
        fsclk = adi_pwr_active_config.clkin;
    }

    /* If PLL is used, calculate  CCLK and SCLK using and PLL_CTL and PLL_DIV settings */
    else
    {
        msel = PLL_CTL_MSEL_GET( adi_pwr_active_config.pll_ctl );
        df   = PLL_CTL_DF_GET(adi_pwr_active_config.pll_ctl);
        csel = PLL_DIV_CSEL_GET(adi_pwr_active_config.pll_div);
        ssel = PLL_DIV_SSEL_GET(adi_pwr_active_config.pll_div);
        fvco = msel*adi_pwr_active_config.clkin/(df+1);
        fcclk = (fvco)/ADI_PWR_CSEL_VALUE(csel);
        fsclk = (fvco)/ssel;
    }


#if defined(__ADSP_TETON__)
    /* close the else clause, for Teton */
    }

#endif    
    
    /* move the values into the pointer arguments */
    *f_cclk = fcclk;
    *f_sclk = fsclk;
    *f_vco = fvco;    
    

    return ADI_PWR_RESULT_SUCCESS;


}



/**********************************************************************************
* Function:    adi_pwr_GetSCLK
* Description: Returns the programmed SCLK frequency by accessing the PLL regs
*               directly.
***********************************************************************************/
u32 adi_pwr_GetSCLK()
{
    u32 fsclk;
    ADI_PWR_PLL_CTL_REG pll_ctl = *(ADI_PWR_PLL_CTL_REG*)pADI_PWR_PLL_CTL;
    ADI_PWR_PLL_DIV_REG pll_div = *(ADI_PWR_PLL_DIV_REG*)pADI_PWR_PLL_DIV;
//    ssync();   Removed per anomaly 05-00-0312
    if (PLL_CTL_BYPASS_GET(pll_ctl) || PLL_CTL_PLL_OFF_GET(pll_ctl) )
    {
        fsclk = adi_pwr_active_config.clkin;
    }
    else
    {
        u32 msel = PLL_CTL_MSEL_GET(pll_ctl);
        u32 df   = PLL_CTL_DF_GET(pll_ctl);
         u32 ssel = PLL_DIV_SSEL_GET(pll_div);
        fsclk = msel*adi_pwr_active_config.clkin/(df+1)/ssel;
    }
    return fsclk;
}

/**********************************************************************************
* Function:    adi_pwr_Init
* Description: Sets up processor type and CLKIN frequency in MHz
***********************************************************************************/

#define ten2pwr6 1000000

ADI_PWR_RESULT 
adi_pwr_Init(
        const ADI_PWR_COMMAND_PAIR *table    // table of commands to configure the module
)
{
    u32 fcclk, fsclk, fvco, i, cclk_values_already_set=0;

   
    ADI_PWR_VLEV vlev;
    ADI_PWR_COMMAND_PAIR *cmd = (ADI_PWR_COMMAND_PAIR *)table;

    // Once initialized, no subsequent call or thread can initialise it
    if (adi_pwr_SetInitFlag())
        return ADI_PWR_RESULT_ALREADY_INITIALIZED;

/* The processor that presently supports the bfrom_SysControl ROM function. */
#if defined(__ADSP_MOAB__) 
    SysControlActionFlag = 0; 
#endif

#ifdef ADI_SSL_DEBUG
    u8 proc_set=0;
    u8 package_set=0;
    u8 vddext_set=0;
    u8 clkin_set=0;


    if (table==0)
        return ADI_PWR_RESULT_FAILED;
#endif


    // initialize the instance data register file to real MMR values    
    *(u16*)&adi_pwr_active_config.pll_ctl     = *pADI_PWR_PLL_CTL;
    *(u16*)&adi_pwr_active_config.pll_div     = *pADI_PWR_PLL_DIV;
    *(u16*)&adi_pwr_active_config.vr_ctl      = *pADI_PWR_VR_CTL;

    *(u16*)&adi_pwr_active_config.pll_lockcnt = *pADI_PWR_PLL_LOCKCNT;

#if defined(__ADSP_MOAB__)
            
      pAdi_pwr_SysControlSettings->uwVrCtl = U16_REG_CAST(adi_pwr_active_config.vr_ctl);
      pAdi_pwr_SysControlSettings->uwPllCtl = U16_REG_CAST(adi_pwr_active_config.pll_ctl);
     pAdi_pwr_SysControlSettings->uwPllDiv = U16_REG_CAST(adi_pwr_active_config.pll_div);
     pAdi_pwr_SysControlSettings->uwPllLockCnt = U16_REG_CAST(adi_pwr_active_config.pll_lockcnt);
#endif    


#if defined(__ADSP_MOAB__) 
/* On these parts, the max core voltage is 1.3 unless using an external voltage regulator.  
   For now, set it to 1.3, and if the SET_VDDINT command is passed, increase it accordingly */
   
    adi_pwr_active_config.MaxCoreVoltage      = ADI_PWR_VLEV_130;
#else
    adi_pwr_active_config.MaxCoreVoltage      = ADI_PWR_VLEV_MAX;
#endif    

    adi_pwr_active_config.MHzFactor          = 1000000;

    adi_pwr_active_config.ClientCLKCallbackCount = 0;

    /* Set sensible defaults */
    adi_int_SICGetIVG(ADI_INT_PLL_WAKEUP,&adi_pwr_active_config.IVG);
//    adi_pwr_active_config.ClientCallback1     = NULL;
//    adi_pwr_active_config.ClientCallback2     = NULL;
//    adi_pwr_active_config.ClientHandle        = NULL;

    

#if defined(__ADSP_TETON__)
    adi_pwr_active_config.auto_sync_enabled   = 0;
    adi_int_SICGetIVG(ADI_INT_SI0,&adi_pwr_active_config.IVG_SuppInt0);
    adi_pwr_active_config.pLockVar  = &adi_pwr_lockvar;
#endif
#if defined(__ADSP_BRAEMAR__)
    // set PC133 compliant by default
    adi_pwr_active_config.PC133_compliant      = TRUE;
#endif

    /* set the callback count to 0 - no callbacks installed */
    adi_pwr_active_config.ClientCLKCallbackCount = 0;
    
    /* Define a pointer to a callback entry fo reference purposes */    
    ADI_PWR_CALLBACK_ENTRY  *pCallbackEntry;    
    
    /* initialize all the entries in the client callback table which is part of the config structure */ 
    for (i = 0, pCallbackEntry = adi_pwr_active_config.ClientCallbackTable; 
      i < ADI_PWR_MAX_SCK_CALLBACKS; i++, pCallbackEntry++ ) 
    {
        /* initialize the Callback field */
        pCallbackEntry->ClientCallback = NULL;
        
        /* initialize the handle field, this value gets passed back to the callback, 
          so the client can use it, perhaps to identify itself or to associate a callback with an event */
        pCallbackEntry->ClientHandle = NULL;        
    }
    

    for ( ; cmd->kind != ADI_PWR_CMD_END ; cmd++)
    {
        switch (cmd->kind)
        {
            default:
                return ADI_PWR_RESULT_BAD_COMMAND;

            case ADI_PWR_CMD_SET_PROC_VARIANT:
#ifdef ADI_SSL_DEBUG
                if ( (ADI_PWR_PROC_KIND)cmd->value >= ADI_PWR_NUM_PROC_KINDS )
                    return ADI_PWR_RESULT_INVALID_PROCESSOR;
                proc_set++;
#endif
                adi_pwr_active_config.proc_type = (ADI_PWR_PROC_KIND)cmd->value;

                break;

            
#if !defined(__ADSP_BRODIE__) && !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__) && !defined(__ADSP_MOAB__)  && !defined(__ADSP_MOY__)  && !defined(__ADSP_DELTA__)    
/* this command is no longer supported for these products */    
            case ADI_PWR_CMD_SET_EZKIT:                
#ifndef ADI_SSL_DEBUG
                adi_pwr_set_ezkit_values((ADI_PWR_EZKIT)cmd->value);
#else
                if (!adi_pwr_set_ezkit_values((ADI_PWR_EZKIT)cmd->value))
                    return ADI_PWR_RESULT_INVALID_EZKIT;
                proc_set++; 
                package_set++; 
                vddext_set++; 
                clkin_set++;
#endif
                break;
#endif                
            case ADI_PWR_CMD_SET_PACKAGE:
                adi_pwr_active_config.package_type = (ADI_PWR_PACKAGE_KIND)cmd->value;
#ifdef ADI_SSL_DEBUG
                package_set++;
#endif
                break;
            case ADI_PWR_CMD_SET_VDDINT:
                {
                     vlev = (ADI_PWR_VLEV)cmd->value;
#ifdef ADI_SSL_DEBUG    
                    if (vlev < ADI_PWR_VLEV_MIN || vlev > ADI_PWR_VLEV_MAX)
                        return ADI_PWR_RESULT_INVALID_VLEV;
#endif
        

                    adi_pwr_active_config.MaxCoreVoltage = (ADI_PWR_VLEV)cmd->value;


#if defined(__ADSP_MOAB__)

                /* SysControl expects the vlev value in the VrCtl register in the SYSCONTROL 
                   settings structure.  VrCtl is not a bit field struct,like adi_pwr_active_config.vr_ctl
                   we will not write this VLEV to the hardware, it is just to tell SysControl the 
                   externally supplied VLEV. The VLEV encodings for Moab for external VR = 1.35V, 1.4V 
                   are 16 and 17, which do not fit into the 4 bit b_VLEV field. MaxCoreVoltage can be 16 
                   or 17, to index into the cclk_vlev table, but we cap the value at 0xF for Syscontrol.  */

                    if (vlev > ADI_PWR_VLEV_130)
                    {
                        vlev = ADI_PWR_VLEV_130;
                    }                      
                                   
                    /* The highest we can pass to Syscontrol is 0xF = 1.3V */
                    VR_CTL_VLEV_SET( adi_pwr_active_config.vr_ctl, vlev );
                    SysControlActionFlag |= (SYSCTRL_EXTVOLTAGE | SYSCTRL_VRCTL);
#endif    

                    
#if !defined(__ADSP_BRODIE__) && !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__) && !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)

                    // set internal Voltage regulator off
                    adi_pwr_SetVoltageRegulator(ADI_PWR_CMD_SET_VR_FREQ, ADI_PWR_VR_FREQ_POWERDOWN);
#endif
                    /* Set VDDINT after because adi_pwr_SetVoltageRegulator does not work if vddint is set */
                    vddint_set++;
                    
                }
                break;

            case ADI_PWR_CMD_SET_VDDEXT:
                {
#ifdef ADI_SSL_DEBUG
                    ADI_PWR_VDDEXT vddext = (ADI_PWR_VDDEXT)cmd->value;
#if !defined(__ADSP_BRODIE__) && !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__)
                    if    ( vddext > ADI_PWR_VDDEXT_250 ) 
                        return ADI_PWR_RESULT_INVALID_VDDEXT;
#else
                    if    ( vddext > ADI_PWR_VDDEXT_180 ) 
                        return ADI_PWR_RESULT_INVALID_VDDEXT;
#endif
                    vddext_set++;
#endif
                    adi_pwr_active_config.ExternalVoltage = (ADI_PWR_VDDEXT)cmd->value;
#if defined(__ADSP_BRODIE__)
					/* Table indexing adjustments. The BRODIE supports three external voltage */
					/* values. The internal power tables have only been set up to handle two */
					/* external VDD values. This needs to be addressed going forward, but for */
					/* now simply treat VDDEXT_250 and VDDEXT_330 as identical (which they are */
					/* for the BRODIE. And then adjust VDDEXT_180 such that it can access the */
					/* second element of the power table array */
					
                    if( adi_pwr_active_config.ExternalVoltage == ADI_PWR_VDDEXT_250)
                    	adi_pwr_active_config.ExternalVoltage = ADI_PWR_VDDEXT_330;
                    else if( adi_pwr_active_config.ExternalVoltage == ADI_PWR_VDDEXT_180)
                    	adi_pwr_active_config.ExternalVoltage = ADI_PWR_VDDEXT_250;
                    
#endif
                }
                break;
            case ADI_PWR_CMD_SET_CLKIN:
                adi_pwr_active_config.clkin = (u32)cmd->value;
#ifdef ADI_SSL_DEBUG
                clkin_set++;
#endif
                break;

            case ADI_PWR_CMD_SET_IVG:
                {
#ifdef ADI_SSL_DEBUG
                    u16 ivg = (u16)(u32)cmd->value;
                    if (ivg < 7 || ivg > 15 )
                         return ADI_PWR_RESULT_INVALID_IVG;
#endif
                    adi_pwr_active_config.IVG = (u16)(u32)cmd->value;
                }
                break;

            // CASE (install a client callback  to be called when clocks change, can be called at Init or in Control)
            case ADI_PWR_CMD_INSTALL_CLK_CLIENT_CALLBACK:            
                {
#ifndef ADI_SSL_DEBUG
                    adi_pwr_Control(cmd->kind, (void*)cmd->value);
#else
                    ADI_PWR_RESULT result;
                    if(( result = adi_pwr_Control(cmd->kind, (void*)cmd->value))!=ADI_PWR_RESULT_SUCCESS )
                        return result;
#endif
                }
                break;

#if defined(__ADSP_TETON__)
            case ADI_PWR_CMD_SET_COREB_SUPP_INT0_IVG:
                {
#ifdef ADI_SSL_DEBUG
                    u16 ivg = (u16)(u32)cmd->value;
                    if (ivg < 7 || ivg > 15)
                        return ADI_PWR_RESULT_INVALID_IVG;
#endif
                    adi_pwr_active_config.IVG_SuppInt0 = (u16)(u32)cmd->value;
                }
                break;
#endif // defined(__ADSP_TETON__) 

            case ADI_PWR_CMD_SET_INPUT_DELAY:
                {
#ifdef ADI_SSL_DEBUG
                    u16 in_delay = (u16)(u32)cmd->value;
                    if (in_delay > ADI_PWR_INPUT_DELAY_ENABLE)
                        return ADI_PWR_RESULT_INVALID_INPUT_DELAY;
#endif
                    PLL_CTL_INPUT_DELAY_SET(adi_pwr_active_config.pll_ctl,(u16)(u32)cmd->value);
                }
                break;
            case ADI_PWR_CMD_SET_OUTPUT_DELAY:
                {
#ifdef ADI_SSL_DEBUG
                    u16 out_delay = (u16)(u32)cmd->value;
                    if (out_delay > ADI_PWR_OUTPUT_DELAY_ENABLE)
                        return ADI_PWR_RESULT_INVALID_OUTPUT_DELAY;
#endif
                    PLL_CTL_OUTPUT_DELAY_SET(adi_pwr_active_config.pll_ctl,(u16)(u32)cmd->value);
#if defined(__ADSP_MOAB__)
                    SysControlActionFlag |= SYSCTRL_PLLCTL;                         
#endif                    
                }
                break;
            case ADI_PWR_CMD_SET_PLL_LOCKCNT:
                {
#ifdef ADI_SSL_DEBUG
                    u16 count = (u16)(u32)cmd->value;
                    if (count > 65535 )
                        return ADI_PWR_RESULT_INVALID_LOCKCNT;
#endif
                    *(u16*)&adi_pwr_active_config.pll_lockcnt = (u16)(u32)cmd->value;

                    /* We set it right away. */
                     *pll_lockcnt_mmr = adi_pwr_active_config.pll_lockcnt;
                                         
                }
                break;
            case ADI_PWR_CMD_FORCE_DATASHEET_VALUES:
                break;

            case ADI_PWR_CMD_SET_CCLK_TABLE:
#if defined(__ADSP_EDINBURGH__) && (__SILICON_REVISION__==0xffff || __SILICON_REVISION__==0x1)
                return ADI_PWR_RESULT_BAD_COMMAND;
#else
                for (i=0, vlev=ADI_PWR_VLEV_MIN ;i<ADI_PWR_NUM_VLEVS; i++, vlev++)
                    adi_pwr_cclk_vlev[i] = ((u16*)cmd->value)[i];

                cclk_values_already_set=1;
                break;
#endif
            case ADI_PWR_CMD_SET_FREQ_AS_MHZ:
                adi_pwr_active_config.MHzFactor = 1;
                break;

#if defined(__ADSP_TETON__)
            case ADI_PWR_CMD_SET_AUTO_SYNC_ENABLED:
                adi_pwr_active_config.auto_sync_enabled = 1;
                break;

            case  ADI_PWR_CMD_SET_SYNC_LOCK_VARIABLE:
                adi_pwr_SetLockVarPointer(cmd->value);
                break;
#endif

#if defined(__ADSP_BRAEMAR__)

        case ADI_PWR_CMD_SET_PC133_COMPLIANCE:
#ifdef ADI_SSL_DEBUG
            {
                ADI_PWR_PC133_COMPLIANCE pc133 = (ADI_PWR_PC133_COMPLIANCE)cmd->value;
                if (pc133 > ADI_PWR_PC133_COMPLIANCE_ENABLED)
                    return ADI_PWR_RESULT_INVALID_PC133_COMPLIANCE;
            }
#endif
            adi_pwr_active_config.PC133_compliant = (u16)(u32)cmd->value;
            break;

#endif  // __ADSP_BRAEMAR__


        }
    }

#ifdef ADI_SSL_DEBUG
    /* If not all the essential parameters are set, return with error */
    if ( !(proc_set && package_set && vddext_set && clkin_set) )
        return ADI_PWR_RESULT_FAILED;


#if defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__) || defined(__ADSP_DELTA__)

    /* MUST have external VR, vddint must be set. */
    if (!vddint_set)
        return ADI_PWR_RESULT_VDDINT_MUST_BE_SUPPLIED;
         
#elif !defined(__ADSP_KOOKABURRA__) 
    if (!vddint_set && ADI_PWR_VOLTAGE_REGULATOR_IS_OFF)
            return ADI_PWR_RESULT_VDDINT_MUST_BE_SUPPLIED;

// on kookaburra, we have no way to know if external voltage regulation 
// is selected in hardware, we cannot read VRSEL so the user must set 
// VDDINT for proper functioning of adi_pwr_SetFreq.

#endif
#endif

    /* If CLKIN is less than one million, assume it's meant to be MHz and adjust accordingly */
    if ( (adi_pwr_active_config.MHzFactor > 1) && ( adi_pwr_active_config.clkin < 1000000/* 1 MHz */) )        
        adi_pwr_active_config.clkin    *= adi_pwr_active_config.MHzFactor;
        
#ifdef ADI_SSL_DEBUG
    /* If CLKIN less than the minimum allowed then return with error */
    if ( (adi_pwr_active_config.clkin < ADI_PWR_CLKIN_MIN) || (adi_pwr_active_config.clkin > ADI_PWR_CLKIN_MAX) )
        return ADI_PWR_RESULT_FAILED;
#endif
        
    // Enable PLL wakeup (it is not necessary to enable interrupts as there is
    // no need to handle the interrupt)
    adi_int_SICWakeup(ADI_INT_PLL_WAKEUP,1);

#if defined(__ADSP_TETON__) 
    if (core_ident == ADI_COREA ) 
    {
#endif
    // determine actual current SCLK based on registers plus CLKIN and VDDEXT settings just provided
    adi_pwr_GetFreq(&fcclk, &fsclk, &fvco);
    
    /* copy the cached PLL Register File to MMRs and adjust SDRAM
      As this is the first time, we force an SDRAM adjustment, 
      now that we know clock info  */
    
    adi_pwr_ApplyConfig( fcclk, fsclk, 1 );
    
    adi_pwr_active_config.fsclk = adi_pwr_GetSCLK();
    
    if (cclk_values_already_set)
    {
        // multiply the values already in the table by 1000000
        for (i=0, vlev=ADI_PWR_VLEV_MIN ;i<ADI_PWR_NUM_VLEVS; i++, vlev++)
            adi_pwr_cclk_vlev[i] *= adi_pwr_active_config.MHzFactor;
    }
    else
    {

        // use the datasheet values * 1000000 
        if( adi_pwr_cclk_vlev[0] < 1000000 )
        {
            for (i=0, vlev=ADI_PWR_VLEV_MIN ; i<ADI_PWR_NUM_VLEVS;  i++,  vlev++)
                adi_pwr_cclk_vlev[i] = adi_pwr_cclk_vlev_datasheet[i][adi_pwr_active_config.proc_type]*adi_pwr_active_config.MHzFactor; 
        }

    }

    // Apply Mhz factor to SCLK limits
    for(i=0;i<ADI_PWR_NUM_PKG_KINDS;i++)
    {
        int j;
        for (j=0;j<2;j++)
        {
            adi_pwr_sclk_limits[i][j].sclk_max_level1 = adi_pwr_sclk_limits_datasheet[i][j].sclk_max_level1*adi_pwr_active_config.MHzFactor;
            adi_pwr_sclk_limits[i][j].sclk_max_level2 = adi_pwr_sclk_limits_datasheet[i][j].sclk_max_level2*adi_pwr_active_config.MHzFactor;
        }
    }



#if defined(__ADSP_TETON__) 
    } 
    else 
    {

        // register supplemental interrupt handler function on Core B ,
        // if the default auto sync behavior is required
        if (adi_pwr_active_config.auto_sync_enabled)
        {
            ADI_INT_RESULT result;
            result = adi_int_CECHook(adi_pwr_active_config.IVG_SuppInt0,adi_pwr_CoreBSuppInt0Handler,NULL,0);
#ifdef ADI_SSL_DEBUG
            if (result!=ADI_INT_RESULT_SUCCESS)
                return ADI_PWR_RESULT_CANT_HOOK_SUPPLEMENTAL_INTERRUPT;
#endif
            adi_int_SICSetIVG(ADI_INT_SI0,adi_pwr_active_config.IVG_SuppInt0);
            adi_int_SICWakeup(ADI_INT_SI0,1);
            adi_int_SICEnable(ADI_INT_SI0);
        }
    }
#endif
    return ADI_PWR_RESULT_SUCCESS;

}

/************************************************************************************
* Function:    adi_pwr_LoadConfig
* Description: Restores the given Power Management settings.
*************************************************************************************/

ADI_PWR_RESULT 
adi_pwr_LoadConfig(
       const ADI_PWR_CONFIG_HANDLE hConfig, 
       const size_t szConfig
)
{
    u32 fcclk, fsclk, fvco;
    ADI_PWR_CONFIG *config = (ADI_PWR_CONFIG *)hConfig;


#ifdef ADI_SSL_DEBUG
    if (szConfig < sizeof(ADI_PWR_CONFIG))
        return ADI_PWR_RESULT_NO_MEMORY;
    else if (config==0)
        return ADI_PWR_RESULT_FAILED;
#endif

    if (!adi_pwr_active_config.Initialized)
        return ADI_PWR_RESULT_NOT_INITIALIZED;

#if defined(__ADSP_TETON__)
    if (core_ident == ADI_COREA ) {
#endif


    // set current settings
    adi_pwr_active_config.proc_type   = config->proc_type;
    adi_pwr_active_config.MaxCoreVoltage = config->MaxCoreVoltage;
    adi_pwr_active_config.clkin       = config->clkin;

#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)   /* no EBIU */
    adi_ebiu_LoadConfig( (ADI_EBIU_CONFIG_HANDLE*)&config->ebiu_config,ADI_EBIU_SIZEOF_CONFIG);
#endif
    // copy back to MMR's 
    adi_pwr_active_config.pll_ctl     = config->pll_ctl;
    adi_pwr_active_config.pll_div     = config->pll_div;
    adi_pwr_active_config.pll_lockcnt = config->pll_lockcnt;
    adi_pwr_active_config.vr_ctl      = config->vr_ctl;

       /* determine actual current SCLK (from MMR) */
    adi_pwr_active_config.fsclk = adi_pwr_GetSCLK();

    /* retrieve SCLK frequencies from Config structure, to set the actual clocks */
    adi_pwr_GetFreq(&fcclk, &fsclk, &fvco);
    /* copy the cached PLL Register File to MMR and adjust SDRAM if SCLK is changed */
    adi_pwr_ApplyConfig( fcclk, fsclk, 0); 
    
#if defined(__ADSP_TETON__)
    }
#endif

    return ADI_PWR_RESULT_SUCCESS;
}

/************************************************************************************

* Function:    adi_pwr_SetPLL

* Description: Sets the cached PLL registers

*************************************************************************************/

static void adi_pwr_SetPLL(
        u32                msel,
        ADI_PWR_DF        df,
        ADI_PWR_CSEL    csel,
        u32                ssel,
        ADI_PWR_VLEV    vlev
)
{
    PLL_CTL_MSEL_SET( adi_pwr_active_config.pll_ctl, msel    );
    PLL_CTL_DF_SET    ( adi_pwr_active_config.pll_ctl, df        );
    PLL_DIV_SSEL_SET( adi_pwr_active_config.pll_div, ssel    );
    PLL_DIV_CSEL_SET( adi_pwr_active_config.pll_div, csel    );
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)
    VR_CTL_VLEV_SET    ( adi_pwr_active_config.vr_ctl,  vlev    );
#endif    
}



/************************************************************************************

  Function:    adi_pwr_WritePMRegs()
  Description: For internal use; Applies the changes to the MMRs using SysControl
                 
*************************************************************************************/

u32 adi_pwr_WritePMRegs(u16 DoIdle)
{        
    ADI_PWR_VLEV OldVLEV, NewVLEV;
    
    u32 ReturnCode = 0;
    u16 Temp16;
    ADI_INT_WAKEUP_REGISTER RegIWR;
    

#if defined(__ADSP_MOAB__) 
     

    /* NOTE: externally supplied VDDINT has been passed to adi_pwr_Init -
             we have already stored MaxCoreVoltage in the VLEV field, for SysControl  */  
           
             
    /* Must make the calls in the correct order, compare VLEV settings */
    Temp16 = pAdi_pwr_SysControlSettings->uwVrCtl;
    OldVLEV = (ADI_PWR_VLEV)VR_CTL_VLEV_GET( *( (ADI_PWR_VR_CTL_REG*) (&Temp16) ));
            
    /* We have the new value in this register - It was copied when we called adi_pwr_SetPLL */
    NewVLEV = (ADI_PWR_VLEV)VR_CTL_VLEV_GET(adi_pwr_active_config.vr_ctl);
    
    /* Copy the VR configuration structure to the SysControl Structure */     
      pAdi_pwr_SysControlSettings->uwVrCtl = U16_REG_CAST(adi_pwr_active_config.vr_ctl);
      
    /* Copy the PLL configuration structure to the SysControl Structure */           
       pAdi_pwr_SysControlSettings->uwPllCtl = U16_REG_CAST(adi_pwr_active_config.pll_ctl);
      pAdi_pwr_SysControlSettings->uwPllDiv = U16_REG_CAST(adi_pwr_active_config.pll_div);
      pAdi_pwr_SysControlSettings->uwPllLockCnt = U16_REG_CAST(adi_pwr_active_config.pll_lockcnt);
            
      /* Order possibilities - (1) PLL only, (2) VR only, (3) both, PLL first, (4) both VR first */
      
      
    /*  (1) PLL only */
    if(( (SysControlActionFlag & SYSCTRL_PLLCTL ) || (SysControlActionFlag & SYSCTRL_PLLDIV ))  
      &&  /* and we are also Changing  internal Voltage Regulator */
      ( OldVLEV == NewVLEV ) )
    {
        /* The adi_pwr_program_pll function should not change VLEV for Moab  */       
        
        /* globally disable wakeups (except for PLL) */
        adi_int_SICGlobalWakeup( FALSE, &RegIWR ); 
        adi_int_SICWakeup(ADI_INT_PLL_WAKEUP,1);
        
        /* apply the changes */              
        adi_pwr_program_pll( &adi_pwr_active_config, DoIdle );
        
        /* restore wakeups */
        adi_int_SICGlobalWakeup( TRUE, &RegIWR ); 
               
        SysControlActionFlag ^= ( SYSCTRL_PLLCTL | SYSCTRL_PLLDIV ); 
    }
    
      
    /*  (2) VR only */
    if( !((SysControlActionFlag & SYSCTRL_PLLCTL ) || (SysControlActionFlag & SYSCTRL_PLLDIV ))  
      &&  /* and we are also Changing  internal Voltage Regulator */
      ( OldVLEV != NewVLEV ) )
    {
        /* Use syscontrol to program the VR     */
        ReturnCode = bfrom_SysControl(SYSCTRL_WRITE | ( SYSCTRL_VRCTL | SYSCTRL_INTVOLTAGE), pAdi_pwr_SysControlSettings, 0);    
        SysControlActionFlag ^= ( SYSCTRL_VRCTL | SYSCTRL_INTVOLTAGE );
    }
    
      
    /*  (3) both, PLL first */
    if( ((SysControlActionFlag & SYSCTRL_PLLCTL ) || (SysControlActionFlag & SYSCTRL_PLLDIV ))  
      &&  /* and we are also Changing  internal Voltage Regulator */
      ( OldVLEV > NewVLEV ) )
    {
        /* globally disable wakeups (except for PLL) */
        adi_int_SICGlobalWakeup( FALSE, &RegIWR );
        adi_int_SICWakeup(ADI_INT_PLL_WAKEUP,1);
        
        /* The adi_pwr_program_pll function should not change VLEV for Moab  */                
        adi_pwr_program_pll( &adi_pwr_active_config, DoIdle );
        
        /* restore wakeups */
        adi_int_SICGlobalWakeup( TRUE, &RegIWR );
        
        SysControlActionFlag ^= ( SYSCTRL_PLLCTL | SYSCTRL_PLLDIV ); 
        
        /* Use syscontrol to program the VR       */
        ReturnCode = bfrom_SysControl(SYSCTRL_WRITE | ( SYSCTRL_VRCTL | SYSCTRL_INTVOLTAGE), pAdi_pwr_SysControlSettings, 0);    
        SysControlActionFlag ^= ( SYSCTRL_VRCTL | SYSCTRL_INTVOLTAGE );
        
    }              
  
    /*  (4) both, VR first */
    if( ((SysControlActionFlag & SYSCTRL_PLLCTL ) || (SysControlActionFlag & SYSCTRL_PLLDIV ))  
      &&  /* and we are also Changing  internal Voltage Regulator */
      ( OldVLEV < NewVLEV ) )
    {
        /* Use syscontrol to program the VR       */
        ReturnCode = bfrom_SysControl(SYSCTRL_WRITE | ( SYSCTRL_VRCTL | SYSCTRL_INTVOLTAGE), pAdi_pwr_SysControlSettings, 0);    
        SysControlActionFlag ^= ( SYSCTRL_VRCTL | SYSCTRL_INTVOLTAGE );
    
        /* globally disable wakeups (except for PLL) */
        adi_int_SICGlobalWakeup( FALSE, &RegIWR );        
        adi_int_SICWakeup(ADI_INT_PLL_WAKEUP,1);

        /* The adi_pwr_program_pll function should not change VLEV for Moab  */               
        adi_pwr_program_pll( &adi_pwr_active_config, DoIdle );
                
        /* restore wakeups */
        adi_int_SICGlobalWakeup( TRUE, &RegIWR );                        
        
        SysControlActionFlag ^= ( SYSCTRL_PLLCTL | SYSCTRL_PLLDIV ); 
    }
            
    SysControlActionFlag = 0;
    
#else    /* no syscontrol in boot ROM */    

       /* globally disable wakeups (except for PLL) */
       adi_int_SICGlobalWakeup( FALSE, &RegIWR );        
    adi_int_SICWakeup(ADI_INT_PLL_WAKEUP,1);

    adi_pwr_program_pll( &adi_pwr_active_config, DoIdle);
    
       /* restore wakeups */
       adi_int_SICGlobalWakeup( TRUE, &RegIWR );            
    
#endif    


    return( ReturnCode );         
}

/************************************************************************************

 Function:    adi_pwr_ExecuteCallbacks
 Description: If any Client Callbacks are installed, execute them.

*************************************************************************************/
void adi_pwr_ExecuteCallbacks(u32 sclk)
{
    u32 i;
    
    for(i=0; i<adi_pwr_active_config.ClientCLKCallbackCount; i++ )
    {
        if( adi_pwr_active_config.ClientCallbackTable[i].ClientCallback != NULL)
        {
            /* execute the callback, passing the client handle */
            (adi_pwr_active_config.ClientCallbackTable[i].ClientCallback)
              (adi_pwr_active_config.ClientCallbackTable[i].ClientHandle, sclk);
             
        }
    }
}

/************************************************************************************

 Function:    adi_pwr_ApplyConfig
 Description: Applies the changes to the MMRs and syncs the core. SDRAM is 
              adjusted if necessary..

*************************************************************************************/

static void /* section("L1_code")  */
adi_pwr_ApplyConfig(u32 fcclk, u32 fsclk, u16 StartUp ) 
{

    /* The arguments fcclk and fsclk were returned from a call to adi_pwr_GetFreq which 
       calculates the clock values based on the cached registers for PLL_CTL and PLL_DIV.
       The value we compare SCLK to is an extra entry in the config structure to save the 
       SCLK value once it has been set.  Therefore, the first comparison is to see whether
       the new SCLK is less than the previous SCLK.  The second check is to determine 
       whether the new SCLK is greater than the previous SCLK.  If startup flag is set, we 
       do not rely on the comparison because the previous SCLK may not be valid. */ 
       
       
#if defined(__ADSP_TETON__)
    /* Bring core B to safe state before proceeding */
    SYNC_COREB
#endif
    /* decreasing SCLK, (and not startup) PLL first then SDRAM  */
    if (fsclk < adi_pwr_active_config.fsclk && !StartUp)
    {
        /* callback to tell clients that clock is about to change */
        adi_pwr_ExecuteCallbacks(0);    
            
         adi_pwr_WritePMRegs(1); 
#if !defined(__ADSP_MOY__)     && !defined(__ADSP_DELTA__) /* no EBIU */
        adi_ebiu_AdjustSDRAM(fsclk);
#endif
        adi_pwr_active_config.fsclk = fsclk;
        
        /* callback to tell clients that clock has changed */        
        adi_pwr_ExecuteCallbacks(fsclk);        
    }
    /* increasing SCLK, (and not startup) SDRAM first then PLL */    
    else if (fsclk > adi_pwr_active_config.fsclk && !StartUp)
    {
        /* callback to tell clients that clock is about to changed */
        adi_pwr_ExecuteCallbacks(0);
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__) /* no EBIU */                 
        adi_ebiu_AdjustSDRAM(fsclk);
#endif
        adi_pwr_WritePMRegs(1);    
        adi_pwr_active_config.fsclk = fsclk;
        
        /* callback to tell clients that clock has changed */        
        adi_pwr_ExecuteCallbacks(fsclk);        
    } 
    else  /* no change in fsclk, or just starting up */
    {        
         adi_pwr_WritePMRegs(1);
#if !defined(__ADSP_MOY__)  && !defined(__ADSP_DELTA__)    /* no EBIU */         
         /* We must adjust SDRAM when the startup flag is set */
         if( StartUp )
         {        
            adi_ebiu_AdjustSDRAM( fsclk );
         } 
#endif        
    }

    // adjust the RTL global
    SET_PROCESSOR_CYCLES_PER_SEC(fcclk)           

#if defined(__ADSP_TETON__)
    // Allow core B to resume normal activity
    RELEASE_COREB
#endif
    
}

/************************************************************************************
* Function:    adi_pwr_Reset
* Description: Restores the default Power Management settings.
*************************************************************************************/

void 
adi_pwr_Reset(void)
{
    u32 fcclk, fsclk, fvco;
    u16 VrCtl;


        
#if defined(__ADSP_TETON__)
    if (core_ident == ADI_COREA )    {
#endif

    if (!adi_pwr_active_config.Initialized)
        return;

//    ssync();   Removed per anomaly 05-00-0312
    *(u16*)&adi_pwr_active_config.pll_ctl     = ADI_PLL_CTL_RESET;
    *(u16*)&adi_pwr_active_config.pll_div     = ADI_PWR_PLL_DIV_RESET;
    *(u16*)&adi_pwr_active_config.pll_lockcnt = ADI_PWR_PLL_LOCKCNT_RESET;

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__)|| defined(__ADSP_MOY )

    /* For these processors, we do not know the contents of the VLEV fine
       tuning field within VR_CTL, so we use the saved value that was read in adi_pwr_Init */

    VrCtl = (u16)VR_CTL_VTRIM_GET(adi_pwr_active_config.vr_ctl);

    /* OR in the hardware reset value as documented in the HRM */
    VrCtl = VrCtl | (ADI_PWR_VR_CTL_RESET & 0x0FFF0);

    /* and write this value to VR_CTL */
    *(u16*)&adi_pwr_active_config.vr_ctl = VrCtl;

#else
    /* otherwise just use the hardware reset value as documented in the HRM */
    *(u16*)&adi_pwr_active_config.vr_ctl = ADI_PWR_VR_CTL_RESET;

#endif

    // determine actual current SCLK (from MMR)
    adi_pwr_active_config.fsclk = adi_pwr_GetSCLK();

    // retrieve SCLK frequency to set (from Config structure)
    adi_pwr_GetFreq( &fcclk, &fsclk, &fvco );

#if defined(__ADSP_MOAB__) 

    SysControlActionFlag = (SYSCTRL_VRCTL | SYSCTRL_PLLCTL | SYSCTRL_PLLDIV | SYSCTRL_LOCKCNT);
    
    if( vddint_set )
    {    
        SysControlActionFlag |= SYSCTRL_EXTVOLTAGE;    
    }
    else
    {    
        SysControlActionFlag |= SYSCTRL_INTVOLTAGE;    
    }
    
#endif    

    
    /* copy the cached PLL Register File to MMR and adjust SDRAM */
    adi_pwr_ApplyConfig( fcclk, fsclk, 1 ); 

#if defined(__ADSP_TETON__)
    }
#endif

}

/***********************************************************************************
* Function:    adi_pwr_SaveConfig
* Description: Saves the current Power Management settings.
************************************************************************************/

ADI_PWR_RESULT 
adi_pwr_SaveConfig(
        ADI_PWR_CONFIG_HANDLE hConfig, 
        const size_t          szConfig
)
{

    ADI_PWR_CONFIG *config = (ADI_PWR_CONFIG *)hConfig;

    if (!adi_pwr_active_config.Initialized)
        return ADI_PWR_RESULT_NOT_INITIALIZED;

#ifdef ADI_SSL_DEBUG
    if (szConfig < sizeof(ADI_PWR_CONFIG))
        return ADI_PWR_RESULT_NO_MEMORY;
    else if (config==0)
        return ADI_PWR_RESULT_FAILED;
#endif

    // save current settings
    config->pll_ctl      = adi_pwr_active_config.pll_ctl;
    config->pll_div      = adi_pwr_active_config.pll_div;
    config->pll_lockcnt  = adi_pwr_active_config.pll_lockcnt;
    config->vr_ctl       = adi_pwr_active_config.vr_ctl;
    config->proc_type    = adi_pwr_active_config.proc_type;
    config->MaxCoreVoltage  = adi_pwr_active_config.MaxCoreVoltage;
    config->clkin        = adi_pwr_active_config.clkin;
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)    /* no EBIU */    
    adi_ebiu_SaveConfig( (ADI_EBIU_CONFIG_HANDLE*)&config->ebiu_config,ADI_EBIU_SIZEOF_CONFIG);
#endif
    return ADI_PWR_RESULT_SUCCESS;
}

/***********************************************************************************
* Function:        adi_pwr_SetMaxFreqForVolt 
* Description:    Calculates and sets optimal MSEL,DF,CSEL & SSEL for  
*                requested voltage level. 
*                CSEL is always set to 0 & DF is set to 1 if possible.
************************************************************************************/

ADI_PWR_RESULT 
adi_pwr_SetMaxFreqForVolt(
        const ADI_PWR_VLEV vlev
)
{
    u32 fvco, fsclk;
    u32 msel,ssel;
    ADI_PWR_DF df = ADI_PWR_DF_NONE;
    ADI_PWR_VLEV vlev1 = vlev;

    if (!adi_pwr_active_config.Initialized)
        return ADI_PWR_RESULT_NOT_INITIALIZED;

#if defined(__ADSP_TETON__)
    if (core_ident == ADI_COREA )    {
#endif

#ifdef ADI_SSL_DEBUG    
    // If using external voltage regulation we cannot change VLEV with this function call.
    // We can only use the existing MaxCoreVoltage to set the maximum frequency for that voltage.
    if (( vddint_set ) && ( vlev != adi_pwr_active_config.MaxCoreVoltage))
        return ADI_PWR_RESULT_VOLTAGE_REGULATOR_BYPASSED;


    // Ensure that fvco is between bounds for processor type
    if ( vlev < ADI_PWR_VLEV_MIN || vlev > ADI_PWR_VLEV_MAX )
        return ADI_PWR_RESULT_INVALID_VLEV;
        
#endif    

#if defined(__ADSP_EDINBURGH__) && (___SILICON_REVISION__==0xffff || __SILICON_REVISION__==0x1)
    vlev1 = (ADI_PWR_VLEV)VR_CTL_VLEV_GET( *vr_ctl_mmr );
#endif
        
    // Set CCLK freq to maximum possible for given Voltage
    fvco = adi_pwr_cclk_vlev[ADI_PWR_CCLK_VIDX(vlev1)];

    // calculate MSEL and DF values, assuming CSEL = 0;
    msel = adi_pwr_SelectOptimalMSEL(&fvco,&df);

    // set SCLK freq to max available for voltage, and package type
    {
        u16 pkg = (u16)adi_pwr_active_config.package_type;
        u16 vddext = (u16)adi_pwr_active_config.ExternalVoltage;
        if (vlev1 < ADI_PWR_VLEV_SCLK_THRESHOLD)
            fsclk = adi_pwr_sclk_limits[pkg][vddext].sclk_max_level1;
        else
            fsclk = adi_pwr_sclk_limits[pkg][vddext].sclk_max_level2;
    }

    // and calculate SSEL value to achieve it
    ssel = adi_pwr_SelectOptimalSSEL1( fvco, &fsclk );

    // Adjust configuration 
    adi_pwr_SetPLL(msel,df, ADI_PWR_CSEL_1, ssel, vlev1);
    
#if defined(__ADSP_MOAB__) 
    SysControlActionFlag = (SYSCTRL_PLLCTL | SYSCTRL_PLLDIV );

     /* We should not be changing VLEV if using an external VR */
     if( !vddint_set )
     {
         SysControlActionFlag |= (SYSCTRL_VRCTL | SYSCTRL_INTVOLTAGE );
     }
#endif

    /* and program PLL (and SDC only if required) */
    adi_pwr_ApplyConfig( fvco, fsclk, 0 ); 

#if defined(__ADSP_TETON__)
    }
#endif
    return ADI_PWR_RESULT_SUCCESS;    

} 

/***********************************************************************************
* Function:        adi_pwr_SetFreq 
* Description:    Calculates and sets optimal MSEL,DF,CSEL & SSEL for requested core and system
*                clock frequencies. Also the value of the DF factor can be optionally fixed.
************************************************************************************/

ADI_PWR_RESULT 
adi_pwr_SetFreq (
    const u32 fcclk, 
    const u32 fsclk, 
    ADI_PWR_DF df
)
{
    
    ADI_PWR_VLEV vlev, vlev_floor;
    u32 msel,ssel;
    u32 fvco = fcclk;
    u32 fsclk1 = fsclk;
    
    u16 pkg    = (u16)adi_pwr_active_config.package_type;
    u16 vddext = (u16)adi_pwr_active_config.ExternalVoltage;

    if (!adi_pwr_active_config.Initialized)
        return ADI_PWR_RESULT_NOT_INITIALIZED;
    
    // If SCLK is less than one million, multiply it by the megahertz factor            
    if (fsclk1>0 && fsclk1 < 1000000 )
        fsclk1 *= adi_pwr_active_config.MHzFactor;
        
    // If CCLK is less than one million, multiply it by the megahertz factor                    
    if (fvco >0 && fvco < 1000000)
            fvco *= adi_pwr_active_config.MHzFactor;
        
 
    if (fvco > 0 && fvco < ADI_PWR_FVCO_MIN * adi_pwr_active_config.MHzFactor)
        fvco = ADI_PWR_FVCO_MIN*adi_pwr_active_config.MHzFactor;
    

#if defined(__ADSP_EDINBURGH__) && (___SILICON_REVISION__==0xffff || __SILICON_REVISION__==0x1)
    vlev = (ADI_PWR_VLEV)VR_CTL_VLEV_GET( *vr_ctl_mmr );
    fvco = adi_pwr_cclk_vlev[ADI_PWR_CCLK_VIDX(vlev)];
    if (vlev < ADI_PWR_VLEV_SCLK_THRESHOLD)
        fsclk1 = adi_pwr_sclk_limits[pkg][vddext].sclk_max_level1;
#endif

#if defined(__ADSP_TETON__)
    if (core_ident == ADI_COREA )    {
#endif
    // If nothing has been specified get best of both    
    if (fvco==0 && fsclk1==0)
    {
        // get max VCO frequency for max voltage level
        fvco = adi_pwr_cclk_vlev[ADI_PWR_CCLK_VIDX(adi_pwr_active_config.MaxCoreVoltage)];
        fsclk1 = adi_pwr_sclk_limits[pkg][vddext].sclk_max_level2;
    }

    // If CCLK has not been specified, then optimize for SCLK
    //    calculate SSEL, MSEL and then adjust voltage
    if (fvco==0) 
    {
        // get max VCO frequency for max voltage level
        // Anomaly 1 against Rev 0.1 silicon
        fvco = adi_pwr_cclk_vlev[ADI_PWR_CCLK_VIDX(adi_pwr_active_config.MaxCoreVoltage)];
        
        // set SCLK freq to max available for voltage, and package type
        if ( fsclk1 > adi_pwr_sclk_limits[pkg][vddext].sclk_max_level2  )
            fsclk1 = adi_pwr_sclk_limits[pkg][vddext].sclk_max_level2;

        // calculate SSEL value 
        ssel = adi_pwr_SelectOptimalSSEL2(&fvco,&fsclk1);
        
        // calculate MSEL and DF values, assuming CSEL = 0;
        msel = adi_pwr_SelectOptimalMSEL(&fvco,&df);
        
        // If using external VR cannot set VLEV
        if ( vddint_set )
        {
            vlev = adi_pwr_active_config.MaxCoreVoltage;
        }
        else
        {
/* The processor that presently supports the bfrom_SysControl ROM function. */
#if defined(__ADSP_MOAB__) 
        
            SysControlActionFlag |= (SYSCTRL_VRCTL  | SYSCTRL_INTVOLTAGE);
#endif
            
            // find Voltage level 
            if (fsclk1>adi_pwr_sclk_limits[pkg][vddext].sclk_max_level1)
                vlev_floor = ADI_PWR_VLEV_SCLK_THRESHOLD;
            else
                vlev_floor = ADI_PWR_VLEV_MIN;

#if defined(__ADSP_EDINBURGH__) && (___SILICON_REVISION__==0xffff || __SILICON_REVISION__==0x1)
            if ( vlev_floor > vlev )
            {
                vlev_floor = vlev;
                fsclk1 = adi_pwr_sclk_limits[pkg][vddext].sclk_max_level1;
            }
#endif
             
            for (vlev=vlev_floor;
                fvco > adi_pwr_cclk_vlev[ADI_PWR_CCLK_VIDX(vlev)];
                vlev++     );
        
        }
    }
    // otherwise we want to optimise for the CCLK
    //     calculate MSEL, voltage, cap SCLK if necessary & finally calculate SSEL
    else 
    {
        // Ensure that CCLK is between bounds for processor type


//        if (fvco < ADI_PWR_FVCO_MIN)
//            fvco = ADI_PWR_FVCO_MIN;


        if (fvco > adi_pwr_cclk_vlev[ADI_PWR_CCLK_VIDX(adi_pwr_active_config.MaxCoreVoltage)])
            fvco = adi_pwr_cclk_vlev[ADI_PWR_CCLK_VIDX(adi_pwr_active_config.MaxCoreVoltage)];
            
        // calculate MSEL and DF values, assuming CSEL = 0;
        msel = adi_pwr_SelectOptimalMSEL(&fvco,&df);
        
        // find Voltage level 
        // If using external VR cannot set VLEV
        if ( vddint_set )
        {
            vlev = adi_pwr_active_config.MaxCoreVoltage;
            /* On Moab, the VLEV encodings for 1.35V and 1.4V do not fit into 4 bits! */
#if defined(__ADSP_MOAB__)             
            if( vlev > ADI_PWR_VLEV_130 )
            {
                vlev = ADI_PWR_VLEV_130;
            }
#endif                        
        }
        else
        {        
/* The processor that presently supports the bfrom_SysControl ROM function. */
#if defined(__ADSP_MOAB__) 
            
            SysControlActionFlag |= (SYSCTRL_VRCTL  | SYSCTRL_INTVOLTAGE);
            
#endif            
            if (fsclk1>adi_pwr_sclk_limits[pkg][vddext].sclk_max_level1)
                vlev_floor = ADI_PWR_VLEV_SCLK_THRESHOLD;
            else
                vlev_floor = ADI_PWR_VLEV_MIN;
            
#if !defined(__ADSP_EDINBURGH__) || (__SILICON_REVISION__!=0xffff && __SILICON_REVISION__!=0x1)
            for (    vlev=vlev_floor;
                fvco > adi_pwr_cclk_vlev[ADI_PWR_CCLK_VIDX(vlev)] ;
                vlev++ 
            );
#endif
        }    
        
        // adjust SCLK value according to voltage level & package limitations
        if (vlev < ADI_PWR_VLEV_SCLK_THRESHOLD 
            && (fsclk1==0 || fsclk1 > adi_pwr_sclk_limits[pkg][vddext].sclk_max_level1)
            )
        {
            fsclk1 = adi_pwr_sclk_limits[pkg][vddext].sclk_max_level1;
        }
        else if (vlev >= ADI_PWR_VLEV_SCLK_THRESHOLD
            && (fsclk1==0 ||fsclk1 > adi_pwr_sclk_limits[pkg][vddext].sclk_max_level2)
            )
        {
            fsclk1 = adi_pwr_sclk_limits[pkg][vddext].sclk_max_level2;
        }
        // SCLK is limited to CCLK
        if (fsclk1 > fvco)
            fsclk1 = fvco;
            
        // calculate SSEL value 
        ssel = adi_pwr_SelectOptimalSSEL1(fvco,&fsclk1);
    }

    // Adjust configuration 
    adi_pwr_SetPLL( msel, df, ADI_PWR_CSEL_1, ssel, vlev);
    
     
    
/* The processor that presently supports the bfrom_SysControl ROM function. */
#if defined(__ADSP_MOAB__)
    SysControlActionFlag |= (SYSCTRL_PLLCTL | SYSCTRL_PLLDIV);        
#endif
    
    /* and program PLL (and SDC only if required) */
    adi_pwr_ApplyConfig(fvco, fsclk1, 0); 

#if defined(__ADSP_TETON__)
    }
#endif

    return ADI_PWR_RESULT_SUCCESS;
}


#if 1
/***********************************************************************************
* Function:    adi_pwr_AdjustFreq
* Description: Adjusts the CCLK and SCLK frequencies by changing PLL_DIV only
************************************************************************************/


ADI_PWR_RESULT 
adi_pwr_AdjustFreq(                        // Sets the PLL Dividor register to adjust CCLK and/or SCLK
    ADI_PWR_CSEL csel,                        // required CSEL value
    ADI_PWR_SSEL ssel                        // required SSEL value
)
{

    u32 fcclk, fsclk, fvco;

    // Ignore call if module not initialized
    if (!adi_pwr_active_config.Initialized)
        return ADI_PWR_RESULT_NOT_INITIALIZED;


#if defined(__ADSP_TETON__)
    // Frequency change only allowed to be initiated from CoreA
    if (core_ident == ADI_COREA )    {
#endif


#ifdef ADI_SSL_DEBUG    
    if (csel != ADI_PWR_CSEL_NONE && csel > ADI_PWR_CSEL_8 )
        return ADI_PWR_RESULT_INVALID_CSEL;

    if (csel == ADI_PWR_CSEL_NONE && ssel == ADI_PWR_SSEL_NONE)
        return ADI_PWR_RESULT_INVALID_SSEL;

    if (ssel != ADI_PWR_SSEL_NONE && ssel > ADI_PWR_SSEL_15 )
        return ADI_PWR_RESULT_INVALID_SSEL;
#endif

    ADI_PWR_CSEL csel_val = (ADI_PWR_CSEL)PLL_DIV_CSEL_GET(adi_pwr_active_config.pll_div);
    if (csel != ADI_PWR_CSEL_NONE)
        csel_val = csel;

    ADI_PWR_SSEL ssel_val = (ADI_PWR_SSEL)PLL_DIV_SSEL_GET(adi_pwr_active_config.pll_div);
    if (ssel != ADI_PWR_SSEL_NONE)
        ssel_val = ssel;

    if ( (1<<((u16)csel_val)) > (u16)ssel_val )
        return ADI_PWR_RESULT_INVALID_CSEL_SSEL_COMBINATION;

    PLL_DIV_CSEL_SET( adi_pwr_active_config.pll_div, csel_val );
    PLL_DIV_SSEL_SET( adi_pwr_active_config.pll_div, ssel_val );

    // retrieve SCLK frequency to set (from Config structure)
    adi_pwr_GetFreq(&fcclk,&fsclk,&fvco);
    
#if defined(__ADSP_MOAB__)
    SysControlActionFlag |= SYSCTRL_PLLDIV;
#endif
        
    /* copy the cached PLL Register File to MMR and adjust SDRAM if necessary */
    adi_pwr_ApplyConfig( fcclk, fsclk, 0 ); 

#if defined(__ADSP_TETON__)
    }
#endif

    return ADI_PWR_RESULT_SUCCESS;
}
#endif



/***********************************************************************************
{ Function:    adi_pwr_SetVoltageRegulator
* Description: Sets the required voltage regulator settings and adjusts clocks
************************************************************************************/

ADI_PWR_RESULT 
adi_pwr_SetVoltageRegulator(
        ADI_PWR_COMMAND command, 
        void *pArg
)
{
#if defined(__ADSP_EDINBURGH__) && (___SILICON_REVISION__==0xffff || __SILICON_REVISION__==0x1)
    // Anomaly 1 against Rev 0.1 silicon
    return ADI_PWR_RESULT_NOT_SUPPORTED;
#else
#ifdef ADI_SSL_DEBUG    
    ADI_PWR_RESULT result; 
#endif

    if (!adi_pwr_active_config.Initialized)
        return ADI_PWR_RESULT_NOT_INITIALIZED;


#if defined(__ADSP_TETON__)
    if (core_ident == ADI_COREA )    {
#endif
    AdjustPLL = FALSE;
    PLL_prog_sequence = 0;

    RecursiveControlEntry++;
    switch(command) 
    {
        default:
            RecursiveControlEntry--;
            return ADI_PWR_RESULT_BAD_COMMAND;

        case ADI_PWR_CMD_END:
            break;
        case ADI_PWR_CMD_PAIR:
            {
                ADI_PWR_COMMAND_PAIR *cmd = (ADI_PWR_COMMAND_PAIR *)pArg;
#ifndef ADI_SSL_DEBUG
                adi_pwr_SetVoltageRegulator(cmd->kind, (void*)cmd->value);
#else
                if ( (result=adi_pwr_SetVoltageRegulator(cmd->kind, (void*)cmd->value))!=ADI_PWR_RESULT_SUCCESS )
                    return result;
#endif
            }
            break;
        case ADI_PWR_CMD_TABLE:
            {
                ADI_PWR_COMMAND_PAIR *cmd = (ADI_PWR_COMMAND_PAIR *)pArg;
                while (cmd->kind != ADI_PWR_CMD_END)
                {
#ifndef ADI_SSL_DEBUG
                    adi_pwr_SetVoltageRegulator(cmd->kind, (void*)cmd->value);
#else
                    if ( (result=adi_pwr_SetVoltageRegulator(cmd->kind, (void*)cmd->value))!=ADI_PWR_RESULT_SUCCESS )
                        return result;
#endif
                    cmd++;
                }
            }
            break;

#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)

        case ADI_PWR_CMD_SET_VR_VLEV:
            {
                ADI_PWR_VLEV vlev = (ADI_PWR_VLEV)pArg;
#ifdef ADI_SSL_DEBUG    
                if (vlev < ADI_PWR_VLEV_MIN || vlev > ADI_PWR_VLEV_MAX)
                    return ADI_PWR_RESULT_INVALID_VLEV;
#endif

                AdjustPLL = TRUE;
                
                VR_CTL_VLEV_SET( adi_pwr_active_config.vr_ctl, vlev );

            }
            break;
#endif
            
        case ADI_PWR_CMD_SET_VR_FREQ:    // set frequency value
            {
                
                
                // If using external VR cannot set internal VR
                if ( vddint_set )
                    return ADI_PWR_RESULT_VOLTAGE_REGULATOR_BYPASSED;
                
                
                ADI_PWR_VR_FREQ freq = (ADI_PWR_VR_FREQ)pArg;
#ifdef ADI_SSL_DEBUG    
                if (freq > ADI_PWR_VR_FREQ_MAX)    
                    return ADI_PWR_RESULT_INVALID_VR_FREQ;
#endif
                VR_CTL_FREQ_SET( adi_pwr_active_config.vr_ctl, freq );
                if (freq == ADI_PWR_VR_FREQ_POWERDOWN ) 
                {
                    VR_CTL_WAKE_SET( adi_pwr_active_config.vr_ctl, ADI_PWR_VR_WAKE_ENABLED );
                    PLL_prog_sequence = 1;
                }
                AdjustPLL = TRUE;
                break;    
            }
            
#if !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__) && !defined(__ADSP_BRODIE__) && !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)
        
        case ADI_PWR_CMD_SET_VR_GAIN:
            {
                ADI_PWR_VR_GAIN gain = (ADI_PWR_VR_GAIN)pArg;
#ifdef ADI_SSL_DEBUG    
                if (gain > ADI_PWR_VR_GAIN_MAX)
                    return ADI_PWR_RESULT_INVALID_VR_GAIN;
#endif
                VR_CTL_GAIN_SET( adi_pwr_active_config.vr_ctl, gain );
                AdjustPLL = TRUE;
            }
            break;
            
#endif            
            
        case ADI_PWR_CMD_SET_VR_WAKE:
            {
                ADI_PWR_VR_WAKE wake = (ADI_PWR_VR_WAKE)pArg;
#ifdef ADI_SSL_DEBUG    
                if (wake > ADI_PWR_VR_WAKE_MAX)
                    return ADI_PWR_RESULT_INVALID_VR_WAKE;
#endif
                VR_CTL_WAKE_SET( adi_pwr_active_config.vr_ctl, wake);
                AdjustPLL = TRUE;
            }
            break;
#if defined(__ADSP_BRODIE__) || defined(__ADSP_BRAEMAR__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_MOY__)

        case ADI_PWR_CMD_SET_VR_PHYWE:
#ifdef ADI_SSL_DEBUG
            {
                u16 phywe = (u16)(u32)pArg;
                if (phywe > ADI_PWR_VR_PHYWE_ENABLED)
                    return ADI_PWR_RESULT_INVALID_VR_PHYWE;
            }
#endif
            VR_CTL_PHYWE_SET ( adi_pwr_active_config.vr_ctl, (u16)(u32)pArg );
            AdjustPLL = TRUE;
            break;
#endif



#if defined(__ADSP_MOAB__)    

        case ADI_PWR_CMD_SET_VR_GPWE_MXVRWE:
        
#ifdef ADI_SSL_DEBUG
            {
                u16 gpwe = (u16)(u32)pArg;
                if (gpwe > ADI_PWR_VR_GPWE_MXVRWE_ENABLED)
                    return ADI_PWR_RESULT_INVALID_VR_GPWE_MXVRWE;
            }
#endif
            VR_CTL_GPWE_MXVRWE_SET ( adi_pwr_active_config.vr_ctl, (u16)(u32)pArg );
            AdjustPLL = TRUE;
            break;


#endif




#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_MOAB__)    


        case ADI_PWR_CMD_SET_VR_USBWE:
#ifdef ADI_SSL_DEBUG
            {
                u16 usbwe = (u16)(u32)pArg;
                if (usbwe > ADI_PWR_VR_USBWE_ENABLED)
                    return ADI_PWR_RESULT_INVALID_VR_USBWE;
            }
#endif
            VR_CTL_USBWE_SET ( adi_pwr_active_config.vr_ctl, (u16)(u32)pArg );
            AdjustPLL = TRUE;
            break;


#endif




#if defined(__ADSP_BRAEMAR__)  || defined(__ADSP_MOAB__)        

        case ADI_PWR_CMD_SET_VR_CANWE:
#ifdef ADI_SSL_DEBUG
            {
                u16 canwe = (u16)(u32)pArg;
                if (canwe > ADI_PWR_VR_CANWE_ENABLED)
                    return ADI_PWR_RESULT_INVALID_VR_CANWE;
            }
#endif
            VR_CTL_CANWE_SET ( adi_pwr_active_config.vr_ctl, (u16)(u32)pArg );
            AdjustPLL = TRUE;
            break;
#endif        

#if defined(__ADSP_BRODIE__) || defined(__ADSP_BRAEMAR__)  || defined(__ADSP_MOAB__)  || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)     || defined(__ADSP_MOY__)
            
        case ADI_PWR_CMD_SET_VR_CLKBUFOE:
#ifdef ADI_SSL_DEBUG
            {
                u16 phyclkoe = (u16)(u32)pArg;
                if (phyclkoe > ADI_PWR_VR_CLKBUFOE_ENABLED)
                    return ADI_PWR_RESULT_INVALID_VR_CLKBUFOE;
            }
#endif
            VR_CTL_CLKBUFOE_SET ( adi_pwr_active_config.vr_ctl, (u16)(u32)pArg );
            AdjustPLL = TRUE;
            break;

        case ADI_PWR_CMD_SET_VR_CKELOW:
#ifdef ADI_SSL_DEBUG
            {
                u16 ckelow = (u16)(u32)pArg;
                if (ckelow > ADI_PWR_VR_CKELOW_ENABLED)
                    return ADI_PWR_RESULT_INVALID_VR_CKELOW;
            }
#endif
            VR_CTL_CKELOW_SET ( adi_pwr_active_config.vr_ctl, (u16)(u32)pArg );
            AdjustPLL = TRUE;
            break;

#endif  // __ADSP_BRAEMAR__  kook, Moab, ETC

    }
     
    // program the PLL - not all require PLL programming sequence (determined by AdjustPLL)
    RecursiveControlEntry--;



    if (AdjustPLL && !RecursiveControlEntry)
    {
        // synchronize the cores before programming the PLL
#if defined(__ADSP_TETON__)
        SYNC_COREB
#endif



#if defined(__ADSP_MOAB__)

        SysControlActionFlag |= (SYSCTRL_VRCTL  | SYSCTRL_INTVOLTAGE);
#endif

        adi_pwr_WritePMRegs( PLL_prog_sequence );
 

    }

#if defined(__ADSP_TETON__)
        RELEASE_COREB
    }
#endif


    
    return ADI_PWR_RESULT_SUCCESS;
#endif


}

/***********************************************************************************

* Function:    adi_pwr_Terminate

* Description: Resets the initialized flag, and unhooks supplemental interrupt (BF561)

************************************************************************************/

ADI_PWR_RESULT

adi_pwr_Terminate(void)

{

    adi_pwr_active_config.Initialized = 0;
#if defined(__ADSP_TETON__)
    if( core_ident!=ADI_COREA && adi_pwr_active_config.auto_sync_enabled )
    {
        // deregister supplemental interrupt handler function
        adi_int_CECUnhook(adi_pwr_active_config.IVG_SuppInt0,adi_pwr_CoreBSuppInt0Handler,NULL);
        adi_int_SICDisable(ADI_INT_SI0);
    }
#endif

    return ADI_PWR_RESULT_SUCCESS;
}

/***********************************************************************************

* Function:    adi_pwr_SetPowerMode

* Description: Sets the required Power Mode - if valid.

************************************************************************************/


ADI_PWR_RESULT /* section("L1_code") */
adi_pwr_SetPowerMode(ADI_PWR_MODE mode)
{
    /* struct to save wakeup register(s) */
    ADI_INT_WAKEUP_REGISTER SaveIWR;
    
    // return if module not initialized
    if (!adi_pwr_active_config.Initialized)
        return ADI_PWR_RESULT_NOT_INITIALIZED;

#if defined(__ADSP_TETON__)
    // Mode change only allowed to be initiated CoreA
    if (core_ident == ADI_COREA )    
    {
#endif

    // Determine current mode
    ADI_PWR_MODE curmode = adi_pwr_GetPowerMode();
    
    // Check that the transition from the current mode to the requested mode
    // is valid. If not return
    if (!ADI_ALLOWED_TRANSITIONS[mode][curmode] )
        return ADI_PWR_RESULT_INVALID_MODE;
        
    // Changed the cached PLL/VR settings
    switch(mode) {
        case ADI_PWR_MODE_FULL_ON:
            *(u16*)&adi_pwr_active_config.pll_ctl &= 0x7EC1;
            break;
            
        case ADI_PWR_MODE_ACTIVE:
            PLL_CTL_BYPASS_SET( adi_pwr_active_config.pll_ctl, 1);
            PLL_CTL_PLL_OFF_SET(adi_pwr_active_config.pll_ctl, 0);
            break;
            
        case ADI_PWR_MODE_ACTIVE_PLLDISABLED:
            PLL_CTL_BYPASS_SET( adi_pwr_active_config.pll_ctl, 1);
            PLL_CTL_PLL_OFF_SET(adi_pwr_active_config.pll_ctl, 1);
            break;
            
        case ADI_PWR_MODE_SLEEP:
            PLL_CTL_STOPCK_SET(adi_pwr_active_config.pll_ctl, 1);
            break;
            
        case ADI_PWR_MODE_DEEP_SLEEP:
            PLL_CTL_PDWN_SET(adi_pwr_active_config.pll_ctl, 1);
            // Set SDRAM into self-refresh mode to preserve its contents
#if defined(__ADSP_MOAB__)
            adi_ebiu_DDRSelfRefreshEnable();           
#else
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)    /* no EBIU */        
            
            adi_ebiu_SelfRefreshEnable();
#endif    
#endif
            break;

        case ADI_PWR_MODE_HIBERNATE:
            // set the bFREQ bits to 0x00 to power off the core
            VR_CTL_FREQ_SET(adi_pwr_active_config.vr_ctl, 0);
#if defined(__ADSP_BRAEMAR__)
            // For Braemar, the user must choose externally of this module 
            // which wake up events to use. 
            // Here we enable SDRAM not to exit self-refresh if 
            // PC133 compliance is not required.
            if (adi_pwr_active_config.PC133_compliant==ADI_PWR_PC133_COMPLIANCE_DISABLED) {
                VR_CTL_CKELOW_SET(adi_pwr_active_config.vr_ctl,1);
            }
#else
            // For non Braemar cores, we must allow RTC to wake it up
            VR_CTL_WAKE_SET(adi_pwr_active_config.vr_ctl,1);
#endif    
            // and write back to VR_CTL MMR
            *pVR_CTL = *(u16*)&adi_pwr_active_config.vr_ctl;
            
            // Set SDRAM into self-refresh mode to preserve its contents
#if defined(__ADSP_MOAB__)     

            adi_ebiu_DDRSelfRefreshEnable();  
#else          
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__) /* no EBIU */  
            adi_ebiu_SelfRefreshEnable();
#endif   
#endif
            break;
    }
    
#if defined(__ADSP_TETON__)
    // Bring core B to safe state before proceeding
    SYNC_COREB
#endif

    

    // If going from active to full on, or another mode, then SCLK will be changed from CLKIN,
    // to whatever it was befoe we went to ACTIVE so we adjust SDRAM before adjusting the frequency 
    //
    if ( ( !(mode==ADI_PWR_MODE_ACTIVE || mode==ADI_PWR_MODE_ACTIVE_PLLDISABLED) )
    
        &&  (curmode==ADI_PWR_MODE_ACTIVE || curmode==ADI_PWR_MODE_ACTIVE_PLLDISABLED) )
    {
        u32 fcclk, fsclk, fvco;
        adi_pwr_GetFreq(&fcclk, &fsclk, &fvco);
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__) /* no EBIU */
        adi_ebiu_AdjustSDRAM( fsclk );
#endif
    }
    
    
    /* If going to sleep the application itself must disable all wakeups except those that will 
     wake up the processor, externally, or else guarantee that no spurious activity
     causes a premature exit of the PLL programming sequence (IDLE).  For other power modes, 
     we do it here. */
     
    if ( mode ==ADI_PWR_MODE_SLEEP )     
    {
        /* For sleep mode we rely on the application to enable whatever wakeup(s) will wake up the 
         processor, and to disable others, but we must make sure they leave the PLL wakeup enabled */
         
        adi_int_SICWakeup( ADI_INT_PLL_WAKEUP, TRUE );                
    }
    else
    {
        /* For other power modes, not SLEEP, we know which wakeups to leave enabled
         and disable the rest. */
        
        
        /* disable all wakeups, then enable only PLL wakeup */ 
        adi_int_SICGlobalWakeup( FALSE, &SaveIWR );

         // Then enable PLL and RTC or just PLL
        adi_int_SICWakeup( ADI_INT_PLL_WAKEUP, TRUE );
        
        if (( mode ==ADI_PWR_MODE_DEEP_SLEEP ) || ( mode ==ADI_PWR_MODE_HIBERNATE ) )
        {
#if !defined (__ADSP_TETON__)    && !defined (__ADSP_MOY__) && !defined(__ADSP_DELTA__)
            /* enable also the RTC (except Teton, which only wakes from DEEP SLEEP by HW reset) */
            adi_int_SICWakeup( ADI_INT_RTC, TRUE );
#endif    
            
            /* come back out of self-refresh */
#if defined(__ADSP_MOAB__)            
            adi_ebiu_DDRSelfRefreshDisable();        
#endif            
        }
        
 
    } 
    
    /* syscontrol is not used to change power modes.  
      Use PLL programming sequence in L1 away with no use of stack */
      
    adi_pwr_PLLProgrammingSequence(*(u16*)&adi_pwr_active_config.pll_ctl );
 
    
    /* restore all wakeups to the previous state */ 
    adi_int_SICGlobalWakeup( TRUE, &SaveIWR );

 
    /* If going to active (from another mode) then SCLK will equal CLKIN which will be less 
     than it was before, so we adjust SDRAM after adjusting the frequency  */
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__) /* no EBIU */
    if (( mode==ADI_PWR_MODE_ACTIVE || mode==ADI_PWR_MODE_ACTIVE_PLLDISABLED )
        && (!( curmode==ADI_PWR_MODE_ACTIVE || curmode==ADI_PWR_MODE_ACTIVE_PLLDISABLED )))
        
        adi_ebiu_AdjustSDRAM( adi_pwr_active_config.clkin );
#endif

#if defined(__ADSP_TETON__)
    /* Allow core B to resume normal activity */
    RELEASE_COREB
#endif


#if defined(__ADSP_TETON__)
    }
#endif

    return ADI_PWR_RESULT_SUCCESS;
}


/***********************************************************************************
***********************************************************************************

                                Static Functions

***********************************************************************************
************************************************************************************/

/***********************************************************************************
* Function:    adi_pwr_SetInitFlag
* Description: Queries the initialized flag and sets it if unset.  Returns 1, if 
*               already initialized, 0, if not.
************************************************************************************/
static int 
adi_pwr_SetInitFlag()
{
    // mask interrupts to protect the Initialized flag from
    // being accessed elsewhere
    u32 imask = cli();
    
    // Test if already initialized. Return 1 if so.
    if (adi_pwr_active_config.Initialized) {
        sti(imask);
        return 1;
    }
    
    // Set Intitialized flag    
    adi_pwr_active_config.Initialized = 1; 
    
    // unmask interrupts
    sti(imask);
    
    // return 0 to indicate success
    return 0;
}


#if !defined(__ADSP_BRODIE__) && !defined(__ADSP_KOOKABURRA__) && !defined(__ADSP_MOCKINGBIRD__) && !defined(__ADSP_MOAB__)   && !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)   
    
    
static int 
adi_pwr_set_ezkit_values(
        ADI_PWR_EZKIT revno
)
{  
    
    switch (revno) 
    {
#if defined(__ADSP_EDINBURGH__)

        case ADI_PWR_EZKIT_BF533_750MHZ:
            adi_pwr_active_config.proc_type            = ADI_PWR_PROC_BF533SKBC750;
            adi_pwr_active_config.package_type        = ADI_PWR_PACKAGE_MBGA;
            adi_pwr_active_config.clkin                = ADI_PWR_CLKIN_EZKIT_BF533*adi_pwr_active_config.MHzFactor;
            adi_pwr_active_config.ExternalVoltage    = ADI_PWR_VDEXT_EZKIT_BF533;
            break;

        case ADI_PWR_EZKIT_BF533_600MHZ:
            adi_pwr_active_config.proc_type            = ADI_PWR_PROC_BF533SKBC600;
            adi_pwr_active_config.package_type        = ADI_PWR_PACKAGE_MBGA;
            adi_pwr_active_config.clkin                = ADI_PWR_CLKIN_EZKIT_BF533*adi_pwr_active_config.MHzFactor;
            adi_pwr_active_config.ExternalVoltage    = ADI_PWR_VDEXT_EZKIT_BF533;
            break;
#endif

#if defined (__ADSP_TETON__)

        case ADI_PWR_EZKIT_BF561_500MHZ:
            adi_pwr_active_config.proc_type            = ADI_PWR_PROC_BF561SKBCZ500X;
            adi_pwr_active_config.package_type        = ADI_PWR_PACKAGE_MBGA;
            adi_pwr_active_config.clkin                = ADI_PWR_CLKIN_EZKIT_BF561*adi_pwr_active_config.MHzFactor;
            adi_pwr_active_config.ExternalVoltage    = ADI_PWR_VDEXT_EZKIT_BF561;
            break;

        case ADI_PWR_EZKIT_BF561_600MHZ:
            adi_pwr_active_config.proc_type            = ADI_PWR_PROC_BF561SKBCZ_6A; //replaces ADI_PWR_PROC_BF561SKBCZ600X;
            adi_pwr_active_config.package_type        = ADI_PWR_PACKAGE_MBGA;
            adi_pwr_active_config.clkin                = ADI_PWR_CLKIN_EZKIT_BF561*adi_pwr_active_config.MHzFactor;
            adi_pwr_active_config.ExternalVoltage    = ADI_PWR_VDEXT_EZKIT_BF561;
            break;
#endif

#if defined(__ADSP_BRAEMAR__)

        case ADI_PWR_EZKIT_BF537_600MHZ:
            adi_pwr_active_config.proc_type            = ADI_PWR_PROC_BF537SKBC1600;
            adi_pwr_active_config.package_type        = ADI_PWR_PACKAGE_MBGA;
            adi_pwr_active_config.clkin                = ADI_PWR_CLKIN_EZKIT_BF537*adi_pwr_active_config.MHzFactor;
            adi_pwr_active_config.ExternalVoltage    = ADI_PWR_VDEXT_EZKIT_BF537;
            break;

#endif

#if defined(__ADSP_STIRLING__)

        case ADI_PWR_EZKIT_BF538_500MHZ:
            adi_pwr_active_config.proc_type        = ADI_PWR_PROC_BF538BBCZ500;
            adi_pwr_active_config.package_type    = ADI_PWR_PACKAGE_MBGA;
            adi_pwr_active_config.clkin    = ADI_PWR_CLKIN_EZKIT_BF538*adi_pwr_active_config.MHzFactor;
            adi_pwr_active_config.ExternalVoltage    = ADI_PWR_VDEXT_EZKIT_BF538;
            break;
#endif


        default:
            return 0;
    }
    return 1;
}

#endif

/************************************************************************************

* Function:    adi_pwr_PowerSaving

* Description: calculates the power saving as %

*************************************************************************************/



static u32
adi_pwr_PowerSaving(
        u32 fcclk,
        ADI_PWR_VLEV  vlev
)
{
    u32 factor=1,pps;
    u64 numerator = (u64)ADI_PWR_MILLIVOLTS(vlev)*(u64)ADI_PWR_MILLIVOLTS(vlev)*fcclk/100;
    u64 denominator = (u64)ADI_PWR_MILLIVOLTS(ADI_PWR_VLEV_MAX)*(u64)ADI_PWR_MILLIVOLTS(ADI_PWR_VLEV_MAX)
        *(u64)adi_pwr_cclk_vlev[ADI_PWR_CCLK_VIDX(ADI_PWR_VLEV_MAX)]/100;
    numerator = (denominator-numerator)*100;
    if (numerator == 0)
        return 0;
        
    while(numerator < denominator)
    {
        factor*=10;
        numerator *= factor;
    }
    pps = (numerator/denominator)/factor;
    return pps;
}


/***********************************************************************************
* Function:    adi_pwr_SelectOptimalMSEL
* Description: Return the optimal MSEL value for the given
*               VCO frequency and PLL input divider.
************************************************************************************/

static u32
adi_pwr_SelectOptimalMSEL(u32 *fvco, ADI_PWR_DF *df)
{
    u32 msel,msel1,msel2,fvco1,fvco2;
    
    if ((*df)==ADI_PWR_DF_ON) 
    {
        // DF = 1, so just calculate for this value
        msel = (*fvco)*2 / adi_pwr_active_config.clkin;
        *fvco = msel * adi_pwr_active_config.clkin/2;

        // make sure VCO does not fall below the minimum        
        while ( *fvco < ADI_PWR_FVCO_MIN*adi_pwr_active_config.MHzFactor) {
            msel++;
            *fvco = msel * adi_pwr_active_config.clkin/2;
        }

    } 
    else 
    {
        // calculate for DF=0
        msel1 = (*fvco) / adi_pwr_active_config.clkin;

        fvco1 = msel1 * adi_pwr_active_config.clkin;
        

        // make sure VCO does not fall below the minimum
        while (fvco1 < ADI_PWR_FVCO_MIN*adi_pwr_active_config.MHzFactor) {
            msel1++;
            fvco1 = msel1 * adi_pwr_active_config.clkin;
        }

        if (msel1 < 32 && (*df)==ADI_PWR_DF_NONE)
        {
            // if we use DF we can nudge the frequency closer to the required,
            // By increasing the overall multiplication factor by 0.5
            msel2 = 2*msel1 + 1;
            fvco2 = msel2*adi_pwr_active_config.clkin/2;
            if (fvco2 <= *fvco) {
                *df = ADI_PWR_DF_ON;
                *fvco = fvco2;
                msel = msel2;
            } 
            else {
                *fvco = fvco1;
                msel = msel1;
                *df = ADI_PWR_DF_OFF;
            }
        }
        else
        {
            // otherwise go with it
            *fvco = fvco1;
            *df = ADI_PWR_DF_OFF;
            msel = msel1;
        }

    }

    // an arithmetic msel value of 64 is achieved with msel=0 in the register
    if (msel == 64) msel = 0;
    return msel;

}

/***********************************************************************************
* Function:    adi_pwr_SelectOptimalSSEL1
* Description: Returns the Optimal SSEL value for given SCLK and VCO 
*               frequencies. The SCLK freq is adjusted on output. 
*               fvco and the CCLK freq are equivalent if CSEL=1, which is the
*               assumption on entry.
************************************************************************************/
static u32
adi_pwr_SelectOptimalSSEL1(u32 fvco, u32 *fsclk)
{
    u32 ssel;
    
#if defined (__ADSP_MOAB__)
    /* May be a minimum SCLK if non-mobile DDR */
    u32 MinSCLK;
#endif
    
    // return highest value that's within limits
    ssel = fvco/(*fsclk);
            
    
#if defined (__ADSP_MOAB__)

    /* We may need to impose a minimum SCLK if non-mobile DDR part */    
     if( *pADI_EBIU_CHIPID == MOBILE_DDR_CHIPID  )
     {
        /* mobile DDR, no minimum, just use 1 MHz */             
         MinSCLK = 1;
     }
     else
     {
        /* standard DDR, min 83MHz */             
         MinSCLK = 83000000;
     }
 
    
    if (( fvco/ssel > (*fsclk) ) && (fvco/ssel >= MinSCLK))
    
#else

    if ( fvco/ssel > (*fsclk) ) 

#endif    
    
    
        {
            ssel++;
        }

        /* ssel must not exceed 15 */
        if (ssel>15) 
        {
            ssel=15;
        }

    // adjust SCLK frequency
    (*fsclk) = fvco/ssel;
    return ssel;
}

static u32
adi_pwr_SelectOptimalSSEL2(u32 *fvco, u32 *fsclk)
{
    u32 ssel = (*fvco)/(*fsclk);
    u32 ssel_max;

    if ( (*fvco)/ssel > (*fsclk) )
        ssel++;

    ssel_max = ssel;
    for (;ssel>0;ssel--)
    {
        u32 fvco1 = (*fsclk)*ssel;
        if (fvco1 > (*fvco))
            continue;

        // we have reached a solution: adjust workable fvco
        *fvco = fvco1;
        return ssel;

    }
    
    // if it reaches here, a suitable SSEL cannot be selected, 
    // so take a hit on the SCLK value, taking the max SSEL value above
    (*fsclk) = (*fvco)/ssel_max;
    return ssel_max;
}


#if defined(__ADSP_TETON__)

static void
adi_pwr_SetLockVarPointer(
        void *pLockVar        // pointer to lock variable in L2 shared memory 
        )
{
    adi_pwr_active_config.pLockVar = (testset_t*)pLockVar;
}

void
adi_pwr_SyncCoreB(testset_t *pLockVar)    // Synchronise with another core
{
    
    volatile u16 Ack;
    
    // if lock variable is assigned 
    if (pLockVar)
    {
        u32 imask = cli();
        adi_acquire_lock(pLockVar); 
        sti(imask);
    }

    // Raise Supplemental Interrupt
    *pSICB_SYSCR |= 0x0080;    

    // Await acknowledgement from other core
    while(1)
    {
        Ack = *pSICB_SYSCR & 0x0080;
        if( Ack == 0 )
            break;    
    }

}

void 
adi_pwr_ReleaseCoreB(testset_t *pLockVar)
{
    if (pLockVar)
        adi_release_lock(pLockVar);
}

void adi_pwr_SyncCoreA(testset_t *);

ADI_INT_HANDLER(adi_pwr_CoreBSuppInt0Handler)
{
    
    /* struct to save wakeup register(s) */
    ADI_INT_WAKEUP_REGISTER SaveIWR;

    /*  Test for Interrupt - required as we are in an Interrupt Manager handler which could've been chained. */
    if ( (*pSICB_SYSCR&0x0080) != 0x0080)
        return ADI_INT_RESULT_NOT_PROCESSED;

//  REMOVED: Call First Client Callback - callback will not be used in supplemental interrupt
//    ADI_EBIU_CALLBACK_FN callback = adi_pwr_active_config.ClientCallback1;
//    if (callback)
//        (callback) (adi_pwr_active_config.ClientHandle);



    /* save and disable wakeups */        
    adi_int_SICGlobalWakeup( FALSE, &SaveIWR );

    /* enable only the PLL */

    adi_int_SICWakeup( ADI_INT_PLL_WAKEUP, TRUE );    
    adi_int_SICWakeup( ADI_INT_SI1, TRUE );        
        
    // the next call places CoreB in L1 memory with no stack usage (i.e ASM routine)
    // (The interrupt is acknowledged in here, right before it IDLEs)
    adi_pwr_SyncCoreA(adi_pwr_active_config.pLockVar); 
    
    
    /* restore all wakeups to the previous state */ 
    adi_int_SICGlobalWakeup( TRUE, &SaveIWR );

    
    
    
    // Cache the PLL registers
    adi_pwr_active_config.pll_ctl = *(ADI_PWR_PLL_CTL_REG*)pADI_PWR_PLL_CTL;

    adi_pwr_active_config.pll_div = *(ADI_PWR_PLL_DIV_REG*)pADI_PWR_PLL_DIV;

//    ssync();   Removed per anomaly 05-00-0312



    // Retrieve frequencies 

    u32 fcclk,fsclk,fvco;

    adi_pwr_GetFreq(&fcclk,&fsclk,&fvco);


    // adjust the RTL global
    SET_PROCESSOR_CYCLES_PER_SEC(fcclk)


// REMOVED: Call Second Client Callback - callback will not be used in supplemental interrupt
//    callback = adi_pwr_active_config.ClientCallback2;
//    if (callback)
//        (callback) (adi_pwr_active_config.ClientHandle);

    return ADI_INT_RESULT_PROCESSED;
}

#endif // __ADSP_TETON__


