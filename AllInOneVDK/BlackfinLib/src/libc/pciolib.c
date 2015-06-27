/* Copyright (C) 2003-2011 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** ADI/Intel Confidential
*/
/*
@section PCI IO library

This file (pciolib.c) contains the system call routines which are invoked for
I/O over PCI, via the device handler.  Reads and writes are made to memory
in PCI space to implement the functionality of the routines.
More information is provided in the file, device.c

*/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr(  "libName=librt_fileio")
#pragma file_attr(  "libFunc=_pciIO")
#pragma file_attr(  "libFunc=__pciIO")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#endif


/*
** -------------------------------------------------------------------
** PCI Bus IO.
*/

#if defined(__ADSPLPBLACKFIN__) 
/* Not Supported */
#else

#include <ccblkfn.h>
#include <device.h>
#include <cplb.h>
#include <cplbtab.h>
#include <pcio.h>
#include <tbubp.h>

#define PCIIO_STDIN_FD 0
#define PCIIO_STDOUT_FD 1
#define PCIIO_STDERR_FD 2

#pragma section("pci_io") 
volatile PciIoCB pci[1];

extern int __default_io_device;

/*
** Send command to PCI and get response.
*/

void
_PciIO(void)
{
  volatile int *pci_ctl = (volatile int *)(PCI_CTL);
  unsigned int enable = PCI_CTL_ENABPCI | PCI_CTL_ENABINTA | PCI_CTL_OUTPUTINTA;

  /* Enable PCI interface with interrupts */
  *pci_ctl = enable;
  ssync();
  /* wait for "command complete" from host */
  while (pci->cmd) {
    flushinv((void*)&pci->cmd);
    ssync();
  }
  /* flush the control block, so responses are visible. */
  flush_data_buffer((void *)pci, (void *)(pci+1), /*invalidate*/ 1);
  return;
}

/*
** Initialise the PCI device.
*/

static int pci_initialised = 0;

/*
@subsection pci_io_init
Initializes stdin, stdout and stderr in the Device Entry table
*/
static int
pci_io_init(struct DevEntry *deventry){
  if (__default_io_device == deventry->DeviceID) {
    pci_initialised = 1;
    pci->cmd = PCI_RESET_DRIVER_REQUEST;
    _PciIO();
  }
  return 0;
}


/* 
@subsection pci_io_open
*/

static int
pci_io_open(const char *name, int mode)
{
  pci->filename = (char *)name;
  pci->mode = mode;
  pci->cmd = PCI_OPEN_REQUEST;
  _PciIO();
  return pci->fileid;
}

/*
@subsection pci_io_close
*/

static int
pci_io_close(int fd)
{
  pci->fileid = fd;
  pci->cmd = PCI_CLOSE_REQUEST;
  _PciIO();
  return pci->fileid;
}

/*
@subsection pci_io_write
*/
static int 
pci_io_write(int fd, unsigned char  *buf, int size)
{
  pci->fileid = fd;
  pci->buffer = buf;
  pci->count = size;
  pci->cmd = PCI_WRITE_REQUEST;
  _PciIO();
  return pci->count;
}


/*
@subsection pci_io_read
*/

static int
pci_io_read(int fd, unsigned char *buf, int size)
{
  pci->fileid = fd;
  pci->buffer = buf;
  pci->count = size;
  pci->cmd = PCI_READ_REQUEST;
  _PciIO();
  flush_data_buffer((void *)buf, (void *)(buf+size), /*invalidate*/1);
  return pci->count;
}

/*
@subsection pci_io_lseek
*/
long
pci_io_lseek(int fd, long offset, int whence){
  pci->fileid = fd;
  pci->count = offset;
  pci->mode = whence;
  pci->cmd = PCI_SEEK_REQUEST;
  _PciIO();
  return pci->count;
}

DevEntry pci_io_deventry = { PCI_IO, 
			     0,
			     &pci_io_init,
			     &pci_io_open, 
			     &pci_io_close, 
			     &pci_io_write, 
			     &pci_io_read,
			     &pci_io_lseek,
			     PCIIO_STDIN_FD,
			     PCIIO_STDOUT_FD,
			     PCIIO_STDERR_FD};
#endif /* ADSPLPBLACKFIN */
