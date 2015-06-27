/*------------------------------------------------------------------------
 . smc91111.c
 . This is a driver for SMSC's 91C111 single-chip Ethernet device.
 .
 . Copyright (C) 2001-2004 Standard Microsystems Corporation (SMSC)
 .       Developed by Simple Network Magic Corporation (SNMC)
 . Copyright (C) 1996 by Erik Stahlman (ES)
 .
 . This program is free software; you can redistribute it and/or modify
 . it under the terms of the GNU General Public License as published by
 . the Free Software Foundation; either version 2 of the License, or
 . (at your option) any later version.
 .
 . This program is distributed in the hope that it will be useful,
 . but WITHOUT ANY WARRANTY; without even the implied warranty of
 . MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 . GNU General Public License for more details.
 .
 . You should have received a copy of the GNU General Public License
 . along with this program; if not, write to the Free Software
 . Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 .
 . Information contained in this file was obtained from the LAN91C111
 . manual from SMC.  To get a copy, if you really want one, you can find 
 . information under www.smsc.com.
 . 
 .
 . "Features" of the SMC chip:
 .   Integrated PHY/MAC for 10/100BaseT Operation
 .   Supports internal and external MII
 .   Integrated 8K packet memory
 .   EEPROM interface for configuration
 .
 . Arguments:
 . 	io	= for the base address
 .	irq	= for the IRQ
 .	nowait	= 0 for normal wait states, 1 eliminates additional wait states
 .
 . author:
 . 	Erik Stahlman				( erik@vt.edu )
 . 	Daris A Nevil				( dnevil@snmc.com )
 .  	Pramod B Bhardwaj   			(pramod.bhardwaj@smsc.com)
 .  	M David Gelbman   			(david.gelbman@smsc.com)
 .
 .
 . Hardware multicast code from Peter Cammaert ( pc@denkart.be )
 .
 . Sources:
 .    o   SMSC LAN91C111 databook (www.smsc.com)
 .    o   smc9194.c by Erik Stahlman
 .    o   skeleton.c by Donald Becker ( becker@cesdis.gsfc.nasa.gov )
 .
 . History:
 .    17/08/06 (v2.02) by Frank Meisenbach (f.meisenbach@cbl.de)
 .             - Bugs fixed to improve the stability
 .             --> Sometimes the system hung up, e.g. under heavy broadcast load condition
 .             - Added the function eth_change_mtu()
 .             - Added support for RevC
 .             --> smc_recv() was buggy, RevC was handled as RevA
 .             - Modified smc_timeout()
 .             --> The interface will have the same behavior as before (PROMISC, MULTICAST)
 .	  06/23/06  M David Gelbman add "insmod" link_mode parameter
 .    11/03/05  M David Gelbman  Added support for RevC of LAN91C111
 .    12/13/04  Steven Chew, Added code to reset PHY upon link down
 .    09/24/01  Pramod B Bhardwaj, Added the changes for Kernel 2.4
 .    08/21/01  Pramod B Bhardwaj Added support for RevB of LAN91C111
 .	04/25/01  Daris A Nevil  Initial public release through SMSC
 .	03/16/01  Daris A Nevil  Modified smc9194.c for use with LAN91C111
 ----------------------------------------------------------------------------*/

/*********************************************************************************
 *
 *  Portions Copyright (c) 2006 - 2010 Analog Devices, Inc.
 *
 *********************************************************************************/


// USE_16BIT has to be defined since the transmittable data will start on a half word boundary

#if !defined(__ADSPBF561__)
#define USE_16_BIT
#endif

#ifdef __ADSPBF535__
#include <cdefbf535.h>
#endif /* __ADSPBF535__ */

#ifdef __ADSPBF533__
#include <cdefbf533.h>
#endif  /* __ADSPBF533__ */

#ifdef __ADSPBF561__
#include <cdefbf561.h>
#endif  /* __ADSPBF561__ */

#ifdef __ADSPBF537__
#include <cdefbf537.h>
#endif  /* __ADSPBF537__ */

#ifdef __ADSPBF538__
#include <cdefbf538.h>
#endif /* __ADSPBF538__ */

#include <string.h>
#include <sys/exception.h>
#include <services/services.h>
#include <drivers/adi_dev.h>
#include "lan91c111.h"
#include "io_sprt.h"
#include <ADI_ETHER_USBLAN.h>

// Acknowledge the LAN interrupt by clearing associated PF
//
#if (defined(__ADSPBF533__) || defined(__ADSPBF538__))
#define ACK_LAN_INT(pf) ((*pFIO_FLAG_C = pf))
static int buff_prefix =0;
#endif

#ifdef __ADSPBF561__
#define ACK_LAN_INT(pf) ((*pFIO0_FLAG_C = pf))
static int buff_prefix =2;
#endif

#ifdef __ADSPBF537__
#define ACK_LAN_INT(pf) ((*pPORTFIO_CLEAR = pf))
static int buff_prefix =0;
#endif


#define MAX_RCVE_FRAME 1560

static void *CriticalHandle;

#define ENTER_CRITICAL_REGION() (CriticalHandle=adi_int_EnterCriticalRegion(dev->CriticalData))
#define EXIT_CRTICIAL_REGION()  (adi_int_ExitCriticalRegion(CriticalHandle))

#define LAN91C111_LOG_NET_ERRORS 1

/* The transmission attempt failed due to lack of memory - so queue and retry next time */
#define LAN91C111_TX_RETRY     1
/* The transmission attempt is an unrecoverable error - free the packet, drop it */
#define LAN91C111_TX_ERROR     2
/* The transmission attempt completed successfully */
#define LAN91C111_TX_SUCCESS   3

/* global static data */
static ADI_ETHER_LAN91C111_DATA EtherDev={0};

/* function prototypes */
int StartMac(ADI_ETHER_LAN91C111_DATA *dev);
int SetDevicePFConfig (ADI_ETHER_LAN91C111_DATA *dev);
int QueueNewRcvFrames (ADI_ETHER_LAN91C111_DATA *dev,ADI_ETHER_BUFFER *bufs);
int QueueNewXmtFrames (ADI_ETHER_LAN91C111_DATA *dev,ADI_ETHER_BUFFER *bufs);

enum
{
    MC_TABLE0=0,
    MC_TABLE1,
    MC_TABLE2,
    MC_TABLE3,
    MC_TABLE4,
    MC_TABLE5,
    MC_TABLE6,
    MC_TABLE7
};

#include <cycle_count_bf.h>

static void ker_msec(void)
{
    unsigned long long int cur,nd;

    _GET_CYCLE_COUNT(cur);

    nd = cur + (__PROCESSOR_SPEED__/1000);
    while (cur < nd) {
        _GET_CYCLE_COUNT(cur);
    }

}

static int smsc_sleep(unsigned int msec)
{
    while (msec != 0) {
        ker_msec();
        msec--;
    }
    return 0;
}

#pragma optimize_off
/****************************************************************************
 *  Read register from EEPROM
 ****************************************************************************/
static int eeprom_read_reg(int reg_no)
{
    int timeout;

    // select bank 2
    _outpw(BSR_REG, 2);
    _outpw(PTR_REG,reg_no);

    // select bank 1
   _outpw(BSR_REG, 1);
   _outpw(CTL_REG,_inpw(CTL_REG) | CTL_EEPROM_SELECT | CTL_RELOAD);

   timeout = 100;
   while((_inpw(CTL_REG) & CTL_RELOAD) && --timeout)
   {
       int t=10000;
       while(t>0)
           t--;
   }
   // error in reading from eeprom
   if(timeout == 0)
       return -1;

   return(_inpw(GP_REG));
}

/****************************************************************************
 *  Write register to EEPROM
 ****************************************************************************/
static int eeprom_write_reg(int reg_no, int val)
{
    int timeout;
    // select bank 2
    _outpw(BSR_REG, 2);
    _outpw(PTR_REG,reg_no);

    // select bank 1
   _outpw(BSR_REG, 1);
   _outpw(GP_REG, val);
   _outpw(CTL_REG,_inpw(CTL_REG) | CTL_EEPROM_SELECT | CTL_STORE);

   timeout = 100;
   while((_inpw(CTL_REG) & CTL_STORE) && --timeout)
   {
       int t=10000;
       while(t>0)
           t--;
   }
   // error in reading from eeprom
   if(timeout == 0)
       return -1;
   return 1;
}

/******************************************************************************
 * Set MAC address in the SMSC
 *****************************************************************************/
static int set_mac_in_SMSC(const unsigned char* mac_addr)
{
   unsigned short page;
   short wtmp;
   int timeout;

   // save old page
   page = _inpw(BSR_REG);

   // select bank 1
   _outpw(BSR_REG, 1);

   // Set EEPROM Store
   _outpw(CTL_REG,CTL_EEPROM_SELECT | CTL_STORE);

   timeout = 100;
   while((_inpw(CTL_REG) & CTL_STORE) && --timeout)
   {
     int t=10000;
         while(t>0)
       t--;
   }
   // error in reading from eeprom
   if(timeout == 0)
       return -1;

    wtmp = (((mac_addr[1])<<8)|(mac_addr[0]));
    _outpw(ADDR0_REG, wtmp);
    wtmp = (((mac_addr[3])<<8)|(mac_addr[2]));
    _outpw(ADDR1_REG, wtmp);
    wtmp = (((mac_addr[5])<<8)|(mac_addr[4]));
    _outpw(ADDR2_REG, wtmp);

   // restore bank
   _outpw(BSR_REG, page);

   return 1;
}

/******************************************************************************
 * Get MAC address From the SMSC
 *****************************************************************************/
static void get_mac_frm_SMSC(unsigned char* mac_addr)
{
    volatile unsigned short addr;
    volatile unsigned short page;
    page = _inpw(BSR_REG);

    _outpw(BSR_REG, 1);
    addr = _inpw(ADDR0_REG); // get MAC addr
    mac_addr[0] = addr & 0xFF;
    mac_addr[1] = addr >> 8;

    addr = _inpw(ADDR1_REG); // get MAC addr
    mac_addr[2] = addr & 0xFF;
    mac_addr[3] = addr >> 8;

    addr = _inpw(ADDR2_REG); // get MAC addr
    mac_addr[4] = addr & 0xFF;
    mac_addr[5] = addr >> 8;
    _outpw(BSR_REG, page);
    return;
}

