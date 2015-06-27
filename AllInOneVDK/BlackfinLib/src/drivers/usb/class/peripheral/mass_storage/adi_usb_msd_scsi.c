/********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_usb_msd_scsi.c,v $
$Revision: 2386 $
$Date: 2010-03-25 17:26:08 -0400 (Thu, 25 Mar 2010) $

Description:

    This is the SCSI protocol code for the USB Mass
    Storage Class Driver (Device Mode).

*********************************************************************************/

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_USB_SCSI_SECTION_CODE    section("adi_usb_scsi_code")
#define __ADI_USB_SCSI_SECTION_DATA    section("adi_usb_scsi_data")
#else
#define __ADI_USB_SCSI_SECTION_CODE
#define __ADI_USB_SCSI_SECTION_DATA
#endif


#include <drivers/usb/usb_core/adi_usb_types.h>         /* USB types includes */

#include <services/services.h>      /* System service includes */

#include <services/fss/adi_fss.h>   /* File System header                  */

#include <drivers/usb/class/peripheral/mass_storage/adi_usb_msd_class.h>        /* USB Mass Storage structure */
#include <drivers/usb/class/peripheral/mass_storage/adi_usb_msd_scsi.h>     /* SCSI includes */

#include <string.h>
#include <stdio.h>


#include <drivers/pid/atapi/adi_atapi.h>

extern  ADI_DEV_DEVICE_HANDLE RAW_PID_DeviceHandle;

/* Define min and max block sizes for this device (8MB or 16MB) */

#if defined(ADI_DISK_16MB)
#include <drivers/usb/class/peripheral/mass_storage/formatted_16MB.h>
#define MSD_MAX_BLOCKS                      (u32)32768
#else
#include <drivers/usb/class/peripheral/mass_storage/formatted_8MB.h>
#define MSD_MAX_BLOCKS                      (u32)16384
#endif

#define MSD_BLOCK_SIZE                      (u32)512

#define SCSI_SENSE_DATA_VALID           0x80
#define SCSI_SENSE_DATA_CURRENT_ERROR   0x70

#define SCSI_INQUIRY_REPORT_SIZE                36
#define SCSI_READ_CAPACITY_REPORT_SIZE          8
#define SCSI_REQUEST_SENSE_REPORT_SIZE          18
#define SCSI_MODE_SENSE_REPORT_SIZE             0xc0
#define SCSI_READ_FORMAT_CAPACITIES_SIZE        12

/* Globals */

__ADI_USB_SCSI_SECTION_DATA
static char adi_msd_InquiryReport[SCSI_INQUIRY_REPORT_SIZE];
__ADI_USB_SCSI_SECTION_DATA
static char adi_msd_InquiryReport_1[SCSI_INQUIRY_REPORT_SIZE];
__ADI_USB_SCSI_SECTION_DATA
static u8 adi_msd_DataSaveLocation;
__ADI_USB_SCSI_SECTION_DATA
static u8 adi_msd_ReadCapacityReport[SCSI_READ_CAPACITY_REPORT_SIZE];
__ADI_USB_SCSI_SECTION_DATA
static u8 adi_msd_RequestSenseReport[SCSI_REQUEST_SENSE_REPORT_SIZE];
__ADI_USB_SCSI_SECTION_DATA
static u8 adi_msd_ModeSenseReport[SCSI_MODE_SENSE_REPORT_SIZE];
__ADI_USB_SCSI_SECTION_DATA
static u8 adi_msd_ReadFormatCapacitiesReport[SCSI_READ_FORMAT_CAPACITIES_SIZE];
__ADI_USB_SCSI_SECTION_DATA
static u8 adi_msd_ModeSense10Report[SCSI_MODE_SENSE_REPORT_SIZE];

__ADI_USB_SCSI_SECTION_DATA
u32 adi_msd_CurrentDataWriteLocation;
__ADI_USB_SCSI_SECTION_DATA
u32 adi_msd_starting_block;
__ADI_USB_SCSI_SECTION_DATA
u32 adi_msd_blocks_To_Read;

__ADI_USB_SCSI_SECTION_DATA
const char VID[] = "ADI\0";
__ADI_USB_SCSI_SECTION_DATA
const char PID[] = "MS Class\0";
__ADI_USB_SCSI_SECTION_DATA
const char REV[] = "0.1\0";

__ADI_USB_SCSI_SECTION_DATA
const char VID1[] = "ADI MSD\0";

#define MSD_RAMDISKSIZE      MSD_MAX_BLOCKS * MSD_BLOCK_SIZE

