/*****************************************************************************
 * Raw PID module for accessing a PID at its most basic level.
 * Support for one thread only.
 *
 * New with Update 3 of VisualDSP 5.0 is the ability to register and deregister
 * the device handle of a PID that has already been opened. It is envisaged that
 * in due course teh Init/Terminate approach will be deprecated.
 *
 *****************************************************************************/

#include <drivers/pid/atapi/adi_atapi.h>
#define __ADI_RAWPID_C__
#include <drivers/pid/adi_rawpid.h>

static void RawPIDCallback(void *hArg, u32 event, void *pArg);
extern u32 _adi_fss_PendOnFlag( void *pArg, volatile u32 *pFlag, u32 Value );
extern void _adi_fss_FlushMemory( char *StartAddress, u32 NumBytes, ADI_DMA_WNR Direction);

static u32 VolumeDetectComplete;
static ADI_SEM_HANDLE RawPidSemaphoreHandle = NULL;

/* Client callback function - set by application as an extern */
ADI_DCB_CALLBACK_FN adi_rawpid_ClientCallbackFn = NULL;

/* macros to encode CompletionBitMask Field */

#define __DEV_INT_MASK  0x00000001
#define __BUF_PROC_MASK 0x00000002
#define __ADI_FSS_MASK_ 0xADF50000

#define SET_DEV_INT(P)  \
        { \
            if ( ((P)&__ADI_FSS_MASK_)==__ADI_FSS_MASK_ ) { \
                (*(u32*)&(P)) |= __DEV_INT_MASK; \
            } \
        }
#define SET_BUF_PROC(P)  \
        { \
            if ( ((P)&__ADI_FSS_MASK_)==__ADI_FSS_MASK_ ) { \
                (*(u32*)&(P)) |= __BUF_PROC_MASK; \
            } \
        }


/*********************************************************************

    Function:       adi_rawpid_Init

    Description:    Initializes and activates the PID Device Driver

*********************************************************************/
u32 adi_rawpid_Init(
                    ADI_FSS_DEVICE_DEF     *pDeviceDef,
                    ADI_DEV_MANAGER_HANDLE DevManHandle,
                    ADI_DMA_MANAGER_HANDLE DmaManHandle,
                    ADI_DCB_HANDLE         DCBQueueHandle,
                    ADI_DEV_DEVICE_HANDLE  *pDeviceHandle,
                    ADI_SEM_HANDLE         *pSemaphoreHandle
)
{
    u32 Result=ADI_FSS_RESULT_SUCCESS;
    u32 Count;

    /* Force the user to supply locations for these data types
    */
    if ( !pSemaphoreHandle || !pDeviceHandle )
    {
        Result = ADI_FSS_RESULT_FAILED;
    }

    /* Create Transfer Completion Semaphore
    */
    if ( Result==ADI_FSS_RESULT_SUCCESS && adi_sem_Create(0, pSemaphoreHandle, NULL)!=ADI_SEM_RESULT_SUCCESS )
    {
        Result = ADI_FSS_RESULT_FAILED;
    }
    RawPidSemaphoreHandle = *pSemaphoreHandle;

    if ( Result==ADI_FSS_RESULT_SUCCESS )
    {
        /* Open the Device Driver
        */
        Result = adi_dev_Open(
                            DevManHandle,
                            pDeviceDef->pEntryPoint,
                            0,
                            pDeviceHandle,
                            pDeviceHandle,
                            ADI_DEV_DIRECTION_BIDIRECTIONAL,
                            DmaManHandle,
                            DCBQueueHandle,
                            RawPIDCallback
                            );

        if ( Result==ADI_DEV_RESULT_SUCCESS ) {
            Result = adi_dev_Control( *pDeviceHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void *)ADI_DEV_MODE_CHAINED );
        }

        adi_dev_Control( *pDeviceHandle, ADI_PID_CMD_SET_DIRECT_CALLBACK, (void*)RawPIDCallback );

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {

            if (pDeviceDef->pConfigTable) {
                ADI_DEV_CMD_VALUE_PAIR *pair = &pDeviceDef->pConfigTable[0];
                while (!Result && pair->CommandID!=ADI_DEV_CMD_END)
                {
                    Result = adi_dev_Control( *pDeviceHandle, pair->CommandID, (void *)pair->Value );
                    pair++;
                }
            }
        }

        /* Activate the Dard Drive and read the IDENTIFY data
        */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            Result = adi_dev_Control( *pDeviceHandle, ADI_PID_CMD_MEDIA_ACTIVATE,(void *)true );
            if ( Result==ADI_FSS_RESULT_SUCCESS )
            {
                /* This will Result in IDENTIFY being sent
                */
                VolumeDetectComplete = FALSE;
                Result = adi_dev_Control( *pDeviceHandle, ADI_PID_CMD_POLL_MEDIA_CHANGE, (void*)NULL );

                if (Result==ADI_FSS_RESULT_SUCCESS)
                {
                    /* wait for volume detection to complete
                    */
                    Count = 1000001;
                    while ( (--Count) && !VolumeDetectComplete );
                    if (Count==0) {
                        Result = ADI_FSS_RESULT_NO_MEDIA;
                    }
                }

            }
        }
    }
    return Result;

}

