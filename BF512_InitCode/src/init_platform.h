/****************************************************************************
 To activate PLL and Voltage Regulator settings  globally, uncomment the
 following definition. This can be activated individually as well within
 the XXX_initcode.h files
****************************************************************************/

//#define __ACTIVATE_DPM__

/*******************************************************************************
*  Exported Types Area
*******************************************************************************/

#ifdef _LANGUAGE_C
    typedef enum
    {
        NO_ERR,
        ERROR,
    } ERROR_CODE;

    typedef enum
    {
        NO,
        YES,
    } TRUE;
#endif

/****************************************************************************
 Include Section: General
*****************************************************************************/

#include <blackfin.h>
#ifndef __OLD_ROM_REV__
    #include <bfrom.h>
#endif
#include "nBlackfin.h"
#include "system.h"

/****************************************************************************
 Include Section, symbolic constants / definitions
*****************************************************************************/

#ifdef _LANGUAGE_C


#include <ccblkfn.h>
#include <sysreg.h>


#else /* !_LANGUAGE_C_ -> Assembly */


#if ( defined (__ADSPBF527_FAMILY__) && __SILICON_REVISION__ >= 0x002 ) || ( defined (__ADSPBF54x__) && __SILICON_REVISION__ >= 0x001 )
/* For ADSP-BF50x, ADSP-BF51x and ADSP-BF59x defined in project options as here it is independent from the silicon revision */
    #define __USEBFSYSCONTROL__ 1
#endif

#if ( defined (__ADSPBF527_FAMILY__) && __SILICON_REVISION__ <= 0x001 ) || ( defined (__ADSPBF54x__) && __SILICON_REVISION__ == 0x000 )
    #define __WORKAROUND_05000392__ 1
#endif

#if defined (__ADSPBF526_FAMILY__) && (__SILICON_REVISION__ == 0x000)
    #define __WORKAROUND_05000432__ 1
#endif

#if ( defined (__ADSPBF51x__) && __SILICON_REVISION__ == 0x000 ) || ( defined (__ADSPBF526_FAMILY__) && __SILICON_REVISION__ == 0x000 ) || ( defined (__ADSPBF527_FAMILY__) && __SILICON_REVISION__ <= 0x002 )
    #define __WORKAROUND_05000440__ 1
#endif

#ifndef __OLD_ROM_REV__
    .import "bfrom.h";
#endif

#include <macros.h>
#include "macros.h"
#include "libdsp.h"


#endif /* _LANGUAGE_C_  */

/****************************************************************************
 Include Section: Target Specific
*****************************************************************************/

#if defined __ADSPBF506F__
    #include "ezkitBF506f_initcode.h"
#endif

#if defined __ADSPBF518__
    #include "ezboardBF518f_initcode.h"
#endif

#if defined __ADSPBF512__
    #include "AIOBF512f_initcode.h"
#endif

#if defined __ADSPBF526__
    #include "ezboardBF526_initcode.h"
#endif

#if defined __ADSPBF527__
    #include "ezkitBF527_initcode.h"
#endif

#if defined __ADSPBF533__
    #include "ezkitBF533_initcode.h"
#endif

#if defined __ADSPBF537__
    #include "ezkitBF537_initcode.h"
#endif

#if defined __ADSPBF538__
    #include "ezkitBF538f_initcode.h"
#endif

#if defined __ADSPBF548__
    #include "ezkitBF548_initcode.h"
#endif

#if defined __ADSPBF561__
    #include "ezkitBF561_initcode.h"
#endif

#if defined __ADSPBF592__
    #include "ezkitBF592_initcode.h"
#endif

/****************************************************************************
 EOF
*****************************************************************************/
