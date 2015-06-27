/*********************************************************************************

Copyright(c) 2006 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_usb_bulkadi.c,v $
$Revision: 2386 $
$Date: 2010-03-25 17:26:08 -0400 (Thu, 25 Mar 2010) $

Description:
             Provides the custom ADI bulk class driver functionality.

*********************************************************************************/

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_BULK_SECTION_CODE    section("adi_usb_bulk_code")
#define __ADI_USB_BULK_SECTION_DATA    section("adi_usb_bulk_data")
#else
#define __ADI_USB_BULK_SECTION_CODE
#define __ADI_USB_BULK_SECTION_DATA
#endif 


#include <services/services.h>        /* system service includes */
#include <drivers/adi_dev.h>        /* device manager includes */
#include <drivers\usb\usb_core\adi_usb_objects.h>
#include <drivers\usb\usb_core\adi_usb_core.h>
#include <drivers\usb\usb_core\adi_usb_debug.h>
#include <drivers\usb\class\peripheral\vendor_specific\adi\bulkadi\adi_usb_bulkadi.h>
#include <ccblkfn.h>
#include <cplb.h>

/* global static data */
__ADI_USB_BULK_SECTION_DATA
static VENDORSPECIFIC_DEV_DATA VSDevData = {0};
#define VS_INTERFACE_NUM 0
extern int __cplb_ctrl;

#define _OTHER_SPEED_EP_SIZE_ 64
#define _DEFAULT_SPEED_EP_SIZE_ 512

/***********************************************************************
 *
 *  Bulk Callback handler
 ***********************************************************************/
__ADI_USB_BULK_SECTION_CODE
static void VSBulkClassDriverCallback( void *AppHandle, u32 Event, void *pArg)
{
VENDORSPECIFIC_DEV_DATA *pVSDev = &VSDevData;
PUSB_EP_INFO              pEpInfo;
POBJECT_INFO              pOInf;

      /* Invoke the application callback */
       pVSDev->DMCallback(pVSDev->DeviceHandle,Event,pArg);

       switch(Event)
       {

              case ADI_USB_EVENT_ROOT_PORT_RESET:
              case ADI_USB_EVENT_DISCONNECT:
           {
               /* Reset IN endpoint lists */
               if(pVSDev->busSpeed==ADI_USB_DEVICE_SPEED_HIGH)
           {    
                       pOInf = &pVSDev->EndPointObjects[0];
           }
           else
              {
            pOInf = &pVSDev->OtherSpeedEndPointObjects[0];
           }                          
              
              pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);
              pEpInfo->pFreeXmtList = NULL;
              pEpInfo->pQueuedXmtList = NULL;
              pEpInfo->pTransferBuffer=NULL;


               /* Rest OUT endpoint lists */
              if(pVSDev->busSpeed==ADI_USB_DEVICE_SPEED_HIGH)
          {    
                   pOInf = &pVSDev->EndPointObjects[1];
          }
          else
          {
            pOInf = &pVSDev->OtherSpeedEndPointObjects[1];
          } 
                                       
              pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);
              pEpInfo->pFreeRcvList = NULL;
              pEpInfo->pProcessedRcvList = NULL;
              pEpInfo->pTransferBuffer=NULL;
           }
              break;

       }

}

