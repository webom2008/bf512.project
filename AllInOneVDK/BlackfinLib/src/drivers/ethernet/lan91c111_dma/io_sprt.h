/* =============================================================================
 *
 *   $RCSfile: io_sprt.h,v $
 *
 *   Description:
 *   Device Register Access functions
 *
 *   Last modified $Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $
 *
 *   $Revision: 4 $
 *
 *============================================================================*/
#ifndef __io_sprt_h__
#define __io_sprt_h__

#include <ccblkfn.h>

#define _outpw_intoff(addr,value) do{             \
         unsigned int saved_interupts   = cli();  \
         _outpw(addr,value);                      \
         sti(saved_interupts);                    \
}while(0)

#define _outpd_intoff(addr,value) do{             \
         unsigned int saved_interupts   = cli();  \
        _outpd(addr,value);                       \
         sti(saved_interupts);                    \
}while(0)

inline void _outp(unsigned int addr, unsigned char value)
{
  unsigned char volatile *paddr = (unsigned char volatile *)addr;
	
  *paddr = value;
  csync();

}

inline void _outpw(unsigned int addr, unsigned short value)
{
  unsigned short volatile *paddr = (unsigned short volatile *)addr;
	
  *paddr = value;
  csync();
}


inline void _outpd(unsigned int addr, unsigned int value)
{
  unsigned int volatile *paddr = (unsigned int volatile *)addr;

  *paddr = value;
  csync();
}


inline char _inp(unsigned int addr)
{
 volatile unsigned char rval;
 unsigned char volatile *paddr = (unsigned char volatile *)addr;

  rval = *paddr;
  return rval;

}

inline short _inpw(unsigned int addr)
{
  volatile unsigned short rval;
  unsigned short volatile *paddr = (unsigned short volatile *)addr;

  rval = *paddr;
  return rval;
}


inline int _inpd(unsigned int addr)
{
  volatile int rval;
  unsigned int volatile *paddr = (unsigned int volatile *)addr;

  rval = *paddr;
  return rval;
}


inline void _ch_clk(unsigned int addr, unsigned int value)
{
  unsigned int volatile *paddr = (unsigned int volatile *)addr;
	
  *paddr = value;
  ssync();
}


inline int _inpd_intoff(unsigned int addr)
{
  unsigned int saved_interupts   = cli();
  volatile int rval;
  unsigned int volatile *paddr = (unsigned int volatile *)addr;

  rval = *paddr;
  sti(saved_interupts);	
  return rval;
}

inline short _inpw_intoff(unsigned int addr)
{
  unsigned int saved_interupts   = cli();
  volatile unsigned short rval;
  unsigned short volatile *paddr = (unsigned short volatile *)addr;

    rval = *paddr;
    sti(saved_interupts);
	
    return rval;
}
#endif // __io_sprt_h__
