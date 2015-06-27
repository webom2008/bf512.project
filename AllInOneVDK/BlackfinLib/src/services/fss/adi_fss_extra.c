/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_fss_extra.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
				Source code for rename, remove & stat functions. 
				
				These functions act as intermediaries to Analog Devices' File System 
				Service API.

*********************************************************************************/

#include <services/services.h>
#include <services/fss/adi_fss.h>
#if !defined(_UNICODE_SUPPORT_)
#include <string.h>
#endif


int rename(const char *oldfname, const char *newfname)
{

    if (!oldfname || !newfname )
        return -1;

    u32 oldfnamelen, newfnamelen;
#if defined(_UNICODE_SUPPORT_)
    ADI_FSS_WCHAR *woldfname = (ADI_FSS_WCHAR*)AsciiToUnicode(oldfname, &oldfnamelen)
    ADI_FSS_WCHAR *wnewfname = (ADI_FSS_WCHAR*)AsciiToUnicode(newfname, &newfnamelen)
#else
    ADI_FSS_WCHAR *woldfname = (ADI_FSS_WCHAR *)oldfname;
    ADI_FSS_WCHAR *wnewfname = (ADI_FSS_WCHAR *)newfname;
    oldfnamelen = strlen(oldfname);
    newfnamelen = strlen(newfname);
#endif

    u32 result = adi_fss_FileRename( woldfname, oldfnamelen, wnewfname, newfnamelen );

#if defined(_UNICODE_SUPPORT_)
    /* free unicode name */
    adi_fss_free(woldfname);
    adi_fss_free(wnewfname);
#endif

    if ( result )
        return -1;


    return 0;
}

int remove(const char *name)
{
    if (!name )
        return -1;

    u32 namelen;
#if defined(_UNICODE_SUPPORT_)
    ADI_FSS_WCHAR *wname = (ADI_FSS_WCHAR*)AsciiToUnicode(name, &namelen)
#else
    ADI_FSS_WCHAR *wname = (ADI_FSS_WCHAR *)name;
    namelen = strlen(name);
#endif

    u32 result = adi_fss_FileRemove( wname, namelen);

#if defined(_UNICODE_SUPPORT_)
    /* free unicode name */
    adi_fss_free(wname);
#endif

    if ( result )
        return -1;

    return 0;
}

int stat(const char *restrict path, struct stat *restrict buf)
{

    u32 pathlen;

#if defined(_UNICODE_SUPPORT_)
    ADI_FSS_WCHAR *wpath = (ADI_FSS_WCHAR*)AsciiToUnicode(path, &pathlen)
#else
    ADI_FSS_WCHAR *wpath = (ADI_FSS_WCHAR*)path;
    pathlen = strlen(path);
#endif
    ADI_FSS_FILE_HANDLE FileHandle;
    u32 result = adi_fss_Stat( wpath, pathlen, buf );

#if defined(_UNICODE_SUPPORT_)
    /* free unicode path */
    adi_fss_free(wpath);
#endif

    /* error exit */
    if (result) {
        return -1;
    }

    return 0;
}
