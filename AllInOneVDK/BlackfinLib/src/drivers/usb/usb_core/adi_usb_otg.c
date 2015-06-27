/*******************************************************************************
 *
 *  Copyright(c) 2006 Analog Devices, Inc. All Rights Reserved.
 *
 * This software is proprietary and confidential.  By using this software you 
 * agree to the terms of the associated Analog Devices License Agreement.
 *
 * $RCSfile: adi_usb_otg.c,v $
 *
 * $Revision: 2095 $
 *
 * $Date: 2010-02-18 17:08:05 -0500 (Thu, 18 Feb 2010) $
 *
 *  Description:
 *             OTG Host functionality is implemented in this file
 *
 ******************************************************************************/

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_OTG_SECTION_CODE    section("adi_usb_otg_code")
#define __ADI_USB_OTG_SECTION_DATA    section("adi_usb_otg_data")
#else
#define __ADI_USB_OTG_SECTION_CODE
#define __ADI_USB_OTG_SECTION_DATA
#endif 

 
#include <drivers\usb\usb_core\adi_usb_objects.h>
#include <drivers\usb\usb_core\adi_usb_debug.h>
#include <drivers\usb\usb_core\adi_usb_core.h>
#include <string.h>
#include <drivers\usb\usb_core\adi_usb_logevent.h>
#include <drivers\adi_dev.h>
#include <cycle_count_bf.h>

/* configuration option */
//#define USB_OTG_PRINT_DESCRIPTOR

#ifdef _USB_MULTI_THREADED_
#include <kernel_abs.h>
#endif /* multithreaded applicaiton */

/* External variables */
extern USB_CORE_DATA *pUsbCoreData;

/* Device Enumeration Phase (DEP) states */
__ADI_USB_OTG_SECTION_DATA
typedef enum USB_OTG_DEP_STATES_
{
   DEP_IDLE,                    /* Enumeration is not started */
   DEP_GET_DEV_DESCRIPTOR_0,    /* Get device descriptor at Address 0, Endpoint 0 */
   DEP_SET_DEV_ADDRESS,         /* Set New address to the device Starts with  1*/
   DEP_GET_DEV_DESCRIPTOR_1,    /* Get device descriptor at Newly Set address, Endpoint 0 */
   DEP_GET_CONFIG_DESCRIPTOR_0, /* Get configuration descriptor alone - wLength in Setup Pkt 9 bytes */
   DEP_GET_CONFIG_DESCRIPTOR_1, /* Get full configuration descriptor(s) */
   DEP_GET_STRING_DESCRIPTOR,   /* Get all string descriptors */
   DEP_DEV_ENUMERATED,          /* Device is enumerated, i.e we got all data from device */
   DEP_DEV_CONFIGURED,          /* Client driver selected a configuration */
}USB_OTG_DEP_STATES;

/*
 * Default length of string
 */
#define USB_OTG_STRING_LEN 64
/*
 * String descriptors
 */
typedef struct UsbOtgStringInfo
{
  s32_t dwStringIndex;
  u8_t  StringData[USB_OTG_STRING_LEN];
}USB_OTG_STRING_INFO;

/* Usb Core OTG Host specific data */
typedef struct UsbOTGHostData
{
   SETUP_PACKET  *pSetupPkt;          /* Setup Packet used during device enumeration process */
   USB_OTG_DEP_STATES eDepState;      /* Device Enumeration Phase state */
   s32_t           dwDeviceAddress;   /* Device Address */
   PDEVICE_OBJECT  pRemoteDeviceObj;  /* Remote device object */
   s32_t           dwRemoteDeviceID;  /* Remote device ID */
   s32_t           dwDefaultConfigID; /* Default configuration ID */
   PCONFIG_OBJECT  pDefaultConfigObj; /* Default configuration object */
   USB_OTG_STRING_INFO StringInfo[USB_MAX_STRINGS]; /* String descriptor values stored based on index */
   s32_t            dwMaxStringIndex; /* Number of string descriptors */
   s32_t            dwLanguageID;     /* Language ID */
   ADI_USB_DEVICE_SPEED eDeviceSpeed; /* Device speed */
}USB_OTG_HOST_DATA;


#define UPDATE_STRING_INDEX(_str_index) do {                                                                        \
                                             s32_t _index = pUsbOtgHostData->dwMaxStringIndex;                      \
                                             pUsbOtgHostData->StringInfo[_index].dwStringIndex = (s32_t)_str_index; \
                                             pUsbOtgHostData->dwMaxStringIndex++;                                   \
                                           } while(0)


 /*  Converts the given unicode string to ascii string */
#define CONVERT_UNICODE_TO_ASCII(_ucode_str,_ascii_str,_ucode_str_len) \
        do                                                             \
        {                                                              \
           s32_t j;                                                    \
           u8_t  *p_dest = (u8_t*)_ascii_str;                          \
           u16_t *p_src  = (u16_t*)_ucode_str;                         \
           for(j=0;j < ((s32_t)_ucode_str_len);j+=2)                   \
           {                                                           \
              *p_dest = (u8_t)(*p_src & 0xff);                         \
               p_dest++; p_src++;                                      \
           }                                                           \
        }while(0)

__ADI_USB_OTG_SECTION_DATA
static USB_OTG_HOST_DATA  UsbOtgHostData = {0};
__ADI_USB_OTG_SECTION_DATA
static USB_OTG_HOST_DATA *pUsbOtgHostData = &UsbOtgHostData;
__ADI_USB_OTG_SECTION_DATA
static SETUP_PACKET       SetupPkt = {0};
void ResetInterfaceEndpoints(void);

