/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_usb_msd_support.c,v $
$Revision: 2386 $
$Date: 2010-03-25 17:26:08 -0400 (Thu, 25 Mar 2010) $

Description:

    This is the support code for the USB Mass
    Storage Class driver (Device Mode).

*********************************************************************************/
 
#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_MSD_SECTION_CODE    section("adi_usb_msd_code")
#define __ADI_USB_MSD_SECTION_DATA    section("adi_usb_msd_data")
#else
#define __ADI_USB_MSD_SECTION_CODE
#define __ADI_USB_MSD_SECTION_DATA
#endif 

#include <services/services.h>        /* system service includes */
#include <services/fss/adi_fss.h>
#include <drivers/usb/usb_core/adi_usb_core.h>            /* USB Core */
#include <drivers/usb/usb_core/adi_usb_objects.h>        /* USB Objects */
#include <drivers/usb/class/peripheral/mass_storage/adi_usb_msd_class.h>        /* USB Mass Storage structure */
#include <drivers/usb/class/peripheral/mass_storage/adi_usb_msd_support.h>    /* USB Mass Storage support funcs etc. */
#include <drivers/usb/class/peripheral/mass_storage/adi_usb_msd_scsi.h>        /* SCSI */
#include <drivers/usb/usb_core/adi_usb_debug.h>         /* USB types includes */
#include <string.h>                    /* For memcpy etc. */
#include <stdio.h>

#include <drivers/pid/atapi/adi_atapi.h>
//#include <drivers/pid/adi_rawpid.h>

#define _READ_  1
#define _WRITE_ 0

extern    ADI_DEV_DEVICE_HANDLE RAW_PID_DeviceHandle;

__ADI_USB_MSD_SECTION_DATA
static ADI_DEV_1D_BUFFER   MAXLunReturnBuffer;
__ADI_USB_MSD_SECTION_DATA
static u8  MaxLunData[5 * sizeof(u8)];

__ADI_USB_MSD_SECTION_DATA
static u8    dataBufferIndex = 0;

#ifdef _ADI_USB_MOAB_CONTROLLER_

/* Number of endpoints NOT counting EP0 */
#define NUM_PHYSICAL_ENDPOINTS                          7

/* OUT endpoint mask */
#define USB_ENDPOINT_DIRECTION_MASK                   0x80

/* Test direction bit in the bEndpointAddress field of     */
/* an endpoint descriptor.                                */
#define USB_ENDPOINT_DIRECTION_OUT(addr)          (!((addr) & USB_ENDPOINT_DIRECTION_MASK))
#define USB_ENDPOINT_DIRECTION_IN(addr)           ((addr) & USB_ENDPOINT_DIRECTION_MASK)

typedef struct ActiveEPList
{
    u8    Active_EP[NUM_PHYSICAL_ENDPOINTS];
    u8  direction[NUM_PHYSICAL_ENDPOINTS];
}ACTIVE_EP_LIST,*PACTIVE_EP_LIST;

#endif

__ADI_USB_MSD_SECTION_DATA
static  u8    *    adi_usb_buffer_pointer;
__ADI_USB_MSD_SECTION_DATA
static  u8    *    adi_usb_inc_buffer_pointer;

__ADI_USB_MSD_SECTION_DATA
u8       adi_usb_msd_device_ready = FALSE;

extern  u32 adi_msd_CurrentDataWriteLocation;

extern     bool        bFormatCapacityCalled;
extern     bool        bModeSenseReportCalled;
extern    bool        bLargeBlockSize;

extern    u8      adi_usb_msd_IN_EP;
extern    u8      adi_usb_msd_OUT_EP;

extern     ADI_USB_DEV_DEF        adi_msd_usb_dev_def;

extern    u32 adi_msd_starting_block;
extern    u32    adi_msd_blocks_To_Read;
extern    u8 *adi_msd_Disk;
extern  bool adi_msd_HDDetected;
extern  u32 ConfigureEndpoints(void);