//u8 adi_msd_disk_mem[MSD_RAMDISKSIZE];
__ADI_USB_SCSI_SECTION_DATA
u8 *adi_msd_Disk;

__ADI_USB_SCSI_SECTION_DATA
bool    bFormatCapacityCalled   = false;
__ADI_USB_SCSI_SECTION_DATA
bool    bModeSenseReportCalled  = false;
__ADI_USB_SCSI_SECTION_DATA
bool    bLargeBlockSize = false;

extern bool adi_msd_HDDetected;
extern  ADI_USB_DEV_DEF     adi_msd_usb_dev_def;

typedef struct DebugInfo
{
    u32 startingBlock;
    u32 blocksToWrite;
}DEBUG_INFO,*PDEBUG_INFO;

__ADI_USB_SCSI_SECTION_DATA
PDEBUG_INFO pWriteInfo;

extern u32 adi_usb_msd_max_memory_size;

extern u32 adi_msd_RemainingDataToComeIn;


/*------------------------------------------------------------
*   SCSI_Init
*
*   Parameters:
*       None.
*
*   Globals Used:
*       adi_msd_Disk
*
*   Description:
*       Initializes SDRAM access, and also initializes other
*       globals. It also initializes the data block to contain
*       an initialized copy of a FAT formatted disk!
*
*   Returns:
*       Nothing.
*
*------------------------------------------------------------*/
__ADI_USB_SCSI_SECTION_CODE
u32 SCSI_Init(void)
{
    //ADI_RAWPID_MEDIA_INFO MediaInfo;
    ADI_USB_DEV_DEF *pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
    u32 bIsFixedMedia=false;

    u32 Result = ADI_DEV_RESULT_SUCCESS;
    if(adi_msd_HDDetected == false)
    {
        /* Allocate the memory needed */
        adi_msd_Disk = (u8 *)malloc(adi_usb_msd_max_memory_size * sizeof(u8));

        /* If Allocation failed */
        if(adi_msd_Disk == NULL)
            return ADI_DEV_RESULT_FAILED;

        /* Initialize our Disk memory */
        memset (adi_msd_Disk, 0, (adi_usb_msd_max_memory_size * sizeof(u8)));

        /* Make our memory look like a FAT formatted disk */
        #if defined(ADI_DISK_16MB)
        memcpy (adi_msd_Disk, &gFormat_16MB[0], sizeof(gFormat_16MB));
        #else
        memcpy (adi_msd_Disk, &gFormat_8MB[0], sizeof(gFormat_8MB));
        #endif
    }

    /* Initialize our Inquiry Report */
    memset (&adi_msd_InquiryReport[0], 0, SCSI_INQUIRY_REPORT_SIZE);

    /* Direct-access device */
    adi_msd_InquiryReport[0] = SCSI_DEVICE_DIRECT_ACCESS;

    if(adi_msd_HDDetected == false)
    {
        /* Removable device */
        adi_msd_InquiryReport[1] = REMOVABLE_DEVICE;

    } else {
        Result = adi_dev_Control( RAW_PID_DeviceHandle, ADI_PID_CMD_GET_FIXED, (void*)&bIsFixedMedia );
        if (Result==ADI_DEV_RESULT_SUCCESS && !bIsFixedMedia)
        {
            /* Removable device */
            adi_msd_InquiryReport[1] = REMOVABLE_DEVICE;
        }
    }

    /* SPC-2 compliance */
    adi_msd_InquiryReport[2] = SPC_2;

    /* Responce data format */
    adi_msd_InquiryReport[3] = RESPONCE_FORMAT;

    /* Responce has 36 bytes */
    adi_msd_InquiryReport[4] = RESPONCE_SIZE;

    /* Vendor ID string */
    strcpy(&adi_msd_InquiryReport[8], &VID[0]);

    /* Product ID string */
    strcpy(&adi_msd_InquiryReport[16], &PID[0]);

    /* Rev string */
    strcpy(&adi_msd_InquiryReport[32], &REV[0]);

    /* Initialize our Inquiry Report */
    memset (&adi_msd_InquiryReport_1[0], 0x20, SCSI_INQUIRY_REPORT_SIZE);

    /* Direct-access device */
    adi_msd_InquiryReport_1[0] = SCSI_DEVICE_DIRECT_ACCESS;

    /* Page */
    adi_msd_InquiryReport_1[1] = 0x00;

    /* Page length */
    adi_msd_InquiryReport_1[3] = 0x02;

    /* ASCII Length */
    adi_msd_InquiryReport_1[4] = 0x1F;

    /* Vendor ID string */
    strcpy(&adi_msd_InquiryReport_1[5], &VID1[0]);

    memset (&adi_msd_ReadCapacityReport[0], 0, SCSI_READ_CAPACITY_REPORT_SIZE);

    if(adi_msd_HDDetected == true)
    {
        pDevice->MediaInfo.SizeInSectors = 0;
        pDevice->MediaInfo.FirstUsableSector = 0;
        pDevice->MediaInfo.SectorSize = 0;

        Result = adi_rawpid_GetMediaInfo(RAW_PID_DeviceHandle, &pDevice->MediaInfo );
        *(u32 *)(&adi_msd_ReadCapacityReport[0]) = EndianSwap ((pDevice->MediaInfo.SizeInSectors - 1));
        *(u32 *)(&adi_msd_ReadCapacityReport[4]) = EndianSwap (pDevice->MediaInfo.SectorSize);
    }
    else
    {
        pDevice->MediaInfo.SectorSize = MSD_BLOCK_SIZE;
        pDevice->MediaInfo.SizeInSectors = adi_usb_msd_max_memory_size/pDevice->MediaInfo.SectorSize;
        pDevice->MediaInfo.FirstUsableSector = 0;
        *(u32 *)(&adi_msd_ReadCapacityReport[0]) = EndianSwap ((pDevice->MediaInfo.SizeInSectors - 1));
        *(u32 *)(&adi_msd_ReadCapacityReport[4]) = EndianSwap (pDevice->MediaInfo.SectorSize);
    }

    /* Initialize and setup the Format Capacity Report */
    memset (&adi_msd_ReadFormatCapacitiesReport[0], 0, SCSI_READ_FORMAT_CAPACITIES_SIZE);

    adi_msd_ReadFormatCapacitiesReport[0]   = 0x00;
    adi_msd_ReadFormatCapacitiesReport[1]   = 0x00;
    adi_msd_ReadFormatCapacitiesReport[2]   = 0x00;
    adi_msd_ReadFormatCapacitiesReport[3]   = 0x08;

    if(adi_msd_HDDetected == true)
    {
        *(u32 *)(&adi_msd_ReadFormatCapacitiesReport[4]) = EndianSwap ((pDevice->MediaInfo.SizeInSectors - 1));
    }
    else
    {
        *(u32 *)(&adi_msd_ReadFormatCapacitiesReport[4]) = EndianSwap ((MSD_MAX_BLOCKS-1));
    }

    adi_msd_ReadFormatCapacitiesReport[8]   = 0x02;
    adi_msd_ReadFormatCapacitiesReport[9]   = 0x00;
    adi_msd_ReadFormatCapacitiesReport[10]  = 0x02;
    adi_msd_ReadFormatCapacitiesReport[11]  = 0x00;

    if(adi_msd_HDDetected == true)
    {
        pWriteInfo = (PDEBUG_INFO)malloc(sizeof(DEBUG_INFO));
    }

    adi_msd_CurrentDataWriteLocation = 0;

    return ADI_DEV_RESULT_SUCCESS;
}

