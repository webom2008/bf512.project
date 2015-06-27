/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_usb_msd_host_support.c,v $
$Revision: 3459 $
$Date: 2010-09-29 10:37:28 -0400 (Wed, 29 Sep 2010) $

Description:

    This is the HOST support code for the USB Mass
    Storage Class driver.

*********************************************************************************/

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_MSH_SECTION_CODE    section("adi_usb_msh_code")
#define __ADI_USB_MSH_SECTION_DATA    section("adi_usb_msh_data")
#else
#define __ADI_USB_MSH_SECTION_CODE
#define __ADI_USB_MSH_SECTION_DATA
#endif 


#include <services/services.h>      /* system service includes */
#include <drivers/usb/usb_core/adi_usb_core.h>          /* USB Core */
#include <drivers/usb/usb_core/adi_usb_objects.h>       /* USB Objects */
#include <drivers/usb/class/otg/mass_storage/adi_usb_msd_class_host.h> /* USB MSD Host include */
#include <string.h>                 /* For memcpy etc. */
#include <stdio.h>

#include <drivers/pid/atapi/adi_atapi.h>

__ADI_USB_MSH_SECTION_DATA
u32 adi_scsi_command_tag;

__ADI_USB_MSH_SECTION_DATA
static ADI_DEV_1D_BUFFER   SCSICommandBuffer;
__ADI_USB_MSH_SECTION_DATA
static CBW                 CommandBlock;

__ADI_USB_MSH_SECTION_DATA
static ADI_DEV_1D_BUFFER usbBuffer;
__ADI_USB_MSH_SECTION_DATA
static u8  CSWBuffer[CSW_SIZE * sizeof(u8)];

/* The following data buffer is required for all the SCSI
 * commands other than READ/WRITE. It needs to be 32 byte aligned 
 * to avoid problems with the FLUSH/FLUSHINV instructions
 * in the HDRC driver
*/
#pragma align(32)
__ADI_USB_MSH_SECTION_DATA
static u8 DataBuffer[256 * sizeof(u8)];

/* Externals */

extern  ADI_USB_DEV_DEF         adi_msd_usb_host_dev_def;
extern  ADI_DCB_CALLBACK_FN     adi_msd_FSSCallbackFunction;
extern  u32 Bulk_Only_MS_Reset_Cmd(ADI_USB_DEV_DEF *pDevice);
extern  u32 ClearFeature_Cmd(ADI_USB_DEV_DEF *pDevice, u32 EPAddress);

static void StoreFormatCapacitiesData(ADI_USB_DEV_DEF *pDevice, u8 *pData);
static void StoreCapacities10Data(ADI_USB_DEV_DEF *pDevice, u8 *pData);

/*********************************************************************
*
*   Function:       SCSI_REQ_SENSE_CMD
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 SCSI_REQ_SENSE_CMD(ADI_USB_DEV_DEF *pDevice)
{
    u32 Result;

    /* Build the command block */
    CommandBlock.signature = CBW_SIGNATURE;
    CommandBlock.tag = TAG_REQ_SENSE;
    CommandBlock.dataTransferLength = 0x12;
    CommandBlock.flags = CBW_FLAG_DATA_FROM_DEVICE_TO_HOST ;
    CommandBlock.lun = 0x0;
    CommandBlock.wcbLength = 0x0C;
    CommandBlock.wcb[0] = SCSI_REQUEST_SENSE;												
    CommandBlock.wcb[1] = 0x0;																
    CommandBlock.wcb[2] = 0x0;																
    CommandBlock.wcb[3] = 0x0;																
    CommandBlock.wcb[4] = 0x12;																
    CommandBlock.wcb[5] = 0x0;																
    CommandBlock.wcb[6] = 0x0;																
    CommandBlock.wcb[7] = 0x0;																
    CommandBlock.wcb[8] = 0x0;																
    CommandBlock.wcb[9] = 0x0;																
 
    SCSICommandBuffer.Data = &CommandBlock;
    SCSICommandBuffer.ElementCount = CBW_LENGTH;
    SCSICommandBuffer.ElementWidth = 1;
    SCSICommandBuffer.CallbackParameter = &SCSICommandBuffer;
    SCSICommandBuffer.ProcessedFlag = FALSE;
    SCSICommandBuffer.ProcessedElementCount = 0;
    SCSICommandBuffer.pNext = NULL;
    SCSICommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostOutEP;

    pDevice->SCSICommandSent = SCSI_REQUEST_SENSE;
    adi_scsi_command_tag  = TAG_REQ_SENSE;

    pDevice->dwCBWTransferLength = pDevice->RemainingDataToComeIn = (u32)0x12;

    pDevice->TransferComplete = FALSE;

    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                          ADI_DEV_1D,
                          (ADI_DEV_BUFFER *)&SCSICommandBuffer);
                          
    /* !!!! temporary solution to emsure command is sent !!!! */
    while (!SCSICommandBuffer.ProcessedFlag);                          
    
    return Result;
}

