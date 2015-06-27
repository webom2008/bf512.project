/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_usb_msd_class.c,v $
$Revision: 2386 $
$Date: 2010-03-25 17:26:08 -0400 (Thu, 25 Mar 2010) $

Description:

    This is the driver source code for the USB Mass
    Storage Class (Device Mode).

*********************************************************************************/

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_MSD_SECTION_CODE    section("adi_usb_msd_code")
#define __ADI_USB_MSD_SECTION_DATA    section("adi_usb_msd_data")
#else
#define __ADI_USB_MSD_SECTION_CODE
#define __ADI_USB_MSD_SECTION_DATA
#endif 

#include <services/services.h>      /* system service includes */
#include <drivers/adi_dev.h>        /* device manager includes */
#include <drivers/usb/usb_core/adi_usb_objects.h>       /* USB Objects */
#include <drivers/usb/usb_core/adi_usb_core.h>          /* USB Core */
#include <drivers/usb/class/peripheral/mass_storage/adi_usb_msd_class.h>        /* USB Mass Storage structure */
#include <drivers/usb/class/peripheral/mass_storage/adi_usb_msd_scsi.h>     /* SCSI includes */
#include <drivers/pid/adi_rawpid.h>
#include <string.h>
#include <stdio.h>

#include <cplb.h>
extern  int     __cplb_ctrl;

/*********************************************************************/

//#include <drivers/usb/controller/otg/adi/hdrc/adi_usb_hdrc.h>

#if defined(__ADSP_MOAB__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)
#include <drivers/usb/controller/otg/adi/hdrc/adi_usb_hdrc.h>
#elif defined(__ADSP_BRODIE__) || defined(__ADSP_EDINBURGH__) || defined(__ADSP_BRAEMAR__) || defined(__ADSP_TETON__)
#include <drivers/usb/controller/peripheral/plx/net2272/adi_usb_net2272.h>
#else
#error "USB Mass Storage Class not supported for this processor"
#endif


__ADI_USB_MSD_SECTION_DATA
ADI_DEV_DEVICE_HANDLE   RAW_PID_DeviceHandle;

/*********************************************************************
    Globals
*********************************************************************/

/* Pointers to endpoint information */
__ADI_USB_MSD_SECTION_DATA
USB_EP_INFO *adi_usb_msd_pOUTEPInfo;
__ADI_USB_MSD_SECTION_DATA
USB_EP_INFO *adi_usb_msd_pINEPInfo;

__ADI_USB_MSD_SECTION_DATA
ADI_DEV_DEVICE_HANDLE   adi_msd_PeripheralDevHandle;

/* USB Endpoints */
__ADI_USB_MSD_SECTION_DATA
u8  adi_usb_msd_IN_EP;
__ADI_USB_MSD_SECTION_DATA
u8  adi_usb_msd_OUT_EP;

__ADI_USB_MSD_SECTION_DATA
u32  adi_usb_msd_max_memory_size;

/* Assume we use memory */
__ADI_USB_MSD_SECTION_DATA
bool adi_msd_HDDetected = FALSE;

__ADI_USB_MSD_SECTION_DATA
ADI_DEV_MANAGER_HANDLE adi_msd_dev_handle;
__ADI_USB_MSD_SECTION_DATA
ADI_DMA_MANAGER_HANDLE adi_msd_dma_handle;
__ADI_USB_MSD_SECTION_DATA
ADI_DCB_HANDLE         adi_msd_dcb_handle;

extern  u8      adi_usb_msd_device_ready;

/* Initialize the device data structure */
__ADI_USB_MSD_SECTION_DATA
ADI_USB_DEV_DEF adi_msd_usb_dev_def = {0};

/* Used to determin if app wants DEVICE or HOST mode */
__ADI_USB_MSD_SECTION_DATA
DEV_MODE    adi_USB_Device_Mode = MODE_DEVICE;