/*********************************************************************
*
*    Function:         VSBulkConfigure
*
*    Description:     Performs class configuration
*
*********************************************************************/
__ADI_USB_BULK_SECTION_CODE
static s32_t VSBulkConfigure(void)
{
u32 Result                      = ADI_DEV_RESULT_SUCCESS;
VENDORSPECIFIC_DEV_DATA *pVSDev = &VSDevData;
POBJECT_INFO              pOInf;
PCONFIGURATION_DESCRIPTOR pConfigD;
PINTERFACE_DESCRIPTOR     pInterfaceD;
PENDPOINT_DESCRIPTOR      pEndpointD;
PUSB_EP_INFO              pEpInfo;
LOGICAL_EP_INFO           LogEpInfo;

    /* If class is already configured then we return */
    if(pVSDev->IsClassConfigured)
      return(Result);

    USB_ASSERT(pVSDev->PeripheralDevHandle == NULL);

    /* setup the entrypoint and peripheral handle for the usb core */
    adi_usb_SetPhysicalDriverHandle(pVSDev->PeripheralDevHandle);

    /* Get the current Device ID. Once the NET2272 driver is opened its
     *  suppose to create the device. If the device is already opened then
     *  it must return the same device ID.
     */
    adi_dev_Control(pVSDev->PeripheralDevHandle,
                    ADI_USB_CMD_GET_DEVICE_ID,
                    (void*)&pVSDev->DeviceID);

    /* If data cache is enabled then we will let the physical driver know about it*/
     if( (__cplb_ctrl & CPLB_ENABLE_DCACHE ) || (__cplb_ctrl & CPLB_ENABLE_DCACHE2))
     {
            adi_dev_Control(pVSDev->PeripheralDevHandle,
                             ADI_USB_CMD_BUFFERS_IN_CACHE,
                             (void *)TRUE);
     }

    /**********************************************
     * Create VS ConfigObject
     *********************************************/
    pOInf = &pVSDev->ConfigObject;
    pOInf->ID = adi_usb_CreateConfiguration((PCONFIG_OBJECT*)&pOInf->pObj);

    /* setup the EP0 callback */
    ((PCONFIG_OBJECT)pOInf->pObj)->EpZeroCallback = VSBulkClassDriverCallback;

    /* attach configuration to the device object */
    //adi_usb_AttachConfiguration(pVSDev->DeviceID,pOInf->ID);

    /* fill  configuration descriptor */
    pConfigD = ((PCONFIG_OBJECT)pOInf->pObj)->pConfigDesc;
    pConfigD->bLength = CONFIGURATION_DESCRIPTOR_LEN;
    pConfigD->bDescriptorType = TYPE_CONFIGURATION_DESCRIPTOR;
    pConfigD->wTotalLength = CONFIGURATION_DESCRIPTOR_LEN + INTERFACE_DESCRIPTOR_LEN + ENDPOINT_DESCRIPTOR_LEN *2;
    pConfigD->bNumInterfaces      = 0x1;
    pConfigD->bConfigurationValue = 0x1;
    pConfigD->bIConfiguration     = 0x0;
    pConfigD->bAttributes         = 0x80;  /* Bus powered */
    pConfigD->bMaxPower           = 50;    /* Bus powered devices need default power value 50 corresponds to 100mA*/

    /**********************************************
     * add an Interface #1
     *********************************************/
    pOInf = &pVSDev->InterfaceObjects[0];
    pOInf->ID = adi_usb_CreateInterface((PINTERFACE_OBJECT*)&pOInf->pObj);

    /* attach Interface object to the Configuration object */
    adi_usb_AttachInterface(pVSDev->ConfigObject.ID,pOInf->ID);

    /* Initialize Interface descriptor */
    pInterfaceD = ((PINTERFACE_OBJECT)pOInf->pObj)->pInterfaceDesc;
    pInterfaceD->bLength           = INTERFACE_DESCRIPTOR_LEN;
    pInterfaceD->bDescriptorType   = TYPE_INTERFACE_DESCRIPTOR;
    pInterfaceD->bInterfaceNumber  = VS_INTERFACE_NUM;
    pInterfaceD->bAlternateSetting = 0x0;
    pInterfaceD->bNumEndpoints     = 2;
    pInterfaceD->bInterfaceClass   = USB_VENDOR_SPECIFIC_CLASS_CODE;
    pInterfaceD->bInterfaceSubClass = 0x0;
    pInterfaceD->bInterfaceProtocol  = 0x0;
    pInterfaceD->bIInterface        = 0x0;

    /**********************************************
     * create IN Endpoint Object
     *********************************************/
    pOInf = &pVSDev->EndPointObjects[0];
    LogEpInfo.dwMaxEndpointSize = _DEFAULT_SPEED_EP_SIZE_;
    pOInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOInf->pObj,&LogEpInfo);
    pEndpointD = ((PENDPOINT_OBJECT)pOInf->pObj)->pEndpointDesc;

    /* attach endpoint object to the Configuration object */
    adi_usb_AttachEndpoint(pVSDev->InterfaceObjects[0].ID,pOInf->ID);

    pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);

    /* set the callback */
    pEpInfo->EpCallback = VSBulkClassDriverCallback;
    pEpInfo->TransferMode = EP_MODE_TRANSFER;

    pEndpointD->bLength = ENDPOINT_DESCRIPTOR_LEN;
    pEndpointD->bDescriptorType = TYPE_ENDPOINT_DESCRIPTOR;
    pEndpointD->bEndpointAddress = ( ((u8_t)EP_DIR_IN) | ((u8_t)pOInf->ID));
    pEndpointD->bAttributes      = USB_BULK_MODE;
    pEndpointD->wMaxPacketSize   = _DEFAULT_SPEED_EP_SIZE_;
    pEndpointD->bInterval        = 0;

    /**********************************************
     * create OUT Endpoint Object
     *********************************************/
    pOInf = &pVSDev->EndPointObjects[1];
    LogEpInfo.dwMaxEndpointSize = _DEFAULT_SPEED_EP_SIZE_;
    pOInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOInf->pObj,&LogEpInfo);

    pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);

    /* set the callback */
    pEpInfo->EpCallback = VSBulkClassDriverCallback;
    pEpInfo->TransferMode = EP_MODE_TRANSFER;

    /* attach endpoint object to the Configuration object */
    adi_usb_AttachEndpoint(pVSDev->InterfaceObjects[0].ID,pOInf->ID);

    pEndpointD = ((PENDPOINT_OBJECT)pOInf->pObj)->pEndpointDesc;
    pEndpointD->bLength = ENDPOINT_DESCRIPTOR_LEN;
    pEndpointD->bDescriptorType = TYPE_ENDPOINT_DESCRIPTOR;
    pEndpointD->bEndpointAddress = ( ((u8_t)EP_DIR_OUT) | ((u8_t)pOInf->ID));
    pEndpointD->bAttributes      = USB_BULK_MODE;
    pEndpointD->wMaxPacketSize   = _DEFAULT_SPEED_EP_SIZE_;
    pEndpointD->bInterval        = 0;

       /**********************************************
    * other speed configuration 
    * Create VS ConfigObject
    *********************************************/
    pOInf = &pVSDev->OtherSpeedConfigObject;
    pOInf->ID = adi_usb_CreateConfiguration((PCONFIG_OBJECT*)&pOInf->pObj);

    /* setup the EP0 callback */
    ((PCONFIG_OBJECT)pOInf->pObj)->EpZeroCallback = VSBulkClassDriverCallback;

    /* attach other speed configuration to the configobject */
    adi_usb_AttachOtherSpeedConfiguration(pVSDev->ConfigObject.ID,pOInf->ID);

    /* attach configuration to the device object */
    adi_usb_AttachConfiguration(pVSDev->DeviceID,pVSDev->ConfigObject.ID);
        
    /* fill  configuration descriptor */
    pConfigD = ((PCONFIG_OBJECT)pOInf->pObj)->pConfigDesc;
    pConfigD->bLength = CONFIGURATION_DESCRIPTOR_LEN;
    pConfigD->bDescriptorType =  TYPE_CONFIGURATION_DESCRIPTOR;
    pConfigD->wTotalLength = CONFIGURATION_DESCRIPTOR_LEN + INTERFACE_DESCRIPTOR_LEN + ENDPOINT_DESCRIPTOR_LEN *2;
    pConfigD->bNumInterfaces      = 0x1;
    pConfigD->bConfigurationValue = 0x1;
    pConfigD->bIConfiguration     = 0x0;
    pConfigD->bAttributes         = 0x80;  /* Bus powered */
    pConfigD->bMaxPower           = 50;    /* Bus powered devices need default power value 50 corresponds to 100mA*/

    /**********************************************
     * add an Interface #1
     *********************************************/
    pOInf = &pVSDev->OtherSpeedInterfaceObjects[0];
    pOInf->ID = adi_usb_CreateInterface((PINTERFACE_OBJECT*)&pOInf->pObj);

    /* attach Interface object to the Configuration object */
    adi_usb_AttachInterface(pVSDev->OtherSpeedConfigObject.ID,pOInf->ID);

    /* Initialize Interface descriptor */
    pInterfaceD = ((PINTERFACE_OBJECT)pOInf->pObj)->pInterfaceDesc;
    pInterfaceD->bLength           = INTERFACE_DESCRIPTOR_LEN;
    pInterfaceD->bDescriptorType   = TYPE_INTERFACE_DESCRIPTOR;
    pInterfaceD->bInterfaceNumber  = VS_INTERFACE_NUM;
    pInterfaceD->bAlternateSetting = 0x0;
    pInterfaceD->bNumEndpoints     = 2;
    pInterfaceD->bInterfaceClass   = USB_VENDOR_SPECIFIC_CLASS_CODE;
    pInterfaceD->bInterfaceSubClass = 0x0;
    pInterfaceD->bInterfaceProtocol  = 0x0;
    pInterfaceD->bIInterface        = 0x0;

    /**********************************************
     * create IN Endpoint Object
     *********************************************/
    pOInf = &pVSDev->OtherSpeedEndPointObjects[0];
    LogEpInfo.dwMaxEndpointSize = _OTHER_SPEED_EP_SIZE_;
    pOInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOInf->pObj,&LogEpInfo);
    pEndpointD = ((PENDPOINT_OBJECT)pOInf->pObj)->pEndpointDesc;

    /* attach endpoint object to the Configuration object */
    adi_usb_AttachEndpoint(pVSDev->OtherSpeedInterfaceObjects[0].ID,pOInf->ID);

    pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);

    /* set the callback */
    pEpInfo->EpCallback = VSBulkClassDriverCallback;
    pEpInfo->TransferMode = EP_MODE_TRANSFER;

    pEndpointD->bLength = ENDPOINT_DESCRIPTOR_LEN;
    pEndpointD->bDescriptorType = TYPE_ENDPOINT_DESCRIPTOR;
    pEndpointD->bEndpointAddress = ( ((u8_t)EP_DIR_IN) | ((u8_t)pOInf->ID));
    pEndpointD->bAttributes      = USB_BULK_MODE;
    pEndpointD->wMaxPacketSize   = _OTHER_SPEED_EP_SIZE_;
    pEndpointD->bInterval        = 0;

    /**********************************************
     * create OUT Endpoint Object
     *********************************************/
    pOInf = &pVSDev->OtherSpeedEndPointObjects[1];
    LogEpInfo.dwMaxEndpointSize = _OTHER_SPEED_EP_SIZE_;
    pOInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOInf->pObj,&LogEpInfo);

    pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);

    /* set the callback */
    pEpInfo->EpCallback = VSBulkClassDriverCallback;
    pEpInfo->TransferMode = EP_MODE_TRANSFER;

    /* attach endpoint object to the Configuration object */
    adi_usb_AttachEndpoint(pVSDev->OtherSpeedInterfaceObjects[0].ID,pOInf->ID);

    pEndpointD = ((PENDPOINT_OBJECT)pOInf->pObj)->pEndpointDesc;
    pEndpointD->bLength = ENDPOINT_DESCRIPTOR_LEN;
    pEndpointD->bDescriptorType = TYPE_ENDPOINT_DESCRIPTOR;
    pEndpointD->bEndpointAddress = ( ((u8_t)EP_DIR_OUT) | ((u8_t)pOInf->ID));
    pEndpointD->bAttributes      = USB_BULK_MODE;
    pEndpointD->wMaxPacketSize   = _OTHER_SPEED_EP_SIZE_;
    pEndpointD->bInterval        = 0;

    /* Set class configured to true */
    pVSDev->IsClassConfigured = true;

    return(ADI_DEV_RESULT_SUCCESS);
}

