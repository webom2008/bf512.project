/*
 * uart_protocol.c
 *  
 *  Created on: 2013-10-10
 *      Author: QiuWeibo
 *
 *  Note:
 */
#include <string.h>

#include "uart_protocol.h"


//#define UART_PROTOCOL_DEBUG_INFO

#ifdef UART_PROTOCOL_DEBUG_INFO
#define PROTOCOL_DBG_INFO(fmt, arg...) AIO_printf("\r\n[PROTOCOL] "fmt, ##arg)
#else
#define PROTOCOL_DBG_INFO(fmt, arg...)
#endif

/* Private types -------------------------------------------------------------*/
#define TRY_MAX_COUNT   500

void onMessageHandler(const UartProtocolPacket packet);
void AIO_to_SpO2_UART(UartProtocolPacket *pPacket);
void UploadDataByID(const UART_PacketID id,
                    const TRUE_OR_FALSE IsPassData,
                    char* pData,
                    const u8 lenght);

static void InitUartPacketStruct(UartProtocolPacket *p_Packet)
{
    memset(p_Packet, 0, sizeof(UartProtocolPacket));

    p_Packet->DR_Addr = UART_MCU_ADDR;
    p_Packet->SR_Addr = UART_AIO_ADDR;
}

unsigned char crc8(unsigned char *ptr,unsigned char len)
{
	unsigned char crc;
	unsigned char i;
	crc = 0;
	while(len--)
	{
		crc ^= *ptr++;
		for(i = 0; i < 8; i++)
		{
			if(crc&0x01)
			{
				crc = (crc >> 1) ^ 0x8C;
			}	
			else
			{
				crc >>= 1;
			}
		}	
	}
	return crc;
}

u8 CalculatePacketCRC8(UartProtocolPacket *p_Packet)
{
    u8 len = 3 + (p_Packet->Length);//3(PacketNum,PacketID,Length)+Data length
    u8 crc = crc8(&p_Packet->PacketNum, len);
    return crc;
}

void Uart_RxPacketCheck(void)
{
    UartProtocolPacket packet;
    u8 crc = 0;
    u8 i = 0;
    u16 tryTime = 0;
    
    memset(&packet, 0, sizeof(UartProtocolPacket));

    if (RETURN_OK == AIO_UART_GetByte((char *)&packet.DR_Addr))
    {
        if (UART_AIO_ADDR == packet.DR_Addr) //get the head of one packet
        {
            for (tryTime=0; (RETURN_ERROR== AIO_UART_GetByte((char *)&packet.SR_Addr)) && (tryTime < TRY_MAX_COUNT); tryTime++);
            if (UART_MCU_ADDR == packet.SR_Addr)
            {
                for (tryTime=0; (RETURN_ERROR== AIO_UART_GetByte((char *)&packet.PacketNum)) && (tryTime < TRY_MAX_COUNT); tryTime++);
                for (tryTime=0; (RETURN_ERROR== AIO_UART_GetByte((char *)&packet.PacketID)) && (tryTime < TRY_MAX_COUNT); tryTime++);
                for (tryTime=0; (RETURN_ERROR== AIO_UART_GetByte((char *)&packet.Length)) && (tryTime < TRY_MAX_COUNT); tryTime++);

                for (i=0; i <= packet.Length; i++)  //get data and crc value
                {
                    for (tryTime=0; (RETURN_ERROR== AIO_UART_GetByte((char *)&packet.DataAndCRC[i])) && (tryTime < TRY_MAX_COUNT); tryTime++);
                }

                //check CRC vlaue
                crc = CalculatePacketCRC8(&packet);
                if (packet.DataAndCRC[packet.Length] == crc)
                {
                    onMessageHandler(packet);
                    PROTOCOL_DBG_INFO("I have get one packet!");
                }
                else
                {
                    PROTOCOL_DBG_INFO("I have get one wrong CRC! read crc=0x%x cal crc=0x%x",
                                        packet.DataAndCRC[packet.Length], crc);
                }
            } /*End of if (0x55 == packet.SR_Addr)*/
        } /*End of if (0xCC == packet.DR_Addr)*/
    } /* End of BUFFER_NORMAL */
}
    
