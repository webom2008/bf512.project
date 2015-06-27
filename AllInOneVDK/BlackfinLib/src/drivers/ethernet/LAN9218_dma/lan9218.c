/*****************************************************************************
   
	Copyright (c) 2005, SMSC. All rights reserved.

	Use of this source code is subject to the terms of the SMSC Software
	License Agreement (SLA) under which you licensed this software product.	 
	If you did not accept the terms of the SLA, you are not authorized to use
	this source code. 

	This code and information is provided as is without warranty of any kind,
	either expressed or implied, including but not limited to the implied
	warranties of merchantability and/or fitness for a particular purpose.
	 
	File name   : lan911x.c 
	Description : LAN911x EBOOT driver

	History	    :
		ver 0.1		12/01/05	WH
			First WindowsCE 5.0 EBOOT driver for LAN911x

		ver 0.2		12/16/05	WH
			Work with VMINI

		ver 0.3		3/28/06		WH
			Fixed a bug that failed to reading MAC address from EEPROM

		ver 0.4		06/28/06	MDG
			Ported to EDOM Technologies db1200 (MIPSII au1200)

TODO !!!!! Remove all "DEBUG" statments before entering SCCS/Perforce !!!!!
*****************************************************************************/

/***************************************************************************
 *
 *  Portions Copyright (c) 2006 - 2010 Analog Devices, Inc.
 *
 **************************************************************************/
#include <string.h>             // strchr()
#include "lan9218.h"
#include "mdma_lan9218.h"

#define MAX_RCVE_FRAME         (1560)      // maximum ethernet receive frame size in bytes
#define LAN9218_TX_RETRY       (1)         // insufficient tx memory, queue the packet and try again
#define LAN9218_TX_ERROR       (2)         // unrecoverable error, so drop the packet
#define LAN9218_TX_SUCCESS     (3)         // transmission successful

/* MDMA base address */
#define MDMA_SRC_STREAM0_BASE  (0xFFC00F40)
#define MDMA_DST_STREAM0_BASE  (0xFFC00F00)
#define MDMA_SRC_STREAM1_BASE  (0xFFC00FC0)
#define MDMA_DST_STREAM1_BASE  (0xFFC00F80)
#define MDMA_SRC_STREAM2_BASE  (0xFFC01F40)
#define MDMA_DST_STREAM2_BASE  (0xFFC01F00)
#define MDMA_SRC_STREAM3_BASE  (0xFFC01FC0)
#define MDMA_DST_STREAM3_BASE  (0xFFC01F80)

/* SIC interrupt numbers for MDMA channels */
#define MDMA_STREAM0_INT_NUM   (21)
#define MDMA_STREAM1_INT_NUM   (22)
#define MDMA_STREAM2_INT_NUM   (17)
#define MDMA_STREAM3_INT_NUM   (18)

#define ACK_LAN_INT(pf)         ((*pPINT2_IRQ = pf))
#define ENTER_CRITICAL_REGION() (CriticalHandle=adi_int_EnterCriticalRegion(dev->CriticalData))
#define EXIT_CRTICIAL_REGION()  (adi_int_ExitCriticalRegion(CriticalHandle))

#define FLUSH(P)  {asm volatile("FLUSH[%0++];":"+p"(P));}
#define FLUSHINV(P)  {asm volatile("FLUSHINV[%0++];":"+p"(P));}
#define SIMPLEFLUSHINV(P)  {ssync(); asm volatile("FLUSHINV[%0++];"::"#p"(P)); ssync();}

/* global static data */
static ADI_ETHER_LAN9218_DATA EtherDev={0};
static ADI_ETHER_LAN9218_DATA *pHandle;
static int buff_prefix = 6;
void   *CriticalHandle;

// Ethernet chip SMSC Lan9218 is mapped to Aync memory bank 1
// which is A4-A9 
static ADI_PORTS_DIRECTIVE Ethernet_Ports[] = {
        ADI_PORTS_DIRECTIVE_AMC_A4,
        ADI_PORTS_DIRECTIVE_AMC_A5,
        ADI_PORTS_DIRECTIVE_AMC_A6,
        ADI_PORTS_DIRECTIVE_AMC_A7,
        ADI_PORTS_DIRECTIVE_AMC_A8,
        ADI_PORTS_DIRECTIVE_AMC_A9
};

int SetDevicePFConfig (ADI_ETHER_LAN9218_DATA *dev);

//------------------------------------------------------------------------------
// Local Variables
static const char date_code[] = "040810";
static unsigned int dwPhyAddr;
static unsigned int g_chipRevision;
static unsigned int g_pLAN911x;
void LAN911xEnableInts(void);
void LAN911xDisableInts(void);

#pragma optimize_off 
///////////////////////////////////////////
// delay certain amount of time
///////////////////////////////////////////
void OALStall(unsigned int microSeconds)
{
        int i=0;
        for(i=0; i<microSeconds*100; i++)
                asm("nop;");
}

static void FlushArea(void *start, void *nd)
{
   start = (void *)(((unsigned int)start)&(~31));
   ssync();
   while (start<nd)
     FLUSH(start);
   ssync();
}


static void FlushInvArea(void *start, void *nd)
{
   start = (void *)(((unsigned int)start)&(~31));
   ssync();
   while (start<nd) FLUSHINV(start);
       ssync();
}
//////////////////////////////////////////
// Set SMSC LAN9218 register
//////////////////////////////////////////
void SetRegDW(const unsigned int dwBase, const unsigned int dwOffset, const unsigned int dwVal)
{
        (*(volatile unsigned int *)(dwBase + dwOffset)) = dwVal;
}

//////////////////////////////////////////
// Get SMSC LAN9218 register
//////////////////////////////////////////
unsigned int GetRegDW(const unsigned int dwBase, const unsigned int dwOffset)
{
        return (unsigned int)(*(volatile unsigned int *)(dwBase + dwOffset));
}


void LAN9218_SaveAndDisableInts(void)
{
  pHandle->IntMask =  GetRegDW(g_pLAN911x, INT_EN);
  SetRegDW(g_pLAN911x, INT_EN,0x0);
}

void LAN9218_RestoreAndEnableInts(void)
{
  SetRegDW(g_pLAN911x, INT_EN,pHandle->IntMask);
}

void WriteFifo(const unsigned int dwBase, const unsigned int dwOffset, const unsigned int *pdwBuf, unsigned int dwDwordCount)
{
        volatile unsigned int * pdwReg;
        pdwReg = (volatile unsigned int *)(dwBase + dwOffset);

        while (dwDwordCount)
        {
                *pdwReg = *pdwBuf++;
                dwDwordCount--;
        }
}


void ReadFifo(const unsigned int dwBase, const unsigned int dwOffset,  unsigned int *pdwBuf, unsigned int dwWordCount)
{
        const volatile unsigned int * const pdwReg = ( volatile unsigned int * )(dwBase + dwOffset);
        int iData;
        unsigned short * pusPtr = (unsigned short *)pdwBuf;
        
        while (dwWordCount)
        {
                iData = *pdwReg;
                *pusPtr++ = iData;
                *pusPtr++ = iData>>16 &0xffff;
                dwWordCount--;
        }
}