/*********************************************************************
*
*    Function:        pddOpen
*
*    Description:    Opens the peripheral driver and does initialization.
*
*********************************************************************/

__ADI_USB_BULK_SECTION_CODE
static u32 adi_pdd_Open(                        /* Open a device */
   ADI_DEV_MANAGER_HANDLE       ManagerHandle,    /* device manager handle */
   u32                          DeviceNumber,    /* device number */
   ADI_DEV_DEVICE_HANDLE        DeviceHandle,    /* device handle */
   ADI_DEV_PDD_HANDLE           *pPDDHandle,    /* pointer to PDD handle location */
   ADI_DEV_DIRECTION            Direction,      /* data direction */
   void           *pCriticalRegionArg,              /* critical region imask storage location */
   ADI_DMA_MANAGER_HANDLE       DMAHandle,        /* handle to the DMA manager */
   ADI_DCB_HANDLE               DCBHandle,        /* callback handle */
   ADI_DCB_CALLBACK_FN          DMCallback        /* device manager callback function */
)
{
VENDORSPECIFIC_DEV_DATA *pVSDev = &VSDevData;
u32 Result =ADI_DEV_RESULT_SUCCESS;

    /* Check if class driver has been already already opened */
    if(!pVSDev->Open)
    {
        pVSDev->Open = true;
        pVSDev->DeviceHandle = DeviceHandle;
        pVSDev->DCBHandle    = DCBHandle;
        pVSDev->DMCallback   = DMCallback;
        pVSDev->CriticalData = pCriticalRegionArg;
        pVSDev->Direction    = Direction;
          pVSDev->IsClassConfigured = false;
    }
    else
        Result = ADI_DEV_RESULT_DEVICE_IN_USE;

    return(Result);
}

