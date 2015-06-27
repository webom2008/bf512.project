
/*********************************************************************************
 *
 * Copyright(c) 2010 Analog Devices, Inc. All Rights Reserved.
 *
 * This software is proprietary and confidential.  By using this software you agree
 * to the terms of the associated Analog Devices License Agreement.
 *
 *********************************************************************************/
#include <lan9218.h>
#include <cdefbf548.h>
#include <ccblkfn.h>
#include "mdma_lan9218.h"

/*!
 * Macros
 */
#define ENTER_CRITICAL_REGION() (CriticalHandle =                                       \
                                 adi_int_EnterCriticalRegion(pEthernetDev->CriticalData))

#define EXIT_CRTICIAL_REGION()  (adi_int_ExitCriticalRegion(CriticalHandle)) 

#define SET_VAL_SHORT(addr,val) (((*(volatile unsigned short*)addr)) = val)

#define SET_VAL_ADDR(addr,val)  (((*(volatile void**)addr)) = ((volatile void*)(val)))

#define GET_VAL_SHORT(addr,val) (val = (*(volatile unsigned short*)addr))

/*!
 * Static variables and functions
 */
 static ADI_ETHER_LAN9218_DATA *pEthernetDev;
 static void *CriticalHandle;
 static ADI_INT_HANDLER(dma_interrupt_handler);

/*!
 * 
 * @brief Locks the dma for rx or tx transfers
 * 
 * @param pDev       pointer to the lan9218 device data
 *
 * @param direction  transfer direction the value could be
 *                   # LAN9218_DMA_DIR_RX
 *                   # LAN9218_DMA_DIR_TX
 *
 * @return           none
 */
void dma_protect(ADI_ETHER_LAN9218_DATA *pDev,DMA_DIRECTION direction)
{
     ENTER_CRITICAL_REGION();

     /* lock the dma by setting the internal dma protect variable */
     pDev->m_dma_protect = 1;

     /* set the memory dma direction */
     pDev->m_dma_direction = direction;

     EXIT_CRTICIAL_REGION();
}

/*!
 * @breif Releases the DMA so that either transmit or receive can use it.
 *
 * @param pDev       pointer to the lan9218 device data
 *
 * @return           none
 */
void dma_relinquish(struct ADI_ETHER_LAN9218_DATA *pDev)
{
    ENTER_CRITICAL_REGION();

    /* unlock the dma by clearing the internal dma protect variable */
    pDev->m_dma_protect = 0;

    pDev->m_dma_direction = LAN9218_DMA_DIR_NONE;

    EXIT_CRTICIAL_REGION();
}

/*!
 * @brief    Masks or unmask's DMA interrupt.
 *
 * @param flag
 *               flag = 1 enables the DMA interrupt in the SIC
 *               flag = 0 disables the  DMA interrupt in the SIC
 *
 * @details
 *           Mask or Unmask DMA interrupt, if the flag is true then masks the DMA int
 *           else unmasks the dma interrupt. LAN9218 MDMA driver by default uses 
 *           MDMA stream 2.
 *
 * FIXME: Use adi_SIC_Enable API, hardcoded IMASK1
 *
 */
void dma_mask_en(int flag)
{
    unsigned int mask;
    unsigned int imask = cli();

    /* get current imask */
    mask = *pSIC_IMASK1;

    /* if flag is true enable the interrupt */
    if(flag)
        mask |= 1 << pEthernetDev->DmaSICMaskBit;
    else
        mask &= ~(0x0L | (1 << pEthernetDev->DmaSICMaskBit));

    *pSIC_IMASK1 = mask;

    ssync();

    sti(imask);
}

/*!
 *
 * Initialize DMA and hook dma interrupt handler
 *
 */
void init_dma(ADI_ETHER_LAN9218_DATA *pDev)
{
    // set the global static to access pEthernetDevice data
    pEthernetDev = pDev;

    // set the DMA error count to 0
    pEthernetDev->MemDmaErrorCount = 0;

    // set DMA stream bit in SIC.
    dma_mask_en(1);

    // Hook DMA interrupt Handler FIXME: always hooking to default mdma ivg level
    adi_int_CECHook(ik_ivg13,(ADI_INT_HANDLER_FN)dma_interrupt_handler,pEthernetDev,FALSE);
}