bool Lan_MacNotBusy(const unsigned int dwLanBase)
{
        int i=0;

        // wait for MAC not busy, w/ timeout
        for(i=0;i<40;i++)
        {
                if((GetRegDW(dwLanBase, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)==(0UL)) {
                        return true;
                }
        }

        return false;
}

void Lan_SetMacRegDW(const unsigned int dwLanBase, const unsigned int dwOffset, const unsigned int dwVal)
{
        if (GetRegDW(dwLanBase, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)
        {
                return;
        }

        // send the data to write
        SetRegDW(dwLanBase, MAC_CSR_DATA, dwVal);

        // do the actual write
        SetRegDW(dwLanBase, MAC_CSR_CMD,
                ((dwOffset & 0x000000FFUL) | MAC_CSR_CMD_CSR_BUSY_));

        // wait for the write to complete, w/ timeout
        if (!Lan_MacNotBusy(dwLanBase))
        {
        }
}

unsigned int Lan_GetMacRegDW(const unsigned int dwLanBase, const unsigned int dwOffset)
{
        unsigned int    dwRet;

        // wait until not busy, w/ timeout
        if (GetRegDW(dwLanBase, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)
        {
                return 0xFFFFFFFFUL;
        }

        // send the MAC Cmd w/ offset
        SetRegDW(dwLanBase, MAC_CSR_CMD,
                ((dwOffset & 0x000000FFUL) | MAC_CSR_CMD_CSR_BUSY_ | MAC_CSR_CMD_R_NOT_W_));

        // wait for the read to happen, w/ timeout
        if (!Lan_MacNotBusy(dwLanBase))
        {
                dwRet = 0xFFFFFFFFUL;
        }
        else
        {
                // finally, return the read data
                dwRet = GetRegDW(dwLanBase, MAC_CSR_DATA);
        }

        return dwRet;
}

void Lan_SetPhyRegW(const unsigned int dwLanBase, const unsigned int dwPhyAddress, const unsigned int dwMiiIndex, const unsigned short wVal)
{
        unsigned int dwAddr;
        int i=0;

        // confirm MII not busy
        if ((Lan_GetMacRegDW(dwLanBase, MII_ACC) & MII_ACC_MII_BUSY_) != 0UL)
        {
                return;
        }

        // put the data to write in the MAC
        Lan_SetMacRegDW(dwLanBase, MII_DATA, (unsigned int)wVal);

        // set the address, index & direction (write to PHY)
        dwAddr = ((dwPhyAddress & 0x1FUL)<<11) | ((dwMiiIndex & 0x1FUL)<<6) | MII_ACC_MII_WRITE_;
        Lan_SetMacRegDW(dwLanBase, MII_ACC, dwAddr);

        // wait for write to complete w/ timeout
        for(i=0;i<100;i++) {
                // see if MII is finished yet
                if ((Lan_GetMacRegDW(dwLanBase, MII_ACC) & MII_ACC_MII_BUSY_) == 0UL)
                {
                        return;
                }
        }

        return;
}

unsigned short Lan_GetPhyRegW(const unsigned int dwLanBase, const unsigned int dwPhyAddress, const unsigned int dwMiiIndex)
{
        unsigned int dwAddr;
        unsigned short wRet = (unsigned short)0xFFFF;
        int i=0;

        // confirm MII not busy
        if ((Lan_GetMacRegDW(dwLanBase, MII_ACC) & MII_ACC_MII_BUSY_) != 0UL)
        {
                return (unsigned short)0;
        }

        // set the address, index & direction (read from PHY)
        dwAddr = ((dwPhyAddress & 0x1FUL)<<11) | ((dwMiiIndex & 0x1FUL)<<6);
        Lan_SetMacRegDW(dwLanBase, MII_ACC, dwAddr);

        // wait for read to complete w/ timeout
        for(i=0;i<100;i++) {
                // see if MII is finished yet
                if ((Lan_GetMacRegDW(dwLanBase, MII_ACC) & MII_ACC_MII_BUSY_) == 0UL)
                {
                        // get the read data from the MAC & return i
                        wRet = ((unsigned short)Lan_GetMacRegDW(dwLanBase, MII_DATA));
                        break;
                }
        }
        if (i == 100) {
                wRet = ((unsigned short)0xFFFFU);
        }

        return wRet;
}


unsigned int Lan_GetLinkMode(const unsigned int dwLanBase)
{
        unsigned int result = LINK_NO_LINK;
        const short wRegBSR = Lan_GetPhyRegW(dwLanBase, dwPhyAddr, PHY_BSR);

        if(wRegBSR & PHY_BSR_LINK_STATUS_)
        {
                short wTemp;
                const short wRegADV = Lan_GetPhyRegW(dwLanBase, dwPhyAddr, PHY_ANEG_ADV);
                const short wRegLPA = Lan_GetPhyRegW(dwLanBase, dwPhyAddr, PHY_ANEG_LPA);
                wTemp = (short)(wRegLPA & wRegADV);

                if(wTemp & PHY_ANEG_LPA_100FDX_)
                {
                        result = LINK_100MPS_FULL;
                }
                else if(wTemp & PHY_ANEG_LPA_100HDX_)
                {
                        result = LINK_100MPS_HALF;
                }
                else if(wTemp & PHY_ANEG_LPA_10FDX_)
                {
                        result = LINK_10MPS_FULL;
                }
                else if(wTemp & PHY_ANEG_LPA_10HDX_)
                {
                        result = LINK_10MPS_HALF;
                }
        }

        return result;
}



//------------------------------------------------------------------------------

static bool Rx_ForceReceiverDiscard( void )
{
        // This routine will reset the receiver alone
        // NOTE: this assumes that the receiver was already running

        unsigned int    dwMac_cr;
        int             i;

        // stop the receiver (RXEN)
        dwMac_cr = GetRegDW(g_pLAN911x, MAC_CR) & ~(MAC_CR_RXEN_);
        SetRegDW(g_pLAN911x, MAC_CR, dwMac_cr);

        // wait for receiver to actually stop by referencing the RXSTOP intr status
        for (i=0; i < 100; i++)
        {
                unsigned int    dwInt_sts = GetRegDW(g_pLAN911x, INT_STS);
                if ((dwInt_sts & INT_STS_RXSTOP_INT_) != 0)
                {
                        break;
                }
        }
        SetRegDW(g_pLAN911x, INT_STS, INT_STS_RXSTOP_INT_); // clear STS
        if (i == 100)
                return false;   // did not RXSTOP

        // discard all RX FIFO data via the DUMP cmd, & wait for DUMP to self clear
        SetRegDW(g_pLAN911x, RX_CFG, RX_CFG_RX_DUMP_);
        for (i=0; i < 100; i++)
        {
                unsigned int    dwRx_cfg = GetRegDW(g_pLAN911x, RX_CFG);
                if ((dwRx_cfg & RX_CFG_RX_DUMP_) == 0)
                {
                        break;
                }
        }
        if (i == 100)
                return false;   // RX_DUMP did not self clear

        // re-start the receiver (RXEN)
        SetRegDW(g_pLAN911x, MAC_CR, (dwMac_cr|MAC_CR_RXEN_));

        return true;
}


//------------------------------------------------------------------------------

void LAN911xEnableInts(void)
{
        unsigned int    dwTemp;


        // enable master IRQ_EN
        dwTemp = GetRegDW(g_pLAN911x, INT_CFG);
        dwTemp |= INT_CFG_IRQ_EN_;
        SetRegDW(g_pLAN911x, INT_CFG, dwTemp);

        // clear pending one before enable
        SetRegDW(g_pLAN911x, INT_STS, INT_STS_RSFL_);

    // Only enable receive interrupts (we poll for Tx completion)
        dwTemp = GetRegDW(g_pLAN911x, INT_EN);
        /* Enalbe PHY Interrupt in MAC*/
        dwTemp |= INT_EN_RSFL_EN_ | INT_EN_PHY_INT_EN_;
        SetRegDW(g_pLAN911x, INT_EN, dwTemp);

}

//------------------------------------------------------------------------------

void LAN911xDisableInts()
{
        unsigned int    dwTemp;


    // Disable all interrupts
        dwTemp = GetRegDW(g_pLAN911x, INT_EN);
        dwTemp &= ~INT_EN_RSFL_EN_;
        /* Disable Phy Interrupt in MAC*/
        dwTemp &= ~INT_EN_PHY_INT_EN_;
        SetRegDW(g_pLAN911x, INT_EN, dwTemp);

}

//------------------------------------------------------------------------------
static unsigned int ComputeCrc(const unsigned char *pBuffer,  const unsigned int uiLength)
{
        unsigned int i;
        unsigned int crc = 0xFFFFFFFFUL;
        unsigned int result = 0UL;
        const unsigned int poly = 0xEDB88320UL;

        for(i=0U; i<uiLength; i++)
        {
                int bit;
                unsigned int data=((unsigned int)pBuffer[i]);
                for(bit=0; bit<8; bit++)
                {
                        const unsigned int p = (crc^((unsigned int)data))&1UL;
                        crc >>= 1;
                        if(p != 0UL) {
                                crc ^= poly;
                        }
                        data >>=1;
                }
        }
        result=((crc&0x01UL)<<5)|
                   ((crc&0x02UL)<<3)|
                   ((crc&0x04UL)<<1)|
                   ((crc&0x08UL)>>1)|
                   ((crc&0x10UL)>>3)|
                   ((crc&0x20UL)>>5);

        return (result);
}

/////////////////////////////////
// Set MAC address
//////////////////////////////////
void Lan_SetMacAddress(unsigned char *mac_addr)
{
        unsigned int dwHigh16;
        unsigned int dwLow32;

        dwLow32 = ( (mac_addr[3]<<24) | (mac_addr[2]<<16) |(mac_addr[1]<<8) | mac_addr[0] );
        dwHigh16 = ( (mac_addr[5]<<8) | mac_addr[4] );

        // set MAC addr
        Lan_SetMacRegDW(g_pLAN911x, ADDRH, dwHigh16);
        Lan_SetMacRegDW(g_pLAN911x, ADDRL, dwLow32);

}

//////////////////////////////////
// Get MAC address
//////////////////////////////////
void Lan_GetMacAddress(unsigned char *mac_addr)
{

        unsigned int dwHigh16;
        unsigned int dwLow32;
        // get MAC addr
        dwHigh16 = Lan_GetMacRegDW(g_pLAN911x, ADDRH);
        dwLow32 = Lan_GetMacRegDW(g_pLAN911x, ADDRL);

        mac_addr[0] = dwLow32 & 0xFF;
        mac_addr[1] = dwLow32 >> 8 & 0xFF;
        mac_addr[2] = dwLow32 >> 16 & 0xFF;
        mac_addr[3] = dwLow32 >> 24 & 0xFF;
        mac_addr[4] = dwHigh16 & 0xFF;
        mac_addr[5] = dwHigh16 >> 8 & 0xFF;

}


/******************************************************************************
 * LANC91C111: LAN9218_wait_to_send_packet()
 *
 * Purpose:
 *    Attempt to allocate memory for a packet, if chip-memory is not
 *    available, then tell the card to generate an interrupt when it
 *    is available.
 *****************************************************************************/
static unsigned char LAN9218_wait_to_send_packet(ADI_ETHER_LAN9218_DATA * cptr)
{
    unsigned short    length;
    unsigned short    numPages;
    unsigned short    time_out;
    unsigned short    status;
    unsigned short   *elnth;
    unsigned short rc = LAN9218_TX_SUCCESS;
    unsigned int    uiTemp;
    unsigned int    uiCounter;
    unsigned int    uiSpcNeed;
    unsigned int    uiTxSpc;
    unsigned int    uiTxCmdA;
    unsigned int    uiTxCmdB;
    static unsigned int uiSeqNum;
    ADI_ETHER_BUFFER  *bf = cptr->m_TxEnqueuedHead;
    unsigned short    *buf;
    buf = (unsigned short *)(((char *)bf->Data)+2);

        // the length is held in the first two bytes of the 'frame', the ElementCount says the number of elements in the first buffer

        // get length
        elnth = (unsigned short *)cptr->m_TxEnqueuedHead->Data;
        length = *elnth;

        // set length
        length = ETH_ZLEN < length ? length : ETH_ZLEN;

        // wait until space available
        uiCounter = 0;
        uiSpcNeed = (length+3)&(~0x03UL);
        while (1)
        {
                uiTxSpc = GetRegDW(g_pLAN911x, TX_FIFO_INF) & TX_FIFO_INF_TDFREE_;
                if (uiSpcNeed <= uiTxSpc)
                {
                        break;
                }
                OALStall(10);
                if (uiCounter++ > 100)
                {
                        //timeout
                        rc = LAN9218_TX_ERROR;
                }
        }
        
        LAN911xDisableInts();

        uiTxCmdA = (((unsigned int)buf & 0x03) << 16) | length;
        uiTxCmdA |= (TX_CMD_A_INT_FIRST_SEG_ | TX_CMD_A_INT_LAST_SEG_);
        
        
        uiTxCmdB = length;
        uiTxCmdB |= (uiSeqNum << 16);
        uiSeqNum ++;
        SetRegDW(g_pLAN911x, TX_DATA_FIFO_PORT, uiTxCmdA);
        SetRegDW(g_pLAN911x, TX_DATA_FIFO_PORT, uiTxCmdB);

        WriteFifo(g_pLAN911x, TX_DATA_FIFO_PORT, (unsigned int *)((unsigned int)buf & ~0x03UL), (length+3+(((unsigned int)buf)&0x03UL))>>2);

        uiTemp = GetRegDW(g_pLAN911x, TX_FIFO_INF) & TX_FIFO_INF_TSUSED_;
        if ((uiTemp >> 16) & 0xFFFF)
        {
                uiTemp = GetRegDW(g_pLAN911x, TX_STATUS_FIFO_PORT);

                if (uiTemp & TX_STS_ES)
                {
                        //Tx error
                        // Failure.
                        rc = LAN9218_TX_ERROR;
                }
        }

        // now set the status in the buffer bf
        bf->ProcessedFlag = 1;
        bf->ProcessedElementCount = (*elnth+2 + buff_prefix + bf->ElementWidth-1)/bf->ElementWidth;
        bf->StatusWord = 0x3;   // completed and OK


        LAN911xEnableInts();

        return rc;
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
static void LAN9218_rcv(ADI_ETHER_LAN9218_DATA *dev )
{
    int            i, packet_number,chip_id;
    unsigned short status;
    unsigned short packet_length;
    unsigned int   *pBuffer;
    unsigned short *elnth;
    unsigned int   *pBuf;
    unsigned int   dwTemp, RxStatus; 

    dwTemp = GetRegDW(g_pLAN911x, INT_STS);

    if (GetRegDW(g_pLAN911x, INT_STS) & INT_STS_RSFL_)
    {
        dwTemp = GetRegDW(g_pLAN911x, RX_FIFO_INF);
        if ((dwTemp & 0x00FF0000) >> 16)
        {
            RxStatus = GetRegDW(g_pLAN911x, RX_STATUS_FIFO_PORT);

            if (!(RxStatus & RX_STS_ES))
            {
                ADI_ETHER_BUFFER *tmp_q_ele;
#if LAN9218_LOG_NET_ERRORS
                if ( RxStatus & RX_STS_MULTICAST ) dev->Stats->cEMAC_RX_CNT_MULTI++;
#endif
                packet_length = (RxStatus & 0x3FFF0000) >>16;

                // max 2k packets over Ethernet
                packet_length &= 0x07ff;  /* mask off top bits */

                tmp_q_ele = dev->m_RxEnqueuedHead;

                if (tmp_q_ele != NULL) 
                {
                    elnth = (unsigned short *)tmp_q_ele->Data;
                    // subtract 4 bytes for the CRC (even if we dont store it) + prefix if present
                    *elnth = (packet_length + buff_prefix); 
                    tmp_q_ele->ProcessedElementCount = (*elnth+2+tmp_q_ele->ElementWidth-1) / 
                                                                     tmp_q_ele->ElementWidth;
                }

                if ((tmp_q_ele)&&(tmp_q_ele->ProcessedElementCount <= tmp_q_ele->ElementCount))
                {
                   //store packet length
                   // assign pointer,
                   // lwIP wrapper passes the Data to point to the 2byte length header
                   // skipping the prefix number of bytes.
                   
                   pBuffer = (void *)( ((char *)tmp_q_ele->Data) +2  );
                }
                else
                {
#if LAN9218_LOG_NET_ERRORS
                    if (tmp_q_ele) 
                    {
                        dev->Stats->cEMAC_RX_CNT_LONG++;
                    } 
                    else 
                    {
                        dev->Stats->cEMAC_RX_CNT_LOST++;
                    }
#endif

                    // drop packet
                    //  good or bad, delete this packet 
                    // release all pending packets
                    SetRegDW(g_pLAN911x, RX_CFG, RX_CFG_RX_DUMP_);
                    return;
                    
                }
                // store for debug
                dev->LAN_state.rpsize = packet_length;


                // if cache is on flush and invalidate the rx data region
                if(pHandle->Cache)
                {
                    FlushInvArea((char*)pBuffer,(char*)pBuffer+packet_length);
                }

                // initiate the mdma transfer 
                dma_initiate_transfer((unsigned long)(g_pLAN911x + RX_DATA_FIFO_PORT),
                                      (unsigned long)pBuffer,((packet_length +3) >> 2),
                                       LAN9218_DMA_DIR_RX);


#if LAN9218_LOG_NET_ERRORS
                dev->Stats->cEMAC_RX_CNT_OK++;
#endif

            }
            else
            {
#if LAN9218_LOG_NET_ERRORS
//              if ( RxStatus & RS_ALGNERR )  dev->Stats->cEMAC_RX_CNT_ALIGN++;
                if ( RxStatus & RX_STS_LENGTH_ERR) dev->Stats->cEMAC_RX_CNT_LONG++;
                if ( RxStatus & RX_STS_CRC_ERR)    dev->Stats->cEMAC_RX_CNT_FCS++;
#endif
            }
        }
    }

    if (GetRegDW(g_pLAN911x, INT_STS) & INT_STS_RXE_)
    {
        //Rx_ForceReceiverDiscard();
    }

    return;

}

/******************************************************************************
 * TX IRQ Error Handler Function for SMSC91C111
 *
 * Purpose:  Handle the transmit error message.
 *  This is only called when an TX error occured because of
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
static void LAN9218_tx(ADI_ETHER_LAN9218_DATA * dev)
{

        //error handler
//      printf("transaction error.\n");

}
/******************************************************************************
 * LAN Interrupt Handler
 *****************************************************************************/
int LAN9218_InterruptHandler(ADI_ETHER_LAN9218_DATA *dev)
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
        volatile unsigned int     stat;
        volatile unsigned short   card_status;
        volatile unsigned int     mask;
        volatile unsigned int     saved_bank;
        volatile unsigned short   saved_pointer;
        unsigned int              temp;
        ADI_DCB_RESULT            result;

        /* Phy Read and Mask Variables */
        u16 wTemp;
        u16 wMask;
        u32 dwTemp;
        u32 dwRegVal;


        // if dma is in use then return
        if(dev->m_dma_protect == 1)
        {
           // return if MDMA is locked 
           return 0;
        }

        // read IRQ status register
        mask = GetRegDW(g_pLAN911x, INT_EN);

        // disable all LAN IRQs
        SetRegDW(g_pLAN911x, INT_EN, 0x0);


        // read the status flag and mask it
        stat = GetRegDW(g_pLAN911x, INT_STS)& mask;

        if(!stat)
            goto done;


        // check for rx fifo full interrupt 
        if (stat & INT_STS_RSFL_)
        {
                // save off the interrupt mask
                dev->IntMask = mask;

                // lock dma for rx transfers
                dma_protect(dev,LAN9218_DMA_DIR_RX);

                // store debug
                dev->LAN_state.last_IRQ_serviced = INT_STS_RSFL_;

                // trigger rcv transfer using MDMA
                LAN9218_rcv(dev);

                // Acknowledge interrupt
                ACK_LAN_INT(dev->pf);
                
                // rest of the processing is done in MDMA interrupt handler
                return(1);
        }
        else if (stat & INT_STS_TSFL_ )
        {
                // store debug
                dev->LAN_state.last_IRQ_serviced = INT_STS_TSFL_;
                LAN9218_tx(dev);
                // Acknowledge the interrupt
                SetRegDW(g_pLAN911x, INT_STS, INT_STS_TSFL_);
        }

        // Check for PHY Interrupt 
        else if (stat & INT_STS_PHY_INT_)
        {
                ENTER_CRITICAL_REGION();
                
                // Read the Phy Interrupt Mask and source 
                wMask = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_INT_MASK);
                wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_INT_SRC) & wMask;
            
                // Set the MAC Options if Auto Negotiation Complete 
                if (wTemp & PHY_INT_SRC_ANEG_COMP_)
                {
                    // Get the Link Mode after Auto-Negotiation 
                    dwTemp = Lan_GetLinkMode(g_pLAN911x);

                    // Read the MAC_CR Register 
                    dwRegVal = Lan_GetMacRegDW(g_pLAN911x, MAC_CR);
                    dwRegVal &= ~(MAC_CR_FDPX_|MAC_CR_RCVOWN_);

                    // Set the options depending upon the Link State 
                    switch(dwTemp)
                    {
                        case LINK_NO_LINK:
                             //link:none
                            break;
                        case LINK_10MPS_HALF:
                            dwRegVal|=MAC_CR_RCVOWN_;
                            //link:10H 
                            break;
                        case LINK_10MPS_FULL:
                            dwRegVal|=MAC_CR_FDPX_;
                            // link:10F
                            break;
                        case LINK_100MPS_HALF:
                            dwRegVal|=MAC_CR_RCVOWN_;
                            // link:100H
                            break;
                        case LINK_100MPS_FULL:
                            dwRegVal|=MAC_CR_FDPX_;
                            // link:100F
                        break;
                     }
                     // Write the MAC_CR Register 
                     Lan_SetMacRegDW(g_pLAN911x, MAC_CR, dwRegVal); 
             }
                
                EXIT_CRTICIAL_REGION();
            }
 

        // read the status flag and mask it
        stat = GetRegDW(g_pLAN911x, INT_STS)& mask;

done:      
        // enable interrupt again
        SetRegDW(g_pLAN911x, INT_EN, mask);

        // Acknowledge interrupt
        ACK_LAN_INT(dev->pf);

        return 1;

}

#pragma optimize_for_speed

static int QueueNewRcvFrames(ADI_ETHER_LAN9218_DATA *dev,ADI_ETHER_BUFFER *bufs)
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
/******************************************************************************
 * LANC91C111: LAN9218_dma_send_packet()
 *
 * Purpose:
 *    Attempt to allocate memory for a packet, if chip-memory is not
 *    available, then tell the card to generate an interrupt when it
 *    is available.
 *****************************************************************************/
static unsigned char LAN9218_dma_send_packet(ADI_ETHER_LAN9218_DATA * cptr)
{
    unsigned short  length;
    unsigned short  numPages;
    unsigned short  time_out;
    unsigned short  status;
    unsigned short  *elnth;
    unsigned short  rc = LAN9218_TX_SUCCESS;
    unsigned int    uiTemp;
    unsigned int    uiCounter;
    unsigned int    uiSpcNeed;
    unsigned int    uiTxSpc;
    unsigned int    uiTxCmdA;
    unsigned int    uiTxCmdB;
    static unsigned int uiSeqNum;
    ADI_ETHER_BUFFER  *bf = cptr->m_TxEnqueuedHead;
    unsigned short    *buf;
    buf = (unsigned short *)(((char *)bf->Data)+2);

        // the length is held in the first two bytes of the 'frame', 
        // the ElementCount says the number of elements in the first buffer

        // get length
        elnth = (unsigned short *)cptr->m_TxEnqueuedHead->Data;
        length = *elnth;

        // set length
        length = ETH_ZLEN < length ? length : ETH_ZLEN;

        // wait until space available
        uiCounter = 0;
        uiSpcNeed = (length+3)&(~0x03UL);

        while (1)
        {
            uiTxSpc = GetRegDW(g_pLAN911x, TX_FIFO_INF) & TX_FIFO_INF_TDFREE_;
            if (uiSpcNeed <= uiTxSpc)
            {
               break;
            }
            OALStall(10);
            if (uiCounter++ > 100)
            {
               //timeout
               rc = LAN9218_TX_ERROR;
               return(rc);
            }
        }
        
        //LAN911xDisableInts();

        uiTxCmdA = (((unsigned int)buf & 0x03) << 16) | length;
        uiTxCmdA |= (TX_CMD_A_INT_FIRST_SEG_ | TX_CMD_A_INT_LAST_SEG_);
        
        
        uiTxCmdB = length;
        uiTxCmdB |= (uiSeqNum << 16);
        uiSeqNum ++;
        SetRegDW(g_pLAN911x, TX_DATA_FIFO_PORT, uiTxCmdA);
        SetRegDW(g_pLAN911x, TX_DATA_FIFO_PORT, uiTxCmdB);

        // flush the area
        if(cptr->Cache)
        {
            FlushArea((char*)buf,((char*)buf + length));
        }

        // initiate dma transfer
        dma_initiate_transfer((unsigned long)buf & ~0x03UL,
                              (unsigned long) (g_pLAN911x + TX_DATA_FIFO_PORT),
                              ((length +3) >> 2),LAN9218_DMA_DIR_TX);

        return rc;
}

/* this function may get called from ISR level or from thread level
 * when application tries to send a packet this will called. In case of
 * dma interrupt this may be initiated from the transmit complete portion
 * of the DMA interrupt handler */
int LAN9218_SendTxEnqueuedPacket(ADI_ETHER_LAN9218_DATA *dev)
{
volatile unsigned char status;

    status  = LAN9218_dma_send_packet(dev);

    if ( (status == LAN9218_TX_RETRY) || (status == LAN9218_TX_ERROR))
    {
        /* Keep the packet in the queue and retry next time */
         dev->Stats->cEMAC_TX_CNT_DEFER++;
         return (0);
    }

    return (1);
}

static int QueueNewXmtFrames(ADI_ETHER_LAN9218_DATA *dev,ADI_ETHER_BUFFER *bufs)
{
    int mr = 0;
    ADI_ETHER_BUFFER *tmp_q_ele;
        ADI_DCB_RESULT   result;

        // if the incoming element is NULL throw an error
        if (bufs == NULL)
         return (0);

        ENTER_CRITICAL_REGION();

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


        // we inserted the element in the TxEnquedHead list, we will try to grab
        // dma and sent it across. If not DMA interrupt handler will send it.
        //
        if(dev->m_dma_protect == 1)
        {
            EXIT_CRTICIAL_REGION();
            return (0);
        }

        // lock dma for xmt
        dma_protect(dev,LAN9218_DMA_DIR_TX);

        // save off the LAN9218 interrupt mask
        dev->IntMask =  GetRegDW(g_pLAN911x, INT_EN);

        // disable all interrupts
        SetRegDW(g_pLAN911x, INT_EN, 0x0);

       // dma locked. and try to send the packet. this may fail if tx can not
       // acquire memory from the device.
       //
       if(!LAN9218_SendTxEnqueuedPacket(dev))
       {
           dma_relinquish(dev);
           SetRegDW(g_pLAN911x, INT_EN, dev->IntMask);
       }

       EXIT_CRTICIAL_REGION();
#if 0
        {
                int cont = 1;
                while(cont)
                {
                        unsigned char status;

                        status = LAN9218_wait_to_send_packet(dev);

                        if (status == LAN9218_TX_RETRY)
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
        //## what happens if a packet is trasnmitted while in the callback
        dev->m_TxDequeuedHead = NULL;
                dev->m_TxDequeuedTail = NULL;
        dev->m_TxDequeuedCount = 0;
        //}
        EXIT_CRTICIAL_REGION();

        return mr;
#endif
      return(1);
}

//bool Lan_Initialize(ADI_ETHER_LAN9218_DATA *dev)
bool StartMac(ADI_ETHER_LAN9218_DATA *dev)
{
        int i=0;
        unsigned int dwTimeout, dwTemp;
        unsigned int    dwPhyAddr = 1UL;
        unsigned short wTemp=0, wstat;

        unsigned int dwRegVal;
        unsigned short mac[3];

        memset(&mac, 0, sizeof(mac));

        SetDevicePFConfig(dev);
        pHandle = dev;

        // device is started and active now.
        dev->Started = true;

        // Reset the Chip
        SetRegDW(g_pLAN911x, HW_CFG, HW_CFG_SRST_);
        dwTimeout = 100000UL;
        do {
        OALStall(10);
                dwTemp = GetRegDW(g_pLAN911x, HW_CFG);
                dwTimeout--;
        } while((dwTimeout-- > 0UL) && (dwTemp & HW_CFG_SRST_));

        if (dwTemp & HW_CFG_SRST_) {
                //Failed to initialize LAN9218
                return false;
        }


    // read the chip ID and revision.
        g_chipRevision = GetRegDW(g_pLAN911x, ID_REV);
        if (((g_chipRevision & 0x0FF00000) == 0x01100000) ||
                ((g_chipRevision & 0xFF000000) == 0x11000000))
        {
                //get Chip Id
        }
        else
        {
                //Not a LAN9118 family
                return false;
        }

        // NOTE
        // Internal PHY only
        // If want to support External Phy,
        //  assign External Phy address to dwPhyAddr
        dwPhyAddr = 1UL;

        // reset PHY
        Lan_SetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BCR, PHY_BCR_RESET_);
        dwTimeout = 1000UL;
        do {
                OALStall(10000);                        // 10mSec
                dwTemp = (unsigned int)Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BCR);
                dwTimeout--;
        } while ((dwTimeout>0UL) && (dwTemp & (unsigned int)PHY_BCR_RESET_));

        if (dwTemp & (unsigned int)PHY_BCR_RESET_) {
                //PHY reset failed to complete
                return false;
        }

        // Auto Neg
        Lan_SetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BCR, PHY_BCR_AUTO_NEG_ENABLE_);
        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_ANEG_ADV);
        wTemp &= ~(PHY_ANEG_ADV_PAUSE_OP_ | PHY_ANEG_ADV_SPEED_);
        wTemp |= PHY_ANEG_ADV_10H_| PHY_ANEG_ADV_10F_| PHY_ANEG_ADV_100H_| PHY_ANEG_ADV_100F_;
        Lan_SetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_ANEG_ADV, wTemp);

        Lan_SetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BCR, PHY_BCR_AUTO_NEG_ENABLE_ | PHY_BCR_RESTART_AUTO_NEG_);

        dwTimeout = 1000;
        do {
                OALStall(1000);                 // 1mSec
                wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BSR);
        } while ((dwTimeout-- > 0UL) &&
                        !((wTemp & (short)(PHY_BSR_REMOTE_FAULT_|PHY_BSR_AUTO_NEG_COMP_))));

        if (wTemp & (short)PHY_BSR_AUTO_NEG_COMP_)
        {
                //Auto Negotiation done
                // We are done

        }
        else
        {
                //Auto Negotiation Failed
                return false;
        }

        dwTemp = Lan_GetLinkMode(g_pLAN911x);

        dwRegVal = Lan_GetMacRegDW(g_pLAN911x, MAC_CR);
        dwRegVal &= ~(MAC_CR_FDPX_|MAC_CR_RCVOWN_);

        switch(dwTemp)
        {
                case LINK_NO_LINK:
                        //link:none
                        break;
                case LINK_10MPS_HALF:
                        dwRegVal|=MAC_CR_RCVOWN_;
                        //link:10H 
                        break;
                case LINK_10MPS_FULL:
                        dwRegVal|=MAC_CR_FDPX_;
                        // link:10F
                        break;
                case LINK_100MPS_HALF:
                        dwRegVal|=MAC_CR_RCVOWN_;
                        // link:100H
                        break;
                case LINK_100MPS_FULL:
                        dwRegVal|=MAC_CR_FDPX_;
                        // link:100F
                        break;
        }
        Lan_SetMacRegDW(g_pLAN911x, MAC_CR, dwRegVal);


        // initialize TX
        dwTemp = Lan_GetMacRegDW(g_pLAN911x, MAC_CR);
        dwTemp |= (MAC_CR_TXEN_ );
        Lan_SetMacRegDW(g_pLAN911x, MAC_CR, dwTemp);

        //setup TLI store-and-forward, and preserve TxFifo size
        dwTemp = GetRegDW(g_pLAN911x, HW_CFG);
        dwTemp &= (HW_CFG_TX_FIF_SZ_ | 0xFFFUL);
        dwTemp |= HW_CFG_SF_;
        SetRegDW(g_pLAN911x, HW_CFG, dwTemp);
        dwTemp = GetRegDW(g_pLAN911x, HW_CFG);

        SetRegDW(g_pLAN911x, TX_CFG, TX_CFG_TX_ON_);

       // Check for Phy power down/up flag
        // Read the Basic Control Register
        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BCR);
        if (dev->bPhyPowerDown)
        {
            // Set the Power Down Flag 
            wTemp |= PHY_BCR_POWER_DOWN_;
        }
        else
        {
            // Set the PHY to Power Up
            // Clear the Power Down Flag 
            wTemp &= (~PHY_BCR_POWER_DOWN_);
        }
        // Write the Basic COntrol Register
        Lan_SetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BCR, wTemp);

        // initialize RX
        SetRegDW(g_pLAN911x, RX_CFG, 0x00000000UL);
        dwTemp = Lan_GetMacRegDW(g_pLAN911x, MAC_CR);
        dwTemp |= MAC_CR_RXEN_;
        Lan_SetMacRegDW(g_pLAN911x, MAC_CR, dwTemp);

         // EEPROM force a re-load of the MAC ADDRH/ADDRL registers from the EEPROM
         SetRegDW( g_pLAN911x, E2P_CMD, E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_RELOAD_ );
         OALStall(1000);
        dwTemp = GetRegDW(g_pLAN911x, E2P_CMD);
        // Issue EEPROM Reload cmd
        if (GetRegDW(g_pLAN911x, E2P_CMD) & E2P_CMD_MAC_ADDR_LOADED_)
        {
                unsigned int    dwADDR;
                // MAC addr is read from EEPROM
                dwADDR = Lan_GetMacRegDW(g_pLAN911x, ADDRL);
                mac[0] = dwADDR & 0xFFFF;
                mac[1] = (dwADDR >> 16)& 0xFFFF;
                dwADDR = Lan_GetMacRegDW(g_pLAN911x, ADDRH);
                mac[2] = dwADDR & 0xFFFF;
        }

        // if error, set dummy MAC
        if ((mac[0] == 0) && (mac[1] == 0) && (mac[2] == 0))
        {
//              mac[0] = 0x1200;
//              mac[1] = 0x5533;
//              mac[2] = 0xAACC;

                Lan_SetMacRegDW(g_pLAN911x, ADDRH, 0x0000CCAA);
                Lan_SetMacRegDW(g_pLAN911x, ADDRL, 0x55331200);
        }

        /* Enable Physical Interrupt Mask */
    
        /* Read the Phy Interrupt Mask */
        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_INT_MASK);
        /* Enable for Auto-Completion */
        wTemp |= PHY_INT_MASK_ANEG_COMP_;
        /* Write the Interrupt Mask */
        Lan_SetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_INT_MASK, wTemp); 

        // Hook interrupt handler.Same interrupt for all RXIVG/TXIVG/ERRIVG
        //
        adi_int_CECHook(dev->RXIVG,(ADI_INT_HANDLER_FN)LAN9218_InterruptHandler,&EtherDev,FALSE);
        // Enable interrupts in SIC Pin interrupt 0 (PE8) flag pin is mapped to lan interrupt
        //
        adi_int_SICSetIVG(ADI_INT_PINT2,dev->RXIVG);
        adi_int_SICEnable(ADI_INT_PINT2);

