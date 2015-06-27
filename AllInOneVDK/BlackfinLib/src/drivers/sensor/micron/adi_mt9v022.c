/******************************************************************************

Copyright (c) 2005 Analog Devices.  All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  
*******************************************************************************

$File: adi_MT9V022.c $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Project:    MT9V022 Micron Imaging sensor device driver
Title:      MT9V022 source
Author(s):  BJ

Description:
   This is the primary source file for the MT9V022 Micron Imaging sensor driver..

Note:
	MT9V022I77ATC ES - Color CMOS VGA 640x480 sensor.
	MT9V022I77ATM ES - Black and white CMOS VGA 640x480 sensor.

*******************************************************************************

Modification History:
====================
Revision 1.00
	Created driver
Revision 1.01 - BJ - 22/1/2006
	Fixed PPI auto configuration
Revision 1.03 - BJ - 22/1/2006
	Fixed height and width command when binning is turned on
Revision 1.04 - BJ - 23/4/2006
	Added 32bit DMA support, this is automatically used when a buffer with
	an elementwidth of 4 is passed to this driver.
Revision 1.06 - BJ - 8/5/2006
	Updated driver to support revised device access funcitons
	
******************************************************************************/



/******************************************************************************

Include files

******************************************************************************/

#include <services/services.h>							// system services
#include <drivers/adi_dev.h>							// device manager includes
#include <drivers/ppi/adi_ppi.h>
#include <drivers/twi/adi_twi.h>
#include <drivers/deviceaccess/adi_device_access.h>
#include <drivers/sensor/micron/adi_mt9v022.h>			// sensor includes

#include <string.h>

/*****************************************************************************/



/******************************************************************************

Internal data structures:

Note: 

******************************************************************************/

typedef struct
{
	// Configuration data, can be setup in Device[] structures
	// TWI driver configuration data
	u32 TWINumber,TWIAddress;
	ADI_DEV_CMD_VALUE_PAIR *TWIConfig;
	
	// PPI driver configuration data
	u32 PPINumber,PPIDataflow;
	ADI_DEV_CMD_VALUE_PAIR *PPIConfig;
	
	
	// Storage used internally by driver, note: this data is re-initialised
	// everytime the driver is opened
	ADI_DEV_MANAGER_HANDLE		MgrHandle;			// Device manager
	ADI_DEV_DEVICE_HANDLE		DevHandle;			// Device handle
	ADI_DMA_MANAGER_HANDLE		DMAHandle;			// handle to the DMA manager
	ADI_DCB_HANDLE				DCBHandle;			// callback handle
	ADI_DCB_CALLBACK_FN         DMCallback;			// the callback function supplied by the Device Manager
	
	// used to store volatile PPI data
	ADI_DEV_DEVICE_HANDLE		PPIHandle;			// Handle to the underlying PPI device driver
	
	// Some status registers for the driver to keep its opened status
	u32							Dataflow;
	u32							PPIDma32;
	u32							PPIRead;
} ADI_MT9V022;										// physical device driver data


/*****************************************************************************/



/******************************************************************************

Macros

******************************************************************************/

// alias for the actual device structure
#define ADI_PDD_DRIVER ADI_MT9V022


// number of drivers that have been defined
#define ADI_PDD_DRIVERS (sizeof(Device)/sizeof(Device[0]))


// ADI_PDD_CHECK verifies a pointer to a driver (as received from the Device 
// Manager)
#ifdef ADI_DEV_DEBUG
#define ADI_PDD_CHECK(pdd,rv)  {int i=0; while (pdd!=&Device[i++]) if (i>=ADI_PDD_DRIVERS) return rv;}
#else
#define ADI_PDD_CHECK(pdd,rv)
#endif


// total number of registers needed by device, ignore the last 2 regs, they're not used
#define ADI_MT9V022_REGISTER_TOTAL	0xFD

/*****************************************************************************/



/******************************************************************************

Device specific data

******************************************************************************/


/* Edinburgh *******************************************************/

#if defined(__ADSP_EDINBURGH__)

static ADI_PDD_DRIVER Device[]={
	{0,ADI_MT9V022_TWIADDR,NULL,0,ADI_DEV_MODE_UNDEFINED,0,0,NULL},
};

#endif
	

/* Braemar *********************************************************/

#if defined(__ADSP_BRAEMAR__)

static ADI_PDD_DRIVER Device[]={
	{0,ADI_MT9V022_TWIADDR,NULL,0,ADI_DEV_MODE_UNDEFINED,0,0,NULL},
};

#endif
	

/* Teton-Lite ******************************************************/

#if defined(__ADSP_TETON__)