/******************************************************************************
 * Get MAC address From the EEPROM
 *****************************************************************************/
static void set_mac_in_EEPROM(const unsigned char* mac_addr)
{
   unsigned short page;
   short wtmp;

   // save old page
   page = _inpw(BSR_REG);

    wtmp = (((mac_addr[1])<<8)|(mac_addr[0]));
    eeprom_write_reg(0x20,wtmp);
    wtmp = (((mac_addr[3])<<8)|(mac_addr[2]));
    eeprom_write_reg(0x21, wtmp);
    wtmp = (((mac_addr[5])<<8)|(mac_addr[4]));
    eeprom_write_reg(0x22, wtmp);

   // restore bank
   _outpw(BSR_REG, page);

   return;
}
/******************************************************************************
 * Get MAC address From the EEPROM
 *****************************************************************************/
static void get_mac_frm_EEPROM(unsigned char* mac_addr)
{
    volatile unsigned short addr,i,val,j=0;
    volatile unsigned short page;
    page = _inpw(BSR_REG);

    // eeprom offset 20 has mac addr
    for(i=0x20;i<0x23;i++)
    {
        val = eeprom_read_reg(i);
        mac_addr[j++] = val & 0xff;
        mac_addr[j++] = val >> 8;
    }

    _outpw(BSR_REG, page);
    return;
}
/******************************************************************************
 *  Change the current IRQ mask
 *****************************************************************************/
static void LAN91C111_enable_int (unsigned char IRQ)
{
   volatile unsigned short page;

   // save old page
   page = _inpw(BSR_REG);
   // switch to bank 2
   _outpw(BSR_REG, 2);
   // update current mask
   _outp(IM_REG, _inp(IM_REG) | IRQ);
   // restore bank
   _outpw(BSR_REG, page);
   return;
}

/******************************************************************************
 *  Change the current IRQ mask
 *****************************************************************************/
static void LAN91C111_disable_int (unsigned char IRQ)
{
   volatile unsigned short page;
   volatile unsigned char mask;

   // save old page
   page = _inpw(BSR_REG);
   // switch to bank 2
   _outpw(BSR_REG, 2);
   // write new mask
   _outp(IM_REG, _inp(IM_REG) & ~IRQ);
   // restore bank
   _outpw(BSR_REG, page);
   return;
}

/******************************************************************************
 * Writes a word through the MII interface to the PHY
 *****************************************************************************/

static void LAN91C111_write_phy_register(unsigned char phyreg, unsigned short phydata)
{
   volatile int i;
   volatile unsigned short oldBank, wmask, mii_reg;
   volatile unsigned char bits[65], bmask;
   volatile int  clk_idx = 0;

   // Prepare bit stream for PHY
   // 32 consecutive ones on MDO to establish sync
   for (i = 0; i < 32; ++i)
      bits[clk_idx++] = MII_MDOE | MII_MDO;

   // Start code <01>
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE | MII_MDO;
   // Write command <01>
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE | MII_MDO;

   // Output the PHY address, msb first, internal PHY = 0
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE;

   // Output the phy register number, msb first
   bmask = 0x10;
   for (i = 0; i < 5; ++i)
   {
      if (phyreg & bmask)
         bits[clk_idx++] = MII_MDOE | MII_MDO;
      else
         bits[clk_idx++] = MII_MDOE;
      // Shift to next lowest bit
      bmask >>= 1;
   }

   // Tristate and turnaround (2 bit times) <10>
   bits[clk_idx++] = MII_MDOE | MII_MDO;;
   bits[clk_idx++] = 0;

   // Write out 16 bits of data, msb first
   wmask = 0x8000;
   for (i = 0; i < 16; ++i)
   {
      if (phydata & wmask)
         bits[clk_idx++] = MII_MDOE | MII_MDO;
      else
         bits[clk_idx++] = MII_MDOE;
      // Shift to next lowest bit
      wmask >>= 1;
   }

   // Final clock bit (tristate)
   bits[clk_idx++] = 0;

   // Save the current bank
   oldBank = _inpw(BSR_REG);
   // Select bank 3
   _outpw(BSR_REG, 3);

   // Get the current MII register value
   mii_reg = _inpw(MII_REG);

   // Turn off all MII Interface bits
   mii_reg &= ~(MII_MDOE|MII_MCLK|MII_MDI|MII_MDO);

   // Clock all cycles
   for (i = 0; i < sizeof bits; ++i)
   {
      // Clock Low - output data
      _outpw(MII_REG, mii_reg | bits[i] );
      // Clock Hi - input data
      _outpw(MII_REG, mii_reg | bits[i] | MII_MCLK);
   }

   // Return to idle state
   // Set clock to low, data to low, and output tristated
   _outpw(MII_REG, mii_reg);

   // Restore original bank select
   _outpw(BSR_REG, oldBank );
   return;
}

/******************************************************************************
 * Reads a word through the MII interface off the PHY
 *****************************************************************************/
static unsigned short LAN91C111_read_phy_register(unsigned char phyreg)
{
   volatile unsigned char bits[64];
   volatile unsigned char bmask;
   volatile unsigned short oldBank, mii_reg, phydata, wmask;
   volatile int i, clk_idx = 0;
   volatile int input_idx;

   // Prepare bit stream for PHY
   // 32 consecutive ones on MDO to establish sync
   for (i = 0; i < 32; ++i)
      bits[clk_idx++] = MII_MDOE | MII_MDO;

   // Start code <01>
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE | MII_MDO;
   // Read command <10>
   bits[clk_idx++] = MII_MDOE | MII_MDO;
   bits[clk_idx++] = MII_MDOE;
   // internal PHY address = <00000>
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE;
   bits[clk_idx++] = MII_MDOE;

   // Output the phy register number, msb first
   bmask = 0x10;
   for (i = 0; i < 5; ++i)
   {
      if (phyreg & bmask)
         bits[clk_idx++] = MII_MDOE | MII_MDO;
      else
         bits[clk_idx++] = MII_MDOE;
      // Shift to next lowest bit
      bmask >>= 1;
   }

   // Tristate and turnaround (2 bit times)
   bits[clk_idx++] = 0;

   // Input starts at this bit time
   input_idx = clk_idx;

   // Will input 16 bits
   for (i = 0; i < 16; ++i)
      bits[clk_idx++] = 0;

   // Final clock bit
   bits[clk_idx++] = 0;

   // Save the current bank
   oldBank = _inpw(BSR_REG);
   // Select bank 3
   _outpw(BSR_REG, 3);

   // Get the current MII register value
   mii_reg = _inpw(MII_REG);

   // Turn off all MII Interface bits
   mii_reg &= ~(MII_MDOE|MII_MCLK|MII_MDI|MII_MDO);

   // Clock all 64 cycles
   for (i = 0; i < clk_idx /*sizeof bits*/; ++i)
   {
      // Clock Low - output data
      _outpw(MII_REG, mii_reg | bits[i]);
      // Clock Hi - input data
      _outpw(MII_REG, mii_reg | bits[i] | MII_MCLK);
      bits[i] |= _inpw(MII_REG) & MII_MDI;
   }

   // Return to idle state
   // Set clock to low, data to low, and output tristated
   _outpw(MII_REG, mii_reg);

   // Restore original bank select
   _outpw(BSR_REG, oldBank);

   // Recover input data
   phydata = 0;
   for (i = 0; i < 16; ++i)
   {
      phydata <<= 1;
      if (bits[input_idx++] & MII_MDI)
         phydata |= 0x0001;
   }
   return (phydata);
}

/******************************************************************************
 * IRQ Sub Function: SMC_RCV for the LAN91C111
 *
 * Purpose:
 *  There is a packet waiting to be read from 91C111
 *
 * Actions:
 *  - Read the status
 *  - If an error, record it
 *  - otherwise, read in the packet
 *
 *****************************************************************************/
