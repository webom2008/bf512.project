
/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.



*********************************************************************************/

/* disable misra diagnostics as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#include <services/services_misra_suppress.h>
#endif /* _MISRA_RULES */

/*********************************************************************

Include files

*********************************************************************/
#include <drivers/ethernet/ADI_ETHER.h>

/*********************************************************************

User configurable items

*********************************************************************/



/*********************************************************************

Extensible enumerations and defines

*********************************************************************/


enum {										/* Command IDs	*/
	ADI_ETHER_BF537_CMD_FIRST=ADI_ETHER_CMD_SPECIFIC_START,	/* insure this order remains	*/
	ADI_ETHER_CMD_BF537_NO_RCVE_LNTH,
	ADI_ETHER_CMD_BF537_STRIP_PAD,
	ADI_ETHER_CMD_BF537_USE_IVG,
	ADI_ETHER_CMD_BF537_USE_DMA,
	ADI_ETHER_CMD_BF537_CLKIN,
	ADI_ETHER_CMD_BF537_SET_PHY_ADDR,
	ADI_ETHER_CMD_BF537_SET_TRACE,
	ADI_ETHER_CMD_BF537_GET_TRACE,
	ADI_ETHER_CMD_BF537_SET_PHY_INT_MASK,
	ADI_ETHER_CMD_BF537_GET_PHY_INT_MASK
};


enum {										/* Events	*/
	ADI_ETHER_EVENT_BF537_FIRST=ADI_ETHER_EVENT_SPECIFIC_START
};


enum {								/* Return codes	*/
	ADI_ETHER_BF537_RESULT_START=ADI_ETHER_RESULT_SPECIFIC_START
};

#define ADI_ETHER_MEM_BF537_BASE_SIZE 0x180
#define ADI_ETHER_MEM_BF537_PER_RECV 0x3C
#define ADI_ETHER_MEM_BF537_PER_XMIT 0x48


/*********************************************************************

Data Structures

*********************************************************************/
extern ADI_DEV_PDD_ENTRY_POINT ADI_ETHER_BF537_Entrypoint;		/* entry point to the device driver	*/



typedef struct adi_ether_bf537_trace_info {
	void 		*Mem;				/* memory area to be used to hold trace data */
	int			LnthMem;			/* length of memory area */
	int			FirstByte;			/* offset of first byte of each frame to be trace */
	int			MaxBytes;			/* max. number of bytes of frame to be added to trace buffer */
} ADI_ETHER_BF537_TRACE_INFO;

typedef struct adi_ether_bf537_trace_entry {
	u16			NoBytes;			/* no of bytes of frame data in */
	u8			Dirn;				/* 'T' for transmit , 'R' for received */
	u8			Seqn;				/* incrmeenting sequence number */
	u8			Data[1];			/* data from the frame */
} ADI_ETHER_BF537_TRACE_ENTRY;

typedef struct adi_ether_bf537_trace_data {
	ADI_ETHER_BF537_TRACE_ENTRY		*BaseEntry;	/* start of the trace area */
	ADI_ETHER_BF537_TRACE_ENTRY		*EndOfData;	/* address imm. following end of buffer */
	ADI_ETHER_BF537_TRACE_ENTRY 	*OldestEntry;	/* pointer to oldest entry */
	int			EntryLnth;			/* length of each trace entry */
	int			NoOfEntries;		/* no. of filled entries */
} ADI_ETHER_BF537_TRACE_DATA;

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */



/********************************************************************************/
