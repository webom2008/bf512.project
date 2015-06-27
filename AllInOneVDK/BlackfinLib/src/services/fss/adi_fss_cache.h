/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_fss_cache.h,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This file contains the includes for the file cache

*********************************************************************************/

#ifndef __ADI_FSS_CACHE_H__         /* avoid recursive includes */
#define __ADI_FSS_CACHE_H__



u32 adi_fss_CacheInit(                      /* initializes a cache for a file */
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor,   /* pointer to the file descriptor */
    s32                     HeapID,             /* Heap ID for mallocs */
    u32                     BlockCount,         /* number of blocks in the cache */
    u32                     ClustersPerBlock,   /* number of clusters per block */
    u32                     ClusterSize,        /* cluster size (in bytes) */
    u32						SemTimeOutArg,		/* Argument for adi_sem_Pend timeout arg */
    u32						MaxRetryCount		/* Max number of retry attempts (0 means, fail after 1st attempt) */
);

u32 adi_fss_CacheTerminate(                 /* terminates the cache for a file */
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor    /* pointer to the file descriptor */
);

    	
   
u32 adi_fss_CachePrecharge(                 /* charges the cache with data */
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor,   /* pointer to the file descriptor */
    u32                     FilePosition        /* file position */
);

u32 adi_fss_CacheSeek(                      /* seeks within the file */
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor,   /* pointer to the file descriptor */
    u32                     FilePosition,       /* file position */
    u32                     ForceRefreshFlag    /* flag to force the cache to flush and refill */
);
    
u32 adi_fss_CacheRead(                      /* reads data from the file */
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor,   /* pointer to the file descriptor */
    u8                      *pData,             /* where to store the data */
    u32                     BytesRequested,     /* number of bytes requested */
    u32                     *pBytesRead         /* location where number of bytes read is stored */
);
u32 adi_fss_CacheWrite(                     /* writes data to the file */
    ADI_FSS_FILE_DESCRIPTOR *pFileDescriptor,   /* pointer to the file descriptor */
    u8                      *pData,             /* where to read the data from */
    u32                     BytesRequested,     /* number of bytes to be written */
    u32                     *pBytesWritten      /* location where number of bytes written is stored */
);


u32 adi_fss_CacheCallback(   /* processes callback events for the cache */
    u32 Event,                  /* event that occurred */
    void *pArg                  /* pointer to the a buffer for the block */
);


#endif          /* end recursive includes */
