/***********************************************************************/
/*                                                                     */
/*   Copyright(c) 2006 Analog Devices, Inc. All Rights Reserved.       */ 
/*                                                                     */
/*  This software is proprietary and confidential.   By using the      */
/*  software in this file, you agree to the terms of the associated    */
/*  Analog Devices License Agreement.                                  */
/*                                                                     */
/*  File Name:        adi_rtc.c                                        */
/*                                                                     */
/*  Description:                                                       */
/*                  Source code for the Real Time Clock Service.       */
/*                                                                     */		
/***********************************************************************/


/********************************************************************/
/*                                                                  */
/*  Include files                                                   */
/*                                                                  */
/********************************************************************/

#include <services/services.h>        /* System Service includes   */
#include <services/rtc/adi_rtc.h>     /* RTC Service includes     */


/********************************************************************/
/*                                                                  */
/*  Teton isn't supported so stub out all API functions             */
/*                                                                  */
/********************************************************************/
#if defined(__ADSP_TETON__)  

ADI_RTC_RESULT adi_rtc_Init( void *pCriticalRegionArg) {
    return( ADI_RTC_RESULT_SERVICE_NOT_SUPPORTED );
}

ADI_RTC_RESULT adi_rtc_Terminate( void ) {
    return( ADI_RTC_RESULT_SERVICE_NOT_SUPPORTED );
}

ADI_RTC_RESULT adi_rtc_SetDateTime( struct tm *pDateTime) {
    return( ADI_RTC_RESULT_SERVICE_NOT_SUPPORTED );
}

ADI_RTC_RESULT adi_rtc_GetDateTime( struct tm *pDateTime) {
    return( ADI_RTC_RESULT_SERVICE_NOT_SUPPORTED );
}

ADI_RTC_RESULT adi_rtc_InstallCallback(ADI_RTC_EVENT_ID EventID, void *ClientHandle, ADI_DCB_HANDLE DCBHandle, ADI_DCB_CALLBACK_FN ClientCallback, void *Value) {
    return( ADI_RTC_RESULT_SERVICE_NOT_SUPPORTED );
}

ADI_RTC_RESULT adi_rtc_RemoveCallback( ADI_RTC_EVENT_ID EventID ) {
    return( ADI_RTC_RESULT_SERVICE_NOT_SUPPORTED );
}

ADI_RTC_RESULT adi_rtc_EnableWakeup( void ) {
    return( ADI_RTC_RESULT_SERVICE_NOT_SUPPORTED );
}

ADI_RTC_RESULT adi_rtc_DisableWakeup( void ) {
    return( ADI_RTC_RESULT_SERVICE_NOT_SUPPORTED );
}

ADI_RTC_RESULT adi_rtc_ResetStopwatch( u32 Count) {
    return( ADI_RTC_RESULT_SERVICE_NOT_SUPPORTED );
}

#else


/********************************************************************/
/*                                                                  */
/*  Constant Declarations                                           */
/*                                                                  */
/********************************************************************/

/* 
   For tar #39931 
   Until we know whether this is anomaly 315 or 283 or something else,
   insert nop after branch and before MMR write, in WriteCacheRegister.
*/
 
#define Workaround_283_315                        	


/* maximum number of asynchronous events in this service */
#define ADI_RTC_MAX_EVENTS      (8)

/* minimum and maximum values for the fields of the tm date/time structure */
#define ADI_RTC_MIN_SECONDS     (0)
#define ADI_RTC_MAX_SECONDS     (59)
#define ADI_RTC_MIN_MINUTES     (0)
#define ADI_RTC_MAX_MINUTES     (59)
#define ADI_RTC_MIN_HOURS       (0)
#define ADI_RTC_MAX_HOURS       (23)
#define ADI_RTC_MIN_MDAY        (1)
#define ADI_RTC_MAX_MDAY        (31)
#define ADI_RTC_MIN_MONTH       (0)
#define ADI_RTC_MAX_MONTH       (11)
#define ADI_RTC_MIN_YEAR        (0)
#define ADI_RTC_MAX_YEAR        (9999)

#define ADI_RTC_SECONDS_IN_A_DAY    (86400)

#define ADI_RTC_RESULT_SECONDS_OUT_OF_RANGE 0x01   /*  tm struct seconds field out of range */
#define ADI_RTC_RESULT_MINUTES_OUT_OF_RANGE 0x02   /*  tm struct minutes field out of range */ 
#define ADI_RTC_RESULT_HOUR_OUT_OF_RANGE    0x04   /*   tm struct hour field out of range */ 
#define ADI_RTC_RESULT_MDAY_OUT_OF_RANGE    0x08   /*   tm struct day of month field out of range */ 
#define ADI_RTC_RESULT_MONTH_OUT_OF_RANGE   0x10   /*   tm struct month field out of range */ 
#define ADI_RTC_RESULT_YEAR_OUT_OF_RANGE    0x20   /*   tm struct year field out of range */ 

/* bits in the ICTL and ISTAT registers */
#define	ADI_RTC_BIT_STOPWATCH			0x0001		/* Stopwatch Interrupt Enable								*/
#define	ADI_RTC_BIT_ALARM				0x0002		/* Alarm Interrupt Enable									*/
#define	ADI_RTC_BIT_SECOND				0x0004		/* Seconds (1 Hz) Interrupt Enable							*/
#define	ADI_RTC_BIT_MINUTE				0x0008		/* Minutes Interrupt Enable									*/
#define	ADI_RTC_BIT_HOUR				0x0010		/* Hours Interrupt Enable									*/
#define	ADI_RTC_BIT_DAY					0x0020		/* 24 Hours (Days) Interrupt Enable							*/
#define	ADI_RTC_BIT_DAY_ALARM			0x0040		/* Day Alarm (Day, Hour, Minute, Second) Interrupt Enable	*/
#define	ADI_RTC_BIT_WRITE_PENDING		0x4000		/* Write Pending Status										*/
#define	ADI_RTC_BIT_WRITE_COMPLETE		0x8000		/* Write Complete Interrupt Enable							*/

/* bits in the PREN register */
#define ADI_RTC_BIT_PREN				0x0001	    /* Enable Prescaler, RTC Runs @1 Hz	*/


