/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_usb_msd_class_host.c,v $
$Revision: 3459 $
$Date: 2010-09-29 10:37:28 -0400 (Wed, 29 Sep 2010) $

Description:

    This is the driver source code for the USB Mass
    Storage Class (Host Mode).

*********************************************************************************/

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_MSH_SECTION_CODE    section("adi_usb_msh_code")
#define __ADI_USB_MSH_SECTION_DATA    section("adi_usb_msh_data")
#else
#define __ADI_USB_MSH_SECTION_CODE
#define __ADI_USB_MSH_SECTION_DATA
#endif 


#include <services/services.h>          /* system service includes */
#include <drivers/adi_dev.h>            /* device manager includes */
#include <drivers/usb/usb_core/adi_usb_objects.h>           /* USB Objects */
#include <drivers/usb/usb_core/adi_usb_core.h>              /* USB Core */

#include <drivers/usb/class/otg/mass_storage/adi_usb_msd_class_host.h>     /* MSD Class Host include */

#include <string.h>
#include <stdio.h>

#include <ccblkfn.h>
#include <cplb.h>

__ADI_USB_MSH_SECTION_DATA
ADI_DEV_DEVICE_HANDLE   adi_msd_host_PeripheralDevHandle;

__ADI_USB_MSH_SECTION_DATA
ADI_DCB_CALLBACK_FN adi_msd_FSSCallbackFunction;

/* Initialize the device data structure */
__ADI_USB_MSH_SECTION_DATA
ADI_USB_DEV_DEF adi_msd_usb_host_dev_def = {0};

void    HostEndpointZeroCompleteCallback( void *Handle, u32 Event, void *pBuffer);

static u32 DevMassStorageHostConfigure(ADI_USB_DEV_DEF *pDevice);
static void ResetState(ADI_USB_DEV_DEF *pDevice);

/****************************************************************************/

__ADI_USB_MSH_SECTION_DATA
static ADI_DEV_1D_BUFFER   CommandBuffer;
__ADI_USB_MSH_SECTION_DATA
static ADI_USB_MSD_STD_REQ_CMD_BLOCK GetMaxLunCmd;
__ADI_USB_MSH_SECTION_DATA
static u8  MAX_LUN_DataBuffer[1 * sizeof(u8)];
__ADI_USB_MSH_SECTION_DATA
static ADI_USB_MSD_STD_REQ_CMD_BLOCK BulkResetCmd;

__ADI_USB_MSH_SECTION_DATA
static ADI_USB_MSD_STD_REQ_CMD_BLOCK ClearFeatureCmd;
u32 ClearFeature_Cmd(ADI_USB_DEV_DEF *pDevice, u32 EPAddress);

/********************************************************************************
* Critical Region builtins
*********************************************************************************/
__ADI_USB_MSH_SECTION_DATA
static void *_hCriticalRegionData;
__ADI_USB_MSH_SECTION_DATA
static void *exit_critical;

#define ADI_INT_ENTER_CRITICAL_REGION		\
			exit_critical = adi_int_EnterCriticalRegion(_hCriticalRegionData)
#define ADI_INT_EXIT_CRITICAL_REGION	\
			adi_int_ExitCriticalRegion(exit_critical)

/****************************************************************************/

extern  void    EndpointCompleteCallback_Host(void *Handle, u32 Event, void *pBuffer);
extern  int     __cplb_ctrl;

extern	u32 adi_msd_Data_Phase;

extern	u32 SCSI_WRITE10_CMD(ADI_USB_DEV_DEF *pDevice, u32 LBASector, u16 BlockSize);
extern	u32 SCSI_READ10_CMD(ADI_USB_DEV_DEF *pDevice, u32 LBASector, u16 BlockSize);
extern	u32 SCSI_TEST_UNIT_READY_CMD(ADI_USB_DEV_DEF *pDevice);
extern	u32 SCSI_INQUIRY_CMD(ADI_USB_DEV_DEF *pDevice);
extern	u32 SCSI_READ_FORMAT_CAPACITIES_CMD(ADI_USB_DEV_DEF *pDevice);
extern	u32 SCSI_READ_CAPACITY_CMD(ADI_USB_DEV_DEF *pDevice);
extern  u32 SCSI_MODE_SENSE6_CMD(ADI_USB_DEV_DEF *pDevice);

