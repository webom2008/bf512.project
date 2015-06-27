/*********************************************************************************
 * Copyright(c) 2006 Analog Devices, Inc. All Rights Reserved.
 *
 * This software is proprietary and confidential.  By using this software you agree
 * to the terms of the associated Analog Devices License Agreement.
 *
 * $RCSfile: adi_usb_core.c,v $
 * $Revision: 2386 $
 * $Date: 2010-03-25 17:26:08 -0400 (Thu, 25 Mar 2010) $
 *
 * Description:
 *           USB Core Layer provides services to the class and peripheral drivers.
 *           These services include the management of device configuration, interfaces
 *           and endpoints of the device.
 *
 *********************************************************************************/

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_CORE_SECTION_CODE    section("adi_usb_core_code")
#define __ADI_USB_CORE_SECTION_DATA    section("adi_usb_core_data")
#else
#define __ADI_USB_CORE_SECTION_CODE
#define __ADI_USB_CORE_SECTION_DATA
#endif 


#include <drivers\usb\usb_core\adi_usb_objects.h>
#include <drivers\usb\usb_core\adi_usb_debug.h>
#include <drivers\usb\usb_core\adi_usb_core.h>
#include <string.h>
#include <drivers\usb\usb_core\adi_usb_logevent.h>
#include <drivers\usb\usb_core\adi_usb_ids.h>
#include <drivers\adi_dev.h>

/* ********************************************* */
/* uncomment the following macro to restore the original single alternate interface model */
#define _FIX_FOR_MULTIPLE_ALTERNATE_INTERFACES
/* ********************************************* */

#ifdef _USB_MULTI_THREADED_
#include <kernel_abs.h>
#endif /* multithreaded applicaiton */

/* Macros */
#define NUM_TX_CONTROL_BUFFERS       (4)    /* Number of EP0 Tx Command buffers */
#define NUM_RX_CONTROL_BUFFERS       (4)    /* Number of EP0 Rx Command buffers */

/* static prototypes */
static void ResetDevices(void);
static s32_t PeripheralTxCompleteHandler(void *AppHandle,unsigned int wEvent,void *pArg);

/* Objects */
__ADI_USB_CORE_SECTION_DATA
static DEVICE_OBJECT    device_objects[USB_MAX_DEVICES];
__ADI_USB_CORE_SECTION_DATA
static CONFIG_OBJECT    config_objects[USB_MAX_CONFIGURATIONS];
__ADI_USB_CORE_SECTION_DATA
static INTERFACE_OBJECT interface_objects[USB_MAX_INTERFACES];
__ADI_USB_CORE_SECTION_DATA
static INTERFACE_OBJECT alt_interface_objects[USB_MAX_INTERFACES];
__ADI_USB_CORE_SECTION_DATA
static ENDPOINT_OBJECT  endpoint_objects[USB_MAX_ENDPOINTS];

/* Descriptors */
__ADI_USB_CORE_SECTION_DATA
static DEVICE_DESCRIPTOR        device_descriptors[USB_MAX_DEVICES];
__ADI_USB_CORE_SECTION_DATA
static CONFIGURATION_DESCRIPTOR config_descriptors[USB_MAX_CONFIGURATIONS];
__ADI_USB_CORE_SECTION_DATA
static INTERFACE_DESCRIPTOR     interface_descriptors[USB_MAX_INTERFACES];
__ADI_USB_CORE_SECTION_DATA
static INTERFACE_DESCRIPTOR     alt_interface_descriptors[USB_MAX_INTERFACES];
__ADI_USB_CORE_SECTION_DATA
static ENDPOINT_DESCRIPTOR      endpoint_descriptors[USB_MAX_ENDPOINTS];
__ADI_USB_CORE_SECTION_DATA
static PSTRING_DESCRIPTOR       ptr_string_descriptors[USB_MAX_STRINGS];
__ADI_USB_CORE_SECTION_DATA
static u32_t string_index = 0;  /* number of strings */
/* note, STRING_DESCRIPTORs are created dynamically so we use pointers here */

#define EP0_BUFFER_MAX          256 

/* Maximum possible data that can be sent or received on EP0 with single read or write
 * request on EP0. Typically it would the total configuration size in bytes.
 */
#define EP0_MAX_SETUP_DATA_SIZE 1024

__ADI_USB_CORE_SECTION_DATA
static EP_ZERO_BUFFER_INFO  gEpZeroBufferInfo = { CORE_CONTROL_CODE, EP0_BUFFER_MAX };

/* Ep zero buffers */
__ADI_USB_CORE_SECTION_DATA
static ADI_DEV_1D_BUFFER TxControlBuffers[NUM_TX_CONTROL_BUFFERS];
__ADI_USB_CORE_SECTION_DATA
static ADI_DEV_1D_BUFFER RxControlBuffers[NUM_RX_CONTROL_BUFFERS];

/* use to initialize new device descriptors, then user can customize them as needed */
__ADI_USB_CORE_SECTION_DATA
static DEVICE_DESCRIPTOR        default_device_descriptor =
                                {
                                    sizeof(DEVICE_DESCRIPTOR),  /* Descriptor size in bytes */
                                    TYPE_DEVICE_DESCRIPTOR,     /* The constant DEVICE 01h  */
                                    USB_SPEC_2_0,               /* USB Specification Release Number (BCD) */
                                    0x0,                        /* Class code */
                                    0x0,                        /* Subclass code */
                                    0x0,                        /* Protocol code */
                                    EP0_MAX_PACKET_SIZE_HIGH64, /* Maximum packet size for Endpoint 0 */
                                    USB_VID_ADI_TOOLS,          /* Vendor ID */
                                    USB_PID_ADI_UNDEFINED,      /* Product ID */
                                    0x0100,                     /* Device release number */
                                    0,                          /* Index of string descriptor for manufacturer */
                                    0,                          /* Index of string descriptor for product */
                                    0,                          /* Index of string descriptor contains serial no# */
                                    0x1                         /* Number of possible configurations */
                                };

__ADI_USB_CORE_SECTION_DATA
static DEVICE_QUALIFIER_DESCRIPTOR DefaultDeviceQualifierDescriptor =
                                {
                                    sizeof(DEVICE_QUALIFIER_DESCRIPTOR),
                    TYPE_DEVICEQUALIFIER_DESCRIPTOR,
                                    USB_SPEC_2_0,               /* USB Specification Release Number (BCD) */
                    USB_VENDOR_SPECIFIC_CLASS_CODE,
                    0x00,
                    0x00,
                    0x40,
                    0x01,
                    0x00
                };
/* strings */
__ADI_USB_CORE_SECTION_DATA
static STRING_DESCRIPTOR DefaultLanguageIdDescriptor[] = {0x04, 0x03, 0x09, 0x04};

__ADI_USB_CORE_SECTION_DATA
static USB_CORE_DATA usb_core_data = {0};
__ADI_USB_CORE_SECTION_DATA
USB_CORE_DATA *pUsbCoreData = &usb_core_data;

/* Static memory block to hold the current configuration */
/* TODO: Check if we need to allocate dynamically incase  larger configurations */
__ADI_USB_CORE_SECTION_DATA
static char ConfigMemoryArea[2048] = {0};

__ADI_USB_CORE_SECTION_DATA
static bool    gbConfigSwapped    = false;

__ADI_USB_CORE_SECTION_DATA
static bool gbFullSpeed        = false;

s32_t SendEpZeroDataEx(DEVICE_OBJECT *pDevO,u8_t *pData,int length);
s32_t HostModeTransmitEpZeroBuffer(ADI_DEV_1D_BUFFER *pBuffer);

static void SetAltInterfaceToDefault(PDEVICE_OBJECT pDevO);

/*********************************************************************************
 * adi_usb_CreateDevice API.
 * 
 * Description:
 *
 * This API is used to create a device object.Device object identifies a physical 
 * device. Only one device object exists for one peripheral device.Device Object 
 * is created by the peripheral control driver or OTG host driver.In multi-threaded 
 * environment this function is expected to be called only from thread-domain. So 
 * there is no need to mask off interrupts.These APIs should not be called from 
 * interrupt level or ISRs.
 *
 * array index --> ID (zero based Device index,ID)
 *
 * @param pDv is a double pointer to DEVICE_OBJECT
 *
 * @return The ID of the created device object upon success.
 *         pDv pointer is set to point to the object itself.
 *         Upon failure the API returns -1.
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_CreateDevice(PDEVICE_OBJECT *pDv)
{
int i=0,id=ADI_USB_RESULT_INVALID_ID;
DEVICE_OBJECT *pCurDevO = &device_objects[0];

    USB_DEBUG_PRINT("Invalid Input pDv",
            (USB_DEBUG_FLAGS & USB_DEVICE_DEBUG),
            (pDv == NULL));

    #ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    for(i=0; i< USB_MAX_DEVICES; i++)
    {
        if(pCurDevO && (-1 == pCurDevO->ID) )
        {
            /* set entire device object structure to zeros */
            memset(pCurDevO,0,sizeof(DEVICE_OBJECT));

            pCurDevO->ID = i; /* set device ID */

            /* initialize the device descriptor with default values, the user
               can customize as needed */
            pCurDevO->pDeviceDesc = &device_descriptors[i];
            memset(pCurDevO->pDeviceDesc,0,sizeof(DEVICE_DESCRIPTOR));
            memcpy(pCurDevO->pDeviceDesc, &default_device_descriptor, sizeof(DEVICE_DESCRIPTOR));
            pCurDevO->pDeviceQuaDesc = &DefaultDeviceQualifierDescriptor;

            pCurDevO->pEndpointZeroObj = &endpoint_objects[0];
            pCurDevO->ppStringDescriptors = ptr_string_descriptors;
            pCurDevO->pSetupDataArea = malloc(sizeof(char) * EP0_MAX_SETUP_DATA_SIZE);
            pCurDevO->pAppEpZeroBuffer = NULL;
            
            pCurDevO->nBusSpeed = ADI_USB_DEVICE_SPEED_UNKNOWN;
            
            USB_ASSERT(pCurDevO->pSetupDataArea == NULL);

            /* Store this device object in the input parameter */
            *pDv = pCurDevO;

            if(pUsbCoreData->pDeviceHead == NULL)
                pUsbCoreData->pDeviceHead = pCurDevO;
            else
            {
                DEVICE_OBJECT *pTempDevO=pUsbCoreData->pDeviceHead;
                while(pTempDevO->pNext != NULL)
                    pTempDevO = pTempDevO->pNext;
                pTempDevO->pNext = pCurDevO;
            }
            id = i;
            break;
        }
        else
            pCurDevO++;
    } /* end for */

    #ifdef USB_DEBUG
    if(id < 0)
    {
        USB_DEBUG_PRINT("No more available device objects",
                USB_DEBUG_FLAGS & USB_DEVICE_DEBUG,
                1);
    }
    #endif /* USB_DEBUG */

    #ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    return(id);
}

/*********************************************************************************
 * adi_usb_CreateConfiguration API
 *
 * Description:
 *
 * Creates Configuration Object and returns ID. Configuration object is
 * typically created by the upper level class drivers. Configuration ID which is
 * used as bConfigurationValue must be greater than zero. Thats the reason
 * the IDs starts from 1. In case of configuration objects
 * an object index is not directly correspond to its ID. (ID value = array
 * index +1).
 *
 * bConfigurationValue = 0 --> device is in Address State
 * bConfigurationValue > 0 --> device is configured.
 *
 * array index -1 = ID (zero based Configuration index, 1 based ID)
 *
 * @param pCf is a double pointer to CONFIG_OBJECT
 *
 * @return The ID of the created configuration object upon success.
 *         pCf pointer is set to point to the object itself.
 *         Upon failure the API returns -1.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_CreateConfiguration(PCONFIG_OBJECT *pCf)
{
int i=0,id=ADI_USB_RESULT_INVALID_ID;
CONFIG_OBJECT *pCurConfigO = &config_objects[0];

    USB_DEBUG_PRINT("Invalid Input pCf",
            USB_DEBUG_FLAGS & USB_CONFIG_DEBUG,
            (pCf == NULL));

    #ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    for(i=0; i< USB_MAX_CONFIGURATIONS; i++)
    {
        if(pCurConfigO && (-1 == pCurConfigO->ID) )
        {
            memset(pCurConfigO,0,sizeof(CONFIG_OBJECT));
            /* id is one more than the actual index. */
            pCurConfigO->ID = i+1;
            pCurConfigO->pConfigDesc = &config_descriptors[i];
            memset(pCurConfigO->pConfigDesc,0,sizeof(CONFIGURATION_DESCRIPTOR));
            pCurConfigO->pConfigDesc->bDescriptorType = TYPE_CONFIGURATION_DESCRIPTOR;
            *pCf = pCurConfigO;
            id   = i+1;
            break;
        }
        else
            pCurConfigO++;
    }

    #ifdef USB_DEBUG
    if(id <0)
    {
    USB_DEBUG_PRINT("No more available configuration objects",
            USB_DEBUG_FLAGS & USB_CONFIG_DEBUG,
                1);
    }
    #endif /* USB_DEBUG */

    #ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    return(id) ;
}

/*********************************************************************************
 * adi_usb_CreateInterface API
 *
 * Description:
 *
 * Creates Interface Object and returns ID. Used by the upper level class
 * drivers. CreateInterface API by default creates and sets up one alternate interface.
 * Elements of alternate interface object were initialized to zero. Unless class
 * drivers use an alternate interface these elements should not be changed.
 * If an interface is deleted the alternate interface also gets deleted.
 *
 * array index = ID  (zero based Interface index,ID)
 *
 * @param pIf is a double pointer to INTERFACE_OBJECT
 *
 * @return The ID of the created configuration object upon success.
 *         pCf pointer is set to point to the object itself.
 *         Upon failure the API returns -1.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_CreateInterface(PINTERFACE_OBJECT *pIf)
{
int i=0,id=ADI_USB_RESULT_INVALID_ID;
INTERFACE_OBJECT *pCurInterfaceO = &interface_objects[0];
INTERFACE_OBJECT *pAltInterfaceO = NULL;

    USB_DEBUG_PRINT("Invalid Input pIf",
            USB_DEBUG_FLAGS & USB_INTERFACE_DEBUG,
            (pIf == NULL));

    #ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    for(i=0; i< USB_MAX_INTERFACES; i++)
    {
        if(pCurInterfaceO && (-1 == pCurInterfaceO->ID) )
        {
            memset(pCurInterfaceO,0,sizeof(INTERFACE_OBJECT));
            pCurInterfaceO->ID = i;
            pCurInterfaceO->pInterfaceDesc = &interface_descriptors[i];
            memset(pCurInterfaceO->pInterfaceDesc,0,sizeof(INTERFACE_DESCRIPTOR));
            pCurInterfaceO->pInterfaceDesc->bDescriptorType = TYPE_INTERFACE_DESCRIPTOR;

            *pIf = pCurInterfaceO;
            /* setup alternate interface parameters */
#ifndef _FIX_FOR_MULTIPLE_ALTERNATE_INTERFACES
            pCurInterfaceO->pAltInterfaceObj= &alt_interface_objects[i];
            pAltInterfaceO = pCurInterfaceO->pAltInterfaceObj;
            memset(pAltInterfaceO,0,sizeof(INTERFACE_OBJECT));
            pAltInterfaceO->pInterfaceDesc = &alt_interface_descriptors[i];
            memset(pAltInterfaceO->pInterfaceDesc,0,sizeof(INTERFACE_DESCRIPTOR));
            /* alternate interface id is same as the interface id */
            pAltInterfaceO->ID = i;
#endif
            id = i;
            break;
        }
        else
            pCurInterfaceO++;
    }

    #ifdef USB_DEBUG
    if(id < 0)
    {
        USB_DEBUG_PRINT("No more available interaface objects",
                USB_DEBUG_FLAGS & USB_CONFIG_DEBUG,
                1);
    }
    #endif  /* USB_DEBUG */

    #ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    return(id);
}

#ifdef _FIX_FOR_MULTIPLE_ALTERNATE_INTERFACES
/*********************************************************************************
 * adi_usb_CreateInterfaceObject API
 *
 * Description:
 *
 * Creates Interface Object. Used by the upper level class
 * drivers. 
 *
 * @param pIf is a double pointer to INTERFACE_OBJECT
 *
 * @return 0 on success, -1 on failure.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_CreateInterfaceObject(PINTERFACE_OBJECT *pIf)
{
    s32_t Result = -1;
    
    PINTERFACE_OBJECT pAltInterfaceObj = (PINTERFACE_OBJECT)malloc(sizeof(INTERFACE_OBJECT));
    if (pAltInterfaceObj) {
        memset(pAltInterfaceObj, 0, sizeof(INTERFACE_OBJECT));    
        pAltInterfaceObj->pInterfaceDesc = (INTERFACE_DESCRIPTOR*)malloc(sizeof(INTERFACE_DESCRIPTOR));
        if ( pAltInterfaceObj->pInterfaceDesc ) {
            memset(pAltInterfaceObj->pInterfaceDesc,0,sizeof(INTERFACE_DESCRIPTOR));
            Result = 0;
        }
    }
    *pIf = pAltInterfaceObj;

    return Result;
}

#endif

/*********************************************************************************
 * adi_usb_CreateEndPoint API
 *
 * Description:
 *
 * Creates Endpoint Object and returns ID. This API is used by the class drivers.
 * These endpoint objects are logical endpoints. Once host selects a configuration 
 * or interface the physical endpoints will be configured during runtime.
 *
 * array index = ID  (zero based Endpoint index,ID)
 *
 * @param pEp is a double pointer to ENDPOINT_OBJECT
 *
 * @param pLogicalEpInfo pointer to the logical endpoint information
 *
 * @return The ID of the created endpoint object upon success.
 *         pCf pointer is set to point to the object itself.
 *         Upon failure the API returns -1.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_CreateEndPoint(PENDPOINT_OBJECT *pEp,LOGICAL_EP_INFO *pLogicalEpInfo)
{
int start_index =1;

#if defined(__ADSPBF548__) || defined(__ADSPBF527__) || defined(__ADSPBF526__)
   USB_ASSERT(pLogicalEpInfo == NULL);
   start_index = (pLogicalEpInfo->dwMaxEndpointSize >128) ? 5:1;
#endif /* __ADSPBF548__ */

int i=0,id=ADI_USB_RESULT_INVALID_ID;
ENDPOINT_OBJECT *pCurEndpointO = &endpoint_objects[start_index];

    USB_DEBUG_PRINT("Invalid Input pEp",
            USB_DEBUG_FLAGS & USB_EP_DEBUG,
            (pEp == NULL));

    #ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    /* endpoint_objects[0] is control endpoint so starts at 1 */

    for(i=start_index; i< USB_MAX_ENDPOINTS; i++)
    {
        if(pCurEndpointO && (-1 == pCurEndpointO->ID) )
        {
            memset(pCurEndpointO,0,sizeof(ENDPOINT_OBJECT));
            pCurEndpointO->ID = i;
            pCurEndpointO->pEndpointDesc = &endpoint_descriptors[i];
            memset(pCurEndpointO->pEndpointDesc,0,sizeof(ENDPOINT_DESCRIPTOR));
            pCurEndpointO->pEndpointDesc->bDescriptorType = TYPE_ENDPOINT_DESCRIPTOR;
            memset(&pCurEndpointO->EPInfo,0,sizeof(USB_EP_INFO));
            *pEp = pCurEndpointO;
            id = i;
            break;
        }
        else
            pCurEndpointO++;
    } /* end for */

    #ifdef USB_DEBUG
    if(id < 0)
    {
    USB_DEBUG_PRINT("No more available endpoint objects",
            USB_DEBUG_FLAGS & USB_EP_DEBUG,
            1);
    }
    #endif /* USB_DEBUG */

    #ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    return(id);
}


