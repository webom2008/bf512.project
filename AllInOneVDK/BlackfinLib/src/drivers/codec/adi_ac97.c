/**********************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_ac97.c,v $
$Revision: 2095 $
$Date: 2010-02-18 17:08:05 -0500 (Thu, 18 Feb 2010) $

Description:
    This is main source code for Audio Codec '97 (AC'97) driver

***********************************************************************/

/*********************************************************************

Include files

*********************************************************************/

/* system service includes */
#include <services/services.h>
/* device manager includes */
#include <drivers/adi_dev.h>
/* AC'97 driver includes */
#include <drivers/codec/adi_ac97.h>
/* memset function */
#include <string.h>

/*********************************************************************

Macros/Masks to manipulate AC'97 Output Slot 0 (TAG)

**********************************************************************/

#define     ADI_AC97_OUT_TAG_FRAME_VALID                0x8000      /* Frame valid                          */
#define     ADI_AC97_OUT_TAG_ADDR_VALID                 0xC000      /* Slot 1  - Command Address port valid */
#define     ADI_AC97_OUT_TAG_DATA_VALID                 0xA000      /* Slot 2  - Command Data valid         */
#define     ADI_AC97_OUT_TAG_PCM_L_VALID                0x9000      /* Slot 3  - PCM Left channel valid     */
#define     ADI_AC97_OUT_TAG_PCM_R_VALID                0x8800      /* Slot 4  - PCM Right channel valid    */
#define     ADI_AC97_OUT_TAG_PCM_OUT_VALID              0x9800      /* Slots 3 & 4 - PCM Front Out valid    */
#define     ADI_AC97_OUT_TAG_MODEM_LINE1_VALID          0x8400      /* Slot 5  - Modem Line 1 valid         */
#define     ADI_AC97_OUT_TAG_PCM_CTR_VALID              0x8200      /* Slot 6  - PCM Center valid           */
#define     ADI_AC97_OUT_TAG_PCM_SURR_L_VALID           0x8100      /* Slot 7  - PCM Left Surround valid    */
#define     ADI_AC97_OUT_TAG_PCM_SURR_R_VALID           0x8080      /* Slot 8  - PCM Right Surround valid   */
#define     ADI_AC97_OUT_TAG_PCM_SURR_VALID             0x8180      /* Slots 7 & 8 - PCM Surround out valid */
#define     ADI_AC97_OUT_TAG_PCM_LFE_VALID              0x8040      /* Slot 9  - PCM LFE valid              */
#define     ADI_AC97_OUT_TAG_MODEM_LINE2_VALID          0x8020      /* Slot 10 - Modem Line 2 valid         */
#define     ADI_AC97_OUT_TAG_PCM_L_N1_VALID             0x8020      /* Slot 10 - PCM Left n+1 valid         */
#define     ADI_AC97_OUT_TAG_MODEM_HANDSET_VALID        0x8010      /* Slot 11 - Modem Handset valid        */
#define     ADI_AC97_OUT_TAG_PCM_R_N1_VALID             0x8010      /* Slot 11 - PCM Right n+1 valid        */
#define     ADI_AC97_OUT_TAG_PCM_OUT_N1_VALID           0x8030      /* Slots 10 & 11 - PCM Out n+1 valid    */
#define     ADI_AC97_OUT_TAG_MODEM_GPIO_VALID           0x8008      /* Slot 12 - Modem GPIO valid           */
#define     ADI_AC97_OUT_TAG_PCM_CTR_N1_VALID           0x8008      /* Slot 12 - PCM Center n+1 valid       */
#define     ADI_AC97_OUT_TAG_CODEC_ID_MASK              0x0003      /* Codec ID                             */

/*********************************************************************

Macros/Masks to manipulate AC'97 Input Slot 0 (TAG)

**********************************************************************/

#define     ADI_AC97_IN_TAG_CODEC_READY                 0x8000      /* Codec Ready                              */
#define     ADI_AC97_IN_TAG_ADDR_VALID                  0xC000      /* Slot 1  - Command Address port valid     */
#define     ADI_AC97_IN_TAG_DATA_VALID                  0xE000      /* Slot 2  - Command Data valid             */
#define     ADI_AC97_IN_TAG_PCM_L_VALID                 0x9000      /* Slot 3  - PCM Record Left channel valid  */
#define     ADI_AC97_IN_TAG_PCM_R_VALID                 0x8800      /* Slot 4  - PCM Record Right channel valid */
#define     ADI_AC97_IN_TAG_PCM_L_R_VALID               0x9800      /* Slot 3 & 4  - PCM Record Left and Right channels valid */
#define     ADI_AC97_IN_TAG_MODEM_LINE1_VALID           0x8400      /* Slot 5  - Modem Line 1 valid             */
#define     ADI_AC97_IN_TAG_MIC_RECORD_VALID            0x8200      /* Slot 6  - MIC Record valid               */
#define     ADI_AC97_IN_TAG_VENDOR_RSRV_0_VALID         0x8100      /* Slot 7  - Vendor Reserved Slot 0 valid   */
#define     ADI_AC97_IN_TAG_VENDOR_RSRV_1_VALID         0x8080      /* Slot 8  - Vendor Reserved Slot 1 valid   */
#define     ADI_AC97_IN_TAG_VENDOR_RSRV_3_VALID         0x8040      /* Slot 9  - Vendor Reserved Slot 2 valid   */
#define     ADI_AC97_IN_TAG_MODEM_LINE2_VALID           0x8020      /* Slot 10 - Modem Line 2 valid             */
#define     ADI_AC97_IN_TAG_MODEM_HANDSET_VALID         0x8010      /* Slot 11 - Modem Handset valid            */
#define     ADI_AC97_IN_TAG_MODEM_GPIO_VALID            0x8008      /* Slot 12 - Modem GPIO valid               */

#define     ADI_AC97_ADDR_INDEX_MASK                    0x7F00
#define     ADI_AC97_SUB_SYSTEM_READY                   0xF000

/*********************************************************************

Macros/Masks to manipulate AC'97 Output Frame slots other than TAG

**********************************************************************/

#define     ADI_AC97_OUT_SLOT1_READ_CODEC_REG           0x8000      /* Slot 1 - Read/Write codec reg            */

/*********************************************************************

Macros/Masks to manipulate AC'97 Driver status Register 1
    'DriverStatus1' fields in AC'97 driver instance
    Bit 15      - Reserved
    Bit 14      - Reserved
    Bit 13      - LFE DAC Powerdown
    Bit 12      - Surround DAC Powerdown
    Bit 11      - Center DAC Powerdown
    Bit 10 to 3 - Reserved
    Bit 2       - SPDIF Tx Subsystem Enabled/Disabled
    Bit 1       - Double Rate Audio Enabled/Disabled
    Bit 0       - Variable Rate Audio Enabled/Disabled
**********************************************************************/
#define     ADI_AC97_FLAG_LFE_DAC_PDN                   0x2000
#define     ADI_AC97_FLAG_SURR_DAC_PDN                  0x1000
#define     ADI_AC97_FLAG_CTR_DAC_PDN                   0x0800
#define     ADI_AC97_FLAG_SPDIF_ENABLED                 0x0004
#define     ADI_AC97_FLAG_DRA_ENABLED                   0x0002
#define     ADI_AC97_FLAG_VRA_ENABLED                   0x0001

/* LFE,Surround,Center DACs power down */
#define     ADI_AC97_FLAG_LFE_SURR_CTR_PDN              0x3800

/*********************************************************************

Macros/Masks to manipulate AC'97 Driver status Register 2
    'DriverStatus2' fields in AC'97 driver instance
    Bit 15  - SLOT 16 enabled/disabled
    Bit 14  - Reserved
    Bit 13  - Reserved
    Bit 12  - Line output Enabled/Disabled
    Bit 11  - Center/LFE output Enabled/Disabled
    Bit 10  - Reserved
    Bit 9   - All DACs Powerdown
    Bit 8   - All ADCs Powerdown
    Bit 7   - Reserved
    Bit 6   - Receive buffer Loopback Enabled/Disabled
    Bit 5   - Transmit buffer Loopback Enabled/Disabled
    Bit 4   - DAC channles Enabled/Disabled
    Bit 3   - True Multi-channel audio Enabled/Disabled
    Bit 2   - Multichannel Audio Enabled/Disabled
    Bit 1   - Receive Dataflow Enabled/Disabled
    Bit 0   - Transmit Dataflow Enabled/Disabled

**********************************************************************/
#define     ADI_AC97_FLAG_SLOT16_ENABLED                    0x8000
#define     ADI_AC97_FLAG_LINE_OUT_DISABLED                 0x1000
#define     ADI_AC97_FLAG_CTR_LFE_OUT_DISABLED              0x0800
#define     ADI_AC97_FLAG_ALL_DAC_PDN                       0x0200
#define     ADI_AC97_FLAG_ALL_ADC_PDN                       0x0100
#define     ADI_AC97_FLAG_RX_LOOPBACK_ENABLED               0x0040
#define     ADI_AC97_FLAG_TX_LOOPBACK_ENABLED               0x0020
#define     ADI_AC97_FLAG_DAC_ENABLED                       0x0010
#define     ADI_AC97_FLAG_TRUE_MULTICHANNEL_AUDIO_ENABLED   0x0008
#define     ADI_AC97_FLAG_MULTICHANNEL_AUDIO_ENABLED        0x0004
#define     ADI_AC97_FLAG_RX_DATAFLOW_ENABLED               0x0002
#define     ADI_AC97_FLAG_TX_DATAFLOW_ENABLED               0x0001

/* Line out & Center/LFE disabled */
#define     ADI_AC97_FLAG_LINE_OUT_CTR_LFE_DISABLED         0x1800

/*********************************************************************

Macros/Masks to manipulate Audio Codec register field values
monitored by AC'97 driver instance

**********************************************************************/

/* Mask to extract following fields from Extended audio control register */
/*
    Bit 13 - LFE DAC Powerdown
    Bit 12 - Surround DAC Powerdown
    Bit 11 - Center DAC Powerdown
    Bit 2  - SPDIF Tx enable/disable
    Bit 1  - Double Audio Rate enable/disable
    Bit 0  - Variable Audio Rate enable/disable
*/
#define     ADI_AC97_MASK_EXTD_AUDIO_CTRL               0x7007

/* Mask to extract following fields from Serial Port Configuration register */
/*
    Bit 0  - Slot 16 mode enabled/disabled
*/
#define     ADI_AC97_MASK_SERIAL_PORT_CONFIG            0x8000

/* Mask to extract following fields from Power Control/Status register */
/*
    Bit 10 - All DACs Powerdown
    Bit 9  - All ADCs Powerdown
*/
#define     ADI_AC97_MASK_POWER_CTRL_STATUS             0x0300

/* Mask to extract following fields from Misc Control register 1 */
/*
    Bit 12 - Line Out Disable
    Bit 11 - Center/LFE Output Disable
*/
#define     ADI_AC97_MASK_MISC_CTRL_1                   0x1800

/*********************************************************************

AC'97 1D application buffer Reserved area format

**********************************************************************/
typedef struct ADI_AC97_BUFFER_RESERVED_AREA {  /* AC'97 1D application buffer Reserved area format             */
    u16                 *pData;                 /* pointer to data buffer holding/to hold audio data            */
    u32                 WordsRemaining;         /* # bytes yet to receive/transfer                              */
    ADI_DEV_1D_BUFFER   *pNext;                 /* Next 1D buffer in this Queue                                 */
    ADI_DEV_1D_BUFFER   *pLoopback;             /* Loopback to this 1D buffer when Loopback is enabled          */
}ADI_AC97_BUFFER_RESERVED_AREA;

/*********************************************************************

Local function prototypes

**********************************************************************/

/* Queues the given 1D buffer(s) to corresponding ADC/DAC channel */
static u32 adi_ac97_AddBuffersToQueue (
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* AC'97 driver instance we're working on                       */
    ADI_DEV_1D_BUFFER           **ChannelHead,  /* Address of ptr to Head of this ADC/DAC channel buffer queue  */
    ADI_DEV_1D_BUFFER           **ChannelTail,  /* Address of ptr to Tail of this ADC/DAC channel buffer queue  */
    ADI_DEV_1D_BUFFER           *pNewBuffer     /* New Buffer(s) to queue                                       */
);

/* Gets next ADC/DAC application buffer address from the ADC/DAC buffer chain */
static ADI_AC97_BUFFER_RESERVED_AREA* adi_ac97_GetNextAppBuffer(
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* AC'97 driver instance we're working on                       */
    ADI_DEV_1D_BUFFER           **ChannelHead,  /* Address of ptr to Head of this ADC/DAC channel buffer queue  */
    ADI_DEV_1D_BUFFER           **ChannelTail,  /* Address of ptr to Tail of this ADC/DAC channel buffer queue  */
    u32                         nLoopbackStatus /* 0 when loopback disabled, non-zero when enabled */
);