static void LAN91C111_rcv(ADI_ETHER_LAN91C111_DATA *dev )
{
    int     i, packet_number,chip_id;
    unsigned short    status;
    unsigned short    packet_length;
#ifndef USE_16_BIT
    unsigned int   *data,stat_pktlen;
#else
    unsigned short  *data;
#endif
    unsigned short *elnth;

   // bank 2 already opened by main IRQ entry

   packet_number = _inpw(FIFO_REG);

   if ( packet_number & RXFIFO_REMPTY )
   {
      // we got called , but nothing was on the FIFO
      // so, just leave
      return;
   }

   //  start reading from the start of the packet
   _outpw(PTR_REG, PTR_READ | PTR_RCV | PTR_AUTOINC);

#ifndef USE_16_BIT
   stat_pktlen   = _inpd(DATA_REG);
   status        = (unsigned short)(stat_pktlen & 0x0000ffff);
   packet_length = (unsigned short)( (stat_pktlen>>16) & 0x000ffff);
#else
   // First two words are status and packet_length
   status          = _inpw(DATA_REG);
   packet_length   = _inpw(DATA_REG);
#endif

   // store for debug
   dev->LAN_state.rpsize = packet_length;

   // max 2k packets over Ethernet
   packet_length &= 0x07ff;  /* mask off top bits */

   if ( !(status & RS_ERRORS ) )
   {
      ADI_ETHER_BUFFER *tmp_q_ele;

#if LAN91C111_LOG_NET_ERRORS
      if ( status & RS_MULTICAST ) dev->Stats->cEMAC_RX_CNT_MULTI++;
#endif

      // collect the packet
      // first two words already read: status and packet size
      if((status & 0x1000))
          packet_length -= 5;   //## used to be 4
      else
      {
          // Get the chip id and fix the hardware anomaly rev-A
          _outpw(BSR_REG,3);
          chip_id = _inpw(REV_REG);
         if( ((chip_id & 0x0F00) == 9) && ((chip_id & 0xF000) == 0) )
             packet_length -= 4;
         else
             packet_length -= 6;
         _outpw(BSR_REG, 2);
      }


      tmp_q_ele = dev->m_RxEnqueuedHead;
      if (tmp_q_ele != NULL) {
          elnth = (unsigned short *)tmp_q_ele->Data;
          *elnth = (packet_length + 4 + buff_prefix);   // add on 4 bytes for the CRC (even if we don't store it) + prefix if present
          tmp_q_ele->ProcessedElementCount = (*elnth+2+tmp_q_ele->ElementWidth-1)/tmp_q_ele->ElementWidth;
      }

      if ((tmp_q_ele)&&(tmp_q_ele->ProcessedElementCount <= tmp_q_ele->ElementCount))
      {
         //store packet length
         // assign pointer,
         // lwIP wrapper passes the Data to point to the 2byte length header skipping the prefix number of bytes.
         //
         data = (void *)( ((char *)tmp_q_ele->Data) +2);
      }
      else
      {
#if LAN91C111_LOG_NET_ERRORS
         if (tmp_q_ele) {
            dev->Stats->cEMAC_RX_CNT_LONG++;
         } else {
            dev->Stats->cEMAC_RX_CNT_LOST++;
         }
#endif
         // drop packet
         //  good or bad, delete this packet */
         _outpw(MMU_CMD_REG, MC_RELEASE);
         return;
      }

#ifndef USE_16_BIT
      // read in 32bits
      for (i=0; i <= (packet_length >> 2); i++)
      {
         *data++ = _inpd(DATA_REG);
      }
#else
      for (i=0; i <= (packet_length >> 1); i++)
      {
         *data++ = _inpw(DATA_REG);
      }
#endif // USE_16_BIT


      /* Change the Enqueued head */
      dev->m_RxEnqueuedHead = (ADI_ETHER_BUFFER*)tmp_q_ele->pNext;
      dev->m_RxEnqueuedCount--;
      if (dev->m_RxEnqueuedCount == 0)
         dev->m_RxEnqueuedTail = NULL;

      /* Add the packet to dequeued list*/
      if (dev->m_RxDequeuedCount)
         dev->m_RxDequeuedTail->pNext = (ADI_ETHER_BUFFER *)tmp_q_ele;
      else
         dev->m_RxDequeuedHead = tmp_q_ele;

      //No matter what this is also the tail.
      dev->m_RxDequeuedTail = tmp_q_ele;
      //And tail->next should point to NULL
      tmp_q_ele->pNext = NULL;
      dev->m_RxDequeuedCount++;

      // finally we have to update the status word etc
      tmp_q_ele->ProcessedFlag = 1;
      tmp_q_ele->StatusWord = 0x3000 + (packet_length+4);

#if LAN91C111_LOG_NET_ERRORS
      dev->Stats->cEMAC_RX_CNT_OK++;
#endif
   }
   else
   {
#if LAN91C111_LOG_NET_ERRORS
      if ( status & RS_ALGNERR )  dev->Stats->cEMAC_RX_CNT_ALIGN++;
      if ( status & (RS_TOOSHORT|RS_TOOLONG)) dev->Stats->cEMAC_RX_CNT_LONG++;
      if ( status & RS_BADCRC)    dev->Stats->cEMAC_RX_CNT_FCS++;
#endif
   }

   while ( _inpw(MMU_CMD_REG ) & MC_BUSY );

   //  good or bad, delete this packet */
   _outpw(MMU_CMD_REG, MC_RELEASE);
   return;
}

/******************************************************************************
 * TX IRQ Error Handler Function for SMSC91C111
 *
 * Purpose:  Handle the transmit error message.
 *  This is only called when an TX error occurred because of
 *  the AUTO_RELEASE mode.
 *
 * Actions:
 *  - Save pointer and packet no
 *  - Get the packet no from the top of the queue
 *  - check if it's valid ( if not, is this an error??? )
 *  - read the status word
 *  - record the error
 *  - ( resend?  Not really, since we don't want old packets around )
 *  - Restore saved values
 *
 *****************************************************************************/
static void LAN91C111_tx(ADI_ETHER_LAN91C111_DATA * dev)
{
   volatile unsigned char saved_packet;
   volatile unsigned char packet_no;
   volatile unsigned short tx_status;

   // bank 2 already opened by main IRQ entry
   saved_packet = _inp(PN_REG);
   packet_no = _inpw(FIFO_REG);
   packet_no &= 0x7F;

   // If the TX FIFO is empty then nothing to do
   if ( packet_no & TXFIFO_TEMPTY )
      return;

   // select this as the packet to read from
   _outp(PN_REG, packet_no);

   // read the first word (status word) from this packet
   _outpw(PTR_REG, PTR_AUTOINC|PTR_READ);

   // read tx status
   tx_status = _inpw(DATA_REG );

#if LAN91C111_LOG_NET_ERRORS
   dev->Stats->cEMAC_TX_CNT_ABORT++;
   if ( tx_status & TS_LOSTCAR ) dev->Stats->cEMAC_TX_CNT_CRS++;
   if ( tx_status & TS_LATCOL  ) dev->Stats->cEMAC_TX_CNT_LATE++;
#endif

   // re-enable transmit
   _outpw(BSR_REG, 0);
   _outpw(TCR_REG, _inpw(TCR_REG) | TCR_TXENA);

   // kill the packet
   _outpw(BSR_REG, 2);
   _outpw(MMU_CMD_REG, MC_FREEPKT);

   // one less packet waiting for me
   dev->LAN_state.packets_waiting--;

   // Don't change Packet Number Reg until busy bit is cleared
   while ( _inpw(MMU_CMD_REG) & MC_BUSY );

   _outp(PN_REG, saved_packet);

   return;
}

/******************************************************************************
 * IRQ Sub Function: smc_phy_interrupt for the LAN91C111
 *
 * Purpose:
 *  Handle interrupts relating to PHY register 18.
 *
 * Actions:
 *  Log last Phy18 Interrupt Source
 *
 *****************************************************************************/
static void LAN91C111_phy_interrupt(ADI_ETHER_LAN91C111_DATA * cptr)
{
   volatile unsigned short phy18;

   while (1)
   {
      // Read PHY Register 18, Status Output
      phy18 = LAN91C111_read_phy_register(PHY_INT_REG);

      // Exit if no more changes
      if (phy18 == cptr->LAN_state.lastPhy18)
         break;

      // Update the last phy 18 variable
      cptr->LAN_state.lastPhy18 = phy18;
   } // end while
}
/******************************************************************************
 * LANC91C111: LAN91C111_hardware_send_packet
 *
 * Purpose:
 *  This sends the actual packet to the SMC9xxx chip.
 *
 *  - First, see if a pending packet is available.
 *      this should NOT be called if there is none
 *  - Find the packet number that the chip allocated
 *  - Point the data pointers at it in memory
 *  - Set the length word in the chip's memory
 *  - Dump the packet to chip memory
 *  - Check if a last byte is needed ( odd length packet )
 *      if so, set the control flag right
 *  - Tell the LAN 91C111 to send it
 *  - Enable the transmit interrupt, so I know if it failed
 *  - Free the packet data if I actually sent it.
 *
 *****************************************************************************/
static unsigned char LAN91C111_hardware_send_packet(ADI_ETHER_LAN91C111_DATA * cptr)
{
   int     i;
   unsigned char    packet_no;
   unsigned short   length;
#ifndef USE_16_BIT
    unsigned int    *buf;
#else
    unsigned short  *buf;
#endif
   unsigned short    *elnth;
   unsigned short    lnth_first;
   ADI_ETHER_BUFFER  *bf = cptr->m_TxEnqueuedHead;

   // get pointer
#ifndef USE_16_BIT
   buf = (unsigned int *)(((char *)bf->Data)+2);
#else
   buf = (unsigned short *)(((char *)bf->Data)+2);
#endif


   // the length is held in the first two bytes of the 'frame', the ElementCount says the number of elements in the first buffer

   // get length
   elnth = (unsigned short *)bf->Data;
   length = *elnth;
   lnth_first = bf->ElementCount*bf->ElementWidth - 2;

   length = ETH_ZLEN < length ? length : ETH_ZLEN;
   lnth_first = ETH_ZLEN < lnth_first ? lnth_first : ETH_ZLEN;

   // If I get here, I _know_ there is a packet slot waiting for me
   packet_no = _inp(AR_REG);

   // or isn't there?  BAD CHIP
   if ( packet_no & AR_FAILED ) return LAN91C111_TX_ERROR;

   // we have a packet address, so tell the card to use it
   _outp(PN_REG, packet_no);

   // point to the beginning of the packet
   _outpw(PTR_REG, PTR_AUTOINC);

   // send the packet length ( +6 for status, length and ctl byte )
   //   and the status word ( set to zeros ) */

#ifndef USE_16_BIT
   _outpd(DATA_REG, (length+6) << 16);
#else
   _outpw(DATA_REG, 0);
   _outpw(DATA_REG, (length+6));
#endif

   // send the actual data
#ifndef USE_16_BIT
   for (i=0; i < (length>>2); i++)
      _outpd(DATA_REG, *buf++);
   // last 16 bit word
   if ( length & 0x2  ) _outpw(DATA_REG, *buf);
#else
    // do the first buffer
   for (i=0; i < (lnth_first>>1); i++)
      _outpw(DATA_REG, *buf++);
     // check to see if more to go
     if (lnth_first<length) {
        buf = bf->PayLoad;
        length = length-lnth_first;
        // do the second buffer
        for (i=0; i < (length>>1); i++)
          _outpw(DATA_REG, *buf++);
     }
#endif // USE_32_BIT

   /* Send the last byte, if there is one.   */
   if ((length & 1) == 0)
   {
      // write 0 as CONTROL unsigned char
      _outpw(DATA_REG, 0x0000);
   }
   else
   {
#ifndef USE_16_BIT
      // last byte to be sent
      if (length & 2) *buf = (*buf>>16);
#endif
      // last byte to be sent
      _outpw(DATA_REG, 0x2000 | (*buf) & 0xff);
   }
   // now set the status in the buffer bf
   bf->ProcessedFlag = 1;
   bf->ProcessedElementCount = (*elnth+2 + buff_prefix + bf->ElementWidth-1)/bf->ElementWidth;
   bf->StatusWord = 0x3;   // completed and OK

   #if 0
   ///////////////////////////////////////////////////////////////////
   // This code is added to handle the case where the source and
   // destination MAC address is the same. It just copies the broadcast
   // and frames that are destinated to the same MAC.
   // The below code could be commented if the client and server runs on
   // different boards.
   //
    if( (!memcmp(cptr->phyAddr,(unsigned char *)cptr->m_TxEnqueuedHead->data_start,6) )
        || (!memcmp((unsigned char *)cptr->m_TxEnqueuedHead->data_start,"\xff\xff\xff\xff\xff\xff",6)))
    {
        struct buffer_t *tmp_q_ele;

        tmp_q_ele = cptr->m_RxEnqueuedHead;
         if(tmp_q_ele != NULL)
         memcpy(tmp_q_ele->data_start,(unsigned short *)cptr->m_TxEnqueuedHead->data_start,cptr->m_TxEnqueuedHead->num_elements);

           /* Change the Enqueued head */
        cptr->m_RxEnqueuedHead = (struct buffer_t*)tmp_q_ele->next;
        cptr->m_RxEnqueuedCount--;
        if (cptr->m_RxEnqueuedCount == 0)
             cptr->m_RxEnqueuedTail = (struct buffer_t*)NULL;

        /* Add the packet to dequeued list*/
        if (cptr->m_RxDequeuedCount)
             cptr->m_RxDequeuedTail->next = tmp_q_ele;
        else
             cptr->m_RxDequeuedHead = tmp_q_ele;

      //No matter what this is also the tail.
      cptr->m_RxDequeuedTail = tmp_q_ele;
      //And tail->next should point to NULL
      tmp_q_ele->next = NULL;
      cptr->m_RxDequeuedCount++;
    }
    ////////////////////////////////////////////////////////////
   #endif
   // enable the interrupts
   LAN91C111_enable_int( (IM_TX_INT | IM_TX_EMPTY_INT) );

   // and let the chipset deal with it
   _outpw(MMU_CMD_REG, MC_ENQUEUE);


   return LAN91C111_TX_SUCCESS;
}
/******************************************************************************
 * LANC91C111: LAN91C111_wait_to_send_packet()
 *
 * Purpose:
 *    Attempt to allocate memory for a packet, if chip-memory is not
 *    available, then tell the card to generate an interrupt when it
 *    is available.
 *****************************************************************************/