/*********************************************************************
*
*    Function:        StallEndpoint
*
*    Description:    Stalls the endpoint
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
void SetStallEndpoint(u8 Endpoint)
{
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
    u32 i = 0;
    
    /* Look for the correct index then call
       the clear stall control function
    */   
    for(i=0; i < MAX_ENDPOINTS; i++)
    {
        /* Only stall if we have a match */
        if(pDevice->EndPointObjects[i].ID == Endpoint)
        {
            adi_dev_Control(adi_msd_PeripheralDevHandle,
                            ADI_USB_CMD_SET_STALL_EP,
                            (void*)pDevice->EndPointObjects[i].pObj);            
        }
    }
                   
    return;
}

/*********************************************************************
*
*    Function:        EnableEndpoint
*
*    Description:    Re-enable a stalled endpoint
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
void ClearStallEndpoint(u8 Endpoint)
{
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
    u32 i = 0;
    
    /* Look for the correct index then call
       the clear stall control function
    */ 
    for(i=0; i < MAX_ENDPOINTS; i++)
    {
        /* Only stall if we have a match */
        if(pDevice->EndPointObjects[i].ID == Endpoint)
        {
            adi_dev_Control(adi_msd_PeripheralDevHandle,
                            ADI_USB_CMD_CLEAR_STALL_EP,
                            (void*)pDevice->EndPointObjects[i].pObj);            
        }
    }

    return;
}

/*********************************************************************
*
*    Function:        SetupEPBuffer
*
*    Description:    Sets up the data endpoint buffer
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
void SetupEPBuffer(ADI_DEV_BUFFER *pBuff, u8 epNum, u32 ulSize)
{

u32 Result;
ADI_DEV_1D_BUFFER *pBuffer = (ADI_DEV_1D_BUFFER*)pBuff;

    pBuffer->ElementCount = ulSize;
    pBuffer->Reserved[BUFFER_RSVD_EP_ADDRESS] = epNum;
    pBuffer->ProcessedElementCount = 0x0;
    pBuffer->pNext = NULL;

    Result = adi_dev_Read(adi_msd_PeripheralDevHandle,
                  ADI_DEV_1D,
                  (ADI_DEV_BUFFER*)pBuffer);
}

/*********************************************************************
*
*    Function:        SendStatusCSW
*
*    Description:    Return Control Status packet to the host
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static void SendStatusCSW(u8 CSWStatus, void *pBuf)
{
    bool bFailed = false;

    if(CSWStatus == CSW_STATUS_COMMAND_PASSED)
    {
           bFailed = false;
    }
    else
    {
        bFailed = true;
    }

    ADI_DEV_1D_BUFFER* pBuffer = (ADI_DEV_1D_BUFFER*)pBuf;

    /* If this was an error we MUST STALL the endpoint
       before sending the CSW */
    if(bFailed == true)
    {
      SetStallEndpoint(adi_usb_msd_IN_EP);
    }

    BuildCSW (pBuffer->Data, adi_msd_CurrentCBW.tag, 0, CSWStatus);

    pBuffer->ElementWidth = 1;
    pBuffer->ElementCount = CSW_SIZE;
    pBuffer->Reserved[BUFFER_RSVD_EP_ADDRESS] = adi_usb_msd_IN_EP;
    pBuffer->ProcessedElementCount = 0x0;

    /* Now we can re-enable the endpoint */
    if(bFailed == true)
    {
        ClearStallEndpoint(adi_usb_msd_IN_EP);
    }

    adi_msd_CommandStatusState = STATE_CSW_SENT;

    adi_dev_Write(adi_msd_PeripheralDevHandle,
                  ADI_DEV_1D,
                  (ADI_DEV_BUFFER*)pBuffer);
                  

}