/*********************************************************************
*
*   Function:       SCSI_MODE_SENSE6_CMD
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 SCSI_MODE_SENSE6_CMD(ADI_USB_DEV_DEF *pDevice)
{
    u32 Result;

    /* Build the command block */
    CommandBlock.signature = CBW_SIGNATURE;
    CommandBlock.tag = TAG_MODE_SENSE6;
    CommandBlock.dataTransferLength = 0xC0;
    CommandBlock.flags = CBW_FLAG_DATA_FROM_DEVICE_TO_HOST ;
    CommandBlock.lun = 0x0;
    CommandBlock.wcbLength = SCSI_COMMAND_BLOCK_SIZE_6;
    CommandBlock.wcb[0] = SCSI_MODE_SENSE6;
    CommandBlock.wcb[1] = 0x0;
    CommandBlock.wcb[2] = 0x3F;
    CommandBlock.wcb[3] = 0x0;
    CommandBlock.wcb[4] = 0xC0;
    CommandBlock.wcb[5] = 0x0;
    CommandBlock.wcb[6] = 0x0;
    CommandBlock.wcb[7] = 0x0;
    CommandBlock.wcb[8] = 0x0;
    CommandBlock.wcb[9] = 0x0;

    SCSICommandBuffer.Data = &CommandBlock;
    SCSICommandBuffer.ElementCount = CBW_LENGTH;
    SCSICommandBuffer.ElementWidth = 1;
    SCSICommandBuffer.CallbackParameter = &SCSICommandBuffer;
    SCSICommandBuffer.ProcessedFlag = FALSE;
    SCSICommandBuffer.ProcessedElementCount = 0;
    SCSICommandBuffer.pNext = NULL;
    SCSICommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostOutEP;

    pDevice->SCSICommandSent = SCSI_MODE_SENSE6;
    adi_scsi_command_tag  = TAG_MODE_SENSE6;

    pDevice->dwCBWTransferLength = pDevice->RemainingDataToComeIn = (u32)0xC0;

    pDevice->TransferComplete = FALSE;

    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                          ADI_DEV_1D,
                          (ADI_DEV_BUFFER *)&SCSICommandBuffer);

    /* !!!! temporary solution to emsure command is sent !!!! */
    while (!SCSICommandBuffer.ProcessedFlag);

    return Result;
}

/*********************************************************************
*
*   Function:       SCSI_READ_CAPACITY_CMD
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 SCSI_READ_CAPACITY_CMD(ADI_USB_DEV_DEF *pDevice)
{
    u32 Result;

    /* Build the command block */
    CommandBlock.signature = CBW_SIGNATURE;
    CommandBlock.tag = TAG_READ_CAPACITY;
    CommandBlock.dataTransferLength = 0x8;
    CommandBlock.flags = CBW_FLAG_DATA_FROM_DEVICE_TO_HOST ;
    CommandBlock.lun = 0x0;
    CommandBlock.wcbLength = SCSI_COMMAND_BLOCK_SIZE_10;
    CommandBlock.wcb[0] = SCSI_READ_CAPACITY;
    CommandBlock.wcb[1] = 0x0;
    CommandBlock.wcb[2] = 0x0;
    CommandBlock.wcb[3] = 0x0;
    CommandBlock.wcb[4] = 0x0;
    CommandBlock.wcb[5] = 0x0;
    CommandBlock.wcb[6] = 0x0;
    CommandBlock.wcb[7] = 0x0;
    CommandBlock.wcb[8] = 0x0;
    CommandBlock.wcb[9] = 0x0;

    SCSICommandBuffer.Data = &CommandBlock;
    SCSICommandBuffer.ElementCount = CBW_LENGTH;
    SCSICommandBuffer.ElementWidth = 1;
    SCSICommandBuffer.CallbackParameter = &SCSICommandBuffer;
    SCSICommandBuffer.ProcessedFlag = FALSE;
    SCSICommandBuffer.ProcessedElementCount = 0;
    SCSICommandBuffer.pNext = NULL;
    SCSICommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostOutEP;

    pDevice->SCSICommandSent = SCSI_READ_CAPACITY;
    adi_scsi_command_tag  = TAG_READ_CAPACITY;

    pDevice->dwCBWTransferLength = pDevice->RemainingDataToComeIn = (u32)CommandBlock.dataTransferLength;

    pDevice->TransferComplete = FALSE;

    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                          ADI_DEV_1D,
                          (ADI_DEV_BUFFER *)&SCSICommandBuffer);

    /* !!!! temporary solution to emsure command is sent !!!! */
    while (!SCSICommandBuffer.ProcessedFlag);

    return Result;
}