/* register address for the RTC */
#define ADI_RTC_ADDRESS_STAT    ((volatile u32 *)0xFFC00300)
#define ADI_RTC_ADDRESS_ICTL    ((volatile u16 *)0xFFC00304)
#define ADI_RTC_ADDRESS_ISTAT   ((volatile u16 *)0xFFC00308)
#define ADI_RTC_ADDRESS_SWCNT   ((volatile u16 *)0xFFC0030C)
#define ADI_RTC_ADDRESS_ALARM   ((volatile u32 *)0xFFC00310)
#define ADI_RTC_ADDRESS_PREN    ((volatile u16 *)0xFFC00314)
        

/********************************************************************

  These data/macros are used to identify and convert event ID to
  masks in the ISTAT and ICTL registers.
  
*********************************************************************/

/* This array is used to convert an event ID to a mask for setting bits in the */
/* RTC_ICTL register and examining bits in the RTC_ISTAT register.  First the  */
/* Event ID is converted to an index used to index into the array.             */


static u16 RTCEventIDToMask[] = 
{
	ADI_RTC_BIT_SECOND,         /* mask for one second periodic event */
	ADI_RTC_BIT_MINUTE,         /* mask for one minute periodic event */
	ADI_RTC_BIT_HOUR,           /* mask for hourly periodic event     */
	ADI_RTC_BIT_DAY,            /* mask for daily periodic event      */
	ADI_RTC_BIT_STOPWATCH,      /* mask for 'stopwatch' event         */
	ADI_RTC_BIT_ALARM,          /* mask for each day alarm event      */
	ADI_RTC_BIT_DAY_ALARM,      /* mask for once-only alarm event     */
	ADI_RTC_BIT_WRITE_COMPLETE  /* mask for writes complete event     */
};

#define ADI_RTC_GET_BIT_FROM_EVENT_ID(EventID) (RTCEventIDToMask[EventID-ADI_RTC_ENUMERATION_START-1])

/********************************************************************

  These macros are used for the cached registers 
  
*********************************************************************/

typedef enum {          /* enumeration for the cache registers */
    ADI_RTC_CACHE_REGISTER_ICTL,    /* ICTL register */
    ADI_RTC_CACHE_REGISTER_STAT,    /* STAT register */
    ADI_RTC_CACHE_REGISTER_SWCNT,   /* SWCNT register */
    ADI_RTC_CACHE_REGISTER_ALARM,   /* ALARM register */
    ADI_RTC_CACHE_REGISTER_COUNT    /* count of number of cached registers */
} ADI_RTC_CACHE_REGISTER;



/*********************************************************************

      Structures and Data Types                                       

**********************************************************************/

/* structure used to read and write the RTC_STAT register */
/* The bit fields correspond to the register bits         */
typedef struct 
{
    volatile u32 secs:6;    /* number of seconds */
    volatile u32 mins:6;    /* number of minutes */
    volatile u32 hours:5;   /* number of hours */
    volatile u32 days:15;   /* number of days */
} ADI_RTC_STAT_REG;



/* callback entry structure used to build the callback list */
typedef struct ADI_RTC_CALLBACK_ENTRY 
{	
    void *ClientHandle;                 /* defined by the app; passed to callbacks */
    ADI_DCB_HANDLE DCBHandle;           /* handle to deferred callback service */
    ADI_DCB_CALLBACK_FN ClientCallback; /* name of client callback function */
} ADI_RTC_CALLBACK_ENTRY;


/* structure to control writing to cached RTC registers */
typedef struct 
{
    volatile void   *pMMRAddress;       /* address of the real RTC MMR */
    u8              MMRWidth;           /* width of the MMR */
    u8              WritePendingFlag;   /* indicates that the real RTC register has been written to */
    u8              DirtyFlag;          /* indicates that the cache register value has been written to */
    u32             Value;              /* value for the register */
} ADI_RTC_REGISTER;


typedef struct      /* structure to control the RTC */
{
    u8                      InitFlag;                               /* indicates the RTC service was initialized */
    u8                      WakeupFlag;                             /* wakeup on RTC events flag */
    u8                      WritesPendingFlag;                      /* cached register writed pending flag */
    u8                      ClientCallbackCount;                    /* number of callback events the client has */
    u32                     IVG;                                    /* IVG */
    ADI_RTC_EPOCH           EpochTime;                              /* Epoch Time: Jan 1 1970 or user specified */     
    void                    *pEnterCriticalArg;                     /* value for entering crtical regions */
    ADI_RTC_REGISTER        Register[ADI_RTC_CACHE_REGISTER_COUNT]; /* register caches */
    ADI_RTC_CALLBACK_ENTRY  CallbackEntry[ADI_RTC_MAX_EVENTS];      /* data for the client callbacks */
} ADI_RTC;
    


	
/*********************************************************************

  Static storage to manage the RTC

**********************************************************************/

static ADI_RTC RTC = {  /* RTC device */
    FALSE,                  /* initialization flag */
    TRUE,                   /* wakeup flag */
    FALSE,                  /* writes pending flag */
    0,                      /* client callback count */
    0,                      /* IVG */    
    { 1970, 1, 1 },         /* default RTC Epoch Time */
    
    NULL,                   /* critical region parameter */
    {                       /* register caches */
        {   ADI_RTC_ADDRESS_ICTL,  16, FALSE,  FALSE,  0   },  /* ICTL cache */
        {   ADI_RTC_ADDRESS_STAT,  32, FALSE,  FALSE,  0   },  /* STAT cache */
        {   ADI_RTC_ADDRESS_SWCNT, 16, FALSE,  FALSE,  0   },  /* SWCNT cache */
        {   ADI_RTC_ADDRESS_ALARM, 32, FALSE,  FALSE,  0   }   /* ALARM cache */
    },
    {                       /* callback entries */
        {   NULL,   NULL,   NULL    },  /* second callback */
        {   NULL,   NULL,   NULL    },  /* hour callback */
        {   NULL,   NULL,   NULL    },  /* minute callback */
        {   NULL,   NULL,   NULL    },  /* day callback */
        {   NULL,   NULL,   NULL    },  /* stopwatch callback */
        {   NULL,   NULL,   NULL    },  /* alarm callback */
        {   NULL,   NULL,   NULL    },  /* day alarm callback */
        {   NULL,   NULL,   NULL    }   /* write complete */
    }
};



	
/*********************************************************************

   Static functions defined in this module

*********************************************************************/

