/* *****************************************************
 * FSD Driver File Allocation Table Cache 
 * *****************************************************
 */
 
#include <drivers/fsd/adi_fsd_cache.h>
#include <string.h>


#if 0
#define __ADI_FSD_CACHE_SECTION_CODE    section("adi_fsd_cache_code")
#define __ADI_FSD_CACHE_SECTION_DATA    section("adi_fsd_cache_data")
#else
#define __ADI_FSD_CACHE_SECTION_CODE
#define __ADI_FSD_CACHE_SECTION_DATA
#endif 

#define WRITE 0
#define READ 1


typedef struct PDD_FSD_CACHE_ENTRY {
    u32                         SectorNumber; 
    u32                         SectorCount;
    u32                         Locked; 
    u32                         Modified;
    
    struct PDD_FSD_CACHE_ENTRY  *pNext;
    struct PDD_FSD_CACHE_ENTRY  *pPrev;
    
    struct {
        struct PDD_FSD_CACHE_ENTRY  **pHead;
        struct PDD_FSD_CACHE_ENTRY  *pNext;
    } Hash;
    ADI_FSS_SUPER_BUFFER Buffer;
    void *pDataBuffer;
} PDD_FSD_CACHE_ENTRY;


typedef struct {
    ADI_DEV_DEVICE_HANDLE       PIDHandle;
    u8                          *pData;
    u8                          *pEntries;
    u32                         Flags;
    u32                         DeviceNumber;
    u32                         CacheSize;      /* in blocks */
    u32                         BlockSize;      /* in Sectors */
    u32                         SectorSize;     /* in bytes */
    u32                         DataSize;
    u32                         EntrySize;
    u32                         DataElementWidth;
    u32                         GeneralHeapIndex;
    u32                         CacheHeapIndex;
    ADI_SEM_HANDLE              SemaphoreHandle;
    
    PDD_FSD_CACHE_ENTRY         *pHead;     /* Most recently accessed block */
    
    u32                         HashSize;   /* Number of hash entries */
    u32                         HashMagicNumber; /* Magic number to convert sector into a hash key */
    PDD_FSD_CACHE_ENTRY         **pHash;    /* Hash array, pointer to first 
                                            entry in the hash bin. This value
                                            is dynamically allocated when hash
                                            size is set */
} PDD_FSD_CACHE_DEF;


/* Macros to calculate the block index from the data pointer and to
 * dereference the block entry 
*/
#define GET_INDEX(P) ((u32)((P) - (u8*)pCache->pData) / pCache->DataSize)
#define GET_ENTRY(P) (&((PDD_FSD_CACHE_ENTRY *)pCache->pEntries)[GET_INDEX(P)])


/*********************************************************************
* non-public FSS functions
*********************************************************************/
extern void *_adi_fss_malloc(int id, size_t size );
extern void _adi_fss_free(int id, void *p );
extern void *_adi_fss_MallocAligned( int id, size_t size );
extern void _adi_fss_FreeAligned( int id, void *p );
extern u32 _adi_fss_PIDTransfer( ADI_DEV_DEVICE_HANDLE PIDHandle, ADI_FSS_SUPER_BUFFER *pBuffer, u32 BlockFlag );


#define pdd_GetHash(MagicNumber,Index) \
    ((Index>>(MagicNumber>>24)) & MagicNumber & 0x00FFFFFF)
/*static u32 pdd_GetHash(
    PDD_FSD_CACHE_DEF   *pCache,
    u32                 Index
);*/

static u32 pdd_GetEntry(
    PDD_FSD_CACHE_DEF   *pCache,
    u32                 SectorNumber,
    u32                 SectorCount,
    PDD_FSD_CACHE_ENTRY **pEntry
);

static u32 pdd_Transfer(
    PDD_FSD_CACHE_DEF   *pCache,
    PDD_FSD_CACHE_ENTRY *pEntry,
    u32                 ReadFlag,
    u32                 SectorNumber,
    u32                 SectorCount
);