/*********************************************************************
*
*   Function:       SCSI_READ_FORMAT_CAPACITIES_CMD
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 SCSI_READ_FORMAT_CAPACITIES_CMD(ADI_USB_DEV_DEF *pDevice)
{
    u32 Result;

    /* Build the command block */
    CommandBlock.signature = CBW_SIGNATURE;
    CommandBlock.tag = TAG_READ_FORMAT_CAPACITIES;
    CommandBlock.dataTransferLength = 0xC;
    CommandBlock.flags = CBW_FLAG_DATA_FROM_DEVICE_TO_HOST ;
    CommandBlock.lun = 0x0;
    CommandBlock.wcbLength = SCSI_COMMAND_BLOCK_SIZE_10;
    CommandBlock.wcb[0] = SCSI_READ_FORMAT_CAPACITIES;
    CommandBlock.wcb[1] = 0x0;
    CommandBlock.wcb[2] = 0x0;
    CommandBlock.wcb[3] = 0x0;
    CommandBlock.wcb[4] = 0x0;
    CommandBlock.wcb[5] = 0x0;
    CommandBlock.wcb[6] = 0x0;
    CommandBlock.wcb[7] = 0x0;
    CommandBlock.wcb[8] = 0xC;
    CommandBlock.wcb[9] = 0x0;

    SCSICommandBuffer.Data = &CommandBlock;
    SCSICommandBuffer.ElementCount = CBW_LENGTH;
    SCSICommandBuffer.ElementWidth = 1;
    SCSICommandBuffer.CallbackParameter = &SCSICommandBuffer;
    SCSICommandBuffer.ProcessedFlag = FALSE;
    SCSICommandBuffer.ProcessedElementCount = 0;
    SCSICommandBuffer.pNext = NULL;
    SCSICommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostOutEP;

    pDevice->SCSICommandSent = SCSI_READ_FORMAT_CAPACITIES;
    adi_scsi_command_tag  = TAG_READ_FORMAT_CAPACITIES;

    pDevice->dwCBWTransferLength = pDevice->RemainingDataToComeIn = 0xC;

    pDevice->TransferComplete = FALSE;

    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                          ADI_DEV_1D,
                          (ADI_DEV_BUFFER *)&SCSICommandBuffer);

    /* !!!! temporary solution to emsure command is sent !!!! */
    while (!SCSICommandBuffer.ProcessedFlag);

    return Result;
}

/*********************************************************************
*
*   Function:       SCSI_TEST_UNIT_READY_CMD
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 SCSI_TEST_UNIT_READY_CMD(ADI_USB_DEV_DEF *pDevice)
{
    u32 Result;

    /* Build the command block */
    CommandBlock.signature = CBW_SIGNATURE;
    CommandBlock.tag = TAG_TEST_UNIT_READY;
    CommandBlock.dataTransferLength = 0;
    CommandBlock.flags = CBW_FLAG_DATA_FROM_DEVICE_TO_HOST ;
    CommandBlock.lun = 0x0;
    CommandBlock.wcbLength = SCSI_COMMAND_BLOCK_SIZE_6;
    CommandBlock.wcb[0] = SCSI_TEST_UNIT_READY;
    CommandBlock.wcb[1] = 0x0;
    CommandBlock.wcb[2] = 0x0;
    CommandBlock.wcb[3] = 0x0;
    CommandBlock.wcb[4] = 0x0;
    CommandBlock.wcb[5] = 0x0;
    CommandBlock.wcb[6] = 0x0;
    CommandBlock.wcb[7] = 0x0;
    CommandBlock.wcb[8] = 0x0;
    CommandBlock.wcb[9] = 0x0;

    SCSICommandBuffer.Data = &CommandBlock;
    SCSICommandBuffer.ElementCount = CBW_LENGTH;
    SCSICommandBuffer.ElementWidth = 1;
    SCSICommandBuffer.CallbackParameter = &SCSICommandBuffer;
    SCSICommandBuffer.ProcessedFlag = FALSE;
    SCSICommandBuffer.ProcessedElementCount = 0;
    SCSICommandBuffer.pNext = NULL;
    SCSICommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostOutEP;

    pDevice->SCSICommandSent = SCSI_TEST_UNIT_READY;
    adi_scsi_command_tag  = TAG_TEST_UNIT_READY;

    pDevice->dwCBWTransferLength = pDevice->RemainingDataToComeIn = (u32)CommandBlock.dataTransferLength;

    pDevice->TransferComplete = FALSE;

    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                          ADI_DEV_1D,
                          (ADI_DEV_BUFFER *)&SCSICommandBuffer);

    /* !!!! temporary solution to emsure command is sent !!!! */
    while (!SCSICommandBuffer.ProcessedFlag);

    return Result;
}

