/*********************************************************************************

Copyright(c) 2006 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_usb_logevent.c,v $
$Revision: 2095 $
$Date: 2010-02-18 17:08:05 -0500 (Thu, 18 Feb 2010) $

Description:
             Logs USB events if logging is enabled.

*********************************************************************************/

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_LOG_SECTION_CODE    section("adi_usb_log_code")
#define __ADI_USB_LOG_SECTION_DATA    section("adi_usb_log_data")
#else
#define __ADI_USB_LOG_SECTION_CODE
#define __ADI_USB_LOG_SECTION_DATA
#endif 

#include <drivers\usb\usb_core\adi_usb_logevent.h>
#include <drivers\usb\usb_core\adi_usb_debug.h>
#include <stdio.h>

#if defined(_USB_LOGEVENT_ENABLE_)
__ADI_USB_LOG_SECTION_DATA
USB_LOGEVENT_STRUCT   g_UsbLogEvents[NUM_MAX_EVENTS];

__ADI_USB_LOG_SECTION_DATA
static int log_index;

/**
 * adi_uusb_LogEvent  API
 *
 * Logs the specified event along with a value and a string. The buffer thats used to stored
 * the logged event is configurable. Once the buffer is full the index is set to start and the
 * buffers are reused.
 *
 * @param dwEvent specifies the event to be logged
 * @param dwValue specifies a value associated with the event
 * @paran eventString specifies a string associated with the event.
 */
__ADI_USB_LOG_SECTION_CODE
void adi_usb_LogEvent(int dwEvent, int dwValue, char *eventString)
{
    USB_ASSERT(strlen(eventString) > MAX_EVENT_STRING);
    g_UsbLogEvents[log_index].dwEvent  = dwEvent;
    g_UsbLogEvents[log_index].dwValue  = dwValue;
    strcpy(g_UsbLogEvents[log_index].pEventString,eventString);
    log_index++;

    if(log_index >= NUM_MAX_EVENTS)   log_index = 0;

    return;
}

/**
 * adi_uusb_ShowEventLog API
 * This API shows the currently avaialble USB events and their associated values and event strings.
 */
__ADI_USB_LOG_SECTION_CODE
void adi_usb_ShowEventLog(void)
{
int i=0;
  if(log_index <= 0) { printf("*** Log is empty ***\n"); return; }

  printf("\t-------------------------------------------\n");
  printf("\t\t *** USB Event Log *** \n");
  printf("\t-------------------------------------------\n");
  printf("\t EVENT \t -- VALUE \t --  EVENT STRING \n");
  printf("\t-------------------------------------------\n");
  for(i=0;i<log_index;i++)
  	printf("\t 0x%x \t -- 0x%x \t -- %s \n",g_UsbLogEvents[i].dwEvent,g_UsbLogEvents[i].dwValue,g_UsbLogEvents[i].pEventString);
  printf("\t-------------------------------------------\n");
}

#endif /* __USB_LOGEVENT__ */
