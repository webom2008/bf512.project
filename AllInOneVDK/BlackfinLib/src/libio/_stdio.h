/************************************************************************
 *
 * _stdio.h: $Revision: 3544 $
 *
 * (c) Copyright 1998-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/* _stdio.h : internal header for standard I/O functions */

#ifndef __SUB_STDIO_DEFINED
#define __SUB_STDIO_DEFINED

#include <stdio.h>
#include <stdarg.h>
#include "xsync.h"

enum {
    M_OPENR =           0x0001,  /* opened for reading           */
    M_OPENW =           0x0002,  /* opened for writing           */
    M_OPENA =           0x0004,  /* opened for append            */

    M_TRUNCATE =        0x0008,
    M_CREATE =          0x0010,
    M_BINARY =          0x0020,
    M_FREE_BUFFER =     0x0040,  /* free buf on closing          */
    M_FREE_FILE =       0x0080,  /* free the filename on closing */

    M_EOF =             0x0100,  /* EOF encountered on physical file */
    M_ERROR =           0x0200,  /* Error encountered            */
    M_LINE_BUFFERING =  0x0400,  /* line buffering in use        */
    M_NO_BUFFERING =    0x0800,  /* no buffer in use             */
    M_LAST_READ =       0x01000, /* last operation was a read    */
    M_LAST_WRITE =      0x02000, /* last operation was a write   */
    M_STRLEN_PROVIDED = 0x08000  /* strlen provided for optimized 
                                 ** symbol manager */
};

/*  Modes for use with seek command */
enum {
    M_SEEK_SET =        0x0001,
    M_SEEK_CUR =        0x0002,
    M_SEEK_END =        0x0004
};

#define M_OPENRW           (M_OPENR | M_OPENW)

int _fputc(int c, FILE *str);
int _fflush(FILE *str);
int _fclose(FILE *str);

FILE *_doopenfile(const char *name, const char *mods, FILE *str);
int _fillreadbuf(FILE *);
int _flushwritebuf(FILE *);
int _fflush_all(void);
long _doseek(FILE *str, long offset, int whence);

/* Data Structure to perform output using snprintf and vsnprintf */
typedef struct {
    char  *buf;
    size_t n;
    size_t pos;
} FILE_buff;

/*
   Some functions are sensitive to the size of type double. The #define's
   below are used to modify the name of its entry point to include the
   size of double that they support
*/

#if defined(__DOUBLES_ARE_FLOATS__)
#define _doprnt _doprnt_32
#define _doscan _doscan_32
#else
#define _doprnt _doprnt_64
#define _doscan _doscan_64
#endif

typedef void *FargT;
typedef int (*FuncT)(FargT, char);

extern int
_doprnt(const char* fmtString, va_list args, FuncT func, FargT farg);

#define DO_GETC   (-2)

#define READ_LIMIT  (-3)      /* Indicates width-limit reached */

typedef void* ScanContextT;
typedef int (*ScanFnT)(int ch, ScanContextT context);

extern int
_doscan(const char* fmtString, va_list args, ScanFnT fn, ScanContextT farg);

/*
   Configuration Macros
*/

#include "_wordsize.h"

#define BITS_PER_BYTE     8
#define BYTE_MASK         ((1 << BITS_PER_BYTE) - 1)
#define BYTES_PER_WORD    (BITS_PER_WORD/BITS_PER_BYTE)
     /* BYTES_PER_WORD is set to the number of 8-bit bytes that can fit
     ** into the smallest addressible memory location on the DSP hardware
     */

#if (BYTES_PER_WORD==1)

typedef unsigned char *byte_addr_t;

#define BYTE_ADDR(ptr)           (ptr)
#define WORD_ADDR(bAddr)         (bAddr)

#else

typedef unsigned long byte_addr_t;

#define BYTE_ADDR(ptr)                                                \
            ((unsigned long) BYTES_PER_WORD * (unsigned) (ptr))
#define WORD_ADDR(bAddr)                                              \
            ((int*) ((unsigned int)((((unsigned long)bAddr) >>        \
               (BYTES_PER_WORD/2)))))
#define FETCH_WORD(nxt)                                               \
            (*WORD_ADDR(nxt))
#define BYTE_SHIFT(ctr)                                               \
            (((ctr) * BITS_PER_BYTE) % BITS_PER_WORD)
#define FETCH_BYTE(wrd,ctr)                                           \
            (((wrd) >> BYTE_SHIFT(ctr)) & BYTE_MASK)
#define DEPOSIT_BYTE(wrd,ctr,byt)                                     \
            ((wrd)=(wrd) & ((1 << BYTE_SHIFT(ctr)) - 1)|(byt)<<BYTE_SHIFT(ctr))

#endif


/* The stream table lock (for protecting the _Files array) */

_DECL_MUTEX(_Files_mutex)

static __inline void
_LOCK_FILES(void)
{ 
    _CHECK_AND_ACQUIRE_MUTEX(&_Files_mutex, true);
}

static __inline void
_UNLOCK_FILES(void)
{
    _RELEASE_MUTEX(&_Files_mutex);
}


/* Stream locks */

static __inline void
_LOCK_FILE(FILE *str)
{
#if _FILE_OP_LOCKS
    _CHECK_AND_ACQUIRE_MUTEX((MUTEX *)&str->_Mutex, true);
#else
    _LOCK_FILES();
#endif
}

static __inline void
_UNLOCK_FILE(FILE *str)
{
#if _FILE_OP_LOCKS
    _RELEASE_MUTEX((MUTEX *)&str->_Mutex);
#else
    _UNLOCK_FILES();
#endif
}


#endif /* !defined(__SUB_STDIO_DEFINED) */

// end of file