/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens the Mass Storage Class device
*                   in Device mode
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
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
    u32 i           = 0;
    u32 Result      = ADI_DEV_RESULT_SUCCESS;

    /* Create an instance of the device driver */
    ADI_USB_DEV_DEF *pDevice = &adi_msd_usb_host_dev_def;

    pDevice->ManagerHandle  = ManagerHandle;
    pDevice->Direction      = Direction;
    pDevice->DCBHandle      = DCBHandle;
    pDevice->DMCallback     = DMCallback;
    pDevice->DeviceHandle   = DeviceHandle;
    pDevice->DeviceNumber   = DeviceNumber;
    pDevice->DMAHandle      = DMAHandle;
    pDevice->CriticalData   = pCriticalRegionArg;

    *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;
    pDevice->pPDDHandle =   (ADI_DEV_PDD_HANDLE *)pDevice;

	_hCriticalRegionData = pCriticalRegionArg;

#if defined(__ADSP_MOAB__)
    /* Provide a local work around for 05000359 */
    {

	ADI_INT_ENTER_CRITICAL_REGION;
    	
    #define LDEB2_URGENT 0x00004000

    u32 *pEBIU_ptr = NULL;
    u32 nulTempVal = 0;

    pEBIU_ptr = (u32 *)EBIU_DDRQUE;
    nulTempVal = *pEBIU_ptr;
    nulTempVal |= LDEB2_URGENT;
    *pEBIU_ptr = nulTempVal;

	ADI_INT_EXIT_CRITICAL_REGION;
    
    }
#endif
    /* Reset to initial Values */
    ResetState(pDevice);

    return(Result);
}

/*********************************************************************
*
*   Function:       pddClose
*
*   Description:    Closes down a device
*
*********************************************************************/

__ADI_USB_MSH_SECTION_CODE
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE PDDHandle    /* PDD handle */
)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;

	ADI_USB_DEV_DEF   *pDevice = (ADI_USB_DEV_DEF *)PDDHandle;

    Result = adi_dev_Close(pDevice->PeripheralDevHandle);

    return(Result);
}

/*********************************************************************
*
*   Function:       pddRead
*
*   Description:
*
*********************************************************************/

__ADI_USB_MSH_SECTION_CODE
static u32 adi_pdd_Read(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;

	ADI_USB_DEV_DEF   *pDevice = (ADI_USB_DEV_DEF *)PDDHandle;

    pDevice->pCurrentBuffer = pBuffer;
    pDevice->TransferComplete = FALSE;

    ((ADI_DEV_1D_BUFFER*)pBuffer)->ElementCount =
            (((ADI_DEV_1D_BUFFER*)pBuffer)->ElementCount * ((ADI_DEV_1D_BUFFER*)pBuffer)->ElementWidth);

    ((ADI_DEV_1D_BUFFER*)pBuffer)->ElementWidth = 1;

    ((ADI_DEV_1D_BUFFER*)pBuffer)->Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostInEP;
    ((ADI_DEV_1D_BUFFER*)pBuffer)->pNext = NULL;

    /* Wait until we are in DATA phase */
	while(pDevice->dwDataPhase != TRUE)
	{
	};

    Result = adi_dev_Read(  pDevice->PeripheralDevHandle,
                            BufferType,
                            pBuffer);

    return(Result);
}

/*********************************************************************
*
*   Function:       pddWrite
*
*   Description:
*
*********************************************************************/

__ADI_USB_MSH_SECTION_CODE
static u32 adi_pdd_Write(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;

	ADI_USB_DEV_DEF   *pDevice = (ADI_USB_DEV_DEF *)PDDHandle;

    pDevice->pCurrentBuffer = pBuffer;
    pDevice->TransferComplete = FALSE;

    ((ADI_DEV_1D_BUFFER*)pBuffer)->Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostOutEP;
    ((ADI_DEV_1D_BUFFER*)pBuffer)->pNext = NULL;

    /* Wait until we are in DATA phase */
	while(pDevice->dwDataPhase != TRUE)
	{
	};
	   
    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                            BufferType,
                            pBuffer);

    return(Result);
}

