/*********************************************************************************

Copyright(c) 2008 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

Description:

    This is the driver source code for the USB Audio
    Class Driver (Device Mode).

*********************************************************************************/

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_AUDIO_SECTION_CODE    section("adi_usb_audio_code")
#define __ADI_USB_AUDIO_SECTION_DATA    section("adi_usb_audio_data")
#else
#define __ADI_USB_AUDIO_SECTION_CODE
#define __ADI_USB_AUDIO_SECTION_DATA
#endif

/******* ENABLE THE DESCRETE FREQUENCY CODE *******/
#define DISCRETE_FREQ							0
/* Test for 44.1khz */
#define TEST_44100								0

#if DISCRETE_FREQ
#if TEST_44100
#define	EXTRA_BYTES								6
#else
#define	EXTRA_BYTES								3
#endif
#else
#define	EXTRA_BYTES								0
#endif

/**************************************************/

#include <services/services.h>
#include <drivers/adi_dev.h>

#include <drivers/usb/usb_core/adi_usb_objects.h>
#include <drivers/usb/usb_core/adi_usb_core.h>
#include <drivers/usb/usb_core/adi_usb_ids.h>
#if defined(__ADSP_MOAB__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)
#include <drivers/usb/controller/otg/adi/hdrc/adi_usb_hdrc.h>
#elif defined(__ADSP_BRODIE__) || defined(__ADSP_EDINBURGH__) || defined(__ADSP_BRAEMAR__) || defined(__ADSP_TETON__)
#include <drivers/usb/controller/peripheral/plx/net2272/adi_usb_net2272.h>
#else
#error "USB Audio Class not supported for this processor"
#endif
#include <drivers/usb/usb_core/adi_usb_debug.h>

#include <stdint.h>
#include <adi_types.h>

#include <drivers/usb/class/peripheral/audio/adi_usb_audio_class.h>        /* USB Audio class include */

#include <string.h>
#include <stdio.h>

#include <cycle_count.h>	//For Basic Cycle Count

/*********************************************************************/

/* USB Audio command block handlers */
static bool AudioGetRequest (u8 bRequest, u8 wIndexHi, u16 wValue, u16 wLength);

#if DISCRETE_FREQ
static bool AudioSetRequest (u8 bRequest, u16 wIndex, u16 wValue, u16 wLength, u32 data);
#else
static bool AudioSetRequest (u8 bRequest, u8 wIndexHi, u16 wValue, u16 wLength, u16 controlData);
#endif

/* Endpoint Zero callback function */
void EndpointZeroCallback( void *Handle, u32  Event, void *pArg);

/* Return data from Audio control requests to host */
static void ReturnAudioControlData (u16 bytes);

/* Get data from Audio control requests from host */
static void GetAudioControlData (u16 bytes);

/* Audio control return buffer */
__ADI_USB_AUDIO_SECTION_DATA
static ADI_DEV_1D_BUFFER   AudioControlBuffer;

//#pragma align 32
static u8  AudioControlData[6 * sizeof(u8)];


/*********************************************************************/

#include <cplb.h>
extern  int     __cplb_ctrl;

/*********************************************************************
    Globals
*********************************************************************/
u32 gnMissed=0;

ADI_DEV_DEVICE_HANDLE   adi_usb_audio_PeripheralDevHandle;

ADI_DEV_MANAGER_HANDLE adi_usb_audio_dev_handle;
ADI_DMA_MANAGER_HANDLE adi_usb_audio_dma_handle;
ADI_DCB_HANDLE         adi_usb_audio_dcb_handle;

/* Initialize the device data structure */
AUDIO_STREAMING_DEV_DATA adi_usb_audio_def = {0};

void AudioEndpointCompleteCallback(void *Handle, u32  Event, void *pArg);

/* Feedback control buffer */
static ADI_DEV_1D_BUFFER   FeedbackControlBuffer;
static u8  FeedbackControlData[3];

void FeedbackEndpointCompleteCallback(void *Handle, u32  Event, void *pArg);
static void ProvideFeedback(u32 feedbackValue);
static s16 DbToHexValue( s16 decibels );
static s16 HexToDbValue( s16 HexValue );