static ADI_PDD_DRIVER Device[]={
	{0,ADI_MT9V022_TWIADDR,NULL,0,ADI_DEV_MODE_UNDEFINED,0,0,NULL},
	{1,ADI_MT9V022_TWIADDR,NULL,1,ADI_DEV_MODE_UNDEFINED,0,0,NULL},
};

#endif


/*****************************************************************************/



/******************************************************************************

MT9V022 Register Tables

******************************************************************************/


/********************************************************************

Table for Register Field Error check and Register field access
Table structure - 
	'Count' of Register addresses containing individual fields
	Register address containing individual fields, 
	Register field locations in the corresponding register, 
	Reserved bit locations in the corresponding register
	
Register field location - Bit indicating start of new field in a 
register will be 1. Reserved bit locations will be 1

********************************************************************/

// Register address to perform Register Field Error check and 
// Register field access
static u16 RegFieldAddr[]={
	ADI_MT9V022_CHIPCONTROL,
	ADI_MT9V022_SHUTTERCTRL,
	ADI_MT9V022_RESET,
	ADI_MT9V022_READMODE,
	ADI_MT9V022_PIXELMODE,
	ADI_MT9V022_LEDOUTCTRL,
	ADI_MT9V022_BLCALIBCTRL,
	ADI_MT9V022_ROWNOISECORRCTRL1,
	
	ADI_MT9V022_PIXELCLK,
	ADI_MT9V022_TESTPATTERN,
	ADI_MT9V022_TILEX0Y0,
	ADI_MT9V022_TILEX1Y0,
	ADI_MT9V022_TILEX2Y0,
	ADI_MT9V022_TILEX3Y0,
	ADI_MT9V022_TILEX4Y0,
	ADI_MT9V022_TILEX0Y1,
	
	ADI_MT9V022_TILEX1Y1,
	ADI_MT9V022_TILEX2Y1,
	ADI_MT9V022_TILEX3Y1,
	ADI_MT9V022_TILEX4Y1,
	ADI_MT9V022_TILEX0Y2,
	ADI_MT9V022_TILEX1Y2,
	ADI_MT9V022_TILEX2Y2,
	ADI_MT9V022_TILEX3Y2,
	
	ADI_MT9V022_TILEX4Y2,
	ADI_MT9V022_TILEX0Y3,
	ADI_MT9V022_TILEX1Y3,
	ADI_MT9V022_TILEX2Y3,
	ADI_MT9V022_TILEX3Y3,
	ADI_MT9V022_TILEX4Y3,
	ADI_MT9V022_TILEX0Y4,
	ADI_MT9V022_TILEX1Y4,
	
	ADI_MT9V022_TILEX2Y4,
	ADI_MT9V022_TILEX3Y4,
	ADI_MT9V022_TILEX4Y4,
	ADI_MT9V022_LVDSMASTERCTRL,
	ADI_MT9V022_LVDSSHIFTCLKCTRL,
	ADI_MT9V022_LVDSDATACTRL,
	ADI_MT9V022_STEREOERRORCTRL,
	ADI_MT9V022_ANALOGCTRL,
	
	ADI_MT9V022_NTSCCTRL,
	ADI_MT9V022_NTSCHBLANKCTRL,
	ADI_MT9V022_NTSCVBLANKCTRL,
};

// Register Field start locations corresponding to the entries in 
// MT9V022RegAddr (Reserved Bit locations marked as 1)
static u16 RegFieldValue[]={
	0x03F9,0x0311,0x0003,0x00F5,0x00C4,0x0003,0x0021,0x0821,
	0x001F,0x6C01,0x0011,0x0011,0x0011,0x0011,0x0011,0x0011,
	0x0011,0x0011,0x0011,0x0011,0x0011,0x0011,0x0011,0x0011,
	0x0011,0x0011,0x0011,0x0011,0x0011,0x0011,0x0011,0x0011,
	0x0011,0x0011,0x0011,0x000F,0x0011,0x0011,0x0007,0x0880,
	0x0003,0x0101,0x0101,
};

static ADI_DEVICE_ACCESS_REGISTER_FIELD RegisterFieldTable[]=
	{sizeof(RegFieldAddr)/sizeof(RegFieldAddr[0]),RegFieldAddr,RegFieldValue};


/********************************************************************

Table to configure the reserved bits in the device to its recommended 
	values
Table structure -
	'Count' of Register addresses containing Reserved Locations
	Register address containing Reserved Locations
	Recommended value for the Reserved Locations
					
********************************************************************/

// No specific values are recommended for reserved bits.

static u16 RegReservedAddr[]={
	ADI_MT9V022_BLCALIBCTRL,
	ADI_MT9V022_ROWNOISECORRCTRL1,
};

