/*
 * module_spo2_uart.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 *  Note: communicate with control board by UART0
 */
#include <ccblkfn.h>
#include <string.h>

#include "module_spo2_uart.h"


//#define SpO2_DEBUG_INFO

#ifdef SpO2_DEBUG_INFO
#define SpO2_DBG_INFO(fmt, arg...) AIO_printf("\r\n[SpO2] "fmt, ##arg)
#else
#define SpO2_DBG_INFO(fmt, arg...)
#endif


#define UART0_BAUD_RATE 115200  

//-QWB-131108 Bug:此时间影响ECG结果
#define SpO2_RX_PERIOD_MS   4   //进行一次处理的间隔
#define SpO2_RX_TRY_COUNT   1000 //串口接收尝试的次数

#define SpO2_UART_COM_RxBUFSIZE  1024   //must less than SDRAM page size(1K Bytes)
#define SpO2_UART_COM_TxBUFSIZE  1024   //must less than SDRAM page size(1K Bytes)

#pragma section("sdram0_bank3_page0")
char chSpO2RxBuf[SpO2_UART_COM_RxBUFSIZE];
#pragma section("sdram0_bank3_page1")
char chSpO2TxBuf[SpO2_UART_COM_TxBUFSIZE];

static unsigned int u32SpO2RxPopIndex;
static unsigned int u32SpO2RxPushIndex;
static unsigned int uiSpO2TxIdxStart;
static unsigned int uiSpO2TxIdxEnd;
bool bSpO2IsOverBufLenght;
unsigned char g_spo2_mask_count;
bool g_IsSpO2_Upload;
unsigned char g_is_aio_spo2_error;
unsigned long g_spo2_test_tick;