static unsigned char LAN91C111_wait_to_send_packet(ADI_ETHER_LAN91C111_DATA * cptr)
{
   unsigned short    length;
   unsigned short    numPages;
   unsigned short    time_out;
   unsigned short    status;
   unsigned short    *elnth;


   // the length is held in the first two bytes of the 'frame', the ElementCount says the number of elements in the first buffer

   // get length
   elnth = (unsigned short *)cptr->m_TxEnqueuedHead->Data;
   length = *elnth;

   // set length
   length = ETH_ZLEN < length ? length : ETH_ZLEN;

   // clear allocation flag
   cptr->LAN_state.alloc_success = 0;

   /*
    * The MMU wants the number of pages to be the number of 256 bytes
    * 'pages', minus 1 ( since a packet can't ever have 0 pages :) )
    *
    * Pkt size for allocating is data length +6 (for additional status
    * words, length and ctl!)
    *
    * If odd size then last byte is included in this header.
    */
   numPages = ((length & 0xfffe) + 6);
   numPages >>= 8; // Divide by 256

   if (numPages > 7 )
   {
      // way too big packet, this is an error
      // free packet
      return LAN91C111_TX_ERROR;
   }
   // either way, a packet is waiting now
   cptr->LAN_state.packets_waiting++;

   // now, try to allocate the memory */
   _outpw(BSR_REG, 2);
   _outpw(MMU_CMD_REG, MC_ALLOC | numPages);

   /*
    * Performance Hack
    *
    * wait a short amount of time.. if I can allocate a packet now, I set
    * the flag now.  Otherwise, I enable an interrupt and wait for one to be
    * available.
    *
    */

  TX_RETRY_LAN:

   time_out = 200;// number of iterations
   do
   {
      status = _inp(INT_REG);
      if ( status & IM_ALLOC_INT )
      {
         /* acknowledge the interrupt */
         _outp(INT_REG, IM_ALLOC_INT);
         break;
      }
   } while ( --time_out );

   // too bad, no immediate allocation...
   if ( !time_out )
   {
      /* Check the status bit one more time just in case */
      /* it snuck in between the time we last checked it */
      /* and when we set the interrupt bit */
      status = _inp(INT_REG );
      if ( !(status & IM_ALLOC_INT) )
      {
        /* The allocation used to fail when ever the RXOVRN was set and
         * the packets were not transmitted. This takes cares of the
         * above, I have not observed any packet loss, but I am not
         * that sure also, MC_RELEASE release the receive packet,
         * At the max we may loose one packet.
         */

        // Release and reset the FIFO, Now the SMSC memory is free
        while ( _inpw(MMU_CMD_REG ) & 1 ) // 1 is MC_BUSY
        {
            //when the last release is completed
            ssync();
        }

        _outpw(MMU_CMD_REG, MC_RELEASE);
        _outpw(MMU_CMD_REG, MC_RSTTXFIFO);

        goto TX_RETRY_LAN;
      }
   }

   // or YES - I got my memory now...
   cptr->LAN_state.alloc_success = 1;

   return (LAN91C111_hardware_send_packet(cptr));
}



/******************************************************************************
 * LAN Interrupt Handler
 *****************************************************************************/
int LAN91C111_InterruptHandler(ADI_ETHER_LAN91C111_DATA *dev)
{
      /*
       * This is the main routine of the driver, to handle the
       * net_device when it needs some attention.
       * So:
       *  first, save state of the chipset
       *  branch off into routines to handle each case,
       *  and acknowledge each to the interrupt register
       *  and finally restore state.
       */
      volatile unsigned char    stat;
      volatile unsigned short   card_status;
      volatile unsigned char    mask;
      volatile unsigned short   saved_bank;
      volatile unsigned short   saved_pointer;
      ADI_DCB_RESULT            result;


      // save current bank
      saved_bank = _inpw(BSR_REG);

      // switch to bank 2 and save pointer
      _outpw(BSR_REG, 2);
      saved_pointer = _inpw(PTR_REG);

      // read IRQ status register
      mask = _inp(IM_REG);

      // disable all LAN IRQs
      _outp(IM_REG, 0x0);

      // read the status flag and mask it
      stat = _inp(INT_REG)& mask;
      while (stat)
      {

         if (stat & IM_RCV_INT)
         {
            // store debug
            dev->LAN_state.last_IRQ_serviced = IM_RCV_INT;
            // Got a packet(s), receive them
            LAN91C111_rcv(dev);
            if (dev->m_RxDequeuedHead != NULL) {
                if (dev->DCBHandle!=NULL) {
                    result = adi_dcb_Post(dev->DCBHandle,ik_ivg12,dev->DMCallback,dev->DeviceHandle,ADI_ETHER_EVENT_FRAME_RCVD,dev->m_RxDequeuedHead->CallbackParameter);
                } else {
                    (dev->DMCallback)(dev->DeviceHandle,ADI_ETHER_EVENT_FRAME_RCVD,dev->m_RxDequeuedHead->CallbackParameter);
                    result = ADI_DCB_RESULT_SUCCESS;
                }
                if (result == ADI_DCB_RESULT_SUCCESS) {
                    //## what happens if a packet is received while in the callback
                        dev->m_RxDequeuedHead = NULL;
                    dev->m_RxDequeuedTail = NULL;
                dev->m_RxDequeuedCount = 0;
                }
            }

         }
         else if (stat & IM_TX_INT )
         {
            // store debug
            dev->LAN_state.last_IRQ_serviced = IM_TX_INT;
            LAN91C111_tx(dev);
            // Acknowledge the interrupt
            _outp(INT_REG, IM_TX_INT);
         }
         else if (stat & IM_TX_EMPTY_INT )
         {
            // store debug
            dev->LAN_state.last_IRQ_serviced = IM_TX_EMPTY_INT;

            _outpw(BSR_REG, 0);
            card_status = _inpw(COUNTER_REG );

#if LAN91C111_LOG_NET_ERRORS
            // multiple collisions
            dev->Stats->cEMAC_TX_CNT_ABORTC += card_status & 0xF;
            card_status >>= 4;
            // multiple collisions
            dev->Stats->cEMAC_TX_CNT_ABORTC += card_status & 0xF;
#endif

            _outpw(BSR_REG, 2);
            // Acknowledge the interrupt
            _outp(INT_REG, IM_TX_EMPTY_INT);
            mask &= ~IM_TX_EMPTY_INT;

#if LAN91C111_LOG_NET_ERRORS
            // Delayed tx packets
            dev->Stats->cEMAC_TX_CNT_DEFER += dev->LAN_state.packets_waiting;
#endif
            // clear state of waiting packets
            dev->LAN_state.packets_waiting = 0;

         }
         else if (stat & IM_ALLOC_INT )
         {
            // store debug
            dev->LAN_state.last_IRQ_serviced = IM_ALLOC_INT;

            // allocation IRQ
            dev->LAN_state.alloc_success = 1;

            // clear this interrupt so it doesn't happen again
            mask &= ~IM_ALLOC_INT;

            /* enable xmit interrupts based on this */
            mask |= ( IM_TX_EMPTY_INT | IM_TX_INT );

         }
         else if (stat & IM_RX_OVRN_INT )
         {
            // store debug
            dev->LAN_state.last_IRQ_serviced = IM_RX_OVRN_INT;

#if LAN91C111_LOG_NET_ERRORS
            //dev->stats.rx_errors++;
            //dev->stats.rx_fifo_errors++; // XXX: FIFO errors?>
            dev->Stats->cEMAC_RX_CNT_IRL++;
#endif
            // Acknowledge the interrupt
            _outp(INT_REG, IM_RX_OVRN_INT);

         }
         else if (stat & IM_EPH_INT )
         {
            // currently unsupported IRQ
            // store debug
            dev->LAN_state.last_IRQ_serviced = IM_EPH_INT;

         }
         else if (stat & IM_MDINT )
         {
            // store debug
            dev->LAN_state.last_IRQ_serviced = IM_MDINT;
            LAN91C111_phy_interrupt(dev);
            // Acknowledge the interrupt
            _outp(INT_REG, IM_MDINT);

         }
         else if (stat & IM_ERCV_INT )
         {
            // store debug
            // currently unsupported IRQ
            dev->LAN_state.last_IRQ_serviced = IM_ERCV_INT;
            // Acknowledge the interrupt
            _outp(INT_REG, IM_ERCV_INT);
         }

         // read the status flag and mask it
         stat = _inp(INT_REG)& mask;
      }

      //restore found register states
      _outpw(BSR_REG, 2);
      _outp(IM_REG, mask);

      _outpw(PTR_REG, saved_pointer);
      _outpw(BSR_REG, saved_bank);


      // Acknowledge interrupt
      ACK_LAN_INT(dev->pf);
     return 1;
}