// Reserved Bit locations corresponding to the entries in 
// MT9V022RegAddr
static u16 RegReservedBits[]={
	0xFF00,0x0010,
};

static u16 RegReservedDefaults[]={
	0x8000,0x0010
};


static ADI_DEVICE_ACCESS_RESERVED_VALUES RegisterReservedTable[]=
	{sizeof(RegReservedAddr)/sizeof(RegReservedAddr[0]),RegReservedBits,RegReservedDefaults};


/********************************************************************

Table for Register Error check for invalid & read-only register(s) access
Structure -
	'Count' of Invalid Register addresses.
	Invalid Register addresses.
	'Count' of Read-only Register addresses.
	Read-only Register addresses.

********************************************************************/

// array of invalid registers addresses.
static u16 RegInvalidAddr[]={
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
	0x18,0x19,0x1A,0x1D,0x1E,0x1F,0x20,0x21,
	0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,
	0x2A,0x2B,0x2D,0x2E,0x2F,0x30,0x37,0x38,
	0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x40,
	0x41,0x43,0x44,0x45,0x49,0x4A,0x4B,0x4D,
	0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,0x55,
	0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,
	0x5E,0x5F,0x60,0x61,0x62,0x63,0x64,0x65,
	0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,
	0x6E,0x6F,0x71,0x75,0x76,0x77,0x78,0x79,
	0x7A,0x7B,0x7C,0x7D,0x7E,0xA7,0xAA,0xAC,
	0xAD,0xAE,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,
	0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,
	0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,
	0xDC,0xDD,0xDE,0xDF,0xE0,0xE1,0xE2,0xE3,
	0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,
	0xEC,0xED,0xEE,0xEF,0xF2,0xF3,0xF4,0xF5,
	0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,
};

// array of read-only registers addresses.
static u16 RegReadOnlyAddr[]={
	ADI_MT9V022_CHIPVERSION,
	ADI_MT9V022_DARKAVERAGE,
	ADI_MT9V022_STEREOERRORFLAG,
	ADI_MT9V022_LVDSDATAOUTPUT,
	ADI_MT9V022_AGCGAINOUTPUT,
	ADI_MT9V022_AECGAINOUTPUT,
	ADI_MT9V022_AECAGCCURRENTBIN,
	ADI_MT9V022_TEMPERATURE,
};

static ADI_DEVICE_ACCESS_VALIDATE_REGISTER RegisterValidateTable[]={
	sizeof(RegInvalidAddr)/sizeof(RegInvalidAddr[0]),RegInvalidAddr,
	sizeof(RegReadOnlyAddr)/sizeof(RegReadOnlyAddr[0]),RegReadOnlyAddr
};

/*****************************************************************************/



/******************************************************************************

Device driver function declerations

******************************************************************************/

// phyisical device driver hooks
static u32 adi_pdd_open(ADI_DEV_MANAGER_HANDLE,u32,ADI_DEV_DEVICE_HANDLE,ADI_DEV_PDD_HANDLE *,ADI_DEV_DIRECTION,void *,ADI_DMA_MANAGER_HANDLE,ADI_DCB_HANDLE,ADI_DCB_CALLBACK_FN);
static u32 adi_pdd_close(ADI_DEV_PDD_HANDLE);
static u32 adi_pdd_control(ADI_DEV_PDD_HANDLE,u32,void *);
static u32 adi_pdd_read(ADI_DEV_PDD_HANDLE,ADI_DEV_BUFFER_TYPE,ADI_DEV_BUFFER *);
static u32 adi_pdd_not_supported(ADI_DEV_PDD_HANDLE,ADI_DEV_BUFFER_TYPE,ADI_DEV_BUFFER *);


// general driver functions
static u32 driver_start(ADI_PDD_DRIVER *);
static u32 driver_stop(ADI_PDD_DRIVER *);
static u32 driver_cmdcopy(ADI_DEV_CMD_VALUE_PAIR *,ADI_DEV_CMD_VALUE_PAIR **);
static u32 driver_ppiconfig(ADI_PDD_DRIVER *,u32 *,u32 *,u32 *);


// register access functions
static u32 driver_register(ADI_PDD_DRIVER *,u32,void *);
static u32 driver_register_callback(void *,u16 *,u16,ADI_DEVICE_ACCESS_MODE);

/*****************************************************************************/



/*****************************************************************************

Device driver interface functions:
	Standard device driver driver interface functions supported by the Sensor 
	device driver

	adi_pdd_open()				--	opens the device
	adi_pdd_close()				--	closes the device
	adi_pdd_control()			--	setup the device
	adi_pdd_read()				--	supply a buffer to the device
	adi_pdd_write()				--	not supported
	adi_pdd_sequential()		--	not supported

*****************************************************************************/