#if LAN9218_DBG
        printf("\n");
        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BCR);
        printf("PHY_BCR: 0x%x\n", wTemp);

        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BSR);
        printf("PHY_BSR: 0x%x\n", wTemp);

        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_ID_1);
        printf("PHY_ID_1:  0x%x\n", wTemp);

        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_ID_2);
        printf("PHY_ID_2:  0x%x\n", wTemp);

        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_ANEG_ADV);
        printf("PHY_ANEG_ADV:   0x%x\n", wTemp);

        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_ANEG_LPA);
        printf("PHY_ANEG_LPA:  0x%x\n", wTemp);

        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_ANEG_EXP);
        printf("PHY_ANEG_EXP: 0x%x\n", wTemp);

        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_MODE_CTRL_STS);
        printf("PHY_MODE_CTRL_STS: 0x%x\n", wTemp);

        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_INT_SRC);
        printf("PHY_INT_SRC:  0x%x\n", wTemp);

        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_INT_MASK);
        printf("PHY_INT_MASK: 0x%x\n", wTemp);

        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_SPECIAL);
        printf("PHY_SPECIAL: 0x%x\n", wTemp);

#endif //LAN9218_DBG

        // configure the indicator LEDs
        SetRegDW( g_pLAN911x, GPIO_CFG, GPIO_CFG_LED3_EN_ | 
                                GPIO_CFG_LED2_EN_ | GPIO_CFG_LED1_EN_ );

        init_dma(dev);
        //Fixed TAR36309 Static IP assignment not working on BF54x                              
        LAN911xEnableInts();                    

                                
        return TRUE;
}