/******************************************************************************
 * LAN Initialization routine
 *****************************************************************************/

static SetPhy(ADI_ETHER_LAN91C111_DATA *dev)
{
    u16 phydat;

    phydat = LAN91C111_read_phy_register(PHY_CNTL_REG);
    LAN91C111_write_phy_register(PHY_CNTL_REG, 0x8000);   // get phy from reset
    phydat = 0x8000;
    while (phydat&0x8000) {
        phydat = LAN91C111_read_phy_register(PHY_CNTL_REG);
    }
    //      Program PHY registers
    phydat = 0;
    int pha;

    phydat = 0;
    if (dev->Auto) {
        phydat |= 0x1000;       // enable auto negotiation
    } else {
        if (dev->FullDuplex) {
            phydat |= (1 << 8);     // full duplex
        } else {
            phydat &= (~(1 << 8));  // half duplex
        }
        if (dev->Port100) {
            phydat |= (1 << 13);    // 100 Mbps
        } else {
            phydat &= (~(1 << 13)); // 10 Mbps
        }
    }
    LAN91C111_write_phy_register(PHY_CNTL_REG, phydat);
    phydat = LAN91C111_read_phy_register(PHY_CNTL_REG);
    if (dev->Loopback) {
        phydat |= (1 << 14);    // enable TX->RX loopback
    }
    
    /* Set the power down flag if enabled */
    if (dev->bPhyPowerDown) {
        phydat |= PHY_CNTL_PDN;
    }

    LAN91C111_write_phy_register(PHY_CNTL_REG, phydat);

}


static int  StartMac(ADI_ETHER_LAN91C111_DATA *dev)
{
      int i;
      short wtmp, wstat;

       //set PF configuration
       SetDevicePFConfig(dev);

      //init SMSC
      _outpw(BSR_REG, 0);                             // select bank 0
      _outpw(RCR_REG, RCR_SOFTRST);                   // issue soft reset
      LAN91C111_write_phy_register(PHY_CNTL_REG, 0x8000);   // get phy from reset
      _outpw(RCR_REG, 0);                             // clear reset

      smsc_sleep(250);

      // Hook interrupt handler. Same interrupt for all RXIVG/TXIVG/ERRIVG
      //
      adi_int_CECHook(dev->RXIVG,(ADI_INT_HANDLER_FN)LAN91C111_InterruptHandler,&EtherDev,FALSE);


       // Enable interrupts in SIC PFB flag pin is mapped to lan interrupt
       //
#if (defined(__ADSPBF533__) || defined(__ADSPBF538__))
      adi_int_SICSetIVG(ADI_INT_PFB,dev->RXIVG);
      adi_int_SICEnable(ADI_INT_PFB);
#endif /* __ADSPBF533__ || __ADSPBF538__ */

#ifdef __ADSPBF561__
      // map the lan interrupt to PFB.
      adi_int_SICSetIVG(ADI_INT_PF0_15_B,dev->RXIVG);
      adi_int_SICEnable(ADI_INT_PF0_15_B);
#endif /* __ADSPBF561__ */


#ifdef __ADSPBF537__
     // Enable interrupts in SIC PFB flag pin is mapped to lan interrupt
      adi_int_SICSetIVG(ADI_INT_WATCHDOG_PORTF_B,dev->RXIVG);
      adi_int_SICEnable(ADI_INT_WATCHDOG_PORTF_B);
#endif /* __ADSPBF537__ */

      // disable TX and RX functionality
      _outpw(RCR_REG, 0);                             // clear RX
      _outpw(TCR_REG, 0);                             // clear TX

      // init MAC address
      _outpw(BSR_REG, 1);                             // select bank 1
      _outpw(CFG_REG, CFG_EPH_POWER_EN);              // get out of low power mode
      _outpw(CFG_REG, _inpw(CFG_REG)|CFG_NOWAIT);     // set NO_WAIT

      if(memcmp(dev->phyAddr,"\x00\x00\x00\x00\x00\x00",6))
      {
        //set MAC Addr
        wtmp = (((dev->phyAddr[1])<<8)|(dev->phyAddr[0]));
        _outpw(ADDR0_REG, wtmp);
        wtmp = (((dev->phyAddr[3])<<8)|(dev->phyAddr[2]));
        _outpw(ADDR1_REG, wtmp);
        wtmp = (((dev->phyAddr[5])<<8)|(dev->phyAddr[4]));
        _outpw(ADDR2_REG, wtmp);
      }
      else // get mac from the EEPROM
      {
            wtmp = _inpw(ADDR0_REG);
            dev->phyAddr[0] = wtmp & 0x00FF;
            dev->phyAddr[1] = wtmp >>8 & 0x00FF;

              wtmp = _inpw(ADDR1_REG);
            dev->phyAddr[2] = wtmp & 0x00FF;
            dev->phyAddr[3] = wtmp >>8 & 0x00FF;

            wtmp = _inpw(ADDR2_REG);
            dev->phyAddr[4] = wtmp & 0x00FF;
            dev->phyAddr[5] = wtmp >>8 & 0x00FF;
      }

      // release all pending packets
      _outpw(CTL_REG, _inpw(CTL_REG)|CTL_AUTO_RELEASE);

      // reset MMU
      _outpw(BSR_REG, 2);
      _outpw(MMU_CMD_REG, MC_RESET);

      // disable all IRQs
      _outp(IM_REG, 0);

      // auto negotiate + LEDs
      _outpw(BSR_REG, 0);
      _outpw(RPC_REG, RPC_ANEG | 0x80);

      // turn off isolation mode and also set the power down flag if enabled 
      LAN91C111_write_phy_register(PHY_CNTL_REG, 0x3000 | (dev->Loopback?PHY_CNTL_LPBK:0) | (dev->bPhyPowerDown?PHY_CNTL_PDN:0));

      // read PHY_STAT once due to update latency
      LAN91C111_read_phy_register(PHY_STAT_REG);

      i= 250;
      while (i>0) {
        wtmp = LAN91C111_read_phy_register(PHY_STAT_REG);
        if (wtmp & PHY_STAT_LINK) break; // the link is up
        smsc_sleep(100);
        i--;
      }



#if LAN91C111_DBG
      printf("\n");
      wtmp = LAN91C111_read_phy_register(PHY_CNTL_REG);
      printf("PHY_CNTL_REG: 0x%x\n", wtmp);

      wtmp = LAN91C111_read_phy_register(PHY_STAT_REG);
      printf("PHY_STAT_REG: 0x%x\n", wtmp);

      wtmp = LAN91C111_read_phy_register(PHY_ID1_REG);
      printf("PHY_ID1_REG:  0x%x\n", wtmp);

      wtmp = LAN91C111_read_phy_register(PHY_ID2_REG);
      printf("PHY_ID2_REG:  0x%x\n", wtmp);

      wtmp = LAN91C111_read_phy_register(PHY_AD_REG);
      printf("PHY_AD_REG:   0x%x\n", wtmp);

      wtmp = LAN91C111_read_phy_register(PHY_RMT_REG);
      printf("PHY_RMT_REG:  0x%x\n", wtmp);

      wtmp = LAN91C111_read_phy_register(PHY_CFG1_REG);
      printf("PHY_CFG1_REG: 0x%x\n", wtmp);

      wtmp = LAN91C111_read_phy_register(PHY_CFG2_REG);
      printf("PHY_CFG2_REG: 0x%x\n", wtmp);

      wtmp = LAN91C111_read_phy_register(PHY_INT_REG);
      printf("PHY_INT_REG:  0x%x\n", wtmp);

      wtmp = LAN91C111_read_phy_register(PHY_STAT_REG);
      printf("PHY_STAT_REG: 0x%x\n", wtmp);
#endif //LAN91C111_DBG

      // check ANEG bits
      wtmp = LAN91C111_read_phy_register(PHY_STAT_REG);
      if ((wtmp & (PHY_STAT_LINK|PHY_STAT_ANEG_ACK)) == (PHY_STAT_LINK|PHY_STAT_ANEG_ACK))
      {   // success
         wstat = LAN91C111_read_phy_register(PHY_INT_REG);
#if LAN91C111_DBG
         printf("\nANEG success, Status = 0x%x", wstat);
#endif

         // read transmit register
         wtmp = _inpw(TCR_REG);

         // check for full duplex FDX
         if ((wstat & PHY_INT_DPLXDET) == PHY_INT_DPLXDET)
            wtmp |= TCR_SWFDUP;
         else
            wtmp &= ~TCR_SWFDUP;

         // write FDX result
         _outpw(TCR_REG, wtmp);
      }
      else
      {   // fail
         wstat = LAN91C111_read_phy_register(PHY_INT_REG);
#if LAN91C111_DBG
         printf("\nANEG failed, Status = 0x%x", wstat);
#endif
      }

      // device is started and active now.
      dev->Started = true;

      // enable RX and TX
      _outpw(BSR_REG, 0);
      _outpw(TCR_REG, _inpw(TCR_REG)|TCR_DEFAULT);  // enable TX
      _outpw(RCR_REG, _inpw(RCR_REG)|RCR_DEFAULT);  // enable RX

      // enable IRQs
      _outpw(BSR_REG, 2);
      _outp(IM_REG, SMC_INTERRUPT_MASK);                 // standard IRQ mask
      _outpw(BSR_REG, 0);
    return 1;
}

#pragma optimize_for_speed
static int QueueNewRcvFrames(ADI_ETHER_LAN91C111_DATA *dev,ADI_ETHER_BUFFER *bufs)
{
      ADI_ETHER_BUFFER *tmp_q_ele;
      void *CriticalResult;

      if (bufs == NULL)
         return (0);

      ENTER_CRITICAL_REGION();
     //CriticalResult = adi_int_EnterCriticalRegion(dev->CriticalData);

      tmp_q_ele = bufs;
      tmp_q_ele->ProcessedFlag = 0;

      if (dev->m_RxEnqueuedCount)
         dev->m_RxEnqueuedTail->pNext = (ADI_ETHER_BUFFER*)tmp_q_ele;
      else
         dev->m_RxEnqueuedHead = tmp_q_ele;

      //We have attached one
      dev->m_RxEnqueuedCount++;

      //Now look for rest
      while (tmp_q_ele->pNext != NULL)
      {
         // increment the count
         dev->m_RxEnqueuedCount++;
         tmp_q_ele = (ADI_ETHER_BUFFER*)tmp_q_ele->pNext;
         tmp_q_ele->ProcessedFlag = 0;
      }

      //Amd update tail
      dev->m_RxEnqueuedTail = tmp_q_ele;
      dev->RxStarted = 1;

      EXIT_CRTICIAL_REGION();
     //adi_int_ExitCriticalRegion(CriticalResult);
      return 1;
}