__ADI_USB_MSD_SECTION_DATA
static DEVICE_QUALIFIER_DESCRIPTOR gDeviceQualifierDesc =
{ // Device Qualifier descriptor
sizeof(DEVICE_QUALIFIER_DESCRIPTOR),// 0x00 Length of this
// descriptor
TYPE_DEVICEQUALIFIER_DESCRIPTOR,    // 0x01 Descriptor Type
0x0200,
0x08,                                 // 0x04 Class Code
0x00,                                  // 0x05 Sub Class Code
0x00,                                  // 0x06 Protocol
(u8_t)64,                             // 0x07 Maximum Endpoint 0 Packet Size
0x01,                                // 0x08 Number of configurations
0x00                                 // 0x09 Reserved
};

#include <services/fss/adi_fss.h>      /* system service includes */

extern ADI_DCB_CALLBACK_FN adi_rawpid_ClientCallbackFn;
extern void PidCallbackFunction(void *pClientHandle, u32 nEvent, void *pClientArg);
extern void SetupEPBuffer(ADI_DEV_BUFFER *pBuff, u8 epNum, u32 ulSize);

/*********************************************************************
*
*   Function:       DevMassStorageConfigure
*
*   Description:    Configures the Mass storage device
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static u32 DevMassStorageConfigure(void)
{
    u32 Result;
    POBJECT_INFO              pOInf;
    PCONFIGURATION_DESCRIPTOR pConfigD;
    PINTERFACE_DESCRIPTOR     pInterfaceD;
    PENDPOINT_DESCRIPTOR      pEndpointD;
    PUSB_EP_INFO              pEpInfo;
    LOGICAL_EP_INFO           LogEpInfo;
    ADI_USB_DEV_DEF *pDevice = &adi_msd_usb_dev_def;
    s8 DefaultSerialNumber[]    = "123456789012";

    PDEVICE_OBJECT pDeviceObject;

    Result = ADI_DEV_RESULT_SUCCESS;

    /* Set Peripheral driver handle */
    adi_usb_SetPhysicalDriverHandle(pDevice->PeripheralDevHandle);

    /* Set MODE to OTG Device */
    adi_usb_SetDeviceMode(MODE_DEVICE);

    adi_dev_Control(pDevice->PeripheralDevHandle,
                    ADI_USB_CMD_GET_DEVICE_ID,
                    (void*)&pDevice->DeviceID);

    adi_usb_GetObjectFromID(pDevice->DeviceID,USB_DEVICE_OBJECT_TYPE,(void*)&pDeviceObject);

    pDeviceObject->pDeviceQuaDesc = &gDeviceQualifierDesc;

    /* Save the peripherals device handle */
    adi_msd_PeripheralDevHandle = pDevice->PeripheralDevHandle;

    /* Create the Mass Storage Config Object */
    pOInf = &pDevice->ConfigObject;
    pOInf->ID = adi_usb_CreateConfiguration((PCONFIG_OBJECT*)&pOInf->pObj);

    /* Setup the callback for Endpoint 0 */
    ((PCONFIG_OBJECT)pOInf->pObj)->EpZeroCallback = (ADI_DCB_CALLBACK_FN)EndpointZeroCompleteCallback;

    /* Fill  configuration descriptor */
    pConfigD = ((PCONFIG_OBJECT)pOInf->pObj)->pConfigDesc;
    pConfigD->bLength =             CONFIGURATION_DESCRIPTOR_LEN;
    pConfigD->bDescriptorType     = TYPE_CONFIGURATION_DESCRIPTOR;
    pConfigD->wTotalLength        = CONFIGURATION_DESCRIPTOR_LEN + INTERFACE_DESCRIPTOR_LEN + ENDPOINT_DESCRIPTOR_LEN *2;
    pConfigD->bNumInterfaces      = MAX_INTERFACES;
    pConfigD->bConfigurationValue = 0x1;
    pConfigD->bIConfiguration     = 0x0;
    pConfigD->bAttributes         = 0x80;    /* Bus Powered */
    pConfigD->bMaxPower           = 50;        /* 100ma max   */

    /* Create a the Interface */
    pOInf = &pDevice->InterfaceObjects[0];
    pOInf->ID = adi_usb_CreateInterface((PINTERFACE_OBJECT*)&pOInf->pObj);

    /* Attach Interface object to the Configuration object */
    adi_usb_AttachInterface(pDevice->ConfigObject.ID,pOInf->ID);

    /* Initialize the Interface descriptor */
    pInterfaceD = ((PINTERFACE_OBJECT)pOInf->pObj)->pInterfaceDesc;
    pInterfaceD->bLength            = INTERFACE_DESCRIPTOR_LEN;
    pInterfaceD->bDescriptorType    = TYPE_INTERFACE_DESCRIPTOR;
    pInterfaceD->bInterfaceNumber   = MASS_STORAGE_INTERFACE_NUM;
    pInterfaceD->bAlternateSetting  = 0x0;
    pInterfaceD->bNumEndpoints      = MAX_ENDPOINTS;
    pInterfaceD->bInterfaceClass    = USB_MASS_STORAGE_CLASS_CODE;
    pInterfaceD->bInterfaceSubClass = USB_GENERIC_SCSI;
    pInterfaceD->bInterfaceProtocol = USB_BULK_ONLY;
    pInterfaceD->bIInterface        = 0x0;

    /* Create IN Endpoint Object */
    pOInf = &pDevice->EndPointObjects[0];
    LogEpInfo.dwMaxEndpointSize = HS_EP_PKT_SIZE;
    pOInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOInf->pObj,&LogEpInfo);

    pEndpointD = ((PENDPOINT_OBJECT)pOInf->pObj)->pEndpointDesc;

    /* Attach IN Endpoint Object */
    adi_usb_AttachEndpoint(pDevice->InterfaceObjects[0].ID,pOInf->ID);

    pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);

    /* Save info pointer (OUT from host) */
    adi_usb_msd_pINEPInfo = pEpInfo;

    /* Set the callback */
    pEpInfo->EpCallback = EndpointCompleteCallback;

    pEndpointD->bLength =           ENDPOINT_DESCRIPTOR_LEN;
    pEndpointD->bDescriptorType  =  TYPE_ENDPOINT_DESCRIPTOR;
    pEndpointD->bEndpointAddress =  (((u8)EP_DIR_IN) | ((u8)pOInf->ID));
    pEndpointD->bAttributes      =  USB_BULK_MODE;
    pEndpointD->wMaxPacketSize   =  HS_EP_PKT_SIZE;
    pEndpointD->bInterval        =  0;

    /* Create OUT Endpoint Object */
    pOInf = &pDevice->EndPointObjects[1];
    LogEpInfo.dwMaxEndpointSize = HS_EP_PKT_SIZE;
    pOInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOInf->pObj,&LogEpInfo);

    pEndpointD = ((PENDPOINT_OBJECT)pOInf->pObj)->pEndpointDesc;

    pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);

    /* Set the callback */
    pEpInfo->EpCallback = EndpointCompleteCallback;

    /* Save OUT EP info pointer (OUT from host) */
    adi_usb_msd_pOUTEPInfo = pEpInfo;

    /* Attach OUT Endpoint Object */
    adi_usb_AttachEndpoint(pDevice->InterfaceObjects[0].ID,pOInf->ID);

    pEndpointD->bLength          = ENDPOINT_DESCRIPTOR_LEN;
    pEndpointD->bDescriptorType  = TYPE_ENDPOINT_DESCRIPTOR;
    pEndpointD->bEndpointAddress = (((u8)EP_DIR_OUT) | ((u8)pOInf->ID));
    pEndpointD->bAttributes      = USB_BULK_MODE;
    pEndpointD->wMaxPacketSize   = HS_EP_PKT_SIZE;
    pEndpointD->bInterval        = 0;

    /* Create the Mass Storage Config Object */
    pOInf = &pDevice->OSConfigObject;
    pOInf->ID = adi_usb_CreateConfiguration((PCONFIG_OBJECT*)&pOInf->pObj);

    /* Setup the callback for Endpoint 0 */
    ((PCONFIG_OBJECT)pOInf->pObj)->EpZeroCallback = (ADI_DCB_CALLBACK_FN)EndpointZeroCompleteCallback;

    /* Attach Other Speed Configuration */
    adi_usb_AttachOtherSpeedConfiguration(pDevice->ConfigObject.ID,pOInf->ID);
    
    /* Attach configuration to the device object */
    adi_usb_AttachConfiguration(pDevice->DeviceID,pDevice->ConfigObject.ID);

    /* Fill  configuration descriptor */
    pConfigD = ((PCONFIG_OBJECT)pOInf->pObj)->pConfigDesc;
    pConfigD->bLength =             CONFIGURATION_DESCRIPTOR_LEN;
    pConfigD->bDescriptorType     = TYPE_CONFIGURATION_DESCRIPTOR;
    pConfigD->wTotalLength        = CONFIGURATION_DESCRIPTOR_LEN + INTERFACE_DESCRIPTOR_LEN + ENDPOINT_DESCRIPTOR_LEN *2;
    pConfigD->bNumInterfaces      = MAX_INTERFACES;
    pConfigD->bConfigurationValue = 0x1;
    pConfigD->bIConfiguration     = 0x0;
    pConfigD->bAttributes         = 0x80;    /* Bus Powered */
    pConfigD->bMaxPower           = 50;        /* 100ma max   */

    /* Create a the Interface */
    pOInf = &pDevice->OSInterfaceObjects[0];
    pOInf->ID = adi_usb_CreateInterface((PINTERFACE_OBJECT*)&pOInf->pObj);

       /* attach Interface object to the Configuration object */
       adi_usb_AttachInterface(pDevice->OSConfigObject.ID,pOInf->ID);
        
    /* Initialize the Interface descriptor */
    pInterfaceD = ((PINTERFACE_OBJECT)pOInf->pObj)->pInterfaceDesc;
    pInterfaceD->bLength            = INTERFACE_DESCRIPTOR_LEN;
    pInterfaceD->bDescriptorType    = TYPE_INTERFACE_DESCRIPTOR;
    pInterfaceD->bInterfaceNumber   = MASS_STORAGE_INTERFACE_NUM;
    pInterfaceD->bAlternateSetting  = 0x0;
    pInterfaceD->bNumEndpoints      = MAX_ENDPOINTS;
    pInterfaceD->bInterfaceClass    = USB_MASS_STORAGE_CLASS_CODE;
    pInterfaceD->bInterfaceSubClass = USB_GENERIC_SCSI;
    pInterfaceD->bInterfaceProtocol = USB_BULK_ONLY;
    pInterfaceD->bIInterface        = 0x0;

    /* Create IN Endpoint Object */
    pOInf = &pDevice->OSEndPointObjects[0];
    LogEpInfo.dwMaxEndpointSize = FS_EP_PKT_SIZE;

    pOInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOInf->pObj,&LogEpInfo);

    pEndpointD = ((PENDPOINT_OBJECT)pOInf->pObj)->pEndpointDesc;

    /* Attach IN Endpoint Object */
    adi_usb_AttachEndpoint(pDevice->OSInterfaceObjects[0].ID,pOInf->ID);

    pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);

    /* Save info pointer (OUT from host) */
    adi_usb_msd_pINEPInfo = pEpInfo;

    /* Set the callback */
    pEpInfo->EpCallback = EndpointCompleteCallback;

    pEndpointD->bLength =           ENDPOINT_DESCRIPTOR_LEN;
    pEndpointD->bDescriptorType  =  TYPE_ENDPOINT_DESCRIPTOR;
    pEndpointD->bEndpointAddress =  (((u8)EP_DIR_IN) | ((u8)pOInf->ID));
    pEndpointD->bAttributes      =  USB_BULK_MODE;
    pEndpointD->wMaxPacketSize   =  FS_EP_PKT_SIZE;
    pEndpointD->bInterval        =  0;

    /* Create OUT Endpoint Object */
    pOInf = &pDevice->OSEndPointObjects[1];
    LogEpInfo.dwMaxEndpointSize = FS_EP_PKT_SIZE;

    pOInf->ID = adi_usb_CreateEndPoint((PENDPOINT_OBJECT*)&pOInf->pObj,&LogEpInfo);

    pEndpointD = ((PENDPOINT_OBJECT)pOInf->pObj)->pEndpointDesc;
    
        /* Attach OUT Endpoint Object */
    adi_usb_AttachEndpoint(pDevice->OSInterfaceObjects[0].ID,pOInf->ID);

    pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);

    /* Set the callback */
    pEpInfo->EpCallback = EndpointCompleteCallback;

    /* Save OUT EP info pointer (OUT from host) */
    adi_usb_msd_pOUTEPInfo = pEpInfo;

    pEndpointD->bLength          = ENDPOINT_DESCRIPTOR_LEN;
    pEndpointD->bDescriptorType  = TYPE_ENDPOINT_DESCRIPTOR;
    pEndpointD->bEndpointAddress = (((u8)EP_DIR_OUT) | ((u8)pOInf->ID));
    pEndpointD->bAttributes      = USB_BULK_MODE;
    pEndpointD->wMaxPacketSize   = FS_EP_PKT_SIZE;
    pEndpointD->bInterval        = 0;

    /* Initialize the initial state variables */
    adi_msd_CommandStatusState = STATE_WAITFOR_CBW;
    adi_msd_IsTransferActive    = false;
    adi_msd_HDDetected = FALSE;

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
*   Description:    Opens the Mass Storage Class device
*                   in Device mode
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
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
    u32 i = 0;
    u32 Result      = ADI_DEV_RESULT_SUCCESS;
    adi_msd_dev_handle = ManagerHandle;
    adi_msd_dma_handle = DMAHandle;
    adi_msd_dcb_handle = DCBHandle;
    ADI_USB_DEV_DEF *pDevice = &adi_msd_usb_dev_def;


    /* Check if the device is already opened */
    if(!pDevice->Open)
    {
        pDevice->Open = true;
        pDevice->DeviceHandle = DeviceHandle;
        pDevice->DCBHandle    = DCBHandle;
        pDevice->DMCallback   = DMCallback;
        pDevice->CriticalData = pCriticalRegionArg;
        pDevice->Direction    = Direction;
        
        /* zero client buffer details */
        pDevice->pClientBuffer = NULL;        
        pDevice->nClientBufferElementCount = 0;        
    }
    
    return(Result);
}