static void pdd_Callback(
    void                *pHandle,
    u32                 Event,
    void                *pArg
);





__ADI_FSD_CACHE_SECTION_CODE
u32 adi_fsd_cache_Open (
    ADI_DEV_DEVICE_HANDLE   PIDHandle,
    u32                     DeviceNumber,
    ADI_FSD_CACHE_HANDLE    *CacheHandle,
    u32                     Flags,
    u32                     CacheSize,          /* Number of cache blocks */
    u32                     BlockSize,          /* Number of sectors per block */
    u32                     SectorSize,         /* Number of bytes per sectos */
    u32                     DataElementWidth,   /* Data transfer element width */
    u32                     GeneralHeapIndex,
    u32                     CacheHeapIndex,
    u32                     PreCharge,
    u32                     PreChargeSector,
    ADI_SEM_HANDLE          SemaphoreHandle)
{
    u32 Result;
    PDD_FSD_CACHE_DEF *pCache;
    
    Result=ADI_DEV_RESULT_SUCCESS;
    
    pCache=(PDD_FSD_CACHE_DEF *)_adi_fss_malloc(
        GeneralHeapIndex,
        sizeof(*pCache));
    
    if (pCache==NULL)
    {
        Result=ADI_DEV_RESULT_NO_MEMORY;
    }
    
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        pCache->PIDHandle           = PIDHandle;
        pCache->DeviceNumber        = DeviceNumber;
        pCache->Flags               = Flags;
        pCache->DataElementWidth    = DataElementWidth;
        pCache->SectorSize          = SectorSize;
        pCache->CacheSize           = CacheSize;
        pCache->BlockSize           = BlockSize;
        pCache->GeneralHeapIndex    = GeneralHeapIndex;
        pCache->CacheHeapIndex      = CacheHeapIndex;
        
        pCache->pHead               = NULL;
        
        /* Don't use hash by default */
        pCache->pHash               = NULL;
        pCache->HashSize            = 0;
        pCache->HashMagicNumber     = 0;
        
        pCache->SemaphoreHandle     = SemaphoreHandle;
        
        pCache->DataSize            = (pCache->BlockSize * pCache->SectorSize);
        pCache->EntrySize           = sizeof(PDD_FSD_CACHE_ENTRY);

        /* Allocate cache entries block */
        pCache->pEntries = (u8 *)_adi_fss_malloc(
                                            pCache->GeneralHeapIndex,
                                            pCache->CacheSize*sizeof(PDD_FSD_CACHE_ENTRY)
                                           );
        /* Allocate cache data block */
        pCache->pData = (u8 *)_adi_fss_MallocAligned(
                                            pCache->CacheHeapIndex,
                                            pCache->CacheSize*pCache->DataSize
                                           );
        if (pCache->pData==NULL)
        {
            Result==ADI_DEV_RESULT_NO_MEMORY;
        }
    }
    
    if (Result==ADI_DEV_RESULT_SUCCESS) 
    {
        u32 Index;
        PDD_FSD_CACHE_ENTRY *pNew;
        u8 *pData;
        
        
        /* Reset cache data blocks */
        memset(pCache->pEntries,0,pCache->CacheSize*sizeof(PDD_FSD_CACHE_ENTRY));
        memset(pCache->pData,0,pCache->CacheSize*pCache->DataSize);
        
        /* Store cache blocks and in a linked list */
        pData = (u8*)pCache->pData;
        pNew  = (PDD_FSD_CACHE_ENTRY *)pCache->pEntries;
        
        for (Index=0; Index<pCache->CacheSize; Index++, pData += pCache->DataSize, pNew++)
        {           
            /* If this is not the first entry then make the current head's 
                previous point to the new entry
            */
            pNew->pPrev=NULL;
            pNew->pNext=pCache->pHead;
            if (pNew->pNext)
            {
                pNew->pNext->pPrev=pNew;
            }
            
            pCache->pHead=pNew;
            
            /* Assign data pointer to buffer 
            */
            pNew->pDataBuffer = (void*)pData;

            /* Read cache block if pre-charge set */
            if (PreCharge)
            {
                Result=pdd_Transfer( 
                    pCache,
                    pNew,
                    READ,
                    PreChargeSector,
                    pCache->BlockSize);
                
                PreChargeSector+=pCache->BlockSize;
            }
        }
    }
    
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        *CacheHandle=(ADI_FSD_CACHE_HANDLE)pCache;
    } else
    {
        *CacheHandle=NULL;
        adi_fsd_cache_Close(pCache,FALSE);
    }
    
    
    return Result;
}


