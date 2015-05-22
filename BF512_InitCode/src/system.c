/*****************************************************************************
 Include Files
******************************************************************************/

#include "init_platform.h"

#if defined (__ADSPBF54x__)
    #include "NOR_FLASH.h"
#endif

#if defined (__EXTVOLTAGE__)
    #include "VR_DIGIPOT.h"
#endif

/*****************************************************************************
 Functions
******************************************************************************/


#if defined (__USEBFSYSCONTROL__)


#if defined (__EXTVOLTAGE__)


/****************************************************************************
 Name:          full_on
 Description:   set EXTERNAL Voltage Regulator and PLL Registers
                using the System Control ROM Function
 Input:         none
 Return:        none
*****************************************************************************/

void full_on(void)
{
    ADI_SYSCTRL_VALUES init;
    TRUE SkipVr;
    ERROR_CODE Result;
    u32 ulCnt = 0;
    u8 usRdac = 0;
    #if defined (__WORKAROUND_05000432__)
        u32 SIC_IWR1_reg;
        SIC_IWR1_reg = *pSIC_IWR1;  /* Save value of SIC_IWR1 */
        *pSIC_IWR1 = 0;             /* Disable wakeups from SIC_IWR1 */
    #endif


    /*****************************************************************************
     If you change VDDint without putting the PLL into bypass / active mode, you
     risk running at excessive speed. As in Active mode or in the transition phase
     to other modes, changes to MSEL are not latched by the PLL, the PLL registers
     will be pre-programmed with the desired values when entering Active mode.
     When the PLL is in bypass, the external voltage regulator can be programmed
     ( vr_digipot_program() ) and the active mode can be left afterwards.
    *****************************************************************************/

    usRdac = vr_digipot_rdac_read();

    if ( usRdac == RDAC_STEP_VAL ) {
        init.uwPllCtl = PLL_CTL_VAL;
        SkipVr = YES;
    }
    else {
        init.uwPllCtl = ( PLL_CTL_VAL | BYPASS ); /* pre-load PLL_CTL register with new values as changes to MSEL will not be latched when in or leaving active mode */
        SkipVr = NO;
    }

    init.uwPllLockCnt = PLL_LOCKCNT_VAL;

    #if defined (__WORKAROUND_05000440__)
        *pPLL_DIV = PLL_DIV_VAL;
        bfrom_SysControl( SYSCTRL_EXTVOLTAGE | SYSCTRL_PLLCTL |                  SYSCTRL_LOCKCNT | SYSCTRL_WRITE, &init, NULL );
    #else
        init.uwPllDiv = PLL_DIV_VAL;
        bfrom_SysControl( SYSCTRL_EXTVOLTAGE | SYSCTRL_PLLCTL | SYSCTRL_PLLDIV | SYSCTRL_LOCKCNT | SYSCTRL_WRITE, &init, NULL );
    #endif


    /*****************************************************************************
     The external voltage regulator on the
     ADSP-BF518F EZ-Board and
     ADSP-BF526 EZ-Board
     requires about 100us to stabilize. We keep the core in the meantime in a
     delay loop until we can safely leave active mode. As we bypass the PLL, the 
     core needs to wait 100*(10^-6)*CLKIN cycles. When VDDint is stable, the PLL
     needs additionally 0x0200 (512) CLKIN cycles to re-lock (please see the data
     sheet of your product for required PLL_LOCKCNT value).
     The compiler (no optimization enabled) will create a loop that takes about
     10 cylces -> ( CLKIN[Hz] / ( 10 * 10,000 ) ) or CLKIN_Hz >> 16.
     That is no perfect but robust calculation. It safes execution time.
    *****************************************************************************/

    if ( SkipVr == NO ) {
        Result = vr_digipot_program();
//        if ( Result == ERROR ) { asm("EMUEXCPT;"); while(1); }

        ulCnt = ( ( CLKIN_Hz >> 16 ) + 0x0200 );
        while (ulCnt != 0) {ulCnt--;}

        init.uwPllCtl = PLL_CTL_VAL;

        /*****************************************************************************
         ADSP-BF54x HRM (Dynamic Power Managment):
         Even if the internal voltage regulator is bypassed and the VDDINT voltage is 
         applied by an external regulator, the bfrom_SysControl() routine must be 
         called at startup or whenever the voltage changes at run time. Afterwards, 
         the SYSCTRL_EXTVOLTAGE bit should be set along with a proper VLEV value 
         in the VR_CTL register.
        *****************************************************************************/

        #if defined (__ADSPBF54x__)
            init.uwVrCtl = VR_CTL_VAL;
            bfrom_SysControl( SYSCTRL_EXTVOLTAGE | SYSCTRL_VRCTL | SYSCTRL_PLLCTL | SYSCTRL_WRITE, &init, NULL );
        #else
            bfrom_SysControl( SYSCTRL_EXTVOLTAGE |                 SYSCTRL_PLLCTL | SYSCTRL_WRITE, &init, NULL );
        #endif
    }


    #if defined (__WORKAROUND_05000432__)
        *pSIC_IWR1 = SIC_IWR1_reg;  /* Restore original SIC_IWR1 register value */
    #endif


    return;
}