/*********************************************************************

    Function:       adi_rawpid_Terminate

    Description:    Terminates the PID Device Driver

*********************************************************************/
u32 adi_rawpid_Terminate( ADI_DEV_DEVICE_HANDLE DeviceHandle, ADI_SEM_HANDLE SemaphoreHandle )
{
    u32 Result;

    /* Delete Transfer Completion Semaphore
    */
    adi_sem_Delete( SemaphoreHandle );

    /* close PID device driver
    */
    Result = adi_dev_Close( DeviceHandle );

    return Result;
}

/*********************************************************************

    Function:       adi_rawpid_RegisterPID

    Description:    Registers the PID Device Driver. Assumes it has been opened
                    and configured prior to this call.

*********************************************************************/
u32 adi_rawpid_RegisterPID(
                    ADI_DEV_DEVICE_HANDLE  DeviceHandle,
                    void                   *ClientHandle
)
{
    u32 Result=ADI_DEV_RESULT_SUCCESS;
    u32 Count;

    /* Change the client callback function to the one provided in this file */
    Result = adi_dev_Control( DeviceHandle, ADI_DEV_CMD_CHANGE_CLIENT_CALLBACK_FUNCTION, (void*)RawPIDCallback );

    /* Change the client handle to the passed argument */
    if ( Result==ADI_DEV_RESULT_SUCCESS ) {
        Result = adi_dev_Control( DeviceHandle, ADI_DEV_CMD_CHANGE_CLIENT_HANDLE,(void *)ClientHandle );
    }

    /* Activate the device driver
    */
    if ( Result==ADI_DEV_RESULT_SUCCESS )
    {
        Result = adi_dev_Control( DeviceHandle, ADI_PID_CMD_MEDIA_ACTIVATE,(void *)true );
        if ( Result==ADI_DEV_RESULT_SUCCESS )
        {
            /* set the direct callback function - used for media detection */
            Result = adi_dev_Control( DeviceHandle, ADI_PID_CMD_SET_DIRECT_CALLBACK, (void*)RawPIDCallback );
        }
    }
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* Now poll for changes
        */
        VolumeDetectComplete = FALSE;
        Result = adi_rawpid_PollForMedia ( DeviceHandle );

        if (Result==ADI_FSS_RESULT_SUCCESS)
        {
            /* wait for volume detection to complete
            */
            Count = 1000001;
            while ( (--Count) && !VolumeDetectComplete );
            if (Count==0) {
                Result = ADI_FSS_RESULT_NO_MEDIA;
            }
        }

    }

    return Result;
}


/*********************************************************************

    Function:       adi_rawpid_DeregisterPID

    Description:    Removes the PID Device Driver

*********************************************************************/
u32 adi_rawpid_DeregisterPID( ADI_DEV_DEVICE_HANDLE DeviceHandle )
{
    u32 Result;

    /* Deactivate the PID */
    Result = adi_dev_Control( DeviceHandle, ADI_PID_CMD_MEDIA_ACTIVATE, (void *)false );

    return Result;
}