/*------------------------------------------------------------
*   IsValidCommandBlock
*
*   Parameters:
*       pCommandBlock - pointer to the CDB.
*       len - length of the CDB
*
*   Globals Used:
*       adi_msd_RequestSenseReport
*
*   Description:
*       Determines if the command data block received is
*       vaild or not. Accordinly it updates the request sense data.
*
*   Returns:
*
*
*------------------------------------------------------------*/

__ADI_USB_SCSI_SECTION_CODE
bool SCSI_IsValidCommandBlock (u8 * pCommandBlock, u8 len)
{
            switch (pCommandBlock[0])
            {
                case SCSI_INQUIRY:
                case SCSI_READ_CAPACITY:
                case SCSI_READ_FORMAT_CAPACITIES:
                case SCSI_READ10:
                case SCSI_WRITE10:
                case SCSI_REQUEST_SENSE:
                case SCSI_MODE_SENSE6:
                case SCSI_MODE_SENSE10:
                case SCSI_TEST_UNIT_READY:
                case SCSI_MEDIUM_REMOVAL:
                case SCSI_START_STOP:
                case SCSI_VERIFY:

                    memset (&adi_msd_RequestSenseReport[0], 0, SCSI_REQUEST_SENSE_REPORT_SIZE);
                    adi_msd_RequestSenseReport[0] = SCSI_SENSE_DATA_VALID | SCSI_SENSE_DATA_CURRENT_ERROR;
                    adi_msd_RequestSenseReport[2] = SENSE_NONE; //Sense key

                return (true);

                default:
                    memset (&adi_msd_RequestSenseReport[0], 0, SCSI_REQUEST_SENSE_REPORT_SIZE);
                    adi_msd_RequestSenseReport[0] = SCSI_SENSE_DATA_VALID | SCSI_SENSE_DATA_CURRENT_ERROR;
                    adi_msd_RequestSenseReport[2] = SENSE_ILLEGAL_REQUEST; //Sense key
                    adi_msd_RequestSenseReport[12] =  ASC_INVALID_COMMAND;//Additional Sense Code

                return(false);
            }
}