#else /* !(__EXTVOLTAGE__) */


/****************************************************************************
 Name:          full_on
 Description:   set INTERNAL Voltage Regulator and PLL Registers
                using the System Control ROM Function
 Input:         none
 Return:        none
*****************************************************************************/

void full_on(void)
{
    ADI_SYSCTRL_VALUES init;
    #if defined (__WORKAROUND_05000432__)
        u32 SIC_IWR1_reg;
        SIC_IWR1_reg = *pSIC_IWR1;  /* Save value of SIC_IWR1 */
        *pSIC_IWR1 = 0;             /* Disable wakeups from SIC_IWR1 */
    #endif


    init.uwVrCtl = VR_CTL_VAL;
    init.uwPllCtl = PLL_CTL_VAL;
    init.uwPllLockCnt = PLL_LOCKCNT_VAL;


    #if defined (__WORKAROUND_05000440__)
        *pPLL_DIV = PLL_DIV_VAL;
        bfrom_SysControl( SYSCTRL_VRCTL | SYSCTRL_INTVOLTAGE | SYSCTRL_PLLCTL |                  SYSCTRL_LOCKCNT | SYSCTRL_WRITE, &init, NULL );
    #else
        init.uwPllDiv = PLL_DIV_VAL;
        bfrom_SysControl( SYSCTRL_VRCTL | SYSCTRL_INTVOLTAGE | SYSCTRL_PLLCTL | SYSCTRL_PLLDIV | SYSCTRL_LOCKCNT | SYSCTRL_WRITE, &init, NULL );
    #endif


    #if defined (__WORKAROUND_05000432__)
        *pSIC_IWR1 = SIC_IWR1_reg;  /* Restore original SIC_IWR1 register value */
    #endif


    return;
}


#endif /* (__EXTVOLTAGE__) */


/****************************************************************************
 Name:          get_vco_hz
 Description:   get current PLL VCO frequency in Hz
                using the System Control ROM Function
 Input:         none
 Return:        u32 VCO [Hz]
*****************************************************************************/

u32 get_vco_hz(void)
{
    u32 udMSEL = 0;
    u32 udVCO_Hz = 0;
    ADI_SYSCTRL_VALUES vco;

    bfrom_SysControl ( SYSCTRL_READ | SYSCTRL_PLLCTL, &vco, NULL );

    udMSEL = ( (vco.uwPllCtl & MSEL) >> 9 );
    if ( udMSEL == 0 ) { udMSEL = 64; }
    udVCO_Hz = CLKIN_Hz;
    udVCO_Hz = ( udMSEL * udVCO_Hz );

    return ( udVCO_Hz >> (DF & vco.uwPllCtl) );
}