/*********************************************************************
*
*   Function:       pddClose
*
*   Description:    Closes down a device
*
*********************************************************************/

__ADI_USB_MSD_SECTION_CODE
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE PDDHandle    /* PDD handle */
)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    ADI_USB_DEV_DEF   *pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;

    /* Close down the RAW PID if using HDD */
    if(adi_msd_HDDetected == TRUE)
    {
        adi_rawpid_Terminate(RAW_PID_DeviceHandle, NULL);
    }

    return(Result);
}

/*********************************************************************
*
*   Function:       pddRead
*
*   Description:
*
*********************************************************************/

__ADI_USB_MSD_SECTION_CODE
static u32 adi_pdd_Read(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
   
    u32 Result = ADI_DEV_RESULT_SUCCESS;

#if 0
    ADI_USB_DEV_DEF   *pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;

    ((ADI_DEV_1D_BUFFER*)pBuffer)->Reserved[BUFFER_RSVD_EP_ADDRESS] = adi_usb_msd_OUT_EP;

    Result = adi_dev_Read(  pDevice->PeripheralDevHandle,
                            BufferType,
                            pBuffer);
#endif                            

    return(Result);
}

/*********************************************************************
*
*   Function:       pddWrite
*
*   Description:
*
*********************************************************************/

__ADI_USB_MSD_SECTION_CODE
static u32 adi_pdd_Write(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;

    return (adi_dev_Write(pDevice->PeripheralDevHandle,
                            BufferType,
                            pBuffer));
}