/* RTC Interrupt Handler */
static ADI_INT_HANDLER ( RTCHandler );    

/* Validates the EventID passed as the argument */
static ADI_RTC_RESULT adi_rtc_ValidateEventID( ADI_RTC_EVENT_ID EventID );

/* Validates tm structure passed as the argument */
static u32 adi_rtc_ValidateDateTime( struct tm* pTmDateTime );

static void SetICTLBits(u16 Bits);      /* safely sets bits in ICTL */
static void ClearICTLBits(u16 Bits);    /* safely clears bits in ICTL */
static void WriteCacheRegister(ADI_RTC_CACHE_REGISTER RegisterID,   u32 Value);     /* safely writes a cache register */

static u32  ConvertTMStructToRTCSTAT(struct tm *tmStruct);              /* converts a (struct tm) datatype to a u32 for RTC_STAT */
static void ConvertRTCSTATToTMStruct(u32 *pValue, struct tm *tmStruct);   /* converts a u32 from RTC_STAT to a (struct tm) datatype */

static s32 ScalarIsLeapYear (u32 yr);
static u32 ScalarMonthsToDays (u32 month);
static s32 ScalarYearsToDays (u32 yr);
static s32 ScalarYMDToScalar (u32 yr, u32 mo, u32 day);
static void ScalarScalarToYMD (s32 scalar, u32 *yr, u32 *mo, u32 *day);
static u32 ScalarDayOfWeek(u32 yr, u32 mo, u32 day);



/********************************************************************

    Function:     RTC Interrupt Handler                    
                                                                   
    Description:  Called from the Interrupt Manager when the RTC
                  Interrupt is serviced. It services all events for
                  which an event flag is raised, in RTC_ISTAT. 

*********************************************************************/

static ADI_INT_HANDLER ( RTCHandler )
{

    ADI_INT_HANDLER_RESULT  Result;             /* return code */
	int                     i;                  /* counter */
	u16                     LocalISTAT;         /* local copy of ISTAT */
	ADI_RTC_REGISTER        *pCacheRegister;    /* pointer to cached register */
	ADI_RTC_CALLBACK_ENTRY  *pCallbackEntry;    /* callback entry */
	
	/* IF (RTC is asserting an interrupt) */
	if (adi_int_SICInterruptAsserted(ADI_INT_RTC) == ADI_INT_RESULT_ASSERTED) {
    	
        /* read the interrupt status register */
        LocalISTAT = *ADI_RTC_ADDRESS_ISTAT;
    	
        /* IF (the service had register writes pending and they indeed completed) */
        if (RTC.WritesPendingFlag && (LocalISTAT & ADI_RTC_BIT_WRITE_COMPLETE)) {
            
            /* clear writes pending flag */
            RTC.WritesPendingFlag = FALSE;
                        
            /* FOR (each cache register) */
            for (i = 0, pCacheRegister = &RTC.Register[0]; i < ADI_RTC_CACHE_REGISTER_COUNT; i++, pCacheRegister++) {
                
                /* IF (this register was being written) */
                if (pCacheRegister->WritePendingFlag) {
                    
                    /* IF (the register is dirty again) */
                    if (pCacheRegister->DirtyFlag) {
                        
                        /* write the new value to the RTC register */
                        if (pCacheRegister->MMRWidth == 16) {
                            *((volatile u16 *)pCacheRegister->pMMRAddress) = pCacheRegister->Value;
                        } else {
                            *((volatile u32 *)pCacheRegister->pMMRAddress) = pCacheRegister->Value;
                        }
                        
                        /* set writes pending flag */
                        RTC.WritesPendingFlag = TRUE;
                        
                        /* clear the dirty flag */
                        pCacheRegister->DirtyFlag = FALSE;
                        
                    /* ELSE */
                    } else {
                        
                        /* clear write pending */
                        pCacheRegister->WritePendingFlag = FALSE;
                        
                    /* ENDIF */
                    }
                    
                /* ENDIF */
                }
                
            /* ENDFOR */
            }
            
        /* ENDIF */
        }
        
        /* IF (we have client callbacks) */
        if (RTC.ClientCallbackCount) {
        	
        	/* FOR (each potential event in ISTAT) */
            for (i = 0, pCallbackEntry = RTC.CallbackEntry; i < ADI_RTC_MAX_EVENTS; i++, pCallbackEntry++ ) {

                /* IF (ISTAT is indicating the event triggered) */
                if (LocalISTAT & RTCEventIDToMask[i]) {
                
                    /* IF (we have a callback installed for that event) */
                    if (pCallbackEntry->ClientCallback) {
                    
                        /* execute the callback */
                        if (pCallbackEntry->DCBHandle) {
                            adi_dcb_Post(pCallbackEntry->DCBHandle, 0, pCallbackEntry->ClientCallback, pCallbackEntry->ClientHandle, (i + ADI_RTC_EVENT_START + 1), NULL);
                        } else {
                            (pCallbackEntry->ClientCallback)(pCallbackEntry->ClientHandle, (i + ADI_RTC_EVENT_START + 1), NULL);
                        }
                    
                    /* ENDIF */
                    }
                
                /* ENDIF */
                }
            
            /* ENDFOR */
            }
            
        /* ENDIF */
        }

        /* clear all pending events */
        *ADI_RTC_ADDRESS_ISTAT = LocalISTAT;

        /* indicate the interrupt was for us */
        Result = ADI_INT_RESULT_PROCESSED;
        
    /* ELSE */
    } else {
        
        /* indicate the interrupt was not for us */
        Result = ADI_INT_RESULT_NOT_PROCESSED;
        
    /* ENDIF */
    }

    /* return */
    return(Result);
}



/*********************************************************************

    Function:     adi_rtc_Init                              
                                                                   
    Description:  Initializes the Real Time Clock Service     
                                                                    
*********************************************************************/

