/*********************************************************************************

Copyright(c) 2009 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_fss_cache.c,v $
$Revision: 3578 $
$Date: 2010-10-19 09:36:04 -0400 (Tue, 19 Oct 2010) $

Description:
            This file contains the code for the file cache

*********************************************************************************/

//#define ADI_SSL_FSS_CACHE_DEBUG

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_FSS_CACHE_SECTION_CODE    section("adi_fss_cache_code")
#define __ADI_FSS_CACHE_SECTION_DATA    section("adi_fss_cache_data")
#else
#define __ADI_FSS_CACHE_SECTION_CODE
#define __ADI_FSS_CACHE_SECTION_DATA
#endif


#ifdef ADI_SSL_FSS_CACHE_DEBUG
#include <stdio.h>
__ADI_FSS_CACHE_SECTION_DATA
static int IndentCount;
#define PUSH_INDENT (IndentCount += 2)
#define POP_INDENT (IndentCount -= 2)
#define PAD {                                           \
                u32 tmp;                                \
                for (tmp = IndentCount; tmp; tmp--) {   \
                    printf (" ");                       \
                }                                       \
            }

__ADI_FSS_CACHE_SECTION_DATA
static int devReadCount;
__ADI_FSS_CACHE_SECTION_DATA
static int devWriteCount;
__ADI_FSS_CACHE_SECTION_DATA
static int CallbackCount;

#endif

#include <string.h>
#include <services/fss/adi_fss.h>
#include "adi_fss_cache.h"


/*********************************************************************

public defines and includes not globally exported
 (ie non advertised) functions .

*********************************************************************/


extern void *_adi_fss_malloc(int id,  size_t size );
extern void _adi_fss_free(int id,  void *p );
extern void *_adi_fss_MallocAligned( int id, size_t size );
extern void _adi_fss_FreeAligned( int id, void *p );

#define ADI_FSS_CACHE_GENERAL_HEAP_ID   (-1)    // head ID for general (non-data) mallocs

/*******************************************************************************

    Data Structures

********************************************************************************/

__ADI_FSS_CACHE_SECTION_DATA
typedef enum {
    ADI_FSS_CACHE_STATE_EMPTY,      // block is empty
    ADI_FSS_CACHE_STATE_FILLING,    // block is currently reading in data from the file
    ADI_FSS_CACHE_STATE_FLUSHING,   // block is currently writing data out to the file
    ADI_FSS_CACHE_STATE_FULL,       // block is filled with data
} ADI_FSS_CACHE_STATE;


typedef struct adi_cache_block {
    volatile ADI_FSS_CACHE_STATE    State;                  // current state of the block
    u8                              DirtyFlag;              // dirty status flag
    u8                              RefillFlag;             // block needs to be refilled after flush
    u32                             CurrentOffset;          // current offset within the block
    u32                             StartPosition;          // position within the file of the first piece of data for the block
    u32                             NewCurrentOffset;       // current offset after the block is filled
    u32                             NewStartPosition;       // start position after the block is filled
    u32                             HighestDirtyOffset;     // highest offset within the block that is dirty
    ADI_FSS_SUPER_BUFFER            *pFirstSuperBuffer;     // first superbuffer in the chain for this block
    ADI_FSS_SUPER_BUFFER            *pLastSuperBuffer;      // last superbuffer in the chain for this block
    struct adi_cache                *pCache;                // pointer to cache this block belongs to
    struct adi_cache_block          *pNext;                 // pointer to the next block in the chain
    u8                              *pData;                 // pointer to the actual data for the block
} ADI_FSS_CACHE_BLOCK;

typedef struct adi_cache {
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor;       // pointer to the file descriptor
    s32                     HeapID;                 // heap ID for mallocs/frees
    u32                     StartPosition;          // position within the file of the first byte cached
    u32                     EndPosition;            // position within the file of the last byte cached
    u32                     FSDPosition;            // position the FSD has into the file
    u32                     BlockSize;              // size of the data within each block (in bytes)
    u32                     ClustersPerBlock;       // clusters per block
    u32                     ClusterSize;            // cluster size
    u32                     ElementCount;
    ADI_FSS_CACHE_BLOCK     *pHead;                 // head block
    ADI_FSS_CACHE_BLOCK     *pTail;                 // tail block
    ADI_SEM_HANDLE          DataSemaphoreHandle;    /* Semaphore Handle for Data transfer completion */
    u8                      ZeroFillFlag;           // flag indicating if we need to zero fill blocks
} ADI_FSS_CACHE;



/*******************************************************************************

    static functions

********************************************************************************/

static u32 adi_fss_CacheFlushBlock          (ADI_FSS_CACHE_BLOCK *pBlock);
static u32 adi_fss_CacheFillBlock           (ADI_FSS_CACHE_BLOCK *pBlock);
static u32 adi_fss_CacheFlushAndRefillBlock (ADI_FSS_CACHE_BLOCK *pBlock);
static u32 adi_fss_CacheMoveToTail          (ADI_FSS_CACHE_BLOCK *pBlock);
static u32 adi_fss_CacheBlocked             (ADI_FSS_CACHE_BLOCK *pBlock);

#ifdef ADI_SSL_DEBUG
static u32 adi_fss_CacheValidateCache       (ADI_FSS_CACHE *pCache);
#endif


__ADI_FSS_CACHE_SECTION_DATA
static u32 gSemTimeOutArg = ADI_SEM_TIMEOUT_FOREVER;
__ADI_FSS_CACHE_SECTION_DATA
static u32 gMaxRetryCount = 0;


