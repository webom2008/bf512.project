/*********************************************************************************
 *
 * Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.
 *
 * This software is proprietary and confidential.  By using this software you agree
 * to the terms of the associated Analog Devices License Agreement.
 *
 *********************************************************************************/
#ifndef __MDMA_LAN91C111_H_
#define __MDMA_LAN91C111_H_
#include <lan91c111.h>
#define DMA_WORD_SIZE 4
typedef enum 
{
	DMA_DIR_RX,
	DMA_DIR_TX
}DMA_DIRECTION;

void dma_initiate_transfer(unsigned long src_addr, unsigned long des_addr, unsigned long num_bytes,DMA_DIRECTION dir);
 
void dma_protect(void *pli_handle,DMA_DIRECTION direction);

#endif /* __MDMA_LAN91C111_H_ */
