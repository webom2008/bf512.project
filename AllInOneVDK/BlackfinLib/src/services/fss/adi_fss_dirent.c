/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_fss_dirent.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
				Source code for directory navigation and access functions, the API
				of which conforms to the Open Group Base Specifications Issue 6,
				IEEE Std 1003.1, 2004 Edition. 
				
				These functions act as intermediaries to Analog Devices' File System 
				Service API.

*********************************************************************************/

#include <services/fss/adi_fss.h>
#if !defined(_UNICODE_SUPPORT_)
#include <string.h>
#endif


/*********************************************************************

	Function:		opendir
	
	Description:	Opens a directory stream for reading
	
*********************************************************************/

DIR *
opendir(const char *name)
{
    u32 namelen;
#if defined(_UNICODE_SUPPORT_)
    ADI_FSS_WCHAR *wname = (ADI_FSS_WCHAR*)AsciiToUnicode(name, &namelen)
#else
    ADI_FSS_WCHAR *wname = (ADI_FSS_WCHAR *)name;
    namelen = strlen(name);
#endif

    ADI_FSS_DIR_HANDLE DirHandle;
    u32 result = adi_fss_DirOpen( wname, namelen, &DirHandle );

#if defined(_UNICODE_SUPPORT_)
    /* free unicode name */
    adi_fss_free(wname);
#endif

    if ( result )
        return NULL;

    return (DIR*)DirHandle;
}

/*********************************************************************

	Function:		closedir
	
	Description:	Closes a directory stream
	
*********************************************************************/

int
closedir(DIR *DirHandle)
{
    if (!DirHandle)
    {
        return -1;
	}
	
    u32 result = adi_fss_DirClose( (ADI_FSS_DIR_HANDLE)DirHandle );

    if (result) 
        return -1;

    return 0;
}

/*********************************************************************

	Function:		readdir
	
	Description:	Reads the next directory entry from the directory stream
	
*********************************************************************/

struct dirent *
readdir(DIR *DirHandle)
{
    struct dirent *pEntry;
    if (!DirHandle)
    {
		/* set errno to EBADF */
        return NULL;
	}

    u32 result = adi_fss_DirRead( (ADI_FSS_DIR_HANDLE)DirHandle,  (ADI_FSS_DIR_ENTRY **)&pEntry);

    if ( result )
        return NULL;

    return pEntry;

}

/*********************************************************************

	Function:		readdir_r
	
	Description:	Reads the next directory entry from the directory 
                    stream. Copies current entry to given location 
                    (entry)
	
*********************************************************************/

int            
readdir_r(DIR *DirHandle, struct dirent *entry, struct dirent **result)
{
    /* Perfome nornal readdir operation */
    struct dirent *pDirEntry = readdir(DirHandle);
    if (!pDirEntry) {
        *result = NULL;
        return -1;
    }

    /* Copy contents to entry argument */
    entry->d_ino = pDirEntry->d_ino;
    entry->d_namlen = pDirEntry->d_namlen;
    strncpy(entry->d_name, pDirEntry->d_name,pDirEntry->d_namlen);
    entry->d_name[entry->d_namlen] = '\0';
    entry->d_off = pDirEntry->d_off;
    entry->d_type = pDirEntry->d_type;
    entry->d_size = pDirEntry->d_size;
    entry->DateCreated = pDirEntry->DateCreated;
    entry->DateLastAccess = pDirEntry->DateLastAccess;
    entry->DateModified = pDirEntry->DateModified;

    /* Assign result pointer */
    *result = entry;

    return 0;
}

/*********************************************************************

	Function:		rewinddir
	
	Description:	Rewinds to start of directory stream
	
*********************************************************************/

void           
rewinddir(DIR *DirHandle)
{
    adi_fss_DirRewind((ADI_FSS_DIR_HANDLE)DirHandle);
    
}

/*********************************************************************

	Function:		seekdir
	
	Description:	Seeks to the given point in the directory stream
	
*********************************************************************/

void           
seekdir(DIR *DirHandle, long pos)
{
    /* All that is necessary is to pass request to FSS */
    adi_fss_DirSeek((ADI_FSS_DIR_HANDLE)DirHandle, (u32)pos);
    
}

