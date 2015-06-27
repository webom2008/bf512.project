/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_dcb.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			Deferred Callback (DCB) Manager Module

*********************************************************************************/

#include <services/services.h>

#if !defined(__ADSPBF561__)
/********************************************************************************
* Define a NULL semaphore for critical region 
* in the order they are received.
*********************************************************************************/
#define ADI_DCB_CRITICAL_SEMAPHORE NULL
#else
//static section("sm") u8 adi_dcb_semaphore;
//#define ADI_DCB_CRITICAL_SEMAPHORE (&adi_dcb_semaphore)
#endif

/********************************************************************************
* Set up a unique time stamp variable & access macro to enable jobs to be completed
* in the order they are received.
*********************************************************************************/

// use a 32-bit sequence number 
static u32 SequenceNumber=0;
#define ADI_DCB_TIMESTAMP SequenceNumber++;  // will roll over to 0 at top


/********************************************************************************
* Callback Entry structure
*********************************************************************************/
#pragma pack(4)
typedef struct ADI_DCB_ENTRY
{
	u32						priority;			// Priority level
	u32						timestamp;			// time stamp (from RTC_STAT)
	ADI_DCB_CALLBACK_FN		ClientCallback;		// Client Callback function 
	void					*pHandle;			// 1st argument to callback
	u32						u32Arg;				// 2nd argument to callback
	void					*pArg;			// 3rd argument to callback
} ADI_DCB_ENTRY;
#pragma pack()

/********************************************************************************
* Callback pQueue Server structure
*********************************************************************************/
#pragma pack(4)
typedef struct ADI_DCB_SERVER
{
	ADI_DCB_ENTRY_HDR		header;				// Header for use with VDK
	u32						IVG;				// IVG level that queue dispatcher is to run
	ADI_DCB_ENTRY			*pQueue;			// The queue of callback entries
	u16						NumEntries;			// The maximum number of entries in the queue
	u16						QueueSize;			// Counter (ISR exits when ==0)
	ADI_DCB_ENTRY			*pCurrEntry;		// Pointer to the current available entry for posting
	u32						currPriority;		// Current Priority being processed
	ADI_DCB_ENTRY			*pStartNextPriority;// Pointer to first entry of next priority to process
	u16						maxQueueSize;		// watermark to keep track of max jobs on queue at any time
} ADI_DCB_SERVER;
#pragma pack()

/********************************************************************************
* Deferred Callback Manager structure
*********************************************************************************/
#pragma pack(4)
typedef struct ADI_DCB_MEM_ASSIGN 
{
	ADI_DCB_SERVER *Servers;					// List of queue servers
	u32 NumServers;					// Maximum number of queue servers
} ADI_DCB_MEM_ASSIGN;
#pragma pack()


/********************************************************************************
* The DCB manager instance
*********************************************************************************/
static ADI_DCB_MEM_ASSIGN adi_dcb_Manager = {0,0};

/********************************************************************************
* Macro to iterate the list of servers
*********************************************************************************/
#define ADI_DCB_SERVER_ITERATOR(ptr,i) \
	(ptr=adi_dcb_Manager.Servers, i=0; i < adi_dcb_Manager.NumServers; i++,ptr++)

/********************************************************************************
* Convenience Macro to set a value for an inactive queue entry
*********************************************************************************/
#define ADI_DCB_INACTIVE_ENTRY ((ADI_DCB_CALLBACK_FN)-1)
#define ADI_DCB_AVAILABLE_ENTRY ((u32)-1)
#define ADI_DCB_INACTIVE_QUEUE ((u32)-1)
#define ADI_DCB_NO_NEXT_PRIORITY ((u32)-1)

/********************************************************************************
* global for totalling all queues
*********************************************************************************/
#ifdef ADI_SSL_DEBUG
u32 adi_dbc_DispatchCount=0;
#endif