void SpO2_to_AIO_UART(UartProtocolPacket *pPacket);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void SpO2_Module_Init(void);
#pragma section("sdram0_bank3")
void UART0_Init(void);
#pragma section("sdram0_bank3")
void Init_UART0_Interrupts(void);
#pragma section("sdram0_bank3")
void SpO2_UART_SetInterrupt(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

static void UART0_Init(void)
{
    short UartDivisor = -1;
    
    *pPORTG_FER |= 0x0600;  //PG9[Tx] PG10[Rx] enable peripheral function
    ssync();
    *pPORTG_MUX |= 0x400;   //bit[11:10]=b#01,configure UART0 RX and UART0 TX pins
    ssync();

    UartDivisor = UartSetBitrate(0, UART0_BAUD_RATE);
    if (UartDivisor < 0)
    {
        //configure error happen.
    }
}

/**
  * @brief  This function initializes the UART0 interrupt.
  *         IVG10, mask Tx and Rx Interrupt.
  */
static void Init_UART0_Interrupts(void)
{
    //Step1:Enable the SIC(SIC_IMASK) interrupt
    unsigned long imask0 = *pSIC_IMASK0;
    imask0 |= (1 << 22);    //Rx interrupt mask
    imask0 |= (1 << 23);    //Tx interrupt mask
    *pSIC_IMASK0 = imask0;


    //Step2(Optionanl):Program the interrupt priority (SIC_IARx)
    /* Setup for  IVG11 : 4 */
    unsigned long iar2 = *pSIC_IAR2;
    iar2  &= 0x00FFFFFF;
    iar2  |= 0x44000000;    //Tx[31:28] Rx[27:24] interrupt
    *pSIC_IAR2 = iar2;

    //Step3:Set IVGx bit in the IMASK
    register_handler(ik_ivg11, IVG11_SpO2_UART_ISR);
}

void SpO2_UART_SetInterrupt(void)
{
    //ERBFI  0x01   Enable Receive Buffer Full Interrupt
    //ETBEI  0x02   Enable Transmit Buffer Empty Interrupt
    //ELSI   0x04   Enable RX Status Interrupt         
    *pUART0_IER = (ELSI|ERBFI);
}

/******************************************************************************/
/*  UART0 Receive API                                                          */
/******************************************************************************/
//@return   :int, lenght of vaild data
static int SpO2RxGetDataLen(void)
{
    int s32Len;
    s32Len = u32SpO2RxPushIndex - u32SpO2RxPopIndex;
    if(s32Len < 0)  s32Len += SpO2_UART_COM_RxBUFSIZE;
    return s32Len;
}

//@return   :>0, lenght of vaild data
//           =0, no data 
static int SpO2_UART_GetByte(char* pValue)
{
    int s32Len = SpO2RxGetDataLen();

    if(s32Len)
    {
        *pValue = chSpO2RxBuf[u32SpO2RxPopIndex];
        u32SpO2RxPopIndex = (u32SpO2RxPopIndex + 1) % SpO2_UART_COM_RxBUFSIZE;
    }
    
    return s32Len;
}

/******************************************************************************/
/*  UART0 Send API                                                             */
/******************************************************************************/
static int IsSpO2TxBufFull(const unsigned int uiIdxEnd)
{
    if(((uiSpO2TxIdxStart == 0) && (uiIdxEnd == (SpO2_UART_COM_TxBUFSIZE - 1)))
        || (uiSpO2TxIdxStart == (uiIdxEnd + 1)))
        return 1;   // full
    return 0;       // not full
}

void SpO2_UART_SendByte(const char chByte)
{
    while(IsSpO2TxBufFull(uiSpO2TxIdxEnd))
    {
        *pUART0_IER |= ETBEI;       // Enable UART_COM Transmit interrupts
        //Delay1ms(1);
    }
    chSpO2TxBuf[uiSpO2TxIdxEnd] = chByte;
    if(uiSpO2TxIdxEnd < (SpO2_UART_COM_TxBUFSIZE - 1))  uiSpO2TxIdxEnd++;
    else    uiSpO2TxIdxEnd = 0;     // Circular buffer
    *pUART0_IER |= ETBEI;       // Enable UART_COM Transmit interrupts
}

void SpO2_UART_SendBuf(char* p_chBuf, const unsigned int uiLength)
{
    unsigned int uiIdxTemp, uiLen;

    uiIdxTemp = uiSpO2TxIdxEnd;
    uiLen = 0;
    while(uiLen < uiLength)
    {
        while(IsSpO2TxBufFull(uiIdxTemp))
        {
            *pUART0_IER |= ETBEI;       // Enable UART_COM Transmit interrupts
            //Delay1ms(1);
        }
        chSpO2TxBuf[uiIdxTemp] = p_chBuf[uiLen++];
        uiIdxTemp++;
        if(uiIdxTemp >= SpO2_UART_COM_TxBUFSIZE)    uiIdxTemp = 0; // Circular buffer
    }

    uiSpO2TxIdxEnd = uiIdxTemp;
    *pUART0_IER |= ETBEI; // Enable UART_COM Transmit interrupts
}

/*******************************************************************
*   Function:    EX_INTERRUPT_HANDLER(IVG11_SpO2_UART_ISR)
*   Description: UART0_ISR
*******************************************************************/
EX_INTERRUPT_HANDLER(IVG11_SpO2_UART_ISR)
{	
    volatile int uartLsr;
    //unsigned int uiTIMASK = cli();
    /************ >>>>>>>> Handler for UART0 Start <<<<<<<< *******************/
    uartLsr = *pUART0_LSR;  
    if (uartLsr & DR)//UART0 Receive ready, get data from UART0_RBR
    {
        chSpO2RxBuf[u32SpO2RxPushIndex] = *pUART0_RBR;   // Read one byte from the receive data register

        u32SpO2RxPushIndex = (u32SpO2RxPushIndex + 1) % SpO2_UART_COM_RxBUFSIZE;
        if(u32SpO2RxPopIndex == u32SpO2RxPushIndex)    // overflow
        {
            u32SpO2RxPopIndex = (u32SpO2RxPopIndex + 1) % SpO2_UART_COM_RxBUFSIZE;
        }       
    }
    else if (uartLsr & THRE)//UART0 Send empty, fill data into UART0_THR to send
    {
        if(uiSpO2TxIdxStart == uiSpO2TxIdxEnd)    // Send buffer is empty
        {
            *pUART0_IER &= ~ETBEI;      // Disable TXE Interrupt when not data to be send
        }
        else
        {
            *pUART0_THR = chSpO2TxBuf[uiSpO2TxIdxStart];   // Write one byte to the transmit data register
            uiSpO2TxIdxStart++;
            if(uiSpO2TxIdxStart >= SpO2_UART_COM_TxBUFSIZE) uiSpO2TxIdxStart = 0;  // Circular buffer
        }
    }
    /************ >>>>>>>> Handler for UART0 Stop <<<<<<<< *******************/
    //sti(uiTIMASK);
}

static void SpO2_RxPacketCheck(void)
{
    UartProtocolPacket packet;
    u8 crc = 0;
    u8 i = 0;
    u16 tryTime = 0;
    int nLen;
    
    memset(&packet, 0, sizeof(UartProtocolPacket));

    if (SpO2_UART_GetByte((char *)&packet.DR_Addr))
    {
        if (UART_AIO_ADDR == packet.DR_Addr) //get the head of one packet
        {
            for (tryTime=0; (!SpO2_UART_GetByte((char *)&packet.SR_Addr)) && (tryTime < SpO2_RX_TRY_COUNT); tryTime++);
            if (UART_SpO2_ADDR == packet.SR_Addr)//来自血氧板的数据
            {
                for (tryTime=0; (!SpO2_UART_GetByte((char *)&packet.PacketNum)) && (tryTime < SpO2_RX_TRY_COUNT); tryTime++);
                for (tryTime=0; (!SpO2_UART_GetByte((char *)&packet.PacketID)) && (tryTime < SpO2_RX_TRY_COUNT); tryTime++);
                for (tryTime=0; (!SpO2_UART_GetByte((char *)&packet.Length)) && (tryTime < SpO2_RX_TRY_COUNT); tryTime++);

                for (i=0; i <= packet.Length; i++)  //get data and crc value
                {
                    for (tryTime=0; (!SpO2_UART_GetByte((char *)&packet.DataAndCRC[i])) && (tryTime < SpO2_RX_TRY_COUNT); tryTime++);
                }

                //check CRC vlaue
                crc = CalculatePacketCRC8(&packet);
                if (packet.DataAndCRC[packet.Length] == crc)
                {
                    SpO2_to_AIO_UART(&packet);
                    SpO2_DBG_INFO("I have get one packet!");
                }
                else
                {
                    SpO2_DBG_INFO("I have get one wrong CRC! read crc=0x%x cal crc=0x%x",
                                        packet.DataAndCRC[packet.Length], crc);
                }
            } /*End of if packet.SR_Addr*/
        } /*End of if packet.DR_Addr*/
    } /* End of BUFFER_NORMAL */
}

static void SpO2_to_AIO_UART(UartProtocolPacket *pPacket)
{
#if 0//时标无法同步
    if (SpO2_TX_SPO2_REALTIME_ID == (UART_PacketID)pPacket->PacketID)//实时数据包需要替换Systick信息
    {
        //u32 curTick = SysTick_Get();
        u32 curTick = SysTick_Get()/10;
        pPacket->DataAndCRC[13] = (curTick>>24)&0xFF;   // Tick MSB
        pPacket->DataAndCRC[14] = (curTick>>16)&0xFF;
        pPacket->DataAndCRC[15] = (curTick>>8)&0xFF;
        pPacket->DataAndCRC[16] = curTick&0xFF;         //Tick LSB
    }
    else if (SpO2_TX_SPO2_NORMALIZED_ID == (UART_PacketID)pPacket->PacketID)//替换Systick信息
    {
        //u32 curTick = SysTick_Get();
        u32 curTick = SysTick_Get()/10;
        pPacket->DataAndCRC[2] = (curTick>>24)&0xFF;   // Tick MSB
        pPacket->DataAndCRC[3] = (curTick>>16)&0xFF;
        pPacket->DataAndCRC[4] = (curTick>>8)&0xFF;
        pPacket->DataAndCRC[5] = curTick&0xFF;         //Tick LSB
    }
#endif
    if (true == g_IsSpO2_Upload)
    {
#ifdef _UPLOAD_SpO2_ADC_
        UploadDataByID((UART_PacketID)pPacket->PacketID,
                        TRUE,
                        (char *)(pPacket->DataAndCRC),
                        pPacket->Length);
#endif
    }
    //AIO_UART_SendBuf((char *)pPacket, pPacket->Length + PACKET_FIXED_LENGHT);
}

static void SpO2_RxHandler(void)
{
    while(SpO2RxGetDataLen() > 20)
    {
        g_spo2_test_tick = SysTick_Get();
        g_is_aio_spo2_error = 0;
        SpO2_RxPacketCheck();
    }
    if (!g_is_aio_spo2_error)
    {
        if (getMSByTickcount(SysTick_Get()-g_spo2_test_tick) > 10000)//10s
        {
            g_is_aio_spo2_error = 1;
        }
    }
}

static void SpO2_ResetMaskAIOTick(void)
{
    static unsigned long ulNextChangeTime = 0;
    
    if(IsOnTime(ulNextChangeTime)) // is on time or over time
    {
        if (0 == g_spo2_mask_count)
        {
            g_IsECG_Upload = false;
            g_IsRESP_Upload = false;
            g_IsTemp_Upload = false;
            g_IsCommon_Upload = false;
            g_IsNIBP_Upload = false;
        }
        if (UPDATE_MASK_AIO_TIME_S == g_spo2_mask_count)
        {
            g_IsECG_Upload = true;
            g_IsRESP_Upload = true;
            g_IsTemp_Upload = true;
            g_IsCommon_Upload = true;
            g_IsNIBP_Upload = true;
        }
        if (g_spo2_mask_count++ > UPDATE_MASK_AIO_TIME_S)
        {
            g_spo2_mask_count = UPDATE_MASK_AIO_TIME_S + 1;
        }
//        if (g_is_aio_spo2_error)
//            AIO_printf("\r\ng_is_aio_spo2_error");
//        if (g_is_aio_stm_error)
//            AIO_printf("\r\ng_is_aio_stm_error");
        ulNextChangeTime += getTickNumberByMS(1000);
    }
}

void SpO2_Module_Init(void)
{
    u32SpO2RxPopIndex = 0;
    u32SpO2RxPushIndex = 0;
    uiSpO2TxIdxStart = 0;
    uiSpO2TxIdxEnd = 0;
    bSpO2IsOverBufLenght = false;
    g_spo2_mask_count = UPDATE_MASK_AIO_TIME_S +1;
    g_IsSpO2_Upload = true;
    g_is_aio_spo2_error = 0;
    g_spo2_test_tick = SysTick_Get();

    UART0_Init();
    Init_UART0_Interrupts();
}

void SpO2_Module_Handler(void)
{
    //SpO2_RxPacketCheck();
    SpO2_RxHandler();
    SpO2_ResetMaskAIOTick();
}