/********************************************************************

    Function:       adi_fsd_cache_Close

    Description:    Close and deallocate memory from cache, the flush
                    flag is used to indicate if modified block should
                    be flushed before closing

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
u32 adi_fsd_cache_Close(
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u32                     FlushFlag)
{
    u32 Result;
    PDD_FSD_CACHE_DEF *pCache;
    
    
    Result=ADI_FSS_RESULT_SUCCESS;
    pCache=(PDD_FSD_CACHE_DEF *)CacheHandle;
    
    
    if (pCache!=NULL)
    {
        /* Flush the cache */
        if (FlushFlag)
        {
            Result=adi_fsd_cache_Flush(CacheHandle);
        }
        
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            if (pCache->pHash)
            {
                /* Free up hash table memory */
                _adi_fss_free(pCache->GeneralHeapIndex,pCache->pHash);
            }
            
            /* Free up cache block data memory */
            _adi_fss_FreeAligned(pCache->CacheHeapIndex,pCache->pData);
            _adi_fss_free(pCache->GeneralHeapIndex,pCache->pEntries);

            /* Free up cache handle */
            _adi_fss_free(pCache->GeneralHeapIndex,pCache);
        }
    } else
    {
        Result=ADI_DEV_RESULT_FAILED;
    }
    
    return Result;
}


/********************************************************************

    Function:       adi_fsd_cache_CreateHash

    Description:    Use hash for cache lookup, this will speed up 
                    large caches

********************************************************************/
u32 adi_fsd_cache_CreateHash(
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u32                     Size)       /* Number of cache blocks for each hash bin */
{
    u32 Result;
    PDD_FSD_CACHE_DEF *pCache;
    
    
    Result=ADI_DEV_RESULT_SUCCESS;
    pCache=(PDD_FSD_CACHE_DEF *)CacheHandle;
    
    /* First check if a Hash already exists and is a different size, if so 
        then flush the entire cache and delete the existing hash table
    */
    if (Result==ADI_DEV_RESULT_SUCCESS && pCache->pHash && pCache->HashSize!=Size)
    {
        Result=adi_fsd_cache_Flush(CacheHandle);
        
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            _adi_fss_free(
                pCache->GeneralHeapIndex,
                pCache->pHash);
        }
    }
    
    /* We need to create a new hash if the size is not NULL */
    if (Result==ADI_DEV_RESULT_SUCCESS && pCache->pHash==NULL && Size!=0)
    {
        pCache->pHash=(PDD_FSD_CACHE_ENTRY **)_adi_fss_malloc(
            pCache->GeneralHeapIndex,
            Size*sizeof(PDD_FSD_CACHE_ENTRY *));
        
        if (pCache->pHash==NULL)
        {
            Result=ADI_DEV_RESULT_FAILED;
        }
        
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            u32 BinSize,i,j;
            PDD_FSD_CACHE_ENTRY *pEntry;
            
            pCache->HashSize=Size;
            
            BinSize=pCache->CacheSize/pCache->HashSize;
            for (pEntry=pCache->pHead,i=0; i<pCache->HashSize && pEntry; i++)
            {
                /* Initialise hash bin */
                pCache->pHash[i]=NULL;
                
                for (j=0; j<BinSize && pEntry; j++,pEntry=pEntry->pNext)
                {
                    pEntry->Hash.pHead=&pCache->pHash[i];
                    pEntry->Hash.pNext=pCache->pHash[i];
                    pCache->pHash[i]=pEntry;
                }
            }
            
            
            /* Generate MagicNumber which is used to generate the hash key */
            for (i=0; ((1<<i)<pCache->BlockSize); i++); 
            pCache->HashMagicNumber=
                ((i<<24) & 0xFF000000) |
                ((pCache->HashSize-1) & 0x00FFFFFF);
        }
    }
    
    
    return Result;
}