/*********************************************************************

    Function:       adi_fss_CacheInit

    Description:    Mallocs and initializes a cache for a file

                    Note: PIDs expect 16 bit transfers so all buffers
                    are setup for 16 bit wide elements

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
u32 adi_fss_CacheInit(                      /* initializes a cache for a file */
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor,   /* pointer to the file descriptor */
    s32                     HeapID,             /* heap ID for the cache data */
    u32                     BlockCount,         /* number of blocks in the cache */
    u32                     ClustersPerBlock,   /* number of clusters per block */
    u32                     ClusterSize,        /* cluster size (in bytes) */
    u32                     SemTimeOutArg,      /* Argument for adi_sem_Pend timeout arg */
    u32                     MaxRetryCount       /* Max number of retry attempts (0 means, fail after 1st attempt) */
) {

    u32                         Result;             // return code
    ADI_FSS_CACHE               *pCache;            // pointer to the cache
    ADI_FSS_CACHE_BLOCK         *pBlock;            // pointer to a cache block
    ADI_FSS_SUPER_BUFFER        *pSuperBuffer;      // pointer to a superbuffer
    ADI_DEV_1D_BUFFER           *pBuffer;           // pointer to a buffer
    u8                          *pByte;             // generic pointer
    u32                         i;                  // counter
    u32                         MemoryRequired;     // amount of memory required for mallocs of blocks
    u32                         BlocksAllocated;    // number of blocks successfully allocated
    u32                         DataElementWidth;   /* Width of each data element - depends on FSD/PID */
    ADI_DEV_DEVICE_HANDLE       FSDHandle;              // handle to the FSD
    u32                         MaxClustersPerBlock;    

    // be an optimist
    Result = ADI_FSS_RESULT_SUCCESS;
    gSemTimeOutArg = SemTimeOutArg;
    gMaxRetryCount = MaxRetryCount;
    FSDHandle = pFileDescriptor->FSD_device_handle;

    // need a minimum of 2 cache blocks
    if (BlockCount < 2) {
        BlockCount = 2;
    }
    if ( adi_dev_Control(FSDHandle, ADI_FSD_CMD_GET_MAX_CONTIG_BLOCKS, &MaxClustersPerBlock) == ADI_DEV_RESULT_SUCCESS) 
    {
        /* take the minimum of desired and maximum */
        MaxClustersPerBlock = ( (MaxClustersPerBlock > ClustersPerBlock) ? ClustersPerBlock : MaxClustersPerBlock);
    }
    else {
        /* otherwise we need to play it safe with just one block */
        MaxClustersPerBlock = 1;
    }

    // compute how much memory we need for the cache itself, each block structure and each superbuffer
    MemoryRequired = sizeof(ADI_FSS_CACHE) + (BlockCount * (sizeof(ADI_FSS_CACHE_BLOCK) + (MaxClustersPerBlock * sizeof(ADI_FSS_SUPER_BUFFER))));

    // IF (we can malloc this memory from the general heap)
    if ((pCache = _adi_fss_malloc(ADI_FSS_CACHE_GENERAL_HEAP_ID, MemoryRequired)) != NULL) {

        // setup pointers to the cache itself, the first block and the first superbuffer
        pBlock = (ADI_FSS_CACHE_BLOCK *)(pCache + 1);
        pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)(pBlock + BlockCount);

        // initialize the cache structure
        pCache->pFileDescriptor = pFileDescriptor;
        pCache->BlockSize = ClusterSize * MaxClustersPerBlock;
        pCache->FSDPosition = 0;
        pCache->StartPosition = 0;
        pCache->EndPosition = 0;
        pCache->HeapID = HeapID;
        pCache->ClustersPerBlock = MaxClustersPerBlock;
        pCache->ClusterSize = ClusterSize;
        pCache->pHead = NULL;
        pCache->pTail = NULL;
        pCache->DataSemaphoreHandle = NULL;


        // zero fill blocks if the file was opened for create or truncate
        if (pFileDescriptor->mode & (ADI_FSS_MODE_CREATE | ADI_FSS_MODE_TRUNCATE)) {
            pCache->ZeroFillFlag = TRUE;
        } else {
            pCache->ZeroFillFlag = FALSE;
        }

        // save the pointer to the cache in the file descriptor.
        pFileDescriptor->Cache_data_handle = (ADI_FSS_CACHE_DATA_HANDLE) pCache;

        // IF (we can create the semaphore required for transfer completion)
        if (adi_sem_Create (0, &pCache->DataSemaphoreHandle, NULL) == ADI_SEM_RESULT_SUCCESS) {

            /* determine the Data Element Width */
            adi_dev_Control( FSDHandle, ADI_FSS_CMD_GET_DATA_ELEMENT_WIDTH, &DataElementWidth );

            // FOR (each block)
            for (BlocksAllocated = 0; BlocksAllocated < BlockCount; BlocksAllocated++, pBlock++) {

                // IF (we can malloc the memory for the cache data itself from the specified heap)
                if ((pByte = _adi_fss_MallocAligned(HeapID, pCache->BlockSize)) != NULL) {

                    // initialize the cache block structure
                    pBlock->pFirstSuperBuffer = pSuperBuffer;
                    pBlock->State = ADI_FSS_CACHE_STATE_EMPTY;
                    pBlock->DirtyFlag = FALSE;
                    pBlock->RefillFlag = FALSE;
                    pBlock->CurrentOffset = 0;
                    pBlock->HighestDirtyOffset = 0;
                    pBlock->pData = pByte;
                    pBlock->pCache = pCache;
                    pBlock->pNext = NULL;

                    // insert the block into the chain of blocks for the cache
                    if (pCache->pHead == NULL) {
                        pCache->pHead = pBlock;
                        pCache->pTail = pBlock;
                    } else {
                        pCache->pTail->pNext = pBlock;
                        pCache->pTail = pBlock;
                    }

                    // FOR (each superbuffer)
                    for (i = 0; i < MaxClustersPerBlock; i++, pSuperBuffer++) {

                        // populate the superbuffer
                        pSuperBuffer->pBlock = pBlock;
                        pSuperBuffer->LastInProcessFlag = FALSE;

                        // point to the buffer within this superbuffer
                        pBuffer = &pSuperBuffer->Buffer;

                        // point to the data this buffer will control
                        pBuffer->Data = pByte + (i * ClusterSize);

                        // set the element width
                        pBuffer->ElementWidth = DataElementWidth;

                        /* optimize the counts for the requried data width */
                        if (DataElementWidth==1) {
                            pBuffer->ElementCount = ClusterSize;
                        }
                        else if (DataElementWidth==2) {
                            pBuffer->ElementCount = ClusterSize>>1;
                        }
                        else if (DataElementWidth==4) {
                            pBuffer->ElementCount = ClusterSize>>2;
                        }
                        pCache->ElementCount = pBuffer->ElementCount;
                        
                        // the cache only cares about the callback on the last buffer in the chain, however, the
                        // FSD requires that callbacks occur on every buffer.  So within the cache, we need to
                        // ignore every callback except the one for the last buffer hence the following comment:
                        // if the FSD is asynchronous, have the FSD call us back when the last buffer in the chain is processed
                        // (parameter is the buffer address which allows us to stay compliant with the FSS constraint and allows us
                        //  to deterministically calculate the address of the block when given the address of the last buffer)
                        pBuffer->CallbackParameter = pBuffer;

                        // additional info must be the file descriptor
                        pBuffer->pAdditionalInfo = pFileDescriptor;

                        // make pNext point to the buffer in the next superbuffer
                        pBuffer->pNext = &((pSuperBuffer + 1)->Buffer);

                    // ENDFOR
                    }

                    // back up to the last superbuffer in the chain
                    pSuperBuffer--;

                    /* Assign the semaphore handle */
                    pSuperBuffer->SemaphoreHandle = pCache->DataSemaphoreHandle;

                    // terminate the buffer chain
                    pSuperBuffer->Buffer.pNext = NULL;

                    // make note of the address of the last superbuffer in the chain
                    pBlock->pLastSuperBuffer = pSuperBuffer;

                    // point to the first superbuffer for the next block
                    pSuperBuffer++;

                // ELSE
                } else {

                    // indicate that we got a malloc error and break out
                    Result = ADI_FSS_RESULT_NO_MEMORY;
                    break;

                // ENDIF
                }

            // ENDFOR
            }

        // ELSE
        } else {

            // indicate that we couldn't create a semaphore
            Result = ADI_FSS_RESULT_CANT_CREATE_SEMAPHORE;

        // ENDIF
        }

    // ELSE
    } else {

        // indicate that we got a malloc error and break out
        Result = ADI_FSS_RESULT_NO_MEMORY;

    // ENDIF
    }

    // IF (failed)
    if (Result!=ADI_FSS_RESULT_SUCCESS && pCache!=NULL)
    {
        // terminate the cache to free any mallocs that were successful
        adi_fss_CacheTerminate(pFileDescriptor);

    // ENDIF
    }

    // return
    return(Result);
}