/*********************************************************************

    Function:       adi_rawpid_PollForMedia

    Description:    Polls for media on the given device

*********************************************************************/
u32 adi_rawpid_PollForMedia(
                    ADI_DEV_DEVICE_HANDLE  DeviceHandle
)
{
    u32 Result;

    /* Instruct the PID to detect any changes to the attached media */
    Result = adi_dev_Control( DeviceHandle, ADI_PID_CMD_POLL_MEDIA_CHANGE, (void*)NULL );

    return Result;

}


/*********************************************************************

    Function:       adi_rawpid_GetMediaInfo

    Description:    Get pertinent Drive Info

*********************************************************************/
u32 adi_rawpid_GetMediaInfo( ADI_DEV_DEVICE_HANDLE DeviceHandle, ADI_RAWPID_MEDIA_INFO *pDriveInfo )
{
    ADI_FSS_VOLUME_DEF VolumeDef;
    u32 Result;

    Result = adi_dev_Control( DeviceHandle, ADI_PID_CMD_GET_GLOBAL_MEDIA_DEF, (void*)&VolumeDef );
    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        pDriveInfo->SizeInSectors = VolumeDef.VolumeSize;
        pDriveInfo->FirstUsableSector = VolumeDef.StartAddress;
        pDriveInfo->SectorSize = VolumeDef.SectorSize ;
    }


    return Result;
}

/*********************************************************************

    Function:       adi_rawpid_TransferSectors

    Description:    Read/Write sectors from the Device

*********************************************************************/
u32 adi_rawpid_TransferSectors(
                                ADI_DEV_DEVICE_HANDLE DeviceHandle,
                                u32 Drive,
                                u32 SectorNumber,
                                u32 SectorCount,
                                u32 SectorSize,
                                u16 *buf,
                                u32 ReadFlag,
                                ADI_SEM_HANDLE SemaphoreHandle
)
{
    ADI_FSS_SUPER_BUFFER SuperBuffer;
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    u32 Locked = FALSE;

    if ( adi_dev_Control( DeviceHandle, ADI_FSS_CMD_GET_DATA_ELEMENT_WIDTH, &SuperBuffer.Buffer.ElementWidth ) )
    {
        Result = ADI_FSS_RESULT_FAILED;
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* set up for ATA request
        */
        SuperBuffer.LBARequest.StartSector      = SectorNumber;
        SuperBuffer.LBARequest.SectorCount      = SectorCount;
        SuperBuffer.LBARequest.DeviceNumber     = Drive;
        SuperBuffer.LBARequest.ReadFlag         = ReadFlag;
        SuperBuffer.LBARequest.pBuffer          = &SuperBuffer;


        /* reset completion mask */
        SuperBuffer.CompletionBitMask = __ADI_FSS_MASK_;

        /* set up Transfer Buffer
        */
        SuperBuffer.Buffer.Data                 = (void*)buf;
        SuperBuffer.Buffer.ElementCount         = (SectorCount*SectorSize)/SuperBuffer.Buffer.ElementWidth;
        SuperBuffer.Buffer.CallbackParameter    = &SuperBuffer;
        SuperBuffer.Buffer.pAdditionalInfo      = NULL;
        SuperBuffer.Buffer.ProcessedFlag        = FALSE;
        SuperBuffer.Buffer.pNext                = NULL;

        /* Assign Semaphore
        */
        SuperBuffer.SemaphoreHandle             = SemaphoreHandle;
        RawPidSemaphoreHandle                   = SemaphoreHandle;

        /* Assign Other fields to zero/false
        */
        SuperBuffer.FSDCallbackFunction         = NULL;
        SuperBuffer.FSDCallbackHandle           = NULL;
        SuperBuffer.LastInProcessFlag           = FALSE;
        SuperBuffer.pBlock                      = NULL;
        SuperBuffer.pFileDesc                   = NULL;


        /* Acquire PID Lock Semaphore
        */
        adi_dev_Control( DeviceHandle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );

        /* Send LBA request
        */
        Result = adi_dev_Control( DeviceHandle, ADI_PID_CMD_SEND_LBA_REQUEST, (void*)&SuperBuffer.LBARequest );

        /* Queue Buffer
        */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            if ( SuperBuffer.LBARequest.ReadFlag ) {
                _adi_fss_FlushMemory( SuperBuffer.Buffer.Data, SuperBuffer.Buffer.ElementCount * SuperBuffer.Buffer.ElementWidth, ADI_DMA_WNR_WRITE);
                Result = adi_dev_Read( DeviceHandle, ADI_DEV_1D, (ADI_DEV_BUFFER*)&SuperBuffer );
            } else {
                _adi_fss_FlushMemory( SuperBuffer.Buffer.Data, SuperBuffer.Buffer.ElementCount * SuperBuffer.Buffer.ElementWidth, ADI_DMA_WNR_READ);
                Result = adi_dev_Write( DeviceHandle, ADI_DEV_1D, (ADI_DEV_BUFFER*)&SuperBuffer );
            }
        }

        /* Enable PID Data Flow
        */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            adi_dev_Control( DeviceHandle, ADI_PID_CMD_ENABLE_DATAFLOW, (void*)TRUE );
        }

        /* Pend on data completion
        */
        adi_sem_Pend ( SuperBuffer.SemaphoreHandle, ADI_SEM_TIMEOUT_FOREVER );

        /* Release PID Lock Semaphore
        */
        adi_dev_Control( DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
    }

    return Result;

}

