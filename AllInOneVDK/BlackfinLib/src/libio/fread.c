/************************************************************************
 *
 * fread.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr ("libName=libio")
#pragma file_attr ("libGroup=stdio.h")

#pragma file_attr ("libFunc=fread")
#pragma file_attr ("libFunc=_fread")

#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <device_int.h>
#include "_stdio.h"

static __inline size_t _readbinary(void* a, size_t n, FILE* str);

#if (BYTES_PER_WORD!=1)
static __inline size_t _readchar(void* a, size_t n, FILE* str);
#endif


size_t
fread(void* a, size_t size, size_t nelem, FILE* str)
{
  size_t ns = (size * nelem);
  size_t n;

  if ((ns == 0) || (a == NULL) || (str == NULL))
    return 0;

  _LOCK_FILE(str);

  if (str->nback > 0)
  {
    /* If there are ungetc'd characters and there's room in the buffer,
       then restore the characters.  Anything that doesn't fit gets
       flushed.  This may violate the guarantee that at least one
       ungetc must work. */

    while ((str->nback > 0) && (str->_Next > str->_Buf))
    {
      --str->_Next;

#if (BYTES_PER_WORD==1)
      *((unsigned char*) str->_Next) = str->_Back[--str->nback];
#else
      if (str->_Mode & M_BINARY) {
        *((unsigned char*) str->_Next) = str->_Back[--str->nback];
      } else {
        DEPOSIT_BYTE(*WORD_ADDR(str->_Next),
                     str->_Next,
                     str->_Back[--str->nback]);
      }
#endif
    }
    str->_Rend = str->rsave;
  }

#if (BYTES_PER_WORD==1)
  n = _readbinary(a,ns,str);
#else

  if ((BYTES_PER_WORD==1) || (str->_Mode & M_BINARY))
    n = _readbinary(a,ns,str);
  else
    n = _readchar(a,ns,str);

#endif

  _UNLOCK_FILE(str);

  return n / size;

}


#if (BYTES_PER_WORD!=1)

static __inline size_t
_readchar(void* a, size_t n, FILE* str)
{

/* reads characters from the I/O buffer of a text stream into the caller's
** namespace (assumes that n, the number of characters to be read, is
** positive) - returns the number of characters read
*/

  size_t   cnt;          /* number of characters read */

  unsigned char* s = (unsigned char*) a;

  for (cnt = 0; cnt < n; cnt++)
  {
    if (str->_Next >= str->_Rend)
      if (_fillreadbuf(str) <= 0)
        break;

    *(s++) = FETCH_BYTE(*WORD_ADDR(str->_Next), str->_Next);

    str->_Next++;
  }

  return cnt;

}

#endif


static __inline size_t
_readbinary(void* a, size_t n, FILE* str)
{

/* reads in data from a stream into the caller's namespace
** using binary mode (assumes that n, the number of memory locations
** to be read, is positive) - returns the number of words read
**
** there is no difference in the way that fread reads a file in text
** mode or in binary mode when (BYTES_PER_WORD==1).
**
** any data that is in the I/O buffer is read first - if the fread
** request is not satisfied then either further data is read from
** the file directly into the caller's namespace (but only if the
** amount of data exceeds the size of the I/O buffer), or the buffer
** is refilled.
**
** note that some care is required because this procedure short-cuts
** the usual mechanisms.
**
** parameters:
**    a   - data is read into this array
**    n   - amount of data (still) needed (in memory units)
**    str - ptr to the FILE table for the stream
*/

  size_t buflen;    /* the size of the I/O buffer */

  int m;            /* how much data available (in memory units)      */
  int c = 0;        /* how much data has been read (in memory units)  */
  int r;            /* result from a read operation                   */
  int w;            /* number of memory units read                    */

  do
  {
    m = str->_Rend - str->_Next;
    if (m > 0) {

      /* there is data in the input buffer */

      if (m > n)
        m = n;      /* take what we want if there is more than we need */

      memcpy(((unsigned char *) a) + c,
              (unsigned char *) str->_Next,m);
      str->_Next += m;

      if (m == n)
        return c+n;

      /* still need data - there is an I/O buffer but it is empty */

      n -= m;       /* decrement the amount still needed */
      c += m;       /* increment the amount read so far  */
    }

    /* read some data (PS: there might be no I/O buffer) */

    buflen = (str->bufadr) ? (str->_Bend - str->bufadr) : BUFSIZ;
    if (n > buflen)
    {

      /* optimize - bypass buffer - read directly into array */

      r = _dev_read(str->fileID,((char *) a) + c,n * BYTES_PER_WORD);
      if (r < 0) {
          str->_Mode |= M_ERROR;
          return 0;
      }

      w = (r + (BYTES_PER_WORD - 1)) / BYTES_PER_WORD;
      if (n != w) {
        str->_Mode = (str->_Mode & ~M_LAST_READ) | M_EOF;
      } else {

#if (BYTES_PER_WORD != 1)

        unsigned char *const ptr = &((unsigned char *)a)[(r-1)/BYTES_PER_WORD];
        int                  rem = r & (BYTES_PER_WORD-1);

        if (rem != 0)
        {
          *ptr &= (UCHAR_MAX >> ((BYTES_PER_WORD - rem) * 8));
               // organize padding of last incomplete word
        }

#endif
      }
      return c + w;
    }

  /* (Create and) fill the buffer */

  } while (_fillreadbuf(str) > 0);

  return c;

}

// end of file