/*********************************************************************
*
*   Function:       SCSI_INQUIRY_CMD
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 SCSI_INQUIRY_CMD(ADI_USB_DEV_DEF *pDevice)
{
    u32 Result;

    /* Build the command block */
    CommandBlock.signature = CBW_SIGNATURE;
    CommandBlock.tag = TAG_INQUIRY;
    CommandBlock.dataTransferLength = RESPONCE_SIZE;
    CommandBlock.flags = CBW_FLAG_DATA_FROM_DEVICE_TO_HOST ;
    CommandBlock.lun = 0x0;
    CommandBlock.wcbLength = SCSI_COMMAND_BLOCK_SIZE_6;
    CommandBlock.wcb[0] = SCSI_INQUIRY;
    CommandBlock.wcb[1] = 0x0;
    CommandBlock.wcb[2] = 0x0;
    CommandBlock.wcb[3] = 0x0;
    CommandBlock.wcb[4] = RESPONCE_SIZE;
    CommandBlock.wcb[5] = 0x0;
    CommandBlock.wcb[6] = 0x0;
    CommandBlock.wcb[7] = 0x0;
    CommandBlock.wcb[8] = 0x0;
    CommandBlock.wcb[9] = 0x0;

    SCSICommandBuffer.Data = &CommandBlock;
    SCSICommandBuffer.ElementCount = CBW_LENGTH;
    SCSICommandBuffer.ElementWidth = 1;
    SCSICommandBuffer.CallbackParameter = &SCSICommandBuffer;
    SCSICommandBuffer.ProcessedFlag = FALSE;
    SCSICommandBuffer.ProcessedElementCount = 0;
    SCSICommandBuffer.pNext = NULL;
    SCSICommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostOutEP;

    pDevice->SCSICommandSent = SCSI_INQUIRY;
    adi_scsi_command_tag  = TAG_INQUIRY;

    pDevice->dwCBWTransferLength = pDevice->RemainingDataToComeIn = (u32)CommandBlock.dataTransferLength;

    pDevice->TransferComplete = FALSE;

    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                          ADI_DEV_1D,
                          (ADI_DEV_BUFFER *)&SCSICommandBuffer);

    /* !!!! temporary solution to emsure command is sent !!!! */
    while (!SCSICommandBuffer.ProcessedFlag);

    return Result;
}

/*********************************************************************
*
*   Function:       SCSI_READ10_CMD
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 SCSI_READ10_CMD(ADI_USB_DEV_DEF *pDevice, u32 LBASector, u16 BlockSize)
{
    u32 Result;

    /* Build the command block */
    CommandBlock.signature = CBW_SIGNATURE;
    CommandBlock.tag = TAG_READ10;
    CommandBlock.dataTransferLength = BlockSize * 512;
    CommandBlock.flags = CBW_FLAG_DATA_FROM_DEVICE_TO_HOST ;
    CommandBlock.lun = 0x0;
    CommandBlock.wcbLength = SCSI_COMMAND_BLOCK_SIZE_10;
    CommandBlock.wcb[0] = SCSI_READ10;												/* SCSI Command */
    CommandBlock.wcb[1] = 0x0;																/* LUN/DPO/FUA/Reserved/Reldr */
    CommandBlock.wcb[2] = ((LBASector & 0xff000000) >> 24);		/* LBA 1 (MSB) */
    CommandBlock.wcb[3] = ((LBASector & 0xff0000) >> 16);			/* LBA 2 */
    CommandBlock.wcb[4] = ((LBASector & 0xff00) >> 8);				/* LBA 3 */
    CommandBlock.wcb[5] = (LBASector & 0xff);									/* LBA 4 (LSB) */
    CommandBlock.wcb[6] = 0x0;																/* Reserved */
    CommandBlock.wcb[7] = ((BlockSize & 0xff00) >> 8);				/* Transfer Length (MSB) */
    CommandBlock.wcb[8] = (BlockSize & 0xff);									/* Transfer Length (LSB) */
    CommandBlock.wcb[9] = 0x0;																/* Control */

    SCSICommandBuffer.Data = &CommandBlock;
    SCSICommandBuffer.ElementCount = CBW_LENGTH;
    SCSICommandBuffer.ElementWidth = 1;
    SCSICommandBuffer.CallbackParameter = &SCSICommandBuffer;
    SCSICommandBuffer.ProcessedFlag = FALSE;
    SCSICommandBuffer.ProcessedElementCount = 0;
    SCSICommandBuffer.pNext = NULL;
    SCSICommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostOutEP;

    pDevice->SCSICommandSent = SCSI_READ10;
    adi_scsi_command_tag  = TAG_READ10;

    pDevice->dwCBWTransferLength = pDevice->RemainingDataToComeIn = (u32)(BlockSize * 512);

    pDevice->TransferLength = pDevice->RemainingDataToComeIn;

    pDevice->TransferComplete = FALSE;
    
	pDevice->dwDataPhase = FALSE;
	
    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                          ADI_DEV_1D,
                          (ADI_DEV_BUFFER *)&SCSICommandBuffer);

    /* !!!! temporary solution to emsure command is sent !!!! */
    while (!SCSICommandBuffer.ProcessedFlag);

    return Result;
}