/*********************************************************************

    Function:       adi_fss_CacheTerminate

    Description:    Terminates and frees the cache for a file

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
u32 adi_fss_CacheTerminate(                 /* terminates the cache for a file */
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor    /* pointer to the file descriptor */
) {

    ADI_FSS_CACHE       *pCache;    // pointer to the cache
    ADI_FSS_CACHE_BLOCK *pBlock;    // pointer to a cache block
    u32                 Result;     // return code

    // look on the bright side
    Result = ADI_FSS_RESULT_SUCCESS;

    // point to the cache
    pCache = (ADI_FSS_CACHE *) pFileDescriptor->Cache_data_handle;

#if defined(ADI_SSL_DEBUG)

    // validate the cache
    if ((Result = adi_fss_CacheValidateCache(pCache)) != ADI_FSS_RESULT_SUCCESS) {
        return (Result);
    }

#endif

    // WHILE (more blocks in the cache)
    while ((pBlock = pCache->pHead) != NULL) {

        // wait till the block is stable
        while (!Result && (pBlock->State != ADI_FSS_CACHE_STATE_EMPTY) && (pBlock->State != ADI_FSS_CACHE_STATE_FULL)) {
            Result = adi_fss_CacheBlocked(pBlock);
        }
        if (Result) {
            break;
        }

        // clear the refill flag for the block
        pBlock->RefillFlag = FALSE;

        // flush the block
        Result = adi_fss_CacheFlushBlock(pBlock);

        if (Result==ADI_FSS_RESULT_SUCCESS)
        {
            // wait till the block empties
            while (!Result && pBlock->State != ADI_FSS_CACHE_STATE_EMPTY) {
                Result = adi_fss_CacheBlocked(pBlock);
            }
        }

        // take the block out of the list
        pCache->pHead = pBlock->pNext;

        // free the data for the block
        _adi_fss_FreeAligned(pCache->HeapID, pBlock->pData);

    // ENDWHILE
    }

    // NULL out the handle in the file descriptor
    pFileDescriptor->Cache_data_handle = NULL;


    /* delete the semaphore */
    if (pCache->DataSemaphoreHandle) {
        Result = adi_sem_Delete ( pCache->DataSemaphoreHandle );
    }

    // free the cache itself
    _adi_fss_free(ADI_FSS_CACHE_GENERAL_HEAP_ID, pCache);

    return (Result);
}




/*********************************************************************

    Function:       adi_fss_CachePrecharge

    Description:    Charges the cache with data from the file.  This
                    function should be called only once and immediately
                    after the cache has been initialized.  It does the
                    exact same thing as a seek.

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
u32 adi_fss_CachePrecharge(
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor,   /* pointer to the file descriptor */
    u32                     FilePosition        /* file position */
) {

    // alias to the seek function
    return (adi_fss_CacheSeek(pFileDescriptor, FilePosition, TRUE));
}