/*------------------------------------------------------------
*   SCSI_ProcessCDB
*
*   Parameters:
*
*   Globals Used:
*       adi_msd_ModeSenseReport, adi_msd_pDataBlock, adi_msd_CurrentDataWriteLocation
*       adi_msd_InquiryReport, adi_msd_ReadCapacityReport, adi_msd_RequestSenseReport
*
*   Description:
*       This routine processes the SCSI command data block received by
*       the Mass storage protocol handler.
*
*   Returns:
*       True if the command was handled successfully, false otherwise.
*
*------------------------------------------------------------*/
__ADI_USB_SCSI_SECTION_CODE
bool SCSI_ProcessCDB (u8 * pCommandBlock, u8 len, u32 datalen, void *pBuffer, u32 *pActualBytesToReturn)
{
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;

    *pActualBytesToReturn = datalen;

    pDevice->SCSIReadCmd = FALSE;
    pDevice->SCSIWriteCmd = FALSE;

        switch (pCommandBlock[0])
        {
            case SCSI_INQUIRY:
            if(pCommandBlock[1] == 0)
            {
                if (datalen  == sizeof (adi_msd_InquiryReport))
                {
                    memcpy(pBuffer, &adi_msd_InquiryReport, datalen);
                    return (true);
                }
            }
            else // Added for Windows Vista
            if(pCommandBlock[1] == 1)
            {
                memcpy(pBuffer, &adi_msd_InquiryReport_1, 36);
                *pActualBytesToReturn = 36;
                return(true);
            }
            break;

            case SCSI_READ_CAPACITY:
            if (datalen  == sizeof (adi_msd_ReadCapacityReport))
            {
                memcpy(pBuffer, &adi_msd_ReadCapacityReport, datalen);
                return (true);
            }
            break;

            case SCSI_READ_FORMAT_CAPACITIES:
            datalen = SCSI_READ_FORMAT_CAPACITIES_SIZE;

            if (datalen  == sizeof (adi_msd_ReadFormatCapacitiesReport))
            {
                bFormatCapacityCalled = true;
                memcpy(pBuffer, &adi_msd_ReadFormatCapacitiesReport, datalen);

                /* Set actual amount to send to host */
                *pActualBytesToReturn = 12;

                return (true);
            }
            break;

            case SCSI_REQUEST_SENSE:
            if (datalen  == sizeof (adi_msd_RequestSenseReport))
            {
                memcpy(pBuffer, &adi_msd_RequestSenseReport, datalen);
                return (true);
            }
            break;

            case SCSI_TEST_UNIT_READY:
            return (true);

            case SCSI_START_STOP:
            return (true);

            case SCSI_MEDIUM_REMOVAL:
            return (true);

            case SCSI_VERIFY:
            return (true);

            case SCSI_MODE_SENSE6:
            if (datalen  >= 4)
            {
                memset (&adi_msd_ModeSenseReport[0], 0, SCSI_MODE_SENSE_REPORT_SIZE);
                /* mode data length */
                adi_msd_ModeSenseReport[0] = 0x03;
                /* medium type (current) */
                adi_msd_ModeSenseReport[1] = 0x00;
                /* device-specific */
                adi_msd_ModeSenseReport[2] = 0x00;
                /* block descriptor length */
                adi_msd_ModeSenseReport[3] = 0x00;

                bModeSenseReportCalled = true;

                memcpy(pBuffer, &adi_msd_ModeSenseReport, 4);

                /* Set actual amount to send to host */
                *pActualBytesToReturn = 4;

                return(true);
            }
            break;

            // Added for Apple Mac OSX
            case SCSI_MODE_SENSE10:
            {
                memset (&adi_msd_ModeSense10Report[0], 0, SCSI_MODE_SENSE_REPORT_SIZE);

                adi_msd_ModeSense10Report[0] = 0x00;
                adi_msd_ModeSense10Report[1] = 0x26;
                adi_msd_ModeSense10Report[2] = 0x00;
                adi_msd_ModeSense10Report[3] = 0x00;
                adi_msd_ModeSense10Report[4] = 0x00;
                adi_msd_ModeSense10Report[5] = 0x00;
                adi_msd_ModeSense10Report[6] = 0x00;
                adi_msd_ModeSense10Report[7] = 0x00;

                memcpy(pBuffer, &adi_msd_ModeSense10Report, 8);

                return(true);
            }

            case SCSI_READ10:
            {
                u32 startingBlock, blocksToRead;

                adi_msd_starting_block = startingBlock =
                    (pCommandBlock[2] << 24) + (pCommandBlock[3] << 16) + (pCommandBlock[4] << 8) + pCommandBlock[5];

                adi_msd_blocks_To_Read = blocksToRead =
                    (pCommandBlock[7] << 8) + pCommandBlock[8];

                pDevice->SCSIReadCmd = TRUE;

                *pActualBytesToReturn = (blocksToRead * pDevice->MediaInfo.SectorSize);
                return (true);
            }

            case SCSI_WRITE10:
            {
                pDevice->SCSIWriteCmd = TRUE;
                u32 startingBlock, blocksToWrite;

                startingBlock = (pCommandBlock[2] << 24) + (pCommandBlock[3] << 16) + (pCommandBlock[4] << 8) + pCommandBlock[5];
                blocksToWrite = (pCommandBlock[7] << 8) + pCommandBlock[8];
                *pActualBytesToReturn = (blocksToWrite * pDevice->MediaInfo.SectorSize);

                if(adi_msd_HDDetected == true)
                {
                    pWriteInfo->startingBlock = startingBlock;
                    pWriteInfo->blocksToWrite = blocksToWrite;

                    adi_msd_CurrentDataWriteLocation = startingBlock;

                }
                else
                {
                    /* For built-in RAM Disk the current location is the byte position not the sector */
                    adi_msd_CurrentDataWriteLocation = startingBlock * pDevice->MediaInfo.SectorSize;
                }
                return (true);
            }
        }

        memset (&adi_msd_RequestSenseReport[0], 0, SCSI_REQUEST_SENSE_REPORT_SIZE);
        adi_msd_RequestSenseReport[0] = SCSI_SENSE_DATA_VALID | SCSI_SENSE_DATA_CURRENT_ERROR;
        adi_msd_RequestSenseReport[2] = SENSE_ILLEGAL_REQUEST;
        adi_msd_RequestSenseReport[12] =  ASC_INVALID_COMMAND;

        return(false);
}