/********************************************************************************
* Private functions
*********************************************************************************/
EX_REENTRANT_HANDLER( adi_dcb_ISR );
ADI_INT_HANDLER (adi_dcb_DispatchCallbacks);
extern void adi_dcb_RegisterISR( u16 IvgLevel,ADI_INT_HANDLER_FN Dispatcher,ADI_DCB_HANDLE hServer);
extern void adi_dcb_Forward(ADI_DCB_ENTRY_HDR* Entry, u16 IvgLevel, u16 Priority);
static void adi_dcb_DeferredFunction(ADI_DCB_ENTRY_HDR* Entry);
#ifdef ADI_SSL_DEBUG
static ADI_DCB_RESULT adi_dcb_ValidateHandle(ADI_DCB_HANDLE hServer);
#endif


/********************************************************************************
* Critical Region builtins
*********************************************************************************/
static void *_hCriticalRegionData;

#define ADI_INT_ENTER_CRITICAL_REGION		\
			tmpIMASK = adi_int_EnterCriticalRegion(_hCriticalRegionData)
#define ADI_INT_EXIT_CRITICAL_REGION	\
			adi_int_ExitCriticalRegion(tmpIMASK)


/*********************************************************************

Include operating environment specific functionality

*********************************************************************/

#if defined(ADI_SSL_STANDALONE)
#include "adi_dcb_standalone.c"
#endif

#if defined(ADI_SSL_VDK)
// no routines are required for VDK as the entry points are defined in VDK.
#endif

#if defined(ADI_SSL_THREADX)
#include "adi_dcb_threadx.c"
#endif


#if defined(ADI_SSL_INTEGRITY)
#include "adi_dcb_dcbegrity.c"
#endif

#if defined(ADI_SSL_UCOS)
#include "adi_dcb_ucos.c"
#endif

/*********************************************************************************
*********************************************************************************

						THE API FUNCTIONS

*********************************************************************************
*********************************************************************************/


/*********************************************************************************
* Function:		adi_dcb_Init
* Description:	Initializes the DCB Manager
*********************************************************************************/


ADI_DCB_RESULT 
adi_dcb_Init(
		void *ServerMemData,
		size_t szServer,
		u32 *NumServers,
		void *hCriticalRegionData
		)
{
	int i;

	// debug
#if defined(ADI_SSL_DEBUG)
	if (ADI_DCB_QUEUE_SIZE != sizeof(ADI_DCB_SERVER)) {
		return (ADI_DCB_RESULT_BAD_QUEUE_SIZE_MACRO);
	}
	if (ADI_DCB_ENTRY_SIZE != sizeof(ADI_DCB_ENTRY)) {
		return (ADI_DCB_RESULT_BAD_ENTRY_SIZE_MACRO);
	}
#endif
	
	// Initialisation can only be done once per core
	if (adi_dcb_Manager.NumServers!=0)
		return ADI_DCB_RESULT_ALREADY_INITIALIZED;

	*NumServers = szServer/sizeof(ADI_DCB_SERVER);
	
#ifdef ADI_SSL_DEBUG
	if (*NumServers==0)
		return ADI_DCB_RESULT_NO_MEMORY;
#endif

	// assign memory to server array
	adi_dcb_Manager.Servers = (ADI_DCB_SERVER*)ServerMemData;
	adi_dcb_Manager.NumServers = *NumServers;

	_hCriticalRegionData = hCriticalRegionData;

	// assign default values
	for (i=0;i<*NumServers;i++)
		adi_dcb_Manager.Servers[i].IVG = ADI_DCB_INACTIVE_QUEUE;

	return ADI_DCB_RESULT_SUCCESS;
}

/*********************************************************************************
* Function:		adi_dcb_Open
* Description:	Opens a queue server 
*********************************************************************************/