/*********************************************************************
*
*    Function:        SendDeviceToHostDataCSW
*
*    Description:    Return Control Status packet to the host
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static void SendDeviceToHostDataCSW(u8 CSWStatus, void *pBuf, u32 DataResidue)
{
    ADI_DEV_1D_BUFFER* pBuffer = (ADI_DEV_1D_BUFFER*)pBuf;

    BuildCSW (pBuffer->Data, adi_msd_CurrentCBW.tag, DataResidue, CSWStatus);

    pBuffer->ElementWidth = 1;
    pBuffer->ElementCount = CSW_SIZE;
    pBuffer->Reserved[BUFFER_RSVD_EP_ADDRESS] = adi_usb_msd_IN_EP;
    pBuffer->ProcessedElementCount = 0x0;

    adi_msd_CommandStatusState = STATE_CSW_SENT;
  
    adi_dev_Write(adi_msd_PeripheralDevHandle,
                  ADI_DEV_1D,
                  (ADI_DEV_BUFFER*)pBuffer);
}

/*********************************************************************
*
*    Function:        ProcessCBW
*
*    Description:    Process the data control block packet
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static bool ProcessCBW (PCBW pCBW, ADI_DEV_1D_BUFFER * pBuffer)
{
    u32 validDataLen;
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
  
    if (!IsCBWMeaningful(pCBW))
    {
        /* Send Command FAILED status */
        SendStatusCSW(CSW_STATUS_COMMAND_FAILED, pBuffer);

        return (false);
    }

    /* If we're here the CBW is meaningful, let's do what the command asks us to do! */
    if (SCSI_ProcessCDB (pCBW->wcb, pCBW->wcbLength, pCBW->dataTransferLength, pBuffer->Data, &validDataLen))
    {
        if (pCBW->dataTransferLength == 0)
        {
            /* Send Command PASSED status */
            SendStatusCSW(CSW_STATUS_COMMAND_PASSED, pBuffer);

            return (false);
        }

        /* If the direction was device to host - start sending the data back */
        if (pCBW->flags & CBW_FLAG_DATA_FROM_DEVICE_TO_HOST)
        {
            adi_msd_IsTransferActive = true;

              adi_msd_RemainingDataToShipUp = validDataLen;
              
            /* Save the initial buffer address */
            adi_usb_buffer_pointer = pBuffer->Data;

            adi_msd_pDataToShipUp = pBuffer;

            SendDataToHost();
        }
        else
        {
            u32 nBytesToReadFromHost;
            /* Wait for more data */
            adi_msd_IsTransferActive = true;

            adi_msd_RemainingDataToComeIn = pCBW->dataTransferLength;

            adi_msd_CommandStatusState = STATE_WAITFOR_DATA;

            /* Setup our Buffer (D->H) */
            if (adi_msd_RemainingDataToComeIn > pDevice->nClientBufferElementCount)
            {
                nBytesToReadFromHost = pDevice->nClientBufferElementCount;
            } else {
                nBytesToReadFromHost = adi_msd_RemainingDataToComeIn;
            }
            SetupEPBuffer((ADI_DEV_BUFFER *)pBuffer, adi_usb_msd_OUT_EP, nBytesToReadFromHost);
            return (true);
        }
    }
    else
    {
        /* Send Command FAILED status */
        SendStatusCSW(CSW_STATUS_COMMAND_FAILED, pBuffer);
    }

    return (false);
}

/*********************************************************************
*
*    Function:        IsCBWMeaningful
*
*    Description:    Determine if the control block packet is valid
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static bool IsCBWMeaningful (PCBW pCBW)
{
    if ((pCBW->lun <= 0x0f) &&
        (pCBW->wcbLength <= 0x10) &&
        (pCBW->wcbLength >= 0x01) &&
        (pCBW->flags == 0x00 | pCBW->flags == 0x80))
    {
        /* Check the command opcode */
        return SCSI_IsValidCommandBlock (&pCBW->wcb[0], pCBW->wcbLength);
    }
    else
    {
        return(false);
    }
}