/******************************************************************************
 *
 * Configures the PF/IMASK/IAR settings associated with the driver.
 *
 *****************************************************************************/
static int  SetDevicePFConfig(ADI_ETHER_LAN9218_DATA *dev)
{
        unsigned int dwTemp;


        // Default PF configuration is PF9
        //
        if(dev->pf == 0) 
        {
                dev->pf = PE8;
    }


        
        // SMSC ethernet interrupt singal is wired to 
        // PORTE8. So set up portE level interrupt by default
        // and map the interrupt to INT2 
    //adi_flag_Open(ADI_FLAG_PE8);
    //adi_flag_SetDirection(ADI_FLAG_PE8, ADI_FLAG_DIRECTION_INPUT);
    
    
    *pPORTE_FER = 0x0;
    *pPORTE_MUX = 0x0;
    *pPORTE_INEN = 0x0100;
    *pPORTE_DIR_CLEAR = 0x0100;
        

        //using int2
        *pPINT2_INVERT_SET = 0x0100;
        // unmask interrupts
    *pPINT2_MASK_SET    = 0x0100;
    //assign Port E to PINT2
    *pPINT2_ASSIGN              = 0x00000202;

        //initiate extern memory bank 1 for ethernet
    //initiate external memory bank
    *pEBIU_AMGCTL   |= 0xE;
    
        // get which flags are currently set
        dwTemp = *pPINT2_LATCH;                                         
        // clear interrupt request if there is any
    *pPINT2_IRQ = dwTemp;


         return 1;

}


