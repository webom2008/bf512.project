/************************************************************************
 *
 * xseek.c : $Revision: 3544 $
 *
 * (c) Copyright 2003-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_doseek")
#pragma file_attr(  "libFunc=__doseek")
#pragma file_attr(  "libFunc=fseek")
#pragma file_attr(  "libFunc=_fseek")
#pragma file_attr(  "libFunc=fgetpos")
#pragma file_attr(  "libFunc=_fgetpos")
#pragma file_attr(  "libFunc=rewind")
#pragma file_attr(  "libFunc=_rewind")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>

#include "_stdio.h"
#include "device_int.h"

long
_doseek(FILE *str, long offset, int whence)
{
  if (str == NULL)
    return EOF;

  if (!(str->_Mode & (M_OPENR | M_OPENW)) || _fflush(str))
    return EOF;                         /* stream not open or a write error */

  /* Position a file */

  if ((whence == SEEK_CUR) && (str->_Mode & M_LAST_READ))
  {

/*
The fseek request is implemented via the SEEK service of the associated device
driver, whereby the file is positioned at `offset` character positions relative
to a specified base. This base may be one of:

   [1] the start of the file (SEEK_SET)
   [2] the current position  (SEEK_CUR)
   [3] the end of the file   (SEEK_END)

When the origin is SEEK_CUR, then the specified offset must be adjusted to
take account of the logical file pointer which will not necessarily be the
same as the value of the file pointer on the host, particularly when the
I/O buffer is not empty. If the stream is currently configured for writing,
the above code will have flushed the I/O buffer, and the logical file
pointer and the value of the file pointer as perceived by the host will be
the same.

If however the stream is currently configured for reading, then the offset
has to be adjusted (downwards) by the number of unread characters in the
I/O buffer. However, when a stream has been opened as a text stream, the
situation is more complicated when the host is Windows (or MS-DOS) because
line terminators are (normally) \r\n which the READ service condenses to \n
before delivering input into the I/O buffer. Thus the specified offset has
also to be adjusted (again downwards) by the number of \n characters within
the unread portion of the I/O buffer. In this respect also note that when
BYTE_PER_WORD is not set 1, then the individual characters are packed within
the I/O buffer.

Note that positioning a text stream is only valid if:

   [1] the origin is SEEK_SET and offset is 0, or
   [2] the origin is SEEK_END and offset is 0, or
   [3] the origin is SEEK_SET and offset had been obtained from
       a previous call to either fgetpos of ftell.

If an ftell or fgetpos operation is performed after one or more characters
have been pushed back via ungetc, then this function assumes that they do
not contain a \n character.
*/

    /* Variables for adjusting the file offset in a text file */

    long adjust;               /* data left in the buffer                    */
    int  cnt;                  /* count of \n in unscanned data in buffer    */
    char c;                    /* next character in unscanned data in buffer */

#if (BYTES_PER_WORD==1)
    unsigned char *ptr;        /* ptr through unscanned data in buffer   */
#else
    unsigned long ptr;         /* 'ptr' through unscanned data in buffer */
#endif

    adjust = str->_Rend - str->_Next;

    if ( !(str->_Mode & M_BINARY))
    {
      ptr = str->_Next;

      for (cnt = 0, ptr = str->_Next; ptr < str->_Rend; ptr++)
      {
#if (BYTES_PER_WORD==1)
        c = *ptr;
#else
        c = FETCH_BYTE(*WORD_ADDR(ptr),ptr);
#endif
        if (c == '\n')
          cnt++;
      }
      adjust += cnt;
    }

    offset -= adjust - str->nback;
  }

  if ((whence == SEEK_CUR) && (offset == 0))
                                      ; /* nothing to do */
  else {
    if (str->_Mode & M_BINARY)
       offset *= BYTES_PER_WORD;
      
       /* In the C standard, a file position in a file that is connected to a
          binary stream is measured in units of characters. Each character on
          the DSP will have been written to the file (in the host environment,
          which will be a PC) as BYTES_PER_WORD bytes, and the seek primitive
          therefore requires the offset in units of bytes.
       */

    if ( _dev_seek(str->fileID, offset, whence) < 0L)
    {
      str->_Mode |= M_ERROR;
      return EOF;
    }
  }

  /* Positioning successful */

  if (str->_Mode & (M_LAST_READ | M_LAST_WRITE))
  {
    str->_Next = str->_Buf;             /* empty the buffer */
    str->_Rend = str->_Buf;
    str->_Wend = str->_Buf;
    str->nback = 0;
    str->rsave = 0;
  }

  str->_Mode &= ~(M_EOF | M_LAST_READ | M_LAST_WRITE);
  return 0;
}

// end of file