/*********************************************************************
*
*    Function:        PidReadDataComplete
*
*    Description:    PID read completed send the data to the host
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
void PidReadDataComplete(void)
{
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;

    /* Increment block pointer */
    adi_msd_starting_block += pDevice->ReadIndex;
    
    /* send  MIN(adi_msd_RemainingDataToShipUp, pDevice->nClientBufferElementCount)
     * bytes to host
    */
    if (adi_msd_RemainingDataToShipUp >= pDevice->nClientBufferElementCount)
    {
        adi_msd_pDataToShipUp->ElementCount = pDevice->nClientBufferElementCount;
    } else {
        adi_msd_pDataToShipUp->ElementCount = adi_msd_RemainingDataToShipUp;
    }

    /* Endpoint to send data to */
    adi_msd_pDataToShipUp->Reserved[BUFFER_RSVD_EP_ADDRESS] = adi_usb_msd_IN_EP;

    /* Setup next state */
    adi_msd_CommandStatusState = STATE_SEND_DATA;

    /* Send the data */
    adi_dev_Write(adi_msd_PeripheralDevHandle,
                  ADI_DEV_1D,
                  (ADI_DEV_BUFFER*)adi_msd_pDataToShipUp);

    /* Decrement our counter */
    adi_msd_RemainingDataToShipUp -= adi_msd_pDataToShipUp->ElementCount;
}

/*********************************************************************
*
*    Function:        SendDataToHost
*
*    Description:    Send the data to the host
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static void SendDataToHost()
{
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
   
    u32 inThisPass;
    u32 nBlocksInThisPass;
    
    pDevice->PIDRead = FALSE;

    if (adi_msd_RemainingDataToShipUp)
    {
        /* Read MIN(adi_msd_RemainingDataToShipUp, pDevice->nClientBufferElementCount)
         * bytes from PID
        */
        if (adi_msd_RemainingDataToShipUp >= pDevice->nClientBufferElementCount)
        {
            inThisPass = pDevice->nClientBufferElementCount;
        }
        else {
            inThisPass = adi_msd_RemainingDataToShipUp;
        }
        /* calculate the number of blocks (Sectors) to be read from PID in this pass */
        nBlocksInThisPass = inThisPass/pDevice->MediaInfo.SectorSize;
        
        if(pDevice->SCSIReadCmd == TRUE)
        {
            if(adi_msd_HDDetected == TRUE)
            {
                pDevice->PIDRead = TRUE;
                adi_rawpid_TransferSectorsNonBlock(RAW_PID_DeviceHandle, 0, adi_msd_starting_block, nBlocksInThisPass, pDevice->MediaInfo.SectorSize, adi_msd_pDataToShipUp->Data, _READ_);
                /* save aside the value to increment block pointer later */
                pDevice->ReadIndex = nBlocksInThisPass;

            }
            else
            {
                memcpy(adi_msd_pDataToShipUp->Data, (adi_msd_Disk + (adi_msd_starting_block * pDevice->MediaInfo.SectorSize)), nBlocksInThisPass*pDevice->MediaInfo.SectorSize);
                adi_msd_starting_block += nBlocksInThisPass;
            }
        }
        
        /* We have not called a PID access function so continue processing */
        if(pDevice->PIDRead == FALSE)
        {
            /* Number of bytes to send */
            adi_msd_pDataToShipUp->ElementCount = inThisPass;

            /* Endpoint to send data to */
            adi_msd_pDataToShipUp->Reserved[BUFFER_RSVD_EP_ADDRESS] = adi_usb_msd_IN_EP;

            /* Setup next state */
            adi_msd_CommandStatusState = STATE_SEND_DATA;

            /* Send the data */
            adi_dev_Write(adi_msd_PeripheralDevHandle,
                            ADI_DEV_1D,
                            (ADI_DEV_BUFFER*)adi_msd_pDataToShipUp);

            /* Decrement our counter */
            adi_msd_RemainingDataToShipUp -= inThisPass;
        }        
    }
    else
    {
        /* All data has been sent for this pass, so now send the CSW */
        adi_msd_IsTransferActive = false;

        /* Restore the initial buffer address */
        adi_msd_pDataToShipUp->Data = adi_usb_buffer_pointer;

        /* Send Command PASSED status along with the residue */
        SendDeviceToHostDataCSW(CSW_STATUS_COMMAND_PASSED, adi_msd_pDataToShipUp, 0);
    }
}

