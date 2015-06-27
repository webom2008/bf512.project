/*********************************************************************************
 *
 * Copyright(c) 2010 Analog Devices, Inc. All Rights Reserved.
 *
 * This software is proprietary and confidential.  By using this software you agree
 * to the terms of the associated Analog Devices License Agreement.
 *
 *********************************************************************************/
#ifndef __MDMA_LAN9218_H_
#define __MDMA_LAN9218_H_

   /*
    * Macros
    */
    #define DMA_WORD_SIZE       (4)      /* DMA transfer size in bytes         */
    #define OFFSET_NXT_DES      (0x0)    /* MDMA next descriptor offset        */          
    #define OFFSET_START_ADDR   (0x4)    /* MDMA start or source address offset*/
    #define OFFSET_CONFIG       (0x8)    /* MDMA configuration register offset */
    #define OFFSET_X_COUNT      (0x10)   /* X-count register offset            */
    #define OFFSET_X_MODIFY     (0x14)   /* X-modify register offset           */
    #define OFFSET_Y_COUNT      (0x18)   /* Y-count register offset            */
    #define OFFSET_Y_MODIFY     (0x1C)   /* Y-modify register offset           */
    #define OFFSET_IRQ_STATUS   (0x28)   /* MDMA irq status register offset    */


   /*
    * Data types
    */
    struct ADI_ETHER_LAN9218_DATA;       /* LAN9218 driver data block          */

    
    typedef enum                         /* MDMA direction                     */
    {
        LAN9218_DMA_DIR_RX=0,
        LAN9218_DMA_DIR_TX,
        LAN9218_DMA_DIR_NONE
    }DMA_DIRECTION;


   /*
    * Function proto-types
    */
    void init_dma(struct ADI_ETHER_LAN9218_DATA *pDev);  

    void dma_initiate_transfer(unsigned int  SrcAddress,
                               unsigned int  DstAddress,
                               unsigned int  NumBytes,
                               DMA_DIRECTION Dir);

    void dma_protect(struct ADI_ETHER_LAN9218_DATA *pDev, DMA_DIRECTION Dir);

    void dma_relinquish(struct ADI_ETHER_LAN9218_DATA *pDev);


#endif /* __MDMA_LAN91C111_H_ */