ADI_RTC_RESULT adi_rtc_Init( void *pEnterCriticalArg )  
{

	ADI_RTC_REGISTER        *pCacheRegister;    /* pointer to cached register */
	ADI_RTC_CALLBACK_ENTRY  *pCallbackEntry;    /* callback entry */
    int                     i; 
    ADI_RTC_RESULT          Result;             /* return code */
    
#if defined(ADI_SSL_DEBUG)

    /* make sure the service is not already initialized */
    if ( RTC.InitFlag == TRUE )
    {
        return( ADI_RTC_RESULT_ALREADY_INITIALIZED );
    }
    
#endif

    /* save the critical region parameter */
    RTC.pEnterCriticalArg = pEnterCriticalArg;

    /* Enable prescaler so the RTC functions as a clock */
    *ADI_RTC_ADDRESS_PREN = ADI_RTC_BIT_PREN;
    
    /* clear any pending event flags  */
    *ADI_RTC_ADDRESS_ISTAT = (ADI_RTC_BIT_SECOND | ADI_RTC_BIT_MINUTE | ADI_RTC_BIT_HOUR | ADI_RTC_BIT_DAY | ADI_RTC_BIT_STOPWATCH | ADI_RTC_BIT_ALARM | ADI_RTC_BIT_DAY_ALARM | ADI_RTC_BIT_WRITE_COMPLETE);
    
    /* initialize our register caches */
    for (i = 0, pCacheRegister = RTC.Register; i < ADI_RTC_CACHE_REGISTER_COUNT; i++, pCacheRegister++) {
        pCacheRegister->WritePendingFlag = FALSE;
        pCacheRegister->DirtyFlag = FALSE;
        pCacheRegister->Value = 0;
    }
    
    /* initialize that no writes are pending */
    RTC.WritesPendingFlag = FALSE;
    
    /* initialize that we have no client callbacks */
    RTC.ClientCallbackCount = 0;
    for (i = 0, pCallbackEntry = RTC.CallbackEntry; i < ADI_RTC_MAX_EVENTS; i++, pCallbackEntry++ ) {
        pCallbackEntry->ClientCallback = NULL;
    }
    
    /* initialize wakeup status to TRUE */
    RTC.WakeupFlag = TRUE;
    
    /* get the IVG for the RTC */
    adi_int_SICGetIVG(ADI_INT_RTC, &RTC.IVG);
    
    /* IF (we can hook our handler into the IVG chain) */
    if (adi_int_CECHook (RTC.IVG, RTCHandler, NULL, TRUE) == ADI_INT_RESULT_SUCCESS) {
        
        /* indicate that we're successful */
        Result = ADI_RTC_RESULT_SUCCESS;
        
        /* enable the write complete event */
        SetICTLBits(ADI_RTC_BIT_WRITE_COMPLETE);
        
        /* clear all other bits in ICTL */
        ClearICTLBits(ADI_RTC_BIT_SECOND | ADI_RTC_BIT_MINUTE | ADI_RTC_BIT_HOUR | ADI_RTC_BIT_DAY | ADI_RTC_BIT_STOPWATCH | ADI_RTC_BIT_ALARM | ADI_RTC_BIT_DAY_ALARM);
        
        /* enable the RTC interrupt and wakeup */
        adi_int_SICEnable(ADI_INT_RTC);
        adi_int_SICWakeup(ADI_INT_RTC, TRUE);
        
        /* set our init flag */
        RTC.InitFlag = TRUE;
        
    /* ELSE */
    } else {
        
        /* indicate that we failed */
        Result = ADI_RTC_RESULT_INTERRUPT_MANAGER_ERROR;
        
    /* ENDIF */
    }

    /* return */
    return (Result);
}


/*********************************************************************

    Function:     adi_rtc_Terminate

    Description:  Removes the Real Time Clock Service.  Don't need to 
                  worry about pending writes or anything as we're
                  terminating the service and our writes to the registers
                  will get latched.  

*********************************************************************/

ADI_RTC_RESULT adi_rtc_Terminate( void )
{
    ADI_RTC_RESULT Result;
  
#if defined(ADI_SSL_DEBUG)

    /* make sure the service is initialized */
    if ( RTC.InitFlag == FALSE )
    {
        return (ADI_RTC_RESULT_NOT_INITIALIZED);
    }
    
#endif    
    
    /* disable RTC interrupts */
    adi_int_SICDisable(ADI_INT_RTC);
        
    /* IF (we can unhook our interrupt handler) */
    if (adi_int_CECUnhook (RTC.IVG, RTCHandler, NULL) == ADI_INT_RESULT_SUCCESS) {
    
        /* tell the RTC to not generate any more interrupts */
        *ADI_RTC_ADDRESS_ICTL = 0;
        
        /* looks like we were successful */
        Result = ADI_RTC_RESULT_SUCCESS;
        
    /* ELSE */
    } else {
        
        /* looks like we weren't successful */
        Result = ADI_RTC_RESULT_INTERRUPT_MANAGER_ERROR;
        
    /* ENDIF */
    }
    
    /* clear the init flag */
    RTC.InitFlag = FALSE;
    
    /* return */
    return (Result);
}


/*********************************************************************

    Function:      adi_rtc_SetDateTime

    Description:   Writes the specified current date and Time to 
                   the RTC_STAT register.  

*********************************************************************/
	
ADI_RTC_RESULT adi_rtc_SetDateTime( struct tm* pTmDateTime ) 
{
	
    u32     DateTime;  /* a datetime structure that corresponds to the register */

#if defined(ADI_SSL_DEBUG)

    /* make sure the service is initialized */
    if ( RTC.InitFlag == FALSE )
    {
        return( ADI_RTC_RESULT_NOT_INITIALIZED );
    }
    
    /* check the fields of the tm struct */   
    if (adi_rtc_ValidateDateTime( pTmDateTime )  != 0)
    {
    	return( ADI_RTC_RESULT_DATETIME_OUT_OF_RANGE );
    }
    
#endif

    /* convert to the STAT register format */
    DateTime = ConvertTMStructToRTCSTAT(pTmDateTime);
   
    /* safely write the STAT register */
    WriteCacheRegister(ADI_RTC_CACHE_REGISTER_STAT, DateTime);
        
    /* return */
    return( ADI_RTC_RESULT_SUCCESS );
    
}
                                  


/*********************************************************************

    Function:      adi_rtc_GetDateTime

    Description:  Reads the current date and Time from the 
                  RTC_STAT register. 

*********************************************************************/
ADI_RTC_RESULT adi_rtc_GetDateTime( struct tm *pTmDateTime ) 
{
   
    u32 DateTime;   /* value from the RTC_STAT register */

#if defined(ADI_SSL_DEBUG)

    /* make sure the service is initialized */
    if ( RTC.InitFlag == FALSE )
    {
        return( ADI_RTC_RESULT_NOT_INITIALIZED );
    }

#endif

    /* Read the hardware register and store the results */
    DateTime = *ADI_RTC_ADDRESS_STAT; 

    /* convert to tm struct */
    ConvertRTCSTATToTMStruct(&DateTime, pTmDateTime);
    
    return( ADI_RTC_RESULT_SUCCESS );
    
}