/*********************************************************************
*
*   Function:       DevAudioConfigure
*
*   Description:    Configures the Audio device
*
*********************************************************************/
__ADI_USB_AUDIO_SECTION_CODE
static u32 DevAudioConfigure(void)
{
   unsigned int Result;
   int i;
   u32 playbackFeatureUnitLength, lineInMicMixer, feedbackEPLength;
   u32_t effectiveInterfaceID;

   OBJECT_INFO *pOConfigInf;
   OBJECT_INFO *pOAudioControlInf;
   OBJECT_INFO *pOStrmIdlePbkInf;
   OBJECT_INFO *pOStrmIdleRecInf;
   OBJECT_INFO *pOStrmEpPbkInf;
   OBJECT_INFO *pOStrmEpRecInf;
   OBJECT_INFO *pOAltInf;
   OBJECT_INFO *pOFeedBackStrmEpPbkInf;

   PINPUT_TERMINAL_DESCRIPTOR          			pOutInputTD;
   POUTPUT_TERMINAL_DESCRIPTOR         			pOutOutputTD;
   PINPUT_TERMINAL_DESCRIPTOR          			pInInputTD;
   POUTPUT_TERMINAL_DESCRIPTOR         			pInOutputTD;
   PFEATURE_UNIT_DESCRIPTOR            			pFeatrureUD;
   PENDPOINT_DESCRIPTOR                			pEndpointD;
   PCS_SPECIFIC_AS_INTERFACE_DESCRIPTOR         pFormatTypeD;
   PTYPE_1_FORMAT_TYPE_DESCRIPTOR               pType1FormatD;
   PCONFIGURATION_DESCRIPTOR           			pConfigD;
   PINTERFACE_DESCRIPTOR               			pInterfaceD;
   PINTERFACE_OBJECT                   			pInterfaceO;
   PCS_SPECIFIC_AC_DESCRIPTOR          			pCSpecACD;
   PUSB_EP_INFO                        			pEpInfo;
   LOGICAL_EP_INFO                     			LogEpInfo;
   PAS_ISOCHRONOUS_EP_DESCRIPTOR       			pASEndpointD;
   PCS_SPECIFIC_AS_ISOCHRONOUS_EP_DESCRIPTOR    pAudioEpD;
   PAC_EP_DESCRIPTOR_EXT           				pAudioEpDExt;
   PCS_SPECIFIC_AS_INTERFACE_DESCRIPTOR         pFormatTypeD_IN;
   PTYPE_1_FORMAT_TYPE_DESCRIPTOR               pType1FormatD_IN;
   PSELECTOR_UNIT_DESCRIPTOR                    pSelectorUnitD;
   PMIXER_UNIT_DESCRIPTOR                       pMixerUnitD;

   /* Get a pointer to our audio device structure */
   AUDIO_STREAMING_DEV_DATA *pDevice = &adi_usb_audio_def;

   /* Assume success */
   Result = ADI_DEV_RESULT_SUCCESS;

   /* Set Peripheral driver handle */
   adi_usb_SetPhysicalDriverHandle(pDevice->PeripheralDevHandle);

   /* Set USB to Device mode */
   adi_usb_SetDeviceMode(MODE_DEVICE);

   /* Get the current Device ID. Once the driver is opened its
   *  suppose to create the device. If the device is already opened then
   *  it must return the same device ID.
   */
   adi_dev_Control(pDevice->PeripheralDevHandle,
                    ADI_USB_CMD_GET_DEVICE_ID,
                    (void*)&pDevice->DeviceID);

  /* Determin the size of the playback feature unit */
  /* This is determined by the number of channels   */
  if(pDevice->playbackChannels == 2)
  {
    playbackFeatureUnitLength = TWO_CHANNEL_PBK_FU_SIZE;
  }
  else /* It's six channels */
  {
    playbackFeatureUnitLength = SIX_CHANNEL_PBK_FU_SIZE;
  }

  /* If we have a MIXER Unit */
  if(pDevice->lineInMicMixer == true)
  {
   /* if MIC and Line In in the mixer */
   if(pDevice->micInAdcMixer == true)
   {
    	lineInMicMixer = LINE_IN_MIC_MIXER_LEN;
   }
   else /* ONLY Line In in the Mixer */
   {
   		/* If the Selector Unit only has 1 pin */
	  	if((pDevice->enableMicRecordCtl == false) ||
  			(pDevice->enableLineInRecordCtl == false))
		{

	   		lineInMicMixer = LINE_IN_MIC_MIXER_LEN - 2;
		}
		else
		{
	   		lineInMicMixer = LINE_IN_MIC_MIXER_LEN - 1;
		}
   }
  }
  else
  {
   lineInMicMixer = 0;
  }

  /* If feedback is enabled then need to get value to add to total configuration length */
  if(pDevice->enableFeedback == true)
  {
  	feedbackEPLength = AUDIO_STREAMING_ENDPOINT_DESCRIPTOR_LEN;
  }
  else
  {
  	feedbackEPLength = 0;
  }

  /*****************************************************
  *
  * Create Audio Configuration Descriptor Object
  *
  */
  pOConfigInf = &pDevice->ConfigObject;
  pOConfigInf->ID = adi_usb_CreateConfiguration((PCONFIG_OBJECT*)&pOConfigInf->pObj);

  /* Setup the callback for Endpoint 0 */
  ((PCONFIG_OBJECT)pOConfigInf->pObj)->EpZeroCallback = (ADI_DCB_CALLBACK_FN)EndpointZeroCallback;

  /* Attach configuration to the device object */
  adi_usb_AttachConfiguration(pDevice->DeviceID,pOConfigInf->ID);

  /* Fill  configuration descriptor */
  pConfigD = ((PCONFIG_OBJECT)pOConfigInf->pObj)->pConfigDesc;

  pConfigD->bLength             = CONFIGURATION_DESCRIPTOR_LEN;     /* Configuration Descriptor Length */
  pConfigD->bDescriptorType     = TYPE_CONFIGURATION_DESCRIPTOR;    /* Type */
  pConfigD->wTotalLength      	= (TOTAL_CONFIG_DESCRIPTOR_LEN
                                    + lineInMicMixer
                                    + playbackFeatureUnitLength
                                    + feedbackEPLength
									+ EXTRA_BYTES);
  pConfigD->bNumInterfaces      = AUDIO_NUM_TOTAL_INTERFACES;       /* Total number of Interfaces */
  pConfigD->bConfigurationValue = 0x01;                             /* Configuration Value */
  pConfigD->bIConfiguration     = 0x00;                             /* Configuration String */
  pConfigD->bAttributes         = 0x80;                             /* Bus Powered */
  pConfigD->bMaxPower           = 50; 	                            /* Bus powered device 100 mA*/


  /*******************************************************
  *
  * Standard Audio Control Interface descriptor
  * Create Audio Control Interface Object and configure it.
  *
  */
  pOAudioControlInf = &pDevice->InterfaceObjects[AUDIO_CONTROL];
  pOAudioControlInf->ID = adi_usb_CreateInterface((PINTERFACE_OBJECT*)&pOAudioControlInf->pObj);

  /* Attach Audio control Interface object to the Configuration object */
  adi_usb_AttachInterface(pDevice->ConfigObject.ID,pOAudioControlInf->ID);

  pInterfaceD = ((PINTERFACE_OBJECT)pOAudioControlInf->pObj)->pInterfaceDesc;

  pInterfaceD->bLength              = INTERFACE_DESCRIPTOR_LEN;
  pInterfaceD->bDescriptorType      = TYPE_INTERFACE_DESCRIPTOR;
  pInterfaceD->bInterfaceNumber     = pOAudioControlInf->ID;
  pInterfaceD->bAlternateSetting    = ALTSETTING_FOR_AUDIOCONTROL;
  pInterfaceD->bNumEndpoints        = 0;
  pInterfaceD->bInterfaceClass      = USB_AUDIO_CLASS_CODE;
  pInterfaceD->bInterfaceSubClass   = USB_AUDIO_SUBCLASS_AUDIO_CONTROL;
  pInterfaceD->bInterfaceProtocol   = USB_AUDIO_PROTOCOL_NONE;
  pInterfaceD->bIInterface          = 0;

  /* Add the Device Specific interfaces */
  ((PINTERFACE_OBJECT)pOAudioControlInf->pObj)->pIntSpecificObj = &pDevice->IFaceClassSpecificObj[0];

  /*******************************************************
  *
  * Class Specific Audio Control Header Descriptor
  * Create Audio Control Interface Object and configure it.
  *
  */
  pCSpecACD = &pDevice->acSpecifcInfo.csACD;

  pCSpecACD->bLength                = AUDIO_CONTROL_INTERFACE_DESCRIPTOR_LEN;
  pCSpecACD->bDescriptorType        = USB_AUDIO_DESCRIPTOR_INTERFACE;
  pCSpecACD->bDescriptorSubtype     = USB_AC_DESCRIPTOR_SUBTYPE_HEADER;
  pCSpecACD->bcdADC_lo  			= LOBYTE(USB_AUDIO_SPEC_1_0);
  pCSpecACD->bcdADC_hi      		= HIBYTE(USB_AUDIO_SPEC_1_0);
  pCSpecACD->TotalLength_lo     	= LOBYTE(TOTAL_AUDIO_CONTROL_DESCRIPTOR_LEN
  											 + playbackFeatureUnitLength
  											 + lineInMicMixer
  											 + feedbackEPLength
											 + EXTRA_BYTES);

  pCSpecACD->TotalLength_hi   		= HIBYTE(TOTAL_AUDIO_CONTROL_DESCRIPTOR_LEN
  											 + playbackFeatureUnitLength
  											 + lineInMicMixer
  											 + feedbackEPLength
											 + EXTRA_BYTES);

  pCSpecACD->bInCollection      	= MAX_CS_AC_NUM_INTERFACES;
  pCSpecACD->baInterfaceNr[0]       = INTERFACE_NUM_FOR_AUDIO_STREAMING;
  pCSpecACD->baInterfaceNr[1]       = INTERFACE_NUM_FOR_AUDIO_STREAMING_RECORD;

  pDevice->IFaceClassSpecificObj[0].length = pCSpecACD->bLength;
  pDevice->IFaceClassSpecificObj[0].pIntSpecificData = pCSpecACD;
  pDevice->IFaceClassSpecificObj[0].pNext = &pDevice->IFaceClassSpecificObj[1];

  /*******************************************************
  *
  *  Initialize Line Out - Input Terminal descriptor audio data arrives here
  *  via the USB OUT endpoint from the PC.
  *
  */
  pOutInputTD =  &pDevice->acSpecifcInfo.out_input_TD;

  pOutInputTD->bLength            = AUDIO_CONTROL_OUT_INPUT_TERMINAL_DESCRIPTOR_LEN;
  pOutInputTD->bDescriptorType    = USB_AUDIO_CS_INTERFACE;
  pOutInputTD->bDescriptorSubtype = USB_AC_DESCRIPTOR_SUBTYPE_INPUT_TERMINAL;
  pOutInputTD->bTerminalID        = AUDIO_TERMID_USBOUT;
  pOutInputTD->wTerminalType      = USB_AUDIO_TERMINAL_TYPE_STREAMING;
  pOutInputTD->bAssocTerminal     = 0;
  pOutInputTD->bNrChannels        = pDevice->playbackChannels;
  pOutInputTD->wChannelConfig     = USB_AUDIO_CHANNEL_CONFIG_BITMAP(pDevice->playbackChannels);
  pOutInputTD->iChannelNames      = 0;
  pOutInputTD->iTerminal          = 0;

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[1].length = pOutInputTD->bLength;
  pDevice->IFaceClassSpecificObj[1].pIntSpecificData = pOutInputTD;
  pDevice->IFaceClassSpecificObj[1].pNext = &pDevice->IFaceClassSpecificObj[2];

  /*******************************************************
  *
  *  Initialize Feature unit descriptor. Number of bytes per
  *  control is assumed to be 2. The logic supports any number
  *  of logical channels but if application changes the number
  *  of bytes to represent a channel then the code needs to be
  *  changed.
  *
  *  This is the Playback Feature Unit this is connected to the
  *  Line Out output terminal (AUDIO_TERMID_LINEOUT)
  */
  pFeatrureUD = &pDevice->acSpecifcInfo.featureUD;

  pFeatrureUD->bLength      			=  playbackFeatureUnitLength;
  pFeatrureUD->bDescriptorType          =  USB_AUDIO_CS_INTERFACE;
  pFeatrureUD->bDescriptorSubtype       =  USB_AC_DESCRIPTOR_SUBTYPE_FEATURE_UNIT;
  pFeatrureUD->bUnitID    				=  AUDIO_UNITID_PLAYBACK_VOLUME_OUT;

  if(pDevice->lineInMicMixer == true)
  {
  	pFeatrureUD->bSourceID  				=  AUDIO_UNITID_MIXER;
  }
  else
  {
  	pFeatrureUD->bSourceID  				=  AUDIO_TERMID_USBOUT;
  }

  pFeatrureUD->bControlSize     	    =  0x01;

  /* If we are a two channel device */
  if(pDevice->playbackChannels == 2)
  {
      pFeatrureUD->bmaControls[0]   	=  FEATURE_UNIT_MUTE_CONTROL |
      									   FEATURE_UNIT_VOLUME_CONTROL;
      pFeatrureUD->bmaControls[1]   	=  0x00;
      pFeatrureUD->bmaControls[2]   	=  0x00;
  }
  else /* Six channel device */
  {
      pFeatrureUD->bmaControls[0]   	=  FEATURE_UNIT_MUTE_CONTROL |
      									   FEATURE_UNIT_VOLUME_CONTROL;
      pFeatrureUD->bmaControls[1]   	=  0x00;
      pFeatrureUD->bmaControls[2]   	=  0x00;
      pFeatrureUD->bmaControls[3]   	=  0x00;
      pFeatrureUD->bmaControls[4]   	=  0x00;
      pFeatrureUD->bmaControls[5]   	=  0x00;
      pFeatrureUD->bmaControls[6]   	=  0x00;
  }

  pFeatrureUD->iFeature       		=  0x00;  /* No string descriptor */

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[2].length =   pFeatrureUD->bLength;
  pDevice->IFaceClassSpecificObj[2].pIntSpecificData = pFeatrureUD;
  pDevice->IFaceClassSpecificObj[2].pNext = &pDevice->IFaceClassSpecificObj[3];

  /*******************************************************
  *
  *  Initialize Line Out - Output Terminal descriptor
  *  Audio data exits through this terminal to the codec
  *
  */
  pOutOutputTD =  &pDevice->acSpecifcInfo.out_output_TD;

  pOutOutputTD->bLength 				= AUDIO_CONTROL_OUT_OUTPUT_TERMINAL_DESCRIPTOR_LEN;
  pOutOutputTD->bDescriptorType 		= USB_AUDIO_DESCRIPTOR_INTERFACE;
  pOutOutputTD->bDescriptorSubtype 	    = USB_AC_DESCRIPTOR_SUBTYPE_OUTPUT_TERMINAL;
  pOutOutputTD->bTerminalID 			= AUDIO_TERMID_LINEOUT;
  pOutOutputTD->wTerminalType 			= USB_AUDIO_OUTPUT_TERMINALTYPE_SPEAKER;
  pOutOutputTD->bAssocTerminal 			= 0;
  pOutOutputTD->bSourceID 				= AUDIO_UNITID_PLAYBACK_VOLUME_OUT;
  pOutOutputTD->iTerminal 				= 0;

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[3].length = pOutOutputTD->bLength;
  pDevice->IFaceClassSpecificObj[3].pIntSpecificData = pOutOutputTD;
  pDevice->IFaceClassSpecificObj[3].pNext = &pDevice->IFaceClassSpecificObj[4];

  /*******************************************************
  *
  *  Initialize Microphone - Input Terminal descriptor
  *
  */
  pInInputTD =  &pDevice->acSpecifcInfo.in_input_TD[0];

  pInInputTD->bLength           	= AUDIO_CONTROL_IN_INPUT_TERMINAL_DESCRIPTOR_LEN;
  pInInputTD->bDescriptorType   	= USB_AUDIO_CS_INTERFACE;
  pInInputTD->bDescriptorSubtype    = USB_AC_DESCRIPTOR_SUBTYPE_INPUT_TERMINAL;
  pInInputTD->bTerminalID       	= AUDIO_TERMID_MICIN;
  pInInputTD->wTerminalType     	= USB_AUDIO_INPUT_TERMINALTYPE_MICROPHONE;
  pInInputTD->bAssocTerminal    	= 0;
  pInInputTD->bNrChannels       	= NUM_CHANNELS_RECORDING;
  pInInputTD->wChannelConfig    	= USB_AUDIO_CHANNEL_CONFIG_BITMAP(NUM_CHANNELS_RECORDING);
  pInInputTD->iChannelNames     	= 0;
  pInInputTD->iTerminal         	= 0;

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[4].length = pInInputTD->bLength;
  pDevice->IFaceClassSpecificObj[4].pIntSpecificData = pInInputTD;

  /*******************************************************
  *
  *  Initialize Line In - Input Terminal descriptor
  *  This is data from the Line In connector on the board
  *  this is routed to the codec Line In.
  *
  */
  pDevice->IFaceClassSpecificObj[4].pNext = &pDevice->IFaceClassSpecificObj[5];

  pInInputTD =  &pDevice->acSpecifcInfo.in_input_TD[1];

  pInInputTD->bLength           	= AUDIO_CONTROL_LINE_IN_INPUT_TERMINAL_DESCRIPTOR_LEN;
  pInInputTD->bDescriptorType   	= USB_AUDIO_CS_INTERFACE;
  pInInputTD->bDescriptorSubtype    = USB_AC_DESCRIPTOR_SUBTYPE_INPUT_TERMINAL;
  pInInputTD->bTerminalID       	= AUDIO_TERMID_LINEIN;
  pInInputTD->wTerminalType     	= USB_EXTERNAL_TERMINALTYPE_LINE_CONNECTOR;
  pInInputTD->bAssocTerminal    	= 0;
  pInInputTD->bNrChannels       	= NUM_CHANNELS_RECORDING;
  pInInputTD->wChannelConfig    	= USB_AUDIO_CHANNEL_CONFIG_BITMAP(NUM_CHANNELS_RECORDING);
  pInInputTD->iChannelNames     	= 0;
  pInInputTD->iTerminal         	= 0;

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[5].length = pInInputTD->bLength;
  pDevice->IFaceClassSpecificObj[5].pIntSpecificData = pInInputTD;
  pDevice->IFaceClassSpecificObj[5].pNext = &pDevice->IFaceClassSpecificObj[6];

  /*******************************************************
  *
  *  Initialize Line In - Feature Unit descriptor it is
  *  connected to the record Selector Unit (AUDIO_UNITID_SELECTOR_RECORD)
  *
  */
  pFeatrureUD = &pDevice->acSpecifcInfo.in_featureUD[2];

  pFeatrureUD->bLength      			=  AUDIO_CONTROL_FEATURE_UNIT_DESCRIPTOR_LEN_RECORD_2;
  pFeatrureUD->bDescriptorType          =  USB_AUDIO_CS_INTERFACE;
  pFeatrureUD->bDescriptorSubtype       =  USB_AC_DESCRIPTOR_SUBTYPE_FEATURE_UNIT;
  pFeatrureUD->bUnitID    				=  AUDIO_UNITID_LINEIN_VOLUME_RECORD;
  pFeatrureUD->bSourceID  				=  AUDIO_TERMID_LINEIN;
  pFeatrureUD->bControlSize     	    =  0x01;

  pFeatrureUD->bmaControls[0]   	    =  FEATURE_UNIT_MUTE_CONTROL |
  										   FEATURE_UNIT_VOLUME_CONTROL;
  pFeatrureUD->bmaControls[1]   	    =  0x00;
  pFeatrureUD->bmaControls[2]   	    =  0x00;

  pFeatrureUD->iFeature       		    =  0x00;  /* No string descriptor */

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[6].length = pFeatrureUD->bLength;
  pDevice->IFaceClassSpecificObj[6].pIntSpecificData = pFeatrureUD;
  pDevice->IFaceClassSpecificObj[6].pNext = &pDevice->IFaceClassSpecificObj[7];

  /*******************************************************
  *
  *  Initialize Line In - Output Terminal descriptor this
  *  is routed to the USB IN endpoint on the PC
  *
  */
  pInOutputTD =  &pDevice->acSpecifcInfo.in_output_TD;

  pInOutputTD->bLength				= AUDIO_CONTROL_IN_OUTPUT_TERMINAL_DESCRIPTOR_LEN;
  pInOutputTD->bDescriptorType 		= USB_AUDIO_DESCRIPTOR_INTERFACE;
  pInOutputTD->bDescriptorSubtype 	= USB_AC_DESCRIPTOR_SUBTYPE_OUTPUT_TERMINAL;
  pInOutputTD->bTerminalID 			= AUDIO_TERMID_USBIN;
  pInOutputTD->wTerminalType 		= USB_AUDIO_TERMINAL_TYPE_STREAMING;
  pInOutputTD->bAssocTerminal 		= 0;
  pInOutputTD->bSourceID            = AUDIO_UNITID_SELECTOR_RECORD;
  pInOutputTD->iTerminal 			= 0;

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[7].length = pInOutputTD->bLength;
  pDevice->IFaceClassSpecificObj[7].pIntSpecificData = pInOutputTD;
  pDevice->IFaceClassSpecificObj[7].pNext = &pDevice->IFaceClassSpecificObj[8];

  /*******************************************************
  *
  *  Initialize record control selector unit.  This unit takes input
  *  from two feature units the Line IN and Microphone feature units
  *  The output of this selector unit is connected to the Line IN
  *  output terminal (AUDIO_TERMID_USBIN)
  *
  */
    pSelectorUnitD = &pDevice->acSpecifcInfo.in_selectorUD;

  	if((pDevice->enableMicRecordCtl == false) ||
  		(pDevice->enableLineInRecordCtl == false))
	{
    	pSelectorUnitD->bLength             = AUDIO_SELECTOR_UNIT_DESCRIPTOR_LEN - 1;
	}
	else
	{
    	pSelectorUnitD->bLength             = AUDIO_SELECTOR_UNIT_DESCRIPTOR_LEN;
	}

    pSelectorUnitD->bDescriptorType     = USB_AUDIO_CS_INTERFACE;
    pSelectorUnitD->bDescriptorSubtype  = USB_AC_DESCRIPTOR_SUBTYPE_SELECTOR_UNIT;
    pSelectorUnitD->bUnitID             = AUDIO_UNITID_SELECTOR_RECORD;

    if((pDevice->enableMicRecordCtl == true) &&
    	(pDevice->enableLineInRecordCtl == true))
	{
    	pSelectorUnitD->bNrInPins           	= 0x02;

		if(pDevice->micDefaultRecord == true)
		{
    		pSelectorUnitD->baSourceID[0]       = AUDIO_UNITID_MIC_VOLUME_RECORD;
    		pSelectorUnitD->baSourceID[1]       = AUDIO_UNITID_LINEIN_VOLUME_RECORD;
		}
		else
		{
    		pSelectorUnitD->baSourceID[0]       = AUDIO_UNITID_LINEIN_VOLUME_RECORD;
    		pSelectorUnitD->baSourceID[1]       = AUDIO_UNITID_MIC_VOLUME_RECORD;
		}
	}
	else
	{
   		pSelectorUnitD->bNrInPins           = 0x01;

    	if((pDevice->enableMicRecordCtl == false) &&
    		(pDevice->enableLineInRecordCtl == true))
		{
    		pSelectorUnitD->baSourceID[0]       = AUDIO_UNITID_LINEIN_VOLUME_RECORD;
		}
		else
    	if((pDevice->enableMicRecordCtl == true) &&
    	(pDevice->enableLineInRecordCtl == false))
		{
    		pSelectorUnitD->baSourceID[0]       = AUDIO_UNITID_MIC_VOLUME_RECORD;
		}
	}

    pSelectorUnitD->iSelector           = 0x00;

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[8].length = pSelectorUnitD->bLength;
  pDevice->IFaceClassSpecificObj[8].pIntSpecificData = pSelectorUnitD;
  pDevice->IFaceClassSpecificObj[8].pNext = &pDevice->IFaceClassSpecificObj[9];

  /*******************************************************
  *
  *  Initialize Feature unit descriptor. Number of bytes per
  *  control is assumed to be 2. The logic supports any number
  *  of logical channels but if application changes the number
  *  of bytes to represent a channel then the code needs to be
  *  changed.
  *
  *  This is the micropnone Feature Unit it is connected to the record
  *  Selector Unit (AUDIO_UNITID_SELECTOR_RECORD)
  *
  */
  pFeatrureUD = &pDevice->acSpecifcInfo.in_featureUD[0];

  pFeatrureUD->bLength      			=  AUDIO_CONTROL_FEATURE_UNIT_DESCRIPTOR_LEN_RECORD;
  pFeatrureUD->bDescriptorType          =  USB_AUDIO_CS_INTERFACE;
  pFeatrureUD->bDescriptorSubtype       =  USB_AC_DESCRIPTOR_SUBTYPE_FEATURE_UNIT;
  pFeatrureUD->bUnitID    				=  AUDIO_UNITID_MIC_VOLUME_RECORD;
  pFeatrureUD->bSourceID  				=  AUDIO_TERMID_MICIN;
  pFeatrureUD->bControlSize     	    =  0x01;
  pFeatrureUD->bmaControls[0]   	    =  FEATURE_UNIT_MUTE_CONTROL |
  										   FEATURE_UNIT_VOLUME_CONTROL;
  pFeatrureUD->bmaControls[1]   	    =  0x00;
  pFeatrureUD->bmaControls[2]   	    =  0x00;
  pFeatrureUD->iFeature       		    =  0x00;  /* No string descriptor */

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[9].length = pFeatrureUD->bLength;
  pDevice->IFaceClassSpecificObj[9].pIntSpecificData = pFeatrureUD;
  pDevice->IFaceClassSpecificObj[9].pNext = NULL;

  if(pDevice->lineInMicMixer == true)
  {
  /*******************************************************
  *
  *  Initialize Microphone Feature unit for control
  *  that connects to the mixer unit
  *
  */
  pDevice->IFaceClassSpecificObj[9].pNext = &pDevice->IFaceClassSpecificObj[10];

  pFeatrureUD = &pDevice->acSpecifcInfo.in_featureUD[1];

  pFeatrureUD->bLength      			=  AUDIO_CONTROL_FEATURE_UNIT_DESCRIPTOR_LEN_RECORD_1;
  pFeatrureUD->bDescriptorType          =  USB_AUDIO_CS_INTERFACE;
  pFeatrureUD->bDescriptorSubtype       =  USB_AC_DESCRIPTOR_SUBTYPE_FEATURE_UNIT;
  pFeatrureUD->bUnitID    				=  AUDIO_UNITID_MIC_VOLUME_RECORD_1;
  pFeatrureUD->bSourceID  				=  AUDIO_TERMID_MICIN;
  pFeatrureUD->bControlSize     	    =  0x01;

  pFeatrureUD->bmaControls[0]   	    =  FEATURE_UNIT_MUTE_CONTROL |
  										   FEATURE_UNIT_VOLUME_CONTROL;
  pFeatrureUD->bmaControls[1]   	    =  0x00;
  pFeatrureUD->iFeature       		    =  0x00;  /* No string descriptor */

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[10].length = pFeatrureUD->bLength;
  pDevice->IFaceClassSpecificObj[10].pIntSpecificData = pFeatrureUD;
  pDevice->IFaceClassSpecificObj[10].pNext = &pDevice->IFaceClassSpecificObj[11];

  /*******************************************************
  *
  *  Initialize Line-In Feature unit for control
  *  that connects to the mixer unit
  *
  */
  pFeatrureUD = &pDevice->acSpecifcInfo.in_featureUD[3];

  pFeatrureUD->bLength      			=  AUDIO_CONTROL_FEATURE_UNIT_DESCRIPTOR_LEN_RECORD_3;
  pFeatrureUD->bDescriptorType          =  USB_AUDIO_CS_INTERFACE;
  pFeatrureUD->bDescriptorSubtype       =  USB_AC_DESCRIPTOR_SUBTYPE_FEATURE_UNIT;
  pFeatrureUD->bUnitID    				=  AUDIO_UNITID_LINEIN_VOLUME_RECORD_1;
  pFeatrureUD->bSourceID  				=  AUDIO_TERMID_LINEIN;
  pFeatrureUD->bControlSize     	    =  0x01;

  pFeatrureUD->bmaControls[0]   	    =  FEATURE_UNIT_MUTE_CONTROL |
  										   FEATURE_UNIT_VOLUME_CONTROL;
  pFeatrureUD->bmaControls[1]   	    =  0x00;
  pFeatrureUD->iFeature       		    =  0x00;  /* No string descriptor */

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[11].length = pFeatrureUD->bLength;
  pDevice->IFaceClassSpecificObj[11].pIntSpecificData = pFeatrureUD;
  pDevice->IFaceClassSpecificObj[11].pNext = &pDevice->IFaceClassSpecificObj[12];

  /*******************************************************
  *
  *  Initialize Mixer Unit
  *
  */
  pMixerUnitD = &pDevice->acSpecifcInfo.in_mixerUD;

  if(pDevice->micInAdcMixer == true)
  {
	  pMixerUnitD->bLength              = AUDIO_CONTROL_MIXER_UNIT_DESCRIPTOR_LEN_RECORD;
  }
  else
  {
	  pMixerUnitD->bLength              = AUDIO_CONTROL_MIXER_UNIT_DESCRIPTOR_LEN_RECORD - 1;
  }
  pMixerUnitD->bDescriptorType          = USB_AUDIO_CS_INTERFACE;
  pMixerUnitD->bDescriptorSubtype       = USB_AC_DESCRIPTOR_SUBTYPE_MIXER_UNIT;
  pMixerUnitD->bUnitID                  = AUDIO_UNITID_MIXER;

#if defined(__ADSP_MOAB__)
  if(pDevice->micInAdcMixer == true)
  {
  	pMixerUnitD->bNrInPins                  = 0x03;
  	pMixerUnitD->baSourceID[0]              = AUDIO_TERMID_USBOUT;
  	pMixerUnitD->baSourceID[1]              = AUDIO_UNITID_LINEIN_VOLUME_RECORD_1;
  	pMixerUnitD->baSourceID[2]              = AUDIO_UNITID_MIC_VOLUME_RECORD_1;
  }
#else
  	pMixerUnitD->bNrInPins                  = 0x02;
  	pMixerUnitD->baSourceID[0]	            = AUDIO_TERMID_USBOUT;
  	pMixerUnitD->baSourceID[1]              = AUDIO_UNITID_LINEIN_VOLUME_RECORD_1;
#endif

  /* If we are a two channel device */
  if(pDevice->playbackChannels == 2)
  {
      pMixerUnitD->bNrChannels              = 0x02;
	  pMixerUnitD->bChannelConfig_lo		= LOBYTE(USB_AUDIO_CHANNEL_CONFIG_BITMAP(pDevice->playbackChannels));
  }
  else
  {
      pMixerUnitD->bNrChannels              = 0x06;
	  pMixerUnitD->bChannelConfig_lo		= LOBYTE(USB_AUDIO_CHANNEL_CONFIG_BITMAP(pDevice->playbackChannels));
  }

  pMixerUnitD->bChannelConfig_hi		= 0x00;
  pMixerUnitD->iChannelNames            = 0x00;
  pMixerUnitD->bmControls               = 0x00;
  pMixerUnitD->iMixer                   = 0x00;

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[12].length = pMixerUnitD->bLength;
  pDevice->IFaceClassSpecificObj[12].pIntSpecificData = pMixerUnitD;
  pDevice->IFaceClassSpecificObj[12].pNext = NULL;
  }

  /****************************************************************
  *
  * Audio Streaming Interface (USB OUT) configuration, Idle
  *
  */
  pOStrmIdlePbkInf = &pDevice->InterfaceObjects[AUDIO_STREAM_OUT];
  pOStrmIdlePbkInf->ID = adi_usb_CreateInterface((PINTERFACE_OBJECT*)&pOStrmIdlePbkInf->pObj);

  /* Attach Audio Stream OUT Interface object to the Configuration object */
  adi_usb_AttachInterface(pDevice->ConfigObject.ID,pOStrmIdlePbkInf->ID);

  pInterfaceD = ((PINTERFACE_OBJECT)pOStrmIdlePbkInf->pObj)->pInterfaceDesc;

  pInterfaceD->bLength            = INTERFACE_DESCRIPTOR_LEN;
  pInterfaceD->bDescriptorType    = TYPE_INTERFACE_DESCRIPTOR;
  pInterfaceD->bInterfaceNumber   = pOStrmIdlePbkInf->ID;
  pInterfaceD->bAlternateSetting  = ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING_IDLE;
  pInterfaceD->bNumEndpoints      = 0;
  pInterfaceD->bInterfaceClass    = USB_AUDIO_CLASS_CODE;
  pInterfaceD->bInterfaceSubClass = USB_AUDIO_SUBCLASS_AUDIO_STREAMING;
  pInterfaceD->bInterfaceProtocol = USB_AUDIO_PROTOCOL_NONE;
  pInterfaceD->bIInterface        = 0;

  /****************************************************************
  *
  * Audio Streaming Interface (USB OUT) configuration
  *
  */
  /* Create an alternate interface object */
  if (!adi_usb_CreateInterfaceObject(&((PINTERFACE_OBJECT)pOStrmIdlePbkInf->pObj)->pAltInterfaceObj))
  {
	  /* Get a pointer to the Alt Interface descriptor and the fill it */
  	pInterfaceD = ((PINTERFACE_OBJECT)pOStrmIdlePbkInf->pObj)->pAltInterfaceObj->pInterfaceDesc;

  	pInterfaceD->bLength            = INTERFACE_DESCRIPTOR_LEN;
  	pInterfaceD->bDescriptorType    = TYPE_INTERFACE_DESCRIPTOR;
    pInterfaceD->bInterfaceNumber   = pOStrmIdlePbkInf->ID;
  	pInterfaceD->bAlternateSetting  = ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING;

    if(pDevice->enableFeedback == true)
    {
  		pInterfaceD->bNumEndpoints      = 2;
    }
	else
	{
  		pInterfaceD->bNumEndpoints      = 1;
	}

  	pInterfaceD->bInterfaceClass    = USB_AUDIO_CLASS_CODE;
  	pInterfaceD->bInterfaceSubClass = USB_AUDIO_SUBCLASS_AUDIO_STREAMING;
  	pInterfaceD->bInterfaceProtocol = USB_AUDIO_PROTOCOL_NONE;
  	pInterfaceD->bIInterface        = 0;
  }

  /* Now attach the next descriptor */
  ((PINTERFACE_OBJECT)pOStrmIdlePbkInf->pObj)->pAltInterfaceObj->pIntSpecificObj = &pDevice->IFaceClassSpecificObj[13];

  /****************************************************************
  *
  * Create Audio Class specific General Descriptor
  *
  */
  pFormatTypeD = &pDevice->acSpecifcInfo.formatTypeD;

  pFormatTypeD->bLength       		= AUDIO_STREAMING_GENERAL_DESCRIPTOR_LEN;
  pFormatTypeD->bDescriptorType   	= USB_AUDIO_DESCRIPTOR_INTERFACE;
  pFormatTypeD->bDescriptorSubtype  = USB_AS_DESCRIPTOR_SUBTYPE_GENERAL;
  pFormatTypeD->bTerminalLink     	= AUDIO_TERMID_USBOUT;
  pFormatTypeD->bDelay        		= PLAYBACK_DELAY;
  pFormatTypeD->bFormatTag_lo     	= LOBYTE(USB_AUDIO_FORMAT_TAG_PCM);
  pFormatTypeD->bFormatTag_hi     	= HIBYTE(USB_AUDIO_FORMAT_TAG_PCM);

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[13].length = pFormatTypeD->bLength;
  pDevice->IFaceClassSpecificObj[13].pIntSpecificData = pFormatTypeD;
  pDevice->IFaceClassSpecificObj[13].pNext = &pDevice->IFaceClassSpecificObj[14];

  /****************************************************************
  *
  * Create Audio Class specific Type 1 Format Descriptor
  *
  */
  pType1FormatD = &pDevice->acSpecifcInfo.type1FormatD;
  if((pDevice->pbkSampleRate == SAMPLE_RATE_48K) ||
  	 (pDevice->pbkSampleRate == SAMPLE_RATE_96K))
  {
  	pType1FormatD->bLength        	= AUDIO_STREAMING_TYPE_I_FMT_DESCRIPTOR_LEN + EXTRA_BYTES;	/* Size of this descriptor */
  }

  pType1FormatD->bDescriptorType    = USB_AUDIO_DESCRIPTOR_INTERFACE;               /* Interface Descriptor Type */
  pType1FormatD->bDEscriptorSubType = USB_AS_DESCRIPTOR_SUBTYPE_FORMAT_TYPE;        /* Descriptor Sub Type */
  pType1FormatD->bFormatType      	= USB_AUDIO_FORMAT_TYPE_I;                      /* Format Type */
  pType1FormatD->bNrChannels      	= pDevice->playbackChannels;                    /* Number of Physical Channels */

  /* Set the desired bit resolution to 16 bits */
  if(pDevice->playback16BitResolution == true)
  {
  	pType1FormatD->bSubFrameSize    	= CONTAINER_SIZE / 8;                           /* Number of bytes in subframe */
  	pType1FormatD->bBitResolution   	= BIT_DEPTH;                                    /* Number of bits used */
  }
  /* Set the desired bit resolution to 32 bits */
  else
  {
  	pType1FormatD->bSubFrameSize    	= 0x04;			                           /* Number of bytes in subframe */
  	pType1FormatD->bBitResolution   	= 0x20;                                    /* Number of bits used */
  }

  if(pDevice->pbkSampleRate == SAMPLE_RATE_48K)
  {
#if DISCRETE_FREQ
#if TEST_44100
  	pType1FormatD->bSamFreqType     	= 3;        /* Number of descrete frequencies */
  	pType1FormatD->tSampFreq[0]     	= 0x44; 	/* Sampling freq = 44.1Khz (0x00AC44) */
  	pType1FormatD->tSampFreq[1]       	= 0xAC;
  	pType1FormatD->tSampFreq[2]     	= 0x00;
  	pType1FormatD->tSampFreq[3]     	= 0x80; 	/* Sampling freq = 48Khz (0x00BB80) */
  	pType1FormatD->tSampFreq[4]       	= 0xBB;
  	pType1FormatD->tSampFreq[5]     	= 0x00;
  	pType1FormatD->tSampFreq[6]     	= 0x00; 	/* Sampling freq = 96Khz (0x017700) */
  	pType1FormatD->tSampFreq[7]       	= 0x77;
  	pType1FormatD->tSampFreq[8]     	= 0x01;
#else
  	pType1FormatD->bSamFreqType     	= 2;        /* Number of descrete frequencies */
  	pType1FormatD->tSampFreq[0]     	= 0x80; 	/* Sampling freq = 48Khz (0x00BB80) */
  	pType1FormatD->tSampFreq[1]       	= 0xBB;
  	pType1FormatD->tSampFreq[2]     	= 0x00;
  	pType1FormatD->tSampFreq[3]     	= 0x00; 	/* Sampling freq = 96Khz (0x017700) */
  	pType1FormatD->tSampFreq[4]       	= 0x77;
  	pType1FormatD->tSampFreq[5]     	= 0x01;
#endif
#else
  	pType1FormatD->bSamFreqType     	= 1;        /* Number of descrete frequencies */
  	pType1FormatD->tSampFreq[0]     	= 0x80; 	/* Sampling freq = 48Khz (0x00BB80) */
  	pType1FormatD->tSampFreq[1]       	= 0xBB;
  	pType1FormatD->tSampFreq[2]     	= 0x00;
#endif
  }
  else if(pDevice->pbkSampleRate == SAMPLE_RATE_96K)
  {
  	pType1FormatD->bSamFreqType     	= 1;		/* Number of descrete frequencies */
  	pType1FormatD->tSampFreq[0]     	= 0x00; 	/* Sampling freq = 96Khz (0x017700) (16 bits)*/
  	pType1FormatD->tSampFreq[1]       	= 0x77;
  	pType1FormatD->tSampFreq[2]     	= 0x01;
  }
  else
  {
  	pType1FormatD->bSamFreqType     	= 2;		/* Number of descrete frequencies */
  	pType1FormatD->tSampFreq[0]     	= 0x80; 	/* Sampling freq = 48Khz (0x00BB80) */
  	pType1FormatD->tSampFreq[1]       	= 0xBB;
  	pType1FormatD->tSampFreq[2]     	= 0x00;
  	pType1FormatD->tSampFreq[3]     	= 0x00; 	/* Sampling freq = 96Khz (0x017700) */
  	pType1FormatD->tSampFreq[4]       	= 0x77;
  	pType1FormatD->tSampFreq[5]     	= 0x01;
  }

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[14].length = pType1FormatD->bLength;
  pDevice->IFaceClassSpecificObj[14].pIntSpecificData = pType1FormatD;
  pDevice->IFaceClassSpecificObj[14].pNext = NULL;

  /****************************************************************
  *
  * Create Standard Audio Endpoint Endpoint descriptor - Audio Playback
  *
  */
  /* Calculate the playback endpoint packet size */
  pDevice->playbackMaxPacketSize = 192 * pDevice->playbackChannels;

  pOStrmEpPbkInf = &pDevice->EndPointObjects[AUDIO_STREAM_EP_PLAYBACK];

  /* This will determine what EP number we are assigned */
  LogEpInfo.dwMaxEndpointSize = pDevice->playbackMaxPacketSize;

  pOStrmEpPbkInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOStrmEpPbkInf->pObj,&LogEpInfo);

  pEndpointD = ((PENDPOINT_OBJECT)pOStrmEpPbkInf->pObj)->pEndpointDesc;

  /* Attach OUT Endpoint Object */
  /* Create effectiveInterfaceID HI byte is Alt Interface Number and LOW Byte is the
     Interface number */
  effectiveInterfaceID = (pDevice->InterfaceObjects[AUDIO_STREAM_OUT].ID | (ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING << 24));
  adi_usb_AttachEndpoint(effectiveInterfaceID, pOStrmEpPbkInf->ID);

  pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOStrmEpPbkInf->pObj)->EPInfo);

  /* Set the callback */
  pEpInfo->EpCallback = AudioEndpointCompleteCallback;

  pDevice->pOUTEPInfo = pEpInfo;

  pEndpointD->bLength 			=  AUDIO_STREAMING_ENDPOINT_DESCRIPTOR_LEN;
  pEndpointD->bDescriptorType  	=  TYPE_ENDPOINT_DESCRIPTOR;
  pEndpointD->bEndpointAddress 	=  (((u8)EP_DIR_OUT) | ((u8)pOStrmEpPbkInf->ID));

  if(pDevice->enableFeedback == true)
  {
  	pEndpointD->bAttributes      	=  USB_AUDIO_EP_ATTRIB_ASYNC | USB_AUDIO_EP_ATTRIB_ISOCH;
  }
  else
  {
  	pEndpointD->bAttributes      	=  USB_AUDIO_EP_ATTRIB_ADAPTIVE | USB_AUDIO_EP_ATTRIB_ISOCH;
  }

  pEndpointD->wMaxPacketSize   	=  pDevice->playbackMaxPacketSize;
  pEndpointD->bInterval        	=  ENDPOINT_INTERVAL;

  /* Add the endpoint */
  ((PENDPOINT_OBJECT)pOStrmEpPbkInf->pObj)->pEndpointSpecificObj = &pDevice->EndpointSpecificObj[0];

  /****************************************************************
  *
  * Add the Audio specific info to the end of our standard endpoint
  *
  */
  pAudioEpDExt = &pDevice->acSpecifcInfo.audioEpDExt[0];

  if(pDevice->enableFeedback == true)
  {
  	pAudioEpDExt->bRefresh        =  1;
  	pAudioEpDExt->bSynchAddress   =  FEEDBACK_PBK_EP;
  }
  else
  {
  	pAudioEpDExt->bRefresh        =  0;
  	pAudioEpDExt->bSynchAddress   =  0;
  }

  /* Add the class specific information */
  pDevice->EndpointSpecificObj[0].length = AUDIO_EP_EXTRA_BYTES;
  pDevice->EndpointSpecificObj[0].pEpSpecificData = pAudioEpDExt;
  pDevice->EndpointSpecificObj[0].pNext = &pDevice->EndpointSpecificObj[1];

  /****************************************************************
  *
  * Create Class Specific Audio Endpoint Endpoint descriptor
  *
  */
  pAudioEpD = &pDevice->acSpecifcInfo.audioEpD;

  pAudioEpD->bLength       		= AUDIO_STREAMING_ENDPOINT_GENERAL_DESCRIPTOR_LEN;
  pAudioEpD->bDescriptorType    = USB_AUDIO_DESCRIPTOR_ENDPOINT;
  pAudioEpD->bDescriptorSubtype = USB_AS_DESCRIPTOR_SUBTYPE_GENERAL;