ADI_DCB_RESULT 
	adi_dcb_Open(
		u32			   IvgLevel,
		void		   *QueueMemData,
		size_t		   szQueue,
		u32			   *NumEntries,
		ADI_DCB_HANDLE *hServer
		)
{
	u32 i;
	void *tmpIMASK;
	ADI_DCB_SERVER *pServer;

	if (adi_dcb_Manager.NumServers==0)
		return ADI_DCB_RESULT_NOT_INITIALIZED;

#ifdef ADI_SSL_DEBUG
	// Check that an existing server is not already using this IVG level
    ADI_INT_ENTER_CRITICAL_REGION;
	for ADI_DCB_SERVER_ITERATOR(pServer,i)
	{
		if (pServer->IVG == IvgLevel) {
        	ADI_INT_EXIT_CRITICAL_REGION;
			return ADI_DCB_RESULT_QUEUE_IN_USE;
		}
	}
	ADI_INT_EXIT_CRITICAL_REGION;
#endif

    // protect us
    ADI_INT_ENTER_CRITICAL_REGION;

	// locate a free server slot
	for ADI_DCB_SERVER_ITERATOR(pServer,i)
	{
		if (pServer->IVG == ADI_DCB_INACTIVE_QUEUE)
			break;
	}

	// reserve this slot
	pServer->IVG = IvgLevel;

	// unprotect us
	ADI_INT_EXIT_CRITICAL_REGION;

#ifdef ADI_SSL_DEBUG
	// return if no free slot.
	if (i == adi_dcb_Manager.NumServers) 
		return ADI_DCB_RESULT_NO_MEMORY;
#endif

	*NumEntries = szQueue/sizeof(ADI_DCB_ENTRY);

#ifdef ADI_SSL_DEBUG
	// return if cannot form queue
	if (*NumEntries==0)
		return ADI_DCB_RESULT_NO_MEMORY;
#endif

	// initialize the queue
	pServer->NumEntries = *NumEntries;
	pServer->pQueue = (ADI_DCB_ENTRY*)QueueMemData;
	// set current entry to last so circptr calc starts queue at first item
	pServer->pCurrEntry = &pServer->pQueue[pServer->NumEntries-1];
	pServer->QueueSize = 0;

	pServer->header.pDeferredFunction = (ADI_DCB_DEFERRED_FN)adi_dcb_DispatchCallbacks;
	for (i=0;i<pServer->NumEntries;i++) 
	{
		pServer->pQueue[i].ClientCallback = ADI_DCB_INACTIVE_ENTRY;
		pServer->pQueue[i].priority = ADI_DCB_AVAILABLE_ENTRY;
	}

	// Register the DCB dispatcher on the given IVG level
	// (if using VDK this does nothing)

	// tell the client the handle
	*hServer = (ADI_DCB_HANDLE)pServer;

	adi_dcb_RegisterISR(IvgLevel,adi_dcb_DispatchCallbacks,pServer);

	return ADI_DCB_RESULT_SUCCESS;
}

/*********************************************************************************
* Function:		adi_dcb_Post
* Description:	Posts a callback to the specified queue server 
*********************************************************************************/