/********************************************************************

    Function:       adi_fsd_cache_Flush

    Description:    Flush whole cache

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
u32 adi_fsd_cache_Flush(
    ADI_FSD_CACHE_HANDLE    CacheHandle)
{
    u32 Result;
    PDD_FSD_CACHE_ENTRY *pEntry;
    
    
    Result=ADI_FSS_RESULT_SUCCESS;
    
    /* Flush each modified sector to media */
    for (pEntry=((PDD_FSD_CACHE_DEF *)CacheHandle)->pHead;
        pEntry!=NULL;
        pEntry=pEntry->pNext)
    {
        if (pEntry->Modified)
        {
            Result=pdd_Transfer( 
                (PDD_FSD_CACHE_DEF *)CacheHandle,
                pEntry,
                WRITE,
                0,
                0);
        }
    }
    
    return Result;
    
}


/********************************************************************

    Function:       adi_fsd_cache_Read

    Description:    Fetch a block from cache, if it isn't in the
                    cache then load in and return address

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
u32 adi_fsd_cache_Read(
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u32                     SectorNumber,
    u32                     SectorCount,
    u8                      **pData)
{
    u32 Result;;
    PDD_FSD_CACHE_DEF *pCache;
    PDD_FSD_CACHE_ENTRY *pEntry;
    
    
    pCache=(PDD_FSD_CACHE_DEF *)CacheHandle;
    Result=pdd_GetEntry(pCache,SectorNumber,SectorCount,&pEntry);
    
    /* If pdd_GetEntry Failed and *pData is not NULL, then flush new entry
        and reload with new sector
    */
    if (Result!=ADI_DEV_RESULT_SUCCESS && pEntry)
    {
        Result=ADI_DEV_RESULT_SUCCESS;
        
        /* Flush old entry to media before overwriting */
        if (pEntry->Modified) 
        {
            Result=pdd_Transfer(
                pCache,
                pEntry,
                WRITE,
                0,
                0);
        }
    }
    if (Result!=ADI_DEV_RESULT_SUCCESS && !pEntry)
    {
        Result=ADI_DEV_RESULT_FAILED;
    }
    
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* Fill cache buffer with new data */
        Result=pdd_Transfer( 
            pCache,
            pEntry,
            READ,
            SectorNumber, 
            SectorCount);
    }
    
    /* Increment locked flag, locked gets decremented by release or write */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        *pData = (u8*)(pEntry->pDataBuffer);
        pEntry->Locked++;
    }
    
    return Result;
}


/********************************************************************

    Function:       adi_fsd_cache_Allocate

    Description:    The same as adi_fsd_cache_Read but the block is
                    not read in.

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
u32 adi_fsd_cache_Allocate(
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u32                     SectorNumber,
    u32                     SectorCount,
    u8                      **pData)
{
    u32 Result;;
    PDD_FSD_CACHE_DEF *pCache;
    PDD_FSD_CACHE_ENTRY *pEntry;
    
    
    pCache=(PDD_FSD_CACHE_DEF *)CacheHandle;
    
    Result=pdd_GetEntry(pCache,SectorNumber,SectorCount,&pEntry);
    
    /* If pdd_GetEntry Failed and *pData is not NULL, then flush new entry
        and reload with new sector
    */
    if (Result!=ADI_DEV_RESULT_SUCCESS && pEntry)
    {
        Result=ADI_DEV_RESULT_SUCCESS;
        
        /* Flush old entry to media before overwriting */
        if (pEntry->Modified) 
        {
            Result=pdd_Transfer(
                pCache,
                pEntry,
                WRITE,
                0,
                0);
        }
    }
    if (Result!=ADI_DEV_RESULT_SUCCESS && !pEntry)
    {
        Result=ADI_DEV_RESULT_FAILED;
    }
    
    
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* Point pData to the pEntry data section */
        *pData = (u8*)(pEntry->pDataBuffer);
        
        /* Update the required field to make it appear that the block was 
            read in
        */
        if (pEntry->SectorNumber!=SectorNumber ||
            pEntry->SectorCount!=SectorCount)
        {
            /* Reset entry arguments */
            pEntry->SectorNumber    = SectorNumber;
            pEntry->SectorCount     = SectorCount;
            pEntry->Locked          = 0;
            pEntry->Modified        = 0;
        }
        
        /* Increment locked flag, locked gets decremented by release or write */
        pEntry->Locked++;
    }
    
    return Result;
}