/*********************************************************************

    Function:       adi_fss_CacheSeek

    Description:    Repositions the cache, flushing and refilling as
                    necessary, to the given file location.  The
                    adi_cache_CachePrecharge() function could be called
                    to do this same function however that function
                    flushes the whole cache regardless.  This function
                    has the smarts to only flush what is necessary.

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
u32 adi_fss_CacheSeek(
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor,   /* pointer to the file descriptor */
    u32                     FilePosition,       /* file position */
    u32                     ForceRefreshFlag    /* flag to force the cache to flush and refill */
) {

    ADI_FSS_CACHE       *pCache;            // pointer to the cache
    ADI_FSS_CACHE_BLOCK *pBlock;            // pointer to a cache block
    ADI_FSS_CACHE_BLOCK *pNext;             // pointer to the next block in the chain
    u32                 BlockStartPosition; // start position for the block
    u32                 CurrentOffset;      // current offset for the block
    u32                 Result;             // return code

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Entering adi_fss_CacheSeek\n");
PUSH_INDENT;
#endif

     // ever the optimist
    Result = ADI_FSS_RESULT_SUCCESS;

    // point to the cache
    pCache = (ADI_FSS_CACHE *) pFileDescriptor->Cache_data_handle;

#if defined(ADI_SSL_DEBUG)
    // validate the cache
    if ((Result = adi_fss_CacheValidateCache(pCache)) != ADI_FSS_RESULT_SUCCESS) {
        return (Result);
    }
#endif

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Seeking to file position %x, currently caching (%x - %x)\n", FilePosition, pCache->StartPosition, pCache->EndPosition);
#endif

    // IF (forcing the cache to flush/refill or the seek position is outside what we have cached)
    if ((ForceRefreshFlag) || (FilePosition < pCache->StartPosition) || (FilePosition > pCache->EndPosition)) {

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Seeking outside of current cache\n");
#endif

        // compute the starting address of the cluster that we're going to seek into
        // this will be the starting address for the first block in the cache
        BlockStartPosition = ((FilePosition/pCache->ClusterSize) * pCache->ClusterSize);
        pCache->StartPosition = BlockStartPosition;

        // compute the current offset for the first block in the cache, all others will start at 0 for their current offset
        CurrentOffset = FilePosition - BlockStartPosition;

        // FOR (each block)
        for (pBlock = pCache->pHead; pBlock; pBlock = pBlock->pNext, BlockStartPosition += pCache->BlockSize, CurrentOffset = 0) {

            // need to wait till the block is stable
            while (!Result && (pBlock->State != ADI_FSS_CACHE_STATE_FULL) && (pBlock->State != ADI_FSS_CACHE_STATE_EMPTY)) {
                Result = adi_fss_CacheBlocked(pBlock);
            }
            if (Result) {
                break;
            }

            // set the new start position and new current offset for the block
            pBlock->NewStartPosition = BlockStartPosition;
            pBlock->NewCurrentOffset = CurrentOffset;

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Setting NewStartPosition of block %x to file position %x\n", pBlock, pBlock->NewStartPosition);
PAD
printf ("Setting NewCurrentOffset of block %x to offset %x\n", pBlock, pBlock->NewCurrentOffset);
#endif

            // call FlushAndRefillBlock(pBlock)
            if ((Result = adi_fss_CacheFlushAndRefillBlock(pBlock)) != ADI_FSS_RESULT_SUCCESS) {
                break;
            }

        // ENDFOR
        }

        // update the cache end position
        pCache->EndPosition = pCache->pTail->NewStartPosition + (pCache->ClusterSize * pCache->ClustersPerBlock) - 1;

    // ELSE
    } else {

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Seeking within current cache\n");
#endif

        // FOR (each block)
        for (pBlock = pCache->pHead; pBlock; pBlock = pNext) {

            // IF (FilePosition is in this block)
            if ((FilePosition >= pBlock->StartPosition) && (FilePosition <= (pBlock->StartPosition + pCache->BlockSize))) {

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Found desired position in block %x\n", pBlock);
#endif

                // set CurrentOffset to the proper value within this block
                pBlock->CurrentOffset = FilePosition - pBlock->StartPosition;

                // nothing else to do
                break;

            // ELSE
            } else {

                // need to wait till the block is stable
                while (!Result && (pBlock->State != ADI_FSS_CACHE_STATE_FULL) && (pBlock->State != ADI_FSS_CACHE_STATE_EMPTY)) {
                    Result = adi_fss_CacheBlocked(pBlock);
                }
                if (Result) {
                    break;
                }

                // point to the next sequential block
                // (need to do this because moving it to the tail will change the block's pNext pointer)
                pNext = pBlock->pNext;

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Moving block %x to tail\n", pBlock);
#endif

                // call MoveToTail(pBlock)
                if ((Result = adi_fss_CacheMoveToTail(pBlock)) != ADI_FSS_RESULT_SUCCESS) {
                    break;
                }

            // ENDIF
            }

        // ENDFOR
        }

    // ENDIF
    }

#ifdef ADI_SSL_FSS_CACHE_DEBUG
POP_INDENT;
PAD
printf ("Exiting\n");
#endif

    // return
    return (Result);
}



/*********************************************************************

    Function:       adi_fss_CacheRead

    Description:    Stores the amount of data requested into the buffer
                    supplied.

                    Note the FSS clamps the size of the request to the
                    amount of data left in the file so the cache does
                    not need to do this.

                    TODO: use 32 byte transfers whenever possible
                    (Note: if doing so, must align the pointer within
                    the block and the caller's pointer to 32 bits before
                    doing the 32 bit transfers)

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
u32 adi_fss_CacheRead(
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor,   /* pointer to the file descriptor */
    u8                      *pData,             /* where to store the data */
    u32                     BytesRequested,     /* number of bytes requested */
    u32                     *pBytesRead         /* location where number of bytes read is stored */
) {

    ADI_FSS_CACHE       *pCache;        // pointer to the cache
    ADI_FSS_CACHE_BLOCK *pBlock;        // pointer to a cache block
    u8                  *pBlockData;    // pointer to the data we're taking from the block
    u32                 BytesFromBlock; // bytes needed from the current block
    u32                 BytesProvided;  // # of bytes provided to the caller
    u32                 Result;         // return code

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Entering adi_fss_CacheRead\n");
PUSH_INDENT;
#endif

    // look on the bright side
    Result = ADI_FSS_RESULT_SUCCESS;

    // point to the cache
    pCache = (ADI_FSS_CACHE *) pFileDescriptor->Cache_data_handle;

#if defined(ADI_SSL_DEBUG)
    // validate the cache
    if ((Result = adi_fss_CacheValidateCache(pCache)) != ADI_FSS_RESULT_SUCCESS) {
        return (Result);
    }
#endif

    // initialize the number of bytes provided
    BytesProvided = 0;

    // WHILE (more data requested)
    while (BytesRequested) {

        // point to the head block
        pBlock = pCache->pHead;

        // we have to block until the block has data
        while (!Result && (pBlock->State != ADI_FSS_CACHE_STATE_FULL) )
        {
            Result = adi_fss_CacheBlocked(pBlock);
        }
        if (Result) {
            break;
        }


#ifdef ADI_SSL_DEBUG
if ((pCache->pHead->State == ADI_FSS_CACHE_STATE_FULL) && (pCache->pTail->State == ADI_FSS_CACHE_STATE_FULL) && (pCache->pHead->StartPosition > pCache->pTail->StartPosition)) {
    Result = Result;
}
#endif

        // compute how many bytes are left in the current block
        BytesFromBlock = pCache->BlockSize - pBlock->CurrentOffset;

        // decide how many bytes we need from the current block
        if (BytesRequested <= BytesFromBlock) {
            BytesFromBlock = BytesRequested;
        }

        // point to where we're going to start copying data from the block
        pBlockData = pBlock->pData + pBlock->CurrentOffset;

        // copy that amount of data from the block into the caller's buffer
        memcpy(pData, pBlockData, BytesFromBlock);
        pData += BytesFromBlock;
        pBlockData += BytesFromBlock;

        // move the CurrentOffset pointer
        pBlock->CurrentOffset += BytesFromBlock;

        // decrement the amount of bytes still requested and increment the number of bytes provided
        BytesRequested -= BytesFromBlock;
        BytesProvided += BytesFromBlock;

        // IF (the CurrentOffset pointer is at the end of the block)
        if (pBlock->CurrentOffset == pCache->BlockSize) {

            // call MoveToTail(pBlock)
            if ((Result = adi_fss_CacheMoveToTail(pBlock)) != ADI_FSS_RESULT_SUCCESS) {
                break;
            }

        // ENDIF
        }

    // ENDWHILE
    }

    // tell the caller the number of bytes actually read
    *pBytesRead = BytesProvided;

#ifdef ADI_SSL_FSS_CACHE_DEBUG
POP_INDENT;
PAD
printf ("Exiting\n");
#endif

    // return
    return (Result);
}