static int QueueNewXmtFrames(ADI_ETHER_LAN91C111_DATA *dev,ADI_ETHER_BUFFER *bufs)
{
    int mr = 0;
    ADI_ETHER_BUFFER *tmp_q_ele;
    ADI_DCB_RESULT   result;

      // if the incoming element is NULL throw an error
      if (bufs == NULL)
         return (0);

      ENTER_CRITICAL_REGION();

     //CriticalResult = adi_int_EnterCriticalRegion(dev->CriticalData);

      tmp_q_ele = bufs;
      tmp_q_ele->ProcessedFlag = 0;

      if (dev->m_TxEnqueuedCount)
         dev->m_TxEnqueuedTail->pNext = (ADI_ETHER_BUFFER*)tmp_q_ele;
      else
         dev->m_TxEnqueuedHead = tmp_q_ele;

      //We have attached one
      dev->m_TxEnqueuedCount++;

      //Now look for rest
      while (tmp_q_ele->pNext != NULL)
      {
         // increment the count
         dev->m_TxEnqueuedCount++;
         tmp_q_ele = (ADI_ETHER_BUFFER*)tmp_q_ele->pNext;
         tmp_q_ele->ProcessedFlag = 0;
      }

      //And update tail
      dev->m_TxEnqueuedTail = tmp_q_ele;
      {
         int cont = 1;
         while(cont)
         {
            unsigned char status;

            status = LAN91C111_wait_to_send_packet(dev);

            if (status == LAN91C111_TX_RETRY)
            {
               /* Keep the packet in the queue and retry next time */
               cont = 0;
               mr =0;
               dev->Stats->cEMAC_TX_CNT_DEFER++;
            }
            else  // If transmission attempt is an unrecoverable error or if it completed successfully
            {

                dev->Stats->cEMAC_TX_CNT_OK++;

                tmp_q_ele = dev->m_TxEnqueuedHead;

               /* Change the Enqueued head */
               dev->m_TxEnqueuedHead = (ADI_ETHER_BUFFER*)tmp_q_ele->pNext;
               dev->m_TxEnqueuedCount--;
               if (dev->m_TxEnqueuedCount == 0)
               {
                  dev->m_TxEnqueuedTail = NULL;
                  cont = 0;
               }

               /* Add the packet to dequeued list*/
               if (dev->m_TxDequeuedCount)
                  dev->m_TxDequeuedTail->pNext = (ADI_ETHER_BUFFER*)tmp_q_ele;
               else
                  dev->m_TxDequeuedHead = tmp_q_ele;

               //No matter what this is also the tail.
               dev->m_TxDequeuedTail = tmp_q_ele;
               //And tail->next should point to NULL
               tmp_q_ele->pNext = NULL;
               dev->m_TxDequeuedCount++;

            }
         }
      }
     if (dev->DCBHandle != NULL) {
        result = adi_dcb_Post(dev->DCBHandle,ik_ivg13,dev->DMCallback,dev->DeviceHandle,ADI_ETHER_EVENT_FRAME_XMIT,dev->m_TxDequeuedHead->CallbackParameter);
     } else {
        (dev->DMCallback)(dev->DeviceHandle,ADI_ETHER_EVENT_FRAME_XMIT,dev->m_TxDequeuedHead->CallbackParameter);
        result = ADI_DCB_RESULT_SUCCESS;
     }
     //if (result == ADI_DCB_RESULT_SUCCESS) {
        //## what happens if a packet is trassmitted while in the callback
        dev->m_TxDequeuedHead = NULL;
        dev->m_TxDequeuedTail = NULL;
        dev->m_TxDequeuedCount = 0;
     //}
      EXIT_CRTICIAL_REGION();

      return mr;
}

/******************************************************************************
 * Get MAC address in the SMSC
 *****************************************************************************/
static void GetMacAddr(unsigned char *mac_addr)
{
    volatile unsigned short addr;
    volatile unsigned short page;
    page = _inpw(BSR_REG);

    _outpw(BSR_REG, 1);
    addr = _inpw(ADDR0_REG); // get MAC addr
    mac_addr[0] = addr & 0xFF;
    mac_addr[1] = addr >> 8;

    addr = _inpw(ADDR1_REG); // get MAC addr
    mac_addr[2] = addr & 0xFF;
    mac_addr[3] = addr >> 8;

    addr = _inpw(ADDR2_REG); // get MAC addr
    mac_addr[4] = addr & 0xFF;
    mac_addr[5] = addr >> 8;
    _outpw(BSR_REG, page);
    return;
}
/******************************************************************************
 * Set MAC address in the SMSC
 *****************************************************************************/
static void SetupMacAddr(unsigned char *mac_addr)
{
   unsigned short page;
   short wtmp;

   // save old page
   page = _inpw(BSR_REG);

   // select bank 1
   _outpw(BSR_REG, 1);

   // Set EEPROM Store
   _outpw(CTL_REG,CTL_EEPROM_SELECT | CTL_STORE);

    wtmp = (((mac_addr[1])<<8)|(mac_addr[0]));
    _outpw(ADDR0_REG, wtmp);
    wtmp = (((mac_addr[3])<<8)|(mac_addr[2]));
    _outpw(ADDR1_REG, wtmp);
    wtmp = (((mac_addr[5])<<8)|(mac_addr[4]));
    _outpw(ADDR2_REG, wtmp);


   // restore bank
   _outpw(BSR_REG, page);

   return;
}

/******************************************************************************
 *
 * Configures the PF/IMASK/IAR settings associated with the driver.
 *
 *****************************************************************************/
static int  SetDevicePFConfig(ADI_ETHER_LAN91C111_DATA *dev)
{
    // Default PF configuration is PF9
    //
    if(dev->pf == 0) {
#if defined(__ADSPBF537__)
        dev->pf = PF5;
#elif defined(__ADSPBF538__)
        dev->pf = PF0;
#else    /* for 533 & 561 PF9 is used */
        dev->pf = PF9;
#endif
         }

    // for BF533,BF538 Ezkit with USB LAN extender
#if (defined(__ADSPBF533__)|| defined(__ADSPBF538__))
    // Configure PF as input, clear PF bit
    *pFIO_DIR     &= ~(dev->pf);
    *pFIO_POLAR   &= ~(dev->pf);
    *pFIO_MASKB_S = dev->pf;
    // Enable input buffering for PF9/PF0
    *pFIO_INEN    |= dev->pf;
#endif /* __ADSPBF533__, __ADSPBF538__ */

    // for BF561 USB LAN extender
#ifdef __ADSPBF561__
    //Configure PF9 as input, clear PF9 bit
    *pFIO0_DIR     &= ~(dev->pf);
    *pFIO0_POLAR   &= ~(dev->pf);
    // drive PF12 low, set PF12 to output mode and clear
    *pFIO0_DIR    |= PF12;
    *pFIO0_FLAG_C  = PF12;
    // configure pf to raise the lan interrupt
    *pFIO0_MASKB_S = dev->pf;
    // enable input buffering
    *pFIO0_INEN    |= dev->pf;
#endif /* __ADSPBF561__ */


#ifdef __ADSPBF537__
    // Clear off any pending interrupts
    *pPORTFIO_DIR          &= ~(dev->pf);
    *pPORTFIO_POLAR        &= ~(dev->pf);
    *pPORTFIO_MASKB_CLEAR   = dev->pf;
    *pPORTFIO_MASKB_SET     = dev->pf;
    *pPORTFIO_INEN      |= dev->pf;
#endif /* __ADSPBF537__ */

     return 1;

}

/*********************************************************************
*
*   Function:       pddOpen
*
*   Description:    Opens the Network Driver and does initialization.
*
*********************************************************************/

static u32 adi_pdd_Open(                // Open a device
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,  // device manager handle
    u32                     DeviceNumber,   // device number
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,   // device handle
    ADI_DEV_PDD_HANDLE      *pPDDHandle,    // pointer to PDD handle location
    ADI_DEV_DIRECTION       Direction,      // data direction
    void            *pCriticalRegionArg,        // critical region imask storage location
    ADI_DMA_MANAGER_HANDLE  DMAHandle,      // handle to the DMA manager
    ADI_DCB_HANDLE          DCBHandle,      // callback handle
    ADI_DCB_CALLBACK_FN     DMCallback      // device manager callback function
)
{
    u32             Result;     // return value
    void            *CriticalResult;
    ADI_ETHER_LAN91C111_DATA *dev= &EtherDev;


    // check for errors if required
#ifdef ADI_ETHER_ERROR_CHECKING_ENABLED
    if (DeviceNumber > 0) {     // check the device number
        return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
    }
#endif

    // insure the device the client wants is available
    Result = ADI_DEV_RESULT_DEVICE_IN_USE;
    //CriticalResult = adi_int_EnterCriticalRegion(pCriticalRegionArg);

    // Open the device
    if(!dev->Open)
    {
        // initialize the device settings
        memset(dev,0,sizeof(ADI_ETHER_LAN91C111_DATA));

        dev->CriticalData = pCriticalRegionArg;
        dev->DeviceHandle = DeviceHandle;
        dev->DCBHandle = DCBHandle;
        dev->DMCallback = DMCallback;
        dev->Direction = Direction;
        dev->Started = false;
        dev->bEnableGracefulShutdown = false;
        dev->bPhyPowerDown = false;

#if ( defined(__ADSPBF533__) || defined(__ADSPBF537__))
        // default ivg levels
        dev->RXIVG  = ik_ivg8;
        dev->TXIVG  = ik_ivg8;
        dev->ERRIVG = ik_ivg8;
#endif /* __ADSPBF533__ */

#ifdef __ADSPBF561__
        // default ivg levels
        dev->RXIVG  = ik_ivg11;
        dev->TXIVG  = ik_ivg11;
        dev->ERRIVG = ik_ivg11;
        // set bit 7 to zero - 32bit access to async-bank3
        *pEBIU_AMGCTL &= 0xFF7F;
#endif /* __ADSPBF561__ */

#if defined(__ADSPBF538__)
        // default ivg levels for PF interrupt B
        dev->RXIVG  = ik_ivg12;
        dev->TXIVG  = ik_ivg12;
        dev->ERRIVG = ik_ivg12;
#endif /* __ADSPBF538__ */

        // This is required as after opening the device we will query
        // the EEPROM to get the MAC (Async Bank-3)
        //
            *pEBIU_AMGCTL   |= 0xF;

        dev->Open = true;
        Result = ADI_DEV_RESULT_SUCCESS;
    }

    // exit critical region
    //
    //adi_int_ExitCriticalRegion(CriticalResult);

    if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);

    *pPDDHandle = (ADI_DEV_PDD_HANDLE *)&EtherDev;
    return(ADI_DEV_RESULT_SUCCESS);
}