/*********************************************************************

    Function:     adi_rtc_InstallCallback

    Description:  Installs callback functionality for a given event


*********************************************************************/

ADI_RTC_RESULT adi_rtc_InstallCallback(	
    ADI_RTC_EVENT_ID    EventID,        /* ID of the RTC event */
    void                *ClientHandle,  /* client handle argument to callbacks */
    ADI_DCB_HANDLE      DCBHandle,      /* deferred callback service handle */
    ADI_DCB_CALLBACK_FN ClientCallback, /* name of client callback function */
    void                *Value          /* stopwatch #seconds, alarm date/time */
) 
{

    void                    *pExitCriticalArg;	/* argument returned from 'enter critical region' function */
    ADI_RTC_RESULT          Result;             /* result code to pas back to caller */	
    u32                     DateTime;           /* value for the RTC_ALARM register */
    ADI_RTC_CALLBACK_ENTRY  *pCallbackEntry;    /* pointer to the callback entry */  	
    
#if defined( ADI_SSL_DEBUG )

    /* make sure the service is initialized */
    if( RTC.InitFlag == FALSE )
    {
        return( ADI_RTC_RESULT_NOT_INITIALIZED );
    }
    
    /* validate the event ID */    
    if( adi_rtc_ValidateEventID( EventID ) !=  ADI_RTC_RESULT_SUCCESS ) 
    {
        return( ADI_RTC_RESULT_INVALID_EVENT_ID );
    }
    
    /* validate the date/time for alarm events */
    if(( EventID == ADI_RTC_EVENT_EACH_DAY_ALARM ) || ( EventID == ADI_RTC_EVENT_ONCE_ALARM )) {
        if (adi_rtc_ValidateDateTime((struct tm*) Value ) != 0) {
    	    return( ADI_RTC_RESULT_DATETIME_OUT_OF_RANGE );
        }
    }
	
    /* validate that we'll only have one alarm, either daily or once only */
    if(( EventID == ADI_RTC_EVENT_EACH_DAY_ALARM ) || ( EventID == ADI_RTC_EVENT_ONCE_ALARM )) {
        if ((RTC.CallbackEntry[ADI_RTC_GET_BIT_FROM_EVENT_ID(ADI_RTC_EVENT_EACH_DAY_ALARM)].ClientCallback != NULL) &&
            (RTC.CallbackEntry[ADI_RTC_GET_BIT_FROM_EVENT_ID(ADI_RTC_EVENT_ONCE_ALARM)].ClientCallback != NULL)) {
            	return (ADI_RTC_RESULT_CALLBACK_CONFLICT);
            }
    }
        

#endif

    /* point to the callback entry */
    pCallbackEntry = &RTC.CallbackEntry[EventID - (ADI_RTC_EVENT_START + 1)];

    /* protect this region of code */
    pExitCriticalArg = adi_int_EnterCriticalRegion(RTC.pEnterCriticalArg );

    /* IF (the event is free) */
    if (pCallbackEntry->ClientCallback == NULL ) {
        	
        /* OK to install the callback, saving the info to pass to the callback */
        pCallbackEntry->ClientHandle      = ClientHandle;
        pCallbackEntry->DCBHandle         = DCBHandle;
        pCallbackEntry->ClientCallback    = ClientCallback;
         
        /* increment the count of client callback events */
        RTC.ClientCallbackCount++;
        
        /* so far so good */
        Result = ADI_RTC_RESULT_SUCCESS;
        
    /* ELSE */
    } else {
        
        /* the event is already hooked */
        Result = ADI_RTC_RESULT_CALLBACK_ALREADY_INSTALLED;
        
    /* ENDIF */
    }

    /* unprotect this region of code */
    adi_int_ExitCriticalRegion(pExitCriticalArg);        
 
    /* IF (no errors) */
    if (Result == ADI_RTC_RESULT_SUCCESS) {
        
        /* CASEOF (EventID) */
        switch (EventID) {
            
            /* CASE (once alarm or daily alarm) */
            case ADI_RTC_EVENT_ONCE_ALARM:
            case ADI_RTC_EVENT_EACH_DAY_ALARM:
            
                /* convert to the STAT register format */
                DateTime = ConvertTMStructToRTCSTAT((struct tm *)Value);
   
                /* safely write the date/time to the alarm register */
                WriteCacheRegister(ADI_RTC_CACHE_REGISTER_ALARM, DateTime);
                
                /* trigger the proper interrupt */
                if (EventID == ADI_RTC_EVENT_ONCE_ALARM) {
                    SetICTLBits(ADI_RTC_BIT_DAY_ALARM);
                } else {
                    SetICTLBits(ADI_RTC_BIT_ALARM);
                }
                break;
                
            /* CASE (stopwatch event) */
            case ADI_RTC_EVENT_STOPWATCH:
            
                /* write the count to the stopwatch register */
                WriteCacheRegister(ADI_RTC_CACHE_REGISTER_SWCNT, (u32)Value);
                
                /* safely set the proper bit in the ICTL register */
                SetICTLBits(ADI_RTC_BIT_STOPWATCH);
                break;
                
            /* CASEELSE */
            default:
            
                /* safely set the proper bit in the ICTL register */
                SetICTLBits(ADI_RTC_GET_BIT_FROM_EVENT_ID(EventID));
                break;
                
        /* ENDCASE */
        }
        
    /* ENDIF */
    }
    
    /* return */
    return(Result);
    
}


/*********************************************************************

   Function:      adi_rtc_RemoveCallback

   Description:   Removes callback functionality for a given event


*********************************************************************/