ADI_DCB_RESULT  
adi_dcb_Post(
		ADI_DCB_HANDLE		hServer,
		u32					Priority,
		ADI_DCB_CALLBACK_FN Callback,
		void				*pHandle,
		u32				    u32Arg,
		void				*pArg
		)
{
	ADI_DCB_SERVER *pServer = (ADI_DCB_SERVER *)hServer;
	ADI_DCB_ENTRY *pEntry;
#ifdef ADI_SSL_DEBUG
	ADI_DCB_RESULT result;
#endif
	u32 count;
	void *tmpIMASK;

#ifdef ADI_SSL_DEBUG

	// validate the Server handle
	result = adi_dcb_ValidateHandle(hServer);
	if (result != ADI_DCB_RESULT_SUCCESS)
		return result;

	// abort if no entries left
	if (pServer->QueueSize == pServer->NumEntries)
	{
		return ADI_DCB_RESULT_NO_MEMORY;
	}

#endif

	// increment (circular) pointer to queue until we locate next free entry
	count=0;
	ADI_INT_ENTER_CRITICAL_REGION;

	do {
		pServer->pCurrEntry = circptr(
							pServer->pCurrEntry,
							sizeof(ADI_DCB_ENTRY), 
							pServer->pQueue, pServer->NumEntries*sizeof(ADI_DCB_ENTRY)
							);
	} while (count++ < pServer->NumEntries+2 && pServer->pCurrEntry->priority!=ADI_DCB_AVAILABLE_ENTRY);

	// if we've completed a complete loop around the queue and no free entries found then abort
	if (count > pServer->NumEntries) {
		ADI_INT_EXIT_CRITICAL_REGION;
		return ADI_DCB_RESULT_NO_MEMORY;
	}

	// the following locks out all other post processes
	pServer->pCurrEntry->priority = Priority;
	pEntry = pServer->pCurrEntry;
	// so we can exit critical and be safe
	// The dispatch routine can not alter this entry 
	ADI_INT_EXIT_CRITICAL_REGION;

	// add new entry details
	pEntry->timestamp = ADI_DCB_TIMESTAMP;
	pEntry->pHandle = pHandle;
	pEntry->u32Arg = u32Arg;
	pEntry->pArg = pArg;

	ADI_INT_ENTER_CRITICAL_REGION;
	if (pServer->QueueSize==0) 
	{
		// if queue is empty, kick off with this one
		pServer->pStartNextPriority = pEntry;
	}
	else
	{
		// otherwise, update the next priority entry if higher 
		if (pServer->pStartNextPriority && pEntry->priority < pServer->pStartNextPriority->priority)
		{
			pServer->pStartNextPriority = pEntry;
		}
	}
	// increment Entry Count
	pServer->QueueSize++;

#ifdef ADI_SSL_DEBUG
	if (pServer->QueueSize > pServer->maxQueueSize)
		pServer->maxQueueSize = pServer->QueueSize;
#endif
	// enable callback so 
	pEntry->ClientCallback = Callback;

	// forward it to VDK or raise interrupt if VDK not present
	adi_dcb_Forward( (ADI_DCB_ENTRY_HDR*)pServer, pServer->IVG, Priority );

	ADI_INT_EXIT_CRITICAL_REGION;

	return ADI_DCB_RESULT_SUCCESS;
}

/*********************************************************************************
* Function:		adi_dcb_Remove
* Description:	Removes job(s) from queue server 
*********************************************************************************/