#if DISCRETE_FREQ
  pAudioEpD->bmAttributes     	= 0x01;
#else
  pAudioEpD->bmAttributes     	= 0x00;
#endif
  pAudioEpD->bLockDelayUnits    = 0x00;
  pAudioEpD->bLockDelay[0] 		= 0x00;
  pAudioEpD->bLockDelay[1] 		= 0x00;

  /* Just attach the AS Class Specific EP Information */
  pDevice->EndpointSpecificObj[1].length = AUDIO_STREAMING_ENDPOINT_GENERAL_DESCRIPTOR_LEN;
  pDevice->EndpointSpecificObj[1].pEpSpecificData = pAudioEpD;
  pDevice->EndpointSpecificObj[1].pNext = NULL;


   if(pDevice->enableFeedback == true)
   {
		/* Create a feedback endpoint.  This endpoint will be used if we have defined
   		an Asynchronous Isochronous endpoint.  It will notify the host as to if the
   		playback data needs to be sent faster, slower or normally.  This endpoint
   		will be attached to the streaming 48k interface.
		*/

  		/* Get a free endpoint object */
  		pOFeedBackStrmEpPbkInf = &pDevice->EndPointObjects[AUDIO_STREAM_EP_FEEDBACK];

  		/* This will determine what EP number we are assigned */
  		LogEpInfo.dwMaxEndpointSize = MAX_PACKET_SIZE_FEEDBACK;

  		/* Create the endpoint, this will return the endpoint number */
  		pOFeedBackStrmEpPbkInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOFeedBackStrmEpPbkInf->pObj,&LogEpInfo);

  		/* Get a pointer to the endpoint descriptor */
  		pEndpointD = ((PENDPOINT_OBJECT)pOFeedBackStrmEpPbkInf->pObj)->pEndpointDesc;

  		/* Attach IN Endpoint Object */
  		/* Create effectiveInterfaceID HI byte is Alt Interface Number and LOW Byte is the
     	   Interface number */
  		effectiveInterfaceID = (pDevice->InterfaceObjects[AUDIO_STREAM_OUT].ID | (ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING << 24));
  		adi_usb_AttachEndpoint(effectiveInterfaceID, pOFeedBackStrmEpPbkInf->ID);

  		/* Get a pointer to the endpoint info structure */
  		pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOFeedBackStrmEpPbkInf->pObj)->EPInfo);

  		/* Set the callback */
  		pEpInfo->EpCallback = FeedbackEndpointCompleteCallback;

  		pDevice->pOUTEPInfo = pEpInfo;

  		/* Lets fill in the endpoint information */
  		pEndpointD->bLength 			=  AUDIO_STREAMING_ENDPOINT_DESCRIPTOR_LEN;
  		pEndpointD->bDescriptorType  	=  TYPE_ENDPOINT_DESCRIPTOR;
  		pEndpointD->bEndpointAddress 	=  (((u8)EP_DIR_IN) | ((u8)pOFeedBackStrmEpPbkInf->ID));
  		pEndpointD->bAttributes      	=  USB_AUDIO_EP_ATTRIB_ISOCH;
  		pEndpointD->wMaxPacketSize   	=  MAX_PACKET_SIZE_FEEDBACK;
  		pEndpointD->bInterval        	=  4;

  		/* Add the endpoint */
  		((PENDPOINT_OBJECT)pOFeedBackStrmEpPbkInf->pObj)->pEndpointSpecificObj = &pDevice->EndpointSpecificObj[2];

  		/****************************************************************
  		*
  		* Add the Audio specific info to the end of our standard endpoint
  		*
  		*/
  		pAudioEpDExt = &pDevice->acSpecifcInfo.audioEpDExt[1];

  		pAudioEpDExt->bRefresh        =  1;
  		pAudioEpDExt->bSynchAddress   =  0;

  		/* Add the class specific information */
  		pDevice->EndpointSpecificObj[2].length = AUDIO_EP_EXTRA_BYTES;
  		pDevice->EndpointSpecificObj[2].pEpSpecificData = pAudioEpDExt;
  		pDevice->EndpointSpecificObj[2].pNext = NULL;
   }

  /****************************************************************
  *
  * Audio Streaming Interface (USB IN) configuration, Idle
  *
  */
  pOStrmIdleRecInf = &pDevice->InterfaceObjects[AUDIO_STREAM_IN];
  pOStrmIdleRecInf->ID = adi_usb_CreateInterface((PINTERFACE_OBJECT*)&pOStrmIdleRecInf->pObj);

  /* Attach Audio Stream IN Interface object to the Configuration object */
  adi_usb_AttachInterface(pDevice->ConfigObject.ID,pOStrmIdleRecInf->ID);

  pInterfaceD = ((PINTERFACE_OBJECT)pOStrmIdleRecInf->pObj)->pInterfaceDesc;

  pInterfaceD->bLength            = INTERFACE_DESCRIPTOR_LEN;
  pInterfaceD->bDescriptorType    = TYPE_INTERFACE_DESCRIPTOR;
  pInterfaceD->bInterfaceNumber   = pOStrmIdleRecInf->ID;
  pInterfaceD->bAlternateSetting  = ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING_IDLE;
  pInterfaceD->bNumEndpoints      = 0;
  pInterfaceD->bInterfaceClass    = USB_AUDIO_CLASS_CODE;
  pInterfaceD->bInterfaceSubClass = USB_AUDIO_SUBCLASS_AUDIO_STREAMING;
  pInterfaceD->bInterfaceProtocol = USB_AUDIO_PROTOCOL_NONE;
  pInterfaceD->bIInterface        = 0;

  /****************************************************************
  *
  * Audio Streaming Interface (USB IN) configuration
  *
  */

  /* Create an alternate interface object */
  if (!adi_usb_CreateInterfaceObject(&((PINTERFACE_OBJECT)pOStrmIdleRecInf->pObj)->pAltInterfaceObj))
  {
	  /* Get a pointer to the Alt Interface descriptor and the fill it */
  	pInterfaceD = ((PINTERFACE_OBJECT)pOStrmIdleRecInf->pObj)->pAltInterfaceObj->pInterfaceDesc;

  	pInterfaceD->bLength            = INTERFACE_DESCRIPTOR_LEN;
  	pInterfaceD->bDescriptorType    = TYPE_INTERFACE_DESCRIPTOR;
  	pInterfaceD->bInterfaceNumber   = pOStrmIdleRecInf->ID;
  	pInterfaceD->bAlternateSetting  = ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING;
  	pInterfaceD->bNumEndpoints      = 1;
  	pInterfaceD->bInterfaceClass    = USB_AUDIO_CLASS_CODE;
  	pInterfaceD->bInterfaceSubClass = USB_AUDIO_SUBCLASS_AUDIO_STREAMING;
  	pInterfaceD->bInterfaceProtocol = USB_AUDIO_PROTOCOL_NONE;
  	pInterfaceD->bIInterface        = 0;
  }

  /* Now add the general descriptor */
  ((PINTERFACE_OBJECT)pOStrmIdleRecInf->pObj)->pAltInterfaceObj->pIntSpecificObj = &pDevice->IFaceClassSpecificObj[15];

  /****************************************************************
  *
  * Create Audio Class specific General Descriptor
  *
  */
  pFormatTypeD_IN = &pDevice->acSpecifcInfo.in_formatTypeD;

  pFormatTypeD_IN->bLength       		= AUDIO_STREAMING_GENERAL_DESCRIPTOR_LEN;
  pFormatTypeD_IN->bDescriptorType   	= USB_AUDIO_DESCRIPTOR_INTERFACE;
  pFormatTypeD_IN->bDescriptorSubtype   = USB_AS_DESCRIPTOR_SUBTYPE_GENERAL;
  pFormatTypeD_IN->bTerminalLink     	= AUDIO_TERMID_USBIN;
  pFormatTypeD_IN->bDelay        		= 1;
  pFormatTypeD_IN->bFormatTag_lo     	= LOBYTE(USB_AUDIO_FORMAT_TAG_PCM);
  pFormatTypeD_IN->bFormatTag_hi     	= HIBYTE(USB_AUDIO_FORMAT_TAG_PCM);

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[15].length = pFormatTypeD_IN->bLength;
  pDevice->IFaceClassSpecificObj[15].pIntSpecificData = pFormatTypeD_IN;
  pDevice->IFaceClassSpecificObj[15].pNext = &pDevice->IFaceClassSpecificObj[16];

  /****************************************************************
  *
  * Create Audio Class specific Type 1 Format Descriptor
  *
  */
  pType1FormatD_IN = &pDevice->acSpecifcInfo.in_type1FormatD;

  pType1FormatD_IN->bLength        	    = AUDIO_REC_STREAMING_TYPE_I_FMT_DESCRIPTOR_LEN;/* Size of this descriptor */
  pType1FormatD_IN->bDescriptorType     = USB_AUDIO_DESCRIPTOR_INTERFACE;               /* Interface Descriptor Type */
  pType1FormatD_IN->bDEscriptorSubType  = USB_AS_DESCRIPTOR_SUBTYPE_FORMAT_TYPE;        /* Descriptor Sub Type */
  pType1FormatD_IN->bFormatType      	= USB_AUDIO_FORMAT_TYPE_I;                      /* Format Type */
  pType1FormatD_IN->bNrChannels      	= NUM_CHANNELS_RECORDING;                       /* Number of Physical Channels */
  pType1FormatD_IN->bSubFrameSize    	= CONTAINER_SIZE / 8;                           /* Number of bytes in subframe */
  pType1FormatD_IN->bBitResolution   	= BIT_DEPTH;                                    /* Number of bits used */
  pType1FormatD_IN->bSamFreqType     	= USB_AUDIO_SAMPLING_FREQ_DISCREET;             /* How sampling freq can be programmed */
  pType1FormatD_IN->tSampFreq[0]     	= 0x80; /* 0xBB80 = 48000 Hz */                 /* Sampling freq = 48000hz (0x00BB80) (24 bits)*/
  pType1FormatD_IN->tSampFreq[1]        = 0xBB;
  pType1FormatD_IN->tSampFreq[2]     	= 0x00;
  pType1FormatD_IN->tSampFreq[3]     	= 0x44; /* 0xAC44 = 44000 Hz */
  pType1FormatD_IN->tSampFreq[4]        = 0xAC;
  pType1FormatD_IN->tSampFreq[5]     	= 0x00;

  /* Add the class specific information */
  pDevice->IFaceClassSpecificObj[16].length = pType1FormatD_IN->bLength;
  pDevice->IFaceClassSpecificObj[16].pIntSpecificData = pType1FormatD_IN;
  pDevice->IFaceClassSpecificObj[16].pNext = NULL;

  /****************************************************************
  *
  * Create Standard Audio Endpoint Endpoint descriptor - Audio Record
  *
  */
  if(pDevice->enableFeedback)
  {
  	pOStrmEpRecInf = &pDevice->EndPointObjects[AUDIO_STREAM_EP_RECORD];
  }
  else
  {
  	pOStrmEpRecInf = &pDevice->EndPointObjects[AUDIO_STREAM_EP_RECORD - 1];
  }

  /* Set the record endpoint packet size */
  pDevice->recordMaxPacketSize = 512;

  /* This will determine what EP number we are assigned */
  LogEpInfo.dwMaxEndpointSize = pDevice->recordMaxPacketSize;

  pOStrmEpRecInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOStrmEpRecInf->pObj,&LogEpInfo);

  pEndpointD = ((PENDPOINT_OBJECT)pOStrmEpRecInf->pObj)->pEndpointDesc;

  /* Attach IN Endpoint Object */
  /* Create effectiveInterfaceID HI byte is Alt Interface Number and LOW Byte is the
     Interface number */
  effectiveInterfaceID = (pDevice->InterfaceObjects[AUDIO_STREAM_IN].ID | (ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING << 24));
  adi_usb_AttachEndpoint(effectiveInterfaceID, pOStrmEpRecInf->ID);

  pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOStrmEpRecInf->pObj)->EPInfo);

  /* Set the callback */
  pEpInfo->EpCallback = AudioEndpointCompleteCallback;

  pDevice->pINEPInfo = pEpInfo;

  pEndpointD->bLength 			=  AUDIO_STREAMING_ENDPOINT_DESCRIPTOR_LEN;
  pEndpointD->bDescriptorType  	=  TYPE_ENDPOINT_DESCRIPTOR;
  pEndpointD->bEndpointAddress 	=  (((u8)EP_DIR_IN) | ((u8)pOStrmEpRecInf->ID));
  pEndpointD->bAttributes      	=  USB_AUDIO_EP_ATTRIB_ISOCH;
  pEndpointD->wMaxPacketSize   	=  pDevice->recordMaxPacketSize;
  pEndpointD->bInterval        	=  ENDPOINT_INTERVAL;

  /* Add the endpoint */
  ((PENDPOINT_OBJECT)pOStrmEpRecInf->pObj)->pEndpointSpecificObj = &pDevice->EndpointSpecificObj[3];

  /****************************************************************
  *
  * Add the Audio specific info to the end of our standard endpoint
  *
  */
  pAudioEpDExt = &pDevice->acSpecifcInfo.in_audioEpDExt;

  pAudioEpDExt->bRefresh        =  0;
  pAudioEpDExt->bSynchAddress   =  0;

  /* Add the class specific information */
  pDevice->EndpointSpecificObj[3].length = AUDIO_EP_EXTRA_BYTES;
  pDevice->EndpointSpecificObj[3].pEpSpecificData = pAudioEpDExt;
  pDevice->EndpointSpecificObj[3].pNext = &pDevice->EndpointSpecificObj[4];

  /****************************************************************
  *
  * Create Class Specific Audio Endpoint Endpoint descriptor
  *
  */
  pAudioEpD = &pDevice->acSpecifcInfo.in_audioEpD;

  pAudioEpD->bLength       		= AUDIO_STREAMING_ENDPOINT_GENERAL_DESCRIPTOR_LEN;
  pAudioEpD->bDescriptorType    = USB_AUDIO_DESCRIPTOR_ENDPOINT;
  pAudioEpD->bDescriptorSubtype = USB_AS_DESCRIPTOR_SUBTYPE_GENERAL;
  pAudioEpD->bmAttributes     	= 0x00;
  pAudioEpD->bLockDelayUnits    = 0x00;
  pAudioEpD->bLockDelay[0] 		= 0x00;
  pAudioEpD->bLockDelay[1] 		= 0x00;

  /* Just attach the AS Class Specific EP Information */
  pDevice->EndpointSpecificObj[4].length = AUDIO_STREAMING_ENDPOINT_GENERAL_DESCRIPTOR_LEN;
  pDevice->EndpointSpecificObj[4].pEpSpecificData = pAudioEpD;
  pDevice->EndpointSpecificObj[4].pNext = NULL;

  /****************************************************************
  *
  * If needed enable buffers in cache
  *
  */
  if( (__cplb_ctrl & CPLB_ENABLE_DCACHE ) || (__cplb_ctrl & CPLB_ENABLE_DCACHE2))
  {
      adi_dev_Control(pDevice->PeripheralDevHandle,
                      ADI_USB_CMD_BUFFERS_IN_CACHE,
                      (void *)TRUE);
  }

  return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens the Audio Class device