/* Inserts command address/data to AC'97 Out frame(s) and/or Retrives command data from AC'97 Input frame(s)    */
static void adi_ac97_RegAccess(
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* AC'97 driver instance we're working on                       */
    u16                         *pAdcFrame,     /* pointer to AC'97 ADC data frame we're working on             */
    u16                         *pDacFrame      /* pointer to AC'97 DAC data frame we're working on             */
);

/*  Updates 'DriverStatus' register in the given AC'97 driver instance */
static void adi_ac97_UpdateStatus(
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* AC'97 driver instance we're working on                       */
    u16                         RegAddr,        /* Audio Codec Register address to be updated                   */
    u16                         RegData         /* Register data to be updated with                             */
);

/* AC'97 driver callback function */
static void adi_ac97_Callback(                  /* Callback function passed to Audio Codec driver               */
    void                       *DeviceHandle,   /* Address of AC'97 driver instance used by the audio codec     */
    u32                         Event,          /* Callback Event                                               */
    void                       *pArg            /* Callback Argument                                            */
);

/*********************************************************************

    Function:       adi_ac97_Init_Instance

    Description:    Initialises AC'97 Driver instance

*********************************************************************/
u32 adi_ac97_Init_Instance (
    ADI_AC97_DRIVER_INIT        *pInstance     /* address of AC'97 Driver init information table   */
){
    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    ADI_DEV_CIRCULAR_BUFFER *pCircbuf;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Check if the given Data frame memory is sufficient */
    if ((pInstance->pDataFrame == NULL) ||
        (pInstance->DataFrameSize < ADI_AC97_DATA_FRAME_BASE_MEMORY))
    {
        Result = ADI_AC97_RESULT_INSUFFICIENT_DATA_FRAME_MEMORY;
    }
    /* Check whether the AC'97 driver instance is valid */
    else if (pInstance->pAC97 == NULL)
    {
        Result = ADI_AC97_RESULT_DRIVER_INSTANCE_INVALID;
    }

    /* continue only if the instance data is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif
        /***** Clear data frame memory to start with *****/
        memset(pInstance->pDataFrame, 0, pInstance->DataFrameSize);

        /***** Initialise ADC circular buffer *****/
        /* Circular buffer to handle ADC data */
        pCircbuf = &(pInstance->pAC97->AdcCircBuf);
        /* Location to hold Inbound Frame data */
        pCircbuf->Data = (void *) pInstance->pDataFrame;
        /* 2 sub-buffers (ping-pong buffers) */
        pCircbuf->SubBufferCount = 2;
        /* Element count per sub buffer */
        /* Data frame split in to 4 sub buffers, 2 for Inbound & 2 for outbound */
        pCircbuf->SubBufferElementCount = (pInstance->DataFrameSize/4);
        /* SPORT DMA bus width - 16bits by default */
        pCircbuf->ElementWidth = 2;
        pCircbuf->SubBufferElementCount = pCircbuf->SubBufferElementCount/pCircbuf->ElementWidth;
        /* No Callback for inbound data */
        pCircbuf->CallbackType = ADI_DEV_CIRC_SUB_BUFFER;
        /* No additional Info */
        pCircbuf->pAdditionalInfo = NULL;

        /***** Initialise DAC circular buffer *****/
        /* Circular buffer to handle DAC data */
        pCircbuf = &(pInstance->pAC97->DacCircBuf);
        /* Location to hold Outbound Frame data */
        pCircbuf->Data = (void *) ((u8 *)(pInstance->pDataFrame)+(pInstance->DataFrameSize/2));
        /* 2 sub-buffers (ping-pong buffers) */
        pCircbuf->SubBufferCount = 2;
        /* Element count per sub buffer */
        /* Data frame split in to 4 sub buffers, 2 for Inbound & 2 for outbound */
        pCircbuf->SubBufferElementCount = (pInstance->DataFrameSize/4);
        /* SPORT DMA bus width - 16bits by default */
        pCircbuf->ElementWidth = 2;
        pCircbuf->SubBufferElementCount = pCircbuf->SubBufferElementCount/pCircbuf->ElementWidth;
        /* Callback Type */
        pCircbuf->CallbackType = ADI_DEV_CIRC_NO_CALLBACK;
        /* No additional Info */
        pCircbuf->pAdditionalInfo = NULL;

        /* Calculate Number of AC'97 frames per sub-buffer from the data frame memory size allocated by the application */
        pInstance->pAC97->FramesPerSubBuf  = (pInstance->DataFrameSize / ADI_AC97_DATA_FRAME_BASE_MEMORY);
        /* Sub buffer 0 will be the first to generate callback */
        pInstance->pAC97->CallbackSubBuffer = 0;

        /***** Initialise application specific ADC data buffer info *****/
        /* Clear pointers to ADC buffers submitted by the application */
        pInstance->pAC97->AdcAppBuf.pInHead   = NULL;
        pInstance->pAC97->AdcAppBuf.pInTail   = NULL;

        /***** Initialise application specific DAC data buffer info *****/
        /* Clear pointers to DAC buffers submitted by the application */
        pInstance->pAC97->DacAppBuf.pOutHead  = NULL;
        pInstance->pAC97->DacAppBuf.pOutTail  = NULL;

        /* Update DAC data slot structure to handle 48kHz sample rate */
        adi_ac97_UpdateStatus(pInstance->pAC97,AC97_REG_FRONT_DAC_RATE,ADI_AC97_PCM_SAMPLE_RATE_48KHZ);

        /***** Clear Device access info *****/
        pInstance->pAC97->RegAccess.Cmd        = 0;
        pInstance->pAC97->RegAccess.pTable     = NULL;
        pInstance->pAC97->RegAccess.pAccess    = NULL;
        pInstance->pAC97->RegAccess.Mode       = ADI_AC97_CODEC_REG_ACCESS_IDLE;

        /***** AC'97 driver instance Callback function supplied to the Audio Codec *****/
        pInstance->pAC97->AC97Callback  = adi_ac97_Callback;

        /***** Clear AC'97 Driver Status registers *****/
        pInstance->pAC97->DriverStatus1 = 0;
        pInstance->pAC97->DriverStatus2 = 0;

        /* Set codec in idle state */
        pInstance->pAC97->DriverMode = ADI_AC97_MODE_CODEC_IDLE;

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}