/*******************************************************************************
 * EpZeroHostStateMachineHandler()
 *
 * Description:
 * USB OTG Host mode State machine handler.
 * 
 * EP_STATE_IDLE               - Idle state
 * EP_STATE_SETUP_RQST_PHASE   - Setup request has been sent
 * EP_STATE_SETUP_DATA_PHASE   - Receving or Sending Data
 * EP_STATE_SETUP_STATUS_PHASE - Status phase sends IN token or Zero length pkt
 *
 * @param AppHandle Application Callback handle
 * @param wEvent  Identifies the event
 * @param pArg Argument to the function
 *
 * @return 0 upon success, 1 if the state machine is in data phase
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
static int EpZeroHostStateMachineHandler(DEVICE_OBJECT *pDev,int dwInput, void *p)
{
USB_EP_INFO *pEpInfo = &pUsbCoreData->pDeviceHead->pEndpointZeroObj->EPInfo;
u32_t ReturnValue = 0x0;

    USB_ASSERT(pUsbCoreData->eDevMode != MODE_OTG_HOST);
    USB_ASSERT(pEpInfo == NULL);

    switch(pEpInfo->EpState)
    {
       /* 
        * configure to send setup packet 
        */
        case EP_STATE_IDLE:
        {
            PSETUP_PACKET pSetupPkt = (PSETUP_PACKET)p;
            pEpInfo->EpDir = (pSetupPkt->bmRequestType & USB_DIR_DEVICE_TO_HOST);
            pEpInfo->TransferSize = SETUP_PACKET_LEN;
            pDev->pSetupData      = (u8_t*)pSetupPkt;
            pEpInfo->EpNumTotalBytes = 0;
            pEpInfo->EpState = EP_STATE_SETUP_RQST_PHASE;
            pDev->pAppEpZeroBuffer = NULL;
        }
        break;

       /* 
        * setup request has been sent, prepare for data phase or status phase 
        */
        case EP_STATE_SETUP_RQST_PHASE:
        {
             pEpInfo->EpNumTotalBytes = 0;
            
             /* Issue I/O control to the driver to issue an IN token to receive data. */
             if ((pEpInfo->EpDir & USB_DIR_DEVICE_TO_HOST) && pEpInfo->TransferSize >0)
                adi_dev_Control(pUsbCoreData->pPddHandle, ADI_USB_CMD_OTG_REQ_IN_TOKEN, (void*)0);

             /* If input is 1 we move to status phase else data phase */
             pEpInfo->EpState = ((dwInput == 1) ? EP_STATE_SETUP_STATUS_PHASE :  EP_STATE_SETUP_DATA_PHASE );
        }
        break;

       /* 
        * send IN tokens to receive data or transmit data 
        */
        case EP_STATE_SETUP_DATA_PHASE:
        {
             ADI_DEV_1D_BUFFER *pDataBuffer = (ADI_DEV_1D_BUFFER*)p;

             /* transfer complete, move to status phase */
             if(pEpInfo->EpNumTotalBytes >= pEpInfo->TransferSize)
             {
                 pEpInfo->EpState = EP_STATE_SETUP_STATUS_PHASE;
                 ReturnValue = 0x1;
                 break;
             }

             /* host is receiving data from device */
             if(pEpInfo->EpDir == USB_DIR_DEVICE_TO_HOST)
             {                
                 USB_ASSERT(pDataBuffer == NULL);

                 memcpy((u8_t*)pDev->pSetupData + pEpInfo->EpNumTotalBytes,
                         pDataBuffer->Data,
                         pDataBuffer->ProcessedElementCount);

                 /* increment the number of total bytes received so far */
                  pEpInfo->EpNumTotalBytes += pDataBuffer->ProcessedElementCount;

                 /* have to receive mode data, send IN tokens */
                 if(pEpInfo->EpNumTotalBytes < pEpInfo->TransferSize) 
                 {
                    adi_dev_Control(pUsbCoreData->pPddHandle, ADI_USB_CMD_OTG_REQ_IN_TOKEN, (void*)0);
                    ReturnValue = 0x0;
                 }
                 else
                    ReturnValue = 0x1;
             }
             else /* host is transmitting data */
             {
                 ReturnValue = SendEpZeroDataEx(pDev,pDev->pSetupData,1);
             }
        }
        break;

       /*
        * status phase, end the ep zero transactiob by sending IN token or Zero
        * length packet
        */
        case EP_STATE_SETUP_STATUS_PHASE:
        {
             if ((pEpInfo->EpDir & USB_DIR_DEVICE_TO_HOST) && pEpInfo->TransferSize > 0)
                 adi_dev_Control(pUsbCoreData->pPddHandle, ADI_USB_CMD_OTG_SEND_ZERO_LEN_PKT, (void*)0);
             else
                 adi_dev_Control(pUsbCoreData->pPddHandle, ADI_USB_CMD_OTG_REQ_IN_TOKEN, (void*)0);

             pEpInfo->EpState = EP_STATE_IDLE;
        }
        break;
    }
    
    return(ReturnValue);
}

/*******************************************************************************
 * ParseConfigurationMemory()
 *
 * Description:
 * Parses the configuration memory and constructs the objects associates with 
 * the device object. If they are configuration, interface,endpoint specific 
 * descriptors were present this function will place them in the allocated 
 * memory segment.
 *
 * @pConfigMemoryArea pointer to the configuration descriptor memory
 * @dwConfigMemoryLen total configuration length
 * @dwRemoteDeviceID  remote device object ID
 *
 * @return 1 upon complete process, 0 upon partial process -1 upon failure.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
int ParseConfigurationMemory(u8_t *pConfigMemoryArea,
		             const s32_t dwConfigMemoryLen, 
			     const s32_t dwRemoteDeviceID)
{
PCONFIG_OBJECT            pCfgO;
PINTERFACE_OBJECT         pIfceO;
PENDPOINT_OBJECT          pEpO;
LOGICAL_EP_INFO           LogicalEpInfo ={0};
PUSB_EP_INFO              pEpInfo=NULL;
u8_t   *pCurDesc=NULL;
s32_t  dwCurDescType,dwCurConfigID,dwCurInterfaceID,dwEpID;
s32_t  dwCurDescLen,dwBytesProcessed=0;
u32_t uInterruptStatus = cli();

   USB_ASSERT(pConfigMemoryArea == NULL);
   USB_ASSERT(dwConfigMemoryLen < CONFIGURATION_DESCRIPTOR_LEN);

   pCurDesc =  pConfigMemoryArea;

   while(dwBytesProcessed < dwConfigMemoryLen)
   {
      /* First byte of any descriptor is descriptor length */
      dwCurDescLen  = (s32_t)(*pCurDesc & 0xff);
      dwCurDescType = (s32_t) (*(pCurDesc+1) & 0xff);

      switch(dwCurDescType)
      {
         case TYPE_CONFIGURATION_DESCRIPTOR:
         {
             if((*(pCurDesc+5) & 0xff) == 0x01)
             {
                 dwCurConfigID = 0x01;
                 pCfgO = pUsbOtgHostData->pDefaultConfigObj;
             }
             else
             {
               /* We have more than one configuration for this device */
                 dwCurConfigID = adi_usb_CreateConfiguration(&pCfgO);
                 USB_ASSERT(dwCurConfigID == -1);
             }
             memcpy(pCfgO->pConfigDesc,pCurDesc,CONFIGURATION_DESCRIPTOR_LEN);

	     if(pCfgO->pConfigDesc->bIConfiguration > 0 ) 
             {
                UPDATE_STRING_INDEX((s32_t)pCfgO->pConfigDesc->bIConfiguration);
             }

            /* check if we have configuration specific descriptor */
             if(dwCurDescLen > CONFIGURATION_DESCRIPTOR_LEN)
             {
                 /* TODO: we have configuration specific descriptor create configuration specific
                  * object and copy the extra data
                  */
             }

             /* Attach configuraiton object to the remote device object */
             adi_usb_AttachConfiguration(dwRemoteDeviceID,dwCurConfigID);
         }
         break;

         case TYPE_INTERFACE_DESCRIPTOR:
         {
             dwCurInterfaceID = adi_usb_CreateInterface(&pIfceO);

             USB_ASSERT(dwCurInterfaceID == -1);
             memcpy(pIfceO->pInterfaceDesc,pCurDesc,INTERFACE_DESCRIPTOR_LEN);

	     if(pIfceO->pInterfaceDesc->bIInterface >0 ) 
	     {
                UPDATE_STRING_INDEX((s32_t)pIfceO->pInterfaceDesc->bIInterface);
	     }
             /* check if we have configuration specific descriptor */
             if(dwCurDescLen > INTERFACE_DESCRIPTOR_LEN)
             {
                  /* TODO: we have interface specific descriptor, copy it to the
                   * class specific object
                   **/
             }

             /* attach the interface to the current configuraiton */
             adi_usb_AttachInterface(dwCurConfigID,dwCurInterfaceID);
         }
         break;

	 case TYPE_ENDPOINT_DESCRIPTOR:
	 {

	     LogicalEpInfo.dwMaxEndpointSize =  ( ((*(pCurDesc+5)&0xff) << 8)  | ((*(pCurDesc+4)&0xff)) );
             dwEpID = adi_usb_CreateEndPoint(&pEpO,&LogicalEpInfo);

	     USB_ASSERT(dwEpID == -1);
	     memcpy(pEpO->pEndpointDesc,pCurDesc,ENDPOINT_DESCRIPTOR_LEN);

	     pEpInfo = &pEpO->EPInfo;

	    /* Set up the default Epzero callback for the data endpoints as well. class drivers
	     * may overwrite the data endpoint callbacks.
	     */
	      pEpInfo->EpCallback = pUsbOtgHostData->pDefaultConfigObj->EpZeroCallback;

	     /* check if we have configuration specific descriptor */
	     if(dwCurDescLen > ENDPOINT_DESCRIPTOR_LEN)
	     {
              /* TODO: we have interface specific descriptor, copy it to the
               * class specific object
               **/
	     }

	      /* attach the interface to the current configuraiton */
	      adi_usb_AttachEndpoint(dwCurInterfaceID,dwEpID);
	   }
	   break;

	   case TYPE_OTG_DESCRIPTOR:
	   /* TODO: copy the OTG descriptor and make it avaialble to the class driver */
	   break;

	   default:
	   break;
      }

      dwBytesProcessed += dwCurDescLen;
      /* goto next descriptor */
      pCurDesc += dwCurDescLen;
   }
   sti(uInterruptStatus);

   return(1);
}