/*********************************************************************
*
*   Function:       SCSI_WRITE10_CMD
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
u32 SCSI_WRITE10_CMD(ADI_USB_DEV_DEF *pDevice, u32 LBASector, u16 BlockSize)
{
    u32 Result;

    /* Build the command block */
    CommandBlock.signature = CBW_SIGNATURE;
    CommandBlock.tag = TAG_WRITE10;
    CommandBlock.dataTransferLength = BlockSize * 512;
    CommandBlock.flags = CBW_FLAG_DATA_FROM_HOST_TO_DEVICE;
    CommandBlock.lun = 0x0;
    CommandBlock.wcbLength = SCSI_COMMAND_BLOCK_SIZE_10;
    CommandBlock.wcb[0] = SCSI_WRITE10;												/* SCSI Command */
    CommandBlock.wcb[1] = 0x0;																/* LUN/DPO/FUA/Reserved/Reldr */
    CommandBlock.wcb[2] = ((LBASector & 0xff000000) >> 24);		/* LBA 1 (MSB) */
    CommandBlock.wcb[3] = ((LBASector & 0xff0000) >> 16);			/* LBA 2 */
    CommandBlock.wcb[4] = ((LBASector & 0xff00) >> 8);				/* LBA 3 */
    CommandBlock.wcb[5] = (LBASector & 0xff);									/* LBA 4 (LSB) */
    CommandBlock.wcb[6] = 0x0;																/* Reserved */
    CommandBlock.wcb[7] = ((BlockSize & 0xff00) >> 8);				/* Transfer Length (MSB) */
    CommandBlock.wcb[8] = (BlockSize & 0xff);									/* Transfer Length (LSB) */
    CommandBlock.wcb[9] = 0x0;																/* Control */


    SCSICommandBuffer.Data = &CommandBlock;
    SCSICommandBuffer.ElementCount = CBW_LENGTH;
    SCSICommandBuffer.ElementWidth = 1;
    SCSICommandBuffer.CallbackParameter = NULL;
    SCSICommandBuffer.ProcessedFlag = FALSE;
    SCSICommandBuffer.ProcessedElementCount = 0;
    SCSICommandBuffer.pNext = NULL;
    SCSICommandBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostOutEP;

    pDevice->SCSICommandSent = SCSI_WRITE10;
    adi_scsi_command_tag  = TAG_WRITE10;

    pDevice->dwCBWTransferLength = pDevice->RemainingDataToSend = (u32)(BlockSize * 512);

    pDevice->TransferComplete = FALSE;
    
	pDevice->dwDataPhase = FALSE;
	
    Result = adi_dev_Write(pDevice->PeripheralDevHandle,
                          ADI_DEV_1D,
                          (ADI_DEV_BUFFER *)&SCSICommandBuffer);

    /* !!!! temporary solution to emsure command is sent !!!! */
    while (!SCSICommandBuffer.ProcessedFlag);

    return Result;
}

/*********************************************************************
*
*   Function:       GetCSWFromDevice
*
*   Description:
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
void GetCSWFromDevice(ADI_USB_DEV_DEF *pDevice)
{
    u32 Result;

    // Be sure that the CSW buffer is zeroed out before
    // we use it.
    memset(CSWBuffer, 0, (CSW_SIZE * sizeof(u8)));

    usbBuffer.Data = (u8*)&CSWBuffer;
    usbBuffer.ElementCount = CSW_SIZE;
    usbBuffer.ElementWidth = 1;
    usbBuffer.ProcessedElementCount = 0;
    usbBuffer.ProcessedFlag = FALSE;
    usbBuffer.CallbackParameter = &usbBuffer;
    usbBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostInEP;
    usbBuffer.pNext = NULL;

    Result = adi_dev_Read(pDevice->PeripheralDevHandle,
                  ADI_DEV_1D,
                  (ADI_DEV_BUFFER*)&usbBuffer);
}

/*********************************************************************
*
*   Function:       GetDataFromDevice
*
*   Description:
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
void GetDataFromDevice(ADI_USB_DEV_DEF *pDevice)
{

    u32 Result;

    // Be sure that the buffer is zeroed out before
    // we use it.
    memset(DataBuffer, 0, (256 * sizeof(u8)));

    usbBuffer.Data = (u8*)&DataBuffer;
    usbBuffer.ElementCount = pDevice->RemainingDataToComeIn;
    usbBuffer.ElementWidth = 1;
    usbBuffer.ProcessedElementCount = 0;
    usbBuffer.ProcessedFlag = FALSE;
    usbBuffer.CallbackParameter = &usbBuffer;
    usbBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = pDevice->HostInEP;
    usbBuffer.pNext = NULL;

    Result = adi_dev_Read(pDevice->PeripheralDevHandle,
                  ADI_DEV_1D,
                  (ADI_DEV_BUFFER*)&usbBuffer);
}

/*********************************************************************
*
*   Function:       OTG_Host_To_Device_TransferComplete
*
*   Description:    This function is called when a Host to Device
*                   transfer is completed
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
void OTG_Host_To_Device_TransferComplete(ADI_USB_DEV_DEF *pDevice, void *pBuf)
{
 ADI_DEV_1D_BUFFER *pBuffer = (ADI_DEV_1D_BUFFER *)pBuf;

        switch(pDevice->CommandState)
        {
            case STATE_CBW_SENT:
            {
                if((pDevice->RemainingDataToComeIn != 0) ||
                    (pDevice->RemainingDataToSend != 0))
                {
                    /* Writing data from host -> device */
                    if(pDevice->SCSICommandSent == SCSI_WRITE10)
                    {
                        /* Send more data to the device */
                        pDevice->CommandState = STATE_SEND_DATA;
                        
                        /* Notify read/write commands we should now 
                           be in DATA phase 					*/
                       	pDevice->dwDataPhase = TRUE;
                    }
                    else
                    {
                        /* Expecting data back from the command sent */
                        pDevice->CommandState = STATE_WAITFOR_DATA;
                        
                        /* Notify read/write commands we should now 
                           be in DATA phase 					*/
                       	pDevice->dwDataPhase = TRUE;

                        if((pDevice->SCSICommandSent != SCSI_READ10) &&
                        	(pDevice->SCSICommandSent != SCSI_WRITE10))
                        {
     						GetDataFromDevice(pDevice);
                        }
                    }
                }
                else
                {
                    /* No Data expecting CSW */
                    pDevice->CommandState = STATE_WAITFOR_CSW;
                    GetCSWFromDevice(pDevice);
                }
            }
            break;

            case STATE_SEND_DATA:
            {
                if(pDevice->RemainingDataToSend != 0)
                {
                    pDevice->RemainingDataToSend -= pBuffer->ElementCount;

                    if(pDevice->RemainingDataToSend == 0)
                    {
                        pDevice->CommandState = STATE_WAITFOR_CSW;
                        GetCSWFromDevice(pDevice);
                    }
                    else
                    {
                        pDevice->CommandState = STATE_SEND_DATA;

					    if (pDevice->pCurrentBuffer) {
        					(pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, (void *)pDevice->pCurrentBuffer );
       						 pDevice->pCurrentBuffer = NULL;
    					}

                    }
                }
            }
            break;

            default:
            break;
        }
}

