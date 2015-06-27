/************************************************************************
 *
 * fclose.c : $Revision: 3543 $
 *
 * (c) Copyright 1998-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=fclose")
#pragma file_attr(  "libFunc=_fclose")
#pragma file_attr(  "libFunc=__fclose")
#pragma file_attr(  "libFunc=freopen")
#pragma file_attr(  "libFunc=_freopen")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <stdlib.h>

#include "_stdio.h"
#include "device_int.h"

/*wrapper function*/
int fclose(FILE *str)
{
  _LOCK_FILES();
  int tmp = _fclose(str);
  _UNLOCK_FILES();
  return tmp;
}

/* actual implementation */
int _fclose(FILE *str)
{
  int status;

  if (str == NULL)
    return 0;

  status = _fflush(str);

  if (str->_Mode & M_FREE_BUFFER)
    free(str->bufadr);

  str->bufadr = NULL;
  str->_Buf = (byte_addr_t) NULL;
  if (str->fileID >= 0)
    if (_dev_close(str->fileID))
      status = EOF;

  if (str->_Mode & M_FREE_FILE)
  {
    size_t i;
    for (i = 0; i < FOPEN_MAX; i++)
      if (_Files[i] == str)
      {
        _Files[i] = NULL;
        break;
      }
    str->_Mode = 0;
    str->_Next = str->_Rend = str->_Wend = BYTE_ADDR(&str->onechar);
    str->nback = 0;
#if _FILE_OP_LOCKS
    _DEINIT_MUTEX((MUTEX *)&str->_Mutex);
#endif
    free(str); 
  } else {
    str->_Mode = 0;
    str->_Next = str->_Rend = str->_Wend = BYTE_ADDR(&str->onechar);
    str->nback = 0;
  }

  return status;
}

/* end of file */