/*********************************************************************

    Function:       adi_ac97_HwReset

    Description:    Resets the Audio codec via Hardware

*********************************************************************/
u32 adi_ac97_HwReset(
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* AC'97 driver instance we're working on       */
    ADI_FLAG_ID                 ResetFlagId     /* Flag ID connected to Audio Codec Reset pin   */
){
    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    u32 fcclk,fsclk,fvco, DelayCycles_100us;
    volatile u32 i;

    do
    {
        /* Set codec in idle state */
        pAC97->DriverMode = ADI_AC97_MODE_CODEC_IDLE;

        /* Clear pointers to application provided ADC data buffers */
        pAC97->AdcAppBuf.pInHead   = NULL;
        pAC97->AdcAppBuf.pInTail   = NULL;
        /* Clear pointers to application provided DAC data buffers */
        pAC97->DacAppBuf.pOutHead  = NULL;
        pAC97->DacAppBuf.pOutTail  = NULL;

        /* Get the present CCLK & SCLK frequency */
        if ((Result = adi_pwr_GetFreq(&fcclk,&fsclk,&fvco))!= ADI_PWR_RESULT_SUCCESS)
        {
            break;
        }

        /* calculate the Base delay count (for 100us) */
        DelayCycles_100us = (fcclk/10000);

        /* open this flag pin in GPIO mode */
        if ((Result = adi_flag_Open(ResetFlagId)) != ADI_FLAG_RESULT_SUCCESS)
        {
            break;
        }
        /* set this flag pin as output */
        if ((Result = adi_flag_SetDirection(ResetFlagId,ADI_FLAG_DIRECTION_OUTPUT)) != ADI_FLAG_RESULT_SUCCESS)
        {
            break;
        }

        /* De-assert Codec Reset */
        if ((Result = adi_flag_Set(ResetFlagId)) != ADI_FLAG_RESULT_SUCCESS)
        {
            break;
        }
        /* wait for 100us */
        for (i=DelayCycles_100us;i;i--);

        /* Assert Codec Reset */
        if ((Result = adi_flag_Clear(ResetFlagId)) != ADI_FLAG_RESULT_SUCCESS)
        {
            break;
        }
        /* keep the reset signal high for 1ms */
        for (i=(DelayCycles_100us * 10);i;i--);

        /* De-Assert Codec Reset */
        if ((Result = adi_flag_Set(ResetFlagId)) != ADI_FLAG_RESULT_SUCCESS)
        {
            break;
        }
        /* delay 1ms to allow the Codec to recover from Reset */
        for (i=(DelayCycles_100us * 10);i;i--);

        /* wait for codec to be ready */
        pAC97->DriverMode = ADI_AC97_MODE_CODEC_WAIT_FOR_CODEC_READY;

    }while(0);

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_ac97_Read

    Description:    Queues ADC (Inbound) buffers
                    Buffer to hold data from ADC channles in
                    interleaved format (Left channel, Right channel)

*********************************************************************/
u32 adi_ac97_Read (
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* Pointer to AC'97 Driver Instance                             */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* Buffer type (only supports ADI_DEV_1D_BUFFER type buffers)   */
    ADI_DEV_BUFFER              *pBuffer        /* pointer to buffer                                            */
){
    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* IF (buffer type is invalid) */
    if (BufferType != ADI_DEV_1D)
    {
        Result = ADI_AC97_RESULT_DRIVER_ONLY_SUPPORT_1D_BUFFERS;
    }
    /* ELSE (buffer type is valid) */
    else
    {
        /* Add this buffer to the existing ADC buffer queue */
        Result = adi_ac97_AddBuffersToQueue (pAC97,
                                             &pAC97->AdcAppBuf.pInHead,
                                             &pAC97->AdcAppBuf.pInTail,
                                             (ADI_DEV_1D_BUFFER *)pBuffer);
    }
    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_ac97_Write

    Description:    Queues DAC (Outbound) buffers
                    Buffer holds data for all DAC channles in
                    interleaved format

*********************************************************************/
u32 adi_ac97_Write (
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* Pointer to AC'97 Driver Instance                             */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* Buffer type (only supports ADI_DEV_1D_BUFFER type buffers)   */
    ADI_DEV_BUFFER              *pBuffer        /* pointer to buffer                                            */
){
    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* IF (buffer type is invalid) */
    if (BufferType != ADI_DEV_1D)
    {
        Result = ADI_AC97_RESULT_DRIVER_ONLY_SUPPORT_1D_BUFFERS;
    }
    /* ELSE (buffer type is valid) */
    else
    {
        /* Add this buffer to the existing DAC buffer queue */
        Result = adi_ac97_AddBuffersToQueue (pAC97,
                                             &pAC97->DacAppBuf.pOutHead,
                                             &pAC97->DacAppBuf.pOutTail,
                                             (ADI_DEV_1D_BUFFER *)pBuffer);
    }
    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_ac97_AddBuffersToQueue

    Description:    Queues the given 1D buffer(s) to corresponding
                    ADC/DAC channel

*********************************************************************/
static u32 adi_ac97_AddBuffersToQueue (
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* AC'97 driver instance we're working on                       */
    ADI_DEV_1D_BUFFER           **ChannelHead,  /* Address of ptr to Head of this ADC/DAC channel buffer queue  */
    ADI_DEV_1D_BUFFER           **ChannelTail,  /* Address of ptr to Tail of this ADC/DAC channel buffer queue  */
    ADI_DEV_1D_BUFFER           *pNewBuffer     /* New Buffer(s) to queue                                       */
){
    void                            *pExitCriticalArg;  /* return value from EnterCritical                                  */
    ADI_AC97_BUFFER_RESERVED_AREA   *pReservedArea;     /* reserved area of the buffer we're working on                     */
    ADI_AC97_BUFFER_RESERVED_AREA   *pLastBuffer;       /* reserved area of the last buffer in the already existing queue   */
    ADI_DEV_1D_BUFFER               *pWorkingBuffer;    /* Pointer to 1D buffer we're working on                            */

    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* FOR each buffer in the list */
    for (pWorkingBuffer = pNewBuffer; pWorkingBuffer != NULL; pWorkingBuffer = pWorkingBuffer->pNext)
    {
        /* reset the processed fields */
        pWorkingBuffer->ProcessedFlag           = FALSE;
        pWorkingBuffer->ProcessedElementCount   = 0;

        /* Reserved area of this buffer */
        pReservedArea = (ADI_AC97_BUFFER_RESERVED_AREA *)pWorkingBuffer->Reserved;
        /* Initialise the reserved area */
        pReservedArea->pData = pWorkingBuffer->Data;

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
        /* verify that the Data buffer is valid */
        if (pReservedArea->pData == NULL)
        {
            Result = ADI_AC97_RESULT_APP_DATA_POINTS_TO_NULL;
            break;
        }
#endif
        pReservedArea->WordsRemaining = (pWorkingBuffer->ElementCount * pWorkingBuffer->ElementWidth)/2;

        /* IF (this this the last buffer in the given chain) */
        if (pWorkingBuffer->pNext == NULL)
        {
            /* end of buffer chain */
            pReservedArea->pNext = NULL;
            /* protect this region from interrupts */
            pExitCriticalArg = adi_int_EnterCriticalRegion(pAC97->pEnterCriticalArg);
            /* IF (this is the first buffer to be queued) */
            if (*ChannelHead == NULL)
            {
                /* Point the channel head to first buffer in this chain */
                *ChannelHead = pNewBuffer;
                /* Point the channel head to last buffer in this chain */
                *ChannelTail = pWorkingBuffer;
                /* Set the loopback pointer of the last buffer to point the first buffer in this chain */
                pReservedArea->pLoopback = pNewBuffer;
            }
            /* ELSE (Queue this buffer to already exitsting chain) */
            else
            {
                pLastBuffer = (ADI_AC97_BUFFER_RESERVED_AREA *)(*ChannelTail)->Reserved;
                pReservedArea->pLoopback = pLastBuffer->pLoopback;
                pLastBuffer->pNext = pNewBuffer;
                *ChannelTail = pWorkingBuffer;
            }
            /* exit protected region */
            adi_int_ExitCriticalRegion(pExitCriticalArg);
        }
        /* ELSE (there are more buffers in this chain) */
        else
        {
            pReservedArea->pNext        = pWorkingBuffer->pNext;
            pReservedArea->pLoopback    = NULL;
        }

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
        /* verify that the buffer list chain is NULL terminated */
        if (pWorkingBuffer->pNext == pNewBuffer)
        {
            Result = ADI_DEV_RESULT_NON_TERMINATED_LIST;
            break;
        }
#endif
    /* ENDFOR */
    }

    /* return */
    return(Result);
}

/*********************************************************************

    Function:       adi_ac97_Control

    Description:    Sets/Senses AC'97 codec related information

*********************************************************************/
u32 adi_ac97_Control (
    ADI_AC97_DRIVER_INSTANCE    *pAC97,     /* Pointer to AC'97 Driver Instance         */
    u32                         Command,    /* Command ID                               */
    void                        *Value      /* Command specific value                   */
){
    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    ADI_DEV_BUFFER_PAIR  *pBufPair;
    u32 u32Value;

    /* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    if (pAC97 == NULL)
    {
        Result = ADI_AC97_RESULT_DRIVER_INSTANCE_INVALID;
    }
    /* Continue only if the given AC'97 driver instance is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif
        /* CASEOF (Command ID)  */
        switch (Command)
        {

            /* CASE: Control dataflow */
            case (ADI_DEV_CMD_SET_DATAFLOW):

                /* IF (Enable audio dataflow) */
                if ((u32)Value == true)
                {
                    /* Update driver status as transmit and receive dataflow enabled */
                    pAC97->DriverStatus2 |= (ADI_AC97_FLAG_TX_DATAFLOW_ENABLED |\
                                             ADI_AC97_FLAG_RX_DATAFLOW_ENABLED);

                    /*IF (Codec is ready for audio stream) */
                    if (pAC97->DriverMode >= ADI_AC97_MODE_CODEC_READY_FOR_AUDIO_STREAM)
                    {
                        /* Enable both Transmit and Receive dataflow */
                        pAC97->DriverMode |= (ADI_AC97_MODE_CODEC_TX_DATAFLOW_ENABLED |\
                                              ADI_AC97_MODE_CODEC_RX_DATAFLOW_ENABLED);
                    }
                }
                /* ELSE (Disable audio dataflow) */
                else
                {
                    /* Update driver status as transmit and receive dataflow disabled */
                    pAC97->DriverStatus2 &= ~(ADI_AC97_FLAG_TX_DATAFLOW_ENABLED |\
                                              ADI_AC97_FLAG_RX_DATAFLOW_ENABLED);

                    /*IF (Transmit or receive dataflow is enabled) */
                    if (pAC97->DriverMode > ADI_AC97_MODE_CODEC_READY_FOR_AUDIO_STREAM)
                    {
                        /* Reset driver mode as codec ready for audio stream */
                        pAC97->DriverMode = ADI_AC97_MODE_CODEC_READY_FOR_AUDIO_STREAM;
                    }
                    /* Clear pointers to DAC buffers submitted by the application */
                    pAC97->DacAppBuf.pOutHead  = NULL;
                    pAC97->DacAppBuf.pOutTail  = NULL;
                    /* Clear pointers to ADC buffers submitted by the application */
                    pAC97->AdcAppBuf.pInHead   = NULL;
                    pAC97->AdcAppBuf.pInTail   = NULL;
                }
                break;

            /* CASE (Control Audio Transmit dataflow) */
            case (ADI_AC97_CMD_SET_TRANSMIT_DATAFLOW):

                /* IF (Enable transmit dataflow) */
                if ((u32)Value == true)
                {
                    /* Update driver status as transmit dataflow enabled */
                    pAC97->DriverStatus2 |= ADI_AC97_FLAG_TX_DATAFLOW_ENABLED;

                    /*IF (Codec is ready for audio stream) */
                    if (pAC97->DriverMode >= ADI_AC97_MODE_CODEC_READY_FOR_AUDIO_STREAM)
                    {
                        /* Update driver mode as transmit dataflow enabled */
                        pAC97->DriverMode |= ADI_AC97_MODE_CODEC_TX_DATAFLOW_ENABLED;
                    }
                }
                /* ELSE (Disable transmit dataflow) */
                else
                {
                    /* Update driver status as transmit dataflow disabled */
                    pAC97->DriverStatus2 &= ~ADI_AC97_FLAG_TX_DATAFLOW_ENABLED;
                    /* Update driver mode as transmit dataflow disabled */
                    pAC97->DriverMode &= ~ADI_AC97_MODE_CODEC_TX_DATAFLOW_ENABLED;
                    /* Clear pointers to DAC buffers submitted by the application */
                    pAC97->DacAppBuf.pOutHead  = NULL;
                    pAC97->DacAppBuf.pOutTail  = NULL;
                }
                break;

            /* CASE (Control Audio Receive dataflow) */
            case (ADI_AC97_CMD_SET_RECEIVE_DATAFLOW):

                /* IF (Enable receive dataflow) */
                if ((u32)Value == true)
                {
                    /* Update driver status as receive dataflow enabled */
                    pAC97->DriverStatus2 |= ADI_AC97_FLAG_RX_DATAFLOW_ENABLED;

                    /*IF (Codec is ready for audio stream) */
                    if (pAC97->DriverMode >= ADI_AC97_MODE_CODEC_READY_FOR_AUDIO_STREAM)
                    {
                        /* Update driver mode as receive dataflow enabled */
                        pAC97->DriverMode |= ADI_AC97_MODE_CODEC_RX_DATAFLOW_ENABLED;
                    }
                }
                /* ELSE (Disable receive dataflow) */
                else
                {
                    /* Update driver status as receive dataflow disabled */
                    pAC97->DriverStatus2 &= ~ADI_AC97_FLAG_RX_DATAFLOW_ENABLED;
                    /* Update driver mode as receive dataflow disabled */
                    pAC97->DriverMode &= ~ADI_AC97_MODE_CODEC_RX_DATAFLOW_ENABLED;
                    /* Clear pointers to ADC buffers submitted by the application */
                    pAC97->AdcAppBuf.pInHead   = NULL;
                    pAC97->AdcAppBuf.pInTail   = NULL;
                }
                break;

            /* CASE: Set Dataflow method */
            case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
                /* Check for a valid dataflow method */
                /* The driver only supports Chained and Chained loopback methods */
                if (((ADI_DEV_MODE)Value != ADI_DEV_MODE_CHAINED) &&
                    ((ADI_DEV_MODE)Value != ADI_DEV_MODE_CHAINED_LOOPBACK))
                {
                    Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
                }
#endif
                /* update Loopback flag */
                if ((ADI_DEV_MODE)Value == ADI_DEV_MODE_CHAINED)
                {
                    /* disable loopback */
                    pAC97->DriverStatus2 &= ~(ADI_AC97_FLAG_TX_LOOPBACK_ENABLED |\
                                              ADI_AC97_FLAG_RX_LOOPBACK_ENABLED);
                }
                else
                {
                    /* enable loopback */
                    pAC97->DriverStatus2 |= (ADI_AC97_FLAG_TX_LOOPBACK_ENABLED |\
                                             ADI_AC97_FLAG_RX_LOOPBACK_ENABLED);
                }

                /* Clear pointers to DAC buffers submitted by the application */
                pAC97->DacAppBuf.pOutHead  = NULL;
                pAC97->DacAppBuf.pOutTail  = NULL;
                /* Clear pointers to ADC buffers submitted by the application */
                pAC97->AdcAppBuf.pInHead   = NULL;
                pAC97->AdcAppBuf.pInTail   = NULL;
                break;

            /* CASE (Set Transmit Dataflow method) */
            case (ADI_AC97_CMD_SET_TRANSMIT_DATAFLOW_METHOD):

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
                /* Check for a valid dataflow method */
                /* The driver only supports Chained and Chained loopback methods */
                if (((ADI_DEV_MODE)Value != ADI_DEV_MODE_CHAINED) &&
                    ((ADI_DEV_MODE)Value != ADI_DEV_MODE_CHAINED_LOOPBACK))
                {
                    Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
                }
#endif
                /* update Loopback flag */
                if ((ADI_DEV_MODE)Value == ADI_DEV_MODE_CHAINED)
                {
                    /* Disable loopback */
                    pAC97->DriverStatus2 &= ~ADI_AC97_FLAG_TX_LOOPBACK_ENABLED;
                }
                else
                {
                    /* Enable loopback */
                    pAC97->DriverStatus2 |= ADI_AC97_FLAG_TX_LOOPBACK_ENABLED;
                }
                /* Clear pointers to DAC buffers submitted by the application */
                pAC97->DacAppBuf.pOutHead  = NULL;
                pAC97->DacAppBuf.pOutTail  = NULL;
                break;

            /* CASE (Set Receive Dataflow method) */
            case (ADI_AC97_CMD_SET_RECEIVE_DATAFLOW_METHOD):

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
                /* Check for a valid dataflow method */
                /* The driver only supports Chained and Chained loopback methods */
                if (((ADI_DEV_MODE)Value != ADI_DEV_MODE_CHAINED) &&
                    ((ADI_DEV_MODE)Value != ADI_DEV_MODE_CHAINED_LOOPBACK))
                {
                    Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
                }
#endif
                /* update Loopback flag */
                if ((ADI_DEV_MODE)Value == ADI_DEV_MODE_CHAINED)
                {
                    /* Disable loopback */
                    pAC97->DriverStatus2 &= ~ADI_AC97_FLAG_RX_LOOPBACK_ENABLED;
                }
                else
                {
                    /* Enable loopback */
                    pAC97->DriverStatus2 |= ADI_AC97_FLAG_RX_LOOPBACK_ENABLED;
                }
                /* Clear pointers to ADC buffers submitted by the application */
                pAC97->AdcAppBuf.pInHead   = NULL;
                pAC97->AdcAppBuf.pInTail   = NULL;
                break;

            /* CASE: Control Audio Mode */
            case (ADI_AC97_CMD_ENABLE_MULTICHANNEL_AUDIO):
                /* IF (Enable Multichannel Audio) */
                if ((u32)Value)
                {
                    pAC97->DriverStatus2 |= ADI_AC97_FLAG_MULTICHANNEL_AUDIO_ENABLED;
                }
                /* ELSE (Disable Multichannel Audio) */
                else
                {
                    pAC97->DriverStatus2 &= ~ADI_AC97_FLAG_MULTICHANNEL_AUDIO_ENABLED;
                }
                break;

            /* CASE: Enable/Disable True Multi-channel audio mode. */
            case (ADI_AC97_CMD_ENABLE_TRUE_MULTICHANNEL_AUDIO):
                /* IF (Enable TRUE Multi-channel audio) */
                if ((u32)Value)
                {
                    /* Enable multi-channel audio mode */
                    pAC97->DriverStatus2 |= ADI_AC97_FLAG_MULTICHANNEL_AUDIO_ENABLED;
                    /* Enable True multi-channel audio flag */
                    pAC97->DriverStatus2 |= ADI_AC97_FLAG_TRUE_MULTICHANNEL_AUDIO_ENABLED;
                }
                /* ELSE (Disable TRUE Multi-channel audio) */
                else
                {
                    /* Disable True multi-channel audio flag */
                    pAC97->DriverStatus2 &= ~ADI_AC97_FLAG_TRUE_MULTICHANNEL_AUDIO_ENABLED;
                    /* leave multi-channel audio mode enabled */
                    pAC97->DriverStatus2 |= ADI_AC97_FLAG_MULTICHANNEL_AUDIO_ENABLED;
                }
                break;

            /* CASE (Clear pointers to ADC buffers submitted by the application) */
            case (ADI_AC97_CMD_CLEAR_ADC_APP_BUFFER_PTRS):
                pAC97->AdcAppBuf.pInHead   = NULL;
                pAC97->AdcAppBuf.pInTail   = NULL;
                break;

            /* CASE (Clear pointers to DAC buffers submitted by the application) */
            case (ADI_AC97_CMD_CLEAR_DAC_APP_BUFFER_PTRS):
                pAC97->DacAppBuf.pOutHead  = NULL;
                pAC97->DacAppBuf.pOutTail  = NULL;
                break;

            /* CASE (Update SPORT DMA Bus Width) */
            case (ADI_AC97_CMD_UPDATE_SPORT_DMA_BUS_WIDTH):

                /* IF (The DMA bus width is valid) */
                if ((u32)Value != 0)
                {
                    /* Get the present sub-buffer size in bytes */
                    u32Value = (pAC97->AdcCircBuf.SubBufferElementCount * pAC97->AdcCircBuf.ElementWidth);

                    /* Update Transmit SPORT DMA bus width */
                    pAC97->AdcCircBuf.ElementWidth = (u32) Value;
                    pAC97->AdcCircBuf.SubBufferElementCount = (u32Value/pAC97->AdcCircBuf.ElementWidth);

                    /* Update Receive SPORT DMA bus width */
                    pAC97->DacCircBuf.ElementWidth = (u32) Value;
                    pAC97->DacCircBuf.SubBufferElementCount = (u32Value/pAC97->DacCircBuf.ElementWidth);
                }
                break;

            /* unrecognised command */
            default:
                Result = ADI_AC97_RESULT_CMD_NOT_SUPPORTED;
                break;
        }
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}

/*********************************************************************

    Function:       adi_ac97_Callback

    Description:    AC'97 driver callback function

*********************************************************************/
static void adi_ac97_Callback(                  /* Callback function passed to Audio Codec driver               */
    void                       *DeviceHandle,   /* Address of AC'97 driver instance used by the audio codec     */
    u32                         Event,          /* Callback Event                                               */
    void                       *pArg            /* Callback Argument                                            */
){
    /* Pointer to circular buffer data we're working on */
    u16 *pDacData,*pAdcData;
    volatile u16 *ptAdcData,*ptDacData;
    ADI_AC97_BUFFER_RESERVED_AREA *pSrc;      /* Pointer to buffer holding source data (to transmit)                */
    ADI_AC97_BUFFER_RESERVED_AREA *pDest;     /* Pointer to buffer to hold destination data (to store received data)*/
    volatile u32 u32Temp;
    volatile u32 i,j;

    /* pointer to AC'97 instance we're working on */
    ADI_AC97_DRIVER_INSTANCE    *pAC97 = (ADI_AC97_DRIVER_INSTANCE *) DeviceHandle;

    /* enter buffer processing only if the codec is not in idle state */
    if (pAC97->DriverMode > ADI_AC97_MODE_CODEC_IDLE)
    {
        /* get the data buffer that generated this callback */
        if (pAC97->CallbackSubBuffer == 0)
        {
            /* ADC sub-buffer 0 data location */
            pAdcData = (u16*) (pAC97->AdcCircBuf.Data);
            /* DAC sub-buffer 0 data location */
            pDacData = (u16*) (pAC97->DacCircBuf.Data);
            /* next sub-buffer to generate callback */
            pAC97->CallbackSubBuffer = 1;
        }
        else
        {
            /* ADC sub-buffer 1 data location */
            pAdcData = (u16*) ((u8*)(pAC97->AdcCircBuf.Data) + (pAC97->FramesPerSubBuf * 32));
            /* DAC sub-buffer 1 data location */
            pDacData = (u16*) ((u8*)(pAC97->DacCircBuf.Data) + (pAC97->FramesPerSubBuf * 32));
            /* next sub-buffer to generate callback */
            pAC97->CallbackSubBuffer = 0;
        }

        /* store the addresses in corresponding temp locations */
        ptAdcData = pAdcData;
        ptDacData = pDacData;

        /* IF (Codec is ready for audio data stream) */
        if (pAC97->DriverMode >= ADI_AC97_MODE_CODEC_READY_FOR_AUDIO_STREAM)
        {
            /* IF (Transmit dataflow is enabled) */
            if (pAC97->DriverMode & ADI_AC97_MODE_CODEC_TX_DATAFLOW_ENABLED)
            {
                /* Get the address of DAC application buffer we will be working on */
                pSrc = (ADI_AC97_BUFFER_RESERVED_AREA*)(pAC97->DacAppBuf.pOutHead);
            }
            /* ELSE (Transmit dataflow is disabled) */
            else
            {
                /* Set DAC application buffer address as NULL */
                pSrc = NULL;
            }

            /* IF (Receive dataflow is enabled) */
            if (pAC97->DriverMode & ADI_AC97_MODE_CODEC_RX_DATAFLOW_ENABLED)
            {
                /* Get the address of ADC application buffer we will be working on */
                pDest = (ADI_AC97_BUFFER_RESERVED_AREA*)(pAC97->AdcAppBuf.pInHead);
            }
            /* ELSE (Receive dataflow is disabled) */
            else
            {
                /* Set ADC application buffer address as NULL */
                pDest = NULL;
            }

            /* (FOR all ADC and DAC data frames) */
            for (i=0;
                 i < pAC97->FramesPerSubBuf;
                 i++, ptAdcData+=16, ptDacData+=16)
            {
                /* IF (we've any ADC application buffer to process) */
                if (pDest != NULL)
                {
                    /* while (ADC application buffer has no elements to process) */
                    while (pDest->WordsRemaining == 0)
                    {
                        /* move to next application buffer */
                        pDest = adi_ac97_GetNextAppBuffer(pAC97,
                                                          &pAC97->AdcAppBuf.pInHead,
                                                          &pAC97->AdcAppBuf.pInTail,
                                                          (pAC97->DriverStatus2 & ADI_AC97_FLAG_RX_LOOPBACK_ENABLED));
                        /* IF (no more application buffers left) */
                        if (pDest == NULL)
                        {
                            break;
                        }
                    }
                }

                /* Extract data from ADC frame(s) */

                /* IF (we've any ADC application buffer to process) */
                if (pDest != NULL)
                {
                    /* IF (PCM record left & right channel data is valid) */
                    if ((*ptAdcData & ADI_AC97_IN_TAG_PCM_L_R_VALID) == ADI_AC97_IN_TAG_PCM_L_R_VALID)
                    {
                        /* IF (Codec in Slot-16 mode) */
                        if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
                        {
                            /* copy PCM Record Left channel data to application provided location */
                            *(pDest->pData) = *(ptAdcData+3);
                            pDest->pData++;
                            /* copy PCM Record Right channel data to application provided location */
                            *(pDest->pData) = *(ptAdcData+4);
                            pDest->WordsRemaining-=2;
                        }
                        /* ELSE (Codec is set in Standard AC'97 data mode) */
                        else
                        {
                            /* Combine the PCM record left channel data from slots 3 & 4 */
                            /* copy PCM Record Left channel data to application provided location */
                            /* copy lower 16 bits */
                            *(pDest->pData) = ((*(ptAdcData+3) << 12) | (*(ptAdcData+4) >> 4));
                            pDest->pData++;
                            /* copy remaining 4 bits */
                            *(pDest->pData) = ((*(ptAdcData+3) >> 4) & 0xF);
                            pDest->pData++;
                            /* Combine the PCM record Right channel data from slots 4 & 5 */
                            /* copy PCM Record Right channel data to application provided location */
                            /* copy lower 16 bits */
                            *(pDest->pData) = *(ptAdcData+5);
                            pDest->pData++;
                            /* copy remaining 4 bits */
                            *(pDest->pData) = (*(ptAdcData+4) & 0xF);
                            pDest->WordsRemaining-=4;
                        }
                        pDest->pData++;
                    }
                    else
                    {
                        pDest->pData = pDest->pData;
                    }
                }

                /* IF (we've any DAC application buffer to process) */
                if (pSrc != NULL)
                {
                    /* while (DAC application buffer has no elements to process) */
                    while (pSrc->WordsRemaining == 0)
                    {
                        /* move to next application buffer */
                        pSrc = adi_ac97_GetNextAppBuffer(pAC97,
                                                         &pAC97->DacAppBuf.pOutHead,
                                                         &pAC97->DacAppBuf.pOutTail,
                                                         (pAC97->DriverStatus2 & ADI_AC97_FLAG_TX_LOOPBACK_ENABLED));

                        /* IF (no more application buffers left) */
                        if (pSrc == NULL)
                        {
                            break;
                        }
                    }
                }

                /* Insert data to DAC frame(s) */

                /* IF (one or more DACs enabled and we've DAC application buffer to process */
                if ((pSrc != NULL) && (pAC97->DriverStatus2 & ADI_AC97_FLAG_DAC_ENABLED))
                {
                    /* clear TAG field to start with */
                    *ptDacData = 0;

                    /* IF (Any valid slots remaining to transfer) */
                    if (pAC97->DacAppBuf.ValidSlotsRemain)
                    {
                        /* IF (PCM Front/Line out DACs Enabled) */
                        if (!(pAC97->DriverStatus2 & ADI_AC97_FLAG_LINE_OUT_DISABLED))
                        {
                            /* IF (Codec in Slot-16 mode) */
                            if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
                            {
                                /* PCM Front/Line out Left channel */
                                *(ptDacData+3) = *(pSrc->pData);
                                pSrc->pData++;
                                /* PCM Front/Line out Right channel */
                                *(ptDacData+4) = *(pSrc->pData);
                                pSrc->pData++;
                                pSrc->WordsRemaining -= 2;
                            }
                            /* ELSE (Codec is set in Standard AC'97 data mode) */
                            else
                            {
                                /* get PCM Line out - Left channel data */
                                u32Temp = *((u32*)pSrc->pData);
                                /* split this data and copy it to AC'97 frame */
                                *(ptDacData+3) = (u16)((u32Temp >> 12) & 0xFF);
                                *(ptDacData+4) = (u16)(u32Temp << 4);

                                /* get PCM Line out - Right channel data */
                                pSrc->pData+=2;
                                u32Temp = *((u32*)pSrc->pData);
                                /* split this data and copy it to AC'97 frame */
                                *(ptDacData+4) |= (u16)((u32Temp >> 16) & 0xF);
                                *(ptDacData+5) = (u16)(u32Temp);

                                /* move to next location to access DAC application buffer as 32-bit chunks */
                                pSrc->pData+=2;
                                pSrc->WordsRemaining -= 4;
                            }
                            /* Set TAG - mark PCM Front out slots as valid */
                            *ptDacData |= ADI_AC97_OUT_TAG_PCM_OUT_VALID;
                        }

                        /******************
                        Double Rate Audio
                        ******************/
                        /* When using the double rate audio, the audio codec only supports front out DACs and
                           all other DACs (surround, center and LFE) are automatically powered down */
                        /* IF (Double-Rate Audio Enabled) */
                        if (pAC97->DriverStatus1 & ADI_AC97_FLAG_DRA_ENABLED)
                        {
                            /* IF (PCM Front/Line out DACs Enabled) */
                            if (!(pAC97->DriverStatus2 & ADI_AC97_FLAG_LINE_OUT_DISABLED))
                            {
                                /* IF (Codec in Slot-16 mode) */
                                if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
                                {
                                     /* PCM Front/Line out Left channel n+1 */
                                    *(ptDacData+10) = *(pSrc->pData);
                                    pSrc->pData++;
                                     /* PCM Front/Line out Right channel n+1 */
                                    *(ptDacData+11) = *(pSrc->pData);
                                    pSrc->pData++;
                                    pSrc->WordsRemaining -= 2;
                                }
                                /* ELSE (Codec is set in Standard AC'97 data mode) */
                                else
                                {
                                    /* get PCM Line out - Left channel n+1 data */
                                    u32Temp = *((u32*)pSrc->pData);
                                    /* split this data and copy it to AC'97 frame */
                                    *(ptDacData+12) |= (u16)((u32Temp >> 8) & 0xFFF);
                                    *(ptDacData+13) = (u16)(u32Temp << 8);

                                    /* get PCM Line out - Right channel n+1 data */
                                    pSrc->pData+=2;
                                    u32Temp = *((u32*)pSrc->pData);
                                    /* split this data and copy it to AC'97 frame */
                                    *(ptDacData+13) |= (u16)((u32Temp >> 12) & 0xFF);
                                    *(ptDacData+14) = (u16)(u32Temp << 4);

                                    /* move to next location to access DAC application buffer as 32-bit chunks */
                                    pSrc->pData+=2;
                                    pSrc->WordsRemaining -= 4;
                                }
                                /* Set TAG - mark PCM Front out n+1 slots as valid */
                                *ptDacData |= ADI_AC97_OUT_TAG_PCM_OUT_N1_VALID;
                            }
                        }
                        /*******************
                        Multichannel Audio
                        *******************/
                        /* ELSE IF (Multi-channel audio mode enabled) */
                        else if (pAC97->DriverStatus2 & ADI_AC97_FLAG_MULTICHANNEL_AUDIO_ENABLED)
                        {
                            /************************
                            True Multi-channel Audio
                            ************************/
                            /* IF (True Multi-channel audio mode enabled) */
                            if (pAC97->DriverStatus2 & ADI_AC97_FLAG_TRUE_MULTICHANNEL_AUDIO_ENABLED)
                            {
                                /* IF (Center DAC Enabled) */
                                if (!(pAC97->DriverStatus2 & ADI_AC97_FLAG_CTR_LFE_OUT_DISABLED))
                                {
                                    /* IF (Codec in Slot-16 mode) */
                                    if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
                                    {
                                        *(ptDacData+6) = *(pSrc->pData);
                                        pSrc->pData++;
                                        pSrc->WordsRemaining--;
                                    }
                                    /* ELSE (Codec is set in Standard AC'97 data mode) */
                                    else
                                    {
                                        /* get center DAC data */
                                        u32Temp = *((u32*)pSrc->pData);
                                        /* split this data and copy it to AC'97 frame */
                                        *(ptDacData+7) = (u16)(u32Temp >> 8);
                                        *(ptDacData+8) = (u16)(u32Temp << 8);

                                        /* move to next location to access DAC application buffer as 32-bit chunks */
                                        pSrc->pData+=2;
                                        pSrc->WordsRemaining-=2;
                                    }
                                    /* Set TAG - mark PCM center slot as valid */
                                    *ptDacData |= ADI_AC97_OUT_TAG_PCM_CTR_VALID;
                                }

                                /* IF (Surround DACs Enabled) */
                                if (!(pAC97->DriverStatus1 & ADI_AC97_FLAG_SURR_DAC_PDN))
                                {
                                    /* IF (Codec in Slot-16 mode) */
                                    if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
                                    {
                                        /* PCM Surround out Left channel */
                                        *(ptDacData+7) = *(pSrc->pData);
                                        pSrc->pData++;
                                        /* PCM Surround out Right channel */
                                        *(ptDacData+8) = *(pSrc->pData);
                                        pSrc->pData++;
                                        pSrc->WordsRemaining -= 2;
                                    }
                                    /* ELSE (Codec is set in Standard AC'97 data mode) */
                                    else
                                    {
                                        /* get Surround - Left channel data */
                                        u32Temp = *((u32*)pSrc->pData);
                                        /* split this data and copy it to AC'97 frame */
                                        *(ptDacData+8) |= (u16)((u32Temp >> 12) & 0xFF);
                                        *(ptDacData+9) = (u16)(u32Temp << 4);

                                        /* get Surround - Right channel data */
                                        pSrc->pData+=2;
                                        u32Temp = *((u32*)pSrc->pData);
                                        /* split this data and copy it to AC'97 frame */
                                        *(ptDacData+9) |= (u16)((u32Temp >> 16) & 0xF);
                                        *(ptDacData+10) = (u16)(u32Temp);

                                        /* move to next location to access DAC application buffer as 32-bit chunks */
                                        pSrc->pData+=2;
                                        pSrc->WordsRemaining -= 4;
                                    }
                                    /* Set TAG - mark PCM Surround slots as valid */
                                    *ptDacData |= ADI_AC97_OUT_TAG_PCM_SURR_VALID;
                                }

                                /* IF (LFE DAC Disabled) */
                                if ((pAC97->DriverStatus1 & ADI_AC97_FLAG_LFE_DAC_PDN) ||
                                    (pAC97->DriverStatus2 & ADI_AC97_FLAG_CTR_LFE_OUT_DISABLED))
                                {
                                    /* clear LFE DAC slot */
                                }
                                /* ELSE (LFE DAC Enabled) */
                                else
                                {
                                    /* IF (Codec in Slot-16 mode) */
                                    if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
                                    {
                                        *(ptDacData+9) = *(pSrc->pData);
                                        pSrc->pData++;
                                        pSrc->WordsRemaining--;
                                    }
                                    /* ELSE (Codec is set in Standard AC'97 data mode) */
                                    else
                                    {
                                        /* Get LFE DAC data */
                                        u32Temp = *((u32*)pSrc->pData);
                                        /* split this data and copy it to AC'97 frame */
                                        *(ptDacData+11) = (u16)(u32Temp >> 4);
                                        *(ptDacData+12) = (u16)(u32Temp << 12);

                                        /* move to next location to access DAC application buffer as 32-bit chunks */
                                        pSrc->pData+=2;
                                        pSrc->WordsRemaining-=2;
                                    }
                                    /* Set TAG - mark PCM LFE slot as valid */
                                    *ptDacData |= ADI_AC97_OUT_TAG_PCM_LFE_VALID;
                                }
                            }
                            /**************************
                            Pseudo Multi-channel Audio
                            **************************/
                            /* ELSE (Pseudo multi-channel audio mode) */
                            else
                            {
                                /* IF (Codec in Slot-16 mode) */
                                if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
                                {
                                    /* copy Line Out Left channel data to Center out channel */
                                    *(ptDacData+6) = *(ptDacData+3);
                                    /* copy Line Out Left channel data to Surround out Left channel */
                                    *(ptDacData+7) = *(ptDacData+3);
                                    /* copy Line Out Right channel data to Surround out Right channel */
                                    *(ptDacData+8) = *(ptDacData+4);
                                    /* copy Line Out Right channel data to LFE channel */
                                    *(ptDacData+9) = *(ptDacData+4);
                                }
                                /* ELSE (Codec set in Standard AC'97 data mode) */
                                else
                                {
                                    /* get Line-out Left channel data */
                                    u32Temp = *((u32*)((pSrc->pData)-6));
                                    /* split this data and copy it to Center DAC slots */
                                    *(ptDacData+7) = (u16)(u32Temp >> 8);
                                    *(ptDacData+8) = (u16)(u32Temp << 8);
                                    /* split this data and copy it to Surround out Left channel slots */
                                    *(ptDacData+8) |= (u16)((u32Temp >> 12) & 0xFF);
                                    *(ptDacData+9) = (u16)(u32Temp << 4);

                                    /* get Line-out Right channel data */
                                    u32Temp = *((u32*)((pSrc->pData)-4));
                                    /* split this data and copy it to Surround out Right channel slots */
                                    *(ptDacData+9) |= (u16)((u32Temp >> 16) & 0xF);
                                    *(ptDacData+10) = (u16)(u32Temp);
                                    /* split this data and copy it to AC'97 frame */
                                    *(ptDacData+11) = (u16)(u32Temp >> 4);
                                    *(ptDacData+12) = (u16)(u32Temp << 12);
                                }
                                /* Set TAG - mark PCM center,surround & LFE slots as valid */
                                *ptDacData |= ( ADI_AC97_OUT_TAG_PCM_CTR_VALID  |\
                                                ADI_AC97_OUT_TAG_PCM_SURR_VALID |\
                                                ADI_AC97_OUT_TAG_PCM_LFE_VALID);
                            }
                        }
                        /* decrement valid slots remaining count */
                        pAC97->DacAppBuf.ValidSlotsRemain--;
                    }
                    /* ELSE (no valid slots left to transfer) */
                    else
                    {
                        /* send an invalid slot */
                        /* Set TAG - Mark Frame as valid, but DAC data slots as invalid */
                        *ptDacData = ADI_AC97_OUT_TAG_FRAME_VALID;
                        /* decrement the invalid slots remaining count */
                        pAC97->DacAppBuf.InValidSlotsRemain--;
                    }

                    /* IF (no more valid or invalid slots left to transfer) */
                    if ((pAC97->DacAppBuf.ValidSlotsRemain == 0) &&
                        (pAC97->DacAppBuf.InValidSlotsRemain == 0))
                    {
                        /* If (Variable Rate Audio is enabled) */
                        if (pAC97->DriverStatus1 & ADI_AC97_FLAG_VRA_ENABLED)
                        {
                            /* decrement the slot out ratio repeat count */
                            pAC97->DacAppBuf.SlotRepeatCount--;
                            /* IF (done with the present slot out ratio) */
                            if (pAC97->DacAppBuf.SlotRepeatCount == 0)
                            {
                                /* move to next slot out ratio combination in the list */
                                pAC97->DacAppBuf.pSlotOutRatio = (pAC97->DacAppBuf).pSlotOutRatio->pNextSlotRatio;
                                /* IF (end of slot out ratio list) */
                                if (pAC97->DacAppBuf.pSlotOutRatio == NULL)
                                {
                                    /* decrement the slot out sequence count */
                                    pAC97->DacAppBuf.SlotSequenceCount--;
                                    /* IF (done with the present slot out sequence) */
                                    if (pAC97->DacAppBuf.SlotSequenceCount == 0)
                                    {
                                        /* move to next slot sequence in the list */
                                        pAC97->DacAppBuf.pSlotSequence = (pAC97->DacAppBuf).pSlotSequence->pNextSequence;
                                        /* IF (end of slot out sequence list) */
                                        if (pAC97->DacAppBuf.pSlotSequence == NULL)
                                        {
                                            /* loopback to slot sequence 0 */
                                            pAC97->DacAppBuf.pSlotSequence = &(pAC97->DacAppBuf.SlotSequence[0]);
                                        }
                                        /* get the number of times to repeat this slot sequence */
                                        pAC97->DacAppBuf.SlotSequenceCount = (pAC97->DacAppBuf).pSlotSequence->RepeatSlotSequence;
                                    }
                                    /* update the pointer to slot output ratio we're working on */
                                    pAC97->DacAppBuf.pSlotOutRatio = (pAC97->DacAppBuf).pSlotSequence->pSlotRatio;
                                }
                                /* Get the slot out ratio repeat count */
                                pAC97->DacAppBuf.SlotRepeatCount = (pAC97->DacAppBuf).pSlotOutRatio->RepeatSlotOut;
                            }
                            /* Get the number of valid samples to be sent */
                            pAC97->DacAppBuf.ValidSlotsRemain   = (pAC97->DacAppBuf).pSlotOutRatio->ValidSlots;
                            /* Get the number of invalid samples to be sent */
                            pAC97->DacAppBuf.InValidSlotsRemain = (pAC97->DacAppBuf).pSlotOutRatio->InvalidSlots;
                        }
                        /* ELSE (VRA is disabled, means DAC sampling rate is 48kHz) */
                        else
                        {
                            /* number of valid samples to be sent */
                            pAC97->DacAppBuf.ValidSlotsRemain   = 48000;
                            /* no invalid samples for 48kHz */
                            pAC97->DacAppBuf.InValidSlotsRemain = 0;
                        }
                    }
                }
                /* ELSE (All DACs disabled or No DAC application buffer(s) available to process) */
                else
                {
                    /* Set TAG - Mark Frame as valid, but DAC data slots as invalid */
                    *ptDacData = ADI_AC97_OUT_TAG_FRAME_VALID;
                }

            } /* End of for (All ADC and DAC frames) */

        }
        /* ELSE IF (we waiting for the to be codec ready) */
        else if (pAC97->DriverMode == ADI_AC97_MODE_CODEC_WAIT_FOR_CODEC_READY)
        {
            /* check if the codec is ready */
            for (i=0;i<pAC97->FramesPerSubBuf;i++,pAdcData+=16)
            {
                if (*pAdcData & ADI_AC97_IN_TAG_CODEC_READY)
                {
                    /* codec is ready. read power status register */
                    for (j=0;j<pAC97->FramesPerSubBuf;j++,pDacData+=16)
                    {
                        /* AC'97 address slot - Read power control/status register */
                        *(pDacData+1) = (AC97_REG_POWER_CTRL_STAT | ADI_AC97_OUT_SLOT1_READ_CODEC_REG) ;
                        /* Set TAG - mark command address slot as valid  */
                        *pDacData = ADI_AC97_OUT_TAG_ADDR_VALID;
                    }
                    pAC97->DriverMode = ADI_AC97_MODE_CODEC_WAIT_FOR_SUB_SYSTEM_READY;
                    break;
                }
            }
        }
        /* ELSE IF (we waiting for codec sub-system to be ready) */
        else if (pAC97->DriverMode == ADI_AC97_MODE_CODEC_WAIT_FOR_SUB_SYSTEM_READY)
        {
            /* Check sub-system status (Power Register bits 0 to 3) */
            for (i=0;i<pAC97->FramesPerSubBuf;i++,pAdcData+=16)
            {
                /* check for valid slot with vaild command address and data */
                if ((*pAdcData & ADI_AC97_IN_TAG_DATA_VALID) == ADI_AC97_IN_TAG_DATA_VALID)
                {
                    /* Audio Codec is in Standard AC'97 frame mode */
                    /* IF (Register address is to read Power Control register) */
                    if ((*(pAdcData + 1) & ADI_AC97_ADDR_INDEX_MASK) == AC97_REG_POWER_CTRL_STAT)
                    {
                        /* data block 3 - bits 12 to 15 hold Power status bits 0 to 3 */
                        if ((*(pAdcData + 3) & ADI_AC97_SUB_SYSTEM_READY) == ADI_AC97_SUB_SYSTEM_READY)
                        {
                            /* Update driver mode as Codec is ready to start Audio stream */
                            pAC97->DriverMode = ADI_AC97_MODE_CODEC_READY_FOR_AUDIO_STREAM;

                            /* IF (Transmit dataflow is enabled) */
                            if (pAC97->DriverStatus2 & ADI_AC97_FLAG_TX_DATAFLOW_ENABLED)
                            {
                                /* update driver mode */
                                pAC97->DriverMode |= ADI_AC97_MODE_CODEC_TX_DATAFLOW_ENABLED;
                            }

                            /* IF (Receive dataflow is enabled) */
                            if (pAC97->DriverStatus2 & ADI_AC97_FLAG_RX_DATAFLOW_ENABLED)
                            {
                                /* update driver mode */
                                pAC97->DriverMode |= ADI_AC97_MODE_CODEC_RX_DATAFLOW_ENABLED;
                            }

                            /* exit this loop */
                            break;
                        }
                    }
                }
            }
            /* Don't enter audio stream mode until the codec sub-system is ready */
            /* Keep reading power contol register to validate codec sub-system status */
        }

        /* IF (Codec is ready for audio data stream) */
        if (pAC97->DriverMode >= ADI_AC97_MODE_CODEC_READY_FOR_AUDIO_STREAM)
        {
            /* IF (we've any register access command/table to process) */
            if (pAC97->RegAccess.Cmd)
            {
                /* check if the register access is complete */
                if (pAC97->RegAccess.Mode == ADI_AC97_CODEC_REG_ACCESS_COMPLETE)
                {
                    /* clear register access info */
                    pAC97->RegAccess.Cmd = 0;
                    /* post callback as register access complete */
                    (pAC97->CodecCallback) ((void *)pAC97, ADI_AC97_EVENT_REGISTER_ACCESS_COMPLETE, pAC97->RegAccess.pTable);
                }
                /* ELSE (process the register access table) */
                else
                {
                    /* insert/extract required register data */
                    adi_ac97_RegAccess(pAC97,pAdcData,pDacData);
                }
            }
        }
    }
    /* return */
}

/*********************************************************************

    Function:       adi_ac97_GetNextAppBuffer

    Description:    Gets next ADC/DAC application buffer address
                    from the ADC/DAC buffer chain

*********************************************************************/
static ADI_AC97_BUFFER_RESERVED_AREA* adi_ac97_GetNextAppBuffer(
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* AC'97 driver instance we're working on */
    ADI_DEV_1D_BUFFER           **ChannelHead,  /* Address of ptr to Head of this ADC/DAC channel buffer queue  */
    ADI_DEV_1D_BUFFER           **ChannelTail,  /* Address of ptr to Tail of this ADC/DAC channel buffer queue  */
    u32                         nLoopbackStatus /* 0 when loopback disabled, non-zero when enabled */
){

    ADI_AC97_BUFFER_RESERVED_AREA   *pWorkingReserved;
    ADI_DEV_1D_BUFFER               *pWorkingBuffer,*pCallbackBuffer;
    void                            *pExitCriticalArg;  /* return value from EnterCritical */

    pWorkingBuffer = *ChannelHead;
    pWorkingReserved = (ADI_AC97_BUFFER_RESERVED_AREA*)(*ChannelHead);
    pCallbackBuffer = NULL;

    /* IF (there is any ADC application buffer to work with) */
    if (pWorkingBuffer != NULL)
    {
        /* IF (callback enabled for this buffer) */
        if (pWorkingBuffer->CallbackParameter)
        {
            pCallbackBuffer = pWorkingBuffer;
        }

        /* IF (Loopback enabled) */
        if (nLoopbackStatus != 0)
        {
            /* reset the working buffer reserved area */
            pWorkingReserved->pData = pWorkingBuffer->Data;
            pWorkingReserved->WordsRemaining = (pWorkingBuffer->ElementCount * pWorkingBuffer->ElementWidth)/2;
        }

        /* Update processed element count */
        pWorkingBuffer->ProcessedElementCount = (pWorkingBuffer->ElementCount *\
                                                 pWorkingBuffer->ElementWidth);
        /* Update processed flag */
        pWorkingBuffer->ProcessedFlag = true;

        /* protect this region from interrupts */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pAC97->pEnterCriticalArg);
        /* IF (end of buffer chain) */
        if (pWorkingReserved->pNext == NULL)
        {
            /* IF (Loopback enabled) */
            if (nLoopbackStatus != 0)
            {
                /* loopback to start of this buffer chain */
                *ChannelHead = pWorkingReserved->pLoopback;
            }
            /* ELSE (Loopback disabled) */
            else
            {
                /* reset this buffer chain */
                *ChannelHead = NULL;
                *ChannelTail = NULL;
            }
        }
        /* ELSE (more buffer(s) avaliable in this chain) */
        else
        {
            /* Update channel head to point next buffer in the chain */
            *ChannelHead = pWorkingReserved->pNext;
        }
        /* exit protected region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        /* Get the reserved area address of the buffer pointed by Channel head */
        pWorkingReserved = (ADI_AC97_BUFFER_RESERVED_AREA*)(*ChannelHead);
    }
    /* ELSE (there are no application buffers available to process) */
    else
    {
        pWorkingReserved = NULL;
    }

    /* IF (callback enabled for this buffer) */
    if (pCallbackBuffer)
    {
        /* post callback to indicate buffer completion */
        (pAC97->CodecCallback) ((void *)pAC97, ADI_DEV_EVENT_BUFFER_PROCESSED, pCallbackBuffer->CallbackParameter);

        /* IF (Loopback enabled) */
        if (nLoopbackStatus != 0)
        {
            /* update data pointer & word count of this callback buffer reserved area since
               the application might have updated it within its callback */
            ((ADI_AC97_BUFFER_RESERVED_AREA *)pCallbackBuffer)->pData = pCallbackBuffer->Data;
            ((ADI_AC97_BUFFER_RESERVED_AREA *)pCallbackBuffer)->WordsRemaining = (pCallbackBuffer->ElementCount * pCallbackBuffer->ElementWidth)/2;
        }

    }

    return (pWorkingReserved);
}

/*********************************************************************

    Function:       adi_ac97_RegAccess

    Description:    Inserts command assress/command data to
                    AC'97 Out frame(s) and/or
                    Retrives command data from AC'97 In frame(s)

*********************************************************************/
static void adi_ac97_RegAccess(
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* AC'97 driver instance we're working on           */
    u16                         *pAdcFrame,     /* pointer to AC'97 ADC data frame we're working on */
    u16                         *pDacFrame      /* pointer to AC'97 DAC data frame we're working on */
){

    u32 i,j;
    u16 RegData,FieldData,ShiftMask;
    u8  ShiftCount;
    u8  DataValid;  /* register data valid flag */

    /* CASEOF (Device Access Command) */
    switch (pAC97->RegAccess.Cmd)
    {
        /* CASE (Configure a specific register) */
        case(ADI_DEV_CMD_REGISTER_WRITE):
        /* CASE (Configure selected registers) */
        case(ADI_DEV_CMD_REGISTER_TABLE_WRITE):

            /* Load AC'97 Command Address port slot with the register address to update */
            *(pDacFrame+1) = *(pAC97->RegAccess.pAccess);
            /* increment pAccess to point to the location holding register configuration data */
            (pAC97->RegAccess.pAccess)++;
            /* Is the codec set in Slot-16 mode? */
            if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
            {
                /* copy application data to control register data slot */
                *(pDacFrame+2) = *(pAC97->RegAccess.pAccess);
            }
            /* Codec set in Standard AC'97 mode */
            else
            {
                /* Split register data and store it to AC'97 slot as required */
                *(pDacFrame+2) = (*(pAC97->RegAccess.pAccess) >> 4);
                *(pDacFrame+3) &= 0x00FF;   /* clear Register Data portion in slot 3 */
                *(pDacFrame+3) |= (*(pAC97->RegAccess.pAccess) << 12);
            }
            /* update AC'97 Driver status registers */
            adi_ac97_UpdateStatus(pAC97, *(pDacFrame+1), *(pAC97->RegAccess.pAccess));
            /* increment pAccess to point to next register address/data pair */
            (pAC97->RegAccess.pAccess)++;
            /* Set TAG - mark command address and command data slots as valid */
            *pDacFrame |= (ADI_AC97_OUT_TAG_ADDR_VALID | ADI_AC97_OUT_TAG_DATA_VALID);

            /* Check for Register access complete */
            if ((*(pAC97->RegAccess.pAccess) == ADI_DEV_REGEND) ||
                (pAC97->RegAccess.Cmd == ADI_DEV_CMD_REGISTER_WRITE))
            {
                /* Mark register acccess as complete */
                pAC97->RegAccess.Mode  = ADI_AC97_CODEC_REG_ACCESS_COMPLETE;
            }
            break;

        /* CASE (Read a specific register) */
        case (ADI_DEV_CMD_REGISTER_READ):
        /* CASE (Read selected registers) */
        case(ADI_DEV_CMD_REGISTER_TABLE_READ):

            DataValid = 0;  /* assume that no valid register data is available in receive buffer */

            /* FOR all receive frames in the given ADC sub-buffer */
            for (i=0; i < pAC97->FramesPerSubBuf; i++, pAdcFrame+=16, pDacFrame+=16)
            {
                /* check for valid slot with vaild command address and data */
                /* pAccess now points to location holding register address to access */
                if (((*pAdcFrame & ADI_AC97_IN_TAG_DATA_VALID) == ADI_AC97_IN_TAG_DATA_VALID) &&
                    ((*(pAdcFrame + 1) & ADI_AC97_ADDR_INDEX_MASK) == *(pAC97->RegAccess.pAccess)))
                {
                    /* Extract the register value from AC'97 input (ADC) frame */
                    /* IF (codec set in Slot 16 mode) */
                    if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
                    {
                        /* get the command register data from slot 2 */
                        /* store the register data to the register access table provided by application */
                        /* increment pAccess to point location that should hold the read register value */
                        *(++pAC97->RegAccess.pAccess) = *(pAdcFrame+2);
                    }
                    /* ELSE (Codec set in Standard AC'97 mode) */
                    else
                    {
                        /* Combine the command register data from slot 2 & 3 */
                        /* store the register data to the register access table provided by application */
                        /* increment pAccess to point location that should hold the read register value */
                        *(++pAC97->RegAccess.pAccess) = ((*(pAdcFrame+2) << 4) | ((*(pAdcFrame+3) >> 12) & 0xF));
                    }

                    /* Check for Register access complete */
                    if ((*(++pAC97->RegAccess.pAccess) == ADI_DEV_REGEND) ||
                        (pAC97->RegAccess.Cmd == ADI_DEV_CMD_REGISTER_READ))
                    {
                        /* Mark register acccess as complete */
                        pAC97->RegAccess.Mode  = ADI_AC97_CODEC_REG_ACCESS_COMPLETE;
                        break;
                    }

                    DataValid = 1;  /* indicates that recieve buffer had a valid register data */
                }
                /* Load the register address to be read to AC'97 Command Address port slot*/
                *(pDacFrame+1) = (*(pAC97->RegAccess.pAccess) | ADI_AC97_OUT_SLOT1_READ_CODEC_REG);
                /* Set TAG - mark command address slot as valid */
                *pDacFrame |= ADI_AC97_OUT_TAG_ADDR_VALID;
                /* Check if the receive buffer had a valid register data */
                if (DataValid)
                {
                    break;  /* exit this loop */
                }
            }
            break;

        /* CASE (Read a specific field from a single register) */
        case(ADI_DEV_CMD_REGISTER_FIELD_READ):
        /* CASE (Read selected field(s) of the selected Register(s)) */
        case(ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ):
        /* CASE (Write to a specific field in a single register) */
        case(ADI_DEV_CMD_REGISTER_FIELD_WRITE):
        /* CASE (Write to selected field(s) of the selected Register(s)) */
        case(ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE):

            DataValid = 0;  /* assume that no valid register data is available in receive buffer */

            /* FOR all receive frames in the given ADC sub-buffer */
            for (i=0; i < pAC97->FramesPerSubBuf ; i++, pAdcFrame+=16, pDacFrame+=16)
            {
                /* check for valid slot with vaild command address and data */
                /* pAccess now points to location holding register address to access */
                if (((*pAdcFrame & ADI_AC97_IN_TAG_DATA_VALID) == ADI_AC97_IN_TAG_DATA_VALID) &&
                    ((*(pAdcFrame + 1) & ADI_AC97_ADDR_INDEX_MASK) == *(pAC97->RegAccess.pAccess)))
                {
                    /* Extract the register value from AC'97 input (ADC) frame */
                    /* IF (codec set in Slot 16 mode) */
                    if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
                    {
                        /* get the command register data from slot 2 */
                        RegData = *(pAdcFrame+2);
                    }
                    /* ELSE (Codec set in Standard AC'97 mode) */
                    else
                    {
                        /* Combine the command register data from slot 2 & 3 */
                        RegData = ((*(pAdcFrame+2) << 4) | ((*(pAdcFrame+3) >> 12) & 0xF));
                    }

                    ShiftCount=0;   /* initialise shiftcount */

                    /* use the register field mask to determine the
                       shift count required to reach first bit of this register field */
                    for (ShiftMask = *(pAC97->RegAccess.pAccess+1); ShiftMask; ShiftMask >>= 1)
                    {
                        /* check if we've reached the first bit location of this field */
                        if (ShiftMask & 1)
                        {
                            break;  /* reached first bit of this field. exit this loop */
                        }
                        ShiftCount++;   /* increment shiftcount */
                    }

                    /* IF (read register field) */
                    if (pAC97->RegAccess.Mode == ADI_AC97_CODEC_REG_ACCESS_READ)
                    {
                        /* strip out the register field data by masking the field we're interested in */
                        FieldData = (RegData & *(++pAC97->RegAccess.pAccess));
                        /* shift 'FieldData' to report the exact field value back to application */
                        FieldData >>= ShiftCount;
                        /* store the register field data to the register access table provided by application */
                        /* increment pAccess to point location that should hold the read register field value */
                        *(++pAC97->RegAccess.pAccess) = FieldData;
                        /* IF (End of register access table) */
                        if (*(++pAC97->RegAccess.pAccess) == ADI_DEV_REGEND)
                        {
                            /* Mark register acccess as complete */
                            pAC97->RegAccess.Mode  = ADI_AC97_CODEC_REG_ACCESS_COMPLETE;
                            break;
                        }
                    }
                    /* ELSE (update a register field) */
                    else
                    {
                        /* IF (Register write is in progress) */
                        if (pAC97->RegAccess.Mode == ADI_AC97_CODEC_REG_ACCESS_WRITE_IN_PROGRESS)
                        {
                            /* IF (Register configuration complete) */
                            if (RegData == pAC97->RegAccess.tRegData)
                            {
                                /* move to next register in the list */
                                pAC97->RegAccess.pAccess += 3;
                                /* Configure this register when we have a valid register data */
                                pAC97->RegAccess.Mode = ADI_AC97_CODEC_REG_ACCESS_WRITE;
                                break;
                            }
                        }
                        else
                        {
                            /* Load AC'97 Command Address port slot with the register address to update */
                            *(pDacFrame+1) = *(pAC97->RegAccess.pAccess);
                            /* clear the register field that we're about to update */
                            /* increment pAccess to point location holding register field mask */
                            RegData &= ~(*(++pAC97->RegAccess.pAccess));
                            /* get the new field data from register access table */
                            FieldData = *(++pAC97->RegAccess.pAccess);
                            /* shift the 'FieldData' until we reach its corresponding field location */
                            FieldData <<= ShiftCount;
                            /* update the register field with new value */
                            RegData |= FieldData;
                            /* save the register data in temp location */
                            pAC97->RegAccess.tRegData = RegData;

                            /* IF (AC'97 codec set in Slot-16 mode) */
                            if (pAC97->DriverStatus2 & ADI_AC97_FLAG_SLOT16_ENABLED)
                            {
                                /* copy the updated register value to control register data slot */
                                *(pDacFrame+2) = RegData;
                            }
                            /* ELSE (Codec set in Standard AC'97 mode) */
                            else
                            {
                                /* copy the updated register value to control register data slot */
                                /* Split register data and store it to AC'97 slot as required */
                                *(pDacFrame+2) = (RegData >> 4);
                                *(pDacFrame+3) &= 0x00FF;   /* clear Register Data portion in slot 3 */
                                *(pDacFrame+3) |= (RegData << 12);
                            }
                            /* update AC'97 driver status registers */
                            adi_ac97_UpdateStatus(pAC97, *(pDacFrame+1), RegData);
                            /* Set TAG - mark command address and command data slots as valid */
                            *pDacFrame |= (ADI_AC97_OUT_TAG_ADDR_VALID | ADI_AC97_OUT_TAG_DATA_VALID);
                            /* IF (End of register access table) */
                            if (*(++pAC97->RegAccess.pAccess) == ADI_DEV_REGEND)
                            {
                                /* Mark register acccess as complete */
                                pAC97->RegAccess.Mode  = ADI_AC97_CODEC_REG_ACCESS_COMPLETE;
                            }
                            else
                            {
                                /* Update register access mode as Register write in progress */
                                pAC97->RegAccess.Mode = ADI_AC97_CODEC_REG_ACCESS_WRITE_IN_PROGRESS;
                                /* reset Register access pointer to point the location
                                   holding register address being configured */
                                pAC97->RegAccess.pAccess -=3;
                            }
                            break;
                        }
                    }
                }
                /* Load the register address to be read to AC'97 Command Address port slot*/
                *(pDacFrame+1) = (*(pAC97->RegAccess.pAccess) | ADI_AC97_OUT_SLOT1_READ_CODEC_REG);
                /* Set TAG - mark command address slot as valid */
                *pDacFrame |= ADI_AC97_OUT_TAG_ADDR_VALID;
            }
            break;

        default:
            break;
    }

    /* return */
}

/*********************************************************************

    Function:       adi_ac97_UpdateStatus

    Description:    Updates 'DriverStatus' register in the given
                   AC'97 driver instance

*********************************************************************/
static void adi_ac97_UpdateStatus(
    ADI_AC97_DRIVER_INSTANCE    *pAC97,     /* AC'97 driver instance we're working on       */
    u16                         RegAddr,    /* Audio Codec Register address to be updated   */
    u16                         RegData     /* Register data to be updated with             */
){
    u16 Mask;

    /* CASEOF (Register Address) */
    switch(RegAddr)
    {
        /* Extended Audio status/control register */
        case (AC97_REG_EXTD_AUDIO_CTRL):
            /* IF (update VRA bit) */
            if ((RegData & ADI_AC97_FLAG_VRA_ENABLED) != (pAC97->DriverStatus1 & ADI_AC97_FLAG_VRA_ENABLED))
            {
                /* Update DAC data slot structure to handle selected sample rate */
                pAC97->DacAppBuf.pSlotOutRatio      = &(pAC97->DacAppBuf.SlotRatio[0]);
                pAC97->DacAppBuf.SlotRepeatCount    = (pAC97->DacAppBuf).pSlotOutRatio->RepeatSlotOut;
                pAC97->DacAppBuf.pSlotSequence      = &(pAC97->DacAppBuf.SlotSequence[0]);
                pAC97->DacAppBuf.SlotSequenceCount  = (pAC97->DacAppBuf).pSlotSequence->RepeatSlotSequence;
                pAC97->DacAppBuf.ValidSlotsRemain   = (pAC97->DacAppBuf).pSlotOutRatio->ValidSlots;
                pAC97->DacAppBuf.InValidSlotsRemain = (pAC97->DacAppBuf).pSlotOutRatio->InvalidSlots;
            }
            /* mask the fields we're intreseted in */
            RegData &= ADI_AC97_MASK_EXTD_AUDIO_CTRL;
            /* update Driver status register 1 */
            pAC97->DriverStatus1 &= ~ADI_AC97_MASK_EXTD_AUDIO_CTRL;
            pAC97->DriverStatus1 |= RegData;

            /* update DAC Enabled/Disabled flag */
            /* IF (all DACs power down OR (CTR,LFE,Line out disabled and Surroud power down)) */
            if ((pAC97->DriverStatus2 & ADI_AC97_FLAG_ALL_DAC_PDN) ||
                ((pAC97->DriverStatus2 & ADI_AC97_FLAG_LINE_OUT_CTR_LFE_DISABLED) &&
                 (pAC97->DriverStatus1 & ADI_AC97_FLAG_LFE_SURR_CTR_PDN)))
            {
                /* mark as DAC channels disabled */
                pAC97->DriverStatus2 &= ~ADI_AC97_FLAG_DAC_ENABLED;
            }
            /* ELSE (one or more DAC channel(s) enabled) */
            else
            {
                /* mark as one or more DAC channel(s) enabled */
                pAC97->DriverStatus2 |= ADI_AC97_FLAG_DAC_ENABLED;
            }
            break;

        /* Serial Port Configuration register */
        case (AC97_REG_SERIAL_CONFIG):
        /* Power Control/Status register */
        case (AC97_REG_POWER_CTRL_STAT):
        /* Misc Control register 1 */
        case (AC97_REG_MISC_CTRL_1):

            /* Get the Mask value corresponding to the register address */
            /* IF (Serial port configuration register) */
            if (RegAddr == AC97_REG_SERIAL_CONFIG)
            {
                Mask = ADI_AC97_MASK_SERIAL_PORT_CONFIG;
            }
            /* ELSE IF (Power Control/Status register) */
            else if (RegAddr == AC97_REG_POWER_CTRL_STAT)
            {
                Mask = ADI_AC97_MASK_POWER_CTRL_STATUS;
            }
            /* ELSE (Misc Control register 1) */
            else
            {
                Mask = ADI_AC97_MASK_MISC_CTRL_1;
            }

            /* mask the fields we're intreseted in */
            RegData &= Mask;
            /* update Driver status register 2 */
            pAC97->DriverStatus2 &= ~Mask;
            pAC97->DriverStatus2 |= RegData;

            /* update DAC Enabled/Disabled flag */
            /* IF (all DACs power down OR (CTR,LFE,Line out disabled and Surroud power down)) */
            if ((pAC97->DriverStatus2 & ADI_AC97_FLAG_ALL_DAC_PDN) ||
                ((pAC97->DriverStatus2 & ADI_AC97_FLAG_LINE_OUT_CTR_LFE_DISABLED) &&
                 (pAC97->DriverStatus1 & ADI_AC97_FLAG_LFE_SURR_CTR_PDN)))
            {
                /* mark as DAC channels disabled */
                pAC97->DriverStatus2 &= ~ADI_AC97_FLAG_DAC_ENABLED;
            }
            /* ELSE (one or more DAC channel(s) enabled) */
            else
            {
                /* mark as one or more DAC channel(s) enabled */
                pAC97->DriverStatus2 |= ADI_AC97_FLAG_DAC_ENABLED;
            }

            break;

        /* DAC Sampling Rate Register */
        case (AC97_REG_FRONT_DAC_RATE):
            /* CASEOF (DAC Sampling rate) */
            switch (RegData)
            {
                /* CASE: 48kHz */
                case (ADI_AC97_SR_48000):
                /* CASE: 32kHz */
                case (ADI_AC97_SR_32000):
                /* CASE: 24kHz */
                case (ADI_AC97_SR_24000):
                /* CASE: 16kHz */
                case (ADI_AC97_SR_16000):
                /* CASE: 12kHz */
                case (ADI_AC97_SR_12000):
                /* CASE: 8kHz */
                case (ADI_AC97_SR_8000):
                    /* IF (DAC Sample rate is 48kHz) */
                    if (RegData == ADI_AC97_SR_48000)
                    {
                        /*** Slot out ratio (valid : invalid) = 1:0 (all slots are valid) ***/
                        pAC97->DacAppBuf.SlotRatio[0].ValidSlots        = 48000;
                        pAC97->DacAppBuf.SlotRatio[0].InvalidSlots      = 0;
                    }
                    /* ELSE IF (DAC Sample rate is 32kHz) */
                    else if (RegData == ADI_AC97_SR_32000)
                    {
                        /*** Slot out ratio (valid : invalid) = 2:1 ***/
                        pAC97->DacAppBuf.SlotRatio[0].ValidSlots        = 2;
                        pAC97->DacAppBuf.SlotRatio[0].InvalidSlots      = 1;
                    }
                    /* ELSE IF (DAC Sample rate is 24kHz) */
                    else if (RegData == ADI_AC97_SR_24000)
                    {
                        /*** Slot out ratio (valid : invalid) = 1:1 ***/
                        pAC97->DacAppBuf.SlotRatio[0].ValidSlots        = 1;
                        pAC97->DacAppBuf.SlotRatio[0].InvalidSlots      = 1;
                    }
                    /* ELSE IF (DAC Sample rate is 16kHz) */
                    else if (RegData == ADI_AC97_SR_16000)
                    {
                        /*** Slot out ratio (valid : invalid) = 1:2 ***/
                        pAC97->DacAppBuf.SlotRatio[0].ValidSlots        = 1;
                        pAC97->DacAppBuf.SlotRatio[0].InvalidSlots      = 2;
                    }
                    /* ELSE IF (DAC Sample rate is 12kHz) */
                    else if (RegData == ADI_AC97_SR_12000)
                    {
                        /*** Slot out ratio (valid : invalid) = 1:3 ***/
                        pAC97->DacAppBuf.SlotRatio[0].ValidSlots        = 1;
                        pAC97->DacAppBuf.SlotRatio[0].InvalidSlots      = 3;
                    }
                    /* ELSE (DAC Sample rate must be 8kHz) */
                    else
                    {
                        /*** Slot out ratio (valid : invalid) = 1:5 ***/
                        pAC97->DacAppBuf.SlotRatio[0].ValidSlots        = 1;
                        pAC97->DacAppBuf.SlotRatio[0].InvalidSlots      = 5;
                    }

                    /* repeat this as long as possible */
                    pAC97->DacAppBuf.SlotRatio[0].RepeatSlotOut         = 0xFFFFFFFF;
                    /* repeat the same slot out ratio */
                    pAC97->DacAppBuf.SlotRatio[0].pNextSlotRatio        = &(pAC97->DacAppBuf.SlotRatio[0]);

                    /*** Slot out sequence ***/
                    pAC97->DacAppBuf.SlotSequence[0].RepeatSlotSequence = 1;
                    pAC97->DacAppBuf.SlotSequence[0].pSlotRatio         = &(pAC97->DacAppBuf.SlotRatio[0]);
                    /* repeat the same slot sequence */
                    pAC97->DacAppBuf.SlotSequence[0].pNextSequence      = &(pAC97->DacAppBuf.SlotSequence[0]);
                    break;

                /* CASE: 44.1kHz */
                case (ADI_AC97_SR_44100):
                    /*** Slot out ratio 0 (valid : invalid) = 11:1 ***/
                    pAC97->DacAppBuf.SlotRatio[0].RepeatSlotOut         = 2;
                    pAC97->DacAppBuf.SlotRatio[0].ValidSlots            = 11;
                    pAC97->DacAppBuf.SlotRatio[0].InvalidSlots          = 1;
                    /* Slot out Ratio 0 & Ratio 1 must be carried out in sequence */
                    pAC97->DacAppBuf.SlotRatio[0].pNextSlotRatio        = &(pAC97->DacAppBuf.SlotRatio[1]);

                    /*** Slot out ratio 1 (valid : invalid) = 12:1 ***/
                    pAC97->DacAppBuf.SlotRatio[1].RepeatSlotOut         = 1;
                    pAC97->DacAppBuf.SlotRatio[1].ValidSlots            = 12;
                    pAC97->DacAppBuf.SlotRatio[1].InvalidSlots          = 1;
                    /* terminate slot out ratio list */
                    pAC97->DacAppBuf.SlotRatio[1].pNextSlotRatio        = NULL;

                    /*** Slot out sequence 0 - Slot out ratio 0 & ratio 1 ***/
                    pAC97->DacAppBuf.SlotSequence[0].RepeatSlotSequence = 4;
                    pAC97->DacAppBuf.SlotSequence[0].pSlotRatio         = &(pAC97->DacAppBuf.SlotRatio[0]);
                    pAC97->DacAppBuf.SlotSequence[0].pNextSequence      = &(pAC97->DacAppBuf.SlotSequence[1]);

                    /*** Slot out ratio 2 (valid : invalid) = 11:1 ***/
                    pAC97->DacAppBuf.SlotRatio[2].RepeatSlotOut         = 1;
                    pAC97->DacAppBuf.SlotRatio[2].ValidSlots            = 11;
                    pAC97->DacAppBuf.SlotRatio[2].InvalidSlots          = 1;
                    /* terminate slot out ratio list */
                    pAC97->DacAppBuf.SlotRatio[2].pNextSlotRatio        = NULL;

                    /*** Slot out sequence 1 - Slot out ratio 2 ***/
                    pAC97->DacAppBuf.SlotSequence[1].RepeatSlotSequence = 1;
                    pAC97->DacAppBuf.SlotSequence[1].pSlotRatio         = &(pAC97->DacAppBuf.SlotRatio[2]);
                    /* loopback to first slot out sequence in the list */
                    pAC97->DacAppBuf.SlotSequence[1].pNextSequence      = &(pAC97->DacAppBuf.SlotSequence[0]);
                    break;

                /* CASE: 22.05kHz */
                case (ADI_AC97_SR_22050):
                    /*** Slot out ratio 0 (valid : invalid) = 1:1 ***/
                    pAC97->DacAppBuf.SlotRatio[0].RepeatSlotOut         = 5;
                    pAC97->DacAppBuf.SlotRatio[0].ValidSlots            = 1;
                    pAC97->DacAppBuf.SlotRatio[0].InvalidSlots          = 1;
                    /* Slot out Ratio 0 & Ratio 1 must occur in sequence */
                    pAC97->DacAppBuf.SlotRatio[0].pNextSlotRatio        = &(pAC97->DacAppBuf.SlotRatio[1]);

                    /*** Slot out ratio 1 (valid : invalid) = 1:2 ***/
                    pAC97->DacAppBuf.SlotRatio[1].RepeatSlotOut         = 1;
                    pAC97->DacAppBuf.SlotRatio[1].ValidSlots            = 1;
                    pAC97->DacAppBuf.SlotRatio[1].InvalidSlots          = 2;
                    /* terminate slot out ratio list */
                    pAC97->DacAppBuf.SlotRatio[1].pNextSlotRatio        = NULL;

                    /*** Slot out sequence 0 - Slot out ratio 0 & ratio 1 ***/
                    pAC97->DacAppBuf.SlotSequence[0].RepeatSlotSequence = 26;
                    pAC97->DacAppBuf.SlotSequence[0].pSlotRatio         = &(pAC97->DacAppBuf.SlotRatio[0]);
                    pAC97->DacAppBuf.SlotSequence[0].pNextSequence      = &(pAC97->DacAppBuf.SlotSequence[1]);

                    /*** Slot out ratio 2 (valid : invalid) = 1:1 ***/
                    pAC97->DacAppBuf.SlotRatio[2].RepeatSlotOut         = 1;
                    pAC97->DacAppBuf.SlotRatio[2].ValidSlots            = 1;
                    pAC97->DacAppBuf.SlotRatio[2].InvalidSlots          = 1;
                    /* terminate slot out ratio list */
                    pAC97->DacAppBuf.SlotRatio[2].pNextSlotRatio        = &(pAC97->DacAppBuf.SlotRatio[3]);

                    /*** Slot out ratio 3 (valid : invalid) = 1:2 ***/
                    pAC97->DacAppBuf.SlotRatio[3].RepeatSlotOut         = 2;
                    pAC97->DacAppBuf.SlotRatio[3].ValidSlots            = 1;
                    pAC97->DacAppBuf.SlotRatio[3].InvalidSlots          = 2;
                    /* terminate slot out ratio list */
                    pAC97->DacAppBuf.SlotRatio[3].pNextSlotRatio        = NULL;

                    /*** Slot out sequence 1 - Slot out ratio 2 ***/
                    pAC97->DacAppBuf.SlotSequence[1].RepeatSlotSequence = 1;
                    pAC97->DacAppBuf.SlotSequence[1].pSlotRatio         = &(pAC97->DacAppBuf.SlotRatio[2]);
                    /* loopback to first slot out sequence in the list */
                    pAC97->DacAppBuf.SlotSequence[1].pNextSequence      = &(pAC97->DacAppBuf.SlotSequence[0]);
                    break;

                /* CASE: 11.025kHz */
                case (ADI_AC97_SR_11025):
                    /*** Slot out ratio 0 (valid : invalid) = 1:3 ***/
                    pAC97->DacAppBuf.SlotRatio[0].RepeatSlotOut         = 2;
                    pAC97->DacAppBuf.SlotRatio[0].ValidSlots            = 1;
                    pAC97->DacAppBuf.SlotRatio[0].InvalidSlots          = 3;
                    /* Slot out Ratio 0 & Ratio 1 must occur in sequence */
                    pAC97->DacAppBuf.SlotRatio[0].pNextSlotRatio        = &(pAC97->DacAppBuf.SlotRatio[1]);

                    /*** Slot out ratio 1 (valid : invalid) = 1:4 ***/
                    pAC97->DacAppBuf.SlotRatio[1].RepeatSlotOut         = 1;
                    pAC97->DacAppBuf.SlotRatio[1].ValidSlots            = 1;
                    pAC97->DacAppBuf.SlotRatio[1].InvalidSlots          = 4;
                    /* terminate slot out ratio list */
                    pAC97->DacAppBuf.SlotRatio[1].pNextSlotRatio        = NULL;

                    /*** Slot out sequence 0 - Slot out ratio 0 & ratio 1 ***/
                    pAC97->DacAppBuf.SlotSequence[0].RepeatSlotSequence = 47;
                    pAC97->DacAppBuf.SlotSequence[0].pSlotRatio         = &(pAC97->DacAppBuf.SlotRatio[0]);
                    pAC97->DacAppBuf.SlotSequence[0].pNextSequence      = &(pAC97->DacAppBuf.SlotSequence[1]);

                    /*** Slot out ratio 2 (valid : invalid) = 1:3 ***/
                    pAC97->DacAppBuf.SlotRatio[2].RepeatSlotOut         = 1;
                    pAC97->DacAppBuf.SlotRatio[2].ValidSlots            = 1;
                    pAC97->DacAppBuf.SlotRatio[2].InvalidSlots          = 3;
                    /* terminate slot out ratio list */
                    pAC97->DacAppBuf.SlotRatio[2].pNextSlotRatio        = &(pAC97->DacAppBuf.SlotRatio[3]);

                    /*** Slot out ratio 3 (valid : invalid) = 1:4 ***/
                    pAC97->DacAppBuf.SlotRatio[3].RepeatSlotOut         = 5;
                    pAC97->DacAppBuf.SlotRatio[3].ValidSlots            = 1;
                    pAC97->DacAppBuf.SlotRatio[3].InvalidSlots          = 4;
                    /* terminate slot out ratio list */
                    pAC97->DacAppBuf.SlotRatio[3].pNextSlotRatio        = NULL;

                    /*** Slot out sequence 1 - Slot out ratio 2 ***/
                    pAC97->DacAppBuf.SlotSequence[1].RepeatSlotSequence = 1;
                    pAC97->DacAppBuf.SlotSequence[1].pSlotRatio         = &(pAC97->DacAppBuf.SlotRatio[2]);
                    /* loopback to first slot out sequence in the list */
                    pAC97->DacAppBuf.SlotSequence[1].pNextSequence      = &(pAC97->DacAppBuf.SlotSequence[0]);
                    break;

                    /* other sample rates
                       this should never happen, as the audio codec driver using this library
                       must validate DAC sample rate before passing it to us */
                    default:
                        break;
            }
            /* Update DAC data slot structure to handle selected sample rate */
            pAC97->DacAppBuf.pSlotOutRatio      = &(pAC97->DacAppBuf.SlotRatio[0]);
            pAC97->DacAppBuf.SlotRepeatCount    = (pAC97->DacAppBuf).pSlotOutRatio->RepeatSlotOut;
            pAC97->DacAppBuf.pSlotSequence      = &(pAC97->DacAppBuf.SlotSequence[0]);
            pAC97->DacAppBuf.SlotSequenceCount  = (pAC97->DacAppBuf).pSlotSequence->RepeatSlotSequence;
            pAC97->DacAppBuf.ValidSlotsRemain   = (pAC97->DacAppBuf).pSlotOutRatio->ValidSlots;
            pAC97->DacAppBuf.InValidSlotsRemain = (pAC97->DacAppBuf).pSlotOutRatio->InvalidSlots;
            break;

        /* other registers */
        default:
            break;
    }

    /* return */
}

/*********************************************************************

    Function:       adi_ac97_ValidateRegVal

    Description:    Checks whether the given codec register
                    configuration value is valid

*********************************************************************/
u32 adi_ac97_ValidateRegVal (
    ADI_AC97_DRIVER_INSTANCE    *pAC97,         /* Pointer to AC'97 Driver Instance             */
    u16                         RegAddr,        /* AC'97 Codec register address to be validated */
    u16                         RegVal          /* Register value                               */
){
    /* Return value - assume that the register value is valid   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    volatile u8 i;

    /* list of DAC sampling rates supported by the driver */
    const u16 SupportedSR[] =
    {
        48000,  /* 48kHz        */
        44100,  /* 44.1kHz      */
        32000,  /* 32kHz        */
        24000,  /* 24kHz        */
        22050,  /* 22.05kHz     */
        16000,  /* 16kHz        */
        12000,  /* 12kHz        */
        11025,  /* 11.025kHz    */
         8000   /* 8kHz         */
    };

    /* CASE OF (RegAddr) */
    switch(RegAddr)
    {
        /* CASE (ADC Sampling rate register) */
        case (AC97_REG_ADC_RATE):
            /* IF (sampling rate is between 7kHz and 48kHz) */
            if ((RegVal < 7000) || (RegVal > 48000))
            {
                /* No, we don't support this sampling rate */
                Result = ADI_AC97_RESULT_ADC_SAMPLE_RATE_NOT_SUPPORTED;
            }
            break;

        /* CASE (Front DAC Sampling rate register) */
        case (AC97_REG_FRONT_DAC_RATE):
        /* CASE (Surround DAC Sampling rate register) */
        case (AC97_REG_SURR_DAC_RATE):
        /* CASE (CTR/LFE DAC Sampling rate register) */
        case (AC97_REG_CTR_LFE_DAC_RATE):

                /* assume that the given DAC sampling rate is not supported by us */
                Result = ADI_AC97_RESULT_DAC_SAMPLE_RATE_NOT_SUPPORTED;
                /* Validate the given sampling rate */
                for (i = 0;i<(sizeof(SupportedSR)/sizeof(SupportedSR[0]));i++)
                {
                    /* IF(we do support this sampling rate) */
                    if (RegVal == SupportedSR[i])
                    {
                        Result = ADI_DEV_RESULT_SUCCESS;
                        break;
                    }
                }
                break;

            /* other registers */
            default:
                break;
    }

    /* return */
    return (Result);
}

/*****/