/********************************************************************

    Function:       adi_fsd_cache_Write

    Description:    Unlock cache sector, signals to the cache that
                    the FSD has finished modifying block until next
                    read.

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
u32 adi_fsd_cache_Write(
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u8                      *pData)
{
    u32 Result;
    PDD_FSD_CACHE_DEF *pCache;
    
    Result=ADI_DEV_RESULT_SUCCESS;
    pCache=(PDD_FSD_CACHE_DEF *)CacheHandle;
    
    if (pData)
    {
        PDD_FSD_CACHE_ENTRY *pEntry;
        
        /* locate beginning of enclosing data block */
        pEntry = GET_ENTRY(pData);
        
        pEntry->Locked--;
        pEntry->Modified=1;
        
        /* If Flush on Write flag is set flush buffer */
        if (pCache->Flags&ADI_FSD_CACHE_FLUSH_ON_WRITE)
        {
            Result=pdd_Transfer(
                pCache,
                pEntry,
                WRITE,
                0,
                0);
        }
    }
    
    return Result;
}


/********************************************************************

    Function:       adi_fsd_cache_Release

    Description:    Unlock a sector without updating the modify flag,
                    this is used when a sector is read but not
                    modified. The caller has to execute either the
                    Write or Release function after a Read

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
u32 adi_fsd_cache_Release(
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u8                      *pData)
{
    PDD_FSD_CACHE_DEF *pCache;
    PDD_FSD_CACHE_ENTRY *pEntry;

    pCache=(PDD_FSD_CACHE_DEF *)CacheHandle;
    
    if (pData)
    {
        /* locate beginning of enclosing data block */
        pEntry = GET_ENTRY(pData);
        pEntry->Locked--;
    }
    
    return ADI_DEV_RESULT_SUCCESS;
}


/********************************************************************

    Function:       adi_fsd_cache_FlushBlock

    Description:    Flush the block pointed to by SectorNumber/
                    SectorCount to PDD if it has been modified

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
u32 adi_fsd_cache_FlushBlock(
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u32                     SectorNumber,
    u32                     SectorCount)
{
    u32 Result;
    PDD_FSD_CACHE_DEF *pCache;
    PDD_FSD_CACHE_ENTRY *pEntry;
    
    
    pCache=(PDD_FSD_CACHE_DEF *)CacheHandle;
    Result=pdd_GetEntry(pCache,SectorNumber,SectorCount,&pEntry);
    
    if (Result==ADI_DEV_RESULT_SUCCESS && pEntry)
    {
        if (pEntry->Modified) 
        {
            Result=pdd_Transfer(
                pCache,
                pEntry,
                WRITE,
                0,
                0);
        }
    }
    
    return Result;
}


/********************************************************************

    Function:       adi_fsd_cache_RemoveBlock

    Description:    Remove block from cache and set block entry to 
                    free. The block is flushed first.

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
u32 adi_fsd_cache_RemoveBlock(
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u32                     SectorNumber,
    u32                     SectorCount)
{
    u32 Result;
    PDD_FSD_CACHE_DEF *pCache;
    PDD_FSD_CACHE_ENTRY *pEntry;
    
    
    pCache=(PDD_FSD_CACHE_DEF *)CacheHandle;
    Result=pdd_GetEntry(pCache,SectorNumber,SectorCount,&pEntry);
    
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* If modified then flush */
        if (pEntry->Modified) 
        {
            Result=pdd_Transfer(
                pCache,
                pEntry,
                WRITE,
                0,
                0);
        }
        
        /* Clear entry info */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            pEntry->SectorNumber    = 0;
            pEntry->SectorCount     = 0;
            pEntry->Locked          = 0;
            pEntry->Modified        = 0;
        }
    }
    else
    {
        Result=ADI_DEV_RESULT_SUCCESS;
    }
    
    return Result;
}