/*********************************************************************
*
*   Function:       EnumerateEndpoints
*
*   Description:
*
*********************************************************************/

__ADI_USB_MSD_SECTION_CODE
static s32 EnumerateEndpoints(ADI_ENUM_ENDPOINT_INFO *pEnumEpInfo)
{
ADI_USB_DEV_DEF   *pDev = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
ADI_USB_APP_EP_INFO *pMSDEpInfo = pEnumEpInfo->pUsbAppEpInfo;

int i;
unsigned int Result=ADI_DEV_RESULT_SUCCESS;
s32  dwTotalEpEntries = pEnumEpInfo->dwEpTotalEntries;
s32  dwNumMSDEndpoints = 0;
ENDPOINT_DESCRIPTOR *pEndpointD;

    /* Get the current device speed */
    adi_dev_Control(pDev->PeripheralDevHandle, ADI_USB_CMD_GET_DEV_SPEED, (void*)&pDev->busSpeed);

    /* Get the correct ep object based on the speed */
    if(pDev->busSpeed==ADI_USB_DEVICE_SPEED_HIGH)
    {
        dwNumMSDEndpoints = sizeof(pDev->EndPointObjects) / sizeof(OBJECT_INFO);
    }
    else
    {
        dwNumMSDEndpoints = sizeof(pDev->OSEndPointObjects) / sizeof(OBJECT_INFO);
    }

     /* If supplied memory is not sufficent then we return error */
     if(pEnumEpInfo->dwEpTotalEntries < dwNumMSDEndpoints)
     {
         /* Set the total required entries */
         pEnumEpInfo->dwEpProcessedEntries = dwNumMSDEndpoints;
         Result = ADI_DEV_RESULT_NO_MEMORY;
         return(Result);
     }

     for(i=0; i< dwNumMSDEndpoints; i++)
     {
        if(pDev->busSpeed==ADI_USB_DEVICE_SPEED_HIGH)
        {
        /* Get the associated endpoint descriptor */
        pEndpointD = ((PENDPOINT_OBJECT)(pDev->EndPointObjects[i].pObj))->pEndpointDesc;

        /* Get the endpoint ID */
        pMSDEpInfo->dwEndpointID = pDev->EndPointObjects[i].ID;
        }
        else
        {
            /* Get the associated endpoint descriptor */
            pEndpointD = ((PENDPOINT_OBJECT)(pDev->OSEndPointObjects[i].pObj))->pEndpointDesc;

            /* Get the endpoint ID */
            pMSDEpInfo->dwEndpointID = pDev->OSEndPointObjects[i].ID;            
        } 

        /* Set endpoint direction */
        pMSDEpInfo->eDir = ((pEndpointD->bEndpointAddress >> 7) & 0x1) ? USB_EP_IN : USB_EP_OUT ;

        /* Set the endpoint attributes */
        pMSDEpInfo->bAttributes = pEndpointD->bAttributes;

        pMSDEpInfo++;

     }
   return(Result);
}

