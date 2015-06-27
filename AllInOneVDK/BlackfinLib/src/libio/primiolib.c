/* Copyright (C) 2003-2009 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** ADI/Intel Confidential
*/
/*
@section Primitive IO library

This file (primiolib.c) contains the system call routines which are invoked in
the host system via the device handler.  Reads and writes are made to memory
monitored by the host to implement the functionality of the routines.
More information is provided in the file, device.c
*/

#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=primio_init")
#pragma file_attr(  "libFunc=_primio_init")
#pragma file_attr(  "libFunc=primio_open")
#pragma file_attr(  "libFunc=_primio_open")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")
#include <device.h>
#include <device_int.h>

extern int PRIMITIVE_OPEN(const char *, int);
extern int PRIMITIVE_READ(int, unsigned char *, int);
extern int PRIMITIVE_WRITE(int, int, unsigned char *, int);
extern int PRIMITIVE_WRITE2(int, unsigned char *, int);
extern int PRIMITIVE_CLOSE(int);
extern long PRIMITIVE_SEEK(int fd, long offset, int whence);

/* Default PrimIO file descriptors for the standard streams. Set
   to dev_not_claimed if another device is to provide these streams.
*/
#define PRIMIO_STDIN_FD 0
#define PRIMIO_STDOUT_FD 1
#define PRIMIO_STDERR_FD 2

/*
** ------------------------------------------------------------------
** PrimIO device.
** Functions in this device are either the PrimIO functions themselves,
** or invoke them directly.
*/

/*
@subsection primio_init
*/
static int
primio_init(struct DevEntry *deventry)
{
  return _DEV_IS_THREADSAFE;
}

#if defined(__ADSPBLACKFIN__)

/*
@subsection primio_open
*/
static int
primio_open(const char *name, int mode)
{
  int pmode;

  /* primio flags  - see _stdio.h */
  enum {
    pio_openr    = 0x0001,
    pio_openw    = 0x0002,
    pio_append   = 0x0004,
    pio_truncate = 0x0008,
    pio_create   = 0x0010,
    pio_binary   = 0x0020
  };

  /* map the stdio mode flags into the primio equivalents */

  switch (mode&3)
  {
    default:  /* shouldn't happen */
    case _dev_rdonly:
      pmode = pio_openr;
      break;
    case _dev_wronly:
      pmode = pio_openw;
      break;
    case _dev_rdwr:
      pmode = (pio_openr|pio_openw);
      break;
  }

  if (mode&_dev_append)
    pmode |= pio_append;
  if (mode&_dev_create)
    pmode |= pio_create;
  if (mode&_dev_truncate)
    pmode |= pio_truncate;
  if (mode&_dev_binary)
    pmode |= pio_binary;

  return PRIMITIVE_OPEN(name, pmode);
}

#else
/* primio_open not needed - use PRIMITIVE_OPEN */
#endif

/* primio_close not needed - use PRIMITIVE_CLOSE  */
/* primio_write not needed - use PRIMITIVE_WRITE2 */
/* primio_read  not needed - use PRIMITIVE_READ   */
/* primio_seek  not needed - use PRIMITIVE_SEEK   */

DevEntry primio_deventry = {
              PRIMIO,
              0,
              &primio_init,
#if defined(__ADSPBLACKFIN__)
              &primio_open,
#else
              &PRIMITIVE_OPEN,
#endif
              &PRIMITIVE_CLOSE,
              &PRIMITIVE_WRITE2,
              &PRIMITIVE_READ,
              &PRIMITIVE_SEEK,
              PRIMIO_STDIN_FD,
              PRIMIO_STDOUT_FD,
              PRIMIO_STDERR_FD
};