/****************************************************************************
 Name:          get_cclk_hz
 Description:   get current core clock frequency in Hz
                using the System Control ROM Function
 Input:         none
 Return:        u32 CCLK [Hz]
*****************************************************************************/

u32 get_cclk_hz(void)
{
    u32 udCSEL = 0;
    u32 udSSEL = 0;
    ADI_SYSCTRL_VALUES cclk;

    bfrom_SysControl ( SYSCTRL_READ | SYSCTRL_PLLCTL | SYSCTRL_PLLDIV, &cclk, NULL );

    if (cclk.uwPllCtl & BYPASS) { return CLKIN_Hz; }

    udSSEL = cclk.uwPllDiv;
    udCSEL = ( (udSSEL & CSEL) >> 4);
    udSSEL &= SSEL;

    if (udSSEL & udSSEL < (1 << udCSEL)) return ( get_vco_hz() / udSSEL ); /* SCLK > CCLK */

    return ( get_vco_hz() >> udCSEL );    
}


/****************************************************************************
 Name:          get_sclk_hz
 Description:   get current system clock frequency in Hz
                using the System Control ROM Function
 Input:         nonev
 Return:        u32 SCLK [Hz]
*****************************************************************************/

u32 get_sclk_hz(void)
{
    ADI_SYSCTRL_VALUES sclk;

    bfrom_SysControl ( SYSCTRL_READ | SYSCTRL_PLLCTL | SYSCTRL_PLLDIV, &sclk, NULL );

    if (sclk.uwPllCtl & BYPASS) { return CLKIN_Hz; }

    return ( get_vco_hz() / ( sclk.uwPllDiv & SSEL ) );
}


#else /* !(__USEBFSYSCONTROL__) */


/****************************************************************************
 Name:          full_on
 Description:   set Voltage Regulator and PLL Registers
 Input:         none
 Return:        none
*****************************************************************************/

void full_on(void)
{
    u16 uwIMASK_reg = 0;

    /*****************************************************************************************
     Description:   Dynamic Power Managment

     Important Notice:
     Depending on increasing/decreasing speed (core clock (CCLK) and/or system clock (SCLK)),
     the order of the registers PLL_CTL, PLL_DIV (and VR_CTL) have to be adapted!
     - Increase speed: 1.  VR_CTL, 2. PLL_DIV, 3. PLL_CTL
     - Decrease speed: 1. PLL_CTL, 2. PLL_DIV, 3.  VR_CTL
     If SCLK is changed and SDRAM is enabled, the value for RDIV in the EBIU_SDRRC register has to be re-calculated
    *****************************************************************************************/

    /**************************
    * PLL Lock Count Register *
    **************************/

    *pPLL_LOCKCNT = PLL_LOCKCNT_VAL;


    /*****************************************************************************
     If the new value written to the PLL_CTL or VR_CTL register is the same as the
     previous value, the PLL wake-up occurs immediately (PLL is already locked),
     but the core and system clock are bypassed for the PLL_LOCKCNT duration. For
     this interval, code executes at the CLKIN rate instead of the expected CCLK
     rate. Software guards against this condition by comparing the current value
     to the new value before writing the new value.
    ******************************************************************************/


    /*****************************************************************************
     Description:   Configure Voltage Regulator Control Register
    ******************************************************************************/

    if ( *pVR_CTL != VR_CTL_VAL )
    {
        uwIMASK_reg = cli();    /* disable interrupts, copy IMASK to uwIMASK_reg */
        *pVR_CTL = VR_CTL_VAL;  /* write new value to VR_CTL */
        idle();                 /* drain pipeline, enter idled state, wait for PLL wakeup */
        sti(uwIMASK_reg);       /* after PLL wakeup occurs, restore interrupts and IMASK */
    }


    /*********************************
    * PLL Divide Register            *
    * Can be configured on the fly   *
    * First configure divider,       *
    * than configure MSEL in PLL_CTL *
    *********************************/

    *pPLL_DIV = PLL_DIV_VAL;


    /*****************************************************************************
     Description:   Configure PLL Control Register
    ******************************************************************************/

    if ( *pPLL_CTL != PLL_CTL_VAL )
    {
        uwIMASK_reg = cli();        /* disable interrupts, copy IMASK to uwIMASK_reg */
        *pPLL_CTL = PLL_CTL_VAL;    /* write new value to PLL_CTL */
        idle();                     /* drain pipeline, enter idled state, wait for PLL wakeup */
        sti(uwIMASK_reg);           /* after PLL wakeup occurs, restore interrupts and IMASK */
    }

    return;
}