ADI_DCB_RESULT  
adi_dcb_Remove(
		ADI_DCB_HANDLE		hServer,
		ADI_DCB_CALLBACK_FN Callback
		)
{
	ADI_DCB_SERVER *pServer = (ADI_DCB_SERVER *)hServer;
	u32 i;
	void *tmpIMASK;

#ifdef ADI_SSL_DEBUG
	ADI_DCB_RESULT result; 

	result = adi_dcb_ValidateHandle(hServer);
	if (result != ADI_DCB_RESULT_SUCCESS)
		return result;
#endif

	ADI_INT_ENTER_CRITICAL_REGION;
	if (pServer->QueueSize==0)
	{
		ADI_INT_EXIT_CRITICAL_REGION;
#ifdef ADI_SSL_DEBUG
		return ADI_DCB_RESULT_NONE_FLUSHED;
#else
		return ADI_DCB_RESULT_FLUSHED_OK;
#endif
	}		
	ADI_INT_EXIT_CRITICAL_REGION;
		
	if (!Callback && pServer->QueueSize > 0)
	{
		// Flush all callbacks
		ADI_INT_ENTER_CRITICAL_REGION;
		for (i=0;i<pServer->NumEntries;i++) 
		{
			pServer->pQueue[i].ClientCallback = ADI_DCB_INACTIVE_ENTRY;
			pServer->pQueue[i].priority = ADI_DCB_AVAILABLE_ENTRY;
		}
		pServer->QueueSize = 0;
		pServer->pStartNextPriority = NULL;
		ADI_INT_EXIT_CRITICAL_REGION;
	}
	else
	{
		// Flush callbacks matching function pointer
#ifdef ADI_SSL_DEBUG
		u32 nfound=0;
#endif
		for (i=0;i<pServer->NumEntries;i++) 
		{
			if (pServer->pQueue[i].ClientCallback == Callback)
			{
				ADI_INT_ENTER_CRITICAL_REGION;
				pServer->pQueue[i].ClientCallback = ADI_DCB_INACTIVE_ENTRY;
				pServer->pQueue[i].priority = ADI_DCB_AVAILABLE_ENTRY;
				pServer->QueueSize--;
				ADI_INT_EXIT_CRITICAL_REGION;
#ifdef ADI_SSL_DEBUG
				nfound++;
#endif
				// it's safest to reset the next priority entry pointer even if some
				// are remaining (dispatcher will have to hunt for next priority - no big deal)
				if (pServer->pStartNextPriority && pServer->pStartNextPriority->priority==pServer->pQueue[i].priority)
				{
					ADI_INT_ENTER_CRITICAL_REGION;
					pServer->pStartNextPriority = NULL;
					ADI_INT_EXIT_CRITICAL_REGION;
				}
			}
		}
#ifdef ADI_SSL_DEBUG
		if (nfound==0)
			return ADI_DCB_RESULT_NONE_FLUSHED;
#endif
	}
	return ADI_DCB_RESULT_FLUSHED_OK;
}

/*********************************************************************************
* Function:		adi_dcb_Close
* Description:	Closes a queue server 
*********************************************************************************/

ADI_DCB_RESULT  
adi_dcb_Close(
		ADI_DCB_HANDLE hServer
		)
{
	ADI_DCB_SERVER *pServer = (ADI_DCB_SERVER *)hServer;
	void *tmpIMASK;

#ifdef ADI_SSL_DEBUG
	ADI_DCB_RESULT result = adi_dcb_ValidateHandle(hServer);
	if (result != ADI_DCB_RESULT_SUCCESS)
		return result;

	ADI_INT_ENTER_CRITICAL_REGION;
	if (pServer->QueueSize) {
		ADI_INT_EXIT_CRITICAL_REGION;
		return ADI_DCB_RESULT_QUEUE_IN_USE;
	}
	ADI_INT_EXIT_CRITICAL_REGION;
#endif

	// deregister handler 
	adi_dcb_RegisterISR(pServer->IVG,adi_dcb_DispatchCallbacks,NULL);

	// reset server data
	pServer->NumEntries = 0;
	pServer->pQueue = 0;
	pServer->IVG = ADI_DCB_INACTIVE_QUEUE;

	return ADI_DCB_RESULT_SUCCESS;

}

/*********************************************************************************
* Function:		adi_dcb_Control
* Description:	Performs various tasks 
*********************************************************************************/

static int ProcessingTable;