/********************************************************************

Device driver entry points:

********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADIMT9V022EntryPoint={
	adi_pdd_open,
	adi_pdd_close,
	adi_pdd_read,
	adi_pdd_not_supported,
	adi_pdd_control,
	adi_pdd_not_supported
};


/********************************************************************

	Function: adi_pdd_open
	
	Description: entry point to prepare both driver and the Sensor for
		use.

********************************************************************/

u32 adi_pdd_open(
	ADI_DEV_MANAGER_HANDLE	MgrHandle,				// device manager handle
	u32 					DevNumber,				// device number
	ADI_DEV_DEVICE_HANDLE 	DevHandle,				// device handle
	ADI_DEV_PDD_HANDLE 		*pPDDHandle,			// pointer to PDD handle location 
	ADI_DEV_DIRECTION 		DevDirection,			// data direction
	void					*pCriticalRegionArg,	// critical region imask storage location
	ADI_DMA_MANAGER_HANDLE	DMAHandle,				// handle to the DMA manager
	ADI_DCB_HANDLE			DCBHandle,				// callback handle
	ADI_DCB_CALLBACK_FN		DMCallback)				// device manager callback function
{
	ADI_PDD_DRIVER *drv;
	u32 Code=ADI_DEV_RESULT_SUCCESS;

#ifdef ADI_DEV_DEBUG
	// check that the device number is vaild
	if (DevNumber>=ADI_PDD_DRIVERS) return ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
	
	// check that data direction is valid
	switch (DevDirection)
	{
		case ADI_DEV_DIRECTION_INBOUND: break;
		default: return ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED;
	}
#endif

	// check if driver alread exists
	drv=&Device[DevNumber];
	
	// return handle to physical device driver
	*pPDDHandle=(ADI_DEV_PDD_HANDLE *)drv;
	{
		void *exit_critical=adi_int_EnterCriticalRegion(pCriticalRegionArg);
		u8 in_use=drv->DevHandle?TRUE:FALSE;

		// setup the device handle
		if (!in_use) drv->DevHandle=DevHandle;
		
		adi_int_ExitCriticalRegion(exit_critical);
		if (in_use) return ADI_DEV_RESULT_DEVICE_IN_USE;
	}
	
	// setup the driver variables
	drv->MgrHandle=MgrHandle;		// Device manager Handle
	drv->DMAHandle=DMAHandle;		// handle to the DMA manager
	drv->DCBHandle=DCBHandle;		// callback handle
	drv->DMCallback=DMCallback;		// the callback function supplied by the Device Manager
	
	drv->TWIConfig=NULL;
	drv->Dataflow=0;
	drv->PPIHandle=NULL;			// Clear the PPI handle
	drv->PPIConfig=NULL;
	drv->PPIDma32=0;
	drv->PPIRead=FALSE;
	
	return Code;
}


/********************************************************************

	Function: adi_pdd_close
	
	Description: entry point to terminate a driver's services
		-	set Sensor hardware to a neutral state
		-	remove interrupt service
		-	mark driver as ready to open again

********************************************************************/

u32 adi_pdd_close(ADI_DEV_PDD_HANDLE pdd)
{
	ADI_PDD_DRIVER *drv=(ADI_PDD_DRIVER *)pdd;
	
	ADI_PDD_CHECK(drv,ADI_DEV_RESULT_BAD_PDD_HANDLE);
	
	if (!drv->DevHandle) ADI_DEV_RESULT_SUCCESS;

	// stop the physical device driver
	driver_stop(drv);
		
	// clean up driver, free any memory allocated
	if (drv->TWIConfig) {free(drv->TWIConfig); drv->TWIConfig=NULL;}
	if (drv->PPIConfig) {free(drv->PPIConfig); drv->PPIConfig=NULL;}
	
	drv->DevHandle=NULL;
	drv->MgrHandle=NULL;
	drv->DMAHandle=NULL;
	drv->DCBHandle=NULL;
	drv->DMCallback=NULL;
	drv->PPIHandle=NULL;
		
	return ADI_DEV_RESULT_SUCCESS;
}


/********************************************************************

	Function: adi_pdd_control
	
	Description: entry point allowing configuration of the driver and/or
		sensor hardware.

********************************************************************/

