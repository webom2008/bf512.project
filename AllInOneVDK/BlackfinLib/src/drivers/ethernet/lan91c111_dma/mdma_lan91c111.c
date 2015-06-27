/*********************************************************************************
 *
 * Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.
 *
 * This software is proprietary and confidential.  By using this software you agree
 * to the terms of the associated Analog Devices License Agreement.
 *
 *********************************************************************************/

#define USE_16_BIT

#include <lan91c111.h>
#include <cdefbf533.h>
#include <ccblkfn.h>
#include <limits.h>

int num_dma_protect_zeros=0;
static ADI_ETHER_LAN91C111_DATA *dev=0;
extern void *drv_CriticalHandle;
int SendTxEnqueuedPacket(ADI_ETHER_LAN91C111_DATA *dev);
void LAN91C111_enable_int (unsigned char IRQ);
static int resend_cnt;
#define ENTER_CRITICAL_REGION() (drv_CriticalHandle=adi_int_EnterCriticalRegion(dev->CriticalData))
#define EXIT_CRTICIAL_REGION()  (adi_int_ExitCriticalRegion(drv_CriticalHandle)) 
// MDMA register offsets from the base register
//
#define  OFFSET_NXT_DES      0x0
#define  OFFSET_START_ADDR   0x4
#define  OFFSET_CONFIG       0x8 
#define  OFFSET_X_COUNT      0x10
#define  OFFSET_X_MODIFY     0x14
#define  OFFSET_Y_COUNT      0x18
#define  OFFSET_Y_MODIFY     0x1C
#define  OFFSET_IRQ_STATUS   0x28

/* sets values for the DMA registers */
#define SET_VAL_SHORT(addr,val) (((*(volatile unsigned short*)addr)) = val)
#define SET_VAL_ADDR(addr,val) (((*(volatile void**)addr)) = ((volatile void*)(val)))
#define GET_VAL_SHORT(addr,val) (val = (*(volatile unsigned short*)addr))

// Defined in lan91c111.c
// 
extern "C" transmit_complete(void);
extern "C" receive_complete(void);
void dma_relinquish(void);

/******************************************************************************
 * if mas_int is 1 it masks the ethernet interrupt. if its 0 unmasks it.
 *****************************************************************************/
#pragma optimize_off
void mask_ethernet_int(int mask_int)
{
     if(mask_int)
        *pSIC_IMASK &= ~(1 << dev->DmaSICMaskBit);
     else
        *pSIC_IMASK |= 1 << dev->DmaSICMaskBit;
}

/******************************************************************************
 * If any DMA operation is in progress then this will block. Else it will
 * lock the DMA and return
 *****************************************************************************/
void dma_protect(ADI_ETHER_LAN91C111_DATA *d,DMA_DIRECTION direction)
{
     dev->m_dma_protect = 1;
     dev->m_dma_direction = direction;

}

/******************************************************************************
 * releases the DMA so that either transmit or receive can use it.
 *****************************************************************************/
void dma_relinquish()
{
	ENTER_CRITICAL_REGION();
	dev->m_dma_protect = 0;
	dev->m_dma_direction = DMA_NONE;
	// set destination config to 0
	SET_VAL_SHORT((dev->DstStreamBaseAddr+OFFSET_CONFIG),(unsigned short)0x0);
	// set source config to 0
	SET_VAL_SHORT((dev->SrcStreamBaseAddr+OFFSET_CONFIG),(unsigned short)0x0);
	EXIT_CRTICIAL_REGION();
}

/******************************************************************************
 * Mask or Unmask DMA interrupt, if the flag is true then masks the DMA int
 * else unmasks the dma interrupt
 * flag = 1 means enables the DMA interrupt in the SIC
 * flag = 0 means disables the  DMA interrupt in the SIC
 *****************************************************************************/
void dma_mask_en(int flag)
{
unsigned int mask;

    mask = *pSIC_IMASK;

    // 1 means enable
    if(flag)
       mask |= 1 << dev->DmaSICMaskBit;
    else
       mask &= ~(0x0L | (1 << dev->DmaSICMaskBit));

    *pSIC_IMASK = mask;
    ssync();
}