ADI_RTC_RESULT adi_rtc_RemoveCallback( ADI_RTC_EVENT_ID EventID )
{
	
    ADI_RTC_RESULT          Result;             /* result */
    ADI_RTC_CALLBACK_ENTRY  *pCallbackEntry;    /* pointer to the callback entry */  	
    void                    *pExitCriticalArg;	/* critical region parameter */
    u16                     Bit;                /* bit for the event */
	 
#if defined( ADI_SSL_DEBUG )

    /* make sure the service is initialized */
    if( RTC.InitFlag == FALSE )
    {
        return( ADI_RTC_RESULT_NOT_INITIALIZED );
    }
    
    /* validate the event ID */    
    if( adi_rtc_ValidateEventID( EventID ) !=  ADI_RTC_RESULT_SUCCESS ) 
    {
        return( ADI_RTC_RESULT_INVALID_EVENT_ID );
    }

#endif

    /* point to the callback entry */
    pCallbackEntry = &RTC.CallbackEntry[EventID - (ADI_RTC_EVENT_START + 1)];

    /* protect this region of code */
    pExitCriticalArg = adi_int_EnterCriticalRegion(RTC.pEnterCriticalArg );

    /* IF (the event is active) */
    if (pCallbackEntry->ClientCallback != NULL ) {
        	
        /* remove the callback from the list */
        pCallbackEntry->ClientCallback    = NULL;
         
        /* decrement the count of client callback events */
        RTC.ClientCallbackCount--;
        
        /* safely clear the event bit in the ICTL register (but always leave WRITE_COMPLETE enabled) */
        Bit = ADI_RTC_GET_BIT_FROM_EVENT_ID(EventID);
        if (Bit != ADI_RTC_BIT_WRITE_COMPLETE) {
            ClearICTLBits(Bit);
        }
        
        /* looks good */
        Result = ADI_RTC_RESULT_SUCCESS;
        
    /* ELSE */
    } else {
        
        /* the event isn't installed */
        Result = ADI_RTC_RESULT_CALLBACK_NOT_INSTALLED;
        
    /* ENDIF */
    }

    /* unprotect this region of code */
    adi_int_ExitCriticalRegion(pExitCriticalArg);        
 
    /* return */
    return( Result );
}
	

/*********************************************************************

    Function:      adi_rtc_EnableWakeup

    Description:   Enables the RTC interrupt to wake up the processor

*********************************************************************/

ADI_RTC_RESULT adi_rtc_EnableWakeup( void ) 
{

	
#if defined( ADI_SSL_DEBUG )

    /* make sure the service is initialized */
    if ( RTC.InitFlag == FALSE )
    {
        return( ADI_RTC_RESULT_NOT_INITIALIZED );
    }
    
#endif

    /* save our status and tell the SIC */
    RTC.WakeupFlag = TRUE;
    adi_int_SICWakeup( ADI_INT_RTC, TRUE );
    return( ADI_RTC_RESULT_SUCCESS );
    
    
}

/*********************************************************************

    Function:      adi_rtc_DisableWakeup

    Description:   Disables the RTC wakeup to the processor

*********************************************************************/
 
ADI_RTC_RESULT adi_rtc_DisableWakeup( void ) 
{
	

#if defined( ADI_SSL_DEBUG )

    /* make sure the service is initialized */
    if ( RTC.InitFlag == FALSE )
    {
        return( ADI_RTC_RESULT_NOT_INITIALIZED );
    }
 
#endif

    /* save our status and tell the SIC */
    RTC.WakeupFlag = FALSE;
    adi_int_SICWakeup( ADI_INT_RTC, FALSE );
    return( ADI_RTC_RESULT_SUCCESS );    
    
    
}



/*********************************************************************

    Function:    adi_rtc_ResetStopwatch

    Description: Sets the stopwatch timeout register so the 
                 stopwatch occurs again.  Used only when 
                 stopwatch has already been enabled and a 
                 callback is already installed.

*********************************************************************/

ADI_RTC_RESULT adi_rtc_ResetStopwatch( u32 Count )
{
	
#if defined( ADI_SSL_DEBUG )

    /* make sure the service is initialized */
    if( RTC.InitFlag == FALSE )
    {
        return( ADI_RTC_RESULT_NOT_INITIALIZED );
    }
    
#endif

    /* safely write to the SWCNT register */	
    WriteCacheRegister(ADI_RTC_CACHE_REGISTER_SWCNT, Count);
        
    /* return */
    return( ADI_RTC_RESULT_SUCCESS );
}


/*********************************************************************

    Function:    adi_rtc_GetEpoch

    Description: Returns the epoch's year, month and day.

*********************************************************************/

ADI_RTC_RESULT adi_rtc_GetEpoch( ADI_RTC_EPOCH *pEpoch)
{
	pEpoch->year = RTC.EpochTime.year;
	pEpoch->month = RTC.EpochTime.month;
	pEpoch->day = RTC.EpochTime.day;
  	
    /* return */
    return( ADI_RTC_RESULT_SUCCESS );
}


/*********************************************************************

    Function:    adi_rtc_SetEpoch

    Description: Sets the epoch's year, month and day.

*********************************************************************/
ADI_RTC_RESULT adi_rtc_SetEpoch(ADI_RTC_EPOCH *pEpoch)
{
	RTC.EpochTime.year = pEpoch->year;
	RTC.EpochTime.month = pEpoch->month;
	RTC.EpochTime.day = pEpoch->day;	
	
    /* return */
    return( ADI_RTC_RESULT_SUCCESS );
}