/*********************************************************************
*
*    Function:        ExtractCBW
*
*    Description:    Extract data from a Control Block packet
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static bool ExtractCBW (PCBW pCBW, u8 *pBuffer)
{
    pCBW->signature = *(u32 *)pBuffer;
    pCBW->tag = *(u32 *)(pBuffer+CBW_TAG_OFFSET);
    pCBW->dataTransferLength = *(u32 *)(pBuffer+CBW_DATA_TRANSFER_LENGTH_OFFSET);
    pCBW->flags = *(u8 *)(pBuffer+CBW_FLAGS_OFFSET);
    pCBW->lun = *(u8 *)(pBuffer+CBW_LUN_OFFSET);
    pCBW->wcbLength = *(u8 *)(pBuffer+CBW_WCBLENGTH_OFFSET);

    memcpy (&pCBW->wcb[0], pBuffer+CBW_WCB_OFFSET, CBW_WCB_LEN);

    return (pCBW->signature == CBW_SIGNATURE);
}

/*********************************************************************
*
*    Function:        BuildCSW
*
*    Description:    Create a Command Status Block packet
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static void BuildCSW (u8 *pBuffer, u32 tag, u32 dataResidue, u8 status)
{

    *(u32 *)pBuffer = CSW_SIGNATURE;
    *(u32 *)(pBuffer + CSW_TAG_OFFSET) = tag;
    *(u32 *)(pBuffer + CSW_DATA_RESIDUE_OFFSET) = dataResidue;
    *(pBuffer + CSW_STATUS_OFFSET) = status;

}

/*********************************************************************
*
*    Function:        BuildCBW
*
*    Description:    Create a Command Block packet
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static void BuildCBW (u8 *pBuffer, u32 command)
{
    *(u32 *)pBuffer = CBW_SIGNATURE;

    switch(command)
    {
        case SCSI_INQUIRY:
        {
            *(u32 *)(pBuffer + CBW_TAG_OFFSET) = 0x85F6B6C8;
            *(u32 *)(pBuffer + CBW_DATA_TRANSFER_LENGTH_OFFSET) = 0x24;
               *(pBuffer + CBW_FLAGS_OFFSET) = CBW_FLAG_DATA_FROM_DEVICE_TO_HOST ;

               *(pBuffer + CBW_LUN_OFFSET) = 0x0;
               *(pBuffer + CBW_WCBLENGTH_OFFSET) = 0x06;

               /* Now fill in the command info. */
               *(pBuffer + CBW_WCB_OFFSET)     = SCSI_INQUIRY;
               *(pBuffer + CBW_WCB_OFFSET+1)     = 0x0;
               *(pBuffer + CBW_WCB_OFFSET+2)     = 0x0;
               *(pBuffer + CBW_WCB_OFFSET+3)     = 0x0;
               *(pBuffer + CBW_WCB_OFFSET+4)     = 0x24;
               *(pBuffer + CBW_WCB_OFFSET+5)     = 0x0;
               *(pBuffer + CBW_WCB_OFFSET+6)     = 0x0;
        };
        break;

        case SCSI_TEST_UNIT_READY:
        {
            *(u32 *)(pBuffer + CBW_TAG_OFFSET) = 0x85D90728;
            *(u32 *)(pBuffer + CBW_DATA_TRANSFER_LENGTH_OFFSET) = 0x24;
               *(pBuffer + CBW_FLAGS_OFFSET) = CBW_FLAG_DATA_FROM_DEVICE_TO_HOST ;

               *(pBuffer + CBW_LUN_OFFSET) = 0x0;
               *(pBuffer + CBW_WCBLENGTH_OFFSET) = 0x06;

               /* Now fill in the command info. */
               *(pBuffer + CBW_WCB_OFFSET)     = SCSI_TEST_UNIT_READY;
               *(pBuffer + CBW_WCB_OFFSET+1)     = 0x0;
               *(pBuffer + CBW_WCB_OFFSET+2)     = 0x0;
               *(pBuffer + CBW_WCB_OFFSET+3)     = 0x0;
               *(pBuffer + CBW_WCB_OFFSET+4)     = 0x0;
               *(pBuffer + CBW_WCB_OFFSET+5)     = 0x0;
               *(pBuffer + CBW_WCB_OFFSET+6)     = 0x0;
        };


    }
}