static void onMessageHandler(UartProtocolPacket packet)
{
    UART_PacketID id = (UART_PacketID)packet.PacketID;
    //<!----------- ECG Start ------------>
    //根据ID处理相关事件，再返回结果
    if (AIO_RX_PROBE_MODE_ID == id)
    {
        if (packet.DataAndCRC[0]) //read
        {
            //respond the current work mode
            ECG_RespondProbeMode();
        }
        else //write
        {
            ECG_SetProbeMode(&packet);
        }
    }
    
    else if (AIO_RX_ECG12_CHANNEL_ID == id)
    {
        ECG_SetECG12Channel(&packet);
    }
    
    else if (AIO_RX_ECG_CAL_MODE_ID == id)
    {
        u8 data = packet.DataAndCRC[0];
        if (data)//enter cal mode
        {
            ECG_CAL_DRV_ModeSet(ENABLE);
        }
        else//exit cal mode
        {
            ECG_CAL_DRV_ModeSet(DISABLE);
        }
    }
    
    else if (AIO_RX_ECG_ST_SW_ID == id)
    {
        ECG_SetSTAnalyzeSW(&packet);
    }

    else if (AIO_RX_ECG_ST_MEASURE_ID == id)
    {
        ECG_SetSTMesureSetting(&packet);
    }
    
    else if (AIO_RX_ECG_ARRHYTHMIA_SW_ID == id)
    {
        ECG_SetArrhythmiaSW(&packet);
    }
    
    else if (AIO_RX_ECG_NOTCH_SW_ID == id)
    {
        ECG_SetNotchFilterSW(&packet);
    }
    
    else if (AIO_RX_ECG_Alarm_ID == id)
    {
        ECG_setAlarm(&packet);
    }
    
    else if (AIO_RX_ECG_PACE_SW_ID == id)
    {
        ECG_SetPaceSW(&packet);
    }
    
    else if (AIO_RX_ECG_PACE_CHANNEL_ID == id)
    {
        ECG_SetPaceChannel(&packet);
    }
    
    else if (AIO_RX_ECG_PACE_OVERSHOOT_ID == id)
    {
        ECG_SetPaceOvershootSw(&packet);
    }
    
    else if (AIO_RX_ECG_Debug_ID == id)
    {
        ECG_DebugInterface(&packet);
    }
    //<!----------- ECG End ------------>
    
    //<!----------- RESP Start ------------>
    else if (AIO_RX_RESP_UPLOAD_TYPE_ID == id)
    {
        RESP_setUploadDataType(&packet);
    }
    
    else if (AIO_RX_RESP_THRESHOLD_ID == id)
    {
        RESP_setThreshold(&packet);
    }
    
    else if (AIO_RX_RESP_CHANNEL_SEL_ID == id)
    {
        u8 data = packet.DataAndCRC[0];
        RESP_SetChannel(data);
    }
    
    else if (AIO_RX_RESP_CARRIER_SW_ID == id)
    {
        RESP_setCarrierWave(&packet);
    }
    
    else if (AIO_RX_RESP_ASPHYXIA_TIME_ID == id)
    {
        RESP_setAsphyxiaTime(&packet);
    }

    else if (AIO_RX_RESP_Debug_ID == id)
    {
        RESP_DebugInterface(&packet);
    }
    //<!----------- RESP End ------------>
    
    //<!----------- NIBP Start ------------>
    else if (AIO_RX_NIBP_START_ID == id)
    {
        u8 data = packet.DataAndCRC[0];
        if ((data & 0xF0) == 0xF0)
        {
            NIBP_SampleOneTime();
        }
    }
    
    else if (AIO_RX_NIBP_STOP_ID == id)
    {
        u8 data = packet.DataAndCRC[0];
        if ((data & 0x0F) == 0x0F)
        {
            NIBP_ForceStop();
        }
    }
    
    else if (AIO_RX_NIBP_CYCLE_ID == id)
    {
        u8 data = packet.DataAndCRC[0];
        NIBP_SetCycle(data);
    }
    else if (AIO_RX_NIBP_DUTY_CYCLE_ID == id)
    {
        u16 data = (packet.DataAndCRC[0] << 8) + packet.DataAndCRC[1];
        NIBP_SetPWM_DutyCycles(data);
    }
    else if (AIO_RX_NIBP_VERIFY_ID == id)
    {
        u8 data = packet.DataAndCRC[0];
        NIBP_VerifyBySimulator(data);
    }
    else if (AIO_RX_NIBP_VERIFYING_ID == id)
    {
        u8 data = packet.DataAndCRC[0];
        NIBP_VerifyAction(data);
    }
    else if (AIO_RX_NIBP_STM32_PRESS_ID == id)
    {
        NIBP_STM32VerifyAction(&packet.DataAndCRC[0], 3);
    }
    else if (AIO_RX_NIBP_PREPROCESS_PRESS_ID == id)
    {
        u16 data = (packet.DataAndCRC[0] << 8) + packet.DataAndCRC[1];
        NIBP_SetPrepressure(data);
    }
    else if (AIO_RX_NIBP_RESET_ID == id)
    {
        u8 data = packet.DataAndCRC[0];
        if ((data & 0xF0) == 0xF0)
        {
            NIBP_resetModule();
        }
    }
    else if (AIO_RX_NIBP_GET_RESULT_ID == id)
    {
        NIBP_getResult(&packet);
    }
    
    else if (AIO_RX_NIBP_VENIPUNCTURE_ID == id)
    {
        NIBP_setVenipuncture(&packet);
    }
    
    else if (AIO_RX_NIBP_CONTINUED_ID == id)
    {
        NIBP_setContinueMode(&packet);
    }
    
    else if (AIO_RX_NIBP_GAS_LEAK_ID == id)
    {
        NIBP_setGasLeakTest(&packet);
    }

    else if (AIO_RX_NIBP_Debug_ID == id)
    {
        //血压模块调试接口
        NIBP_DebugInterface(&packet);
    }
    //<!----------- NIBP Stop ------------>

    //<!----------- TEMP Start ------------>
    else if (AIO_RX_TEMP_Debug_ID == id)
    {
        TEMP_DebugInterface(&packet);
    }
    //<!----------- TEMP Stop ------------>

    /*----------- common Start ------------*/
    else if (COM_SOFTWARE_VERSION_ID == id)
    {
        COM_GetSoftVersion();
    }
    else if (COM_SELF_CHECK_ID == id)
    {
        COM_GetSelfCheck();
    }
    else if (COM_PATIENT_TYPE_ID == id)
    {
        COM_PatientTypeAction(&packet);
    }
    else if (COM_PM_WORK_MODE_ID == id)
    {
        COM_WorkModeAction(&packet);
    }
    /*----------- common Stop ------------*/

    /*----------- update Start ------------*/
    else if (SF_SPO2_UPDATE == id)
    {
        g_spo2_mask_count = 0;
        ExtWatchDog_Reset();
        AIO_to_SpO2_UART(&packet);
    }
    else if (SF_AIO_STM_UPDATE == id)
    {
        g_stm32_mask_count = 0;
        ExtWatchDog_Reset();
        aio_stm32_UpdateHandle(&packet);
    }
    else if (SF_AIO_DSP_UPDATE == id)
    {
        system_sf_reset(&packet);
    }
    /*----------- update Stop ------------*/

    //处理血氧模块的指令
    else if ((SpO2_RX_MODEL_VERSION_ID == id) \
                || (SpO2_RX_SELF_CHECK_ID == id) \
                || (SpO2_RX_PATIENT_SPEED_ID == id) \
                || (SpO2_RX_WORK_MODE_ID == id) \
                || (SpO2_RX_SOFTWARE_UPDATE_ID == id) \
                || (SpO2_RX_MODEL_LOWPOWER_ID == id))
    {
        AIO_to_SpO2_UART(&packet);
    }
}

static void AIO_to_SpO2_UART(UartProtocolPacket *pPacket)
{
    //只修改目的地址，不用改变其他数据。CRC也不用重新校验。
    pPacket->DR_Addr = UART_SpO2_ADDR;
    pPacket->SR_Addr = UART_AIO_ADDR;
    SpO2_UART_SendBuf((char *)pPacket, pPacket->Length + PACKET_FIXED_LENGHT);
}

void UploadDataByID(const UART_PacketID id, const TRUE_OR_FALSE IsPassData, char* pData, const u8 lenght)
{
    UartProtocolPacket packet;
    static u8 s_packetNum = 0;

    InitUartPacketStruct(&packet);
    packet.PacketID = id;

    if (TRUE == IsPassData)
    {
        packet.Length = lenght;
        memcpy(packet.DataAndCRC, pData, lenght);
    }
    packet.PacketNum = s_packetNum;
    packet.DataAndCRC[packet.Length] = CalculatePacketCRC8(&packet);
    
#ifdef _START_UPLOAD_DATA_
    AIO_UART_SendBuf((char *)&packet, packet.Length + PACKET_FIXED_LENGHT);
    s_packetNum++;
#endif    
}