/*********************************************************************

    Function:    SetICTLBits

    Description: Safely sets bits in the ICTL register

*********************************************************************/
static void SetICTLBits(u16 Bits)
{

	ADI_RTC_REGISTER    *pCacheRegister;    /* pointer to cached register */
	void                *pExitCriticalArg;	/* critical region parameter */
    
    /* point to the register cache */
    pCacheRegister = &RTC.Register[ADI_RTC_CACHE_REGISTER_ICTL];
    
    /* protect us */
    pExitCriticalArg = adi_int_EnterCriticalRegion(RTC.pEnterCriticalArg );
    
    /* set our cached value to what the caller wants */
    pCacheRegister->Value |= Bits;
        
    /* unprotect us */
    adi_int_ExitCriticalRegion(pExitCriticalArg);
    
    /* do the write */
    WriteCacheRegister(ADI_RTC_CACHE_REGISTER_ICTL, pCacheRegister->Value);

    /* return */
}

    
/*********************************************************************

    Function:    ClearICTLBits

    Description: Safely clear bits in the ICTL register

*********************************************************************/
static void ClearICTLBits(u16 Bits)
{
    
	ADI_RTC_REGISTER    *pCacheRegister;    /* pointer to cached register */
	void                *pExitCriticalArg;	/* critical region parameter */
	
    /* point to the register cache */
    pCacheRegister = &RTC.Register[ADI_RTC_CACHE_REGISTER_ICTL];
    
    /* protect us */
    pExitCriticalArg = adi_int_EnterCriticalRegion(RTC.pEnterCriticalArg );
    
    /* set our cached value to what the caller wants */
    pCacheRegister->Value &= ~Bits;
        
    /* unprotect us */
    adi_int_ExitCriticalRegion(pExitCriticalArg);
    
    /* do the write */
    WriteCacheRegister(ADI_RTC_CACHE_REGISTER_ICTL, pCacheRegister->Value);

    /* return */
}


    
/*********************************************************************

    Function:    WriteCacheRegister

    Description: Safely writes a cached MMR

*********************************************************************/
static void WriteCacheRegister(ADI_RTC_CACHE_REGISTER RegisterID, u32 Value)
{

	ADI_RTC_REGISTER    *pCacheRegister;    /* pointer to cached register */
	void                *pExitCriticalArg;	/* critical region parameter */

	/* point to the register cache */
    pCacheRegister = &RTC.Register[RegisterID];
    
    /* protect us */
    pExitCriticalArg = adi_int_EnterCriticalRegion(RTC.pEnterCriticalArg );
    
    /* IF (the MMR has not been written) */
    if (pCacheRegister->WritePendingFlag == FALSE) {
        
        /* set write pending flags */
        pCacheRegister->WritePendingFlag = TRUE;
        RTC.WritesPendingFlag = TRUE;
        
        /* write to the real MMR */
        if (pCacheRegister->MMRWidth == 16) {
#if defined(Workaround_283_315)                        	
            asm("nop; ");
#endif
            *((volatile u16 *)pCacheRegister->pMMRAddress) = Value;
        } else {
#if defined(Workaround_283_315)                        	
            asm("nop; ");
#endif
            *((volatile u32 *)pCacheRegister->pMMRAddress) = Value;
        }
        
    /* ELSE */
    } else {
        
        /* update our cached value */
        pCacheRegister->Value = Value;

        /* mark the register as dirty */
        pCacheRegister->DirtyFlag = TRUE;
        
    /* ENDIF */
    }

    /* unprotect us */
    adi_int_ExitCriticalRegion(pExitCriticalArg);
    
    /* return */
}




/*********************************************************************

     Function:    ConvertTMStructToRTCStat

     Description: Converts a (struct tm) datatype to a value that can
                  be stored in the RTC_STAT or RTC_ALARM register
                  
*********************************************************************/
static u32 ConvertTMStructToRTCSTAT(struct tm *tmStruct)
{
	
    ADI_RTC_STAT_REG DateTime;  /* value for the RTC register */
    s32 Scalar;
    s32 EpochScalar;
    
    /* save the time of day for RTC register */
    DateTime.secs  = tmStruct->tm_sec;
    DateTime.mins  = tmStruct->tm_min;
    DateTime.hours = tmStruct->tm_hour;

    /* convert year, month and day to scalar
       NOTE: the scalar functions expect year to be absolute (not relative), month to be from 
       1 to 12, and day to be from 1 to 31.  This is different than tmStruct which uses year
       relative to 1970, month from 0 to 11 and day from 1 to 31.  Hence the adjustments. */
    Scalar = ScalarYMDToScalar (tmStruct->tm_year + 1900, tmStruct->tm_mon + 1, tmStruct->tm_mday);
    
    /* convert epoch to scalar */
    EpochScalar = ScalarYMDToScalar (RTC.EpochTime.year, RTC.EpochTime.month,  RTC.EpochTime.day );
    
    /* compute number of days difference and store in RTC register */
    DateTime.days = Scalar - EpochScalar;
    
    /* return */
    return (*((u32 *)&DateTime));
}
    
    
   

	
/*********************************************************************

     Function:    ConvertRTCStatToTMStruct

     Description: Converts the value from the RTC_STAT or RTC_ALARM 
                  register into a (struct tm) datatype.

*********************************************************************/
static void ConvertRTCSTATToTMStruct(u32 *pValue, struct tm *tmStruct)
{
	
    ADI_RTC_STAT_REG *pDateTime;  /* value for the RTC register */
    s32 Scalar;
    s32 EpochScalar;
    s32 FirstOfYearScalar;
    u32 year, month, day;
    
    /* avoid casts */
    pDateTime = (ADI_RTC_STAT_REG *)(pValue);
    
    /* extract the time of day from the RTC value and store in the tmStruct */
    tmStruct->tm_sec = pDateTime->secs;
    tmStruct->tm_min = pDateTime->mins;
    tmStruct->tm_hour = pDateTime->hours;

    /* convert epoch to scalar */
    EpochScalar = ScalarYMDToScalar (RTC.EpochTime.year, RTC.EpochTime.month, RTC.EpochTime.day );
    
    /* add the number of days from the RTC value to the epoch to get the current scalar */
    Scalar = EpochScalar + pDateTime->days;
    
    /* convert scalar to year, month and day */
    ScalarScalarToYMD (Scalar, &year, &month, &day);
    
    /* store year, month and day in tmStruct 
       NOTE: the scalar functions expect year to be absolute (not relative), month to be from 
       1 to 12, and day to be from 1 to 31.  This is different than tmStruct which uses year
       relative to 1970, month from 0 to 11 and day from 1 to 31.  Hence the adjustments. */
    tmStruct->tm_year = year - 1900;
    tmStruct->tm_mon = month - 1;
    tmStruct->tm_mday = day;
    
    /* compute and store day of week */
    tmStruct->tm_wday = ScalarDayOfWeek(year, month, day);
    
    /* compute and store day of year */
    FirstOfYearScalar = ScalarYMDToScalar(year, 1, 1);
    tmStruct->tm_yday = Scalar - FirstOfYearScalar;
    
    /* we don't support daylight savings time */
    tmStruct->tm_isdst = 0;
    
    /* return */
	return;
}
	
	
	