/*********************************************************************

	Function:		telldir
	
	Description:	Reports the current location within the directory 
                    stream.
	
*********************************************************************/

long           
telldir(DIR *DirHandle)
{
	u32 tellpos;

    /* All that is necessary is to pass request to FSS */
    adi_fss_DirTell((ADI_FSS_DIR_HANDLE)DirHandle, &tellpos);

    /* and cast the resulting position as long */
	return (long)tellpos;
}

/*********************************************************************

	Function:		chdir
	
	Description:	Changes the location of the Current Working Directory
	
*********************************************************************/

int 
chdir(const char *path)
{
    u32 pathlen;
#if defined(_UNICODE_SUPPORT_)
    /* Convert name to Unicode array */
    ADI_FSS_WCHAR *wpath = (u16*)AsciiToUnicode(path, &pathlen)
#else
    /* Otherwise simply recast the path pointer */
    ADI_FSS_WCHAR *wpath = (ADI_FSS_WCHAR *)path;
    if (path)
        pathlen = strlen(path);
    else pathlen = 0;
#endif

    /* pass request to FSS */
    u32 result = adi_fss_DirChange (wpath, pathlen);

#if defined(_UNICODE_SUPPORT_)
    /* free unicode name */
    adi_fss_free(wpath);
#endif

    if ( result )
        return -1;

    return 0;
}

/*********************************************************************

	Function:		getcwd
	
	Description:	Requests the location of the Current Working Directory
	
*********************************************************************/

char *getcwd(char *buf, size_t size)
{
    u32 pathlen = (u32)size;
#if defined(_UNICODE_SUPPORT_)
    ADI_FSS_WCHAR path[size];

    /* pass request to FSS */
    u32 result = adi_fss_GetCurrentDir(path, &pathlen);
#else
    /* pass request to FSS */
    u32 result = adi_fss_GetCurrentDir((ADI_FSS_WCHAR*)buf, &pathlen);
#endif

    if (result)
        return NULL;

#if defined(_UNICODE_SUPPORT_)
    /* Convert resulting Unicode path to ASCII */
    UnicodeToAscii(buf,path,pathlen);
#endif

    return buf;
}

/*********************************************************************

	Function:		mkdir
	
	Description:	Create a new directory, mode is for compatibility
	                but is ignored
	
*********************************************************************/

int 
mkdir(const char *path, mode_t mode)
{
    u32 pathlen;
#if defined(_UNICODE_SUPPORT_)
    /* Convert name to Unicode array */
    ADI_FSS_WCHAR *wpath = (u16*)AsciiToUnicode(path, &pathlen)
#else
    /* Otherwise simply recast the path pointer */
    ADI_FSS_WCHAR *wpath = (ADI_FSS_WCHAR *)path;
    if (path)
        pathlen = strlen(path);
    else pathlen = 0;
#endif

    /* pass request to FSS */
    u32 result = adi_fss_DirCreate (wpath, pathlen, mode);

#if defined(_UNICODE_SUPPORT_)
    /* free unicode name */
    adi_fss_free(wpath);
#endif

    if ( result )
        return -1;

    return 0;
}

/*********************************************************************

	Function:		rmdir
	
	Description:	Create a new directory
	
*********************************************************************/

int 
rmdir(const char *path)
{
    u32 pathlen;
#if defined(_UNICODE_SUPPORT_)
    /* Convert name to Unicode array */
    ADI_FSS_WCHAR *wpath = (u16*)AsciiToUnicode(path, &pathlen)
#else
    /* Otherwise simply recast the path pointer */
    ADI_FSS_WCHAR *wpath = (ADI_FSS_WCHAR *)path;
    if (path)
        pathlen = strlen(path);
    else pathlen = 0;
#endif

    /* pass request to FSS */
    u32 result = adi_fss_DirRemove (wpath, pathlen);

#if defined(_UNICODE_SUPPORT_)
    /* free unicode name */
    adi_fss_free(wpath);
#endif

    if ( result )
        return -1;

    return 0;
}