/*********************************************************************
*
*    Function:        MSReset
*
*    Description:    Force the Device into a reset mode
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
static void MSReset(void)
{
    adi_msd_CommandStatusState     = STATE_WAITFOR_CBW;
    adi_msd_IsTransferActive    = false;

    adi_usb_msd_device_ready = FALSE;

    adi_msd_RemainingDataToComeIn     = 0;
    adi_msd_RemainingDataToShipUp     = 0;

    adi_msd_CurrentDataWriteLocation = 0;
}

/*********************************************************************
*
*    Function:        WriteDataComplete
*
*    Description:    This function is called when write is 
*                    complete.
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
void WriteDataComplete(void)
{
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
    
    /* Update the write position */         
    adi_msd_CurrentDataWriteLocation += pDevice->WriteIndex;
            
    adi_msd_RemainingDataToComeIn -= pDevice->PIDWriteBuffer->ProcessedElementCount;

    if (adi_msd_RemainingDataToComeIn)
    {
           /* Setup our RX Buffer (D->H) */
        SetupEPBuffer((ADI_DEV_BUFFER *)pDevice->PIDWriteBuffer, adi_usb_msd_OUT_EP,
                       (adi_msd_RemainingDataToComeIn > pDevice->nClientBufferElementCount) ? pDevice->nClientBufferElementCount : adi_msd_RemainingDataToComeIn);

        return;
     }
     else
     {
        adi_msd_IsTransferActive = false;
        adi_msd_CurrentDataWriteLocation = 0;
        
    
        /* Send Command PASSED status */
        SendStatusCSW(CSW_STATUS_COMMAND_PASSED, pDevice->PIDWriteBuffer);
      }
}

/*********************************************************************
*
*    Function:        Device_To_Host_TransferComplete
*
*    Description:    This function is called when a Device to Host
*                   transfer is completed
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
void Device_To_Host_TransferComplete(void *Handle, u32 Event, void *pBuf)
{
        switch(adi_msd_CommandStatusState)
        {
            case STATE_CSW_SENT:

            adi_msd_CommandStatusState = STATE_WAITFOR_CBW;

            SetupEPBuffer((ADI_DEV_BUFFER *)pBuf, adi_usb_msd_OUT_EP, CBW_LENGTH);

            break;

            case STATE_SEND_DATA:

                SendDataToHost();

            break;

            default:
            break;
        }
}

/*********************************************************************
*
 *    Function:        Host_To_Device_TransferComplete
*
*    Description:    This function is called when a Host to Device
*                   transfer is completed
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
void Host_To_Device_TransferComplete(void *Handle, u32 Event, void *pBuf)
{
    bool validCBW = false;
    ADI_DEV_1D_BUFFER *pBuffer = (ADI_DEV_1D_BUFFER *)pBuf;
    
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;

        switch(adi_msd_CommandStatusState)
        {
            case STATE_WAITFOR_CBW:
            if (pBuffer->ProcessedElementCount >= CBW_LENGTH)
            {
                if (ExtractCBW(&adi_msd_CurrentCBW, (u8 *)pBuffer->Data))
                {
                    validCBW = true;
                    ProcessCBW (&adi_msd_CurrentCBW, (ADI_DEV_1D_BUFFER *)pBuffer);

                    return;
                }
            }
            /*
                CBW is invalid send back a CSW with status
                failed
            */
            if (!validCBW)
            {
                /* Send Command FAILED status */
                SendStatusCSW(CSW_STATUS_COMMAND_FAILED, pBuffer);

                return;
            }
            break;

            case STATE_WAITFOR_DATA:
              {    
                  /*Save the buffer for later processing in PID callback */
                  pDevice->PIDWriteBuffer = pBuffer;
                  
                SCSI_ProcessData (pBuffer->Data, pBuffer->ProcessedElementCount);

                /* Not accessing a PID so we need to continue processing here */
                if(pDevice->PIDWrite == FALSE)
                {
                    WriteDataComplete();

                }
              }
            break;
                
                default:
                return;
        }
     return;
}