/********************************************************************

    Function:       pdd_GetEntry

    Description:    Find if a sector number/count match is found in
                    the cache, if a match is found load the index
                    and return success

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
static u32 pdd_GetEntry(
    PDD_FSD_CACHE_DEF   *pCache,
    u32                 SectorNumber,
    u32                 SectorCount,
    PDD_FSD_CACHE_ENTRY **pEntry)
{
    u32 Result;
    PDD_FSD_CACHE_ENTRY *pIndex,*pFound;
    
    
    Result=ADI_FSS_RESULT_FAILED;
    pFound=NULL;
    
    if (pCache->pHash)
    {
        u32 Index;
        PDD_FSD_CACHE_ENTRY **pHash,*pFoundLast,*pLast;
        
        
        Index=pdd_GetHash(pCache->HashMagicNumber,SectorNumber);
        pHash=&pCache->pHash[Index];
        
        for (pLast=NULL,pIndex=*pHash;
            pIndex;
            pLast=pIndex,pIndex=pIndex->Hash.pNext)
        {
            if (pIndex->SectorNumber==SectorNumber && pIndex->SectorCount<=SectorCount)
            {
                pFound=pIndex;
                pFoundLast=pLast;
                Result=ADI_DEV_RESULT_SUCCESS;
                break;
            }
        
            if (!pIndex->Locked)
            {
                pFound=pIndex;
                pFoundLast=pLast;
            }
        }
        
        /* If an entry has been found then move it to the head of the list */
        if (pFound && pFoundLast)
        {
            pFoundLast->Hash.pNext=pFound->Hash.pNext;
            pFound->Hash.pNext=*pHash;
            *pHash=pFound;
        }
    }
    else
    {
        /* Look for sector in cache */
        for (pIndex=pCache->pHead;
            pIndex;
            pIndex=pIndex->pNext)
        {
            if (pIndex->SectorNumber==SectorNumber && pIndex->SectorCount<=SectorCount)
            {
                pFound=pIndex;
                Result=ADI_DEV_RESULT_SUCCESS;
                break;
            }
            
            if (!pIndex->Locked)
            {
                pFound=pIndex;
            }
        }
        
        /* If an entry has been found then move it to the head of the list */
        if (pFound && pFound->pPrev!=NULL)
        {
            pFound->pPrev->pNext=pFound->pNext;
            
            if (pFound->pNext)
            {
                pFound->pNext->pPrev=pFound->pPrev;
            }
            
            pFound->pNext=pCache->pHead;
            pFound->pPrev=NULL;
            pCache->pHead=pFound;
        }
    }
    
    *pEntry=pFound;
    
    return Result;
}