/*******************************************************************************
 * GetStringDescriptor()
 *
 * Description:
 *
 * GetStringDescriptor,String descriptor index is specified in the 
 * USB_OTG_STRING_INFO structure.
 *
 * @param pUsbOtgHostData points to the USB_OTG_STRING_INFO structure.
 * @return returns 1 upon success.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
static s32_t GetOtgStringDescriptor(USB_OTG_STRING_INFO *pUsbStringInfo)
{
USB_EP_INFO *pEpInfo = &pUsbCoreData->pDeviceHead->pEndpointZeroObj->EPInfo;
s32_t ret;
PDEVICE_OBJECT pDevO = pUsbCoreData->pDeviceHead;

    memset(pUsbOtgHostData->pSetupPkt,0,sizeof(SETUP_PACKET));
    pUsbOtgHostData->pSetupPkt->bmRequestType =  USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD |
 						   USB_RECIPIENT_DEVICE;
    pUsbOtgHostData->pSetupPkt->bRequest =  USB_STD_RQST_GET_DESCRIPTOR;
    pUsbOtgHostData->pSetupPkt->wValue   =  ( ((TYPE_STRING_DESCRIPTOR << 8) & 0xffff) | (pUsbStringInfo->dwStringIndex & 0xff) );
    pUsbOtgHostData->pSetupPkt->wIndex   =  ((pUsbOtgHostData->dwLanguageID >> 16) & 0xffff);
    pUsbOtgHostData->pSetupPkt->wLength  =  255;
    pEpInfo->EpState                     = EP_STATE_IDLE;

   /* Puts the EpZero endpoint state machine in EP_STATE_SETUP_RQST_PHASE */
    pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
    ret = SendEpZeroDataEx(pUsbCoreData->pDeviceHead,(char*)pUsbOtgHostData->pSetupPkt,SETUP_PACKET_LEN);

    pDevO->pSetupData  = pDevO->pSetupDataArea;
    pEpInfo->TransferSize = 255;
    pUsbOtgHostData->eDepState = DEP_GET_STRING_DESCRIPTOR;

    /* Move from SETUP RQST phase to data phase. Send IN token in case you are expecting data */
    pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);

    USB_LOG_EVENT(HSD(GET_DESCRIPTOR),pUsbOtgHostData->pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-GET_STRING_DESCRIPTOR");
    return(ret);
}