/*********************************************************************
*
*       Function:               pddOpen
*
*       Description:    Opens the Network Driver and does initialization.
*
*********************************************************************/

static u32 adi_pdd_Open(                                        // Open a device
        ADI_DEV_MANAGER_HANDLE  ManagerHandle,  // device manager handle
        u32                                     DeviceNumber,   // device number
        ADI_DEV_DEVICE_HANDLE   DeviceHandle,   // device handle
        ADI_DEV_PDD_HANDLE              *pPDDHandle,    // pointer to PDD handle location
        ADI_DEV_DIRECTION               Direction,              // data direction
        void                    *pCriticalRegionArg,    // critical region imask storage location
        ADI_DMA_MANAGER_HANDLE  DMAHandle,              // handle to the DMA manager
        ADI_DCB_HANDLE                  DCBHandle,              // callback handle
        ADI_DCB_CALLBACK_FN             DMCallback              // device manager callback function
)
{
        u32                     Result;         // return value
        void                    *CriticalResult;
        ADI_ETHER_LAN9218_DATA *dev= &EtherDev;


        // check for errors if required
#ifdef ADI_ETHER_ERROR_CHECKING_ENABLED
        if (DeviceNumber > 0) {         // check the device number
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
                memset(dev,0,sizeof(ADI_ETHER_LAN9218_DATA));

                dev->CriticalData = pCriticalRegionArg;
                dev->DeviceHandle = DeviceHandle;
                dev->DCBHandle = DCBHandle;
                dev->DMCallback = DMCallback;
                dev->Direction = Direction;
                dev->Started = false;
                dev->bEnableGracefulShutdown = false;
                dev->bPhyPowerDown  = false;
                dev->Auto = true;


                //dev->DmaSICMaskBit     = MDMA_STREAM0_INT_NUM; //stream 0 by default
                dev->DmaSICMaskBit     = MDMA_STREAM2_INT_NUM; //stream 2 by default

                dev->MemDMAStream     = 0; //stream 0 by default
                dev->SrcStreamBaseAddr = MDMA_SRC_STREAM2_BASE;
                dev->DstStreamBaseAddr = MDMA_DST_STREAM2_BASE;
                dwPhyAddr = 1UL;
            // initialize port control, parameters are
            //      parameter for adi_int_EnterCriticalRegion (always NULL for VDK and standalone systems)
            if (adi_ports_Init(ADI_SSL_ENTER_CRITICAL) != ADI_PORTS_RESULT_SUCCESS) {
                return 0;
            }
    
            // configure 
            Result = adi_ports_Configure(Ethernet_Ports, sizeof(Ethernet_Ports)/sizeof(Ethernet_Ports[0]));
    
                
                // PE8 input for LAN IRQ
                // default ivg levels for Pin interrupt
                dev->RXIVG  = ik_ivg12;
                dev->TXIVG  = ik_ivg12;
                dev->ERRIVG = ik_ivg12;
                //memory map for LAN9218 ethernet chip
                g_pLAN911x = SMSC911x_PHYS_ADDR;


                //initiate extern async memory bank 1 for ethernet
            *pEBIU_AMGCTL   |= 0xE;

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
*       Function:               pddClose
*
*       Description:    Closes the driver and releases any memory
*
*********************************************************************/

static u32 adi_pdd_Close(               // Closes a device
        ADI_DEV_PDD_HANDLE PDDHandle    // PDD handle
)
{
        ADI_ETHER_LAN9218_DATA *dev = (ADI_ETHER_LAN9218_DATA *)PDDHandle;
        u32             Result = ADI_DEV_RESULT_SUCCESS;                                // return value
        bool active=true;

        // check for errors if required
#if defined(ADI_ETHER_DEBUG)
        if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

        // disable all LAN IRQs
        adi_int_SICDisable(ADI_INT_PINT2);

        adi_flag_Close(ADI_FLAG_PE8);
        adi_ports_Terminate();

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
*       Function:               pddRead
*
*       Description:    Gives list of read buffers to the driver
*
*********************************************************************/
static u32 adi_pdd_Read(                // Reads data or queues an inbound buffer to a device
        ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
        ADI_DEV_BUFFER_TYPE     BufferType,     // buffer type
        ADI_DEV_BUFFER *pBuffer                 // pointer to buffer
)
{
        u32             Result;                         // return value
        ADI_ETHER_LAN9218_DATA *dev = (ADI_ETHER_LAN9218_DATA *)PDDHandle;

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
        //read frames



        return(ADI_DEV_RESULT_SUCCESS);
}


/*********************************************************************
*
*       Function:               pddWrite
*
*       Description:    Sends packet over the physical channel
*
*********************************************************************/
static u32 adi_pdd_Write(                       // Writes data or queues an outbound buffer to a device
        ADI_DEV_PDD_HANDLE PDDHandle,                           // PDD handle
        ADI_DEV_BUFFER_TYPE     BufferType,                             // buffer type
        ADI_DEV_BUFFER *pBuffer                                         // pointer to buffer
)
{
        u32             Result;                         // return value
        ADI_ETHER_LAN9218_DATA *dev = (ADI_ETHER_LAN9218_DATA *)PDDHandle;

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
        //send frames


        return(ADI_DEV_RESULT_SUCCESS);
}
/************************************************************
 *   Function: ShutDownDriver( void )
 *
 * Description: 
 *              Shutdown the driver more gracefully than adi_pdd_Close()
 *              It will wait until all send and recevied queue be processed
 *              before return
 *
 ***************************************************************/

static u32 ShutDownDriver( 
                                        ADI_DEV_PDD_HANDLE      PDDHandle                       // PDD handle
                                        )
{
        ADI_ETHER_LAN9218_DATA *dev = (ADI_ETHER_LAN9218_DATA *)PDDHandle;
        u32             Result = ADI_DEV_RESULT_SUCCESS;                                // return value
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
                
                // Disable the interrupt in SIC
                adi_int_SICDisable(ADI_INT_PINT2);

                // disable all lan interrupts
                LAN911xDisableInts();
        
        adi_int_CECUnhook(dev->RXIVG,(ADI_INT_HANDLER_FN)LAN9218_InterruptHandler,&EtherDev);
        }
        
        sti(uInterruptStatus);
        // return
        return(Result);
        
}
/*********************************************************************
*
*       Function:               GetMultiCastMAC
*
*       Description: 
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
*       Function:               GetMultiCastHashBin
*
*       Description:    Returns the hash bin index given the Multicast MAC
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

/*********************************************************************
*
*       Function:               AddMultiCastMACFilter
*
*       Description:    Computes the multicast hash bit position and sets
*                   or clears for a given multicast group address.
*
*********************************************************************/

static u32 AddMultiCastMACfilter(u32 GroupIpAddress,bool bAddAddress)
{
char MultiCastMac[6];
char *p = (char*)&GroupIpAddress;
int HashBinIndex;
unsigned int uHashVal;

        // Get the multicast MAC address for the given address
        GetMultiCastMAC(GroupIpAddress,&MultiCastMac[0]);

        // Get the hash index for the multicast address 
        HashBinIndex = GetMultiCastHashBin((char*)&MultiCastMac[0],6);
        
        if((HashBinIndex == -1) || (HashBinIndex > 63))
           return ADI_DEV_RESULT_FAILED;

        if(bAddAddress)
        {
                if(HashBinIndex > 31)
                {
                        HashBinIndex -= 32;
                        uHashVal = Lan_GetMacRegDW(g_pLAN911x, HASHH);
                        uHashVal |= (1 << HashBinIndex);
                        Lan_SetMacRegDW(g_pLAN911x, HASHH, uHashVal);
                }
                else
                {
                        uHashVal = Lan_GetMacRegDW(g_pLAN911x, HASHL);
                        uHashVal |= (1 << HashBinIndex);
                        Lan_SetMacRegDW(g_pLAN911x, HASHL, uHashVal);
                }
        }
        else // remove the address
        {
                if(HashBinIndex > 31)
                {
                        HashBinIndex -= 32;
                        uHashVal = Lan_GetMacRegDW(g_pLAN911x, HASHH);
                        uHashVal &= ~(1 << HashBinIndex);
                        Lan_SetMacRegDW(g_pLAN911x, HASHH, uHashVal);
                }
                else
                {
                        uHashVal = Lan_GetMacRegDW(g_pLAN911x, HASHL);
                        uHashVal &= ~(1 << HashBinIndex);
                        Lan_SetMacRegDW(g_pLAN911x, HASHL, uHashVal);
                }
        }

        return(ADI_DEV_RESULT_SUCCESS);
}

/*********************************************************************
*
*       Function:               pddControl
*
*       Description:    List of I/O control commands to the driver
*
*********************************************************************/
static u32 adi_pdd_Control(                     // Sets or senses a device specific parameter
        ADI_DEV_PDD_HANDLE PDDHandle,                           // PDD handle
        u32 Command,                                    // command ID
        void *pArg                                                              // pointer to argument
)
{
        u32             Result;                         // return value
        ADI_ETHER_LAN9218_DATA *dev = (ADI_ETHER_LAN9218_DATA *)PDDHandle;
        ADI_ETHER_MEM_SIZES *msizes;
        ADI_ETHER_SUPPLY_MEM *memsup;
        int maxbuf,i;
        u32 *prefix;
        ADI_ETHER_BUFFER_COUNTS *bufcnts;
        ADI_ETHER_IVG_MAPPING *ivgs;

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
                        // no we dont want peripheral DMA suppot
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
                        Lan_GetMacAddress((unsigned char *)pArg);
                        break;

                case ADI_ETHER_CMD_SET_MAC_ADDR:
                        Lan_SetMacAddress((unsigned char *)pArg);
                        // store mac in an internel variable
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
                case ADI_ETHER_CMD_GEN_CHKSUMS:
                        Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                        break;
                case ADI_ETHER_CMD_SET_SPEED: // Default Auto Nego mode
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
                                //set Phy to loopback mode
                                Lan_SetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BCR, 0x4100);

                        }
                        break;

                case ADI_ETHER_CMD_SET_PHY_POWER_DOWN:
                    i = (int) (pArg);
                    dev->bPhyPowerDown = (i!=0);
                    /* Set/Clear the Power down flag if the device is already started */
                    if (dev->Started) {
                        u16 wTemp;
                        
                        ENTER_CRITICAL_REGION();
                        /* Ge the Basic Control Register */
                        wTemp = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BCR);
                        /* Set/Clear the Flag */
                        if (dev->bPhyPowerDown)
                        {
                            wTemp |= PHY_BCR_POWER_DOWN_;
                        }
                        else
                        {
                            wTemp &= (~PHY_BCR_POWER_DOWN_);
                        }
                        /* Write the Basic Control Register */
                        Lan_SetPhyRegW(g_pLAN911x, dwPhyAddr, PHY_BCR, wTemp);
                        
                        EXIT_CRTICIAL_REGION();
                        
                    }
                    break;

                case ADI_ETHER_CMD_GET_PHY_REGS:
                        {
                                short *arg = (short *)pArg;
                                int ic;

                                for (ic=0;ic<=PHY_SPECIAL;ic++)
                                        arg[ic] = Lan_GetPhyRegW(g_pLAN911x, dwPhyAddr,ic);
                        }
                        break;
                case ADI_ETHER_CMD_BUFFERS_IN_CACHE:
                        i = (int)(pArg);
                        dev->Cache = (i!=0);
                        break;
                case ADI_ETHER_CMD_START:
                        StartMac(PDDHandle);
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

                        // we don't understand this command
                        Result = ADI_DEV_RESULT_NOT_SUPPORTED;

        }

        // return
        return(Result);
}