/*********************************************************************

    Function:       adi_rawpid_TransferSectorsNonBlock

    Description:    Read/Write sectors from the Device. returns without blocking

*********************************************************************/
u32 adi_rawpid_TransferSectorsNonBlock(
                                ADI_DEV_DEVICE_HANDLE DeviceHandle,
                                u32 Drive,
                                u32 SectorNumber,
                                u32 SectorCount,
                                u32 SectorSize,
                                u16 *buf,
                                u32 ReadFlag
)
{
    static ADI_FSS_SUPER_BUFFER SuperBuffer;
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    u32 Locked = FALSE;

    if ( adi_dev_Control( DeviceHandle, ADI_FSS_CMD_GET_DATA_ELEMENT_WIDTH, &SuperBuffer.Buffer.ElementWidth ) )
    {
        Result = ADI_FSS_RESULT_FAILED;
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* set up for ATA request
        */
        SuperBuffer.LBARequest.StartSector      = SectorNumber;
        SuperBuffer.LBARequest.SectorCount      = SectorCount;
        SuperBuffer.LBARequest.DeviceNumber     = Drive;
        SuperBuffer.LBARequest.ReadFlag         = ReadFlag;
        SuperBuffer.LBARequest.pBuffer          = &SuperBuffer;

        /* reset completion mask */
        SuperBuffer.CompletionBitMask = __ADI_FSS_MASK_;

        /* set up Transfer Buffer
        */
        SuperBuffer.Buffer.Data                 = (void*)buf;
        SuperBuffer.Buffer.ElementCount         = (SectorCount*SectorSize)/SuperBuffer.Buffer.ElementWidth;
        SuperBuffer.Buffer.CallbackParameter    = &SuperBuffer;
        SuperBuffer.Buffer.pAdditionalInfo      = NULL;
        SuperBuffer.Buffer.ProcessedFlag        = FALSE;
        SuperBuffer.Buffer.pNext                = NULL;

        /* Assign Semaphore to -1 so that it does not match with the global semaphore
        */
        SuperBuffer.SemaphoreHandle             = (ADI_SEM_HANDLE)(-1);

        /* Assign Other fields to zero/false
        */
        SuperBuffer.FSDCallbackFunction         = NULL;
        SuperBuffer.FSDCallbackHandle           = NULL;
        SuperBuffer.LastInProcessFlag           = FALSE;
        SuperBuffer.pBlock                      = NULL;
        SuperBuffer.pFileDesc                   = NULL;


        /* Send LBA request
        */
        Result = adi_dev_Control( DeviceHandle, ADI_PID_CMD_SEND_LBA_REQUEST, (void*)&SuperBuffer.LBARequest );

        /* Queue Buffer
        */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            if ( SuperBuffer.LBARequest.ReadFlag ) {
                _adi_fss_FlushMemory( SuperBuffer.Buffer.Data, SuperBuffer.Buffer.ElementCount * SuperBuffer.Buffer.ElementWidth, ADI_DMA_WNR_WRITE);
                Result = adi_dev_Read( DeviceHandle, ADI_DEV_1D, (ADI_DEV_BUFFER*)&SuperBuffer );
            } else {
                _adi_fss_FlushMemory( SuperBuffer.Buffer.Data, SuperBuffer.Buffer.ElementCount * SuperBuffer.Buffer.ElementWidth, ADI_DMA_WNR_READ);
                Result = adi_dev_Write( DeviceHandle, ADI_DEV_1D, (ADI_DEV_BUFFER*)&SuperBuffer );
            }
        }

        /* Enable PID Data Flow
        */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            adi_dev_Control( DeviceHandle, ADI_PID_CMD_ENABLE_DATAFLOW, (void*)TRUE );
        }

    }

    return Result;

}