/*********************************************************************
*
*    Function:        pddClose
*
*    Description:    Closes the driver and releases any memory
*
*********************************************************************/

__ADI_USB_BULK_SECTION_CODE
static u32 adi_pdd_Close(            /* Closes a device */
    ADI_DEV_PDD_HANDLE PDDHandle    /* PDD handle */
)
{
    unsigned int Result=ADI_DEV_RESULT_SUCCESS;
    VENDORSPECIFIC_DEV_DATA *pVSDev = &VSDevData;

    pVSDev->Open = false;
    //Result = adi_dev_Close(    pVSDev->PeripheralDevHandle );

    return(Result);
}

/*********************************************************************
*
*    Function:        pddRead
*
*    Description:    Gives list of read buffers to the driver
*
*********************************************************************/
__ADI_USB_BULK_SECTION_CODE
static u32 adi_pdd_Read(            /* Reads data or queues an inbound buffer to a device */
    ADI_DEV_PDD_HANDLE PDDHandle,    /* PDD handle */
    ADI_DEV_BUFFER_TYPE    BufferType,    /* buffer type */
    ADI_DEV_BUFFER *pBuffer            /* pointer to buffer */
)
{
    unsigned int Result;
    unsigned int epNum;
    VENDORSPECIFIC_DEV_DATA *pVSDev = &VSDevData;

    Result = adi_dev_Read(    pVSDev->PeripheralDevHandle,
                            BufferType,
                            pBuffer);


    return(Result);
}