/*********************************************************************
*
*   Function:       ProcessSetupCommandEvents
*
*   Description:    This function sends the processed events back
*					to the PID
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
void ProcessSetupCommandEvents(ADI_USB_DEV_DEF *pDevice)
{
	switch(pDevice->SCSICommandSent)
	{
		case SCSI_REQUEST_SENSE:
		case SCSI_INQUIRY:
		case SCSI_TEST_UNIT_READY:
		case SCSI_READ_CAPACITY:
		case SCSI_READ_FORMAT_CAPACITIES:
		case SCSI_MODE_SENSE6:
		{
			pDevice->DMCallback(pDevice->DeviceHandle, ADI_USB_MSD_EVENT_SCSI_CMD_COMPLETE, 0);  
		}			
		break;

		default:
		break;
	}
}

/*********************************************************************
*
*   Function:       OTG_Device_To_Host_TransferComplete
*
*   Description:    This function is called when a Device to Host
*                   transfer is completed
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
void OTG_Device_To_Host_TransferComplete(ADI_USB_DEV_DEF *pDevice, void *pBuf)
{
    char VendorInfo[80];
    u32 CSWSignature;

    ADI_DEV_BUFFER *pBuffer = (ADI_DEV_BUFFER *)pBuf;
    CSW * pCSW = (CSW *)(pBuffer->OneD.Data);

        switch(pDevice->CommandState)
        {
            case STATE_WAITFOR_DATA:
            {
                /* Save Pointer to last Buffer */
                pDevice->pBuffer = (ADI_DEV_1D_BUFFER*)pBuf;

                /*
                   Do this for mode sense6 since value may be
                   different per device
                */
				if(pDevice->SCSICommandSent == SCSI_MODE_SENSE6)
					pDevice->RemainingDataToComeIn -= pBuffer->OneD.ElementCount;
				else
                	pDevice->RemainingDataToComeIn -= pBuffer->OneD.ProcessedElementCount;

                if(pDevice->RemainingDataToComeIn == 0)
                {
                    
                    if (pDevice->SCSICommandSent == SCSI_READ_FORMAT_CAPACITIES)
                    {
                        StoreFormatCapacitiesData(pDevice, (u8*)pDevice->pBuffer->Data);
                    }                    
                    if (pDevice->SCSICommandSent == SCSI_READ_CAPACITY)
                    {
                        StoreCapacities10Data(pDevice, (u8*)pDevice->pBuffer->Data);
                    }                    
                    /* No more data, set state to wait for CSW */
                    pDevice->CommandState = STATE_WAITFOR_CSW;
					
                    /* ONLY peek into the FIFO if we are using DMA Mode 1 */
                    if(pDevice->dmaConfig.DmaMode == ADI_USB_DMA_MODE_1)
                    {
						/*
							This code was added to handle data
						   	transfers > 512 bytes
						*/
                	    u32 Value;
                    	if(pDevice->TransferLength > 512)
                    	{

                    		/* peek fifo to determine when the CSW packet is in the FIFO */
                   	 		do {
                        			Value = pDevice->HostInEP;

                        			adi_dev_Control(pDevice->PeripheralDevHandle,
                        					ADI_USB_CMD_PEEK_EP_FIFO,
                        					(void*)&Value);

                    			} while (Value != CSW_SIZE);

                    		/* Instruct the controller to stop asserting the IN token */
                   			Value = pDevice->HostInEP;
                   	 		adi_dev_Control(pDevice->PeripheralDevHandle,
                    					ADI_USB_CMD_STOP_EP_TOKENS,
                    					(void*)&Value);
                    	}
                    }
                    
					/* Reset this value */
					pDevice->TransferLength = 0;

                    /* Now request the CSW packet from the controller */
                    GetCSWFromDevice(pDevice);
                }
                else
                {
                    /* Still more data to come in, set state to wait for more data */
                    pDevice->CommandState = STATE_WAITFOR_DATA;

			    	if (pDevice->pCurrentBuffer)
			    	{
  					 	pDevice->pCurrentBuffer = NULL;
                	}
                }
            }
            break;

            case STATE_WAITFOR_CSW:
            {
				/* Check for valid CSW */  	
          	 	if((pCSW->signature != CSW_SIGNATURE)   ||
          	 	   (pCSW->tag != adi_scsi_command_tag)  ||
            		(pBuffer->OneD.ProcessedElementCount != CSW_SIZE))
            	{
       	    		/* Notify the PID that things are REALLY screwed up */
                   	pDevice->DMCallback(pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_ERROR, 0);  
                   	
					/* Break out of here */
                   	break;
	           	}
          	
            	/* Check for a command pass or fail */
                if((pCSW->status == CSW_STATUS_COMMAND_PASSED) ||
                	(pCSW->status == CSW_STATUS_COMMAND_FAILED)) 
                {
					pDevice->dwDataNotProcessedLength = 0; 

					if(pCSW->dataResidue != 0)
					{
	               		// Determine the amount of data processed from the difference between
       		       		// dataTransferLength and dwCSWResidue
           				pDevice->dwDataNotProcessedLength = pDevice->dwCBWTransferLength -
				    										pCSW->dataResidue;
					}				    										
                	
					/* If command failed notify the PID if a setup 
					   command failed */																		                	
                    if((pCSW->status == CSW_STATUS_COMMAND_FAILED)
                       && ((pDevice->SCSICommandSent != SCSI_READ10) ||
                       		(pDevice->SCSICommandSent != SCSI_WRITE10)))
                    {
						/* Notify PID setup command failed */
                    	pDevice->DMCallback(pDevice->DeviceHandle, ADI_USB_MSD_EVENT_SCSI_CMD_ERROR, 0);  
    					
						/* Transfer complete */
    	               	pDevice->TransferComplete = TRUE;
    	               	
              			/* Setup for the next Command */
                   		pDevice->CommandState = STATE_CBW_SENT;
                    		
                  		// Process the setup command events
						ProcessSetupCommandEvents(pDevice);
   						
						/* Reset CBW Transfer length */
						pDevice->dwCBWTransferLength = 0;    	               	
						
						/* Break out of here */
						break;
                    }
					
                   	if((pDevice->SCSICommandSent == SCSI_READ10) ||
                       		(pDevice->SCSICommandSent == SCSI_WRITE10))
                   	{                    
		                   	pDevice->TransferComplete = TRUE;
                   		
    	                	/* Setup for the next Command */
        	            	pDevice->CommandState = STATE_CBW_SENT;
							pDevice->dwCBWTransferLength = 0;
                   		
    						/* Transfer complete callback to FSS */
   							if (pDevice->TransferComplete &&
    			 					pDevice->pCurrentBuffer) {
        							(pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, (void *)pDevice->pCurrentBuffer );
        	
    							pDevice->pCurrentBuffer = NULL;
							}                    
                   	}
					else /* Passed setup command */
					{                    
              			/* Setup for the next Command */
                  		pDevice->CommandState = STATE_CBW_SENT;
                    		
                  		/* Process the setup command events
                  		   and notify PID */
						ProcessSetupCommandEvents(pDevice);
   						
						pDevice->dwCBWTransferLength = 0;
					}                    	
                }
                else
                {   
	            	/* Check for a command PHASE error */ 
 					if(pCSW->status == CSW_STATUS_COMMAND_PHASE_ERROR)
        	    	{
        	    		/* Notify the PID that things are REALLY screwed up */
                    	pDevice->DMCallback(pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_ERROR, 0);  
	           		}
                }                	             	            
            }
            break;

            default:
            break;
        }
}