/*********************************************************************************
 * adi_usb_CreateString API
 *
 * Description:
 *
 * Creates a string descriptor from a user supplied NULL-terminated ASCII
 * string.malloc is used to allocate memory to store the converted UNICODE
 * string as well as the standard string descriptor itself.
 *
 * @param pszAsciiString is pointer to the users ASCII string.
 *
 * @return The string index for use in other descriptors.
 *         Upon failure (no string index's are available) then the API returns -1.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_CreateString(const char *pszAsciiString)
{
PSTRING_DESCRIPTOR pStringDesc = NULL;  /* string descriptor pointer */
char *ptr;                              /* byte pointer */
int a_length = 0;                       /* ASCII length */
int sd_length = 0;                      /* string descriptor length */
int i = 0;                              /* index */


    /* if this is the first time we're called we should init the LanguageID descriptor,
       otherwise if there are no strings created we do not need a language ID descriptor */
    if ( string_index == 0 )
    {
        ptr_string_descriptors[string_index++] = DefaultLanguageIdDescriptor;
    }

    /* if the string is NULL there's nothing more to do */
    if ( NULL == pszAsciiString )
        return ADI_USB_RESULT_INVALID_INPUT;

    /* if there's no more strings available return -1 */
    if ( string_index >= USB_MAX_STRINGS)
        return ADI_USB_RESULT_INVALID_ID;

    /* get the length of the ASCII string */
    a_length = strlen((const char*)pszAsciiString);

    /* get the length of the string descriptor which is the length of the
       ASCII string UNICODE plus the other fields */
    sd_length = (a_length * 2) + sizeof(STRING_DESCRIPTOR);

    /* allocate storage for the string descriptor */
    pStringDesc = (PSTRING_DESCRIPTOR)malloc(sd_length);

    USB_ASSERT(pStringDesc == NULL);

    /* if there was an error allocating memory we must fail */
    if ( NULL == pStringDesc )
        return ADI_USB_RESULT_INVALID_ID;

    /* fill in the other fields */
    pStringDesc->bLength = sd_length;
    pStringDesc->bDescriptorType = TYPE_STRING_DESCRIPTOR;

    /* create Unicode string from ASCII string */
    ptr = (char*)pStringDesc + sizeof(STRING_DESCRIPTOR);
    for ( i = 0; i < a_length; i++)
    {
        *(ptr++) = *(pszAsciiString++);
        *(ptr++) = 0;
    }

    /* save a pointer to this string descriptor */
    ptr_string_descriptors[string_index] = pStringDesc;

    /* return the string index */
    return string_index++;
}

/*********************************************************************************
 * adi_usb_AttachConfiguration
 *
 * Description:
 *
 * Attaches configuration object to a device object.These APIs are used by
 * the class drivers to construct various class configurations to the device.
 *
 * @param wDeviceID is the device object id returnd by the adi_usb_CreateDevice API
 * @param wConfigID is the configuration object id returned by the 
 *        adi_usb_CreateConfiguration API
 *
 * @return void
 *
 * @see adi_usb_CreateDevice
 * @see adi_usb_CreateConfiguration
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_AttachConfiguration(const s32_t wDeviceID,const s32_t wConfigID)
{
DEVICE_OBJECT *pCurDevO;
CONFIG_OBJECT *pCurCfgO;

    #ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    USB_DEBUG_PRINT("Invalid device id",
            USB_DEBUG_FLAGS & USB_CONFIG_DEBUG,
            ( (wDeviceID < 0) || (wDeviceID > USB_MAX_DEVICES)));

    USB_DEBUG_PRINT("Invalid config id",
            USB_DEBUG_FLAGS & USB_CONFIG_DEBUG,
            ( ((wConfigID-1) < 0) || (wConfigID > USB_MAX_CONFIGURATIONS)));

    pCurDevO = &device_objects[wDeviceID];
    pCurCfgO = pCurDevO->pConfigObj;

    if(NULL == pCurCfgO)
    {
        pCurDevO->pConfigObj = &config_objects[wConfigID-1];
    }
    else /* more than one configuration for the device */
    {
        while(pCurCfgO->pNext != NULL)
           pCurCfgO = pCurCfgO->pNext;

        pCurCfgO->pNext = &config_objects[wConfigID-1];
    }

    /* Get the configuration object */
    pCurCfgO = &config_objects[wConfigID-1];

    /* If application did not setup the other speed configuration then by
     * default we use the same configuration.
     */
    if(pCurCfgO->pOtherSpeedConfigObj == NULL) {
        pCurCfgO->pOtherSpeedConfigObj= pCurCfgO;
        pCurDevO->pDeviceQuaDesc->bNumConfigurations = 0;
    } else {
        pCurDevO->pOtherSpeedConfigObj = pCurCfgO->pOtherSpeedConfigObj;
        pCurDevO->pDeviceQuaDesc->bNumConfigurations = 1;
        pCurDevO->pDeviceQuaDesc->bMaxPacketSize0 = 64;
    }
    #ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    return(ADI_USB_RESULT_SUCCESS);
}

/*********************************************************************************
 * adi_usb_AttachOtherSpeedConfiguration
 *
 * Description:
 * Attaches Other Speed configuration object to a configuration object.These APIs 
 * are used by the class drivers to construct various class configurations.
 *
 * @param wConfigID is the configuration object id returned by the 
 *        adi_usb_CreateConfiguration API
 * @param wOtherSpeedConfigID is the Other speed configuration object id returnd 
 *        by the adi_usb_CreateConfiguration API
 *
 * @return void
 *
 * @see adi_usb_CreateConfiguration
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_AttachOtherSpeedConfiguration(const s32_t wConfigID,const s32_t wOtherSpeedConfigID)
{
CONFIG_OBJECT *pCurCfgO,*pCurOtherSpeedCfgO;

    #ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    USB_DEBUG_PRINT("Invalid device id",
            USB_DEBUG_FLAGS & USB_CONFIG_DEBUG,
            ( (wConfigID < 0) || (wOtherSpeedConfigID > USB_MAX_DEVICES)));

    USB_DEBUG_PRINT("Invalid config id",
            USB_DEBUG_FLAGS & USB_CONFIG_DEBUG,
            ( ((wConfigID-1) < 0) || (wConfigID > USB_MAX_CONFIGURATIONS)));

    pCurCfgO = &config_objects[wConfigID-1];
    pCurOtherSpeedCfgO = &config_objects[wOtherSpeedConfigID-1];

    USB_ASSERT(pCurCfgO == NULL);
    USB_ASSERT(pCurOtherSpeedCfgO == NULL);

    /* Set the other speed configuration object */
    pCurCfgO->pOtherSpeedConfigObj = pCurOtherSpeedCfgO;
    pCurOtherSpeedCfgO->pOtherSpeedConfigObj = NULL;

    #ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    return(ADI_USB_RESULT_SUCCESS);
}

/*********************************************************************************
 * adi_usb_DetachConfiguration API
 *
 * Detach interface from a configuration, all associated endpoints,
 * any interfaces and alternate interfaces will also be released.
 *
 * @param wDeviceID is the device object id returned by the adi_usb_CreateDevice API
 *
 * @param wConfigID is the configuration object id returned by the 
 *        adi_usb_CreateConfiguration API
 *
 * @return Deleted Configuration object ID upon success and -1 upon failure
 * @see adi_usb_CreateDevice
 * @see adi_usb_CreateConfiguration
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_DetachConfiguration(const s32_t wDeviceID,const s32_t wConfigID)
{
   return 0;
}

/*********************************************************************************
 * adi_usb_AttachInterface
 *
 * Attach interface to a configuration. This API is used by the class drivers for
 * attaching their interface to a configuration.
 *
 * @param wConfigID is the configuration object id returned by the 
 *        adi_usb_CreateConfiguration API
 *
 * @param wInterfaceID is the interface object id returned by the 
 *        adi_usb_CreateInterface API
 *
 * @see adi_usb_CreateConfiguration
 * @see adi_usb_CreateInterface
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_AttachInterface(const s32_t wConfigID,const s32_t wInterfaceID)
{
CONFIG_OBJECT *pCurConfigO;
INTERFACE_OBJECT *pCurIfceO;

    USB_DEBUG_PRINT("Invalid config id",
            USB_DEBUG_FLAGS & USB_INTERFACE_DEBUG,
            ( ((wConfigID-1) < 0) || (wConfigID > USB_MAX_CONFIGURATIONS)));

    USB_DEBUG_PRINT("Invalid interface id",
            USB_DEBUG_FLAGS & USB_INTERFACE_DEBUG,
            ( (wInterfaceID < 0) || (wInterfaceID > USB_MAX_INTERFACES)));

    #ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    pCurConfigO = &config_objects[wConfigID-1];
    pCurIfceO   = pCurConfigO->pInterfaceObj;

    if(NULL == pCurIfceO)
    {
        pCurConfigO->pInterfaceObj = &interface_objects[wInterfaceID];
    }
    else /* more than one interface active */
    {
        while(pCurIfceO->pNext != NULL)
            pCurIfceO = pCurIfceO->pNext;
        pCurIfceO->pNext = &interface_objects[wInterfaceID];
    }

    #ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    return(ADI_USB_RESULT_SUCCESS);
}

/*********************************************************************************
 * adi_usb_DetachInterface
 *
 * Detach interface and its associated alternate interface and all the
 * associated endpoints will be released.
 *
 * @param wConfigID is the configuration object id returned by the 
 *        adi_usb_CreateConfiguration API
 * @param wInterfaceID is the interface object id returned by the 
 *        adi_usb_CreateInterface API
 *
 * @return Deleted interface ID upon success and -1 upon failure
 *
 * @see adi_usb_CreateConfiguration
 * @see adi_usb_CreateInterface
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_DetachInterface(const s32_t wConfigID,const s32_t wInterfaceID)
{
    return 0;
}

/*********************************************************************************
 * adi_usb_AttachEndpoint
 *
 * Description:
 *
 * Attach Endpoint to an interface. This endpoint is the logical endpoint.
 * The logical endpoint gets binded to the physical endpoint once the host
 * selects a configuration and an interface.
 *
 * @param wInterfaceID is the interface object id returned by the 
 *        adi_usb_CreateInterface API
 * @param wEndpointID is the endpoint object id returned by the 
 *        adi_usb_CreateEndPoint API
 *
 * @see adi_usb_CreateInterface
 * @see adi_usb_CreateEndPoint
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_AttachEndpoint(const s32_t wInterfaceID,const s32_t wEndpointID)
{
    INTERFACE_OBJECT *pCurInterfaceO;
    ENDPOINT_OBJECT  *pCurEpO;
    s32_t            AlternateInterfaceID = ((wInterfaceID >> 24) & 0xff);


    USB_DEBUG_PRINT("Invalid interface id",
            USB_DEBUG_FLAGS & USB_EP_DEBUG,
            (((wInterfaceID & 0xff) < 0) || ((wInterfaceID & 0xff) > USB_MAX_INTERFACES)));

    USB_DEBUG_PRINT("Invalid endpoint id",
            USB_DEBUG_FLAGS & USB_EP_DEBUG,
            ( (wEndpointID < 0) || (wEndpointID > USB_MAX_ENDPOINTS)));

    #ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    if(!AlternateInterfaceID) {
        pCurInterfaceO = &interface_objects[wInterfaceID&0xff];
    } else { /* TODO: parse through the list of alternate settings. */
         pCurInterfaceO = interface_objects[wInterfaceID&0xff].pAltInterfaceObj;
    }
    pCurEpO = pCurInterfaceO->pEndpointObj;

    #ifdef USB_DEBUG
    { /* Check if an interface has endpoints greater than physical endpoints */
        int NumEps=0;
        PENDPOINT_OBJECT pDebugEpO = pCurInterfaceO->pEndpointObj;
            while(pDebugEpO != NULL)
            {
                NumEps++;
                pDebugEpO = pDebugEpO->pNext;

                USB_DEBUG_PRINT("Interface has more endpoints than the device supports",
                        USB_DEBUG_FLAGS & USB_EP_DEBUG,
                        (NumEps > pUsbCoreData->wNumPhysicalEndpoints));
            }
    }
    #endif /* USB_DEBUG */

    if(NULL == pCurEpO)
    {
        pCurInterfaceO->pEndpointObj = &endpoint_objects[wEndpointID];
    }
    else /* more than one endpoint */
    {
        while(pCurEpO->pNext != NULL)
        {
            pCurEpO = pCurEpO->pNext;
        }
        pCurEpO->pNext = &endpoint_objects[wEndpointID];
    }

    #ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    return(ADI_USB_RESULT_SUCCESS);
}

/*********************************************************************************
 * adi_usb_SetDeviceName
 *
 * Used to set a device name to the device.
 *
 * @param wDeviceID is the device object id returned by the adi_usb_CreateDevice API
 * @param pszDevName device name string
 *
 * @see adi_usb_GetDeviceName
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_SetDeviceName(s32_t wDeviceID,const char *pszDevName)
{
DEVICE_OBJECT *pCurDevO;
    USB_DEBUG_PRINT("Invalid device id",
            USB_DEBUG_FLAGS & USB_DEVICE_DEBUG,
            ( (wDeviceID < 0) || (wDeviceID > USB_MAX_DEVICES)));

    USB_DEBUG_PRINT("Invalid device Name",
            USB_DEBUG_FLAGS & USB_DEVICE_DEBUG,
            (pszDevName == NULL ));

    USB_DEBUG_PRINT("too big device name",
            USB_DEBUG_FLAGS & USB_DEVICE_DEBUG,
            ((strlen(pszDevName) > SIZEOF_DEVICE_NAME )));

    pCurDevO = &device_objects[wDeviceID];
    strcpy(pCurDevO->pDevName,pszDevName);

    return(ADI_USB_RESULT_SUCCESS);
}

/*********************************************************************************
 * adi_usb_GetDeviceName API
 * Used to get the device name.
 *
 * @param wDeviceID is the device object id returned by the adi_usb_CreateDevice API
 * @param pszDevName device name string
 *
 * @return Upon success pszDevName contains the device name.
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_GetDeviceName(s32_t wDeviceID,char *pszDevName)
{
DEVICE_OBJECT *pCurDevO;
    USB_DEBUG_PRINT("Invalid device id",
            USB_DEBUG_FLAGS & USB_DEVICE_DEBUG,
            ( (wDeviceID < 0) || (wDeviceID > USB_MAX_DEVICES)));

    USB_DEBUG_PRINT("Invalid device Name",
            USB_DEBUG_FLAGS & USB_DEVICE_DEBUG,
            (pszDevName == NULL ));

    USB_DEBUG_PRINT("too big device name",
            USB_DEBUG_FLAGS & USB_DEVICE_DEBUG,
            ((sizeof(pszDevName) > SIZEOF_DEVICE_NAME )));

    pCurDevO = &device_objects[wDeviceID];
    strcpy(pszDevName,pCurDevO->pDevName);

    return(ADI_USB_RESULT_SUCCESS);
}

/*********************************************************************************
 * adi_usb_DetachEndpoint API
 *
 * Detach Endpoint from a interface, returns the deleted objects ID.
 *
 * @param wInterfaceID is the interface object id returned by the 
 *        adi_usb_CreateInterface API
 * @param wEndpointID is the endpoint object id returned by the 
 *        adi_usb_CreateEndPoint API
 *
 * @return Deleted endpoint ID upon success and -1 upon failure.
 *
 * @see adi_usb_CreateInterface
 * @see adi_usb_CreateEndPoint
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_DetachEndpoint(const s32_t wInterfaceID,const s32_t wEndpointID)
{
INTERFACE_OBJECT *pCurInterfaceO;
ENDPOINT_OBJECT  *pCurEpO,*pLastEpO;
int              id;
bool             found=false;

    pCurInterfaceO = &interface_objects[wInterfaceID];
    pCurEpO        = &endpoint_objects[wEndpointID];

    id  =  pCurEpO->ID;

    /* point pCurEpO to the start of endpoint list for this interface. */
    pCurEpO  = pCurInterfaceO->pEndpointObj;
    pLastEpO = NULL;

    /* If its first node */
    if(pCurEpO->ID == id)
    {
        pCurInterfaceO->pEndpointObj = pCurEpO->pNext;
        pCurEpO->pNext = NULL;
        found          = true;
    }
    else
    {
        while(pCurEpO != NULL)
        {
            if(pCurEpO->ID == id)
            {
                 pLastEpO->pNext = pCurEpO->pNext;
                 pCurEpO->pNext  = NULL;
                 found           = true;
                 break;
            }
            pLastEpO = pCurEpO;
            pCurEpO = pCurEpO->pNext;
        }/* end while */
    }

    if(found)
    {
        /* Reset the ID, rest all will be reset during the creation. */
        pCurEpO->ID = -1;
        return id;
    }
    else
    {
        USB_DEBUG_PRINT("Endpoint Object does not exist",
                USB_DEBUG_FLAGS & USB_EP_DEBUG,
        (found == false));
        return -1;
    }
}

/*********************************************************************************
 *
 * adi_usb_TransmitEpZeroBuffer  API
 *
 * Description:
 *
 * Transmits EP zero data working in conjuction with USB state machine.Has to be
 * issued only in SETUP data phase.
 *
 * @param pData pointer to data
 * @param dwSize size of data in bytes
 *
 * @return upon success returns the number of bytes transmitted. 
 *         Applicable only in device mode. 
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_TransmitEpZeroBuffer(ADI_DEV_BUFFER *pBuffer)
{
DEVICE_OBJECT       *pDevO = pUsbCoreData->pDeviceHead;  /* head should be set to point active device */
SETUP_PACKET        *pActiveSetupPkt;
USB_EP_INFO         *pEpInfo = &pDevO->pEndpointZeroObj->EPInfo; /* Endpoint zero object */
s32_t               ret;
ADI_DEV_1D_BUFFER   *pCurBuffer = (ADI_DEV_1D_BUFFER*)pBuffer;

#ifdef _USB_MULTI_THREADED_
ker_disable_scheduler();
#endif /* _USB_MULTI_THREADED_ */
   
     /* Device mode */
     if(pUsbCoreData->eDevMode == MODE_DEVICE)
     {
         pActiveSetupPkt = &pDevO->ActiveSetupPkt;
         USB_ASSERT(pActiveSetupPkt == NULL);
         USB_ASSERT(pEpInfo->EpState != EP_STATE_SETUP_DATA_PHASE); /* error if we are not in data phase */
         USB_ASSERT(pCurBuffer == NULL);

         pEpInfo->TransferSize = pCurBuffer->ElementWidth * pCurBuffer->ElementCount;
         pDevO->pSetupData = (u8_t*)pCurBuffer->Data;
         pDevO->pAppEpZeroBuffer = (ADI_DEV_BUFFER*)pCurBuffer;

         ret = SendEpZeroDataEx(pDevO,(u8_t*)pCurBuffer->Data,pEpInfo->TransferSize);            
     }
     else /* Host mode */
     {
       /* host mode epzero transfer */
       HostModeTransmitEpZeroBuffer(pCurBuffer);
     }