*                   in Device mode
*
*********************************************************************/
__ADI_USB_AUDIO_SECTION_CODE
static u32 adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE       ManagerHandle,         /* device manager handle */
    u32                          DeviceNumber,          /* device number */
    ADI_DEV_DEVICE_HANDLE        DeviceHandle,          /* device handle */
    ADI_DEV_PDD_HANDLE           *pPDDHandle,           /* pointer to PDD handle location */
    ADI_DEV_DIRECTION            Direction,             /* data direction */
    void                         *pCriticalRegionArg,   /* critical region imask storage location */
    ADI_DMA_MANAGER_HANDLE       DMAHandle,             /* handle to the DMA manager */
    ADI_DCB_HANDLE               DCBHandle,             /* callback handle */
    ADI_DCB_CALLBACK_FN          DMCallback             /* device manager callback function */
)
{
    u32 Result      = ADI_DEV_RESULT_SUCCESS;
    adi_usb_audio_dev_handle = ManagerHandle;
    adi_usb_audio_dma_handle = DMAHandle;
    adi_usb_audio_dcb_handle = DCBHandle;

    AUDIO_STREAMING_DEV_DATA *pDevice = &adi_usb_audio_def;

    /* Check if the device is already opened */
    if(!pDevice->Open)
    {
        pDevice->Open               = true;
        pDevice->DeviceHandle       = DeviceHandle;
        pDevice->DCBHandle          = DCBHandle;
        pDevice->DMCallback         = DMCallback;
        pDevice->CriticalData       = pCriticalRegionArg;
        pDevice->Direction          = Direction;

        pDevice->deviceReady        = false;
        pDevice->playbackStarted    = false;
        pDevice->pbkNotifyApp		= false;
        pDevice->recordStarted      = false;

        pDevice->currentRxPointer   = 0;
        pDevice->pTopRxBuffer       = NULL;
        pDevice->pCurrentRxBuffer   = NULL;
        pDevice->pRxBufferQueue     = NULL;
        pDevice->pRxBufferQueueTop  = NULL;

        pDevice->codecStarted       = false;
        pDevice->bufferReady        = false;

		/* Set default number of playback channels */
        pDevice->playbackChannels   = 2;

        /* MIC is default record device */
        pDevice->micDefaultRecord		= true;

        /* Line-In is default record input */
        pDevice->recordSelect 	 	= MIC_IN_SELECT;

        /* Include Line-In Mic in mixer */
        pDevice->lineInMicMixer 	= true;

        /* Include Mic in Mixer */
        pDevice->micInAdcMixer		= true;

        /* Include Mic in record controls */
        pDevice->enableMicRecordCtl = true;

        /* Include line-in in record controls */
        pDevice->enableLineInRecordCtl = true;

        /* 16 bits of playback stream subframe used */
        pDevice->playback16BitResolution = true;

        /* Playback Sample Rate 48k */
        pDevice->pbkSampleRate = SAMPLE_RATE_48K;

        /* Default feedback setting (NO FEEDBACK) */
        pDevice->enableFeedback			= false;

        /* Set the default Playback volume levels
         * (NOTE: these values are the original values used, but
         * they are incorrect as the lower bound should be 0x8001
         * and do not reflect the true values of the actual codec.
         * they are kept for backward compatibility, but can be
         * overridden with the ADI_USB_AUDIO_CMD_SET_PLAYBACK_VOL_LEVELS
         * IOCTL.
        */
        pDevice->PlaybackLevels.Min = PBKVOLUME_MIN;
        pDevice->PlaybackLevels.Max = PBKVOLUME_MAX;
        pDevice->PlaybackLevels.Res = PBKVOLUME_RES;
        pDevice->PlaybackLevels.Cur = 0;
        
        /* Set the default Record Volume levels
         * (NOTE: these values are the original values used, but
         * they are incorrect as the lower bound should be 0x8001
         * and do not reflect the true values of the actual codec.
         * they are kept for backward compatibility, but can be
         * overridden with the ADI_USB_AUDIO_CMD_SET_PLAYBACK_VOL_LEVELS
         * IOCTL.
        */
        pDevice->RecordLevels.Min = RECVOLUME_MIN;
        pDevice->RecordLevels.Max = RECVOLUME_MAX;
        pDevice->RecordLevels.Res = RECVOLUME_RES;
    }

    return(Result);
}

