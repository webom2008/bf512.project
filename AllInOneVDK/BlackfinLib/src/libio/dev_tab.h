/************************************************************************
 *
 * dev_tab.h: $Revision: 3543 $
 *
 * (c) Copyright 2003-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#ifndef _DEV_TAB_H
#define _DEV_TAB_H

/* 

@section Device Handler (The Device Driver Manager)

This file contains wrapper functions which invoke the appropriate
routines associated with the correct peripheral dictated by the file
descriptor.  

After the programmer invokes a system call (e.g., open(), close(), read() ),
the user library (librt_fileio.dlb) causes a software exception and passes a
system call code to the event handler. After the exception is raised, the
processor goes into supervisor mode where the event handler determines which
routine below to call depending on the system call code.

This device handler uses several tables for 'bookeeping' purposes.  The Device
Driver table (DevDrvTable[]) is an array which contain pointers to device
entry structs.  Each peripheral has a unique device entry struct associated
with its particular set of system routines.  The device driver table contain
pointers to each of these structs.  For example, DevDrvTable[0] contains the
pointer to the device entry struct of device0, so on & so forth.
NOTE ** The index of the element in this table is used as the device
ID number ** END NOTE

Another table which is used the Device Entry table.  Each element on this table 
(DevIOtable[]) is a struct which contains a Device ID (index of DevDrvTable[])
and a file descriptor (which is returned from the target library routine).
A space in the table is used for EACH file that is opened by any peripheral.
NOTE ** The index of this table (DevIOtable[]) is the one returned for the
end user/programmer to use in their code ** END NOTE

The initialization routine for the Device Entry table sets the elements in each 
structure in Device Entry table to -1, which is the designated value for
"invalid" or "error". The Device Driver table is set up with the first
element pointing to a File IO device entry struct (because that's the only one
we have right now) and the others are set to null pointers. The routine will
be initiated according depending what peripherals are attached and if
libraries are available for them.

The Device Entry table will somehow automatically be initialized with the
first three elements for stdin, stdout & stderr, respectively. As more files
are opened, spaces are taken up sequentially. Then, as files are closed,
the first available open slot on the table will be used for the next open file.  


@example
@display
  These indices get ret. to
  the user as the file des.  
       |                     
       +-> [0]   [1]   [2]        [4]   [5] ...    [MAXFD-1]
         +-----+-----+-----+-----+-----+-----+-->  >-+----+
DeviceID |  0  |  0  |  0  |  -1 | -1  | -1  |       | -1 |      Device Entry
         +-----+-----+-----+-----+-----+-----+-->  >-+----+	 Table
FileID   |  0  |  1  |  2  |  ^  | -1  |  -1 |       | -1 |
         +-----+-----+-----+--|--+-----+-----+-->  >-+----+
         stdin stdout stderr  |
                              +--- file descriptor returned 
                                   from open() of the peripheral
                                   associated with the Device
                                   indicated by the Device ID
@end display
@end example

@example
@display
  These indices are used
  as the Dev ID's stored
  in the Dev. Entry Table.
     |
     +--->[0]   [1]   [2]   [3]   [4]   [5] ...   [MAXDEV-1]
        +-----+-----+-----+-----+-----+-----+--/ /-+---+
        |  *  |  *  |  *  |  0  |  0  |  0  |      |   |      Device Driver 
        +--|--+--|--+--|--+-----+-----+-----+--/ /-+---+	Table
           |     |     +----------------+
          \/     +---+                  |
        File IO Lib. |    Dev 1. Lib    |   Dev 2. Lib
        +---------+  |    +---------+   |   +---------+
        |  Device |  +--->|  Device |   +-> |  Device |
        |  Entry  |       |  Entry  |       |  Entry  |   ...  etc.
        |  Struct |       |  Struct |       |  Struct |
        +---------+       +---------+       +---------+

        /         |
       /          |
      /           |
  
       +--------+      
       | Dev. ID|
       +--------+
       |    *-----> void
       +--------+
       |    *-----> fileio_open()
       +--------+
       |    *-----> fileio_write()
       +--------+
       |    *-----> fileio_etc()
       +--------+

@end display
@end example

*/

#include <device.h>
#include <device_int.h>
#include "xsync.h"

extern sDevTab      DeviceIOtable[MAXFD];/* table of structs consisting of a
                                            file descriptor and a pointer to a
                                            DevEntry struct.                 */

extern DevEntry_t   DevDrvTable[MAXDEV]; /* table of Dev Entry Ptrs.  One for
                                            each device                      */

#define DEVFD(fd)   DeviceIOtable[fd].DevFileDes
#define DEVDRV(fd)  DeviceIOtable[fd].DeviceID
#define FDOK(fd)    ((fd) >= 0 && (fd) < MAXFD)

#define STDIN_FD  0
#define STDOUT_FD 1
#define STDERR_FD 2


/* Device tables initialisation */

extern bool __devtabs_initialized;
void __init_devtabs(void);

static __inline void
_INIT_DEVTABS(void)
{
    if (!__devtabs_initialized)
        __init_devtabs();
}


/* Device driver initialisation */

bool __init_devdrv(DevEntry *dp, int i);


#if defined(_ADI_THREADS) && _FILE_OP_LOCKS

/* Device tables lock */

_DECL_MUTEX(__devtabs_mutex)

static __inline void
_LOCK_DEVTABS(void)
{
    _ACQUIRE_MUTEX(&__devtabs_mutex);
}

static __inline void
_UNLOCK_DEVTABS(void)
{
    _RELEASE_MUTEX(&__devtabs_mutex);
}


/* Device driver locks */

_DECL_MUTEX(__devdrv_mutexs[MAXFD])

static __inline void
_LOCK_DEVDRV(int drv)
{ 
    if (__devdrv_mutexs[drv])
        _ACQUIRE_MUTEX(&__devdrv_mutexs[drv]);
}

static __inline void
_UNLOCK_DEVDRV(int drv)
{
    if (__devdrv_mutexs[drv])
        _RELEASE_MUTEX(&__devdrv_mutexs[drv]);
}

#else /* defined(_ADI_THREADS) && _FILE_OP_LOCKS */

#define _LOCK_DEVTABS()
#define _UNLOCK_DEVTABS()
#define _LOCK_DEVDRV(drv)
#define _UNLOCK_DEVDRV(drv)

#endif /* defined(_ADI_THREADS) && _FILE_OP_LOCKS */

#endif /* _DEV_TAB_H */