#ifdef _USB_MULTI_THREADED_
ker_enable_scheduler();
#endif /* _USB_MULTI_THREADED_ */

    return(ret);
}

/*********************************************************************************
 * adi_usb_ReceiveEpZeroBuffer API
 *
 * Descripton:
 *
 * This API can be issued only in Data phase in a setup transaction. In device mode this
 * call has to be issued once vendor specific setup packet is passed to the class driver.
 * ElementCount * ElementWidth must be in accordance with the setup packet's wLength.
 *
 * @param pBuffer pointer to the buffer
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_ReceiveEpZeroBuffer(ADI_DEV_BUFFER *pBuffer)
{
DEVICE_OBJECT       *pDevO = pUsbCoreData->pDeviceHead;  /* head should be set to point active device */
SETUP_PACKET        *pActiveSetupPkt;
USB_EP_INFO         *pEpInfo = &pDevO->pEndpointZeroObj->EPInfo; /* Endpoint zero object */
s32_t               ret =0;
ADI_DEV_1D_BUFFER   *pCurBuffer = (ADI_DEV_1D_BUFFER*)pBuffer;
u32_t uInterruptStatus = cli();

   
     /* Device mode */
     if(pUsbCoreData->eDevMode == MODE_DEVICE)
     {
         pActiveSetupPkt = &pDevO->ActiveSetupPkt;
         USB_ASSERT(pActiveSetupPkt == NULL);
         USB_ASSERT(pEpInfo->EpState != EP_STATE_SETUP_DATA_PHASE); /* error if we are not in data phase */
         USB_ASSERT(pCurBuffer == NULL);

     /* request length is more than length in the setup packet */
     USB_ASSERT(pCurBuffer->ElementWidth * pCurBuffer->ElementCount > pActiveSetupPkt->wLength);

         pDevO->pAppEpZeroBuffer = (ADI_DEV_BUFFER*)pCurBuffer;
     pEpInfo->TransferSize     = pCurBuffer->ElementCount * pCurBuffer->ElementWidth;
     }
     else /* Host mode */
     {
         /* host mode epzero transfer */
         HostModeReceiveEpZeroBuffer(pCurBuffer);
     }

    sti(uInterruptStatus);
    return(ret);
}

/*********************************************************************************
 * adi_usb_StallEpZero API
 *  
 *  Descripton:
 *  Sends STALL response during data phase for setup requests
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_StallEpZero(void)
{
USB_EP_INFO *pEpInfo = &pUsbCoreData->pDeviceHead->pEndpointZeroObj->EPInfo;
u32_t uInterruptStatus = cli();


   adi_dev_Control(pUsbCoreData->pPddHandle,ADI_USB_CMD_SETUP_RESPONSE,(void*)ADI_USB_SETUP_RESPONSE_STALL);

   /* reset state machine */
   pEpInfo->EpState = EP_STATE_IDLE;

   sti(uInterruptStatus);
   return(1);
}


__ADI_USB_CORE_SECTION_CODE
void ResetInterfaceEndpoints(void)
{
int i;
    for(i=0; i < USB_MAX_INTERFACES; i++)       { interface_objects[i].ID = -1; }
    for(i=0; i < USB_MAX_ENDPOINTS; i++)        { endpoint_objects[i].ID = -1;  }
}

/*********************************************************************************
 * SetAltInterfaceToDefault
 *
 * This function sets the interfaces back to there default state.
 * The default is Interface zero.  The interface may have been set to a
 * value other than zero by a class driver in response to a command from
 * the host PC. 
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
static void SetAltInterfaceToDefault(PDEVICE_OBJECT pDevO)
{
PCONFIG_OBJECT pConfigO = pDevO->pActiveConfigObj;
PINTERFACE_OBJECT pIfceO = pConfigO->pInterfaceObj;
PINTERFACE_OBJECT pAltIfceO,pPrevIfceO=NULL,pNextIfceO=NULL;
u32_t uInterruptStatus;

    /* If device is not configured return error. */
    if(pDevO->wDeviceState != DEVICE_STATE_CONFIGURED)
        return;

    /* Assert if we have more than one interface objects for this configuration */
    USB_ASSERT(pConfigO->pConfigDesc->bNumInterfaces > USB_MAX_INTERFACES);
    USB_ASSERT(pIfceO == NULL);

    uInterruptStatus = cli();

    /* we mostly will not loop here as we will possibly have only one interface
     * if more than one interface is there then we have to loop accordingly.
     */
    while(pIfceO != NULL)
    {
        if(pIfceO->pInterfaceDesc->bAlternateSetting != 0)
        {
            /* Get the next interface object from the main configuraiton. */
            pNextIfceO = pIfceO->pNext;

            /* Get current interfaces alternate interface pointer */
            pAltIfceO = pIfceO->pAltInterfaceObj;

            if(pAltIfceO->pInterfaceDesc->bAlternateSetting == 0)
            {
                /* Set the alternate setting to the current */
                pAltIfceO->pAltInterfaceObj = pIfceO;

                if(pPrevIfceO != NULL)
                    pPrevIfceO->pNext = pAltIfceO;
                else
                    pConfigO->pInterfaceObj = pAltIfceO;

                pAltIfceO->pNext  = pNextIfceO;
                
                /* now pIfceO became the alternate object */
                pIfceO->pNext = NULL;
                
                /* Reset the altenrate setting object. */
                pIfceO->pAltInterfaceObj = NULL;

                /* Set the currently active interface object in the Device object */
                pDevO->pActiveInterfaceObj = pAltIfceO;
            }
        }
        pPrevIfceO = pIfceO;
        pIfceO = pIfceO->pNext;
    }

    sti(uInterruptStatus);
        
    return;        
}

/*********************************************************************************
 * adi_usb_CoreInit API
 *
 * Core Init function initializes the core. The initialization includes various 
 * core specific data structures.This API must be called before invoking any core 
 * APIs. Core initialization happens only once. Once core is initialized calling 
 * this API has no affect.
 *
 * @param pConfigData holds the configuration data
 *
 * @return 1 upon success and -1 upon failure.
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_CoreInit(void *pConfigData)
{
    int i;
    PENDPOINT_OBJECT pEpO;
    char DefaultManufacturerString[]    = "Analog Devices, Inc.";
    char DefaultProductString[]         = "Blackfin USB Device";
    s8_t iManufacturer = 0;
    s8_t iProduct = 0;

    /* check if core has been initialzied already, if so return 0 */
    if(!pUsbCoreData->isCoreInitialized)
    {
        pUsbCoreData->isCoreInitialized = true;
    }
    else
    {
        return 0;
    }

    /* initialize objects */
    for(i=0; i < USB_MAX_DEVICES; i++)
    {
        device_objects[i].ID = -1;
        device_objects[i].wDeviceState = DEVICE_STATE_NOT_CONFIGURED;
    }

    for(i=0; i < USB_MAX_CONFIGURATIONS; i++)   { config_objects[i].ID = -1;    }
    for(i=0; i < USB_MAX_INTERFACES; i++)       { interface_objects[i].ID = -1; }
    for(i=0; i < USB_MAX_ENDPOINTS; i++)        { endpoint_objects[i].ID = -1;  }
    for(i=0; i < USB_MAX_STRINGS; i++)          { ptr_string_descriptors[i] = NULL; }

    /*
     * setup default manufacturer and product string indexes in the default device
     * descriptor, note the user can create their own strings and update the device
     * descriptor themselves.
     */
    iManufacturer = adi_usb_CreateString(DefaultManufacturerString);
    if ( (iManufacturer > 0) && (iManufacturer < USB_MAX_STRINGS) )
        default_device_descriptor.bIManufacturer = iManufacturer;

    iProduct = adi_usb_CreateString(DefaultProductString);
    if ( (iProduct > 0) && (iProduct < USB_MAX_STRINGS) )
        default_device_descriptor.bIProduct = iProduct;

    /* Configure EP0 */
    pEpO = &endpoint_objects[0];
    pEpO->ID = 0;
    pEpO->pEndpointDesc = &endpoint_descriptors[0];
    memset(pEpO->pEndpointDesc,0,sizeof(ENDPOINT_DESCRIPTOR));
    pEpO->pEndpointDesc->bDescriptorType = TYPE_ENDPOINT_DESCRIPTOR;
    memset(&pEpO->EPInfo,0,sizeof(USB_EP_INFO));
    pEpO->pEndpointSpecificObj=NULL;
    pEpO->pNext = NULL;

    /* Setup callback for EP0 */
    pEpO->EPInfo.EpCallback = (ADI_DCB_CALLBACK_FN)adi_usb_EpZeroCallback;

    /*
     *  Add few free xmt buffers for EP0 traffic. currently we have each buffer has data of 256
     *  TODO: Do we need more control buffers? check to allocate memory statically. malloced memory may
     *  be in SDRAM and slow down the process.
     *
     *  EP0 or Control Endpoint buffer size requirements: Section 5.5.3 / pg 39 USB 2.0 Specification
     *  High Speed : 64 bytes
     *  Full Speed : 8,16,32,64 bytes
     *  Low Speed  : 8 bytes
     */
          
    for(i=0;i<NUM_TX_CONTROL_BUFFERS;i++)
    {
      TxControlBuffers[i].ElementCount = gEpZeroBufferInfo.wMaxEpZeroBufferSize;
      TxControlBuffers[i].CallbackParameter = &TxControlBuffers[i];
      TxControlBuffers[i].ElementWidth =1;
      TxControlBuffers[i].Data = malloc(sizeof(char) * gEpZeroBufferInfo.wMaxEpZeroBufferSize);
      USB_ASSERT(TxControlBuffers[i].Data == NULL);
      TxControlBuffers[i].pNext =  ((i+1) == NUM_TX_CONTROL_BUFFERS) ? NULL : &TxControlBuffers[i+1];
      /* We may have to set pAdditionalInfo *only* when we transmit on Ep0.
       * Are we going to distinguish between * application buffers and CORE buffers for EP0 ?
       */
      TxControlBuffers[i].pAdditionalInfo = &gEpZeroBufferInfo;
    }
    pEpO->EPInfo.pFreeXmtList =  (ADI_DEV_BUFFER*)&TxControlBuffers[0];

    /*
     *  Add few free rx buffers for EP0 traffic. currently we have each buffer has data of 2048
     *  TODO: Do we need more control buffers? check to allocate memory statically. malloced memory may
     *  be in SDRAM and slow down the process.
     */
    for(i=0;i<NUM_RX_CONTROL_BUFFERS;i++)
    {
      RxControlBuffers[i].ElementCount = gEpZeroBufferInfo.wMaxEpZeroBufferSize;
      RxControlBuffers[i].CallbackParameter = &RxControlBuffers[i];
      RxControlBuffers[i].ElementWidth =1;
      RxControlBuffers[i].Data = malloc(sizeof(char) * gEpZeroBufferInfo.wMaxEpZeroBufferSize);
      USB_ASSERT(RxControlBuffers[i].Data == NULL);
      RxControlBuffers[i].pNext =  ((i+1) == NUM_RX_CONTROL_BUFFERS) ? NULL : &RxControlBuffers[i+1];
      RxControlBuffers[i].pAdditionalInfo = &gEpZeroBufferInfo;
    }
    pEpO->EPInfo.pFreeRcvList =  (ADI_DEV_BUFFER*)&RxControlBuffers[0];

    /* setup configuraiton memory */
    pUsbCoreData->pConfigMemoryArea = (u8_t*)&ConfigMemoryArea[0];

    gbConfigSwapped = false;

    /* Set default to device mode. in case of OTG during BUS interrupt driver
     * has to issue adi_usb_SetDeviceMode(MODE_OTG_HOST) call. */
    //adi_usb_SetDeviceMode(MODE_DEVICE);
    return(ADI_USB_RESULT_SUCCESS);
}

/*********************************************************************************
 *
 * GetTotalConfiguration  - Core Internal Function
 *
 *  This function copies the interface and interface specific descriptors
 *  and the associated endpoint descriptors of the interface into the pMem
 *  area. It also retuns the length of the total copied bytes.
 *
 *  @param pIfO pointer to the interface object
 *  @param pMem pointer to the memory area to which the interface and endpoint data 
 *         will get copied.
 *  @param pLen length of the supplied memory area.
 *
 *  @return total number of copied bytes
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static int GetInterfaceConfiguration(PINTERFACE_OBJECT pIfO, 
                                     char *pMem, 
                                     int *pLen,
                                     bool bUseDefaultOSpeed)
{
PENDPOINT_OBJECT           pEpO;
PINTERFACE_DESCRIPTOR      pIfD;
PENDPOINT_DESCRIPTOR       pEpD;
PENDPOINT_SPECIFIC_OBJECT  pEpSpecO;
PINTERFACE_SPECIFIC_OBJECT pIfceSpecO;
s32_t                      dwMaxEpSize;

    /* set the length first */
    *pLen = 0;

     /* Get the interface descriptor */
    pIfD = pIfO->pInterfaceDesc;
    memcpy(pMem,(char*)pIfD,INTERFACE_DESCRIPTOR_LEN);
    pMem += INTERFACE_DESCRIPTOR_LEN;
    *pLen += INTERFACE_DESCRIPTOR_LEN;

    /* Copy interface specific configuration descriptors */
    pIfceSpecO = pIfO->pIntSpecificObj;

    while(NULL != pIfceSpecO)
    {
        memcpy(pMem,(char*)pIfceSpecO->pIntSpecificData,pIfceSpecO->length);
        pMem += pIfceSpecO->length;
        *pLen += pIfceSpecO->length;
        pIfceSpecO = pIfceSpecO->pNext;
    }

    /* Get endpoint descriptors for this interface */
    pEpO = pIfO->pEndpointObj;

    /* Get all endpoints of this interface */
    while(NULL != pEpO)
    {
        pEpD = pEpO->pEndpointDesc;

        memcpy(pMem,(char*)pEpD,ENDPOINT_DESCRIPTOR_LEN);

    /* we do't change the actual endpoint descriptor when default other speed descriptor
     * is requested, instead we use the default with overwritten max values
     * bUseDefaultOSpeed will be true only when we got request for the other speed configuration
     * and there is no other speed configuration specified by the class driver. If other speed
     * conf is specified by the class driver it will follow similar to the normal config process.
     */
    if(bUseDefaultOSpeed)
    {
          *(pMem+4) = 0x40; *(pMem+5) = 0x00;
    }

        pMem  += ENDPOINT_DESCRIPTOR_LEN;
        *pLen += ENDPOINT_DESCRIPTOR_LEN;

        /* Get any endpoint specific descriptors */
        pEpSpecO = pEpO->pEndpointSpecificObj;
        while(NULL != pEpSpecO)
        {
            memcpy(pMem,(char*)pEpSpecO->pEpSpecificData,pEpSpecO->length);
            pMem  += pEpSpecO->length;
            *pLen += pEpSpecO->length;
            pEpSpecO = pEpSpecO->pNext;
        }

        pEpO = pEpO->pNext;
    }
    return (*pLen);
}