/*********************************************************************
*
*    Function:        pddWrite
*
*    Description:    Sends packet over the physical channel
*
*********************************************************************/
__ADI_USB_BULK_SECTION_CODE
static u32 adi_pdd_Write(                /* Writes data or queues an outbound buffer to a device */
    ADI_DEV_PDD_HANDLE PDDHandle,        /* PDD handle */
    ADI_DEV_BUFFER_TYPE    BufferType,        /* buffer type */
    ADI_DEV_BUFFER *pBuffer                /* pointer to buffer */
)
{
    unsigned int Result;
    VENDORSPECIFIC_DEV_DATA *pVSDev = &VSDevData;

    Result = adi_dev_Write(    pVSDev->PeripheralDevHandle,
                            BufferType,
                            pBuffer);
    return(Result);
}

/*********************************************************************
*
*    Function:        EnumerateEndpoints
*
*    Description:    Returns currently active endpoint information
*
*********************************************************************/
__ADI_USB_BULK_SECTION_CODE
static s32 EnumerateEndpoints(ADI_ENUM_ENDPOINT_INFO *pEnumEpInfo)
{
VENDORSPECIFIC_DEV_DATA *pDev = &VSDevData;
ADI_USB_APP_EP_INFO *pUsbAppEpInfo=pEnumEpInfo->pUsbAppEpInfo;
int i;
unsigned int Result=ADI_DEV_RESULT_SUCCESS;
ENDPOINT_DESCRIPTOR *pEndpointD;
s32  dwTotalEpEntries = pEnumEpInfo->dwEpTotalEntries;
s32  dwNumVsEndpoints;

    /* Get the current device speed */
    adi_dev_Control(pDev->PeripheralDevHandle, ADI_USB_CMD_GET_DEV_SPEED, (void*)&pDev->busSpeed);

    /* Get the correct ep object based on the speed */
    if(pDev->busSpeed==ADI_USB_DEVICE_SPEED_HIGH)
    {
        dwNumVsEndpoints = sizeof(pDev->EndPointObjects) / sizeof(OBJECT_INFO);
    }
    else
    {
        dwNumVsEndpoints = sizeof(pDev->OtherSpeedEndPointObjects) / sizeof(OBJECT_INFO);
    }

     /* If supplied memory is not sufficent then we return error */
     if(pEnumEpInfo->dwEpTotalEntries < dwNumVsEndpoints)
     {
         /* Set the total required entries */
         pEnumEpInfo->dwEpProcessedEntries = dwNumVsEndpoints;
         Result = ADI_DEV_RESULT_NO_MEMORY;
         return(Result);
     }

     for(i=0; i< dwNumVsEndpoints; i++)
     {
        if(pDev->busSpeed==ADI_USB_DEVICE_SPEED_HIGH)
        {
        /* Get the associated endpoint descriptor */
        pEndpointD = ((PENDPOINT_OBJECT)(pDev->EndPointObjects[i].pObj))->pEndpointDesc;

        /* Get the endpoint ID */
        pUsbAppEpInfo->dwEndpointID = pDev->EndPointObjects[i].ID;
        }            
        else
        {
             /* Get the associated endpoint descriptor */
            pEndpointD = ((PENDPOINT_OBJECT)(pDev->OtherSpeedEndPointObjects[i].pObj))->pEndpointDesc;

            /* Get the endpoint ID */
            pUsbAppEpInfo->dwEndpointID = pDev->OtherSpeedEndPointObjects[i].ID;
        }

        /* Set endpoint direction */
        pUsbAppEpInfo->eDir = ((pEndpointD->bEndpointAddress >> 7) & 0x1) ? USB_EP_IN : USB_EP_OUT ;

        /* Set the endpoint attributes */
        pUsbAppEpInfo->bAttributes = pEndpointD->bAttributes;

        pUsbAppEpInfo++;

     }
   return(Result);
}