u32 adi_pdd_control(ADI_DEV_PDD_HANDLE pdd,u32 Cmd,void *pArg)
{
	ADI_PDD_DRIVER *drv=(ADI_PDD_DRIVER *)pdd;
	u32 stopped=ADI_DEV_RESULT_FAILED,Code=ADI_DEV_RESULT_SUCCESS;

	ADI_PDD_CHECK(drv,ADI_DEV_RESULT_BAD_PDD_HANDLE);

	// stop the driver for the following commands
	switch (Cmd)
	{
		case ADI_DEV_CMD_SET_DATAFLOW_METHOD:
		case ADI_MT9V022_CMD_SET_TWI:			// Set the TWI device number
		case ADI_MT9V022_CMD_SET_TWIADDR:		// Set the sensors TWI address
		case ADI_MT9V022_CMD_SET_TWICONFIG:		// Set the TWI config list
		case ADI_MT9V022_CMD_SET_PPI:			// Set the PPI hardware number
		case ADI_MT9V022_CMD_SET_PPICONFIG:		// Set the PPI config list
			stopped=driver_stop(drv);
	};
	
	switch (Cmd)
	{
		case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:
			*((u32 *)pArg)=FALSE;
			return ADI_DEV_RESULT_SUCCESS;
			
		case ADI_DEV_CMD_SET_DATAFLOW_METHOD:
			switch ((ADI_DEV_MODE)pArg)
			{
				case ADI_DEV_MODE_CIRCULAR:				// circular buffer
				case ADI_DEV_MODE_CHAINED:				// chained
				case ADI_DEV_MODE_CHAINED_LOOPBACK:		// chained with loopback
					stopped=driver_stop(drv);
					drv->PPIDataflow=(ADI_DEV_MODE)pArg;
					break;
				default:
					return ADI_DEV_RESULT_NOT_SUPPORTED;
			}
			break;
			
		case ADI_DEV_CMD_SET_DATAFLOW:
			// if opt is set then start the device, otherwise stop it
			drv->Dataflow=(u32)pArg;
			Code=drv->Dataflow?driver_start(drv):driver_stop(drv);
			return Code==ADI_MT9V022_RESULT_ALREADYSTOPPED?ADI_DEV_RESULT_SUCCESS:Code;
			
			
		// interface to TWI device configuration
		case ADI_MT9V022_CMD_SET_TWI:			// Set the TWI device number
			drv->TWINumber=(u32)pArg;
			break;
			
		case ADI_MT9V022_CMD_SET_TWIADDR:		// Set the sensors TWI address
			drv->TWIAddress=(u32)pArg;
			break;
			
		case ADI_MT9V022_CMD_SET_TWICONFIG:		// Set the TWI config list
			Code=driver_cmdcopy((ADI_DEV_CMD_VALUE_PAIR *)pArg,&drv->TWIConfig);
			break;
			
			
		// interface to PPI device configuration
		case ADI_MT9V022_CMD_SET_PPI:			// Set the PPI hardware number
			drv->PPINumber=(u32)pArg;
			break;
			
		case ADI_MT9V022_CMD_SET_PPICONFIG:		// Set the PPI config list
			// if pArg is not null then copy the PPI config table
			if (pArg) Code=driver_cmdcopy((ADI_DEV_CMD_VALUE_PAIR *)pArg,&drv->PPIConfig);
			break;
			
		case ADI_MT9V022_CMD_SET_PPICMD:			// Send commands directly to PPI control
			if (!drv->PPIHandle) return ADI_MT9V022_RESULT_PPI_CLOSED;
			Code=adi_dev_Control(drv->PPIHandle,ADI_DEV_CMD_TABLE,pArg);
			break;			
			
			
		case ADI_MT9V022_CMD_GET_HWID:
		{
			ADI_DEV_ACCESS_REGISTER Regs[]={
				{ADI_MT9V022_CHIPVERSION,0x0000},
				{ADI_DEV_REGEND,0}
			};
			
			Code=driver_register(drv,ADI_DEV_CMD_REGISTER_TABLE_READ,(void *)&Regs);
			*((u32 *)pArg)=(u32)(Regs[0].Data);
			return Code;
		}
		
		case ADI_MT9V022_CMD_GET_HEIGHT:
		{
			ADI_DEV_ACCESS_REGISTER Regs[]={
				{ADI_MT9V022_WINDOWHEIGHT,0x0000},
				{ADI_MT9V022_READMODE,0x0000},
				{ADI_DEV_REGEND,0}
			};
			
			Code=driver_register(drv,ADI_DEV_CMD_REGISTER_TABLE_READ,(void *)&Regs);
			*((u32 *)pArg)=(u32)(Regs[0].Data>>(Regs[1].Data&0x03));
			return Code;
		}
		
		case ADI_MT9V022_CMD_GET_WIDTH:
		{
			ADI_DEV_ACCESS_REGISTER Regs[]={
				{ADI_MT9V022_WINDOWWIDTH,0x0000},
				{ADI_MT9V022_READMODE,0x0000},
				{ADI_DEV_REGEND,0}
			};
			
			Code=driver_register(drv,ADI_DEV_CMD_REGISTER_TABLE_READ,(void *)&Regs);
			*((u32 *)pArg)=(u32)(Regs[0].Data>>((Regs[1].Data>>2)&0x03));
			return Code;
		}
				
		default: Code=driver_register(drv,Cmd,pArg);
	}
	if (Code!=ADI_DEV_RESULT_SUCCESS) return Code;
	
	// restart the driver if it was stopped, otherwise success
	return stopped==ADI_DEV_RESULT_SUCCESS?driver_start(drv):ADI_DEV_RESULT_SUCCESS;
}