/*********************************************************************
*
*   Function:       pddRead
*
*   Description:    Gives list of read buffers to the driver
*
*********************************************************************/
static u32 adi_pdd_Read(        // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
)
{
    u32         Result;             // return value
    ADI_ETHER_LAN91C111_DATA *dev = (ADI_ETHER_LAN91C111_DATA *)PDDHandle;

    if(dev->bEnableGracefulShutdown == true)
        return (ADI_DEV_RESULT_FAILED);
    // check for errors if required
#if defined(ADI_ETHER_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
    if (BufferType != DEV_1D) {
        return (ADI_DEV_BUFFER_TYPE_INCOMPATIBLE);
    }
#endif

    QueueNewRcvFrames((void*)PDDHandle,(ADI_ETHER_BUFFER*)pBuffer);
    return(ADI_DEV_RESULT_SUCCESS);
}


/*********************************************************************
*
*   Function:       pddClose
*
*   Description:    Closes the driver and releases any memory
*
*********************************************************************/

static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle    // PDD handle
)
{
    ADI_ETHER_LAN91C111_DATA *dev = (ADI_ETHER_LAN91C111_DATA *)PDDHandle;
    u32         Result = ADI_DEV_RESULT_SUCCESS;                // return value
    bool active=true;

    // check for errors if required
#if defined(ADI_ETHER_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    dev->Closing = true;
    dev->Open = false;
    if (dev->Started) {
        dev->Started = false;
        // wait for the current frames to complete
    }
    // return
    return(Result);
}

/*********************************************************************
*
*   Function:       pddWrite
*
*   Description:    Sends packet over the physical channel
*
*********************************************************************/
static u32 adi_pdd_Write(           // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,               // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType,             // buffer type
    ADI_DEV_BUFFER *pBuffer                     // pointer to buffer
)
{
    u32         Result;             // return value
    ADI_ETHER_LAN91C111_DATA *dev = (ADI_ETHER_LAN91C111_DATA *)PDDHandle;

    if(dev->bEnableGracefulShutdown == true)
        return (ADI_DEV_RESULT_FAILED);
    // check for errors if required
#if defined(ADI_ETHER_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
    if (BufferType != DEV_1D) {
        return (ADI_DEV_BUFFER_TYPE_INCOMPATIBLE);
    }
    if (!dev->Start) {
        return ADI_DEV_RESULT_INVALID_SEQUENCE;
    }
#endif
    QueueNewXmtFrames((void*)PDDHandle,(ADI_ETHER_BUFFER*)pBuffer);
    return(ADI_DEV_RESULT_SUCCESS);
}

/************************************************************
 *   Function: ShutDownDriver( void )
 *
 * Description:
 *      Shutdown the driver more gracefully than adi_pdd_Close()
 *      It will wait until all send and recevied queue be processed
 *      before return
 *
 ***************************************************************/

static u32 ShutDownDriver(
                    ADI_DEV_PDD_HANDLE  PDDHandle           // PDD handle
                    )
{
    ADI_ETHER_LAN91C111_DATA *dev = (ADI_ETHER_LAN91C111_DATA *)PDDHandle;
    u32         Result = ADI_DEV_RESULT_SUCCESS;                // return value
    bool active=true;
    u32 uInterruptStatus = cli();

    // check for errors if required
#if defined(ADI_ETHER_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    dev->Closing = true;
    dev->Open    = false;

    if (dev->Started)
    {
        dev->Started = false;

        /* set up the flag to block read/write command */
        dev->bEnableGracefulShutdown = true;

        #if (defined(__ADSPBF533__) || defined(__ADSPBF538__))
            adi_int_SICDisable(ADI_INT_PFB);
        #endif /* __ADSPBF533__ || __ADSPBF538__ */

        #ifdef __ADSPBF561__
            adi_int_SICDisable(ADI_INT_PF0_15_B);
        #endif /* __ADSPBF561__ */

        #ifdef __ADSPBF537__
        adi_int_SICDisable(ADI_INT_WATCHDOG_PORTF_B);
        #endif /* __ADSPBF537__ */

            // switch to bank 2
        _outpw(BSR_REG, 2);

            // disable all SMSC LAN IRQs
            _outp(IM_REG, 0x0);

        // reset configuration register
               _outpw(BSR_REG, 1);
               _outpw(CFG_REG, 0x0);

        // unhook the interrupt handler
            adi_int_CECUnhook(dev->RXIVG,(ADI_INT_HANDLER_FN)LAN91C111_InterruptHandler,&EtherDev);
    }

    sti(uInterruptStatus);
    // return
    return(Result);

}
/*********************************************************************
*
*   Function:       GetMultiCastMAC
*
*   Description:
*                 Creates the multicast mac address given the group ip
*                 address the incoming pMultiCastMac must be greater than
*                 or equal to 6 bytes
*
*********************************************************************/
static void GetMultiCastMAC(u32 GroupIpAddress, char *pMultiCastMac)
{
char *p=(char*)&GroupIpAddress;

    // compute the destination multi cast mac address
    //
    memset(pMultiCastMac,0,6);

    // copy the ip address
    memcpy((pMultiCastMac+2),p,4);

    // convert the incoming group ip address to multi cast mac address
    // 0-23 bits are fixed we get the rest from the group ipaddress
    //
    *(pMultiCastMac + 0) = 0x01;
    *(pMultiCastMac + 1) = 0x00;
    *(pMultiCastMac + 2) = 0x5E;
    *(pMultiCastMac + 3) &= 0x7F;

    return;
}

/*********************************************************************
*
*   Function:       GetMultiCastHashBin
*
*   Description:    Returns the hash bin index given the Multicast MAC
*
*********************************************************************/
#define CRC32_POLYNOMIAL 0xEDB88320
static unsigned int GetMultiCastHashBin(char *pMultiCastMac,const int length)
{
s32  i,j;
bool isXorTrue;
u32  crc32 = ~0L;
s32  binPosition=-1;
u8   data;

     for(i=0; i < length; i++)
     {
        // get next byte from the data
        data = (u8)pMultiCastMac[i];

        // check all bits in a byte
        for(j=8; j >0;j--)
        {
            isXorTrue = (crc32 ^ data)& 0x1;
            crc32 >>= 1;

             if(isXorTrue)
                crc32 ^= CRC32_POLYNOMIAL;

            // get the next bit
            data >>=1;
        }
     }

     // swap and get 6 bits
     binPosition = ((crc32&0x01)<<5)| ((crc32&0x02)<<3) |
                   ((crc32&0x04)<<1)| ((crc32&0x08)>>1) |
                   ((crc32&0x10)>>3)| ((crc32&0x20)>>5);

     return (binPosition);
}


//
// sets the bit given by 'bp' in the multicast table
//
#define SET_MULTICAST_TABLE(t,bp) do {             \
     volatile short w = _inpw((unsigned int)t);    \
                    w |= (1 << (short)bp);         \
    _outpw((unsigned int)t,w);                     \
}while(0)

//
// clears the bit given by 'bp' in the multicast table
//
#define CLEAR_MULTICAST_TABLE(t,bp) do {             \
     volatile short w = _inpw((unsigned int)t);      \
                    w &= ~((short)(1 << (short)bp)); \
    _outpw((unsigned int)t,w);                       \
}while(0)

/*********************************************************************
*
*   Function:       AddMultiCastMACFilter
*
*   Description:    Computes the multicast hash bit position and sets
*                   or clears for a given multicast group address.
*
*********************************************************************/

static u32 AddMultiCastMACfilter(u32 GroupIpAddress,bool bAddAddress)
{
char MultiCastMac[6];
char *p = (char*)&GroupIpAddress;
int HashBinIndex,BitPostion,TableNumber;
volatile unsigned short save_bank;
unsigned int uInterruptMask;

    // Get the multicast MAC address for the given address
    GetMultiCastMAC(GroupIpAddress,&MultiCastMac[0]);

    // Get the hash index for the multicast address
    HashBinIndex = GetMultiCastHashBin((char*)&MultiCastMac[0],6);

    // compute the bit position
    BitPostion = (HashBinIndex & 0x7);

    // compute the bit position and the table number (MT0-MT7)
    TableNumber = ((HashBinIndex >> 3) & 0x7);

    uInterruptMask = cli();

        // Save bank
    save_bank = _inpw(BSR_REG);

     // switch to bank-3
    _outpw(BSR_REG,3);

    if(bAddAddress)
    {
        switch(TableNumber)
        {
            case MC_TABLE0:
                    SET_MULTICAST_TABLE(MCAST_REG1,BitPostion);
                    break;
            case MC_TABLE1:
                    SET_MULTICAST_TABLE(MCAST_REG1,(BitPostion+8));
                    break;
            case MC_TABLE2:
                    SET_MULTICAST_TABLE(MCAST_REG2,BitPostion);
                    break;
            case MC_TABLE3:
                    SET_MULTICAST_TABLE(MCAST_REG2,(BitPostion+8));
                    break;
            case MC_TABLE4:
                    SET_MULTICAST_TABLE(MCAST_REG3,BitPostion);
                    break;
            case MC_TABLE5:
                    SET_MULTICAST_TABLE(MCAST_REG3,(BitPostion+8));
                    break;
            case MC_TABLE6:
                    SET_MULTICAST_TABLE(MCAST_REG4,BitPostion);
                    break;
            case MC_TABLE7:
                    SET_MULTICAST_TABLE(MCAST_REG4,(BitPostion+8));
                    break;
            default:
                    break;
        }
    }
    else // remove the address
    {
        switch(TableNumber)
        {
            case MC_TABLE0:
                    CLEAR_MULTICAST_TABLE(MCAST_REG1,BitPostion);
                    break;
            case MC_TABLE1:
                    CLEAR_MULTICAST_TABLE(MCAST_REG1,(BitPostion+8));
                    break;
            case MC_TABLE2:
                    CLEAR_MULTICAST_TABLE(MCAST_REG2,BitPostion);
                    break;
            case MC_TABLE3:
                    CLEAR_MULTICAST_TABLE(MCAST_REG2,(BitPostion+8));
                    break;
            case MC_TABLE4:
                    CLEAR_MULTICAST_TABLE(MCAST_REG3,BitPostion);
                    break;
            case MC_TABLE5:
                    CLEAR_MULTICAST_TABLE(MCAST_REG3,(BitPostion+8));
                    break;
            case MC_TABLE6:
                    CLEAR_MULTICAST_TABLE(MCAST_REG4,BitPostion);
                    break;
            case MC_TABLE7:
                    CLEAR_MULTICAST_TABLE(MCAST_REG4,(BitPostion+8));
                    break;
            default:
                    break;
        }
    }

    // restore back the bank
    _outpw(BSR_REG,save_bank);

    sti(uInterruptMask);

    return(ADI_DEV_RESULT_SUCCESS);
}