/******************************************************************************
 * Rests the MDMA config registers and sets MDMA bit in SIC
 *****************************************************************************/
void init_dma(ADI_ETHER_LAN91C111_DATA *d)
{
	// set the global static to access device data
	dev =d;
	// set destination config to 0
	SET_VAL_SHORT((dev->DstStreamBaseAddr+OFFSET_CONFIG),(unsigned short)0x0);
	// set source config to 0
	SET_VAL_SHORT((dev->SrcStreamBaseAddr+OFFSET_CONFIG),(unsigned short)0x0);
	// set the DMA error count to 0
	dev->MemDmaErrorCount=0;
	// set DMA stream bit in SIC. Stream0 its 21 and 22 for stream1
	dma_mask_en(1);
}
/******************************************************************************
 * DMA interrupt handler, Upon completion of the DMA operation this will get
 * called.
 *****************************************************************************/
void dma_interrupt_handler(void *arg,unsigned int v, unsigned int g)
{
	short dma_status=0x0;
	ADI_DCB_RESULT result;
	static int recall_count=0;


	// NO DMA in action. May be spurious interrupt.
	if(dev->m_dma_protect == 0)
	{
		num_dma_protect_zeros++;
		return;
	}

	ENTER_CRITICAL_REGION();

	// set destination config register
	SET_VAL_SHORT((dev->DstStreamBaseAddr+OFFSET_CONFIG),(unsigned short)0x0);
	// set source config register
	SET_VAL_SHORT((dev->SrcStreamBaseAddr+OFFSET_CONFIG),(unsigned short)0x0);
	// get dma status register
	GET_VAL_SHORT((dev->DstStreamBaseAddr+OFFSET_IRQ_STATUS),dma_status);

	// check for the dma complete.
	//
	if(dma_status & DMA_DONE)
	{
		dma_status |= DMA_DONE;
		// acknowledge DMA  completion
		SET_VAL_SHORT((dev->DstStreamBaseAddr+OFFSET_IRQ_STATUS),dma_status);
	}
	// DMA error
	else if (dma_status & DMA_ERR)
	{
		dev->MemDmaErrorCount++;
		dma_status |= DMA_DONE;
		// we will release the DMA
		dma_relinquish();
		// acknowledge dma error
		SET_VAL_SHORT((dev->DstStreamBaseAddr+OFFSET_IRQ_STATUS),dma_status);
		// enable SMSC interrupts
		LAN91C111_enable_int(SMC_INTERRUPT_MASK);
	}
	ssync();

	switch(dev->m_dma_direction)
	{
		//
		// Transmit Complete
		//
		case DMA_DIR_TX:
				transmit_complete();
				EXIT_CRTICIAL_REGION();

        if (dev->DCBHandle != NULL){
                      result = adi_dcb_Post(dev->DCBHandle,
                                            0,
                                            dev->DMCallback, 
                                            dev->DeviceHandle,
                                            ADI_ETHER_EVENT_FRAME_XMIT,
                                            dev->m_TxDequeuedHead->CallbackParameter);
     		}
                else{
                          (dev->DMCallback)(dev->DeviceHandle, 
                                            ADI_ETHER_EVENT_FRAME_XMIT,
                                            dev->m_TxDequeuedHead->CallbackParameter);
			     	result = ADI_DCB_RESULT_SUCCESS;
     		}

               if (result == ADI_DCB_RESULT_SUCCESS) {
		//## what happens if a packet is trasnmitted while in the callback
                    dev->m_TxDequeuedHead = NULL;
                    dev->m_TxDequeuedTail = NULL;
                    dev->m_TxDequeuedCount = 0;
		}
               break;
		//
		// Receive Complete
		//
		case DMA_DIR_RX:
                         receive_complete();
                         dma_relinquish();
                         EXIT_CRTICIAL_REGION();

			if (dev->m_RxDequeuedHead != NULL){
				if (dev->DCBHandle!=NULL) {
                                    result = adi_dcb_Post(dev->DCBHandle,
                                                        0,
                                                        dev->DMCallback,
                                                        dev->DeviceHandle,
                                                        ADI_ETHER_EVENT_FRAME_RCVD, 
							  dev->m_RxDequeuedHead->CallbackParameter); } 
                      else{
                               (dev->DMCallback)(dev->DeviceHandle, 
				  ADI_ETHER_EVENT_FRAME_RCVD, 
				  dev->m_RxDequeuedHead->CallbackParameter); 
                               result = ADI_DCB_RESULT_SUCCESS;
			}
                     if (result == ADI_DCB_RESULT_SUCCESS) {
					//## what happens if a packet is received while in the callback
                         dev->m_RxDequeuedHead = NULL;
                         dev->m_RxDequeuedTail = NULL;
                         dev->m_RxDequeuedCount = 0;  
			}
		}
		break;
		//
		// default
		//
		default:
		     break;
	} // switch end

	// we check if there is any xmted packets to be sent
	// we try on every odd packet.
	//
	if( (recall_count & 0x1) && (dev->m_TxEnqueuedCount > 0)){
		ENTER_CRITICAL_REGION();
		dma_protect(dev,DMA_DIR_TX);
		resend_cnt++;
		if(!SendTxEnqueuedPacket(dev)){
	  	  dma_relinquish();
  		  LAN91C111_enable_int(SMC_INTERRUPT_MASK);
		}
		EXIT_CRTICIAL_REGION();
	}
	else {
		dma_relinquish();
  	LAN91C111_enable_int(SMC_INTERRUPT_MASK);
	}

	recall_count++;
	if(recall_count == UINT_MAX) recall_count =0;
}