/*******************************************************************************
 *
 * EnumDeviceStateMachine()
 *
 * Description:
 * 
 * Host's Device Enumeration State machine. Each of the steps in 
 * EnumDeviceStateMachine goes through EpZeroHostStateMachineHandler.
 *
 * @param AppHandle Application Callback handle
 * @param wEvent  Identifies the event
 * @param pArg Argument to the function
 *
 * @return Upon sucess returns 1.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
static s32_t EnumDeviceStateMachine(void *AppHandle,unsigned int wEvent,void *pArg)
{
s32_t ret;
USB_EP_INFO *pEpInfo = &pUsbCoreData->pDeviceHead->pEndpointZeroObj->EPInfo;
ADI_DEV_1D_BUFFER *pBuffer = (ADI_DEV_1D_BUFFER*)pArg;
DEVICE_DESCRIPTOR *pDevD;
CONFIGURATION_DESCRIPTOR *pConfigD;
s32_t dwTotalConfigurationLength=0,dwDeviceSpeed;
static int dwCurrentStringIndex;
PDEVICE_OBJECT pDevO = pUsbCoreData->pDeviceHead;

      USB_ASSERT(pUsbCoreData->eDevMode != MODE_OTG_HOST);

      /* In data phase we will not change the DEP state until we finish the transfer completely */
      if((pEpInfo->EpState == EP_STATE_SETUP_DATA_PHASE) && 
         (pUsbOtgHostData->eDepState != DEP_GET_STRING_DESCRIPTOR))
      {
          if(!pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pArg))
	  {
    	      return 0;
          }
      }

      /* class specific request for read */
      if(pDevO->pAppEpZeroBuffer != NULL && pUsbOtgHostData->eDepState == DEP_DEV_CONFIGURED)
      {
      	
     	  pUsbCoreData->RxCompleteHandler(AppHandle,ADI_USB_EVENT_RX_COMPLETE,pDevO->pAppEpZeroBuffer);
		/* move from data to status phase */
          pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pArg);
          pDevO->pAppEpZeroBuffer = NULL;
	  	  /* move from status to IDLE */
          pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pArg);
  	      pEpInfo->EpState  = EP_STATE_IDLE;
  	      return (0);
      }

      /* Step:1 With Vbus interrupt send SETUP_PKT with GET_DESCRIPTOR request at Address 0 Endpoint 0 */
      switch(pUsbOtgHostData->eDepState)
      {
         /*
          * We have to start the device enumeration by sending setup packet for the device descriptor
          * at address:0 endpoint:0
          *
          */
          case DEP_IDLE:
          {
              USB_ASSERT(pUsbCoreData->pPddHandle == NULL);
              adi_dev_Control(pUsbCoreData->pPddHandle, ADI_USB_CMD_GET_DEV_SPEED, (void*)&dwDeviceSpeed);
              pUsbOtgHostData->eDeviceSpeed = (ADI_USB_DEVICE_SPEED)dwDeviceSpeed;

              pUsbOtgHostData->dwMaxStringIndex  = 0;
              pUsbOtgHostData->pSetupPkt = &SetupPkt;
              memset(pUsbOtgHostData->pSetupPkt,0,sizeof(SETUP_PACKET));
              pUsbOtgHostData->pSetupPkt->bmRequestType =  USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD |
			                                           USB_RECIPIENT_DEVICE;
              pUsbOtgHostData->pSetupPkt->bRequest =  USB_STD_RQST_GET_DESCRIPTOR;
              pUsbOtgHostData->pSetupPkt->wValue   =  ((TYPE_DEVICE_DESCRIPTOR << 8) & 0xffff);
              pUsbOtgHostData->pSetupPkt->wIndex   =  0x0000;
              pUsbOtgHostData->pSetupPkt->wLength  =  DEVICE_DESCRIPTOR_LEN; /* size of device descriptor 18 bytes */
              pEpInfo->EpState                     =  EP_STATE_IDLE;

              /* Puts the EpZero endpoint state machine in EP_STATE_SETUP_RQST_PHASE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
              ret = SendEpZeroDataEx(pDevO,(char*)pUsbOtgHostData->pSetupPkt,SETUP_PACKET_LEN);

              pUsbOtgHostData->eDepState = DEP_GET_DEV_DESCRIPTOR_0;
              /* Switching to data phase configure setup paramaters accordingly */
              pDevO->pSetupData     = pDevO->pSetupDataArea;
              pEpInfo->TransferSize = DEVICE_DESCRIPTOR_LEN;
	      
              /* Move from SETUP RQST phase to data phase. Send IN token in case you are expecting data */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
              USB_LOG_EVENT(HSD(GET_DESCRIPTOR),pUsbOtgHostData->pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-GET_DESCRIPTOR:ADDR:0 EP:0");
          }
          break;
         /*
          * We will get into this stage once we have sent the device descriptor request. We got the response
          * back from the device and the buffer contains the device descriptor.
          */
          case DEP_GET_DEV_DESCRIPTOR_0:
          {
              USB_ASSERT(pBuffer == NULL);
              pDevD = (PDEVICE_DESCRIPTOR)pDevO->pSetupData;
              USB_ASSERT(pDevD->bDescriptorType != TYPE_DEVICE_DESCRIPTOR);

              /* Request has been sent now move from Data phase to Status phase */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);

              /* move from Status phase to IDLE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);

              pUsbOtgHostData->pSetupPkt = &SetupPkt;
              memset(pUsbOtgHostData->pSetupPkt,0,sizeof(SETUP_PACKET));
              pUsbOtgHostData->pSetupPkt->bmRequestType =  USB_DIR_HOST_TO_DEVICE | USB_TYPE_STANDARD |
			                                           USB_RECIPIENT_DEVICE;
              pUsbOtgHostData->pSetupPkt->bRequest =  USB_STD_RQST_SET_ADDRESS;
              pUsbOtgHostData->pSetupPkt->wValue   =  ((pUsbOtgHostData->dwDeviceAddress) & 0xffff);
              pUsbOtgHostData->pSetupPkt->wIndex   =  0x0000;
              pUsbOtgHostData->pSetupPkt->wLength  =  0x0;
              pEpInfo->EpState                     =  EP_STATE_IDLE;

             /* Puts the EpZero endpoint state machine in EP_STATE_SETUP_RQST_PHASE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
              ret = SendEpZeroDataEx(pDevO,(char*)pUsbOtgHostData->pSetupPkt,SETUP_PACKET_LEN);
              pUsbOtgHostData->eDepState = DEP_SET_DEV_ADDRESS;

             /* Request has been sent now move to status phase */
              pEpInfo->EpStateMachineHandler(pDevO,1,(void*)pUsbOtgHostData->pSetupPkt);

             /* Move status phase to IDLE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
              adi_dev_Control(pUsbCoreData->pPddHandle,ADI_USB_CMD_SET_DEV_ADDRESS,(void*)pUsbOtgHostData->pSetupPkt->wValue);
              USB_LOG_EVENT(HSD(SET_ADDRESS),pUsbOtgHostData->pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-SET_ADDRESS");
         }
         /* case fall through next */
         /*
          * We give a new address to the device and issue y i/o control  to set the FAddr.
          * By default FAddr register should be configured to 0.
          */
          case DEP_SET_DEV_ADDRESS:
          {

              pUsbOtgHostData->pSetupPkt = &SetupPkt;
              memset(pUsbOtgHostData->pSetupPkt,0,sizeof(SETUP_PACKET));
              pUsbOtgHostData->pSetupPkt->bmRequestType =  USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD |
			                                           USB_RECIPIENT_DEVICE;
              pUsbOtgHostData->pSetupPkt->bRequest =  USB_STD_RQST_GET_DESCRIPTOR;
              pUsbOtgHostData->pSetupPkt->wValue   =  ((TYPE_DEVICE_DESCRIPTOR << 8) & 0xffff);
              pUsbOtgHostData->pSetupPkt->wIndex   =  0x0000;
              pUsbOtgHostData->pSetupPkt->wLength  =  DEVICE_DESCRIPTOR_LEN; /* size of device descriptor */
              pEpInfo->EpState                     =  EP_STATE_IDLE;

              /* Puts the EpZero endpoint state machine in EP_STATE_SETUP_RQST_PHASE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
              ret = SendEpZeroDataEx(pDevO,(char*)pUsbOtgHostData->pSetupPkt,SETUP_PACKET_LEN);

              pUsbOtgHostData->eDepState = DEP_GET_DEV_DESCRIPTOR_1;
              /* Switching to data phase, configure setup parameters accordingly */
              pDevO->pSetupData     = pDevO->pSetupDataArea;
              pEpInfo->TransferSize = DEVICE_DESCRIPTOR_LEN;

              /* Move from SETUP RQST phase to data phase. Send IN token in case you are expecting data */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
              USB_LOG_EVENT(HSD(GET_DESCRIPTOR),pUsbOtgHostData->pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-GET_DESCRIPTOR");
          }
          break;

         /*
          * We issued Get Device descrioptor request already. By the time we come here buffer should have
          * the obtained device descriptor.
          */
          case DEP_GET_DEV_DESCRIPTOR_1:
          {
              USB_ASSERT(pBuffer == NULL);
              pDevD = (PDEVICE_DESCRIPTOR)pDevO->pSetupData;

              USB_ASSERT(pDevD->bDescriptorType != TYPE_DEVICE_DESCRIPTOR);
              USB_ASSERT(pUsbOtgHostData->pRemoteDeviceObj == NULL);

              /* Save any valid string descriptor indexes for later retrieval */
              if(pDevD->bIManufacturer > 0) { UPDATE_STRING_INDEX((s32_t)pDevD->bIManufacturer); };
              if(pDevD->bIProduct > 0)      { UPDATE_STRING_INDEX((s32_t)pDevD->bIProduct); };
              if(pDevD->bISerialNumber > 0) { UPDATE_STRING_INDEX((s32_t)pDevD->bISerialNumber); };

              memcpy(pUsbOtgHostData->pRemoteDeviceObj->pDeviceDesc,pBuffer->Data,DEVICE_DESCRIPTOR_LEN);

              #ifdef USB_OTG_PRINT_DESCRIPTOR
              adi_usb_PrintDeviceDescriptor(pUsbOtgHostData->pRemoteDeviceObj);
              #endif /* USB_OTG_PRINT_DESCRIPTOR */

              /* Request has been sent now move from Data phase to Status phase */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);
              /* move from data phase to IDLE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);

              memset(pUsbOtgHostData->pSetupPkt,0,sizeof(SETUP_PACKET));
              pUsbOtgHostData->pSetupPkt->bmRequestType =  USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD |
			                                           USB_RECIPIENT_DEVICE;
              pUsbOtgHostData->pSetupPkt->bRequest =  USB_STD_RQST_GET_DESCRIPTOR;
              pUsbOtgHostData->pSetupPkt->wValue   =  ((TYPE_CONFIGURATION_DESCRIPTOR << 8) & 0xffff);
              pUsbOtgHostData->pSetupPkt->wIndex   =  0x0000;
              pUsbOtgHostData->pSetupPkt->wLength  =  CONFIGURATION_DESCRIPTOR_LEN;
              pEpInfo->EpState                     = EP_STATE_IDLE;

              /* Puts the EpZero endpoint state machine in EP_STATE_SETUP_RQST_PHASE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
              ret = SendEpZeroDataEx(pDevO,(char*)pUsbOtgHostData->pSetupPkt,SETUP_PACKET_LEN);

              pUsbOtgHostData->eDepState = DEP_GET_CONFIG_DESCRIPTOR_0;
              pDevO->pSetupData      = pDevO->pSetupDataArea;
              pEpInfo->TransferSize  = CONFIGURATION_DESCRIPTOR_LEN;

              /* Move from SETUP RQST phase to data phase. Send IN token in case you are expecting data */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
              USB_LOG_EVENT(HSD(GET_CONFIGURATION),pUsbOtgHostData->pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-GET_CONFIGURATION_DESCRIPTOR");
          }
          break;

         /*
          * We get configuration descriptor alone - only 9 bytes. wTotalLength field in
          * the configuraiton descriptor will specify the total configuraiton size in bytes
          * which includes the configuraiton, interface and endpoint descriptors.
          */
          case DEP_GET_CONFIG_DESCRIPTOR_0:
          {
              USB_ASSERT(pBuffer == NULL);
              pConfigD = (PCONFIGURATION_DESCRIPTOR)pDevO->pSetupData;
              USB_ASSERT(pConfigD->bDescriptorType != TYPE_CONFIGURATION_DESCRIPTOR);

              /* Request has been sent now move to Status phase */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);
              /* move from data phase to IDLE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);

              dwTotalConfigurationLength = pConfigD->wTotalLength;

              /* minimum length of configuration descriptor is 9 */
              USB_ASSERT(dwTotalConfigurationLength <= CONFIGURATION_DESCRIPTOR_LEN);

              memset(pUsbOtgHostData->pSetupPkt,0,sizeof(SETUP_PACKET));
              pUsbOtgHostData->pSetupPkt->bmRequestType =  USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD |
			                                           USB_RECIPIENT_DEVICE;
              pUsbOtgHostData->pSetupPkt->bRequest =  USB_STD_RQST_GET_DESCRIPTOR;
              pUsbOtgHostData->pSetupPkt->wValue   =  ((TYPE_CONFIGURATION_DESCRIPTOR << 8) & 0xffff);
              pUsbOtgHostData->pSetupPkt->wIndex   =  0x0000;
              pUsbOtgHostData->pSetupPkt->wLength  =  dwTotalConfigurationLength;
              pEpInfo->EpState                     = EP_STATE_IDLE;

              /* Puts the EpZero endpoint state machine in EP_STATE_SETUP_RQST_PHASE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
              ret = SendEpZeroDataEx(pDevO,(char*)pUsbOtgHostData->pSetupPkt,SETUP_PACKET_LEN);

              pUsbOtgHostData->eDepState = DEP_GET_CONFIG_DESCRIPTOR_1;
              pDevO->pSetupData          = pDevO->pSetupDataArea;
              pEpInfo->TransferSize = dwTotalConfigurationLength;

              /* Move from SETUP RQST phase to data phase. Send IN token in case you are expecting data */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
              USB_LOG_EVENT(HSD(GET_CONFIGURATION),pUsbOtgHostData->pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-GET_CONFIGURATION");
          }
          break;

         /*
          * Get configuration 1 gets the entire configuraiton including all interface, and endpoint
          * descriptors for the configuration.
          */
          case DEP_GET_CONFIG_DESCRIPTOR_1:
          {
              USB_ASSERT(pEpInfo->TransferSize == 0);

              /* Request has been sent now move to Status phase */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);
              /* move from data phase to IDLE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);

              /* entire configuratio is present in SetupData */
              memcpy(pUsbCoreData->pConfigMemoryArea,pDevO->pSetupData,pEpInfo->TransferSize);

              /* parse configuration */
              ParseConfigurationMemory(pUsbCoreData->pConfigMemoryArea,
                                       pEpInfo->TransferSize,
                                       pUsbOtgHostData->dwRemoteDeviceID);


              memset(pUsbOtgHostData->pSetupPkt,0,sizeof(SETUP_PACKET));
              pUsbOtgHostData->pSetupPkt->bmRequestType =  USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD |
			                                           USB_RECIPIENT_DEVICE;
              pUsbOtgHostData->pSetupPkt->bRequest =  USB_STD_RQST_GET_DESCRIPTOR;
              pUsbOtgHostData->pSetupPkt->wValue   =  ((TYPE_STRING_DESCRIPTOR << 8) & 0xffff);

              /* Index 0 implies we are getting the default language descriptor 0409 - English - United States
               * For more information about language descriptors
               * Ref: http://www.usb.org/developers/docs/USB_LANGIDs.pdf
               */
              pUsbOtgHostData->pSetupPkt->wIndex   =  0x0000;
              pUsbOtgHostData->pSetupPkt->wLength  =  255;
              pEpInfo->EpState                     = EP_STATE_IDLE;

              /* Puts the EpZero endpoint state machine in EP_STATE_SETUP_RQST_PHASE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);

              ret = SendEpZeroDataEx(pDevO,(char*)pUsbOtgHostData->pSetupPkt,SETUP_PACKET_LEN);

              pUsbOtgHostData->eDepState = DEP_GET_STRING_DESCRIPTOR;
              pDevO->pSetupData          = pDevO->pSetupDataArea;
              pEpInfo->TransferSize = 0x4;
              /* Move from SETUP RQST phase to data phase. Send IN token in case you are expecting data */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);

              /* Set the current string index to zero */
              dwCurrentStringIndex =0;
              USB_LOG_EVENT(HSD(GET_DESCRIPTOR),pUsbOtgHostData->pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-GET_LANGID");
          }
          break;

          /*
           * Get all string descriptors. String descriptor indexs are stored from the previous get
           * device, configuraiton descriptor data.We stay in this state until we get all string
           * descriptors.
           */
           case DEP_GET_STRING_DESCRIPTOR:
           {
              USB_ASSERT(pBuffer == NULL);

              pEpInfo->EpNumTotalBytes = pEpInfo->TransferSize = *((u8_t*)pBuffer->Data);

              /* Request has been sent now move to Status phase */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);
              /* move from data phase to IDLE */
              pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);

              /* We got the LANGID */
              if(!dwCurrentStringIndex)
              {
                 /* copy the language ID */
                 memcpy((char*)&pUsbOtgHostData->dwLanguageID,(char*)pBuffer->Data,4);
                 if(pUsbOtgHostData->dwMaxStringIndex > 0)
                 {
                     GetOtgStringDescriptor(&pUsbOtgHostData->StringInfo[dwCurrentStringIndex]);
                     dwCurrentStringIndex++;
                     break;
                 }
              }
              else if(dwCurrentStringIndex <= pUsbOtgHostData->dwMaxStringIndex)
              {
                 USB_ASSERT(pBuffer == NULL);
                 /* The length of the incoming string descriptor is larger than the default string length */
                 USB_ASSERT(pBuffer->ProcessedElementCount > USB_OTG_STRING_LEN);

                /* we have a string descriptor to process */
                 memcpy((u8_t*)&pUsbOtgHostData->StringInfo[dwCurrentStringIndex-1].StringData,(u8_t*)pBuffer->Data,pBuffer->ProcessedElementCount);

                 if(dwCurrentStringIndex < pUsbOtgHostData->dwMaxStringIndex)
                 {
                     GetOtgStringDescriptor(&pUsbOtgHostData->StringInfo[dwCurrentStringIndex]);
                     dwCurrentStringIndex++;
                     break;
                 }
              }
              pUsbOtgHostData->eDepState = DEP_DEV_ENUMERATED;
              pEpInfo->EpState = EP_STATE_IDLE;
	   }
           /*break; */
           /* FALL THROUGH */

          /*
           * We received all the required descriptors from the device. Now we invoke the class drivers
           * EpZero callback to let know that device has been enumerated. Enumerated data is supplied
           * via an i/o control. Class driver selects one of the configuraitons and issues SET_CONFIGURATION.
           */
           case DEP_DEV_ENUMERATED:
           {
                (pUsbOtgHostData->pDefaultConfigObj->EpZeroCallback)(AppHandle,
								     ADI_USB_OTG_EVENT_ENUMERATION_COMPLETE,
								     (void*)pUsbOtgHostData->pRemoteDeviceObj);
           }
           break;

           default:
           break;
    }

    return(0);
}