/******************************************************************************
 *
 * This routine is called from the DMA handler. Once the DMA transfers the
 * data the packet is removed from the PLI buffers.
 * Will get called from the dma interrupt routine. All SMSC interrupts were
 * off and the core interrupts were on during this time.
 *****************************************************************************/
void LAN9218_TransmitComplete(void)
{
   unsigned int     length,rc;
   ADI_ETHER_BUFFER *tmp_q_ele;
   unsigned short   *buf;
   unsigned short   *elnth;
   unsigned short   lnth_first;
   ADI_ETHER_BUFFER *bf = pHandle->m_TxEnqueuedHead;
   unsigned int     uiTemp;

   // get length
   elnth = (unsigned short *)bf->Data;

   uiTemp = GetRegDW(g_pLAN911x, TX_FIFO_INF) & TX_FIFO_INF_TSUSED_;

   if ((uiTemp >> 16) & 0xFFFF)
   {
       uiTemp = GetRegDW(g_pLAN911x, TX_STATUS_FIFO_PORT);

       if (uiTemp & TX_STS_ES)
       {
           //rc = LAN9218_TX_ERROR;
       }
   }

   pHandle->Stats->cEMAC_TX_CNT_OK++;

   // now set the status in the buffer bf
   bf->ProcessedFlag = 1;
   bf->ProcessedElementCount = (*elnth + 2 + buff_prefix + bf->ElementWidth-1)/bf->ElementWidth;
   bf->StatusWord = 0x3;   // completed and OK

   tmp_q_ele = pHandle->m_TxEnqueuedHead;

   /* Change the Enqueued head */
   pHandle->m_TxEnqueuedHead = (ADI_ETHER_BUFFER*)tmp_q_ele->pNext;
   pHandle->m_TxEnqueuedCount--;

   if (pHandle->m_TxEnqueuedCount == 0)
   {
      pHandle->m_TxEnqueuedTail = (ADI_ETHER_BUFFER*)NULL;
   }

   /* Add the packet to dequeued list*/
   if (pHandle->m_TxDequeuedCount)
          pHandle->m_TxDequeuedTail->pNext = (ADI_ETHER_BUFFER*)tmp_q_ele;
   else
          pHandle->m_TxDequeuedHead = tmp_q_ele;

   //No matter what this is also the tail.
   pHandle->m_TxDequeuedTail = tmp_q_ele;

   //And tail->next should point to NULL
   tmp_q_ele->pNext = (ADI_ETHER_BUFFER*)NULL;
   pHandle->m_TxDequeuedCount++;
}