/*********************************************************************

    Function:       adi_fss_CacheWrite

    Description:    Writes the amount of data provided into the cache
                    buffer.  When the user advances past the end of a
                    buffer, the buffer is flushed as necessary.

                    TODO: use 32 byte transfers whenever possible
                    (Note: if doing so, must align the pointer within
                    the block and the caller's pointer to 32 bits before
                    doing the 32 bit transfers)

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
u32 adi_fss_CacheWrite(
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor,   /* pointer to the file descriptor */
    u8                      *pData,             /* where to read the data from */
    u32                     BytesRequested,     /* number of bytes to be written */
    u32                     *pBytesWritten      /* location where number of bytes written is stored */
) {


    ADI_FSS_CACHE       *pCache;        // pointer to the cache
    ADI_FSS_CACHE_BLOCK *pBlock;        // pointer to a cache block
    u8                  *pBlockData;    // pointer to the data we're writing to within the block
    u32                 BytesToBlock;   // bytes needed stored into the current block
    u32                 BytesWritten;   // # of bytes actually written
    u32                 Result;         // return code

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Entering adi_fss_CacheWrite\n");
PUSH_INDENT;
#endif

    // look on the bright side
    Result = ADI_FSS_RESULT_SUCCESS;

    // point to the cache
    pCache = (ADI_FSS_CACHE *) pFileDescriptor->Cache_data_handle;

#if defined(ADI_SSL_DEBUG)
    // validate the cache
    if ((Result = adi_fss_CacheValidateCache(pCache)) != ADI_FSS_RESULT_SUCCESS) {
        return (Result);
    }
#endif

    // initialize the numberof bytes actually written
    BytesWritten = 0;

    // WHILE (more data requested)
    while (BytesRequested) {

        // point to the head block
        pBlock = pCache->pHead;

        // we have to block until the block has data
        while (!Result && (pBlock->State != ADI_FSS_CACHE_STATE_FULL) )
         {
            Result = adi_fss_CacheBlocked(pBlock);
         }

        if (Result) {
            break;
        }

        // compute how many bytes are left in the current block
        BytesToBlock = pCache->BlockSize - pBlock->CurrentOffset;

        // decide how many bytes we need from the current block
        if (BytesRequested <= BytesToBlock) {
            BytesToBlock = BytesRequested;
        }

        // point to where we're going to start copying data from the block
        pBlockData = pBlock->pData + pBlock->CurrentOffset;

        // copy that amount of data from the caller's buffer into the block
        memcpy(pBlockData, pData, BytesToBlock);
        pData += BytesToBlock;
        pBlockData += BytesToBlock;

        // move the CurrentOffset pointer
        pBlock->CurrentOffset += BytesToBlock;

        // keep track of the highest offset within the block that is dirty
        if (pBlock->HighestDirtyOffset < pBlock->CurrentOffset) {
            pBlock->HighestDirtyOffset = pBlock->CurrentOffset;
        }

        // mark that the block is dirty
        pBlock->DirtyFlag = TRUE;

        // decrement the amount of bytes still requested
        BytesRequested -= BytesToBlock;
        BytesWritten += BytesToBlock;

        // IF (the CurrentOffset pointer is at the end of the block)
        if (pBlock->CurrentOffset == pCache->BlockSize) {

            // call MoveToTail(pBlock)
            if ((Result = adi_fss_CacheMoveToTail(pBlock)) != ADI_FSS_RESULT_SUCCESS) {
                break;
            }

        // ENDIF
        }

    // ENDWHILE
    }

    // tell the user how many bytes we wrote
    *pBytesWritten = BytesWritten;

#ifdef ADI_SSL_FSS_CACHE_DEBUG
POP_INDENT;
PAD
printf ("Exiting\n");
#endif

    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_fss_CacheCallback

    Description:    Gets invoked when the FSD has completed processing
                    of a buffer.

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
u32 adi_fss_CacheCallback(
    u32 Event,                  /* event that occurred */
    void *pArg                  /* pointer to the last buffer in the block if buffer complete event */
){

    ADI_FSS_CACHE_BLOCK         *pBlock;            // pointer to the block we're working on
    ADI_FSS_FILE_DESCRIPTOR     *pFileDescriptor;   // pointer to the file descriptor
    ADI_FSS_SUPER_BUFFER        *pSuperBuffer;      // pointer to a superbuffer
    u32                         Result;             // result code


    // look on the bright side
    Result = ADI_FSS_RESULT_SUCCESS;

    // avoid casts (the parameter passed in is a buffer address (FSD constraint!) which is also the superbuffer address
    pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;
    
    // IF (this is a buffer processed event)
    if ((pSuperBuffer->CompletionBitMask&(~0xADF50000)) == 3) 
    {


        // IF (this is the last superbuffer for the block)
        // only for combined blocks 
        if (pSuperBuffer->LastInProcessFlag == TRUE) 
        {

#ifdef ADI_SSL_FSS_CACHE_DEBUG
CallbackCount++;
#endif

            // point to the block that completed
            pBlock = pSuperBuffer->pBlock;

            // IF (current state is filling)
            if (pBlock->State == ADI_FSS_CACHE_STATE_FILLING) {

                // set current state to full
                pBlock->State = ADI_FSS_CACHE_STATE_FULL;

            // ELSE (buffer was emptying so the state is flushing)
            } 
            else if(pBlock->State == ADI_FSS_CACHE_STATE_FLUSHING) {

                // set current state to empty
                pBlock->State = ADI_FSS_CACHE_STATE_EMPTY;

            // ENDIF
            }

            /* Post the Semaphore, which an access request may be pending on */
            adi_sem_Post ( pBlock->pFirstSuperBuffer->SemaphoreHandle );

        // ENDIF
        }

    // ENDIF
    }

    return (Result);
}




/*********************************************************************

    Function:       adi_fss_CacheFlushAndRefillBlock

    Description:    Flushes a block if it's dirty and refills the block
                    with new data.  When refilling, the block is filled
                    with new data starting at the NewStartPosition
                    value.

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
static u32 adi_fss_CacheFlushAndRefillBlock(ADI_FSS_CACHE_BLOCK *pBlock) {

    u32 Result; // return code

    Result = ADI_FSS_RESULT_FAILED;
    if (pBlock->DirtyFlag) {

        // set refill flag
        pBlock->RefillFlag = TRUE;

        // flush the block
        Result = adi_fss_CacheFlushBlock(pBlock);

        while (!Result && (pBlock->State != ADI_FSS_CACHE_STATE_EMPTY) )
         {
            Result = adi_fss_CacheBlocked(pBlock);
         }

    // ELSE
    }
    // clear refill flag
    pBlock->RefillFlag = FALSE;
    if (Result != ADI_FSS_RESULT_MEDIA_FULL)
    {
        // fill the block
        Result = adi_fss_CacheFillBlock(pBlock);
    }
    if (Result == ADI_FSS_RESULT_MEDIA_FULL)
    {
        pBlock->State = ADI_FSS_CACHE_STATE_EMPTY;
    }
    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_fss_CacheFillBlock

    Description:    Fills a block with data from the file

                    TODO: potentially use the FSD offset rather than
                    seeking the FSD all the time.

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
static u32 adi_fss_CacheFillBlock(ADI_FSS_CACHE_BLOCK *pBlock) {

    u32                         Result;                 // return code
    ADI_DEV_DEVICE_HANDLE       FSDHandle;              // handle to the FSD
    ADI_FSS_FILE_DESCRIPTOR     *pFileDescriptor;       // pointer to the file descriptor
    ADI_FSS_SEEK_REQUEST        SeekRequest;            // structure used to tell FSD where to seek
    ADI_FSS_CACHE               *pCache;                // pointer to the cache
    ADI_FSS_SUPER_BUFFER        *pSuperBuffer;          // pointer to a superbuffer
    ADI_FSS_SUPER_BUFFER        *pPreviousSuperBuffer;  // pointer to a superbuffer
    u32                         ClustersPerBlock;       // clusters per block
    u32                         ClusterSize;            // cluster size
    u32                         ClusterStartPosition;   // cluster start position
    u32                         i;                      // counter

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Entering adi_fss_CacheFillBlock\n");
PUSH_INDENT;
#endif

    // look on the bright side
    Result = ADI_FSS_RESULT_SUCCESS;

    // get the handle of the cache, FSD and synchronicity
    pCache = pBlock->pCache;
    ClustersPerBlock = pCache->ClustersPerBlock;
    ClusterSize = pCache->ClusterSize;
    pFileDescriptor = pCache->pFileDescriptor;
    FSDHandle = pFileDescriptor->FSD_device_handle;

    // set the start position for the block
    pBlock->StartPosition = pBlock->NewStartPosition;

    // set the CurrentOffset for the block;
    pBlock->CurrentOffset = pBlock->NewCurrentOffset;

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Filling block %x, from file position %x\n", pBlock, pBlock->StartPosition);
#endif

    // IF (StartPosition is past the end of the file)
    if (pBlock->StartPosition >= pFileDescriptor->fsize) {

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Block is past the end of the file so is being set to full\n");
#endif

        // zero out the data if necessary
        if (pCache->ZeroFillFlag) {
            memset (pBlock->pData, 0, pCache->BlockSize);
        }

        // set current state to full
        pBlock->State = ADI_FSS_CACHE_STATE_FULL;

    // ELSE
    } else {

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Block is within the file so is being read from file\n");
#endif

        // set current state to filling
        pBlock->State = ADI_FSS_CACHE_STATE_FILLING;

        // acquire the FSD
        adi_dev_Control(FSDHandle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL);

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("FSD currently positioned at %x\n", pCache->FSDPosition);
#endif

        // IF (we need to seek the FSD)
        if (pCache->FSDPosition != pBlock->StartPosition) {

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Seeking FSD from current position of %x to %x\n", pCache->FSDPosition, pBlock->StartPosition);
#endif

            // create the seek structure for the FSD
            SeekRequest.pFileDesc = pFileDescriptor;
            SeekRequest.whence = 0;
            SeekRequest.offset = pBlock->StartPosition;

            // IF (we can seek the FSD to seek to where this block is)
            if ((Result = adi_dev_Control(FSDHandle, ADI_FSD_CMD_SEEK_FILE, &SeekRequest)) == ADI_DEV_RESULT_SUCCESS) {

                // update the FSDPosition
                pCache->FSDPosition = pBlock->StartPosition;

            // ENDIF
            }

#ifdef ADI_SSL_FSS_CACHE_DEBUG
if (Result != ADI_FSS_RESULT_SUCCESS) {
    Result = Result;
}
#endif
        // ENDIF
        }

        // IF (no errors)
        if (Result == ADI_FSS_RESULT_SUCCESS) {

            // point to the first superbuffer for the block
            pSuperBuffer = pBlock->pFirstSuperBuffer;

            // assume there will only be one buffer in the chain to adi_dev_Read()
            pSuperBuffer->Buffer.pNext = NULL;
            pSuperBuffer->LastInProcessFlag = TRUE;
            pSuperBuffer->Buffer.ElementCount = pCache->ElementCount;

            // FOR (each remaining cluster in the block)
            for (i = ClustersPerBlock - 1, ClusterStartPosition = pBlock->StartPosition + ClusterSize, pPreviousSuperBuffer = pSuperBuffer, pSuperBuffer++;
                 i;
                 i--, ClusterStartPosition += ClusterSize, pSuperBuffer++, pPreviousSuperBuffer++) {

                // IF (this cluster is within the size of the file)
                if (ClusterStartPosition < pFileDescriptor->fsize) {

                    // make the previous buffer in this cluster link to this buffer
                    pPreviousSuperBuffer->Buffer.pNext = &pSuperBuffer->Buffer;
                    pPreviousSuperBuffer->LastInProcessFlag = FALSE;

                    // assume the current buffer will be the last one in the chain
                    pSuperBuffer->Buffer.pNext = NULL;
                    pSuperBuffer->LastInProcessFlag = TRUE;
                    pSuperBuffer->Buffer.ElementCount = pCache->ElementCount;

                // ELSE
                } else {

                    // stop looking at more buffers as we're past the end of file
                    break;

                // ENDIF
                }

            // ENDFOR
            }

            /* Assign the File Descriptor to the Super Buffer */
            pBlock->pFirstSuperBuffer->pFileDesc = pFileDescriptor;

            /* Add the semaphore to the Super Buffer */
            pBlock->pFirstSuperBuffer->SemaphoreHandle = pCache->DataSemaphoreHandle;

            // update the FSDPosition
            pCache->FSDPosition += (ClustersPerBlock - i) * ClusterSize;

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Calling adi_dev_Read()\n");
devReadCount++;
#endif

            // submit the buffer chain to adi_dev_Read()
            Result = adi_dev_Read(FSDHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&pBlock->pFirstSuperBuffer->Buffer);

        // ENDIF
        }

        // release the FSD
        adi_dev_Control(FSDHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL);

    // ENDIF
    }