/*********************************************************************
*
*    Function:        pddControl
*
*    Description:    List of I/O control commands to the driver
*
*********************************************************************/
__ADI_USB_BULK_SECTION_CODE
static u32 adi_pdd_Control(            /* Sets or senses a device specific parameter */
    ADI_DEV_PDD_HANDLE PDDHandle,    /* PDD handle */
    u32 Command,                    /* command ID */
    void *pArg                        /* pointer to argument */
)
{
VENDORSPECIFIC_DEV_DATA *pVSDev = &VSDevData;
u32 Result;

        switch(Command)
        {
           /*
            * Class drivers do not use peripheral DMA.
            */
            case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:
            {
                (*(u32 *)pArg) = FALSE;
                Result = ADI_DEV_RESULT_SUCCESS;
            }
            break;

           /*
            * Returns currently active endpoints.
            */
            case ADI_USB_CMD_CLASS_ENUMERATE_ENDPOINTS:
            {
                 ADI_ENUM_ENDPOINT_INFO  *pEnumEpInfo = (ADI_ENUM_ENDPOINT_INFO*)pArg;
                 Result = EnumerateEndpoints(pEnumEpInfo);
            }
            break;

           /*
            * Underlying controller driver handle is passed through this i/o control.
            */
            case ADI_USB_CMD_CLASS_SET_CONTROLLER_HANDLE:
            {
                 USB_ASSERT(pArg == NULL);
                 pVSDev->PeripheralDevHandle =(ADI_DEV_DEVICE_HANDLE)pArg;
                 Result = ADI_DEV_RESULT_SUCCESS;
            }
            break;

           /*
            * Returns the supplied controller driver handle.
            */
            case ADI_USB_CMD_CLASS_GET_CONTROLLER_HANDLE:
            {
             u32 *pArgument = (u32*)pArg;
                 *pArgument = (u32)pVSDev->PeripheralDevHandle;
                 Result = ADI_DEV_RESULT_SUCCESS;

            }
            break;

           /*
            * Configures the class, sets up various class descriptors and attaches to the
            * USB device.
            */
            case ADI_USB_CMD_CLASS_CONFIGURE:
            {
                 Result =  VSBulkConfigure();
            }
            break;

           /*
            * If class driver can not handle the request it will pass it down
            * to the peripheral if its present.
            */
            default:
            {
                if(pVSDev->PeripheralDevHandle != NULL)
                {
                    Result = adi_dev_Control(pVSDev->PeripheralDevHandle,
                                             Command,
                                             (void*)pArg);
                }
            }
            break;
        }
        return(Result);
}

/**************************************************************************
 *
 * Audio Streaming driver entry point
 *
 **************************************************************************/
__ADI_USB_BULK_SECTION_DATA
ADI_DEV_PDD_ENTRY_POINT ADI_USB_VSBulk_Entrypoint  = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};