/*********************************************************************
*
*   Function:       ConfigureEndpoints
*
*   Description:
*
*********************************************************************/

__ADI_USB_MSD_SECTION_CODE
u32 ConfigureEndpoints(void)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    ADI_ENUM_ENDPOINT_INFO EnumEpInfo;
    static ADI_USB_APP_EP_INFO MSDEpInfo[7] = {0};

    EnumEpInfo.pUsbAppEpInfo = &MSDEpInfo[0];
    EnumEpInfo.dwEpTotalEntries = sizeof(MSDEpInfo)/sizeof(ADI_USB_APP_EP_INFO);
    EnumEpInfo.dwEpProcessedEntries = 0;

    Result = EnumerateEndpoints(&EnumEpInfo);

    if (Result != ADI_DEV_RESULT_SUCCESS)
    {
        return ADI_DEV_RESULT_FAILED;
    }

    if(MSDEpInfo[0].eDir == USB_EP_IN)
    {
        adi_usb_msd_IN_EP  = MSDEpInfo[0].dwEndpointID;
        adi_usb_msd_OUT_EP = MSDEpInfo[1].dwEndpointID;
    }
    else
    {
        adi_usb_msd_OUT_EP  =  MSDEpInfo[0].dwEndpointID;
        adi_usb_msd_IN_EP   =  MSDEpInfo[1].dwEndpointID;
    }

    return Result;
}