/*********************************************************************
*
*    Function:        EndpointZeroCompleteCallback
*
*    Description:    Handle Endpoint 0 data traffic specific to
*                    this device class
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
void EndpointZeroCompleteCallback( void *Handle, u32 Event, void *pBuffer)
{
    ADI_USB_DEV_DEF *pDevice= (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
    PUSB_EP_INFO    pEpInfo;
    POBJECT_INFO    pOInf;
    u32             Result;

    switch(Event)
    {
        case ADI_USB_EVENT_SET_CONFIG:
            if(((u32)pBuffer) == 1)
            {
                Result = ConfigureEndpoints();
                if (Result == ADI_DEV_RESULT_SUCCESS)
                {
                   adi_usb_msd_device_ready = TRUE;
                   /* and set up buffer for first CBW */
                   if (pDevice->pClientBuffer) {
                       SetupEPBuffer((ADI_DEV_BUFFER *)pDevice->pClientBuffer, adi_usb_msd_OUT_EP, CBW_LENGTH);
                   }
                       
                }
            }
            else {
                   adi_usb_msd_device_ready = FALSE;
            }
            break;

        case ADI_USB_EVENT_SETUP_PKT:
        {
            /* Get the EP0 buffer in the correct format */
            ADI_DEV_1D_BUFFER  *pEP0Buffer     = (ADI_DEV_1D_BUFFER *)pBuffer;

            /* Get the buffer data in USB packet format */
            USB_SETUP_PACKET* pSetupData = (USB_SETUP_PACKET*)pEP0Buffer->Data;

            if ((USB_CONTROL_REQUEST_GET_TYPE (pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_CLASS) &&
                (USB_CONTROL_REQUEST_GET_RECIPIENT (pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_INTERFACE) &&
                (LOBYTE (pSetupData->wIndex) == MASS_STORAGE_INTERFACE_NUM))
            {
                /* Host Requesting a RESET */
                if ((USB_CONTROL_REQUEST_GET_DIRECTION(pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_OUT)  &&
                    (pSetupData->bRequest == USBMS_REQUEST_RESET) && (pSetupData->wValue == 0) && (pSetupData->wLength == 0))
                {
                    /* Reset the Mass Storage Device */
                    MSReset ();

                    break;
                }
                /* Host requesting the number of LUNs */
                else if ((USB_CONTROL_REQUEST_GET_DIRECTION(pSetupData->bmRequestType) == USB_CONTROL_REQUEST_TYPE_IN)  &&
                    (pSetupData->bRequest == USBMS_REQUEST_GET_MAX_LUN) && (pSetupData->wValue == 0) && (pSetupData->wLength == 1))
                {
                    memset(MaxLunData, 0, (5 * sizeof(s8)));

                    MAXLunReturnBuffer.Data = &MaxLunData;
                    MAXLunReturnBuffer.ElementCount = 1;
                    MAXLunReturnBuffer.ElementWidth = 1;
                    MAXLunReturnBuffer.CallbackParameter = &MAXLunReturnBuffer;
                    MAXLunReturnBuffer.ProcessedElementCount = 0;
                    MAXLunReturnBuffer.ProcessedFlag    = 0;
                    MAXLunReturnBuffer.pNext = NULL;

                    /* Set the address to endpoint zero */
                    MAXLunReturnBuffer.Reserved[BUFFER_RSVD_EP_ADDRESS] = 0x0;
                    
                    /* Send the Max Lun information */
                    adi_usb_TransmitEpZeroBuffer((ADI_DEV_BUFFER*)&MAXLunReturnBuffer);
                }
            }
        }
        break;

        case ADI_USB_EVENT_START_OF_FRAME:
        break;

        case ADI_USB_EVENT_VBUS_TRUE:
            (pDevice->DMCallback) ( Handle, ADI_USB_EVENT_VBUS_TRUE, 0);
        break;

        case ADI_USB_EVENT_SUSPEND:
        break;
        
        case ADI_USB_EVENT_DISCONNECT:
        {
            /* Reset the initial state variables */
            MSReset();

               if(pDevice->busSpeed==ADI_USB_DEVICE_SPEED_HIGH)
            {    
               pOInf = &pDevice->EndPointObjects[0];
            }
            else
            {
                pOInf = &pDevice->OSEndPointObjects[0];
            }                          
              
               /* Reset IN endpoint lists */
              
            pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);
            pEpInfo->pFreeXmtList = NULL;
            pEpInfo->pQueuedXmtList = NULL;
            pEpInfo->pTransferBuffer=NULL;
            pEpInfo->EpBytesProcessed=0;

            if(pDevice->busSpeed==ADI_USB_DEVICE_SPEED_HIGH)
            {    
             pOInf = &pDevice->EndPointObjects[1];
            }
            else
            {
                pOInf = &pDevice->OSEndPointObjects[1];
            }                          

             /* Reset OUT endpoint lists */

             pEpInfo = (PUSB_EP_INFO)&(((PENDPOINT_OBJECT)pOInf->pObj)->EPInfo);
             pEpInfo->pFreeRcvList = NULL;
            pEpInfo->pProcessedRcvList = NULL;
            pEpInfo->pTransferBuffer=NULL;
            pEpInfo->EpBytesProcessed=0;

            (pDevice->DMCallback) ( Handle, ADI_USB_EVENT_DISCONNECT, 0);
        }
        break;

        case ADI_USB_EVENT_ROOT_PORT_RESET: // reset signaling detected
            (pDevice->DMCallback) ( Handle, ADI_USB_EVENT_ROOT_PORT_RESET, 0);
        break;

        case ADI_USB_EVENT_VBUS_FALSE:         // cable unplugged
            (pDevice->DMCallback) ( Handle, ADI_USB_EVENT_VBUS_FALSE, 0);
        break;

        case ADI_USB_EVENT_RESUME:             // device resumed
            (pDevice->DMCallback) ( Handle, ADI_USB_EVENT_RESUME, 0);
        break;

        default:
        break;
    }

}