/********************************************************************

	Function: adi_pdd_read
	
	Description: Give driver a buffer to read data into from the 
		physical device, this function just passed the buffer down to
		the PPI driver. If dataflow is false then the PPIHandle will
		be null, so the PPI driver function should throw a bad device
		error.

********************************************************************/

u32 adi_pdd_read(ADI_DEV_PDD_HANDLE pdd,ADI_DEV_BUFFER_TYPE Type,ADI_DEV_BUFFER *Buffer)
{
	ADI_PDD_DRIVER *drv=(ADI_PDD_DRIVER *)pdd;
	u32 Code=ADI_DEV_RESULT_SUCCESS;
	
	ADI_PDD_CHECK(drv,ADI_DEV_RESULT_BAD_PDD_HANDLE);
	
	
	switch (Type)
	{
		case ADI_DEV_1D: drv->PPIDma32=Buffer->OneD.ElementWidth==4?1:0; break;
		case ADI_DEV_2D: drv->PPIDma32=Buffer->TwoD.ElementWidth==4?1:0; break;
		case ADI_DEV_CIRC: drv->PPIDma32=Buffer->Circular.ElementWidth==4?1:0; break;
	}
	
	switch (Type)
	{
		case ADI_DEV_1D:
		case ADI_DEV_2D:
		case ADI_DEV_CIRC:
			if (!drv->PPIHandle||!drv->PPIRead) Code=driver_start(drv);
			
			if (Code==ADI_DEV_RESULT_SUCCESS)
				Code=adi_dev_Read(drv->PPIHandle,Type,Buffer);
			if (Code==ADI_DEV_RESULT_SUCCESS&&drv->Dataflow&&!drv->PPIRead)
				Code=adi_dev_Control(drv->PPIHandle,ADI_DEV_CMD_SET_DATAFLOW,(void*)TRUE);
			if (Code==ADI_DEV_RESULT_SUCCESS) drv->PPIRead=TRUE;
	}
	return Code;
}


/********************************************************************

	Function: adi_pdd_not_supported
	
	Description: Not supported by this driver

********************************************************************/

u32 adi_pdd_not_supported(ADI_DEV_PDD_HANDLE pdd,ADI_DEV_BUFFER_TYPE Type,ADI_DEV_BUFFER *Buffer)
{
	return ADI_DEV_RESULT_NOT_SUPPORTED;
}


/*****************************************************************************/



/******************************************************************************

General driver functions

	driver_start()
	driver_stop()
	driver_cmdcopy()
	driver_ppiconfig()
	driver_register()
	driver_regcallback()

- Note. 

******************************************************************************/


/********************************************************************

	Function: driver_start
	
	Description: Start the driver, if there is no read buffer then
		one will be requested by a callback
		- if PPI control is auto (0xFFFF) use the TWI to get sensor
			registor data to create the PPI control word.
		- open and configure the PPI driver
		- if a buffer was sent to driver before PPI was started then
			pass it to the PPI now, and start the ppi
		
	Return value:
		ADI_DEV_RESULT_SUCCESS - the device was successfully started.
		ADI_DEV_RESULT_FAILED - there was a problem trying to start
			the Sensor driver.
		ADI_DEV_RESULT_BAD_PDD_HANDLE - the device handle is not a
			Sensor device.
		* see return value for 'driver_ppiautoconfig'

********************************************************************/