/*********************************************************************
*
*   Function:       pddControl
*
*   Description:    List of I/O control commands to the driver
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static u32 adi_pdd_Control(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    u32 Command,                    /* command ID */
    void *pArg                      /* pointer to argument */
)
{
    ADI_USB_DEV_DEF   *pDevice;
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
    PDEVICE_DESCRIPTOR pDevDesc;
    signed char iSerialNumber = 0;

    switch(Command)
    {
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
        case (ADI_DEV_CMD_SET_DATAFLOW):
        case (ADI_USB_CMD_ENABLE_USB):
            /* Send device specific command peripheral driver */
            Result = adi_dev_Control(pDevice->PeripheralDevHandle,
                                Command,
                                (void*)pArg);
        break;

        case (ADI_USB_CMD_CLASS_SET_CONTROLLER_HANDLE):
               pDevice->PeripheralDevHandle =(ADI_DEV_DEVICE_HANDLE)pArg;
        break;

        case ADI_USB_CMD_CLASS_GET_CONTROLLER_HANDLE:
        {
             u32 *pArgument = (u32*)pArg;
                 *pArgument = (u32)pDevice->PeripheralDevHandle;
        }
        break;

        case ADI_USB_CMD_CLASS_CONFIGURE:
             Result =  DevMassStorageConfigure();
        break;

        case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:
        break;

        case ADI_USB_MSD_CMD_SET_MEMORY_SIZE:
            adi_usb_msd_max_memory_size = (u32)pArg;
        break;

        case ADI_USB_MSD_CMD_SCSI_INIT:
            Result = SCSI_Init();
        break;

        case ADI_USB_MSD_CMD_SET_BUFFER:
        {
           u32 bAlreadySet = pDevice->pClientBuffer != NULL;
           pDevice->pClientBuffer = (ADI_DEV_1D_BUFFER*)pArg;
           /* Normalize buffer size to an integral number of media sectors */
           u32 nSectPerClientBuffer = pDevice->pClientBuffer->ElementCount/pDevice->MediaInfo.SectorSize;
           if (nSectPerClientBuffer) 
           {
               /* save aside element count as this is required for each PID access */
               pDevice->nClientBufferElementCount = nSectPerClientBuffer * pDevice->MediaInfo.SectorSize;
               
               /* and set up buffer for first CBW */
               if (!bAlreadySet && adi_usb_msd_device_ready) {
                   SetupEPBuffer((ADI_DEV_BUFFER *)pArg, adi_usb_msd_OUT_EP, CBW_LENGTH);
               }
           }
           else {
               Result = ADI_DEV_RESULT_NO_MEMORY;
           }
           
        }                         
        break;

        case ADI_USB_MSD_CMD_IS_DEVICE_CONFIGURED:
            if(adi_usb_msd_device_ready == TRUE)
            {
                Result = ConfigureEndpoints();

                if (Result != ADI_DEV_RESULT_SUCCESS)
                {
                    *((u32 *)pArg) = FALSE;
                    break;
                }

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
        break;

        case ADI_USB_MSD_CMD_SET_VID:
            pDevDesc = adi_usb_GetDeviceDescriptor();
            if (!pDevDesc)
            {
                Result = ADI_DEV_RESULT_FAILED;
                break;
            }

            pDevDesc->wIdVendor = (u32)pArg;
        break;

        case ADI_USB_MSD_CMD_SET_PID:
            pDevDesc = adi_usb_GetDeviceDescriptor();
            if (!pDevDesc)
            {
                Result = ADI_DEV_RESULT_FAILED;
                break;
            }

            pDevDesc->wIdProduct = (u32)pArg;
        break;

        case ADI_USB_MSD_CMD_SET_SERIAL_NUMBER:
            pDevDesc = adi_usb_GetDeviceDescriptor();
            if (!pDevDesc)
            {
                Result = ADI_DEV_RESULT_FAILED;
                break;
            }

            iSerialNumber = adi_usb_CreateString((char *)pArg);
            if ((iSerialNumber > 0) && (iSerialNumber < USB_MAX_STRINGS))
                pDevDesc->bISerialNumber = iSerialNumber;
        break;

        case ADI_USB_MSD_CMD_ENUMERATE_ENDPOINTS:
        {
            ADI_ENUM_ENDPOINT_INFO EnumEpInfo;
            ADI_USB_APP_EP_INFO MSDEpInfo[2] = {0};

            EnumEpInfo.pUsbAppEpInfo = &MSDEpInfo[0];
            EnumEpInfo.dwEpTotalEntries = sizeof(MSDEpInfo)/sizeof(ADI_USB_APP_EP_INFO);
            EnumEpInfo.dwEpProcessedEntries = 0;

            Result = EnumerateEndpoints(&EnumEpInfo);

            if (Result != ADI_DEV_RESULT_SUCCESS)
               break;

            if(MSDEpInfo[0].eDir == USB_EP_IN)
            {
                adi_usb_msd_IN_EP  = MSDEpInfo[0].dwEndpointID;
                adi_usb_msd_OUT_EP = MSDEpInfo[1].dwEndpointID;
            }
            else
            {
                adi_usb_msd_OUT_EP  =  MSDEpInfo[0].dwEndpointID;
                 adi_usb_msd_IN_EP   =  MSDEpInfo[1].dwEndpointID;
            }
        }
        break;
 
        case ADI_USB_MSD_CMD_REGISTER_FSSPID:
            RAW_PID_DeviceHandle = (ADI_DEV_DEVICE_HANDLE)pArg;
            Result = adi_rawpid_RegisterPID ( RAW_PID_DeviceHandle, pDevice );
            if(Result == ADI_DEV_RESULT_SUCCESS) 
            {
                adi_msd_HDDetected = TRUE;
                adi_rawpid_ClientCallbackFn = PidCallbackFunction;
                Result = ADI_DEV_RESULT_SUCCESS;
            }
            else
            {
                adi_msd_HDDetected = FALSE;
                Result = ADI_DEV_RESULT_FAILED;
            }
        break;

        case ADI_USB_MSD_CMD_DEREGISTER_FSSPID:
            adi_rawpid_DeregisterPID( RAW_PID_DeviceHandle );
            adi_msd_HDDetected = FALSE;
            adi_rawpid_ClientCallbackFn = NULL;            
            RAW_PID_DeviceHandle = NULL;
        break;

        default:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
        break;
    }

    return(Result);
}

/**************************************************************************
 *
 * USB Mass Storage Class driver entry point (Device Mode)
 *
 **************************************************************************/
__ADI_USB_MSD_SECTION_DATA
ADI_DEV_PDD_ENTRY_POINT ADI_USB_Device_MassStorageClass_Entrypoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