/*********************************************************************
*
*   Function:       pddControl
*
*   Description:    List of I/O control commands to the driver
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
static u32 adi_pdd_Control(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    u32 Command,                    /* command ID */
    void *pArg                      /* pointer to argument */
)
{
	ADI_USB_DEV_DEF   *pDevice = (ADI_USB_DEV_DEF *)PDDHandle;

	ADI_USB_MSD_SCSI_CMD_BLOCK * pSCSICmd;
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    switch(Command)
    {
        case ADI_DEV_CMD_SET_DATAFLOW_METHOD:
        case ADI_DEV_CMD_SET_DATAFLOW:
        case ADI_USB_CMD_ENABLE_USB:
            Result = adi_dev_Control(pDevice->PeripheralDevHandle,
                                Command,
                                (void*)pArg);
        break;

        case ADI_USB_CMD_CLASS_CONFIGURE:
             Result =  DevMassStorageHostConfigure(pDevice);
        break;

        case ADI_USB_CMD_CLASS_SET_CONTROLLER_HANDLE:
               pDevice->PeripheralDevHandle =(ADI_DEV_DEVICE_HANDLE)pArg;
               // the following line is not extensible
               adi_msd_host_PeripheralDevHandle = (ADI_DEV_DEVICE_HANDLE)pArg;
        break;

        case ADI_USB_CMD_CLASS_GET_CONTROLLER_HANDLE:
				*((u32 *)pArg) = (u32)pDevice->PeripheralDevHandle;
        break;

        case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:
            *((u32 *)pArg) = FALSE;
        break;

        case ADI_USB_MSD_CMD_IS_DEVICE_ENUMERATED:
            *((u32 *)pArg) = pDevice->DeviceConfigured;
		break;

		case ADI_USB_MSD_CMD_SCSI_READ10:
			pSCSICmd = (ADI_USB_MSD_SCSI_CMD_BLOCK *)pArg;
			SCSI_READ10_CMD(pDevice, pSCSICmd->LBASector, pSCSICmd->BlockSize);
		break;

		case ADI_USB_MSD_CMD_SCSI_WRITE10:
			pSCSICmd = (ADI_USB_MSD_SCSI_CMD_BLOCK *)pArg;
			SCSI_WRITE10_CMD(pDevice, pSCSICmd->LBASector, pSCSICmd->BlockSize);
		break;

		case ADI_USB_MSD_CMD_INQUIRY:
			SCSI_INQUIRY_CMD(pDevice);
		break;

		case ADI_USB_MSD_CMD_TEST_UNIT_READY:
			SCSI_TEST_UNIT_READY_CMD(pDevice);
		break;

		case ADI_USB_MSD_CMD_READ_FORMAT_CAPACITIES:
			SCSI_READ_FORMAT_CAPACITIES_CMD(pDevice);
		break;

		case ADI_USB_MSD_CMD_GET_FORMAT_CAPACITIES_DATA:
		    if (pArg) {
		        memcpy(pArg,&pDevice->FormatCapacities, sizeof(pDevice->FormatCapacities));
		    }
		    else {
		        Result = ADI_DEV_RESULT_FAILED;
		    }
		break;
		
		case ADI_USB_MSD_CMD_READ_CAPACITY:
			SCSI_READ_CAPACITY_CMD(pDevice);
		break;

		case ADI_USB_MSD_CMD_GET_CAPACITY_TEN_DATA:
		    if (pArg) {
		        memcpy(pArg,&pDevice->CapacityTen, sizeof(pDevice->CapacityTen));
		    }
		    else {
		        Result = ADI_DEV_RESULT_FAILED;
		    }
		break;
		
		case ADI_USB_MSD_CMD_MODE_SENSE6:
			SCSI_MODE_SENSE6_CMD(pDevice);
		break;

		case ADI_USB_MSD_CMD_REQ_SENSE: 	
			SCSI_REQ_SENSE_CMD(pDevice);
		break;	
		
#if defined(__ADSP_MOAB__)

		case ADI_USB_MSD_DISABLE_LDEB2_URGENT:
        /* Disable local work around for 05000359 */
    	{
			ADI_INT_ENTER_CRITICAL_REGION;

    		#define DISABLE_LDEB2_URGENT 0xFFFFBFFF

    		u32 *pEBIU_ptr = NULL;
    		u32 nulTempVal = 0;

    		pEBIU_ptr = (u32 *)EBIU_DDRQUE;
    		nulTempVal = *pEBIU_ptr;
    		nulTempVal &= DISABLE_LDEB2_URGENT;
    		*pEBIU_ptr = nulTempVal;

			ADI_INT_EXIT_CRITICAL_REGION;

    	}
		break;
#endif
        default:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
        break;
    }

    return(Result);
}