/*******************************************************************************
 *
 * OtgHostDisconnect()
 *
 * Description:
 *
 * Resets OTG host data structures when disconnect is detected. 
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
static u32_t OtgHostDisconnect(void)
{
PCONFIG_OBJECT pCfgO;

     /* Reset previouly attached configration object
      * TODO: use detach configuration
      **/
      pUsbOtgHostData->pRemoteDeviceObj->pConfigObj = NULL;

      /* Get the default configuration object */
      pCfgO = pUsbOtgHostData->pDefaultConfigObj;

      /* Reset the configuration descriptor */
      memset(pCfgO->pConfigDesc,0,sizeof(CONFIGURATION_DESCRIPTOR));

      /* Reset the interface object */
      pCfgO->pInterfaceObj = NULL;

      /* Reset all interface and endpoint objects */
      ResetInterfaceEndpoints();

      /* reset the DEP state */
      pUsbOtgHostData->eDepState = DEP_IDLE;

      /* reset the EP state */
      pUsbCoreData->pDeviceHead->pEndpointZeroObj->EPInfo.EpState = EP_STATE_IDLE;

      return(1);
}

/*******************************************************************************
 * HostBusEventHandler()
 * 
 * Description:
 *
 * Event handler in case of USB peripheral is acting as an OTG host. The handler
 * approriately initiates the peripheral enumeration process.
 *
 * @param AppHandle Application Callback handle
 * @param wEvent  Identifies the event
 * @param pArg Argument to the function
 *
 * @return Upon sucess returns 1.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
static s32_t HostBusEventHandler(void *AppHandle,unsigned int wEvent, void *pArg)
{
DEVICE_OBJECT       *pCurDeviceO = pUsbCoreData->pDeviceHead;  /* head should be set to point active device */
USB_EP_INFO         *pEpInfo = &pCurDeviceO->pEndpointZeroObj->EPInfo; /* Endpoint zero object */


        USB_ASSERT(pUsbCoreData->eDevMode != MODE_OTG_HOST);

        switch(wEvent)
        {
                case ADI_USB_EVENT_START_OF_FRAME:
                case ADI_USB_EVENT_ROOT_PORT_RESET:
                case ADI_USB_EVENT_RESUME:
                case ADI_USB_EVENT_SUSPEND:
                case ADI_USB_EVENT_VBUS_TRUE:
                case ADI_USB_EVENT_VBUS_FALSE:
                    if(pUsbOtgHostData->pDefaultConfigObj->EpZeroCallback != NULL) 
                    {
                        (pUsbOtgHostData->pDefaultConfigObj->EpZeroCallback)(AppHandle,wEvent,pArg);
                    }
                    break;
                case ADI_USB_EVENT_DISCONNECT:
                    /* First Disconnect OTG from Host. This allows the class driver to 
                       re-set things in its callback
                    */
                    OtgHostDisconnect();
                    /* Call the EP zero callback function (in class driver)
                    */
                    if(pUsbOtgHostData->pDefaultConfigObj->EpZeroCallback != NULL)
                    {
                        (pUsbOtgHostData->pDefaultConfigObj->EpZeroCallback)(AppHandle,wEvent,pArg);
                    }
            		break;
                default:
                break;

        }
        return(1);
}