/****************************************************************************
 Name:          get_vco_hz
 Description:   get current PLL VCO frequency in Hz
 Input:         none
 Return:        u32 VCO [Hz]
*****************************************************************************/

u32 get_vco_hz(void)
{
    u32 udMSEL = 0;
    u32 udVCO_Hz = 0;

    udMSEL = ( (*pPLL_CTL & MSEL) >> 9 );
    if ( udMSEL == 0 ) { udMSEL = 64; }
    udVCO_Hz = CLKIN_Hz;
    udVCO_Hz = ( udMSEL * udVCO_Hz );

    return ( udVCO_Hz >> (DF & *pPLL_CTL) );
}


/****************************************************************************
 Name:          get_cclk_hz
 Description:   get current core clock frequency in Hz
 Input:         none
 Return:        u32 CCLK [Hz]
*****************************************************************************/

u32 get_cclk_hz(void)
{
    u32 udCSEL = 0;
    u32 udSSEL = 0;

    if (*pPLL_CTL & BYPASS) { return CLKIN_Hz; }

    udSSEL = *pPLL_DIV;
    udCSEL = ( (udSSEL & CSEL) >> 4);
    udSSEL &= SSEL;

    if (udSSEL & udSSEL < (1 << udCSEL)) return ( get_vco_hz() / udSSEL ); /* SCLK > CCLK */

    return ( get_vco_hz() >> udCSEL );    
}


/****************************************************************************
 Name:          get_sclk_hz
 Description:   get current system clock frequency in Hz
 Input:         none
 Return:        u32 SCLK [Hz]
*****************************************************************************/

u32 get_sclk_hz(void)
{
    if (*pPLL_CTL & BYPASS) { return CLKIN_Hz; }

    return ( get_vco_hz() / ( *pPLL_DIV & SSEL ) );
}


#endif /* (__USEBFSYSCONTROL__) */


/****************************************************************************
 Name:          verify_clocks
 Description:   verify desired VCO/CCLK/SCLK clock settings
                if the definitions PLL_CTL_VAL & PLL_DIV_VAL should be verified
                the parameters usMSEL & usSSEL must be 0
 Input:         u8 usMSEL, u8 usCSEL, u8 usSSEL
 Return:        ERROR_CODE
*****************************************************************************/

ERROR_CODE verify_clocks(u8 usMSEL, u8 usCSEL, u8 usSSEL)
{
    u32 udVCO_Hz  = 0;
    u32 udCCLK_Hz = 0;
    u32 udSCLK_Hz = 0;

    if ( usMSEL == 0 && usSSEL == 0 ) {
        usMSEL = ( (PLL_CTL_VAL & MSEL) >> 9 );
        usCSEL = ( (PLL_CTL_VAL & CSEL) >> 4 );
        usSSEL = ( (PLL_DIV_VAL & SSEL) >> 0 );
    }

    if ( usMSEL == 0 || usSSEL == 0 ) { return ERROR; }
    if ( usCSEL >  4 ) { return ERROR; }
    if ( usSSEL > 64 ) { return ERROR; }

    udVCO_Hz  = ( CLKIN_Hz * usMSEL );
    udCCLK_Hz = ( udVCO_Hz >> usCSEL );
    udSCLK_Hz = ( udVCO_Hz / usSSEL );

    if ( (udVCO_Hz < VCO_MIN_Hz) || (udVCO_Hz > VCO_MAX_Hz) )
        return ERROR;
    else if ( udCCLK_Hz > CCLK_MAX_Hz )
        return ERROR;
    else if ( udSCLK_Hz > SCLK_MAX_Hz )
        return ERROR;
    else return NO_ERR;
}