/*********************************************************************

     Function:    ScalarXXX

     Description: These functions convert back and forth from y/m/d to
                  scalar.  A scalar is an absolute count of the number
                  of days from some starting point.  These functions
                  are adapted from the C Snippets (Ray Gardner).
                  
                  "Numerically, these will work over the range 1/01/01
                  thru 14699/12/31. Practically, these only work from 
                  the beginning of the Gregorian calendar thru 
                  14699/12/31.  The Gregorian calendar took effect in
                  much of Europe in about 1582, some parts of Germany 
                  in about 1700, in England and the colonies in about
                  1752, and in Russia in 1918.

*********************************************************************/

static s32 ScalarIsLeapYear (u32 yr)
{
   return yr % 400 == 0 || (yr % 4 == 0 && yr % 100 != 0);
}

static u32 ScalarMonthsToDays (u32 month)
{
   return (month * 3057 - 3007) / 100;
}

static s32 ScalarYearsToDays (u32 yr)
{
   return yr * 365L + yr / 4 - yr / 100 + yr / 400;
}

static s32 ScalarYMDToScalar (u32 yr, u32 mo, u32 day)
{
   s32 scalar;
   scalar = day + ScalarMonthsToDays(mo);
   if ( mo > 2 )                         /* adjust if past February */
      scalar -= ScalarIsLeapYear(yr) ? 1 : 2;
   yr--;
   scalar += ScalarYearsToDays(yr);
   return scalar;
}

static void ScalarScalarToYMD (s32 scalar, u32 *yr, u32 *mo, u32 *day)
{
   u32 n;                /* compute inverse of ScalarYearsToDays() */

   for ( n = (u32)((scalar * 400L) / 146097L); ScalarYearsToDays(n) < scalar;)
      n++;                          /* 146097 == ScalarYearsToDays(400) */
   *yr = n;
   n = (u32)(scalar - ScalarYearsToDays(n-1));
   if ( n > 59 ) {                       /* adjust if past February */
      n += 2;
      if ( ScalarIsLeapYear(*yr) )
         n -= n > 62 ? 1 : 2;
   }
   *mo = (n * 100 + 3007) / 3057;    /* inverse of ScalarMonthsToDays() */
   *day = n - ScalarMonthsToDays(*mo);
}


#if ADI_RTC_ISO_CALENDAR                /* monday == 0 */
 #define ADI_RTC_CALENDAR_ADJUSTMENT 5
#else                                   /* sunday == 0 */
 #define ADI_RTC_CALENDAR_ADJUSTMENT 6
#endif


static u32 ScalarDayOfWeek(u32 yr, u32 mo, u32 day)
{
 
#if (!ADI_RTC_ISO_CALENDAR)    /* Sunday(0) -> Saturday(6) (i.e. U.S.) calendars  */
      return (u32)(ScalarYMDToScalar(yr, mo, day) % 7L);
#else             /* International Monday(0) -> Sunday(6) calendars  */
      return (u32)((ScalarYMDToScalar(yr, mo, day) - 1L) % 7L);
#endif
}


	

/*********************************************************************

     Function:    adi_rtc_ValidateEventID

     Description: Called from within the RTC module, returns 
                  generic success or generic failure. The  
                  calling function decides how to break the news  
                  to the application, depending on the context.


*********************************************************************/

static ADI_RTC_RESULT adi_rtc_ValidateEventID( ADI_RTC_EVENT_ID EventID )
{
    /* Result code to return to the caller */	   
    ADI_RTC_RESULT Result;
	
    if(( EventID > ADI_RTC_EVENT_START ) && ( EventID <= (ADI_RTC_EVENT_START + ADI_RTC_MAX_EVENTS )))
    {
        Result = ADI_RTC_RESULT_SUCCESS;
    }
    else
    {
        Result = ADI_RTC_RESULT_FAILED;
    }
    return( Result );
}



/*********************************************************************

  Function:    adi_rtc_ValidateDateTime

  Description: Called from within the RTC module, returns a
             specific error message or generic success. The calling
             function returns a generic failure if any errors are
             returned,and the user may set a breakpoint to determine 
             the specific error.


*********************************************************************/

static u32 adi_rtc_ValidateDateTime( struct tm* pTmDateTime )
{
    u32  Result;
    Result = 0;

    struct tm tm_time;
    tm_time = *pTmDateTime;

    u32 MaxDays;
    
    /* get the correct value for MaxDays */
    
    /* 30 days - april june sept november */        
    if (( tm_time.tm_mon == 3 ) || ( tm_time.tm_mon == 5) 
       || ( tm_time.tm_mon == 8 ) || ( tm_time.tm_mon == 10 ))
    {
    	MaxDays = 30;
    }
       
     /* 28 or 29 days (we don't check leap year) february */
    else if ( tm_time.tm_mon == 1 )    
    {  
        MaxDays =  29;   
    }        
       
    /* all the rest have 31 */
    else
    {
    	  MaxDays =   ADI_RTC_MAX_MDAY;
    }      
    
    /* validate the data in the argument */    
    if (( tm_time.tm_sec <  ADI_RTC_MIN_SECONDS ) || ( tm_time.tm_sec >  ADI_RTC_MAX_SECONDS))
       Result |= ADI_RTC_RESULT_SECONDS_OUT_OF_RANGE;

    if (( tm_time.tm_min <  ADI_RTC_MIN_MINUTES ) || ( tm_time.tm_min >  ADI_RTC_MAX_MINUTES))
        Result |= ADI_RTC_RESULT_MINUTES_OUT_OF_RANGE;

    if (( tm_time.tm_hour <  ADI_RTC_MIN_HOURS ) || ( tm_time.tm_hour >  ADI_RTC_MAX_HOURS))
        Result |= ADI_RTC_RESULT_HOUR_OUT_OF_RANGE;

    if (( tm_time.tm_mday <  ADI_RTC_MIN_MDAY ) || ( tm_time.tm_mday > MaxDays))
        Result |=  ADI_RTC_RESULT_MDAY_OUT_OF_RANGE;

    if (( tm_time.tm_mon <  ADI_RTC_MIN_MONTH ) || ( tm_time.tm_mon >  ADI_RTC_MAX_MONTH))
        Result |=  ADI_RTC_RESULT_MONTH_OUT_OF_RANGE;

    if (( tm_time.tm_year <  ADI_RTC_MIN_YEAR ) || ( tm_time.tm_year >  ADI_RTC_MAX_YEAR))
        Result |=  ADI_RTC_RESULT_YEAR_OUT_OF_RANGE;
        
        
    return( Result );
}


#endif  /* non-Teton devices */