/*********************************************************************************
 * GetTotalConfiguration - Core Internal function
 *
 * Gets the total configuration to the supplied memory. it will copy the
 * configuration, interface, endpoint descriptors and any associated class
 * specific descriptors in to the supplied memory. if bnOtherConfig is true
 * it will get the info for the other configuration. This function is used to get
 * the entire configuration of the device. This is only used in peripheral mode.
 *
 * @param dwDeviceID id of the device for which configuraiton has to be obtained.
 * @param pConfigMemory pointer to the memory area where entire configuration data 
 *        gets copied.
 * @param dwLenConfigMemory length of the memory area supplied in bytes
 * @param OtherConfig if true the other configuration of the device is copied.
 *
 * @return The total number of bytes copied upon success.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static s32_t GetTotalConfiguration(const s32_t dwDeviceID, 
                                   void *pConfigMemory, 
                                   const s32_t dwLenConfigMemory,
                                   bool OtherConfig)
{
PCONFIG_OBJECT        pCfO,pOspeedCfO;
PINTERFACE_OBJECT     pIfO;
char                  *pMem;
PCLASS_SPECIFIC_OBJECT     pClassSpecO;
PCONFIGURATION_DESCRIPTOR  pCfD;
short                  *wLenInCfgDArea;
int                   TotalLength=0,wLength=0,wConfigLength;
bool                  bUseDefaultConfig=false;
DEVICE_OBJECT *pCurDevO = &device_objects[dwDeviceID];
static u8 bSwapped = 0;

    #ifdef USB_DEBUG
    /* TODO: Check if supplied memory is sufficent to copy the configuration */
    #endif /* USB_DEBUG */

    USB_ASSERT(pConfigMemory == NULL);

    /* Get the current configuraton object */
    pCfO = pCurDevO->pConfigObj;

    /* Get the other speed configuration object */
    pOspeedCfO = pCfO->pOtherSpeedConfigObj;

    /* If we got request for OtherSpeedConfiguraiton and we do not have other speed configuration
     * pCfO will be same as pOspeedCfO.We set bUseDefaultConfig to true to indicate that we want
     * to use the primary configuration by changing the descriptiors.
     */
    if ((pOspeedCfO != pCfO) && (gbConfigSwapped == false)) 
    {
        adi_dev_Control(pUsbCoreData->pPddHandle, ADI_USB_CMD_GET_DEV_SPEED, (void*)&pCurDevO->nBusSpeed);
        /* If Speed is FULL then we need to switch to other speed configuration 
        */
        if ( pCurDevO->nBusSpeed==ADI_USB_DEVICE_SPEED_FULL ) 
        {
            /* We are full speed already no need for swap */
            if(gbFullSpeed == false)
            {
                /* Use other speed configuration */
                pCurDevO->pOtherSpeedConfigObj  = pCfO;
                pCurDevO->pConfigObj            = pOspeedCfO;
            }
            
            pCfO                                     = pCurDevO->pConfigObj;
            pOspeedCfO                               = pCurDevO->pOtherSpeedConfigObj;
            
            gbFullSpeed = true;
        }
        else if (pCurDevO->nBusSpeed==ADI_USB_DEVICE_SPEED_HIGH )
        {
            /* We were full speed now swap back to high speed */
            if(gbFullSpeed == true)
            {
                /* Swap back to main configuration */
                pCurDevO->pOtherSpeedConfigObj  = pCfO;
                pCurDevO->pConfigObj            = pOspeedCfO;
                
                pCfO                                     = pCurDevO->pConfigObj;
                pOspeedCfO                               = pCurDevO->pOtherSpeedConfigObj;
            
                gbFullSpeed = false;
            }
            else
            {
                /* Use main configuration */
                pCfO                                     = pCurDevO->pConfigObj;
                  pOspeedCfO                               = pCurDevO->pOtherSpeedConfigObj;
            }                  
        }

        pCfO->pConfigDesc->bDescriptorType       = TYPE_CONFIGURATION_DESCRIPTOR;
        pOspeedCfO->pConfigDesc->bDescriptorType = TYPE_OTHER_SPEED_CONFIGURATION_DESCRIPTOR;
        pCfO->pOtherSpeedConfigObj               = pOspeedCfO;
        pOspeedCfO->pOtherSpeedConfigObj         = NULL;

    gbConfigSwapped = true;
    }
        
    if(OtherConfig)
    {
        if(pCfO == pOspeedCfO)
            bUseDefaultConfig = true;
        else
        {
            /* Other speed configuraiton is different from that of primary configuration
             * use it */
             pCfO = pCfO->pOtherSpeedConfigObj;
        }
    }

    pMem = (char*)pConfigMemory;

    /* Check for all configuration objects */
    while(pCfO != NULL)
    {
        /* Computes the total number of bytes in the configuration descriptor
         * by the end of this iteration
         */
        wConfigLength = 0;

        pCfD = pCfO->pConfigDesc;

        /* Copy configuration descriptor */
        memcpy(pMem,(char*)pCfD,CONFIGURATION_DESCRIPTOR_LEN);

    /* We got request for the other speed configuration and there is no class specified other speed
     * configuration descriptor use the default by changing the type
     */
#if 1     
    if(bUseDefaultConfig)
            *(pMem+1) = (char)TYPE_OTHER_SPEED_CONFIGURATION_DESCRIPTOR;
#endif
        wLenInCfgDArea = (short*) (pMem + 2 );
        pMem += CONFIGURATION_DESCRIPTOR_LEN;
        TotalLength += CONFIGURATION_DESCRIPTOR_LEN;

        wConfigLength += CONFIGURATION_DESCRIPTOR_LEN;

        /* Copy all class specific descriptors */
        pClassSpecO = pCfO->pClassSpecificObj;

        while(NULL != pClassSpecO)
        {
            memcpy(pMem,(char*)pClassSpecO->pClassSpecificData,pClassSpecO->length);
            pMem          += pClassSpecO->length;
            TotalLength   += pClassSpecO->length;
            wConfigLength += pClassSpecO->length;
            pClassSpecO    = pClassSpecO->pNext;
        }

        /* Copy all interface descriptors for this class */
        pIfO = pCfO->pInterfaceObj;

        /* Get all Interface descriptors and its associated endpoint descriptors */
        while(pIfO)
        {
            GetInterfaceConfiguration(pIfO,pMem,&wLength,bUseDefaultConfig);
            pMem += wLength;             /* Added to stop corruption */
            TotalLength += wLength;
            wConfigLength += wLength;

            /* Check if there are any alternate interfaces if so get it
             * By default we have one alternate setting so we check the
             * validity of the alternate setting by checking the length.
             */
#ifdef _FIX_FOR_MULTIPLE_ALTERNATE_INTERFACES
            {
                INTERFACE_OBJECT *pAltInterfaceObj = pIfO->pAltInterfaceObj;
                while (pAltInterfaceObj) {

                    /* Add the next interface to the configuration data */
                    wLength = 0;
                    GetInterfaceConfiguration(pAltInterfaceObj,pMem,&wLength,bUseDefaultConfig);
                    pMem += wLength;          /* increment pMem by the size of total interface configuration */
                    TotalLength += wLength;
                    wConfigLength += wLength;

                    /* move on to next in the list */
                    pAltInterfaceObj = pAltInterfaceObj->pNext;
                }
            }
#else
            if(pIfO->pAltInterfaceObj->pInterfaceDesc->bLength != 0)
            {
               wLength = 0;
               GetInterfaceConfiguration(pIfO->pAltInterfaceObj,pMem,&wLength,bUseDefaultConfig);
               pMem += wLength;          /* increment the pMem by the size of total interface configuration */
               TotalLength += wLength;
               wConfigLength += wLength;
            }
#endif
            pIfO = pIfO->pNext;
        } /* while(pIf0 != NULL); */

        pCfO->pConfigDesc->wTotalLength = wConfigLength;
        /* update the total length for this configuration in the memory area. */
        *wLenInCfgDArea = wConfigLength;
        pCfO = pCfO->pNext;
    } /* while(pCfO != NULL) */

    return TotalLength;
}
/**
 * Returns the requied configuration memory to the caller. Should be used
 * to determined the size of pConfigMemory of GetTotalConfiguraton API.
 * TODO: As the configuration is used by the core itself check if we can delete this API
 */
__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_GetRequiredConfigurationMemory(void)
{
    return (ADI_USB_RESULT_FAILED);
}

/*********************************************************************************
 * adi_usb_SetNumPhysicalEndPoints API
 *
 * Descripton:
 * Set Maximum number of physical endpoints. This API is used by the physical driver
 * to set the maximum number of endpoints it can support.
 *
 * @param wNumEndPoints specifies the number of physical endpoints
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_SetPhysicalEndPointInfo(const s32_t wNumEndPoints,PHYSICAL_EP_INFO PhysicalEndpointInfo[])
{
    #ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
    #endif

    pUsbCoreData->wNumPhysicalEndpoints = wNumEndPoints;

    memcpy(&pUsbCoreData->PhysicalEndpointInfo[0],PhysicalEndpointInfo, sizeof(PHYSICAL_EP_INFO) * wNumEndPoints);

    #ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
    #endif

    return(ADI_USB_RESULT_SUCCESS);
}

/*********************************************************************************
 * ResetDevices()
 *
 * Core internal function
 * Called when Reset signanling is detected. Applicable only in the device mode
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static void ResetDevices(void)
{
PDEVICE_OBJECT pDevO = pUsbCoreData->pDeviceHead;

   if(pDevO != NULL)
   {
      pDevO->wDeviceState = DEVICE_STATE_NOT_CONFIGURED;
      pDevO->pActiveConfigObj = NULL;
      pDevO->pActiveInterfaceObj = NULL;
      pDevO->nBusSpeed = ADI_USB_DEVICE_SPEED_UNKNOWN;
      pDevO->pAppEpZeroBuffer = NULL;
   }
}

/*********************************************************************************
 *
 * adi_usb_EpZeroCallback - API
 *  
 * Descripton:
 * EpZeroCallback handler is responsible for handling various control events.
 * Control events include various bus events as well as setup phase events.
 * The handlers gets initialized during runtime depending on the peripheral mode
 * of operation. If peripheral is operating in OTG host mode the handlers respond
 * differently to that of device mode operation. If the control traffic is not
 * intended to the core then the data will be passed to the application by invoking
 * the currently active configurations EpZero callback.
 *
 * @param AppHandle Application Handle for the callback
 * @param wEvent    Identifies the event.
 * @param pArg      Argument to the callback. Value depends on the wEvent.
 *
 * @see USB_EPZERO_CALLBACK_EVENTS
 *
 *********************************************************************************/
 
__ADI_USB_CORE_SECTION_CODE
void adi_usb_EpZeroCallback(void *AppHandle,unsigned int wEvent, void *pArg)
{
    switch(wEvent)
    {
        case ADI_USB_EVENT_SETUP_PKT:
            pUsbCoreData->StandardUsbRequestHandler(AppHandle,wEvent,pArg);
            break;

        case ADI_USB_EVENT_TX_COMPLETE:
            pUsbCoreData->TxCompleteHandler(AppHandle,wEvent,pArg);
            break;

        case ADI_USB_EVENT_RX_COMPLETE:
            pUsbCoreData->RxCompleteHandler(AppHandle,wEvent,pArg);
            break;

        case ADI_USB_EVENT_ROOT_PORT_RESET:
            ResetDevices();
            /* FALLTHROUGH */

        case ADI_USB_EVENT_START_OF_FRAME:
        case ADI_USB_EVENT_RESUME:
        case ADI_USB_EVENT_SUSPEND:
        case ADI_USB_EVENT_VBUS_TRUE:
        case ADI_USB_EVENT_VBUS_FALSE:
        case ADI_USB_EVENT_DISCONNECT:
        {
            /* ONLY do this if we are in device mode */
            if(pUsbCoreData->eDevMode == MODE_DEVICE)
            {            
                gbConfigSwapped = false;
                
                PDEVICE_OBJECT       pCurDeviceO = pUsbCoreData->pDeviceHead;  
                SetAltInterfaceToDefault(pCurDeviceO);
            }
            
            pUsbCoreData->BusEventHandler(AppHandle, wEvent, pArg);
        }                        
            break;

        case ADI_USB_EVENT_START_DEV_ENUM:
            pUsbCoreData->StandardUsbRequestHandler(AppHandle,wEvent,pArg);
            break;

        default:
            break;
    }
}

/*********************************************************************************
 *
 * ResetEpStallFeature - Core Internal function
 *
 * Resets Halt Attribute for all endpoint objects for the given
 * configurations all interfaces and its alternate setting.
 *
 * @param pConfigObj pointer to the configuration object.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static void ResetEpStallFeature(PCONFIG_OBJECT pConfigObj)
{
PINTERFACE_OBJECT pIfceO;
PENDPOINT_OBJECT pEpO;

    USB_ASSERT(pConfigObj == NULL);
    pIfceO = pConfigObj->pInterfaceObj;

    while(pIfceO != NULL)
    {
            /* check the primrary interface */
            pEpO = pIfceO->pEndpointObj;
            while(pEpO != NULL)
            {
                pEpO->EPInfo.bnEpHalt = false;
                pEpO = pEpO->pNext;
            }

            /* Reset its alternate interface if its present
             * TODO: Currently We do not handle list of alternate settings.
             */
            if(pIfceO->pAltInterfaceObj && pIfceO->pAltInterfaceObj->pInterfaceDesc 
                && pIfceO->pAltInterfaceObj->pInterfaceDesc->bLength >0)
            {
                pEpO = pIfceO->pAltInterfaceObj->pEndpointObj;
                while(pEpO != NULL)
                {
                    pEpO->EPInfo.bnEpHalt = false;
                    pEpO = pEpO->pNext;
                }
            }
            /* go to the next interface object */
            pIfceO = pIfceO->pNext;
    }
}

/*********************************************************************************
 *
 * UpdateActiveEpList  - Core Internal function
 *
 * Update Currently Active Endpoint list. This call is issed if host issues
 * SET_CONFIGURATION or SET_INTERFACE requests. Host could potentially send
 * data to any endpoint that is present in the currently selected interface.
 * This call must be issued before informing the peripheral driver about the
 * changed configuration/interface.
 *
 * We expect pDevO->pActiveConfigObj will hold the currently active config
 *
 * @param pDevO pointer to the device object.
 *
 * @return 1 upon success.
 *
 * @see SetConfiguration
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t UpdateActiveEpList(PDEVICE_OBJECT pDevO)
{
PCONFIG_OBJECT pCfgO;
PINTERFACE_OBJECT pIfceO;
PENDPOINT_OBJECT pEpO=NULL,pCurEpO=NULL;

    USB_ASSERT(pDevO->pActiveConfigObj == NULL);

    pCfgO  = pDevO->pActiveConfigObj;
    pIfceO = pCfgO->pInterfaceObj;

    /* We will assert if we do not have an interface. */
    USB_ASSERT((pCfgO==NULL) || (pIfceO == NULL));

    /* Reset previous value */
    pEpO = pCfgO->pActiveEpObj = NULL;

    /* Walk through all interface to construct the possible endpoint list
     * we do not include the alternate settings as the endpoints will be
     * mutually exclusive.We always have the currently active alternate
     * setting in the InterfaceObject list if an interface has multiple
     * alternate settings.
     */
    while(pIfceO != NULL)
    {
        /* Get the current interfaces endpoint object */
        pCurEpO = pIfceO->pEndpointObj;

        /* walk through current interfaces EP list */
        while(pCurEpO != NULL)
        {
            /* first node */
            if(pCfgO->pActiveEpObj == NULL)
            {
                pCfgO->pActiveEpObj = pCurEpO;
                pEpO = pCfgO->pActiveEpObj;
            }
            else
            {
                    pEpO->pNextActiveEpObj  = pCurEpO;
                    pEpO = pCurEpO;
            }
            /* Get the next active endpoint */
            pCurEpO = pCurEpO->pNext;
        }
        /* Go to the next interface Object */
        pIfceO = pIfceO->pNext;
    }

    /* let peripheral know that EP list has been updated. */
    if(pCfgO->pActiveEpObj){
        adi_dev_Control(pUsbCoreData->pPddHandle, ADI_USB_CMD_UPDATE_ACTIVE_EP_LIST, (void*)pCfgO->pActiveEpObj);
    }
    return(1);
}


/*********************************************************************************
 *
 * SetConfiguration - Core Internal function
 *
 * Sets the currently active configuration in the passed device object. If
 * the configuraiton ID not available in the list of configuration objects
 * it will return -1. Upon sucess this will return the newly selected
 * if an interface has multiple alternate settings. configuraiton value.
 *
 * @param pDevO pointer to the device object
 * @param bConfigurationValue selected configuration value
 * @param AppHandle application handle to call the callback
 *
 * @return -1 upon failure.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static int SetConfiguration(PDEVICE_OBJECT pDevO,u8_t bConfigValue,void *AppHandle)
{
PCONFIG_OBJECT pConfigO = pDevO->pConfigObj;
u8_t  bCfgValue = bConfigValue;
u8_t  ret = (u8_t)-1;

    switch(pDevO->wDeviceState)
    {
        case DEVICE_STATE_NOT_CONFIGURED:
             /* If we get any configuraiton request other than changing to address state
              * we will assert.
              */
              USB_ASSERT(bConfigValue != (u8_t)0);
              pDevO->wDeviceState = DEVICE_STATE_ADDRESS;
        break;

        case DEVICE_STATE_ADDRESS:
        case DEVICE_STATE_CONFIGURED:

            /* If we get configuration value of zero we will remain in the address state
             * or change to the address state if we are already configured.
             */
            if(bConfigValue == (u8_t)0)
            {
                /* inform upper level that it's configuration has been cleared */
                if (pUsbCoreData->pDeviceHead->pActiveConfigObj)
                {
                    if (pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback)
                        pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback(AppHandle, ADI_USB_EVENT_SET_CONFIG, (void*)bConfigValue);
                }

                pDevO->wDeviceState = DEVICE_STATE_ADDRESS;
                /* reset currently active configuraiton, interface object values */
                pDevO->pActiveConfigObj    = NULL;
                pDevO->pActiveInterfaceObj = NULL;
                ret = bConfigValue;
            }
            else  /* we are about to select the supplied configuration */
            {
                while(pConfigO != NULL)
                {
                    /* if we find the configuration object setup it to be the
                     * currently active configuration object and also set its
                     * interface being the active interface object.
                     */
                    if(pConfigO->pConfigDesc->bConfigurationValue == bCfgValue)
                    {
                        pDevO->pActiveConfigObj =  pConfigO;
                        pDevO->pActiveInterfaceObj = pConfigO->pInterfaceObj;
                        ret = bCfgValue;
                        /* Set the device state to configured state. */
                        pDevO->wDeviceState = DEVICE_STATE_CONFIGURED;

                        /* Reset all endpoints halt feature is reset to zero for SET_CONFIGURATION
                         * and SET_INTERFACE requests.
                         * USB 2.0 specification pg 256
                         */
                        ResetEpStallFeature(pConfigO);

                        /* Update Active Endpoint list */
                        UpdateActiveEpList(pDevO);
                        /* inform upper level that it's configuration has been set */
                        if (pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback){
                            pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback(AppHandle, ADI_USB_EVENT_SET_CONFIG, (void*)bConfigValue);
                        }
                        break;
                    }

            /* Check if host is trying to select the other speed configuration
             * if there is no other speed configuration then otherspeed configuration
             * points to the configuration itself. We need not check the object that
             * we have checked already.
             */
            if( (pConfigO != pConfigO->pOtherSpeedConfigObj) &&
                    (pConfigO->pOtherSpeedConfigObj->pConfigDesc->bConfigurationValue == bCfgValue))
            {
                        pDevO->pActiveConfigObj =  pConfigO->pOtherSpeedConfigObj;
                        pDevO->pActiveInterfaceObj = pConfigO->pOtherSpeedConfigObj->pInterfaceObj;
                        ret = bCfgValue;
                        /* Set the device state to configured state. */
                        pDevO->wDeviceState = DEVICE_STATE_CONFIGURED;

                        /* Reset all endpoints halt feature is reset to zero for SET_CONFIGURATION
                         * and SET_INTERFACE requests.
                         * USB 2.0 specification pg 256
                         */
                        ResetEpStallFeature(pConfigO);

                        /* Update Active Endpoint list */
                        UpdateActiveEpList(pDevO);
                        /* inform upper level that it's configuration has been set */
                        if (pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback) {
                            pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback(AppHandle, ADI_USB_EVENT_SET_CONFIG, (void*)bConfigValue);
                        }
                        break;
            }

                    /* go to next object */
                    pConfigO = pConfigO->pNext;
                }

            }
        break;
    }

    USB_ASSERT(pConfigO == NULL);
    if (pConfigO == NULL)
        return -1;

    return(bCfgValue);
}

/*********************************************************************************
 *
 * SetInterface - Core Internal Function
 *
 * Selects alternate setting (interface) for the specified interface. We expect that
 * a configuration has been already selected if not its an error.
 *
 * @param pDevO identifies Device object identifier
 *
 * @param bInterfaceNumber Interface number whose alternate setting is selected
 *
 * @param bAlternateSetting Alternate Interface setting number
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
u8_t SetInterface(PDEVICE_OBJECT pDevO,u8_t bInterfaceNumber, u8_t bAltSettingID,void *AppHandle)
{
PCONFIG_OBJECT pConfigO = pDevO->pActiveConfigObj;
PINTERFACE_OBJECT pIfceO = pConfigO->pInterfaceObj;
PINTERFACE_OBJECT pAltIfceO,pPrevIfceO=NULL,pNextIfceO=NULL;
u8_t ret = (u8_t)-1;
u32_t uInterruptStatus;

    /* If device is not configured return error. */
    if(pDevO->wDeviceState != DEVICE_STATE_CONFIGURED)
        return ret;

    /* Assert if we have more than one interface objects for this configuration */
    USB_ASSERT(pConfigO->pConfigDesc->bNumInterfaces > USB_MAX_INTERFACES);
    USB_ASSERT(pIfceO == NULL);

    uInterruptStatus = cli();

    /* we mostly will not loop here as we will possibly have only one interface
     * if more than one interface is there then we have to loop accordingly.
     */
    while(pIfceO != NULL)
    {
        if(pIfceO->pInterfaceDesc->bInterfaceNumber == bInterfaceNumber)
            break;
        else
        {
            pPrevIfceO = pIfceO;
            pIfceO = pIfceO->pNext;
        }
    }

    /* We assert if the interface object is not correctly the active interface object. */
    //USB_ASSERT((pIfceO == NULL) || (pIfceO != pDevO->pActiveInterfaceObj) );

    if(pIfceO == NULL)
    {
        sti(uInterruptStatus);
        return (ret);
    }

    /* Get the next interface object from the main configuraiton. */
    pNextIfceO = pIfceO->pNext;

    /* Get current interfaces alternate interface pointer */
    pAltIfceO = pIfceO->pAltInterfaceObj;

    if(pIfceO->pInterfaceDesc->bAlternateSetting == bAltSettingID)
    {
        /* Already the currently active interface. */
        pDevO->pActiveInterfaceObj = pIfceO;
    }
    else if(pAltIfceO->pInterfaceDesc->bAlternateSetting == bAltSettingID)
    {
        /* Set the alternate setting to the current */
        pAltIfceO->pAltInterfaceObj = pIfceO;

        if(pPrevIfceO != NULL)
            pPrevIfceO->pNext = pAltIfceO;
        else
            pConfigO->pInterfaceObj = pAltIfceO;

        pAltIfceO->pNext  = pNextIfceO;
        /* now pIfceO became the alternate object */
        pIfceO->pNext = NULL;
        /* Reset the altenrate setting object. */
        pIfceO->pAltInterfaceObj = NULL;

        /* Set the currently active interface object in the Device object */
        pDevO->pActiveInterfaceObj = pAltIfceO;
    }
    /* Reset endpoint halt features for this configuration.
     * pg 256 of USB 2.0 Specification
     */
    ResetEpStallFeature(pConfigO);

    /* Update Active Endpoint list */
    UpdateActiveEpList(pDevO);

    /* inform upper level that it's interface has been set */
    if (pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback) {
        pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback(AppHandle, ADI_USB_EVENT_SET_INTERFACE, (void*)(bInterfaceNumber | (bAltSettingID << 24)) );
    }    
    sti(uInterruptStatus);
    
    return bAltSettingID;
}