#if (__ADSPBF59x__ == 0)
/****************************************************************************
 Name:          async_mem_en
 Description:   enable asynchronous memory
 Input:         none
 Return:        none
*****************************************************************************/

void async_mem_en(void)
{
    /**********************************************
    * Asynchronous Memory Bank Control 0 Register *
    **********************************************/

    *pEBIU_AMBCTL0 = EBIU_AMBCTL0_VAL;


    /**********************************************
    * Asynchronous Memory Bank Control 1 Register *
    **********************************************/

    #ifndef __ADSPBF50x__
        *pEBIU_AMBCTL1 = EBIU_AMBCTL1_VAL;
    #endif


    /*******************************
    * Memory Mode Control Register *
    *******************************/

    /**********************************************
    * Asynchronous Memory Global Control Register *
    ***********************************************/

    #if defined (__ADSPBF50x__) || defined (__ADSPBF51x__) || defined (__ADSPBF52x__)
        *pEBIU_AMGCTL = EBIU_AMGCTL_VAL;
    #endif


    /*****************************************
    * Configure FLASH Mode                   *
    * Choose ONE of the following options    *
    *****************************************/

    #if defined (__ADSPBF54x__)
        SetToAsyncMode(); /* default mode */
//        SetToFlashMode();
//        SetToPageMode();
//        SetToBurstMode();
    #endif

    return;
}
#endif


#if defined (__SDRSDRAM__)


/****************************************************************************
 Name:          sdram_en
 Description:   enable SDR-SDRAM
 Input:         none
 Return:        none
*****************************************************************************/

void sdram_en(void)
{
    u16 *pTmp = 0;

    /* already powered up? */
    if (!(*pEBIU_SDSTAT & SDRS)) { return; }


    /*****************************************************************************
     Description:   Configure and enable SDRAM

     1. Ensure the clock to the SDRAM is stable after the power has stabilized
        for the proper amount of time (typically 100 us).
     2. Write to the SDRAM refresh rate control register (EBIU_SDRRC).
     3. Write to the SDRAM memory bank control register (EBIU_SDBCTL).
     4. Write to and SDRAM memory global control register (EBIU_SDGCTL).
     5. Perform SDRAM access.
    ******************************************************************************/

    /********************************
    * SDRAM Control Status Register *
    ********************************/

    /* Is SDC busy performing an access or an Auto-Refresh? */
    while( (*pEBIU_SDSTAT & SDCI) != SDCI ) { /* wait */ }

    /* clear SDRAM EAB sticky error status (W1C) */
    *pEBIU_SDSTAT |= SDEASE;


    /****************************************************************************
    * SDRAM Refresh Rate Control Register                                       *
    * RDIV has always to be re-calculated according to the actual system clock  *
    * You can either use the pre-defined value that fits to the PLL settings or *
    * call get_rdiv() to calculate RDIV 'on the fly'                            *
    ****************************************************************************/

    #if defined (EBIU_SDRRC_VAL)
        *pEBIU_SDRRC = EBIU_SDRRC_VAL;
    #else
        *pEBIU_SDRRC = get_rdiv();
    #endif


    /*************************************
    * SDRAM Memory Bank Control Register *
    **************************************/

    *pEBIU_SDBCTL = EBIU_SDBCTL_VAL;


    /***************************************
    * SDRAM Memory Global Control Register *
    ***************************************/

    *pEBIU_SDGCTL = EBIU_SDGCTL_VAL;


    /******************************
    * Finalize SDC initialization *
    ******************************/

    ssync();

    /**************************************************************************
     Once the PSSE bit in the EBIU_SDGCTL register is set to 1, and a transfer
     occurs to enabled SDRAM address space, the SDC initiates the SDRAM
     powerup sequence. The exact sequence is determined by the PSM bit in the
     EBIU_SDGCTL register. The transfer used to trigger the SDRAM powerup
     sequence can be either a read or a write. This transfer occurs when the
     SDRAM powerup sequence has completed. This initial transfer takes
     many cycles to complete since the SDRAM powerup sequence must take place.
    **************************************************************************/

    pTmp = (u16*) 0x0;
    *pTmp = 0xBEEF;


    /* wait until the SDC has powered up */
    while( (*pEBIU_SDSTAT & SDRS) == SDRS ) { /* wait */ }

    return;
}