/*********************************************************************
*
*    Function:        EndpointCompleteCallback
*
*    Description:    Handle normal Endpoint data traffic
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
void EndpointCompleteCallback(void *Handle, u32 Event, void *pBuffer)
{
    switch(Event)
    {
        /* Transmit data to Host complete (Device Mode) */
        case ADI_USB_EVENT_DATA_TX:
            Device_To_Host_TransferComplete(Handle, Event, pBuffer);
        break;

        /* Recieve data from Host complete (Device Mode)*/
        case ADI_USB_EVENT_DATA_RX:
            Host_To_Device_TransferComplete(Handle, Event, pBuffer);
        break;

        /* Recieved a packet but we have no buffer */
        case ADI_USB_EVENT_PKT_RCVD_NO_BUFFER:
        break;

        default:
        break;
    }
}

/*********************************************************************
*
*    Function:        PidCallbackFunction
*
*    Description:    This function is called when pid access is 
*                    complete.
*
*********************************************************************/
__ADI_USB_MSD_SECTION_CODE
void PidCallbackFunction(void *pClientHandle, u32 nEvent, void *pClientArg)
{
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
    
    switch (nEvent)
    {
        /* CASE (Device Interrupt Event) */
        case (ADI_PID_EVENT_DEVICE_INTERRUPT):
        {
            if(pDevice->SCSIReadCmd == TRUE)
            {
                /* 
                       PID READ transfer complete now
                       we can write the data to the USB
                    */
                PidReadDataComplete();            
            }
            else /* PID write complete */
            {
                WriteDataComplete();            
            }
        }
        break;
    }
}