__ADI_USB_CORE_SECTION_CODE
s32_t SendEpZeroDataEx(DEVICE_OBJECT *pDevO,u8_t *pData,int length)
{
    ADI_DEV_1D_BUFFER   *pBuffer=NULL;
    u32                 TxCount;
    ENDPOINT_OBJECT     *pEpZeroO;
    u32_t               uInterruptStatus,uResult;
    u32                 BytesRemaining;
    u32                 LoopCount = 0;

    USB_ASSERT((pDevO == NULL) || (pData == NULL) || (length == 0));

    USB_EP_INFO *pEpInfo = &pDevO->pEndpointZeroObj->EPInfo;

    /* Get the endpoint zero object */
    pEpZeroO = pDevO->pEndpointZeroObj;
    BytesRemaining = pEpInfo->TransferSize - pEpInfo->EpNumTotalBytes;

    TxCount = (BytesRemaining > pDevO->pDeviceDesc->bMaxPacketSize0 ) ? pDevO->pDeviceDesc->bMaxPacketSize0 :
                                                                           BytesRemaining;

        /* enter critical region whilst a buffer is located */
        uInterruptStatus = cli();
        
        pBuffer = (ADI_DEV_1D_BUFFER*)pEpZeroO->EPInfo.pFreeXmtList;

        USB_ASSERT(pBuffer == NULL);

        if(pBuffer)
        {
            /* move the free list to point to the next buffer. */
            pEpZeroO->EPInfo.pFreeXmtList = (ADI_DEV_BUFFER*)pBuffer->pNext;
            /* take off current buffer from the list. */
            pBuffer->pNext = NULL;
        }
        /* exit critical region */
        sti(uInterruptStatus);

        /* Assert if we do not have sufficent buffer space. */
        USB_ASSERT(pBuffer->ElementCount < TxCount);

        memcpy(pBuffer->Data,(u8_t*)(pDevO->pSetupData + pEpInfo->EpNumTotalBytes),TxCount);
        pBuffer->ElementCount   = TxCount;
        pBuffer->ElementWidth   = 1;
        pBuffer->CallbackParameter = pBuffer;
        pBuffer->ProcessedElementCount = 0;
        pBuffer->ProcessedFlag  = 0;
      
        pEpInfo->EpNumTotalBytes += TxCount;
                                       
        USB_ASSERT(pUsbCoreData->pPddHandle == NULL);

        /* Now send the data over the avaialble peripheral driver */
        uResult = adi_dev_Write(pUsbCoreData->pPddHandle,ADI_DEV_1D,(ADI_DEV_BUFFER*)pBuffer);
        USB_ASSERT(uResult != ADI_DEV_RESULT_SUCCESS);

    if(pEpInfo->TransferSize == pEpInfo->EpNumTotalBytes)    
        return(1);
    else
        return(0);
}

/*********************************************************************************
 *
 * SendEpZeroData - Core Internal Funciton
 *
 * This function is used to send data on Ep0. If for some reason it can not
 * get hold of a free buffer it will log error. We copy the data here to
 * a free buffer from the Ep0 logical endpoint object. We expect the EP0
 * control data to be small. Most of the cases this function will get called
 * from the Interrupt domain. typically from the peripheral drivers ISR. We have
 * to protect from oversleves from other interrupts.
 *
 * @param pDevO Identifies the device object
 * @param pData Pointer to the data
 * @param length Length of the data in bytes
 *
 * @return 1 upon success
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
s32_t SendEpZeroData(DEVICE_OBJECT *pDevO,char *pData,int length)
{
    ADI_DEV_1D_BUFFER   *pBuffer=NULL;
    u32                 TxCount;
    ENDPOINT_OBJECT     *pEpZeroO;
    u32_t               uInterruptStatus,uResult;
    u32                 BytesRemaining;
    u32                 LoopCount = 0;

    USB_ASSERT((pDevO == NULL) || (pData == NULL) || (length == 0));

    USB_EP_INFO *pEpInfo = &pDevO->pEndpointZeroObj->EPInfo;

    /* Get the endpoint zero object */
    pEpZeroO = pDevO->pEndpointZeroObj;

    /* Buffer sizes greater than the EP0 max packet size need to be sent to the Host, 
     * in multiple packets.
     * pEpInfo->TransferSize is used to communicate to the controller driver
     * the total required number of bytes.
    */
    pEpInfo->TransferSize = length;

    /* assign a counter to keep track of the remaining bytes to send */
    BytesRemaining = pEpInfo->TransferSize;

    /* Transmission count initally set to either the max packet size or the
     * total size whichever is smaller
    */
    if (length < pDevO->pDeviceDesc->bMaxPacketSize0) {
        TxCount = length;
    } else {
        TxCount = pDevO->pDeviceDesc->bMaxPacketSize0;
    }

    /* Loop while data remains to be transmitted */
    while (BytesRemaining) 
    {        
        /* If remaining bytes indicates a short packet, reset transmission
         * count to size of short packet
        */
        if (BytesRemaining < pDevO->pDeviceDesc->bMaxPacketSize0) {
            TxCount = BytesRemaining;
        }       
        
        /* enter critical region whilst a buffer is located */
        uInterruptStatus = cli();
        
        pBuffer = (ADI_DEV_1D_BUFFER*)pEpZeroO->EPInfo.pFreeXmtList;

        USB_ASSERT(pBuffer == NULL);

        if(pBuffer)
        {
            /* move the free list to point to the next buffer. */
            pEpZeroO->EPInfo.pFreeXmtList = (ADI_DEV_BUFFER*)pBuffer->pNext;
            /* take off current buffer from the list. */
            pBuffer->pNext = NULL;
        }
        /* exit critical region */
        sti(uInterruptStatus);

        /* Assert if we do not have sufficent buffer space. */
        USB_ASSERT(pBuffer->ElementCount < TxCount);

        memcpy(pBuffer->Data,pData,TxCount);
        pBuffer->ElementCount   = TxCount;
        pBuffer->ElementWidth   = 1;
        pBuffer->CallbackParameter = pBuffer;
        pBuffer->ProcessedElementCount = 0;
        pBuffer->ProcessedFlag  = 0;
      
        BytesRemaining -= TxCount;
        pData += TxCount;
                                       
        USB_ASSERT(pUsbCoreData->pPddHandle == NULL);

        /* Now send the data over the avaialble peripheral driver */
        uResult = adi_dev_Write(pUsbCoreData->pPddHandle,ADI_DEV_1D,(ADI_DEV_BUFFER*)pBuffer);
        USB_ASSERT(uResult != ADI_DEV_RESULT_SUCCESS);
    }
    
    return(1);
}

/*********************************************************************************
 *
 * FreeReceivedPackets - Core Internal Function
 *
 * Process received epzero packet. This rountine to be called once the buffer is
 * successfully processed.
 *
 * @param pDevO Pointer to the device object
 * @param pBuffer Received buffer that needs to be freed.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t FreeReceivedPackets(DEVICE_OBJECT *pDevO,ADI_DEV_BUFFER *pBuffer)
{
ADI_DEV_1D_BUFFER *b = (ADI_DEV_1D_BUFFER*)pBuffer,*p;
EP_ZERO_BUFFER_INFO *pInf= (EP_ZERO_BUFFER_INFO*)b->pAdditionalInfo;
u32_t uInterruptStatus;

        USB_ASSERT(pBuffer == NULL);
    USB_ASSERT(pDevO == NULL);

        /* Check if core has initiated the transfer */
        if(pInf->wCoreControlCode != CORE_CONTROL_CODE)
                return 0;

        uInterruptStatus = cli();
        p = (ADI_DEV_1D_BUFFER*)pDevO->pEndpointZeroObj->EPInfo.pFreeRcvList;

        /* Reset all values of pBuffer entities */
        b->ElementCount =  pInf->wMaxEpZeroBufferSize;
        b->CallbackParameter =b;
        b->ProcessedElementCount =0;
        b->ProcessedFlag =0;
        b->pNext = NULL;

        if(!p){
            pDevO->pEndpointZeroObj->EPInfo.pFreeRcvList = (ADI_DEV_BUFFER*)b;
        }
        else {
            while(p->pNext != NULL)
                p = p->pNext;
            p->pNext = b;
        }
        sti(uInterruptStatus);
        return(1);
}