/**************************************************************************
 *
 * USB Mass Storage Class driver entry point (Host Mode)
 *
 **************************************************************************/
__ADI_USB_MSH_SECTION_DATA
ADI_DEV_PDD_ENTRY_POINT ADI_USB_Host_MassStorageClass_Entrypoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

/*********************************************************************
*
*   Function:       ClearFeature_Cmd
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 ClearFeature_Cmd(ADI_USB_DEV_DEF *pDevice, u32 EPAddress)
{
	u32 Result;
	
	// Build Clear Feature request
	ClearFeatureCmd.bRequestType 	= STD_REQ_TYPE;
	ClearFeatureCmd.bRequest 		= STD_REQ_CLEAR_FEATURE;
	ClearFeatureCmd.wValue 			= 0x0000;	
	ClearFeatureCmd.wIndex 			= (u16)EPAddress;
	ClearFeatureCmd.wLength 		= 0x0000;
	
    CommandBuffer.Data = &ClearFeatureCmd;
    CommandBuffer.ElementCount = 8;
    CommandBuffer.ElementWidth = 1;
    CommandBuffer.CallbackParameter = &CommandBuffer;
    CommandBuffer.ProcessedFlag = FALSE;
    CommandBuffer.ProcessedElementCount = 0;
    CommandBuffer.pNext = NULL;
    CommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = 0x0;
    
    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                          ADI_DEV_1D,
                          (ADI_DEV_BUFFER *)&CommandBuffer);

    return Result;
}

/*********************************************************************
*
*   Function:       Bulk_Only_MS_Reset_Cmd
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 Bulk_Only_MS_Reset_Cmd(ADI_USB_DEV_DEF *pDevice)
{
	u32 Result;

	// Build Bulk_Only_MS_Reset Command request
	BulkResetCmd.bRequestType 	= CLASS_REQUEST_TYPE_RESET;
	BulkResetCmd.bRequest 		= CLASS_REQUEST_MS_BULK_RESET;
	BulkResetCmd.wValue			= 0x0000;
	BulkResetCmd.wIndex			= 0x0000;
	BulkResetCmd.wLength 		= 0x0000;

    CommandBuffer.Data = &BulkResetCmd;
    CommandBuffer.ElementCount = 8;
    CommandBuffer.ElementWidth = 1;
    CommandBuffer.CallbackParameter = &CommandBuffer;
    CommandBuffer.ProcessedFlag = FALSE;
    CommandBuffer.ProcessedElementCount = 0;
    CommandBuffer.pNext = NULL;
    CommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = 0x0;

    pDevice->CSCommand = CS_BULK_RESET;
    
    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                          ADI_DEV_1D,
                          (ADI_DEV_BUFFER *)&CommandBuffer);

    return Result;
}

/*********************************************************************
*
*   Function:       GetMaxLun_Cmd
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 GetMaxLun_Cmd(ADI_USB_DEV_DEF *pDevice)
{
	u32 Result = ADI_DEV_RESULT_SUCCESS;

	/* Build Get MAX LUN Command request */
	GetMaxLunCmd.bRequestType 	= CLASS_REQUEST_TYPE;
	GetMaxLunCmd.bRequest 		= CLASS_REQUEST_GET_MAX_LUN;
	GetMaxLunCmd.wValue 		= 0x0000;
	GetMaxLunCmd.wIndex 		= 0x0000;
	GetMaxLunCmd.wLength 		= 0x0001;  

    CommandBuffer.Data = &GetMaxLunCmd;
    CommandBuffer.ElementCount = 8;
    CommandBuffer.ElementWidth = 1;
    CommandBuffer.CallbackParameter = &CommandBuffer;
    CommandBuffer.ProcessedFlag = FALSE;
    CommandBuffer.ProcessedElementCount = 0;
    CommandBuffer.pNext = NULL;
    
    /* Set the address to endpoint zero */
    CommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = 0x0;

    /* Set command status to get the data */
    pDevice->CSCommand = CS_GET_MAX_LUN;
   
    /* Send the request packet for MAX LUN information */
	adi_usb_TransmitEpZeroBuffer((ADI_DEV_BUFFER*)&CommandBuffer);
    
    return Result;
}