/*!
 *
 * DMA interrupt handler, Upon completion of the DMA operation this will get
 * called.
 *
 */
static ADI_INT_HANDLER(dma_interrupt_handler)
{
    ADI_DCB_RESULT DcbResult;
    ADI_INT_HANDLER_RESULT HandlerResult = ADI_INT_RESULT_NOT_PROCESSED;
    static int CheckTxQueueCnt = 0;
    short dma_status = 0x0;

    // check if dma is locked or not. If dma is not locked the interrupt
    // might be with other mdma channels
    //
    if(pEthernetDev->m_dma_protect == 0)
    {
        return (HandlerResult);
    }

    ENTER_CRITICAL_REGION();

    // get dma status register
    GET_VAL_SHORT((pEthernetDev->DstStreamBaseAddr+OFFSET_IRQ_STATUS),dma_status);

    // check if DMA is indeed completed - this may happen if another MDMA is active 
    // and the handlers are chained
    if(dma_status & DMA_RUN)
    {
       EXIT_CRTICIAL_REGION();
       return (HandlerResult);     
    }

    // check for the dma complete.
    if(dma_status & DMA_DONE)
    {
        dma_status |= DMA_DONE;
        // acknowledge DMA  completion
        SET_VAL_SHORT((pEthernetDev->DstStreamBaseAddr+OFFSET_IRQ_STATUS),dma_status);
    }
    // DMA error
    else if (dma_status & DMA_ERR)
    {
        pEthernetDev->MemDmaErrorCount++;
        dma_status |= DMA_DONE;
        // we will release the DMA
        dma_relinquish(pEthernetDev);
        // acknowledge dma error
        SET_VAL_SHORT((pEthernetDev->DstStreamBaseAddr+OFFSET_IRQ_STATUS),dma_status);

        // enable SMSC interrupts
        LAN9218_RestoreAndEnableInts();
     }
     ssync();

    switch(pEthernetDev->m_dma_direction)
    {
        //
        // Transmit Complete
        //
        case LAN9218_DMA_DIR_TX:
                        LAN9218_TransmitComplete();

                        dma_relinquish(pEthernetDev);

                        EXIT_CRTICIAL_REGION();

                        if (pEthernetDev->DCBHandle != NULL)
                        {
                            DcbResult = adi_dcb_Post(pEthernetDev->DCBHandle,
                                                  0,
                                                  pEthernetDev->DMCallback, 
                                                  pEthernetDev->DeviceHandle,
                                                  ADI_ETHER_EVENT_FRAME_XMIT,
                                                  pEthernetDev->m_TxDequeuedHead->CallbackParameter);
                        }
                        else
                        {
                          (pEthernetDev->DMCallback)(pEthernetDev->DeviceHandle, 
                                            ADI_ETHER_EVENT_FRAME_XMIT,
                                            pEthernetDev->m_TxDequeuedHead->CallbackParameter);

                           DcbResult = ADI_DCB_RESULT_SUCCESS;
                        }

                        if (DcbResult == ADI_DCB_RESULT_SUCCESS) 
                        {
                            //## what happens if a packet is trasnmitted while in the callback
                            pEthernetDev->m_TxDequeuedHead = NULL;
                            pEthernetDev->m_TxDequeuedTail = NULL;
                            pEthernetDev->m_TxDequeuedCount = 0;
                        }
         break;
        //
        // Receive Complete
        //
        case LAN9218_DMA_DIR_RX:

                         // receive complete
                         LAN9218_ReceiveComplete();

                         // relinquish dma
                         dma_relinquish(pEthernetDev);

                         EXIT_CRTICIAL_REGION();

                        if (pEthernetDev->m_RxDequeuedHead != NULL)
                        {
                                if (pEthernetDev->DCBHandle!=NULL) 
                                {
                                    DcbResult = adi_dcb_Post(pEthernetDev->DCBHandle,
                                                        0,
                                                        pEthernetDev->DMCallback,
                                                        pEthernetDev->DeviceHandle,
                                                        ADI_ETHER_EVENT_FRAME_RCVD, 
                                                        pEthernetDev->m_RxDequeuedHead->CallbackParameter);
                                } 
                                else
                                {
                                    (pEthernetDev->DMCallback)(pEthernetDev->DeviceHandle, 
                                                      ADI_ETHER_EVENT_FRAME_RCVD, 
                                                      pEthernetDev->m_RxDequeuedHead->CallbackParameter); 

                                     DcbResult = ADI_DCB_RESULT_SUCCESS;
                                }

                                if (DcbResult == ADI_DCB_RESULT_SUCCESS) 
                                {
                                   //## what happens if a packet is received while in the callback
                                   pEthernetDev->m_RxDequeuedHead = NULL;
                                   pEthernetDev->m_RxDequeuedTail = NULL;
                                   pEthernetDev->m_RxDequeuedCount = 0;  
                                }
                         }
          break;
          //
          // default
          //
          default:
                     break;
    } // switch end

    // enable interrupt again
    LAN9218_RestoreAndEnableInts();

    // we check if there is any xmted packets to be sent
    if( !(CheckTxQueueCnt & 0x1) && (pEthernetDev->m_TxEnqueuedCount > 0))
    {
        ENTER_CRITICAL_REGION();

        // lock dma for xmt
        dma_protect(pEthernetDev,LAN9218_DMA_DIR_TX);

        // save off the LAN9218 interrupt mask
        LAN9218_SaveAndDisableInts();

        if(!LAN9218_SendTxEnqueuedPacket(pEthernetDev))
        {
            dma_relinquish(pEthernetDev);
            LAN9218_RestoreAndEnableInts();
        }
    

        EXIT_CRTICIAL_REGION();
    }

    CheckTxQueueCnt++;
    if(CheckTxQueueCnt == UINT_MAX) CheckTxQueueCnt = 0;

    return(HandlerResult);
}

