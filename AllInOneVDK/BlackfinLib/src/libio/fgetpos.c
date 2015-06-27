/************************************************************************
 *
 * fgetpos.c : $Revision: 3544 $
 *
 * (c) Copyright 2003-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr("libName=libio")
#pragma file_attr("libGroup=stdio.h")

#pragma file_attr("libFunc=fgetpos")
#pragma file_attr("libFunc=_fgetpos")
#pragma file_attr("libFunc=ftell")
#pragma file_attr("libFunc=_ftell")

#pragma file_attr("prefersMem=external")
#pragma file_attr("prefersMemNum=70")
#endif

#include <stdio.h>
#include <device_int.h>
#include "_stdio.h"

#if (BYTES_PER_WORD==1)
static int count_NL_chars(unsigned char *p_start, unsigned char *p_end);
#else
static int count_NL_chars(unsigned long p_start, unsigned long p_end);
#endif

extern
int fgetpos(FILE *str, fpos_t *pos)
{
  /*
  The current position within the file is based on the SEEK service of the
  associated device driver. This service is expected to return the value
  of the file pointer from a host perspective, which will correspond to
  the previous I/O request made. This if the previous request was a write
  operation then the file pointer will correspond to the end of the record
  written (i.e. the start of the current record), and if the previous request
  was a read operation then the file pointer will correspond to the end of
  the current contents of the I/O buffer.

  Thus if the stream is currently configured for writing, the offset returned
  by the SEEK service will have to be adjusted by the amount of the data in
  the I/O buffer. A further adjustment will also be required for files that
  have been opened as a text stream - the adjustment required will be the
  number of newline ('\n') characters that are in the I/O buffer. Consider:

    Suppose that the SEEK service returns X as the current file position
    and suppose that there are Y characters in the current I/O buffer,
    then the logical file position will be (X+Y). In addition, now suppose
    one of the Y characters is a newline ('\n') character; then if the
    current I/O buffer were to be written out now, then the SEEK service
    would expand the \n character to \r\n BEFORE writing the buffer out
    to the file - and so the physical position would be (X+Y+1).

  If the stream is currently configured for reading, then the returned offset
  will have to adjusted by the amount of unread data in the I/O buffer. However,
  when a stream has been opened as a text stream, the situation is more
  complicated when the host is Windows (or MS-DOS) because line terminators
  are (normally) \r\n which the READ service condenses to \n before delivering
  input into the I/O buffer. Thus the offset returned by the SEEK service will
  also have to be adjusted by the number of \n characters within the unread
  portion of the I/O buffer. In this respect also note that when BYTE_PER_WORD
  is not set 1, then the individual characters are packed within the I/O buffer.

  If an ftell or fgetpos operation is performed after one or more characters
  have been pushed back via ungetc, then this function assumes that they do
  not contain a \n character.
  */

  long offset;

  /* Variables for adjusting the file offset in a text file */

  long adjust;                 /* data left in the buffer */

#if (BYTES_PER_WORD==1)
  unsigned char *buff_end;     /* normally a copy of str->_Rend, but will be
                                  set to str->rsave if ungetc characters are
                                  available                 */
#else
  unsigned long buff_end;
#endif

  _LOCK_FILE(str);

  if (!str) {
    _UNLOCK_FILE(str);
    return EOF;
  }

  offset = _dev_seek (str->fileID, 0L, SEEK_CUR);

  if (offset < 0L)
  {
    str->_Mode |= M_ERROR;
    _UNLOCK_FILE(str);
    return EOF;
  }

  if (str->_Mode & M_BINARY)
    offset = (offset + (BYTES_PER_WORD-1)) / BYTES_PER_WORD;

  /* 
  In the C standard, a file position in a file that is connected to a
  binary stream is measured in units of characters. Each character on
  the DSP will have been copied to the file (in the host environment,
  which will be a PC) as BYTES_PER_WORD bytes. The offset returned by
  the seek primitive will be in the units assumed by the host (i.e. 8
  bit bytes), and thus the offset will be to be converted back into the
  units assumed by the DSP (ie. characters).

  For binary streams, a file position will normally be a multiple of
  the address unit. However when a file is positioned at EOF and the
  size of the file is not a multiple of the address unit, then the
  the current file position will not be a multiple. When this happens
  the current position is rounded up to the next multiple - this is
  consistent with the behavior when reading such a file sequentially.
  */

  if (str->_Mode & M_LAST_WRITE) {
    offset += str->_Next - str->_Buf;
    
    if ((str->_Mode & M_BINARY) == 0)
        offset += count_NL_chars (str->_Buf,str->_Next);
  } 
  else if (str->_Mode & M_LAST_READ) {
    if (str->nback)
      buff_end = str->rsave;
    else
      buff_end = str->_Rend;

    adjust = buff_end - str->_Next;

    if ((str->_Mode & M_BINARY) == 0)
      adjust += count_NL_chars (str->_Next,buff_end);

    offset -= (adjust + str->nback);
  }

#ifdef __ADSPBLACKFIN__
  pos->_Off = offset;
#else
  *pos = offset;
#endif

  _UNLOCK_FILE(str);
  return 0;
}


#if (BYTES_PER_WORD==1)

static int
count_NL_chars(unsigned char *p_start, unsigned char *p_end)
{
  /*
  This function is a utility procedure that scans the data in an I/O buffer
  between p_start and p_end and returns the number of newline ('\n') characters
  that it finds.
  */
  unsigned char *ptr;
  int cnt = 0;
  for (ptr = p_start; ptr < p_end; ptr++)
  {
    if (*ptr == '\n')
      cnt++;
  }
  return cnt;
}

#else

static int
count_NL_chars(unsigned long p_start, unsigned long p_end)
{
  /*
  This function is a utility procedure that scans the data in an I/O buffer
  between p_start and p_end and returns the number of newline ('\n') characters
  that it finds.

  Note that when BYTE_PER_WORD is not set 1, then the individual characters
  will be packed within the I/O buffer.
  */
  unsigned long ptr;
  char c;
  int  cnt = 0;
  for (ptr = p_start; ptr < p_end; ptr++)
  {
    c = FETCH_BYTE(*WORD_ADDR(ptr),ptr);
    if (c == '\n')
      cnt++;
  }
  return cnt;
}

#endif

// end of file