/*********************************************************************
*
*   Function:       pddClose
*
*********************************************************************/

__ADI_USB_AUDIO_SECTION_CODE
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE PDDHandle    /* PDD handle */
)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    AUDIO_STREAMING_DEV_DATA   *pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;

    /* Close down the peripheral driver */
    Result = adi_dev_Close(pDevice->PeripheralDevHandle);

    return(Result);
}

/*********************************************************************
*
*   Function:       pddRead
*
*********************************************************************/

__ADI_USB_AUDIO_SECTION_CODE
static u32 adi_pdd_Read(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
	u32 i;
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    AUDIO_STREAMING_DEV_DATA   *pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;

    /* Here we queue up the buffers for audio playbeck  */
    /* This is called from the application which passes */
    /* down the audio codec buffers                     */

    /* Simply queue up the next one */
    void *pExitCriticalRegionArg = adi_int_EnterCriticalRegion(NULL);

    if (pDevice->pCurrentRxBuffer) {
        if (pDevice->pRxBufferQueue) {
            pDevice->pRxBufferQueue->pNext = &pBuffer->OneD;
            pDevice->pRxBufferQueue = pDevice->pRxBufferQueue->pNext;
        }
        else {
            pDevice->pRxBufferQueue = &pBuffer->OneD;
            pDevice->pRxBufferQueueTop = &pBuffer->OneD;
        }

    }
    else {
        /* Add a buffer to our RX queue */
        pDevice->pTopRxBuffer       = &pBuffer->OneD;
        pDevice->pCurrentRxBuffer   = &pBuffer->OneD;
        pDevice->currentRxPointer   = 0;
    }

    adi_int_ExitCriticalRegion(pExitCriticalRegionArg);

    return(Result);
}

/*********************************************************************
*
*   Function:       pddWrite
*
*********************************************************************/

__ADI_USB_AUDIO_SECTION_CODE
static u32 adi_pdd_Write(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    AUDIO_STREAMING_DEV_DATA   *pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;

    /* Get the proper endpoint to write to */
    ((ADI_DEV_1D_BUFFER*)pBuffer)->Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->audio_IN_EP;

    /* Write to the peripheral driver */
     adi_dev_Write(pDevice->PeripheralDevHandle,
                   ADI_DEV_1D,
                   (ADI_DEV_BUFFER *)pBuffer);

    return(Result);
}

/*********************************************************************
*
*   Function:       EnumerateEndpoints
*
*********************************************************************/

__ADI_USB_AUDIO_SECTION_CODE
static s32 EnumerateEndpoints(ADI_ENUM_ENDPOINT_INFO *pEnumEpInfo, AUDIO_STREAMING_DEV_DATA *pDevice)
{
int i;
unsigned int Result=ADI_DEV_RESULT_SUCCESS;
ENDPOINT_DESCRIPTOR *pEndpointD;

ADI_USB_APP_EP_INFO *pEpInfo = pEnumEpInfo->pUsbAppEpInfo;

s32  dwTotalEpEntries = pEnumEpInfo->dwEpTotalEntries;
s32  dwNumEndpoints = sizeof(pDevice->EndPointObjects) / sizeof(OBJECT_INFO);


     /* If supplied memory is not sufficent then we return error */
     if(pEnumEpInfo->dwEpTotalEntries < dwNumEndpoints)
     {
         /* Set the total required entries */
         pEnumEpInfo->dwEpProcessedEntries = dwNumEndpoints;
         Result = ADI_DEV_RESULT_NO_MEMORY;
         return(Result);
     }

     for(i=0; i < dwNumEndpoints; i++)
     {
        /* Get the associated endpoint descriptor */
        pEndpointD = ((PENDPOINT_OBJECT)(pDevice->EndPointObjects[i].pObj))->pEndpointDesc;

        /* Must be a valid endpoint */
        if(pDevice->EndPointObjects[i].ID != 0)
        {
       		/* Get the endpoint ID */
       		pEpInfo->dwEndpointID = pDevice->EndPointObjects[i].ID;

       		/* Set endpoint direction */
       		pEpInfo->eDir = ((pEndpointD->bEndpointAddress >> 7) & 0x1) ? USB_EP_IN : USB_EP_OUT ;

       		/* Set the endpoint attributes */
       		pEpInfo->bAttributes = pEndpointD->bAttributes;

    		pEpInfo++;
        }
     }

   return(Result);
}

/*********************************************************************
*
*   Function:       AudioConfigureEndpoints
*
*********************************************************************/

__ADI_USB_AUDIO_SECTION_CODE
u32 AudioConfigureEndpoints(AUDIO_STREAMING_DEV_DATA *pDevice)
{
  u32 Result = ADI_DEV_RESULT_SUCCESS;
  ADI_ENUM_ENDPOINT_INFO EnumEpInfo;
  static ADI_USB_APP_EP_INFO EpInfo[MAX_ENDPOINTS_OBJS] = {0};

  EnumEpInfo.pUsbAppEpInfo = &EpInfo[0];
  EnumEpInfo.dwEpTotalEntries = sizeof(EpInfo)/sizeof(ADI_USB_APP_EP_INFO);
  EnumEpInfo.dwEpProcessedEntries = 0;

  Result = EnumerateEndpoints(&EnumEpInfo, pDevice);

  if (Result != ADI_DEV_RESULT_SUCCESS)
  {
      return ADI_DEV_RESULT_FAILED;
  }
  if(EpInfo[0].eDir == USB_EP_IN)
  {
    if(pDevice->enableFeedback == true)
    {
    	pDevice->audio_IN_EP  = EpInfo[0].dwEndpointID;
    	pDevice->audio_OUT_EP = EpInfo[1].dwEndpointID;

		/* Feedback comes after the OUT endpoint */
    	pDevice->audio_FB_EP  = EpInfo[2].dwEndpointID;
    }
	else
	{
    	pDevice->audio_IN_EP  = EpInfo[0].dwEndpointID;
    	pDevice->audio_OUT_EP = EpInfo[1].dwEndpointID;
	}
  }
  else
  {
    if(pDevice->enableFeedback == true)
    {
    	pDevice->audio_OUT_EP  =  EpInfo[0].dwEndpointID;

		/* Feedback comes after the OUT endpoint */
    	pDevice->audio_FB_EP   =  EpInfo[1].dwEndpointID;
    	pDevice->audio_IN_EP   =  EpInfo[2].dwEndpointID;
    }
	else
	{
    	pDevice->audio_OUT_EP  =  EpInfo[0].dwEndpointID;
    	pDevice->audio_IN_EP   =  EpInfo[1].dwEndpointID;
	}
  }

  return Result;
}