/******************************************************************************
 *
 * Transfer  num_bytes from source address to the destination address
 *
 *****************************************************************************/
void dma_initiate_transfer(unsigned int src_addr, unsigned int des_addr,
                                                  unsigned int num_bytes,DMA_DIRECTION dir)
{
    unsigned short  dma_config_source;
    unsigned short  dma_config_destination;
    unsigned short  d0_x_modify, s0_x_modify;

    ENTER_CRITICAL_REGION();

    if(dir == LAN9218_DMA_DIR_TX) 
    {
        d0_x_modify = 0; // auto increment, same port.
        s0_x_modify = DMA_WORD_SIZE;
    } 
    else 
    {        
        d0_x_modify = DMA_WORD_SIZE;
        s0_x_modify = 0; // auto inrement, same port.
    }

    // configure the source address register
    SET_VAL_ADDR((pEthernetDev->SrcStreamBaseAddr+OFFSET_START_ADDR),src_addr);
    // configure number of bytes to send at the source end
    SET_VAL_SHORT((pEthernetDev->SrcStreamBaseAddr+OFFSET_X_COUNT),(unsigned short)num_bytes);
    // configure modify at the source end
    SET_VAL_SHORT((pEthernetDev->SrcStreamBaseAddr+OFFSET_X_MODIFY),(unsigned short)s0_x_modify);

    // configure the source address register
    SET_VAL_ADDR((pEthernetDev->DstStreamBaseAddr+OFFSET_START_ADDR),des_addr);

    //configure destination x_count register
    SET_VAL_SHORT((pEthernetDev->DstStreamBaseAddr+OFFSET_X_COUNT),(unsigned short)num_bytes);
    // configure destiantion x_modify register
    SET_VAL_SHORT((pEthernetDev->DstStreamBaseAddr+OFFSET_X_MODIFY),(unsigned short)d0_x_modify);

    // Configure source DMA config register, enable bit set, and transfer
    // size of 32 bits.
    //
    dma_config_source = (WDSIZE_32 | DMAEN);
    SET_VAL_SHORT((pEthernetDev->SrcStreamBaseAddr+OFFSET_CONFIG),(unsigned short)dma_config_source);

    // Configure destination config register, enable DMA, transfer size 32
    // bits and enable DMA completion interrupt
    //
    dma_config_destination = (DI_EN | WDSIZE_32 | DMAEN | WNR); 
    // DMA transfer starts here.
    SET_VAL_SHORT((pEthernetDev->DstStreamBaseAddr+OFFSET_CONFIG),(unsigned short)dma_config_destination);
    ssync();
        
    EXIT_CRTICIAL_REGION();
}

/* EOF */