void LAN9218_ReceiveComplete(void)
{
   ADI_ETHER_BUFFER *tmp_q_ele;
   volatile unsigned short t_val;
   unsigned short *elnth;
   unsigned short packet_length;

   tmp_q_ele = pHandle->m_RxEnqueuedHead;
   elnth = (unsigned short *)tmp_q_ele->Data;
   packet_length = *elnth;


      /* Change the Enqueued head */
      pHandle->m_RxEnqueuedHead = (ADI_ETHER_BUFFER*)tmp_q_ele->pNext;
      pHandle->m_RxEnqueuedCount--;
      if (pHandle->m_RxEnqueuedCount == 0)
         pHandle->m_RxEnqueuedTail = (ADI_ETHER_BUFFER*)NULL;

      /* Add the packet to dequeued list*/
      if (pHandle->m_RxDequeuedCount)
         pHandle->m_RxDequeuedTail->pNext =(ADI_ETHER_BUFFER*) tmp_q_ele;
      else
         pHandle->m_RxDequeuedHead = tmp_q_ele;

      //No matter what this is also the tail.
      pHandle->m_RxDequeuedTail = tmp_q_ele;
      //And tail->next should point to NULL
      tmp_q_ele->pNext = (ADI_ETHER_BUFFER*) NULL;
      pHandle->m_RxDequeuedCount++;

      // finally we have to update the status word etc
      tmp_q_ele->ProcessedFlag = 1;
      tmp_q_ele->StatusWord = 0x3000 + packet_length;

      // we received a new packet flush and invalidate the cache line
      //
      if(pHandle->Cache)
      {
          FlushInvArea((char*)tmp_q_ele->Data+2,(char*)tmp_q_ele->Data+packet_length);
      }
      SetRegDW(g_pLAN911x, INT_STS,INT_STS_RSFL_);
}

/**************************************************************************
 *
 * Lan9218 entry point
 *
 **************************************************************************/
ADI_DEV_PDD_ENTRY_POINT ADI_ETHER_LAN9218_Entrypoint  = {
        adi_pdd_Open,
        adi_pdd_Close,
        adi_pdd_Read,
        adi_pdd_Write,
        adi_pdd_Control
};