/*********************************************************************
*
*   Function:       pddControl
*
*   Description:    List of I/O control commands to the driver
*
*********************************************************************/
static u32 adi_pdd_Control(         // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE PDDHandle,               // PDD handle
    u32 Command,                    // command ID
    void *pArg                              // pointer to argument
)
{
    u32         Result;             // return value
    ADI_ETHER_LAN91C111_DATA *dev = (ADI_ETHER_LAN91C111_DATA *)PDDHandle;
    ADI_ETHER_MEM_SIZES *msizes;
    ADI_ETHER_SUPPLY_MEM *memsup;
    int maxbuf,i;
    u32 *prefix;
    ADI_ETHER_BUFFER_COUNTS *bufcnts;
    ADI_ETHER_IVG_MAPPING *ivgs;
    clock_t ndtime;
    clock_t period;
    clock_t nClocksPerSecond = ((clock_t)CLOCKS_PER_SEC);
    
    u32 fcclk,fsclk,fvco;
    ADI_PWR_RESULT nPwrResult;
    
    /* Get the CPU frequency */
    nPwrResult = adi_pwr_GetFreq(&fcclk,&fsclk,&fvco);
    /* if DDSS fails, nClockPerSecond = CLOCKS_PER_SEC (compile time constant) */
    if (nPwrResult == ADI_PWR_RESULT_SUCCESS)
    {
        nClocksPerSecond = (clock_t)fcclk;
    }


    // check for errors if required
#if defined(ADI_ETHER_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
    if (BufferType != DEV_1D) {
        return (ADI_DEV_BUFFER_TYPE_INCOMPATIBLE);
    }
#endif

    // avoid casts
    Result = ADI_DEV_RESULT_SUCCESS;

    // CASEOF (Command ID)
    switch (Command) {

        case ADI_DEV_CMD_SET_DATAFLOW:
            // enable or disable accordingly
            dev->FlowEnabled = (int)pArg;
            break;


        case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:
            // no we don't want peripheral DMA support
            (*(int *)pArg) = false;
            break;

        case ADI_ETHER_CMD_MEM_SIZES:
            msizes = (ADI_ETHER_MEM_SIZES *)pArg;
            msizes->BaseMemSize = sizeof(ADI_ETHER_STATISTICS_COUNTS);
            msizes->MemPerRecv = 0;
            msizes->MemPerXmit = 0;
            break;

        case ADI_ETHER_CMD_SUPPLY_MEM:
            memsup = (ADI_ETHER_SUPPLY_MEM *)pArg;

            if (memsup->BaseMemLength < sizeof(ADI_ETHER_STATISTICS_COUNTS)) {
                Result = ADI_DEV_RESULT_NO_MEMORY;
            } else {
                dev->Stats = memsup->BaseMem;
                memset(dev->Stats,0,sizeof(ADI_ETHER_STATISTICS_COUNTS));
            }
            break;

        case ADI_ETHER_CMD_GET_MAC_ADDR:
            GetMacAddr((unsigned char *)pArg);
            break;

        case ADI_ETHER_CMD_SET_MAC_ADDR:
            SetupMacAddr((unsigned char *)pArg);
            // store mac in an internal variable
            memcpy(dev->phyAddr,(unsigned char*)pArg,6);
            break;
        case ADI_ETHER_CMD_GET_STATISTICS:
            memcpy(pArg,dev->Stats,sizeof(ADI_ETHER_STATISTICS_COUNTS));
            break;
        case ADI_ETHER_CMD_GET_BUFFER_PREFIX:
            prefix = (u32 *)pArg;
            *prefix = buff_prefix;
            break;
        case ADI_ETHER_CMD_UNPROCESSED_BUFFER_COUNTS:
            bufcnts = (ADI_ETHER_BUFFER_COUNTS *)pArg;
            bufcnts->RcvrBufferCnt = dev->m_RxEnqueuedCount;
            bufcnts->XmitBufferCnt = dev->m_TxEnqueuedCount;
            break;
        case ADI_ETHER_CMD_GET_MIN_RECV_BUFSIZE:
            prefix = (u32 *)pArg;
            *prefix = MAX_RCVE_FRAME;
            break;
        case ADI_ETHER_CMD_USBLAN_USE_IVG:
            ivgs = (ADI_ETHER_IVG_MAPPING *)pArg;
            dev->RXIVG  = (interrupt_kind)ivgs->RxIVG;
            dev->TXIVG  = (interrupt_kind)ivgs->TxIVG;
            dev->ERRIVG = (interrupt_kind)ivgs->ErrIVG;
            break;
        case ADI_ETHER_CMD_USBLAN_USE_PF:
                        dev->pf = (int)pArg;
                        break;
        case ADI_ETHER_CMD_USBLAN_USE_DMA:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            break;
        case ADI_ETHER_CMD_GEN_CHKSUMS:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            break;
        case ADI_ETHER_CMD_SET_SPEED: // Default Auto Negotiate mode
            i = (int)pArg;
            if ((i <= 0) || (i > 2)) {
                Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            } else {
                dev->Port100 = (i == 2);
                dev->Auto = false;
            }
            break;
        case ADI_ETHER_CMD_SET_FULL_DUPLEX:
            i = (int)(pArg);
            dev->FullDuplex = (i!=0);
            dev->Auto = false;
            break;
        case ADI_ETHER_CMD_SET_NEGOTIATE:
            i = (int)(pArg);
            dev->Auto = (i!=0);
            break;
        case ADI_DEV_CMD_SET_DATAFLOW_METHOD:
            break;

        case ADI_ETHER_CMD_SET_LOOPBACK:
            i = (int)(pArg);
            dev->Loopback = (i!=0);
            if (dev->Started) {
                // change the phy
                u16 cur;

                cur = LAN91C111_read_phy_register(PHY_CNTL_REG);
                if (dev->Loopback) {
                    cur |= (1 << 14);   // enable TX->RX loopback
                } else {
                    cur &= (~(1 << 14));
                }
                LAN91C111_write_phy_register(PHY_CNTL_REG,cur);
            }
            break;
        case ADI_ETHER_CMD_SET_PHY_POWER_DOWN:
            i = (int)(pArg);
            dev->bPhyPowerDown = (i!=0);
            if (dev->Started) {
                // change the phy
                u16 cur;

                /* Read the Control Register */
                cur = LAN91C111_read_phy_register(PHY_CNTL_REG);
                /* Set/Clear the power down flag */
                if (dev->bPhyPowerDown) {
                    cur |= PHY_CNTL_PDN;    /* Enable Power Down */
                } else {
                    cur &= (~PHY_CNTL_PDN);
                }
                /* Write the control register */
                LAN91C111_write_phy_register(PHY_CNTL_REG,cur);
                
                /* Power Up Works */
                if (dev->bPhyPowerDown == false)
                {
                    /* Wait 750ms for the device to be ready */
                    period = (nClocksPerSecond/3)*2;
                    ndtime = clock()+period;
                    while (clock()<ndtime);

                    /* MAC and PHY are isolated on power up */
                    cur = LAN91C111_read_phy_register(PHY_CNTL_REG);
                    /* Clear the isolation mode to enable MII to Phy */
                    cur &= (~PHY_CNTL_MII_DIS);
                    LAN91C111_write_phy_register(PHY_CNTL_REG,cur);
                }
            }
            break;
        case ADI_ETHER_CMD_GET_PHY_REGS:
            {
                short *arg = (short *)pArg;
                int ic;

                for (ic=0;ic<=PHY_MASK_REG;ic++) arg[ic] = LAN91C111_read_phy_register(ic);
            }
            break;
        case ADI_ETHER_CMD_BUFFERS_IN_CACHE:
            i = (int)(pArg);
            dev->Cache = (i!=0);
            break;
        case ADI_ETHER_CMD_START:
            if (dev->RxStarted) {
                StartMac(PDDHandle);
            } else {
                Result = ADI_DEV_RESULT_INVALID_SEQUENCE;
            }
            break;
        case ADI_DEV_CMD_TABLE:
            break;

        case ADI_DEV_CMD_END:
            break;

        case ADI_ETHER_CMD_SHUTDOWN_DRIVER:
            ShutDownDriver(PDDHandle);
            break;

        case ADI_ETHER_ADD_MULTICAST_MAC_FILTER:
            {
              u32 MultiCastGroupAddr = (u32)pArg;

              AddMultiCastMACfilter(MultiCastGroupAddr,true);
            }
            break;

        case ADI_ETHER_DEL_MULTICAST_MAC_FILTER:
            {
              u32 MultiCastGroupAddr = (u32)pArg;

              AddMultiCastMACfilter(MultiCastGroupAddr,false);
            }
            break;


        default:
            smsc_sleep(0);
            // we don't understand this command
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;

    }

    // return
    return(Result);
}

/**************************************************************************
 *
 * Lan91c111 entry point
 *
 **************************************************************************/
ADI_DEV_PDD_ENTRY_POINT ADI_ETHER_USBLAN_Entrypoint  = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