/*******************************************************************************
 * HostTxCompleteHandler()
 * 
 * Description:
 *
 * Host Tx complete handler.
 *
 * @param AppHandle Application Callback handle
 * @param wEvent  Identifies the event
 * @param pArg Argument to the function
 *
 * @return Upon sucess returns 1.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
static s32_t HostTxCompleteHandler(void *AppHandle,unsigned int wEvent,void *pArg)
{
//PDEVICE_OBJECT pDevO = pUsbOtgHostData->pRemoteDeviceObj;
PDEVICE_OBJECT pDevO = pUsbCoreData->pDeviceHead;
ADI_DEV_1D_BUFFER *b = (ADI_DEV_1D_BUFFER*)pArg,*p;
ADI_DCB_CALLBACK_FN  epZeroCallback = pUsbOtgHostData->pRemoteDeviceObj->pActiveConfigObj->EpZeroCallback;
EP_ZERO_BUFFER_INFO *pInf= (EP_ZERO_BUFFER_INFO*)b->pAdditionalInfo;
u32_t uInterruptStatus;
USB_EP_INFO         *pEpInfo = &pDevO->pEndpointZeroObj->EPInfo; /* Endpoint zero object */

    USB_ASSERT(pUsbCoreData->eDevMode != MODE_OTG_HOST);

    /* Check if core has initiated the transfer */
    if(pInf->wCoreControlCode != CORE_CONTROL_CODE)
	{
        /* call currently active configuraions's EP zero callback */
        //USB_ASSERT(pDevO->pActiveConfigObj == NULL);
        USB_ASSERT(epZeroCallback == NULL);
        //pDevO->pActiveConfigObj->EpZeroCallback(AppHandle,wEvent,pArg);
        epZeroCallback(AppHandle,wEvent,pArg);
        return(0);
	}

    uInterruptStatus = cli();
    p = (ADI_DEV_1D_BUFFER*)pDevO->pEndpointZeroObj->EPInfo.pFreeXmtList;

    /* Reset all values of pBuffer entities */
    b->ElementCount =  pInf->wMaxEpZeroBufferSize;
    b->CallbackParameter =b;
    b->ProcessedElementCount =0;
    b->ProcessedFlag =0;
    b->pNext = NULL;

    if(!p){
        pDevO->pEndpointZeroObj->EPInfo.pFreeXmtList = (ADI_DEV_BUFFER*)b;
    }
    else {
        while(p->pNext != NULL)
            p = p->pNext;
        p->pNext = b;
    }
    sti(uInterruptStatus);

    /* Tx completed we either send more data or move the state machine to status phase */
    if(pEpInfo->TransferSize > pEpInfo->EpNumTotalBytes) 
        pDevO->pEndpointZeroObj->EPInfo.EpStateMachineHandler(pDevO,1,pDevO->pSetupData);
    else if(pDevO->pAppEpZeroBuffer != NULL)
    {
        /*  we are in request phase means just sent the SETUP_PKT return it */
        if(pEpInfo->EpState == EP_STATE_SETUP_RQST_PHASE)
        {
           //pDevO->pActiveConfigObj->EpZeroCallback(AppHandle,wEvent,(void*)pDevO->pAppEpZeroBuffer);
           /* move to data phase */
           epZeroCallback(AppHandle,wEvent,(void*)pDevO->pAppEpZeroBuffer);
        }
        else /* we are in data phase */
        {
           USB_ASSERT(pDevO->pAppEpZeroBuffer == NULL);
           /* move to the status phase */	
           pDevO->pEndpointZeroObj->EPInfo.EpStateMachineHandler(pDevO,0,pDevO->pSetupData);
           /* return back the application buffer */
           //pDevO->pActiveConfigObj->EpZeroCallback(AppHandle,wEvent,(void*)pDevO->pAppEpZeroBuffer); 
	      epZeroCallback(AppHandle,wEvent,(void*)pDevO->pAppEpZeroBuffer);
           /* reset the buffer */
           pDevO->pAppEpZeroBuffer = NULL;
           pDevO->pEndpointZeroObj->EPInfo.EpStateMachineHandler(pDevO,0,pDevO->pSetupData);
        }
    }

    return(1);
}

/*******************************************************************************
 * HostRxCompleteHandler()
 *
 * Description:
 * Host Rx complete handler
 *
 * @param AppHandle Application Callback handle
 * @param wEvent  Identifies the event
 * @param pArg Argument to the function
 *
 * @return Upon sucess returns 1.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
static s32_t HostRxCompleteHandler(void *AppHandle,unsigned int wEvent,void *pArg)
{
PDEVICE_OBJECT pDevO = pUsbOtgHostData->pRemoteDeviceObj;
ADI_DEV_1D_BUFFER *b = (ADI_DEV_1D_BUFFER*)pArg,*p;
EP_ZERO_BUFFER_INFO *pInf= (EP_ZERO_BUFFER_INFO*)b->pAdditionalInfo;
u32_t uInterruptStatus;

    USB_ASSERT(pUsbCoreData->eDevMode != MODE_OTG_HOST);

    /* Check if core has initiated the transfer */
    if(pInf->wCoreControlCode != CORE_CONTROL_CODE)
    {
        /* call currently active configuraions's EP zero callback */
        USB_ASSERT(pDevO->pActiveConfigObj == NULL);
        pDevO->pActiveConfigObj->EpZeroCallback(AppHandle,wEvent,pArg);
    }
    return(0);
}