ADI_DCB_RESULT
adi_dcb_Control(
		ADI_DCB_HANDLE hServer,
		ADI_DCB_COMMAND command,
		void *value
)
{
#ifdef ADI_SSL_DEBUG
	ADI_DCB_RESULT result; 

	result = adi_dcb_ValidateHandle(hServer);
	if (result != ADI_DCB_RESULT_SUCCESS)
		return result;
#endif

	switch(command) 
	{
		default:
			return ADI_DCB_RESULT_BAD_COMMAND;

		case ADI_DCB_CMD_END:
			break;

		case ADI_DCB_CMD_PAIR:
			{
				ADI_DCB_COMMAND_PAIR *cmd = (ADI_DCB_COMMAND_PAIR *)value;
#ifndef ADI_SSL_DEBUG
				adi_dcb_Control(hServer, cmd->kind, &cmd->value);
#else
				if ( (result=adi_dcb_Control(hServer, cmd->kind, &cmd->value))!=ADI_DCB_RESULT_SUCCESS )
					return result;
#endif
			}
			break;

		case ADI_DCB_CMD_TABLE:
			{
				ADI_DCB_COMMAND_PAIR *cmd = (ADI_DCB_COMMAND_PAIR *)value;
				ProcessingTable++;
				while (cmd->kind != ADI_DCB_CMD_END)
				{
#ifndef ADI_SSL_DEBUG
					adi_dcb_Control(hServer, cmd->kind, &cmd->value);
#else
					if ( (result=adi_dcb_Control(hServer, cmd->kind, &cmd->value))!=ADI_DCB_RESULT_SUCCESS )
					{
						ProcessingTable--;
						return result;
					}
#endif
					cmd++;
				}
				ProcessingTable--;
			}
			break;

		case ADI_DCB_CMD_FLUSH_QUEUE:
#ifndef ADI_SSL_DEBUG
			adi_dcb_Remove((ADI_DCB_HANDLE)hServer,(ADI_DCB_CALLBACK_FN)value);
#else
			if ( (result=adi_dcb_Remove((ADI_DCB_HANDLE)hServer,(ADI_DCB_CALLBACK_FN)value))!=ADI_DCB_RESULT_FLUSHED_OK )
			{
				return result;
			}
#endif
	}
	return ADI_DCB_RESULT_SUCCESS;
}


/*********************************************************************************
*********************************************************************************

				END OF API FUNCTIONS

*********************************************************************************
*********************************************************************************/

/*********************************************************************************
* Function:		adi_dcb_DispatchCallbacks
* Description:	DCB Manager Interrupt Handler to handle queues.
				This ISR is associated with the hardware IVG level for each queue.
				(only in standalone mode). Only higher priority hardware interrupts 
				can pre-empt this ISR. Thus if a callback with a higher software 
				priority is posted it must wait until the current lower priority 
				callback completes its processing, whence it will be executed next.
*********************************************************************************/
ADI_INT_HANDLER (adi_dcb_DispatchCallbacks) 
{
	ADI_DCB_SERVER *pServer;// = (ADI_DCB_SERVER *)ClientArg;
	u16 priority,count;
	ADI_DCB_ENTRY *pEntry;
	ADI_DCB_ENTRY *pNextEntry;
	void *tmpIMASK;
	pServer = (ADI_DCB_SERVER *)ClientArg;

#ifdef ADI_SSL_DEBUG
	adi_dbc_DispatchCount++;
#endif
	if (pServer->QueueSize==0) {
#ifdef ADI_SSL_DEBUG
		adi_dbc_DispatchCount--;
#endif
		return ADI_INT_RESULT_NOT_PROCESSED;
	}
	else
	{
/*********************************************************************************************
 *      loop until all jobs consumed
 *
 *		The current callback is pEntry
 *		The current priority is pServer->currPriority
 *      The next one is pServer->pStartNextPriority, which could be a higher priority than the 
 *		priority of the current entry. In which case pServer->currPriority is updated.
 *		Thus, while higher priority entries cannot preempt a lower priority entry already 
 *		executing, they will be scheduled to run once its completed and before further entries 
 *		of the same priority as the current lower priority entry.
 ********************************************************************************************/
		
		// first entry - always pServer->pStartNextPriority
		ADI_INT_ENTER_CRITICAL_REGION;
		pEntry = pServer->pStartNextPriority;
		pServer->pStartNextPriority = NULL;
		ADI_INT_EXIT_CRITICAL_REGION;

		// if nothing is set to run, locate next highest priority 
		// from beginning of queue
		if (!pEntry) {
			pEntry = &pServer->pQueue[0];
			pNextEntry=NULL;
			ADI_INT_ENTER_CRITICAL_REGION;
			goto locate_next_callback;
		}
		
		// pServer->currPriority is only modified in this routine (could go on stack?)
		pServer->currPriority = pEntry->priority;

		do {
			// execute current callback entry
			(pEntry->ClientCallback)(pEntry->pHandle, pEntry->u32Arg, pEntry->pArg);
			// free the entry for subsequent use
			pNextEntry=NULL;
			ADI_INT_ENTER_CRITICAL_REGION;
			pEntry->ClientCallback = ADI_DCB_INACTIVE_ENTRY;
			pEntry->priority = ADI_DCB_AVAILABLE_ENTRY;
			// decrement queue size
			pServer->QueueSize--;

			// Now check whether another higher or same priority entry has been posted in the meantime
			// if all jobs at same priority this code will probably always execute
			if (pServer->pStartNextPriority)
			{
				pEntry = pServer->pStartNextPriority;
				pServer->pStartNextPriority = NULL;
				ADI_INT_EXIT_CRITICAL_REGION;
				pServer->currPriority = pEntry->priority;
				// execute new entry
				continue;
			}
locate_next_callback:
			// no new entries posted, so look for an entry of next highest priority already in the queue
			// 		loop around queue making a note of the highest priority entry

			// perform a complete circuit of the queue 
			for (count=0;count<pServer->NumEntries; count++)
			{
				pEntry = circptr(pEntry, sizeof(ADI_DCB_ENTRY), pServer->pQueue, pServer->NumEntries*sizeof(ADI_DCB_ENTRY));

				if ( (pEntry->ClientCallback!=ADI_DCB_INACTIVE_ENTRY) && (!pNextEntry || pEntry->priority < pNextEntry->priority) )
					pNextEntry = pEntry;
					
			} // end for loop

			ADI_INT_EXIT_CRITICAL_REGION;

			if (pNextEntry) 
			{
				pEntry = pNextEntry;
				pServer->currPriority = pEntry->priority;
				// execute new job
				continue;
			}

			// no more entries in queue so exit loop and hence ISR and await next interrupt
			break;
		
		} while(1); // end do ... while loop
	}

#ifdef ADI_SSL_DEBUG
	adi_dbc_DispatchCount--;
#endif
	return ADI_INT_RESULT_PROCESSED;
}