u32 driver_start(ADI_PDD_DRIVER *drv)
{
	u32 Code=ADI_DEV_RESULT_SUCCESS,Control,Cols,Rows;
	
	ADI_PDD_CHECK(drv,ADI_DEV_RESULT_BAD_PDD_HANDLE);
	
	// if the PPI is closed then open it
	if (!drv->PPIHandle)
	{
		if ((Code=adi_dev_Open(
			drv->MgrHandle,				// device manager handle
			&ADIPPIEntryPoint,			// PPI Entry point
			drv->PPINumber,				// PPI device number
			drv->DevHandle,				// client handle - passed to internal callback function
			&drv->PPIHandle,			// pointer to DM handle (for PPI driver) location
			ADI_DEV_DIRECTION_INBOUND,	// PPI used only to output video data
			drv->DMAHandle,				// handle to the DMA manager
			drv->DCBHandle,				// handle to the callback manager
			drv->DMCallback))!=ADI_DEV_RESULT_SUCCESS) return Code;
		
		
		// set the dataflow method
		Code=adi_dev_Control(drv->PPIHandle,ADI_DEV_CMD_SET_DATAFLOW_METHOD,(void *)(drv->PPIDataflow));
	}
	
	
	// If needed get the Sensor configuration to create the PPI config table
	if (Code==ADI_DEV_RESULT_SUCCESS&&
		(drv->PPIConfig||(Code=driver_ppiconfig(drv,&Control,&Cols,&Rows))==ADI_DEV_RESULT_SUCCESS))
	{
		// Open the PPI driver and configure, if there is an error return error Code
		// standard PPI device configuration options
		ADI_DEV_CMD_VALUE_PAIR PPIConfig[]={
			{ADI_PPI_CMD_SET_CONTROL_REG,(void *)(Control|(drv->PPIDma32?0x0100:0))},
			{ADI_PPI_CMD_SET_LINES_PER_FRAME_REG,(void *)(Rows)},
			{ADI_PPI_CMD_SET_TRANSFER_COUNT_REG,(void *)(Cols-1)},
			{ADI_DEV_CMD_END,NULL}
		};
		
		
		// configure the PPI with the auto generated PPI config table of with the user supplied one
		if (Code==ADI_DEV_RESULT_SUCCESS) Code=adi_dev_Control(drv->PPIHandle,ADI_DEV_CMD_TABLE,drv->PPIConfig?drv->PPIConfig:(void*)PPIConfig);
	}
	
	if (Code!=ADI_DEV_RESULT_SUCCESS) driver_stop(drv);
	return Code;
}


/********************************************************************

	Function: driver_stop
	
	Description: Stop the driver.
		- if PPI is started then stop it.
		
	Return value:
		ADI_DEV_RESULT_SUCCESS - the device was successfully stopped.
		ADI_DEV_RESULT_ALREADYSTOPPED - the device is already stopped.
		ADI_DEV_RESULT_BAD_PDD_HANDLE - the device handle is not a
			Sensor device.
		* check 'adi_dev_Close' for PPI driver
			
********************************************************************/

u32 driver_stop(ADI_PDD_DRIVER *drv)
{
	u32 Code;
	
	ADI_PDD_CHECK(drv,ADI_DEV_RESULT_BAD_PDD_HANDLE);
	
	// if PPIHandle is NULL we're already stopped
	if (drv->PPIHandle==NULL) return ADI_MT9V022_RESULT_ALREADYSTOPPED;
	
	// otherwise close the driver and clear the handle
	Code=adi_dev_Close(drv->PPIHandle);
	drv->PPIHandle=NULL;
	
	return Code;
}


/********************************************************************

	Function: driver_cmdcopy
	
	Description: Allocate memory and copy a driver command list

	Return value:
		ADI_DEV_RESULT_SUCCESS - Config list copied successfully
		ADI_DEV_RESULT_NO_MEMORY - There was a problem allocating
			memory for config list.
	
********************************************************************/

u32 driver_cmdcopy(ADI_DEV_CMD_VALUE_PAIR *src,ADI_DEV_CMD_VALUE_PAIR **dst)
{
	u32 i=0;
	
	// if the dst address is not null then free the memory
	if (*dst) {free(*dst); *dst=NULL;}
	
	// count the number of entries in the list
	if (src) while (src[i++].CommandID!=ADI_DEV_CMD_END);
	
	// now copy them
	if ((*dst=malloc(i*sizeof(ADI_DEV_CMD_VALUE_PAIR)))!=NULL)
	{
		memcpy(*dst,src,i*sizeof(ADI_DEV_CMD_VALUE_PAIR));
		i=0;
	}
	return i==0?ADI_DEV_RESULT_SUCCESS:ADI_DEV_RESULT_NO_MEMORY;
}


/********************************************************************

	Function: driver_ppiconfig
	
	Description: This is automatically called before the PPI opened 
		in driver_start. Sensor should only be configured once each 
		time the driver is opened, and is only called when dataflow 
		is set to TRUE

	Return value:
		ADI_DEV_RESULT_SUCCESS - Config info is valid, either user
			defined or setup according to registers read back from
			sensor
		* check 'driver_control'
	
********************************************************************/