/*------------------------------------------------------------
*   SCSI_ProcessData
*
*   Parameters:
*       pBuffer -pointer to the data to process.
*       len - total number of bytes to process.
*
*   Globals Used:
*       gpDataBlock, gCurrentDataWriteLocation
*
*   Description:
*       Processes data associated with a SCSI_WRITE operation.
*
*   Returns:
*       Nothing
*
*------------------------------------------------------------*/
__ADI_USB_SCSI_SECTION_CODE
void SCSI_ProcessData (void *pBuffer, u32 len)
{
    ADI_USB_DEV_DEF   *pDevice;
    pDevice = (ADI_USB_DEV_DEF *)&adi_msd_usb_dev_def;
    u32 nBlocksInThisPass, inThisPass;

    if(adi_msd_HDDetected == true)
    {
        /* How many we are going to send */
        if (adi_msd_RemainingDataToComeIn >= pDevice->nClientBufferElementCount)
        {
            inThisPass = pDevice->nClientBufferElementCount;
        }
        else {
            inThisPass = adi_msd_RemainingDataToComeIn;
        }
        nBlocksInThisPass = inThisPass/pDevice->MediaInfo.SectorSize;
        /* We are going to write using the PID */
        pDevice->PIDWrite = TRUE;

        /* Save the write increment value */
        pDevice->WriteIndex = nBlocksInThisPass;

        /* Write the data to the mass storage media */
        adi_rawpid_TransferSectorsNonBlock( RAW_PID_DeviceHandle,
                                    0,
                                    adi_msd_CurrentDataWriteLocation,
                                    nBlocksInThisPass,
                                    pDevice->MediaInfo.SectorSize,
                                    pBuffer,
                                    _WRITE_
                                    );
    }
    else
    {
        /* Write directly to memory using memcpy() */
        pDevice->PIDWrite = FALSE;

        /* Save the write increment value */
        pDevice->WriteIndex = len;

        memcpy (adi_msd_Disk + adi_msd_CurrentDataWriteLocation, pBuffer, len);
    }

}