/******************************************************************************
 * Trasfer  num_bytes from source address to the destination address
 *
 *****************************************************************************/
void dma_initiate_transfer(unsigned long src_addr, unsigned long des_addr,
						   unsigned long num_bytes,DMA_DIRECTION dir)
{
	unsigned short 	dma_config_source;
	unsigned short 	dma_config_destination;
	unsigned short 	d0_x_modify, s0_x_modify;

	ENTER_CRITICAL_REGION();

	if(dir == DMA_DIR_TX) {
		d0_x_modify = 0; // auto increment, same port.
		s0_x_modify = DMA_WORD_SIZE;
	} else {	
		d0_x_modify = DMA_WORD_SIZE;
		s0_x_modify = 0; // auto inrement, same port.
	}

	// configure the source address register
	SET_VAL_ADDR((dev->SrcStreamBaseAddr+OFFSET_START_ADDR),src_addr);
	// configure number of bytes to send at the source end
	SET_VAL_SHORT((dev->SrcStreamBaseAddr+OFFSET_X_COUNT),(unsigned short)num_bytes);
	// configure modify at the source end
	SET_VAL_SHORT((dev->SrcStreamBaseAddr+OFFSET_X_MODIFY),(unsigned short)s0_x_modify);

	// configure destination address register
	//*pMDMA_D0_START_ADDR =	(volatile void*)des_addr;
	// configure the source address register
	SET_VAL_ADDR((dev->DstStreamBaseAddr+OFFSET_START_ADDR),des_addr);

	//configure destination x_count register
	SET_VAL_SHORT((dev->DstStreamBaseAddr+OFFSET_X_COUNT),(unsigned short)num_bytes);
	// configure destiantion x_modify register
	SET_VAL_SHORT((dev->DstStreamBaseAddr+OFFSET_X_MODIFY),(unsigned short)d0_x_modify);

	// Configure source DMA config register, enable bit set, and transfer
	// size if 16 bits.
	//
	dma_config_source = (WDSIZE_16 | DMAEN);
	SET_VAL_SHORT((dev->SrcStreamBaseAddr+OFFSET_CONFIG),(unsigned short)dma_config_source);

	// Configure destination config register, enable DMA, transfer size 16
	// bits and enable DMA completion interrupt
	//
	dma_config_destination = (DI_EN | WDSIZE_16 | DMAEN | WNR); 
	// DMA transfer starts here.
	SET_VAL_SHORT((dev->DstStreamBaseAddr+OFFSET_CONFIG),(unsigned short)dma_config_destination);
	ssync();
	
	EXIT_CRTICIAL_REGION();

}