/*******************************************************************************
 * HostStandardHandler()
 *
 * Description:
 *
 * Standard Host handler. Starts the enumeration
 *
 * @param AppHandle Application Callback handle
 * @param wEvent  Identifies the event
 * @param pArg Argument to the function
 *
 * @return Upon sucess returns 1.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
static s32_t HostStandardHandler(void *AppHandle,unsigned int wEvent,void *pArg)
{
DEVICE_OBJECT  *pCurDeviceO = pUsbCoreData->pDeviceHead;  /* head should be set to point active device */
USB_EP_INFO    *pEpInfo = &pCurDeviceO->pEndpointZeroObj->EPInfo; /* Endpoint zero object */
ADI_DEV_1D_BUFFER *pBuffer = (ADI_DEV_1D_BUFFER*)pArg;

     USB_ASSERT(pUsbCoreData->eDevMode != MODE_OTG_HOST);

     /* Set Host EpZero State Machine handler */
     if(!pEpInfo->EpStateMachineHandler)
     {
         pEpInfo->EpStateMachineHandler = EpZeroHostStateMachineHandler;
         pEpInfo->EpState = EP_STATE_IDLE;
	 pUsbOtgHostData->dwDeviceAddress = 0x5;
     }

     if( (pUsbOtgHostData->eDepState != DEP_DEV_CONFIGURED) || (pCurDeviceO->pAppEpZeroBuffer != NULL))
     {
          /* Call the enumeration state machine */
          EnumDeviceStateMachine(AppHandle,wEvent,pArg);

          /* free the received packet */
          if(pBuffer != NULL){
         FreeReceivedPackets(pCurDeviceO,(ADI_DEV_BUFFER*)pBuffer);
         }

     }
     else if(pCurDeviceO->pAppEpZeroBuffer == NULL)
     {
     	  pUsbCoreData->RxCompleteHandler(AppHandle,ADI_USB_EVENT_RX_COMPLETE,pArg);
     }

     return(0);
}

/*******************************************************************************
 * CreateRemoteDeviceObjects()
 *
 * Description:
 *
 * Creates remote device object and default configuration object.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
static int CreateRemoteDeviceObjects(void)
{
     /* Create Device Object for the remote device i.e hard disc or flash drive */
     pUsbOtgHostData->pRemoteDeviceObj->ID = adi_usb_CreateDevice(&pUsbOtgHostData->pRemoteDeviceObj);
     pUsbOtgHostData->dwRemoteDeviceID = pUsbOtgHostData->pRemoteDeviceObj->ID;
     USB_ASSERT(pUsbOtgHostData->pRemoteDeviceObj->ID == -1);

     /* Create default configuration object */
     pUsbOtgHostData->pDefaultConfigObj->ID = adi_usb_CreateConfiguration(&pUsbOtgHostData->pDefaultConfigObj);
     pUsbOtgHostData->dwDefaultConfigID = pUsbOtgHostData->pDefaultConfigObj->ID;
     USB_ASSERT(pUsbOtgHostData->dwDefaultConfigID == -1);

     return(1);
}

/*******************************************************************************
 *
 * Sets the default Host handlers.
 *
 * @see adi_usb_SetDeviceMode
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
void SetHostHandlers(void)
{
     pUsbCoreData->StandardUsbRequestHandler = HostStandardHandler;
     pUsbCoreData->BusEventHandler           = HostBusEventHandler;
     pUsbCoreData->TxCompleteHandler         = HostTxCompleteHandler;
     pUsbCoreData->RxCompleteHandler         = HostRxCompleteHandler;
     pUsbOtgHostData->eDepState = DEP_IDLE; /* device is not enumerated yet */
}

/*******************************************************************************
 * adi_usb_otg_SetEpZeroCallback()
 * 
 * Description:
 *
 * Applications use this function to set the default ep zero callback function
 * this function will be called for various endpoint zero events.
 *
 * @param dwConfigID  configuration id 
 * @param CallBack    callback function
 *
 * @return ADI_USB_RESULT_SUCCESS upon success
 * 
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
s32_t adi_usb_otg_SetEpZeroCallback(const s32_t dwConfigID, ADI_DCB_CALLBACK_FN CallBack)
{
CONFIG_OBJECT *pConfigO;

    USB_ASSERT((dwConfigID != 1));
    USB_ASSERT(CallBack == NULL);

    if(pUsbOtgHostData->pRemoteDeviceObj == NULL)
	     CreateRemoteDeviceObjects();

    /* Currently supports only one configuration expand this function for multiple configurations */
    pConfigO =  pUsbOtgHostData->pDefaultConfigObj;

    pConfigO->EpZeroCallback = CallBack;

  return(ADI_USB_RESULT_SUCCESS);
}

/*******************************************************************************
 *
 * Sets the selected configuration.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
s32_t adi_usb_otg_SetConfiguration(const s32_t dwConfigID)
{
s32_t ret;
PCONFIG_OBJECT pConfigO;
USB_EP_INFO *pEpInfo = &pUsbCoreData->pDeviceHead->pEndpointZeroObj->EPInfo;
PDEVICE_OBJECT pDevO = pUsbCoreData->pDeviceHead;

    USB_ASSERT(dwConfigID < 0);
    adi_usb_GetObjectFromID(dwConfigID,USB_CONFIGURATION_OBJECT_TYPE,(void**)&pConfigO);
    USB_ASSERT(pConfigO == NULL);

    memset(pUsbOtgHostData->pSetupPkt,0,sizeof(SETUP_PACKET));
    pUsbOtgHostData->pSetupPkt->bmRequestType =  USB_DIR_HOST_TO_DEVICE | USB_TYPE_STANDARD |
			                                           USB_RECIPIENT_DEVICE;
    pUsbOtgHostData->pSetupPkt->bRequest =  USB_STD_RQST_SET_CONFIGURATION;
    pUsbOtgHostData->pSetupPkt->wValue   =  (dwConfigID & 0xff);
    pUsbOtgHostData->pSetupPkt->wIndex   =  0x0000;
    pUsbOtgHostData->pSetupPkt->wLength  =  0;
    pEpInfo->EpState                     = EP_STATE_IDLE;

    /* Puts the EpZero endpoint state machine in EP_STATE_SETUP_RQST_PHASE */
    pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
    //ret = SendEpZeroData(pUsbCoreData->pDeviceHead,(char*)pUsbOtgHostData->pSetupPkt,SETUP_PACKET_LEN);
    ret = SendEpZeroDataEx(pUsbCoreData->pDeviceHead,(char*)pUsbOtgHostData->pSetupPkt,SETUP_PACKET_LEN);

    /* request has been sent move to status phase */
    pEpInfo->EpStateMachineHandler(pDevO,1,(void*)pUsbOtgHostData->pSetupPkt);

    /* Mark the state as device configured */
    pUsbOtgHostData->eDepState = DEP_DEV_CONFIGURED;

    /* Move from status state to the IDLE state */
    pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);

    pUsbOtgHostData->pRemoteDeviceObj->pActiveConfigObj = pConfigO;
    pUsbOtgHostData->pRemoteDeviceObj->pActiveInterfaceObj = pConfigO->pInterfaceObj;
    UpdateActiveEpList(pUsbOtgHostData->pRemoteDeviceObj);

    USB_LOG_EVENT(HSD(SET_CONFIGURATION),pUsbOtgHostData->pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-SET_CONFIGURATION");
  return(ADI_USB_RESULT_SUCCESS);
}