#ifdef ADI_SSL_FSS_CACHE_DEBUG
POP_INDENT;
PAD
printf ("Exiting\n");
#endif

    // return
    return (Result);
}


/*********************************************************************

    Function:       adi_fss_CacheFlushBlock

    Description:    Flushes a block if it's dirty

                    TODO: potentially use the FSD offset rather than
                    seeking the FSD all the time.

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
static u32 adi_fss_CacheFlushBlock(ADI_FSS_CACHE_BLOCK *pBlock) {

    u32                     Result;             // return code
    ADI_DEV_DEVICE_HANDLE   FSDHandle;          // handle to the FSD
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor;   // pointer to the file descriptor
    ADI_FSS_SEEK_REQUEST    SeekRequest;        // structure used to tell FSD where to seek
    ADI_FSS_CACHE           *pCache;            // pointer to the cache
    ADI_FSS_SUPER_BUFFER    *pSuperBuffer;          // pointer to a superbuffer
    ADI_FSS_SUPER_BUFFER    *pPreviousSuperBuffer;  // pointer to a superbuffer
    u32                     ClustersPerBlock;       // clusters per block
    u32                     ClusterSize;            // cluster size
    u32                     ClusterStartPosition;   // cluster start position
    u32                     i;                      // counter
    u32                     HighestDirtyPosition;   // highest position within the file that is dirty


#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Entering adi_fss_CacheFlushBlock\n");
PUSH_INDENT;
#endif

    // look on the bright side
    Result = ADI_FSS_RESULT_SUCCESS;
    
#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Flushing block %x, to file position %x, will refill from %x\n", pBlock, pBlock->StartPosition, pBlock->NewStartPosition);
#endif

    // IF (dirty)
    if (pBlock->DirtyFlag) {

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Block is dirty so is being written to file\n");
#endif

        // clear the dirty flag
        pBlock->DirtyFlag = FALSE;

        // compute the highest position within the file that is dirty and reset the dirty offset
        HighestDirtyPosition = pBlock->HighestDirtyOffset + pBlock->StartPosition;
        pBlock->HighestDirtyOffset = 0;

        // get the handle of the cache, FSD and synchronicity
        pCache = pBlock->pCache;
        ClustersPerBlock = pCache->ClustersPerBlock;
        ClusterSize = pCache->ClusterSize;
        pFileDescriptor = pCache->pFileDescriptor;
        FSDHandle = pFileDescriptor->FSD_device_handle;

        // set current state to flushing
        pBlock->State = ADI_FSS_CACHE_STATE_FLUSHING;

        // acquire the FSD
        adi_dev_Control(FSDHandle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL);

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("FSD currently positioned at %x\n", pCache->FSDPosition);
#endif

        // IF (we need to seek the FSD)
        if (pCache->FSDPosition != pBlock->StartPosition) {

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Seeking FSD from current position of %x to %x\n", pCache->FSDPosition, pBlock->StartPosition);
#endif

            // create the seek structure for the FSD
            SeekRequest.pFileDesc = pFileDescriptor;
            SeekRequest.whence = 0;
            SeekRequest.offset = pBlock->StartPosition;

            // IF (we can seek the FSD to seek to where this block is)
            if ((Result = adi_dev_Control(FSDHandle, ADI_FSD_CMD_SEEK_FILE, &SeekRequest)) == ADI_DEV_RESULT_SUCCESS) {

                // update the FSDPosition
                pCache->FSDPosition = pBlock->StartPosition;

            // ENDIF
            }

        // ENDIF
        }

        // IF (no errors)
        if (Result == ADI_FSS_RESULT_SUCCESS) {

            // point to the first superbuffer for the block
            pSuperBuffer = pBlock->pFirstSuperBuffer;

            // assume there will only be one buffer in the chain to adi_dev_Write()
            pSuperBuffer->Buffer.pNext = NULL;
            pSuperBuffer->LastInProcessFlag = TRUE;
            pSuperBuffer->Buffer.ElementCount = pCache->ElementCount;

            // FOR (each remaining cluster in the block)
            for (i = ClustersPerBlock - 1, ClusterStartPosition = pBlock->StartPosition + ClusterSize, pPreviousSuperBuffer = pSuperBuffer, pSuperBuffer++;
                 i;
                 i--, ClusterStartPosition += ClusterSize, pSuperBuffer++, pPreviousSuperBuffer++) {

                // IF (this cluster is dirty)
                if (ClusterStartPosition <= HighestDirtyPosition) {

                    // make the previous buffer in this cluster link to this buffer
                    pPreviousSuperBuffer->Buffer.pNext = &pSuperBuffer->Buffer;
                    pPreviousSuperBuffer->LastInProcessFlag = FALSE;

                    // assume the current buffer will be the last one in the chain
                    pSuperBuffer->Buffer.pNext = NULL;
                    pSuperBuffer->LastInProcessFlag = TRUE;
                    pSuperBuffer->Buffer.ElementCount = pCache->ElementCount;

                // ELSE
                } else {

                    // stop looking at more buffers as we're past the end of file
                    break;

                // ENDIF
                }

            // ENDFOR
            }
            
            /* Assign the File Descriptor to the Super Buffer */
            pBlock->pFirstSuperBuffer->pFileDesc = pFileDescriptor;
            
            /* Add the semaphore to the Super Buffer */
            pBlock->pFirstSuperBuffer->SemaphoreHandle = pCache->DataSemaphoreHandle;

            // update the FSDPosition
            pCache->FSDPosition += (ClustersPerBlock - i) * ClusterSize;

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Calling adi_dev_Write()\n");
devWriteCount++;
#endif

            // submit the buffer chain to adi_dev_Write()
            Result = adi_dev_Write(FSDHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&pBlock->pFirstSuperBuffer->Buffer);

        // ENDIF
        }

        // release the FSD
        adi_dev_Control(FSDHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL);

    // ELSE
    } else {

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Block is not dirty so is not being written to file\n");
#endif
        // set state to empty
        pBlock->State = ADI_FSS_CACHE_STATE_EMPTY;

    // ENDIF
    }