/*********************************************************************
*
*   Function:       ReadMaxLun_Cmd
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 ReadMaxLun_Cmd(ADI_USB_DEV_DEF *pDevice)
{
	u32 Result = ADI_DEV_RESULT_SUCCESS;

    CommandBuffer.Data = &MAX_LUN_DataBuffer;
    CommandBuffer.ElementCount = 1;
    CommandBuffer.ElementWidth = 1;
    CommandBuffer.CallbackParameter = &CommandBuffer;
    CommandBuffer.ProcessedFlag = FALSE;
    CommandBuffer.ProcessedElementCount = 0;
    CommandBuffer.pNext = NULL;
    
    /* Set the address to endpoint zero */
    CommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = 0x0;
  
	/* Get the MAX LUN value */  
    adi_usb_ReceiveEpZeroBuffer((ADI_DEV_BUFFER*)&CommandBuffer);

    return Result;
}

/*********************************************************************
*
*   Function:       HostEndpointZeroCompleteCallback
*
*   Description:    Handle Endpoint 0 data traffic specific to
*                   this device class
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
void HostEndpointZeroCompleteCallback( void *Handle, u32 Event, void *pArg)
{
	u32 Result;
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_host_dev_def;

    switch(Event)
    {
    	/* D->H transfer complete */
    	case ADI_USB_EVENT_RX_COMPLETE:
    	   	if((pDevice->DeviceConfigured == FALSE) &&
    	   		(pDevice->CSCommand == CS_GET_MAX_LUN))
			{
    	    	/* Complete the MAX LUN transaction */
    	    	pDevice->CSCommand = CS_NONE;
            	pDevice->DeviceConfigured = TRUE;
			    pDevice->DMCallback(pDevice->DeviceHandle, ADI_USB_EVENT_CONNECT, 0);
			}            	
    	break;

    	/* H->D transfer complete */
    	case ADI_USB_EVENT_TX_COMPLETE:
    	   	if((pDevice->DeviceConfigured == FALSE) &&
    	   		(pDevice->CSCommand == CS_GET_MAX_LUN))
    	   	{
	 	   		ReadMaxLun_Cmd(pDevice);
    	   	}
    	   	else
    	   	{
    	   		if((pDevice->dwDeviceStalled == TRUE) &&
    	   			(pDevice->DeviceConfigured == TRUE))
    	   		{
				  /* Issue a request for IN_TOKEN to complete clear_feature */	
    	          adi_dev_Control(pDevice->PeripheralDevHandle, ADI_USB_CMD_OTG_REQ_IN_TOKEN, (void*)0);
       	   		  pDevice->dwDeviceStalled = FALSE;
    	   		}
    	   	}		 	   		
    	break;

        case ADI_USB_OTG_EVENT_ENUMERATION_COMPLETE:
        {
        /* Set our configuration */
        adi_usb_otg_SetConfiguration(1);

        PDEVICE_OBJECT          pDevO = (PDEVICE_OBJECT)pArg;
        PCONFIG_OBJECT          pCfgO = pDevO->pConfigObj;
        PENDPOINT_OBJECT        pEpO  = pCfgO->pActiveEpObj;
        PENDPOINT_OBJECT        pActiveEpO = pEpO;
        ACTIVE_EP_LIST          ActiveEPList;
        PUSB_EP_INFO            pUSBEPInfo;

        u8                      i = 0;
        u8                      x = 0;

        pActiveEpO->pEndpointDesc->bEndpointAddress;

        while(pActiveEpO != NULL)
        {
        	/* BULK endpoints ONLY */
        	if(pActiveEpO->pEndpointDesc->bAttributes == USB_BULK_MODE)
        	{
            	if((pActiveEpO->pEndpointDesc->bEndpointAddress & 0x80))
            	{
                	ActiveEPList.Active_EP[i]   = pActiveEpO->ID;
                	ActiveEPList.direction[i]   = 1;
            	}
            	else
            	{
                	ActiveEPList.Active_EP[i]   = pActiveEpO->ID;
                	ActiveEPList.direction[i]   = 0;
            	}

            	/* Save IN/OUT endpoint values */
            	if(ActiveEPList.direction[i] == 1)
	        	{
	        		pDevice->HostInEP  = ActiveEPList.Active_EP[i];
	        	}
	        	else
	        	{
	        		pDevice->HostOutEP = ActiveEPList.Active_EP[i];
	        	}

            	/* Save the Endpoint callback address */
            	pActiveEpO->EPInfo.EpCallback = EndpointCompleteCallback_Host;
        	}

            	pActiveEpO = pActiveEpO->pNextActiveEpObj;
            	i++;
        }

        /* Get MAX LUN from the device */
       	GetMaxLun_Cmd(pDevice);
    	}
    	break;

		case ADI_USB_EVENT_DISCONNECT:
		{
			/* Reset the state machine */
			ResetState(pDevice);
			
            /* Setup the OTG Endpoint Zero Callback */
            adi_usb_otg_SetEpZeroCallback(1,
                	(ADI_DCB_CALLBACK_FN)HostEndpointZeroCompleteCallback);

            /* Set MODE to OTG Host */
            adi_usb_SetDeviceMode(MODE_OTG_HOST);
            
			/* Let world know USB device is disconnected */
    		pDevice->DMCallback(pDevice->DeviceHandle, Event, 0);
		}
		break;

		default:
		break;
    }
}