/****************************************************************************
 Name:          u16 get_rdiv (void)
 Description:   Calculate RDIV value for SDRAM Refresh Rate Control Register
                RDIV = ( ( SCLK[MHz] * tREF[ms] ) / NRA ) - ( tRAS + tRP ) [clock cycles]
                RDIV = ( ( SCLK * 10^6 * 1/s * tREF * 10^-3 s ) / NRA ) - ( tRAS + tRP ) [clock cycles]
                SCLK = ( CLKIN * MSEL ) / SSEL = VCO / SSEL
                VCO = _get_vco_hz
                SCLK = _get_sclk_hz
                MSEL = Extracted from PLL_CTL register
                SSEL = Extracted from PLL_DIV register
                tREF = Definition
                NRA  = Definition
                tRAS = Extracted from EBIU_SDGCTL_VAL
                tRP = Extracted from EBIU_SDGCTL_VAL
 Input:         none
 Return:        u16 RDIV value
*****************************************************************************/

u16 get_rdiv (void)
{
    return (( (get_sclk_hz()/1000) * tREF ) / NRA ) - ( ( (EBIU_SDGCTL_VAL & TRAS) >> 6 ) + ( (EBIU_SDGCTL_VAL & TRP) >> 11 ) );
}


#endif /* __SDRSDRAM__ */


#if defined (__DDRSDRAM__)


/****************************************************************************
 Name:          sdram_en
 Description:   enable DDR-SDRAM
 Input:         none
 Return:        none
*****************************************************************************/

void sdram_en(void)
{
    /******************************************************************************************************
     IMPORTANT:
     According to general DDR-SDRAM specification, a frequency of at least 83MHz is necessary !
     Therefore the PLL has to be set before releasing the DDR controller from reset !
     Do not modify reserved bits in this registers!
    ******************************************************************************************************/

    /******************************************************************************************************
     Programming Model:
     Access to the DDR controller registers ONLY can be made after releasing the DDR controller soft reset
     bit in the  reset control register by writing a 1 in bit[0] in the register.
     The user may write to the DDR control registers as long as the controller is not accessing memory
     devices. Otherwise, the controller responds to any writes to its registers after it finishes any
     ongoing memory accesses.
    ******************************************************************************************************/

    /*************************
    * Reset Control Register *
    **************************/

    *pEBIU_RSTCTL |= DDRSRESET;     /* release the DDR controller from reset as per spec */


    /****************************
    * Memory Control 0 Register *
    ****************************/

    *pEBIU_DDRCTL0 = EBIU_DDRCTL0_VAL;


    /****************************
    * Memory Control 1 Register *
    ****************************/

    *pEBIU_DDRCTL1 = EBIU_DDRCTL1_VAL;


    /****************************
    * Memory Control 2 Register *
    ****************************/

    *pEBIU_DDRCTL2 = EBIU_DDRCTL2_VAL;

    return;
}


#endif /* __DDRSDRAM__ */


/****************************************************************************
 EOF
*****************************************************************************/
