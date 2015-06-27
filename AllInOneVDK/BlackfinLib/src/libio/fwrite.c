/************************************************************************
 *
 * fwrite.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=fwrite")
#pragma file_attr(  "libFunc=_fwrite")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <device_int.h>
#include "_stdio.h"

static __inline size_t _writebinary(const void* a, size_t n, FILE* str);

#if (BYTES_PER_WORD!=1)
static __inline size_t _writechar(const void* a, size_t n, FILE* str);
#endif


size_t
fwrite(const void* a, size_t size, size_t nelem, FILE* str)
{
  size_t ns = (size * nelem);
  size_t n;

  if ((ns == 0) || (a == NULL) || (str == NULL))
    return 0;

   _LOCK_FILE(str);

#if (BYTES_PER_WORD==1)
    n = _writebinary(a,ns,str);
#else

  if (str->_Mode & M_BINARY) {
    n = _writebinary(a,ns,str);
  } else {
    n = _writechar(a,ns,str);
  }
#endif

  _UNLOCK_FILE(str);

  return n / size;

}


#if (BYTES_PER_WORD!=1)

static __inline size_t
_writechar(const void* a, size_t n, FILE* str)
{

/* writes characters from the caller's namespace into an I/O buffer using
** text mode (assumes that n, the number of characters to be written, is
** positive) - returns the number of characters written
*/

  size_t   cnt;          /* number of characters written */

  unsigned char* s = (unsigned char*) a;

  for (cnt = 0; cnt < n; cnt++)
  {
    if (str->_Next >= str->_Wend)
      if (_flushwritebuf(str) < 0)
        break;

    DEPOSIT_BYTE(*WORD_ADDR(str->_Next), str->_Next, *(s++));
    str->_Next++;
  }

  str->_Mode |= M_LAST_WRITE;
  return cnt;

}

#endif


static __inline size_t
_writebinary(const void* a, size_t n, FILE* str)
{

/* writes out data from the caller's namespace to a stream that has
** been opened in binary mode (assumes that n, the number of memory
** locations to be written, is positive) - returns the number of words
** written.
**
** there is no difference in the way that fwrite writes a file in text
** mode or in binary mode when (BYTES_PER_WORD==1).
**
** if there is already any data in the I/O buffer then the new data
** is written to the buffer until it becomes full, at which point
** the I/O buffer is flushed. If any remaining data to be written
** out is larger than the I/O buffer then it is sent directly to
** the stream, otherwise the remaining data is placed in the buffer.
**
** note that some care is required because this procedure short-cuts
** the usual mechanisms.
*/

  int c = 0;       /* the count of how much we have written     */
  int m;           /* amount of data to write to the I/O buffer */
  int r;           /* result from dev_write                     */

  int buflen;      /* size of the I/O buffer                    */

  if (str->bufadr == NULL)
  {

    /* There currently is no buffer */

    if (n >= BUFSIZ)
    {

      /* and one is not required (because the array to be written
      ** is larger than the buffer if one were created)
      */

      r = _dev_write(str->fileID,
                     (unsigned char *) a, n * BYTES_PER_WORD);
      if (r <= 0)
      {
        str->_Mode |= M_ERROR;
        return 0;
      }

      str->_Mode |= M_LAST_WRITE;
      return (r / BYTES_PER_WORD);
    }
  }

  do
  {

    if (_flushwritebuf(str))
      return c;
          /* flushwritebuf will create a buffer if one does not
          ** exist; it will flush a buffer that is full; or it
          ** will do nothing
          */

    buflen = str->_Bend - str->bufadr;
    if ((str->_Buf == str->_Next) && (n >= buflen))
    {

      /* Write directly to the file (if the buffer is empty AND
      ** the output array is larger than the buffer length)
      */

      r = _dev_write(str->fileID,
                    (unsigned char *) a+c, n * BYTES_PER_WORD);
      if (r <= 0)
      {
        str->_Mode |= M_ERROR;
        return 0;
      }

      str->_Mode |= M_LAST_WRITE;
      return (r / BYTES_PER_WORD) + c;
    }

    m = min(str->_Wend - str->_Next,n);
        /* calculate the smaller of the amount of data
        ** still to output and the space left in the buffer
        */

    memcpy((unsigned char *) str->_Next,
           (unsigned char *) a+c,m);
    str->_Next += m;

    c += m;
    n -= m;

  } while (n > 0);

  return c;

}

// end of file
