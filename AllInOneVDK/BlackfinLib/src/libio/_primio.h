/************************************************************************
 *
 * _primio.h: $Revision: 3544 $
 *
 * (c) Copyright 2000-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#ifndef __PRIMIO_H
#define __PRIMIO_H

#include "_stdio.h"

extern void _primIO(void);

typedef struct {
    enum
    {
        PRIM_OPEN = 100,
        PRIM_READ,
        PRIM_WRITE,
        PRIM_CLOSE,
        PRIM_SEEK,
        PRIM_REMOVE,
        PRIM_RENAME
    } op;
    
    int                     fileID;
    int                     flags;      /* file mode or seek mode */
    volatile unsigned char  *buf;       /* data buffer, or file name */
    int                     nDesired;   /* Number of characters to read 
                                           or write */
    int                     nCompleted; /* Number of characters actually 
                                           read or written, or tell/seek result
                                        */
    void*                   more;       /* For future use, also used as 
                                           rename-to name.*/
} PrimIOCB_T;

extern PrimIOCB_T PrimIOCB;

int PRIMITIVE_OPEN(const char* name, int mode);
int PRIMITIVE_READ(int id, unsigned char* buf, size_t n);
int PRIMITIVE_WRITE(int id, int aligned, unsigned char* buf, size_t n);
int PRIMITIVE_WRITE2(int id, unsigned char* buf, size_t n);
int PRIMITIVE_CLOSE(int id);
long PRIMITIVE_SEEK(int fd, long offset, int whence);
int PRIMITIVE_REMOVE(char *name);
int PRIMITIVE_RENAME(char *from, char *to);

#if _FILE_OP_LOCKS
_DECL_MUTEX(__primio_mutex);
#endif

static __inline void
_LOCK_PRIMIO(void)
{
#if _FILE_OP_LOCKS
    _CHECK_AND_ACQUIRE_MUTEX(&__primio_mutex, true);
#else
    /* Rely on stdio-level locking. */
#endif
}

static __inline void
_UNLOCK_PRIMIO(void)
{
#if _FILE_OP_LOCKS
    _RELEASE_MUTEX(&__primio_mutex);
#else
    /* Rely on stdio-level locking. */
#endif
}

#endif /*__PRIMIO_H */
// end of file