/*********************************************************************************
 * PeripheralTxCompleteHandler - Core Internal Function
 *
 *  Handles processed transmitted packets. In core if this is called means its the
 *  control packet that is initiated by the core itself. If application
 *  performs dev_write we will detect it in the cores main callback handler
 *  and will invoke the applications configuration specific callback handler.
 *  This function is only called in case of peripheral operating in device mode.
 *
 *  @param AppHandle Application Callback handle
 *  @param wEvent Event thats triggerd
 *  @param pArg Argument to the handler
 *
 *  @return -1 upon failure to process the packet.
 *  @see adi_usb_SetDeviceMode
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static s32_t PeripheralTxCompleteHandler(void *AppHandle,unsigned int wEvent,void *pArg)
{
PDEVICE_OBJECT pDevO = pUsbCoreData->pDeviceHead;
ADI_DEV_1D_BUFFER *b = (ADI_DEV_1D_BUFFER*)pArg,*p;
EP_ZERO_BUFFER_INFO *pInf= (EP_ZERO_BUFFER_INFO*)b->pAdditionalInfo;
u32_t uInterruptStatus;
USB_EP_INFO *pEpInfo = &pUsbCoreData->pDeviceHead->pEndpointZeroObj->EPInfo;

       USB_ASSERT(pDevO == NULL);

       /* Check if core has initiated the transfer */
       if(pInf->wCoreControlCode != CORE_CONTROL_CODE)
       {
             /* call currently active configuraions's EP zero callback */
             USB_ASSERT(pDevO->pActiveConfigObj == NULL);
             pDevO->pActiveConfigObj->EpZeroCallback(AppHandle,wEvent,pArg);

            /* Reset state machine, for legacy class drivers( direct EP0 writes )
             */
              pEpInfo->EpState  = EP_STATE_IDLE;
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

        if(!p) {
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
        else
        {
             /*  check if application has initiated the ep0 transfer for this device */
              if(pDevO->pAppEpZeroBuffer != NULL)
              {
                    /* move to the status phase */    
                  pDevO->pEndpointZeroObj->EPInfo.EpStateMachineHandler(pDevO,0,pDevO->pSetupData);
                  /* return back the application buffer */
                  pDevO->pActiveConfigObj->EpZeroCallback(AppHandle,wEvent,(void*)pDevO->pAppEpZeroBuffer); 
                  /* reset the buffer */
                  pDevO->pAppEpZeroBuffer = NULL;
              }
              pDevO->pEndpointZeroObj->EPInfo.EpStateMachineHandler(pDevO,0,pDevO->pSetupData);
        }

        return(1);
}

/*********************************************************************************
 * PeripheralRxCompleteHandler - Core Internal Function
 *
 * Peripheral Rx Complete handler. In case of peripheral mode if the event is
 * other than the USB_EVENT_SETUP_PKT on the Epzero means the data is destinated
 * to the applicaiton. We will invoke the currently active configuraitons EpZero
 * callback handler with the USB_EVENT_RX_COMPLETE event.
 *
 * @param AppHandle Application handle
 * @param wEvent   Indentifies the event
 * @param pArg     Argument to the callback varies with the wEvent
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static s32_t PeripheralRxCompleteHandler(void *AppHandle,unsigned int wEvent, void *pArg)
{
PDEVICE_OBJECT pDevO = pUsbCoreData->pDeviceHead;

   USB_ASSERT(pDevO->pActiveConfigObj == NULL);
   pDevO->pActiveConfigObj->EpZeroCallback(AppHandle,wEvent,pArg);
   return(1);
}

/*********************************************************************************
 * GetDescriptor  - Core Internal function
 *
 * Depending on the request in the setup packet this function sends
 * the requested Device or Configuration or String or other descriptor
 * to the host. Only used in case peripheral is operating in device mode.
 *
 * @param pDevO pointer to the device object
 * @param pSetupPkt pointer to the setup packet.
 *
 * @return 1 upon success
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static int GetDescriptor(PDEVICE_OBJECT pDevO,PSETUP_PACKET pSetupPkt)
{
    int  bytesToReturn=0,totalLen=0,ret=0;
    USB_EP_INFO *pEpInfo = &pUsbCoreData->pDeviceHead->pEndpointZeroObj->EPInfo;

    USB_ASSERT((pDevO == NULL) || (pSetupPkt == NULL));

    /* call the state machine handler. */
    USB_ASSERT(pDevO->pEndpointZeroObj->EPInfo.EpStateMachineHandler == NULL);
    
    /* puts the state machine in DATA phase or status phase depending on whether there is data phase or not */
    pDevO->pEndpointZeroObj->EPInfo.EpStateMachineHandler(pDevO,((pSetupPkt->wLength > 0) ? 0 :1),pSetupPkt);

    switch(HIGH_BYTE_LW(pSetupPkt->wValue))
    {
        case TYPE_DEVICE_DESCRIPTOR:
            USB_LOG_EVENT(DSD(GET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: DTH-STD-RD-GET_DEVICE_DESCRIPTOR");
            bytesToReturn = (pSetupPkt->wLength < sizeof(DEVICE_DESCRIPTOR)) ?
            pSetupPkt->wLength:sizeof(DEVICE_DESCRIPTOR);
            /* we do not expect host asks to return zero bytes. */
            USB_ASSERT(bytesToReturn == 0);
            /* device descriptor is configured? */
            USB_ASSERT(pDevO->pDeviceDesc->bLength != sizeof(DEVICE_DESCRIPTOR));

            pEpInfo->TransferSize = bytesToReturn;
            pDevO->pSetupData = (u8_t*)pDevO->pDeviceDesc;
            ret = SendEpZeroDataEx(pDevO,(u8_t*)pDevO->pDeviceDesc,bytesToReturn);            
        break;

        case TYPE_DEVICEQUALIFIER_DESCRIPTOR:
            /* We will assert if device qualifier descriptor is NULL. */
            USB_LOG_EVENT(DSD(GET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: DTH-STD-RD-GET_DEVICE_QUALIFIER_DESCRIPTOR");
            USB_ASSERT(pDevO->pDeviceQuaDesc == NULL);
            USB_ASSERT(pDevO->pDeviceQuaDesc->bLength != sizeof(DEVICE_QUALIFIER_DESCRIPTOR));
            bytesToReturn = (pSetupPkt->wLength < sizeof(DEVICE_QUALIFIER_DESCRIPTOR)) ?
            pSetupPkt->wLength:sizeof(DEVICE_QUALIFIER_DESCRIPTOR);
            pEpInfo->TransferSize = bytesToReturn;
            pDevO->pSetupData = (u8_t*)pDevO->pDeviceQuaDesc;
            ret = SendEpZeroDataEx(pDevO,(u8_t*)pDevO->pDeviceQuaDesc,bytesToReturn);
        break;

        case TYPE_CONFIGURATION_DESCRIPTOR:
            USB_LOG_EVENT(DSD(GET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: DTH-STD-RD-GET_CONFIGURATION_DESCRIPTOR");
            memset(ConfigMemoryArea,0,sizeof(ConfigMemoryArea));
            totalLen = GetTotalConfiguration(pDevO->ID,(void*)&ConfigMemoryArea[0],sizeof(ConfigMemoryArea),false);
            bytesToReturn = (pSetupPkt->wLength < totalLen) ? pSetupPkt->wLength : totalLen;
            /* we do not expect host asks to return zero bytes. */
            USB_ASSERT(bytesToReturn == 0);
            pEpInfo->TransferSize = bytesToReturn;
            pDevO->pSetupData = (u8_t*)ConfigMemoryArea;
            ret = SendEpZeroDataEx(pDevO,(u8_t*)pDevO->pDeviceDesc,bytesToReturn);
        break;

        case TYPE_OTHER_SPEED_CONFIGURATION_DESCRIPTOR:
            USB_LOG_EVENT(DSD(GET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: DTH-STD-RD-GET_OTHER_SPEED_CONFIGURATION_DESCRIPTOR");
            memset(ConfigMemoryArea,0,sizeof(ConfigMemoryArea));
            /* get total other speed configuraiton */
            totalLen = GetTotalConfiguration(pDevO->ID,(void*)&ConfigMemoryArea[0],sizeof(ConfigMemoryArea),true);
            bytesToReturn = (pSetupPkt->wLength < totalLen) ? pSetupPkt->wLength : totalLen;
            /* we do not expect host asks to return zero bytes. */
            USB_ASSERT(bytesToReturn == 0);
            pEpInfo->TransferSize = bytesToReturn;
            pDevO->pSetupData = (u8_t*)ConfigMemoryArea;
            ret = SendEpZeroDataEx(pDevO,(u8_t*)ConfigMemoryArea,bytesToReturn);
        break;

        case TYPE_STRING_DESCRIPTOR:
            USB_LOG_EVENT(DSD(GET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: DTH-STD-RD-GET_STRING_DESCRIPTOR");

            /* For newly installing drivers Windows is querying for string index 238 which is not
             * valid index for us.
             */
             if((LOW_BYTE_LW(pSetupPkt->wValue)) <= USB_MAX_STRINGS)
             {
                  bytesToReturn = (pSetupPkt->wLength < pDevO->ppStringDescriptors[LOW_BYTE_LW(pSetupPkt->wValue)]->bLength) ?
                  pSetupPkt->wLength:pDevO->ppStringDescriptors[LOW_BYTE_LW(pSetupPkt->wValue)]->bLength;
                  /* we do not expect host asks to return zero bytes. */
                  USB_ASSERT(bytesToReturn == 0);
                  /* valid string descriptor? */
                  USB_ASSERT(LOW_BYTE_LW(pSetupPkt->wValue) >= USB_MAX_STRINGS);

                  pEpInfo->TransferSize = bytesToReturn;
                  pDevO->pSetupData = (u8_t*)pDevO->ppStringDescriptors[LOW_BYTE_LW(pSetupPkt->wValue)];
                  ret = SendEpZeroDataEx(pDevO,(u8_t*)pDevO->ppStringDescriptors[LOW_BYTE_LW(pSetupPkt->wValue)],bytesToReturn);
             }
             else
             {
                  /* Send a zero as response packet */
                  int wStatus = 0x0;
                  pEpInfo->TransferSize = sizeof(char);
                  pDevO->pSetupData = (u8_t*)&wStatus;
                  ret = SendEpZeroDataEx(pDevO,(u8_t*)&wStatus,sizeof(char));
             }
        break;

        default:
        break;
    }

    /* change the state depending on the success or failure. */
    if(ret > 0 )
      pDevO->pEndpointZeroObj->EPInfo.EpStateMachineHandler(pDevO,0,pSetupPkt);
    
    return(ret);
}

/*********************************************************************************
 * FindEndpointObject - Core Internal function
 *
 * Finds the endpoint object in the configuraiton
 *
 * @param pDevO Pointer to the device object
 * @param pConfigO Pointer to the configuraton object
 * @param wIndex Endpoint object ID
 *
 * @return  Upon success endppoint object pointer is returned and NULL is returned 
 *          upon failure.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static ENDPOINT_OBJECT* FindEndpointObject(PDEVICE_OBJECT pDevO,PCONFIG_OBJECT pConfigO,u16_t wIndex)
{
ENDPOINT_OBJECT *pEpO=NULL;

    /* We expect to have some endpoints in the active configuration. */
    USB_ASSERT((pConfigO == NULL) || (pConfigO->pInterfaceObj->pEndpointObj == NULL));

    pEpO = pConfigO->pInterfaceObj->pEndpointObj;

    while(pEpO != NULL)
    {
            /* if passed endpoint object is same as the one */
        if(pEpO->pEndpointDesc->bEndpointAddress == (u8_t)wIndex)
                break;
        else
            pEpO = pEpO->pNext;
    }
    return pEpO;
}

/*********************************************************************************
 * GetEndpointStatus - Core Internal Function
 *
 * Gets the endpoint status to the host. Only used in case of peripheral operating 
 * in device mode.
 *
 * @param pDevO Pointer to the device object
 * @param pSetupPkt Pointer to the setup packet
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static int GetEndpointStatus(PDEVICE_OBJECT pDevO,PSETUP_PACKET pSetupPkt)
{
ENDPOINT_OBJECT  *pEpO;
u32_t            uResult;
u16_t            wStatus=0x00;

    /* return the endpoint status specified in the wIndex.
     * We expect the query for the endpoint would be for the currently active configuration
     */
    USB_ASSERT(pDevO->pActiveConfigObj == NULL);

    /* Ref: pg 256 of USB 2.0 specification
     * We do not expect the request to be for the Endpoint zero.
     * Endpoint zero typically will not Halt. If host asks for it
     * then its some fatal error condition.
     */
    USB_ASSERT(pSetupPkt->wIndex == 0);

    pEpO = FindEndpointObject(pDevO,pDevO->pActiveConfigObj,pSetupPkt->wIndex);

    USB_ASSERT(pEpO == NULL);

    /* found the logical endpoint object now findout the physical endpoint state
     * Bit 0 indicates if the Endpoint is in Halt state. Interrupt and Bulk
     * mode endpoint types have to implement this feature.
     * Ref: USB 2.0 specification pg 256 figure 9-6
     */
    wStatus = pEpO->EPInfo.bnEpHalt ? 0x01 : 0x00;
    
    SendEpZeroData(pDevO,(char*)&wStatus,sizeof(wStatus));
    return(-1);
}

/*********************************************************************************
 * PeripheralBusEventHandler - Core Internal API
 *
 * Event handler to handle bus events in case the peripheral is operating in
 * device mode.
 *
 * @param AppHandle Application handle
 * @param wEvent  Event in hand
 * @param pArg    Pointer to data which varies depending on the wEvent
 *
 * @return 1 upon success
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static s32_t PeripheralBusEventHandler(void *AppHandle,unsigned int wEvent, void *pArg)
{
    PCONFIG_OBJECT pConfigO = pUsbCoreData->pDeviceHead->pConfigObj;

    switch(wEvent)
    {
            case ADI_USB_EVENT_START_OF_FRAME:
                break;

            case ADI_USB_EVENT_ROOT_PORT_RESET:
                break;

            case ADI_USB_EVENT_RESUME:
                break;

            case ADI_USB_EVENT_SUSPEND:
                break;

            case ADI_USB_EVENT_VBUS_TRUE:
                break;

            case ADI_USB_EVENT_VBUS_FALSE:
                break;

            default:
                break;
    }

    /* call the active configuration's EP0 callback in case they want to take action */

    /* TODO - should we only call the active configuration, or each config registered? */
    /* for testing, we're calling each config because at startup no one is the active
       config so no one is aware of bus events */
#if 1       
    while(pConfigO != NULL)
    {
        /* if a callback was supplied call it now */
        if ( pConfigO->EpZeroCallback )
            pConfigO->EpZeroCallback(AppHandle,wEvent,pArg);

        /* go to next object */
        pConfigO = pConfigO->pNext;
    }
#endif

    return(1);
}

/*********************************************************************************
 * EpZeroDeviceStateMachineHandler - Core Internal function
 *
 * Responsible for changing the EP0 state. In case of control transfer during
 * setup process, initially the state would be in EP_IDLE. Once setup packet
 * is received the state will change to SETUP_PHASE
 *
 * State transition happens with the dwInput value.With the below logic
 * By default 0 takes to the next expected good sequence state. 1 considerd as
 * error or reset.
 *
 * EP_IDLE  --> 0 --> EP_SETUP_RQST_PHASE
 * EP_SETUP_RQST_PHASE --> 0 --> EP_SETUP_DATA_PHASE
 * EP_SETUP_RQST_PHASE --> 1 --> EP_IDLE
 * EP_SETUP_DATA_PHASE --> 0 --> EP_SETUP_STATUS_PHASE
 * EP_SETUP_DATA_PHASE --> 1 --> EP_SETUP_DATA_PHASE ( sends packets remaining packets )
 * EP_SETUP_STATUS_PHASE --> 0 --> EP_IDLE
 *
 * @param dwEpNum  Endpoint number
 * @param dwInput  Input that takes to the next stage
 * @param p       Pointer to data whose value depend on the state
 *
 * @return   1 upon success
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static int EpZeroDeviceStateMachineHandler(DEVICE_OBJECT *pDevO ,int dwInput, void *p)
{
USB_EP_INFO *pEpInfo = &pUsbCoreData->pDeviceHead->pEndpointZeroObj->EPInfo;
u32_t ReturnValue=1;

        USB_ASSERT(pEpInfo == NULL);

        switch(pEpInfo->EpState)
        {
            case EP_STATE_IDLE:
            {
                adi_dev_Control(pUsbCoreData->pPddHandle,
                                ADI_USB_CMD_SETUP_RESPONSE,
                                (void*)ADI_USB_SETUP_RESPONSE_ACK);

                PSETUP_PACKET pSetupPkt = (PSETUP_PACKET)p;
                pEpInfo->EpDir = (pSetupPkt->bmRequestType & USB_DIR_DEVICE_TO_HOST);
                pEpInfo->EpNumTotalBytes = 0;
                pEpInfo->EpBytesProcessed = 0;

               /* in case of trasmit the calling entity must set the transfer size as 
                * setup packet may not reflect the exact transfer size. 
                */
                if(pEpInfo->EpDir == USB_DIR_HOST_TO_DEVICE)
                   pEpInfo->TransferSize = pSetupPkt->wLength; 

                pEpInfo->EpState = EP_STATE_SETUP_RQST_PHASE;
            }
            break;

            case EP_STATE_SETUP_RQST_PHASE:
        {
                /*  We have some data to receive or to send. */
                pEpInfo->EpState = (dwInput == 0) ? EP_STATE_SETUP_DATA_PHASE : EP_STATE_SETUP_STATUS_PHASE;
        }
            break;
 
            case EP_STATE_SETUP_DATA_PHASE:
        {
                if(dwInput == 0)
                {
                    if(pEpInfo->EpNumTotalBytes == pEpInfo->TransferSize)
                    {
                        adi_dev_Control(pUsbCoreData->pPddHandle,
                                        ADI_USB_CMD_SETUP_RESPONSE,
                                        (void*)ADI_USB_SETUP_RESPONSE_ACK_DATA_END);
                        pEpInfo->EpState = EP_STATE_SETUP_STATUS_PHASE;
                     }
                }
                else
                {
                     /* Host is sending us data - Rx */
                     if(pEpInfo->EpDir == USB_DIR_HOST_TO_DEVICE)
                     {
                        ADI_DEV_1D_BUFFER *pDataBuffer = (ADI_DEV_1D_BUFFER*)p;

                        memcpy((u8_t*)(pDevO->pSetupData + pEpInfo->EpNumTotalBytes),
                                       pDataBuffer->Data,
                                       pDataBuffer->ProcessedElementCount);

                        pEpInfo->EpNumTotalBytes += pDataBuffer->ProcessedElementCount;

                        adi_dev_Control(pUsbCoreData->pPddHandle,
                                        ADI_USB_CMD_SETUP_RESPONSE,
                    (void*)ADI_USB_SETUP_RESPONSE_ACK);

                         /* let the caller know we have not done yet so staying in data-phase */
                         if(pEpInfo->EpNumTotalBytes < pEpInfo->TransferSize)
                             ReturnValue = 0x0;
                    }
                    else /* Tx */
                    {
                        /* we are sending data, we will get called from EP0 Tx handler 
                         * in case of multuiple packets 
                         */ 
                         ReturnValue = SendEpZeroDataEx(pDevO,pDevO->pSetupData,1);
                    }
                }

            }
            break;
            
            case EP_STATE_SETUP_STATUS_PHASE:
        {
                /* issue i/o control to the driver to perform status handshake
                 * Any additional operations that needs to be done at the driver level in status phase.
                 */
                 adi_dev_Control(pUsbCoreData->pPddHandle,ADI_USB_CMD_ENABLE_CNTRL_STATUS_HANDSHAKE,(void*)0);
                 pEpInfo->EpState = EP_STATE_IDLE;
        }
            break;
        }
        return(ReturnValue);
}

/*********************************************************************************
 * PeripheralSetupPktProcessHandler - Core Internal Function
 *
 * Responsible for handling the setup packets. Invoked in case of MODE_DEVICE in 
 * response of USB_EVENT_SETUP_PKT Handles all device side setup events like 
 * GetDescriptor, SetConfiguration, GetStatus etc
 *
 * @param AppHandle Application's Callback handle
 * @param wEvent Identifies the event its USB_EVENT_SETUP_PKT when this function is invoked
 * @param pArg   Pointer to data, Most of the cases it points to the Setup Packet
 *
 * @return 1 upon success
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static s32_t PeripheralSetupPktProcessHandler(void *AppHandle,unsigned int wEvent, void *pArg)
{
ADI_DEV_1D_BUFFER   *pDevBuf = (ADI_DEV_1D_BUFFER*)pArg;
SETUP_PACKET        *pSetupPkt = pDevBuf->Data,*pActiveSetupPkt;
DEVICE_OBJECT       *pCurDeviceO = pUsbCoreData->pDeviceHead;  /* head should be set to point active device */
CONFIG_OBJECT       *pActiveConfigO = pCurDeviceO->pActiveConfigObj; /* currently active configuration object */
u16_t           wStatus=0x00;
USB_EP_INFO             *pEpInfo = &pCurDeviceO->pEndpointZeroObj->EPInfo; /* Endpoint zero object */

pActiveSetupPkt = &pCurDeviceO->ActiveSetupPkt;

    USB_ASSERT(pSetupPkt == NULL);
    USB_ASSERT(pEpInfo->EpStateMachineHandler == NULL);
    /* we have to be in device mode whilst processing setup packets */
    USB_ASSERT(pUsbCoreData->eDevMode != MODE_DEVICE);

    switch(pEpInfo->EpState)
    {
         /*
          * Setup packet. Save it off and continue with next phase
          */
          case EP_STATE_IDLE:
      {
              /* save off the setup packet in the device object */ 
               memcpy(pActiveSetupPkt,pSetupPkt,sizeof(SETUP_PACKET));
      }

          break;

        /* 
         * We continue to stay in data phase if more data has to be received.
         */
          case EP_STATE_SETUP_DATA_PHASE:
      {
              /* transfer is completed if return value is 1 */
              if(pEpInfo->EpStateMachineHandler(pCurDeviceO, 1, pDevBuf) == 1)
              {
                pSetupPkt = pActiveSetupPkt;
              }
              else
              {
                 /* Free the data packet and stay in data phase */
                 FreeReceivedPackets(pCurDeviceO,(ADI_DEV_BUFFER*)pArg);
                 return (0);
              }

      }
          break;

          default:
      break;
    }

    /* Processing the setup packet type. Check pg 248 of USB 2.0 Specification */
    switch(pSetupPkt->bmRequestType)
    {
        /**********************************************************
         * HOST to DEVICE --- STANDARD --- DEVICE
         **********************************************************/

        case USB_DIR_HOST_TO_DEVICE | USB_TYPE_STANDARD | USB_RECIPIENT_DEVICE:
        {

            switch(pSetupPkt->bRequest)
            {
                case USB_STD_RQST_GET_STATUS:
                    USB_LOG_EVENT(HSD(GET_STATUS),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-GET_STATUS");
                break;

                case USB_STD_RQST_CLEAR_FEATURE:

                    USB_LOG_EVENT(HSD(CLEAR_FEATURE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-CLEAR_FEATURE");
                    /* Clear the feature to wake up a suspended host  */
                    if(USB_STANDARD_FEATURE_DEVICE_REMOTE_WAKEUP == pSetupPkt->wValue)
                    {
                        pCurDeviceO->bnHostWakeupEnable = false;
                    }
                break;

                case USB_STD_RQST_SET_FEATURE:

                    switch(pSetupPkt->wValue)
                    {
                        case USB_STANDARD_FEATURE_DEVICE_REMOTE_WAKEUP:
                            USB_LOG_EVENT(HSD(SET_FEATURE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-SET_FEATURE_REMOTE_WAKEUP");
                            pCurDeviceO->bnHostWakeupEnable = true;

                            /* if a device reports its remote-wakeup capability its
                             * configuration should also report the same. if not its error
                             * check with currently active configuration.
                             */
                             USB_ASSERT(!(pActiveConfigO->pConfigDesc->bAttributes & (1 << 5))); /* bit 5 */
                        break;

                        case USB_STANDARD_FEATURE_TEST_MODE:
                             USB_LOG_EVENT(HSD(SET_FEATURE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-SET_FEATURE_TEST_MODE");
                             /* acknowledge the setup packet */
                             adi_dev_Control(pUsbCoreData->pPddHandle,
                                            ADI_USB_CMD_SETUP_RESPONSE,
                                            (void*)ADI_USB_SETUP_RESPONSE_ACK);
                                            
                                            
                             /* set data end - no data phase for test mode*/                
                             adi_dev_Control(pUsbCoreData->pPddHandle,
                                              ADI_USB_CMD_SETUP_RESPONSE,
                                              (void*)ADI_USB_SETUP_RESPONSE_ACK_DATA_END);

                              /* issue i/o control with the requested test mode */
                             adi_dev_Control(pUsbCoreData->pPddHandle,
                                              ADI_USB_CMD_ENTER_TEST_MODE,
                                              (void*)pSetupPkt->wIndex);
                        break;

                        default:
                            USB_LOG_EVENT(HSD(SET_FEATURE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-SET_FEATURE_UNKNOWN");
                        break;
                    }
                break;

                case USB_STD_RQST_SET_ADDRESS:

                    //USB_ASSERT(pEpInfo->EpState  != EP_STATE_IDLE);
           
                    pEpInfo->EpState = EP_STATE_IDLE;

                    /* Moves from IDLE state to SETUP REQ state */
                    pCurDeviceO->pEndpointZeroObj->EPInfo.EpStateMachineHandler(pCurDeviceO,0,pSetupPkt);
                    USB_LOG_EVENT(HSD(SET_ADDRESS),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD_SET_ADDRESS");
                    pCurDeviceO->wDeviceAddress =  pSetupPkt->wValue;


                    adi_dev_Control(pUsbCoreData->pPddHandle,ADI_USB_CMD_SET_DEV_ADDRESS,(void*)pSetupPkt->wValue);

                    /* Moves from SETUP REQ to STATUS phase - No data phase  */
                    pCurDeviceO->pEndpointZeroObj->EPInfo.EpStateMachineHandler(pCurDeviceO,((pSetupPkt->wLength > 0) ? 0 :1),pSetupPkt);

                    /* Moves from Status to IDLE */
                    pCurDeviceO->pEndpointZeroObj->EPInfo.EpStateMachineHandler(pCurDeviceO,0,pSetupPkt);
                    pCurDeviceO->wDeviceState = DEVICE_STATE_ADDRESS;

                break;

                case USB_STD_RQST_GET_DESCRIPTOR:
                    USB_LOG_EVENT(HSD(GET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-GET_DESCRIPTOR");
                break;

                case USB_STD_RQST_SET_DESCRIPTOR:
                    USB_LOG_EVENT(HSD(SET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-SET_DESCRIPTOR");
                break;

                case USB_STD_RQST_GET_CONFIGURATION:
                    USB_LOG_EVENT(HSD(GET_CONFIGURATION),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-GET_CONFIGURATION");
                break;

                case USB_STD_RQST_SET_CONFIGURATION:
                    USB_LOG_EVENT(HSD(SET_CONFIGURATION),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-SET_CONFIGURATION");
               pEpInfo->EpState  = EP_STATE_IDLE;

                    adi_dev_Control(pUsbCoreData->pPddHandle,ADI_USB_CMD_SETUP_RESPONSE,(void*)ADI_USB_SETUP_RESPONSE_ACK);

                    /* sets the configuration */
                    if(SetConfiguration(pCurDeviceO,LOW_BYTE_LW(pSetupPkt->wValue),AppHandle) == -1)
                    {
                        /* If we fail to set the selected configuration we set back to 0 */
                        SetConfiguration(pCurDeviceO,(u8_t)0,AppHandle);
                        /* TODO: Respond with request error */
                    }
                    adi_dev_Control(pUsbCoreData->pPddHandle,ADI_USB_CMD_ENABLE_CNTRL_STATUS_HANDSHAKE,(void*)0);
                break;

                case USB_STD_RQST_GET_INTERFACE:
                    USB_LOG_EVENT(HSD(GET_INTERFACE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-GET_INTERFACE");
                break;

                case USB_STD_RQST_SET_INTERFACE:
                    USB_LOG_EVENT(HSD(SET_INTERFACE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-SET_INTERFACE");
                break;

                case USB_STD_RQST_SYNCH_FRAME:
                    USB_LOG_EVENT(HSD(SYNCH_FRAME),pSetupPkt->wValue,"SetupPkt: HTD-STD-RD-SYNCH_FRAME");
                break;
            }

        }
        break;   /* case USB_DIR_HOST_TO_DEVICE | USB_TYPE_STANDARD | USB_RECIPIENT_DEVICE: */

        /**********************************************************
         * HOST to DEVICE --- STANDARD --- INTERFACE
         **********************************************************/

        case USB_DIR_HOST_TO_DEVICE | USB_TYPE_STANDARD | USB_RECIPIENT_INTERFACE:
        {

            switch(pSetupPkt->bRequest)
            {
                case USB_STD_RQST_GET_STATUS:
                    USB_LOG_EVENT(HSI(GET_STATUS),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-GET_STATUS");
                break;

                case USB_STD_RQST_CLEAR_FEATURE:
                    USB_LOG_EVENT(HSI(CLEAR_FEATURE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-CLEAR_FEATURE");
                break;

                case USB_STD_RQST_SET_FEATURE:
                    USB_LOG_EVENT(HSI(SET_FEATURE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-SET_FEATURE");
                break;

                case USB_STD_RQST_SET_ADDRESS:
                    USB_LOG_EVENT(HSI(SET_ADDRESS),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-SET_ADDRESS");
                break;

                case USB_STD_RQST_GET_DESCRIPTOR:
                    USB_LOG_EVENT(HSI(GET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-GET_DESCRIPTOR");
                break;

                case USB_STD_RQST_SET_DESCRIPTOR:
                    USB_LOG_EVENT(HSI(SET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-SET_DESCRIPTOR");
                break;

                case USB_STD_RQST_GET_CONFIGURATION:
                    USB_LOG_EVENT(HSI(GET_CONFIGURATION),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-GET_CONFIGURATION");
                break;

                case USB_STD_RQST_SET_CONFIGURATION:
                    USB_LOG_EVENT(HSI(SET_CONFIGURATION),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-SET_CONFIGURATION");
                break;

                case USB_STD_RQST_GET_INTERFACE:
                    USB_LOG_EVENT(HSI(GET_INTERFACE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-GET_INTERFACE");
                break;
                case USB_STD_RQST_SET_INTERFACE:
                {
                    USB_LOG_EVENT(HSI(SET_INTERFACE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-SET_INTERFACE");
                    adi_dev_Control(pUsbCoreData->pPddHandle,ADI_USB_CMD_SETUP_RESPONSE,(void*)ADI_USB_SETUP_RESPONSE_ACK);
#if 1                    
                    if(SetInterface(pCurDeviceO,LOW_BYTE_LW(pSetupPkt->wIndex),LOW_BYTE_LW(pSetupPkt->wValue),AppHandle) == (u8_t)-1)
                    {
                        /* TODO: failure respond with stall or Request Error. */
                    }
                    adi_dev_Control(pUsbCoreData->pPddHandle,ADI_USB_CMD_ENABLE_CNTRL_STATUS_HANDSHAKE,(void*)0);
#endif           
                }         
                break;

                case USB_STD_RQST_SYNCH_FRAME:
                    USB_LOG_EVENT(HSI(SYNCH_FRAME),pSetupPkt->wValue,"SetupPkt: HTD-STD-RI-SYNCH_FRAME");
                break;
            }

        }
        break; /* case USB_DIR_HOST_TO_DEVICE | USB_TYPE_STANDARD | USB_RECIPIENT_INTERFACE: */

        /**********************************************************
         * HOST to DEVICE --- STANDARD --- ENDPOINT
         **********************************************************/

        case USB_DIR_HOST_TO_DEVICE | USB_TYPE_STANDARD | USB_RECIPIENT_ENDPOINT:
        {

            switch(pSetupPkt->bRequest)
            {
                ENDPOINT_OBJECT *pEpO;

                case USB_STD_RQST_GET_STATUS:
                    USB_LOG_EVENT(HSE(GET_STATUS),pSetupPkt->wValue,"SetupPkt: HTD-STD-RE-GET_STATUS");
                break;

                case USB_STD_RQST_CLEAR_FEATURE:

                    USB_LOG_EVENT(HSE(CLEAR_FEATURE),
                      ((pSetupPkt->wValue << 16) | pSetupPkt->wIndex),
                      "SetupPkt: HTD-STD-RE-CLEAR_FEATURE");

                    pEpO = FindEndpointObject(pCurDeviceO,pActiveConfigO,pSetupPkt->wIndex);
                    USB_ASSERT(pEpO == NULL);

                    USB_ASSERT(pSetupPkt->wValue != USB_STANDARD_FEATURE_ENDPOINT_HALT);

                    /* We can only get this request
                     * Ref: USB 2.0 specification pg 256 for details
                     */
                    if(pSetupPkt->wValue == USB_STANDARD_FEATURE_ENDPOINT_HALT)
                    {
                        /* We already got the logical endpoint object in pEpO */
                        pEpO->EPInfo.bnEpHalt =  false;

                        /* inform the peripheral driver to clear the halt on the endpoint */
                        //adi_dev_Control(pUsbCoreData->pPddHandle, ADI_USB_CMD_CLEAR_STALL_EP, (void*)pEpO);
                    }
                    adi_dev_Control(pUsbCoreData->pPddHandle,
                                        ADI_USB_CMD_SETUP_RESPONSE,
                                        (void*)ADI_USB_SETUP_RESPONSE_ACK_DATA_END);

                 pEpInfo->EpState  = EP_STATE_IDLE;
                break;

                case USB_STD_RQST_SET_FEATURE:

                    USB_LOG_EVENT(HSE(SET_FEATURE),
                      ((pSetupPkt->wValue << 16) | pSetupPkt->wIndex),
                      "SetupPkt: HTD-STD-RE-SET_FEATURE");                   
                    
                    pEpO = FindEndpointObject(pCurDeviceO,pActiveConfigO,pSetupPkt->wIndex);

                    USB_ASSERT(pEpO == NULL);

                    USB_ASSERT(pSetupPkt->wValue != USB_STANDARD_FEATURE_ENDPOINT_HALT);
                    /* We can only get this request
                     * Ref: USB 2.0 specification pg 256 for details
                     */
                    if(pSetupPkt->wValue == USB_STANDARD_FEATURE_ENDPOINT_HALT)
                    {
                        /* We already got the logical endpoint object in pEpO */
                        pEpO->EPInfo.bnEpHalt =  true;

                        /* inform the peripheral driver to set halt on the endpoint */
                        //adi_dev_Control(pUsbCoreData->pPddHandle, ADI_USB_CMD_SET_STALL_EP, (void*)pEpO);
                    }
                    
                    adi_dev_Control(pUsbCoreData->pPddHandle,
                                        ADI_USB_CMD_SETUP_RESPONSE,
                                        (void*)ADI_USB_SETUP_RESPONSE_ACK_DATA_END);
                                        
                pEpInfo->EpState  = EP_STATE_IDLE;
                break;

                case USB_STD_RQST_SET_ADDRESS:
                    USB_LOG_EVENT(HSE(SET_ADDRESS),pSetupPkt->wValue,"SetupPkt: HTD-STD-RE-SET_ADDRESS");
                break;
                case USB_STD_RQST_GET_DESCRIPTOR:
                    USB_LOG_EVENT(HSE(GET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: HTD-STD-RE-GET_DESCRIPTOR");
                break;
                case USB_STD_RQST_SET_DESCRIPTOR:
                    USB_LOG_EVENT(HSE(SET_DESCRIPTOR),pSetupPkt->wValue,"SetupPkt: HTD-STD-RE-SET_DESCRIPTOR");
                break;
                case USB_STD_RQST_GET_CONFIGURATION:
                    USB_LOG_EVENT(HSE(GET_CONFIGURATION),pSetupPkt->wValue,"SetupPkt: HTD-STD-RE-GET_CONFIGURATION");
                break;
                case USB_STD_RQST_SET_CONFIGURATION:
                    USB_LOG_EVENT(HSE(SET_CONFIGURATION),pSetupPkt->wValue,"SetupPkt: HTD-STD-RE-SET_CONFIGURATION");
                break;
                case USB_STD_RQST_GET_INTERFACE:
                    USB_LOG_EVENT(HSE(GET_INTERFACE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RE-GET_INTERFACE");
                break;
                case USB_STD_RQST_SET_INTERFACE:
                    USB_LOG_EVENT(HSE(SET_INTERFACE),pSetupPkt->wValue,"SetupPkt: HTD-STD-RE-SET_INTERFACE");
                break;
                case USB_STD_RQST_SYNCH_FRAME:
                    USB_LOG_EVENT(HSE(SYNCH_FRAME),pSetupPkt->wValue,"SetupPkt: HTD-STD-RE-SYNCH_FRAME");
                break;
            }

        }
        break; /* case USB_DIR_HOST_TO_DEVICE | USB_TYPE_STANDARD | USB_RECIPIENT_ENDPOINT: */

        /**********************************************************
         * DEVICE to HOST --- STANDARD --- DEVICE
         **********************************************************/

        case USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD | USB_RECIPIENT_DEVICE:
        {
            switch(pSetupPkt->bRequest)
            {
                case USB_STD_RQST_GET_STATUS:
                    USB_LOG_EVENT(DSD(GET_STATUS),pSetupPkt->wValue,"SetupPkt: DTH-STD-RD-GET_STATUS");
                    wStatus  = 0x0;
                    /* Ref: USB 2.0 specification document pg 255 (Fig 9.4,9.5)
                     * Bit:0 - self powered,  Bit:1 - Remote wakeup enable, Rest all reserved.
             */
                    if(pCurDeviceO->bnHostWakeupEnable == true)
                    { /* device has remote wakeup capability is enabled inform it to the host */
                        wStatus = 0x02; /* Bit postion 1 set */
                        /* Active configuraiton must have the remote wakeup capability enabled
                         * Bit-5 will be set if remote wakeup capability is enabled.
                         */
                        USB_ASSERT(!(pActiveConfigO->pConfigDesc->bAttributes & (1 << 5))); /* bit 5 */
                    }
                    else { wStatus = 0x00; }

                    /* check bit-6 to see if its self-powered. If so inform to the host
                     * Ref: USB 2.0 Specification pg 266 or table 9-10
                     *      USB 2.0 Specification pg 255  figure 9-4
                     */
                    if(pActiveConfigO && (pActiveConfigO->pConfigDesc->bAttributes & (1<<6)))
                    {
                        /* A self powered device will not get any power from the USB.
                         * bMaxPower element of active configuration must be zero. For
                         * devices which can swith power sources this value has to be zero.
                         */
                        wStatus |= 0x01; /* set 1 at bit position 0 */
                        /* if bMaxPower is not zero then assert. */
                        USB_ASSERT(pActiveConfigO->pConfigDesc->bMaxPower != 0);
                    }
                    SendEpZeroData(pCurDeviceO,(char*)&wStatus,sizeof(wStatus));
                break;
                case USB_STD_RQST_CLEAR_FEATURE:
                break;
                case USB_STD_RQST_SET_FEATURE:
                break;
                case USB_STD_RQST_SET_ADDRESS:
                break;
                case USB_STD_RQST_GET_DESCRIPTOR:

                    //USB_ASSERT(pEpInfo->EpState != EP_STATE_IDLE);
                    pEpInfo->EpState = EP_STATE_IDLE;

                /* Move state machine from IDLE -> RQST phase */
                    pEpInfo->EpStateMachineHandler(pCurDeviceO,0,(void*)pSetupPkt);
                    GetDescriptor(pCurDeviceO,pSetupPkt);
                break;
                case USB_STD_RQST_SET_DESCRIPTOR:
                break;

                case USB_STD_RQST_GET_CONFIGURATION:

                    USB_LOG_EVENT(DSD(GET_CONFIGURATION),pSetupPkt->wValue,"SetupPkt: DTH-STD-RD-GET_CONFIGURATION");
                    /* Ref: USB 2.0 Specification 9.4.2
                     * Return the currently active configuration number
             */

                    /* Device is not configured return  byte with 0 */
                    if(pCurDeviceO->wDeviceState != DEVICE_STATE_CONFIGURED)
                    {
                        wStatus = 0x0;
                        SendEpZeroData(pCurDeviceO,(char*)&wStatus,sizeof(char));
                    }
                    /* Send the configuration valu  sizeof(char) assumed to be 1 byte */
                    else
                    {
                        USB_ASSERT(pActiveConfigO == NULL);
                        SendEpZeroData(pCurDeviceO,(char*)&pActiveConfigO->pConfigDesc->bConfigurationValue,sizeof(char));
                    }
                break;

                case USB_STD_RQST_SET_CONFIGURATION:
                break;
                case USB_STD_RQST_GET_INTERFACE:
                break;
                case USB_STD_RQST_SET_INTERFACE:
                break;
                case USB_STD_RQST_SYNCH_FRAME:
                break;
            }

        }
        break;  /*case USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD | USB_RECIPIENT_DEVICE: */

        /**********************************************************
         * DEVICE to HOST --- STANDARD --- INTERFACE
         **********************************************************/

        case USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD | USB_RECIPIENT_INTERFACE:
        {
            switch(pSetupPkt->bRequest)
            {
                case USB_STD_RQST_GET_STATUS:
                    /* Ref USB 2.0 Specification document pg 255, fig 9-5
                     * Return always zero for interface
                     */
                    USB_LOG_EVENT(DSI(GET_STATUS),pSetupPkt->wValue,"SetupPkt: DTH-STD-RI-GET_STATUS");
                    USB_ASSERT(pCurDeviceO->wDeviceState != DEVICE_STATE_CONFIGURED);
                    SendEpZeroData(pCurDeviceO,(char*)&wStatus,sizeof(wStatus));
                break;

                case USB_STD_RQST_CLEAR_FEATURE:
                break;
                case USB_STD_RQST_SET_FEATURE:
                break;
                case USB_STD_RQST_SET_ADDRESS:
                break;
                case USB_STD_RQST_GET_DESCRIPTOR:
                break;
                case USB_STD_RQST_SET_DESCRIPTOR:
                break;
                case USB_STD_RQST_GET_CONFIGURATION:
                break;
                case USB_STD_RQST_SET_CONFIGURATION:
                break;

                case USB_STD_RQST_GET_INTERFACE:

                    USB_LOG_EVENT(DSI(GET_INTERFACE),pSetupPkt->wValue,"SetupPkt: DTH-STD-RI-GET_INTERFACE");

                    /* Ref: USB 2.0 specification, pg 254 9.4.4
                     * Returns the selected alternate setting for the specified interface
             */
                    USB_ASSERT(pCurDeviceO->wDeviceState != DEVICE_STATE_CONFIGURED);

                    /* Length specified should be 1 byte */
                    USB_ASSERT(pSetupPkt->wLength != 1);

                    /* we expect the pActiveInterfaceObj in the device object to point to the
                     * currently active interface in the configuration.
             */
                    USB_ASSERT(pCurDeviceO->pActiveInterfaceObj == NULL);

                    /* Check if the requested interface number matches the currently active one
                     * if not we have failed to setup properly.
             */
                    USB_ASSERT(pCurDeviceO->pActiveInterfaceObj->pInterfaceDesc->bInterfaceNumber != pSetupPkt->wIndex);

                    /* If everything looks good then return the alternate setting for the interface
                     * TODO: Check for bAlternateSetting value in the primary interface. Are we swapping values?
                     * SendEpZeroData(pCurDeviceO,(char*)&pActiveInterfaceObj->bAlternateSetting,sizeof(char));
             */

                    /* Host requested for the alternate setting for interface 0 */
                    if(pSetupPkt->wIndex == 0)
                        SendEpZeroData(pCurDeviceO,(char*)&wStatus,sizeof(char));
                break;
                case USB_STD_RQST_SET_INTERFACE:
                break;
                case USB_STD_RQST_SYNCH_FRAME:
                break;
            }

        }
        break;/* case USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD | USB_RECIPIENT_INTERFACE: */

        /**********************************************************
         * DEVICE to HOST --- STANDARD --- ENDPOINT
         **********************************************************/

        case USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD | USB_RECIPIENT_ENDPOINT:
        {

            switch(pSetupPkt->bRequest)
            {
                case USB_STD_RQST_GET_STATUS:
                    USB_LOG_EVENT(DSE(GET_STATUS),pSetupPkt->wValue,"SetupPkt: DTH-STD-RE-GET_STATUS");

                    pEpInfo->EpState = EP_STATE_IDLE;

                    GetEndpointStatus(pCurDeviceO,pSetupPkt);

                    adi_dev_Control(pUsbCoreData->pPddHandle,
                                        ADI_USB_CMD_SETUP_RESPONSE,
                                        (void*)ADI_USB_SETUP_RESPONSE_ACK_DATA_END);
                    pEpInfo->EpState = EP_STATE_IDLE;
                break;

                case USB_STD_RQST_CLEAR_FEATURE:
                break;
                case USB_STD_RQST_SET_FEATURE:
                break;
                case USB_STD_RQST_SET_ADDRESS:
                break;
                case USB_STD_RQST_GET_DESCRIPTOR:
                break;
                case USB_STD_RQST_SET_DESCRIPTOR:
                break;
                case USB_STD_RQST_GET_CONFIGURATION:
                break;
                case USB_STD_RQST_SET_CONFIGURATION:
                break;
                case USB_STD_RQST_GET_INTERFACE:
                break;
                case USB_STD_RQST_SET_INTERFACE:
                break;
                case USB_STD_RQST_SYNCH_FRAME:
                break;
            }

        }
        break;/* case USB_DIR_DEVICE_TO_HOST | USB_TYPE_STANDARD | USB_RECIPIENT_ENDPOINT: */

        /* All class and vendor specific requests get passed to the upper levels, the core
           does not handle those.  It is up to the upper level to send/receive data from the
           host and update the EP0 pEpInfo->EpBytesProcessed member so that we can correctly
           apply the control status phase handshake */

        /**********************************************************
         * HOST to DEVICE --- CLASS --- DEVICE
         **********************************************************/

        case USB_DIR_HOST_TO_DEVICE | USB_TYPE_CLASS | USB_RECIPIENT_DEVICE:

        /**********************************************************
         * HOST to DEVICE --- CLASS --- INTERFACE
         **********************************************************/

        case USB_DIR_HOST_TO_DEVICE | USB_TYPE_CLASS | USB_RECIPIENT_INTERFACE:

        /**********************************************************
         * HOST to DEVICE --- CLASS --- ENDPOINT
         **********************************************************/

        case USB_DIR_HOST_TO_DEVICE | USB_TYPE_CLASS | USB_RECIPIENT_ENDPOINT:

        /**********************************************************
         * HOST to DEVICE --- VENDOR --- DEVICE
         **********************************************************/

        case USB_DIR_HOST_TO_DEVICE | USB_TYPE_VENDOR | USB_RECIPIENT_DEVICE:

        /**********************************************************
         * HOST to DEVICE --- VENDOR --- INTERFACE
         **********************************************************/

        case USB_DIR_HOST_TO_DEVICE | USB_TYPE_VENDOR | USB_RECIPIENT_INTERFACE:

        /**********************************************************
         * HOST to DEVICE --- VENDOR --- ENDPOINT
         **********************************************************/

        case USB_DIR_HOST_TO_DEVICE | USB_TYPE_VENDOR | USB_RECIPIENT_ENDPOINT:

        /**********************************************************
         * DEVICE to HOST --- CLASS --- DEVICE
         **********************************************************/

        case USB_DIR_DEVICE_TO_HOST | USB_TYPE_CLASS | USB_RECIPIENT_DEVICE:

        /**********************************************************
         * DEVICE to HOST --- CLASS --- INTERFACE
         **********************************************************/

        case USB_DIR_DEVICE_TO_HOST | USB_TYPE_CLASS | USB_RECIPIENT_INTERFACE:

        /**********************************************************
         * DEVICE to HOST --- CLASS --- ENDPOINT
         **********************************************************/

        case USB_DIR_DEVICE_TO_HOST | USB_TYPE_CLASS | USB_RECIPIENT_ENDPOINT:

        /**********************************************************
         * DEVICE to HOST --- VENDOR --- DEVICE
         **********************************************************/

        case USB_DIR_DEVICE_TO_HOST | USB_TYPE_VENDOR | USB_RECIPIENT_DEVICE:

        /**********************************************************
         * DEVICE to HOST --- VENDOR --- INTERFACE
         **********************************************************/

        case USB_DIR_DEVICE_TO_HOST | USB_TYPE_VENDOR | USB_RECIPIENT_INTERFACE:

        /**********************************************************
         * DEVICE to HOST --- VENDOR --- ENDPOINT
         **********************************************************/

        case USB_DIR_DEVICE_TO_HOST | USB_TYPE_VENDOR | USB_RECIPIENT_ENDPOINT:

        /* inform upper level of standard request */
        if (pUsbCoreData->pDeviceHead->pActiveConfigObj)
        {
            if( pEpInfo->EpState == EP_STATE_IDLE)
            {
                USB_ASSERT(pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback == NULL);

        /* reset application specific buffer */
        pCurDeviceO->pAppEpZeroBuffer = NULL;

                /* move from IDLE phase to setup request phase */            
                pEpInfo->EpStateMachineHandler(pCurDeviceO,0,(void*)pSetupPkt);
            
                /* if we are about send or receive so enter into data phase */
                pEpInfo->EpStateMachineHandler(pCurDeviceO,((pSetupPkt->wLength > 0) ? 0 :1),pSetupPkt);

                /* let the class driver know about the setup packet */
                if (pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback)
                    pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback(AppHandle, wEvent, pArg);

        /* class drivers may issue STALL which changes the state to IDLE */
        if(pEpInfo->EpState != EP_STATE_IDLE)
        {
                    /* we are about to expect data from host, have the buffer setup to fill in incoming data */
            if( (pSetupPkt->wLength > 0) && (pEpInfo->EpDir == USB_DIR_HOST_TO_DEVICE) )
                   pCurDeviceO->pSetupData = pCurDeviceO->pSetupDataArea;

            /* if we do not have any data phase move from status to IDLE */
                    if(pSetupPkt->wLength == 0)
                        pEpInfo->EpStateMachineHandler(pCurDeviceO, 0, pSetupPkt);
        }
            }
           /* if we are in data phase we have received all data.Return the buffer to the application
        */
            else if(pEpInfo->EpState == EP_STATE_SETUP_DATA_PHASE)
            {
                ADI_DEV_1D_BUFFER Buffer,*pBuf; /* To return application data - local buffer class drivers must copy data */
                Buffer.Data = pCurDeviceO->pSetupData;
                Buffer.ProcessedElementCount = pEpInfo->EpNumTotalBytes;
                Buffer.ProcessedFlag = true;

                pBuf = &Buffer; /* default */
        
            /* Switch to status phase */
                pEpInfo->EpStateMachineHandler(pCurDeviceO, 0, pSetupPkt);               

        /* application supplied a read buffer during setup phase */
        if(pCurDeviceO->pAppEpZeroBuffer != NULL)
                {
                   pBuf = (ADI_DEV_1D_BUFFER*)pCurDeviceO->pAppEpZeroBuffer;
           memcpy(pBuf->Data,pCurDeviceO->pSetupData,pEpInfo->EpNumTotalBytes);
        }

                if (pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback)
                    pUsbCoreData->pDeviceHead->pActiveConfigObj->EpZeroCallback(AppHandle, 
                                                                ADI_USB_EVENT_RX_COMPLETE, 
                                        (void*)pBuf);
                /* Switch from status phase to IDLE */
                pEpInfo->EpStateMachineHandler(pCurDeviceO, 0, pSetupPkt);    

                pEpInfo->EpState = EP_STATE_IDLE;
            }
        }

        break;

    }
   
    /* Free the setup packet */
    FreeReceivedPackets(pCurDeviceO,(ADI_DEV_BUFFER*)pArg);

    return 1;
}

/*********************************************************************************
 * adi_usb_SetPhysicalDriverEntrypoint API
 *
 * Set physical driver entry point, done by the application. Must be called
 * only after the core is initialized and before the class drivers were opened.
 * class driver will use adi_usb_GetPhysicalDriverEntrypoint and open the
 * physical device driver.
 *
 * @param pEntryPoint Pointer to the physical USB driver entry point
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_SetPhysicalDriverHandle(ADI_DEV_PDD_HANDLE *pPDDHandle)
{

    USB_DEBUG_PRINT("Invalid Physical driver Entrypoint",
            USB_DEBUG_FLAGS & USB_DEVICE_DEBUG,
            (pPDDHandle == NULL));

    #ifdef USB_DEBUG
      if(pUsbCoreData->pPddHandle)
        USB_DEBUG_PRINT("Trying to set physical driver entrypoint multiple times",1,1);
    #endif /* USB_DEBUG */

    #ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    pUsbCoreData->pPddHandle = pPDDHandle;

    #ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
    #endif /* _USB_MULTI_THREADED_ */

    return(ADI_USB_RESULT_SUCCESS);
}


/*********************************************************************************
 * adi_usb_SetNotifyEventMask  API
 *
 * adi_usb_SetNotifyEventMask is used by the class drivers to let the core
 * know about the events that its interested on EP0 control endpoint.
 * Note that set will overwrite any previous settings so class drivers
 * must get the mask first and enable the approriate mask bits and then issue
 * set.
 *
 * @param EventMask Identifies the currently interested events
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_SetNotifyEventMask(const unsigned int EventMask)
{
#ifdef _USB_MULTI_THREADED_
    ker_disable_scheduler();
#endif /* _USB_MULTI_THREADED_ */

    pUsbCoreData->wEventMask = EventMask;

#ifdef _USB_MULTI_THREADED_
    ker_enable_scheduler();
#endif /* _USB_MULTI_THREADED_ */
    return(1);
}

/*********************************************************************************
 * adi_usb_GetNotifyEventMask API
 *
 * adi_usb_GetNotifyEventMask is used by the class drivers to get the
 * currnet event mask.
 *
 * @retun Returns the current event mask
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
u32_t adi_usb_GetNotifyEventMask()
{
    return(pUsbCoreData->wEventMask);
}

/*********************************************************************************
 * adi_usb_GetDeviceDescriptor API
 *
 * adi_usb_GetDeviceDescriptor is used by the class drivers to get the
 * device descriptor.
 *
 * @retun Returns the Device Descriptor Object
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
PDEVICE_DESCRIPTOR adi_usb_GetDeviceDescriptor()
{
    return(pUsbCoreData->pDeviceHead->pDeviceDesc);
}

/*********************************************************************************
 * SetPeripheralHandlers - Core internal function
 *
 * Sets up default peripheral handlers which include the Setup packet handler,
 * Transmitted packet handler, Bus Event Handler and Receive complete handler.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
static void SetPeripheralHandlers(void)
{
DEVICE_OBJECT       *pCurDeviceO = pUsbCoreData->pDeviceHead;  /* head should be set to point active device */
USB_EP_INFO         *pEpInfo = &pCurDeviceO->pEndpointZeroObj->EPInfo; /* Endpoint zero object */

     USB_ASSERT(pCurDeviceO == NULL);
     pUsbCoreData->StandardUsbRequestHandler  = PeripheralSetupPktProcessHandler;
     pUsbCoreData->BusEventHandler = PeripheralBusEventHandler;
     pUsbCoreData->TxCompleteHandler = PeripheralTxCompleteHandler;
     pUsbCoreData->RxCompleteHandler = PeripheralRxCompleteHandler;

     /* Setup EP Zero State machine handler to handle the SETUP Phase. */
     pEpInfo->EpStateMachineHandler = EpZeroDeviceStateMachineHandler;
     pEpInfo->EpState  = EP_STATE_IDLE;
}

/*********************************************************************************
 * adi_usb_SetDeviceMode(int wDevMode)
 *
 * Informs the core about the current operating mode of the device. This is typically
 * used by the peripheral driver to inform about its operation in case of dual role devie.
 * by default core gets initialized to MODE_DEVICE.
 *
 * @param eDevMode Device mode which could be MODE_DEVICE or MODE_OTG_HOST
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_SetDeviceMode(DEV_MODE eDevMode )
{
    switch(eDevMode)
    {
        case MODE_DEVICE:
             /* Initialize the default peripheral handlers */
             SetPeripheralHandlers();
             pUsbCoreData->eDevMode = MODE_DEVICE;
             /* TODO: Reset any previous data structures as this may get
              * called if the current peripheral becomes host (check HNP/SRP)
              */
        break;
        case MODE_OTG_HOST:
             SetHostHandlers();
             pUsbCoreData->eDevMode = MODE_OTG_HOST;

             /* TODO: Reset any previous data structures as this may get
              * called if the current OTG host becomes OTG peripheral (check HNP/SRP)
              */
         break;

    }
    /* let the driver know about the mode
     * By default both driver and core starts up in MODE_DEVICE. Only case where pPddHandle
     * will be NULL is when this API is invoked from CoreInit. As the driver initilizes to
     * its fine and later applications can change the mode by using the same API.
     */
    if(pUsbCoreData->pPddHandle != NULL)
        adi_dev_Control(pUsbCoreData->pPddHandle, ADI_USB_CMD_SET_DEV_MODE, (void*)eDevMode);

    return(ADI_USB_RESULT_SUCCESS);
}

/*********************************************************************************
 * adi_usb_GetDeviceMode  API
 *
 * Returns the current device mode.
 *
 * @return Returns the current mode which could be MODE_DEVICE or MODE_OTG_HOST.
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
DEV_MODE adi_usb_GetDeviceMode()
{
    return(pUsbCoreData->eDevMode);
}

/*********************************************************************************
 * adi_usb_GetObjectFromID
 *
 * @param dwObjectID is the ID of the object pointer.
 * @param eObjectType specifies the type of the object
 * @param **ppObject where the object pointer will be returned.
 *
 *********************************************************************************/

__ADI_USB_CORE_SECTION_CODE
s32_t adi_usb_GetObjectFromID(const s32_t dwObjectID,USB_OBJECT_TYPE eObjectType,void **ppObject)
{
int dwReturn=ADI_USB_RESULT_SUCCESS;
    USB_ASSERT(dwObjectID < 0);
    USB_ASSERT(ppObject == NULL);

    switch(eObjectType)
    {
        case USB_DEVICE_OBJECT_TYPE:
        {
            PDEVICE_OBJECT *pDeviceO = (PDEVICE_OBJECT*)ppObject;
            *pDeviceO = &device_objects[dwObjectID];
        }
        break;

        case USB_CONFIGURATION_OBJECT_TYPE:
        {
            PCONFIG_OBJECT *pConfigO = (PCONFIG_OBJECT*)ppObject;
           *pConfigO = &config_objects[dwObjectID -1];
        }
        break;

        case USB_INTERFACE_OBJECT_TYPE:
        {
            PINTERFACE_OBJECT *pIfceO = (PINTERFACE_OBJECT*)ppObject;
            *pIfceO = &interface_objects[dwObjectID];
        }
        break;

        case USB_ENDPOINT_OBJECT_TYPE:
        {
            PENDPOINT_OBJECT *pEpO = (PENDPOINT_OBJECT*)ppObject;
            *pEpO = &endpoint_objects[dwObjectID];
        }
        break;

        default:
            dwReturn = ADI_USB_RESULT_FAILED;
        break;
    }
    return(dwReturn);
}

/*********************************************************************************
 * adi_usb_ClearCoreData API
 *
 * Called from PDD_Close in the peripheral driver when device is closed 
 *
 *********************************************************************************/
__ADI_USB_CORE_SECTION_CODE
u32 adi_usb_ClearCoreData(void)
{
    int i;
    DEVICE_OBJECT  *pCurDevO;
    
    if(pUsbCoreData->isCoreInitialized != true)
      return(ADI_USB_RESULT_FAILED);

    for(i=0; i<USB_MAX_DEVICES; i++)
    {
       pCurDevO = &device_objects[i];
       if((pCurDevO->ID != -1) && (pCurDevO->pSetupDataArea != NULL))
            free(pCurDevO->pSetupDataArea);
    }
    
    /* free transmit and receive buffers */
    for(i=0;i<NUM_TX_CONTROL_BUFFERS;i++)
    {
        if(TxControlBuffers[i].Data != NULL)
        {
            free(TxControlBuffers[i].Data);
            TxControlBuffers[i].Data = NULL;
        }
    }
    
    for(i=0;i<NUM_RX_CONTROL_BUFFERS;i++)
    {
        if(RxControlBuffers[i].Data != NULL)
        {
            free(RxControlBuffers[i].Data);
            RxControlBuffers[i].Data = NULL;
        }
    }
    

    /* Zero out core objects */
    memset(&device_objects[0], 0, sizeof(DEVICE_OBJECT) * USB_MAX_DEVICES);
    memset(&config_objects[0], 0, sizeof(CONFIG_OBJECT) * USB_MAX_CONFIGURATIONS);
    memset(&interface_objects[0], 0, sizeof(INTERFACE_OBJECT) * USB_MAX_INTERFACES);
    memset(&alt_interface_objects[0], 0, sizeof(INTERFACE_OBJECT) * USB_MAX_INTERFACES);
    memset(&endpoint_objects[0], 0, sizeof(ENDPOINT_OBJECT) * USB_MAX_ENDPOINTS);

    /* Free the string descriptors */
    for(i = 1; i < USB_MAX_STRINGS; i++)
    {
        if(ptr_string_descriptors[i] != NULL)
        {     
            free(ptr_string_descriptors[i]);
            ptr_string_descriptors[i] = NULL;
        }
    }

    /* reset string index */
    string_index = 0;

    /* Zero out core data */
    memset(&usb_core_data, 0, sizeof(USB_CORE_DATA));
    
    pUsbCoreData->isCoreInitialized = false;

    return(ADI_USB_RESULT_SUCCESS);
}

#ifdef USB_DEBUG
/**
 * adi_usb_PrintDeviceDescriptor API
 *
 * Prints values of the device descriptor
 *
 * @param pDevO Pointer to the device object
 */
__ADI_USB_CORE_SECTION_CODE
void adi_usb_PrintDeviceDescriptor(PDEVICE_OBJECT pDevO)
{
PDEVICE_DESCRIPTOR pDevD = pDevO->pDeviceDesc;

    PRINT_BYTE("DD:bLength        :",pDevD->bLength);
    PRINT_BYTE("DD:bDescriptorType:",pDevD->bDescriptorType);
    PRINT_SHORT("DD:wBcdUSB        :",pDevD->wBcdUSB);
    PRINT_BYTE("DD:bDeviceClass   :",pDevD->bDeviceClass);
    PRINT_BYTE("DD:bDeviceSubClass:",pDevD->bDeviceSubClass);
    PRINT_BYTE("DD:bDeviceProtocol:",pDevD->bDeviceProtocol);
    PRINT_BYTE("DD:bMaxPacketSize0:",pDevD->bMaxPacketSize0);
    PRINT_SHORT("DD:wIdVendor     :",pDevD->wIdVendor);
    PRINT_SHORT("DD:wIdProduct     :",pDevD->wIdProduct);
    PRINT_SHORT("DD:wBcdDevice     :",pDevD->wBcdDevice);
    PRINT_BYTE("DD:bIManufacturer  :",pDevD->bIManufacturer);
    PRINT_BYTE("DD:bIProduct       :",pDevD->bIProduct);
    PRINT_BYTE("DD:bISerialNumber  :",pDevD->bISerialNumber);
    PRINT_BYTE("DD:bNumConfigurations:",pDevD->bNumConfigurations);
}

/**
 * adi_usb_PrintConfigDescriptor API
 *
 * prints the values of configuration descriptor
 *
 * @param pCfgO Pointer to the configuration object
 */
__ADI_USB_CORE_SECTION_CODE
void adi_usb_PrintConfigDescriptor(PCONFIG_OBJECT pCfgO)
{
PCONFIGURATION_DESCRIPTOR pCfgD = pCfgO->pConfigDesc;

    PRINT_BYTE("CD:bLength        :",pCfgD->bLength);
    PRINT_BYTE("CD:bDescriptorType:",pCfgD->bDescriptorType);
    PRINT_SHORT("CD:wTotalLength  :",pCfgD->wTotalLength);
    PRINT_BYTE("CD:bNumInterfaces :",pCfgD->bNumInterfaces);
    PRINT_BYTE("CD:bConfigurationValue:",pCfgD->bConfigurationValue);
    PRINT_BYTE("CD:bIConfiguration:",pCfgD->bIConfiguration);
    PRINT_BYTE("CD:bAttributes    :",pCfgD->bAttributes);
    PRINT_BYTE("CD:bMaxPower      :",pCfgD->bMaxPower);
}

/**
 * adi_usb_PrintInterfaceDescriptor  API
 *
 * Prints the values of interface descriptor
 *
 * @param pIfceO Pointer to the interface descriptor
 *
 */
__ADI_USB_CORE_SECTION_CODE
void adi_usb_PrintInterfaceDescriptor(PINTERFACE_OBJECT pIfceO)
{
PINTERFACE_DESCRIPTOR pIfceD = pIfceO->pInterfaceDesc;

    PRINT_BYTE("ID:bLength        :",pIfceD->bLength);
    PRINT_BYTE("ID:bDescriptorType:",pIfceD->bDescriptorType);
    PRINT_SHORT("ID:bInterfaceNumber  :",pIfceD->bInterfaceNumber);
    PRINT_BYTE("ID:bAlternateSetting :",pIfceD->bAlternateSetting);
    PRINT_BYTE("ID:bNumEndpoints :",pIfceD->bNumEndpoints);
    PRINT_BYTE("ID:bInterfaceClass :",pIfceD->bInterfaceClass);
    PRINT_BYTE("ID:bInterfaceSubClass :",pIfceD->bInterfaceSubClass);
    PRINT_BYTE("ID:bInterfaceProtocol :",pIfceD->bInterfaceProtocol);
    PRINT_BYTE("ID:bIInterface        :",pIfceD->bIInterface);
}
/**
 * adi_usb_PrintEndpointDescriptor  API
 *
 * Prints the values of endpoint descriptor
 *
 * @param pEpO Pointer to the interface descriptor
 *
 */
__ADI_USB_CORE_SECTION_CODE
void adi_usb_PrintEndpointDescriptor(PENDPOINT_OBJECT pEpO)
{
PENDPOINT_DESCRIPTOR pEpD = pEpO->pEndpointDesc;

    PRINT_BYTE("ED:bLength        :",pEpD->bLength);
    PRINT_BYTE("ED:bDescriptorType:",pEpD->bDescriptorType);
    PRINT_BYTE("ED:bEndpointAddress  :",pEpD->bEndpointAddress);
    PRINT_BYTE("ED:bAttributes  :",pEpD->bAttributes);
    PRINT_SHORT("ED:wMaxPacketSize  :",pEpD->wMaxPacketSize);
    PRINT_BYTE("ED:bInterval     :",pEpD->bInterval);
}
#endif /* USB_DEBUG */