/*********************************************************************

    Function:       PIDCallback

    Description:    Callback function for ATAPI PID events

*********************************************************************/
static void
RawPIDCallback(void *pHandle, u32 Event, void *pArg)
{
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;

    /* CASEOF ( Event flag ) */
    switch (Event)
    {
        /* CASE (Device Interrupt Event) */
        case (ADI_PID_EVENT_DEVICE_INTERRUPT):
            SET_DEV_INT(pSuperBuffer->CompletionBitMask);
            /* Call PID callback function */
            if (pSuperBuffer->PIDCallbackFunction) {
                (pSuperBuffer->PIDCallbackFunction)( pSuperBuffer->PIDCallbackHandle, Event, pArg );
            }
            /* and Post the transfer completion semaphore */
            if ((pSuperBuffer->CompletionBitMask&(~0xADF50000)) == 3) {
                if (RawPidSemaphoreHandle==pSuperBuffer->SemaphoreHandle ) {
                    adi_sem_Post ( pSuperBuffer->SemaphoreHandle );
                }
            }
            break;

        /* CASE (Transfer Completion Event) */
        case (ADI_DEV_EVENT_BUFFER_PROCESSED):
            SET_BUF_PROC(pSuperBuffer->CompletionBitMask);
            /* Call PID callback function */
            if (pSuperBuffer->PIDCallbackFunction) {
                (pSuperBuffer->PIDCallbackFunction)( pSuperBuffer->PIDCallbackHandle, Event, pArg );
            }
            if ((pSuperBuffer->CompletionBitMask&(~0xADF50000)) == 3) {
                if (RawPidSemaphoreHandle==pSuperBuffer->SemaphoreHandle ) {
                    adi_sem_Post ( pSuperBuffer->SemaphoreHandle );
                }
            }
            break;

        /* The following are greatly simplified, as we do not need to read the MBR and mount a file system
        */

        /* CASE (Media Insertion event) */
        case (ADI_FSS_EVENT_MEDIA_INSERTED):
            /* normally issue command to detect volumes */
            *((u32 *)pArg) = ADI_FSS_RESULT_SUCCESS;
            VolumeDetectComplete = TRUE;
            break;

        /* CASE (Media removal event) */
        case (ADI_FSS_EVENT_MEDIA_REMOVED):
            /* normally issue command to detect volumes */

        /* CASE (Media removal event) */
        case (ADI_FSS_EVENT_VOLUME_DETECTED):
            /* but we will drop through to here to tell our POST function we are ready to go */
            VolumeDetectComplete = TRUE;
            break;
    /* END CASEOF */
    }

    if (adi_rawpid_ClientCallbackFn) {
        (adi_rawpid_ClientCallbackFn) ( pHandle, Event, pArg );
    }
}