u32 driver_ppiconfig(ADI_PDD_DRIVER *drv,u32 *Control,u32 *Cols,u32 *Rows)
{
	ADI_DEV_ACCESS_REGISTER Regs[]={
		{ADI_MT9V022_PIXELCLK,0x0000},
		{ADI_MT9V022_WINDOWWIDTH,0x0000},
		{ADI_MT9V022_WINDOWHEIGHT,0x0000},
		{ADI_MT9V022_READMODE,0x0000},
		{ADI_DEV_REGEND,0}
	};
	u32 Code;
	
	*Control=0;
	*Cols=0;
	*Rows=0;
	
	ADI_PDD_CHECK(drv,ADI_DEV_RESULT_BAD_PDD_HANDLE);
	
	// read registers from sensor
	Code=driver_register(drv,ADI_DEV_CMD_REGISTER_TABLE_READ,(void *)&Regs);
	if (Code!=ADI_DEV_RESULT_SUCCESS) return Code;

	// setup the PPI Control with the correct settings
	*Control=0x82C|
		(Regs[0].Data&0x0010?0x8000:0x0000)|
		(Regs[0].Data&0x0003?0x4000:0x0000);
	*Cols=Regs[1].Data>>((Regs[3].Data>>2)&0x03);
	*Rows=Regs[2].Data>>(Regs[3].Data&0x03);
	return ADI_DEV_RESULT_SUCCESS;
}


/********************************************************************

	Function:  driver_register

	Description: Pass register table onto register access function

********************************************************************/

u32 driver_register(ADI_PDD_DRIVER *DevHandle,u32 Cmd,void *pArg)
{
	ADI_DEV_CMD_VALUE_PAIR TWIConfig[]={
		{ADI_DEV_CMD_SET_DATAFLOW_METHOD,(void *)ADI_DEV_MODE_SEQ_CHAINED},
		{ADI_DEV_CMD_SET_DATAFLOW,(void *)TRUE},
		{ADI_DEV_CMD_END,(void *)NULL},
	};
	ADI_DEV_CMD_VALUE_PAIR *Cfg;
	ADI_DEVICE_ACCESS_SELECT AccessMode={
		0,
		ADI_DEVICE_ACCESS_LENGTH0,
		ADI_DEVICE_ACCESS_LENGTH1,
		ADI_DEVICE_ACCESS_LENGTH2,
		ADI_DEVICE_ACCESS_TYPE_TWI
	};
	u32 Code=ADI_DEV_RESULT_SUCCESS,i=0;
	
	// count the number of entries in the list
	if (DevHandle->TWIConfig) while (DevHandle->TWIConfig[i++].CommandID!=ADI_DEV_CMD_END);
	if (i>1) i--;
	
	// now copy them
	if ((Cfg=malloc((i*sizeof(ADI_DEV_CMD_VALUE_PAIR))+sizeof(TWIConfig)))!=NULL)
	{
		memcpy(Cfg,DevHandle->TWIConfig,i*sizeof(ADI_DEV_CMD_VALUE_PAIR));
		memcpy(Cfg+i,TWIConfig,sizeof(TWIConfig));
		i=0;
	} else Code=ADI_DEV_RESULT_NO_MEMORY;

	if (Code==ADI_DEV_RESULT_SUCCESS)
	{
		ADI_DEVICE_ACCESS_REGISTERS Device={
			DevHandle->MgrHandle,
			DevHandle,
			DevHandle->TWINumber,
			DevHandle->TWIAddress,
			DevHandle->DCBHandle,
			driver_register_callback,
			Cmd,
			pArg,
			ADI_MT9V022_REGISTER_TOTAL,
			RegisterFieldTable,
			RegisterReservedTable,
			RegisterValidateTable,
			Cfg,
			&AccessMode,
			(void *)NULL
		};
		Code=adi_device_access(&Device);
	}
	if (Cfg) free(Cfg);
	return Code;
}


/********************************************************************

	Function:  driver_register_callback
	
	Description: Register callback function

********************************************************************/

u32 driver_register_callback(void *Handle,u16 *Reg,u16 Value,ADI_DEVICE_ACCESS_MODE Mode)
{
	ADI_PDD_DRIVER *DevHandle=(ADI_PDD_DRIVER *)Handle;
	u32 Code=ADI_DEV_RESULT_SUCCESS;
	
	ADI_PDD_CHECK(DevHandle,ADI_DEV_RESULT_BAD_PDD_HANDLE);
	
	return Code;
}

/*****************************************************************************/