/********************************************************************

    Function:       pdd_Transfer

    Description:    Transfer 1 or more sectors to/from a PID

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
static u32 pdd_Transfer(
    PDD_FSD_CACHE_DEF   *pCache,
    PDD_FSD_CACHE_ENTRY *pEntry,
    u32                 ReadFlag,
    u32                 SectorNumber,
    u32                 SectorCount)
{
    u32 Result;
    ADI_FSS_SUPER_BUFFER *pBuffer = &pEntry->Buffer;
    memset(pBuffer, 0, sizeof(ADI_FSS_SUPER_BUFFER));
    
    
    Result=ADI_DEV_RESULT_SUCCESS;
    
    if (pEntry->SectorNumber!=SectorNumber ||
        pEntry->SectorCount!=SectorCount ||
        !ReadFlag)
    {
        if (ReadFlag)
        {
            /* Reset entry arguments */
            pEntry->SectorNumber    = SectorNumber;
            pEntry->SectorCount     = SectorCount;
            pEntry->Locked          = 0;
        }
        
        /* set up LBA request */
        pBuffer->LBARequest.StartSector    = pEntry->SectorNumber;
        pBuffer->LBARequest.SectorCount    = pEntry->SectorCount;
        pBuffer->LBARequest.DeviceNumber   = pCache->DeviceNumber;
        pBuffer->LBARequest.ReadFlag       = ReadFlag;
        pBuffer->LBARequest.pBuffer        = pBuffer;
        
        /* Set up One D Buffer */
        pBuffer->Buffer.Data               = pEntry->pDataBuffer;
        pBuffer->Buffer.ElementWidth       = pCache->DataElementWidth;
        pBuffer->Buffer.ElementCount       = (pBuffer->LBARequest.SectorCount*pCache->SectorSize)/pBuffer->Buffer.ElementWidth;
        pBuffer->Buffer.CallbackParameter  = pBuffer;
        pBuffer->Buffer.pAdditionalInfo    = NULL;
        pBuffer->Buffer.ProcessedFlag      = FALSE;
        pBuffer->Buffer.pNext              = NULL;
        
        /* Assign Semaphore */
        pBuffer->SemaphoreHandle           = pCache->SemaphoreHandle;
        
        /* Assign callback function and handle */
        pBuffer->FSDCallbackFunction       = pdd_Callback;
        pBuffer->FSDCallbackHandle         = (void*)pCache;
        
        /* Assign Other fields to zero/false */
        pBuffer->LastInProcessFlag         = FALSE;
        pBuffer->pBlock                    = NULL;
        pBuffer->pFileDesc                 = NULL;
        if ( ((u32)pBuffer)==0x01fffac0 ) {
            Result = 0;
        }
        
        Result=_adi_fss_PIDTransfer(pCache->PIDHandle,pBuffer,1);
        
        /* If successful then clear the modified flag */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            pEntry->Modified=0;
        }
    }
    
    return Result;
}

static int gCallbackCount=0;
/********************************************************************

    Function:       pdd_Callback

    Description:    The callback to be taken upon buffer completion 
                    and device interrupt callbacks.

********************************************************************/
__ADI_FSD_CACHE_SECTION_CODE
static void pdd_Callback(
    void                *pHandle,
    u32                 Event,
    void                *pArg)
{
    PDD_FSD_CACHE_DEF *pCache;
    ADI_FSS_SUPER_BUFFER *pBuffer;
        
    pCache = (PDD_FSD_CACHE_DEF *)pHandle;
    pBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;

    if ((pBuffer->CompletionBitMask&(~0xADF50000)) == 3)
    {
        /* Upon the completion of the buffer transfer we check to see whether
         * this FSD is responsible for the semaphore. If so, we post it
         */
        if (pBuffer->SemaphoreHandle==pCache->SemaphoreHandle
        )
        {
            adi_sem_Post(pBuffer->SemaphoreHandle);
        }
        
        /* Upon PID Device Interrupt, we can either */
        if (pBuffer->Buffer.pNext)
        {
            /* Submit next LBA request, if requried */
            adi_dev_Control(
                pCache->PIDHandle,
                ADI_PID_CMD_SEND_LBA_REQUEST,
                (void*)&pBuffer->LBARequest);
        }
        else
        {
            /* or, release the PID Lock Semaphore */
            adi_dev_Control(
                pCache->PIDHandle,
                ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE,
                NULL);
        }
    }
}



/* End Of File ***************************************************************/