/*********************************************************************
*
*   Function:       EndpointCompleteCallback
*
*   Description:    Handle normal Endpoint data traffic
*
*********************************************************************/
__ADI_USB_MSH_SECTION_CODE
void EndpointCompleteCallback_Host(void *Handle, u32 Event, void *pBuffer)
{
    ADI_USB_DEV_DEF *pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_host_dev_def;

    switch(Event)
    {
        /* Transmit data to DEVICE complete (Host Mode) */
        case ADI_USB_EVENT_DATA_TX:
            OTG_Host_To_Device_TransferComplete(pDevice, pBuffer);
        break;

         /* Recieve data from DEVICE complete (Host Mode)*/
        case ADI_USB_EVENT_DATA_RX:
            OTG_Device_To_Host_TransferComplete(pDevice, pBuffer);
        break;

        /* Recieved a packet but we have no buffer */
        case ADI_USB_EVENT_PKT_RCVD_NO_BUFFER:
            pDevice->CommandState = STATE_WAITFOR_CSW;
            GetCSWFromDevice(pDevice);
        break;
        
        case ADI_USB_EVENT_RX_STALL:
        {
		   	u32 dwEPID = (u32)pBuffer;
   			u32 EPAddress;
			PENDPOINT_OBJECT pEpO;
			 
			pDevice->dwDeviceStalled = TRUE;
			                    
			/* Get the correct EP object */ 
            adi_usb_GetObjectFromID(dwEPID,
            						USB_ENDPOINT_OBJECT_TYPE,
            						(void**)&pEpO);
			
				
			/* Get the stalled EP address */                        
            EPAddress = pEpO->pEndpointDesc->bEndpointAddress;
            						
			/* Clear the STALLED Endpoint */                                    
			ClearFeature_Cmd(pDevice, EPAddress);
              
			while(pDevice->dwDeviceStalled == TRUE);
			      
            /* Endpoint should be ready request a CSW */
            pDevice->CommandState = STATE_WAITFOR_CSW;
            GetCSWFromDevice(pDevice);
        }            
        break;
        
        case ADI_USB_EVENT_TX_STALL:
        {
		   	u32 dwEPID = (u32)pBuffer;
   			u32 EPAddress;
			PENDPOINT_OBJECT pEpO;
			 
			pDevice->dwDeviceStalled = TRUE;
			                    
			/* Get the correct EP object */ 
            adi_usb_GetObjectFromID(dwEPID,
            						USB_ENDPOINT_OBJECT_TYPE,
            						(void**)&pEpO);
			
				
			/* Get the stalled EP address */                        
            EPAddress = pEpO->pEndpointDesc->bEndpointAddress;
            						
			/* Clear the STALLED Endpoint */                                    
			ClearFeature_Cmd(pDevice, EPAddress);
              
			while(pDevice->dwDeviceStalled == TRUE);
			      
            /* Endpoint should be ready request a CSW */
            pDevice->CommandState = STATE_WAITFOR_CSW;
            GetCSWFromDevice(pDevice);        	
        }
        break;

        case ADI_USB_EVENT_RX_NAK_TIMEOUT:
        {
        	/* NEED TO NOTIFY THE UPPER LAYER */
			pDevice->DMCallback(pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_ERROR , 0);
        }
        break;
                 
        case ADI_USB_EVENT_RX_ERROR:
        {
        	/* NEED TO NOTIFY THE UPPER LAYER */
			pDevice->DMCallback(pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_ERROR, 0);
        }
        break;
        
        case ADI_USB_EVENT_TX_NAK_TIMEOUT:
        {
        	/* NEED TO NOTIFY THE UPPER LAYER */
			pDevice->DMCallback(pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_ERROR, 0);
        }
        break;
        
        case ADI_USB_EVENT_TX_ERROR:
        {
        	/* NEED TO NOTIFY THE UPPER LAYER */
			pDevice->DMCallback(pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_ERROR, 0);
        }
        break;
        
        default:
        break;
    }
}