/*********************************************************************
*
*   Function:       pddControl
*
*********************************************************************/
__ADI_USB_AUDIO_SECTION_CODE
static u32 adi_pdd_Control(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    u32 Command,                    /* command ID */
    void *pArg                      /* pointer to argument */
)
{
    AUDIO_STREAMING_DEV_DATA   *pDevice;
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;
    PDEVICE_DESCRIPTOR pDevDesc;
    signed char iProductString, iManufacturerString;
    s32                 s32Value;       /* s32 type to avoid casts/warnings etc.    */
    u32                 u32Value;       /* u32 type to avoid casts/warnings etc.    */
    u16                 u16Value;       /* u16 type to avoid casts/warnings etc.    */
    s16                 s16Value;       /* s16 type to avoid casts/warnings etc.    */

    /* assign 16 and 32 bit values for the Value argument */
    s32Value        = (s32)pArg;
    u32Value        = (u32)pArg;
    s16Value        = (s16)s32Value;
    u16Value        = (u16)u32Value;

    switch(Command)
    {
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
            pDevice->DataFlowMethod = (ADI_DEV_MODE)pArg;

        case (ADI_DEV_CMD_SET_DATAFLOW):
        case (ADI_USB_CMD_ENABLE_USB):
        {
            /* Send device specific command peripheral driver */
            Result = adi_dev_Control(pDevice->PeripheralDevHandle,
                                Command,
                                (void*)pArg);
        }
        break;

        case (ADI_USB_CMD_CLASS_SET_CONTROLLER_HANDLE):
        {
             pDevice->PeripheralDevHandle =(ADI_DEV_DEVICE_HANDLE)pArg;
        }
        break;

        case ADI_USB_CMD_CLASS_GET_CONTROLLER_HANDLE:
        {
             u32 *pArgument = (u32*)pArg;
               *pArgument = (u32)pDevice->PeripheralDevHandle;
        }
        break;

        case ADI_USB_CMD_CLASS_CONFIGURE:
        {
             Result =  DevAudioConfigure();
        }
        break;

        case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:
        break;

        case ADI_USB_AUDIO_CMD_SET_RX_BUFFER:
        {
          ((ADI_DEV_1D_BUFFER*)pArg)->Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->audio_OUT_EP;

          adi_dev_Read(pDevice->PeripheralDevHandle,
             ADI_DEV_1D,
             (ADI_DEV_BUFFER *)pArg);
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_TX_BUFFER:
        {
          ((ADI_DEV_1D_BUFFER*)pArg)->Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->audio_IN_EP;

          adi_dev_Write(pDevice->PeripheralDevHandle,
             ADI_DEV_1D,
             (ADI_DEV_BUFFER *)pArg);
        }
        break;

        case ADI_USB_AUDIO_CMD_IS_DEVICE_CONFIGURED:
        {
            if(pDevice->deviceReady == TRUE)
            {
                /*
                Device configured and endpoints set
                now tell the application we are ready
                */
                *((u32 *)pArg) = TRUE;
           }
            else
            {
                *((u32 *)pArg) = FALSE;
            }
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_VID:
        {
            pDevDesc = adi_usb_GetDeviceDescriptor();
            if (!pDevDesc)
            {
                Result = ADI_DEV_RESULT_FAILED;
                break;
            }

            pDevDesc->wIdVendor = u16Value;
        }
        break;

        /* Override default Product ID */
        case ADI_USB_AUDIO_CMD_SET_PID:
        {
            pDevDesc = adi_usb_GetDeviceDescriptor();
            if (!pDevDesc)
            {
                Result = ADI_DEV_RESULT_FAILED;
                break;
            }

            pDevDesc->wIdProduct = u16Value;
        }
        break;

        /* Override default Product string */
        case ADI_USB_AUDIO_CMD_SET_PRODUCT_STRING:
            pDevDesc = adi_usb_GetDeviceDescriptor();
            if (!pDevDesc)
            {
                Result = ADI_DEV_RESULT_FAILED;
                break;
            }

            iProductString = adi_usb_CreateString((char *)pArg);
            if ((iProductString > 0) && (iProductString < USB_MAX_STRINGS))
                pDevDesc->bIProduct = iProductString;
        break;

        /* Override default Manufacturer string */
        case ADI_USB_AUDIO_CMD_SET_MANUFACTURER_STRING:
            pDevDesc = adi_usb_GetDeviceDescriptor();
            if (!pDevDesc)
            {
                Result = ADI_DEV_RESULT_FAILED;
                break;
            }

            iManufacturerString = adi_usb_CreateString((char *)pArg);
            if ((iManufacturerString > 0) && (iManufacturerString < USB_MAX_STRINGS))
                pDevDesc->bIManufacturer = iManufacturerString;
        break;

        /* Set number of playback channels */
        case ADI_USB_AUDIO_CMD_SET_PLAYBACK_CHANNELS:
        {
            pDevice->playbackChannels = u16Value;
        }
        break;

        /* Return one byte back via endpoint zero */
		case ADI_USB_AUDIO_CMD_RETURN_CTL_DATA_1:
		{
             AudioControlData[0] = LOBYTE(u16Value);

             /* Return the Audio control data */
             ReturnAudioControlData(1);
		}

        /* Return two bytes back via endpoint zero */
		case ADI_USB_AUDIO_CMD_RETURN_CTL_DATA_2:
		{
             AudioControlData[0] = LOBYTE(u16Value);
             AudioControlData[1] = HIBYTE(u16Value);

             /* Return the Audio control data */
             ReturnAudioControlData(2);
		}
		break;

		/* Remove both Line-In and Mic from ADC Mixer */
		/* TRUE = remove, FALSE = include			  */
		case ADI_USB_AUDIO_CMD_DISABLE_MIXER:
		{
			if(u16Value & 1)
			{
			  pDevice->lineInMicMixer = false;
			}
			else
			{
			  pDevice->lineInMicMixer = true;
			}
		}
		break;

		/* Include Mic in ADC Mixer */
		/* TRUE = include mic, FALSE = remove mic */
		case ADI_USB_AUDIO_CMD_MIC_IN_ADC_MIXER:
		{
			if(u16Value & 1)
			{
			  pDevice->micInAdcMixer = true;
			}
			else
			{
			  pDevice->micInAdcMixer = false;
			}
		}
        break;

        /* Include MIC in record controls */
        case ADI_USB_AUDIO_CMD_ENABLE_MIC_RECORD_CTL:
        {
			if(u16Value & 1)
			{
	        	pDevice->enableMicRecordCtl = true;
			}
			else
			{
	        	pDevice->enableMicRecordCtl = false;
			}
        }
        break;

        /* Include line-in in record controls */
        case ADI_USB_AUDIO_CMD_ENABLE_LINEIN_RECORD_CTL:
        {
			if(u16Value & 1)
			{
	        	pDevice->enableLineInRecordCtl = true;
			}
			else
			{
	        	pDevice->enableLineInRecordCtl = false;
			}
        }
        break;

        case ADI_USB_AUDIO_CMD_GET_PBK_SAMPLE_FREQ:
        {
             *((u32 *)pArg) = pDevice->playbackSampleFreq;
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_32BIT_PBK_RESOLUTION:
        {
			pDevice->playback16BitResolution = false;
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_16BIT_PBK_RESOLUTION:
        {
			pDevice->playback16BitResolution  = true;
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_PBK_SAMPLE_RATE_48K_ONLY:
        {
        	pDevice->pbkSampleRate = SAMPLE_RATE_48K;
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_PBK_SAMPLE_RATE_96K_ONLY:
        {
        	pDevice->pbkSampleRate = SAMPLE_RATE_96K;
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_PBK_SAMPLE_RATE_96K_48K:
        {
        	pDevice->pbkSampleRate = SAMPLE_RATE_96K_48K;
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_MIC_RECORD_SELECT:
        {
        	pDevice->recordSelect = 1;
        	pDevice->micDefaultRecord = true;
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_PLAYBACK_VOL_LEVELS:
        {
            ADI_USB_AUDIO_LEVELS_DEF *pLevels = (ADI_USB_AUDIO_LEVELS_DEF *)pArg;
            pDevice->PlaybackLevels.Min = DbToHexValue(pLevels->Min);
            pDevice->PlaybackLevels.Max = DbToHexValue(pLevels->Max);
            pDevice->PlaybackLevels.Res = DbToHexValue(pLevels->Res);
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_RECORD_VOL_LEVELS:
        {
            ADI_USB_AUDIO_LEVELS_DEF *pLevels = (ADI_USB_AUDIO_LEVELS_DEF *)pArg;
            pDevice->RecordLevels.Min = DbToHexValue(pLevels->Min);
            pDevice->RecordLevels.Max = DbToHexValue(pLevels->Max);
            pDevice->RecordLevels.Res = DbToHexValue(pLevels->Res);
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_CUR_PLAYBACK_VOL_LEVEL:
        {
            pDevice->PlaybackLevels.Cur = DbToHexValue(s32Value);
            AudioControlData[1] = HIBYTE(pDevice->PlaybackLevels.Cur);
            AudioControlData[0] = LOBYTE(pDevice->PlaybackLevels.Cur);
            ReturnAudioControlData(2);
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_CUR_RECORD_VOL_LEVEL:
        {
            pDevice->RecordLevels.Cur = DbToHexValue(s32Value);
            AudioControlData[1] = HIBYTE(pDevice->RecordLevels.Cur);
            AudioControlData[0] = LOBYTE(pDevice->RecordLevels.Cur);
            ReturnAudioControlData(2);
        }
        break;

        case ADI_USB_AUDIO_CMD_GET_CUR_PLAYBACK_VOL_LEVEL:
        {
            *((s16*)pArg) = HexToDbValue(pDevice->PlaybackLevels.Cur);
        }
        break;

        case ADI_USB_AUDIO_CMD_GET_CUR_RECORD_VOL_LEVEL:
        {
            *((s16*)pArg) = HexToDbValue(pDevice->RecordLevels.Cur);
        }
        break;

        case ADI_USB_AUDIO_CMD_SET_LINEIN_RECORD_SELECT:
        {
        	pDevice->recordSelect = 1;
        	pDevice->micDefaultRecord = false;
        }
        break;

        case ADI_USB_AUDIO_CMD_ENABLE_FEEDBACK:
        {
        	pDevice->feedbackInterval = u32Value;
        	pDevice->enableFeedback = true;
        }
        break;

        case ADI_USB_AUDIO_CMD_DISABLE_FEEDBACK:
        {
        	pDevice->enableFeedback = false;
        }
        break;

        default:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
        break;
    }
    return(Result);
}

/*********************************************************************
*
* Function:   FeedbackEndpointCompleteCallback
*
* Description:  Handle Endpoint feedback data traffic specific to
*               this device class.
*
*********************************************************************/
__ADI_USB_AUDIO_SECTION_CODE
void FeedbackEndpointCompleteCallback(void *Handle, u32  Event, void *pArg)
{
	AUDIO_STREAMING_DEV_DATA   *pDevice;
    pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;

    switch(Event)
    {
        case ADI_USB_EVENT_DATA_TX:
        {
        }
        break;

        default:
        break;
    }
}

/*********************************************************************
*
* Function:   AudioEndpointCompleteCallback
*
* Description:  Handle Endpoint data traffic specific to
*         this device class.
*
*********************************************************************/
__ADI_USB_AUDIO_SECTION_CODE
void AudioEndpointCompleteCallback(void *Handle, u32  Event, void *pArg)
{
    AUDIO_STREAMING_DEV_DATA   *pDevice;
    pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;
    ADI_DEV_1D_BUFFER *pBuffer = (ADI_DEV_1D_BUFFER*)pArg;
    ADI_DEV_1D_BUFFER *pBuff;
    volatile u32 nBytesRemaining, nBytesToCopy;
    u32 *paUsbData = NULL;
    u32 *paClientData = NULL;
    volatile u32 nRelPos,i,j;
	volatile u8 nData1, nData2;

	switch(Event)
    {
        /* Transmit data to Host complete (Device Mode) */
        case ADI_USB_EVENT_DATA_TX:
        {
             /* Callback the application TX complete */
             (pDevice->DMCallback) (Handle, ADI_USB_EVENT_DATA_TX, (void *)0);
        }
        break;

        /* Recieve data from Host complete */
        /* Here  we buffer up the packets when out buffer is full   */
        /* we callback the application with the current buffer and  */
        /* continue filling the next one.                           */
        case ADI_USB_EVENT_DATA_RX:
        {
            if(pDevice->pCurrentRxBuffer)
            {

                nBytesRemaining = pBuffer->ProcessedElementCount;
                paUsbData       = pBuffer->Data;
                nRelPos         = pDevice->currentRxPointer;
				paClientData    = (u32*)(&(((u8*)pDevice->pCurrentRxBuffer->Data)[nRelPos]));


                while (nBytesRemaining && pDevice->pCurrentRxBuffer)
                {
                    /* calculate the number of bytes to transfer
                    */
                    if ( (nRelPos + nBytesRemaining) <= (pDevice->pCurrentRxBuffer->ElementCount *
                    										pDevice->pCurrentRxBuffer->ElementWidth))
                    {

                        nBytesToCopy = nBytesRemaining;
                        nRelPos += nBytesRemaining;
                        nBytesRemaining -= nBytesToCopy;
                    }
                    else
                    {
                        nBytesToCopy = (pDevice->pCurrentRxBuffer->ElementCount *
                    							 pDevice->pCurrentRxBuffer->ElementWidth) - nRelPos;
                        nBytesRemaining = nRelPos + nBytesRemaining - (pDevice->pCurrentRxBuffer->ElementCount *
                        														pDevice->pCurrentRxBuffer->ElementWidth);

                    }

                    /* Copy audio packets to client buffer */
                    for (i=0;i<nBytesToCopy/sizeof(u32);i++) {
                        paClientData[i] = paUsbData[i];
                    }

                    /* If the relative position in the Client buffer is the end of the buffer
                     * reset to zero and move to next buffer; if chained loopback and next buffer
                     * pointer is null move back to the top buffer
                    */
                    if (nRelPos == (pDevice->pCurrentRxBuffer->ElementCount * pDevice->pCurrentRxBuffer->ElementWidth))
                    {
					    nRelPos = 0;

				    	/* Give the buffer to the application */
                       	(pDevice->DMCallback) (Handle, ADI_DEV_EVENT_BUFFER_PROCESSED, (void *)pDevice->pCurrentRxBuffer);

                        /*
                            The first buffer is filled and now we can start the
                            codec.  This is only done if playback is not already
                            started.
                        */
                        if(pDevice->pbkNotifyApp == true)
                        {
                            pDevice->pbkNotifyApp = false;
                            pDevice->playbackStarted = true;

                            /* Calculate the sample frequency based on the packet size */
							pDevice->playbackSampleFreq = (pBuffer->ProcessedElementCount / 4) * 1000;

                            (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_USB_AUDIO_EVENT_PLAYBACK_STARTED, (void *)0);
                        }

                        /*!!!! THIS IS HERE ONLY FOR TEST.  WE NEED TO DETERMINE
                               THE BEST PLACE TO UPDATE THE FEEDBACK VALUE
                        !!!!*/
                        /* Provide feedback if enabled */
                        if(pDevice->enableFeedback == true)
                        {
                        	ProvideFeedback((u32)FEEDBACK_FREQUENCY_NORMAL);
                        }

                        void *pExitCriticalRegionArg = adi_int_EnterCriticalRegion(NULL);
                        pDevice->pCurrentRxBuffer = pDevice->pCurrentRxBuffer->pNext;

                        if (pDevice->pCurrentRxBuffer==NULL) {

                            if (pDevice->DataFlowMethod==ADI_DEV_MODE_CHAINED_LOOPBACK)
                            {
                                pDevice->pCurrentRxBuffer = pDevice->pTopRxBuffer;
                            }
                            else if (pDevice->pRxBufferQueueTop) {
                                /* attach queued buffers to buffer list */
                                pDevice->pCurrentRxBuffer = pDevice->pRxBufferQueueTop;
                                pDevice->pRxBufferQueueTop = NULL;
                                pDevice->pRxBufferQueue = NULL;
                            }
                        }
                        adi_int_ExitCriticalRegion(pExitCriticalRegionArg);
                    }
                }
                /* store the updated relative pointer */
                pDevice->currentRxPointer = nRelPos;
            }
            else {
                gnMissed++;
            }

            pBuffer->ElementCount = pDevice->playbackMaxPacketSize;
            pBuffer->Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->audio_OUT_EP;
	        pBuffer->ProcessedElementCount = 0x0;
	        pBuffer->pNext = NULL;

	        /* Give the back to the driver */
            adi_dev_Read(pDevice->PeripheralDevHandle,
                         ADI_DEV_1D,
                         (ADI_DEV_BUFFER*)pBuffer);
        }
        break;

        /* Recieved a packet but we have no buffer */
        case ADI_USB_EVENT_PKT_RCVD_NO_BUFFER:
        {
            /* Callback the application */
            (pDevice->DMCallback) (Handle, Event, (void *)0);
        }
        break;

       default:
       break;
    }
}

/*********************************************************************
*
* Function:   EndpointZeroCallback
*
* Description:  Handle Endpoint 0 data traffic specific to
*         this device class. Including the Audio
*         specific requests.
*
*********************************************************************/
__ADI_USB_AUDIO_SECTION_CODE
void EndpointZeroCallback( void *Handle, u32 Event, void *pBuffer)
{
    PUSB_EP_INFO              pEpInfo;
    OBJECT_INFO               *pOInf;

    u16                       codecReg;
#if DISCRETE_FREQ
    u32						  *controlData;
#else
    u16						  *controlData;
#endif
    u32                       interface;
    u32                       altInterface;
    u32						  feedbackControl;

    AUDIO_STREAMING_DEV_DATA   *pDevice;
    pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;

    u32 Result = ADI_DEV_RESULT_SUCCESS;		/* result */

  switch(Event)
  {
    /* Host sent Set Configuration Command */
    case ADI_USB_EVENT_SET_CONFIG:
    {
      /* Configure our endpoints now */
      AudioConfigureEndpoints(pDevice);
    }
    break;

    /* Host sent Set Interface Command */
    case ADI_USB_EVENT_SET_INTERFACE:
    {
	  /* Get interface and alt-interface numbers */
	  interface = (u32)pBuffer & 0xff;
	  altInterface = ((u32)pBuffer >> 24) & 0xff;

      /* Report that the device is ready */
      if(pDevice->deviceReady == false)
      {
        if((interface == INTERFACE_NUM_FOR_AUDIO_STREAMING) &&
           (altInterface == ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING))
        {
            /* Set the device ready flag */
            pDevice->deviceReady = true;
        }
        else
        {
            /* Clear the device ready flag */
            pDevice->deviceReady = false;
        }

        /* Clear playback and record status */
        pDevice->playbackStarted = false;
        pDevice->recordStarted   = false;
      }

      /* Playback/Record stopped */
      if(((interface == INTERFACE_NUM_FOR_AUDIO_STREAMING) &&
         (altInterface == ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING_IDLE)) ||
         ((interface == INTERFACE_NUM_FOR_AUDIO_STREAMING_RECORD) &&
         (altInterface == ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING_IDLE)) &&
          ((pDevice->playbackStarted == true) || (pDevice->recordStarted == true)))
      {

            /* Notify application record stopped */
            if((pDevice->recordStarted == true) &&
            	(interface == INTERFACE_NUM_FOR_AUDIO_STREAMING_RECORD))
            {
                (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_USB_AUDIO_EVENT_RECORD_STOPPED, (void *)0);

                /* Reset the record started flag */
                pDevice->recordStarted    = false;
            }

            /* Notify application playback stopped */
            if((pDevice->playbackStarted == true) &&
            	(interface == INTERFACE_NUM_FOR_AUDIO_STREAMING))
            {
                (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_USB_AUDIO_EVENT_PLAYBACK_STOPPED, (void *)0);

                /* Reset the playback started flag */
                pDevice->playbackStarted  = false;
                pDevice->pbkNotifyApp	  = false;

                /* Clear the RX buffer pointer */
                pDevice->currentRxPointer = 0;
            }

        return;
      }

      /* Playback started */
      if((interface == INTERFACE_NUM_FOR_AUDIO_STREAMING) &&
         (altInterface == ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING) &&
         (pDevice->playbackStarted == false))
      {

		  pDevice->pbkNotifyApp = true;

		  /* Provide feedback if enabled */
          if(pDevice->enableFeedback == true)
          {
		  	/* Send initial feedback value */
		  	ProvideFeedback((u32)FEEDBACK_FREQUENCY_NORMAL);
          }

          return;
      }

      /* Record started */
      if((interface == INTERFACE_NUM_FOR_AUDIO_STREAMING_RECORD) &&
         (altInterface == ALT_INTERFACE_NUM_FOR_AUDIO_STREAMING) &&
         (pDevice->recordStarted == false))
      {
          (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_USB_AUDIO_EVENT_RECORD_STARTED, (void *)0);

		  pDevice->recordStarted = true;

        return;
      }
    }
    break;

    /* Received data on endpoint zero (Data phase), now process it */
    case ADI_USB_EVENT_RX_COMPLETE:
    {
        /* Get the EP0 buffer in the correct format */
        ADI_DEV_1D_BUFFER  *pEP0Buffer  = (ADI_DEV_1D_BUFFER *)pBuffer;
#if DISCRETE_FREQ
        controlData = (u32 *)pEP0Buffer->Data;
#else
        controlData = (u16 *)pEP0Buffer->Data;
#endif

		/* Host writing codec control data */
        if(pDevice->controlDataDirection == USB_CONTROL_REQUEST_OUT)
		{


        	/* Now write the data to the specified codec control register */
			AudioSetRequest (pDevice->AudioSetupPkt.bRequest,
							#if DISCRETE_FREQ
							 pDevice->AudioSetupPkt.wIndex,
							#else
							 HIBYTE(pDevice->AudioSetupPkt.wIndex),
							#endif
							 pDevice->AudioSetupPkt.wValue,
							 pDevice->AudioSetupPkt.wLength,
							 controlData[0]);
		}
    }
    break;

    /* We recieved an audio setup packet */
    case ADI_USB_EVENT_SETUP_PKT:
    {
      /* Get the EP0 buffer in the correct format */
      ADI_DEV_1D_BUFFER  *pEP0Buffer  = (ADI_DEV_1D_BUFFER *)pBuffer;

      /* Get the buffer data in USB packet format */
      USB_SETUP_PACKET* pSetupData = (USB_SETUP_PACKET*)pEP0Buffer->Data;

      /* Determine what type of request we have */
#if DISCRETE_FREQ
      if((((USB_CONTROL_REQUEST_GET_TYPE (pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_CLASS) &&
        (USB_CONTROL_REQUEST_GET_RECIPIENT (pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_INTERFACE) &&
        (LOBYTE (pSetupData->wIndex) == INTERFACE_FOR_AUDIOCONTROL))
        ||
        ((USB_CONTROL_REQUEST_GET_TYPE (pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_CLASS) &&
        (USB_CONTROL_REQUEST_GET_RECIPIENT (pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_ENDPOINT)))
        )
#else
      if((USB_CONTROL_REQUEST_GET_TYPE (pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_CLASS) &&
        (USB_CONTROL_REQUEST_GET_RECIPIENT (pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_INTERFACE) &&
        (LOBYTE (pSetupData->wIndex) == INTERFACE_FOR_AUDIOCONTROL))
#endif
      {
          /* Process Audio GET/SET Requests */
          if (USB_CONTROL_REQUEST_GET_DIRECTION(pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_OUT)
          {
            /* Save our Audio Setup packet info. for the data phase of the request */
            pDevice->AudioSetupPkt.bRequest = pSetupData->bRequest;
            pDevice->AudioSetupPkt.wIndex   = pSetupData->wIndex;
            pDevice->AudioSetupPkt.wValue   = pSetupData->wValue;
            pDevice->AudioSetupPkt.wLength  = pSetupData->wLength;

            /* Request data from the host */
            /* Data will be returned when we get ADI_USB_EVENT_RX_COMPLETE */
            pDevice->controlDataDirection = USB_CONTROL_REQUEST_OUT;

            /* Get the control data from the host */
            GetAudioControlData(pDevice->AudioSetupPkt.wLength);
          }
          else
          {
          	/* Request data to the host */
            pDevice->controlDataDirection = USB_CONTROL_REQUEST_IN;

            /* Send Control data to the host */
            AudioGetRequest (pSetupData->bRequest, HIBYTE(pSetupData->wIndex), pSetupData->wValue, pSetupData->wLength);
          }
      }
    }
    break;

    case ADI_USB_EVENT_VBUS_TRUE:
        {
          /* Notify the application */
          (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_USB_EVENT_VBUS_TRUE, (void *)0);
        }
    break;

    case ADI_USB_EVENT_SUSPEND:
    break;

    case ADI_USB_EVENT_DISCONNECT:
    {
         /* Reset device ready flag */
         pDevice->deviceReady = FALSE;

   	     /* Reset playback endpoint lists  (data from PC)*/
       	 pOInf = &pDevice->EndPointObjects[AUDIO_STREAM_EP_PLAYBACK];
         pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);
     	 pEpInfo->pFreeRcvList = NULL;
         pEpInfo->pProcessedRcvList = NULL;
         pEpInfo->pFreeXmtList = NULL;
         pEpInfo->pQueuedXmtList = NULL;
         pEpInfo->pTransferBuffer=NULL;
         pEpInfo->EpBytesProcessed=0;

    	 if(pDevice->enableFeedback == true)
    	 {
   	     	/* Reset Feedback endpoint lists */
       	 	pOInf = &pDevice->EndPointObjects[AUDIO_STREAM_EP_FEEDBACK];
         	pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);
 	     	pEpInfo->pFreeRcvList = NULL;
         	pEpInfo->pProcessedRcvList = NULL;
         	pEpInfo->pFreeXmtList = NULL;
         	pEpInfo->pQueuedXmtList = NULL;
         	pEpInfo->pTransferBuffer=NULL;
         	pEpInfo->EpBytesProcessed=0;

	 	    /* Reset record endpoint lists (data to PC) */
 	        pOInf = &pDevice->EndPointObjects[AUDIO_STREAM_EP_RECORD];
 	        pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);
         	pEpInfo->pFreeXmtList = NULL;
         	pEpInfo->pQueuedXmtList = NULL;
 	     	pEpInfo->pFreeRcvList = NULL;
         	pEpInfo->pProcessedRcvList = NULL;
            pEpInfo->pTransferBuffer=NULL;
            pEpInfo->EpBytesProcessed=0;
    	 }
		 else
		 {
 	     	/* Reset record endpoint lists (data to PC) */
 	     	pOInf = &pDevice->EndPointObjects[AUDIO_STREAM_EP_RECORD - 1];
 	     	pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);
         	pEpInfo->pFreeXmtList = NULL;
         	pEpInfo->pQueuedXmtList = NULL;
 	     	pEpInfo->pFreeRcvList = NULL;
         	pEpInfo->pProcessedRcvList = NULL;
         	pEpInfo->pTransferBuffer=NULL;
         	pEpInfo->EpBytesProcessed=0;
		 }

         /* Notify the application via callback */
         (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_USB_EVENT_DISCONNECT, (void *)0);
    }
    break;

    case ADI_USB_EVENT_ROOT_PORT_RESET: // reset signaling detected
    {
          (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_USB_EVENT_ROOT_PORT_RESET, (void *)0);
    }
    break;

    case ADI_USB_EVENT_VBUS_FALSE:    // cable unplugged
    {
          (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_USB_EVENT_VBUS_FALSE, (void *)0);
    }
    break;

    case ADI_USB_EVENT_RESUME:      // device resumed
    {
          (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_USB_EVENT_RESUME, (void *)0);
    }
    break;

    default:
    break;
  }
}

/*------------------------------------------------------------
*
* AudioGetRequest
*
*   This handles the Audio Class Get Requests
*
*------------------------------------------------------------*/
__ADI_USB_AUDIO_SECTION_CODE
static bool AudioGetRequest (u8 bRequest, u8 wIndexHi, u16 wValue, u16 wLength)
{
    bool retVal = false;

    AUDIO_STREAMING_DEV_DATA   *pDevice;
    pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;

    switch (bRequest)
      {
        case GET_CUR:
          switch (wIndexHi)
            {
              case AUDIO_UNITID_MIXER:
              {
                      AudioControlData[0] = LOBYTE(VOLUME_MID);
                      AudioControlData[1] = HIBYTE(VOLUME_MID);

                      /* Return the Audio control data */
                      ReturnAudioControlData(2);

                      retVal = true;
              }
              break;

              case AUDIO_UNITID_PLAYBACK_VOLUME_OUT:
                  switch (HIBYTE(wValue))
                  {
                    case USB_AC_FEATURE_UNIT_MUTE_CONTROL:
                    {
                      AudioControlData[0] = 0x00;

                      /* Return the Audio control data */
                      ReturnAudioControlData(sizeof(u8));

                      retVal = true;
                    }
                    break;

                    case USB_AC_FEATURE_UNIT_VOLUME_CONTROL:
                      switch (LOBYTE(wValue))
                        {
                          case 0:
                          {
	                		/* Callback the application to handle the codec register access */
       	                	(pDevice->DMCallback)(pDevice->DeviceHandle,
    	                					  	ADI_USB_AUDIO_EVENT_GET_MASTER_VOLUME,
    	                					  	(void *)0);
                            retVal = true;
                          }
                          break;

                          case 1:
                          {
                      		AudioControlData[0] = LOBYTE(VOLUME_MID);
                      		AudioControlData[1] = HIBYTE(VOLUME_MID);

                            /* Return the Audio control data */
                            ReturnAudioControlData(2);

                            retVal = true;
                          }
                          break;

                          case 2:
                          {
		                    AudioControlData[0] = LOBYTE(VOLUME_MID);
        		            AudioControlData[1] = HIBYTE(VOLUME_MID);

                            /* Return the Audio control data */
                            ReturnAudioControlData(2);

                            retVal = true;
                          }
                          break;

                          case 0xff:
                          {
                            AudioControlData[0] = 0x00;
                            AudioControlData[1] = 0x00;
                            AudioControlData[2] = 0x00;
                            AudioControlData[3] = 0x00;
                            AudioControlData[4] = 0x00;
                            AudioControlData[5] = 0x00;

                            /* Return the Audio control data */
                            ReturnAudioControlData(6);

                            retVal = true;
                          }
                          break;

                          default:
                          break;
                        }
                      break;
                  }
                break;

              case AUDIO_UNITID_SELECTOR_RECORD:
              {
                  AudioControlData[0] = pDevice->recordSelect;

                  /* Return the Audio control data */
                  ReturnAudioControlData(1);

                  retVal = true;
              }
              break;

              case AUDIO_UNITID_MIC_VOLUME_RECORD:
              case AUDIO_UNITID_MIC_VOLUME_RECORD_1:
              case AUDIO_UNITID_LINEIN_VOLUME_RECORD_1:
              case AUDIO_UNITID_LINEIN_VOLUME_RECORD:
              {
                 switch (HIBYTE(wValue))
                 {
                    case USB_AC_FEATURE_UNIT_MUTE_CONTROL:
                    {
                   	  AudioControlData[0] = 0x00;

                      /* Return the Audio control data */
                      ReturnAudioControlData(1);

                      retVal = true;
                    }
                    break;

                    case USB_AC_FEATURE_UNIT_VOLUME_CONTROL:
                    {
                      switch (LOBYTE(wValue))
                        {
                          case 0:
                          {
                    		if(wIndexHi == AUDIO_UNITID_LINEIN_VOLUME_RECORD_1)
							{
	                        	/* If we have a Line In in the Mixer */
								if(pDevice->lineInMicMixer == true)
								{
	                				/* Callback the application to handle the codec register access */
       	                			(pDevice->DMCallback)(pDevice->DeviceHandle,
    	                					   		  	  ADI_USB_AUDIO_EVENT_GET_LINEIN_VOLUME,
    	                							  	  (void *)0);

								}
								else
								{
			                      AudioControlData[0] = LOBYTE(VOLUME_MID);
            			          AudioControlData[1] = HIBYTE(VOLUME_MID);

        	                      /* Return the Audio control data */
            	                  ReturnAudioControlData(2);
								}
							}
    	                	else if(wIndexHi == AUDIO_UNITID_MIC_VOLUME_RECORD_1)
	                        {

	                        	/* If we have a MIC in the Mixer */
	                        	if(pDevice->micInAdcMixer == true)
	                        	{
	                				/* Callback the application to handle the codec register access */
       	                			(pDevice->DMCallback)(pDevice->DeviceHandle,
    	                					   		  	  ADI_USB_AUDIO_EVENT_GET_MIC_VOLUME,
    	                							  	  (void *)0);
	                        	}
	                        	else
	                        	{
			                      AudioControlData[0] = LOBYTE(VOLUME_MID);
            			          AudioControlData[1] = HIBYTE(VOLUME_MID);

        	                      /* Return the Audio control data */
            	                  ReturnAudioControlData(2);
	                        	}
	                        }
							else /* Record GAIN settings */
							{
		                      AudioControlData[0] = LOBYTE(VOLUME_MID);
        		              AudioControlData[1] = HIBYTE(VOLUME_MID);

                              /* Return the Audio control data */
                              ReturnAudioControlData(2);
							}

                            retVal = true;
                          }
                          break;

                          default:
                          break;
                        }
                    }
                    break;
                  }
            	}
            	break;

              default:
              break;
            }
          break;

        case GET_MIN:
          switch (wIndexHi)
            {
              case AUDIO_UNITID_MIXER:
              {
                AudioControlData[0] = LOBYTE(pDevice->PlaybackLevels.Min);
                AudioControlData[1] = HIBYTE(pDevice->PlaybackLevels.Min);

                /* Return the Audio control data */
                ReturnAudioControlData(2);

                retVal = true;
              }
              break;

              case AUDIO_UNITID_PLAYBACK_VOLUME_OUT:
              {
                AudioControlData[0] = LOBYTE(pDevice->PlaybackLevels.Min);
                AudioControlData[1] = HIBYTE(pDevice->PlaybackLevels.Min);

				if (LOBYTE(wValue) == 0xff)
				{
                    AudioControlData[2] = 0x00;
                    AudioControlData[3] = 0x00;
                    AudioControlData[4] = 0x00;
                    AudioControlData[5] = 0x00;

                    ReturnAudioControlData(6);
				}
				else
				{
                    /* Return the Audio control data */
                    ReturnAudioControlData(2);
				}

                retVal = true;
              }
              break;

              case AUDIO_UNITID_MIC_VOLUME_RECORD:
              case AUDIO_UNITID_MIC_VOLUME_RECORD_1:
              case AUDIO_UNITID_LINEIN_VOLUME_RECORD_1:
              case AUDIO_UNITID_LINEIN_VOLUME_RECORD:
              {
                AudioControlData[0] = LOBYTE(pDevice->RecordLevels.Min);
                AudioControlData[1] = HIBYTE(pDevice->RecordLevels.Min);

                /* Return the Audio control data */
                ReturnAudioControlData(2);

                retVal = true;
              }
              break;

              case AUDIO_UNITID_SELECTOR_RECORD:
              {
                AudioControlData[0] = 1;
                ReturnAudioControlData(1);
              }
              break;

              default:
              break;
            }
          break;

        case GET_MAX:
          switch (wIndexHi)
            {
              case AUDIO_UNITID_MIXER:
              {
                    AudioControlData[0] = LOBYTE(pDevice->PlaybackLevels.Max);
                    AudioControlData[1] = HIBYTE(pDevice->PlaybackLevels.Max);

                    /* Return the Audio control data */
                    ReturnAudioControlData(2);

                    retVal = true;
              }
              break;

              case AUDIO_UNITID_PLAYBACK_VOLUME_OUT:
              {
                    AudioControlData[0] = LOBYTE(pDevice->PlaybackLevels.Max);
                    AudioControlData[1] = HIBYTE(pDevice->PlaybackLevels.Max);

				if (LOBYTE(wValue) == 0xff)
				{
                    AudioControlData[2] = 0x00;
                    AudioControlData[3] = 0x00;
                    AudioControlData[4] = 0x00;
                    AudioControlData[5] = 0x00;

                    ReturnAudioControlData(6);
				}
                else
                {
                    /* Return the Audio control data */
                    ReturnAudioControlData(2);
                }

                retVal = true;
              }
              break;

              case AUDIO_UNITID_MIC_VOLUME_RECORD:
              case AUDIO_UNITID_MIC_VOLUME_RECORD_1:
              case AUDIO_UNITID_LINEIN_VOLUME_RECORD_1:
              case AUDIO_UNITID_LINEIN_VOLUME_RECORD:
              {
                AudioControlData[0] = LOBYTE(pDevice->RecordLevels.Max);
                AudioControlData[1] = HIBYTE(pDevice->RecordLevels.Max);

                /* Return the Audio control data */
                ReturnAudioControlData(2);

                retVal = true;
              }
              break;

              case AUDIO_UNITID_SELECTOR_RECORD:
              {
#if defined(__ADSP_MOAB__)
                AudioControlData[0] = 3;
#else
                AudioControlData[0] = 2;
#endif
                ReturnAudioControlData(1);
              }
              break;

              default:
              break;
            }
          break;

        case GET_RES:
          switch (wIndexHi)
            {
              case AUDIO_UNITID_MIXER:
              {
                    AudioControlData[0] = LOBYTE(pDevice->PlaybackLevels.Res);
                    AudioControlData[1] = HIBYTE(pDevice->PlaybackLevels.Res);

                    /* Return the Audio control data */
                    ReturnAudioControlData(2);

                    retVal = true;
              }
              break;

              case AUDIO_UNITID_PLAYBACK_VOLUME_OUT:
              {
                AudioControlData[0] = LOBYTE(pDevice->PlaybackLevels.Res);
                AudioControlData[1] = HIBYTE(pDevice->PlaybackLevels.Res);

				if (LOBYTE(wValue) == 0xff)
				{
                    AudioControlData[2] = 0x00;
                    AudioControlData[3] = 0x00;
                    AudioControlData[4] = 0x00;
                    AudioControlData[5] = 0x00;

                    ReturnAudioControlData(6);
				}
                else
                {
                    /* Return the Audio control data */
                    ReturnAudioControlData(2);
                }
                retVal = true;

              }
              break;

             case AUDIO_UNITID_MIC_VOLUME_RECORD:
             case AUDIO_UNITID_MIC_VOLUME_RECORD_1:
             case AUDIO_UNITID_LINEIN_VOLUME_RECORD_1:
             case AUDIO_UNITID_LINEIN_VOLUME_RECORD:
             {
                AudioControlData[0] = LOBYTE(pDevice->RecordLevels.Res);
                AudioControlData[1] = HIBYTE(pDevice->RecordLevels.Res);

                /* Return the Audio control data */
                ReturnAudioControlData(2);

                retVal = true;
              }
              break;

              default:
              break;
            }
          break;
      }

    return(retVal);
}

/*------------------------------------------------------------
*
*	AudioSetRequest
*
*   This handles the Audio Class Set Requests
*
*------------------------------------------------------------*/
__ADI_USB_AUDIO_SECTION_CODE
#if DISCRETE_FREQ
static bool AudioSetRequest (u8 bRequest, u16 wIndex, u16 wValue, u16 wLength, u32 data)
#else
static bool AudioSetRequest (u8 bRequest, u8 wIndexHi, u16 wValue, u16 wLength, u16 controlData)
#endif
{
bool retVal = false;

#if DISCRETE_FREQ
u16  controlData = (u16)data;
u32  sampleRate  = data;
u8	 wIndexHi = HIBYTE(wIndex);
#endif

AUDIO_STREAMING_DEV_DATA   *pDevice;
pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;

#if DISCRETE_FREQ
	/* Handle sample rate requests for OUT (playback) endpoint */
	if((bRequest == SET_CUR) && (LOBYTE(wIndex) == pDevice->audio_OUT_EP))
	{
       	/* Callback the application to set the codec sample rate */
        (pDevice->DMCallback)(pDevice->DeviceHandle,
           					  ADI_USB_AUDIO_EVENT_SET_CODEC_SAMPLE_RATE,
           					  (void *)sampleRate);

	  	return true;
	}
#endif

    switch (bRequest)
      {
        case SET_CUR:
          switch (wIndexHi)
            {
              case AUDIO_UNITID_MIXER:
              {
                   retVal = true;
              }
              break;

              case AUDIO_UNITID_PLAYBACK_VOLUME_OUT:
              {
                switch (HIBYTE(wValue))
                  {
                    case USB_AC_FEATURE_UNIT_MUTE_CONTROL:
                    {
                    	/* Callback the application to handle the codec register access */
       	                (pDevice->DMCallback)(pDevice->DeviceHandle,
       	                					  ADI_USB_AUDIO_EVENT_SET_MASTER_MUTE,
       	                 					  (void *)(controlData & 0xff));

                        retVal = true;
                    }
                    break;

                    case USB_AC_FEATURE_UNIT_VOLUME_CONTROL:
                    {
                      switch (LOBYTE(wValue))
                        {
                          case 0:
                          {
                          	s16 decibels = HexToDbValue(controlData);
                          	pDevice->PlaybackLevels.Cur = (s16)controlData;
	                    	/* Callback the application to handle the codec register access */
    	   	                (pDevice->DMCallback)(pDevice->DeviceHandle,
       		                					  ADI_USB_AUDIO_EVENT_SET_MASTER_VOLUME,
       		                					  (void *)controlData);
                          }
                          break;

                          case 1:
                          break;

                          case 2:
                          break;

                          case 0xff:
                          break;

                          default:
                          break;
                        }
                    }
                    break;
                  }
              }
              break;

              case AUDIO_UNITID_SELECTOR_RECORD:
              {
              	   controlData = controlData & 0xff;

              	   pDevice->recordSelect = (u8)controlData & 0x3;

           	   		if(pDevice->recordSelect == 1)
           	   		{
           	   			if(pDevice->micDefaultRecord)
           	   			{
	               			/* Callback the application to handle the codec register access */
                    		(pDevice->DMCallback)(pDevice->DeviceHandle,
    	               							  ADI_USB_AUDIO_EVENT_SET_MIC_SELECT,
    	               							  (void *)0);
           	   			}
           	   			else
           	   			{
	              			/* Callback the application to handle the codec register access */
       	               		(pDevice->DMCallback)(pDevice->DeviceHandle,
    	               							  ADI_USB_AUDIO_EVENT_SET_LINEIN_SELECT,
    	               							  (void *)0);
           	   			}
           	   		}
           	   		else if (pDevice->recordSelect == 2)
           	   		{
           	   			if(pDevice->micDefaultRecord)
           	   			{
		             		/* Callback the application to handle the codec register access */
    	                	(pDevice->DMCallback)(pDevice->DeviceHandle,
    	                						  ADI_USB_AUDIO_EVENT_SET_LINEIN_SELECT,
    	   	            						  (void *)0);
           	   			}
           	   			else
           	   			{
		               		/* Callback the application to handle the codec register access */
    	                	(pDevice->DMCallback)(pDevice->DeviceHandle,
    	                						  ADI_USB_AUDIO_EVENT_SET_MIC_SELECT,
    	   	            						  (void *)0);
           	   			}
           	   		}
                   retVal = true;
              }
              break;

              case AUDIO_UNITID_MIC_VOLUME_RECORD:
              case AUDIO_UNITID_MIC_VOLUME_RECORD_1:
              case AUDIO_UNITID_LINEIN_VOLUME_RECORD_1:
              case AUDIO_UNITID_LINEIN_VOLUME_RECORD:
              {
                switch (HIBYTE(wValue))
                {
                    case USB_AC_FEATURE_UNIT_MUTE_CONTROL:
                    {
                    	if((wIndexHi == AUDIO_UNITID_LINEIN_VOLUME_RECORD_1) ||
                    	   (wIndexHi == AUDIO_UNITID_LINEIN_VOLUME_RECORD))
                    	{
	                		/* Callback the application to handle the codec register access */
       	                	(pDevice->DMCallback)(pDevice->DeviceHandle,
    	                					  	ADI_USB_AUDIO_EVENT_SET_LINEIN_MUTE,
    	                					  	(void *)(controlData & 0xff));
                    	}

                    	if((wIndexHi == AUDIO_UNITID_MIC_VOLUME_RECORD_1) ||
                    	   (wIndexHi == AUDIO_UNITID_MIC_VOLUME_RECORD))
                    	{
	                		/* Callback the application to handle the codec register access */
       	                	(pDevice->DMCallback)(pDevice->DeviceHandle,
    	                					  	ADI_USB_AUDIO_EVENT_SET_MIC_MUTE,
    	                					  	(void *)(controlData & 0xff));
                    	}

                    	retVal = true;
                    }
					break;

                    case USB_AC_FEATURE_UNIT_VOLUME_CONTROL:
                    {
                      switch (LOBYTE(wValue))
                      {
                          case 0:
                          {
                          	s16 decibels = HexToDbValue(controlData);
                          	pDevice->RecordLevels.Cur = (s16)controlData;

                    		if(wIndexHi == AUDIO_UNITID_LINEIN_VOLUME_RECORD_1)
                    		{
		                    	/* Callback the application to handle the codec register access */
	    	   	                (pDevice->DMCallback)(pDevice->DeviceHandle,
	       		                					  ADI_USB_AUDIO_EVENT_SET_LINEIN_VOLUME,
	       		                					  (void *)controlData);
                    		}
							else
                    		if(wIndexHi == AUDIO_UNITID_MIC_VOLUME_RECORD_1)
                    		{
		                    	/* Callback the application to handle the codec register access */
	    	   	                (pDevice->DMCallback)(pDevice->DeviceHandle,
	       		                					  ADI_USB_AUDIO_EVENT_SET_MIC_VOLUME,
	       		                					  (void *)controlData);
                    		}
                    		else
                    		{
		                    	/* Callback the application to handle the codec register access */
	    	   	                (pDevice->DMCallback)(pDevice->DeviceHandle,
	       		                					  ADI_USB_AUDIO_EVENT_SET_RECORD_MASTER_GAIN,
	       		                					  (void *)(controlData));
                   			}
   	                        retVal = true;
                          }
                          break;

                          default:
                          break;
                      }
                    }
                    break;
                  }
               }
               break;

               default:
               break;
             }
             break;

        default:
        break;
      }

    return(retVal);
}

/*********************************************************************
*
*    Function:       ReturnAudioControlData
*
*	 Return Audio control data to the host.
*
*********************************************************************/
__ADI_USB_AUDIO_SECTION_CODE
static void ReturnAudioControlData (u16 bytes)
{
    AUDIO_STREAMING_DEV_DATA   *pDevice;
    pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;

    AudioControlBuffer.Data = &AudioControlData;
    AudioControlBuffer.ElementCount = bytes;
    AudioControlBuffer.ElementWidth = 1;
    AudioControlBuffer.CallbackParameter = &AudioControlBuffer;
    AudioControlBuffer.ProcessedElementCount = 0;
    AudioControlBuffer.ProcessedFlag = 0;
    AudioControlBuffer.pNext = NULL;

    /* Set the the address to endpoint zero */
    AudioControlBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = EP_ZERO;

    /* Call the core function to send data */
	adi_usb_TransmitEpZeroBuffer((ADI_DEV_BUFFER*)&AudioControlBuffer);
};

/*********************************************************************
*
*    Function:       GetAudioControlData
*
*	 Get Audio control data from the host.
*
*********************************************************************/
__ADI_USB_AUDIO_SECTION_CODE
static void GetAudioControlData (u16 bytes)
{
    AUDIO_STREAMING_DEV_DATA   *pDevice;
    pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;

    AudioControlBuffer.Data = &AudioControlData;
    AudioControlBuffer.ElementCount = bytes;
    AudioControlBuffer.ElementWidth = 1;
    AudioControlBuffer.CallbackParameter = &AudioControlBuffer;
    AudioControlBuffer.ProcessedElementCount = 0;
    AudioControlBuffer.ProcessedFlag = 0;
    AudioControlBuffer.pNext = NULL;

    /* Set the the address to endpoint zero */
    AudioControlBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = EP_ZERO;

    /* Call the core function to get data */
	adi_usb_ReceiveEpZeroBuffer((ADI_DEV_BUFFER*)&AudioControlBuffer);
};

__ADI_USB_AUDIO_SECTION_CODE
static void ProvideFeedback(u32 feedbackValue)
{
    AUDIO_STREAMING_DEV_DATA   *pDevice;
    pDevice = (AUDIO_STREAMING_DEV_DATA *)&adi_usb_audio_def;

    /* Get the Feedback value */
    FeedbackControlData[0] = (feedbackValue & 0x0000ff);
    FeedbackControlData[1] = ((feedbackValue & 0x00ff00) >> 8);
    FeedbackControlData[2] = ((feedbackValue & 0xff0000) >> 16);

    /* Construct the buffer */
    FeedbackControlBuffer.Data = &FeedbackControlData;
    FeedbackControlBuffer.ElementCount = MAX_PACKET_SIZE_FEEDBACK;
    FeedbackControlBuffer.ElementWidth = 1;
    FeedbackControlBuffer.CallbackParameter = &FeedbackControlBuffer;
    FeedbackControlBuffer.ProcessedElementCount = 0;
    FeedbackControlBuffer.ProcessedFlag = 0;
    FeedbackControlBuffer.pNext = NULL;

    /* Set the FEEDBACK CONTROL Endpoint */
    FeedbackControlBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->audio_FB_EP;

    /* Write to the peripheral driver */
    adi_dev_Write(pDevice->PeripheralDevHandle,
    	          ADI_DEV_1D,
           	      (ADI_DEV_BUFFER *)&FeedbackControlBuffer);
}

static s16 HexToDbValue( s16 HexValue )
{
    s16 decibels;
    if (HexValue < 0)
    {
        decibels = (-1)*((-1)*HexValue/0x0100);
    }
    else {
        decibels = HexValue/0x0100;
    }
    return decibels;
}


static s16 DbToHexValue( s16 decibels )
{
    s16 HexValue;
    if (decibels<0) {
        HexValue = (decibels-1)*0x0100;
    }
    else {
        HexValue = decibels*0x0100;
    }
    return HexValue;
}

/**************************************************************************
 *
 * USB Audio Class driver entry point (Device Mode)
 *
 **************************************************************************/
__ADI_USB_AUDIO_SECTION_DATA
ADI_DEV_PDD_ENTRY_POINT ADI_USB_Device_AudioClass_Entrypoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