/*******************************************************************************
 * adi_usb_otg_SetInterface()
 *
 * Description:
 *
 * Set interface, the interface and the alternate setting must be valid for the 
 * last selected configuration.
 *
 * @param dwInterfaceID interface id 
 * @param dwAltSetting alternate setting of the interface
 *
 * @return upon success returns ADI_USB_RESULT_SUCCESS
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
s32_t adi_usb_otg_SetInterface(const s32_t dwInterfaceID, const s32_t dwAltSetting)
{
s32_t ret;
USB_EP_INFO *pEpInfo = &pUsbCoreData->pDeviceHead->pEndpointZeroObj->EPInfo;
PDEVICE_OBJECT pDevO = pUsbCoreData->pDeviceHead;

    USB_ASSERT(dwInterfaceID < 0);

    /* you can not issue set_interface without a previous set configuration */
    USB_ASSERT(pUsbOtgHostData->pRemoteDeviceObj->pActiveConfigObj == NULL);

    SetInterface(pUsbOtgHostData->pRemoteDeviceObj,dwInterfaceID,(u8_t)(dwAltSetting& 0xff));

    memset(pUsbOtgHostData->pSetupPkt,0,sizeof(SETUP_PACKET));
    pUsbOtgHostData->pSetupPkt->bmRequestType =  USB_DIR_HOST_TO_DEVICE | USB_TYPE_STANDARD |
			                                           USB_RECIPIENT_DEVICE;
    pUsbOtgHostData->pSetupPkt->bRequest =  USB_STD_RQST_SET_INTERFACE;
    pUsbOtgHostData->pSetupPkt->wValue   =  (dwAltSetting & 0xff);
    pUsbOtgHostData->pSetupPkt->wIndex   =  (dwInterfaceID & 0xff);
    pUsbOtgHostData->pSetupPkt->wLength  =  0;
    pEpInfo->EpState                     = EP_STATE_IDLE;

    /* Puts the EpZero endpoint state machine in EP_STATE_SETUP_RQST_PHASE */
    pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
    ret = SendEpZeroDataEx(pUsbCoreData->pDeviceHead,(char*)pUsbOtgHostData->pSetupPkt,SETUP_PACKET_LEN);

    /* request has been sent move to status phase */
    pEpInfo->EpStateMachineHandler(pDevO,1,(void*)pUsbOtgHostData->pSetupPkt);

    /* Mark the state as device configured */
    pUsbOtgHostData->eDepState = DEP_DEV_CONFIGURED;

    /* Move from status state to the IDLE state */
    pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);

    UpdateActiveEpList(pUsbOtgHostData->pRemoteDeviceObj);
    USB_LOG_EVENT(HSD(SET_INTERFACE),pUsbOtgHostData->pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-SET_INTERFACE");
  return(ADI_USB_RESULT_SUCCESS);
}

/*******************************************************************************
 * adi_usb_otg_GetStringFromIndex()
 *
 * Description:
 * Returns the string associated with the index.
 *
 * @dwStringIndex points to the string index
 * @pString points to the memory where string will get copied
 *
 * @return returns the length of the string, and copies ASCII string into the pString
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
s32_t adi_usb_otg_GetStringFromIndex(const s32_t dwStringDescIndex, u8_t *pAsciiString)
{
s32_t i,ret=ADI_USB_RESULT_INVALID_ID,j,dwLength;

    USB_ASSERT(pAsciiString == NULL);

    for(i=0;i<pUsbOtgHostData->dwMaxStringIndex;i++)
    {
        /* check the string index in the available list */
        if(pUsbOtgHostData->StringInfo[i].dwStringIndex == dwStringDescIndex)
        {
            /* In the string descriptor first byte has the length */
            dwLength  = (s32_t)(pUsbOtgHostData->StringInfo[i].StringData[0] & 0xff);
            dwLength -= 2;
            ret = dwLength >> 2;
            CONVERT_UNICODE_TO_ASCII(&pUsbOtgHostData->StringInfo[i].StringData[2],pAsciiString,dwLength);
            break;
        }
    }
    return(ret);
}

/*******************************************************************************
 * adi_usb_otg_ClearOTGHostData
 *
 * Description:
 * Clears the OTG host data structures
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
void  adi_usb_otg_ClearOTGHostData(void)
{
   memset(&UsbOtgHostData, 0, sizeof(USB_OTG_HOST_DATA));
   memset(&SetupPkt, 0, sizeof(SETUP_PACKET));
}

/*******************************************************************************
 * HostModeTransmitEpZeroBuffer()
 *
 * Description:
 * Host mode epzero transfer. whether class driver is receiving data or trasmit
 * always the first packet would be the setup packet.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
s32_t HostModeTransmitEpZeroBuffer(ADI_DEV_1D_BUFFER *pBuffer)
{
DEVICE_OBJECT       *pDevO = pUsbCoreData->pDeviceHead;  /* head should be set to point active device */
USB_EP_INFO         *pEpInfo = &pDevO->pEndpointZeroObj->EPInfo; /* Endpoint zero object */
s32_t ReturnValue = 0;

     /* new transfer, first packet must be setup packet */
     if(pDevO->pAppEpZeroBuffer == NULL)
     {
        PSETUP_PACKET pSetupPkt = (PSETUP_PACKET)pBuffer->Data;

	memcpy(&pDevO->ActiveSetupPkt,pSetupPkt,sizeof(SETUP_PACKET));

	/* first packet must be a setup packet */
	USB_ASSERT(pBuffer->ElementCount * pBuffer->ElementWidth != SETUP_PACKET_LEN);
        pEpInfo->EpState = EP_STATE_IDLE;

	/* configure to send setup packet */
        pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pSetupPkt);

        pDevO->pAppEpZeroBuffer  = (ADI_DEV_BUFFER*)pBuffer;
        ReturnValue = SendEpZeroDataEx(pDevO,(char*)pSetupPkt,SETUP_PACKET_LEN);

	/* check if there is data phase, if not move to status */
	if(pSetupPkt->wLength == 0)
	{
             /* Move to status phase */	
             pEpInfo->EpStateMachineHandler(pDevO,1,(void*)pBuffer);

	     /* Move to Idle */
             pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);
	}
     }
     else 
     {
	 USB_ASSERT(pBuffer == NULL);

	 USB_ASSERT(pEpInfo->EpDir != USB_DIR_HOST_TO_DEVICE);
         
	 /* reset the buffer */
         pDevO->pAppEpZeroBuffer  = (ADI_DEV_BUFFER*)pBuffer;
	 USB_ASSERT(pEpInfo->EpState != EP_STATE_SETUP_RQST_PHASE);

	 pEpInfo->TransferSize = pBuffer->ElementWidth * pBuffer->ElementCount;
         pDevO->pSetupData = (u8_t*)pBuffer->Data;

         /* Move to data phase */
         pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pBuffer);

	 /* start transmitting data, tx complete handler will return the buffer to the app */
         ReturnValue = SendEpZeroDataEx(pDevO,(char*)pBuffer->Data,pEpInfo->TransferSize);
     }
     return(ReturnValue);
}

/*******************************************************************************
 * HostModeReceiveEpZeroBuffer()
 *
 * Receives data buffer. This call is issued to read data in data phase of a setup
 * transaction. ElementCount * ElementWidth has to be in accordance with he setup
 * packet that has been sent.
 *
 ******************************************************************************/
__ADI_USB_OTG_SECTION_CODE
s32_t HostModeReceiveEpZeroBuffer(ADI_DEV_1D_BUFFER *pBuffer)
{
s32_t ReturnValue =0;
DEVICE_OBJECT       *pDevO = pUsbCoreData->pDeviceHead;  /* head should be set to point active device */
USB_EP_INFO         *pEpInfo = &pDevO->pEndpointZeroObj->EPInfo; /* Endpoint zero object */
u32_t uInterruptStatus = cli();

    //USB_ASSERT(pEpInfo->EpDir != USB_DIR_DEVICE_TO_HOST);
    USB_ASSERT(pEpInfo->EpState != EP_STATE_SETUP_RQST_PHASE);
    USB_ASSERT(pBuffer == NULL);

    pDevO->pAppEpZeroBuffer = (ADI_DEV_BUFFER*)pBuffer;
    pDevO->pSetupData  = pBuffer->Data;
    pEpInfo->EpNumTotalBytes = 0;
    pEpInfo->TransferSize = pBuffer->ElementWidth * pBuffer->ElementCount;

    /* Move from SETUP RQST phase to data phase. Send IN token in case you are expecting data */
    pEpInfo->EpStateMachineHandler(pDevO,0,(void*)pUsbOtgHostData->pSetupPkt);
    
    sti(uInterruptStatus);

return(ReturnValue);

}