#ifdef ADI_SSL_FSS_CACHE_DEBUG
POP_INDENT;
PAD
printf ("Exiting\n");
#endif

    // return
    return (Result);
}



/*********************************************************************

    Function:       adi_fss_CacheMoveToTail

    Description:    Moves a block to the tail of the list, updates
                    it's starting position.  This function must only
                    be called from CacheRead or CacheWrite, or from
                    CacheSeek if we're seeking within the cache.

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
static u32 adi_fss_CacheMoveToTail(ADI_FSS_CACHE_BLOCK *pBlock) {

    u32             Result;             // return code
    void            *pExitCriticalArg;  // parameter for exit critical
    ADI_FSS_CACHE   *pCache;            // pointer to the cache

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Entering adi_fss_CacheMoveToTail\n");
PUSH_INDENT;
#endif

    // point to the cache
    pCache = pBlock->pCache;

    // set refill flag
    pBlock->RefillFlag = TRUE;

    // protect this region
    pExitCriticalArg = adi_int_EnterCriticalRegion(pCache->pFileDescriptor->pCriticalRegionData);

    // compute the new starting position for the block based on what the tail block is set to
    pBlock->NewStartPosition = pCache->EndPosition + 1;

    // update the cache starting position
    pCache->StartPosition = pBlock->StartPosition + pCache->BlockSize;

    // update the cache ending position
    pCache->EndPosition = pBlock->NewStartPosition + pCache->BlockSize - 1;

    // move this block to the tail position
    pCache->pHead = pBlock->pNext;
    pCache->pTail->pNext = pBlock;
    pCache->pTail = pBlock;
    pBlock->pNext = NULL;

    // unprotect this region
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // reset the new current to 0
    pBlock->NewCurrentOffset = 0;

    // call FlushAndRefillBlock(pBlock)
    Result = adi_fss_CacheFlushAndRefillBlock(pBlock);

#ifdef ADI_SSL_FSS_CACHE_DEBUG
PAD
printf ("Head block = %x, StartPosition = %x, NewStartPosition = %x, State = %x\n", pCache->pHead, pCache->pHead->StartPosition, pCache->pHead->NewStartPosition, pCache->pHead->State);
PAD
printf ("Tail block = %x, StartPosition = %x, NewStartPosition = %x, State = %x\n", pCache->pTail, pCache->pTail->StartPosition, pCache->pTail->NewStartPosition, pCache->pTail->State);
#endif

#ifdef ADI_SSL_FSS_CACHE_DEBUG
POP_INDENT;
PAD
printf ("Exiting\n");
#endif

    // return
    return(Result);
}




/*********************************************************************

    Function:       adi_fss_CacheBlocked

    Description:    Called when the cache is blocked waiting for a
                    block to become either full or empty.

                    This function looks to see if the block is empty
                    and needs refilling.  If it does, it calls the
                    proper function to get refilled.  It then pends
                    on the semaphore for the block.

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
static u32 adi_fss_CacheBlocked(ADI_FSS_CACHE_BLOCK *pBlock) {

    u32 Result;     // return code
    /* gMaxRetryCount is zero if only only one attempt is allowed */
    u32 RetryCount = gMaxRetryCount+1;

    // assume success
    Result = ADI_FSS_RESULT_SUCCESS;
    // IF (no errors)
    if (Result == ADI_FSS_RESULT_SUCCESS) {

        do {
            /* Pend on semaphore */
            Result = adi_sem_Pend ( pBlock->pFirstSuperBuffer->SemaphoreHandle, gSemTimeOutArg );
            RetryCount--;
        } while(Result!=ADI_FSS_RESULT_SUCCESS && RetryCount);

    // ENDIF
    }

    // return
    return (Result);
}



#if defined(ADI_SSL_DEBUG)

/*********************************************************************

    Function:       adi_fss_CacheValidateCache

    Description:    Insures the cache is valid

*********************************************************************/

__ADI_FSS_CACHE_SECTION_CODE
static u32 adi_fss_CacheValidateCache(ADI_FSS_CACHE *pCache) {

    u32 Result; // return code

    // IF (not NULL)
    if (pCache != NULL) {

        // indicate success
        Result = ADI_FSS_RESULT_SUCCESS;

    // ELSE
    } else {

        // indicate failure
        Result = ADI_FSS_RESULT_BAD_CACHE_HANDLE;

    // ENDIF
    }

    // return
    return (Result);
}


#endif