/*********************************************************************************
* Function:		adi_dcb_Terminate
* Description:	Dissociates the supplied memory with the DCB manager, etc.
*********************************************************************************/

ADI_DCB_RESULT 

adi_dcb_Terminate(void)

{
	// Close all open servers
	ADI_DCB_SERVER *pServer;
	u32 i;
	for ADI_DCB_SERVER_ITERATOR(pServer,i)
	{
		if (pServer->IVG != ADI_DCB_INACTIVE_QUEUE)
			adi_dcb_Close((ADI_DCB_HANDLE)pServer);
	}

	// Deassign memory from server array
	adi_dcb_Manager.Servers = (ADI_DCB_SERVER*)NULL;
	adi_dcb_Manager.NumServers = 0;

	// Deassign Critical region data pointer
	_hCriticalRegionData = NULL;

	// Job done
	return ADI_DCB_RESULT_SUCCESS;
}

#ifdef ADI_SSL_DEBUG

/*********************************************************************************
* Function:		adi_dcb_ValidateHandle
* Description:	Validates a queue server handle, by ensuring its in the registered
*				set of queue servers.
*********************************************************************************/
  
static ADI_DCB_RESULT
adi_dcb_ValidateHandle(
		ADI_DCB_HANDLE hServer
		)
{
	ADI_DCB_SERVER *pServer;
	u32 i;

	for ADI_DCB_SERVER_ITERATOR(pServer,i)
	{
		if ((ADI_DCB_HANDLE)pServer == hServer)
			return ADI_DCB_RESULT_SUCCESS;
	}

	return ADI_DCB_RESULT_NO_SUCH_QUEUE;
}

#endif //ADI_SSL_DEBUG

