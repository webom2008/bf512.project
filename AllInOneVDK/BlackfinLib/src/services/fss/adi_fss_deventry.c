/*********************************************************************************

Copyright(c) 2006 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_fss_deventry.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
				Source code for libio Device Driver Implemenation of the 
				File System Service (FSS).

*********************************************************************************/

/*********************************************************************
*      CRT Dewvice I/O entry points
*********************************************************************/

#include <services/fss/adi_fss.h>
#include <device_int.h>
#if !defined(_UNICODE_SUPPORT_)
#include <string.h>
#endif

static int  init(struct DevEntry *dev);
static int  fss_open(const char *name, int mode);
static int  fss_close(int fd);
static int  fss_write(int fd, unsigned char *buf, int size);
static int  fss_read(int fd, unsigned char *buf, int size);
static long fss_seek(int fd, long offset, int whence);


/*********************************************************************
*      DevEntry structure is exported to devtab.c
*********************************************************************/

struct DevEntry adi_fss_entry = {
	ADI_FSS_DEVID,		    /* Device ID                */
	NULL,				    /* Data area not used       */       
	init,				    /* init function point      */
	fss_open,				    /* open function pointer    */
	fss_close,				    /* close function pointer   */
	fss_write,				    /* write function pointer   */
	fss_read,				    /* read function pointer    */
	fss_seek,				    /* seek function pointer    */
	(int)dev_not_claimed,   /* stdin defined elsewhere  */
	(int)dev_not_claimed,	/* stdout defined elsewhere */
	(int)dev_not_claimed,	/* stderr defined elsewhere */
};


/*********************************************************************

	Function:		init

	Description:	Initialization of Device Driver. FSS is initialized
	                elsewhere (in init_ssl), so all we do here is make
	                it the default I/O device 

*********************************************************************/

static int init(struct DevEntry *dev)
{
	set_default_io_device(dev->DeviceID);
	return 1;
}


/*********************************************************************

	Function:		open

	Description:	Opens the specified file for the given mode of access

*********************************************************************/
static int 
fss_open(const char *name, int mode)
{
    // Open file
    u32 namelen;
#if defined(_UNICODE_SUPPORT_)
    ADI_FSS_WCHAR *wname = (ADI_FSS_WCHAR*)AsciiToUnicode(name, &namelen)
#else
    ADI_FSS_WCHAR *wname = (ADI_FSS_WCHAR*)name;
    namelen = strlen(name);
#endif
    ADI_FSS_FILE_HANDLE FileHandle;
    u32 result = adi_fss_FileOpen ( wname, namelen, (u32)mode, &FileHandle );

#if defined(_UNICODE_SUPPORT_)
    // free unicode name
    adi_fss_free(wname);
#endif
    // error exit
    if (result) {
        return -1;
	}

    // return FileHandle
    return (int)FileHandle;
}

static int 
fss_close(int fd)
{
    if ( adi_fss_FileClose( (ADI_FSS_FILE_HANDLE)fd ) )
        return -1;
    
    return 0;
}

/*********************************************************************

	Function:		write

	Description:	Write a number of bytes to the current file position 
	                for the given file.

*********************************************************************/

static int 
fss_write(int fd, unsigned char *buf, int size)
{
    u32 bytes_transfered;
    if ( adi_fss_FileWrite( (ADI_FSS_FILE_HANDLE)fd, (u8*)buf, (u32)size, &bytes_transfered ) )
        return -1;

    return (int)bytes_transfered;
}

/*********************************************************************

	Function:		read

	Description:	Read a number of bytes from the current file position 
	                for the given file.


*********************************************************************/

static int 
fss_read(int fd, unsigned char *buf, int size)
{
    u32 bytes_transfered;
    u32 result;
    
    result = adi_fss_FileRead( (ADI_FSS_FILE_HANDLE)fd, (u8*)buf, (u32)size, &bytes_transfered );
    if ( result == ADI_FSS_RESULT_EOF )
        return 0;
    else if (result != ADI_FSS_RESULT_SUCCESS)
        return -1;
        
    return (int)bytes_transfered;
}

/*********************************************************************

	Function:		seek

	Description:	Seeks to the specified location within the given file

*********************************************************************/

static long 
fss_seek(int fd, long offset, int whence)
{
	long curpos;
	
    if ( adi_fss_FileSeek( (ADI_FSS_FILE_HANDLE)fd, (s32)offset, (u32)whence, (u32*)&curpos ) )
        return -1;
    
    return curpos;
}