/*********************************************************************
*
*   Function:       ResetState()
*
*   Description:    Sets variables to initial values
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
static void ResetState(ADI_USB_DEV_DEF *pDevice)
{

   	pDevice->pCurrentBuffer = NULL;

   	pDevice->TransferComplete 	= FALSE;
	pDevice->DeviceConfigured 	= FALSE;
	
    pDevice->dwDeviceStalled 	= FALSE;

	pDevice->RemainingDataToSend = 0;
	pDevice->RemainingDataToComeIn = 0;

	pDevice->CSCommand = CS_NONE;
		
   	pDevice->CommandState = STATE_CBW_SENT;
}

/*********************************************************************
*
*   Function:       DevMassStorageHostConfigure
*
*   Description:    Configures the Mass storage Host
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
static u32 DevMassStorageHostConfigure(ADI_USB_DEV_DEF *pDevice)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* setup the entrypoint and peripheral handle for the usb core */
    adi_usb_SetPhysicalDriverHandle(pDevice->PeripheralDevHandle);

    /* If data cache is enabled then we will let the physical driver know about it */
    if( (__cplb_ctrl & CPLB_ENABLE_DCACHE ) || (__cplb_ctrl & CPLB_ENABLE_DCACHE2))
    {
        adi_dev_Control(pDevice->PeripheralDevHandle,
                        ADI_USB_CMD_BUFFERS_IN_CACHE,
                        (void *)TRUE);
    }

    /* Setup the OTG Endpoint Zero Callback */
    adi_usb_otg_SetEpZeroCallback(1,
        	(ADI_DCB_CALLBACK_FN)HostEndpointZeroCompleteCallback);

    /* Set MODE to OTG Host */
    adi_usb_SetDeviceMode(MODE_OTG_HOST);
    
    /* Get the HDRC DMA Mode */
    pDevice->dmaConfig.DmaChannel = ADI_USB_HDRC_RX_CHANNEL;
    adi_dev_Control(pDevice->PeripheralDevHandle, ADI_USB_CMD_GET_DMA_MODE, (void*)&pDevice->dmaConfig);

    /* Reset to initial Values */
    ResetState(pDevice);

   	return Result;
}