/*********************************************************************
*
*   Function:       StoreFormatCapacitiesData
*
*   Description:    Store the READ FORMAT CAPACITIES INFORMATION 
*
*********************************************************************/
static void StoreFormatCapacitiesData(ADI_USB_DEV_DEF *pDevice, u8 *pData)
{
    /* capacity list header 
     *      bytes 0-2 reserved
     *      byte 3  list length (bytes)
    */
    pDevice->FormatCapacities.NumItems = pData[3]/SCSI_FORMAT_CAPACITIES_CURMAX_DESC_SIZE;

    /* Current/Maximum capacity Descriptor
     *      bytes 4 - 7 Number of blocks, byte 4 MSB
     *      byte 8 bits 1:0 descriptor type
     *      bytes 9 - 11 Block Length, byte 9 MSB
    */
    pDevice->FormatCapacities.NumBlocks = EndianSwap( (*(u32*)&pData[4]) );
    pDevice->FormatCapacities.DescriptorType = pData[8] & 0x03;
    pDevice->FormatCapacities.BlockSize = ((u32)pData[9])<<16 | ((u32)pData[10])<<8 | ((u32)pData[11]);

}

/*********************************************************************
*
*   Function:       StoreCapacities10Data
*
*   Description:    Store the READ CAPACITY (10) INFORMATION 
*
*********************************************************************/
static void StoreCapacities10Data(ADI_USB_DEV_DEF *pDevice, u8 *pData)
{
    /* bytes 0-3: Number of blocks, byte 0 MSB 
    */
    pDevice->CapacityTen.NumBlocks = EndianSwap( (*(u32*)&pData[0]) );

    /* bytes 4-7: Block Length, byte 4 MSB 
    */
    pDevice->CapacityTen.BlockSize = EndianSwap( (*(u32*)&pData[4]) );
}

